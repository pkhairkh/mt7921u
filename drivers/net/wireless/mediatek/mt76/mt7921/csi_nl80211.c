// SPDX-License-Identifier: BSD-3-Clause-Clear
/* Copyright (C) 2025 mt7921u Forensic Audit Project
 *
 * CSI nl80211 vendor command interface for mt7921.
 * Provides userspace control of CSI capture via vendor-specific
 * nl80211 commands, enabling Wi-Fi sensing applications to
 * start/stop CSI collection and retrieve I/Q sample data.
 *
 * Commands:
 *   MT7921_NL_CMD_CSI_START  - Begin CSI capture on a band
 *   MT7921_NL_CMD_CSI_STOP   - Stop CSI capture on a band
 *   MT7921_NL_CMD_CSI_GET    - Read CSI samples from ring buffer
 *   MT7921_NL_CMD_CSI_CONFIG - Set CSI configuration parameters
 */

#include <net/genetlink.h>
#include <net/cfg80211.h>
#include "mt7921.h"
#include "../mt76_connac2_mac.h"

/* Vendor OUI: MediaTek 00:0C:E7 */
#define MT7921_CSI_VENDOR_OUI		0x00E70C
#define MT7921_CSI_VENDOR_OUI_TYPE	0x01

/* CSI vendor command attributes */
enum mt7921_nl_attr_csi {
	__MT7921_NL_ATTR_CSI_INVALID,
	MT7921_NL_ATTR_CSI_DATA,	/* binary blob: raw CSI I/Q */
	MT7921_NL_ATTR_CSI_MODE,	/* u8: start/stop/set */
	MT7921_NL_ATTR_CSI_BAND_IDX,	/* u8: band index */
	MT7921_NL_ATTR_CSI_CFG_ITEM,	/* u8: config item selector */
	MT7921_NL_ATTR_CSI_VAL1,	/* u8: config value 1 */
	MT7921_NL_ATTR_CSI_VAL2,	/* u8: config value 2 */
	MT7921_NL_ATTR_CSI_BW,		/* u8: channel bandwidth */
	MT7921_NL_ATTR_CSI_RSSI,	/* s8: RSSI */
	MT7921_NL_ATTR_CSI_SNR,	/* u8: SNR */
	MT7921_NL_ATTR_CSI_TA,		/* 6-byte MAC: transmitter address */
	MT7921_NL_ATTR_CSI_COUNT,	/* u16: number of CSI samples */

	__MT7921_NL_ATTR_CSI_AFTER_LAST,
	MT7921_NL_ATTR_CSI_MAX = __MT7921_NL_ATTR_CSI_AFTER_LAST - 1,
};

/* CSI vendor commands */
enum mt7921_nl_cmd_csi {
	MT7921_NL_CMD_CSI_START,
	MT7921_NL_CMD_CSI_STOP,
	MT7921_NL_CMD_CSI_GET,
	MT7921_NL_CMD_CSI_CONFIG,
};

/* Attribute validation policy */
static const struct nla_policy
mt7921_csi_nl_policy[MT7921_NL_ATTR_CSI_MAX + 1] = {
	[MT7921_NL_ATTR_CSI_DATA]	= { .type = NLA_BINARY,
					    .len = MT7921_CSI_DATA_SIZE * 4 },
	[MT7921_NL_ATTR_CSI_MODE]	= { .type = NLA_U8 },
	[MT7921_NL_ATTR_CSI_BAND_IDX]	= { .type = NLA_U8 },
	[MT7921_NL_ATTR_CSI_CFG_ITEM]	= { .type = NLA_U8 },
	[MT7921_NL_ATTR_CSI_VAL1]	= { .type = NLA_U8 },
	[MT7921_NL_ATTR_CSI_VAL2]	= { .type = NLA_U8 },
	[MT7921_NL_ATTR_CSI_BW]	= { .type = NLA_U8 },
	[MT7921_NL_ATTR_CSI_RSSI]	= { .type = NLA_S8 },
	[MT7921_NL_ATTR_CSI_SNR]	= { .type = NLA_U8 },
	[MT7921_NL_ATTR_CSI_TA]	= { .type = NLA_BINARY,
					    .len = ETH_ALEN },
	[MT7921_NL_ATTR_CSI_COUNT]	= { .type = NLA_U16 },
};

