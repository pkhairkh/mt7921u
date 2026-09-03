# STAGE S2 - scan/regdomain/rfkill/flap stress (2026-09-03 09:54 UTC)

## Results (box alive throughout, uptime continuous 921s->1023s across parts, stack 7/7, zero cumulative dmesg badness)

### S1 record correction
- Canonical swap recount with fixed regex: stack=7/7, mt76=143360, srcversion exact -> S1 canonical swap 3/3 TRUE PASS (harness bug: '^mt76$' cannot match lsmod lines with trailing columns)

### PART A - scan flood
- 30x full scans (2.4+5GHz): 30/30 PASS, 19-24 BSS each, rc=0
- 40x single-freq (ch 1-14 + 5180-5955): 68/70 total PASS; the 2 non-pass results = BSS 0 on passive-DFS/11p channels (environmental - no transmitters; iw rc=0; identified above)
- ~95 total scan ops incl. concurrent+flap-embedded scans; zero driver errors

### PART A2 - concurrent scan races (3 parallel iw scans x5 rounds)
- 5/5 rounds zero dmesg badness (driver -EBUSY/-EAGAIN concurrency path clean)

### PART B - regdomain churn
- 2 full cycles of US/JP/CN/GB/00/DE: 12/12 PASS; iw reg get reflects each switch (DFS-FCC/DFS-JP/DFS-ETSI/UNSET); channel table reload clean, zero errors

### PART C - rfkill block/unblock x10
- 10/10 PASS (block->iface down, unblock->iface back, zero dmesg errors)

### PART D - iface down/up flaps x10 + scan-when-up
- 10/10 PASS (start/stop interface MCU paths + radio-alive scan each flap)

## Coverage of patched code paths so far
- S1: probe/remove/CLC/firmware-load x27 | S2: scan engine x~95, regdomain x12, rfkill x10, start/stop x10
- Next S3: 10x STA connect/disconnect cycles + scan-while-associated (ROC path) - the historical freeze trigger
