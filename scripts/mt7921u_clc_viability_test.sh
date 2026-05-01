#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause-Clear
#
# mt7921u_clc_viability_test.sh — Firmware CLC Viability Test (TASK-014)
#
# Determines whether MCU_CE_CMD(SET_CLC) works over USB bulk transport.
# This is the critical test that decides whether the mainline CLC path
# (Patch 6: enable CLC for USB) is viable, or whether the vendor driver's
# alternative regulatory path (CMD_ID_CAL_BACKUP_IN_HOST_V2 + rlmDomainGetChnlList)
# must be implemented instead.
#
# Prerequisites:
#   - MT7921U USB adapter plugged in
#   - Patched driver loaded (P0 patches applied, including Patch 6: CLC enable)
#   - Root access for dmesg and iw commands
#   - usbmon kernel module loaded (optional, for USB bus trace)
#
# Usage:
#   sudo ./mt7921u_clc_viability_test.sh [--verbose] [--usbmon] [--loop N]
#
# Exit codes:
#   0 - CLC appears viable (firmware accepted SET_CLC)
#   1 - CLC not viable (firmware rejected/timed out SET_CLC)
#   2 - Test setup error (driver not loaded, no USB device, etc.)
#
# Copyright (C) 2025 mt7921u Forensic Audit Project

set -euo pipefail

# ============================================================================
# Configuration
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_DIR="/tmp/mt7921u_clc_test"
DMESG_LOG="${LOG_DIR}/dmesg_clc.log"
USBMON_LOG="${LOG_DIR}/usbmon_clc.log"
RESULT_FILE="${LOG_DIR}/result.txt"
SUMMARY_FILE="${LOG_DIR}/summary.txt"

VERBOSE=0
USE_USBMON=0
LOOP_COUNT=1
TIMEOUT_SECONDS=30

# ============================================================================
# Argument Parsing
# ============================================================================

while [[ $# -gt 0 ]]; do
    case "$1" in
        --verbose|-v)
            VERBOSE=1
            shift
            ;;
        --usbmon|-u)
            USE_USBMON=1
            shift
            ;;
        --loop|-l)
            LOOP_COUNT="${2:-3}"
            shift 2
            ;;
        --timeout|-t)
            TIMEOUT_SECONDS="${2:-30}"
            shift 2
            ;;
        --help|-h)
            echo "Usage: $0 [--verbose] [--usbmon] [--loop N] [--timeout SECS]"
            echo ""
            echo "Options:"
            echo "  --verbose   Enable verbose output"
            echo "  --usbmon    Capture USB bus trace via usbmon"
            echo "  --loop N    Run test N times (default: 1)"
            echo "  --timeout   Max wait for CLC response in seconds (default: 30)"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 2
            ;;
    esac
done

# ============================================================================
# Helper Functions
# ============================================================================

log() {
    local level="$1"
    shift
    local msg="$*"
    local timestamp
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')

    case "$level" in
        INFO)  prefix="[INFO]  " ;;
        WARN)  prefix="[WARN]  " ;;
        ERROR) prefix="[ERROR] " ;;
        PASS)  prefix="[PASS]  " ;;
        FAIL)  prefix="[FAIL]  " ;;
        *)     prefix="[???]   " ;;
    esac

    echo "${timestamp} ${prefix}${msg}"
    [[ "$VERBOSE" -eq 1 ]] && echo "${timestamp} ${prefix}${msg}" >> "${SUMMARY_FILE}"
}

check_root() {
    if [[ $EUID -ne 0 ]]; then
        log ERROR "This script must be run as root"
        exit 2
    fi
}

check_driver_loaded() {
    if lsmod | grep -q mt7921u; then
        log INFO "mt7921u driver is loaded"
        return 0
    elif lsmod | grep -q mt76_usb; then
        log INFO "mt76_usb module is loaded (mt7921u may be built-in)"
        return 0
    else
        log ERROR "mt7921u driver is not loaded"
        return 1
    fi
}

check_usb_device() {
    if lsusb | grep -qi "0e8d:7961\|3574:6211\|0846:9060\|0846:9065\|35bc:0107"; then
        log INFO "MT7921U USB device detected"
        return 0
    else
        log ERROR "MT7921U USB device not found. Known USB IDs:"
        log ERROR "  0e8d:7961 (MediaTek), 3574:6211 (Comfast CF-952AX)"
        log ERROR "  0846:9060 (Netgear A8000), 0846:9065 (Netgear A7500)"
        log ERROR "  35bc:0107 (TP-Link TXE50UH)"
        return 1
    fi
}

