#!/bin/bash
###############################################################################
# mt7921u-safe-install.sh
#
# Safe MT7921U driver upgrade with automatic rollback.
# Designed for Raspberry Pi 5 with two MT7921U USB Wi-Fi cards (wlan1, wlan2).
# Does NOT touch any Wi-Fi configuration files.
#
# HOW IT WORKS:
#   1. Builds the new driver from the GitHub repo
#   2. Installs the new .ko modules into /lib/modules/$KVER/extra/
#   3. Unloads old driver, loads new driver (Wi-Fi goes down for ~5 seconds)
#   4. Waits 5 minutes for you to reconnect
#      - If you reconnect and run:  sudo kill <PID>   -->  INSTALL STAYS
#      - If you cannot reconnect                      -->  AUTO ROLLBACK after 5 min
#
# USAGE:
#   sudo nohup bash mt7921u-safe-install.sh > /var/log/mt7921u-install.log 2>&1 &
#   echo "PID is: $!"
#
#   Then reconnect via Wi-Fi and confirm:
#   sudo kill <PID>
#
# ROLLBACK (manual, if needed later):
#   sudo bash mt7921u-safe-install.sh --rollback
#
###############################################################################

set -uo pipefail

# ===================== CONFIGURATION =====================
KERNEL_VERSION="$(uname -r)"
REPO_URL="https://github.com/pkhairkh/mt7921u.git"
BUILD_DIR="/usr/src/mt7921u-build"
LOG_FILE="/var/log/mt7921u-install.log"
WAIT_SECONDS=300              # 5 minutes
STATE_FILE="/var/run/mt7921u-install-state"
INSTALLED_FILES="/var/run/mt7921u-installed-files.txt"
OLD_MODULES_LOADED="/var/run/mt7921u-old-modules.txt"
STUB_HEADERS_CREATED="/var/run/mt7921u-stub-headers.txt"

# The module dependency chain for MT7921U (USB)
# Load order (bottom-up dependency): mt76 -> mt76-usb -> mt76-connac-lib -> mt792x-lib -> mt792x-usb -> mt7921-common -> mt7921u
# Unload is reverse order
MODULES_UNLOAD="mt7921u mt7921_common mt792x_usb mt792x_lib mt76_connac_lib mt76_usb mt76"
MODULES_LOAD="mt76 mt76_usb mt76_connac_lib mt792x_lib mt792x_usb mt7921_common mt7921u"

# ===================== LOGGING =====================
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a "$LOG_FILE" 2>/dev/null || echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*"
}

separator() {
    log "================================================================"
}

# ===================== STUB HEADER CREATION =====================
# The mt76 driver includes <linux/soc/airoha/airoha_offload.h> and
# <linux/soc/mediatek/mtk_wed.h> which don't exist on Raspberry Pi
# kernel headers. We create minimal stub headers so the build succeeds.
# The driver source code also guards these includes with IS_ENABLED()
# checks, so these stubs are a belt-and-suspenders approach.

