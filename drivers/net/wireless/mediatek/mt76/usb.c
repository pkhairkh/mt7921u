// SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * Copyright (C) 2018 Lorenzo Bianconi <lorenzo.bianconi83@gmail.com>
 */

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/jiffies.h>
#include <linux/atomic.h>
#include <linux/hrtimer.h>
#include "mt76.h"
#include "usb_trace.h"
#include "dma.h"

#define MT_VEND_REQ_MAX_RETRY   10
#define MT_VEND_REQ_TOUT_MS     300

static bool disable_usb_sg;
module_param_named(disable_usb_sg, disable_usb_sg, bool, 0644);
MODULE_PARM_DESC(disable_usb_sg, "Disable usb scatter-gather support");

/* TASK-017: Multi-Endpoint TX QoS — force the number of OUT endpoints
 * used for TX queue mapping. 0 = auto-detect from USB descriptor.
 * Values 2-6 override the detected count, enabling testing of different
 * endpoint configurations. The 6-endpoint mapping gives each AC its own
 * USB bulk OUT pipe, preventing AC_BE from blocking AC_VO.
 */
static int force_num_out_eps;
module_param_named(force_num_out_eps, force_num_out_eps, int, 0644);
MODULE_PARM_DESC(force_num_out_eps,
                 "Force number of USB OUT endpoints for TX QoS (0=auto, 2-6)");

/* sl0027: correct the AC->OUT-endpoint table. The legacy ep_map_6 assumed
 * the old mac80211 AC order (BE=0,BK=1,VI=2,VO=3); this kernel uses
 * VO=0,VI=1,BE=2,BK=3, so legacy put AC_BE frames on the fw's AC_BK pipe
 * (out_ep[2]) and vice versa. AP-mode data on the BK pipe = the fw's
 * AP-BSS silent-death/wedge path (0026a evidence). sl_ep_fix selects the
 * corrected table indexed by the real AC enum, using the out_ep indices
 * from enum mt76u_out_ep so the intent is self-documenting.
 */
static bool sl_ep_fix = true;
module_param_named(sl_ep_fix, sl_ep_fix, bool, 0644);
MODULE_PARM_DESC(sl_ep_fix, "Use corrected AC->OUT endpoint map (default: on)");

/* sl0030: AC TX coalescing hold, in microseconds. When nonzero, AC data
 * URB submissions are deferred for this long so frames arriving inside
 * the window accumulate in the USB queue and the firmware can build
 * real A-MPDUs instead of transmitting one PPDU per frame. Task-45
 * measurement on the unpatched path: 95% of PPDUs carried only 1-2
 * MPDUs (even during a 1k pps flood), downlink cost 3.3x the airtime of
 * uplink for identical traffic, weak-link clients woken per frame.
 * 0 restores the legacy immediate-submit behavior. The PSD/mc pipe is
 * never held (mcast/DTIM/PS-response latency must stay immediate).
 * Tunable at /sys/module/mt76_usb/parameters/sl30_tx_hold_us.
 */
static unsigned int sl30_tx_hold_us = 2000;
module_param_named(sl30_tx_hold_us, sl30_tx_hold_us, uint, 0644);
MODULE_PARM_DESC(sl30_tx_hold_us, "AC TX coalescing hold in us (0=off, default 2000)");

/* sl0026a: TX endpoint lifecycle instrumentation (READ-ONLY diagnostics).
 * Per-queue URB submit/complete counters, last-activity timestamps and a
 * bounded TXWI hex dump for BE frames. Purpose: pin down the AP-mode
 * bulk-OUT endpoint freeze - which queue's URBs stop completing, whether
 * other endpoints keep completing, and the exact timeline vs TXS events.
 * Zero behavioral changes. Counters indexed by mt76 txq id (0..PSD).
 */
static atomic_t sl26_sub[2][__MT_TXQ_MAX];
static atomic_t sl26_cmp[2][__MT_TXQ_MAX];
static atomic_t sl26_enospc[2][__MT_TXQ_MAX];
static int sl27_txwi_budget[2][__MT_TXQ_MAX];
static unsigned long sl26_last_sub[2][__MT_TXQ_MAX];
static unsigned long sl26_last_cmp[2][__MT_TXQ_MAX];
static int sl26_q_ep[2][__MT_TXQ_MAX];
static struct mt76_queue *sl26_q[2][__MT_TXQ_MAX];
static struct mt76_dev *sl26_devs[2];
static int sl26_nreg;

static int sl26_slot_of(struct mt76_dev *dev)
{
        int i;

        for (i = 0; i < sl26_nreg; i++)
                if (sl26_devs[i] == dev)
                        return i;
        if (sl26_nreg < 2)
                sl26_devs[sl26_nreg++] = dev;
        return sl26_nreg - 1;
}

static int sl26_qid_of(struct mt76_dev *dev, struct mt76_queue *q)
{
        int s = sl26_slot_of(dev);
        int i;

        for (i = 0; i < __MT_TXQ_MAX; i++)
                if (sl26_q[s][i] == q)
                        return i;
        return -1;
}

void sl0026a_tx_dump(struct mt76_dev *dev)
{
        int s = sl26_slot_of(dev);
        int i;

        if (!mt76_is_usb(dev))
                return;

        for (i = 0; i <= MT_TXQ_PSD; i++) {
                if (!sl26_q[s][i])
                        continue;
                dev_info(dev->dev,
                         "sl0026a q%d ep%d: sub=%d cmp=%d queued=%d h=%d t=%d enospc=%d subago=%ldms cmpago=%ldms\n",
                         i, sl26_q_ep[s][i],
                         atomic_read(&sl26_sub[s][i]),
                         atomic_read(&sl26_cmp[s][i]),
                         sl26_q[s][i]->queued, sl26_q[s][i]->head,
                         sl26_q[s][i]->tail,
                         atomic_read(&sl26_enospc[s][i]),
                         sl26_last_sub[s][i] ?
                         (long)jiffies_to_msecs(jiffies - sl26_last_sub[s][i]) : -1L,
                         sl26_last_cmp[s][i] ?
                         (long)jiffies_to_msecs(jiffies - sl26_last_cmp[s][i]) : -1L);
        }
}
EXPORT_SYMBOL_GPL(sl0026a_tx_dump);


