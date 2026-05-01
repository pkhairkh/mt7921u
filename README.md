# mt7921u — Forensic Audit & Enhancement Roadmap

> Agentic AI infrastructure for auditing, fixing, and enhancing the mainline Linux
> kernel `mt7921u` USB Wi-Fi 6E driver (MediaTek MT7921U, USB ID `0e8d:7961`)

---

## What This Is

This repository contains the **forensic audit findings**, **production-quality patches**, and **research-backed enhancement proposals** for the mainline `mt7921u` driver. It is organized as an agentic AI infrastructure where autonomous agents collaborate through structured markdown files to drive the driver from its current "minimal STA-only" state toward a reference-class programmable Wi-Fi interface.

This repo contains the **actual mainline driver source** (cloned from `torvalds/linux`), the **out-of-tree vendor driver** (for comparison), and the audit/planning infrastructure. Patches produced here are intended for upstream submission to the wireless-drivers-next tree.

---

## The Driver Source

### Mainline driver (`drivers/net/wireless/mediatek/mt76/`)

This is the actual Linux kernel mt76 driver tree, including:

| Directory/File | Purpose |
|---------------|---------|
| `mt7921/usb.c` | **USB bus glue** — probe, remove, URB management, vendor requests, suspend/resume, firmware download |
| `mt7921/mac.c` | MAC layer — RX/TX processing, WTBL updates, reset |
| `mt7921/main.c` | mac80211 ops — wiphy setup, interface operations, key management |
| `mt7921/mcu.c` | MCU command interface — firmware loading, CLC skip (**BUG-01**), events |
| `mt7921/testmode.c` | Test mode — NULL deref on USB (**BUG-02**) |
| `mt7921/init.c` | Device initialization, regulatory |
| `mt7921/pci.c` | PCIe variant (for comparison) |
| `mt7921/sdio.c` | SDIO variant (for comparison) |
| `mt792x_*.c/h` | Shared core layer — USB ops, DMA, MAC work, MIB stats |
| `usb.c` | mt76 USB transport layer — URB management, vendor requests |
| `connac*.c/h` | Shared connac MCU command framework |

### Vendor driver (`vendor-driver/`)

Out-of-tree MediaTek production driver from `github.com/jeffersonchou/mt7921u`. Used as reference for:
- TWT implementation (`mgmt/twt.c`, `nic_uni_cmd_event.c`)
- BT coexistence (`OID_CUSTOM_BT_COEXIST_CTRL`, `EXT_EVENT_ID_COEXISTENCE`)
- 6 GHz regulatory path (`rlmDomainGetChnlList()`, `gl_init.c`)
- Firmware download with per-chunk ACK (`fw_dl.c`, `DOWNLOAD_CONFIG_ACK_OPTION`)

---

## The Driver Today

The mainline `mt7921u` driver is a functional USB WiFi driver for basic STATION mode on 2.4 GHz and 5 GHz. The forensic audit identified:

- **6 proven bugs** (crash, reliability, regulatory compliance)
- **6 missing features** (6 GHz, TWT, DFS, BT coex, multi-EP QoS, FW download ACK)
- **6 arXiv-backed enhancement proposals** (eBPF ACS, deterministic TWT, CSI sensing, AF_XDP, OFDMA CTI, PTP)
- **2 disproved claims** (toggle mismatch, URB race)

The driver is NOT broken — it's an honest but incomplete port that works for basic use cases but falls apart at the edges (6 GHz, power management edge cases, error recovery).

---

## Repository Structure

```
mt7921u/
├── AGENTS.md          # Agent definitions, roles, coordination protocols
├── TASKS.md           # Prioritized task breakdown with acceptance criteria
├── ISSUES.md          # All findings classified by evidence tier
├── README.md          # This file
├── .gitignore
├── drivers/
│   └── net/wireless/mediatek/mt76/   # ★ ACTUAL MAINLINE DRIVER SOURCE
│       ├── mt7921/                    #   mt7921 driver (USB/PCIe/SDIO)
│       │   ├── usb.c                  #     USB bus glue ← PRIMARY AUDIT TARGET
│       │   ├── mac.c                  #     MAC layer, WTBL, reset
│       │   ├── mcu.c                  #     MCU commands, CLC skip (BUG-01)
│       │   ├── testmode.c            #     Test mode, NULL deref (BUG-02)
│       │   ├── main.c                #     mac80211 ops
│       │   ├── init.c                #     Device init, regulatory
│       │   ├── pci.c                 #     PCIe variant (reference)
│       │   ├── sdio.c                #     SDIO variant (reference)
│       │   └── ...
│       ├── mt792x_*.c/h              #   Shared mt792x core layer
│       ├── usb.c                     #   mt76 USB transport
│       ├── connac*.c/h               #   Shared connac MCU framework
│       └── ...
├── vendor-driver/      # ★ OUT-OF-TREE MEDIATEK PRODUCTION DRIVER
│   ├── chips/common/fw_dl.c          #   Firmware download with ACK
│   ├── mgmt/twt.c                    #   TWT implementation
│   ├── mgmt/rlm_domain.c            #   Regulatory domain
│   ├── nic_cmd_event.h              #   MCU command/event definitions
│   └── ...
├── docs/
│   └── firmware/      # Firmware reverse-engineering notes
├── patches/
│   └── (empty)        # Production patches, formatted for git send-email
├── scripts/
│   └── (empty)        # Test scripts, benchmark harnesses
└── .github/
    ├── ISSUE_TEMPLATE/  # GitHub issue templates
    └── workflows/       # CI pipelines (checkpatch, build, sparse)
```