create_stub_headers() {
    local kbuild_dir="/lib/modules/${KERNEL_VERSION}/build"
    local stub_dir

    log "Creating stub headers for missing kernel API headers..."

    # --- Stub: linux/soc/airoha/airoha_offload.h ---
    stub_dir="${kbuild_dir}/include/linux/soc/airoha"
    if [ ! -f "${stub_dir}/airoha_offload.h" ]; then
        mkdir -p "$stub_dir"
        cat > "${stub_dir}/airoha_offload.h" << 'AIOHA_STUB_EOF'
/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Stub header for linux/soc/airoha/airoha_offload.h
 * Created by mt7921u-safe-install.sh for platforms without Airoha NPU support
 * (e.g., Raspberry Pi). The driver source guards usage with IS_ENABLED(CONFIG_MT76_NPU).
 */
#ifndef _LINUX_AIROHA_OFFLOAD_H_STUB
#define _LINUX_AIROHA_OFFLOAD_H_STUB

struct airoha_npu {
        struct device *dev;
        struct regmap *regmap;
        int irqs[2];
};

struct airoha_ppe_dev;

enum airoha_npu_wlan_set_cmd {
        WLAN_FUNC_SET_WAIT_DESC,
        WLAN_FUNC_SET_WAIT_TX_RING_PCIE_ADDR,
        WLAN_FUNC_SET_WAIT_PCIE_ADDR,
        WLAN_FUNC_SET_WAIT_TOKEN_ID_SIZE,
        WLAN_FUNC_SET_WAIT_PCIE_PORT_TYPE,
        WLAN_FUNC_SET_WAIT_TX_BUF_SPACE_HW_BASE,
        WLAN_FUNC_SET_WAIT_RX_RING_FOR_TXDONE_HW_BASE,
        WLAN_FUNC_SET_WAIT_INODE_TXRX_REG_ADDR,
};

enum airoha_npu_wlan_get_cmd {
        WLAN_FUNC_GET_WAIT_NPU_VERSION,
        WLAN_FUNC_GET_WAIT_RXDESC_BASE,
        WLAN_FUNC_GET_WAIT_NPU_INFO,
};

struct airoha_npu_tx_dma_desc {
        __le32 addr;
        __le32 ctrl;
        __le32 info;
        __le32 txwi[4];
};

struct airoha_npu_rx_dma_desc {
        __le32 addr;
        __le32 ctrl;
        __le32 info;
        __le32 rxd[4];
};

#define NPU_RX_DMA_PKT_COUNT_MASK       GENMASK(23, 16)
#define NPU_RX_DMA_DESC_CUR_LEN_MASK    GENMASK(13, 0)
#define NPU_RX_DMA_DESC_DONE_MASK       BIT(31)
#define NPU_RX_DMA_FOE_ID_MASK          GENMASK(25, 16)
#define NPU_RX_DMA_CRSN_MASK            GENMASK(7, 4)
#define NPU_TX_DMA_DESC_LEN_MASK        GENMASK(13, 0)
#define NPU_TX_DMA_DESC_VEND_LEN_MASK   GENMASK(29, 16)
#define NPU_TX_DMA_DESC_DONE_MASK       BIT(31)

#define PPE_CPU_REASON_HIT_UNBIND_RATE_REACHED  0x1e

#define NPU_RX0_DESC_NUM        512
#define NPU_RX1_DESC_NUM        512
#define NPU_TXWI_LEN            128

static inline struct airoha_npu *airoha_npu_get(struct device *dev)
{
        return ERR_PTR(-EOPNOTSUPP);
}

static inline void airoha_npu_put(struct airoha_npu *npu) {}

static inline struct airoha_ppe_dev *airoha_ppe_get_dev(struct device *dev)
{
        return ERR_PTR(-EOPNOTSUPP);
}

static inline void airoha_ppe_put_dev(struct airoha_ppe_dev *ppe_dev) {}

static inline int airoha_npu_wlan_send_msg(struct airoha_npu *npu, int ifindex,
                                            enum airoha_npu_wlan_set_cmd cmd,
                                            void *data, int len, gfp_t gfp)
{
        return -EOPNOTSUPP;
}

static inline int airoha_npu_wlan_get_msg(struct airoha_npu *npu, int ifindex,
                                           enum airoha_npu_wlan_get_cmd cmd,
                                           void *data, int len, gfp_t gfp)
{
        return -EOPNOTSUPP;
}

static inline u32 airoha_npu_wlan_get_irq_status(struct airoha_npu *npu, int index)
{
        return 0;
}

static inline void airoha_npu_wlan_set_irq_status(struct airoha_npu *npu, u32 status) {}
static inline void airoha_npu_wlan_disable_irq(struct airoha_npu *npu, int index) {}
static inline void airoha_npu_wlan_enable_irq(struct airoha_npu *npu, int index) {}

static inline int airoha_npu_wlan_init_reserved_memory(struct airoha_npu *npu)
{
        return -EOPNOTSUPP;
}

static inline u32 airoha_npu_wlan_get_queue_addr(struct airoha_npu *npu, int qid, bool xmit)
{
        return 0;
}

static inline void airoha_ppe_dev_check_skb(struct airoha_ppe_dev *ppe_dev,
                                             struct sk_buff *skb, u16 hash, bool ether)
{
}

static inline int airoha_ppe_dev_setup_tc_block_cb(struct airoha_ppe_dev *ppe_dev,
                                                    void *type_data)
{
        return -EOPNOTSUPP;
}

#endif /* _LINUX_AIROHA_OFFLOAD_H_STUB */
AIORA_STUB_EOF
        echo "${stub_dir}/airoha_offload.h" >> "$STUB_HEADERS_CREATED"
        log "  Created stub: ${stub_dir}/airoha_offload.h"
    else
        log "  Already exists: ${stub_dir}/airoha_offload.h"
    fi

    # --- Stub: linux/soc/mediatek/mtk_wed.h ---
    stub_dir="${kbuild_dir}/include/linux/soc/mediatek"
    if [ ! -f "${stub_dir}/mtk_wed.h" ]; then
        mkdir -p "$stub_dir"
        cat > "${stub_dir}/mtk_wed.h" << 'MTK_WED_STUB_EOF'
/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Stub header for linux/soc/mediatek/mtk_wed.h
 * Created by mt7921u-safe-install.sh for platforms without MediaTek WED support
 * (e.g., Raspberry Pi). The driver source guards usage with
 * IS_ENABLED(CONFIG_NET_MEDIATEK_SOC_WED).
 */
#ifndef _LINUX_MTK_WED_H_STUB
#define _LINUX_MTK_WED_H_STUB

#include <linux/skbuff.h>
#include <net/pkt_cls.h>

struct mtk_wed_device {
        struct {
                void *hif2;
                unsigned int token_start;
        } wlan;
        struct {
                void *desc;
        } rx_buf_ring;
        struct {
                u32 reg_base;
        } tx_ring[8];
        struct {
                u32 reg_base;
        } txfree_ring;
        struct {
                u32 reg_base;
        } rx_ring[8];
};

struct mtk_wed_bm_desc {
        __le32 buf0;
        __le32 token;
};

static inline bool mtk_wed_device_active(struct mtk_wed_device *wed)
{
        return false;
}

static inline void mtk_wed_device_irq_set_mask(struct mtk_wed_device *wed, u32 mask) {}
static inline u32 mtk_wed_device_irq_get(struct mtk_wed_device *wed, u32 mask) { return 0; }
static inline void mtk_wed_device_detach(struct mtk_wed_device *wed) {}
static inline bool mtk_wed_get_rx_capa(struct mtk_wed_device *wed) { return false; }

static inline int mtk_wed_device_tx_ring_setup(struct mtk_wed_device *wed, int ring,
                                                u32 regs, bool reset) { return 0; }
static inline int mtk_wed_device_txfree_ring_setup(struct mtk_wed_device *wed, u32 regs)
{ return 0; }
static inline int mtk_wed_device_rx_ring_setup(struct mtk_wed_device *wed, int ring,
                                                u32 regs, bool reset) { return 0; }
static inline void mtk_wed_device_rro_rx_ring_setup(struct mtk_wed_device *wed, int ring,
                                                     u32 regs) {}
static inline void mtk_wed_device_msdu_pg_rx_ring_setup(struct mtk_wed_device *wed, int ring,
                                                         u32 regs) {}
static inline void mtk_wed_device_ind_rx_ring_setup(struct mtk_wed_device *wed, u32 regs) {}
static inline int mtk_wed_device_setup_tc(struct mtk_wed_device *wed, struct net_device *dev,
                                           enum tc_setup_type type, void *type_data)
{ return -EOPNOTSUPP; }
static inline void mtk_wed_device_stop(struct mtk_wed_device *wed) {}
static inline void mtk_wed_device_start(struct mtk_wed_device *wed, u32 irq_mask) {}
static inline void mtk_wed_device_start_hw_rro(struct mtk_wed_device *wed, u32 irq_mask,
                                                bool reset) {}
static inline void mtk_wed_device_dma_reset(struct mtk_wed_device *wed) {}
static inline void mtk_wed_device_ppe_check(struct mtk_wed_device *wed, struct sk_buff *skb,
                                             u32 reason, u32 hash) {}
static inline int mtk_wed_device_attach(struct mtk_wed_device *wed) { return -ENODEV; }
static inline int mtk_wed_device_update_msg(struct mtk_wed_device *wed, u32 id,
                                            void *msg, int len) { return 0; }
static inline bool mtk_wed_is_amsdu_supported(struct mtk_wed_device *wed) { return false; }
static inline u32 mtk_wed_device_reg_read(struct mtk_wed_device *wed, u32 reg) { return 0; }
static inline void mtk_wed_device_reg_write(struct mtk_wed_device *wed, u32 reg, u32 val) {}

#define MTK_WED_WO_CMD_RXCNT_CTRL       0
#define WED_WO_STA_REC          0

#endif /* _LINUX_MTK_WED_H_STUB */
MTK_WED_STUB_EOF
        echo "${stub_dir}/mtk_wed.h" >> "$STUB_HEADERS_CREATED"
        log "  Created stub: ${stub_dir}/mtk_wed.h"
    else
        log "  Already exists: ${stub_dir}/mtk_wed.h"
    fi

    log "Stub headers created successfully."
}

