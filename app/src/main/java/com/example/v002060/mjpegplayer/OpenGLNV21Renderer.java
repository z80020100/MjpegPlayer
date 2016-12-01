package com.example.v002060.mjpegplayer;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Created by CLIFF on 2016/8/25.
 */
public class OpenGLNV21Renderer implements GLSurfaceView.Renderer {
    private static final String TAG = "OpenGLNV21Renderer";

    static final float TEXTURE_NO_ROTATION[] = { 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, };
    static final float TEXTURE_ROTATED_90[] = { 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f, };
    static final float TEXTURE_ROTATED_180[] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, };
    static final float TEXTURE_ROTATED_270[] = { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            0.0f, 1.0f, 1.0f, };

    float textureCoordinate[] = TEXTURE_ROTATED_270;

    float vertices[] = new float[] { -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f,
            1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f };

    FloatBuffer mVerticesBuffer;
    FloatBuffer mTextureCoordinateBuffer;

    Context mContext;
    private int mWidth;
    private int mHeight;
    private Data mData;

    private int mProgramHandler;
    private int mPositionHandler;
    private int mTextureCoordinateHandler;
    private int mGLUniformTexture;
    private int mGLUniformUvTexture;
    private int mGLUniformWidthHandler;
    private int mGLUniformHeightHandler;

    private int mYTextureId;
    private int mUvTextureId;

    public OpenGLNV21Renderer(Context context, Data data) {
        mContext = context;
        mData = data;

        mVerticesBuffer = ByteBuffer.allocateDirect(vertices.length * 4)
                .order(ByteOrder.nativeOrder()).asFloatBuffer();
        mVerticesBuffer.put(vertices).position(0);

        mTextureCoordinateBuffer = ByteBuffer
                .allocateDirect(textureCoordinate.length * 4)
                .order(ByteOrder.nativeOrder()).asFloatBuffer();
        mTextureCoordinateBuffer.put(textureCoordinate).position(0);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        Log.i(TAG, "onSurfaceCreated");
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        Log.i(TAG, "onSurfaceChanged");

        GLES20.glViewport(0, 0, width, height);
        mWidth = width;
        mHeight = height;

        mProgramHandler = GLES20.glCreateProgram();
        if(mProgramHandler != 0){
            int vertexShaderHandle = GLES20.glCreateShader(GLES20.GL_VERTEX_SHADER);
            if(vertexShaderHandle != 0){
                // Load the shader
                GLES20.glShaderSource(vertexShaderHandle,
                        loadRawString(mContext, R.raw.filter_vs));
                // Compile the shader.
                GLES20.glCompileShader(vertexShaderHandle);
                // Get the compilation status.
                final int[] compileStatus = new int[1];
                GLES20.glGetShaderiv(vertexShaderHandle, GLES20.GL_COMPILE_STATUS,
                        compileStatus, 0);
                // If the compilation failed, delete the shader.
                if (compileStatus[0] == 0) {
                    Log.d("OpenGLES",
                            "Compilation:"
                                    + GLES20.glGetShaderInfoLog(vertexShaderHandle));
                    GLES20.glDeleteShader(vertexShaderHandle);
                    vertexShaderHandle = 0;
                }
            }
            else{
                Log.e(TAG, "Error: creating fragment shader");
            }

            int fragmentShaderHandle = GLES20.glCreateShader(GLES20.GL_FRAGMENT_SHADER);
            if(fragmentShaderHandle != 0){
                // Pass in the shader source.
                GLES20.glShaderSource(fragmentShaderHandle,
                        loadRawString(mContext, R.raw.filter_fs));
                // Compile the shader.
                GLES20.glCompileShader(fragmentShaderHandle);
                // Get the compilation status.
                final int[] compileStatus = new int[1];
                GLES20.glGetShaderiv(fragmentShaderHandle,
                        GLES20.GL_COMPILE_STATUS, compileStatus, 0);

                // If the compilation failed, delete the shader.
                if (compileStatus[0] == 0) {
                    Log.d("OpenGLES",
                            "Compilation:"
                                    + GLES20.glGetShaderInfoLog(fragmentShaderHandle));
                    GLES20.glDeleteShader(fragmentShaderHandle);
                    fragmentShaderHandle = 0;
                }
            }
            else{
                Log.e(TAG, "Error: creating fragment shader");
            }

            // Bind the vertex shader to the program.
            GLES20.glAttachShader(mProgramHandler, vertexShaderHandle);
            // Bind the fragment shader to the program.
            GLES20.glAttachShader(mProgramHandler, fragmentShaderHandle);

            GLES20.glBindAttribLocation(mProgramHandler, 0, "position");
            GLES20.glBindAttribLocation(mProgramHandler, 1,"inputTextureCoordinate");
            // Link the two shaders together into a program.
            GLES20.glLinkProgram(mProgramHandler);

            // Get the link status.
            final int[] linkStatus = new int[1];
            GLES20.glGetProgramiv(mProgramHandler, GLES20.GL_LINK_STATUS,
                    linkStatus, 0);
            // If the link failed, delete the program.
            if (linkStatus[0] == 0) {
                GLES20.glDeleteProgram(mProgramHandler);
                mProgramHandler = 0;
            }

            mPositionHandler = GLES20.glGetAttribLocation(mProgramHandler, "position");
            mTextureCoordinateHandler = GLES20.glGetAttribLocation(mProgramHandler, "inputTextureCoordinate");
            mGLUniformTexture = GLES20.glGetUniformLocation(mProgramHandler, "inputImageTexture");
            mGLUniformUvTexture = GLES20.glGetUniformLocation(mProgramHandler, "uvTexture");
            mGLUniformWidthHandler = GLES20.glGetUniformLocation(mProgramHandler, "width");
            mGLUniformHeightHandler = GLES20.glGetUniformLocation(mProgramHandler, "height");
        }
        else{
            Log.e(TAG, "Error: creating program");
        }

        int textures[] = new int[2];
        GLES20.glGenTextures(2, textures, 0);
        mYTextureId = textures[0];
        mUvTextureId = textures[1];

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mYTextureId);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, mData.w, mData.h, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, ByteBuffer.wrap(mData.y));

        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mUvTextureId);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D,
                GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D,
                GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S,
                GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T,
                GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE_ALPHA,
                mData.w / 2, mData.h / 2, 0, GLES20.GL_LUMINANCE_ALPHA,
                GLES20.GL_UNSIGNED_BYTE, ByteBuffer.wrap(mData.uv));

    }

    @Override
    public void onDrawFrame(GL10 gl) {
        //Log.i(TAG, "onDrawFrame");

        long start = System.nanoTime();

        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mYTextureId);
        //GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        //GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        //GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        //GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, mData.w, mData.h, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, ByteBuffer.wrap(mData.y));

        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mUvTextureId);
        //GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        //GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        //GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        //GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE_ALPHA, mData.w / 2, mData.h / 2, 0, GLES20.GL_LUMINANCE_ALPHA, GLES20.GL_UNSIGNED_BYTE, ByteBuffer.wrap(mData.uv));

        GLES20.glUseProgram(mProgramHandler);
        // Clears the screen and depth buffer.
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);

        // Draw our scene.
        GLES20.glVertexAttribPointer(mPositionHandler, 3, GLES20.GL_FLOAT,
                false, 0, mVerticesBuffer);
        GLES20.glEnableVertexAttribArray(mPositionHandler);
        GLES20.glVertexAttribPointer(mTextureCoordinateHandler, 2,
                GLES20.GL_FLOAT, false, 0, mTextureCoordinateBuffer);
        GLES20.glEnableVertexAttribArray(mTextureCoordinateHandler);
        GLES20.glUniform1f(mGLUniformWidthHandler, (float) mWidth);
        GLES20.glUniform1f(mGLUniformHeightHandler, (float) mHeight);

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mYTextureId);
        GLES20.glUniform1i(mGLUniformTexture, 0);

        GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mUvTextureId);
        GLES20.glUniform1i(mGLUniformUvTexture, 1);

        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        GLES20.glDisableVertexAttribArray(mPositionHandler);
        GLES20.glDisableVertexAttribArray(mTextureCoordinateHandler);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        long end = System.nanoTime();

        //Log.i(TAG, "onDrawFrame time is: " + (end - start)/1000000000.0);
    }

    public static String loadRawString(Context context, int rawId) {
        InputStream is = context.getResources().openRawResource(rawId);
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        try {
            byte[] buf = new byte[1024];
            int len = -1;
            while ((len = is.read(buf)) != -1) {
                baos.write(buf, 0, len);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return baos.toString();
    }

    public void changeDataSoiurce(Data newData){
        mData = newData;
    }
}
