LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := assimp-prebuilt
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libassimp.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../include/android
include $(PREBUILT_SHARED_LIBRARY)
