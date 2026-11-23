PRODUCT_SOONG_NAMESPACES += vendor/beryl-extras

PRODUCT_PACKAGES += \
    BerylParts \
    MMIGroup_NoIcon \
    goodix_sz \
    beryld \
    interfaces.factoryInterface@1.0-service

# CIT test configs read by the factory service and the CIT app
PRODUCT_COPY_FILES += $(foreach f,$(shell cd vendor/beryl-extras/factory/mmigroup && find . -type f | sed 's|^\./||'),\
    vendor/beryl-extras/factory/mmigroup/$(f):$(TARGET_COPY_OUT_VENDOR)/etc/mmigroup/$(f))

# MediaTek telephony classes CIT needs, loaded only by CIT
PRODUCT_PACKAGES += mediatek-ims-base-cit mediatek-telephony-base-cit cit-mtk-libs.xml

# Stock's HIDL light library for the factory service, only on trees that don't build their own
# (Android 17 dropped it). Lives in its own soong namespace so other trees never see it.
ifeq ($(wildcard hardware/interfaces/light/2.0/Android.bp),)
PRODUCT_SOONG_NAMESPACES += vendor/beryl-extras/factory/light
PRODUCT_PACKAGES += android.hardware.light@2.0_mx
endif
