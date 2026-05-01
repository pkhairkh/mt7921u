# TASKS.md — Prioritized Task Breakdown

> All tasks are derived from the forensic audit findings in ISSUES.md and the
> enhancement roadmap in the coroner's report. Each task references its source
> issue(s), assigned agent, dependencies, and acceptance criteria.

---

## Legend

| Symbol | Meaning |
|--------|---------|
| `[ ]` | Not started |
| `[~]` | In progress |
| `[x]` | Completed |
| `[!]` | Blocked |
| `[?]` | Needs investigation |

---

## Priority 0 — Critical Bug Fixes (No Dependencies)

These tasks fix proven bugs that cause crashes, data corruption, or regulatory
non-compliance. They require no feature work as a prerequisite.

---

### TASK-001: Fix Testmode NULL Dereference on USB

| Field | Value |
|-------|-------|
| **Status** | `[ ]` |
| **Source** | ISSUES.md BUG-02 |
| **Assigned** | `patch-engineer` |
| **Dependencies** | None |
| **Estimate** | 2 hours |
| **Bus impact** | USB only |
| **Evidence** | Proven (source-level) |

**Description:** `mt7921_tm_set()` at `testmode.c:60` calls `__mt792x_mcu_drv_pmctrl(dev)` which expands to `(dev)->hif_ops->drv_own(dev)`. On USB, `drv_own` is NULL in the `hif_ops` struct (`usb.c:165-169`), causing a kernel NULL pointer dereference panic.

**Acceptance criteria:**
- [ ] Patch adds NULL check for `drv_own` before calling it (either in the macro or at call site)
- [ ] If `drv_own` is NULL, the call is skipped (USB does not need PM ownership transfer)
- [ ] PCIe and SDIO behavior is unchanged
- [ ] `checkpatch.pl --strict` clean
- [ ] Commit message includes `Fixes:` tag

**Predicted crash signature (to confirm at runtime):**
```
BUG: kernel NULL pointer dereference, address: 0000000000000000
RIP: 0010:0x0
Call Trace:
 mt7921_tm_set+0xXX/0xXXX [mt7921e]
 mt7921_testmode_cmd+0xXX/0xXXX [mt7921e]
 nl80211_testmode_cmd+...
```

---

### TASK-002: Fix WTBL Poll Timeout for USB

| Field | Value |
|-------|-------|
| **Status** | `[ ]` |
| **Source** | ISSUES.md BUG-03 |
| **Assigned** | `patch-engineer` |
| **Dependencies** | None |
| **Estimate** | 3 hours |
| **Bus impact** | USB only (guarded with `mt76_is_usb()`) |
| **Evidence** | Proven (source-level) |

**Description:** `mt7921_mac_wtbl_update()` uses `mt76_poll()` with a 5000-microsecond timeout. On USB, each `mt76_rr()` requires a USB vendor request round-trip (~1ms), so the driver gets only ~5 poll iterations before timing out. Called 20 times during `mt7921_mac_init()`, this causes a ~10-second hang holding `dev->mt76.mutex`.

**Acceptance criteria:**
- [ ] Patch uses `mt76_poll_msec()` with 50ms timeout for USB, preserving `mt76_poll()` for PCIe/SDIO
- [ ] Bus detection via `mt76_is_usb(&dev->mt76)`
- [ ] PCIe performance not degraded
- [ ] Return value of `mt7921_mac_wtbl_update()` checked by at least one caller (optional improvement)
- [ ] `checkpatch.pl --strict` clean

---

### TASK-003: Add MCU Command Retry Before Chip Reset

| Field | Value |
|-------|-------|
| **Status** | `[ ]` |
| **Source** | ISSUES.md BUG-04 |
| **Assigned** | `patch-engineer` |
| **Dependencies** | None |
| **Estimate** | 4 hours |
| **Bus impact** | All buses (USB benefits most, PCIe/SDIO unaffected) |
| **Evidence** | Proven (source-level) |

