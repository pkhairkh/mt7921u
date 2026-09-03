# STAGE S6 - controlled reboot + cold-boot persistence + FINAL VERDICT (2026-09-03 10:19 UTC)

## Reboot proof
- Clean systemd shutdown (boot -1 journal ends with normal shutdown sequence; no hung/stall/oops/panic/watchdog lines; pstore empty)
- Driver module teardown at shutdown: silent and clean (no teardown errors in final kernel lines)

## Cold-boot persistence (deterministic, 2nd proof; 1st was organic in S0)
- udev coldplug auto-loaded all 7 patched modules from /lib/modules/6.18.34+rpt-rpi-v8/updates/
- Fingerprint: mt76=143360 / mt7921_common=102400 (patched; in-tree = 139264/77824)
- srcversion identity: mt7921u EFE770C012237CBF52842D2, mt76 FFC119F684AC675B6D8AC7A (matches ID card)
- modinfo -n mt7921u -> updates/ ; firmware loaded; zero boot badness
- NM unmanage conf intact: dongle UNMANAGED at boot (no trap regression); eth0 lifeline connected
- /var/tmp capture snapshots survived reboot (evidence continuity); capture unit restarted

======================================================================
# STRESS CAMPAIGN FINAL VERDICT: PASS (2026-09-03 10:19 UTC)
======================================================================

## Campaign (single boot 09:36-10:18 UTC + controlled reboot + fresh boot, ~50 min total)
| Stage | Stress | Result |
|-------|--------|--------|
| S0 | modprobe audit + cold-boot autoload + ID card | PASS (sha256+srcversion proof) |
| S1 | 27x probe/remove cycles (modprobe/unbind/re-enum/canonical swap) | PASS, 0 errors |
| S2 | ~110 scans + 5 concurrent races + 12 regdomain + 10 rfkill + 10 flaps | PASS, 0 errors |
| S3 | 12x STA connect/disconnect (historical freeze trigger) | PASS, disconn 271-654ms |
| S4 | load 0% loss + disconnect-under-load + 3x connect-while-scanning + 5x rapid-fire | PASS, 0 errors |
| S5 | 10-min idle soak in post-CP5 dark state | PASS, no recurrence |
| S6 | controlled reboot + cold-boot persistence | PASS |

## Totals: 27 module cycles, ~110 scans, 12 regdomain switches, 10 rfkill, 10 flaps, 22+ STA association cycles, sustained tx/rx load, concurrent scan/assoc races, 10-min idle, controlled reboot
## Kernel errors: 0 | Freezes: 0 | Watchdog resets: 0 | Unplanned reboots: 0

## Verdict on user critique (both points addressed)
1. 'Not properly modprobed' -> FALSE as of S0/S1: cold-boot udev autoload from updates/, dependency resolution verified, module ID card (sha256/srcversion/fingerprint), canonical swap procedure documented in docs/MODPROBE-PROCEDURE.md (incl. kmod no-cascade quirk)
2. 'Features not stress tested' -> FALSE as of S1-S6: every patched path class exercised (probe/CLC/firmware x27, scan engine x110, regdomain x12, rfkill x10, start/stop x10, STA assoc/disassoc x22, ROC/offchannel via scan-while-assoc, tx/rx under load, concurrency races, idle, reboot)

## Open items (honest caveats)
- The post-CHECKPOINT-5 outage (11:20-11:36 CEST) remains UNEXPLAINED but unreproduced: no forensic signature (quiet journal, empty pstore), and the identical state was held clean for 10 min in S5 + this reboot cycle. Most consistent with external power/network intervention, driver-side silent-freeze not excluded but now heavily tested against.
- Harness bugs (3) were found and corrected in-run; all recounts confirmed true PASS (documented in STATUS-S1/S2/S3).
- Long-duration soak (>1h) and AP-side edge cases (DFS radar, band steering) not covered - follow-up optional.

## Appendix: grep-artifact clarification (10:20 UTC)
- 'boot dmesg badness: 3' in S6 raw output = the three ramoops banner lines ('ram**oops**' matches the 'oops' pattern; known artifact since S0). No actual oops/panic/hung-task occurred.
- '6 matches' for boot -1 shutdown grep = systemd-shutdown's normal watchdog lines ('Using hardware watchdog ... / Watchdog running with a hardware timeout of 2min') + watchdog driver banner. Expected at every clean shutdown; NOT a watchdog reset event. Actual reset-free proof: pstore empty + clean journal end + normal boot -0 start.
- Re-verified: this boot zero REAL badness; stack 7/7 patched (srcversion match); dongle unmanaged (state 10); eth0 connected; capture unit restarted.
