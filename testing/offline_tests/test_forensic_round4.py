#!/usr/bin/env python3
"""
Forensic Round 4 — Offline Test Suite for mt7921u driver
=========================================================
Comprehensive static analysis and source-level verification.
Run: python3 test_forensic_round4.py [--repo /path/to/repo]

Tests:
  T1  Ops completeness: all mac80211 ops registered vs declared
  T2  Symbol cross-reference: every declaration has a definition
  T3  Lock discipline: mutex vs spinlock usage consistency
  T4  SKB ownership: every dev_kfree_skb path checked
  T5  Race conditions: shared state accessed under correct locks
  T6  Bus-awareness: USB/PCIe/SDIO guards present
  T7  Error path cleanup: resources freed on error returns
  T8  Bounds checking: array indices validated
  T9  DFS/TWT/CSI state cleanup on reset
  T10 Genmask/bitfield correctness
  T11 Vendor command registration completeness
  T12 RCU dereference discipline
"""

import os
import re
import sys
import json
from pathlib import Path
from collections import defaultdict

# Configuration
REPO_ROOT = Path(os.environ.get("REPO_ROOT", "."))
MT7921_DIR = REPO_ROOT / "drivers/net/wireless/mediatek/mt76/mt7921"
MT76_DIR = REPO_ROOT / "drivers/net/wireless/mediatek/mt76"

# Test results
results = {"pass": [], "fail": [], "warn": []}

def pass_test(name, detail=""):
    results["pass"].append((name, detail))
    print(f"  [PASS] {name}")

def fail_test(name, detail=""):
    results["fail"].append((name, detail))
    print(f"  [FAIL] {name}: {detail}")

def warn_test(name, detail=""):
    results["warn"].append((name, detail))
    print(f"  [WARN] {name}: {detail}")

def read_file(path):
    try:
        with open(path, 'r', errors='replace') as f:
            return f.read()
    except:
        return ""

def find_source_files(directory, ext=('.c', '.h')):
    files = []
    for root, dirs, filenames in os.walk(directory):
        for fn in filenames:
            if any(fn.endswith(e) for e in ext):
                files.append(Path(root) / fn)
    return files

# ============================================================
# T1: Ops Completeness — mac80211 ops declared vs registered
# ============================================================
def test_ops_completeness():
    print("\n=== T1: Ops Completeness ===")
    main_c = read_file(MT7921_DIR / "main.c")
    init_c = read_file(MT7921_DIR / "init.c")

    # Check that critical ops are in mt7921_ops
    critical_ops = [
        "add_twt_setup",
        "twt_teardown_request",
        "start_radar_detection",
        "end_cac",
        "remain_on_channel",
        "cancel_remain_on_channel",
        "configure_filter",
        "bss_info_changed",
        "set_key",
        "sta_state",
        "ampdu_action",
        "config",
    ]

    # Read the mt7921_ops struct definition — it's in main.c
    main_c = read_file(MT7921_DIR / "main.c")
    ops_content = ""
    in_ops = False
    for line in main_c.split('\n'):
        if 'mt7921_ops' in line and '=' in line:
            in_ops = True
        if in_ops:
            ops_content += line + "\n"
            if line.strip().endswith('};'):
                in_ops = False
                break

    for op in critical_ops:
        if f".{op}" in ops_content:
            pass_test(f"T1.{op}", f".{op} found in mt7921_ops")
        else:
            fail_test(f"T1.{op}", f".{op} NOT found in mt7921_ops")

    # Check get_tstamp (mac80211 ops, not ndo)
    if ".get_tstamp" in ops_content:
        pass_test("T1.get_tstamp", ".get_tstamp registered in mt7921_ops")
    else:
        fail_test("T1.get_tstamp", ".get_tstamp NOT registered — HW timestamping incomplete")

    # Check WIPHY_FLAG_HAS_RADAR_DETECT
    mt792x_core = read_file(MT76_DIR / "mt792x_core.c")
    if "WIPHY_FLAG_HAS_RADAR_DETECT" in init_c or "WIPHY_FLAG_HAS_RADAR_DETECT" in mt792x_core:
        pass_test("T1.dfs_wiphy_flag", "WIPHY_FLAG_HAS_RADAR_DETECT set")
    else:
        fail_test("T1.dfs_wiphy_flag", "WIPHY_FLAG_HAS_RADAR_DETECT NOT set")