get_phy_name() {
    local phy
    phy=$(iw dev | awk '/phy#/{print $2}' | head -1)
    if [[ -z "$phy" ]]; then
        # Try alternative method
        phy=$(ls /sys/class/ieee80211/ 2>/dev/null | head -1)
    fi
    echo "$phy"
}

clear_dmesg() {
    dmesg -c > /dev/null 2>&1 || true
}

capture_dmesg() {
    dmesg > "$DMESG_LOG" 2>/dev/null || true
}

# ============================================================================
# CLC Viability Test Functions
# ============================================================================

test_clc_in_dmesg() {
    # Check dmesg for CLC-related messages after driver reload
    local clc_viable=0
    local clc_rejected=0
    local clc_timeout=0
    local clc_retry=0

    # Pattern 1: CLC was loaded successfully
    if grep -qi "CLC.*load\|load_clc.*success\|mt7921_load_clc" "$DMESG_LOG"; then
        log INFO "CLC load attempt detected in dmesg"
        clc_viable=$((clc_viable + 1))
    fi

    # Pattern 2: SET_CLC command was sent and accepted
    if grep -qi "set_clc\|SET_CLC\|mcu_set_clc" "$DMESG_LOG"; then
        log INFO "SET_CLC MCU command detected in dmesg"
        clc_viable=$((clc_viable + 1))
    fi

    # Pattern 3: CLC command timed out (firmware doesn't support SET_CLC over USB)
    if grep -qi "MCU command timeout.*clc\|timeout.*SET_CLC\|Message.*timeout.*0x5c" "$DMESG_LOG"; then
        log WARN "SET_CLC MCU command timeout detected — CLC may not be viable over USB"
        clc_timeout=1
    fi

    # Pattern 4: MCU retry mechanism kicked in (from Patch 3)
    if grep -qi "MCU command timeout, retrying" "$DMESG_LOG"; then
        log WARN "MCU retry mechanism triggered during CLC — USB transport issue"
        clc_retry=1
    fi

    # Pattern 5: MCU command timeout after all retries
    if grep -qi "MCU command timeout after retries" "$DMESG_LOG"; then
        log ERROR "MCU command failed after all retries — CLC NOT viable over USB"
        clc_rejected=1
    fi

    # Pattern 6: Chip reset triggered by CLC failure
    if grep -qi "mac_reset_work\|reset.*triggered" "$DMESG_LOG"; then
        log ERROR "Chip reset triggered — likely caused by CLC failure"
        clc_rejected=1
    fi

    # Pattern 7: CLC skip still happening (patch not applied correctly)
    if grep -qi "CLC.*skip\|skip.*CLC\|bus_type.*USB.*return 0" "$DMESG_LOG"; then
        log ERROR "CLC skip still detected — Patch 6 may not be applied correctly"
        clc_rejected=1
    fi

    # Determine result
    if [[ $clc_rejected -eq 1 ]] || [[ $clc_timeout -eq 1 ]]; then
        echo "REJECTED"
    elif [[ $clc_viable -ge 1 ]]; then
        echo "ACCEPTED"
    else
        echo "UNKNOWN"
    fi
}

test_6ghz_channels() {
    # Check if 6 GHz channels are available in iw phy info
    local phy="$1"
    local has_6ghz=0

    if [[ -z "$phy" ]]; then
        log WARN "No phy interface found, cannot check 6 GHz channels"
        echo "UNKNOWN"
        return
    fi

    # Check for 6 GHz band in iw phy info
    if iw phy "$phy" info 2>/dev/null | grep -qi "6.*GHz\|5920\|7120\|Band 3"; then
        log INFO "6 GHz channels detected in iw phy info"
        has_6ghz=1
    fi

    if [[ $has_6ghz -eq 1 ]]; then
        echo "PRESENT"
    else
        echo "ABSENT"
    fi
}

