# STAGE S4 - traffic load + concurrency + rapid-fire (2026-09-03 10:06 UTC)

## Results (zero kernel badness across ENTIRE S4 incl. crash-recovery; box alive; uptime continuous to 1774s)

### PART A - 3x (connect -> 45s mixed-size load w/ 3 mid-load scans -> disconnect UNDER load)
- Run completed before harness typo crash (set -u, $RC vs $CRC at CWS echo); evidence recovered:
- small ping stream (300x 56B @ 20pps): 298 recv +1 dup, 0.67% loss (offchannel scan dips)
- big ping stream (300x 1472B @ 20pps): 300 recv, 0% loss
- 3 concurrent scans per cycle executed during load (offchannel ROC under tx/rx)
- disconnect executed DURING an active 1472B ping burst (queue teardown under traffic): completed, state 30, no errors
- kernel badness counter for whole S4 (journal -k grep): 0

### PART B - connect-WHILE-scanning x3 (association during offchannel churn)
- 3/3 PASS: connect rc=0 while 4-scan background loop running; ping 0%; disconnect clean; dmesg bad=0

### PART C - rapid-fire connect/disconnect x5 (no settle time between)
- 5/5 PASS: conn rc=0, dis rc=0, bad=0 every time

## Cumulative stress tally (S1-S4, single continuous boot 09:36 UTC, ~30 min)
- probe/remove/CLC/firmware: 27 full cycles | scans: ~110 ops + 5x 3-way concurrent races
- regdomain switches: 12 | rfkill: 10 | iface flaps: 10
- STA association/disassociation cycles: 22 (10 S3 + 2 recount + 3 load + 3 CWS + 5 rapid-fire + 1 partial-A)
- kernel errors/timeouts/warnings: 0 | freezes: 0 | reboots: 0

## Next: S5 idle soak (reproduces post-CHECKPOINT-5 dark state), S6 controlled reboot + final verdict
