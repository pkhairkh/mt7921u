#!/bin/bash
# test_t5_suspend.sh — TEST-T5: Suspend/resume verification
#
# Verifies that the ROC timer use-after-free (BUG-04, patch 0004) is
# fixed by performing a suspend/resume cycle and checking dmesg for
# ROC timer UAF or other crash signatures.
#
# WITHOUT THE FIX (patch 0004):
#   If a ROC period has expired from mac80211's perspective but the
#   driver's roc_timer hasn't fired yet, ieee80211_roc_purge_local()
#   cannot purge it. The timer survives suspend and fires into freed
#   memory (UAF).
#   Predicted crash: general protection fault or slab corruption
#   in mt7921_roc_work / mt792x_roc_timer after resume.
#
# WITH THE FIX:
#   timer_delete_sync() and cancel_work_sync() are called during
#   mt792xu_cleanup() before wfsys_reset(), preventing the UAF.
#
# Prerequisites:
#   - MT7921U device connected and driver loaded
#   - Associated with an AP (iw ${INTERFACE} link should show connected)
#   - System supports suspend-to-RAM
#
# Usage: ./test_t5_suspend.sh [--interface phy0] [--log-dir /tmp/mt7921u_test]
#
# Safety: Must be run as root. This test suspends the system.
#         Ensure you have a way to wake the system (keyboard, power button).

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

LOG_FILE="${LOG_DIR}/test_t5_suspend_${TIMESTAMP}.log"
mkdir -p "$LOG_DIR"

echo "=== TEST-T5: Suspend/resume verification ===" | tee "$LOG_FILE"
echo "Interface: $INTERFACE" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Safety checks
if [[ $EUID -ne 0 ]]; then
    echo "[SKIP] This test must be run as root." | tee -a "$LOG_FILE"
    exit 0
fi

# Check that the device is connected
DEBUGFS_PATH="/sys/kernel/debug/ieee80211/${INTERFACE}/mt76"
if [[ ! -d "$DEBUGFS_PATH" ]]; then
    echo "[SKIP] Debugfs path $DEBUGFS_PATH not found." | tee -a "$LOG_FILE"
    exit 0
fi

# Check for AP association
WLAN_IF=$(iw dev | grep "Interface ${INTERFACE}" -A1 | grep "wiphy" | head -1 || true)
if ! iw dev 2>/dev/null | grep -q "Connected"; then
    echo "[WARN] Device may not be connected to an AP. Suspend test may be unreliable." | tee -a "$LOG_FILE"
fi

# Check ROC timer state before suspend
TRIGGER_PATH="${DEBUGFS_PATH}/test_trigger"
if [[ -d "$TRIGGER_PATH" ]]; then
    echo "ROC timer state before suspend:" | tee -a "$LOG_FILE"
    cat "${TRIGGER_PATH}/trigger_roc_timer" 2>/dev/null | tee -a "$LOG_FILE" || true
fi

# Enable dynamic debug
echo 'module mt7921 +p' > /sys/kernel/debug/dynamic_debug/control 2>/dev/null || true
echo 'module mt76 +p' > /sys/kernel/debug/dynamic_debug/control 2>/dev/null || true
dmesg -c > /dev/null 2>&1 || true

# Check if system supports suspend
if [[ ! -f /sys/power/state ]]; then
    echo "[SKIP] /sys/power/state not found. System does not support suspend." | tee -a "$LOG_FILE"
    exit 0
fi

# Prompt user before suspending
echo "" | tee -a "$LOG_FILE"
echo "WARNING: This test will suspend the system to RAM." | tee -a "$LOG_FILE"
echo "Press Enter to proceed (or Ctrl+C to abort)..." | tee -a "$LOG_FILE"
read -r

# Suspend
echo "Suspending..." | tee -a "$LOG_FILE"
echo mem > /sys/power/state 2>/dev/null || {
    echo "[SKIP] Suspend failed. System may not support suspend-to-RAM." | tee -a "$LOG_FILE"
    exit 0
}

# After resume
echo "Resumed from suspend." | tee -a "$LOG_FILE"
sleep 5  # Wait for driver to fully resume

# Capture dmesg after resume
DMESG_OUTPUT=$(dmesg 2>/dev/null || true)
echo "" | tee -a "$LOG_FILE"
echo "dmesg (post-resume, filtered):" | tee -a "$LOG_FILE"
echo "$DMESG_OUTPUT" | grep -i "roc_timer\|roc_work\|UAF\|use-after-free\|Oops\|BUG:\|slab\|suspend\|resume" | tee -a "$LOG_FILE" || true

# Check ROC timer state after resume
if [[ -d "$TRIGGER_PATH" ]]; then
    echo "" | tee -a "$LOG_FILE"
    echo "ROC timer state after resume:" | tee -a "$LOG_FILE"
    cat "${TRIGGER_PATH}/trigger_roc_timer" 2>/dev/null | tee -a "$LOG_FILE" || true
fi

# Evaluate result
CRASH_FOUND=false
if echo "$DMESG_OUTPUT" | grep -qi "UAF\|use-after-free\|Oops\|BUG:\|slab.*corrupt\|general protection fault"; then
    CRASH_FOUND=true
fi

if [[ "$CRASH_FOUND" == "true" ]]; then
    echo "[FAIL] Crash detected after resume! BUG-04 fix may not be working." | tee -a "$LOG_FILE"
    exit 1
fi

# Verify connection is restored
if iw dev 2>/dev/null | grep -q "Connected"; then
    echo "[PASS] No crash after resume. Connection restored successfully." | tee -a "$LOG_FILE"
else
    echo "[WARN] No crash after resume, but connection not restored. Check AP." | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"
echo "Test completed. Log saved to: $LOG_FILE" | tee -a "$LOG_FILE"