**Description:** `mt7921_mcu_parse_response()` at `mcu.c:25-31` immediately calls `mt792x_reset()` when an MCU command times out. On USB, transient delays (autosuspend, cable issues, host controller scheduling) can cause one-time timeouts that don't indicate firmware death. The driver should retry the MCU command before resetting the chip.

**Acceptance criteria:**
- [ ] Per-device `mcu_timeout_count` field added to `struct mt792x_dev` (NOT a global atomic)
- [ ] Module parameter `mcu_timeout_retries` (default: 2) controls retry threshold
- [ ] On timeout: increment counter, log warning with attempt count, return `-ETIMEDOUT` without reset
- [ ] On success: reset counter to 0
- [ ] Reset only triggered when counter exceeds threshold
- [ ] PCIe/SDIO behavior preserved (they rarely hit this path)

---

### TASK-004: Fix ROC Timer Use-After-Free on USB Disconnect

| Field | Value |
|-------|-------|
| **Status** | `[ ]` |
| **Source** | ISSUES.md BUG-06 |
| **Assigned** | `patch-engineer` |
| **Dependencies** | None |
| **Estimate** | 3 hours |
| **Bus impact** | USB only |
| **Evidence** | Proven (reinstated — v3 incorrectly retired this) |

**Description:** On USB disconnect, `mt792xu_disconnect()` does NOT cancel `phy->roc_timer` or `phy->roc_work`. If ROC has expired from mac80211's perspective, `ieee80211_roc_purge_local()` has nothing to purge, and the timer survives past `mt76_free_device()`. The timer then fires and accesses freed memory.

**Key insight (self-correction):** v3 incorrectly retired this as "disproved" by claiming `ieee80211_unregister_hw()` → `ieee80211_roc_purge_local()` cancels the timer. But `roc_purge_local()` only cancels mac80211-managed offchannel ops — if the ROC has already expired from mac80211's view but the driver's `roc_timer` hasn't fired, there is no `ieee80211_roc` to purge.

**Acceptance criteria:**
- [ ] Add `timer_delete_sync(&dev->phy.roc_timer)` in `mt792xu_cleanup()` BEFORE `mt76_free_device()`
- [ ] Add `cancel_work_sync(&dev->phy.roc_work)` after timer deletion
- [ ] Also add `mt7921_roc_abort_sync(dev)` in `mt7921u_suspend()` (suspend path has same issue)
- [ ] PCIe/SDIO disconnect paths unchanged
- [ ] `checkpatch.pl --strict` clean

---

### TASK-005: Enable CLC for USB (Experimental — P0 Feature)

| Field | Value |
|-------|-------|
| **Status** | `[ ]` |
| **Source** | ISSUES.md BUG-01, FEATURE-01 |
| **Assigned** | `patch-engineer` + `firmware-analyst` |
| **Dependencies** | TASK-003 (retry mechanism must be in place first) |
| **Estimate** | 1-2 weeks (including firmware viability testing) |
| **Bus impact** | USB only |
| **Evidence** | Proven (CLC skip at mcu.c:425-427) |

**Description:** The `mt7921_load_clc()` function returns 0 immediately for USB devices, blocking 6 GHz and leaving 2.4/5 GHz without country-specific power limits. This is both a feature gap and a regulatory compliance risk.

