# CHECKPOINT 5 - FINAL VERDICT: PASS (2026-09-03 09:17 UTC)

## Mission complete: PATCHED mt7921u stack proven 100% on Pi4/6.18.34+rpt-rpi-v8

## Retest results with deadlock fix (fixed stack, capture unit mt76cap6 running)
1. Full-stack swap to fixed modules: fingerprint verified (mt76 143360, mt7921_common 102400 — patched values)
2. Sanity scan: rc=0, target SSID found (2 entries)
3. CONNECT #1: rc=0, DHCP 192.168.178.147, ping -I wlan1 3/3 + 5/5 0% loss, clean auth/assoc in dmesg
4. **DISCONNECT #1: rc=0 in ~3 s** ('Device wlan1 successfully disconnected'), state 30 (disconnected), dmesg shows ONLY 'deauthenticating by local choice (Reason: 3)', badness=0, box alive
5. Full second cycle: CONNECT #2 rc=0, 4/4 ping 0% loss, **DISCONNECT #2 rc=0**, badness=0, box alive
6. Managed window closed, full unmanage restored (wlan0+wlan1 unmanaged), capture unit stopped

## The complete bug story (BUG-07)
- Pre-fix: every disconnect = AA self-deadlock on dev->mt76.mutex (mt7921_mac_sta_remove re-acquired the mutex the mt76 core wrapper already holds) -> hung_task at 10 s -> wedge -> watchdog reset ~2 min later. Reproduced the 3 historical machine freezes with full instrumentation.
- Post-fix (single 2-line removal + comment): 2 complete connect/disconnect cycles, zero errors, zero noise.

## Files
- snapshot-*.txt: dmesg capture through both clean cycles
- current-boot-kernel.log: full kernel log of the final boot
- final-device-status.txt / final-lsmod.txt: end state
