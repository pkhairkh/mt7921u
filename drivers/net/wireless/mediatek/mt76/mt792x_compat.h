/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/* Copyright (C) 2024 MediaTek Inc. */
/*
 * Kernel compatibility header for the MT7921U driver.
 *
 * The upstream mt76 driver targets kernel 6.13+ which introduced MLO
 * (Multi-Link Operation), multi-radio wiphy, and many ieee80211 API
 * changes. This header provides compat macros, wrappers, and stubs so
 * the driver compiles and runs correctly on kernel 6.12.
 *
 * IMPORTANT: The Raspberry Pi 6.12.62 kernel has BACKPORTED nearly ALL
 * 6.13+ MLO changes — struct layout, callback signatures, and API changes.
 * We detect this by checking for link_conf_dereference_protected which
 * only exists as a macro in kernels with the MLO restructuring.
 *
 * Compatibility check strategy:
 *   MT792X_USE_MLINK_API  - for mac80211 struct layout + callback signatures
 *                            (backported to RPi 6.12)
 *   LINUX_VERSION_CODE    - for features NOT backported to RPi 6.12:
 *                            - vif->cfg.p2p (still at vif->p2p)
 *                            - wiphy_n_radio(), wiphy_mbssid_max_interfaces()
 *                            - ieee80211_emulate_* chanctx functions
 *                            - kzalloc_flex (core kernel helper)
 *                            - page_pool netmem API
 */

#ifndef __MT792X_COMPAT_H
#define __MT792X_COMPAT_H

#include <linux/version.h>

/* ========================================================================
 * Section 0: Feature detection
 *
 * The RPi 6.12.62 kernel backported the MLO struct layout from 6.13+.
 * We detect this by checking for link_conf_dereference_protected which
 * is defined as a macro in mac80211.h only in kernels with MLO support.
 * ======================================================================== */

#ifdef link_conf_dereference_protected
#define MT792X_USE_MLINK_API 1
#else
#define MT792X_USE_MLINK_API 0
#endif

/* ========================================================================
 * Section 1: MLO constants and helpers
 * ======================================================================== */

#if MT792X_USE_MLINK_API
/* Kernel provides these natively */
#else
#ifndef IEEE80211_MLD_MAX_NUM_LINKS
#define IEEE80211_MLD_MAX_NUM_LINKS 1
#endif

#ifndef IEEE80211_LINK_UNSPECIFIED
#define IEEE80211_LINK_UNSPECIFIED 0xff
#endif

/* ieee80211_vif_is_mld() doesn't exist - always false */
static inline bool ieee80211_vif_is_mld(struct ieee80211_vif *vif)
{
        return false;
}

/* link_conf_dereference_protected() doesn't exist */
static inline struct ieee80211_bss_conf *
link_conf_dereference_protected(struct ieee80211_vif *vif, unsigned int link_id)
{
        return &vif->bss_conf;
}
#endif /* !MT792X_USE_MLINK_API */

/* ========================================================================
 * Section 2: sta->deflink compat accessor macros
 *
 * In MLO kernels, struct ieee80211_sta has a 'deflink' member of type
 * struct ieee80211_link_sta that wraps HT/VHT/HE caps, bandwidth, etc.
 * In non-MLO kernels, those fields exist directly on struct ieee80211_sta.
 * ======================================================================== */

#if MT792X_USE_MLINK_API
#define STA_HT_CAP(sta)         ((sta)->deflink.ht_cap)
#define STA_VHT_CAP(sta)                ((sta)->deflink.vht_cap)
#define STA_HE_CAP(sta)         ((sta)->deflink.he_cap)
#define STA_HE_6GHZ_CAPA(sta)           ((sta)->deflink.he_6ghz_capa)
#define STA_BANDWIDTH(sta)              ((sta)->deflink.bandwidth)
#define STA_RX_NSS(sta)         ((sta)->deflink.rx_nss)
#define STA_SMPS_MODE(sta)              ((sta)->deflink.smps_mode)
#define STA_SUPP_RATES(sta)             ((sta)->deflink.supp_rates)
#define STA_ADDR(sta)                   ((sta)->deflink.addr)
#define STA_AID(sta)                    ((sta)->deflink.sta->aid)
#define STA_WME(sta)                    ((sta)->deflink.sta->wme)
#define STA_AGG_MAX_AMSDU_LEN(sta)      ((sta)->deflink.agg.max_amsdu_len)
#else
#define STA_HT_CAP(sta)         ((sta)->ht_cap)
#define STA_VHT_CAP(sta)                ((sta)->vht_cap)
#define STA_HE_CAP(sta)         ((sta)->he_cap)
#define STA_HE_6GHZ_CAPA(sta)           ((sta)->he_6ghz_capa)
#define STA_BANDWIDTH(sta)              ((sta)->bandwidth)
#define STA_RX_NSS(sta)         ((sta)->rx_nss)
#define STA_SMPS_MODE(sta)              ((sta)->smps_mode)
#define STA_SUPP_RATES(sta)             ((sta)->supp_rates)
#define STA_ADDR(sta)                   ((sta)->addr)
#define STA_AID(sta)                    ((sta)->aid)
#define STA_WME(sta)                    ((sta)->wme)
#define STA_AGG_MAX_AMSDU_LEN(sta)      ((sta)->max_amsdu_len)
#endif

