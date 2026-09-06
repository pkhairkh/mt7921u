#!/bin/bash
# ================================================================
# mt7921u Static Analysis Test Suite
# Validates source code patterns without kernel or hardware
# ================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DRIVER_DIR="$REPO_ROOT/drivers/net/wireless/mediatek/mt76/mt7921"
PASS=0
FAIL=0
WARN=0

pass() { PASS=$((PASS+1)); echo "  PASS: $1"; }
fail() { FAIL=$((FAIL+1)); echo "  FAIL: $1"; }
warn() { WARN=$((WARN+1)); echo "  WARN: $1"; }

echo "=== mt7921u Static Analysis Tests ==="
echo "Repo: $REPO_ROOT"
echo "Driver dir: $DRIVER_DIR"
echo ""

# ================================================================
# TEST GROUP 1: Stub Detection
# ================================================================
echo "--- Stub Detection ---"

# Check for return -ENOTSUPP / return -EINVAL / return 0 stubs
STUBS=$(rg -n 'return\s+-(ENOTSUPP|EOPNOTSUPP)\s*;' "$DRIVER_DIR" --include='*.c' || true)
if [ -z "$STUBS" ]; then
    pass "No -ENOTSUPP/EOPNOTSUPP stubs found"
else
    echo "$STUBS"
    fail "Found ENOTSUPP/EOPNOTSUPP stubs — these should be real implementations"
fi

# Check for TODO/FIXME/HACK comments
TODOS=$(rg -n 'TODO|FIXME|HACK|XXX' "$DRIVER_DIR" --include='*.c' --include='*.h' || true)
if [ -n "$TODOS" ]; then
    COUNT=$(echo "$TODOS" | wc -l)
    warn "Found $COUNT TODO/FIXME/HACK/XXX comments — review for completeness"
    echo "$TODOS" | head -20
else
    pass "No TODO/FIXME/HACK/XXX comments found"
fi

# Check for empty function bodies
EMPTIES=$(rg -n '^\s*\}\s*$' "$DRIVER_DIR" --include='*.c' -B3 | rg 'void\s+\w+\(' || true)
if [ -z "$EMPTIES" ]; then
    pass "No empty function bodies found"
else
    echo "$EMPTIES"
    warn "Possibly empty function bodies found"
fi

# ================================================================
# TEST GROUP 2: Missing NULL Checks
# ================================================================
echo ""
echo "--- Missing NULL Checks ---"

# Check for dereferences without NULL guard in regd_set_6ghz_power_type
if rg -n 'chanreq\.oper\.chan->' "$DRIVER_DIR/main.c" > /dev/null 2>&1; then
    # Check if there's a NULL guard before it
    if rg -n 'chanreq\.oper\.chan\b' "$DRIVER_DIR/main.c" | rg -v '->' | rg 'NULL|!.*chan' > /dev/null 2>&1; then
        pass "NULL guard found for chanreq.oper.chan"
    else
        fail "Missing NULL guard for vif->bss_conf.chanreq.oper.chan dereference (main.c:815)"
    fi
else
    pass "No chanreq.oper.chan dereferences found (already fixed?)"
fi

# Check rcu_dereference without NULL check
RCU_DEREF=$(rg -n 'rcu_dereference\(' "$DRIVER_DIR" --include='*.c' | head -10 || true)
if [ -n "$RCU_DEREF" ]; then
    warn "rcu_dereference calls found — verify NULL checks follow:"
    echo "$RCU_DEREF"
else
    pass "No rcu_dereference calls found (or all properly guarded)"
fi

# ================================================================
# TEST GROUP 3: Mutex / Locking Consistency
# ================================================================
echo ""
echo "--- Mutex / Locking Consistency ---"

# Check that cancel_work (not _sync) is used in roc_abort_sync
if rg -n 'cancel_work\b' "$DRIVER_DIR/main.c" | rg -v 'cancel_work_sync' > /dev/null 2>&1; then
    fail "cancel_work() (non-blocking) used instead of cancel_work_sync() in roc_abort_sync"
else
    pass "No cancel_work() (should be cancel_work_sync)"
fi

# Check for double mutex acquire in sta_remove
if rg -n 'mt76_connac_pm_wake' "$DRIVER_DIR/main.c" > /dev/null 2>&1; then
    PM_WAKE_COUNT=$(rg -c 'mt76_connac_pm_wake' "$DRIVER_DIR/main.c" || echo 0)
    MUTEX_ACQ_COUNT=$(rg -c 'mt792x_mutex_acquire' "$DRIVER_DIR/main.c" || echo 0)
    warn "Found $PM_WAKE_COUNT pm_wake and $MUTEX_ACQ_COUNT mutex_acquire in main.c — check for double-lock in sta_remove"