test_regulatory_power_limits() {
    # Check if country-specific power limits are applied
    local phy="$1"

    if [[ -z "$phy" ]]; then
        log WARN "No phy interface found, cannot check regulatory power limits"
        echo "UNKNOWN"
        return
    fi

    # Check current regulatory domain
    local reg_domain
    reg_domain=$(iw reg get 2>/dev/null | head -1 | awk '{print $2}')
    log INFO "Current regulatory domain: ${reg_domain:-unknown}"

    # Check if power limits are applied for 2.4 GHz
    local txpower_2ghz
    txpower_2ghz=$(iw phy "$phy" info 2>/dev/null | grep -A5 "2.*GHz" | grep -i "max.*antenna.*gain\|max.*EIRP" | head -1)
    log INFO "2.4 GHz power info: ${txpower_2ghz:-not available}"

    # Check if UNII bits are constrained (0xff = all enabled, no CLC data)
    if dmesg | grep -qi "clc_chan_conf.*0xff\|chan_conf.*255"; then
        log WARN "clc_chan_conf is 0xff — no CLC data applied, all UNII bits enabled"
        echo "NO_CLC"
    elif dmesg | grep -qi "clc_chan_conf\|chan_conf.*0x"; then
        log INFO "clc_chan_conf has non-default value — CLC data may be applied"
        echo "HAS_CLC"
    else
        echo "UNKNOWN"
    fi
}

test_nic_capability_6g() {
    # Query firmware for 6 GHz capability via debugfs or dmesg
    local cap_6g=0

    if dmesg | grep -qi "has_6ghz\|6.*GHz.*cap\|MT_NIC_CAP_6G"; then
        log INFO "6 GHz capability flag detected in firmware response"
        cap_6g=1
    fi

    # Check debugfs for NIC capabilities
    if [[ -f /sys/kernel/debug/ieee80211/phy0/mt76/nic_cap ]]; then
        if grep -qi "6.*GHz\|0x18" /sys/kernel/debug/ieee80211/phy0/mt76/nic_cap 2>/dev/null; then
            log INFO "6 GHz capability confirmed via debugfs"
            cap_6g=1
        fi
    fi

    if [[ $cap_6g -eq 1 ]]; then
        echo "SUPPORTED"
    else
        echo "UNKNOWN"
    fi
}

# ============================================================================
# USB Monitor Capture (Optional)
# ============================================================================

start_usbmon() {
    if [[ "$USE_USBMON" -eq 0 ]]; then
        return
    fi

    if ! lsmod | grep -q usbmon; then
        modprobe usbmon 2>/dev/null || {
            log WARN "Failed to load usbmon module, skipping USB capture"
            USE_USBMON=0
            return
        }
    fi

    # Find the USB bus number for the MT7921U device
    local usb_bus
    usb_bus=$(lsusb -t 2>/dev/null | grep -B5 "mt76\|7961" | grep -o "usb[0-9]*" | head -1 | tr -dc '0-9')

    if [[ -n "$usb_bus" ]]; then
        cat "/sys/kernel/debug/usb/usbmon/${usb_bus}t" > "$USBMON_LOG" 2>/dev/null &
        USBMON_PID=$!
        log INFO "USB monitor started on bus ${usb_bus} (PID: ${USBMON_PID})"
    else
        log WARN "Could not determine USB bus number, using bus 1"
        cat /sys/kernel/debug/usb/usbmon/1t > "$USBMON_LOG" 2>/dev/null &
        USBMON_PID=$!
        log INFO "USB monitor started on bus 1 (PID: ${USBMON_PID})"
    fi
}

stop_usbmon() {
    if [[ "${USBMON_PID:-0}" -ne 0 ]]; then
        kill "$USBMON_PID" 2>/dev/null || true
        wait "$USBMON_PID" 2>/dev/null || true
        USBMON_PID=0
        log INFO "USB monitor stopped"
    fi
}

# ============================================================================
# Main Test Procedure
# ============================================================================

