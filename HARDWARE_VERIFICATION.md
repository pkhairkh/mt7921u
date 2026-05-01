# Hardware Verification Manual — MT7921U USB Wi-Fi 6E Driver

This manual provides step-by-step procedures for verifying all bug fixes,
feature implementations, and enhancements on real hardware. All work in this
repository is source-level; this document bridges the gap between source patches
and runtime validation.

---

## 1. Required Hardware and Setup

### 1.1 Hardware

| Item | Specification | Purpose |
|------|---------------|---------|
| MT7921U USB adapter | USB ID `0e8d:7961` | Device under test |
| USB 3.0 port | SuperSpeed (5 Gbps) | Avoid USB 2.0 bottlenecks |
| Wi-Fi 6E AP | 802.11ax, 6 GHz capable | TWT, CLC, 6 GHz testing |
| Wi-Fi 6 AP (minimum) | 802.11ax, 5 GHz | HE/TWT basic testing |
| Spectrum analyzer (optional) | 2.4–7.125 GHz | Regulatory power measurement |
| Bluetooth headset (optional) | Any BT 5.x | BT coexistence testing |
| Linuxptp (optional) | ptp4l, phc_ctl | HW timestamping accuracy |

### 1.2 Kernel Configuration

Enable these options in your kernel config:

```
CONFIG_WIRELESS=y
CONFIG_CFG80211=m
CONFIG_MAC80211=m
CONFIG_MT76_USB=m
CONFIG_MT7921U=m
CONFIG_DYNAMIC_DEBUG=y
CONFIG_FTRACE=y
CONFIG_DEBUG_FS=y
CONFIG_DEBUG_INFO=y
```

For advanced tracing:

```
CONFIG_EVENT_TRACING=y
CONFIG_FUNCTION_TRACER=y
CONFIG_DYNAMIC_FTRACE=y
```

### 1.3 Building and Loading the Driver

```bash
# Clone the repository
git clone https://github.com/pkhairkh/mt7921u.git
cd mt7921u

# Build out-of-tree (adjust KVER if needed)
make -C /lib/modules/$(uname -r)/build M=$PWD/drivers/net/wireless/mediatek/mt76

# Load with dynamic debug enabled
modprobe mt7921u dyndbg=+p
# Or with specific parameters:
insmod mt7921u.ko clc_force_usb=1 test_trigger_enable=1 fw_ack_enable=1
```

### 1.4 Enabling Dynamic Debug

```bash
# Enable all mt7921 debug messages
echo 'module mt7921 +p' > /sys/kernel/debug/dynamic_debug/control

# Enable specific subsystem traces
echo 'file mcu.c +p' > /sys/kernel/debug/dynamic_debug/control
echo 'file usb.c +p' > /sys/kernel/debug/dynamic_debug/control
echo 'file mac.c +p' > /sys/kernel/debug/dynamic_debug/control

# Watch dmesg in real-time
dmesg -w | grep mt7921
```

---

## 2. Step-by-Step Test Plan

### TEST-T1: Testmode NULL Dereference (BUG-01)

**What it verifies:** Patch 0001 — guard testmode against NULL `drv_own` on USB.

**Procedure:**

```bash
# 1. Load driver with test triggers enabled
insmod mt7921u.ko test_trigger_enable=1 dyndbg=+p

# 2. Verify device is up
iw phy0 info | grep -A5 "valid interface"

# 3. Trigger the testmode path
cat /sys/kernel/debug/ieee80211/phy0/mt76/test_trigger/trigger_testmode_null

# 4. Check dmesg for the guard message
dmesg | grep "testmode.*drv_own"

# 5. Attempt an iw testmode command (should fail gracefully, not Oops)
iw phy0 testmode
```

**Expected result:** With the fix, the testmode command returns an error
gracefully. Without the fix (reverted patch), the kernel would Oops with:

```
BUG: kernel NULL pointer dereference at 0x0000000000000000
PGD 0 P4D 0
Oops: 0002 [#1] PREEMPT SMP PTI
CPU: 3 PID: 1234 Comm: iw Tainted: G           O
RIP: 0010:mt7921_testmode_cmd+0xXX/0xXX [mt7921u]
```

