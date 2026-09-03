# PHASE 10 — xHCI controller deaths root-caused; SSD copy fixed + BOT migration

## xHCI death #1 (2026-09-03 15:20 CEST, boot 0fe3cdba)
- t+9min: JMicron 152d:0583 attached (sda). D1 script read SSD journal via mmap
- 15:20:47-48: 13x uas_eh_abort_handler (CMD IN reads)
- 15:20:48: xhci_hcd "WARNING: Host System Error"
- 15:20:53: "host not responding, assume dead" -> "HC died; cleaning up"; all USB lost (both hubs, 2x dongle, bridge); SIGBUS in userspace reader
- Machine SURVIVED (root on SD, eth0 on separate MAC controller) — on SSD-boot days this same event = total freeze (rootfs bus death)

## xHCI death #2 (2026-09-04 session, t=403s of boot)
- dd 256M direct read test against sda in UAS mode -> identical abort storm -> HSE -> HC died
- CONCLUSION: UAS mode on JMS583 + VL805 = deterministic controller death under sustained I/O. 2/2 reproducible.

## Fix deployed
- usb-storage.quirks=152d:0583:u in BOTH cmdline.txt (SD + SSD) + live module param
- BOT mode verified: full 2.2GB delta copy + fsck with ZERO xHCI errors (vs UAS dying in <7min)

## SSD-boot failure analysis (why "cannot ssh when booting through ssd")
- m5 reboot (15:07) = WARM reboot; bridge stayed wedged from Linux UAS session; USB_MSD_PWR_OFF_TIME=0 (no power cycle) -> bootloader could not read bridge -> silent fallback to SD (journal of boot 15:07:48 lives on SD, full SSH session present)
- D1 (other agent) had repaired garbage fstab (password line bug) before this; content copy (m3) otherwise verified OK incl. patched modules fingerprints
- EEPROM now: BOOT_ORDER=0xf14 (USB first, SD fallback), USB_MSD_PWR_OFF_TIME=3000, USB_MSD_STARTUP_DELAY=5000 (pending apply on next SD boot)

## Copy incident disclosure (this agent)
- D1 run: sudo -S tee stdin conflict (password line consumed by sudo) -> unbind/quirk writes no-op'd -> sda dropped -> mounts failed -> rsync wrote 4.35GB self-copy to SD /mnt/newroot. CLEANED (rm -rf, df restored 4.2G used). Zero damage to system files. Root cause documented; corrected W() write helper used thereafter.

## Final SSD state (gates all green)
- cmdline: root=PARTUUID=ffbc70da-02 + usb-storage.quirks=152d:0583:u, single line
- fstab: ffbc70da-01/02 only, no SD-uuid leak
- ssh.service enabled; journald persistent drop-in; NM unmanage; sysctl debug; all 7 patched .ko in updates/
- e2fsck -fn /dev/sda2: clean
- Pending: cold-boot test from SSD (SD removed)
