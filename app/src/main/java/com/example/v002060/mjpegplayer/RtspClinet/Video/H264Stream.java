package com.example.v002060.mjpegplayer.RtspClinet.Video;

import android.annotation.TargetApi;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Base64;
import android.util.Log;
import android.view.SurfaceView;
import android.view.ViewStub;

import com.example.v002060.mjpegplayer.RtspClient;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.LinkedBlockingDeque;

/**
 *
 */
public class H264Stream extends VideoStream {
    private final static String tag = "H264Stream";

    private MediaCodec mMeidaCodec;
    private SurfaceView mSurfaceView;
    private ByteBuffer[] inputBuffers;
    private ByteBuffer[] outputBuffers;
    private Handler mHandler;
    private LinkedBlockingDeque<byte[]> bufferQueue = new LinkedBlockingDeque<>();
    private int picWidth,picHeight;
    byte[] header_sps,header_pps;
    private boolean isStop;
    private HandlerThread thread;
    RtspClient.SDPInfo mSDPinfo;

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)

    public H264Stream(RtspClient.SDPInfo sp) {
        mSDPinfo = sp;
        thread = new HandlerThread("H264StreamThread");
        thread.start();
        mHandler = new Handler(thread.getLooper());
        if(sp.SPS != null) decodeSPS();
    }

    private void configMediaDecoder(){
        if(Build.VERSION.SDK_INT > 15) {
            try {
                mMeidaCodec = MediaCodec.createDecoderByType("video/avc");
/*
                // for MTK H.264 decoder
                MediaFormat mediaFormat = new MediaFormat();
                mediaFormat.setString(MediaFormat.KEY_MIME, "video/avc");
                mediaFormat.setByteBuffer("csd-0", ByteBuffer.wrap(header_sps));
                mediaFormat.setByteBuffer("csd-1", ByteBuffer.wrap(header_pps));
                //System.out.println(Arrays.toString(header_sps));
                //System.out.println(Arrays.toString(header_pps));
                mediaFormat.setInteger(MediaFormat.KEY_WIDTH, picWidth);
                mediaFormat.setInteger(MediaFormat.KEY_HEIGHT, picHeight);
*/

                MediaFormat mediaFormat = MediaFormat.createVideoFormat("video/avc", picWidth, picHeight);
                mediaFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 0);
                mMeidaCodec.configure(mediaFormat, mSurfaceView.getHolder().getSurface(), null, 0);
                mMeidaCodec.start();
                inputBuffers = mMeidaCodec.getInputBuffers();
                outputBuffers = mMeidaCodec.getOutputBuffers();
                isStop = false;
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }


    public void startMediaHardwareDecode() {
        mHandler.post(hardwareDecodeThread);
    }

    private Runnable hardwareDecodeThread = new Runnable() {
        @Override
        public void run() {
            int mCount = 0;
            int inputBufferIndex,outputBufferIndex;
            MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
            byte[] tmpByte;
            int framType;
            boolean startKeyFrame = false;

            configMediaDecoder();
            /*
            File RTP_RAW = new File(Environment.getExternalStorageDirectory(), "RTP_RAW");
            FileOutputStream fos = null;
            if (RTP_RAW.exists()) {
                RTP_RAW.delete();
            }
            try {
                fos = new FileOutputStream(RTP_RAW.getPath());
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            }
            */
            byte[] nal_header = {0x00, 0x00, 0x00, 0x01};
            byte[] nal_sps = new byte[nal_header.length + header_sps.length];
            byte[] nal_pps = new byte[nal_header.length + header_pps.length];

            for(int i = 0; i< nal_sps.length; i++){
                if(i < 4){
                    nal_sps[i] = nal_header[i];
                }
                else{
                    nal_sps[i] = header_sps[i-4];
                }
            }
            Log.i(tag, "Feed mRawSPS = " + byte2Hex(nal_sps));

            for(int i = 0; i< nal_pps.length; i++){
                if(i < 4){
                    nal_pps[i] = nal_header[i];
                }
                else{
                    nal_pps[i] = header_pps[i-4];
                }
            }
            Log.i(tag, "Feed mRawPPS = " + byte2Hex(nal_pps));

            // Feed SPS and PPS to decoder
            Log.i(tag, "Feed SPS");
            inputBufferIndex = mMeidaCodec.dequeueInputBuffer(-1);
            if (inputBufferIndex >= 0) {
                ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
                inputBuffer.clear();
                inputBuffer.put(nal_sps);
                mMeidaCodec.queueInputBuffer(inputBufferIndex, 0, nal_sps.length, mCount, 0);
                outputBufferIndex = mMeidaCodec.dequeueOutputBuffer(info, 0);
                //outputBufferIndex = mMeidaCodec.dequeueOutputBuffer(info, 1000000l);
                //Log.i("Decode", "outputBufferIndex = " + Integer.toString(outputBufferIndex));
                switch (outputBufferIndex) {
                    case MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED:
                        break;
                    case MediaCodec.INFO_OUTPUT_FORMAT_CHANGED:
                        break;
                    case MediaCodec.INFO_TRY_AGAIN_LATER:
                        break;
                    default:
                        mMeidaCodec.releaseOutputBuffer(outputBufferIndex, false);
                        break;
                }
                mCount++;
            }
            Log.i(tag, "Feed PPS");
            inputBufferIndex = mMeidaCodec.dequeueInputBuffer(-1);
            if (inputBufferIndex >= 0) {
                ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
                inputBuffer.clear();
                inputBuffer.put(nal_pps);
                mMeidaCodec.queueInputBuffer(inputBufferIndex, 0, nal_pps.length, mCount, 0);
                outputBufferIndex = mMeidaCodec.dequeueOutputBuffer(info, 0);
                //outputBufferIndex = mMeidaCodec.dequeueOutputBuffer(info, 1000000l);
                //Log.i("Decode", "outputBufferIndex = " + Integer.toString(outputBufferIndex));
                switch (outputBufferIndex) {
                    case MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED:
                        break;
                    case MediaCodec.INFO_OUTPUT_FORMAT_CHANGED:
                        break;
                    case MediaCodec.INFO_TRY_AGAIN_LATER:
                        break;
                    default:
                        mMeidaCodec.releaseOutputBuffer(outputBufferIndex, false);
                        break;
                }
                mCount++;
            }

            while (!Thread.interrupted() && !isStop) {
                try {
                    tmpByte = bufferQueue.take();
                    //System.out.println(Arrays.toString(tmpByte));
                    framType = tmpByte[4]&0x1F;
                    //Log.i("RAW", byte2Hex(tmpByte));
                    /*
                    try {
                        fos.write(tmpByte);
                        fos.flush();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    */
                    if(framType == 5/*IDR*/) startKeyFrame = true;
                    if(startKeyFrame || framType == 7/*SPS*/ || framType == 8/*PPS*/) {
                    //if(framType == 5 || framType == 7/*SPS*/ || framType == 8/*PPS*/) {
                        //Log.i(tag, "frameType = " + Integer.toString(framType));
                        /*
                        if(framType == 7){
                            Log.i(tag, "RTP mRawSPS = " + byte2Hex(tmpByte)); // only for debug
                        }
                        if(framType == 8){
                            Log.i(tag, "RTP mRawPPS = " + byte2Hex(tmpByte)); // only for debug
                        }
                        */
                        //inputBufferIndex = mMeidaCodec.dequeueInputBuffer(1000*1000);
                        //inputBufferIndex = mMeidaCodec.dequeueInputBuffer(-1);
                        inputBufferIndex = mMeidaCodec.dequeueInputBuffer(33*1000);
                        // TODO: solve timeout issue on RK3288
                        //Log.i("Decode", "inputBufferIndex = " + Integer.toString(inputBufferIndex));
/*
                        try {
                            fos.write(tmpByte);
                            fos.flush();
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
*/
                        if (inputBufferIndex >= 0) {
                            ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
                            inputBuffer.clear();
                            inputBuffer.put(tmpByte);
                            mMeidaCodec.queueInputBuffer(inputBufferIndex, 0, tmpByte.length, mCount, 0);
                            outputBufferIndex = mMeidaCodec.dequeueOutputBuffer(info, 0);
                            //outputBufferIndex = mMeidaCodec.dequeueOutputBuffer(info, 1000000l);
                            //Log.i("Decode", "outputBufferIndex = " + Integer.toString(outputBufferIndex));
                            switch (outputBufferIndex) {
                                case MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED:
                                    break;
                                case MediaCodec.INFO_OUTPUT_FORMAT_CHANGED:
                                    break;
                                case MediaCodec.INFO_TRY_AGAIN_LATER:
                                    Log.e(tag, "dequeueOutputBuffer timeout");
                                    break;
                                default:
                                    mMeidaCodec.releaseOutputBuffer(outputBufferIndex, true);
                                    break;
                            }
                            mCount++;
                        }
                        else{
                            Log.e(tag, "dequeueInputBuffer timeout");
                        }
                    }
                } catch (InterruptedException e) {
                    Log.e(tag,"Wait the buffer come..");
                }
            }
            bufferQueue.clear();
            mMeidaCodec.stop();
            if(mMeidaCodec == null){
                Log.e(tag, "mMeidaCodec = null before mMeidaCodec.release()");
            }
            else{
                mMeidaCodec.release();
            }

            mMeidaCodec = null;
        }
    };

    public void stop(){
        bufferQueue.clear();
        isStop = true;
        try {
            Thread.sleep(100);
        } catch (InterruptedException e) {}
        mHandler.removeCallbacks(hardwareDecodeThread);
        if(mMeidaCodec != null) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
                //mMeidaCodec.stop();
                //mMeidaCodec.release();
            }
            //mMeidaCodec = null;
        }
        super.stop();
        thread.quit();
        mSurfaceView.setVisibility(ViewStub.GONE);
        mSurfaceView.setVisibility(ViewStub.VISIBLE);
    }

    /* This method is used to decode pic width and height from the sps info,
     * which got from the RTSP DESCRIPE request, SDP info.
     */
    private void decodeSPS(){
        int i,offset = 32;
        int pic_width_len,pic_height_len;
        byte[] sps = Base64.decode(mSDPinfo.SPS, Base64.NO_WRAP);
        byte[] pps = Base64.decode(mSDPinfo.PPS, Base64.NO_WRAP);
        int profile_idc = sps[1];
        header_pps = new byte[pps.length];
        header_sps = new byte[sps.length];
        System.arraycopy(sps,0,header_sps,0,sps.length);
        System.arraycopy(pps,0,header_pps,0,pps.length);

        //Log.i("Decode", "SPS = " + byte2Hex(header_sps)); // only for debug
        //Log.i("Decode", "PPS = " + byte2Hex(header_pps)); // only for debug

        offset += getUeLen(sps,offset);//jump seq_parameter_set_id
        if(profile_idc == 100 || profile_idc == 110 || profile_idc == 122
                || profile_idc == 144) {
            int chroma_format_idc = (getUeLen(sps,offset) == 1)?0:
                    ( sps[(offset+getUeLen(sps,offset))/8] >>
                            (7-((offset+getUeLen(sps,offset))%8)) );
            offset += getUeLen(sps,offset);//jump chroma_format_idc
            if(chroma_format_idc == 3)
                offset++; //jump residual_colour_transform_flag
            offset += getUeLen(sps,offset);//jump bit_depth_luma_minus8
            offset += getUeLen(sps,offset);//jump bit_depth_chroma_minus8
            offset ++; //jump qpprime_y_zero_transform_bypass_flag
            int seq_scaling_matrix_present_flag = (sps[offset/8] >> (8-(offset%8)))&0x01;
            if(seq_scaling_matrix_present_flag == 1) offset += 8; //jump seq_scaling_list_present_flag
        }
        offset += getUeLen(sps,offset);//jump log2_max_frame_num_minus4
        int pic_order_cnt_type = (getUeLen(sps,offset) == 1)?0:
                ( sps[(offset+getUeLen(sps,offset))/8] >>
                        (7-((offset+getUeLen(sps,offset))%8)) );
        offset += getUeLen(sps,offset);
        if(pic_order_cnt_type == 0) {
            offset += getUeLen(sps,offset);
        }
        else if(pic_order_cnt_type == 1) {
            offset++; //jump delta_pic_order_always_zero_flag
            offset += getUeLen(sps,offset); //jump offset_for_non_ref_pic
            offset += getUeLen(sps,offset); //jump offset_for_top_to_bottom_field
            int num_ref_frames_inpic_order_cnt_cycle = ( sps[(offset+getUeLen(sps,offset))/8] >>
                    (7-((offset+getUeLen(sps,offset))%8)) );
            for(i=0; i<num_ref_frames_inpic_order_cnt_cycle; ++i)
                offset += getUeLen(sps,offset); //jump ref_frames_inpic_order
        }
        offset += getUeLen(sps,offset); // jump num_ref_frames
        offset++; // jump gaps_in_fram_num_value_allowed_flag

        pic_width_len = getUeLen(sps,offset);
        //picWidth = (getByteBit(sps,offset+pic_width_len/2,pic_width_len/2+1)+1)*16;
        offset += pic_width_len;
        pic_height_len = getUeLen(sps,offset);
        //picHeight = (getByteBit(sps,offset+pic_height_len/2,pic_height_len/2+1)+1)*16;
        //Log.i("decodeSPS", "The picWidth = " + picWidth + " ,the picHeight = " + picHeight);

        ParameterSetPaser mH264Parser;
        mH264Parser = new ParameterSetPaser(header_sps, header_pps);
        mH264Parser.parse();

        picWidth = mH264Parser.getWidth();
        picHeight =  mH264Parser.getHeight();
        Log.i(tag, "Width = " + picWidth);
        Log.i(tag, "Height = " + picHeight);
        if(picWidth == 1912 && picHeight == 1088){ // for 1080P on F70W and F50-8M
            // TODO: check correctness of parameter set parser
            picWidth = 1920;
            picHeight = 1080;
            Log.i(tag, "Detect SPS resolution = 1912 * 1088, reset resolution = 1920 * 1080");
        }

    }

    private int getUeLen(byte[] bytes, int offset) {
        int zcount = 0;
        while(true) {
            if(( ( bytes[offset/8] >> (7-(offset%8)) ) & 0x01 ) == 0) {
                offset ++;
                zcount ++;
            }
            else break;
        }
        return zcount * 2 + 1;
    }

    /*
     * This method is get the bit[] from a byte[]
     * It may have a more efficient way
     */
    public int getByteBit(byte[] bytes, int offset, int len){
        int tmplen = len/8+ ((len%8+offset%8)>8?1:0) + ((offset%8 == 0)?0:1);
        int lastByteZeroNum = ((len%8+offset%8-8)>0)?(16-len%8-offset%8):(8-len%8-offset%8);
        int data = 0;
        byte tmpC = (byte) (0xFF >> (8 - lastByteZeroNum));
        byte[] tmpB = new byte[tmplen];
        byte[] tmpA = new byte[tmplen];
        int i;
        for(i = 0;i<tmplen;++i) {
            if(i == 0) tmpB[i] = (byte) (bytes[offset/8] << (offset%8) >> (offset%8));
            else if(i+1 == tmplen) tmpB[i] = (byte) ((bytes[offset/8+i] & 0xFF) >> lastByteZeroNum);
            else tmpB[i] = bytes[offset/8+i];
            tmpA[i] = (byte) ((tmpB[i] & tmpC)<<(8-lastByteZeroNum));
            if(i+1 != tmplen && i != 0) {
                tmpB[i] = (byte) ((tmpB[i]&0xFF) >> lastByteZeroNum);
                tmpB[i] = (byte) (tmpB[i] | tmpA[i-1]);
            }
            else if(i == 0) tmpB[0] = (byte) ((tmpB[0]&0xFF) >> lastByteZeroNum);
            else tmpB[i] = (byte) (tmpB[i] | tmpA[i-1]);
            data = ((tmpB[i]&0xFF) << ((tmplen-i-1)*8)) | data ;
        }
        return data;
    }

    public int[] getPicInfo(){
        return new int[]{picWidth, picHeight};
    }

    public void setSurfaceView(SurfaceView s) {
        this.mSurfaceView = s;
        if(Build.VERSION.SDK_INT > 15) {
            startMediaHardwareDecode();
        } else {
            Log.e(tag,"The Platform not support the hardware decode H264!");
        }
    }

    @Override
    protected void decodeH264Stream() {
        try {
            bufferQueue.put(NALUnit);
        } catch (InterruptedException e) {
            Log.e(tag,"The buffer queue is full , wait for the place..");
        }
    }

    public String byte2Hex(byte[] b) {
        String result = "";
        for (int i = 0 ; i < b.length ; i++) {
            result += "0x";
            result += Integer.toString((b[i] & 0xff) + 0x100, 16).substring(1);
            result += " ";
        }
        return result;
    }
}
