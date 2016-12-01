package com.example.v002060.mjpegplayer.RtspClinet.Audio;

import com.example.v002060.mjpegplayer.RtspClinet.Stream.RtpStream;

/**
 *
 */
public abstract class AudioStream extends RtpStream {
    private final static String tag = "AudioStream";

    protected void recombinePacket(RtpStream.StreamPacks streamPacks) {

    }
}
