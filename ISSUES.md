# ISSUES.md — All Findings Classified by Evidence Tier

> This is the single source of truth for all findings from the forensic audit.
> Entries are append-only — no deletion. Updates may change evidence class or
> status, but the original finding and its history are preserved.

---

## Evidence Classification

| Tier | Definition | Tag |
|------|-----------|-----|
| **Proven** | Call chain traceable to defective code path through source analysis | `EVIDENCE: PROVEN` |
| **Probable** | Likely at runtime, but depends on timing, hardware, or configuration | `EVIDENCE: PROBABLE` |
| **Speculative** | Pattern-based inference without confirmed reachability | `EVIDENCE: SPECULATIVE` |
| **Disproved** | Originally claimed but subsequently disproved by deeper analysis | `EVIDENCE: DISPROVED` |

---

## Bugs

---

### BUG-01: CLC Intentionally Disabled for USB — 6 GHz Impossible + Regulatory Risk

| Field | Value |
|-------|-------|
| **Severity** | Major |
| **Category** | Feature elimination / Regulatory compliance |
| **Evidence** | `EVIDENCE: PROVEN` |
| **Location** | `mt7921/mcu.c:425-427` |
| **Status** | Open |
| **Tasks** | TASK-005 |

**Code:**
```c
if (mt7921_disable_clc ||
    mt76_is_usb(&dev->mt76))
        return 0;
```

**Proof:** On USB, `mt76_is_usb()` returns true, so `mt7921_load_clc()` returns 0 immediately. No CLC data is ever loaded. The firmware CLC command (`MCU_CE_CMD(SET_CLC)`) is never sent for USB.

**Impact beyond 6 GHz:** The CLC skip doesn't just eliminate 6 GHz — it also means 2.4 GHz and 5 GHz operate without country-specific power limits. The `clc_chan_conf` remains at `0xff` (all UNII bits appear enabled), but the firmware has no actual regulatory data, creating a compliance risk where the device may transmit at illegal power levels in some jurisdictions.

**Vendor comparison:** The vendor driver does NOT use CLC for USB either. It uses `CMD_ID_CAL_BACKUP_IN_HOST_V2` for calibration data and `rlmDomainGetChnlList()` for regulatory queries. 6 GHz works in the vendor driver because it has an entirely different regulatory path.

**Firmware CLC viability:** Unknown. `MCU_CE_CMD(SET_CLC)` may not work over USB bulk transport. Testing required. If it fails, the alternative path is the vendor driver's direct channel table approach.

**History:**
- v1: Identified as "6 GHz band absent" (F004)
- v2: Root cause traced to CLC skip
- v3: Confirmed; deeper analysis of regulatory compliance risk
- v4: Added firmware CLC viability uncertainty and regulatory risk for 2.4/5 GHz

---

### BUG-02: Testmode NULL Dereference on USB

| Field | Value |
|-------|-------|
| **Severity** | Major (exotic trigger) |
| **Category** | Crash |
| **Evidence** | `EVIDENCE: PROVEN` |
| **Location** | `mt7921/testmode.c:60` → `mt792x.h:210` → `usb.c:165-169` |
| **Status** | Open |
| **Tasks** | TASK-001 |

**Call chain:**
```
mt7921_testmode_cmd()                    // testmode.c:111
  → mt7921_tm_set()                       // testmode.c:34
    → __mt792x_mcu_drv_pmctrl(dev)        // testmode.c:60
      → (dev)->hif_ops->drv_own(dev)      // mt792x.h macro
        → NULL->()                         // usb.c: drv_own not set
```

**Proof:** The USB `hif_ops` at `usb.c:165-169` does not set `.drv_own` or `.fw_own` (both NULL). The PM paths are safe because `pm.enable = false` for USB, but testmode calls `__mt792x_mcu_drv_pmctrl()` unconditionally.

**Trigger:** Root + `CONFIG_NL80211_TESTMODE=y` + monitor mode + testmode switch command. Low accidental trigger rate, but provably reachable.

