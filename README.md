# mt7921u — mt76 driver fork (MT7921AU USB, Raspberry Pi)

Out-of-tree build of the Linux mt76 driver for MediaTek MT7921AU USB
dongles, as run on the SecureLine gateway (Raspberry Pi 5, kernel
6.18.34+rpt-rpi-v8, two dongles: one AP, one STA).

`drivers/net/wireless/mediatek/mt76/` is the shipped state — the patch
series is already applied to the sources. `patches/` holds each patch as
a separate file with the full rationale. `vendor-driver/` is the MediaTek
reference driver the TWT/CSI/feature ports were written against.

## Why the fork exists

The stock driver was not usable for a USB AP+STA setup on this box:

- hard wedges: control-endpoint stalls that never recover, forcing a
  dongle re-enumeration or a reboot
- USB3 LPM (U1/U2) lockups behind hubs (VL805/JMS583)
- no survey noise floor, so hostapd ACS was blind
- ignored BSS_CHANGED_TXPOWER / stale txpower readback
- firmware A-MSDU packing of software-encrypted frames into air-invalid
  A-MSDUs — mass downlink frame loss (fixed by patch 0028)

`docs/patch-series.md` has the complete list.

## Layout

    drivers/net/wireless/mediatek/mt76/  driver sources (patch series applied)
    patches/                             one file per patch, in series order
    docs/                                install/rollback, patch-series notes
    vendor-driver/                       MediaTek reference driver (spec)
    testing/                             offline logic tests
    Documentation/                       kernel-doc extras (CSI)

## Build

    ./build.sh

Needs the kernel headers for the running kernel
(`linux-headers-$(uname -r)`). Artifacts stay in the source tree.

## Install / rollback

See `docs/install.md`. In short: copy the 7 `.ko` into
`/lib/modules/$(uname -r)/updates/`, `depmod -a`, remove the stack
top-down (`modprobe -r mt7921u mt7921-common mt792x-usb mt792x-lib
mt76-usb mt76-connac-lib mt76`), `modprobe mt7921u`, then
`systemctl restart sl-ap` and wait out the DFS CAC (~70 s on ch100).

## Status

Production on the gateway since 2026-09-06 with patch 0028. Reboot
survival is verified (module identity, DFS, AP, resilience stack).
Open issue: uplink to one power-saving client caps at ~3-5 Mbps
(client PS wake-gating suspected, under investigation).