int __mt76u_vendor_request(struct mt76_dev *dev, u8 req, u8 req_type,
                           u16 val, u16 offset, void *buf, size_t len)
{
        struct usb_interface *uintf = to_usb_interface(dev->dev);
        struct usb_device *udev = interface_to_usbdev(uintf);
        unsigned int pipe;
        int i, ret;

        lockdep_assert_held(&dev->usb.usb_ctrl_mtx);

        pipe = (req_type & USB_DIR_IN) ? usb_rcvctrlpipe(udev, 0)
                                       : usb_sndctrlpipe(udev, 0);
        for (i = 0; i < MT_VEND_REQ_MAX_RETRY; i++) {
                if (test_bit(MT76_REMOVED, &dev->phy.state))
                        return -EIO;
                /* Bus-hung fail-fast (upstream 915672c5ae3): once the
                 * control path is declared hung, every vendor request
                 * returns instantly instead of burning a full retry
                 * cycle (~3 s, formerly ~0.5 s in the 0012 fast-fail
                 * mode, still REAL usb_control_msg traffic) on a dead
                 * endpoint. bus_hung is cleared only from probe() after
                 * the queued USB device reset has re-enumerated the
                 * dongle. */
                if (dev->usb.ctrl_timeout && atomic_read(&dev->bus_hung))
                        return -EIO;

                ret = usb_control_msg(udev, pipe, req, req_type, val,
                                      offset, buf, len, MT_VEND_REQ_TOUT_MS);
                if (ret == -ENODEV || ret == -EPROTO)
                        set_bit(MT76_REMOVED, &dev->phy.state);
                if (ret >= 0 || ret == -ENODEV || ret == -EPROTO)
                        return ret;
                usleep_range(5000, 10000);
        }

        dev_err(dev->dev, "vendor request req:%02x off:%04x failed:%d\n",
                req, offset, ret);

        if (dev->usb.ctrl_timeout) {
                atomic_set(&dev->bus_hung, true);
                dev_err(dev->dev,
                        "vendor request req:%02x off:%04x timed out, marking bus hung\n",
                        req, offset);
                dev->usb.ctrl_timeout(dev, ret);
        }

        return ret;
}
EXPORT_SYMBOL_GPL(__mt76u_vendor_request);

/* True while the USB control endpoint is hung (upstream 915672c5ae3
 * architecture). Survey, MIB and RX-watchdog work uses this to skip
 * register sweeps instead of monopolizing dev->mt76.mutex on a dead
 * chip. Cleared only by probe() after the queued USB device reset
 * re-enumerates the dongle - no more flapping on isolated successes. */
bool mt76u_ctl_wedged(struct mt76_dev *dev)
{
        return atomic_read(&dev->bus_hung);
}
EXPORT_SYMBOL_GPL(mt76u_ctl_wedged);

int mt76u_vendor_request(struct mt76_dev *dev, u8 req,
                         u8 req_type, u16 val, u16 offset,
                         void *buf, size_t len)
{
        int ret;

        mutex_lock(&dev->usb.usb_ctrl_mtx);
        ret = __mt76u_vendor_request(dev, req, req_type,
                                     val, offset, buf, len);
        trace_usb_reg_wr(dev, offset, val);
        mutex_unlock(&dev->usb.usb_ctrl_mtx);

        return ret;
}
EXPORT_SYMBOL_GPL(mt76u_vendor_request);

u32 ___mt76u_rr(struct mt76_dev *dev, u8 req, u8 req_type, u32 addr)
{
        struct mt76_usb *usb = &dev->usb;
        u32 data = ~0;
        int ret;

        ret = __mt76u_vendor_request(dev, req, req_type, addr >> 16,
                                     addr, usb->data, sizeof(__le32));
        if (ret == sizeof(__le32))
                data = get_unaligned_le32(usb->data);
        trace_usb_reg_rr(dev, addr, data);

        return data;
}
EXPORT_SYMBOL_GPL(___mt76u_rr);

static u32 __mt76u_rr(struct mt76_dev *dev, u32 addr)
{
        u8 req;

        switch (addr & MT_VEND_TYPE_MASK) {
        case MT_VEND_TYPE_EEPROM:
                req = MT_VEND_READ_EEPROM;
                break;
        case MT_VEND_TYPE_CFG:
                req = MT_VEND_READ_CFG;
                break;
        default:
                req = MT_VEND_MULTI_READ;
                break;
        }

        return ___mt76u_rr(dev, req, USB_DIR_IN | USB_TYPE_VENDOR,
                           addr & ~MT_VEND_TYPE_MASK);
}

static u32 mt76u_rr(struct mt76_dev *dev, u32 addr)
{
        u32 ret;

        mutex_lock(&dev->usb.usb_ctrl_mtx);
        ret = __mt76u_rr(dev, addr);
        mutex_unlock(&dev->usb.usb_ctrl_mtx);

        return ret;
}

void ___mt76u_wr(struct mt76_dev *dev, u8 req, u8 req_type,
                 u32 addr, u32 val)
{
        struct mt76_usb *usb = &dev->usb;

        put_unaligned_le32(val, usb->data);
        __mt76u_vendor_request(dev, req, req_type, addr >> 16,
                               addr, usb->data, sizeof(__le32));
        trace_usb_reg_wr(dev, addr, val);
}
EXPORT_SYMBOL_GPL(___mt76u_wr);

static void __mt76u_wr(struct mt76_dev *dev, u32 addr, u32 val)
{
        u8 req;

        switch (addr & MT_VEND_TYPE_MASK) {
        case MT_VEND_TYPE_CFG:
                req = MT_VEND_WRITE_CFG;
                break;
        default:
                req = MT_VEND_MULTI_WRITE;
                break;
        }
        ___mt76u_wr(dev, req, USB_DIR_OUT | USB_TYPE_VENDOR,
                    addr & ~MT_VEND_TYPE_MASK, val);
}

static void mt76u_wr(struct mt76_dev *dev, u32 addr, u32 val)
{
        mutex_lock(&dev->usb.usb_ctrl_mtx);
        __mt76u_wr(dev, addr, val);
        mutex_unlock(&dev->usb.usb_ctrl_mtx);
}

static u32 mt76u_rmw(struct mt76_dev *dev, u32 addr,
                     u32 mask, u32 val)
{
        mutex_lock(&dev->usb.usb_ctrl_mtx);
        val |= __mt76u_rr(dev, addr) & ~mask;
        __mt76u_wr(dev, addr, val);
        mutex_unlock(&dev->usb.usb_ctrl_mtx);

        return val;
}

static void mt76u_copy(struct mt76_dev *dev, u32 offset,
                       const void *data, int len)
{
        struct mt76_usb *usb = &dev->usb;
        const u8 *val = data;
        int ret;
        int current_batch_size;
        int i = 0;

        /* Assure that always a multiple of 4 bytes are copied,
         * otherwise beacons can be corrupted.
         * See: "mt76: round up length on mt76_wr_copy"
         * Commit 850e8f6fbd5d0003b0
         */
        len = round_up(len, 4);

        mutex_lock(&usb->usb_ctrl_mtx);
        while (i < len) {
                current_batch_size = min_t(int, usb->data_len, len - i);
                memcpy(usb->data, val + i, current_batch_size);
                ret = __mt76u_vendor_request(dev, MT_VEND_MULTI_WRITE,
                                             USB_DIR_OUT | USB_TYPE_VENDOR,
                                             0, offset + i, usb->data,
                                             current_batch_size);
                if (ret < 0)
                        break;

                i += current_batch_size;
        }
        mutex_unlock(&usb->usb_ctrl_mtx);
}

