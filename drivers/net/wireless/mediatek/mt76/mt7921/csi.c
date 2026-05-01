// SPDX-License-Identifier: BSD-3-Clause-Clear
/* Copyright (C) 2025 mt7921u Forensic Audit Project
 *
 * CSI (Channel State Information) extraction for mt7921.
 * Based on vendor driver's CSI control command and data structures.
 *
 * Firmware command: CMD_ID_CSI_CONTROL = 0x4C
 * Firmware event:  EVENT_ID_CSI_DATA   = 0x3C
 *
 * Research basis: "Enhancing CSI-Based Wireless Sensing With Open-Source
 * Linux 802.11ax CSI Tool" (IEEE, May 2025); WhoFi system achieves
 * 95.5% accuracy for human presence detection.
 *
 * This implementation makes the MT7921U the first MediaTek USB adapter
 * with open CSI access — a significant milestone for the Wi-Fi sensing
 * research community.
 */

#include "mt7921.h"
#include "mcu.h"
#include "../mt76_connac2_mac.h"

/* Vendor driver CSI command ID: now in MCU_CE_CMD_CSI_CONTROL (0x4C) */
#define EVENT_ID_CSI_DATA       0x3C

/* CSI event TLV tags (from vendor driver nic_cmd_event.h) */
enum mt7921_csi_event_tlv_tag {
        MT7921_CSI_EVENT_VERSION,
        MT7921_CSI_EVENT_CBW,
        MT7921_CSI_EVENT_RSSI,
        MT7921_CSI_EVENT_SNR,
        MT7921_CSI_EVENT_BAND,
        MT7921_CSI_EVENT_CSI_NUM,
        MT7921_CSI_EVENT_CSI_I_DATA,
        MT7921_CSI_EVENT_CSI_Q_DATA,
        MT7921_CSI_EVENT_DBW,
        MT7921_CSI_EVENT_CH_IDX,
        MT7921_CSI_EVENT_TA,
        MT7921_CSI_EVENT_EXTRA_INFO,
        MT7921_CSI_EVENT_RX_MODE,
        MT7921_CSI_EVENT_H_IDX,
        MT7921_CSI_EVENT_TX_RX_IDX,
};

struct mt7921_csi_tlv_element {
        __le16 tag_type;
        __le16 body_len;
        u8 body[];
} __packed;

/**
 * mt7921_mcu_csi_control - Send CSI control command to firmware
 * @dev: mt792x device
 * @band_idx: band index (0 for primary)
 * @mode: CSI control mode (STOP/START/SET)
 * @cfg_item: configuration item selector
 * @val1: first config value
 * @val2: second config value
 *
 * Sends CMD_ID_CSI_CONTROL (0x4C) to firmware to start/stop/configure
 * CSI data capture. This command is derived from the vendor driver's
 * CMD_CSI_CONTROL_T structure.
 *
 * Return: 0 on success, negative error code on failure
 */
int mt7921_mcu_csi_control(struct mt792x_dev *dev, u8 band_idx,
                           enum mt7921_csi_control_mode mode,
                           enum mt7921_csi_config_item cfg_item,
                           u8 val1, u8 val2)
{
        struct {
                u8 band_idx;
                u8 mode;
                u8 cfg_item;
                u8 rsv;
                u8 val1;
                u8 val2;
                u8 rsv2[2];
        } __packed req = {
                .band_idx = band_idx,
                .mode = mode,
                .cfg_item = cfg_item,
                .val1 = val1,
                .val2 = val2,
        };

        return mt76_mcu_send_msg(&dev->mt76,
                                 MCU_CE_CMD(CSI_CONTROL),
                                 &req, sizeof(req), true);
}

/**
 * mt7921_mcu_csi_event - Process CSI data event from firmware
 * @dev: mt792x device
 * @skb: SKB containing CSI event data
 *
 * Processes EVENT_ID_CSI_DATA (0x3C) from firmware, extracting
 * I/Q sample data, RSSI, SNR, and other metadata into the
 * driver's CSI ring buffer. This follows the vendor driver's
 * TLV-based event format from nic_cmd_event.c:nicEventCSIData().
 *
 * The CSI data is stored in a per-device ring buffer and can be
 * read by userspace via debugfs or a future vendor nl80211 command.
 */
