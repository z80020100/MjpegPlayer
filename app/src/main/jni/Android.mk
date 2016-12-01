LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := MjpegPlayer
LOCAL_SRC_FILES := mjpeg_player.c
LOCAL_LDLIBS += -llog -landroid
LOCAL_SHARED_LIBRARIES := libavformat libavcodec libswscale libavutil libwsresample

include $(BUILD_SHARED_LIBRARY)
$(call import-module,ffmpeg-3.0.2/android/armeabi-v7a)