void mt76u_read_copy(struct mt76_dev *dev, u32 offset,
                     void *data, int len)
{
        struct mt76_usb *usb = &dev->usb;
        int i = 0, batch_len, ret;
        u8 *val = data;

        len = round_up(len, 4);
        mutex_lock(&usb->usb_ctrl_mtx);
        while (i < len) {
                batch_len = min_t(int, usb->data_len, len - i);
                ret = __mt76u_vendor_request(dev, MT_VEND_READ_EXT,
                                             USB_DIR_IN | USB_TYPE_VENDOR,
                                             (offset + i) >> 16, offset + i,
                                             usb->data, batch_len);
                if (ret < 0)
                        break;

                memcpy(val + i, usb->data, batch_len);
                i += batch_len;
        }
        mutex_unlock(&usb->usb_ctrl_mtx);
}
EXPORT_SYMBOL_GPL(mt76u_read_copy);

void mt76u_single_wr(struct mt76_dev *dev, const u8 req,
                     const u16 offset, const u32 val)
{
        mutex_lock(&dev->usb.usb_ctrl_mtx);
        __mt76u_vendor_request(dev, req,
                               USB_DIR_OUT | USB_TYPE_VENDOR,
                               val & 0xffff, offset, NULL, 0);
        __mt76u_vendor_request(dev, req,
                               USB_DIR_OUT | USB_TYPE_VENDOR,
                               val >> 16, offset + 2, NULL, 0);
        mutex_unlock(&dev->usb.usb_ctrl_mtx);
}
EXPORT_SYMBOL_GPL(mt76u_single_wr);

static int
mt76u_req_wr_rp(struct mt76_dev *dev, u32 base,
                const struct mt76_reg_pair *data, int len)
{
        struct mt76_usb *usb = &dev->usb;

        mutex_lock(&usb->usb_ctrl_mtx);
        while (len > 0) {
                __mt76u_wr(dev, base + data->reg, data->value);
                len--;
                data++;
        }
        mutex_unlock(&usb->usb_ctrl_mtx);

        return 0;
}

static int
mt76u_wr_rp(struct mt76_dev *dev, u32 base,
            const struct mt76_reg_pair *data, int n)
{
        if (test_bit(MT76_STATE_MCU_RUNNING, &dev->phy.state))
                return dev->mcu_ops->mcu_wr_rp(dev, base, data, n);
        else
                return mt76u_req_wr_rp(dev, base, data, n);
}

static int
mt76u_req_rd_rp(struct mt76_dev *dev, u32 base, struct mt76_reg_pair *data,
                int len)
{
        struct mt76_usb *usb = &dev->usb;

        mutex_lock(&usb->usb_ctrl_mtx);
        while (len > 0) {
                data->value = __mt76u_rr(dev, base + data->reg);
                len--;
                data++;
        }
        mutex_unlock(&usb->usb_ctrl_mtx);

        return 0;
}

static int
mt76u_rd_rp(struct mt76_dev *dev, u32 base,
            struct mt76_reg_pair *data, int n)
{
        if (test_bit(MT76_STATE_MCU_RUNNING, &dev->phy.state))
                return dev->mcu_ops->mcu_rd_rp(dev, base, data, n);
        else
                return mt76u_req_rd_rp(dev, base, data, n);
}

static bool mt76u_check_sg(struct mt76_dev *dev)
{
        struct usb_interface *uintf = to_usb_interface(dev->dev);
        struct usb_device *udev = interface_to_usbdev(uintf);

        return (!disable_usb_sg && udev->bus->sg_tablesize > 0 &&
                udev->bus->no_sg_constraint);
}

static int
mt76u_set_endpoints(struct usb_interface *intf,
                    struct mt76_usb *usb)
{
        struct usb_host_interface *intf_desc = intf->cur_altsetting;
        struct usb_endpoint_descriptor *ep_desc;
        int i, in_ep = 0, out_ep = 0;

        for (i = 0; i < intf_desc->desc.bNumEndpoints; i++) {
                ep_desc = &intf_desc->endpoint[i].desc;

                if (usb_endpoint_is_bulk_in(ep_desc) &&
                    in_ep < __MT_EP_IN_MAX) {
                        usb->in_ep[in_ep] = usb_endpoint_num(ep_desc);
                        in_ep++;
                } else if (usb_endpoint_is_bulk_out(ep_desc) &&
                           out_ep < __MT_EP_OUT_MAX) {
                        usb->out_ep[out_ep] = usb_endpoint_num(ep_desc);
                        out_ep++;
                }
        }

        usb->num_out_eps = out_ep;

        /* RUNTIME_VERIFY: verify endpoint count with lsusb -v */
        if (out_ep >= 6) {
                dev_dbg(&intf->dev,
                        "6 OUT endpoints detected: per-AC QoS enabled\n");
        } else {
                dev_dbg(&intf->dev,
                        "%d OUT endpoints detected: using shared QoS mapping\n",
                        out_ep);
        }

        if (in_ep != __MT_EP_IN_MAX || out_ep < 2)
                return -EINVAL;
        return 0;
}

static int
mt76u_fill_rx_sg(struct mt76_dev *dev, struct mt76_queue *q, struct urb *urb,
                 int nsgs)
{
        int i;

        for (i = 0; i < nsgs; i++) {
                void *data;
                int offset;

                data = mt76_get_page_pool_buf(q, &offset, q->buf_size);
                if (!data)
                        break;

                sg_set_page(&urb->sg[i], virt_to_head_page(data), q->buf_size,
                            offset);
        }

        if (i < nsgs) {
                int j;

                for (j = nsgs; j < urb->num_sgs; j++)
                        mt76_put_page_pool_buf(sg_virt(&urb->sg[j]), false);
                urb->num_sgs = i;
        }

        urb->num_sgs = max_t(int, i, urb->num_sgs);
        urb->transfer_buffer_length = urb->num_sgs * q->buf_size;
        sg_init_marker(urb->sg, urb->num_sgs);

        return i ? : -ENOMEM;
}

static int
mt76u_refill_rx(struct mt76_dev *dev, struct mt76_queue *q,
                struct urb *urb, int nsgs)
{
        enum mt76_rxq_id qid = q - &dev->q_rx[MT_RXQ_MAIN];
        int offset;

