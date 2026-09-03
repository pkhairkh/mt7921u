# STAGE S1 - modprobe/unbind/re-enum stress (2026-09-03 09:48 UTC)

## Results (box alive throughout, uptime continuous, zero cumulative dmesg badness)
- PART A (12x modprobe -r mt7921u / modprobe mt7921u): 12/12 operationally PASS (rm rc=0, ins rc=0, iface re-created, fingerprint 143360/102400, dmesg bad=0 every cycle). Harness initially scored 0/12 because it required cascade removal - finding below.
- FINDING (kmod behavior): 'modprobe -r mt7921u' removes ONLY the top module; the 6 library modules remain loaded at used-by 0 (no cascade). Documented + canonical explicit top-down removal in docs/MODPROBE-PROCEDURE.md.
- PART B (6x USB driver unbind/bind 2-2:1.0): 6/6 PASS - probe + remove paths clean.
- PART C (6x USB authorized 0/1 = full re-enumeration + firmware reload): 6/6 PASS - fw loaded every time (WM Firmware Version x1 per cycle), no MCU/WTBL/timeout errors.
- Canonical swap (remove all 7 -> single modprobe): 3/3 PASS incl. srcversion identity check.

## Stress coverage of patched code paths
- mt7921u probe/remove (USB probe, CLC/EEPROM load, firmware load): 12+6+6+3 = 27 full cycles, zero errors
- Combined with S0 cold-boot autoload: module management path = properly proven

## Evidence
- capture unit mt76capS running (3s dmesg snapshots to /var/tmp, 153 snapshots)
- Next: S2 scan flood + regdomain churn + rfkill + iface flaps