**Predicted crash signature:**
```
BUG: kernel NULL pointer dereference, address: 0000000000000000
RIP: 0010:0x0
Call Trace:
 mt7921_tm_set
 mt7921_testmode_cmd
 nl80211_testmode_cmd
```

**History:**
- v1: Critical (F001) — claimed PM paths trigger it
- v2: Downgraded to Major — only testmode triggers it; PM has 4 layers of USB guards
- v3: Confirmed Major; proposed macro-level NULL check
- v4: Confirmed; patch ready

---

### BUG-03: WTBL Poll Timeout Too Short for USB

| Field | Value |
|-------|-------|
| **Severity** | Major (functional failure + system hang) |
| **Category** | Reliability |
| **Evidence** | `EVIDENCE: PROVEN` |
| **Location** | `mt7921/mac.c:18-25` |
| **Status** | Open |
| **Tasks** | TASK-002 |

**Proof:**
1. `mt76_poll()` uses `usleep_range(10, 20)` between iterations with 5000µs budget
2. On USB, each `mt76_rr()` takes ~1ms (USB vendor request round-trip)
3. With 5ms budget and ~1ms per iteration → ~5 iterations before timeout
4. Called 20 times in `mt7921_mac_init()` → ~10s total hang holding `dev->mt76.mutex`

**Callers never check the return value**, so stale WTBL counters persist silently.

**History:**
- v1: Major (F005) — identified timeout mismatch
- v2: Added ~10s hang calculation; Critical for USB
- v3: Confirmed; proposed bus-aware poll
- v4: Confirmed; production patch ready

---

### BUG-04: MCU Timeout Triggers Immediate Reset with No Retry

| Field | Value |
|-------|-------|
| **Severity** | Major (reliability — transient glitches cause full chip reset) |
| **Category** | Reliability |
| **Evidence** | `EVIDENCE: PROVEN` |
| **Location** | `mt7921/mcu.c:25-31` |
| **Status** | Open |
| **Tasks** | TASK-003 |

**Code:**
```c
if (!skb) {
    dev_err(mdev->dev, "Message %08x (seq %d) timeout\n", cmd, seq);
    mt792x_reset(mdev);    // Full chip reset on ANY timeout!
    return -ETIMEDOUT;
}
```

**Why disproportionate for USB:**
- USB is inherently less reliable than PCIe (EMI, cable, host scheduling)
- A single missed response should not require full chip reset
- The vendor driver has a retry mechanism for critical commands
- Root cause of commonly reported "Message 00000010 timeout" connection drops

**History:**
- v1: Major (F007) — identified as aggressive reset
- v2: Added retry counter with global atomic (flawed)
- v3: Corrected to per-device counter; added module parameter
- v4: Production patch ready

---

### BUG-05: Missing Queue Wake on Reset Error Path

| Field | Value |
|-------|-------|
| **Severity** | Minor (regression on repeated reset failure) |
| **Category** | Error handling |
| **Evidence** | `EVIDENCE: PROVEN` |
| **Location** | `mt7921/mac.c:653-700` |
| **Status** | Open |
| **Tasks** | TASK-006 |

**Description:** `mt7921u_mac_reset()` never calls `ieee80211_stop_queues()` or `ieee80211_wake_queues()`. The SDIO path at `mac.c:678` has an early return that leaves queues permanently stopped when `atomic_read(&dev->mt76.bus_hung)` is true.

**History:**
- v3: Identified; noted USB path missing stop/wake entirely
- v4: Confirmed; patch covers both USB reset and SDIO early-return

---

### BUG-06: ROC Timer Use-After-Free on USB Disconnect

| Field | Value |
|-------|-------|
| **Severity** | Major (kernel UAF crash) |
| **Category** | Use-after-free |
| **Evidence** | `EVIDENCE: PROVEN` (reinstated) |
| **Location** | `mt792x_core.c:306` (timer), `mt792x_usb.c:300-318` (disconnect) |
| **Status** | Open |
| **Tasks** | TASK-004 |

**Description:** On USB disconnect, `mt792xu_disconnect()` does NOT cancel `phy->roc_timer` or `phy->roc_work`. If the ROC has expired from mac80211's perspective but the driver's `roc_timer` hasn't fired, there is no `ieee80211_roc` for `ieee80211_roc_purge_local()` to purge. The timer survives past `mt76_free_device()`, fires on freed memory.

