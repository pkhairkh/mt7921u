# CHECKPOINT 2 - Patched stack built + installed (2026-09-03 08:55 UTC)

## Build
- Source: repo HEAD (patched mt76 tree incl. BUG-01..06 fixes + 11 patches), kernel 6.18.34+rpt-rpi-v8, out-of-tree M=build in drivers/net/wireless/mediatek/mt76
- make -C /lib/modules/$(uname -r)/build M=$(pwd) modules -j4: rc=0, ZERO errors (first attempt)
- Warnings only: -Wmissing-prototypes (__mt792x_assign/unassign_vif_chanctx), -Wunused-function (test_trigger_debugfs.c result_str)
- Full log: build.log (79 lines)

## Produced (7/7 .ko)
- drivers/net/wireless/mediatek/mt76/mt76.ko 199008 bytes
- drivers/net/wireless/mediatek/mt76/mt7921/mt7921-common.ko 178560 bytes
- (plus mt76-usb, mt76-connac-lib, mt792x-lib, mt792x-usb, mt7921u)

## Install
- Copied 7 .ko to /lib/modules/6.18.34+rpt-rpi-v8/updates/, depmod -a complete
- modinfo -n mt7921u => /lib/modules/6.18.34+rpt-rpi-v8/updates/mt7921u.ko
- modinfo -n mt76 => /lib/modules/6.18.34+rpt-rpi-v8/updates/mt76.ko
- vermagic: 6.18.34+rpt-rpi-v8 SMP preempt mod_unload modversions aarch64

## Next: PHASE 4 module swap + fingerprint + scan battery (no STA connect yet)