else
    pass "No pm_wake/mutex_acquire conflict detected"
fi

# Check CSI ring buffer has no locking
if rg -n 'spin_lock\|mutex_lock\|smp_store_release\|smp_load_acquire' "$DRIVER_DIR/csi.c" > /dev/null 2>&1; then
    pass "CSI ring buffer has synchronization"
else
    fail "CSI ring buffer (csi.c) has NO locking — race condition with concurrent writer/reader"
fi

# Check ACS has no mutex in update/show
if rg -n 'mt792x_mutex_acquire\|mutex_lock' "$DRIVER_DIR/acs.c" > /dev/null 2>&1; then
    pass "ACS has mutex protection"
else
    fail "ACS update/show (acs.c) has NO mutex — race condition reading shared survey data"
fi

# ================================================================
# TEST GROUP 4: SKB Lifecycle
# ================================================================
echo ""
echo "--- SKB Lifecycle ---"

# Check CSI event: does mcu.c call skb_pull before csi_event?
CSI_EVENT_LINE=$(rg -n 'case 0x3C' "$DRIVER_DIR/mcu.c" | head -1 | cut -d: -f1 || true)
TWT_EVENT_LINE=$(rg -n 'case 0x85' "$DRIVER_DIR/mcu.c" | head -1 | cut -d: -f1 || true)

if [ -n "$CSI_EVENT_LINE" ] && [ -n "$TWT_EVENT_LINE" ]; then
    # Check if there's skb_pull between case 0x3C and the csi_event call
    CSI_BLOCK=$(sed -n "${CSI_EVENT_LINE},$((CSI_EVENT_LINE+5))p" "$DRIVER_DIR/mcu.c")
    if echo "$CSI_BLOCK" | rg -q 'skb_pull'; then
        pass "CSI event has skb_pull before handler"
    else
        fail "CSI event (0x3C) missing skb_pull — TWT (0x85) has it but CSI doesn't"
    fi
else
    warn "Could not find CSI/TWT event lines in mcu.c"
fi

# Check for double kfree_skb patterns
DOUBLE_FREE=$(rg -n 'dev_kfree_skb' "$DRIVER_DIR/mcu.c" | rg -c 'dev_kfree_skb' || echo 0)
warn "$DOUBLE_FREE dev_kfree_skb calls in mcu.c — verify no double-free paths"

# ================================================================
# TEST GROUP 5: Error Path Completeness
# ================================================================
echo ""
echo "--- Error Path Completeness ---"

# Check mac_sta_add error path for wcid leak
STA_ADD_LINE=$(rg -n 'mt7921_mcu_sta_update.*MT76_STA_INFO_STATE_NONE' "$DRIVER_DIR/main.c" | head -1 | cut -d: -f1 || true)
if [ -n "$STA_ADD_LINE" ]; then
    # Check if error path after this line frees wcid
    ERROR_PATH=$(sed -n "$((STA_ADD_LINE)),$((STA_ADD_LINE+5))p" "$DRIVER_DIR/main.c")
    if echo "$ERROR_PATH" | rg -q 'wcid_free\|mt76_wcid_free'; then
        pass "mac_sta_add error path frees wcid"
    else
        fail "mac_sta_add error path does NOT free wcid — resource leak on mcu_sta_update failure"
    fi
else
    warn "Could not find mac_sta_add mcu_sta_update call"
fi

# Check DFS end_cac ignores return value
END_CAC_LINE=$(rg -n 'mt7921_end_cac' "$DRIVER_DIR/main.c" | head -1 | cut -d: -f1 || true)
if [ -n "$END_CAC_LINE" ]; then
    # Look for the mt76_mcu_send_msg call and check if return value is used
    END_CAC_BLOCK=$(sed -n "$END_CAC_LINE,$((END_CAC_LINE+40))p" "$DRIVER_DIR/main.c")
    if echo "$END_CAC_BLOCK" | rg -q 'err\s*=\s*mt76_mcu_send_msg\|ret\s*=\s*mt76_mcu_send_msg'; then
        pass "DFS end_cac checks return value of MCU command"
    else
        fail "DFS end_cac ignores return value — firmware/driver state mismatch possible"
    fi
else
    warn "Could not find end_cac implementation"
fi

