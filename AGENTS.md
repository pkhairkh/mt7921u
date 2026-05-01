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
| **Capabilities** | Call-chain tracing, cross-variant comparison (PCIe/SDIO/USB), vendor driver diffing, firmware binary analysis, lock hierarchy verification, error-path auditing |
| **Scope** | `drivers/net/wireless/mediatek/mt76/mt7921/`, shared mt76 layer, vendor driver reference |
| **Output** | ISSUES.md entries with evidence classification (Proven/Probable/Speculative/Disproved), predicted crash signatures, minimal reproducers |
| **Constraints** | Cannot execute code on hardware; all findings are source-level proofs. Must tag each finding with evidence class. Must trace call chains with exact file:line references. |
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

---

### AGENT-02: `patch-engineer`

| Attribute | Value |
|-----------|-------|
| **Role** | Production-quality kernel patch developer |
| **Capabilities** | Bus-aware patch design (USB/PCIe/SDIO), lockdep-compliant locking, error-path correctness, commit message authoring, checkpatch.pl compliance |
| **Scope** | All source files under mt76 tree; shared code modifications must preserve bus compatibility |
| **Output** | Patches in `patches/` directory, formatted for `git format-patch` / `git send-email` workflow |
| **Constraints** | Must NOT break PCIe or SDIO variants. Must use `mt76_is_usb()` / `mt76_is_mmio()` / `mt76_is_sdio()` for bus branching. Must handle error paths (especially `ieee80211_wake_queues()` on failure). Must use per-device state, not global atomics. Must pass `checkpatch.pl --strict`. |
| **Handoff** | Receives findings from `code-auditor` → produces patches → feeds into `test-planner` for verification |

**Patch quality gates:**
1. Bus-aware: Does the patch use `mt76_is_usb()` where needed?
2. Error-path complete: Does every `ieee80211_stop_queues()` have a matching `ieee80211_wake_queues()` on ALL exit paths?
3. Per-device state: No global atomics — use `struct mt792x_dev` fields
4. Shared-code safe: Does the change affect PCIe/SDIO? If so, is it guarded?
5. checkpatch.pl clean: `scripts/checkpatch.pl --strict --no-tree <patch>`
6. Commit message: One-line summary < 72 chars, body with `Fixes:` tag, `Cc: stable@vger.kernel.org` if applicable

---

### AGENT-03: `test-planner`

| Attribute | Value |
|-----------|-------|
| **Role** | Runtime verification test designer and script author |
| **Capabilities** | Kernel test harness design, usbmon capture configuration, lockdep/DEBUG_ATOMIC_SLEEP setup, ftrace/trace-cmd profiling, iperf3 benchmark design, QEMU USB passthrough testing |
| **Scope** | Test scripts in `scripts/`, test documentation in `docs/testing/` |
| **Output** | Executable test scripts, expected crash signatures, measurement procedures |
| **Constraints** | Tests must be runnable without specialized hardware where possible (QEMU fallback). Hardware-specific tests must document exact requirements. |
| **Handoff** | Receives predicted crash signatures from `code-auditor` → designs tests → feeds results back to ISSUES.md (promoting findings from Probable to Proven or Disproved) |

**Test categories:**
| Category | Environment | Evidence Type |
|----------|-------------|---------------|
| Static analysis | CI (no hardware) | Source-level proof |
| QEMU USB passthrough | CI + QEMU | Controlled runtime |
| Hardware reproduce | Physical MT7921U | Confirmed runtime |
| USB protocol analyzer | Beagle USB 5000 | Wire-level evidence |
| Performance benchmark | iperf3 + ethtool | Quantitative data |

---

### AGENT-04: `firmware-analyst`

| Attribute | Value |
|-----------|-------|
| **Role** | Firmware binary reverse-engineering and protocol analysis |
| **Capabilities** | Firmware binary format parsing, patch command table extraction, capability bitmap decoding, CLC/TWT/BT-coex command structure analysis |
| **Scope** | Firmware blobs in `/lib/firmware/mediatek/`, vendor driver command definitions |
| **Output** | `docs/firmware/` — capability maps, command tables, data structures |
| **Constraints** | Must document firmware capability vs driver implementation gaps. Must NOT attempt firmware modification. Must cite vendor driver file:line for each firmware command reference. |
| **Handoff** | Feeds capability data to `code-auditor` for gap analysis; feeds to `patch-engineer` for feature implementation |

---

### AGENT-05: `research-scout`

| Attribute | Value |
|-----------|-------|
| **Role** | arXiv/academic literature researcher for enhancement proposals |
| **Capabilities** | Academic paper search and summarization, algorithm extraction, feasibility assessment for kernel integration, backward-compatibility analysis |
| **Scope** | arXiv, IEEE, ACM publications from 2024-2026 on Wi-Fi 6/6E/7, eBPF networking, AF_XDP, TWT, OFDMA, CSI, PTP |
| **Output** | Enhancement proposals in ISSUES.md with `TYPE: ENHANCEMENT` tag, research citations, implementation sketches |
| **Constraints** | Must verify backward compatibility with mac80211. Must assess kernel integration complexity. Must cite specific papers with arXiv IDs. |
| **Handoff** | Produces enhancement proposals → `patch-engineer` implements; `test-planner` validates |

---

### AGENT-06: `release-coordinator`

| Attribute | Value |
|-----------|-------|
| **Role** | Patch series organization, upstream submission, and release management |
| **Capabilities** | Patch series ordering, `git format-patch` / `git send-email`, LKML submission, stable backport tagging, CI pipeline management |
| **Scope** | `patches/` directory, `.github/workflows/`, submission emails |
| **Output** | Ordered patch series with cover letters, CI pipeline configurations |
| **Constraints** | Must follow kernel submission conventions. Patch series must be bisectable. Must include `Fixes:` tags and Cc maintainers. |
| **Handoff** | Receives validated patches from `test-planner` → organizes into series → submits upstream |

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

---

## 3. Agent Assignment Matrix

| TASKS.md Priority | Primary Agent | Supporting Agents |
|-------------------|---------------|-------------------|
| P0 (Bug fixes) | `patch-engineer` | `code-auditor`, `test-planner` |
| P0 (CLC/6 GHz) | `patch-engineer` | `firmware-analyst`, `test-planner` |
| P1 (TWT, CSI) | `patch-engineer` | `research-scout`, `firmware-analyst` |
| P2 (ACS, AF_XDP) | `patch-engineer` | `research-scout` |
| P3 (OFDMA, PTP) | `patch-engineer` | `research-scout` |
| Audit passes | `code-auditor` | `firmware-analyst` |
| Test execution | `test-planner` | All |
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
