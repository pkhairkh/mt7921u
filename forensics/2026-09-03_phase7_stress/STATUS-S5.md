# STAGE S5 - idle soak (2026-09-03 10:17 UTC)

## 10-minute monitored idle soak in the EXACT state that went dark after CHECKPOINT 5
- stack 7/7 loaded, dongle unmanaged + operstate down, no traffic, capture unit running
- 10 heartbeats at 60s: uptime continuous 1825s -> 2427s, tail-badness 0, pstore 0, snapshots 529 -> 708
- mt76/mt79 dmesg tail at end: silent (no spurious noise while idle)
- VERDICT: no recurrence of the suspected idle freeze in 10 min window

## Cumulative after S5: 45+ min continuous uptime under and after stress; zero errors, zero freezes, zero reboots
