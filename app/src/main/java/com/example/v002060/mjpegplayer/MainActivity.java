package com.example.v002060.mjpegplayer;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.media.MediaPlayer;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;

import java.io.IOException;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity{
    static {
        System.loadLibrary("MjpegPlayer");
    }
    public native void testSurface(Surface surface, byte[] buffer);
    public native void startPlay(Surface surface, byte[] buffer);
    public native void startUvcPreview(String device, int width, int height);
    public native void stopUvcPreview();
    public native void startUvcDevice();
    public native void stopUvcDevice();

    public native int getGlobalAddress();
    public native void sendGlobalAdress(int address);

    private static final String TAG = "MainActivity";
    private String mSdPath;

    private Bitmap mInputBitmap;

    public GLSurfaceView view;
    Data image, testData;
    OpenGLNV21Renderer renderer;
    NativeOpenGLNV21Renderer nativeRenderer;

    private Button greenButton;

    private boolean RtspClientStartFlag = false;
    private RtspClient mRtspClient;

    /* UI */
    private ImageView imgvPreview;
    private SurfaceView videoSurfaceView;
    private SurfaceHolder surfaceHolder;
    private Surface surface;

    public byte[] mBuffer;

    /* For Player */
    private GLSurfaceView mPlayerView;
    private VideoRender mRenderer;
    private MediaPlayer mMediaPlayer;
    protected Resources mResources;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mSdPath = Environment.getExternalStorageDirectory().toString();

        videoSurfaceView = (SurfaceView)findViewById(R.id.preview);
        surfaceHolder = videoSurfaceView.getHolder();
        //surfaceHolder.setFormat(PixelFormat.RGBX_8888);
        surface = surfaceHolder.getSurface();

        mBuffer = new byte[3840*2160*3/2];

        mResources = getResources();
        mMediaPlayer = new MediaPlayer();

        try {
            Log.i(TAG, Environment.getExternalStorageDirectory().getPath() + "/demo.mp4");
            mMediaPlayer.setDataSource(Environment.getExternalStorageDirectory().getPath() + "/demo.mp4");
        } catch (Exception e) {
            Log.e(TAG, e.getMessage(), e);
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_test) {
            //sampleMediaCodec.play(this, surface);
            new Thread(new Runnable() {
                public void run() {
                    byte[] data = new byte[10];
                    testSurface(surface, data);
                    for(int i = 0; i < data.length; i++){
                        //Log.i(TAG, String.format("0x%02X", data[i]));
                    }
                    int address = getGlobalAddress();
                    sendGlobalAdress(address);
                }
            }).start();

            return true;
        }

        if(id == R.id.action_play){

            /*
            new Thread(new Runnable() {
                public void run() {
                    startPlay(surface, mBuffer); // Play MJPEG, decode by FFmpeg, need Surface
                }
            }).start();
            */
            mMediaPlayer.setLooping(true);
            mMediaPlayer.start(); // Play MP4, decode by HW, need GLSurface

            return true;
        }

        if(id == R.id.uvc_preview){
            new Thread(new Runnable() {
                public void run() {
                    startUvcPreview("/dev/video1", 1920, 1080);
                }
            }).start();


            new Thread(new Runnable() {
                public void run() {
                    startUvcDevice();
                }
            }).start();


            return true;
        }

        if(id == R.id.uvc_preview_stop){
            stopUvcDevice();
            stopUvcPreview();

            return true;
        }

        if(id == R.id.change_opengl){
            // Remove the title bar from the window.
            //this.requestWindowFeature(Window.FEATURE_NO_TITLE);

            // Make the windows into full screen mode.
            getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                    WindowManager.LayoutParams.FLAG_FULLSCREEN);
            setContentView(R.layout.main);

            // Create a OpenGL view.
            view = (GLSurfaceView) findViewById(R.id.surfaceView);
            view.setEGLContextClientVersion(2);

            // Creating and attaching the renderer.
            //image = readDataFromAssets("test_4k.yuv420p");
            int w = 3840;
            int h = 2160;
            //byte[] data = new byte[w * h * 3 / 2];
            //testData = new Data(w, h, data);
            //renderer = new OpenGLNV21Renderer(this, testData);
            nativeRenderer = new NativeOpenGLNV21Renderer();
            view.setRenderer(nativeRenderer);
            view.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);


            //greenButton = (Button)findViewById(R.id.button);
            //greenButton.setOnClickListener(this);
            //greenButton.setVisibility(View.INVISIBLE);

            //decodeCallback(data);

            return true;
        }

        if(id == R.id.change_pip){
            /*
            int GLViewWidth = view.getWidth();
            int GLViewHeight = view.getHeight();
            Log.i(TAG, "GLSurface Width = " + GLViewWidth + ", GLSurface Height = " + GLViewHeight);
            ViewGroup.LayoutParams lp = videoSurfaceView.getLayoutParams();
            lp.width = GLViewWidth/2;
            lp.height =GLViewHeight/2;
            view.setLayoutParams(lp);
            */

            int pipWidth = videoSurfaceView.getWidth()/2;
            int pipHeight = videoSurfaceView.getHeight()/2;
            Log.i(TAG, "PIP Width = " + pipWidth + ", PIP Height = " + pipHeight);

            setContentView(R.layout.pip);

            // Create a OpenGL view for UVC
            view = (GLSurfaceView) findViewById(R.id.PipGLSurfaceView);
            view.setEGLContextClientVersion(2);
            ViewGroup.LayoutParams lp = view.getLayoutParams();
            lp.width = pipWidth;
            lp.height = pipHeight;
            view.setLayoutParams(lp);
            nativeRenderer = new NativeOpenGLNV21Renderer();
            view.setRenderer(nativeRenderer);
            view.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

            // Set a SurfaceView for RTSP
            videoSurfaceView = (SurfaceView)findViewById(R.id.PipRtspView);
            lp = videoSurfaceView.getLayoutParams();
            lp.width = pipWidth;
            lp.height = pipHeight;
            videoSurfaceView.setLayoutParams(lp);

            mPlayerView = (GLSurfaceView) findViewById(R.id.PlayGLSurfaceView);
            mPlayerView.setEGLContextClientVersion(2);
            lp = mPlayerView.getLayoutParams();
            lp.width = pipWidth;
            lp.height = pipHeight;
            mPlayerView.setLayoutParams(lp);
            mRenderer = new VideoRender(mMediaPlayer);
            mPlayerView.setRenderer(mRenderer);

            return true;
        }

        if(id == R.id.play_rtsp){
            String host = "rtsp://192.168.0.130:8557/PSIA/Streaming/channels/2?videoCodecType=H.264";
            startRtspClient(host);
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    public Data readDataFromAssets(String fileName) {
        int w = 3840;
        int h = 2160;
        byte[] data = new byte[w * h * 3 / 2];
        try {
            InputStream is = getAssets().open(fileName);
            is.read(data);
            is.close();

            return new Data(w, h, data);
        } catch (IOException e) {
            Log.e("readDataFromAssets", "IoException:" + e.getMessage());
            e.printStackTrace();
        }

        return null;
    }

    /*
    @Override
    public void onClick(View v) {
        switch(v.getId()) {
            case R.id.button:
                //renderer.changeDataSoiurce(testData);
                int w = 3840;
                int h = 2160;
                byte[] data = new byte[w * h * 3 / 2];
                try {
                    InputStream is = getAssets().open("test_4k.yuv420p");
                    is.read(data);
                    is.close();
                    //testData.updateData(data);
                } catch (IOException e) {
                    e.printStackTrace();
                }
                //view.requestRender();
                Log.i("Button", "Green");

                new Thread(new Runnable() {
                    public void run() {
                        startPlay(surface, mBuffer);
                    }
                }).start();

                break;
        }
    }
    */

    public void decodeCallback(){
        //Log.i(TAG, "frameData.length = " + Integer.toString(frameData.length));

        /*
        for(int i = 0; i < frameData.length; i++){
            Log.i(TAG, String.format("0x%02X", frameData[i]));
        }
        */

        //testData.updateData(frameData);
        //long start = System.nanoTime();
        //view.requestRender();
        //long end = System.nanoTime();

        //Log.i(TAG, "onDrawFrame time is: " + (end - start)/1000000000.0);

        //testData.updateData(mBuffer);
        view.requestRender();
    }

    private void startRtspClient(String host){
        if(RtspClientStartFlag == false){
            mRtspClient = new RtspClient(host);
            mRtspClient.setSurfaceView(videoSurfaceView);
            mRtspClient.start();
            RtspClientStartFlag = true;
        }
        else{
            Log.e(TAG, "RTSP client already start!");
        }
    }

    private void closeRtspClient(){
        if(RtspClientStartFlag == true){
            mRtspClient.shutdown();
            RtspClientStartFlag = false;
        }
        else{
            Log.e(TAG, "RTSP client doesn't start!");
        }
    }
}
