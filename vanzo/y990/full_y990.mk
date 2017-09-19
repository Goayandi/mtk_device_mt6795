# Inherit for devices that support 64-bit primary and 32-bit secondary zygote startup script
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)

# Inherit from those products. Most specific first.
#$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)
# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)

# Set target and base project for flavor build
MTK_TARGET_PROJECT := $(subst full_,,$(TARGET_PRODUCT))
MTK_BASE_PROJECT := $(MTK_TARGET_PROJECT)
MTK_PROJECT_FOLDER := $(shell find device/* -maxdepth 1 -name $(MTK_BASE_PROJECT))
MTK_TARGET_PROJECT_FOLDER := $(shell find device/* -maxdepth 1 -name $(MTK_TARGET_PROJECT))

# This is where we'd set a backup provider if we had one
#$(call inherit-product, device/sample/products/backup_overlay.mk)
# Inherit from maguro device
$(call inherit-product, device/vanzo/y990/device.mk)

# set locales & aapt config.
PRODUCT_LOCALES := zh_CN en_US zh_TW

# Vanzo:songlixin on: Sat, 31 Jan 2015 16:41:22 +0800
# for locales and aapt customization
ifneq ($(strip $(VANZO_PRODUCT_LOCALES)),)
    PRODUCT_LOCALES := $(VANZO_PRODUCT_LOCALES)
endif
ifneq ($(strip $(VANZO_PRODUCT_AAPT_CONFIG)),)
    PRODUCT_AAPT_CONFIG := $(VANZO_PRODUCT_AAPT_CONFIG)
endif
# End of Vanzo:songlixin
# Set those variables here to overwrite the inherited values.
PRODUCT_MANUFACTURER := alps
PRODUCT_NAME := full_y990
PRODUCT_DEVICE := y990
PRODUCT_MODEL := y990
PRODUCT_POLICY := android.policy_phone
PRODUCT_BRAND := alps

ifeq ($(TARGET_BUILD_VARIANT), eng)
KERNEL_DEFCONFIG ?= y990_debug_defconfig
else
KERNEL_DEFCONFIG ?= y990_defconfig
endif
PRELOADER_TARGET_PRODUCT ?= y990
LK_PROJECT ?= y990
