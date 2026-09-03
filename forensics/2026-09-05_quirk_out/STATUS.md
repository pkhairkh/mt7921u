# 2026-09-05 — "quirks out" full-tree audit, BUG-12/BUG-13 completion, quirk removal

## Scope

User directive: check and adjust the repo (already on the Pi) so the quirks
are out, fix ALL issues, re-audit, commit and push. Solo agent session,
device dongles unplugged (only VIA hub + JMS583 SSD bridge on the bus).

## What was done

1. Full solo audit of the mt76 tree at 78943c9 (0012 state), cross-checked
   against the live deployment:
   - BUG-01..BUG-11 fixes verified present in-tree (testmode guard, WTBL
     50 ms USB poll, ROC timer cleanup, sta_remove mutex fix, txpower,
     survey NF, 0012 hardening).
   - Found BUG-12 (survey/MIB error-propagation gap, 0013 authored in the
     offline session 4, never built) — applied via git am.
   - Found BUG-13 (escalation engine incomplete: success-clear rescue
     counters, reset_work gives up after 10 failed cycles, mac_work never
     re-armed after reset, -EILSEQ missing from fast-fail set) — patched
     as 0014.
2. Built on-target: make -j4 -C /usr/src/linux-headers-6.18.34+rpt-rpi-v8
   M=.../mt76 modules — RC=0 (one pre-existing warning in
   test_trigger_debugfs.c, unrelated). "Clock skew detected" warning is
   the known broken Pi clock, not a build problem.
3. Installed the 8 rebuilt modules to /lib/modules/6.18.34+rpt-rpi-v8/updates/
   (old set preserved at /home/pkhairkh/module-backups/updates.bak-20260905/),
   depmod, modprobe load/unload test clean.
4. Removed `usbcore.quirks=0e8d:7961:n` (DELAY_CTRL_MSG) from
   /boot/firmware/cmdline.txt — see ISSUES.md Part VII quirk audit for the
   rationale (200 ms sleep after EVERY usb_control_msg; fw download is
   thousands of control chunks; deployed undocumented outside the repo).
   `usb-storage.quirks=152d:0583:u` (JMS583 BOT) KEPT: boot-critical,
   PHASE 10 evidence, different subsystem.
   Effective on next reboot; no runtime effect today (dongles unplugged).

## Evidence in this directory

- cmdline_before.txt / cmdline_after.txt / cmdline_original_backup.txt
- modules_old.sha256 / modules_new.sha256 (ID cards)
- verification.txt (strings + modprobe + modules.dep resolution)

## Verification

- New code confirmed inside deployed modules via strings() on the correct
  module files (mt76-usb.ko, mt792x-lib.ko, mt7921-common.ko).
- Full module chain modprobe/rmmod clean (no device attached).
- modules.dep resolves mt7921u -> updates/ stack.
- Runtime wedge verification PENDING: needs the dongles re-attached; use
  the plan in ISSUES.md Part VI (DFS-pinned auth test with D-state poller).
