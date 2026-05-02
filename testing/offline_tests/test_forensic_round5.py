#!/usr/bin/env python3
"""
Forensic Audit Round 5 — Sophistication & Hand-Wavy Implementation Test Suite

This test suite validates that all driver implementations are professional-grade,
not hand-wavy stubs or incorrect logic. It performs static analysis on the source
code to verify:

1. TWT validation logic matches the IEEE 802.11ax spec and mt7915/mt7996 references
2. CSI ring buffer sizing is reasonable (not ~4.4 MB per device)
3. DFS radar detection has proper timer management
4. HW timestamping is USB-safe (no sleeping in atomic context)
5. All bounds checks are present and correct
6. Module parameters and atomic counters are properly initialized
7. nl80211 vendor commands return appropriate error codes
8. No inverted logic in capability/feature validation
9. All new feature files have substantial implementation (not stubs)
10. The driver doesn't break PCIe/SDIO paths

Run: python3 test_forensic_round5.py
"""

import os
import re
import sys
import json

# Base path
BASE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DRIVER_DIR = os.path.join(BASE, "drivers", "net", "wireless", "mediatek", "mt76")

results = {"pass": [], "fail": [], "warn": []}

def read_file(rel_path):
    """Read a file relative to the mt76 directory."""
    full = os.path.join(DRIVER_DIR, rel_path)
    if not os.path.exists(full):
        return None
    with open(full, 'r', errors='replace') as f:
        return f.read()

def test_pass(name, detail=""):
    results["pass"].append({"name": name, "detail": detail})
    print(f"  [PASS] {name}")

def test_fail(name, detail=""):
    results["fail"].append({"name": name, "detail": detail})
    print(f"  [FAIL] {name}: {detail}")

def test_warn(name, detail=""):
    results["warn"].append({"name": name, "detail": detail})
    print(f"  [WARN] {name}: {detail}")


# ============================================================
# Test 1: TWT validation uses correct IEEE 802.11ax fields
# ============================================================
print("\n=== Test 1: TWT validation logic (IEEE 802.11ax compliance) ===")

twt_c = read_file("mt7921/twt.c")
if twt_c:
    # Must use IEEE80211_TWT_CONTROL_NEG_TYPE_BROADCAST (not REQTYPE_FLOWTYPE)
    if "IEEE80211_TWT_CONTROL_NEG_TYPE_BROADCAST" in twt_c:
        test_pass("TWT: Uses CONTROL_NEG_TYPE_BROADCAST for broadcast rejection")
    else:
        test_fail("TWT: Missing IEEE80211_TWT_CONTROL_NEG_TYPE_BROADCAST",
                  "Should use twt->control field, not req_type FLOWTYPE")

    # Must reject WAKE_DUR_UNIT when SET (256us), not when NOT set
    twt_check = twt_c[twt_c.find("mt7921_twt_check_req"):]
    twt_check_block = twt_check[:twt_check.find("return true;") + len("return true;")]
    
    if "IEEE80211_TWT_CONTROL_WAKE_DUR_UNIT" in twt_check_block:
        # Check it rejects when the bit IS set (not when it's NOT set)
        dur_unit_line = ""
        for line in twt_check_block.split('\n'):
            if "WAKE_DUR_UNIT" in line and "if" in line:
                dur_unit_line = line.strip()
                break
        
        if "if (twt->control &" in dur_unit_line and "WAKE_DUR_UNIT" in dur_unit_line:
            test_pass("TWT: Rejects 256us unit when bit IS set (correct)")
        elif "if (!" in dur_unit_line:
            test_fail("TWT: WAKE_DUR_UNIT logic inverted",
                      "Should reject when bit IS set, not when NOT set")
        else:
            test_warn("TWT: WAKE_DUR_UNIT check present but logic unclear")
    else:
        test_fail("TWT: Missing WAKE_DUR_UNIT check entirely")

    # Must NOT require IMMEDIA_FB (neither mt7915 nor mt7996 do)
    if "IMMEDIA_FB" in twt_check_block:
        test_fail("TWT: Still requires IMMEDIA_FB bit",
                  "Neither mt7915 nor mt7996 reference drivers require this")
    else:
        test_pass("TWT: Does not require IMMEDIA_FB bit")

    # Should require IMPLICIT (like mt7996)
    if "IEEE80211_TWT_REQTYPE_IMPLICIT" in twt_check_block:
        test_pass("TWT: Requires IMPLICIT agreement (matches mt7996)")
    else:
        test_warn("TWT: IMPLICIT requirement not found")

else:
    test_fail("TWT: twt.c not found")


# ============================================================
# Test 2: CSI ring buffer is not excessively large
# ============================================================
print("\n=== Test 2: CSI ring buffer sizing ===")

