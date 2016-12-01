package com.example.v002060.mjpegplayer;

import android.graphics.Bitmap;
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
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;

import java.io.IOException;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity implements View.OnClickListener{
    static {
        System.loadLibrary("MjpegPlayer");
    }
    public native void testSurface(Surface surface, byte[] buffer);
    public native void startPlay(Surface surface, byte[] buffer);

    private static final String TAG = "MainActivity";
    private String mSdPath;

    private Bitmap mInputBitmap;

    public GLSurfaceView view;
    Data image, testData;
    OpenGLNV21Renderer renderer;
    NativeOpenGLNV21Renderer nativeRenderer;

    private Button greenButton;

    /* UI */
    private ImageView imgvPreview;
    private SurfaceView videoSurfaceView;
    private SurfaceHolder surfaceHolder;
    private Surface surface;

    public byte[] mBuffer;

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
            byte[] data = new byte[10];
            testSurface(surface, data);
            for(int i = 0; i < data.length; i++){
                Log.i(TAG, String.format("0x%02X", data[i]));
            }

            return true;
        }

        if(id == R.id.action_play){
            //startPlay(surface);
            return true;
        }

        if(id == R.id.change_opengl){
            setContentView(R.layout.main);

            // Create a OpenGL view.
            view = (GLSurfaceView) findViewById(R.id.surfaceView);
            view.setEGLContextClientVersion(2);

            // Creating and attaching the renderer.
            image = readDataFromAssets("test_4k.yuv420p");
            int w = 3840;
            int h = 2160;
            byte[] data = new byte[w * h * 3 / 2];
            testData = new Data(w, h, data);
            //renderer = new OpenGLNV21Renderer(this, testData);
            nativeRenderer = new NativeOpenGLNV21Renderer();
            view.setRenderer(nativeRenderer);
            view.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);


            greenButton = (Button)findViewById(R.id.button);
            greenButton.setOnClickListener(this);

            //decodeCallback(data);

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
                startPlay(surface, mBuffer);
                break;
        }
    }

    public void decodeCallback(byte[] frameData){
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
}