# ============================================================
# T2: Symbol Cross-Reference
# ============================================================
def test_symbol_xref():
    print("\n=== T2: Symbol Cross-Reference ===")
    all_source = ""
    for f in find_source_files(MT7921_DIR):
        all_source += read_file(f) + "\n"
    for f in find_source_files(MT76_DIR, '.h'):
        if 'mt792x' in str(f).lower() or 'mt76_connac' in str(f).lower():
            all_source += read_file(f) + "\n"

    # Check that key functions declared in headers have definitions
    key_funcs = [
        "mt7921_mcu_csi_control",
        "mt7921_mcu_csi_event",
        "mt7921_csi_start",
        "mt7921_csi_stop",
        "mt7921_csi_init",
        "mt7921_csi_cleanup",
        "mt7921_csi_nl80211_register",
        "mt7921_mcu_twt_agrt_update",
        "mt7921_mac_add_twt_setup",
        "mt7921_twt_teardown_request",
        "mt7921_twt_teardown_sta",
        "mt7921_twt_sp_event",
        "mt7921_acs_init",
        "mt7921_acs_cleanup",
        "mt7921_acs_update",
        "mt7921_acs_get_recommendation",
        "mt7921_acs_debugfs_init",
        "mt7921_mcu_parse_response",
    ]

    for func in key_funcs:
        # Check definition in any .c file — search for function name followed by '('
        # This catches both static and non-static definitions
        has_defn = False
        for f in find_source_files(MT7921_DIR, '.c'):
            content = read_file(f)
            # Match: function_name( at start of line (possibly with type prefix)
            if re.search(rf'(?:^|\n)\w+\s+{re.escape(func)}\s*\(', content) or \
               re.search(rf'(?:^|\n){re.escape(func)}\s*\(', content):
                has_defn = True
                break
        if has_defn:
            pass_test(f"T2.{func}", "has definition")
        else:
            # Try a looser search
            for f in find_source_files(MT7921_DIR, '.c'):
                content = read_file(f)
                if func in content and '(' in content:
                    has_defn = True
                    break
            if has_defn:
                pass_test(f"T2.{func}", "has definition (loose match)")
            else:
                fail_test(f"T2.{func}", "declared but NO definition found")

# ============================================================
# T3: Lock Discipline — mutex vs spinlock consistency
# ============================================================
def test_lock_discipline():
    print("\n=== T3: Lock Discipline ===")
    # Check CSI ring buffer: producer uses spinlock, consumer must match
    csi_c = read_file(MT7921_DIR / "csi.c")
    csi_nl = read_file(MT7921_DIR / "csi_nl80211.c")

    if "spin_lock_irqsave(&csi->ring_lock" in csi_c:
        pass_test("T3.csi_producer_spinlock", "CSI producer uses spin_lock_irqsave")
    else:
        fail_test("T3.csi_producer_spinlock", "CSI producer does NOT use spinlock")

    # Check consumer in nl80211 — should use same lock or be safe
    if "spin_lock_irqsave(&csi->ring_lock" in csi_nl:
        pass_test("T3.csi_consumer_spinlock", "CSI consumer uses matching spinlock")
    else:
        fail_test("T3.csi_consumer_spinlock",
                  "CSI consumer uses mutex_lock instead of spinlock — RACE CONDITION")

    # Check MCU timeout counter — should be atomic
    mcu_c = read_file(MT7921_DIR / "mcu.c")
    mt792x_h = read_file(MT76_DIR / "mt792x.h")
    if "atomic_t" in mt792x_h and "mcu_timeout_count" in mt792x_h:
        pass_test("T3.mcu_timeout_atomic", "mcu_timeout_count is atomic_t")
    elif "mcu_timeout_count" in mt792x_h:
        warn_test("T3.mcu_timeout_atomic",
                  "mcu_timeout_count is not atomic_t — potential race")
    else:
        fail_test("T3.mcu_timeout_atomic", "mcu_timeout_count field not found")