/* ========================================================================
 * Section 3: vif->cfg vs vif->bss_conf compat
 *
 * In MLO kernels, some fields moved from vif->bss_conf to vif->cfg:
 *   assoc, aid, ps, arp_addr_cnt, arp_addr_list
 *
 * NOTE: p2p is different! It moved from vif->p2p to vif->cfg.p2p ONLY
 * in 6.13+. The RPi 6.12 kernel has vif->cfg (backported MLO struct
 * layout) but kept p2p at vif->p2p — it was NOT moved to cfg.
 * ======================================================================== */

#if MT792X_USE_MLINK_API
#define VIF_ASSOC(vif)                  ((vif)->cfg.assoc)
#define VIF_AID(vif)                    ((vif)->cfg.aid)
#define VIF_PS(vif)                     ((vif)->cfg.ps)
#define VIF_ARP_ADDR_CNT(vif)           ((vif)->cfg.arp_addr_cnt)
#define VIF_ARP_ADDR_LIST(vif)          ((vif)->cfg.arp_addr_list)
#else
#define VIF_ASSOC(vif)                  ((vif)->bss_conf.assoc)
#define VIF_AID(vif)                    ((vif)->bss_conf.aid)
#define VIF_PS(vif)                     ((vif)->bss_conf.ps)
#define VIF_ARP_ADDR_CNT(vif)           ((vif)->bss_conf.arp_addr_cnt)
#define VIF_ARP_ADDR_LIST(vif)          ((vif)->bss_conf.arp_addr_list)
#endif

/* p2p: moved from vif->p2p to vif->cfg.p2p in 6.13+ (NOT backported to RPi 6.12) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define VIF_P2P(vif)                    ((vif)->cfg.p2p)
#else
#define VIF_P2P(vif)                    ((vif)->p2p)
#endif

/* ========================================================================
 * Section 4: chanreq vs chandef compat
 * ======================================================================== */

#if MT792X_USE_MLINK_API
#define VIF_CHANDEF(vif)                ((vif)->bss_conf.chanreq.oper)
#else
#define VIF_CHANDEF(vif)                ((vif)->bss_conf.chandef)
#endif

/* ========================================================================
 * Section 5: link_sta type for function parameters
 * ======================================================================== */

#if MT792X_USE_MLINK_API
#define MT792X_LINK_STA_PTR     struct ieee80211_link_sta *
#else
#define MT792X_LINK_STA_PTR     struct ieee80211_sta *
#endif

/* ========================================================================
 * Section 6: ieee80211 API signature differences (backported to RPi 6.12)
 * ======================================================================== */

/* ieee80211_set_sband_iftype_data: In 6.12, this is a 2-arg MACRO that
 * internally calls _ieee80211_set_sband_iftype_data(sband, iftd, ARRAY_SIZE(iftd)).
 * Since we often pass a pointer (not an array), ARRAY_SIZE fails. We must
 * call the 3-arg _ieee80211_set_sband_iftype_data function directly.
 * In 6.13+, ieee80211_set_sband_iftype_data is a 3-arg function directly.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define mt792x_set_sband_iftype_data(band, data, n) \
        ieee80211_set_sband_iftype_data(band, data, n)
#else
#define mt792x_set_sband_iftype_data(band, data, n) \
        _ieee80211_set_sband_iftype_data(band, data, n)
#endif

/* ieee80211_radar_detected: 1 arg in stock 6.12, 2 args with chanctx_conf in
 * RPi 6.12 backport, 2 args with vif in 6.13+. The RPi 6.12 backport added a
 * 2nd chanctx_conf arg but NOT the vif arg. Pass NULL for chanctx_conf.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define mt792x_radar_detected(hw, vif) \
        ieee80211_radar_detected(hw, vif)
#elif MT792X_USE_MLINK_API
/* RPi 6.12 backport: 2 args but 2nd is chanctx_conf, not vif */
#define mt792x_radar_detected(hw, vif) \
        ieee80211_radar_detected(hw, NULL)