**Self-correction history (critical):**
- v1: Claimed as Critical UAF (F008)
- v2: Claimed as Proven with `del_timer_sync` fix in disconnect
- v3: **INCORRECTLY DISPROVED** — claimed `ieee80211_unregister_hw()` → `ieee80211_roc_purge_local()` → `mt7921_abort_roc()` → `timer_delete_sync()` cancels the timer
- v4: **REINSTATED** — the v3 disproof is wrong because `roc_purge_local()` only cancels mac80211-managed offchannel operations. If the ROC has already expired from mac80211's view but the driver's `roc_timer` hasn't fired yet, there is nothing to purge. The timer survives and fires after `mt76_free_device()`.

**The key distinction:** mac80211's ROC state and the driver's `roc_timer` are decoupled. mac80211 may consider the ROC complete (no active `ieee80211_roc` to purge) while the driver's `roc_timer` is still armed. The timer then fires into freed memory.

**Fix:** Add `timer_delete_sync(&dev->phy.roc_timer)` + `cancel_work_sync(&dev->phy.roc_work)` directly in `mt792xu_cleanup()`, before `mt76_free_device()`.

---

## Feature Gaps

---

### FEATURE-01: 6 GHz Band — Blocked by CLC Skip + Regulatory Risk

| Field | Value |
|-------|-------|
| **Severity** | Major |
| **Category** | Missing feature |
| **Evidence** | `EVIDENCE: PROVEN` |
| **Root cause** | BUG-01 (CLC skip) |
| **Status** | Open |
| **Tasks** | TASK-005 |

**What exists:** Full 6 GHz infrastructure in mainline: `MT_NIC_CAP_6G` capability query, `NL80211_BAND_6GHZ` HE caps, UNII-5/6/7/8 channel flags, SP/VLP/LPI power type handlers.

**What's missing:** CLC loading for USB. Without CLC, `mt7921_mcu_set_clc()` is never called and firmware has no country-specific regulatory data for any band.

---

### FEATURE-02: TWT (Target Wake Time) — Zero Implementation

| Field | Value |
|-------|-------|
| **Severity** | Major |
| **Category** | Missing feature |
| **Evidence** | `EVIDENCE: PROVEN` (firmware cmd 0x94 exists in patch table) |
| **Status** | Open |
| **Tasks** | TASK-007 |

**What exists in mainline:** Nothing. No TWT mac80211 ops, no MCU command definitions, no TWT capability bits.

**What the vendor driver has:** Full TWT requester + responder with `UNI_CMD_ID_TWT`, `EXT_CMD_ID_TWT_AGRT_UPDATE = 0x94`, TWT planner managing 16 agreement slots, S1G action frame handling.

---

### FEATURE-03: DFS (Dynamic Frequency Selection) — No Master Support

| Field | Value |
|-------|-------|
| **Severity** | Major |
| **Category** | Missing feature |
| **Evidence** | `EVIDENCE: PROBABLE` (firmware radar detection capability unconfirmed) |
| **Status** | Open |
| **Tasks** | Blocked on AP mode |

**What exists:** Channel-switch infrastructure with `CH_SWITCH_DFS` reason in `mt7921_mcu_set_chan_info()`.

**What's missing:** `WIPHY_FLAG_HAS_RADAR_DETECT`, `start_radar_detection`, `end_cac`, radar event handling.

---

### FEATURE-04: BT Coexistence — Firmware Autonomous, Host Hints Missing

| Field | Value |
|-------|-------|
| **Severity** | Medium |
| **Category** | Missing feature |
| **Evidence** | `EVIDENCE: SPECULATIVE` (firmware autonomous behavior not confirmed) |
| **Status** | Open |

**Analysis:** The MT7921 is a combo chip. Basic TDM coexistence is likely firmware-autonomous (common for MediaTek combo chips). The vendor driver adds host-side rate-limiting hints (`EXT_CMD_ID_COEXISTENCE`, PHY rate limiting). Without these hints, WiFi works but throughput may degrade during BT activity.

