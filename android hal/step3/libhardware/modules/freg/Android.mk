LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SHARED_LIBRARIES := liblog 
LOCAL_CFLAGS += -Wno-unused-parameter

#debug
#这个目录打包后对应设备上的/system/lib64/hw或/system/lib/hw
$(info outpath=$(TARGET_OUT_SHARED_LIBRARIES)/hw)

LOCAL_SRC_FILES := freg.cpp
LOCAL_MODULE := freg.default
include $(BUILD_SHARED_LIBRARY)