remove_stub_headers() {
    if [ -f "$STUB_HEADERS_CREATED" ]; then
        log "Removing stub headers..."
        while IFS= read -r f; do
            if [ -e "$f" ]; then
                rm -f "$f"
                log "  Removed: $f"
            fi
        done < "$STUB_HEADERS_CREATED"
        # Remove empty parent directories
        rm -rf "/lib/modules/${KERNEL_VERSION}/build/include/linux/soc/airoha" 2>/dev/null
        rmdir "/lib/modules/${KERNEL_VERSION}/build/include/linux/soc/mediatek" 2>/dev/null
        rmdir "/lib/modules/${KERNEL_VERSION}/build/include/linux/soc" 2>/dev/null
        rm -f "$STUB_HEADERS_CREATED"
        log "Stub headers removed."
    fi
}

# ===================== STATE MACHINE =====================
# States: INIT, BUILDING, INSTALLING, SWAPPING, WAITING, CONFIRMED, ROLLING_BACK
set_state() {
    echo "$1" > "$STATE_FILE"
    log "STATE -> $1"
}

get_state() {
    cat "$STATE_FILE" 2>/dev/null || echo "INIT"
}

# ===================== SIGNAL HANDLER =====================
handle_signal() {
    local state
    state="$(get_state)"

    case "$state" in
        INIT|BUILDING)
            log "Interrupted during $state. Nothing was changed on the system."
            log "Cleaning up stub headers if any..."
            remove_stub_headers
            log "Clean exit - no rollback needed."
            set_state "ABORTED"
            exit 1
            ;;
        INSTALLING|SWAPPING)
            log "Interrupted during $state. Performing safety rollback..."
            do_rollback
            exit 1
            ;;
        WAITING)
            log ""
            log "============================================================"
            log "  INSTALL CONFIRMED BY USER!"
            log "  New driver is permanent. No rollback will happen."
            log "============================================================"
            log ""
            set_state "CONFIRMED"
            # Show driver info
            log "Loaded mt7921u modules:"
            lsmod | grep -E 'mt76|mt792' | tee -a "$LOG_FILE" 2>/dev/null || true
            log "Wi-Fi interfaces:"
            ip link show | grep -E 'wlan' | tee -a "$LOG_FILE" 2>/dev/null || true
            log "Done. You can delete $BUILD_DIR if you want to save disk space."
            exit 0
            ;;
        *)
            log "Interrupted in state $state. Exiting."
            exit 1
            ;;
    esac
}