        if (qid == MT_RXQ_MAIN && dev->usb.sg_en)
                return mt76u_fill_rx_sg(dev, q, urb, nsgs);

        urb->transfer_buffer_length = q->buf_size;
        urb->transfer_buffer = mt76_get_page_pool_buf(q, &offset, q->buf_size);

        return urb->transfer_buffer ? 0 : -ENOMEM;
}

static int
mt76u_urb_alloc(struct mt76_dev *dev, struct mt76_queue_entry *e,
                int sg_max_size)
{
        unsigned int size = sizeof(struct urb);

        if (dev->usb.sg_en)
                size += sg_max_size * sizeof(struct scatterlist);

        e->urb = kzalloc(size, GFP_KERNEL);
        if (!e->urb)
                return -ENOMEM;

        usb_init_urb(e->urb);

        if (dev->usb.sg_en && sg_max_size > 0)
                e->urb->sg = (struct scatterlist *)(e->urb + 1);

        return 0;
}

static int
mt76u_rx_urb_alloc(struct mt76_dev *dev, struct mt76_queue *q,
                   struct mt76_queue_entry *e)
{
        enum mt76_rxq_id qid = q - &dev->q_rx[MT_RXQ_MAIN];
        int err, sg_size;

        sg_size = qid == MT_RXQ_MAIN ? MT_RX_SG_MAX_SIZE : 0;
        err = mt76u_urb_alloc(dev, e, sg_size);
        if (err)
                return err;

        return mt76u_refill_rx(dev, q, e->urb, sg_size);
}

static void mt76u_urb_free(struct urb *urb)
{
        int i;

        for (i = 0; i < urb->num_sgs; i++)
                mt76_put_page_pool_buf(sg_virt(&urb->sg[i]), false);

        if (urb->transfer_buffer)
                mt76_put_page_pool_buf(urb->transfer_buffer, false);

        usb_free_urb(urb);
}

static void
mt76u_fill_bulk_urb(struct mt76_dev *dev, int dir, int index,
                    struct urb *urb, usb_complete_t complete_fn,
                    void *context)
{
        struct usb_interface *uintf = to_usb_interface(dev->dev);
        struct usb_device *udev = interface_to_usbdev(uintf);
        unsigned int pipe;

        if (dir == USB_DIR_IN)
                pipe = usb_rcvbulkpipe(udev, dev->usb.in_ep[index]);
        else
                pipe = usb_sndbulkpipe(udev, dev->usb.out_ep[index]);

        urb->dev = udev;
        urb->pipe = pipe;
        urb->complete = complete_fn;
        urb->context = context;
}

static struct urb *
mt76u_get_next_rx_entry(struct mt76_queue *q)
{
        struct urb *urb = NULL;
        unsigned long flags;

        spin_lock_irqsave(&q->lock, flags);
        if (q->queued > 0) {
                urb = q->entry[q->tail].urb;
                q->tail = (q->tail + 1) % q->ndesc;
                q->queued--;
        }
        spin_unlock_irqrestore(&q->lock, flags);

        return urb;
}

static int
mt76u_get_rx_entry_len(struct mt76_dev *dev, u8 *data,
                       u32 data_len)
{
        u16 dma_len, min_len;

        dma_len = get_unaligned_le16(data);
        if (dev->drv->drv_flags & MT_DRV_RX_DMA_HDR)
                return dma_len;

        min_len = MT_DMA_HDR_LEN + MT_RX_RXWI_LEN + MT_FCE_INFO_LEN;
        if (data_len < min_len || !dma_len ||
            dma_len + MT_DMA_HDR_LEN > data_len ||
            (dma_len & 0x3))
                return -EINVAL;
        return dma_len;
}

static struct sk_buff *
mt76u_build_rx_skb(struct mt76_dev *dev, void *data,
                   int len, int buf_size)
{
        int head_room, drv_flags = dev->drv->drv_flags;
        struct sk_buff *skb;

        head_room = drv_flags & MT_DRV_RX_DMA_HDR ? 0 : MT_DMA_HDR_LEN;
        if (SKB_WITH_OVERHEAD(buf_size) < head_room + len) {
                struct page *page;

                /* slow path, not enough space for data and
                 * skb_shared_info
                 */
                skb = alloc_skb(MT_SKB_HEAD_LEN, GFP_ATOMIC);
                if (!skb)
                        return NULL;

                skb_put_data(skb, data + head_room, MT_SKB_HEAD_LEN);
                data += head_room + MT_SKB_HEAD_LEN;
                page = virt_to_head_page(data);
                skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags,
                                page, data - page_address(page),
                                len - MT_SKB_HEAD_LEN, buf_size);

                return skb;
        }

        /* fast path */
        skb = build_skb(data, buf_size);
        if (!skb)
                return NULL;

        skb_reserve(skb, head_room);
        __skb_put(skb, len);

        return skb;
}

static int
mt76u_process_rx_entry(struct mt76_dev *dev, struct urb *urb,
                       int buf_size)
{
        u8 *data = urb->num_sgs ? sg_virt(&urb->sg[0]) : urb->transfer_buffer;
        int data_len = urb->num_sgs ? urb->sg[0].length : urb->actual_length;
        int len, nsgs = 1, head_room, drv_flags = dev->drv->drv_flags;
        struct sk_buff *skb;

        if (!test_bit(MT76_STATE_INITIALIZED, &dev->phy.state))
                return 0;

        len = mt76u_get_rx_entry_len(dev, data, urb->actual_length);
        if (len < 0)
                return 0;

        head_room = drv_flags & MT_DRV_RX_DMA_HDR ? 0 : MT_DMA_HDR_LEN;
        data_len = min_t(int, len, data_len - head_room);

        if (len == data_len &&
            dev->drv->rx_check && !dev->drv->rx_check(dev, data, data_len))
                return 0;

        skb = mt76u_build_rx_skb(dev, data, data_len, buf_size);
        if (!skb)
                return 0;

        len -= data_len;
        while (len > 0 && nsgs < urb->num_sgs) {
                data_len = min_t(int, len, urb->sg[nsgs].length);
                skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags,
                                sg_page(&urb->sg[nsgs]),
                                urb->sg[nsgs].offset, data_len,
                                buf_size);
                len -= data_len;
                nsgs++;
        }

        skb_mark_for_recycle(skb);
        dev->drv->rx_skb(dev, MT_RXQ_MAIN, skb, NULL);

        return nsgs;
}

