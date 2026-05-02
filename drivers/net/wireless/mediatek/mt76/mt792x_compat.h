/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/* Copyright (C) 2024 MediaTek Inc. */
/*
 * Kernel 6.12 compatibility header for the MT7921U driver.
 *
 * The upstream mt76 driver targets kernel 6.13+ which introduced MLO
 * (Multi-Link Operation), multi-radio wiphy, and many ieee80211 API
 * changes. This header provides compat macros, wrappers, and stubs so
 * the driver compiles and runs correctly on kernel 6.12.
 */

#ifndef __MT792X_COMPAT_H
#define __MT792X_COMPAT_H

#include <linux/version.h>

/* ========================================================================
 * Section 1: MLO constants and helpers (6.13+ only)
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* 6.13+ provides these natively — nothing to define */
#else

/* In 6.12, MLD/MLD is not supported. Define constants for compilation. */
#ifndef IEEE80211_MLD_MAX_NUM_LINKS
#define IEEE80211_MLD_MAX_NUM_LINKS 1
#endif

#ifndef IEEE80211_LINK_UNSPECIFIED
#define IEEE80211_LINK_UNSPECIFIED 0xff
#endif

/* ieee80211_vif_is_mld() doesn't exist in 6.12 — always false */
static inline bool ieee80211_vif_is_mld(struct ieee80211_vif *vif)
{
	return false;
}

/* link_conf_dereference_protected() doesn't exist in 6.12 */
static inline struct ieee80211_bss_conf *
link_conf_dereference_protected(struct ieee80211_vif *vif, unsigned int link_id)
{
	return &vif->bss_conf;
}

/* link_sta_dereference_protected() doesn't exist in 6.12.
 * In 6.12, ieee80211_sta fields are accessed directly (no deflink).
 * We can't return &sta->deflink because that member doesn't exist.
 * Instead, callers should use the _compat accessor macros below.
 */

#endif /* < 6.13 */

/* ========================================================================
 * Section 2: sta->deflink compat (6.13+ wraps link fields in deflink)
 *
 * In 6.13+, struct ieee80211_sta has a 'deflink' member of type
 * struct ieee80211_link_sta that wraps HT/VHT/HE caps, bandwidth, etc.
 * In 6.12, those fields exist directly on struct ieee80211_sta.
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Use sta->deflink.X directly — native API */
#define mt792x_sta_deflink(sta) (&(sta)->deflink)
#else
/* In 6.12, the fields are directly on sta. We cast sta to the
 * ieee80211_link_sta layout — but that type doesn't exist either.
 * Instead, just access fields directly on sta (see Section 3 macros).
 */
#endif

