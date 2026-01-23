LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := freg_test
LOCAL_SRC_FILES := \
    TestFreg.cpp \

LOCAL_SHARED_LIBRARIES := \
   liblog \
   libhidlbase \
   libutils \
   android.hardware.freg@1.0 \

include $(BUILD_EXECUTABLE)