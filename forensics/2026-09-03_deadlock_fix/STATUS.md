# Deadlock fix: double mutex acquisition in mt7921_mac_sta_remove (2026-09-03 09:14 UTC)

## Root cause (proven: code reading + live deadlock capture)
- mt76 core wrapper mt76_sta_remove() (mt76/mac80211.c:1630) holds dev->mt76.mutex while calling dev->drv->sta_remove()
- repo's mt7921_mac_sta_remove() (mt7921/main.c) ALSO called mt792x_mutex_acquire(dev) = mt76_connac_mutex_acquire = mutex_lock(&dev->mt76.mutex) on the SAME non-recursive mutex
- result: AA self-deadlock on every STA removal triggered under the wrapper (i.e. every disconnect/deauth sta-flush) -> matches the 3 historical machine freezes + our Phase-5 capture
- why connect worked but disconnect hung: mt7921_mac_sta_add() takes NO internal mutex (fine under wrapper); mt7921_mac_sta_event() takes it but is called WITHOUT the wrapper
- blame: the acquire predates the core locking-contract change (port skew); the TWT patch comment ('no separate mutex needed') shows the confusion

## Fix
- Removed mt792x_mutex_acquire(dev) / mt792x_mutex_release(dev) pair from mt7921_mac_sta_remove(); locking now relies solely on the core wrapper
- Added warning comment in place
- Rebuild: rc=0, zero errors; module redeployed to updates/ + depmod; fingerprint re-verified (patched stack loaded)

## Retest
- Full STA connect/disconnect cycle rerun under dmesg capture unit mt76cap6 (results in next commit)
