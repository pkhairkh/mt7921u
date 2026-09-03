# S7 Phase 4 — feature battery + AP-mode stress (2026-09-03)

User requirement: "ALL THE OTHER STUFF IS ALSO NOT PROPERLY TESTED!"
Everything below was executed on live hardware, both dongles, with
before/after state checks and dmesg badness filters after every stage.

## Feature battery results (per feature)
| Feature | Result | Evidence |
|---|---|---|
| Dual-band simultaneous AP+AP | WORKS | wlan1 AP ch6 2.4G + wlan2 AP ch36 5G, both AP-ENABLED concurrently |
| 5GHz AP (20/40MHz) | WORKS | ch36 20MHz + 40MHz (center1 5190) AP up, beaconing |
| VHT80 AP (80MHz) | WORKS (hostapd config fix) | width: 80 MHz center1 5210; hw vht capab 0x339071b2; note: hostapd needs `vht_capab=[SHORT-GI-80]` etc. or it self-fails with "conf vht capab: 0x0 -> Could not set channel" — NOT a driver bug |
| VHT80 end-to-end rate | WORKS | both directions 780.0 MBit/s VHT-MCS 9 80MHz VHT-NSS 2, authorized, tx failed=0, 0% loss @ sub-ms RTT |
| Monitor mode | WORKS | mon0 add/up/channel/del clean; 5x churn zero residue |
| P2P machinery | WORKS (basic) | netns wpa_supplicant p2p_find OK, p2p-dev address allocated; no full GO-negotiation test |
| ACS after BUG-10 fix | WORKS | channel=0 -> ACS-COMPLETED ch10, 3/3 + 2/2 repeat deterministic, 0 missing-NF errors |
| Per-station txpower | NOT SUPPORTED (upstream) | iw station set txpwr refused; mt76 family implements no .sta_set_txpwr op — capability gap, clean refusal, no crash |
| 4addr/WDS | NOT SUPPORTED | EOPNOTSUPP (-95), clean refusal |
| Mesh | NOT SUPPORTED | EOPNOTSUPP (-95), clean refusal (not in interface modes) |
| DFS (52/100/140) | REGULATORY-SAFE REFUSAL | hostapd: "Frequency 5260 not allowed for AP mode, flags RADAR"; iw cac trigger -> EINVAL; no radar detection in FW -> correctly never beacons on DFS |

## AP-mode stress battery (S7-W)
- 10x AP bring-up/teardown cycles: 10/10 OK, zero dmesg badness
- 5x full STA assoc/disassoc churn (hostapd + netns STA + traffic each round):
  5/5 assoc, 0% packet loss every round
- TX power churn under live load (6 settings, 1472B pings each):
  readback tracks exactly 1.00 -> 3.00 -> 8.00 -> 15.00 -> 20.00 -> 20.00 (auto),
  0% loss at every step (BUG-08/09 fix stable under load)
- ACS x2 under repeat: channel 10 both times, 0 "Survey is missing" errors
- 5x monitor add/del churn: zero interface residue
- Concurrent AP+AP at end of battery: 2/2 AP-ENABLED
- Kernel health: ZERO err/fail/warn/timeout/panic/oops/hung lines across the
  whole battery; uptime continuous (no reboot, no freeze, no watchdog).

## Known limitations (documented honestly)
- MIB airtime (busy/tx/rx times) does not accumulate over USB for scan-only
  channels (cc counters only update for the current channel). NF — the only
  field hostapd ACS requires — is complete via the BUG-10 fix.
- RSSI as power meter is near-field-saturated at 30cm; authoritative txpower
  evidence is readback + firmware SKU table (both exact).
- VHT160 not supported by this SKU (init.c: only mt7922 sets 160MHz caps).
- No DFS radar-detection firmware: DFS AP operation is refused (by design).
