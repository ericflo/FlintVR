LOCAL_PATH := $(call my-dir)

#--------------------------------------------------------
# libvrcubeworld.so
#--------------------------------------------------------
include $(CLEAR_VARS)

include ../../../../../cflags.mk

LOCAL_MODULE			:= vrcubeworld
LOCAL_SRC_FILES			:= ../../../Src/VrCubeWorld_Framework.cpp ../../../Src/CoreGeometry.cpp ../../../Src/CoreProgram.cpp ../../../Src/CoreScene.cpp ../../../Src/CoreVector3f.cpp ../../../Src/CoreVector4f.cpp ../../../Src/ParseVertexAttribs.cpp ../../../Src/SceneGraph.cpp ../../../Src/CoreModel.cpp ../../../Src/CoreMatrix4f.cpp
LOCAL_STATIC_LIBRARIES	+= vrsound vrlocale vrgui vrappframework libovrkernel spidermonkey_static bullet_static
LOCAL_SHARED_LIBRARIES	+= vrapi libandroid mozglue-prebuilt
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