static void mt76u_complete_rx(struct urb *urb)
{
        struct mt76_dev *dev = dev_get_drvdata(&urb->dev->dev);
        struct mt76_queue *q = urb->context;
        unsigned long flags;

        trace_rx_urb(dev, urb);

        /* Count data arrivals for the mac_work data-path watchdog */
        atomic_inc(&dev->usb.rx_urb_completions);

        switch (urb->status) {
        case -ECONNRESET:
        case -ESHUTDOWN:
        case -ENOENT:
        case -EPROTO:
                return;
        default:
                dev_err_ratelimited(dev->dev, "rx urb failed: %d\n",
                                    urb->status);
                fallthrough;
        case 0:
                break;
        }

        spin_lock_irqsave(&q->lock, flags);
        if (WARN_ONCE(q->entry[q->head].urb != urb, "rx urb mismatch"))
                goto out;

        q->head = (q->head + 1) % q->ndesc;
        q->queued++;
        mt76_worker_schedule(&dev->usb.rx_worker);
out:
        spin_unlock_irqrestore(&q->lock, flags);
}

static int
mt76u_submit_rx_buf(struct mt76_dev *dev, enum mt76_rxq_id qid,
                    struct urb *urb)
{
        int ep = qid == MT_RXQ_MAIN ? MT_EP_IN_PKT_RX : MT_EP_IN_CMD_RESP;

        mt76u_fill_bulk_urb(dev, USB_DIR_IN, ep, urb,
                            mt76u_complete_rx, &dev->q_rx[qid]);
        trace_submit_urb(dev, urb);

        return usb_submit_urb(urb, GFP_ATOMIC);
}

static void
mt76u_process_rx_queue(struct mt76_dev *dev, struct mt76_queue *q)
{
        int qid = q - &dev->q_rx[MT_RXQ_MAIN];
        struct urb *urb;
        int err, count;

        while (true) {
                urb = mt76u_get_next_rx_entry(q);
                if (!urb)
                        break;

                count = mt76u_process_rx_entry(dev, urb, q->buf_size);
                if (count > 0) {
                        err = mt76u_refill_rx(dev, q, urb, count);
                        if (err < 0)
                                break;
                }
                mt76u_submit_rx_buf(dev, qid, urb);
        }
        if (qid == MT_RXQ_MAIN) {
                local_bh_disable();
                mt76_rx_poll_complete(dev, MT_RXQ_MAIN, NULL);
                local_bh_enable();
        }
}

static void mt76u_rx_worker(struct mt76_worker *w)
{
        struct mt76_usb *usb = container_of(w, struct mt76_usb, rx_worker);
        struct mt76_dev *dev = container_of(usb, struct mt76_dev, usb);
        int i;

        rcu_read_lock();
        mt76_for_each_q_rx(dev, i)
                mt76u_process_rx_queue(dev, &dev->q_rx[i]);
        rcu_read_unlock();
}

static int
mt76u_submit_rx_buffers(struct mt76_dev *dev, enum mt76_rxq_id qid)
{
        struct mt76_queue *q = &dev->q_rx[qid];
        unsigned long flags;
        int i, err = 0;

        spin_lock_irqsave(&q->lock, flags);
        for (i = 0; i < q->ndesc; i++) {
                err = mt76u_submit_rx_buf(dev, qid, q->entry[i].urb);
                if (err < 0)
                        break;
        }
        q->head = q->tail = 0;
        q->queued = 0;
        spin_unlock_irqrestore(&q->lock, flags);

        return err;
}

static int
mt76u_alloc_rx_queue(struct mt76_dev *dev, enum mt76_rxq_id qid)
{
        struct mt76_queue *q = &dev->q_rx[qid];
        int i, err;

        err = mt76_create_page_pool(dev, q);
        if (err)
                return err;

        spin_lock_init(&q->lock);
        q->entry = devm_kcalloc(dev->dev,
                                MT_NUM_RX_ENTRIES, sizeof(*q->entry),
                                GFP_KERNEL);
        if (!q->entry)
                return -ENOMEM;

        q->ndesc = MT_NUM_RX_ENTRIES;
        q->buf_size = PAGE_SIZE;

        for (i = 0; i < q->ndesc; i++) {
                err = mt76u_rx_urb_alloc(dev, q, &q->entry[i]);
                if (err < 0)
                        return err;
        }

        return mt76u_submit_rx_buffers(dev, qid);
}

int mt76u_alloc_mcu_queue(struct mt76_dev *dev)
{
        return mt76u_alloc_rx_queue(dev, MT_RXQ_MCU);
}
EXPORT_SYMBOL_GPL(mt76u_alloc_mcu_queue);

static void
mt76u_free_rx_queue(struct mt76_dev *dev, struct mt76_queue *q)
{
        int i;

        for (i = 0; i < q->ndesc; i++) {
                if (!q->entry[i].urb)
                        continue;

                mt76u_urb_free(q->entry[i].urb);
                q->entry[i].urb = NULL;
        }
        page_pool_destroy(q->page_pool);
        q->page_pool = NULL;
}

static void mt76u_free_rx(struct mt76_dev *dev)
{
        int i;

        mt76_worker_teardown(&dev->usb.rx_worker);

        mt76_for_each_q_rx(dev, i)
                mt76u_free_rx_queue(dev, &dev->q_rx[i]);
}

void mt76u_stop_rx(struct mt76_dev *dev)
{
        int i;

        mt76_worker_disable(&dev->usb.rx_worker);

        mt76_for_each_q_rx(dev, i) {
                struct mt76_queue *q = &dev->q_rx[i];
                int j;

                for (j = 0; j < q->ndesc; j++)
                        usb_poison_urb(q->entry[j].urb);
        }
}
EXPORT_SYMBOL_GPL(mt76u_stop_rx);

int mt76u_resume_rx(struct mt76_dev *dev)
{
        int i;

        mt76_for_each_q_rx(dev, i) {
                struct mt76_queue *q = &dev->q_rx[i];
                int err, j;

                for (j = 0; j < q->ndesc; j++)
                        usb_unpoison_urb(q->entry[j].urb);

                err = mt76u_submit_rx_buffers(dev, i);
                if (err < 0)
                        return err;
        }

        mt76_worker_enable(&dev->usb.rx_worker);

        return 0;
}
EXPORT_SYMBOL_GPL(mt76u_resume_rx);

