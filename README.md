# vendor_beryl-extras

Universal port of device extras for **Redmi Note 14 5G / Redmi Note 15R Pro / POCO M7 Pro 5G (beryl / citrine)**:
- **Beryl Parts** (CPU governor/frequencies, GPU OPP target, thermal profile `sconfig`, touch sampling rate, ZRAM resizing & compression algorithms, refresh rate, resolution, per-app profiles)
- **Xiaomi CIT** (factory hardware tests, factory HAL service `interfaces.factoryInterface@1.0-service`, test configs in `/vendor/etc/mmigroup`, MediaTek telephony classes)
- **Goodix FOD Calibration / Test** screen
- **beryld rootless daemon** allowing Parts to tune kernel settings on user builds without root privileges

Fully compatible with Android 16 and Android 17 AOSP/LineageOS trees, specifically the [TracenPlayground/device_xiaomi_beryl](https://github.com/TracenPlayground/device_xiaomi_beryl) device and vendor trees. All APKs are automatically re-signed with your ROM's platform certificate during compilation.

---

## Requirements

- `git-lfs` installed before syncing:
  ```bash
  sudo apt install git-lfs && git lfs install
  ```
- Device tree at `device/xiaomi/beryl` (e.g. from `TracenPlayground/device_xiaomi_beryl`)
- Vendor blobs at `vendor/xiaomi/beryl` (e.g. from `TracenPlayground/vendor_xiaomi_beryl`)

---

## Integration

### 1. Add to local manifest (`.repo/local_manifests/beryl.xml` or `beryl-extras.xml`):

```xml
<?xml version="1.0" encoding="UTF-8"?>
<manifest>
  <project path="vendor/beryl-extras" name="vendor_beryl-extras" remote="github" revision="main" />
</manifest>
```

### 2. Hook into device tree:

Using the automated script:
```bash
bash vendor/beryl-extras/integrate.sh device/xiaomi/beryl
```

Or manually:

In `device/xiaomi/beryl/device.mk`:
```makefile
$(call inherit-product-if-exists, vendor/beryl-extras/beryl-extras.mk)
```

In `device/xiaomi/beryl/BoardConfig.mk`:
```makefile
-include vendor/beryl-extras/BoardConfigExtras.mk
```

---

## Build & Verification

1. Sync the project:
   ```bash
   repo sync vendor/beryl-extras
   ```
2. Build your ROM as usual (`m bacon`, `brunch beryl`, etc.)
3. Verify output:
   ```bash
   bash vendor/beryl-extras/check.sh
   ```
   `check.sh` should end with **"All good."**

---

## Notes & Technical Details

1. **Automatic Build Patches**:
   On every build, `BoardConfigExtras.mk` automatically:
   - Patches `system/sepolicy` (exempts `beryld` from `coredomain.te` `proc`/`sysfs` neverallows).
   - Patches the Goodix HAL blob (`fingerprint.goodix_fod.default.so`) to point display parameter calls to `/dev/gf_disp_param`.
   - Handles Android 17 vs Android 16 differences (`platform_app_all` attribute detection and legacy vs A17 sepolicy).

2. **Touch Sampling**:
   - Supports FocalTech (`fts_ts`) and Goodix touch controllers found across Redmi Note 14 5G hardware batches.
   - Standard frequencies work root-free.
   - Extreme touch sampling (1000 Hz) requires granting root access in KernelSU / Magisk.

3. **CIT Access**:
   - Open via Parts app or by dialing `*#*#6484#*#*` in the dialer.