**Key unknown:** Does the firmware handle BT/WiFi coexistence completely autonomously, or does it require host-side coordination? This can only be determined by testing on a combo dongle with active BT audio + WiFi throughput measurement.

**MT_NIC_CAP_COEX** is defined in mainline but never queried — even capability detection is missing.

---

### FEATURE-05: Multi-Endpoint TX QoS

| Field | Value |
|-------|-------|
| **Severity** | Medium |
| **Category** | Missing feature |
| **Evidence** | `EVIDENCE: PROVEN` |
| **Status** | Open |

**What exists:** Mainline uses 2-3 bulk OUT endpoints via `mt76u_ac_to_hwq()`.

**What the vendor has:** 6 separate bulk OUT endpoints (EP4-EP9) mapping 6 traffic classes, preventing AC_BE traffic from blocking AC_VO.

---

### FEATURE-06: Per-Chunk ACK in Firmware Download

| Field | Value |
|-------|-------|
| **Severity** | Low |
| **Category** | Missing feature |
| **Evidence** | `EVIDENCE: PROVEN` |
| **Status** | Open |

**Mainline:** Uses `MCU_CMD(FW_SCATTER)` with `wait_resp=false` — fire-and-forget.

**Vendor:** Sets `DOWNLOAD_CONFIG_ACK_OPTION = BIT(31)` for per-chunk ACK. If a USB bulk transfer is lost or corrupted, the mainline driver won't detect it. The firmware will wait for the missing chunk, eventually timing out and triggering BUG-04's chip reset.

---

## Enhancement Proposals (arXiv-Backed)

---

### ENHANCE-01: eBPF-Based Channel Survey and ACS

| Field | Value |
|-------|-------|
| **Category** | Programmable ACS |
| **Evidence** | `EVIDENCE: SPECULATIVE` |
| **Research basis** | arXiv 2025 — RL/multi-armed bandit for Wi-Fi channel selection |
| **Status** | Proposal |
| **Tasks** | TASK-009 |

**Proposal:** Implement `.get_survey` to expose channel busy time, TX time, RX time, noise from `mt76_channel_state`. Add `BPF_PROG_TYPE_WIFI_SURVEY` hook for userspace ML-based channel selection. BPF program outputs recommendations consumed by hostapd/wpa_supplicant via nl80211.

**Backward compatibility:** Additive — if no BPF program attached, survey data flows through existing mac80211 path.

---

### ENHANCE-02: TWT Scheduler with Deterministic Latency Guarantees

| Field | Value |
|-------|-------|
| **Category** | Deterministic TWT |
| **Evidence** | `EVIDENCE: SPECULATIVE` |
| **Research basis** | "Deterministic Scheduling over Wi-Fi 6 using TWT" (arXiv, May 2025) |
| **Status** | Proposal |
| **Tasks** | TASK-007 |

**Proposal:** Implement TWT responder from vendor driver, ported to mac80211 `add_twt_setup`/`twt_teardown` ops. Add eBPF hook for TWT agreement setup — BPF program computes optimal wake interval/service period. Expose TWT statistics via debugfs.

**Backward compatibility:** Standard 802.11ax TWT; non-TWT STAs unaffected; eBPF hook optional.

---

### ENHANCE-03: CSI Extraction for Wi-Fi Sensing

| Field | Value |
|-------|-------|
| **Category** | Wi-Fi sensing |
| **Evidence** | `EVIDENCE: SPECULATIVE` |
| **Research basis** | "Enhancing CSI-Based Wireless Sensing With Open-Source Linux 802.11ax CSI Tool" (IEEE, May 2025); WhoFi (95.5% accuracy) |
| **Status** | Proposal |
| **Tasks** | TASK-008 |

**Proposal:** Add vendor nl80211 command for CSI capture. Firmware command to extract CSI from RX descriptor. Expose CSI data as binary netlink attribute. Add radiotap field for per-frame CSI (Wireshark integration).

**Backward compatibility:** Optional vendor extension; doesn't affect normal frame processing. MT7921U becomes first MediaTek USB adapter with open CSI access.

