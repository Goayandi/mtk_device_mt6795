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
$(call inherit-product, device/vanzo/y991/device.mk)

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
PRODUCT_NAME := full_y991
PRODUCT_DEVICE := y991
PRODUCT_MODEL := y991
PRODUCT_POLICY := android.policy_phone
PRODUCT_BRAND := alps

ifeq ($(TARGET_BUILD_VARIANT), eng)
KERNEL_DEFCONFIG ?= y991_debug_defconfig
else
KERNEL_DEFCONFIG ?= y991_defconfig
endif
PRELOADER_TARGET_PRODUCT ?= y991
LK_PROJECT ?= y991
# Vanzo:hanshengpeng on: Fri, 22 May 2015 17:27:04 +0800
# add 3rd-fpc
ifeq ($(VANZO_FEATURE_ADD_FPC), yes)
$(call inherit-product, device/vanzo/y991/fpc1021_hal/fpc1021.mk)
$(call inherit-product, vendor/fingerprints/device/fingerprints1021.mk)
endif
# End of Vanzo:hanshengpeng
