# Use the non-open-source part, if present
-include vendor/vanzo/vanzo6795_lwt_cu/BoardConfigVendor.mk

# Use the 6795 common part
include device/mediatek/mt6795/BoardConfig.mk

#Config partition size
include device/vanzo/vanzo6795_lwt_cu/partition_size.mk
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_FLASH_BLOCK_SIZE := 4096

include device/vanzo/$(MTK_TARGET_PROJECT)/ProjectConfig.mk
# Vanzo:wangfei on: Wed, 12 Nov 2014 21:30:14 +0800
# added for aosp management to import custom config
#added by wf
project_name:=$(shell echo $(VANZO_INNER_PROJECT_NAME))
-include zprojects/$(project_name)/$(project_name).mk
#above is added by wf
# End of Vanzo:wangfei

MTK_INTERNAL_CDEFS := $(foreach t,$(AUTO_ADD_GLOBAL_DEFINE_BY_NAME),$(if $(filter-out no NO none NONE false FALSE,$($(t))),-D$(t))) 
MTK_INTERNAL_CDEFS += $(foreach t,$(AUTO_ADD_GLOBAL_DEFINE_BY_VALUE),$(if $(filter-out no NO none NONE false FALSE,$($(t))),$(foreach v,$(shell echo $($(t)) | tr '[a-z]' '[A-Z]'),-D$(v)))) 
MTK_INTERNAL_CDEFS += $(foreach t,$(AUTO_ADD_GLOBAL_DEFINE_BY_NAME_VALUE),$(if $(filter-out no NO none NONE false FALSE,$($(t))),-D$(t)=\"$($(t))\")) 

COMMON_GLOBAL_CFLAGS += $(MTK_INTERNAL_CDEFS)
COMMON_GLOBAL_CPPFLAGS += $(MTK_INTERNAL_CDEFS)

ifeq ($(strip $(MTK_IPOH_SUPPORT)), yes)
# Guarantee cache partition size: 420MB.
# Vanzo:hanshengpeng on: Wed, 08 Apr 2015 18:00:24 +0800
# modify platform systemimg size flow partition xls
# BOARD_MTK_SYSTEM_SIZE_KB :=1572864
# End of Vanzo:hanshengpeng
BOARD_MTK_CACHE_SIZE_KB :=430080
BOARD_MTK_USERDATA_SIZE_KB :=1572864
endif
# Vanzo:wangfei on: Wed, 12 Nov 2014 21:31:23 +0800
# added for aosp management to import custom config
#added by wf
ifneq ($(strip $(project_name)),)
VANZO_INTERNAL_CDEFS := $(foreach t,$(VANZO_DEFINE_BY_NAME),$(if $(filter-out no NO none NONE false FALSE,$($(t))),-D$(t))) 
VANZO_INTERNAL_CDEFS += $(foreach t,$(VANZO_DEFINE_BY_VALUE),$(if $(filter-out no NO none NONE false FALSE,$($(t))),$(foreach v,$(shell echo $($(t)) | tr '[a-z]' '[A-Z]'),-D$(v)))) 
VANZO_INTERNAL_CDEFS += $(foreach t,$(VANZO_DEFINE_BY_NAME_VALUE),$(if $(filter-out no NO none NONE false FALSE,$($(t))),-D$(t)=\"$($(t))\")) 
COMMON_GLOBAL_CFLAGS += $(VANZO_INTERNAL_CDEFS)
COMMON_GLOBAL_CPPFLAGS += $(VANZO_INTERNAL_CDEFS)
endif
#above is added by wf
# End of Vanzo:wangfei
