LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := spidermonkey_static
LOCAL_MODULE_FILENAME := js_static
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libjs_static.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../include/android
LOCAL_CPPFLAGS := -D__STDC_LIMIT_MACROS=1 -DIMPL_MFBT
LOCAL_EXPORT_CPPFLAGS := -D__STDC_LIMIT_MACROS=1 -DIMPL_MFBT
include $(PREBUILT_STATIC_LIBRARY)