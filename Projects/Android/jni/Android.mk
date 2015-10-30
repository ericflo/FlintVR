LOCAL_PATH := $(call my-dir)

#--------------------------------------------------------
# libvrcubeworld.so
#--------------------------------------------------------
include $(CLEAR_VARS)

include ../../../../../cflags.mk

LOCAL_MODULE          :=   vrcubeworld
LOCAL_SRC_FILES       :=   ../../../Src/VrCubeWorld_Framework.cpp
LOCAL_SRC_FILES       += ../../../Src/CoreGeometry.cpp
LOCAL_SRC_FILES       += ../../../Src/CoreProgram.cpp
LOCAL_SRC_FILES       += ../../../Src/CoreScene.cpp
LOCAL_SRC_FILES       += ../../../Src/CoreVector2f.cpp
LOCAL_SRC_FILES       += ../../../Src/CoreVector3f.cpp
LOCAL_SRC_FILES       += ../../../Src/CoreVector4f.cpp
LOCAL_SRC_FILES       += ../../../Src/ParseVertexAttribs.cpp
LOCAL_SRC_FILES       += ../../../Src/CoreModel.cpp
LOCAL_SRC_FILES       += ../../../Src/CoreMatrix4f.cpp
LOCAL_SRC_FILES       += ../../../Src/CoreCommon.cpp
LOCAL_SRC_FILES       += ../../../Src/CoreTexture.cpp
LOCAL_STATIC_LIBRARIES  += vrsound vrlocale vrgui vrappframework libovrkernel spidermonkey_static bullet_static
LOCAL_SHARED_LIBRARIES  += vrapi libandroid mozglue-prebuilt
LOCAL_LDLIBS            += -landroid

include $(BUILD_SHARED_LIBRARY)

$(call import-module,LibOVRKernel/Projects/AndroidPrebuilt/jni)
$(call import-module,VrApi/Projects/AndroidPrebuilt/jni)
$(call import-module,VrAppFramework/Projects/AndroidPrebuilt/jni)
$(call import-module,VrAppSupport/VrGui/Projects/Android/jni)
$(call import-module,VrAppSupport/VrLocale/Projects/Android/jni)
$(call import-module,VrAppSupport/VrSound/Projects/Android/jni)
$(call import-module,VrSamples/Native/VrCubeWorld_Framework/Projects/Android/deps/spidermonkey44/prebuilt/android)
$(call import-module,VrSamples/Native/VrCubeWorld_Framework/Projects/Android/deps/libmozglue/prebuilt/android)
$(call import-module,VrSamples/Native/VrCubeWorld_Framework/Projects/Android/deps/bullet)