**Pass criteria:** No Oops. Testmode returns `-EOPNOTSUPP` or similar error.

---

### TEST-T2: WTBL Poll Timeout (BUG-02)

**What it verifies:** Patch 0002 — increased WTBL poll timeout for USB.

**Procedure:**

```bash
# 1. Load driver with test triggers
insmod mt7921u.ko test_trigger_enable=1 dyndbg=+p

# 2. Trigger the WTBL poll test
cat /sys/kernel/debug/ieee80211/phy0/mt76/test_trigger/trigger_wtbl_poll

# 3. Check timing
dmesg | grep "WTBL poll"
```

**Expected result:** WTBL poll completes in under 50 ms (the new USB timeout).
On a healthy system, expect 1–10 ms.

**Predicted crash without fix:** On slow USB hubs or under heavy bus load:

```
mt7921e 0000:03:00.0: WTBL update timeout
```

**Pass criteria:** Poll completes within 50 ms. No timeout error.

---

### TEST-T3: MCU Command Retry (BUG-03)

**What it verifies:** Patch 0003 — MCU command retry on USB.

**Procedure:**

```bash
# 1. Load driver
insmod mt7921u.ko test_trigger_enable=1 dyndbg=+p

# 2. Trigger MCU command test
cat /sys/kernel/debug/ieee80211/phy0/mt76/test_trigger/trigger_mcu_retry

# 3. Check for retry messages
dmesg | grep -i "mcu.*retry\|mcu.*command"
```

**Expected result:** Under normal conditions, MCU commands succeed on first
attempt. Under stress (heavy USB traffic), the retry mechanism allows up to
3 attempts before failing.

**Pass criteria:** No chip reset caused by transient MCU command failures.
If retries occur, they succeed on subsequent attempts.

---

### TEST-T4: CLC Regulatory Compliance (BUG-01 / CLC Fix)

**What it verifies:** Patches 0006 + 0007 — CLC enable for USB + defensive fallback.

**Procedure:**

```bash
# Test A: Normal CLC loading (default behavior)
insmod mt7921u.ko dyndbg=+p
dmesg | grep -i "clc\|load_clc"

# Test B: Force CLC attempt on USB
rmmod mt7921u
insmod mt7921u.ko clc_force_usb=1 dyndbg=+p
dmesg | grep -i "clc\|load_clc"

# Test C: Trigger CLC load
cat /sys/kernel/debug/ieee80211/phy0/mt76/test_trigger/trigger_clc_load

# Test D: Check 6 GHz band availability
iw phy0 info | grep -A5 "Frequencies.*5[0-9][0-9][0-9]"
```

**Expected results:**

| Scenario | Expected |
|----------|----------|
| SET_CLC works over USB | "CLC loaded successfully" in dmesg. 6 GHz channels available. |
| SET_CLC fails over USB (default) | "CLC SET failed on USB (err=...)". "6 GHz disabled until vendor CLC path is ported". Only 2.4/5 GHz available. |
| SET_CLC fails + `clc_force_usb=1` | Error propagated normally, may trigger chip reset. |

**Regulatory measurement procedure (if CLC works):**

1. Set regulatory domain: `iw reg set DE` (or your country)
2. Connect to AP on a 6 GHz channel
3. Using a spectrum analyzer, measure conducted power at the antenna port
4. Compare with CLC-derived limits for your jurisdiction:
   - FCC (US): 30 dBm EIRP max for UNII-5
   - ETSI (EU): 20 dBm EIRP max for indoor LPI
5. If measured power exceeds CLC limits, **disable CLC immediately** with
   `disable_clc=1` and report the finding

**Fallback paths:** If CLC is rejected, manually test the alternative:

```bash
# Disable CLC and use built-in regulatory
insmod mt7921u.ko disable_clc=1
# Set regulatory domain manually
iw reg set US
```

**Pass criteria:**
- If CLC works: 6 GHz channels available, power within regulatory limits
- If CLC fails: Fallback message appears, device operates on 2.4/5 GHz only

---

