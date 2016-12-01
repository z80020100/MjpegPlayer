LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := MjpegPlayer
LOCAL_SRC_FILES := mjpeg_player.c opengl_nv21_renderer.c uvc_host.c
LOCAL_LDLIBS += -llog -landroid -lGLESv2 -lEGL
LOCAL_SHARED_LIBRARIES := libavformat libavcodec libswscale libavutil libwsresample
LOCAL_C_INCLUDES := include

include $(BUILD_SHARED_LIBRARY)
#$(call import-module, ffmpeg-3.0.2/android/armeabi-v7a) # for API 21 (Android 5.X)
$(call import-module, ffmpeg-3.0.2/android-19/arm) # for API 19 (Android 4.4.X)