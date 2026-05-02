#!/bin/bash
# test_harness.sh — Master test harness for mt7921u runtime verification
#
# Orchestrates all runtime verification tests (T1–T7) for the mt7921u
# driver.  Each test exercises a specific code path and verifies that
# the corresponding bug fix is effective.
#
# Tests:
#   T1: Testmode NULL deref verification (BUG-01, patch 0001)
#   T2: WTBL poll timeout verification (BUG-02, patch 0002)
#   T3: MCU command retry verification (BUG-03, patch 0003)
#   T4: CLC regulatory verification (patches 0006, 0007)
#   T5: Suspend/resume verification (BUG-04, patch 0004)
#   T6: Queue wake on reset verification (patch 0005)
#   T7: TWT functionality verification (patch 0008)
#
# Usage:
#   ./test_harness.sh --test all --interface phy0 --log-dir /tmp/mt7921u_test
#   ./test_harness.sh --test T1 --interface phy0
#   ./test_harness.sh --test T2,T3,T4
#
# Safety: Must be run as root on a system with the MT7921U device present.
#         DO NOT run on production systems.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Defaults
TESTS="all"
INTERFACE="phy0"
LOG_DIR="/tmp/mt7921u_test"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Available tests
ALL_TESTS="T1 T2 T3 T4 T5 T6 T7"

# Test descriptions
declare -A TEST_DESC
TEST_DESC[T1]="Testmode NULL deref verification (BUG-01)"
TEST_DESC[T2]="WTBL poll timeout verification (BUG-02)"
TEST_DESC[T3]="MCU command retry verification (BUG-03)"
TEST_DESC[T4]="CLC regulatory verification"
TEST_DESC[T5]="Suspend/resume verification (BUG-04)"
TEST_DESC[T6]="Queue wake on reset verification"
TEST_DESC[T7]="TWT functionality verification"

# Test script mapping
declare -A TEST_SCRIPT
TEST_SCRIPT[T1]="test_t1_testmode.sh"
TEST_SCRIPT[T2]="test_t2_wtbl.sh"
TEST_SCRIPT[T3]="test_t3_mcu_retry.sh"
TEST_SCRIPT[T4]="test_t4_clc.sh"
TEST_SCRIPT[T5]="test_t5_suspend.sh"
TEST_SCRIPT[T6]="test_t6_reset.sh"
TEST_SCRIPT[T7]="test_t7_twt.sh"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --test)
            TESTS="$2"
            shift 2
            ;;
        --interface)
            INTERFACE="$2"
            shift 2
            ;;
        --log-dir)
            LOG_DIR="$2"
            shift 2
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --test T1|T2|...|T7|all   Run specific test(s), comma-separated or 'all'"
            echo "  --interface phyX          Wireless PHY interface (default: phy0)"
            echo "  --log-dir DIR             Directory for log files (default: /tmp/mt7921u_test)"
            echo "  --help                    Show this help"
            echo ""
            echo "Available tests:"
            for t in $ALL_TESTS; do
                echo "  $t: ${TEST_DESC[$t]}"
            done
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Determine which tests to run
if [[ "$TESTS" == "all" ]]; then
    RUN_TESTS="$ALL_TESTS"
else
    # Parse comma-separated list
    RUN_TESTS=$(echo "$TESTS" | tr ',' ' ')
fi

# Create log directory
mkdir -p "$LOG_DIR"

# Master log
MASTER_LOG="${LOG_DIR}/test_harness_${TIMESTAMP}.log"

echo "============================================================" | tee "$MASTER_LOG"
echo "mt7921u Runtime Verification Test Harness" | tee -a "$MASTER_LOG"
echo "Timestamp:  $(date)" | tee -a "$MASTER_LOG"
echo "Interface:  $INTERFACE" | tee -a "$MASTER_LOG"
echo "Log dir:    $LOG_DIR" | tee -a "$MASTER_LOG"
echo "Tests:      $RUN_TESTS" | tee -a "$MASTER_LOG"
echo "============================================================" | tee -a "$MASTER_LOG"
echo "" | tee -a "$MASTER_LOG"

# Safety check
if [[ $EUID -ne 0 ]]; then
    echo "[FATAL] This test harness must be run as root." | tee -a "$MASTER_LOG"
    exit 1
