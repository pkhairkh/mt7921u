#!/bin/bash
# test_t7_twt.sh — TEST-T7: TWT functionality verification
#
# Verifies that the TWT (Target Wake Time) implementation (patch 0008)
# is functional by checking debugfs for TWT agreement statistics.
#
# Prerequisites:
#   - MT7921U device connected and driver loaded
#   - Associated with a TWT-capable AP (802.11ax/HE AP with TWT support)
#
# Usage: ./test_t7_twt.sh [--interface phy0] [--log-dir /tmp/mt7921u_test]
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

LOG_FILE="${LOG_DIR}/test_t7_twt_${TIMESTAMP}.log"
mkdir -p "$LOG_DIR"

echo "=== TEST-T7: TWT functionality verification ===" | tee "$LOG_FILE"
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

# Check for TWT debugfs entry
TWT_STATS_FILE="${DEBUGFS_PATH}/twt_stats"
if [[ ! -f "$TWT_STATS_FILE" ]]; then
    echo "[SKIP] TWT stats debugfs file not found." | tee -a "$LOG_FILE"
    echo "       Was TWT debugfs support compiled in? (patch 0008)" | tee -a "$LOG_FILE"
    exit 0
fi

# Enable dynamic debug
echo 'module mt7921 +p' > /sys/kernel/debug/dynamic_debug/control 2>/dev/null || true
dmesg -c > /dev/null 2>&1 || true

# Read TWT stats
echo "Reading TWT statistics..." | tee -a "$LOG_FILE"
TWT_OUTPUT=$(cat "$TWT_STATS_FILE" 2>&1) || true
echo "$TWT_OUTPUT" | tee -a "$LOG_FILE"

# Capture dmesg for TWT traces
DMESG_OUTPUT=$(dmesg 2>/dev/null || true)
echo "" | tee -a "$LOG_FILE"
echo "dmesg (TWT traces):" | tee -a "$LOG_FILE"
echo "$DMESG_OUTPUT" | grep -i "twt\|target wake" | tee -a "$LOG_FILE" || true

# Evaluate result
# TWT requires a TWT-capable AP, so the test is informational
if echo "$TWT_OUTPUT" | grep -q "n_agrt"; then
    N_AGRT=$(echo "$TWT_OUTPUT" | grep "n_agrt" | awk '{print $2}' || true)
    if [[ -n "$N_AGRT" && "$N_AGRT" -gt 0 ]]; then
        echo "[PASS] TWT agreements established: $N_AGRT" | tee -a "$LOG_FILE"
    else
        echo "[WARN] No TWT agreements currently established." | tee -a "$LOG_FILE"
        echo "       This may be normal if the AP does not support TWT." | tee -a "$LOG_FILE"
        echo "       Connect to a TWT-capable 802.11ax AP and re-run." | tee -a "$LOG_FILE"
    fi
else
    echo "[WARN] Could not parse TWT agreement count." | tee -a "$LOG_FILE"
fi

# Check for TWT-related crashes in dmesg
if echo "$DMESG_OUTPUT" | grep -qi "Oops\|BUG:\|NULL pointer.*twt\|twt.*crash"; then
    echo "[FAIL] TWT-related crash detected in dmesg!" | tee -a "$LOG_FILE"
    exit 1
fi

echo "" | tee -a "$LOG_FILE"
echo "Test completed. Log saved to: $LOG_FILE" | tee -a "$LOG_FILE"