trap handle_signal SIGTERM SIGINT

# ===================== ROLLBACK FUNCTION =====================
do_rollback() {
    set_state "ROLLING_BACK"
    log ""
    separator
    log "  AUTOMATIC ROLLBACK - Restoring original driver"
    separator
    log ""

    # Step 1: Unload whatever is currently loaded (new or broken)
    log "Step 1/4: Unloading current driver modules..."
    for mod in $MODULES_UNLOAD; do
        if lsmod | grep -q "^${mod} "; then
            modprobe -r "$mod" 2>/dev/null || rmmod "$mod" 2>/dev/null || true
            log "  Unloaded: $mod"
        fi
    done
    # Also try mt7921e in case that was the old module
    if lsmod | grep -q "^mt7921e "; then
        modprobe -r mt7921e 2>/dev/null || rmmod mt7921e 2>/dev/null || true
        log "  Unloaded: mt7921e"
    fi
    sleep 2

    # Step 2: Remove the new module files from /lib/modules/$KVER/extra/
    log "Step 2/4: Removing new driver files from extra/..."
    if [ -f "$INSTALLED_FILES" ]; then
        while IFS= read -r f; do
            if [ -e "$f" ]; then
                rm -f "$f"
                log "  Removed: $f"
            fi
        done < "$INSTALLED_FILES"
        # Also remove any empty parent directories under extra/
        find "/lib/modules/${KERNEL_VERSION}/extra" -type d -empty -delete 2>/dev/null || true
    else
        log "  No installed files list found. Removing all mt76/mt792 modules from extra/..."
        find "/lib/modules/${KERNEL_VERSION}/extra" \( -name "mt76*.ko*" -o -name "mt792*.ko*" \) -delete 2>/dev/null || true
        find "/lib/modules/${KERNEL_VERSION}/extra" -type d -empty -delete 2>/dev/null || true
    fi

    # Step 3: Rebuild module dependency database
    log "Step 3/4: Rebuilding module database..."
    depmod -a "$KERNEL_VERSION"

    # Step 4: Reload original driver
    log "Step 4/4: Loading original driver..."
    # Try the module that was originally loaded
    local loaded=0
    if [ -f "$OLD_MODULES_LOADED" ]; then
        # Load in the original module's name - usually mt7921u or mt7921e on RPi
        local top_mod
        top_mod=$(tail -1 "$OLD_MODULES_LOADED" 2>/dev/null)
        if [ -n "$top_mod" ]; then
            modprobe "$top_mod" 2>&1 | tee -a "$LOG_FILE" && loaded=1
        fi
    fi
    if [ "$loaded" -eq 0 ]; then
        # Fallback: try both possible module names
        modprobe mt7921u 2>/dev/null && loaded=1 || true
        if [ "$loaded" -eq 0 ]; then
            modprobe mt7921e 2>/dev/null && loaded=1 || true
        fi
    fi

    if [ "$loaded" -eq 1 ]; then
        log "Original driver loaded successfully."
        log "Wi-Fi should come back within 30 seconds via your existing configs."
    else
        log "WARNING: Could not reload original driver automatically."
        log "You may need to run: sudo modprobe mt7921u"
        log "Or reboot the Pi."
    fi

    # Clean up stub headers on rollback too
    remove_stub_headers

    log ""
    separator
    log "  ROLLBACK COMPLETE"
    log "  If Wi-Fi does not come back in 60 seconds, reboot the Pi."
    separator
    log ""

    rm -f "$STATE_FILE"
}

