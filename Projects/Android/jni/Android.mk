LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include ../../../../../cflags.mk

LOCAL_MODULE           := ovrapp
LOCAL_SRC_FILES        := ../../../Src/OvrApp.cpp
LOCAL_SRC_FILES          += ../../../Src/CoreGeometry.cpp
LOCAL_SRC_FILES          += ../../../Src/CoreProgram.cpp
LOCAL_SRC_FILES          += ../../../Src/CoreScene.cpp
LOCAL_SRC_FILES          += ../../../Src/CoreVector2f.cpp
LOCAL_SRC_FILES          += ../../../Src/CoreVector3f.cpp
LOCAL_SRC_FILES          += ../../../Src/CoreVector4f.cpp
LOCAL_SRC_FILES          += ../../../Src/ParseVertexAttribs.cpp
LOCAL_SRC_FILES          += ../../../Src/CoreModel.cpp
LOCAL_SRC_FILES          += ../../../Src/CoreMatrix4f.cpp
LOCAL_SRC_FILES          += ../../../Src/CoreCommon.cpp
LOCAL_SRC_FILES          += ../../../Src/CoreTexture.cpp
LOCAL_STATIC_LIBRARIES := vrsound vrmodel vrlocale vrgui vrappframework systemutils libovrkernel spidermonkey_static bullet_static
LOCAL_SHARED_LIBRARIES := vrapi libandroid mozglue-prebuilt assimp-prebuilt
LOCAL_LDLIBS           += -landroid

include $(BUILD_SHARED_LIBRARY)

$(call import-module,LibOVRKernel/Projects/AndroidPrebuilt/jni)
$(call import-module,VrApi/Projects/AndroidPrebuilt/jni)
$(call import-module,VrAppSupport/SystemUtils/Projects/AndroidPrebuilt/jni)
$(call import-module,VrAppFramework/Projects/AndroidPrebuilt/jni)
$(call import-module,VrAppSupport/VrGui/Projects/AndroidPrebuilt/jni)
$(call import-module,VrAppSupport/VrLocale/Projects/AndroidPrebuilt/jni)
$(call import-module,VrAppSupport/VrModel/Projects/AndroidPrebuilt/jni)
$(call import-module,VrAppSupport/VrSound/Projects/AndroidPrebuilt/jni)
$(call import-module,VrSamples/Native/Flint/Projects/Android/deps/spidermonkey44/prebuilt/android)
$(call import-module,VrSamples/Native/Flint/Projects/Android/deps/libmozglue/prebuilt/android)
$(call import-module,VrSamples/Native/Flint/Projects/Android/deps/bullet)
$(call import-module,VrSamples/Native/Flint/Projects/Android/deps/assimp/prebuilt/android)