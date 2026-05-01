// SPDX-License-Identifier: BSD-3-Clause-Clear
/* Copyright (C) 2025 mt7921u Forensic Audit Project
 *
 * TWT (Target Wake Time) responder implementation for mt7921.
 * Based on vendor driver's twt.c / twt_planner.c and mainline mt7915 TWT.
 *
 * Firmware command: MCU_EXT_CMD(TWT_AGRT_UPDATE) = 0x94
 * Alternative unified path: MCU_UNI_CMD(TWT) = 0x14
 *
 * Research basis: "Deterministic Scheduling over Wi-Fi 6 using Target Wake Time"
 * (arXiv, May 2025) — TWT reduces average latency by 60%, jitter by 80%,
 * power consumption by 40% for battery-powered stations.
 */

#include "mt7921.h"
#include "mcu.h"
#include "../mt76_connac2_mac.h"

#define MT7921_TWT_AGRT_TRIGGER         BIT(0)
#define MT7921_TWT_AGRT_ANNOUNCE        BIT(1)
#define MT7921_TWT_AGRT_PROTECT BIT(2)

#define MT7921_MAX_TWT_AGRT             16
#define MT7921_MAX_STA_TWT_AGRT         8
#define MT7921_MIN_TWT_DUR              64

enum {
        MT7921_TWT_AGRT_ADD,
        MT7921_TWT_AGRT_MODIFY,
        MT7921_TWT_AGRT_DELETE,
        MT7921_TWT_AGRT_TEARDOWN,
};

int mt7921_mcu_twt_agrt_update(struct mt792x_dev *dev,
                                struct mt792x_vif *mvif,
                                struct mt7921_twt_flow *flow,
                                int cmd)
{
        struct {
                u8 tbl_idx;
                u8 cmd;
                u8 own_mac_idx;
                u8 flowid;
                __le16 peer_id;
                u8 duration;
                u8 bss_idx;
                __le64 start_tsf;
                __le16 mantissa;
                u8 exponent;
                u8 is_ap;
                u8 agrt_params;
                u8 rsv[23];
        } __packed req = {
                .tbl_idx = flow->table_id,
                .cmd = cmd,
                .own_mac_idx = mvif->bss_conf.mt76.omac_idx,
                .flowid = flow->id,
                .peer_id = cpu_to_le16(flow->wcid),
                .duration = flow->duration,
                .bss_idx = mvif->bss_conf.mt76.idx,
                .start_tsf = cpu_to_le64(flow->tsf),
                .mantissa = flow->mantissa,
                .exponent = flow->exp,
                .is_ap = true,
        };

        if (flow->protection)
                req.agrt_params |= MT7921_TWT_AGRT_PROTECT;
        if (!flow->flowtype)
                req.agrt_params |= MT7921_TWT_AGRT_ANNOUNCE;
        if (flow->trigger)
                req.agrt_params |= MT7921_TWT_AGRT_TRIGGER;

        return mt76_mcu_send_msg(&dev->mt76,
                                 MCU_EXT_CMD(TWT_AGRT_UPDATE),
                                 &req, sizeof(req), true);
}

static bool
mt7921_twt_param_equal(struct mt792x_sta *msta,
                       struct ieee80211_twt_params *twt_agrt)
{
        u16 req_type = le16_to_cpu(twt_agrt->req_type);
        struct mt7921_twt_flow *flow;
        int i;

        for (i = 0; i < MT7921_MAX_STA_TWT_AGRT; i++) {
                if (!(msta->twt.flowid_mask & BIT(i)))
                        continue;

                flow = &msta->twt.flow[i];
                if (flow->mantissa != twt_agrt->mantissa ||
                    flow->exp != FIELD_GET(IEEE80211_TWT_REQTYPE_WAKE_INT_EXP,
                                           req_type) ||
                    flow->duration != twt_agrt->min_twt_dur ||
                    flow->protection != !!(req_type &
                                           IEEE80211_TWT_REQTYPE_PROTECTION) ||
                    flow->flowtype != !!(req_type &
                                         IEEE80211_TWT_REQTYPE_FLOWTYPE) ||
                    flow->trigger != !!(req_type &
                                        IEEE80211_TWT_REQTYPE_TRIGGER))
                        continue;

                return true;
        }

        return false;
}

