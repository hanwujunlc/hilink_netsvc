LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libhilinkdevicesdk
LOCAL_SRC_FILES := libhilinkdevicesdk.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libhilinkjson
LOCAL_SRC_FILES := libhilinkjson.so
include $(PREBUILT_SHARED_LIBRARY)

src_files := main.c \
		msg.c \
		utils.c \
		hilink_socket_stub.c \
		hilink_osadapter.c \
		hilink_profile.c \
		hilink_config.c \
		base64.c

c_includes := \

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(src_files)
LOCAL_C_INCLUDES := $(c_includes)
LOCAL_LDLIBS    := -lm -llog
LOCAL_LDFLAGS	:= -pie -fpie
LOCAL_SHARED_LIBRARIES := libhilinkjson
LOCAL_STATIC_LIBRARIES := libhilinkdevicesdk
#LOCAL_LDFLAGS	+= libhilinkdevicesdk.so libhilinkjson.so
LOCAL_CFLAGS := -pie -FPIE
LOCAL_MODULE := hilink_netsvc
include $(BUILD_EXECUTABLE)
