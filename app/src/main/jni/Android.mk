LOCAL_PATH := $(call my-dir)

#### According to prebuilt FFmpeg library in directory "prebuilt" ####
CPU := arm
ANDROID_API := android-19
LOCAL_MODULE_PATH := $(LOCAL_PATH)/prebuilt/ffmpeg-3.0.2/$(ANDROID_API)/$(CPU)

include $(CLEAR_VARS)
LOCAL_MODULE:= libavcodec
LOCAL_SRC_FILES:= $(LOCAL_MODULE_PATH)/lib/libavcodec-57.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_MODULE_PATH)/include
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:= libavformat
LOCAL_SRC_FILES:= $(LOCAL_MODULE_PATH)/lib/libavformat-57.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_MODULE_PATH)/include
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:= libswscale
LOCAL_SRC_FILES:= $(LOCAL_MODULE_PATH)/lib/libswscale-4.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_MODULE_PATH)/include
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:= libavutil
LOCAL_SRC_FILES:= $(LOCAL_MODULE_PATH)/lib/libavutil-55.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_MODULE_PATH)/include
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:= libavfilter
LOCAL_SRC_FILES:= $(LOCAL_MODULE_PATH)/lib/libavfilter-6.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_MODULE_PATH)/include
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:= libwsresample
LOCAL_SRC_FILES:= $(LOCAL_MODULE_PATH)/lib/libswresample-2.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_MODULE_PATH)/include
include $(PREBUILT_SHARED_LIBRARY)
#######################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := MjpegPlayer
LOCAL_SRC_FILES := mjpeg_player.c opengl_nv21_renderer.c uvc_host.c uvc_device.c
LOCAL_LDLIBS += -llog -landroid -lGLESv2 -lEGL
LOCAL_SHARED_LIBRARIES := libavformat libavcodec libswscale libavutil libwsresample
LOCAL_C_INCLUDES := include
include $(BUILD_SHARED_LIBRARY)

#### Use this when putting your prebuilt FFmpeg library on Android NDK ####
#$(call import-module, ffmpeg-3.0.2/android-19/arm) # for API 19 (Android 4.4.X)
############################################################################