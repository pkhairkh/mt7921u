# AGENTS.md — Agentic AI Infrastructure for mt7921u Driver Project

> This document defines the autonomous AI agents, their roles, capabilities, coordination
> protocols, and handoff procedures for the mt7921u mainline driver audit and enhancement
> project. Each agent is a self-contained unit of work that can be assigned tasks from
> TASKS.md and references issues from ISSUES.md.

---

## 1. Agent Registry

### AGENT-01: `code-auditor`

| Attribute | Value |
|-----------|-------|
| **Role** | Source-level forensic code auditor |
| **Capabilities** | Call-chain tracing, cross-variant comparison (PCIe/SDIO/USB), vendor driver diffing, firmware binary analysis, lock hierarchy verification, error-path auditing, mac80211/driver state boundary analysis |
| **Scope** | `drivers/net/wireless/mediatek/mt76/mt7921/`, shared mt76 layer, vendor driver reference |
| **Output** | ISSUES.md entries with evidence classification (Proven/Probable/Speculative/Disproved), predicted crash signatures, minimal reproducers |
| **Constraints** | Cannot execute code on hardware; all findings are source-level proofs. Must tag each finding with evidence class. Must trace call chains with exact file:line references. Must verify assumptions about mac80211 cleanup behavior by source-level tracing (see BUG-06 reinstatement lesson). |
| **Handoff** | Produces findings → feeds into `patch-engineer` for fix development; feeds into `test-planner` for runtime verification design |

**Activation triggers:**
- New kernel version merges affecting mt7921/mt792x/mt76
- New vendor driver release with divergent behavior
- User-reported crash with stack trace matching predicted signature
- Scheduled quarterly audit

**Coordination protocol:**
1. Read current ISSUES.md to avoid duplicate findings
2. Trace call chains using cscope/ctags or grep-based analysis
3. Compare mainline vs vendor vs sibling (PCIe/SDIO) implementations
4. Classify findings using four-tier evidence system
5. Write findings to ISSUES.md with `EVIDENCE:` tag
6. Notify `patch-engineer` via TASKS.md entry creation

**Key lesson from BUG-06 reinstatement:** The boundary between mac80211-managed state and driver-managed state is not always clearly delineated. Assumptions about cleanup behavior (e.g., "mac80211 will cancel the timer") must be verified by tracing the actual code path, not by reasoning about the intended design. When analyzing disconnect/cleanup paths, always verify whether driver-managed timers, work items, and delayed work are explicitly cancelled.

---

### AGENT-02: `patch-engineer`

| Attribute | Value |
|-----------|-------|
| **Role** | Production-quality kernel patch developer |
| **Capabilities** | Bus-aware patch design (USB/PCIe/SDIO), lockdep-compliant locking, error-path correctness, commit message authoring, checkpatch.pl compliance, per-chunk firmware download ACK, multi-endpoint USB QoS |
| **Scope** | All source files under mt76 tree; shared code modifications must preserve bus compatibility |
| **Output** | Patches in `patches/` directory, formatted for `git format-patch` / `git send-email` workflow |
| **Constraints** | Must NOT break PCIe or SDIO variants. Must use `mt76_is_usb()` / `mt76_is_mmio()` / `mt76_is_sdio()` for bus branching. Must handle error paths (especially `ieee80211_wake_queues()` on failure). Must use per-device state, not global atomics. Must pass `checkpatch.pl --strict`. Must provide production-quality patches with actual diff content (not just descriptions). |
| **Handoff** | Receives findings from `code-auditor` → produces patches → feeds into `test-planner` for verification |

**Patch quality gates:**
1. Bus-aware: Does the patch use `mt76_is_usb()` where needed?
2. Error-path complete: Does every `ieee80211_stop_queues()` have a matching `ieee80211_wake_queues()` on ALL exit paths?
3. Per-device state: No global atomics — use `struct mt792x_dev` fields
4. Shared-code safe: Does the change affect PCIe/SDIO? If so, is it guarded?
5. checkpatch.pl clean: `scripts/checkpatch.pl --strict --no-tree <patch>`
6. Commit message: One-line summary < 72 chars, body with `Fixes:` tag, `Cc: stable@vger.kernel.org` if applicable
7. Firmware download: Consider per-chunk ACK for USB reliability (see FEATURE-06)

