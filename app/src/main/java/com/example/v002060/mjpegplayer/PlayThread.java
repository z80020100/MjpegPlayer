package com.example.v002060.mjpegplayer;

/**
 * Created by CLIFF on 2016/8/30.
 */
public class PlayThread extends Thread{
    static {
        System.loadLibrary("MjpegPlayer");
    }


    private static final String TAG = "PlayThread";

    @Override
    public void run() {

    }
}
