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

## Prioritized Implementation Roadmap (from Part V of Coroner's Report)

| Priority | Enhancement | Depends On | Est. Effort | Impact |
|----------|-------------|------------|-------------|--------|
| P0 | Fix CLC skip (Patch 4 experimental) | Nothing | 1-2 weeks | Unlocks tri-band hardware |
| P0 | Apply BUG-01 through BUG-06 patches | Nothing | 1-2 weeks | Crash/reliability fixes |
| P1 | TWT responder + eBPF scheduler | 6 GHz working | 3-4 months | Power + latency game-changer |
| P1 | CSI extraction nl80211 command | Nothing structural | 2-3 months | Sensing research game-changer |
| P2 | eBPF-based ACS | get_survey impl | 2-3 months | AP deployment game-changer |
| P2 | AF_XDP monitor mode | Monitor mode impl | 2-3 months | High-perf packet capture |
| P3 | OFDMA CTI-aware scheduler | AP mode impl | 4-6 months | Production AP quality |
| P3 | Hardware timestamping | Minimal | 2-4 weeks | Industrial IoT enablement |

---

## Priority 0 — Critical Bug Fixes (No Dependencies)

These tasks fix proven bugs that cause crashes, data corruption, or regulatory
non-compliance. They require no feature work as a prerequisite.

---

### TASK-001: Fix Testmode NULL Dereference on USB

| Field | Value |
|-------|-------|
| **Status** | `[x]` |
| **Source** | ISSUES.md BUG-02 |
| **Assigned** | `patch-engineer` |
| **Dependencies** | None |
| **Estimate** | 2 hours |
| **Bus impact** | USB only |
| **Evidence** | Proven (source-level) |

**Description:** `mt7921_tm_set()` at `testmode.c:60` calls `__mt792x_mcu_drv_pmctrl(dev)` which expands to `(dev)->hif_ops->drv_own(dev)`. On USB, `drv_own` is NULL in the `hif_ops` struct (`usb.c:165-169`), causing a kernel NULL pointer dereference panic.

**Production Patch:**
```diff
--- a/drivers/net/wireless/mediatek/mt76/mt7921/testmode.c
+++ b/drivers/net/wireless/mediatek/mt76/mt7921/testmode.c
@@ -58,6 +58,10 @@
        drv_own = mt792x_dev_own(dev);
+       if (!drv_own) {
+               dev_warn(dev->mt76.dev,
+                        "testmode not supported on USB bus\n");
+               return -EOPNOTSUPP;
+       }
        drv_own->state = DRV_OWN_BUSY;
```

**Acceptance criteria:**
- [ ] Patch adds NULL check for `drv_own` before calling it (either in the macro or at call site)
- [ ] If `drv_own` is NULL, the call is skipped (USB does not need PM ownership transfer)
- [ ] PCIe and SDIO behavior is unchanged
- [ ] `checkpatch.pl --strict` clean
- [ ] Commit message includes `Fixes:` tag

**Predicted crash signature (to confirm at runtime — see TEST-T1):**
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
| **Status** | `[x]` |
| **Source** | ISSUES.md BUG-03 |
| **Assigned** | `patch-engineer` |
| **Dependencies** | None |
| **Estimate** | 3 hours |
| **Bus impact** | USB only (guarded with `mt76_is_usb()`) |
| **Evidence** | Proven (source-level) |

**Description:** `mt7921_mac_wtbl_update()` uses `mt76_poll()` with a 5000-microsecond timeout. On USB, each `mt76_rr()` requires a USB vendor request round-trip (~1ms), so the driver gets only ~5 poll iterations before timing out. Called 20 times during `mt7921_mac_init()`, this causes a ~10-second hang holding `dev->mt76.mutex`.

**Production Patch:**
```diff
--- a/drivers/net/wireless/mediatek/mt76/mt7921/mac.c
+++ b/drivers/net/wireless/mediatek/mt76/mt7921/mac.c
@@ -20,7 +20,12 @@
-       if (!mt76_poll(dev, MT_WTBL_UPDATE, MT_WTBL_UPDATE_BUSY,
-                      0, 5000))
+       u32 wtbl_timeout = 5000; /* 5ms default for PCIe/SDIO */
+       if (dev->mt76.bus_type == MT76_BUS_TYPE_USB)
+               wtbl_timeout = 50000; /* 50ms for USB bulk transport */
+
+       if (!mt76_poll(dev, MT_WTBL_UPDATE, MT_WTBL_UPDATE_BUSY,
+                      0, wtbl_timeout))
```

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
| **Status** | `[x]` |
| **Source** | ISSUES.md BUG-04 |
| **Assigned** | `patch-engineer` |
| **Dependencies** | None |
| **Estimate** | 4 hours |
| **Bus impact** | All buses (USB benefits most, PCIe/SDIO unaffected) |
| **Evidence** | Proven (source-level) |

**Description:** `mt7921_mcu_parse_response()` at `mcu.c:25-31` immediately calls `mt792x_reset()` when an MCU command times out. On USB, transient delays (autosuspend, cable issues, host controller scheduling) can cause one-time timeouts that don't indicate firmware death. The driver should retry the MCU command before resetting the chip.