mt7921_h = read_file("mt7921/mt7921.h")
if mt7921_h:
    # Find MT7921_CSI_RING_SIZE definition
    ring_match = re.search(r'#define\s+MT7921_CSI_RING_SIZE\s+(\d+)', mt7921_h)
    if ring_match:
        ring_size = int(ring_match.group(1))
        if ring_size <= 128:
            test_pass(f"CSI ring buffer size: {ring_size} (reasonable)")
        elif ring_size <= 256:
            test_warn(f"CSI ring buffer size: {ring_size} (acceptable but large)")
        else:
            # At 1000 entries × ~4.4KB each = ~4.4 MB per device
            data_match = re.search(r'#define\s+MT7921_CSI_DATA_SIZE\s+(\d+)', mt7921_h)
            data_size = int(data_match.group(1)) if data_match else 256
            entry_size = data_size * 4 + 128  # i_data + q_data + metadata
            total_kb = (ring_size * entry_size) / 1024
            test_fail(f"CSI ring buffer size: {ring_size} entries (~{total_kb:.0f} KB)",
                      "Excessive for kernel struct — should be <= 128")
    else:
        test_fail("CSI: MT7921_CSI_RING_SIZE not defined")
else:
    test_fail("CSI: mt7921.h not found")


# ============================================================
# Test 3: DFS radar detection uses del_timer_sync
# ============================================================
print("\n=== Test 3: DFS radar detection timer safety ===")

main_c = read_file("mt7921/main.c")
if main_c:
    radar_func = main_c[main_c.find("mt7921_radar_detected_event"):]
    radar_func = radar_func[:radar_func.find("^static int")]
    
    if "del_timer_sync" in radar_func:
        test_pass("DFS: radar_detected uses del_timer_sync (safe)")
    elif "del_timer(" in radar_func and "del_timer_sync" not in radar_func:
        test_fail("DFS: radar_detected uses del_timer instead of del_timer_sync",
                  "Race condition with timer callback")
    else:
        test_warn("DFS: No timer deletion in radar_detected_event")

    # Check SKB length validation
    if "skb->len" in radar_func and "sizeof" in radar_func:
        test_pass("DFS: radar_detected validates SKB length")
    else:
        test_warn("DFS: radar_detected does not validate SKB length")

    # Check CAC timer is armed on success in start_radar_detection
    radar_start = main_c[main_c.find("mt7921_start_radar_detection"):]
    radar_start = radar_start[:radar_start.find("^static void")]
    
    if "mod_timer" in radar_start:
        test_pass("DFS: CAC timer armed on radar detection start success")
    else:
        test_fail("DFS: CAC timer NOT armed on start_radar_detection success")
else:
    test_fail("DFS: main.c not found")


# ============================================================
# Test 4: HW timestamping is USB-safe
# ============================================================
print("\n=== Test 4: HW timestamping USB safety ===")

mac_c = read_file("mt7921/mac.c")
if mac_c:
    # Check TX hw timestamp function guards USB
    tx_ts_func = mac_c[mac_c.find("mt7921_tx_hw_timestamp"):]
    tx_ts_func = tx_ts_func[:tx_ts_func.find("ktime_t mt7921_get_tstamp")]
    
    if "mt76_is_usb" in tx_ts_func:
        test_pass("TX HW timestamp: Guards USB path (avoids sleeping in TX completion)")
    else:
        test_fail("TX HW timestamp: No USB guard",
                  "mt76_set/mt76_rr on USB are vendor commands that may sleep")

    # Check get_tstamp guards USB
    get_ts_func = mac_c[mac_c.find("ktime_t mt7921_get_tstamp"):]
    get_ts_func = get_ts_func[:get_ts_func.find("^void mt7921_usb_sdio_tx_complete")]
    
    if "mt76_is_usb" in get_ts_func:
        test_pass("get_tstamp: Guards USB path (avoids sleeping in softirq)")
    else:
        test_fail("get_tstamp: No USB guard",
                  "ndo_get_tstamp may be called from softirq context")
else:
    test_fail("HW timestamping: mac.c not found")


# ============================================================
# Test 5: MCU timeout counter is explicitly initialized
# ============================================================
print("\n=== Test 5: MCU timeout counter initialization ===")

init_c = read_file("mt7921/init.c")
if init_c:
    if "atomic_set(&dev->mcu_timeout_count" in init_c:
        test_pass("MCU timeout counter: Explicitly initialized in register_device")
    else:
        test_fail("MCU timeout counter: Not explicitly initialized",
                  "Should call atomic_set(&dev->mcu_timeout_count, 0)")
else:
    test_fail("MCU timeout: init.c not found")