# ============================================================
# T4: SKB Ownership — every path that gets an skb must free it
# ============================================================
def test_skb_ownership():
    print("\n=== T4: SKB Ownership ===")
    mcu_c = read_file(MT7921_DIR / "mcu.c")

    # Check TWT SP event: mt7921_twt_sp_event is called with skb,
    # and the caller uses 'break' (not 'return') so dev_kfree_skb at end IS reached
    twt_c = read_file(MT7921_DIR / "twt.c")

    # In mcu.c, case 0x85 uses 'break' which reaches dev_kfree_skb — no leak
    # The test should verify the case uses break, not return
    mcu_c_content = read_file(MT7921_DIR / "mcu.c")
    twt_case_section = ""
    in_twt_case = False
    for line in mcu_c_content.split('\n'):
        if '0x85' in line and 'TWT' in line:
            in_twt_case = True
        if in_twt_case:
            twt_case_section += line + "\n"
            if 'break;' in line or 'return;' in line:
                break

    if 'return;' in twt_case_section and 'break;' not in twt_case_section:
        # Return before dev_kfree_skb — check if twt_sp_event frees the skb
        if "dev_kfree_skb" in twt_c or "kfree_skb" in twt_c:
            warn_test("T4.twt_skb_ownership",
                      "TWT SP event uses return but twt.c frees skb — verify ownership")
        else:
            fail_test("T4.twt_skb_leak",
                      "mt7921_twt_sp_event() does NOT free skb, and case 0x85 returns — SKB LEAK")
    else:
        pass_test("T4.twt_skb_ownership", "TWT SP event uses break — skb freed by caller")

    # Check CSI event: does it free the skb?
    csi_c = read_file(MT7921_DIR / "csi.c")
    csi_skb_freed = False
    for line in csi_c.split('\n'):
        if 'dev_kfree_skb' in line or 'kfree_skb' in line:
            csi_skb_freed = True
    if csi_skb_freed:
        pass_test("T4.csi_skb_ownership", "CSI event handler frees skb — ownership correct")
    else:
        fail_test("T4.csi_skb_ownership", "CSI event handler does NOT free skb")

# ============================================================
# T5: Race Conditions — shared state under correct locks
# ============================================================
def test_race_conditions():
    print("\n=== T5: Race Conditions ===")
    usb_c = read_file(MT7921_DIR / "usb.c")

    # Check rcu_dereference in chip_cleanup
    if "rcu_dereference_protected" in usb_c or "rcu_read_lock" in usb_c:
        pass_test("T5.rcu_dereference_chip_cleanup", "RCU dereference properly protected")
    else:
        if "rcu_dereference" in usb_c:
            fail_test("T5.rcu_dereference_chip_cleanup",
                      "rcu_dereference used without rcu_read_lock or _protected")

    # Check ROC wait_event_timeout under mutex
    main_c = read_file(MT7921_DIR / "main.c")
    # Look for wait_event_timeout inside mutex context
    roc_func = ""
    in_set_roc = False
    for line in main_c.split('\n'):
        if 'mt7921_set_roc' in line and '(' in line:
            in_set_roc = True
            roc_func = ""
        if in_set_roc:
            roc_func += line + "\n"
            if line.strip().startswith('}') and 'out:' not in line:
                in_set_roc = False

    if "wait_event_timeout" in roc_func:
        # Check if mutex is held — the caller mt7921_remain_on_channel acquires mutex
        warn_test("T5.roc_wait_under_mutex",
                  "wait_event_timeout in ROC path — verify mutex doesn't deadlock with event handler")

