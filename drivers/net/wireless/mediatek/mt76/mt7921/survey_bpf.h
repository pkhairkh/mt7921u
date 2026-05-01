/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/* Copyright (C) 2025 MediaTek Inc. */
/*
 * TASK-009: eBPF-Based Channel Survey and ACS — BPF Program Type Stub
 *
 * This header defines the interface for a future BPF_PROG_TYPE_WIFI_SURVEY
 * program type that would allow eBPF programs to compute optimal channel
 * selections based on per-channel survey data.
 *
 * The actual kernel BPF subsystem changes required to register a new
 * program type are not yet upstream. This header serves as:
 *   1. Documentation of the intended BPF program context and return values
 *   2. A placeholder that can be filled in when kernel support is added
 *   3. Reference for userspace BPF program development
 *
 * Research basis: arXiv 2025 papers on RL-based Wi-Fi channel selection
 * show 15-30% throughput improvement in dense environments.
 *
 * RUNTIME_VERIFY: validate with BPF programs on real hardware
 */

#ifndef __MT7921_SURVEY_BPF_H
#define __MT7921_SURVEY_BPF_H

/*
 * BPF program context for WIFI_SURVEY programs.
 *
 * When a BPF_PROG_TYPE_WIFI_SURVEY program is attached, it receives
 * this context structure containing per-channel survey data. The program
 * returns the recommended channel index (0-based), or a negative value
 * to indicate "no recommendation" (fall through to standard ACS).
 *
 * This is a stub definition — actual registration requires:
 *   1. Adding BPF_PROG_TYPE_WIFI_SURVEY to include/uapi/linux/bpf.h
 *   2. Registering the program type in kernel/bpf/syscall.c
 *   3. Adding verifier operations in kernel/bpf/verifier.c
 */
struct mt7921_survey_bpf_ctx {
	/* Channel information */
	u32 chan_idx;		/* Channel index in survey array */
	u32 center_freq;	/* Center frequency in MHz */
	u32 band;		/* NL80211_BAND_* enum value */

	/* Survey metrics from mt76_channel_state */
	u64 time_busy;		/* Channel busy time (us) */
	u64 time_ext_busy;	/* Extension channel busy time (us) */
	u64 time_rx;		/* RX time (us) */
	u64 time_tx;		/* TX time (us) */
	u64 time_scan;		/* Scan time (us) */
	u32 noise;		/* Noise floor (dBm) */

	/* BSS-specific data */
	u32 num_bss;		/* Number of BSSes on this channel */
	u32 max_rssi;		/* Strongest BSS RSSI (dBm) */

	/* Historical data (maintained by driver) */
	u64 avg_busy_pct;	/* Average busy percentage over last 10 samples */
	u64 peak_busy_pct;	/* Peak busy percentage over last 60 seconds */
};

/*
 * Return values for WIFI_SURVEY BPF programs:
 *
 *  >= 0 : Recommended channel index
 *  -1   : No recommendation (use standard mac80211 ACS)
 *  -2   : Skip this channel (exclude from consideration)
 */

/*
 * BPF attach point: called from mt76_get_survey() after populating
 * the survey_info structure. The driver would invoke:
 *
 *   if (survey_bpf_prog) {
 *       ret = BPF_PROG_RUN(survey_bpf_prog, &ctx);
 *       if (ret >= 0)
 *           recommended_chan = ret;
 *   }
 *
 * This requires:
 *   1. A bpf_prog pointer in struct mt792x_dev
 *   2. A netlink interface for attaching/detaching BPF programs
 *     (via nl80211 vendor commands)
 *   3. A debugfs interface for querying the current recommendation
 *
 * The BPF program receives survey data for ALL channels in sequence,
 * and the final return value of the last invocation is the recommended
 * channel index.
 */

#endif /* __MT7921_SURVEY_BPF_H */
