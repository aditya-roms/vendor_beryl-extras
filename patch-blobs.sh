#!/bin/bash
# Idempotent blob fixes for beryl. Safe on already-patched blobs.
for B in vendor/xiaomi/beryl/proprietary/vendor vendor/xiaomi/beryl vendor/xiaomi/malachite/proprietary/vendor; do
  [ -d "$B" ] || continue

  # Goodix FOD HAL: point it at /dev/gf_disp_param (created by malachited.rc). Same length, verified.
  H=$B/lib64/hw/fingerprint.goodix_fod.default.so
  if [ -f "$H" ] && grep -q "/sys/class/drm/card0-DSI-1/disp_param" "$H"; then
    S=$(stat -c %s "$H")
    perl -pi -e 's#/sys/class/drm/card0-DSI-1/disp_param#"/dev/gf_disp_param" . ("\x00" x 19)#ge' "$H"
    [ "$(stat -c %s "$H")" = "$S" ] || { echo "beryl-extras: Goodix HAL size changed, restore it from git" >&2; exit 1; }
  fi

  # GPS: MediaTek's libmnl clashes with AOSP's netlink libmnl. Only when the blobs ship it as libmnk.
  M=$B/bin/mnld
  if [ -f "$M" ] && (grep -qs "libmnk" vendor/xiaomi/beryl/Android.bp 2>/dev/null || grep -qs "libmnk" vendor/xiaomi/malachite/Android.bp 2>/dev/null) && grep -q "libmnl.so" "$M"; then
    perl -pi -e 's/libmnl\.so/libmnk.so/g' "$M"
  fi
done
exit 0
