# CHECKPOINT 1 - Killer trap neutralized (2026-09-03 08:45 UTC)

## System
- Kernel: 6.18.34+rpt-rpi-v8, host: net-master-0x01
- Dongle: 0e8d:7961 on USB 2-2, in-tree mt7921u auto-loaded (this boot: interface = wlan1, onboard brcmfmac = wlan0)

## Actions
- /etc/NetworkManager/conf.d/99-unmanage-mt76.conf written (driver:mt7921u,driver:mt792x_usb + wlan0/1/2) - content verified by cat
- Fresh OS = netplan+NM: wifi profile 'netplan-wlan0-FRITZ!Box 7530 IZ' was ACTIVE on boot (autoconnect trap ARMED at first contact)
- /etc/netplan/90-NM-6676db84...yaml (wifi stanza) removed, backed up to ~/netplan-backup/
- eth0 yaml untouched; eth0 lifeline verified: default route via 192.168.178.1

## Verified state
- NM active; wlan0 + wlan1 both unmanaged; no wifi profiles remain (nmcli + system-connections empty)
- Incident: first write of 99-unmanage-mt76.conf used a flawed redirect and put the sudo password line into the file; NM crashed (restart-loop), fixed within ~3 min via /tmp+sudo cp. No reboot needed, box never froze.
- NOTE: on this boot the netplan wifi profile had bound to the ONBOARD brcmfmac wlan0 (driver=brcmfmac), NOT the dongle; interface names can flip between boots, so driver-based unmanage is the correct long-term protection.

## Evidence
