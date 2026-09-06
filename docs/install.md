# Module install & rollback

Verified on the gateway (Raspberry Pi 5, 6.18.34+rpt-rpi-v8).

## Layout and precedence

- The patched `.ko` live flat in `/lib/modules/$(uname -r)/updates/`; the
  in-tree (stock) copies sit in
  `.../kernel/drivers/net/wireless/mediatek/mt76/`.
- After `depmod -a`, `modprobe --show-depends mt7921u` must resolve all
  7 modules through `updates/` — that directory wins.
- Nothing is in initramfs and there are no `/etc/modprobe.d` overrides,
  so a cold boot autoloads the patched stack via udev coldplug.

## Swapping the stack (hot)

Remove top-down — `modprobe -r` does NOT cascade-remove the library
modules on this system, so remove all seven explicitly:

    modprobe -r mt7921u mt7921-common mt792x-usb mt792x-lib \
                mt76-usb mt76-connac-lib mt76

`lsmod | grep -E 'mt76|mt79'` must be empty afterwards. Then:

    cd <repo>
    cp drivers/net/wireless/mediatek/mt76/*.ko \
       drivers/net/wireless/mediatek/mt76/mt7921/*.ko \
       /lib/modules/$(uname -r)/updates/
    depmod -a
    modprobe mt7921u

After every full-stack swap:

    systemctl restart sl-ap

and wait for the DFS CAC (~70 s on ch100) — the udev rule that restarts
the AP on a wlan2 re-add does not reliably fire after a module swap.

## Identity check

    modinfo mt7921u | grep -E 'filename|srcversion'

must show `/lib/modules/$(uname -r)/updates/mt7921u.ko` and the
srcversion of the build you installed. The same check after a reboot
proves the patched stack survived the cold boot.

## Alternatives to a module swap

- rebind one dongle:
  `echo 2-2:1.0 > /sys/bus/usb/drivers/mt7921u/unbind` then `.../bind`
- full re-enumeration + firmware reload:
  `echo 0 > /sys/bus/usb/devices/2-2/authorized; echo 1 > ...`

Both verified repeatedly with zero dmesg errors.

## Known noise

`WARNING ... kthread_park ... mt7921u_mac_reset` can appear in dmesg
when `modprobe -r` races an in-flight reset work. Cosmetic; removal
completes and the next insert is clean.
