ifeq ($(strip $(VANZO_FEATURE_ADD_FPC)), yes)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libcac
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_SRC_FILES := libs/lib/armeabi-v7a/libcac.a
LOCAL_SRC_FILES_arm64 := libs/lib/arm64-v8a/libcac.a
LOCAL_MODULE_SUFFIX := .a
LOCAL_MULTILIB := both
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libpp
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_SRC_FILES := libs/lib/armeabi-v7a/libpp.a
LOCAL_SRC_FILES_arm64 := libs/lib/arm64-v8a/libpp.a
LOCAL_MODULE_SUFFIX := .a
LOCAL_MULTILIB := both
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE    := fpc1021_halmodule

LOCAL_C_INCLUDES += $(JNI_H_INCLUDE) \
                    $(LOCAL_PATH)/ \
                    $(LOCAL_PATH)/libs/include \
                    $(LOCAL_PATH)/libhardware/include \
                    $(LOCAL_PATH)/conf_parser \
                    $(LOCAL_PATH)/fpc102X/ \
                    $(TARGET_OUT_HEADERS)/common/inc

#ifdef BUILD_FPC_KPI_MEASURE
#LOCAL_C_INCLUDES += $(LOCAL_PATH)/hal_preprocessor/fpc_KPI_util
#endif

LOCAL_SRC_FILES := fpc102X/fpc102xdevice.cpp \
                   fpc102X/fpc102xdriver.cpp \
                   fpc1021_halmodule/fpc1021_halmodule.cpp \
                   fpc_linux_helper.cpp \
                   conf_parser/conf_parser.c

#ifdef BUILD_FPC_KPI_MEASURE
#LOCAL_SRC_FILES += $(LOCAL_PATH)/hal_preprocessor/fpc_KPI_util/fpc_kpi_util.cpp
#endif

LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcutils


#LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog
LOCAL_LDLIBS := -llog

LOCAL_STATIC_LIBRARIES := libpp libcac 

ifdef BUILD_FPC_KPI_MEASURE
LOCAL_CFLAGS := -DLOG_TAG='"fpc1021_halmodule"' -Wall -DCONFIG_FPC_KPI_MEASURE
else
LOCAL_CFLAGS := -DLOG_TAG='"fpc1021_halmodule"' -Wall
endif

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
endif