# ===================== MANUAL ROLLBACK ENTRY POINT =====================
if [ "${1:-}" = "--rollback" ]; then
    log "Manual rollback requested..."
    do_rollback
    exit $?
fi

# ===================== MAIN SCRIPT =====================
separator
log "  MT7921U Safe Install - Raspberry Pi 5"
log "  Kernel: $KERNEL_VERSION"
log "  Script PID: $$"
separator
log ""
log "HOW THIS WORKS:"
log "  1. Build new driver from GitHub"
log "  2. Swap old driver for new driver (Wi-Fi goes down ~5 seconds)"
log "  3. Wait 5 minutes for you to reconnect"
log "  4. If you reconnect:  sudo kill $$  --> INSTALL STAYS"
log "     If you can't:      wait 5 min    --> AUTO ROLLBACK"
log ""

# ===================== PRE-FLIGHT CHECKS =====================
set_state "INIT"

log "Running pre-flight checks..."

# Must be root
if [ "$(id -u)" -ne 0 ]; then
    log "ERROR: This script must be run as root. Use: sudo bash $0"
    exit 1
fi

# Kernel headers must be installed
if [ ! -d "/lib/modules/${KERNEL_VERSION}/build" ]; then
    log "ERROR: Kernel headers not found at /lib/modules/${KERNEL_VERSION}/build"
    log "Install them with: sudo apt install linux-headers-${KERNEL_VERSION}"
    exit 1
fi
log "  [OK] Kernel headers present"

# Build tools
for cmd in make gcc git; do
    if ! command -v "$cmd" &>/dev/null; then
        log "ERROR: '$cmd' not found. Install with: sudo apt install build-essential git"
        exit 1
    fi
