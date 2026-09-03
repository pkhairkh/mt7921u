# STAGE S0 - Stress campaign opened: modprobe audit + boot truth (2026-09-03 09:42 UTC)

## Context
- CHECKPOINT 5 claimed 'proven 100%' after only 2 STA cycles. User verdict: patched features NOT stress tested; module management not rigorously proven. This stage opens the stress campaign (S0-S6).

## 1. 'Properly modprobed' - PROVEN at cold-boot level
- Fresh boot (no manual modprobe): udev coldplug loaded the full 7-module stack from updates/ at ~6.7s
- modprobe --show-depends mt7921u: every dependency resolves to /lib/modules/6.18.34+rpt-rpi-v8/updates/*.ko (in-tree .xz copies exist but LOSE to updates/ in modules.dep resolution)
- No mt76/mt7921 in initramfs; no modprobe.d overrides; no force-load options
- Identity: running /sys/module/*/srcversion == modinfo -F srcversion of updates/ files for all 7 modules (see module-id-card.txt incl. sha256)
- Fingerprint: mt76=143360 (in-tree 139264), mt7921_common=102400 (in-tree 77824) - patched stack running

## 2. Outage + boot truth (11:20-11:36 CEST gap after CHECKPOINT 5)
- Post-CHECKPOINT-5 outage: host unreachable from sandbox ~11:20:30 CEST; box returned single fresh boot (real ~11:36:45 CEST, journal clock-skewed first entry 11:21:44 due to no-RTC + NTP step)
- boot -1 ended QUIETLY: clean user-session close 11:18:04 (prior session final_commit.sh done), last event sshd preauth 11:20:31, then silence. NO hung_task (10s detector was armed + persisted), NO RCU stall, NO watchdog bark, pstore EMPTY (no panic)
- Interpretation: EITHER silent hard lockup while idle (IRQ-off spin; systemd RuntimeWatchdog=60s would reset it - timing loosely fits) OR external power intervention. Second-freeze hypothesis stays OPEN, not proven.
- Defense now armed for any recurrence: capture unit mt76capS (3s dmesg snapshots to /var/tmp), persistent journald, hung_task=10 + sysrq=1 persisted in /etc/sysctl.d/99-mt76-debug.conf

## 3. Watchdog posture (discovered)
- systemd arms BCM2835 watchdog with 60s hardware timeout at boot (dmesg 3.36s) - any hard freeze auto-recovers within ~60s, silently (no pstore on non-panic reset)

## Next
- S1: canonical cold-swap procedure + 15x modprobe/unbind/rebind cycles (probe/remove + CLC path each time)
- S2: scan flood + regdomain churn + rfkill + iface flaps; S3: 10x STA cycles; S4: traffic load + ROC paths; S5: idle soak; S6: final reboot persistence