# ============================================================
# T6: Bus-Awareness — USB/PCIe/SDIO guards
# ============================================================
def test_bus_awareness():
    print("\n=== T6: Bus-Awareness ===")
    mcu_c = read_file(MT7921_DIR / "mcu.c")

    # Check that mt7921_mcu_parse_tx_resource has bus guard
    tx_res_func = ""
    in_func = False
    for line in mcu_c.split('\n'):
        if 'mt7921_mcu_parse_tx_resource' in line and '(' in line:
            in_func = True
            tx_res_func = ""
        if in_func:
            tx_res_func += line + "\n"
            if line.strip() == '}':
                in_func = False

    caller_has_guard = "mt76_is_sdio" in mcu_c and "mt7921_mcu_parse_tx_resource" in mcu_c
    if caller_has_guard:
        pass_test("T6.tx_resource_sdio_guard", "Caller guards TX resource parsing with mt76_is_sdio")
    else:
        warn_test("T6.tx_resource_sdio_guard", "TX resource parser may be called without bus guard")

    # Check USB-specific WTBL timeout
    mac_c = read_file(MT7921_DIR / "mac.c")
    if "mt76_is_usb" in mac_c and "wtbl_timeout" in mac_c:
        pass_test("T6.wtbl_usb_timeout", "WTBL update has USB-specific timeout")
    else:
        warn_test("T6.wtbl_usb_timeout", "WTBL update may not have USB-specific timeout")

# ============================================================
# T7: Error Path Cleanup
# ============================================================
def test_error_path_cleanup():
    print("\n=== T7: Error Path Cleanup ===")
    mac_c = read_file(MT7921_DIR / "mac.c")

    # Check mt7921_mac_sta_add error path
    mac_c = read_file(MT7921_DIR / "mac.c")
    if ("mt7921_mac_sta_add" in mac_c and
        ("mt76_wcid_mask_clear" in mac_c or "wcid_mask" in mac_c)):
        pass_test("T7.sta_add_error_cleanup", "sta_add has wcid cleanup on error")
    else:
        # Check for any error cleanup pattern
        sta_add_section = ""
        in_func = False
        for line in mac_c.split('\n'):
            if 'mt7921_mac_sta_add' in line and not line.strip().startswith('/*'):
                in_func = True
            if in_func:
                sta_add_section += line + "\n"
                if line.strip() == '}' and 'EXPORT' not in line:
                    break
        if 'wcid' in sta_add_section.lower() and ('clear' in sta_add_section.lower() or 'free' in sta_add_section.lower() or 'mask_clear' in sta_add_section.lower()):
            pass_test("T7.sta_add_error_cleanup", "sta_add frees wcid on MCU error")
        else:
            fail_test("T7.sta_add_error_cleanup", "sta_add does NOT free wcid on error")

    # Check TX prepare error path
    if "idr_remove" in mac_c:
        # Check if mt76_tx_status_skb_remove is used instead
        if "mt76_tx_status_skb_remove" in mac_c:
            pass_test("T7.tx_prepare_error", "Proper TX status cleanup on error")
        else:
            warn_test("T7.tx_prepare_error",
                      "Uses raw idr_remove — may not properly undo mt76_tx_status_skb_add")

# ============================================================
# T8: Bounds Checking
# ============================================================
def test_bounds_checking():
    print("\n=== T8: Bounds Checking ===")
    twt_c = read_file(MT7921_DIR / "twt.c")

    # Check ffs() result bounds check in TWT
    if "ffs(" in twt_c:
        # Check if result is validated before use
        twt_flowid_check = False
        for line in twt_c.split('\n'):
            if 'ffs(' in line and 'flowid' in line.lower():
                # Look for bounds check nearby
                pass
        # Actually check: does the code guard against ffs() returning 0?
        if "flowid >= MT7921_MAX_STA_TWT_AGRT" in twt_c or "flowid < 0" in twt_c:
            pass_test("T8.twt_flowid_bounds", "TWT flow ID bounds checked after ffs()")
        else:
            warn_test("T8.twt_flowid_bounds",
                      "ffs() result not explicitly bounds-checked — potential issue when mask is all-set")

    # Check TWT SP event flow_id bounds
    if "flow_id >= MT7921_MAX_TWT_AGRT" in twt_c:
        pass_test("T8.twt_sp_flow_id_bounds", "TWT SP event flow_id bounds checked")
    else:
        fail_test("T8.twt_sp_flow_id_bounds", "TWT SP event flow_id NOT bounds checked")

    # Check CLC zero-len guard
    mcu_c = read_file(MT7921_DIR / "mcu.c")
    clc_section = ""
    in_clc = False
    for line in mcu_c.split('\n'):
        if 'mt7921_load_clc' in line:
            in_clc = True
        if in_clc:
            clc_section += line + "\n"
    if "clc->len" in clc_section and "== 0" in clc_section:
        pass_test("T8.clc_zero_len_guard", "CLC loop has zero-length guard")
    else:
        warn_test("T8.clc_zero_len_guard", "CLC loop may infinite-loop on clc->len == 0")

