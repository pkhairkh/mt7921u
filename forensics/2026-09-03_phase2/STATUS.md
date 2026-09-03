# PHASE 2 - Forensic hardening + config.txt incident (2026-09-03 08:49 UTC)

## Hardening applied
- journald: /etc/systemd/journald.conf.d/40-rpi-volatile-storage.conf -> [Journal] Storage=persistent (same-name override of /usr/lib drop-in), /var/log/journal created, journald restarted
- sysctl: /etc/sysctl.d/99-mt76-debug.conf -> hung_task_timeout_secs=10, sysrq=1 (verified live)
- ramoops: dtoverlay=ramoops-pi4 appended as LAST line of config.txt, byte-exact verified

## INCIDENT: sudo-tee stdin leak corrupted config.txt (root-caused)
- Symptom: after 'echo PW | sudo -S tee -a config.txt < stage-file', config.txt gained the line 'pokxug-...' (the sudo PASSWORD) instead of the staged content; wc-l guard also lied (counted the pipe, not the file).
- Mechanism: with a fresh sudo timestamp, sudo does not consume stdin; the piped password line flows to the command run under sudo (tee), bypassing the outer < redirect (bash redirects on a function call do not override pipes INSIDE the function body). tee then appends the password to the target file. This reproduces the 'garbage into config.txt' failure that hurt the previous install.
- Fix: staged grep -v in /tmp as normal user, installed via 'sudo cp' (cp never reads stdin), byte-exact diff verified; exactly 1 garbage line removed, ramoops line added (verified 1 occurrence, 0 password remnants, 55 lines).
- RULE for all future scripts: only ever run non-stdin-reading commands under sudo (cp, bash -c with file args, systemctl, modprobe); NEVER tee/cat-redirect under sudo with a password pipe.

## Next: single planned reboot to arm ramoops + prove journal persistence
