#!/bin/bash
# test_t2_wtbl.sh — TEST-T2: WTBL poll timeout verification
#
# Verifies that BUG-02 (WTBL poll timeout on USB) is fixed by confirming
# the WTBL poll completes within the new USB-increased timeout (50 ms).
#
# WITHOUT THE FIX (patch 0002):
#   The original 5 ms timeout (5000 us) is too short for USB bulk
#   transport latency.  mt7921_mac_wtbl_update() returns false (timeout)
#   and downstream operations may fail.
#
# WITH THE FIX:
#   The USB timeout is increased to 50 ms (50000 us).  The poll
#   completes successfully within that window.
#
# Usage: ./test_t2_wtbl.sh [--interface phy0] [--log-dir /tmp/mt7921u_test]
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

LOG_FILE="${LOG_DIR}/test_t2_wtbl_${TIMESTAMP}.log"
mkdir -p "$LOG_DIR"

echo "=== TEST-T2: WTBL poll timeout verification ===" | tee "$LOG_FILE"
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

# Enable dynamic debug
echo 'module mt7921 +p' > /sys/kernel/debug/dynamic_debug/control 2>/dev/null || true
dmesg -c > /dev/null 2>&1 || true

# Read the trigger_wtbl_poll file
echo "Reading trigger_wtbl_poll..." | tee -a "$LOG_FILE"
TRIGGER_OUTPUT=$(cat "${TRIGGER_PATH}/trigger_wtbl_poll" 2>&1) || true
echo "$TRIGGER_OUTPUT" | tee -a "$LOG_FILE"

# Capture dmesg for wtbl_update traces
DMESG_OUTPUT=$(dmesg 2>/dev/null || true)
echo "" | tee -a "$LOG_FILE"
echo "dmesg (wtbl traces):" | tee -a "$LOG_FILE"
echo "$DMESG_OUTPUT" | grep -i "wtbl" | tee -a "$LOG_FILE" || true

# Parse the WTBL poll time from trigger output
POLL_TIME_US=""
if echo "$TRIGGER_OUTPUT" | grep -q "WTBL poll completed in"; then
    POLL_TIME_US=$(echo "$TRIGGER_OUTPUT" | grep "WTBL poll completed in" | grep -oP '\d+')
    echo "" | tee -a "$LOG_FILE"
    echo "WTBL poll time: ${POLL_TIME_US} us" | tee -a "$LOG_FILE"
fi

# Evaluate result
TIMEOUT_THRESHOLD_US=50000  # 50 ms = 50000 us (new USB timeout)

if echo "$TRIGGER_OUTPUT" | grep -q "Test triggers disabled"; then
    echo "[SKIP] Test triggers are disabled. Load module with test_trigger_enable=1." | tee -a "$LOG_FILE"
    exit 0
fi

if echo "$TRIGGER_OUTPUT" | grep -q "timeout (BUSY still set)"; then
    echo "[FAIL] WTBL poll timed out! BUG-02 fix is NOT working." | tee -a "$LOG_FILE"
    echo "       The poll did not complete within ${TIMEOUT_THRESHOLD_US} us." | tee -a "$LOG_FILE"
    exit 1
fi

if [[ -n "$POLL_TIME_US" ]]; then
    if [[ "$POLL_TIME_US" -lt "$TIMEOUT_THRESHOLD_US" ]]; then
        echo "[PASS] WTBL poll completed in ${POLL_TIME_US} us (< ${TIMEOUT_THRESHOLD_US} us threshold)." | tee -a "$LOG_FILE"
    else
        echo "[FAIL] WTBL poll took ${POLL_TIME_US} us (>= ${TIMEOUT_THRESHOLD_US} us threshold)." | tee -a "$LOG_FILE"
        exit 1
    fi
else
    echo "[WARN] Could not parse poll time from trigger output." | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"
echo "Test completed. Log saved to: $LOG_FILE" | tee -a "$LOG_FILE"