---

## Agentic AI Infrastructure

The project operates through three core files that serve as the coordination layer for autonomous AI agents:

| File | Purpose |
|------|---------|
| **AGENTS.md** | Defines 6 specialized agents (code-auditor, patch-engineer, test-planner, firmware-analyst, research-scout, release-coordinator), their capabilities, and coordination protocols |
| **TASKS.md** | 12 prioritized tasks (P0–P3) with dependencies, estimates, and acceptance criteria |
| **ISSUES.md** | 20 findings (6 bugs, 6 features, 6 enhancements, 2 disproved) with four-tier evidence classification |

### Evidence Classification

| Tier | Meaning |
|------|---------|
| **Proven** | Call chain traceable to defective code through source analysis |
| **Probable** | Likely at runtime but depends on timing/hardware/configuration |
| **Speculative** | Pattern-based inference without confirmed reachability |
| **Disproved** | Originally claimed but subsequently disproved |

### Agent Workflow

```
code-auditor discovers findings → ISSUES.md
    ↓
patch-engineer implements fixes → patches/
    ↓
test-planner validates → promotes evidence tier in ISSUES.md
    ↓
release-coordinator organizes → upstream submission
```

---

## Priority 0 — Immediate Action Items

| Task | Bug | Description | Estimate |
|------|-----|-------------|----------|
| TASK-001 | BUG-02 | Fix testmode NULL deref on USB | 2 hours |
| TASK-002 | BUG-03 | Fix WTBL poll timeout for USB | 3 hours |
| TASK-003 | BUG-04 | Add MCU command retry before chip reset | 4 hours |
| TASK-004 | BUG-06 | Fix ROC timer use-after-free on disconnect | 3 hours |
| TASK-005 | BUG-01 | Enable CLC for USB (6 GHz + regulatory fix) | 1-2 weeks |
| TASK-006 | BUG-05 | Add missing queue wake on reset error path | 2 hours |

---

## Enhancement Roadmap (arXiv-Backed)

| Priority | Enhancement | Impact |
|----------|-------------|--------|
| P1 | TWT scheduler with eBPF + deterministic latency | Power + latency game-changer |
| P1 | CSI extraction for Wi-Fi sensing | Sensing research game-changer |
| P2 | eBPF-based channel survey and ACS | AP deployment game-changer |
| P2 | AF_XDP zero-copy monitor mode | High-performance packet capture |
| P3 | OFDMA-aware MU scheduling with CTI mitigation | Production AP quality |
| P3 | Hardware timestamping for PTP | Industrial IoT enablement |

---

## Key Self-Corrections

The audit has undergone four iterations, each improving on the last:

1. **ROC timer UAF (BUG-06):** v3 incorrectly "disproved" this by assuming `ieee80211_roc_purge_local()` cancels the driver's `roc_timer`. v4 reinstated it — `purge_local()` only cancels mac80211-managed offchannel ops, not the driver-level timer if ROC has expired from mac80211's perspective.

2. **Toggle mismatch (DISPROVED-01):** Originally claimed as Major. Disproved: firmware download and MCU commands use different USB endpoints (per-endpoint toggle; USB 2.0 Spec 8.5.2).

3. **CLC regulatory risk:** v3 identified 6 GHz loss. v4 expanded: the CLC skip also leaves 2.4/5 GHz without country-specific power limits — a regulatory compliance risk, not just a feature gap.

4. **BT coexistence:** Downgraded from "zero implementation = critical" to "speculative" — the firmware likely handles basic TDM autonomously; host-side hints are an optimization.

---

## Getting Started

1. Read **AGENTS.md** to understand the agent roles
2. Read **ISSUES.md** for the current state of all findings
3. Read **TASKS.md** for prioritized work items
4. Pick a task, create a branch (`fix/BUG-XX` or `feature/ENHANCE-XX`)
5. Implement, test, submit PR

---

## Upstream Submission

Patches are submitted to the wireless-drivers-next tree:
- Maintainer: Felix Fietkau <nbd@nbd.name>, Lorenzo Bianconi <lorenzo@kernel.org>
- Mailing list: linux-wireless@vger.kernel.org
- Submission format: `git format-patch` + `git send-email`
- All patches must pass `scripts/checkpatch.pl --strict --no-tree`

---

## License

The audit and infrastructure files in this repository are provided under
CC-BY-4.0. Kernel patches are subject to the Linux kernel's GPL-2.0-only
license when submitted upstream.