static void mt76u_status_worker(struct mt76_worker *w)
{
        struct mt76_usb *usb = container_of(w, struct mt76_usb, status_worker);
        struct mt76_dev *dev = container_of(usb, struct mt76_dev, usb);
        struct mt76_queue_entry entry;
        struct mt76_queue *q;
        int sl26_s = sl26_slot_of(dev);
        int i;

        if (!test_bit(MT76_STATE_RUNNING, &dev->phy.state))
                return;

        for (i = 0; i <= MT_TXQ_PSD; i++) {
                q = dev->phy.q_tx[i];
                if (!q)
                        continue;

                while (q->queued > 0) {
                        if (!q->entry[q->tail].done)
                                break;

                        entry = q->entry[q->tail];
                        q->entry[q->tail].done = false;

                        mt76_queue_tx_complete(dev, q, &entry);
                        atomic_inc(&sl26_cmp[sl26_s][i]);
                        sl26_last_cmp[sl26_s][i] = jiffies;
                }

                if (!q->queued)
                        wake_up(&dev->tx_wait);

                mt76_worker_schedule(&dev->tx_worker);
        }

        if (dev->drv->tx_status_data &&
            !test_and_set_bit(MT76_READING_STATS, &dev->phy.state))
                queue_work(dev->wq, &dev->usb.stat_work);
}

static void mt76u_tx_status_data(struct work_struct *work)
{
        struct mt76_usb *usb;
        struct mt76_dev *dev;
        u8 update = 1;
        u16 count = 0;

        usb = container_of(work, struct mt76_usb, stat_work);
        dev = container_of(usb, struct mt76_dev, usb);

        while (true) {
                if (test_bit(MT76_REMOVED, &dev->phy.state))
                        break;

                if (!dev->drv->tx_status_data(dev, &update))
                        break;
                count++;
        }

        if (count && test_bit(MT76_STATE_RUNNING, &dev->phy.state))
                queue_work(dev->wq, &usb->stat_work);
        else
                clear_bit(MT76_READING_STATS, &dev->phy.state);
}

static void mt76u_complete_tx(struct urb *urb)
{
        struct mt76_dev *dev = dev_get_drvdata(&urb->dev->dev);
        struct mt76_queue_entry *e = urb->context;

        if (mt76u_urb_error(urb))
                dev_err(dev->dev, "tx urb failed: %d\n", urb->status);
        e->done = true;

        mt76_worker_schedule(&dev->usb.status_worker);
}

static int
mt76u_tx_setup_buffers(struct mt76_dev *dev, struct sk_buff *skb,
                       struct urb *urb)
{
        urb->transfer_buffer_length = skb->len;

        if (!dev->usb.sg_en) {
                urb->transfer_buffer = skb->data;
                return 0;
        }

        sg_init_table(urb->sg, MT_TX_SG_MAX_SIZE);
        urb->num_sgs = skb_to_sgvec(skb, urb->sg, 0, skb->len);
        if (!urb->num_sgs)
                return -ENOMEM;

        return urb->num_sgs;
}

static int
mt76u_tx_queue_skb(struct mt76_phy *phy, struct mt76_queue *q,
                   enum mt76_txq_id qid, struct sk_buff *skb,
                   struct mt76_wcid *wcid, struct ieee80211_sta *sta)
{
        struct mt76_tx_info tx_info = {
                .skb = skb,
        };
        struct mt76_dev *dev = phy->dev;
        u16 idx = q->head;
        int err;

        if (q->queued == q->ndesc) {
                int s = sl26_slot_of(dev);

                atomic_inc(&sl26_enospc[s][qid]);
                if (atomic_read(&sl26_enospc[s][qid]) <= 4)
                        sl0026a_tx_dump(dev);
                return -ENOSPC;
        }

        skb->prev = skb->next = NULL;
        err = dev->drv->tx_prepare_skb(dev, NULL, qid, wcid, sta, &tx_info);
        if (err < 0)
                return err;

        /* sl0027: bounded TXWI + 802.11 header dump from the skb itself
         * (the 0026a kick-path dump never fired: SG URBs have no
         * transfer_buffer). 16 frames per queue per device. */
        if (qid < __MT_TXQ_MAX && tx_info.skb->len >= 10) {
                int s = sl26_slot_of(dev);

                if (sl27_txwi_budget[s][qid] > 0) {
                        sl27_txwi_budget[s][qid]--;
                        dev_info(dev->dev, "sl0027-txwi q%d len=%u\n",
                                 qid, tx_info.skb->len);
                        print_hex_dump(KERN_INFO, "sl0027-txwi ",
                                       DUMP_PREFIX_OFFSET, 16, 1,
                                       tx_info.skb->data,
                                       min_t(u32, 48, tx_info.skb->len),
                                       false);
                }
        }

        err = mt76u_tx_setup_buffers(dev, tx_info.skb, q->entry[idx].urb);
        if (err < 0)
                return err;

        mt76u_fill_bulk_urb(dev, USB_DIR_OUT, q->ep, q->entry[idx].urb,
                            mt76u_complete_tx, &q->entry[idx]);

        q->head = (q->head + 1) % q->ndesc;
        q->entry[idx].skb = tx_info.skb;
        q->entry[idx].wcid = 0xffff;
        q->queued++;

        return idx;
}

static void mt76u_tx_submit(struct mt76_dev *dev, struct mt76_queue *q)
{
        struct urb *urb;
        int err;

        int sl26_qid = sl26_qid_of(dev, q);
        int sl26_s = (sl26_qid >= 0) ? sl26_slot_of(dev) : 0;

        while (q->first != q->head) {
                urb = q->entry[q->first].urb;

                trace_submit_urb(dev, urb);
                err = usb_submit_urb(urb, GFP_ATOMIC);
                if (err < 0) {
                        if (err == -ENODEV)
                                set_bit(MT76_REMOVED, &dev->phy.state);
                        else
                                dev_err(dev->dev, "tx urb submit failed:%d\n",
                                        err);
                        break;
                }
                if (sl26_qid >= 0) {
                        atomic_inc(&sl26_sub[sl26_s][sl26_qid]);
                        sl26_last_sub[sl26_s][sl26_qid] = jiffies;
                }

                q->first = (q->first + 1) % q->ndesc;
        }
}

/* sl0030: flush every AC queue. Safe in process and softirq context:
 * every q->lock holder on the TX path holds the lock with bottom
 * halves disabled, so a softirq-context taker cannot deadlock against
 * them on the local CPU.
 */
static void mt76u_tx_hold_flush_acs(struct mt76_dev *dev)
{
        int i;

        for (i = 0; i < IEEE80211_NUM_ACS; i++) {
                struct mt76_queue *q = dev->phy.q_tx[i];

                if (!q)
                        continue;

                spin_lock_bh(&q->lock);
                mt76u_tx_submit(dev, q);
                spin_unlock_bh(&q->lock);
        }
}

static enum hrtimer_restart mt76u_tx_hold_timer_fn(struct hrtimer *t)
{
        struct mt76_usb *usb = container_of(t, struct mt76_usb,
                                            tx_hold_timer);
        struct mt76_dev *dev = container_of(usb, struct mt76_dev, usb);