done
log "  [OK] Build tools present"

# Verify MT7921U hardware is present
if lsusb 2>/dev/null | grep -qi "0e8d:7961\|MediaTek.*7921"; then
    log "  [OK] MT7921U USB device detected"
else
    log "  [WARNING] MT7921U USB device not detected in lsusb."
    log "  Continuing anyway - the dongle may use a different USB ID."
fi

# Record which mt76/mt792 modules are currently loaded
log "  Currently loaded mt76/mt792 modules:"
lsmod | grep -E 'mt76|mt792' | awk '{print $1}' | tee "$OLD_MODULES_LOADED" 2>/dev/null || true

# Record current Wi-Fi interfaces
log "  Current Wi-Fi interfaces:"
ip link show | grep -E 'wlan' | awk -F': ' '{print $2}' | tee -a "$LOG_FILE" 2>/dev/null || true

# Check for sufficient disk space (need ~500MB for build)
available_kb=$(df /usr/src | tail -1 | awk '{print $4}')
if [ "$available_kb" -lt 512000 ]; then
    log "WARNING: Less than 500MB free on /usr/src. Build may fail."
fi

log "Pre-flight checks passed."
log ""

# ===================== PHASE 0: CREATE STUB HEADERS =====================
log "Creating stub headers for missing kernel API headers..."
create_stub_headers
log ""

# ===================== PHASE 1: BUILD =====================
set_state "BUILDING"
separator
log "PHASE 1/4: Building new driver from source"
separator
log ""

# Clone the repo
if [ -d "$BUILD_DIR" ]; then
    log "Removing previous build directory..."
    rm -rf "$BUILD_DIR"
fi

log "Cloning repository: $REPO_URL"
git clone --depth=1 "$REPO_URL" "$BUILD_DIR" 2>&1 | tee -a "$LOG_FILE"
if [ ! -d "$BUILD_DIR/drivers" ]; then
    log "ERROR: Clone failed or repo structure unexpected."
    remove_stub_headers
    exit 1
fi
log "Repository cloned."
log ""

# Build
MT76_SRC="$BUILD_DIR/drivers/net/wireless/mediatek/mt76"
log "Building driver modules (this takes 3-10 minutes on a Pi 5)..."
log "Build directory: $MT76_SRC"
log ""

make -C "/lib/modules/${KERNEL_VERSION}/build" \
     M="$MT76_SRC" \
     CONFIG_MT76_CORE=m \
     CONFIG_MT76_USB=m \
     CONFIG_MT76_CONNAC_LIB=m \
     CONFIG_MT792x_LIB=m \
     CONFIG_MT792x_USB=m \
     CONFIG_MT7921_COMMON=m \
     CONFIG_MT7921U=m \
     CONFIG_MT7921E=m \
     modules 2>&1 | tee -a "$LOG_FILE"

if [ "${PIPESTATUS[0]}" -ne 0 ]; then
    log "ERROR: Build failed! Check the log above for errors."
    log "Common fixes:"
    log "  - Make sure linux-headers-${KERNEL_VERSION} is installed"
    log "  - Make sure build-essential is installed"
    log "  - The kernel headers must match your running kernel exactly"
    log "  - Stub headers were created; check if they are correct"
    set_state "BUILD_FAILED"
    exit 1
fi
log ""
log "Build succeeded! Produced modules:"
find "$MT76_SRC" -name "*.ko" -exec basename {} \; | sort | tee -a "$LOG_FILE"
log ""

# ===================== PHASE 2: INSTALL =====================
set_state "INSTALLING"
separator
log "PHASE 2/4: Installing new driver modules to disk"
separator
log ""

# Install modules to /lib/modules/$KVER/extra/
# This does NOT affect the running driver yet - the old .ko files in
# /lib/modules/$KVER/kernel/ are untouched. The new ones in extra/ take
# priority via depmod.
log "Installing modules (to /lib/modules/${KERNEL_VERSION}/extra/)..."
make -C "/lib/modules/${KERNEL_VERSION}/build" \
     M="$MT76_SRC" \
     modules_install 2>&1 | tee -a "$LOG_FILE"