/* ========================================================================
 * Section 3: Accessor macros for sta link fields
 *
 * These macros abstract the difference between:
 *   6.12: sta->ht_cap, sta->vht_cap, sta->he_cap, etc.
 *   6.13+: sta->deflink.ht_cap, sta->deflink.vht_cap, etc.
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define STA_HT_CAP(sta)		((sta)->deflink.ht_cap)
#define STA_VHT_CAP(sta)		((sta)->deflink.vht_cap)
#define STA_HE_CAP(sta)		((sta)->deflink.he_cap)
#define STA_HE_6GHZ_CAPA(sta)	((sta)->deflink.he_6ghz_capa)
#define STA_BANDWIDTH(sta)		((sta)->deflink.bandwidth)
#define STA_RX_NSS(sta)		((sta)->deflink.rx_nss)
#define STA_SMPS_MODE(sta)		((sta)->deflink.smps_mode)
#define STA_SUPP_RATES(sta)		((sta)->deflink.supp_rates)
#define STA_ADDR(sta)			((sta)->deflink.addr)
#define STA_AID(sta)			((sta)->deflink.sta->aid)
#define STA_WME(sta)			((sta)->deflink.sta->wme)
#define STA_AGG_MAX_AMSDU_LEN(sta)	((sta)->deflink.agg.max_amsdu_len)
#else
#define STA_HT_CAP(sta)		((sta)->ht_cap)
#define STA_VHT_CAP(sta)		((sta)->vht_cap)
#define STA_HE_CAP(sta)		((sta)->he_cap)
#define STA_HE_6GHZ_CAPA(sta)	((sta)->he_6ghz_capa)
#define STA_BANDWIDTH(sta)		((sta)->bandwidth)
#define STA_RX_NSS(sta)		((sta)->rx_nss)
#define STA_SMPS_MODE(sta)		((sta)->smps_mode)
#define STA_SUPP_RATES(sta)		((sta)->supp_rates)
#define STA_ADDR(sta)			((sta)->addr)
#define STA_AID(sta)			((sta)->aid)
#define STA_WME(sta)			((sta)->wme)
#define STA_AGG_MAX_AMSDU_LEN(sta)	((sta)->max_amsdu_len)
#endif

/* ========================================================================
 * Section 4: vif->cfg vs vif->bss_conf compat
 *
 * In 6.13+, some fields moved from vif->bss_conf to vif->cfg:
 *   - vif->cfg.assoc
 *   - vif->cfg.aid
 *   - vif->cfg.ps
 *   - vif->cfg.p2p
 * In 6.12, these are on vif->bss_conf or directly on vif.
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native — use vif->cfg.X directly */
#define VIF_ASSOC(vif)		((vif)->cfg.assoc)
#define VIF_AID(vif)		((vif)->cfg.aid)
#define VIF_PS(vif)		((vif)->cfg.ps)
#define VIF_P2P(vif)		((vif)->cfg.p2p)
#else
#define VIF_ASSOC(vif)		((vif)->bss_conf.assoc)
#define VIF_AID(vif)		((vif)->bss_conf.aid)
#define VIF_PS(vif)		((vif)->bss_conf.ps)
#define VIF_P2P(vif)		((vif)->p2p)
#endif

/* ========================================================================
 * Section 5: chanreq vs chandef compat
 *
 * In 6.13+, vif->bss_conf.chanreq.oper wraps the channel definition
 * for MLO. In 6.12, vif->bss_conf.chandef is used directly.
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define VIF_CHANDEF(vif)	((vif)->bss_conf.chanreq.oper)
#else
#define VIF_CHANDEF(vif)	((vif)->bss_conf.chandef)
#endif

/* ========================================================================
 * Section 6: ieee80211 API signature differences
 * ======================================================================== */

/* ieee80211_set_sband_iftype_data: 2 args in 6.12, 3 in 6.13+ */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define mt792x_set_sband_iftype_data(band, data, n) \
	ieee80211_set_sband_iftype_data(band, data, n)
#else
#define mt792x_set_sband_iftype_data(band, data, n) \
	ieee80211_set_sband_iftype_data(band, data)
#endif

/* ieee80211_radar_detected: 1 arg in 6.12, 2 in 6.13+ */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define mt792x_radar_detected(hw, vif) \
	ieee80211_radar_detected(hw, vif)
#else
#define mt792x_radar_detected(hw, vif) \
	ieee80211_radar_detected(hw)
#endif

/* ieee80211_chswitch_done: 2 args in 6.12, 3 in 6.13+ */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define mt792x_chswitch_done(vif, success, link_id) \
	ieee80211_chswitch_done(vif, success, link_id)
#else
#define mt792x_chswitch_done(vif, success, link_id) \
	ieee80211_chswitch_done(vif, success)
#endif

/* ========================================================================
 * Section 7: ieee80211_cac_finish — doesn't exist in 6.12
 *
 * In 6.12, CAC completion is handled via cfg80211_cac_event().
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define mt792x_cac_finish(hw, vif) \
	ieee80211_cac_finish(hw, vif)
#else
static inline void mt792x_cac_finish(struct ieee80211_hw *hw,
				      struct ieee80211_vif *vif)
{
	struct cfg80211_chan_def chandef = {};
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
	chandef = vif->bss_conf.chandef;
#endif
	cfg80211_cac_event(hw->wiphy, &chandef,
			   NL80211_RADAR_CAC_FINISHED, GFP_KERNEL);
}
#endif

/* ========================================================================
 * Section 8: ieee80211_refresh_tx_agg_session_timer — 6.13+ only
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native */
#else
static inline void
ieee80211_refresh_tx_agg_session_timer(struct ieee80211_sta *sta,
					u8 tid)
{
	/* No-op on 6.12 — the timer refresh is a 6.13+ feature */
}
#endif

