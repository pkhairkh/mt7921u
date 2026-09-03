# S7 Phase 1 — AP mode + two-dongle link + TX power quirk (2026-09-03)

User requirement under test: "AP features, auto channel, DFS, additional features,
two dongles, set the decibel properly (quirk in the original driver)".

## Environment
- 2x MT7921U dongles (0e8d:7961) on USB3 hub: phy2/wlan1 (e8:4e:06:b0:27:dc),
  phy3/wlan2 (e8:4e:06:b0:1f:b7). 2nd dongle hot-plugged at runtime earlier — clean.
- Kernel 6.18.34+rpt-rpi-v8, patched stack (updates/, srcversion verified this boot).
- hostapd 2.10 installed this phase. wpa_supplicant per-netns instance.

## Results
1. AP MODE: WORKS. hostapd bring-up on wlan1 (type AP, ch6 20MHz, WPA2-PSK/CCMP).
   One transient nl80211 -EBUSY on initial mode switch (hostapd retries, succeeds).
2. TWO-DONGLE LINK: WORKS. STA (wlan2 in dedicated netns) associates in 4s,
   complete 4-way handshake, authorized. Data: 0% loss both directions
   (15/15, 15/15, 20/20 @1472B), 130 Mbps MCS15 both ways, tx_failed=0, tx retries=0.
3. TX POWER (the "decibel quirk"): ROOT-CAUSED as BUG-08.
   - Write path works at RF level: RSSI responds measurably and reversibly to
     iw set txpower fixed 300/2000 with live traffic, 0% loss.
   - Read path is broken upstream: phy->txpower_cur is never maintained in the
     mt7921/connac path -> .get_txpower always reports uninitialized garbage
     ("txpower 3.00 dBm" = 0 + 2-chain delta). iw dev info can NEVER show what
     was set. Fix: maintain txpower_cur like mt7615/mt7996/mt7915 do. Next phase.
4. CAPABILITY MAP (per dongle): bands 2.4+5+6GHz exposed (101 channels; 6GHz all
   disabled under DE regdom), iface modes managed/AP/AP-VLAN/monitor/P2P-*,
   combos: {managed,P2P-client}<=2 + AP<=1 same channel; DFS channels 52-144
   flagged "radar detection" under DE.
5. Methodology note: same-host interface-to-interface pinging is INVALID for radio
   testing (local route + oif semantics); dedicated netns per dongle is the correct
   harness and is now part of the toolkit.

## Verdict
AP bring-up + two-dongle WPA2 link + bidirectional data: PROVEN CLEAN (zero kernel
badness). TX power quirk: confirmed real, root-caused (BUG-08), RF-level proof of
write-path captured, fix scheduled next.