#else
#define mt792x_radar_detected(hw, vif) \
        ieee80211_radar_detected(hw)
#endif

/* ieee80211_chswitch_done: 2 args in 6.12, 3 in 6.13+ */
#if MT792X_USE_MLINK_API
#define mt792x_chswitch_done(vif, success, link_id) \
        ieee80211_chswitch_done(vif, success, link_id)
#else
#define mt792x_chswitch_done(vif, success, link_id) \
        ieee80211_chswitch_done(vif, success)
#endif

/* ========================================================================
 * Section 7: ieee80211_cac_finish - doesn't exist in 6.12
 *
 * In 6.12, cfg80211_cac_event() takes struct net_device* first.
 * ======================================================================== */

/* ieee80211_cac_finish: only exists in 6.13+ kernels.
 * RPi 6.12 backported MLO structs but NOT this function.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
#define mt792x_cac_finish(hw, vif) \
        ieee80211_cac_finish(hw, vif)
#else
static inline void mt792x_cac_finish(struct ieee80211_hw *hw,
                                      struct ieee80211_vif *vif)
{
        struct wireless_dev *wdev = ieee80211_vif_to_wdev(vif);
        struct net_device *netdev;
        struct cfg80211_chan_def chandef = {};

        if (!wdev)
                return;
        netdev = wdev->netdev;
        if (!netdev)
                return;

        chandef = VIF_CHANDEF(vif);
        cfg80211_cac_event(netdev, &chandef,
                           NL80211_RADAR_CAC_FINISHED, GFP_KERNEL,
                           0 /* link_id */);
}
#endif

/* ========================================================================
 * Section 8: ieee80211_refresh_tx_agg_session_timer
 * Already exists in 6.12 with u16 tid. Do NOT redefine.
 * ======================================================================== */

/* ========================================================================
 * Section 9: Callback signature wrappers for ieee80211_ops
 * These function signature changes were NOT backported to RPi 6.12.
 * ======================================================================== */

#if MT792X_USE_MLINK_API
#define MT792X_STOP_PROTO       struct ieee80211_hw *hw, bool suspend
#define MT792X_STOP_ARGS        hw, suspend
#define MT792X_STOP_CALL(fn, hw)        fn(hw, false)
#else
#define MT792X_STOP_PROTO       struct ieee80211_hw *hw
#define MT792X_STOP_ARGS        hw
#define MT792X_STOP_CALL(fn, hw)        fn(hw)
#endif

#if MT792X_USE_MLINK_API
#define MT792X_CONFIG_PROTO     struct ieee80211_hw *hw, int radio_idx, u32 changed
#define MT792X_CONFIG_ARGS      hw, radio_idx, changed
#else
#define MT792X_CONFIG_PROTO     struct ieee80211_hw *hw, u32 changed
#define MT792X_CONFIG_ARGS      hw, changed
#endif

#if MT792X_USE_MLINK_API
#define MT792X_CONF_TX_PROTO    struct ieee80211_hw *hw, struct ieee80211_vif *vif, \
                                unsigned int link_id, u16 queue, \
                                const struct ieee80211_tx_queue_params *params
#define MT792X_CONF_TX_ARGS     hw, vif, link_id, queue, params
#else
#define MT792X_CONF_TX_PROTO    struct ieee80211_hw *hw, struct ieee80211_vif *vif, \
                                u16 queue, \
                                const struct ieee80211_tx_queue_params *params
#define MT792X_CONF_TX_ARGS     hw, vif, queue, params
#endif

#if MT792X_USE_MLINK_API
#define MT792X_SET_COVERAGE_PROTO       struct ieee80211_hw *hw, int radio_idx, \
                                        s16 coverage_class
#else
#define MT792X_SET_COVERAGE_PROTO       struct ieee80211_hw *hw, \
                                        s16 coverage_class
#endif

#if MT792X_USE_MLINK_API
#define MT792X_SET_RTS_PROTO    struct ieee80211_hw *hw, int radio_idx, u32 val
#else
#define MT792X_SET_RTS_PROTO    struct ieee80211_hw *hw, u32 val
#endif