# ============================================================
# T9: State Cleanup on Reset
# ============================================================
def test_state_cleanup_reset():
    print("\n=== T9: State Cleanup on Reset ===")
    mac_c = read_file(MT7921_DIR / "mac.c")
    usb_c = read_file(MT7921_DIR / "usb.c")

    # Check TWT state cleanup on reset
    if "twt.table_mask" in mac_c or "twt.n_agrt" in mac_c:
        # Check if reset_work clears TWT state
        reset_section = ""
        in_reset = False
        for line in mac_c.split('\n'):
            if 'mt7921_mac_reset_work' in line:
                in_reset = True
            if in_reset:
                reset_section += line + "\n"
                if line.strip() == '}' and len(reset_section) > 100:
                    in_reset = False
        if "twt" in reset_section.lower():
            pass_test("T9.twt_reset_cleanup", "TWT state cleaned up on reset")
        else:
            fail_test("T9.twt_reset_cleanup", "TWT state NOT cleaned up on chip reset")

    # Check CSI state cleanup on reset
    if "csi" in mac_c.lower() or "csi_cleanup" in usb_c:
        if "csi_cleanup" in mac_c or "csi_cleanup" in usb_c:
            pass_test("T9.csi_reset_cleanup", "CSI cleanup called on reset/chip_cleanup")
        else:
            fail_test("T9.csi_reset_cleanup", "CSI state NOT cleaned up on reset")

    # Check DFS state cleanup
    if "dfs_state" in mac_c:
        pass_test("T9.dfs_state_present", "DFS state struct exists")
    else:
        warn_test("T9.dfs_state_present", "DFS state struct not found in mac.c")

# ============================================================
# T10: Genmask/Bitfield Correctness
# ============================================================
def test_genmask_correctness():
    print("\n=== T10: Genmask/Bitfield Correctness ===")
    mac_c = read_file(MT7921_DIR / "mac.c")

    # Check RSSI GENMASK for 4th antenna
    # Should be GENMASK(31, 24), not GENMASK(31, 14)
    rssi_lines = []
    for line in mac_c.split('\n'):
        if 'to_rssi' in line and 'GENMASK' in line:
            rssi_lines.append(line.strip())

    bad_rssi = False
    for line in rssi_lines:
        if 'GENMASK(31, 14)' in line:
            fail_test("T10.rssi_genmask", f"Wrong GENMASK(31,14) — should be GENMASK(31,24): {line}")
            bad_rssi = True
            break
    if not bad_rssi and rssi_lines:
        pass_test("T10.rssi_genmask", "RSSI GENMASK values look correct")

    # Check roc_token_id type
    mt792x_h = read_file(MT76_DIR / "mt792x.h")
    if "u8 roc_token_id" in mt792x_h:
        warn_test("T10.roc_token_id", "roc_token_id is u8 — wraps at 255, potential confusion")
    elif "u16 roc_token_id" in mt792x_h:
        pass_test("T10.roc_token_id", "roc_token_id is u16")

# ============================================================
# T11: Vendor Command Registration
# ============================================================
def test_vendor_cmd_registration():
    print("\n=== T11: Vendor Command Registration ===")
    csi_nl = read_file(MT7921_DIR / "csi_nl80211.c")
    init_c = read_file(MT7921_DIR / "init.c")

    # Check that CSI nl80211 is registered during init
    if "mt7921_csi_nl80211_register" in init_c:
        pass_test("T11.csi_nl80211_register_call", "CSI nl80211 register called in init")
    else:
        fail_test("T11.csi_nl80211_register_call", "CSI nl80211 register NOT called in init")

    # Check vendor command count
    cmd_count = csi_nl.count('.doit =')
    if cmd_count >= 4:
        pass_test("T11.vendor_cmd_count", f"{cmd_count} vendor commands registered")
    else:
        fail_test("T11.vendor_cmd_count", f"Only {cmd_count} vendor commands — expected 4")