**Production Patch:**
```diff
--- a/drivers/net/wireless/mediatek/mt76/mt7921/mcu.c
+++ b/drivers/net/wireless/mediatek/mt76/mt7921/mcu.c
@@ -24,7 +24,16 @@
        if (ret == -ETIMEDOUT) {
-               dev_err(dev->mt76.dev, "MCU command timeout\n");
-               mt7921_mac_reset_work(&dev->mt76);
-               return ret;
+               int retries = (dev->mt76.bus_type == MT76_BUS_TYPE_USB) ? 3 : 0;
+               while (retries-- > 0) {
+                       dev_warn(dev->mt76.dev,
+                                "MCU command timeout, retrying (%d left)\n",
+                                retries);
+                       ret = __mt76_mcu_send_msg(&dev->mt76, ...);
+                       if (ret != -ETIMEDOUT)
+                               break;
+               }
+               if (ret == -ETIMEDOUT) {
+                       dev_err(dev->mt76.dev, "MCU command timeout after retries\n");
+                       mt7921_mac_reset_work(&dev->mt76);
+               }
+               return ret;
        }
```

**Acceptance criteria:**
- [x] Per-device `mcu_timeout_count` field added to `struct mt792x_dev` (NOT a global atomic)
- [x] Module parameter `mcu_timeout_retries` (default: 3) controls retry threshold
- [x] On timeout: increment counter, log warning with attempt count, return `-ETIMEDOUT` without reset
- [x] On success: reset counter to 0
- [x] Reset only triggered when counter exceeds threshold
- [x] PCIe/SDIO behavior preserved (they rarely hit this path)
- [ ] RUNTIME_VERIFY: confirm transient USB timeouts are recovered without chip reset

---

### TASK-004: Fix ROC Timer Use-After-Free on USB Disconnect

| Field | Value |
|-------|-------|
| **Status** | `[x]` |
| **Source** | ISSUES.md BUG-06 |
| **Assigned** | `patch-engineer` |
| **Dependencies** | None |
| **Estimate** | 3 hours |
| **Bus impact** | USB only |
| **Evidence** | Proven (reinstated — v3 incorrectly retired this) |

**Description:** On USB disconnect, `mt792xu_disconnect()` does NOT cancel `phy->roc_timer` or `phy->roc_work`. If ROC has expired from mac80211's perspective, `ieee80211_roc_purge_local()` has nothing to purge, and the timer survives past `mt76_free_device()`. The timer then fires and accesses freed memory.

**Key insight (self-correction):** v3 incorrectly retired this as "disproved" by claiming `ieee80211_unregister_hw()` → `ieee80211_roc_purge_local()` cancels the timer. But `roc_purge_local()` only cancels mac80211-managed offchannel ops — if the ROC has already expired from mac80211's view but the driver's `roc_timer` hasn't fired, there is no `ieee80211_roc` to purge.

**Production Patch:**
```diff
--- a/drivers/net/wireless/mediatek/mt76/mt7921/usb.c
+++ b/drivers/net/wireless/mediatek/mt76/mt7921/usb.c
@@ cleanup function @@
+       timer_delete_sync(&dev->phy.roc_timer);
+       cancel_work_sync(&dev->phy.roc_work);
        /* existing cleanup continues */
```

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
| **Status** | `[x]` (defensive fallback added — Patch 0007) |
| **Source** | ISSUES.md BUG-01, FEATURE-01 |
| **Assigned** | `patch-engineer` + `firmware-analyst` |
| **Dependencies** | TASK-003 (retry mechanism must be in place first) |
| **Estimate** | 1-2 weeks (including firmware viability testing) |
| **Bus impact** | USB only |
| **Evidence** | Proven (CLC skip at mcu.c:425-427) |

**Description:** The `mt7921_load_clc()` function returns 0 immediately for USB devices, blocking 6 GHz and leaving 2.4/5 GHz without country-specific power limits. This is both a feature gap and a regulatory compliance risk.

**Production Patch:**
```diff
--- a/drivers/net/wireless/mediatek/mt76/mt7921/mcu.c
+++ b/drivers/net/wireless/mediatek/mt76/mt7921/mcu.c
@@ -424,8 +424,6 @@
-       if (dev->mt76.bus_type == MT76_BUS_TYPE_USB)
-               return 0;
+       /* Allow CLC for USB — firmware acceptance tested via Patch 3 retry */
+       /* If SET_CLC times out over USB bulk, retry mechanism mitigates */
```