#if MT792X_USE_MLINK_API
#define MT792X_SET_ANTENNA_PROTO        struct ieee80211_hw *hw, int radio_idx, \
                                        u32 tx_ant, u32 rx_ant
#else
#define MT792X_SET_ANTENNA_PROTO        struct ieee80211_hw *hw, \
                                        u32 tx_ant, u32 rx_ant
#endif

#if MT792X_USE_MLINK_API
#define MT792X_ASSIGN_CHANCTX_PROTO     struct ieee80211_hw *hw, \
                                        struct ieee80211_vif *vif, \
                                        struct ieee80211_bss_conf *link_conf, \
                                        struct ieee80211_chanctx_conf *ctx
#define MT792X_UNASSIGN_CHANCTX_PROTO   struct ieee80211_hw *hw, \
                                        struct ieee80211_vif *vif, \
                                        struct ieee80211_bss_conf *link_conf, \
                                        struct ieee80211_chanctx_conf *ctx
#else
#define MT792X_ASSIGN_CHANCTX_PROTO     struct ieee80211_hw *hw, \
                                        struct ieee80211_vif *vif, \
                                        struct ieee80211_chanctx_conf *ctx
#define MT792X_UNASSIGN_CHANCTX_PROTO   struct ieee80211_hw *hw, \
                                        struct ieee80211_vif *vif, \
                                        struct ieee80211_chanctx_conf *ctx
#endif

#if MT792X_USE_MLINK_API
#define MT792X_START_AP_PROTO   struct ieee80211_hw *hw, \
                                struct ieee80211_vif *vif, \
                                struct ieee80211_bss_conf *link_conf
#define MT792X_STOP_AP_PROTO    struct ieee80211_hw *hw, \
                                struct ieee80211_vif *vif, \
                                struct ieee80211_bss_conf *link_conf
#else
#define MT792X_START_AP_PROTO   struct ieee80211_hw *hw, \
                                struct ieee80211_vif *vif
#define MT792X_STOP_AP_PROTO    struct ieee80211_hw *hw, \
                                struct ieee80211_vif *vif
#endif

#if MT792X_USE_MLINK_API
#define MT792X_ABORT_CS_PROTO   struct ieee80211_hw *hw, \
                                struct ieee80211_vif *vif, \
                                struct ieee80211_bss_conf *link_conf
#else
#define MT792X_ABORT_CS_PROTO   struct ieee80211_hw *hw, \
                                struct ieee80211_vif *vif
#endif

#if MT792X_USE_MLINK_API
#define MT792X_MGD_PREP_TX_PROTO        struct ieee80211_hw *hw, \
                                        struct ieee80211_vif *vif, \
                                        struct ieee80211_prep_tx_info *info
#else
#define MT792X_MGD_PREP_TX_PROTO        struct ieee80211_hw *hw, \
                                        struct ieee80211_vif *vif, \
                                        u16 duration
#endif

/* ========================================================================
 * Section 10: link_sta compat
 * ======================================================================== */

#if MT792X_USE_MLINK_API
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
static inline void *
mt792x_sta_to_link_sta_compat(struct ieee80211_vif *vif,
                               struct ieee80211_sta *sta,
                               unsigned int link_id)
{
        return sta;
}
#endif

/* ========================================================================
 * Section 11: struct ieee80211_link_sta pointer in driver structs
 * ======================================================================== */

#if MT792X_USE_MLINK_API
/* Use native struct ieee80211_link_sta */
#else
#define ieee80211_link_sta      void
#endif

/* ========================================================================
 * Section 12: link_conf->link_id
 * ======================================================================== */

#if MT792X_USE_MLINK_API
#define LINK_CONF_ID(link_conf) ((link_conf)->link_id)
#else
#define LINK_CONF_ID(link_conf) (0)
#endif

/* ========================================================================
 * Section 13: vif->link_conf[] array
 * ======================================================================== */

#if MT792X_USE_MLINK_API
/* Native */
#else
static inline struct ieee80211_bss_conf *
mt792x_vif_link_conf(struct ieee80211_vif *vif, unsigned int link_id)
{
        return &vif->bss_conf;
}
#define vif_link_conf(vif, lid) mt792x_vif_link_conf(vif, lid)
#endif

/* ========================================================================
 * Section 14: IEEE80211_TX_CTRL_MLO_LINK
 * ======================================================================== */

#if MT792X_USE_MLINK_API
/* Native */
#else
#define IEEE80211_TX_CTRL_MLO_LINK      0
#endif