/* ========================================================================
 * Section 9: Callback signature wrappers for ieee80211_ops
 *
 * In 6.13+, several callbacks gained extra parameters (radio_idx,
 * link_id, link_conf, suspend). In 6.12, these parameters don't
 * exist. We provide wrapper function signatures.
 * ======================================================================== */

/* .stop callback */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define MT792X_STOP_PROTO	struct ieee80211_hw *hw, bool suspend
#define MT792X_STOP_ARGS	hw, suspend
#define MT792X_STOP_CALL(fn, hw)	fn(hw, false)
#else
#define MT792X_STOP_PROTO	struct ieee80211_hw *hw
#define MT792X_STOP_ARGS	hw
#define MT792X_STOP_CALL(fn, hw)	fn(hw)
#endif

/* .config callback */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define MT792X_CONFIG_PROTO	struct ieee80211_hw *hw, int radio_idx, u32 changed
#define MT792X_CONFIG_ARGS	hw, radio_idx, changed
#else
#define MT792X_CONFIG_PROTO	struct ieee80211_hw *hw, u32 changed
#define MT792X_CONFIG_ARGS	hw, changed
#endif

/* .conf_tx callback */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define MT792X_CONF_TX_PROTO	struct ieee80211_hw *hw, struct ieee80211_vif *vif, \
				unsigned int link_id, u16 queue, \
				const struct ieee80211_tx_queue_params *params
#define MT792X_CONF_TX_ARGS	hw, vif, link_id, queue, params
#else
#define MT792X_CONF_TX_PROTO	struct ieee80211_hw *hw, struct ieee80211_vif *vif, \
				u16 queue, \
				const struct ieee80211_tx_queue_params *params
#define MT792X_CONF_TX_ARGS	hw, vif, queue, params
#endif

/* .set_coverage_class callback */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define MT792X_SET_COVERAGE_PROTO	struct ieee80211_hw *hw, int radio_idx, \
					s16 coverage_class
#else
#define MT792X_SET_COVERAGE_PROTO	struct ieee80211_hw *hw, \
					s16 coverage_class
#endif

/* .set_rts_threshold callback */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define MT792X_SET_RTS_PROTO	struct ieee80211_hw *hw, int radio_idx, u32 val
#else
#define MT792X_SET_RTS_PROTO	struct ieee80211_hw *hw, u32 val
#endif

/* .set_antenna callback */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define MT792X_SET_ANTENNA_PROTO	struct ieee80211_hw *hw, int radio_idx, \
					u32 tx_ant, u32 rx_ant
#else
#define MT792X_SET_ANTENNA_PROTO	struct ieee80211_hw *hw, \
					u32 tx_ant, u32 rx_ant
#endif

/* .assign_vif_chanctx / .unassign_vif_chanctx callbacks */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define MT792X_ASSIGN_CHANCTX_PROTO	struct ieee80211_hw *hw, \
					struct ieee80211_vif *vif, \
					struct ieee80211_bss_conf *link_conf, \
					struct ieee80211_chanctx_conf *ctx
#define MT792X_UNASSIGN_CHANCTX_PROTO	struct ieee80211_hw *hw, \
					struct ieee80211_vif *vif, \
					struct ieee80211_bss_conf *link_conf, \
					struct ieee80211_chanctx_conf *ctx
#else
#define MT792X_ASSIGN_CHANCTX_PROTO	struct ieee80211_hw *hw, \
					struct ieee80211_vif *vif, \
					struct ieee80211_chanctx_conf *ctx
#define MT792X_UNASSIGN_CHANCTX_PROTO	struct ieee80211_hw *hw, \
					struct ieee80211_vif *vif, \
					struct ieee80211_chanctx_conf *ctx
#endif

/* .start_ap / .stop_ap callbacks */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define MT792X_START_AP_PROTO	struct ieee80211_hw *hw, \
				struct ieee80211_vif *vif, \
				struct ieee80211_bss_conf *link_conf
#define MT792X_STOP_AP_PROTO	struct ieee80211_hw *hw, \
				struct ieee80211_vif *vif, \
				struct ieee80211_bss_conf *link_conf
