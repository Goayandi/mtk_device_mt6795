LOCAL_PATH:= $(call my-dir)
ifneq ($(BUILD_MTK_LDVT), yes)
include $(call all-makefiles-under,$(LOCAL_PATH))
else
include $(LOCAL_PATH)/ldvt/Android.mk
endif