# ================================================================
# TEST GROUP 6: Ops Registration Completeness
# ================================================================
echo ""
echo "--- Ops Registration ---"

# Verify all advertised features have their ops wired
OPS_FILE="$DRIVER_DIR/main.c"

# Check TWT ops
if rg -q 'add_twt_setup\s*=' "$OPS_FILE"; then
    pass "TWT .add_twt_setup wired in ops"
else
    fail "TWT .add_twt_setup NOT in ops"
fi

if rg -q 'twt_teardown_request\s*=' "$OPS_FILE"; then
    pass "TWT .twt_teardown_request wired in ops"
else
    fail "TWT .twt_teardown_request NOT in ops"
fi

# Check DFS ops
if rg -q 'start_radar_detection\s*=' "$OPS_FILE"; then
    pass "DFS .start_radar_detection wired in ops"
else
    fail "DFS .start_radar_detection NOT in ops"
fi

if rg -q '\.end_cac\s*=' "$OPS_FILE"; then
    pass "DFS .end_cac wired in ops"
else
    fail "DFS .end_cac NOT in ops"
fi

# Check HW timestamping flags
if rg -q 'NL80211_FEATURE_HW_TIMESTAMP\|NETIF_F_HW_HWTSTAMP' "$REPO_ROOT/drivers/net/wireless/mediatek/mt76/mt792x_core.c" 2>/dev/null || \
   rg -q 'NL80211_FEATURE_HW_TIMESTAMP\|NETIF_F_HW_HWTSTAMP' "$REPO_ROOT/drivers/net/wireless/mediatek/mt76/mt7921/init.c" 2>/dev/null; then
    pass "HW timestamping feature flag set"
else
    warn "HW timestamping feature flag not found"
fi

# ================================================================
# TEST GROUP 7: Bus Guard Consistency
# ================================================================
echo ""
echo "--- Bus Guard Consistency ---"

# Check that USB-specific code is guarded with mt76_is_usb()
USB_GUARDS=$(rg -n 'mt76_is_usb\(' "$DRIVER_DIR" --include='*.c' | wc -l || echo 0)
if [ "$USB_GUARDS" -gt 0 ]; then
    pass "Found $USB_GUARDS mt76_is_usb() bus guards"
else
    warn "No mt76_is_usb() guards found in mt7921/ — USB-specific code may not be guarded"
fi

# Check MT7921_CLC_MAX_NUM vs MT792x_CLC_MAX_NUM consistency
CLC_MT7921=$(rg -n 'MT7921_CLC_MAX_NUM' "$DRIVER_DIR/mt7921.h" | head -1 || true)
CLC_MT792X=$(rg -n 'MT792x_CLC_MAX_NUM' "$REPO_ROOT/drivers/net/wireless/mediatek/mt76/mt792x.h" | head -1 || true)
if [ -n "$CLC_MT7921" ] && [ -n "$CLC_MT792X" ]; then
    warn "Both MT7921_CLC_MAX_NUM and MT792x_CLC_MAX_NUM exist — verify values match"
    echo "    $CLC_MT7921"
    echo "    $CLC_MT792X"
else
    pass "CLC_MAX_NUM defined consistently"
fi

# ================================================================
# TEST GROUP 8: MCU Timeout Mismatch
# ================================================================
echo ""
echo "--- MCU Timeout Configuration ---"

# Check if bulk_msg timeout is hardcoded
BULK_TIMEOUT=$(rg -n 'mt76u_bulk_msg.*1000' "$DRIVER_DIR/usb.c" || true)
MCU_TIMEOUT=$(rg -n 'mcu\.timeout\s*=\s*3\s*\*\s*HZ' "$DRIVER_DIR/usb.c" || true)
if [ -n "$BULK_TIMEOUT" ] && [ -n "$MCU_TIMEOUT" ]; then
    fail "USB bulk_msg timeout (1000ms) < MCU timeout (3*HZ=3000ms) — premature ETIMEDOUT"
    echo "    $BULK_TIMEOUT"
    echo "    $MCU_TIMEOUT"
else
    pass "Timeout values consistent or already fixed"
fi

# ================================================================
# Summary
# ================================================================
echo ""
echo "=== Static Analysis Results ==="
echo "  PASS: $PASS"
echo "  FAIL: $FAIL"
echo "  WARN: $WARN"
echo "  TOTAL: $((PASS+FAIL+WARN))"

if [ "$FAIL" -gt 0 ]; then
    echo ""
    echo "CRITICAL: $FAIL failures found — must fix before hardware test!"
    exit 1
fi

exit 0