#else
#define MT792X_START_AP_PROTO	struct ieee80211_hw *hw, \
				struct ieee80211_vif *vif
#define MT792X_STOP_AP_PROTO	struct ieee80211_hw *hw, \
				struct ieee80211_vif *vif
#endif

/* .abort_channel_switch callback */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define MT792X_ABORT_CS_PROTO	struct ieee80211_hw *hw, \
				struct ieee80211_vif *vif, \
				struct ieee80211_bss_conf *link_conf
#else
#define MT792X_ABORT_CS_PROTO	struct ieee80211_hw *hw, \
				struct ieee80211_vif *vif
#endif

/* .mgd_prepare_tx callback */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define MT792X_MGD_PREP_TX_PROTO	struct ieee80211_hw *hw, \
					struct ieee80211_vif *vif, \
					struct ieee80211_prep_tx_info *info
#else
#define MT792X_MGD_PREP_TX_PROTO	struct ieee80211_hw *hw, \
					struct ieee80211_vif *vif, \
					u16 duration
#endif

/* ========================================================================
 * Section 10: ieee80211_link_sta compat for mt792x_sta_to_link_sta()
 *
 * In 6.13+, mt792x_sta_to_link_sta() returns struct ieee80211_link_sta*
 * In 6.12, there's no ieee80211_link_sta — fields are on ieee80211_sta
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Use native link_sta API */
static inline struct ieee80211_link_sta *
mt792x_sta_to_link_sta_compat(struct ieee80211_vif *vif,
			       struct ieee80211_sta *sta,
			       unsigned int link_id)
{
	if (!ieee80211_vif_is_mld(vif) ||
	    link_id >= IEEE80211_LINK_UNSPECIFIED)
		return &sta->deflink;

	return link_sta_dereference_protected(sta, link_id);
}
#else
/* In 6.12, we don't have ieee80211_link_sta at all.
 * Return NULL — callers must use the STA_* accessor macros instead
 * of directly dereferencing link_sta fields.
 */
static inline void *
mt792x_sta_to_link_sta_compat(struct ieee80211_vif *vif,
			       struct ieee80211_sta *sta,
			       unsigned int link_id)
{
	return sta;  /* Return sta itself for identification; callers use STA_* macros */
}
#endif

/* ========================================================================
 * Section 11: struct ieee80211_link_sta pointer in driver structs
 *
 * In mt792x_link_sta, the pri_link field uses struct ieee80211_link_sta*
 * which doesn't exist in 6.12. We need to handle this.
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Use native struct ieee80211_link_sta */
#else
/* In 6.12, we use void* as a stand-in for ieee80211_link_sta*.
 * Code that dereferences this should use STA_* accessor macros.
 */
#define ieee80211_link_sta	void
#endif

/* ========================================================================
 * Section 12: link_conf->link_id — doesn't exist in 6.12
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define LINK_CONF_ID(link_conf)	((link_conf)->link_id)
#else
#define LINK_CONF_ID(link_conf)	(0)
#endif

/* ========================================================================
 * Section 13: vif->link_conf[] array — 6.13+ MLO only
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native — use vif->link_conf[link_id] directly */
#else
/* In 6.12, there's no link_conf array. Access bss_conf directly. */
static inline struct ieee80211_bss_conf *
mt792x_vif_link_conf(struct ieee80211_vif *vif, unsigned int link_id)
{
	return &vif->bss_conf;
}
#define vif_link_conf(vif, lid)	mt792x_vif_link_conf(vif, lid)
#endif

/* ========================================================================
 * Section 14: sta->link[] array — 6.13+ MLO only
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native — use sta->link[link_id] directly */
#else
/* In 6.12, there's no link array on sta. Return sta directly. */
#endif

/* ========================================================================
 * Section 15: IEEE80211_TX_CTRL_MLO_LINK — 6.13+ only
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native */
#else
#define IEEE80211_TX_CTRL_MLO_LINK	0
#endif