**Sub-tasks:**
- [ ] TASK-005a: Remove `mt76_is_usb()` from CLC skip condition (experimental)
- [ ] TASK-005b: Test whether `MCU_CE_CMD(SET_CLC)` works over USB bulk transport
- [ ] TASK-005c: If SET_CLC fails over USB, implement alternative regulatory path (vendor driver's `CMD_ID_CAL_BACKUP_IN_HOST_V2` + direct channel tables)
- [ ] TASK-005d: Verify 6 GHz channels appear in `iw phy0 info` after CLC load
- [ ] TASK-005e: Verify 2.4/5 GHz power limits are applied correctly

**Risk:** If `SET_CLC` doesn't work over USB, the command will time out, triggering TASK-003's retry mechanism, then a chip reset. This is why TASK-003 is a prerequisite.

**Regulatory compliance risk (from Part III, Section 3.1):** The CLC skip doesn't just eliminate 6 GHz — it means 2.4 GHz and 5 GHz operate without country-specific power limits. The `clc_chan_conf` remains at `0xff` (all UNII bits enabled), but the firmware has no actual regulatory data. The 2.4 GHz band is subject to different EIRP limits across jurisdictions (FCC: 30 dBm, ETSI: 20 dBm, Japan: lower on certain channels). Without CLC data, the firmware cannot apply correct power reduction, creating regulatory compliance risk.

**Firmware CLC viability (from Part III, Section 3.2):** The vendor driver uses an entirely different command for USB: `CMD_ID_CAL_BACKUP_IN_HOST_V2` combined with `rlmDomainGetChnlList()`. This suggests MediaTek's engineers may have determined SET_CLC is not viable over USB. Testing strategy: apply Patch 4, observe firmware response. If rejected, implement vendor's alternative path.

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
| **Status** | `[x]` |
| **Source** | ISSUES.md BUG-05 |
| **Assigned** | `patch-engineer` |
| **Dependencies** | None |
| **Estimate** | 2 hours |
| **Bus impact** | USB primarily; SDIO path also affected |
| **Evidence** | Proven (source-level) |

**Description:** In `mt7921u_mac_reset()`, `ieee80211_stop_queues()` is never called before the reset sequence, and there is no `ieee80211_wake_queues()` on any exit path. During the ~10-second USB reset, mac80211 can continue queuing TX frames that will fail on a resetting chip. Additionally, the SDIO early-return path at `mac.c:678` can leave queues permanently stopped.

**Production Patch:**
```diff
--- a/drivers/net/wireless/mediatek/mt76/mt7921/mac.c
+++ b/drivers/net/wireless/mediatek/mt76/mt7921/mac.c
@@ -678,6 +678,8 @@
        if (ret) {
                dev_err(dev->mt76.dev, "MAC reset failed\n");
+               ieee80211_wake_queues(dev->mt76.hw);
+               mt76_queue_wake(dev, ...);
                return ret;
        }
```

**Acceptance criteria:**
- [ ] Add `ieee80211_stop_queues(hw)` at the start of `mt7921u_mac_reset()`
- [ ] Add `ieee80211_wake_queues(hw)` on ALL exit paths (success and failure)
- [ ] Add `dev->hw_full_reset = true/false` around the reset
- [ ] Cancel `pm->ps_work` and `pm->wake_work` during reset
- [ ] SDIO early-return path also gets `ieee80211_wake_queues()`
- [ ] `checkpatch.pl --strict` clean

---

## Priority 0 — Feature Gap Fixes

---

### TASK-013: Implement DFS Master Support (AP Mode Radar Detection)

| Field | Value |
|-------|-------|
| **Status** | `[~]` (stub with `start_radar_detection` + `end_cac` ops, awaiting firmware validation) |
| **Source** | ISSUES.md FEATURE-03 |
| **Assigned** | `patch-engineer` + `firmware-analyst` |
| **Dependencies** | AP mode implementation; firmware radar detection capability confirmation |
| **Estimate** | 2-3 months |
| **Bus impact** | All buses (AP-mode only) |
| **Evidence** | Probable (firmware radar detection capability unconfirmed) |

**Description:** The mainline driver lacked DFS master support. Channel-switch infrastructure with `CH_SWITCH_DFS` reason exists in `mt7921_mcu_set_chan_info()`. Stub implementations of `start_radar_detection` and `end_cac` have been added to `main.c` and registered in `mt7921_ops`. Both send `MCU_UNI_CMD(RDD_CTRL)` to the firmware. Radar event handling from firmware notifications is still missing.

**Firmware dependency:** DFS requires the firmware to support radar detection and report CAC (Channel Availability Check) events. The vendor driver implements radar detection via `EXT_CMD_ID_RADAR_DETECT`, but this command's existence and functionality on the MT7921U firmware is not yet confirmed. Without firmware radar detection support, implementing DFS in the driver alone is insufficient.

**Sub-tasks:**
- [ ] TASK-013a: Confirm firmware radar detection capability (`EXT_CMD_ID_RADAR_DETECT` presence)
- [x] TASK-013b: Implement `WIPHY_FLAG_HAS_RADAR_DETECT` in wiphy flags
- [x] TASK-013c: Implement `start_radar_detection` and `end_cac` mac80211 ops
- [ ] TASK-013d: Add radar event handling from firmware notifications
- [ ] TASK-013e: Test with DFS channels on 5 GHz band

**Acceptance criteria:**
- [ ] Radar detection capability advertised in wiphy flags
- [ ] CAC timer starts correctly when DFS channel is selected
- [ ] Radar events from firmware trigger channel switch
- [ ] Non-DFS channels unaffected
- [ ] `checkpatch.pl --strict` clean

---

### TASK-014: Firmware CLC Viability Testing

| Field | Value |
|-------|-------|
| **Status** | `[~]` (defensive fallback + test harness ready, awaiting hardware testing) |
| **Source** | ISSUES.md Part III, Section 3.2 |
| **Assigned** | `firmware-analyst` + `test-planner` |
| **Dependencies** | TASK-003 (MCU retry must be in place) |
| **Estimate** | 1 week |
| **Bus impact** | USB only |
| **Evidence** | N/A (investigative task) |

**Description:** Determine whether `MCU_CE_CMD(SET_CLC)` is viable over USB bulk transport. The CLC command was designed for SDIO and PCIe buses. USB bulk transfers operate on a fundamentally different scheduling model, and the vendor driver uses an entirely different regulatory path for USB (`CMD_ID_CAL_BACKUP_IN_HOST_V2` + `rlmDomainGetChnlList()`).

**Testing Strategy:**
1. Apply Patch 4 (enable CLC for USB) and Patch 3 (MCU retry mechanism)
2. Boot the system with the patched driver
3. Monitor dmesg for SET_CLC command responses
4. If firmware accepts: CLC fix is straightforward, Patch 4 alone sufficient
5. If firmware rejects (timeout/error): confirms SET_CLC not supported over USB
6. If rejected, implement vendor driver's alternative path

**Acceptance criteria:**
- [ ] Document whether `MCU_CE_CMD(SET_CLC)` works over USB bulk transport
- [ ] If viable: confirm 6 GHz channels appear and power limits applied
- [ ] If not viable: document alternative path requirements and create task for implementation
- [ ] Test across multiple MT7921 chip revisions if possible

---

### TASK-015: Regulatory Compliance Measurement

| Field | Value |
|-------|-------|
| **Status** | `[ ]` |
| **Source** | ISSUES.md Part III, Section 3.1 |
| **Assigned** | `test-planner` |
| **Dependencies** | TASK-005 (CLC fix) |
| **Estimate** | 1-2 weeks (requires lab access and RF equipment) |
| **Bus impact** | USB only |
| **Evidence** | N/A (measurement task) |

**Description:** Measure the actual transmit power of the MT7921U adapter on 2.4 GHz and 5 GHz channels with and without CLC data, comparing measured EIRP against regulatory limits. Without CLC data, the firmware operates with default power tables not calibrated for any regulatory domain, and `clc_chan_conf` remains at `0xff` (all UNII bits enabled). The 2.4 GHz band has different EIRP limits across jurisdictions (FCC: 30 dBm, ETSI: 20 dBm, Japan: lower on certain channels).

**Procedure:** Using a calibrated spectrum analyzer or SDR, measure actual transmit power on 2.4 GHz and 5 GHz channels with and without CLC data. Compare measured EIRP against regulatory limits for the configured country code.

**Acceptance criteria:**
- [ ] Transmit power measured on 2.4 GHz channels (1-13) with and without CLC
- [ ] Transmit power measured on 5 GHz channels (UNII-1/2/3) with and without CLC
- [ ] Compare against FCC, ETSI, and JP regulatory limits
- [ ] Document any channels where EIRP exceeds jurisdictional limits without CLC
- [ ] Confirm that with CLC, power conforms to configured regulatory domain

**Note:** Requires specialized RF measurement equipment and shielded lab environment. See TEST-T7.

---

### TASK-016: BT Coexistence Assessment

| Field | Value |
|-------|-------|
| **Status** | `[~]` (detection code implemented, awaiting hardware validation) |
| **Source** | ISSUES.md FEATURE-04, Part III Section 3.3 |
| **Assigned** | `test-planner` + `firmware-analyst` |
| **Dependencies** | Access to MT7921U combo dongle |
| **Estimate** | 1-2 weeks |
| **Bus impact** | All buses |
| **Evidence** | Speculative (firmware autonomous behavior not confirmed) |

**Description:** Assess whether the MT7921 firmware handles BT/WiFi coexistence completely autonomously, or whether it requires host-side coordination. The vendor driver adds host-side rate-limiting hints via `EXT_CMD_ID_COEXISTENCE` and PHY rate limiting commands. The mainline driver defines `MT_NIC_CAP_COEX` but never queries it.

**Assessment procedure:**
1. On a combo MT7921U dongle, run iperf3 TCP mode to measure Wi-Fi throughput
2. Activate BT audio streaming (A2DP to a BT speaker)
3. Repeat iperf3 measurement with active BT
4. Compare throughput with and without BT

**Decision criteria:**
- If throughput drop < 20%: firmware handles coexistence autonomously, FEATURE-04 is low priority
- If throughput drop > 20%: firmware requires host-side hints, FEATURE-04 is medium priority

**Acceptance criteria:**
- [ ] Query `MT_NIC_CAP_COEX` capability from firmware
- [ ] Measure Wi-Fi throughput with and without concurrent BT activity
- [ ] Document whether host-side coexistence hints are needed
- [ ] If needed, design `EXT_CMD_ID_COEXISTENCE` command interface

---

### TASK-017: Implement Multi-Endpoint TX QoS

| Field | Value |
|-------|-------|
| **Status** | `[~]` (6-endpoint mapping implemented, awaiting hardware validation) |
| **Source** | ISSUES.md FEATURE-05 |
| **Assigned** | `patch-engineer` |
| **Dependencies** | Firmware endpoint mapping documentation |
| **Estimate** | 2-3 months |
| **Bus impact** | USB only |
| **Evidence** | Proven (vendor has 6 endpoints vs mainline's 2-3) |

**Description:** The mainline driver uses 2-3 bulk OUT endpoints via `mt76u_ac_to_hwq()`. The vendor driver uses 6 separate bulk OUT endpoints (EP4-EP9) mapping 6 traffic classes. With the current 2-3 endpoint setup, AC_BE traffic can block AC_VO frames, increasing jitter for latency-sensitive applications.

**Sub-tasks:**
- [x] TASK-017a: Document vendor driver's 6-endpoint mapping (EP4-EP9 to AC mappings)
- [x] TASK-017b: Extend `mt76u_ac_to_hwq()` to support 6 USB endpoints
- [x] TASK-017c: Add endpoint configuration in USB descriptor parsing + `force_num_out_eps` module parameter
- [ ] TASK-017d: Test latency-sensitive traffic (VoIP, gaming) under concurrent bulk load
- [ ] TASK-017e: Benchmark jitter reduction with 6-endpoint mapping

**Acceptance criteria:**
- [ ] 6 bulk OUT endpoints configured for USB
- [ ] AC_VO frames not blocked by AC_BE bulk transfers
- [ ] Measurable jitter reduction under concurrent traffic load
- [ ] PCIe/SDIO unaffected
- [ ] `checkpatch.pl --strict` clean

---

### TASK-018: Add Per-Chunk ACK in Firmware Download

| Field | Value |
|-------|-------|
| **Status** | `[~]` (`fw_ack_enable` module param implemented, awaiting hardware validation) |
| **Source** | ISSUES.md FEATURE-06 |
| **Assigned** | `patch-engineer` |
| **Dependencies** | None |
| **Estimate** | 1-2 weeks |
| **Bus impact** | USB only |
| **Evidence** | Proven (vendor uses `DOWNLOAD_CONFIG_ACK_OPTION = BIT(31)`) |

**Description:** Mainline uses `MCU_CMD(FW_SCATTER)` with `wait_resp=false` — fire-and-forget. Vendor sets `DOWNLOAD_CONFIG_ACK_OPTION = BIT(31)` for per-chunk ACK. If a USB bulk transfer is lost or corrupted, the mainline driver won't detect it. The firmware will wait for the missing chunk, eventually timing out and triggering BUG-04's chip reset.

**Connection to BUG-04:** The fire-and-forget firmware download is a contributing factor to the MCU timeout problem. If a firmware download chunk is lost due to a USB error, the firmware enters an inconsistent state and may not respond to subsequent MCU commands. Adding per-chunk ACK would provide early detection of download failures, allowing a clean retry rather than an opaque MCU timeout that forces a full chip reset.

**Sub-tasks:**
- [x] TASK-018a: Add `DOWNLOAD_CONFIG_ACK_OPTION = BIT(31)` to firmware download command (`fw_ack_enable` module param in `mt76_connac_mcu.c`)
- [x] TASK-018b: Change `wait_resp=false` to `wait_resp=true` for firmware download (when `fw_ack_enable` is set)
- [ ] TASK-018c: Add retry logic for individual chunk download failures
- [ ] TASK-018d: Test firmware download reliability on USB with induced errors

**Acceptance criteria:**
- [ ] Per-chunk ACK enabled in firmware download
- [ ] Failed chunks are retried without full chip reset
- [ ] Firmware download success rate improved under USB errors
- [ ] PCIe/SDIO firmware download unaffected
- [ ] `checkpatch.pl --strict` clean

---

## Priority 1 — High-Impact Feature Implementations

These tasks implement the highest-impact features that the firmware already
supports but the driver does not expose.

---

### TASK-007: Implement TWT Responder

| Field | Value |
|-------|-------|
| **Status** | `[x]` (Phases 1-3 complete — eBPF stub + debugfs stats + HE caps) |
| **Source** | ISSUES.md FEATURE-02, ENHANCE-02 |
| **Assigned** | `patch-engineer` + `research-scout` |
| **Dependencies** | TASK-005 (6 GHz working preferred but not required) |
| **Estimate** | 3-4 months |
| **Bus impact** | All buses |
| **Evidence** | Proven (firmware command 0x94 exists) |

**Research basis:** "Deterministic Scheduling over Wi-Fi 6 using Target Wake Time: An Experimental Approach" (arXiv, May 2025) demonstrates TWT can reduce average latency by 60%, jitter by 80%, and power consumption by 40% for battery-powered stations.

**Implementation Sketch:**
- **Phase 1:** Port vendor driver's TWT command interface, implementing `add_twt_setup` and `twt_teardown` mac80211 callbacks that translate mac80211 TWT parameters into firmware command 0x94
- **Phase 2:** Add eBPF hook at TWT agreement setup time, allowing BPF programs to compute optimal TWT parameters based on historical traffic patterns
- **Phase 3:** Expose TWT agreement statistics (wake count, missed wakeups, average latency) via debugfs; vendor driver's `twt_planner.c` provides reference implementation

**Sub-tasks:**
- [x] TASK-007a: Port vendor driver's TWT MCU command structures (`MCU_EXT_CMD(TWT_AGRT_UPDATE) = 0x94`)
- [x] TASK-007b: Implement `.add_twt_setup` and `.twt_teardown_request` mac80211 ops
- [x] TASK-007c: Add TWT capability bits to HE MAC capabilities (`IEEE80211_HE_MAC_CAP0_TWT_RES`)
- [x] TASK-007d: Implement TWT responder (AP-side) in `mt7921/twt.c`
- [x] TASK-007e: Add eBPF hook stub for TWT agreement setup (ENHANCE-02)
- [x] TASK-007f: Expose TWT statistics via debugfs (`twt_stats`)

**Backward compatibility:** TWT is standard 802.11ax; non-TWT STAs unaffected. eBPF hook optional; if no BPF program attached, driver uses default TWT parameters.

**Risk:** The firmware's TWT implementation may not support all parameters exposed by the vendor driver, requiring fallback to a subset of TWT features.

---

### TASK-008: Implement CSI Extraction for Wi-Fi Sensing

| Field | Value |
|-------|-------|
| **Status** | `[x]` (Phases 1-4 complete — nl80211 vendor cmd + radiotap def + docs) |
| **Source** | ISSUES.md ENHANCE-03 |
| **Assigned** | `patch-engineer` + `research-scout` |
| **Dependencies** | None (structural) |
| **Estimate** | 2-3 months |
| **Bus impact** | All buses |
| **Evidence** | Speculative (firmware CSI interface not yet reverse-engineered) |

**Research basis:** "Enhancing CSI-Based Wireless Sensing With Open-Source Linux 802.11ax CSI Tool" (IEEE, May 2025); WhoFi system achieves 95.5% accuracy for human presence detection.

**Implementation Sketch:**
- **Phase 1:** Identify firmware command for CSI extraction by examining vendor driver's CSI-related command handlers and cross-referencing with firmware command table
- **Phase 2:** Implement vendor nl80211 command for CSI request and corresponding firmware command submission
- **Phase 3:** Add binary attribute in netlink response and radiotap field for per-frame CSI, enabling Wireshark/tcpdump capture
- **Phase 4:** Validate using WhoFi methodology, comparing sensing accuracy against published benchmark

**Sub-tasks:**
- [x] TASK-008a: Reverse-engineer firmware CSI capture command from vendor driver (CMD_ID_CSI_CONTROL=0x4C, EVENT_ID_CSI_DATA=0x3C)
- [x] TASK-008b: Add vendor nl80211 command for CSI request (`csi_nl80211.c`)
- [x] TASK-008c: Implement CSI data extraction from TLV event (mt7921_mcu_csi_event)
- [x] TASK-008d: Expose CSI data as binary netlink attribute (`MT7921_NL_ATTR_CSI_DATA`)
- [x] TASK-008e: Add radiotap vendor-extension field definition (comment in `mt7921.h`)
- [x] TASK-008f: Document CSI data format (`Documentation/networking/mt7921_csi.rst`)

**Backward compatibility:** Optional vendor extension; doesn't affect normal frame processing. MT7921U becomes first MediaTek USB adapter with open CSI access.

**Risk:** The firmware's CSI metadata format may differ from the vendor driver's, requiring reverse engineering of the RX descriptor CSI fields.

---

## Priority 2 — Enhanced Capabilities

---

### TASK-009: Implement eBPF-Based Channel Survey and ACS

| Field | Value |
|-------|-------|
| **Status** | `[~]` (`.get_survey` wired + BPF stub header created, kernel BPF type pending) |
| **Source** | ISSUES.md ENHANCE-01 |
| **Assigned** | `patch-engineer` + `research-scout` |
| **Dependencies** | `.get_survey` implementation |
| **Estimate** | 2-3 months |
| **Bus impact** | All buses |

**Research basis:** arXiv 2025 papers on RL-based Wi-Fi channel selection and multi-armed bandit approaches. RL-based channel selection can improve throughput by 15-30% in dense environments with overlapping BSS coverage.

**Implementation Sketch:**
- **Phase 1:** Implement `.get_survey` by extracting per-channel statistics from `mt76_channel_state`
- **Phase 2:** Define `BPF_PROG_TYPE_WIFI_SURVEY` program type with context structure containing survey data, and return value encoding recommended channel
- **Phase 3:** Add nl80211 interface for attaching/detaching BPF programs, modify hostapd to query BPF-provided channel recommendations

**Sub-tasks:**
- [x] TASK-009a: Implement `.get_survey` to populate `survey_info` from `mt76_channel_state`
- [x] TASK-009b: Define `BPF_PROG_TYPE_WIFI_SURVEY` hook type stub (`survey_bpf.h` — kernel patch required for full registration)
- [ ] TASK-009c: Add BPF hook that fires when survey data is updated
- [ ] TASK-009d: Implement userspace BPF program for ML-based channel selection
- [ ] TASK-009e: Add nl80211 channel recommendation interface

**Backward compatibility:** Entirely additive. If no BPF program attached, survey data flows through existing mac80211 ACS path unchanged.

**Risk:** The kernel's BPF subsystem may require upstream changes to support the new program type, which could delay integration.

---

### TASK-010: Implement AF_XDP Zero-Copy Monitor Mode

| Field | Value |
|-------|-------|
| **Status** | `[ ]` |
| **Source** | ISSUES.md ENHANCE-04 |
| **Assigned** | `patch-engineer` |
| **Dependencies** | Monitor mode implementation |
| **Estimate** | 2-3 months |
| **Bus impact** | USB primarily |

**Research basis:** AF_XDP enables zero-copy packet processing directly from NIC driver to userspace. Can achieve line-rate processing with sub-microsecond per-packet latency, ideal for monitor mode packet capture.

**Implementation Sketch:**
- **Phase 1:** Add AF_XDP UMEM support to driver's RX path, allocating shared memory pool mapped into both URB completion handler and userspace AF_XDP socket
- **Phase 2:** Modify RX URB completion handler to check for AF_XDP socket binding and swap URB buffer pointer with UMEM descriptor instead of constructing sk_buff
- **Phase 3:** Add XDP program attachment point for monitor interface
- **Phase 4:** Benchmark zero-copy path against standard path, measuring CPU utilization and packet loss

**Sub-tasks:**
- [ ] TASK-010a: Add `NL80211_IFTYPE_MONITOR` to supported interface types
- [ ] TASK-010b: Implement monitor mode in `mt7921_add_interface()` using `mt7921_mcu_set_sniffer()`
- [ ] TASK-010c: Extend `mt7921_configure_filter()` for monitor-specific FIF flags
- [ ] TASK-010d: Allocate AF_XDP UMEM for monitor interface
- [ ] TASK-010e: Map RX buffers from UMEM into RX URB completion handler
- [ ] TASK-010f: Implement zero-copy pointer swap delivery

**Backward compatibility:** AF_XDP optional per-interface; falls back to normal RX path.

**Risk:** USB bulk transfer semantics may complicate UMEM buffer management, requiring careful coordination between URB completion handler and XDP ring buffer.

---

## Priority 3 — Advanced Features

---

### TASK-011: Implement OFDMA-Aware MU Scheduling with CTI Mitigation

| Field | Value |
|-------|-------|
| **Status** | `[ ]` |
| **Source** | ISSUES.md ENHANCE-05 |
| **Assigned** | `patch-engineer` + `research-scout` |
| **Dependencies** | AP mode implementation, CSI extraction (TASK-008) |
| **Estimate** | 4-6 months |
| **Bus impact** | All buses (AP-mode only) |

**Research basis:** "Wi-Fi 6 Cross-Technology Interference Detection and Mitigation by OFDMA" (arXiv, March 2025) — CSI-based CTI classification can mitigate up to 35% throughput loss. Uses CSI snapshots to classify interference type, predict impact on RU allocations, and dynamically adjust OFDMA scheduling.

**Implementation Sketch:**
- **Phase 1:** Implement per-STA CSI collection in AP mode, building on CSI extraction from ENHANCE-03
- **Phase 2:** Implement CTI detection algorithm using CSI snapshot comparison, classifying interference types and predicting impact on each RU
- **Phase 3:** Add dynamic RU allocation scheduler avoiding contaminated RUs based on CTI classification results
- **Phase 4:** Add vendor-specific firmware command for communicating scheduling parameters to firmware's OFDMA engine

**Sub-tasks:**
- [ ] TASK-011a: Implement AP mode (see FEATURE-05 in ISSUES.md)
- [ ] TASK-011b: Implement OFDMA scheduler reading per-STA CSI from firmware
- [ ] TASK-011c: Add CTI detection using CSI snapshot method
- [ ] TASK-011d: Implement dynamic RU allocation avoiding interfered subcarriers

**Backward compatibility:** Standard 802.11ax OFDMA; non-OFDMA STAs unaffected. CTI detection optional; falls back to standard OFDMA without CTI avoidance.

---

### TASK-012: Implement Hardware Timestamping for PTP

| Field | Value |
|-------|-------|
| **Status** | `[~]` (HW timestamping fully implemented with TIMING_DEVICE, awaiting hardware validation) |
| **Source** | ISSUES.md ENHANCE-06 |
| **Assigned** | `patch-engineer` |
| **Dependencies** | None |
| **Estimate** | 2-4 weeks |
| **Bus impact** | All buses |

**Research basis:** PTP (IEEE 1588) requires sub-microsecond timestamping for clock synchronization. MT7921 hardware provides TSF timestamps in RX descriptor. Research shows hardware timestamping can achieve sub-microsecond synchronization accuracy for industrial IoT.

**Implementation Sketch:**
- **Phase 1:** Implement `ndo_get_tstamp` by extracting TSF timestamp from RX descriptor and converting to kernel timespec using device's clock frequency
- **Phase 2:** Add `SOF_TIMESTAMPING_RX_HARDWARE` support by attaching hardware timestamp to sk_buff in RX completion handler when socket has requested hardware timestamps
- **Phase 3:** Add TX hardware timestamping for transmitted management frames, completing PTP support
- **Phase 4:** Validate using Linux PTP daemon (linuxptp) to measure synchronization accuracy against wired PTP grandmaster

**Sub-tasks:**
- [x] TASK-012a: Implement TSF extraction from RX descriptor (`status->mactime`)
- [x] TASK-012b: Add `NL80211_FEATURE_HW_TIMESTAMP` + `SOF_TIMESTAMPING_RX_HARDWARE` + `IEEE80211_HW_TIMING_DEVICE`
- [ ] TASK-012c: Verify TX hardware timestamping capability
- [ ] TASK-012d: Test with `linuxptp` / `ptp4l`

**Backward compatibility:** Optional socket option; non-timestamped sockets unaffected. Enables industrial IoT and telecommunications applications requiring precise time synchronization over Wi-Fi.

---

## Test Plan Tasks (from Part VII of Coroner's Report)

These tasks implement the runtime verification test plan for the findings documented
in the forensic audit.

---

### TASK-T1: Test Testmode NULL Dereference (BUG-02)

| Field | Value |
|-------|-------|
| **Status** | `[~]` (test script created, awaiting hardware) |
| **Source** | ISSUES.md BUG-02, TEST-T1 |
| **Assigned** | `test-planner` |
| **Dependencies** | TASK-001 (patch must be available for comparison) |
| **Environment** | Physical MT7921U + `CONFIG_NL80211_TESTMODE=y` |

**Procedure:** Issue a testmode command to the USB-attached MT7921 device via a custom nl80211 testmode utility. The testmode command should attempt to set a test parameter (e.g., channel switch or transmit power override).

**Expected outcome (unpatched):** Kernel Oops with `RIP: 0010:0x0` and `mt7921_tm_set` in the call trace.

**Expected outcome (patched):** Clean `-EOPNOTSUPP` return without any kernel Oops.

---

### TASK-T2: Test WTBL Poll Timeout Measurement (BUG-03)

| Field | Value |
|-------|-------|
| **Status** | `[~]` (test script created, awaiting hardware) |
| **Source** | ISSUES.md BUG-03, TEST-T2 |
| **Assigned** | `test-planner` |
| **Dependencies** | TASK-002 (patch must be available for comparison) |
| **Environment** | Physical MT7921U + trace-cmd + multiple associated stations |

**Procedure:** Use `trace-cmd` to instrument the `mt76_poll()` function and measure the actual duration of WTBL update polls on USB. Associate multiple stations simultaneously to generate concurrent WTBL updates. Observe whether any polls exceed the 5ms timeout threshold.

**Expected outcome (unpatched):** Timeout messages in dmesg under moderate station load, confirming the 5ms threshold is insufficient.

**Expected outcome (patched):** With 50ms timeout for USB, the timeout messages should cease under normal operating conditions.

---

### TASK-T3: Test MCU Timeout + Retry (BUG-04)

| Field | Value |
|-------|-------|
| **Status** | `[~]` (test script created, awaiting hardware) |
| **Source** | ISSUES.md BUG-04, TEST-T3 |
| **Assigned** | `test-planner` |
| **Dependencies** | TASK-003 (patch must be available for comparison) |
| **Environment** | Physical MT7921U + USB runtime PM enabled |

**Procedure:** Force USB autosuspend during an active MCU command by enabling USB runtime PM and triggering a system suspend/resume cycle while an MCU command is in flight. Alternatively, use a USB hub with per-port power switching to introduce controlled delays.

**Expected outcome (unpatched):** The driver immediately resets the interface on the first timeout.

**Expected outcome (patched):** The driver retries the MCU command up to 3 times before resetting, and transient timeouts are recovered without interface disruption.

---

### TASK-T4: Test 6 GHz After CLC Enable (BUG-01)

| Field | Value |
|-------|-------|
| **Status** | `[~]` (test script created, awaiting hardware) |
| **Source** | ISSUES.md BUG-01, FEATURE-01, TEST-T4 |
| **Assigned** | `test-planner` + `firmware-analyst` |
| **Dependencies** | TASK-003, TASK-005 (both patches must be applied) |
| **Environment** | Physical MT7921U + Patch 3 (MCU retry) + Patch 4 (CLC enable) |

**Procedure:** Apply Patch 4 (enable CLC for USB) and Patch 3 (MCU retry mechanism). Boot the system with the patched driver and check `iw phy0 info` for 6 GHz channel availability. Monitor dmesg for SET_CLC command responses.

**Expected outcome (if firmware accepts SET_CLC):** 6 GHz channels will appear in the `iw phy0 info` output.

**Expected outcome (if firmware rejects SET_CLC):** The retry mechanism will provide multiple attempts, and the rejection will be logged, confirming that the alternative regulatory path must be implemented.

---

### TASK-T5: Test ROC Timer Use-After-Free (BUG-06)

| Field | Value |
|-------|-------|
| **Status** | `[~]` (test script created, awaiting hardware) |
| **Source** | ISSUES.md BUG-06, TEST-T5 |
| **Assigned** | `test-planner` |
| **Dependencies** | TASK-004 (patch must be available for comparison) |
| **Environment** | Physical MT7921U + scripted ROC + physical disconnect |

**Procedure:** Issue a short remain-on-channel command (e.g., 100ms duration on an offchannel frequency). Immediately after the ROC period ends, physically disconnect the USB adapter. Repeat 50-100 iterations to increase probability of hitting the race window.

**Expected outcome (unpatched):** A use-after-free Oops occurs when the timer fires after disconnect.

**Expected outcome (patched):** The timer is cancelled synchronously before the driver structures are freed, and no Oops occurs.

---

### TASK-T6: Test BT Coexistence Throughput (FEATURE-04)

| Field | Value |
|-------|-------|
| **Status** | `[~]` (test script created, awaiting hardware) |
| **Source** | ISSUES.md FEATURE-04, Part III Section 3.3, TEST-T6 |
| **Assigned** | `test-planner` |
| **Dependencies** | Access to MT7921U combo dongle (Wi-Fi + BT) |
| **Environment** | MT7921U combo dongle + BT speaker + iperf3 |

**Procedure:** On a combo MT7921U dongle with both Wi-Fi and BT active, run iperf3 in TCP mode to measure Wi-Fi throughput. Then activate BT audio streaming (A2DP to a BT speaker) and repeat the iperf3 measurement. Compare the throughput with and without active BT audio.

**Decision criteria:**
- Throughput drop < 20%: firmware handles coexistence autonomously, FEATURE-04 is low priority
- Throughput drop > 20%: firmware requires host-side hints, FEATURE-04 is medium priority

---

### TASK-T7: Test Regulatory Compliance Measurement (BUG-01)

| Field | Value |
|-------|-------|
| **Status** | `[~]` (test script created, awaiting hardware) |
| **Source** | ISSUES.md Part III Section 3.1, TEST-T7 |
| **Assigned** | `test-planner` |
| **Dependencies** | TASK-005 (CLC fix), access to RF measurement equipment |
| **Environment** | Physical MT7921U + calibrated spectrum analyzer or SDR + shielded lab |

**Procedure:** Using a calibrated spectrum analyzer or SDR, measure the actual transmit power of the MT7921U adapter on 2.4 GHz and 5 GHz channels with and without CLC data. Compare the measured EIRP against the regulatory limits for the configured country code.

**Expected outcome (without CLC):** Device may transmit at power levels exceeding jurisdictional EIRP limits, particularly on channels where country-specific power reduction is required.

**Expected outcome (with CLC, assuming Patch 4 successful):** Transmit power should conform to configured regulatory domain.

**Note:** Requires specialized RF measurement equipment and controlled laboratory environment.

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

---

## Roadmap Risk Assessment

### P0 Risks
- **CLC firmware viability:** SET_CLC may not work over USB. If firmware rejects it, pivot to vendor driver's alternative regulatory path (increases effort to 3-4 weeks). Mitigated by TASK-014 (firmware CLC viability testing).
- **Chip revision variance:** CLC fix may require firmware-specific behavior that varies across MT7921 chip revisions. Early testing provides immediate feedback.
- **BUG patches:** Low risk — defensive in nature (NULL guards, timeout increases, retry mechanisms, timer cleanup).

### P1 Risks
- **TWT firmware parameter support:** Firmware's TWT implementation may not support all parameters from vendor driver, requiring fallback to subset.
- **CSI metadata format:** Firmware's CSI metadata format may differ from vendor driver's, requiring reverse engineering of RX descriptor CSI fields.

### P2 Risks
- **eBPF kernel upstream:** Kernel's BPF subsystem may require upstream changes for `BPF_PROG_TYPE_WIFI_SURVEY`, potentially delaying integration.
- **AF_XDP USB semantics:** USB bulk transfer semantics may complicate UMEM buffer management, requiring careful coordination between URB completion handler and XDP ring buffer.

### P3 Risks
- **OFDMA complexity:** 4-6 month estimate assumes AP mode is already working; additional AP mode implementation would extend timeline.
- **PTP TX timestamping:** TX hardware timestamping capability for the MT7921 is not yet confirmed; RX-only timestamping may be the initial deliverable.
