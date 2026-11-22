#!/bin/bash
# Usage (from the source root): vendor/beryl-extras/integrate.sh device/xiaomi/beryl
set -e
D=${1:?usage: integrate.sh <device tree dir>}
add() { grep -qF -- "$2" "$1" || echo "$2" >> "$1"; }
add "$D/device.mk" '$(call inherit-product-if-exists, vendor/beryl-extras/beryl-extras.mk)'
add "$D/BoardConfig.mk" '-include vendor/beryl-extras/BoardConfigExtras.mk'
if [ -f vendor/beryl-extras/sepolicy-patch.sh ]; then
  vendor/beryl-extras/sepolicy-patch.sh
elif [ -f vendor/malachite-extras/sepolicy-patch.sh ]; then
  vendor/malachite-extras/sepolicy-patch.sh
fi
echo "beryl-extras integrated into $D"
