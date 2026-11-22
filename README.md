# vendor_beryl-extras

Port of Malachite Extras for **Redmi Note 14 5G (beryl)**:
- **Parts** (CPU, GPU, thermal, touch sampling, ZRAM, refresh rate, resolution, per-app profiles)
- **Xiaomi CIT** (factory hardware tests, factory HAL, test configs, MediaTek telephony classes)
- **Goodix FOD Calibration / Test** screen
- **malachited daemon** allowing Parts to tune kernel settings on user builds without root

Supports Android 16 and Android 17 source trees. All APKs are automatically re-signed with your platform key during the ROM build.

---

## Requirements

- `git-lfs` installed before syncing:
  ```bash
  sudo apt install git-lfs && git lfs install
  ```
- Device tree at `device/xiaomi/beryl`
- Vendor blobs at `vendor/xiaomi/beryl` (or `vendor/xiaomi/beryl/proprietary/vendor`)

---

## Integration

### 1. Add to local manifest (`.repo/local_manifests/beryl-extras.xml`):

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
   - Patches `system/sepolicy` (exempts `malachited` from `coredomain.te` `proc`/`sysfs` neverallows).
   - Patches the Goodix HAL blob (`fingerprint.goodix_fod.default.so`) to point display parameter calls to `/dev/gf_disp_param`.
   - Handles Android 17 vs Android 16 differences (`platform_app_all` attribute detection and legacy vs A17 sepolicy).

2. **Touch Sampling**:
   - Up to standard frequencies work root-free.
   - High-rate touch sampling (1000 Hz) requires granting root access in KernelSU / Magisk.

3. **CIT Access**:
   - Open via Parts app or by dialing `*#*#6484#*#*` in the dialer.