---

### ENHANCE-04: AF_XDP for Zero-Copy Monitor Mode

| Field | Value |
|-------|-------|
| **Category** | High-performance capture |
| **Evidence** | `EVIDENCE: SPECULATIVE` |
| **Status** | Proposal |
| **Tasks** | TASK-010 |

**Proposal:** Allocate AF_XDP UMEM for monitor interface. Map RX buffers from UMEM into driver's RX URB completion handler. Single pointer swap delivery eliminates memcpy.

**Backward compatibility:** AF_XDP optional per-interface; falls back to normal RX path.

---

### ENHANCE-05: OFDMA-Aware MU Scheduling with CTI Mitigation

| Field | Value |
|-------|-------|
| **Category** | OFDMA scheduling |
| **Evidence** | `EVIDENCE: SPECULATIVE` |
| **Research basis** | "Wi-Fi 6 CTI Detection and Mitigation by OFDMA" (arXiv, March 2025) — 35% throughput recovery |
| **Status** | Proposal |
| **Tasks** | TASK-011 |

**Proposal:** AP-mode-only OFDMA scheduler reading per-STA CSI. CTI detection via CSI snapshot method. Dynamic RU allocation avoiding interfered subcarriers.

**Backward compatibility:** Standard 802.11ax OFDMA; non-OFDMA STAs unaffected.

---

### ENHANCE-06: Hardware Timestamping for PTP

| Field | Value |
|-------|-------|
| **Category** | Industrial IoT |
| **Evidence** | `EVIDENCE: PROBABLE` (RX descriptor contains TSF timestamp) |
| **Status** | Proposal |
| **Tasks** | TASK-012 |

**Proposal:** Implement `ndo_get_tstamp` to extract TSF timestamp from RX descriptor. Add `SOF_TIMESTAMPING_RX_HARDWARE` support. Test with `linuxptp` / `ptp4l`.

**Backward compatibility:** Optional socket option; non-timestamped sockets unaffected.

---

## Disproved Claims

---

### DISPROVED-01: USB Endpoint Toggle Bit Mismatch After Firmware Download

| Field | Value |
|-------|-------|
| **Original severity** | Major |
| **Evidence** | `EVIDENCE: DISPROVED` |
| **Originally** | F009 / F010 |

**Original claim:** Lack of `usb_reset_endpoint()` after firmware download causes data-toggle mismatch, dropping the first MCU command.

**Disproof:**
1. Firmware download uses `MT_EP_OUT_AC_BE` (bulk OUT EP index 1); normal MCU commands use `MT_EP_OUT_INBAND_CMD` (bulk OUT EP index 0) — different USB endpoints
2. Per USB 2.0 spec Section 8.5.2, data toggle is per-endpoint — no cross-endpoint toggle coupling
3. The vendor driver comment confirms: toggle reset is an IOT workaround, not a correctness requirement
4. The `mt792xu_epctl_rst_opt(dev, false)` configures the device's SSUSB endpoint controller to handle toggle resets internally

**Caveat:** During `mt7921u_mac_reset()`, the AC_BE endpoint's toggle COULD desync. This needs runtime verification with a USB protocol analyzer, but it's not the primary cause of MCU timeouts.

---

### DISPROVED-02: URB Completion Race on Disconnect (as originally stated)

| Field | Value |
|-------|-------|
| **Original severity** | Critical |
| **Evidence** | `EVIDENCE: DISPROVED` |
| **Originally** | F003 |

**Original claim:** Race between URB completion callbacks and USB disconnect causes kernel crash on surprise device removal.

**Disproof:**
- `usb_poison_urb()` in `mt76u_stop_rx()` synchronizes with completion callbacks — the USB core ensures callbacks finish before `usb_poison_urb()` returns
- The `dev_set_drvdata(&udev->dev, ...)` not being cleared is a latent issue but not a crash path
- The vendor driver has the same pattern (no special URB synchronization on disconnect)

**Residual concern:** `dev_set_drvdata()` should still be cleared on disconnect for cleanliness, even though it's not a proven crash path.