---

### AGENT-03: `test-planner`

| Attribute | Value |
|-----------|-------|
| **Role** | Runtime verification test designer and script author |
| **Capabilities** | Kernel test harness design, usbmon capture configuration, lockdep/DEBUG_ATOMIC_SLEEP setup, ftrace/trace-cmd profiling, iperf3 benchmark design, QEMU USB passthrough testing, spectrum analyzer/SDR regulatory compliance measurement, BT coexistence throughput assessment |
| **Scope** | Test scripts in `scripts/`, test documentation in `docs/testing/` |
| **Output** | Executable test scripts, expected crash signatures, measurement procedures |
| **Constraints** | Tests must be runnable without specialized hardware where possible (QEMU fallback). Hardware-specific tests must document exact requirements. Regulatory compliance tests (TEST-T7) require RF measurement equipment and shielded lab. BT coexistence tests (TEST-T6) require combo dongle. |
| **Handoff** | Receives predicted crash signatures from `code-auditor` → designs tests → feeds results back to ISSUES.md (promoting findings from Probable to Proven or Disproved) |

**Test categories:**
| Category | Environment | Evidence Type | Test IDs |
|----------|-------------|---------------|----------|
| Static analysis | CI (no hardware) | Source-level proof | — |
| QEMU USB passthrough | CI + QEMU | Controlled runtime | — |
| Hardware reproduce | Physical MT7921U | Confirmed runtime | TEST-T1, T2, T3, T4, T5 |
| USB protocol analyzer | Beagle USB 5000 | Wire-level evidence | TEST-T2, T3 |
| Performance benchmark | iperf3 + ethtool | Quantitative data | TEST-T6 |
| Regulatory compliance | Spectrum analyzer / SDR | RF measurement | TEST-T7 |

**Evidence promotion rules (applied by test-planner):**
| Transition | Required Evidence | Authorized By |
|------------|-------------------|---------------|
| Speculative → Probable | Code pattern match + predicted signature | `code-auditor` |
| Probable → Proven | Captured Oops/lockdep/trace matching prediction | `test-planner` |
| Proven → Disproved | Runtime test shows no crash, or code path unreachable | `test-planner` |
| Any → Disproved | Formal proof of incorrectness | `code-auditor` |

---

### AGENT-04: `firmware-analyst`

| Attribute | Value |
|-----------|-------|
| **Role** | Firmware binary reverse-engineering and protocol analysis |
| **Capabilities** | Firmware binary format parsing, patch command table extraction, capability bitmap decoding, CLC/TWT/BT-coex command structure analysis, SET_CLC USB viability testing, CSI command identification |
| **Scope** | Firmware blobs in `/lib/firmware/mediatek/`, vendor driver command definitions |
| **Output** | `docs/firmware/` — capability maps, command tables, data structures, USB CLC viability report |
| **Constraints** | Must document firmware capability vs driver implementation gaps. Must NOT attempt firmware modification. Must cite vendor driver file:line for each firmware command reference. Must test SET_CLC viability over USB before alternative path is implemented. |
| **Handoff** | Feeds capability data to `code-auditor` for gap analysis; feeds to `patch-engineer` for feature implementation; feeds CLC viability results to TASK-014 |

**Critical deliverables:**
- Firmware CLC command viability over USB (TASK-014): Does `MCU_CE_CMD(SET_CLC)` work over USB bulk transport, or must the vendor driver's alternative path (`CMD_ID_CAL_BACKUP_IN_HOST_V2` + `rlmDomainGetChnlList()`) be implemented?
- Firmware radar detection capability (TASK-013): Does `EXT_CMD_ID_RADAR_DETECT` exist and function on MT7921U firmware?
- Firmware CSI extraction command (TASK-008): What firmware command captures CSI from the RX descriptor?
- Firmware TWT parameter support (TASK-007): Which TWT parameters from vendor driver are actually supported by firmware?
- Firmware coexistence capability (TASK-016): Does `MT_NIC_CAP_COEX` report coexistence capability? How does firmware handle BT/WiFi arbitration?

---

### AGENT-05: `research-scout`

