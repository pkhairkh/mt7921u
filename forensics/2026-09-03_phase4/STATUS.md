# CHECKPOINT 3 - Patched stack validated (swap, fingerprint, scans) (2026-09-03 08:59 UTC)

## Troubleshooting findings during module swap
1. **Partial rmmod fails with EINVAL**: removing only 'mt7921u' leaves in-tree deps loaded; modprobe of patched mt7921u then fails 'Invalid argument' with dmesg 'mt792x_usb: disagrees about version of symbol ...' (modversions CRC mismatch vs in-tree mt76/mt76-usb). FIX: remove the WHOLE stack (mt7921u, mt7921-common, mt792x-usb, mt792x-lib, mt76-usb, mt76-connac-lib, mt76) then fresh modprobe mt7921u. This will also be required at any future swap.
2. **Scan fails with ENETDOWN until interface brought up**: 'iw dev wlan1 scan ap-force' returns 'Network is down (-100)' while iface is DOWN. FIX: ip link set wlan1 up first (proven-safe op class).

## Fingerprint check (lsmod) - PATCHED VALUES CONFIRMED
- mt76 = 143360 (in-tree 139264), mt7921_common = 102400 (in-tree 77824), mt76_connac_lib = 90112 (in-tree 86016)
- Full stack loaded from updates/ on fresh modprobe, rc=0
- Probe clean: 'usbcore: registered new interface driver mt7921u' + HW/SW Version 0x8a108a10 + WM Firmware ____010000
- Dongle interface this boot: wlan1 (wlan0 = onboard brcmfmac); both unmanaged by NM

## Scan battery: 13/13 PASS
- 3x full scan ap-force: rc=0, BSS 11/15/20
- 10x single-freq (2412,2417,2422,2437,2462,5180,5240,5500,5745,5805): rc=0, all with BSS (cache accumulates to 20)
- iw reg: country DE DFS-ETSI (global reg lists 5945-6425 6 GHz band too)

## dmesg noise
- Zero mt76/mt7921 errors during scans; badness counter (mcu/wtbl timeout, hung task, panic) = 0

## Next: PHASE 5 deliberate STA connect/disconnect under ramoops+journal capture
