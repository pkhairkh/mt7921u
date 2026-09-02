# Stress Test Findings — 2026-09-03 (session after reboot)

## Environment
- Raspberry Pi 4, 6.18.34+rpt-rpi-v8 aarch64, Debian 13 (trixie), boot from SSD (BOT mode)
- Patched mt7921u stack rebuilt against 6.18 headers (see commits 989d95d / 40b01e3),
  manually re-insmod'd after reboot by user. Two MT7921U dongles: wlan1 (phy1), wlan2 (phy2).

## Results

| Test | Result | Notes |
|------|--------|-------|
| Scan battery wlan1 (2x full + 6-freq burst) | PASS | 9–12 BSS, silent kernel log |
| Scan battery wlan2 (same) | PASS | 11–16 BSS, silent kernel log |
| T6 forced chip_reset (fw crash + reset) | PASS | driver recovers, debugfs alive, no oops, queues OK |
| T1/T3/T4/T7 | SKIP | need CONFIG_NL80211_TESTMODE (not set in RPi 6.18 headers) and test_trigger debugfs module (not present in repo tree) |
| T5 suspend/resume | NOT RUN | skipped over SSH by design (needs console) |
| Concurrent dual-radio scans (wlan1 hammer + wlan2 battery) | PASS w/ note | see EBUSY note below |
| DFS/CAC path | NOT EXERCISABLE | nl80211 rejects CAC start on managed-mode iface (-22); needs AP-mode test (hostapd) later |

## EBUSY note (benign)
Under concurrent scan load, an iw scan that overlaps a still-running scan on the
SAME radio fails fast with `Device or resource busy (-16)` (iw exit 240).
Cross-radio concurrency works (scan on wlan2 returned 7 BSS while wlan1 was
hammering). Full solo recovery after load: 14/13 BSS, zero kernel log noise.
This is standard mac80211 fail-fast behavior, not a driver bug.

## Crash reproduction status
The pre-reboot stress wedge could NOT be reproduced. Scan batteries, forced
fw-crash reset (T6), and concurrent dual-radio stress all pass clean.

## Forensic gap found + fixes
The pre-crash kernel log was destroyed because RPi OS ships
/usr/lib/systemd/journald.conf.d/40-rpi-volatile-storage.conf (Storage=volatile):
journald never writes to /var/log/journal, and there is no rsyslog, no pstore.

Mitigations deployed this session:
1. /etc/systemd/journald.conf.d/50-persistent-storage.conf (Storage=persistent)
   — verify /var/log/journal fills after next reboot (did not take effect on
   restart; cause under investigation).
2. dtoverlay=ramoops-pi4 appended to /boot/firmware/config.txt (line 55,
   backup at config.txt.bak_ramoops) — arms pstore/ramoops on next boot so
   panics survive reboot in /sys/fs/pstore.
3. ~/stress/ forensic harness:
   - dmesg_cap.sh: dmesg snapshot rewritten+fsynced every 2 s (survives wedge)
   - cap_ctl.sh start/stop: pidfile-based capture control
   - run_battery.sh <iface>: scan battery used above
   - klog_*.txt / battery_*.txt / tests/: artifacts from this session

## Next steps
- Reboot to arm ramoops (+ verify persistent journal), then re-run heavier
  stress (association/traffic, hostapd CAC on DFS channel, long soak) — any
  panic will now leave pstore evidence.
- Revisit T1/T3 testmode paths only if a kernel with CONFIG_NL80211_TESTMODE
  is available.
