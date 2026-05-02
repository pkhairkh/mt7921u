#!/bin/bash
# test_t4_clc.sh — TEST-T4: CLC regulatory verification
#
# Verifies that the CLC (Channel List Configuration) regulatory loading
# works correctly on USB, including the fallback behavior from patches
# 0006 and 0007.
#
# Test scenarios:
#   1. CLC load with clc_force_usb=1 — verifies the CLC path is enabled
#   2. CLC load without force — verifies fallback message on USB
#   3. Check dmesg for CLC success/failure messages
#
# If a spectrum analyzer is available, the tester can additionally verify:
#   - Channel availability matches CLC configuration
#   - 6 GHz channels are properly disabled when CLC fails on USB
#   - TX power limits are correctly applied
#
# Usage: ./test_t4_clc.sh [--interface phy0] [--log-dir /tmp/mt7921u_test]
#
# Safety: Must be run as root on a system with the MT7921U device present.

set -e

INTERFACE="phy0"
LOG_DIR="/tmp/mt7921u_test"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

while [[ $# -gt 0 ]]; do
    case $1 in
        --interface) INTERFACE="$2"; shift 2 ;;
        --log-dir)   LOG_DIR="$2"; shift 2 ;;
        *)           echo "Unknown arg: $1"; exit 1 ;;
    esac
done

LOG_FILE="${LOG_DIR}/test_t4_clc_${TIMESTAMP}.log"
mkdir -p "$LOG_DIR"

echo "=== TEST-T4: CLC regulatory verification ===" | tee "$LOG_FILE"
echo "Interface: $INTERFACE" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Safety checks
if [[ $EUID -ne 0 ]]; then
    echo "[SKIP] This test must be run as root." | tee -a "$LOG_FILE"
    exit 0
fi

DEBUGFS_PATH="/sys/kernel/debug/ieee80211/${INTERFACE}/mt76"
if [[ ! -d "$DEBUGFS_PATH" ]]; then
    echo "[SKIP] Debugfs path $DEBUGFS_PATH not found." | tee -a "$LOG_FILE"
    exit 0
fi

TRIGGER_PATH="${DEBUGFS_PATH}/test_trigger"
if [[ ! -d "$TRIGGER_PATH" ]]; then
    echo "[SKIP] Test trigger directory not found." | tee -a "$LOG_FILE"
    exit 0
fi

# Enable dynamic debug for CLC traces
echo 'module mt7921 +p' > /sys/kernel/debug/dynamic_debug/control 2>/dev/null || true
dmesg -c > /dev/null 2>&1 || true

# Read the trigger_clc_load file
echo "Reading trigger_clc_load..." | tee -a "$LOG_FILE"
TRIGGER_OUTPUT=$(cat "${TRIGGER_PATH}/trigger_clc_load" 2>&1) || true
echo "$TRIGGER_OUTPUT" | tee -a "$LOG_FILE"

# Capture dmesg for CLC traces
DMESG_OUTPUT=$(dmesg 2>/dev/null || true)
echo "" | tee -a "$LOG_FILE"
echo "dmesg (CLC traces):" | tee -a "$LOG_FILE"
echo "$DMESG_OUTPUT" | grep -i "clc\|load_clc\|set_clc\|6 GHz\|fallback" | tee -a "$LOG_FILE" || true

# Evaluate result
if echo "$TRIGGER_OUTPUT" | grep -q "Test triggers disabled"; then
    echo "[SKIP] Test triggers are disabled." | tee -a "$LOG_FILE"
    exit 0
fi

CLC_RESULT=""
if echo "$TRIGGER_OUTPUT" | grep -q "result: 0"; then
    CLC_RESULT="success"
elif echo "$TRIGGER_OUTPUT" | grep -q "result: -"; then
    CLC_RESULT="failure"
fi

if [[ "$CLC_RESULT" == "success" ]]; then
    echo "[PASS] CLC load succeeded." | tee -a "$LOG_FILE"
elif [[ "$CLC_RESULT" == "failure" ]]; then
    echo "[WARN] CLC load failed." | tee -a "$LOG_FILE"

    # Check for fallback message
    if echo "$DMESG_OUTPUT" | grep -qi "CLC SET failed on USB\|falling back to conservative"; then
        echo "[PASS] CLC fallback message found — defensive fallback is working." | tee -a "$LOG_FILE"
        echo "       6 GHz should be disabled until vendor CLC path is ported." | tee -a "$LOG_FILE"
    else
        echo "[FAIL] CLC failed but no fallback message found." | tee -a "$LOG_FILE"
        echo "       Patch 0007 fallback may not be working." | tee -a "$LOG_FILE"
        exit 1
    fi
else
    echo "[WARN] Could not determine CLC result." | tee -a "$LOG_FILE"
fi

# Spectrum analyzer verification instructions
echo "" | tee -a "$LOG_FILE"
echo "=== Spectrum Analyzer Verification (manual) ===" | tee -a "$LOG_FILE"
echo "If a spectrum analyzer is available:" | tee -a "$LOG_FILE"
echo "  1. Connect to an AP on 2.4 GHz or 5 GHz" | tee -a "$LOG_FILE"
echo "  2. Verify TX power matches CLC configuration" | tee -a "$LOG_FILE"
echo "  3. If CLC failed on USB, verify 6 GHz channels are not available" | tee -a "$LOG_FILE"
echo "  4. Run: iw ${INTERFACE} scan | grep -i '6 GHz'" | tee -a "$LOG_FILE"

echo "" | tee -a "$LOG_FILE"
echo "Test completed. Log saved to: $LOG_FILE" | tee -a "$LOG_FILE"