| Attribute | Value |
|-----------|-------|
| **Role** | arXiv/academic literature researcher for enhancement proposals |
| **Capabilities** | Academic paper search and summarization, algorithm extraction, feasibility assessment for kernel integration, backward-compatibility analysis, implementation sketch design |
| **Scope** | arXiv, IEEE, ACM publications from 2024-2026 on Wi-Fi 6/6E/7, eBPF networking, AF_XDP, TWT, OFDMA, CSI, PTP |
| **Output** | Enhancement proposals in ISSUES.md with `TYPE: ENHANCEMENT` tag, research citations, implementation sketches with phased rollout |
| **Constraints** | Must verify backward compatibility with mac80211. Must assess kernel integration complexity. Must cite specific papers with arXiv IDs. Must provide phased implementation sketches (Phase 1, 2, 3, 4). Must document backward compatibility guarantees for each enhancement. |
| **Handoff** | Produces enhancement proposals → `patch-engineer` implements; `test-planner` validates |

**Published research references:**
| Enhancement | Paper | Key Result |
|-------------|-------|------------|
| ENHANCE-01 | arXiv 2025 — RL/multi-armed bandit for Wi-Fi channel selection | 15-30% throughput improvement in dense environments |
| ENHANCE-02 | "Deterministic Scheduling over Wi-Fi 6 using TWT" (arXiv, May 2025) | 60% latency reduction, 80% jitter reduction, 40% power savings |
| ENHANCE-03 | "Enhancing CSI-Based Wireless Sensing With Open-Source Linux 802.11ax CSI Tool" (IEEE, May 2025) | WhoFi: 95.5% presence detection accuracy |
| ENHANCE-04 | AF_XDP kernel technology for zero-copy packet processing | Line-rate processing with sub-microsecond per-packet latency |
| ENHANCE-05 | "Wi-Fi 6 CTI Detection and Mitigation by OFDMA" (arXiv, March 2025) | 35% throughput recovery from CTI |
| ENHANCE-06 | PTP (IEEE 1588) hardware timestamping research | Sub-microsecond synchronization accuracy |

---

### AGENT-06: `release-coordinator`

| Attribute | Value |
|-----------|-------|
| **Role** | Patch series organization, upstream submission, and release management |
| **Capabilities** | Patch series ordering, `git format-patch` / `git send-email`, LKML submission, stable backport tagging, CI pipeline management |
| **Scope** | `patches/` directory, `.github/workflows/`, submission emails |
| **Output** | Ordered patch series with cover letters, CI pipeline configurations |
| **Constraints** | Must follow kernel submission conventions. Patch series must be bisectable. Must include `Fixes:` tags and Cc maintainers. P0 patches must be submitted first as urgent fixes. |
| **Handoff** | Receives validated patches from `test-planner` → organizes into series → submits upstream |

**Patch series ordering:**
1. BUG-02 fix (testmode NULL deref) — standalone, no dependencies
2. BUG-03 fix (WTBL poll timeout) — standalone
3. BUG-04 fix (MCU retry) — standalone, but prerequisite for BUG-01
4. BUG-06 fix (ROC timer UAF) — standalone
5. BUG-05 fix (queue wake) — standalone
6. BUG-01 fix (CLC enable) — depends on BUG-04 fix
7. Feature patches (after bug fixes are merged)

---

## 2. Coordination Protocol

### 2.1 Task Lifecycle

```
ISSUES.md discovery → TASKS.md creation → Agent assignment →
Execution → Review → Merge to patches/ → Validation → Submission
```

### 2.2 Communication Channels

All inter-agent communication happens through three files:

| File | Purpose | Write Access |
|------|---------|-------------|
| `ISSUES.md` | Single source of truth for all findings | All agents (append-only, no deletion) |
| `TASKS.md` | Work items with status, dependencies, assignments | All agents (status updates allowed) |
| `AGENTS.md` | This file — agent definitions and protocols | `release-coordinator` only |

### 2.3 Evidence Promotion Rules

Findings in ISSUES.md can be promoted or demoted based on runtime evidence:

| Transition | Required Evidence | Authorized By |
|------------|-------------------|---------------|
| Speculative → Probable | Code pattern match + predicted signature | `code-auditor` |
| Probable → Proven | Captured Oops/lockdep/trace matching prediction | `test-planner` |
| Proven → Disproved | Runtime test shows no crash, or code path unreachable | `test-planner` |
| Any → Disproved | Formal proof of incorrectness | `code-auditor` |

