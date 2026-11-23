#!/bin/bash
# Run from the source root after a build: bash vendor/beryl-extras/check.sh
O=${OUT:-out/target/product/beryl}; A2=out/host/linux-x86/bin/aapt2; ok=1
chk() { if eval "$2"; then echo "  OK    $1"; else echo "  FAIL  $1"; ok=0; fi; }
x() { $A2 dump xmltree --file AndroidManifest.xml "$1" 2>/dev/null; }
CIT=$O/system_ext/app/MMIGroup_NoIcon/MMIGroup_NoIcon.apk
GX=$(ls $O/system_ext/app/goodix_sz/*.apk 2>/dev/null | head -1)
HAL=$O/vendor/lib64/hw/fingerprint.goodix_fod.default.so
echo "beryl-extras check ($O)"
chk "Parts installed as privileged app"      "ls $O/system_ext/priv-app/BerylParts/*.apk >/dev/null 2>&1 || ls $O/system_ext/priv-app/MalachiteParts/*.apk >/dev/null 2>&1"
chk "Parts privapp permissions"               "[ -f $O/system_ext/etc/permissions/privapp-permissions-berylparts.xml ] || [ -f $O/system_ext/etc/permissions/privapp-permissions-malachiteparts.xml ]"
chk "beryld installed, with gfprep"           "strings $O/system_ext/bin/beryld 2>/dev/null | grep -q disp-DSI-0/disp_param || strings $O/system_ext/bin/malachited 2>/dev/null | grep -q disp-DSI-0/disp_param"
chk "beryld sepolicy applied"                 "grep -q mlstrustedobject system/sepolicy/private/beryld.te 2>/dev/null || grep -q mlstrustedobject system/sepolicy/private/malachited.te 2>/dev/null"
chk "CIT normal-mode entry exported"          "x $CIT | grep -A4 '\"com.factory.mmigroup.MMIGroupNormalMode\"' | grep -q 'exported.*=true'"
chk "CIT MediaTek libraries declared"         "[ \$(x $CIT | grep -c 'E: uses-library') -ge 2 ] && [ -f $O/system_ext/etc/permissions/cit-mtk-libs.xml ]"
chk "CIT MediaTek library jars"               "[ \$(ls $O/system_ext/framework/*-cit.jar 2>/dev/null | wc -l) -ge 2 ]"
chk "factory service + interface line"        "[ -f $O/vendor/bin/hw/interfaces.factoryInterface@1.0-service ] && grep -rqs 'interface interfaces.factoryInterface' $O/vendor/etc/init/"
chk "factory HIDL light library (Android 17)"  "! grep -qs '^attribute platform_app_all;' system/sepolicy/public/attributes || [ -f $O/vendor/lib64/android.hardware.light@2.0.so ]"
chk "factory nvram@1.0 library"               "[ -f $O/vendor/lib64/vendor.mediatek.hardware.nvram@1.0.so ]"
chk "CIT configs (vendor/etc/mmigroup)"       "[ -f $O/vendor/etc/mmigroup/factoryInterfaceConfig.json ]"
chk "Goodix test screen exported"             "x $GX | grep -B4 -A4 '\\.MainActivity\"' | grep -q 'exported.*=true'"
chk "Goodix HAL patched"                      "strings $HAL | grep -q /dev/gf_disp_param"
chk "Goodix HAL is a valid library"           "[ \$(readelf -d $HAL 2>/dev/null | grep -c NEEDED) -gt 0 ]"
chk "GPS mnld uses libmnk"                    "readelf -d $O/vendor/bin/mnld 2>/dev/null | grep -q libmnk.so"
[ $ok = 1 ] && echo "All good." || echo "Something is missing, see FAIL lines."