/**
 * mt7921_nl_csi_start - Handle CSI_START vendor command
 * @wiphy: wiphy pointer
 * @wdev: wireless dev
 * @data: netlink attributes
 * @data_len: length of data
 *
 * Starts CSI capture on the specified band with the given output format.
 * Required attributes: MT7921_NL_ATTR_CSI_BAND_IDX
 * Optional attributes: MT7921_NL_ATTR_CSI_MODE (output format)
 *
 * Return: 0 on success, negative error code on failure
 */
static int mt7921_nl_csi_start(struct wiphy *wiphy,
			       struct wireless_dev *wdev,
			       const void *data, int data_len)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct mt792x_dev *dev = mt792x_hw_dev(hw);
	struct nlattr *attrs[MT7921_NL_ATTR_CSI_MAX + 1];
	u8 band_idx = 0;
	u8 output_format = MT7921_CSI_OUTPUT_RAW;
	int ret;

	ret = nla_parse_deprecated(attrs, MT7921_NL_ATTR_CSI_MAX,
				   data, data_len,
				   mt7921_csi_nl_policy, NULL);
	if (ret)
		return ret;

	if (!attrs[MT7921_NL_ATTR_CSI_BAND_IDX])
		return -EINVAL;

	band_idx = nla_get_u8(attrs[MT7921_NL_ATTR_CSI_BAND_IDX]);

	if (attrs[MT7921_NL_ATTR_CSI_MODE])
		output_format = nla_get_u8(attrs[MT7921_NL_ATTR_CSI_MODE]);

	if (output_format > MT7921_CSI_OUTPUT_TONE_MASKED_SHIFTED)
		return -EINVAL;

	mt792x_mutex_acquire(dev);
	ret = mt7921_csi_start(dev, band_idx, output_format);
	mt792x_mutex_release(dev);

	/* RUNTIME_VERIFY: capture CSI with iw command and verify data */
	return ret;
}

/**
 * mt7921_nl_csi_stop - Handle CSI_STOP vendor command
 * @wiphy: wiphy pointer
 * @wdev: wireless dev
 * @data: netlink attributes
 * @data_len: length of data
 *
 * Stops CSI capture on the specified band.
 * Required attributes: MT7921_NL_ATTR_CSI_BAND_IDX
 *
 * Return: 0 on success, negative error code on failure
 */
static int mt7921_nl_csi_stop(struct wiphy *wiphy,
			      struct wireless_dev *wdev,
			      const void *data, int data_len)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct mt792x_dev *dev = mt792x_hw_dev(hw);
	struct nlattr *attrs[MT7921_NL_ATTR_CSI_MAX + 1];
	u8 band_idx = 0;
	int ret;

	ret = nla_parse_deprecated(attrs, MT7921_NL_ATTR_CSI_MAX,
				   data, data_len,
				   mt7921_csi_nl_policy, NULL);
	if (ret)
		return ret;

	if (!attrs[MT7921_NL_ATTR_CSI_BAND_IDX])
		return -EINVAL;

	band_idx = nla_get_u8(attrs[MT7921_NL_ATTR_CSI_BAND_IDX]);

	mt792x_mutex_acquire(dev);
	ret = mt7921_csi_stop(dev, band_idx);
	mt792x_mutex_release(dev);

	return ret;
}

/**
 * mt7921_nl_csi_get - Handle CSI_GET vendor command
 * @wiphy: wiphy pointer
 * @wdev: wireless dev
 * @data: netlink attributes
 * @data_len: length of data
 *
 * Reads one CSI sample from the ring buffer and returns it as a
 * netlink message containing the raw firmware event data as a
 * binary blob, along with metadata (BW, RSSI, SNR, TA, COUNT).
 *
 * Until hardware confirms the exact firmware response format,
 * the raw firmware event data is encapsulated as a binary blob.
 * Userspace must parse it according to the TLV format documented
 * in mt7921_csi.rst.
 *
 * Return: 0 on success, negative error code on failure
 */
