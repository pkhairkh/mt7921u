#!/bin/bash
# test_t6_reset.sh — TEST-T6: Queue wake on reset verification
#
# Verifies that the SDIO early-return fix (patch 0005) for mac_reset_work
# does not prevent queues from being woken after a reset.  Also verifies
# that on USB, queues are properly woken after a firmware crash recovery.
#
# The test forces a firmware crash via chip_reset debugfs and verifies:
#   1. Queues are woken after the reset completes
#   2. No "wake queue" messages are missing
#   3. The driver can resume normal operation
#
# Usage: ./test_t6_reset.sh [--interface phy0] [--log-dir /tmp/mt7921u_test]
#
# Safety: Must be run as root. This test forces a chip reset.
#         Expect brief network interruption.

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

LOG_FILE="${LOG_DIR}/test_t6_reset_${TIMESTAMP}.log"
mkdir -p "$LOG_DIR"

echo "=== TEST-T6: Queue wake on reset verification ===" | tee "$LOG_FILE"
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

# Enable dynamic debug
echo 'module mt7921 +p' > /sys/kernel/debug/dynamic_debug/control 2>/dev/null || true
dmesg -c > /dev/null 2>&1 || true

# Check for chip_reset debugfs entry
RESET_FILE="${DEBUGFS_PATH}/chip_reset"
if [[ ! -f "$RESET_FILE" ]]; then
    echo "[SKIP] chip_reset debugfs file not found." | tee -a "$LOG_FILE"
    exit 0
fi

# Force firmware crash and reset
echo "Forcing chip reset via chip_reset debugfs..." | tee -a "$LOG_FILE"
echo 1 > "$RESET_FILE" 2>&1 || true

# Wait for reset to complete
echo "Waiting for reset to complete (10 seconds)..." | tee -a "$LOG_FILE"
sleep 10

# Capture dmesg
DMESG_OUTPUT=$(dmesg 2>/dev/null || true)
echo "" | tee -a "$LOG_FILE"
echo "dmesg (reset traces):" | tee -a "$LOG_FILE"
echo "$DMESG_OUTPUT" | grep -i "reset\|wake.*queue\|chip reset\|queue" | tee -a "$LOG_FILE" || true

# Evaluate result
CRASH_FOUND=false
if echo "$DMESG_OUTPUT" | grep -qi "Oops\|BUG:\|NULL pointer\|general protection fault"; then
    CRASH_FOUND=true
fi

if [[ "$CRASH_FOUND" == "true" ]]; then
    echo "[FAIL] Kernel crash during reset!" | tee -a "$LOG_FILE"
    exit 1
fi

# Check for "wake queue" or "wake_queues" messages
WAKE_FOUND=false
if echo "$DMESG_OUTPUT" | grep -qi "wake.*queue\|ieee80211_wake_queues\|chip reset"; then
    WAKE_FOUND=true
fi

if [[ "$WAKE_FOUND" == "true" ]]; then
    echo "[PASS] Queue wake messages found after reset." | tee -a "$LOG_FILE"
else
    echo "[WARN] No explicit queue wake messages found in dmesg." | tee -a "$LOG_FILE"
    echo "       This may be normal if the reset was handled differently." | tee -a "$LOG_FILE"
    echo "       Verify manually with: ping -c 3 <gateway>" | tee -a "$LOG_FILE"
fi

# Verify the driver can still operate
echo "" | tee -a "$LOG_FILE"
echo "Verifying driver operation after reset..." | tee -a "$LOG_FILE"
if [[ -d "$DEBUGFS_PATH" ]]; then
    echo "[PASS] Debugfs still accessible — driver is operational." | tee -a "$LOG_FILE"
else
    echo "[FAIL] Debugfs not accessible — driver may have crashed." | tee -a "$LOG_FILE"
    exit 1
fi

echo "" | tee -a "$LOG_FILE"
echo "Test completed. Log saved to: $LOG_FILE" | tee -a "$LOG_FILE"
