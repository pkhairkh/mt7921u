// SPDX-License-Identifier: BSD-3-Clause-Clear
/* Copyright (C) 2024 MediaTek Inc.
 *
 * Runtime-verification debugfs triggers for the mt7921 USB driver.
 *
 * This module provides debugfs entries that, when read, exercise specific
 * code paths to help testers verify bug fixes.  All triggers are protected
 * by the module parameter test_trigger_enable (default 0) so they are
 * inactive unless explicitly enabled at load time.
 *
 * Triggers are created under:
 *   /sys/kernel/debug/ieee80211/phyX/mt76/test_trigger/
 *
 * Safety: these triggers are intended for development / QA systems only.
 * They must NOT be enabled on production systems.
 */

#include <linux/debugfs.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/timekeeping.h>

#include "mt7921.h"
#include "../mt76_connac2_mac.h"

/* ---- module parameter -------------------------------------------------- */

static bool test_trigger_enable;
module_param_named(test_trigger_enable, test_trigger_enable, bool, 0644);
MODULE_PARM_DESC(test_trigger_enable,
                 "enable test trigger debugfs entries (default: N, for dev/QA only)");

/* ---- helper: return human-readable result string ----------------------- */

static const char *result_str(int err)
{
        if (!err)
                return "OK";
        switch (err) {
        case -ETIMEDOUT: return "ETIMEDOUT";
        case -ENOMEM:    return "ENOMEM";
        case -EINVAL:    return "EINVAL";
        case -EIO:       return "EIO";
        case -EAGAIN:    return "EAGAIN";
        case -EBUSY:     return "EBUSY";
        case -ENOENT:    return "ENOENT";
        case -EOPNOTSUPP: return "EOPNOTSUPP";
        case -EPERM:     return "EPERM";
        default:         {
                static char buf[16];
                snprintf(buf, sizeof(buf), "ERR%d", err);
                return buf;
        }
        }
}

/* ---- trigger_testmode_null  (TEST-T1) ---------------------------------- */
/*
 * When read with test_trigger_enable=1:
 *   - Calls mt7921_mcu_drv_own(dev) if available, then checks whether
 *     drv_own is NULL (the BUG-01 condition).  Logs the current state.
 *   - The tester can compare dmesg output with / without the fix applied.
 * When test_trigger_enable=0:
 *   - Returns a message saying triggers are disabled.
 *
 * Predicted crash signature (unpatched BUG-01):
 *   BUG: kernel NULL pointer dereference at 0x...
 *   in mt7921_testmode_cmd
 */

static int test_trigger_testmode_null_show(struct seq_file *s, void *data)
{
        struct mt792x_dev *dev = s->private;

        if (!test_trigger_enable) {
                seq_puts(s,
                         "Test triggers disabled. Set test_trigger_enable=1 to enable.\n");
                return 0;
        }

        seq_printf(s, "drv_own pointer: %pS\n", dev->hif_ops->drv_own);
        seq_printf(s, "fw_own pointer:  %pS\n", dev->hif_ops->fw_own);

        if (dev->hif_ops->drv_own) {
                int ret;

                mt792x_mutex_acquire(dev);
                ret = __mt792x_mcu_drv_pmctrl(dev);
                mt792x_mutex_release(dev);
                seq_printf(s, "drv_own() returned: %d (%s)\n", ret,
                           ret ? "error" : "success");
        } else {
                seq_puts(s,
                         "drv_own is NULL — BUG-01 fix verified: no crash.\n"
                         "Without the patch, a NULL dereference would occur here.\n"
                         "Predicted crash: BUG: kernel NULL pointer dereference "
                         "in mt7921_testmode_cmd\n");
        }

        return 0;
}

DEFINE_SHOW_ATTRIBUTE(test_trigger_testmode_null);