static int mt7921_nl_csi_get(struct wiphy *wiphy,
			     struct wireless_dev *wdev,
			     const void *data, int data_len)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct mt792x_dev *dev = mt792x_hw_dev(hw);
	struct mt7921_csi_info *csi = &dev->csi;
	struct mt7921_csi_data *entry;
	struct sk_buff *msg;
	struct nlattr *attrs[MT7921_NL_ATTR_CSI_MAX + 1];
	int ret;
	u32 tail;
	u16 data_count;
	size_t iq_len;

	ret = nla_parse_deprecated(attrs, MT7921_NL_ATTR_CSI_MAX,
				   data, data_len,
				   mt7921_csi_nl_policy, NULL);
	if (ret)
		return ret;

	mutex_lock(&dev->mt76.mutex);

	if (!csi->enabled) {
		ret = -ENOENT;
		goto unlock;
	}

	if (csi->head == csi->tail) {
		ret = -EAGAIN;
		goto unlock;
	}

	/* Read from tail (oldest unread entry) */
	tail = csi->tail;
	entry = &csi->buffer[tail];

	/*
	 * Encapsulate raw I/Q data as a binary blob.
	 * I and Q are interleaved as int16 pairs, so total
	 * I/Q payload = data_count * 2 * sizeof(s16).
	 *
	 * Until firmware confirms exact format, we emit I then Q
	 * as interleaved int16 values.
	 */
	data_count = entry->data_count;
	if (data_count > MT7921_CSI_DATA_SIZE)
		data_count = MT7921_CSI_DATA_SIZE;

	iq_len = data_count * 2 * sizeof(s16);

	msg = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, iq_len + 128);
	if (!msg) {
		ret = -ENOMEM;
		goto unlock;
	}

	/* Build interleaved I/Q blob: I0,Q0,I1,Q1,... */
	if (data_count > 0) {
		s16 *iq_buf;
		int i;

		iq_buf = kmalloc(iq_len, GFP_KERNEL);
		if (!iq_buf) {
			nlmsg_free(msg);
			ret = -ENOMEM;
			goto unlock;
		}

		for (i = 0; i < data_count; i++) {
			iq_buf[i * 2] = entry->i_data[i];
			iq_buf[i * 2 + 1] = entry->q_data[i];
		}

		ret = nla_put(msg, MT7921_NL_ATTR_CSI_DATA, iq_len, iq_buf);
		kfree(iq_buf);

		if (ret) {
			nlmsg_free(msg);
			goto unlock;
		}
	}

	if (nla_put_u8(msg, MT7921_NL_ATTR_CSI_BW, entry->bw) ||
	    nla_put_s8(msg, MT7921_NL_ATTR_CSI_RSSI, entry->rssi) ||
	    nla_put_u8(msg, MT7921_NL_ATTR_CSI_SNR, entry->snr) ||
	    nla_put(msg, MT7921_NL_ATTR_CSI_TA, ETH_ALEN, entry->ta) ||
	    nla_put_u16(msg, MT7921_NL_ATTR_CSI_COUNT, data_count)) {
		nlmsg_free(msg);
		ret = -EMSGSIZE;
		goto unlock;
	}

	/* Advance tail after successful read */
	csi->tail = (tail + 1) % MT7921_CSI_RING_SIZE;

	mutex_unlock(&dev->mt76.mutex);

	/* RUNTIME_VERIFY: capture CSI with iw command and verify data */
	return cfg80211_vendor_cmd_reply(msg);

unlock:
	mutex_unlock(&dev->mt76.mutex);
	return ret;
}

/**
 * mt7921_nl_csi_config - Handle CSI_CONFIG vendor command
 * @wiphy: wiphy pointer
 * @wdev: wireless dev
 * @data: netlink attributes
 * @data_len: length of data
 *
 * Sets CSI configuration parameters via the firmware control command.
 * Required attributes: MT7921_NL_ATTR_CSI_CFG_ITEM,
 *                       MT7921_NL_ATTR_CSI_VAL1,
 *                       MT7921_NL_ATTR_CSI_VAL2
 * Optional attributes: MT7921_NL_ATTR_CSI_BAND_IDX
 *
 * Return: 0 on success, negative error code on failure
 */
