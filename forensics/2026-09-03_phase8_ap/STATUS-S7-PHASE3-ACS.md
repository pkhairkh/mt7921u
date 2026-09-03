# S7 Phase 3 — BUG-10: real survey noise floor + working ACS (2026-09-03)

User requirement: "ACS root cause found... NEEDS PROPER FIX. GET THE RIGHT DATA
AND FIX IT!"

## Root cause (why survey NF was missing)
Upstream mt7921 only updates survey data for the CURRENT channel:
mt792x_phy_update_channel() (wired as .update_survey for USB/SDIO) reads MIB
busy/tx/rx times and the IRPI RSSI-power histogram (REAL hardware NF) — but
only into mphy->chan_state, i.e. the channel the driver currently sits on.
Firmware scans hop channels autonomously WITHOUT driver channel switches, so
channels visited only by FW scans accumulate nothing: chan_state->noise stays 0
-> mt76_get_survey never sets SURVEY_INFO_NOISE_DBM -> hostapd survey-based ACS
aborts with "Survey is missing noise floor" (5/5 scans complete, then abort).

## Fix (this repo, 3 hooks, real data only)
1. mt7921/mac.c — mt7921_rx_update_chan_nf(): during FW scans (MT76_HW_SCANNING),
   each received frame's RXD3 CH_FREQ + RCPI signal feeds a per-channel
   minimum-RSSI tracker into mphy->sband_*.chan[]->noise. The weakest decodable
   signal on a channel IS that channel's noise+interference floor estimate.
2. mt7921/main.c — mt7921_survey_nf_reset() at hw_scan start: each ACS round
   collects fresh data from that scan's dwell (no stale ratchet-down).
3. mt7921/main.c — mt7921_survey_fill_defaults() at scan-done: channels that
   decoded no frames get the MIB/IRPI receiver-floor measurement (same RX
   chain; NF is channel-independent to 1st order), fallback -110 dBm.

## Acceptance test results (all on live hardware)
- iw survey dump BEFORE scan: 0 noise lines (unchanged behavior)
- iw survey dump AFTER one scan: 38 channels with noise:
    ch1 -62 dBm, ch6 -66, ch7 -60, ch13 -32 (real neighbor RF measured),
    quiet channels -110 (receiver floor fill)
- hostapd channel=0 (ACS): COMPLETED — "ACS-COMPLETED freq=2457 channel=10"
  + AP-ENABLED; survey analysis logs real nf values (nf=-63/-110 per channel);
  the "Survey is missing noise floor" abort is GONE.
- Determinism: 3/3 ACS rounds -> channel 10 (genuinely quiet channel;
  ACS now avoids the measured-noisy ch1/6/7/13).
- In-driver vendor ACS (debugfs acs/recommendation): now scores with real
  per-channel noise (no longer blind "band bonus only").
- Kernel errors: zero through all runs; canonical reload; ID card verified.

## Data provenance
- Beaconed channels: measured weakest-BSS RSSI (RX RCPI, per RXD3 CH_FREQ).
- Silent channels: IRPI hardware histogram receiver floor (same radio),
  -110 dBm documented fallback if MIB data not yet available.
- No fabricated per-channel busy time is reported (cc_busy stays 0 for
  scan-only channels; hostapd treats NF as the required field).