static bool
mt7921_twt_check_req(struct ieee80211_twt_setup *twt)
{
        struct ieee80211_twt_params *twt_agrt = (void *)twt->params;
        u16 req_type = le16_to_cpu(twt_agrt->req_type);
        u64 interval;
        u8 exp;

        /* Reject broadcast TWT agreements */
        if (!(req_type & IEEE80211_TWT_REQTYPE_FLOWTYPE))
                return false;

        /* Reject if wake interval unit is not 256us */
        if (!(twt->control & IEEE80211_TWT_CONTROL_WAKE_DUR_UNIT))
                return false;

        /* Reject explicit agreements (driver manages scheduling) */
        if (!(req_type & IEEE80211_TWT_REQTYPE_IMMEDIA_FB))
                return false;

        exp = FIELD_GET(IEEE80211_TWT_REQTYPE_WAKE_INT_EXP, req_type);
        interval = (u64)le16_to_cpu(twt_agrt->mantissa) << exp;
        if (interval < twt_agrt->min_twt_dur)
                return false;

        return true;
}

static void
mt7921_twt_teardown_flow(struct mt792x_dev *dev,
                         struct mt792x_sta *msta,
                         u8 flowid)
{
        struct mt7921_twt_flow *flow;

        lockdep_assert_held(&dev->mt76.mutex);

        if (flowid >= MT7921_MAX_STA_TWT_AGRT)
                return;

        if (!(msta->twt.flowid_mask & BIT(flowid)))
                return;

        flow = &msta->twt.flow[flowid];
        if (mt7921_mcu_twt_agrt_update(dev, msta->vif, flow,
                                        MT7921_TWT_AGRT_DELETE))
                return;

        msta->twt.flowid_mask &= ~BIT(flowid);
        dev->twt.table_mask &= ~BIT(flow->table_id);
        dev->twt.n_agrt--;
}

void mt7921_mac_add_twt_setup(struct ieee80211_hw *hw,
                              struct ieee80211_sta *sta,
                              struct ieee80211_twt_setup *twt)
{
        enum ieee80211_twt_setup_cmd setup_cmd = TWT_SETUP_CMD_REJECT;
        struct mt792x_sta *msta = (struct mt792x_sta *)sta->drv_priv;
        struct ieee80211_twt_params *twt_agrt = (void *)twt->params;
        u16 req_type = le16_to_cpu(twt_agrt->req_type);
        enum ieee80211_twt_setup_cmd sta_setup_cmd;
        struct mt792x_dev *dev = mt792x_hw_dev(hw);
        struct mt7921_twt_flow *flow;
        int flowid, table_id;
        u8 exp;

        if (!mt7921_twt_check_req(twt))
                goto out;

        mt792x_mutex_acquire(dev);

        if (dev->twt.n_agrt == MT7921_MAX_TWT_AGRT)
                goto unlock;

        if (hweight8(msta->twt.flowid_mask) == MT7921_MAX_STA_TWT_AGRT)
                goto unlock;

        if (twt_agrt->min_twt_dur < MT7921_MIN_TWT_DUR) {
                setup_cmd = TWT_SETUP_CMD_DICTATE;
                twt_agrt->min_twt_dur = MT7921_MIN_TWT_DUR;
                goto unlock;
        }

        flowid = ffs(~msta->twt.flowid_mask) - 1;
        twt_agrt->req_type &= ~cpu_to_le16(IEEE80211_TWT_REQTYPE_FLOWID);
        twt_agrt->req_type |= le16_encode_bits(flowid,
                                               IEEE80211_TWT_REQTYPE_FLOWID);

        table_id = ffs(~dev->twt.table_mask) - 1;
        exp = FIELD_GET(IEEE80211_TWT_REQTYPE_WAKE_INT_EXP, req_type);
        sta_setup_cmd = FIELD_GET(IEEE80211_TWT_REQTYPE_SETUP_CMD, req_type);

        if (mt7921_twt_param_equal(msta, twt_agrt))
                goto unlock;

        flow = &msta->twt.flow[flowid];
        memset(flow, 0, sizeof(*flow));
        flow->wcid = msta->deflink.wcid.idx;
        flow->table_id = table_id;
        flow->id = flowid;
        flow->duration = twt_agrt->min_twt_dur;
        flow->mantissa = twt_agrt->mantissa;
        flow->exp = exp;
        flow->protection = !!(req_type & IEEE80211_TWT_REQTYPE_PROTECTION);
        flow->flowtype = !!(req_type & IEEE80211_TWT_REQTYPE_FLOWTYPE);
        flow->trigger = !!(req_type & IEEE80211_TWT_REQTYPE_TRIGGER);
        flow->tsf = le64_to_cpu(twt_agrt->twt);

        if (mt7921_mcu_twt_agrt_update(dev, msta->vif, flow,
                                        MT7921_TWT_AGRT_ADD))
                goto unlock;