main() {
    local iteration
    local overall_result="UNKNOWN"
    local clc_results=()
    local sixghz_results=()
    local regulatory_results=()

    echo "========================================================================"
    echo "  mt7921u Firmware CLC Viability Test (TASK-014)"
    echo "  Determines if MCU_CE_CMD(SET_CLC) works over USB bulk transport"
    echo "========================================================================"
    echo ""

    check_root
    mkdir -p "$LOG_DIR"

    # Initial checks
    if ! check_driver_loaded; then
        exit 2
    fi

    if ! check_usb_device; then
        exit 2
    fi

    local phy
    phy=$(get_phy_name)
    log INFO "Using PHY interface: ${phy:-none}"

    # Run test loop
    for iteration in $(seq 1 "$LOOP_COUNT"); do
        log INFO "=== Test iteration ${iteration}/${LOOP_COUNT} ==="

        # Step 1: Clear dmesg and start fresh capture
        clear_dmesg

        # Step 2: Start USB monitor if requested
        start_usbmon

        # Step 3: Reload the driver to trigger CLC loading
        log INFO "Reloading mt7921u driver to trigger CLC loading..."
        rmmod mt7921u 2>/dev/null || true
        sleep 2
        modprobe mt7921u 2>/dev/null || {
            log ERROR "Failed to reload mt7921u driver"
            stop_usbmon
            exit 2
        }

        # Step 4: Wait for CLC to complete (or timeout)
        log INFO "Waiting up to ${TIMEOUT_SECONDS}s for CLC to complete..."
        local elapsed=0
        while [[ $elapsed -lt $TIMEOUT_SECONDS ]]; do
            sleep 1
            elapsed=$((elapsed + 1))

            # Check if CLC completed or failed
            capture_dmesg
            if grep -qi "MCU command timeout after retries\|mac_reset_work" "$DMESG_LOG"; then
                log ERROR "CLC failure detected after ${elapsed}s"
                break
            fi
            if grep -qi "MCU_STATE_RUNNING\|mt7921_run_firmware.*done\|firmware.*running" "$DMESG_LOG"; then
                # Check if CLC was specifically mentioned
                if grep -qi "load_clc\|set_clc" "$DMESG_LOG"; then
                    log INFO "CLC processing detected after ${elapsed}s"
                    break
                fi
            fi
        done

        if [[ $elapsed -ge $TIMEOUT_SECONDS ]]; then
            log WARN "Timeout waiting for CLC response after ${TIMEOUT_SECONDS}s"
        fi

        # Final dmesg capture
        capture_dmesg
        stop_usbmon

        # Step 5: Analyze results
        log INFO "Analyzing CLC viability..."

        local clc_result
        clc_result=$(test_clc_in_dmesg)
        clc_results+=("$clc_result")
        log INFO "CLC dmesg analysis result: $clc_result"

        local sixghz_result
        sixghz_result=$(test_6ghz_channels "$phy")
        sixghz_results+=("$sixghz_result")
        log INFO "6 GHz channel check result: $sixghz_result"

        local reg_result
        reg_result=$(test_regulatory_power_limits "$phy")
        regulatory_results+=("$reg_result")
        log INFO "Regulatory power limits result: $reg_result"

        local cap_result
        cap_result=$(test_nic_capability_6g)
        log INFO "Firmware 6 GHz capability: $cap_result"

        # Save iteration dmesg
        cp "$DMESG_LOG" "${LOG_DIR}/dmesg_iter${iteration}.log"
        if [[ "$USE_USBMON" -eq 1 ]]; then
            cp "$USBMON_LOG" "${LOG_DIR}/usbmon_iter${iteration}.log" 2>/dev/null || true
        fi

        # Brief pause between iterations
        if [[ $iteration -lt $LOOP_COUNT ]]; then
            sleep 3
        fi
    done

    # ============================================================================
    # Result Determination
    # ============================================================================

    echo ""
    echo "========================================================================"
    echo "  CLC VIABILITY TEST RESULTS"
    echo "========================================================================"
    echo ""

    local accepted_count=0
    local rejected_count=0
    local unknown_count=0

    for result in "${clc_results[@]}"; do
        case "$result" in
            ACCEPTED) accepted_count=$((accepted_count + 1)) ;;
            REJECTED) rejected_count=$((rejected_count + 1)) ;;
            *)        unknown_count=$((unknown_count + 1)) ;;
        esac
    done

    log INFO "CLC Test Summary:"
    log INFO "  Iterations: ${LOOP_COUNT}"
    log INFO "  Accepted: ${accepted_count}"
    log INFO "  Rejected: ${rejected_count}"
    log INFO "  Unknown:  ${unknown_count}"

    # Check 6 GHz results
    local sixghz_present=0
    for result in "${sixghz_results[@]}"; do
        if [[ "$result" == "PRESENT" ]]; then
            sixghz_present=1
            break
        fi
    done

    if [[ $sixghz_present -eq 1 ]]; then
        log PASS "6 GHz channels are available — CLC appears viable"
    else
        log WARN "6 GHz channels are NOT available"
    fi

    # Final determination
    echo ""
    if [[ $accepted_count -gt 0 ]] && [[ $rejected_count -eq 0 ]] && [[ $sixghz_present -eq 1 ]]; then
        overall_result="VIABLE"
        log PASS "OVERALL: SET_CLC is VIABLE over USB bulk transport"
        log PASS "Patch 6 (CLC enable for USB) is sufficient"
        log PASS "No alternative regulatory path needed"
    elif [[ $rejected_count -gt 0 ]]; then
        overall_result="NOT_VIABLE"
        log FAIL "OVERALL: SET_CLC is NOT VIABLE over USB bulk transport"
        log FAIL "The vendor driver's alternative regulatory path must be implemented:"
        log FAIL "  - CMD_ID_CAL_BACKUP_IN_HOST_V2 (0xAE)"
        log FAIL "  - rlmDomainGetChnlList() for channel tables"
        log FAIL "  - Direct firmware channel table queries"
    elif [[ $accepted_count -gt 0 ]] && [[ $sixghz_present -eq 0 ]]; then
        overall_result="PARTIAL"
        log WARN "OVERALL: SET_CLC was sent but 6 GHz not available"
        log WARN "Firmware may accept SET_CLC but not support 6 GHz on this chip revision"
        log WARN "2.4/5 GHz regulatory data may still be applied correctly"
    else
        overall_result="INCONCLUSIVE"
        log WARN "OVERALL: Results are INCONCLUSIVE"
        log WARN "Could not determine CLC viability from available data"
        log WARN "Recommend manual analysis of dmesg logs in ${LOG_DIR}/"
    fi

    # ============================================================================
    # Output Files
    # ============================================================================

    cat > "$RESULT_FILE" << EOF
