#!/bin/bash
# Build the mt76/mt7921u modules against the running kernel.
# Artifacts stay in the source tree; nothing is installed.
# For the install/rollback procedure see docs/install.md.
set -euo pipefail

KDIR="/lib/modules/$(uname -r)/build"
SRC="$(cd "$(dirname "$0")" && pwd)/drivers/net/wireless/mediatek/mt76"

[ -d "$KDIR" ] || { echo "kernel headers missing: $KDIR" >&2; exit 1; }

make -C "$KDIR" M="$SRC" modules -j"$(nproc)"

echo
echo "Built:"
find "$SRC" -name '*.ko' -printf '  %p\n' | sort