        atomic_set(&usb->tx_hold_armed, 0);
        mt76u_tx_hold_flush_acs(dev);

        return HRTIMER_NORESTART;
}

static bool mt76u_tx_hold_is_ac(struct mt76_dev *dev, struct mt76_queue *q)
{
        /* only the four AC data queues are held; the PSD/mc pipe
         * (mcast/DTIM, PS responses, offchannel) stays immediate */
        return q == dev->phy.q_tx[MT_TXQ_VO] ||
               q == dev->phy.q_tx[MT_TXQ_VI] ||
               q == dev->phy.q_tx[MT_TXQ_BE] ||
               q == dev->phy.q_tx[MT_TXQ_BK];
}

static void mt76u_tx_kick(struct mt76_dev *dev, struct mt76_queue *q)
{
        /* sl0030: instead of handing every queued URB to the firmware
         * immediately (which transmits each frame as its own PPDU and
         * starves aggregation below USB-saturation rates), arm the
         * coalescing timer for AC queues and let frames accumulate for
         * sl30_tx_hold_us before one batch submission.
         */
        if (sl30_tx_hold_us && mt76u_tx_hold_is_ac(dev, q)) {
                struct mt76_usb *usb = &dev->usb;

                if (atomic_cmpxchg(&usb->tx_hold_armed, 0, 1) == 0)
                        hrtimer_start(&usb->tx_hold_timer,
                                      ktime_set(0, (u64)sl30_tx_hold_us *
                                                   NSEC_PER_USEC),
                                      HRTIMER_MODE_REL_SOFT);
                return;
        }

        mt76u_tx_submit(dev, q);
}

/* sl0030: synchronous drain used from the stop/deinit path so no held
 * frame is left parked in [first, head) while queues are torn down.
 */
static void mt76u_tx_hold_cancel(struct mt76_dev *dev)
{
        hrtimer_cancel(&dev->usb.tx_hold_timer);
        atomic_set(&dev->usb.tx_hold_armed, 0);
}

static void
mt76u_ac_to_hwq(struct mt76_dev *dev, struct mt76_queue *q, u8 qid)
{
        u8 ac = qid < IEEE80211_NUM_ACS ? qid : IEEE80211_AC_BE;

        switch (mt76_chip(dev)) {
        case 0x7663: {
                static const u8 lmac_queue_map[] = {
                        /* ac to lmac mapping */
                        [IEEE80211_AC_BK] = 0,
                        [IEEE80211_AC_BE] = 1,
                        [IEEE80211_AC_VI] = 2,
                        [IEEE80211_AC_VO] = 4,
                };

                q->hw_idx = lmac_queue_map[ac];
                q->ep = q->hw_idx + 1;
                break;
        }
        case 0x7961:
        case 0x7925:
                /* TASK-017: Multi-Endpoint TX QoS.
                 * Q-07 (deep audit 2026-09-05): the 6-EP map is selected
                 * ONLY on an explicit force of 6 or an auto-detected 6-EP
                 * descriptor. Forcing 4/5 previously selected the 6-EP map
                 * (EP5 PSD submit on a 4/5-EP device = TX to invalid
                 * pipe); any other value (incl. out-of-range) falls back
                 * to the shared 2-3 EP mapping. */
                if (force_num_out_eps == 6 ||
                    (!force_num_out_eps && dev->usb.num_out_eps >= 6)) {
                        /* 6-endpoint mapping: each AC gets its own bulk-OUT
                         * pipe. q->ep indexes dev->usb.out_ep[], which is
                         * filled in USB descriptor order: out_ep[1]=AC_BE,
                         * [2]=AC_BK, [3]=AC_VI, [4]=AC_VO, [5]=HCCA/mc.
                         *
                         * sl0027: the legacy table below was written for
                         * the pre-6.x mac80211 AC order (BE=0,BK=1,VI=2,
                         * VO=3); this kernel uses VO=0,VI=1,BE=2,BK=3, so
                         * legacy sent AC_BE traffic down the fw's AC_BK
                         * pipe (out_ep[2]) and vice versa. AP-mode data on
                         * the BK pipe = the fw's silent AP-BSS wedge path
                         * (0026a evidence). sl_ep_fix=1 (default) selects
                         * the corrected table, indexed by the real AC
                         * values using the mt76u_out_ep enum so the pipe
                         * semantics cannot be misread again.
                         */
                        static const u8 ep_map_6_sl[] = {
                                [IEEE80211_AC_VO] = MT_EP_OUT_AC_VO,
                                [IEEE80211_AC_VI] = MT_EP_OUT_AC_VI,
                                [IEEE80211_AC_BE] = MT_EP_OUT_AC_BE,
                                [IEEE80211_AC_BK] = MT_EP_OUT_AC_BK,
                        };
                        static const u8 ep_map_6[] = {
                                [IEEE80211_AC_BK] = 0,
                                [IEEE80211_AC_BE] = 1,
                                [IEEE80211_AC_VI] = 2,
                                [IEEE80211_AC_VO] = 3,
                        };

                        q->hw_idx = mt76_ac_to_hwq(ac);
                        if (sl_ep_fix)
                                q->ep = ep_map_6_sl[ac];
                        else
                                q->ep = ep_map_6[ac] + 1;
                        if (qid == MT_TXQ_PSD)
                                q->ep = MT_EP_OUT_HCCA;
                } else {
                        /* Fallback: 2-3 endpoint mapping
                         * BE/BK share EP0, VI/VO share EP1, MCU on EP2
                         */
                        q->hw_idx = mt76_ac_to_hwq(ac);
                        q->ep = qid == MT_TXQ_PSD ? MT_EP_OUT_HCCA
                                                   : q->hw_idx + 1;
                }
                break;
        default:
                q->hw_idx = mt76_ac_to_hwq(ac);
                q->ep = q->hw_idx + 1;
                break;
        }
}

static int mt76u_alloc_tx(struct mt76_dev *dev)
{
        int i;

        for (i = 0; i <= MT_TXQ_PSD; i++) {
                struct mt76_queue *q;
                int j, err;

                q = devm_kzalloc(dev->dev, sizeof(*q), GFP_KERNEL);
                if (!q)
                        return -ENOMEM;

                spin_lock_init(&q->lock);
                mt76u_ac_to_hwq(dev, q, i);
                dev->phy.q_tx[i] = q;
                {
                        int s = sl26_slot_of(dev);

                        sl26_q[s][i] = q;
                        sl26_q_ep[s][i] = q->ep;
                        sl27_txwi_budget[s][i] = 16;
                }
                dev_info(dev->dev, "sl0026a: qid=%d -> ep=%d hwq=%d\n",
                         i, q->ep, q->hw_idx);

                q->entry = devm_kcalloc(dev->dev,
                                        MT_NUM_TX_ENTRIES, sizeof(*q->entry),
                                        GFP_KERNEL);
                if (!q->entry)
                        return -ENOMEM;

                q->ndesc = MT_NUM_TX_ENTRIES;
                for (j = 0; j < q->ndesc; j++) {
                        err = mt76u_urb_alloc(dev, &q->entry[j],
                                              MT_TX_SG_MAX_SIZE);
                        if (err < 0)
                                return err;
                }
        }
        return 0;
}