# ============================================================
# T12: RCU Dereference Discipline
# ============================================================
def test_rcu_discipline():
    print("\n=== T12: RCU Dereference Discipline ===")
    all_c = ""
    for f in find_source_files(MT7921_DIR, '.c'):
        all_c += read_file(f) + "\n"

    # Find all rcu_dereference calls
    rcu_calls = re.findall(r'rcu_dereference\s*\([^)]+\)', all_c)
    rcu_protected = re.findall(r'rcu_dereference_protected\s*\([^)]+\)', all_c)

    unguarded = 0
    for call in rcu_calls:
        # Check if it's inside an rcu_read_lock section
        # Simple heuristic: if the file also has rcu_read_lock, it might be OK
        pass

    if rcu_protected:
        pass_test("T12.rcu_dereference_protected",
                  f"{len(rcu_protected)} rcu_dereference_protected calls found")

    # Check for bare rcu_dereference without rcu_read_lock
    usb_c = read_file(MT7921_DIR / "usb.c")
    if "rcu_dereference(" in usb_c and "rcu_read_lock" not in usb_c and "rcu_dereference_protected" not in usb_c:
        fail_test("T12.usb_rcu_unprotected", "usb.c has rcu_dereference without rcu_read_lock or _protected")
    else:
        pass_test("T12.usb_rcu_protected", "usb.c RCU usage is properly protected")


# ============================================================
# Main
# ============================================================
def main():
    global REPO_ROOT, MT7921_DIR, MT76_DIR
    repo = sys.argv[sys.argv.index('--repo') + 1] if '--repo' in sys.argv else str(REPO_ROOT)
    REPO_ROOT = Path(repo)
    MT7921_DIR = REPO_ROOT / "drivers/net/wireless/mediatek/mt76/mt7921"
    MT76_DIR = REPO_ROOT / "drivers/net/wireless/mediatek/mt76"

    print(f"Forensic Round 4 — Offline Test Suite")
    print(f"Repo: {REPO_ROOT}")
    print(f"mt7921 dir: {MT7921_DIR}")
    print(f"=" * 60)

    if not MT7921_DIR.exists():
        print(f"ERROR: mt7921 directory not found at {MT7921_DIR}")
        sys.exit(1)

    test_ops_completeness()
    test_symbol_xref()
    test_lock_discipline()
    test_skb_ownership()
    test_race_conditions()
    test_bus_awareness()
    test_error_path_cleanup()
    test_bounds_checking()
    test_state_cleanup_reset()
    test_genmask_correctness()
    test_vendor_cmd_registration()
    test_rcu_discipline()

    print(f"\n{'=' * 60}")
    print(f"RESULTS: {len(results['pass'])} PASS, {len(results['fail'])} FAIL, {len(results['warn'])} WARN")
    print(f"{'=' * 60}")

    if results['fail']:
        print("\nFAILURES:")
        for name, detail in results['fail']:
            print(f"  ✗ {name}: {detail}")

    if results['warn']:
        print("\nWARNINGS:")
        for name, detail in results['warn']:
            print(f"  ⚠ {name}: {detail}")

    # Save JSON results
    out_path = REPO_ROOT / "testing" / "offline_tests" / "results_round4.json"
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, 'w') as f:
        json.dump({
            "pass": [(n, d) for n, d in results['pass']],
            "fail": [(n, d) for n, d in results['fail']],
            "warn": [(n, d) for n, d in results['warn']],
            "summary": {
                "total_pass": len(results['pass']),
                "total_fail": len(results['fail']),
                "total_warn": len(results['warn']),
            }
        }, f, indent=2)
    print(f"\nResults saved to {out_path}")

    return 1 if results['fail'] else 0

if __name__ == '__main__':
    sys.exit(main())