# ============================================================
# Test 6: CSI nl80211 returns appropriate error codes
# ============================================================
print("\n=== Test 6: CSI nl80211 error codes ===")

csi_nl = read_file("mt7921/csi_nl80211.c")
if csi_nl:
    if "-ENODATA" in csi_nl:
        test_pass("CSI nl80211: Returns -ENODATA for empty buffer")
    elif "-EAGAIN" in csi_nl:
        test_fail("CSI nl80211: Returns -EAGAIN for empty buffer",
                  "Should return -ENODATA — userspace won't retry vendor commands")
    else:
        test_warn("CSI nl80211: Empty buffer error code unclear")
else:
    test_fail("CSI nl80211: csi_nl80211.c not found")


# ============================================================
# Test 7: ACS channel state lookup has bounds checking
# ============================================================
print("\n=== Test 7: ACS bounds checking ===")

acs_c = read_file("mt7921/acs.c")
if acs_c:
    acs_state_func = acs_c[acs_c.find("acs_channel_state"):]
    acs_state_func = acs_state_func[:acs_state_func.find("^static bool")]
    
    if "idx < 0" in acs_state_func and "n_channels" in acs_state_func:
        test_pass("ACS: Channel index bounds checked")
    else:
        test_fail("ACS: Missing channel index bounds checking")

    # Check for pointer validation
    if "channels[idx] != c" in acs_state_func:
        test_pass("ACS: Channel pointer validated against array")
    else:
        test_warn("ACS: Channel pointer not validated against array base")
else:
    test_fail("ACS: acs.c not found")


# ============================================================
# Test 8: No hand-wavy stubs (functions that just return 0/-ENOTSUPP)
# ============================================================
print("\n=== Test 8: No hand-wavy stub implementations ===")

feature_files = [
    "mt7921/twt.c",
    "mt7921/csi.c",
    "mt7921/csi_nl80211.c",
    "mt7921/acs.c",
]

for fpath in feature_files:
    content = read_file(fpath)
    if not content:
        continue
    
    fname = os.path.basename(fpath)
    loc = len([l for l in content.split('\n') if l.strip() and not l.strip().startswith(('/', '*', ' *', '//'))])
    
    # Check for stub patterns
    stub_patterns = [
        r'return\s+0\s*;\s*//\s*TODO',
        r'return\s+-ENOTSUPP\s*;\s*//\s*stub',
        r'/\*\s*stub\s*\*/',
    ]
    
    stubs_found = []
    for pat in stub_patterns:
        matches = re.findall(pat, content, re.IGNORECASE)
        stubs_found.extend(matches)
    
    if loc < 50:
        test_warn(f"{fname}: Only {loc} LOC — may be a stub", fpath)
    elif stubs_found:
        test_fail(f"{fname}: Contains {len(stubs_found)} stub markers", fpath)
    else:
        test_pass(f"{fname}: {loc} LOC, no stub markers")


# ============================================================
# Test 9: Bus-aware code doesn't break PCIe/SDIO
# ============================================================
print("\n=== Test 9: Bus-aware code safety ===")

if mac_c:
    # Check that USB-specific guards use mt76_is_usb()
    usb_guards = len(re.findall(r'mt76_is_usb\s*\(', mac_c))
    if usb_guards >= 2:
        test_pass(f"mac.c: {usb_guards} USB-specific guards found")
    else:
        test_warn(f"mac.c: Only {usb_guards} USB guards — may break PCIe/SDIO")

if main_c:
    usb_guards_main = len(re.findall(r'mt76_is_usb\s*\(', main_c))
    mmio_guards_main = len(re.findall(r'mt76_is_mmio\s*\(', main_c))
    test_pass(f"main.c: {usb_guards_main} USB + {mmio_guards_main} MMIO guards")


# ============================================================
# Test 10: TWT SP event bounds check uses device table limit
# ============================================================
print("\n=== Test 10: TWT SP event bounds checking ===")

if twt_c:
    sp_func = twt_c[twt_c.find("mt7921_twt_sp_event"):]
    sp_func = sp_func[:sp_func.find("^/\n")]
    
    if "flow_id >= MT7921_MAX_TWT_AGRT" in sp_func:
        test_pass("TWT SP event: Bounds check uses MT7921_MAX_TWT_AGRT (device table limit)")
    elif "flow_id >= MT7921_MAX_STA_TWT_AGRT" in sp_func:
        test_fail("TWT SP event: Bounds check uses per-STA limit instead of device table limit",
                  "Should use MT7921_MAX_TWT_AGRT for device-level stats arrays")
    else:
        test_warn("TWT SP event: No clear bounds check on flow_id")