if [ "${PIPESTATUS[0]}" -ne 0 ]; then
    log "ERROR: Module installation failed!"
    do_rollback
    exit 1
fi

# Record exactly what we installed (for clean rollback)
find "/lib/modules/${KERNEL_VERSION}/extra" \( -name "mt76*.ko*" -o -name "mt792*.ko*" \) > "$INSTALLED_FILES" 2>/dev/null
log "Installed files:"
cat "$INSTALLED_FILES" | tee -a "$LOG_FILE"
log ""

# Rebuild module dependency database
depmod -a "$KERNEL_VERSION"
log "Module database updated."
log ""

# ===================== PHASE 3: SWAP DRIVERS =====================
set_state "SWAPPING"
separator
log "PHASE 3/4: Swapping driver (Wi-Fi will go down briefly!)"
separator
log ""

# Unload the old driver modules
# We need to unload in reverse dependency order
log "Unloading old driver..."
for mod in $MODULES_UNLOAD; do
    if lsmod | grep -q "^${mod} "; then
        modprobe -r "$mod" 2>/dev/null || rmmod "$mod" 2>/dev/null || true
        log "  Unloaded: $mod"
    fi
done
# Also catch mt7921e if it was loaded instead of mt7921u
if lsmod | grep -q "^mt7921e "; then
    modprobe -r mt7921e 2>/dev/null || rmmod mt7921e 2>/dev/null || true
    log "  Unloaded: mt7921e"
fi

# Wait for modules to fully release
sleep 2

# Verify modules are unloaded
remaining=$(lsmod | grep -cE '^mt76|^mt792' || true)
if [ "$remaining" -gt 0 ]; then
    log "WARNING: Some modules still loaded, force removing..."
    for mod in $MODULES_UNLOAD mt7921e; do
        rmmod "$mod" 2>/dev/null || true
    done
    sleep 2
fi

log "Old driver unloaded."
log ""

# Load the new driver
log "Loading new driver..."
modprobe mt7921u 2>&1 | tee -a "$LOG_FILE"
load_result=$?

if [ $load_result -ne 0 ]; then
    log "ERROR: Failed to load mt7921u module!"
    log "Trying mt7921e as fallback..."
    modprobe mt7921e 2>&1 | tee -a "$LOG_FILE"
    load_result=$?
fi

if [ $load_result -ne 0 ]; then
    log "ERROR: Could not load any mt7921 driver!"
    log "Initiating rollback..."
    do_rollback
    exit 1
fi

log "New driver loaded!"
log ""

# Wait a moment for interfaces to appear
sleep 5

# Show interface status
log "Current Wi-Fi interfaces:"
ip link show 2>/dev/null | grep -E 'wlan' | tee -a "$LOG_FILE" || log "  (none visible yet - NetworkManager may need more time)"
log ""

# ===================== PHASE 4: WAIT FOR CONFIRMATION =====================
set_state "WAITING"
separator
log "PHASE 4/4: Waiting for your confirmation"
separator
log ""
log "  >>> If you can read this via Wi-Fi, the new driver works! <<<"
log ""
log "  TO CONFIRM (keep new driver permanently):"
log "    sudo kill $$"
log ""
log "  TO ROLLBACK (if Wi-Fi is broken):"
log "    Do nothing. Automatic rollback in $((WAIT_SECONDS / 60)) minutes."
log ""
log "  Countdown started at: $(date '+%H:%M:%S')"
log "  Rollback at:          $(date -d "+${WAIT_SECONDS} seconds" '+%H:%M:%S' 2>/dev/null || date '+%H:%M:%S')"
separator
log ""

# Sleep for the wait period. If killed during this sleep,
# the trap handler fires and confirms the install.
# If the sleep completes, we fall through to rollback.
sleep "$WAIT_SECONDS"

# ===================== AUTOMATIC ROLLBACK =====================
# We only get here if the sleep completed = user did NOT confirm
log ""
log "TIMEOUT: $((WAIT_SECONDS / 60)) minutes elapsed without confirmation."
log "Assuming Wi-Fi is broken. Rolling back to original driver..."
log ""

do_rollback

log "Rollback finished. The Pi should reconnect with the old driver."
log "If it doesn't, reboot the Pi: sudo reboot"

exit 0