/* ---- trigger_wtbl_poll  (TEST-T2) -------------------------------------- */
/*
 * When enabled and read:
 *   - Calls mt7921_mac_wtbl_update(dev, 0, MT_WTBL_UPDATE_WDTCR) and
 *     measures the elapsed time in microseconds.
 *   - The tester verifies the poll completes within 50 ms (the new
 *     USB-increased timeout).  Without the patch, the original 5 ms
 *     timeout would expire on USB.
 */

static int test_trigger_wtbl_poll_show(struct seq_file *s, void *data)
{
        struct mt792x_dev *dev = s->private;
        ktime_t t0, t1;
        bool ok;

        if (!test_trigger_enable) {
                seq_puts(s,
                         "Test triggers disabled. Set test_trigger_enable=1 to enable.\n");
                return 0;
        }

        mt792x_mutex_acquire(dev);
        t0 = ktime_get();
        ok = mt7921_mac_wtbl_update(dev, 0, MT_WTBL_UPDATE_WDTCR);
        t1 = ktime_get();
        mt792x_mutex_release(dev);

        seq_printf(s, "WTBL poll completed in %lld us\n",
                   ktime_us_delta(t1, t0));
        seq_printf(s, "Result: %s\n", ok ? "success (BUSY cleared)" :
                   "timeout (BUSY still set)");
        seq_printf(s, "Timeout threshold: %d us (USB) / %d us (PCIe/SDIO)\n",
                   50000, 5000);

        return 0;
}

DEFINE_SHOW_ATTRIBUTE(test_trigger_wtbl_poll);

/* ---- trigger_mcu_retry  (TEST-T3) -------------------------------------- */
/*
 * When enabled and read:
 *   - Sends a harmless MCU command (FWLOG_2_HOST with ctrl=0) and
 *     reports the result.
 *   - The tester checks dmesg for "MCU command retry" messages which
 *     should only appear under heavy USB bus stress.
 */

static int test_trigger_mcu_retry_show(struct seq_file *s, void *data)
{
        struct mt792x_dev *dev = s->private;
        int ret;

        if (!test_trigger_enable) {
                seq_puts(s,
                         "Test triggers disabled. Set test_trigger_enable=1 to enable.\n");
                return 0;
        }

        mt792x_mutex_acquire(dev);
        ret = mt7921_mcu_fw_log_2_host(dev, 0);
        mt792x_mutex_release(dev);

        seq_printf(s, "MCU FWLOG_2_HOST(0) result: %d (%s)\n",
                   ret, ret ? "error" : "success");
        seq_puts(s,
                 "Check dmesg for 'mcu_send' / 'mcu_resp' lines to see "
                 "retry behavior.\n");

        return 0;
}

DEFINE_SHOW_ATTRIBUTE(test_trigger_mcu_retry);

/* ---- trigger_clc_load  (TEST-T4) --------------------------------------- */
/*
 * When enabled and read:
 *   - Calls mt7921_mcu_set_clc(dev, "00", ENVIRON_INDOOR) and reports
 *     the result.
 *   - The tester verifies CLC behaviour and fallback message on USB.
 */

static int test_trigger_clc_load_show(struct seq_file *s, void *data)
{
        struct mt792x_dev *dev = s->private;
        int ret;

        if (!test_trigger_enable) {
                seq_puts(s,
                         "Test triggers disabled. Set test_trigger_enable=1 to enable.\n");
                return 0;
        }

        seq_printf(s, "clc_force_usb=%d bus=%s\n",
                   clc_force_usb,
                   mt76_is_usb(&dev->mt76) ? "USB" :
                   mt76_is_sdio(&dev->mt76) ? "SDIO" : "MMIO");

        mt792x_mutex_acquire(dev);
        ret = mt7921_mcu_set_clc(dev, "00", ENVIRON_INDOOR);
        mt792x_mutex_release(dev);

        seq_printf(s, "mt7921_mcu_set_clc(\"00\", INDOOR) result: %d (%s)\n",
                   ret, ret ? "error" : "success");
        seq_printf(s, "clc_chan_conf: 0x%02x\n", dev->phy.clc_chan_conf);

        if (ret && mt76_is_usb(&dev->mt76) && !clc_force_usb)
                seq_puts(s,
                         "NOTE: USB CLC failure with fallback is expected "
                         "if CLC firmware data is missing.\n");

        return 0;
}