static void mt76u_free_tx(struct mt76_dev *dev)
{
        int i;

        /* sl0030: belt-and-braces for direct deinit paths */
        mt76u_tx_hold_cancel(dev);

        mt76_worker_teardown(&dev->usb.status_worker);

        for (i = 0; i <= MT_TXQ_PSD; i++) {
                struct mt76_queue *q;
                int j;

                q = dev->phy.q_tx[i];
                if (!q)
                        continue;

                for (j = 0; j < q->ndesc; j++) {
                        usb_free_urb(q->entry[j].urb);
                        q->entry[j].urb = NULL;
                }
        }
}

void mt76u_stop_tx(struct mt76_dev *dev)
{
        int ret;

        /* sl0030: submit any frames still parked in the hold window so
         * the drain/wait machinery below sees them, and make sure no
         * timer fires while the queues are being torn down. */
        mt76u_tx_hold_cancel(dev);
        mt76u_tx_hold_flush_acs(dev);

        mt76_worker_disable(&dev->usb.status_worker);

        ret = wait_event_timeout(dev->tx_wait, !mt76_has_tx_pending(&dev->phy),
                                 HZ / 5);
        if (!ret) {
                struct mt76_queue_entry entry;
                struct mt76_queue *q;
                int i, j;

                sl0026a_tx_dump(dev);
                dev_err(dev->dev, "timed out waiting for pending tx\n");

                for (i = 0; i <= MT_TXQ_PSD; i++) {
                        q = dev->phy.q_tx[i];
                        if (!q)
                                continue;

                        for (j = 0; j < q->ndesc; j++)
                                usb_kill_urb(q->entry[j].urb);
                }

                mt76_worker_disable(&dev->tx_worker);

                /* On device removal we maight queue skb's, but mt76u_tx_kick()
                 * will fail to submit urb, cleanup those skb's manually.
                 */
                for (i = 0; i <= MT_TXQ_PSD; i++) {
                        q = dev->phy.q_tx[i];
                        if (!q)
                                continue;

                        while (q->queued > 0) {
                                entry = q->entry[q->tail];
                                q->entry[q->tail].done = false;
                                mt76_queue_tx_complete(dev, q, &entry);
                        }
                }

                mt76_worker_enable(&dev->tx_worker);
        }

        cancel_work_sync(&dev->usb.stat_work);
        clear_bit(MT76_READING_STATS, &dev->phy.state);

        mt76_worker_enable(&dev->usb.status_worker);

        mt76_tx_status_check(dev, true);
}
EXPORT_SYMBOL_GPL(mt76u_stop_tx);

void mt76u_queues_deinit(struct mt76_dev *dev)
{
        mt76u_stop_rx(dev);
        mt76u_stop_tx(dev);

        mt76u_free_rx(dev);
        mt76u_free_tx(dev);
}
EXPORT_SYMBOL_GPL(mt76u_queues_deinit);

int mt76u_alloc_queues(struct mt76_dev *dev)
{
        int err;

        err = mt76u_alloc_rx_queue(dev, MT_RXQ_MAIN);
        if (err < 0)
                return err;

        return mt76u_alloc_tx(dev);
}
EXPORT_SYMBOL_GPL(mt76u_alloc_queues);

static const struct mt76_queue_ops usb_queue_ops = {
        .tx_queue_skb = mt76u_tx_queue_skb,
        .kick = mt76u_tx_kick,
};

int __mt76u_init(struct mt76_dev *dev, struct usb_interface *intf,
                 struct mt76_bus_ops *ops)
{
        struct usb_device *udev = interface_to_usbdev(intf);
        struct mt76_usb *usb = &dev->usb;
        int err;

        INIT_WORK(&usb->stat_work, mt76u_tx_status_data);

        /* sl0030: AC TX coalescing hold timer (6.18 removed
         * hrtimer_init; use the 6.15+ hrtimer_setup API) */
        hrtimer_setup(&usb->tx_hold_timer, mt76u_tx_hold_timer_fn,
                      CLOCK_MONOTONIC, HRTIMER_MODE_REL_SOFT);
        atomic_set(&usb->tx_hold_armed, 0);

        usb->data_len = usb_maxpacket(udev, usb_sndctrlpipe(udev, 0));
        if (usb->data_len < 32)
                usb->data_len = 32;

        usb->data = devm_kmalloc(dev->dev, usb->data_len, GFP_KERNEL);
        if (!usb->data)
                return -ENOMEM;

        mutex_init(&usb->usb_ctrl_mtx);
        dev->bus = ops;
        dev->queue_ops = &usb_queue_ops;

        dev_set_drvdata(&udev->dev, dev);

        usb->sg_en = mt76u_check_sg(dev);

        err = mt76u_set_endpoints(intf, usb);
        if (err < 0)
                return err;

        err = mt76_worker_setup(dev->hw, &usb->rx_worker, mt76u_rx_worker,
                                "usb-rx");
        if (err)
                return err;

        err = mt76_worker_setup(dev->hw, &usb->status_worker,
                                mt76u_status_worker, "usb-status");
        if (err)
                return err;

        sched_set_fifo_low(usb->rx_worker.task);
        sched_set_fifo_low(usb->status_worker.task);

        return 0;
}
EXPORT_SYMBOL_GPL(__mt76u_init);

int mt76u_init(struct mt76_dev *dev, struct usb_interface *intf)
{
        static struct mt76_bus_ops bus_ops = {
                .rr = mt76u_rr,
                .wr = mt76u_wr,
                .rmw = mt76u_rmw,
                .read_copy = mt76u_read_copy,
                .write_copy = mt76u_copy,
                .wr_rp = mt76u_wr_rp,
                .rd_rp = mt76u_rd_rp,
                .type = MT76_BUS_USB,
        };

        return __mt76u_init(dev, intf, &bus_ops);
}
EXPORT_SYMBOL_GPL(mt76u_init);

MODULE_AUTHOR("Lorenzo Bianconi <lorenzo.bianconi83@gmail.com>");
MODULE_DESCRIPTION("MediaTek MT76x USB helpers");
MODULE_LICENSE("Dual BSD/GPL");