fi

# Check device presence
DEBUGFS_PATH="/sys/kernel/debug/ieee80211/${INTERFACE}/mt76"
if [[ ! -d "$DEBUGFS_PATH" ]]; then
    echo "[FATAL] Debugfs path $DEBUGFS_PATH not found." | tee -a "$MASTER_LOG"
    echo "        Is the MT7921U device present and driver loaded?" | tee -a "$MASTER_LOG"
    exit 1
fi

# Summary tracking
declare -A TEST_RESULT
TOTAL=0
PASSED=0
FAILED=0
SKIPPED=0

# Run each test
for test_id in $RUN_TESTS; do
    # Validate test ID
    if [[ ! -v TEST_SCRIPT[$test_id] ]]; then
        echo "[ERROR] Unknown test: $test_id. Skipping." | tee -a "$MASTER_LOG"
        TEST_RESULT[$test_id]="INVALID"
        continue
    fi

    script="${SCRIPT_DIR}/${TEST_SCRIPT[$test_id]}"
    if [[ ! -f "$script" ]]; then
        echo "[ERROR] Test script not found: $script" | tee -a "$MASTER_LOG"
        TEST_RESULT[$test_id]="MISSING"
        continue
    fi

    echo "" | tee -a "$MASTER_LOG"
    echo "--- Running test $test_id: ${TEST_DESC[$test_id]} ---" | tee -a "$MASTER_LOG"
    echo "" | tee -a "$MASTER_LOG"

    TOTAL=$((TOTAL + 1))

    # Run the test script — capture output once to avoid double-execution
    output=$(bash "$script" --interface "$INTERFACE" --log-dir "$LOG_DIR" 2>&1)
    exit_code=$?

    # Append captured output to master log
    echo "$output" >> "$MASTER_LOG"
    echo "$output"

    # Determine result from exit code and output content
    LAST_LINES=$(echo "$output" | tail -5)
    if [[ $exit_code -eq 0 ]]; then
        if echo "$LAST_LINES" | grep -q "\\[PASS\\]"; then
            TEST_RESULT[$test_id]="PASS"
            PASSED=$((PASSED + 1))
        elif echo "$LAST_LINES" | grep -q "\\[SKIP\\]"; then
            TEST_RESULT[$test_id]="SKIP"
            SKIPPED=$((SKIPPED + 1))
        else
            # Default to pass if exit code was 0
            TEST_RESULT[$test_id]="PASS"
            PASSED=$((PASSED + 1))
        fi
    else
        if echo "$LAST_LINES" | grep -q "\\[SKIP\\]"; then
            TEST_RESULT[$test_id]="SKIP"
            SKIPPED=$((SKIPPED + 1))
        else
            TEST_RESULT[$test_id]="FAIL"
            FAILED=$((FAILED + 1))
        fi
    fi

    echo "" | tee -a "$MASTER_LOG"
    echo "Test $test_id result: ${TEST_RESULT[$test_id]}" | tee -a "$MASTER_LOG"
done

# Print summary
echo "" | tee -a "$MASTER_LOG"
echo "============================================================" | tee -a "$MASTER_LOG"
echo "SUMMARY" | tee -a "$MASTER_LOG"
echo "============================================================" | tee -a "$MASTER_LOG"
echo "" | tee -a "$MASTER_LOG"

for test_id in $RUN_TESTS; do
    if [[ -v TEST_RESULT[$test_id] ]]; then
        result="${TEST_RESULT[$test_id]}"
        desc="${TEST_DESC[$test_id]}"
        printf "  %-4s %-10s %s\n" "$test_id" "[$result]" "$desc" | tee -a "$MASTER_LOG"
    fi
done

echo "" | tee -a "$MASTER_LOG"
echo "Total: $TOTAL  Passed: $PASSED  Failed: $FAILED  Skipped: $SKIPPED" | tee -a "$MASTER_LOG"
echo "" | tee -a "$MASTER_LOG"
echo "Logs saved to: $LOG_DIR" | tee -a "$MASTER_LOG"
echo "Master log:    $MASTER_LOG" | tee -a "$MASTER_LOG"

# Exit with failure if any test failed
if [[ $FAILED -gt 0 ]]; then
    exit 1
fi

exit 0