# MT7921U CLC Viability Test Result (TASK-014)
# Date: $(date -Iseconds)
# Result: ${overall_result}
# Iterations: ${LOOP_COUNT}
# CLC Accepted: ${accepted_count}
# CLC Rejected: ${rejected_count}
# CLC Unknown: ${unknown_count}
# 6 GHz Present: ${sixghz_present}
#
# Interpretation:
#   VIABLE      -> Patch 6 sufficient, no alternative needed
#   NOT_VIABLE  -> Must implement vendor alternative path
#   PARTIAL     -> CLC loads but 6 GHz not supported on this chip
#   INCONCLUSIVE -> Needs manual analysis
#
# Next Steps:
EOF

    case "$overall_result" in
        VIABLE)
            echo "#   - Mark TASK-014 as complete" >> "$RESULT_FILE"
            echo "#   - Verify 6 GHz with: iw phy0 info | grep -i '6 GHz'" >> "$RESULT_FILE"
            echo "#   - Proceed to TASK-015 (regulatory compliance measurement)" >> "$RESULT_FILE"
            ;;
        NOT_VIABLE)
            echo "#   - Create TASK-014-ALT: Implement vendor alternative path" >> "$RESULT_FILE"
            echo "#   - Port CMD_ID_CAL_BACKUP_IN_HOST_V2 to mainline" >> "$RESULT_FILE"
            echo "#   - Port rlmDomainGetChnlList() channel table queries" >> "$RESULT_FILE"
            echo "#   - Consider reverting Patch 6 to avoid chip resets on CLC failure" >> "$RESULT_FILE"
            ;;
        PARTIAL)
            echo "#   - Verify 2.4/5 GHz regulatory data is correctly applied" >> "$RESULT_FILE"
            echo "#   - Check if CLC data at least enables power limits" >> "$RESULT_FILE"
            echo "#   - 6 GHz may require chip revision with 6 GHz radio" >> "$RESULT_FILE"
            ;;
        INCONCLUSIVE)
            echo "#   - Increase dmesg log level: echo 8 > /proc/sys/kernel/printk" >> "$RESULT_FILE"
            echo "#   - Add debug prints to mt7921_load_clc() and mt7921_mcu_set_clc()" >> "$RESULT_FILE"
            echo "#   - Rebuild driver with CONFIG_MT76_DEBUG=y" >> "$RESULT_FILE"
            echo "#   - Try with --usbmon flag to capture USB bus trace" >> "$RESULT_FILE"
            ;;
    esac

    echo ""
    log INFO "Result file: ${RESULT_FILE}"
    log INFO "Dmesg logs: ${LOG_DIR}/dmesg_iter*.log"
    log INFO "Full dmesg: ${DMESG_LOG}"

    # Return appropriate exit code
    case "$overall_result" in
        VIABLE)     exit 0 ;;
        NOT_VIABLE) exit 1 ;;
        *)          exit 2 ;;
    esac
}

main "$@"