**Sub-tasks:**
- [ ] TASK-005a: Remove `mt76_is_usb()` from CLC skip condition (experimental)
- [ ] TASK-005b: Test whether `MCU_CE_CMD(SET_CLC)` works over USB bulk transport
- [ ] TASK-005c: If SET_CLC fails over USB, implement alternative regulatory path (vendor driver's `CMD_ID_CAL_BACKUP_IN_HOST_V2` + direct channel tables)
- [ ] TASK-005d: Verify 6 GHz channels appear in `iw phy0 info` after CLC load
- [ ] TASK-005e: Verify 2.4/5 GHz power limits are applied correctly

**Risk:** If `SET_CLC` doesn't work over USB, the command will time out, triggering TASK-003's retry mechanism, then a chip reset. This is why TASK-003 is a prerequisite.

**Acceptance criteria:**
- [ ] CLC data is loaded on USB devices
- [ ] 6 GHz channels appear in wiphy (if firmware supports it)
- [ ] Country-specific power limits are applied to all bands
- [ ] No regression on PCIe/SDIO
- [ ] Document whether SET_CLC works over USB or alternative path is needed

---

### TASK-006: Add Missing Queue Wake on USB Reset Error Path

| Field | Value |
|-------|-------|
| **Status** | `[ ]` |
| **Source** | ISSUES.md BUG-05 |
| **Assigned** | `patch-engineer` |
| **Dependencies** | None |
| **Estimate** | 2 hours |
| **Bus impact** | USB primarily; SDIO path also affected |
| **Evidence** | Proven (source-level) |

**Description:** In `mt7921u_mac_reset()`, `ieee80211_stop_queues()` is never called before the reset sequence, and there is no `ieee80211_wake_queues()` on any exit path. During the ~10-second USB reset, mac80211 can continue queuing TX frames that will fail on a resetting chip. Additionally, the SDIO early-return path at `mac.c:678` can leave queues permanently stopped.

**Acceptance criteria:**
- [ ] Add `ieee80211_stop_queues(hw)` at the start of `mt7921u_mac_reset()`
- [ ] Add `ieee80211_wake_queues(hw)` on ALL exit paths (success and failure)
- [ ] Add `dev->hw_full_reset = true/false` around the reset
- [ ] Cancel `pm->ps_work` and `pm->wake_work` during reset
- [ ] SDIO early-return path also gets `ieee80211_wake_queues()`
- [ ] `checkpatch.pl --strict` clean

---

## Priority 1 — High-Impact Feature Implementations

These tasks implement the highest-impact features that the firmware already
supports but the driver does not expose.

---

### TASK-007: Implement TWT Responder

| Field | Value |
|-------|-------|
| **Status** | `[ ]` |
| **Source** | ISSUES.md FEATURE-02, ENHANCE-02 |
| **Assigned** | `patch-engineer` + `research-scout` |
| **Dependencies** | TASK-005 (6 GHz working preferred but not required) |
| **Estimate** | 3-4 months |
| **Bus impact** | All buses |
| **Evidence** | Proven (firmware command 0x94 exists) |

**Sub-tasks:**
- [ ] TASK-007a: Port vendor driver's TWT MCU command structures (`UNI_CMD_ID_TWT` with tags)
- [ ] TASK-007b: Implement `.add_twt_setup` and `.twt_teardown_request` mac80211 ops
- [ ] TASK-007c: Add TWT capability bits to HE MAC capabilities
- [ ] TASK-007d: Implement TWT responder (AP-side) from vendor's `twt.c` / `twt_planner.c`
- [ ] TASK-007e: Add eBPF hook for TWT agreement setup (optional, ENHANCE-02)
- [ ] TASK-007f: Expose TWT statistics via debugfs

---

### TASK-008: Implement CSI Extraction for Wi-Fi Sensing

| Field | Value |
|-------|-------|
| **Status** | `[ ]` |
| **Source** | ISSUES.md ENHANCE-03 |
| **Assigned** | `patch-engineer` + `research-scout` |
| **Dependencies** | None (structural) |
| **Estimate** | 2-3 months |
| **Bus impact** | All buses |
| **Evidence** | Speculative (firmware CSI interface not yet reverse-engineered) |

**Sub-tasks:**
- [ ] TASK-008a: Reverse-engineer firmware CSI capture command from vendor driver
- [ ] TASK-008b: Add vendor nl80211 command for CSI request
- [ ] TASK-008c: Implement CSI data extraction from RX descriptor
- [ ] TASK-008d: Expose CSI data as binary netlink attribute
- [ ] TASK-008e: Add radiotap field for per-frame CSI (Wireshark integration)
- [ ] TASK-008f: Document CSI data format

---

## Priority 2 — Enhanced Capabilities

---

### TASK-009: Implement eBPF-Based Channel Survey and ACS

| Field | Value |
|-------|-------|
| **Status** | `[ ]` |
| **Source** | ISSUES.md ENHANCE-01 |
| **Assigned** | `patch-engineer` + `research-scout` |
| **Dependencies** | `.get_survey` implementation |
| **Estimate** | 2-3 months |
| **Bus impact** | All buses |

**Sub-tasks:**
- [ ] TASK-009a: Implement `.get_survey` to populate `survey_info` from `mt76_channel_state`
- [ ] TASK-009b: Define `BPF_PROG_TYPE_WIFI_SURVEY` hook type (kernel patch required)
- [ ] TASK-009c: Add BPF hook that fires when survey data is updated
- [ ] TASK-009d: Implement userspace BPF program for ML-based channel selection
- [ ] TASK-009e: Add nl80211 channel recommendation interface

---

### TASK-010: Implement AF_XDP Zero-Copy Monitor Mode

| Field | Value |
|-------|-------|
| **Status** | `[ ]` |
| **Source** | ISSUES.md ENHANCE-03 |
| **Assigned** | `patch-engineer` |
| **Dependencies** | Monitor mode implementation |
| **Estimate** | 2-3 months |
| **Bus impact** | USB primarily |

**Sub-tasks:**
- [ ] TASK-010a: Add `NL80211_IFTYPE_MONITOR` to supported interface types
- [ ] TASK-010b: Implement monitor mode in `mt7921_add_interface()` using `mt7921_mcu_set_sniffer()`
- [ ] TASK-010c: Extend `mt7921_configure_filter()` for monitor-specific FIF flags
- [ ] TASK-010d: Allocate AF_XDP UMEM for monitor interface
- [ ] TASK-010e: Map RX buffers from UMEM into RX URB completion handler
- [ ] TASK-010f: Implement zero-copy pointer swap delivery

---

## Priority 3 — Advanced Features

---

### TASK-011: Implement OFDMA-Aware MU Scheduling with CTI Mitigation

| Field | Value |
|-------|-------|
| **Status** | `[ ]` |
| **Source** | ISSUES.md ENHANCE-05 |
| **Assigned** | `patch-engineer` + `research-scout` |
| **Dependencies** | AP mode implementation, CSI extraction |
| **Estimate** | 4-6 months |
| **Bus impact** | All buses (AP-mode only) |

**Sub-tasks:**
- [ ] TASK-011a: Implement AP mode (see FEATURE-05 in ISSUES.md)
- [ ] TASK-011b: Implement OFDMA scheduler reading per-STA CSI from firmware
- [ ] TASK-011c: Add CTI detection using CSI snapshot method
- [ ] TASK-011d: Implement dynamic RU allocation avoiding interfered subcarriers

---

### TASK-012: Implement Hardware Timestamping for PTP

| Field | Value |
|-------|-------|
| **Status** | `[ ]` |
| **Source** | ISSUES.md ENHANCE-06 |
| **Assigned** | `patch-engineer` |
| **Dependencies** | None |
| **Estimate** | 2-4 weeks |
| **Bus impact** | All buses |

**Sub-tasks:**
- [ ] TASK-012a: Implement `ndo_get_tstamp` to extract TSF from RX descriptor
- [ ] TASK-012b: Add `SOF_TIMESTAMPING_RX_HARDWARE` support
- [ ] TASK-012c: Verify TX hardware timestamping capability
- [ ] TASK-012d: Test with `linuxptp` / `ptp4l`

---

## Recurring Tasks

---

### TASK-R01: Quarterly Source Audit

| Field | Value |
|-------|-------|
| **Frequency** | Quarterly or on wireless-next merge |
| **Assigned** | `code-auditor` |
| **Output** | New ISSUES.md entries |

---

### TASK-R02: Vendor Driver Delta Analysis

| Field | Value |
|-------|-------|
| **Frequency** | On vendor driver release |
| **Assigned** | `code-auditor` + `firmware-analyst` |
| **Output** | ISSUES.md updates, new TASKS.md entries |

---

### TASK-R03: CI Pipeline Maintenance

| Field | Value |
|-------|-------|
| **Frequency** | Continuous |
| **Assigned** | `release-coordinator` |
| **Output** | `.github/workflows/` updates |