void mt7921_mcu_csi_event(struct mt792x_dev *dev, struct sk_buff *skb)
{
        struct mt7921_csi_info *csi = &dev->csi;
        struct mt7921_csi_data *entry;
        struct mt7921_csi_tlv_element *tlv;
        unsigned long flags;
        u16 tag, len;
        int offset = 0;

        if (!csi->enabled) {
                dev_kfree_skb(skb);
                return;
        }

        /* Sanity check: firmware event must have at least a TLV header */
        if (skb->len < sizeof(struct mt7921_csi_tlv_element)) {
                dev_warn_ratelimited(dev->mt76.dev,
                                     "CSI event: skb too short (%u)\n",
                                     skb->len);
                dev_kfree_skb(skb);
                return;
        }

        spin_lock_irqsave(&csi->ring_lock, flags);

        /* Get next ring buffer entry */
        entry = &csi->buffer[csi->head];
        memset(entry, 0, sizeof(*entry));

        /* Parse TLV elements from firmware event */
        while (offset + sizeof(*tlv) <= skb->len) {
                tlv = (struct mt7921_csi_tlv_element *)(skb->data + offset);
                tag = le16_to_cpu(tlv->tag_type);
                len = le16_to_cpu(tlv->body_len);

                if (offset + sizeof(*tlv) + len > skb->len)
                        break;

                switch (tag) {
                case MT7921_CSI_EVENT_VERSION:
                        if (len >= 1)
                                entry->fw_ver = tlv->body[0];
                        break;
                case MT7921_CSI_EVENT_CBW:
                        if (len >= 1)
                                entry->bw = tlv->body[0];
                        break;
                case MT7921_CSI_EVENT_RSSI:
                        if (len >= 1)
                                entry->rssi = (s8)tlv->body[0];
                        break;
                case MT7921_CSI_EVENT_SNR:
                        if (len >= 1)
                                entry->snr = tlv->body[0];
                        break;
                case MT7921_CSI_EVENT_CSI_NUM:
                        if (len >= 2)
                                entry->data_count = le16_to_cpu(*(__le16 *)tlv->body);
                        break;
                case MT7921_CSI_EVENT_CSI_I_DATA:
                        if (len <= sizeof(entry->i_data))
                                memcpy(entry->i_data, tlv->body, len);
                        break;
                case MT7921_CSI_EVENT_CSI_Q_DATA:
                        if (len <= sizeof(entry->q_data))
                                memcpy(entry->q_data, tlv->body, len);
                        break;
                case MT7921_CSI_EVENT_DBW:
                        if (len >= 1)
                                entry->data_bw = tlv->body[0];
                        break;
                case MT7921_CSI_EVENT_CH_IDX:
                        if (len >= 1)
                                entry->primary_ch_idx = tlv->body[0];
                        break;
                case MT7921_CSI_EVENT_TA:
                        if (len >= ETH_ALEN)
                                memcpy(entry->ta, tlv->body, ETH_ALEN);
                        break;
                case MT7921_CSI_EVENT_EXTRA_INFO:
                        if (len >= 4)
                                entry->extra_info = le32_to_cpu(*(__le32 *)tlv->body);
                        break;
                case MT7921_CSI_EVENT_RX_MODE:
                        if (len >= 1)
                                entry->rx_mode = tlv->body[0];
                        break;
                case MT7921_CSI_EVENT_TX_RX_IDX:
                        if (len >= 4)
                                entry->tr_idx = le32_to_cpu(*(__le32 *)tlv->body);
                        break;
                default:
                        break;
                }

                offset += sizeof(*tlv) + len;
        }

        /* Advance ring buffer head */
        csi->head = (csi->head + 1) % MT7921_CSI_RING_SIZE;
        if (csi->head == csi->tail)
                csi->tail = (csi->tail + 1) % MT7921_CSI_RING_SIZE;

        spin_unlock_irqrestore(&csi->ring_lock, flags);

        /* Wake any waiting readers */
        wake_up(&csi->waitq);

        dev_kfree_skb(skb);
}

/**
 * mt7921_csi_start - Enable CSI data capture
 * @dev: mt792x device
 * @band_idx: band index
 * @output_format: CSI output format (raw, tone-masked, etc.)
 *
 * Enables CSI capture by sending START command to firmware,
 * followed by SET commands for output format configuration.
 *
 * Return: 0 on success, negative error code on failure
 */
int mt7921_csi_start(struct mt792x_dev *dev, u8 band_idx,
                     enum mt7921_csi_output_format output_format)
{
        struct mt7921_csi_info *csi = &dev->csi;
        int ret;

        if (csi->enabled)
                return -EBUSY;

        /* Configure output format before starting */
        ret = mt7921_mcu_csi_control(dev, band_idx,
                                      MT7921_CSI_CONTROL_SET,
                                      MT7921_CSI_CONFIG_OUTPUT_FORMAT,
                                      output_format, 0);
        if (ret)
                return ret;

        /* Start CSI capture */
        ret = mt7921_mcu_csi_control(dev, band_idx,
                                      MT7921_CSI_CONTROL_START,
                                      0, 0, 0);
        if (ret)
                return ret;

        csi->enabled = true;
        csi->head = 0;
        csi->tail = 0;

        return 0;
}

/**
 * mt7921_csi_stop - Disable CSI data capture
 * @dev: mt792x device
 * @band_idx: band index
 *
 * Disables CSI capture by sending STOP command to firmware.
 *
 * Return: 0 on success, negative error code on failure
 */
int mt7921_csi_stop(struct mt792x_dev *dev, u8 band_idx)
{
        struct mt7921_csi_info *csi = &dev->csi;
        int ret;

        if (!csi->enabled)
                return 0;

        ret = mt7921_mcu_csi_control(dev, band_idx,
                                      MT7921_CSI_CONTROL_STOP,
                                      0, 0, 0);
        if (ret)
                return ret;

        csi->enabled = false;
        return 0;
}

/* Initialize CSI subsystem during device probe */
void mt7921_csi_init(struct mt792x_dev *dev)
{
        struct mt7921_csi_info *csi = &dev->csi;

        memset(csi, 0, sizeof(*csi));
        spin_lock_init(&csi->ring_lock);
        init_waitqueue_head(&csi->waitq);
        csi->enabled = false;
}

/* Cleanup CSI subsystem during device removal */
void mt7921_csi_cleanup(struct mt792x_dev *dev)
{
        struct mt7921_csi_info *csi = &dev->csi;

        if (csi->enabled)
                mt7921_csi_stop(dev, 0);

        wake_up(&csi->waitq);
}
