# S7 Phase 2 — TX power dBm quirk: root cause + fix (BUG-08, BUG-09)

User requirement: "also check if we can set the decibel properly... that is a
quirk in the original driver." — CONFIRMED, ROOT-CAUSED, AND FIXED.

## Root causes (both upstream mt76 bugs, verified on this tree)
- BUG-09 (the functional quirk): mt7921 NEVER implemented BSS_CHANGED_TXPOWER.
  ftrace proof (function tracer on live kernel):
      iw-12344: mt7921_bss_info_changed <- drv_link_info_changed
  iw set txpower reaches mac80211 -> bss_info_changed(BSS_CHANGED_TXPOWER) ->
  mt7921 silently ignored the flag. mt7921_config/set_tx_sar_pwr NEVER fired,
  the SET_RATE_TX_POWER MCU table never changed, RF power never changed.
  Reference implementation exists in the same tree: mt7996/main.c:1029.
- BUG-08 (the readback lie): nothing in the mt7921/connac path ever writes
  phy->txpower_cur, so .get_txpower reported 0 + 2-chain path delta
  = "txpower 3.00 dBm" garbage, permanently, regardless of any setting.
  Sibling drivers maintain it (mt7615/mcu.c:2075, mt7996/mcu.c:5352,
  mt7915/mcu.c:3390).

## Fix (this repo, 3 files, +63 lines)
- mt76.h: struct mt76_phy gains int txpower_vif (per-vif user limit, half-dBm,
  127 = unset).
- mt76_connac_mcu.c: SET_RATE_TX_POWER source now clamps the user power with
  phy->txpower_vif.
- mt7921/main.c:
  * mt7921_update_txpower_cur(): maintains mphy->txpower_cur =
    (min(user, vif, regdom, sar, sku) for current channel) - path delta.
    Called on CONF_CHANGE_POWER, CONF_CHANGE_CHANNEL, and the new
    BSS_CHANGED_TXPOWER path.
  * bss_info_changed(BSS_CHANGED_TXPOWER): stores the per-vif limit and calls
    mt7921_set_tx_sar_pwr(hw, NULL) -> full SKU reprogram + readback update.

## Verification (all on live hardware, both dongles, canonical reload)
- Readback now tracks EXACTLY (AP active, hostapd, WPA2, live traffic):
    default 23.00 dBm (channel max) -> fixed 10 -> 10.00 -> fixed 3 -> 3.00
    -> fixed 20 -> 20.00 -> auto -> 20.00 (regdom cap)
- Firmware SKU table (debugfs txpower_sku, MCU GET) user rows MOVE in exact
  half-dB units: @10dBm -> 20, @3dBm -> 6 (== 2 x dBm).
- Link quality unchanged under txpower churn: 0% loss, 130 Mbps MCS15 both
  directions, tx_failed=0 across every setting change.
- RSSI response: near-field AGC saturation (dongles on one hub) makes RSSI an
  unreliable power meter at close range (-4..-11 dBm observed); firmware table
  + readback are the authoritative proof. Power-meter grade verification
  requires physical separation or external instrumentation.
- Canonical module swap followed MODPROBE-PROCEDURE.md; ID card (running ==
  updates/ disk srcversion) verified after every reload; zero kernel errors.

## Honest caveats
- iw set txpower on an IDLE interface still won't show in readback until the
  interface becomes active (mac80211 applies vif txpower on BSS changes only).
- "auto" reports the last mac80211-computed value (regdom cap), not the chip
  channel max (23) — mac80211 semantics, documented behavior.
- RF-level dBm accuracy (absolute) not instrumented; no power meter available.