### TEST-T5: Suspend/Resume with ROC Timer (BUG-06)

**What it verifies:** Patch 0004 — cancel ROC timer work on disconnect.

**Procedure:**

```bash
# 1. Load driver and connect to AP
insmod mt7921u.ko dyndbg=+p
nmcli dev wifi connect "SSID" password "PASSWORD"

# 2. Enable dynamic debug for ROC paths
echo 'file mt792x_core.c +p' > /sys/kernel/debug/dynamic_debug/control
echo 'file main.c +p' > /sys/kernel/debug/dynamic_debug/control

# 3. Suspend the system
echo mem > /sys/power/state

# 4. Resume (press power button)

# 5. Check for ROC timer UAF or warnings
dmesg | grep -i "roc\|timer\|use-after-free\|rcu"
```

**Predicted crash without fix:**

```
BUG: KASAN: use-after-free in mt7921_roc_work+0xXX/0xXX [mt7921u]
Call trace:
 mt7921_roc_work+0xXX/0xXX [mt7921u]
 process_one_work+0xXX/0xXX
 worker_thread+0xXX/0xXX
```

**Pass criteria:** No use-after-free warning. Connection restored after resume.
ROC timer state is clean.

---

### TEST-T6: Queue Wake on Chip Reset (BUG-05)

**What it verifies:** Patch 0005 — wake queues after reset failure.

**Procedure:**

```bash
# 1. Load driver and connect to AP
insmod mt7921u.ko test_trigger_enable=1 dyndbg=+p
nmcli dev wifi connect "SSID" password "PASSWORD"

# 2. Force a chip reset (if available)
echo 1 > /sys/kernel/debug/ieee80211/phy0/mt76/force_reset 2>/dev/null || true

# 3. Alternative: unplug and replug the USB adapter quickly

# 4. Check queue state
dmesg | grep -i "queue\|wake\|reset"
```

**Predicted behavior without fix:** After reset failure, all TX queues remain
stopped. `ifconfig wlan0` shows zero TX packets. No traffic can flow until
driver is reloaded.

**Pass criteria:** After reset, queues are woken. Traffic resumes.
`ifconfig wlan0` shows TX packet counter increasing.

---

### TEST-T7: TWT Functionality (TASK-007)

**What it verifies:** TWT responder implementation (Phases 1–3).

**Procedure:**

```bash
# 1. Load driver
insmod mt7921u.ko dyndbg=+p

# 2. Connect to a TWT-capable AP (802.11ax with TWT support)
nmcli dev wifi connect "TWT_AP" password "PASSWORD"

# 3. Check TWT capability advertisement
iw phy0 info | grep -i twt

# 4. Read TWT debugfs stats
cat /sys/kernel/debug/ieee80211/phy0/mt76/twt_stats

# 5. Monitor for TWT events
dmesg | grep -i twt

# 6. Run the TWT test script
./testing/test_t7_twt.sh --interface phy0
```

**Expected output from `twt_stats`:**

```
TWT Agreements: 0
Missed Service Periods: 0 (stub)
--- Active Flows ---
(none)
```

After TWT negotiation with an AP:

```
TWT Agreements: 1
Missed Service Periods: 0 (stub)
--- Active Flows ---
Flow 0: wcid=1 table_id=0 mantissa=0xNN exp=N dur=NN tsf=0xNNNN
  protection=0 flowtype=1 trigger=0
```

**Pass criteria:** TWT stats are readable. When connected to a TWT-capable AP,
agreement appears in stats within 30 seconds.

---

## 3. Predicted Crash Signatures

For reference, these are the predicted Oops/backtrace signatures from the
coroner's report for each bug. If you see any of these, the corresponding
patch was either not applied or is insufficient.

### BUG-01: Testmode NULL deref

```
BUG: kernel NULL pointer dereference
RIP: mt7921_testmode_cmd or mt7921_testmode_dump
```

### BUG-02: WTBL poll timeout

```
mt7921e: WTBL update timeout
```

Followed by potential chip reset.

### BUG-03: MCU command failure

```
Message XXXXXXXX (seq N) timeout
mt792x_reset called
```

