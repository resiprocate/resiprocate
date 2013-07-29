LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := resipares
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libresipares-1.9.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := rutil
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/librutil-1.9.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := resip
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libresip-1.9.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := dum
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libdum-1.9.so
include $(PREBUILT_SHARED_LIBRARY)

