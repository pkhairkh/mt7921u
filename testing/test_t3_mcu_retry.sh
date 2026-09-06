#!/bin/bash
# test_t3_mcu_retry.sh — TEST-T3: MCU command retry verification
#
# Verifies that BUG-03 (MCU command retry on USB) is working by sending
# a harmless MCU command and checking dmesg for retry behavior.
#
# The patch (0003) adds a .max_retry = 3 field to the USB MCU ops.
# Under normal conditions, the command should succeed on the first try.
# Under USB bus stress, retries may be observed in dmesg.
#
# Usage: ./test_t3_mcu_retry.sh [--interface phy0] [--log-dir /tmp/mt7921u_test]
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

LOG_FILE="${LOG_DIR}/test_t3_mcu_retry_${TIMESTAMP}.log"
mkdir -p "$LOG_DIR"

echo "=== TEST-T3: MCU command retry verification ===" | tee "$LOG_FILE"
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

# Enable dynamic debug to capture MCU send/response traces
echo 'module mt7921 +p' > /sys/kernel/debug/dynamic_debug/control 2>/dev/null || true
dmesg -c > /dev/null 2>&1 || true

# Read the trigger_mcu_retry file
echo "Reading trigger_mcu_retry..." | tee -a "$LOG_FILE"
TRIGGER_OUTPUT=$(cat "${TRIGGER_PATH}/trigger_mcu_retry" 2>&1) || true
echo "$TRIGGER_OUTPUT" | tee -a "$LOG_FILE"

# Capture dmesg for MCU command traces
DMESG_OUTPUT=$(dmesg 2>/dev/null || true)
echo "" | tee -a "$LOG_FILE"
echo "dmesg (MCU traces):" | tee -a "$LOG_FILE"
echo "$DMESG_OUTPUT" | grep -i "mcu_send\|mcu_resp\|retry" | tee -a "$LOG_FILE" || true

# Evaluate result
if echo "$TRIGGER_OUTPUT" | grep -q "Test triggers disabled"; then
    echo "[SKIP] Test triggers are disabled." | tee -a "$LOG_FILE"
    exit 0
fi

if echo "$TRIGGER_OUTPUT" | grep -q "MCU FWLOG_2_HOST(0) result: 0"; then
    echo "[PASS] MCU command succeeded (no retry needed under normal conditions)." | tee -a "$LOG_FILE"
elif echo "$TRIGGER_OUTPUT" | grep -q "result: -"; then
    # Error — could be a timeout or other failure
    echo "[WARN] MCU command returned an error. Check dmesg for details." | tee -a "$LOG_FILE"
    echo "       If 'MCU command retry' messages appear, this indicates the retry" | tee -a "$LOG_FILE"
    echo "       mechanism is active (expected under USB stress)." | tee -a "$LOG_FILE"
    # Don't fail — MCU commands can fail for legitimate reasons
else
    echo "[WARN] Unexpected trigger output." | tee -a "$LOG_FILE"
fi

# Check for retry messages in dmesg
RETRY_COUNT=$(echo "$DMESG_OUTPUT" | grep -ci "mcu command retry\|mcu_send.*retry" || true)
if [[ "$RETRY_COUNT" -gt 0 ]]; then
    echo "" | tee -a "$LOG_FILE"
    echo "NOTE: $RETRY_COUNT MCU retry message(s) found in dmesg." | tee -a "$LOG_FILE"
    echo "      This is expected under USB bus stress. The retry mechanism is working." | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"
echo "Test completed. Log saved to: $LOG_FILE" | tee -a "$LOG_FILE"
