# mt7921u (patched stack) - canonical modprobe procedure

Verified 2026-09-03 on Pi 4B, kernel 6.18.34+rpt-rpi-v8 (forensics/2026-09-03_phase7_stress/).

## Install layout (kmod priority)
- Patched .ko live in /lib/modules/$(uname -r)/updates/ ; in-tree .xz copies in .../kernel/.../mt76/
- After `depmod -a`, `modprobe --show-depends mt7921u` resolves ALL 7 modules via updates/ (updates/ wins)
- No mt76/mt7921 in initramfs, no /etc/modprobe.d overrides -> cold boot autoloads the PATCHED stack via udev coldplug (~6.7s)
- Identity card (sha256 + srcversion, running vs on-disk): forensics/2026-09-03_phase7_stress/module-id-card.txt

## FINGERPRINT (in-tree vs patched)
- lsmod: mt76 143360 (in-tree 139264), mt7921_common 102400 (in-tree 77824)
- /sys/module/mt7921u/srcversion = EFE770C012237CBF52842D2 (patched build)

## CANONICAL FULL-STACK REMOVAL (top-down, explicit)
`modprobe -r mt7921u mt7921-common mt792x-usb mt792x-lib mt76-usb mt76-connac-lib mt76`

WHY EXPLICIT: on this system `modprobe -r mt7921u` alone removes ONLY the top module
and leaves the 6 library modules loaded (verified - they sit at used-by 0 and kmod does
not cascade-remove them). If you rebuild a LIBRARY .ko and only remove mt7921u, the OLD
lib stays live -> later insmod of the new lib hits EINVAL (see CHECKPOINT 3 note).
ALWAYS remove all 7 top-down, then verify: `lsmod | grep -E 'mt76|mt79'` must be EMPTY.

## CANONICAL INSERT (single command, dep-resolved)
`modprobe mt7921u`  -> kmod pulls all deps from updates/ via modules.dep

Verify after every swap: 7 modules in lsmod, mt76 size 143360, dongle interface exists
(driver symlink /sys/class/net/*/device/driver -> mt7921u), srcversion matches ID card.

## Hot-replug equivalents (no module unload)
- driver rebind:   echo 2-2:1.0 > /sys/bus/usb/drivers/mt7921u/unbind ; .../bind
- full re-enum + fw reload: echo 0 > /sys/bus/usb/devices/2-2/authorized ; echo 1 > ...
Both paths verified 6x each with zero dmesg errors (stress S1, 2026-09-03).
