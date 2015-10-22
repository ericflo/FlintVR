LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := mozglue-prebuilt
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libmozglue.so
include $(PREBUILT_SHARED_LIBRARY)