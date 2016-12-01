package com.example.v002060.mjpegplayer;

import android.opengl.GLSurfaceView;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Created by CLIFF on 2016/8/29.
 */
public class NativeOpenGLNV21Renderer implements GLSurfaceView.Renderer {
    static {
        System.loadLibrary("MjpegPlayer");
    }
    public native void nativeOnSurfaceCreated(int width, int height);
    public native void nativeOnSurfaceChanged(int width, int height);
    public native void nativeOnDrawFrame();

    private static final String TAG = "NativeOpenGLNV21Renderer";

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        nativeOnSurfaceCreated(3840, 2160);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        nativeOnSurfaceChanged(width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        nativeOnDrawFrame();
    }
}