### 2.4 Conflict Resolution

When two agents disagree on a finding:
1. Both present evidence in ISSUES.md under the same issue ID
2. `code-auditor` performs independent verification
3. If still unresolved, mark as `CONTESTED` and defer to human review

### 2.5 Key Analytical Principles

1. **Verify mac80211 cleanup assumptions:** Never assume mac80211-managed state cleanup also handles driver-managed state. Always trace the actual code path. (Lesson from BUG-06 reinstatement — see Part III, Section 3.4 of coroner's report.)

2. **Firmware CLC viability must be tested:** The vendor driver uses an entirely different regulatory path for USB. Do not assume `MCU_CE_CMD(SET_CLC)` works over USB without testing. (See Part III, Section 3.2.)

3. **BT coexistence severity is hardware-dependent:** The severity of missing host-side coexistence hints depends on whether the firmware handles coexistence autonomously and how well. Must be measured on actual hardware. (See Part III, Section 3.3.)

4. **Regulatory compliance is not just a feature gap:** The CLC skip creates real legal risk beyond just missing 6 GHz. 2.4 GHz and 5 GHz bands operate without country-specific power limits. (See Part III, Section 3.1.)

---

## 3. Agent Assignment Matrix

| TASKS.md Priority | Primary Agent | Supporting Agents |
|-------------------|---------------|-------------------|
| P0 Bug fixes (BUG-01 to BUG-06) | `patch-engineer` | `code-auditor`, `test-planner` |
| P0 CLC/6 GHz (TASK-005) | `patch-engineer` | `firmware-analyst`, `test-planner` |
| P0 Firmware CLC viability (TASK-014) | `firmware-analyst` | `test-planner` |
| P0 Regulatory compliance (TASK-015) | `test-planner` | `firmware-analyst` |
| P0 BT coexistence assessment (TASK-016) | `test-planner` | `firmware-analyst` |
| P0 DFS master (TASK-013) | `patch-engineer` | `firmware-analyst` |
| P0 Multi-endpoint TX QoS (TASK-017) | `patch-engineer` | — |
| P0 Per-chunk ACK (TASK-018) | `patch-engineer` | — |
| P1 TWT (TASK-007) | `patch-engineer` | `research-scout`, `firmware-analyst` |
| P1 CSI (TASK-008) | `patch-engineer` | `research-scout`, `firmware-analyst` |
| P2 ACS (TASK-009) | `patch-engineer` | `research-scout` |
| P2 AF_XDP (TASK-010) | `patch-engineer` | — |
| P3 OFDMA (TASK-011) | `patch-engineer` | `research-scout` |
| P3 PTP (TASK-012) | `patch-engineer` | — |
| Test execution (TASK-T1 to TASK-T7) | `test-planner` | All |
| Audit passes | `code-auditor` | `firmware-analyst` |
| Release prep | `release-coordinator` | All |

---

## 4. Git Workflow

```
main (stable, reviewed patches only)
 ├── audit/YYYY-MM-DD  (code-auditor findings branch)
 ├── fix/BUG-XX         (patch-engineer fix branch)
 ├── feature/ENHANCE-XX (feature implementation branch)
 └── test/TEST-XX       (test-planner verification branch)
```

**Branch naming:** `<type>/<ID>-<short-description>`
**Commit message format:**
```
<subsystem>: <one-line summary>

<body with Fixes: tag>
```

**Merge requirements:**
- At least 1 review from a different agent type
- checkpatch.pl clean
- No regression on other bus types
- Test plan documented for hardware verification

---

## 5. CI Pipeline (`.github/workflows/`)

| Workflow | Trigger | Checks |
|----------|---------|--------|
| `checkpatch.yml` | Push/PR to any branch | `scripts/checkpatch.pl --strict --no-tree` on all patches |
| `build.yml` | Push/PR to main | Cross-compile for x86_64, arm64 with `CONFIG_MT7921U=m` |
| `sparse.yml` | Push/PR to main | `make C=2` sparse static analysis |
| `smatch.yml` | Weekly scheduled | Smatch database check for new warnings |
