LOCAL_PATH:= $(call my-dir)

src_files :=  jsoncpp.cpp \
		jsonapi.cpp

c_includes := $(LOCAL_PATH)/../../android-ndk-r10/sources/cxx-stl/gnu-libstdc++/4.6/include \
	$(LOCAL_PATH)/../../android-ndk-r10/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi/include

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(src_files)
LOCAL_C_INCLUDES := $(c_includes)
LOCAL_LDLIBS := -L$(LOCAL_PATH)/../../android-ndk-r10/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi-v7a/ -lgnustl_shared
LOCAL_CPPFLAGS := -DJSON_IS_AMALGAMATION -fexceptions
LOCAL_MODULE := libhilinkjson
include $(BUILD_SHARED_LIBRARY)