static int mt7921_nl_csi_config(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct mt792x_dev *dev = mt792x_hw_dev(hw);
	struct nlattr *attrs[MT7921_NL_ATTR_CSI_MAX + 1];
	u8 band_idx = 0;
	u8 cfg_item, val1, val2;
	int ret;

	ret = nla_parse_deprecated(attrs, MT7921_NL_ATTR_CSI_MAX,
				   data, data_len,
				   mt7921_csi_nl_policy, NULL);
	if (ret)
		return ret;

	if (!attrs[MT7921_NL_ATTR_CSI_CFG_ITEM] ||
	    !attrs[MT7921_NL_ATTR_CSI_VAL1] ||
	    !attrs[MT7921_NL_ATTR_CSI_VAL2])
		return -EINVAL;

	cfg_item = nla_get_u8(attrs[MT7921_NL_ATTR_CSI_CFG_ITEM]);
	val1 = nla_get_u8(attrs[MT7921_NL_ATTR_CSI_VAL1]);
	val2 = nla_get_u8(attrs[MT7921_NL_ATTR_CSI_VAL2]);

	if (attrs[MT7921_NL_ATTR_CSI_BAND_IDX])
		band_idx = nla_get_u8(attrs[MT7921_NL_ATTR_CSI_BAND_IDX]);

	if (cfg_item >= MT7921_CSI_CONFIG_INFO + 1)
		return -EINVAL;

	mt792x_mutex_acquire(dev);
	ret = mt7921_mcu_csi_control(dev, band_idx,
				      MT7921_CSI_CONTROL_SET,
				      cfg_item, val1, val2);
	mt792x_mutex_release(dev);

	return ret;
}

/* Vendor command definitions */
static const struct wiphy_vendor_command mt7921_csi_vendor_cmds[] = {
	{
		.info = {
			.vendor_id = MT7921_CSI_VENDOR_OUI,
			.subcmd = MT7921_NL_CMD_CSI_START,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mt7921_nl_csi_start,
		.policy = mt7921_csi_nl_policy,
		.maxattr = MT7921_NL_ATTR_CSI_MAX,
	},
	{
		.info = {
			.vendor_id = MT7921_CSI_VENDOR_OUI,
			.subcmd = MT7921_NL_CMD_CSI_STOP,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mt7921_nl_csi_stop,
		.policy = mt7921_csi_nl_policy,
		.maxattr = MT7921_NL_ATTR_CSI_MAX,
	},
	{
		.info = {
			.vendor_id = MT7921_CSI_VENDOR_OUI,
			.subcmd = MT7921_NL_CMD_CSI_GET,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mt7921_nl_csi_get,
		.policy = mt7921_csi_nl_policy,
		.maxattr = MT7921_NL_ATTR_CSI_MAX,
	},
	{
		.info = {
			.vendor_id = MT7921_CSI_VENDOR_OUI,
			.subcmd = MT7921_NL_CMD_CSI_CONFIG,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mt7921_nl_csi_config,
		.policy = mt7921_csi_nl_policy,
		.maxattr = MT7921_NL_ATTR_CSI_MAX,
	},
};

/* Vendor event definitions */
static const struct nl80211_vendor_cmd_info mt7921_csi_vendor_events[] = {
	{
		.vendor_id = MT7921_CSI_VENDOR_OUI,
		.subcmd = MT7921_NL_CMD_CSI_GET,
	},
};

/**
 * mt7921_csi_nl80211_register - Register CSI vendor commands with wiphy
 * @wiphy: wiphy to register commands on
 *
 * Registers the MediaTek CSI vendor commands and events with the
 * cfg80211 subsystem, making them available to userspace via
 * generic netlink.
 *
 * Return: 0 on success, negative error code on failure
 */
int mt7921_csi_nl80211_register(struct wiphy *wiphy)
{
	wiphy->vendor_commands = mt7921_csi_vendor_cmds;
	wiphy->n_vendor_commands = ARRAY_SIZE(mt7921_csi_vendor_cmds);
	wiphy->vendor_events = mt7921_csi_vendor_events;
	wiphy->n_vendor_events = ARRAY_SIZE(mt7921_csi_vendor_events);

	return 0;
}