/* ========================================================================
 * Section 15: HW flags - IEEE80211_HW_SUPPORTS_MULTI_BSSID doesn't
 * exist in 6.12 and __IEEE80211_HW_NUMBER doesn't exist either.
 * Don't define it - code must use #if guards.
 * ======================================================================== */

/* ========================================================================
 * Section 16: NL80211 features
 *
 * NL80211_FEATURE_HW_TIMESTAMP was introduced in 6.13+ but was NOT
 * backported to RPi 6.12 even though MLO was. Use unconditional #ifndef
 * so the fallback define works regardless of MT792X_USE_MLINK_API.
 * Code should use #ifdef NL80211_FEATURE_HW_TIMESTAMP to guard usage.
 * ======================================================================== */

#ifndef NL80211_FEATURE_HW_TIMESTAMP
#define NL80211_FEATURE_HW_TIMESTAMP    0
#endif

/* ========================================================================
 * Section 17: ieee80211_emulate_* chanctx functions - 6.13+ only
 * NOT backported to RPi 6.12 (separate from MLO struct layout)
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native */
#else
#define ieee80211_emulate_add_chanctx           NULL
#define ieee80211_emulate_remove_chanctx                NULL
#define ieee80211_emulate_change_chanctx                NULL
#define ieee80211_emulate_switch_vif_chanctx    NULL
#endif

/* ========================================================================
 * Section 18: wiphy->n_radio - 6.13+ only
 * NOT backported to RPi 6.12 (separate from MLO struct layout)
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native */
#else
static inline unsigned int mt792x_wiphy_n_radio(struct wiphy *wiphy)
{
        return 0;
}
#define wiphy_n_radio(w)        mt792x_wiphy_n_radio(w)
#endif

/* ========================================================================
 * Section 19: kzalloc_flex - 6.13+ only
 * NOT backported to RPi 6.12 (core kernel helper, not mac80211)
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native */
#else
#define kzalloc_flex(type, member, count) \
        kzalloc(struct_size(type, member, count), GFP_KERNEL)
#endif

/* ========================================================================
 * Section 20: wiphy->mbssid_max_interfaces - 6.13+ only
 * NOT backported to RPi 6.12 (separate from MLO struct layout)
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native */
#else
static inline u32 mt792x_wiphy_mbssid_max_interfaces(struct wiphy *wiphy)
{
        return 0;
}
#define wiphy_mbssid_max_interfaces(w)  mt792x_wiphy_mbssid_max_interfaces(w)
#endif

/* ========================================================================
 * Section 21: mgd_complete_tx op - 6.13+ only
 * ======================================================================== */

#if MT792X_USE_MLINK_API
/* Native */
#else
/* No .mgd_complete_tx op - must not assign it */
#endif

/* ========================================================================
 * Section 22: start_radar_detection / end_cac ops - 6.13+ only
 * ======================================================================== */

#if MT792X_USE_MLINK_API
/* Native */
#else
/* Use .set_radar_background or don't register these callbacks */
#endif

/* ========================================================================
 * Section 23: Feature constants NOT backported to RPi 6.12
 *
 * MT792X_USE_MLINK_API detects MLO struct layout, but is NOT a reliable
 * indicator for individual feature constants. Code should use:
 *   - #ifdef SYMBOL for #define'd constants
 *   - LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0) for enum values
 *
 * NOT backported to RPi 6.12.62+rpt-rpi-2712:
 *   - NETIF_F_HW_HWTSTAMP           (netdev feature #define → #ifdef)
 *   - WIPHY_FLAG_HAS_RADAR_DETECT   (wiphy flag #define → #ifdef)
 *   - NL80211_FEATURE_HW_TIMESTAMP  (nl80211 feature #define → #ifdef)
 *   - IEEE80211_HW_TIMING_DEVICE    (enum value → version check)
 *
 * Backported to RPi 6.12.62+rpt-rpi-2712:
 *   - IEEE80211_HW_SUPPORTS_MULTI_BSSID       (enum value)
 *   - IEEE80211_HW_SUPPORTS_ONLY_HE_MULTI_BSSID (enum value)
 *   - IEEE80211_HW_CHANCTX_STA_CSA            (enum value)
 * ======================================================================== */

/* ========================================================================
 * Section 24: system_percpu_wq - 6.13+ only
 *
 * In 6.13, system_percpu_wq was introduced as a per-CPU workqueue.
 * In 6.12, use system_wq as the equivalent fallback.
 * ======================================================================== */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,13,0)
/* Native */
#else
#define system_percpu_wq        system_wq
#endif

#endif /* __MT792X_COMPAT_H */
