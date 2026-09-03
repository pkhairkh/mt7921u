# STAGE S3 - STA connect/disconnect stress - 10 cycles + recount (2026-09-03 10:00 UTC)

## Verdict: 10/10 operationally PASS (verified by recount with fixed harness)

## Harness bug fixed
- 'nmcli -g GENERAL.STATE' returns '30 (disconnected)' WITH parenthetical label; harness compared to bare '30' -> false FAILs. Recount with '30*' match confirms true state transitions.

## Results (all rc=0, zero dmesg badness every cycle, box alive, uptime continuous 1210s->1318s)

| cycle | connect | state | ip | ping | disconnect |
|-------|---------|-------|----|------|------------|
| 1  | 915ms  | 100 connected | 192.168.178.147 | 0% | 302ms |
| 2  | 967ms  | 100 connected | 192.168.178.147 | 0% | 486ms |
| 3  | 1222ms | 100 connected | 192.168.178.147 | 0% | 333ms |
| 4  | 4529ms | 100 connected | 192.168.178.147 | 0% | 315ms |
| 5  | 1048ms | 100 connected | 192.168.178.147 | 0% | 272ms |
| 6  | 1066ms | 100 connected | 192.168.178.147 | 0% | 292ms |
| 7  | 1075ms | 100 connected | 192.168.178.147 | 0% | 271ms |
| 8  | 1033ms | 100 connected | 192.168.178.147 | 0% | 282ms |
| H1 | ok     | 100 connected | 30x ping 0% loss + scan-while-assoc 22 BSS | 361ms |
| H2 | ok     | 100 connected | 30x ping 0% loss + scan-while-assoc 19 BSS | 380ms |

## Significance
- This is the EXACT trigger of the 3 historical machine freezes + the Phase-5 deadlock: rapid NM connect/disconnect on the dongle.
- Disconnect latency (the deadlock signature) now 271-486ms mean ~318ms, vs pre-BUG-07-fix: hard 10s timeout + hung_task + watchdog reset. 30x margin.
- Scan-while-associated (ROC/offchannel path, patched timer code) worked under association with traffic.
- Managed window opened/closed 3x total (incl. recounts) - NM restart cycles clean, eth0 lifeline never disturbed.
