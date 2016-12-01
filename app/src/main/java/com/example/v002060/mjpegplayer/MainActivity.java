package com.example.v002060.mjpegplayer;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.ImageView;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("MjpegPlayer");
    }
    public native void testSurface(Surface surface);
    public native void startPlay(Surface surface);

    private String mSdPath;

    private Bitmap mInputBitmap;

    /* UI */
    private ImageView imgvPreview;
    private SurfaceView videoSurfaceView;
    private SurfaceHolder surfaceHolder;
    private Surface surface;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mSdPath = Environment.getExternalStorageDirectory().toString();

        videoSurfaceView = (SurfaceView)findViewById(R.id.preview);
        surfaceHolder = videoSurfaceView.getHolder();
        //surfaceHolder.setFormat(PixelFormat.RGBX_8888);
        surface = surfaceHolder.getSurface();
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
            testSurface(surface);
            return true;
        }

        if(id == R.id.action_play){
            startPlay(surface);
            return true;
        }

        return super.onOptionsItemSelected(item);
    }
}
