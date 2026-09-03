# CHECKPOINT 4 - PHASE 5 VERDICT (2026-09-03 09:10 UTC)

## VERDICT: PARTIAL PASS -> FAILURE SIGNATURE CAPTURED
- STA CONNECT: **PASS** (patched driver): rc=0, DHCP 192.168.178.147, ping -I wlan1 3/3 0% loss, clean auth/assoc, 120 s idle + traffic clean, badness 0
- STA DISCONNECT: **FAIL - REPRODUCIBLE DEADLOCK** (this is the historical 3x machine-freeze signature, first time captured with full instrumentation)

## Failure signature (from live dmesg + journal -b -1 + snapshot loop)
1. 09:03:49 UTC: 'nmcli device disconnect wlan1' (rc=3, 'Timeout 10 sec expired'), device stuck 110 (deactivating)
2. deauth itself completes: 'wlan1: deauthenticating from dc:39:6f:ed:ba:e7 by local choice (Reason: 3=DEAUTH_LEAVING)'
3. 11 s later hung_task (kernel.hung_task_timeout_secs=10 worked as designed):
   - wpa_supplicant:628 D-state: nl80211_deauthenticate -> cfg80211_mlme_deauth -> ieee80211_mgd_deauth -> ieee80211_set_disassoc -> __sta_info_flush -> __sta_info_destroy_part2 -> drv_sta_state -> mt7921_sta_state -> mt76_sta_state -> __mt76_sta_remove -> mt7921_mac_sta_remove+0x60 -> mutex_lock(dev->mt76.mutex) BLOCKED
   - kernel: mutex 'likely owned by task wpa_supplicant:628' = SELF-DEADLOCK (task holds mt76.mutex from an earlier acquisition in the same syscall and re-acquires in sta-remove)
   - kworker/u16:2:50 D-state: cfg80211_wiphy_work -> mutex_lock (wiphy work stalled behind same owner)
   - kworker/u16:0:3283 D-state: mt792x_mac_work (mt792x_lib) -> mutex_lock (driver mac work stalled)
4. ~09:04-09:06 wedge spreads (systemd stop jobs stall), systemd watchdog heartbeat stops
5. ~09:06: BCM2835 hardware watchdog (1 min timeout) fires -> full box reset, clean SD boot at 09:08
6. pstore EMPTY (watchdog reset is not a panic; ramoops records nothing on silent hang - by design)

## What survived (persistent journal + on-disk snapshots proved their worth)
- journal -b -1 kernel log: full deadlock report (prevboot-kernel.log)
- /var/tmp/mt76snap-001..071.txt: 5 s-resolution dmesg through the entire event (snapshot-*.txt, last 8 kept)
- Current boot clean: full unmanage conf restored, both wlan unmanaged, ramoops re-armed, journal persistent

## Recovery actions taken
- Full driver+name unmanage conf restored immediately after reboot (test window closed)
- mtk-test profile remains (autoconnect no, interface-bound wlan1) - inert

## Next: source audit of mutex balance in the sta-remove/disconnect path (BUG-03/04/05/06 patch areas)