Without the retry patch, transient failures trigger immediate chip reset.

### BUG-04: MCU retry loop

```
MCU retry N/3 for cmd 0xNN
```

This is *expected* behavior with the fix. Without it, no retries occur.

### BUG-05: Queue stall after reset

```
mt7921e: mac reset failed
(all TX queues stopped, no traffic)
```

### BUG-06: ROC timer UAF

```
BUG: KASAN: use-after-free in mt7921_roc_work
Call trace: mt7921_roc_work -> process_one_work -> worker_thread
```

---

## 4. Feature Verification Tests

### CSI Extraction (TASK-008)

```bash
# Start CSI capture (using iw vendor command)
iw phy0 vendor send 0x00e70c 0x01 <nlattr_payload>

# Read CSI data
iw phy0 vendor send 0x00e70c 0x03

# Check debugfs (if available)
ls /sys/kernel/debug/ieee80211/phy0/mt76/csi*

# Stop CSI capture
iw phy0 vendor send 0x00e70c 0x02 <nlattr_payload>
```

### Multi-Endpoint TX QoS (TASK-017)

```bash
# Verify USB endpoint count
lsusb -v -d 0e8d:7961 | grep -A20 "Endpoint Descriptor"

# Check driver detection
dmesg | grep "out_eps\|endpoint"
```

### Firmware ACK (TASK-018)

```bash
# Load with ACK enabled
insmod mt7921u.ko fw_ack_enable=1
dmesg | grep -i "ack\|firmware.*download"
```

### DFS (TASK-013)

```bash
# Configure as AP on a DFS channel
iw phy0 interface add wlan0 type __ap
iw dev wlan0 set freq 5260 HT20
# Check for radar detection messages
dmesg | grep -i "radar\|dfs\|cac"
```

### BT Coexistence (TASK-016)

```bash
# Check firmware capability
cat /sys/kernel/debug/ieee80211/phy0/mt76/bt_coex

# With BT device paired
dmesg | grep -i "coex"
```

### HW Timestamping (TASK-012)

```bash
# Check capability
iw phy0 info | grep -i timestamp

# Use linuxptp
ptp4l -i wlan0 -m -S
```

---

## 5. Module Parameter Reference

| Parameter | Default | Purpose |
|-----------|---------|---------|
| `disable_clc` | 0 | Completely disable CLC loading |
| `clc_force_usb` | 0 | Force SET_CLC on USB (no fallback) |
| `test_trigger_enable` | 0 | Enable debugfs crash-trigger files |
| `fw_ack_enable` | 0 | Enable per-chunk firmware ACK |

---

## 6. Capturing Evidence

### 6.1 Full dmesg capture

```bash
# Before testing
dmesg -C  # Clear buffer

# Run test
./testing/test_t4_clc.sh --interface phy0

# Save
dmesg > /tmp/mt7921u_test_T4_dmesg.log
```

### 6.2 ftrace capture

```bash
# Enable function tracing for driver functions
echo function > /sys/kernel/debug/tracing/current_tracer
echo 'mt7921_*' > /sys/kernel/debug/tracing/set_ftrace_filter
echo 1 > /sys/kernel/debug/tracing/tracing_on

# Run test
# ...

# Save trace
cat /sys/kernel/debug/tracing/trace > /tmp/mt7921u_trace.log
echo 0 > /sys/kernel/debug/tracing/tracing_on
```

### 6.3 trace-cmd capture

```bash
# Record all driver events
trace-cmd record -e mt7921:* -e mac80211:* -e cfg80211:* \
    ./testing/test_harness.sh --test all

# View report
trace-cmd report
```

---

## 7. Reporting Results

For each test, record:

1. **Test ID** (T1–T7)
2. **Kernel version** (`uname -r`)
3. **Driver version** (`modinfo mt7921u | grep version`)
4. **Module parameters** used
5. **PASS/FAIL/SKIP** result
6. **dmesg output** (relevant lines only)
7. **Any unexpected behavior**

File bugs at: https://github.com/pkhairkh/mt7921u/issues

Use the label `hardware-verification` and include all captured evidence.
