# Patch series

One file per patch in `patches/`, in order. The sources under
`drivers/net/wireless/mediatek/mt76/` carry all of them applied; the
files document each change on top of upstream.

| #  | Patch | Summary |
|----|-------|---------|
| 0001 | guard testmode against NULL drv_own on USB | testmode commands crashed on USB, where drv_own is unset |
| 0002 | increase WTBL poll timeout for USB | WTBL writes timed out on the slower USB path |
| 0003 | MCU command retry on USB | transient control-endpoint failures aborted ops a retry survives |
| 0004 | cancel ROC timer work on disconnect | remain-on-channel timer fired after disconnect |
| 0005 | SDIO early return in mac_reset_work | upstream parity; USB path must not skip the reset tail |
| 0006 | CLC for USB (experimental) | enable channel-list-check on USB |
| 0007 | CLC defensive fallback for USB | survive a missing CLC blob |
| 0008 | TWT implementation phases 2-3 | ported from the vendor driver |
| 0009 | CSI extraction phases 2-4 | ported from the vendor driver |
| 0010 | runtime verification harness | debugfs self-checks for bring-up |
| 0011 | remaining source-level feature ports | vendor-driver features not covered above |
| 0012 | harden USB wedge recovery | fast-fail + RX watchdog on a wedged control endpoint |
| 0013 | fail-fast survey MIB on wedged endpoint | survey reads no longer hang on a dead endpoint |
| 0014 | USB wedge escalation engine | decay counters, port reset, watchdog re-arm |
| 0015 | bus_hung architecture | ported from torvalds 915672c5; commit-level, no file |
| 0016 | probe-error UAF fix + disconnect hardening | audit round; commit-level, no file |
| 0017 | disable USB3 LPM U1/U2 in probe | first LPM kill (superseded by 0020) |
| 0018 | AP-mode group keys in software | firmware multicast key handling was unreliable |
| 0019 | group keys in software for all modes | generalizes 0018 |
| 0020 | airtight USB3 LPM kill | kill U1/U2 everywhere, survive hubs (VL805/JMS583) |
| 0021 | LPM enforcement: agg flush + AP software PTK | flush aggregations on LPM state changes; PTK in software |
| 0022 | fix 0021 deadlock | RCU/hub-loop deadlock introduced by 0021 |
| 0023 | native DFS offload + multi-band ACS | firmware RDD, ACS on real survey data |
| 0024 | DFS classic-contract fix | radar-report vif lifetime + mac80211 CAC contract on 6.18; folded into the tree |
| 0025 | RDD engine v2 | track the channel via chanctx, not hw->conf; folded |
| 0026 | TX endpoint lifecycle instrumentation | read-only diagnostics, logs once per second; folded |
| 0027 | USB AC endpoint map + mcast PSD | per-access-class endpoint mapping, multicast PSD |
| 0028 | gate HW A-MSDU on the 802.3 path | the downlink fix: firmware packed software-encrypted frames into air-invalid A-MSDUs and the peer dropped them wholesale |

0015/0016 exist only as commits in the pre-baseline history; 0024-0026
were applied directly to the working tree. No files exist for those
numbers — the sources contain every change.