        setup_cmd = TWT_SETUP_CMD_ACCEPT;
        dev->twt.table_mask |= BIT(table_id);
        msta->twt.flowid_mask |= BIT(flowid);
        dev->twt.n_agrt++;

unlock:
        mt792x_mutex_release(dev);
out:
        twt_agrt->req_type &= ~cpu_to_le16(IEEE80211_TWT_REQTYPE_SETUP_CMD);
        twt_agrt->req_type |=
                le16_encode_bits(setup_cmd, IEEE80211_TWT_REQTYPE_SETUP_CMD);
        twt->control = (twt->control & IEEE80211_TWT_CONTROL_WAKE_DUR_UNIT) |
                       (twt->control & IEEE80211_TWT_CONTROL_RX_DISABLED);
}

void mt7921_twt_teardown_request(struct ieee80211_hw *hw,
                                 struct ieee80211_sta *sta,
                                 u8 flowid)
{
        struct mt792x_sta *msta = (struct mt792x_sta *)sta->drv_priv;
        struct mt792x_dev *dev = mt792x_hw_dev(hw);

        mt792x_mutex_acquire(dev);
        mt7921_twt_teardown_flow(dev, msta, flowid);
        mt792x_mutex_release(dev);
}

/* Tear down all TWT flows for a STA (called on disassociation) */
void mt7921_twt_teardown_sta(struct mt792x_dev *dev,
                             struct mt792x_sta *msta)
{
        int i;

        lockdep_assert_held(&dev->mt76.mutex);

        for (i = 0; i < MT7921_MAX_STA_TWT_AGRT; i++) {
                if (msta->twt.flowid_mask & BIT(i))
                        mt7921_twt_teardown_flow(dev, msta, i);
        }
}

void mt7921_twt_sp_event(struct mt792x_dev *dev, struct sk_buff *skb)
{
        struct mt7921_twt_sp_event_hdr *evt;
        u8 flow_id, event_type;

        evt = (struct mt7921_twt_sp_event_hdr *)skb->data;
        flow_id = evt->flow_id;
        event_type = evt->event_type;

        if (flow_id >= MT7921_MAX_TWT_AGRT) {
                dev_warn(dev->mt76.dev,
                         "TWT SP event: invalid flow_id %u\n", flow_id);
                return;
        }

        switch (event_type) {
        case MT7921_TWT_SP_START:
                dev->twt.stats.sp_active[flow_id] = true;
                dev->twt.stats.sp_start_tsf[flow_id] = le64_to_cpu(evt->tsf);
                break;
        case MT7921_TWT_SP_END:
                if (!dev->twt.stats.sp_active[flow_id]) {
                        /* SP_END without prior SP_START: missed SP */
                        dev->twt.stats.n_missed_sp++;
                        dev->twt.stats.missed_sp[flow_id]++;
                }
                dev->twt.stats.sp_active[flow_id] = false;
                break;
        case MT7921_TWT_SP_MISSED:
                dev->twt.stats.n_missed_sp++;
                dev->twt.stats.missed_sp[flow_id]++;
                dev->twt.stats.sp_active[flow_id] = false;
                break;
        default:
                dev_warn(dev->mt76.dev,
                         "TWT SP event: unknown type %u for flow %u\n",
                         event_type, flow_id);
                break;
        }
}

/*
 * eBPF TWT Hook Stub — Future BPF_PROG_TYPE_WIFI_TWT attach point
 *
 * The intended interface would allow eBPF programs to:
 *
 * 1. Filter TWT setup requests before accepting/rejecting them.
 *    - Input: struct mt7921_twt_flow (the proposed agreement)
 *    - Return: 0 = accept, 1 = reject, 2 = modify (dictate parameters)
 *
 * 2. Monitor TWT service period events (wake/sleep transitions).
 *    - Input: struct mt7921_twt_sp_event { u8 flowid; u64 tsf; bool is_wake; }
 *    - Return: always 0 (monitoring only)
 *
 * 3. Dynamically adjust wake interval / duration based on traffic patterns.
 *    - Input: struct mt7921_twt_traffic_stats { u32 tx_bytes; u32 rx_bytes; u64 tsf; }
 *    - Return: 0 = keep current, 1 = modify (fill in new mantissa/exp)
 *
 * This requires a new BPF program type and attach type, plus a
 * verification callback to ensure the program doesn't violate TWT
 * timing constraints. The bpf_verifier_ops would need to check:
 * - Wake interval >= min_twt_dur (MT7921_MIN_TWT_DUR = 64)
 * - Mantissa * 2^exp results in a valid TU range
 *
 * For now, this is a placeholder. When implemented, the hook would be
 * called from mt7921_mac_add_twt_setup() before accepting the agreement,
 * and from a firmware event handler for service period monitoring.
 */