/* ========================================================================
 * Section 16: HW flags not present in 6.12
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native — all flags available */
#else
/* These IEEE80211_HW flags were added in 6.13 */
#ifndef IEEE80211_HW_SUPPORTS_MULTI_BSSID
#define IEEE80211_HW_SUPPORTS_MULTI_BSSID	__IEEE80211_HW_NUMBER
#endif
#ifndef IEEE80211_HW_SUPPORTS_ONLY_HE_MULTI_BSSID
#define IEEE80211_HW_SUPPORTS_ONLY_HE_MULTI_BSSID	(__IEEE80211_HW_NUMBER + 1)
#endif
#ifndef IEEE80211_HW_CHANCTX_STA_CSA
#define IEEE80211_HW_CHANCTX_STA_CSA	(__IEEE80211_HW_NUMBER + 2)
#endif
#ifndef IEEE80211_HW_TIMING_DEVICE
#define IEEE80211_HW_TIMING_DEVICE	(__IEEE80211_HW_NUMBER + 3)
#endif
#endif

/* ========================================================================
 * Section 17: NL80211 features not present in 6.12
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native */
#else
#ifndef NL80211_FEATURE_HW_TIMESTAMP
#define NL80211_FEATURE_HW_TIMESTAMP	0
#endif
#endif

/* ========================================================================
 * Section 18: ieee80211_emulate_* chanctx functions — 6.13+ only
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native — use ieee80211_emulate_add_chanctx etc. */
#else
/* In 6.12, these don't exist. Set chanctx ops to NULL — mac80211
 * handles the NULL case by not supporting chanctx.
 */
#define ieee80211_emulate_add_chanctx		NULL
#define ieee80211_emulate_remove_chanctx	NULL
#define ieee80211_emulate_change_chanctx	NULL
#define ieee80211_emulate_switch_vif_chanctx	NULL
#endif

/* ========================================================================
 * Section 19: struct ieee80211_vif_chanctx_switch.link_conf — 6.13+ only
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native — vifs->link_conf exists */
#else
/* In 6.12, there's no link_conf member. The struct has:
 *   struct ieee80211_vif *vif;
 *   struct ieee80211_chanctx_conf *old_ctx;
 *   struct ieee80211_chanctx_conf *new_ctx;
 * We provide a safe accessor.
 */
#endif

/* ========================================================================
 * Section 20: wiphy->n_radio — 6.13+ only
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native */
#else
/* In 6.12, there's no n_radio — single radio is assumed */
static inline unsigned int mt792x_wiphy_n_radio(struct wiphy *wiphy)
{
	return 0;
}
#define wiphy_n_radio(w)	mt792x_wiphy_n_radio(w)
#endif

/* ========================================================================
 * Section 21: kzalloc_flex — 6.13+ only
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native */
#else
#define kzalloc_flex(type, member, count) \
	kzalloc(struct_size(type, member, count), GFP_KERNEL)
#endif

/* ========================================================================
 * Section 22: page_pool_alloc_frag return type change
 *
 * In 6.12, page_pool_alloc_frag() returns void* (virtual address).
 * In 6.13+, it returns struct page*.
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native — returns struct page* */
#else
/* Returns void* directly — no page_address() needed */
#endif

/* ========================================================================
 * Section 23: wiphy->mbssid_max_interfaces — 6.13+ only
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native */
#else
static inline u32 mt792x_wiphy_mbssid_max_interfaces(struct wiphy *wiphy)
{
	return 0;
}
#define wiphy_mbssid_max_interfaces(w)	mt792x_wiphy_mbssid_max_interfaces(w)
#endif

/* ========================================================================
 * Section 24: EHT radiotap structs — 6.13+ only
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native — struct ieee80211_radiotap_tlv, ieee80211_radiotap_eht, etc. */
#else
/* These don't exist in 6.12 — any code using them must be #ifdef'd out */
#endif

/* ========================================================================
 * Section 25: mgd_complete_tx op — 6.13+ only
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native — the .mgd_complete_tx op exists in ieee80211_ops */
#else
/* In 6.12, there's no .mgd_complete_tx op — we must not assign it */
#endif

/* ========================================================================
 * Section 26: start_radar_detection / end_cac ops — 6.13+ only
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native — .start_radar_detection and .end_cac exist in ieee80211_ops */
#else
/* In 6.12, these ops don't exist. Use .set_radar_background instead,
 * or simply don't register these callbacks.
 */
#endif

#endif /* __MT792X_COMPAT_H */