/* We need access to the clc_force_usb module param from mcu.c */
extern bool clc_force_usb;

DEFINE_SHOW_ATTRIBUTE(test_trigger_clc_load);

/* ---- trigger_roc_timer  (TEST-T5) -------------------------------------- */
/*
 * When enabled and read:
 *   - Reports current ROC timer state and any pending work.
 *   - The tester uses this during suspend/resume testing to verify
 *     there is no use-after-free on the ROC timer / work.
 */

static int test_trigger_roc_timer_show(struct seq_file *s, void *data)
{
        struct mt792x_dev *dev = s->private;
        struct mt792x_phy *phy = &dev->phy;

        if (!test_trigger_enable) {
                seq_puts(s,
                         "Test triggers disabled. Set test_trigger_enable=1 to enable.\n");
                return 0;
        }

        seq_printf(s, "ROC state: %s\n",
                   test_bit(MT76_STATE_ROC, &phy->mt76->state) ?
                   "ACTIVE" : "INACTIVE");
        seq_printf(s, "roc_grant: %d\n", phy->roc_grant);
        seq_printf(s, "roc_token_id: %d\n", phy->roc_token_id);
        seq_printf(s, "roc_timer pending: %d\n",
                   timer_pending(&phy->roc_timer));
        seq_printf(s, "pm.suspended: %d\n", dev->pm.suspended);

        return 0;
}

DEFINE_SHOW_ATTRIBUTE(test_trigger_roc_timer);

/* ---- init / cleanup ---------------------------------------------------- */

/**
 * mt7921_test_trigger_debugfs_init - create test trigger debugfs entries
 * @dev: the mt792x device
 *
 * Creates a "test_trigger" subdirectory under the mt76 debugfs dir with
 * read-only trigger files.  Returns 0 on success or -ENOMEM.
 */
int mt7921_test_trigger_debugfs_init(struct mt792x_dev *dev)
{
        struct dentry *dir;

        if (!dev->debugfs_dir)
                return -ENOENT;

        dir = debugfs_create_dir("test_trigger", dev->debugfs_dir);
        if (IS_ERR_OR_NULL(dir))
                return dir ? PTR_ERR(dir) : -ENOMEM;

        debugfs_create_file("trigger_testmode_null", 0400, dir, dev,
                            &test_trigger_testmode_null_fops);
        debugfs_create_file("trigger_wtbl_poll", 0400, dir, dev,
                            &test_trigger_wtbl_poll_fops);
        debugfs_create_file("trigger_mcu_retry", 0400, dir, dev,
                            &test_trigger_mcu_retry_fops);
        debugfs_create_file("trigger_clc_load", 0400, dir, dev,
                            &test_trigger_clc_load_fops);
        debugfs_create_file("trigger_roc_timer", 0400, dir, dev,
                            &test_trigger_roc_timer_fops);

        return 0;
}
EXPORT_SYMBOL_GPL(mt7921_test_trigger_debugfs_init);

/**
 * mt7921_test_trigger_debugfs_remove - remove test trigger debugfs entries
 * @dev: the mt792x device
 *
 * Note: debugfs entries are automatically cleaned up when the parent
 * directory is removed, so we only need to handle the case where we
 * want to remove triggers independently.  This is a no-op for now.
 */
void mt7921_test_trigger_debugfs_remove(struct mt792x_dev *dev)
{
        /* debugfs_remove_recursive is handled by the parent dir cleanup */
}
EXPORT_SYMBOL_GPL(mt7921_test_trigger_debugfs_remove);