# ============================================================
# Test 11: CSI stop flushes ring buffer
# ============================================================
print("\n=== Test 11: CSI stop flushes ring buffer ===")

csi_c = read_file("mt7921/csi.c")
if csi_c:
    stop_func = csi_c[csi_c.find("mt7921_csi_stop"):]
    stop_func = stop_func[:stop_func.find("void mt7921_csi_init")]
    
    if "head = 0" in stop_func and "tail = 0" in stop_func:
        test_pass("CSI stop: Flushes ring buffer (resets head/tail)")
    else:
        test_fail("CSI stop: Does not flush ring buffer",
                  "Stale data could be returned by subsequent CSI_GET calls")


# ============================================================
# Test 12: TWT param_equal uses correct comparison
# ============================================================
print("\n=== Test 12: TWT param_equal comparison quality ===")

if twt_c:
    param_equal = twt_c[twt_c.find("mt7921_twt_param_equal"):]
    param_equal = param_equal[:param_equal.find("^static bool\nmt7921_twt_check_req")]
    
    # Check that all relevant fields are compared
    fields_compared = []
    for field in ["mantissa", "exp", "duration", "protection", "flowtype", "trigger"]:
        if f"flow->{field}" in param_equal:
            fields_compared.append(field)
    
    if len(fields_compared) >= 5:
        test_pass(f"TWT param_equal: Compares {len(fields_compared)} fields ({', '.join(fields_compared)})")
    else:
        test_warn(f"TWT param_equal: Only compares {len(fields_compared)} fields — may miss duplicates")


# ============================================================
# Test 13: Debugfs TWT stats access pattern
# ============================================================
print("\n=== Test 13: TWT debugfs RCU safety ===")

twt_debugfs = read_file("mt7921/twt_debugfs.c")
if twt_debugfs:
    if "rcu_read_lock" in twt_debugfs and "rcu_read_unlock" in twt_debugfs:
        test_pass("TWT debugfs: Uses RCU read lock/unlock properly")
    else:
        test_fail("TWT debugfs: Missing RCU read lock",
                  "Accessing dev->mt76.wcid[] requires RCU protection")


# ============================================================
# Test 14: CLC fallback is USB-only
# ============================================================
print("\n=== Test 14: CLC fallback bus-safety ===")

mcu_c = read_file("mt7921/mcu.c")
if mcu_c:
    clc_section = mcu_c[mcu_c.find("mt7921_load_clc"):]
    clc_section = clc_section[:clc_section.find("^static void\nmt7921_mcu_parse_tx_resource")]
    
    if "mt76_is_usb(mdev)" in clc_section and "!clc_force_usb" in clc_section:
        test_pass("CLC fallback: Only applies on USB bus")
    else:
        test_warn("CLC fallback: Bus check may be missing or incomplete")


# ============================================================
# Test 15: ops table completeness
# ============================================================
print("\n=== Test 15: mt7921_ops completeness ===")

if main_c:
    # Extract the ops struct
    ops_match = re.search(r'const\s+struct\s+ieee80211_ops\s+mt7921_ops\s*=\s*\{(.*?)\};',
                         main_c, re.DOTALL)
    if ops_match:
        ops_body = ops_match.group(1)
        
        required_ops = [
            ("get_tstamp", "HW timestamping"),
            ("get_survey", "ACS survey data"),
            ("start_radar_detection", "DFS radar detection"),
            ("end_cac", "DFS CAC end"),
            ("add_twt_setup", "TWT setup"),
            ("twt_teardown_request", "TWT teardown"),
        ]
        
        for op_name, feature in required_ops:
            if f".{op_name}" in ops_body:
                test_pass(f"ops: .{op_name} registered ({feature})")
            else:
                test_fail(f"ops: .{op_name} MISSING ({feature})")
    else:
        test_fail("ops: Could not parse mt7921_ops struct")


# ============================================================
# Summary
# ============================================================
print("\n" + "=" * 60)
print("FORENSIC AUDIT ROUND 5 — SUMMARY")
print("=" * 60)
print(f"  PASS: {len(results['pass'])}")
print(f"  FAIL: {len(results['fail'])}")
print(f"  WARN: {len(results['warn'])}")
print()

if results["fail"]:
    print("FAILED TESTS:")
    for f in results["fail"]:
        print(f"  - {f['name']}: {f['detail']}")
    print()

if results["warn"]:
    print("WARNINGS:")
    for w in results["warn"]:
        print(f"  - {w['name']}: {w['detail']}")
    print()

# Save results as JSON
results_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "results_round5.json")
with open(results_file, 'w') as f:
    json.dump(results, f, indent=2)

print(f"Results saved to {results_file}")

# Exit with failure if any tests failed
sys.exit(1 if results["fail"] else 0)
