# TASK-014: Firmware CLC Viability Testing

## Objective

Determine whether `MCU_CE_CMD(SET_CLC)` (command ID 0x5c) is viable over USB
bulk transport on the MT7921U adapter. This is the critical decision point for
BUG-01 remediation: if SET_CLC works over USB, Patch 6 alone is sufficient;
if it doesn't, the vendor driver's alternative regulatory path must be
implemented.

## Background

The mainline driver skips CLC loading for USB devices (original code at
`mcu.c:425-427`). Patch 6 removes this skip, allowing SET_CLC to be sent.
However, the vendor driver uses an entirely different regulatory path for USB:

- **Mainline path:** `MCU_CE_CMD(SET_CLC)` (0x5c) → firmware applies regulatory data
- **Vendor path:** `CMD_ID_CAL_BACKUP_IN_HOST_V2` (0xAE) + `rlmDomainGetChnlList()`

The vendor's different approach suggests MediaTek's engineers may have determined
SET_CLC is not viable over USB, but this has never been confirmed.

## Test Strategy

### Phase 1: Automated Testing (this deliverable)

The `scripts/mt7921u_clc_viability_test.sh` script performs:

1. **Driver reload test:** Reloads the mt7921u driver with Patch 6 applied,
   monitoring dmesg for SET_CLC command responses.

2. **6 GHz channel detection:** Checks `iw phy0 info` for 6 GHz channel
   availability after CLC loading.

3. **Regulatory power limit verification:** Checks if country-specific power
   limits are applied (vs. default 0xff clc_chan_conf).

4. **Firmware capability query:** Checks for `MT_NIC_CAP_6G` capability in
   firmware response.

5. **USB bus trace (optional):** Captures usbmon trace during CLC exchange
   for wire-level evidence.

### Phase 2: Manual Analysis

If automated results are inconclusive:

1. Add debug prints to `mt7921_load_clc()` and `mt7921_mcu_set_clc()`
2. Rebuild driver with `CONFIG_MT76_DEBUG=y`
3. Capture full USB bus trace with usbmon during CLC
4. Compare SET_CLC command payload against PCIe/SDIO variant behavior

### Phase 3: Alternative Path (if SET_CLC fails)

If SET_CLC is confirmed non-viable over USB:

1. Port `CMD_ID_CAL_BACKUP_IN_HOST_V2` (0xAE) command from vendor driver
2. Port `rlmDomainGetChnlList()` channel table queries
3. Implement USB-specific regulatory configuration that bypasses CLC
4. Consider adding a module parameter to select regulatory path

## Vendor Driver Reference

### CMD_ID_CAL_BACKUP_IN_HOST_V2 (0xAE)

Location: `vendor-driver/include/wsys_cmd_handler_fw.h:301`

Structure: `struct CMD_CAL_BACKUP_STRUCT_V2`
- `ucReason`, `ucAction`, `ucNeedResp`, `ucFragNum`, `ucRomRam`
- `u4ThermalValue`, `u4Address`, `u4Length`, `u4RemainLength`
- `au4Buffer[PARAM_CAL_DATA_DUMP_MAX_NUM]`

This command stores/retrieves calibration data from host memory, critical for
USB where firmware may lose calibration data across suspend/resume cycles.

### rlmDomainGetChnlList()

Location: `vendor-driver/mgmt/rlm_domain.c:1100`

This function queries the firmware's internal channel table, which is
pre-populated with regulatory data at manufacturing time. Unlike SET_CLC,
this approach does not depend on the host sending CLC data.

## Expected Outcomes

| Scenario | SET_CLC Response | 6 GHz | Power Limits | Action |
|----------|-----------------|-------|-------------|--------|
| CLC viable | Accepted | Present | Applied | Patch 6 sufficient |
| CLC rejected | Timeout/Error | Absent | Default | Implement alternative path |
| Partial | Accepted | Absent | Applied | Verify 2.4/5 GHz; 6 GHz may need chip rev |
| Inconclusive | No clear signal | Unknown | Unknown | Manual debug analysis |

## Files

| File | Purpose |
|------|---------|
| `scripts/mt7921u_clc_viability_test.sh` | Automated test harness |
| `docs/firmware/clc_viability_test_plan.md` | This document |
| `vendor-driver/include/wsys_cmd_handler_fw.h` | Vendor command definitions |
| `vendor-driver/mgmt/rlm_domain.c` | Vendor regulatory channel list |
| `drivers/net/wireless/mediatek/mt76/mt7921/mcu.c` | Mainline CLC loading code |

## Related Tasks

- TASK-005: CLC enable for USB (Patch 6 — prerequisite)
- TASK-015: Regulatory compliance measurement (depends on TASK-014 result)
- BUG-01: CLC disabled for USB (root cause)
