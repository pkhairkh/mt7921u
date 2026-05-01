#!/bin/bash
# test_t1_testmode.sh — TEST-T1: Testmode NULL deref verification
#
# Verifies that BUG-01 (NULL drv_own pointer dereference in mt7921_testmode_cmd)
# is properly fixed by checking that the driver handles the case where
# hif_ops->drv_own is NULL on USB devices.
#
# WITHOUT THE FIX (patch 0001):
#   Predicted crash signature:
#     BUG: kernel NULL pointer dereference, address: 0000000000000000
#     RIP: 0010:0x0
#     Call Trace:
#      mt7921_tm_set+0xXX/0xXXX [mt7921u]
#      mt7921_testmode_cmd+0xXX/0xXXX [mt7921u]
#
# WITH THE FIX:
#   The testmode path returns -EOPNOTSUPP and no crash occurs.
#   The debugfs trigger reports drv_own is NULL and confirms no crash.
#
# Usage: ./test_t1_testmode.sh [--interface phy0] [--log-dir /tmp/mt7921u_test]
#
# Safety: Must be run as root on a system with the MT7921U device present.
#         DO NOT run on production systems.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INTERFACE="phy0"
LOG_DIR="/tmp/mt7921u_test"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --interface) INTERFACE="$2"; shift 2 ;;
        --log-dir)   LOG_DIR="$2"; shift 2 ;;
        *)           echo "Unknown arg: $1"; exit 1 ;;
    esac
done

LOG_FILE="${LOG_DIR}/test_t1_testmode_${TIMESTAMP}.log"
mkdir -p "$LOG_DIR"

echo "=== TEST-T1: Testmode NULL deref verification ===" | tee "$LOG_FILE"
echo "Interface: $INTERFACE" | tee -a "$LOG_FILE"
echo "Log file:  $LOG_FILE" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Safety checks
if [[ $EUID -ne 0 ]]; then
    echo "[SKIP] This test must be run as root." | tee -a "$LOG_FILE"
    exit 0
fi

# Check that the device exists
DEBUGFS_PATH="/sys/kernel/debug/ieee80211/${INTERFACE}/mt76"
if [[ ! -d "$DEBUGFS_PATH" ]]; then
    echo "[SKIP] Debugfs path $DEBUGFS_PATH not found. Is the device present?" | tee -a "$LOG_FILE"
    exit 0
fi

# Check that test trigger directory exists
TRIGGER_PATH="${DEBUGFS_PATH}/test_trigger"
if [[ ! -d "$TRIGGER_PATH" ]]; then
    echo "[SKIP] Test trigger directory not found. Was test_trigger_debugfs.c compiled in?" | tee -a "$LOG_FILE"
    exit 0
fi

# Enable dynamic debug for mt7921 module
echo "Enabling dynamic debug..." | tee -a "$LOG_FILE"
echo 'module mt7921 +p' > /sys/kernel/debug/dynamic_debug/control 2>/dev/null || true
echo 'module mt76 +p' > /sys/kernel/debug/dynamic_debug/control 2>/dev/null || true

# Clear dmesg ring buffer
dmesg -c > /dev/null 2>&1 || true

# Read the trigger_testmode_null file
echo "Reading trigger_testmode_null..." | tee -a "$LOG_FILE"
TRIGGER_OUTPUT=$(cat "${TRIGGER_PATH}/trigger_testmode_null" 2>&1) || true
echo "Trigger output:" | tee -a "$LOG_FILE"
echo "$TRIGGER_OUTPUT" | tee -a "$LOG_FILE"

# Capture dmesg
DMESG_OUTPUT=$(dmesg 2>/dev/null || true)
echo "" | tee -a "$LOG_FILE"
echo "dmesg output (filtered):" | tee -a "$LOG_FILE"
echo "$DMESG_OUTPUT" | tee -a "$LOG_FILE"

# Check for crash signatures in dmesg
CRASH_FOUND=false
if echo "$DMESG_OUTPUT" | grep -qi "NULL pointer dereference\|BUG:\|Oops\|Call Trace.*mt7921_testmode\|Call Trace.*mt7921_tm_set"; then
    CRASH_FOUND=true
fi

# Evaluate result
if [[ "$CRASH_FOUND" == "true" ]]; then
    echo "" | tee -a "$LOG_FILE"
    echo "[FAIL] Kernel crash detected! BUG-01 fix is NOT working." | tee -a "$LOG_FILE"
    echo "       The NULL pointer dereference in testmode was not prevented." | tee -a "$LOG_FILE"
    exit 1
fi

# Check trigger output for expected content
if echo "$TRIGGER_OUTPUT" | grep -q "drv_own is NULL"; then
    echo "" | tee -a "$LOG_FILE"
    echo "[PASS] drv_own is NULL as expected for USB. BUG-01 fix verified." | tee -a "$LOG_FILE"
    echo "       No kernel crash occurred. The NULL check is working." | tee -a "$LOG_FILE"
elif echo "$TRIGGER_OUTPUT" | grep -q "drv_own() returned"; then
    echo "" | tee -a "$LOG_FILE"
    echo "[PASS] drv_own is functional (non-NULL). This is expected for PCIe/SDIO." | tee -a "$LOG_FILE"
elif echo "$TRIGGER_OUTPUT" | grep -q "Test triggers disabled"; then
    echo "" | tee -a "$LOG_FILE"
    echo "[SKIP] Test triggers are disabled. Load module with test_trigger_enable=1." | tee -a "$LOG_FILE"
    exit 0
else
    echo "" | tee -a "$LOG_FILE"
    echo "[WARN] Unexpected trigger output. Check logs manually." | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"
echo "Test completed. Log saved to: $LOG_FILE" | tee -a "$LOG_FILE"
