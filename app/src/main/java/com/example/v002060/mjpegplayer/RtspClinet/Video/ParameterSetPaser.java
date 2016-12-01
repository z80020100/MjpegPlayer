package com.example.v002060.mjpegplayer.RtspClinet.Video;

import android.util.Log;

/**
 * Created by CLIFF on 2016/11/7.
 */
public class ParameterSetPaser {
    private final String TAG = this.getClass().getSimpleName();

    private rawParameterSet mRawSPS,mRawPPS;
    public AnalyzedSPS SPS;

    public ParameterSetPaser(byte[] sps, byte[] pps){
        mRawSPS = new rawParameterSet(sps);
        mRawPPS = new rawParameterSet(pps);

        Log.i(TAG, "mRawSPS = " + byte2Hex(mRawSPS.raw)); // only for debug
        Log.i(TAG, "mRawPPS = " + byte2Hex(mRawPPS.raw)); // only for debug
    }

    public void parse(){
        /*
        String binaryString = "";
        for(int i = 0; i < 32; i++){
            binaryString = binaryString + String.valueOf(ReadBit(mRawSPS));
            if((i+1) % 4 == 0){
                binaryString = binaryString + " ";
            }
        }
        Log.i(TAG, binaryString); // Read 32 bits to test ReadBit()
        */

        //Log.i(TAG, String.valueOf(ReadBits(mRawSPS, 8))); // Read the first byte to test ReadBits()

        SPS = new AnalyzedSPS();
        // Parse NAL parameter
        SPS.forbidden_zero_bit = ReadBits(mRawSPS, 1);
        SPS.nal_ref_idc = ReadBits(mRawSPS, 2);
        SPS.nal_unit_type = ReadBits(mRawSPS, 5);

        // Parse SPS
        SPS.profile_idc = ReadBits(mRawSPS, 8);
        SPS.constraint_set0_flag = ReadBits(mRawSPS, 1);
        SPS.constraint_set1_flag = ReadBits(mRawSPS, 1);
        SPS.constraint_set2_flag = ReadBits(mRawSPS, 1);
        SPS.constraint_set3_flag = ReadBits(mRawSPS, 1);
        SPS.constraint_set4_flag = ReadBits(mRawSPS, 1);
        SPS.constraint_set5_flag = ReadBits(mRawSPS, 1);
        SPS.reserved_zero_2bits = ReadBits(mRawSPS, 2);
        SPS.level_idc = ReadBits(mRawSPS, 8);
        SPS.seq_parameter_set_id = ReadExponentialGolombCode(mRawSPS);

        if (SPS.profile_idc == 100 ||  // High profile (F70W)
            SPS.profile_idc == 110 ||  // High10 profile
            SPS.profile_idc == 122 ||  // High422 profile
            SPS.profile_idc == 244 ||  // High444 Predictive profile
            SPS.profile_idc ==  44 ||  // Cavlc444 profile
            SPS.profile_idc ==  83 ||  // Scalable Constrained High profile (SVC)
            SPS.profile_idc ==  86 ||  // Scalable High Intra profile (SVC)
            SPS.profile_idc == 118 ||  // Stereo High profile (MVC)
            SPS.profile_idc == 128 ||  // Multiview High profile (MVC)
            SPS.profile_idc == 138 ||  // Multiview Depth High profile (MVCD)
            SPS.profile_idc == 144) {  // old High444 profile


            SPS.chroma_format_idc = ReadExponentialGolombCode(mRawSPS);
            if(SPS.chroma_format_idc == 3 )
            {
                SPS.residual_colour_transform_flag = ReadBits(mRawSPS, 1);
            }
            SPS.bit_depth_luma_minus8 = ReadExponentialGolombCode(mRawSPS);
            SPS.bit_depth_chroma_minus8 = ReadExponentialGolombCode(mRawSPS);
            SPS.qpprime_y_zero_transform_bypass_flag = ReadBit(mRawSPS);
            SPS.seq_scaling_matrix_present_flag = ReadBits(mRawSPS, 1);
            if(SPS.seq_scaling_matrix_present_flag != 0)
            {
                for (int i = 0; i < 8; i++)
                {
                    int seq_scaling_list_present_flag = ReadBits(mRawSPS, 1);
                    if (seq_scaling_list_present_flag != 0)
                    {
                        int sizeOfScalingList = (i < 6) ? 16 : 64;
                        int lastScale = 8;
                        int nextScale = 8;
                        int j=0;
                        for ( j = 0; j < sizeOfScalingList; j++)
                        {
                            if (nextScale != 0)
                            {
                                int delta_scale = ReadSE(mRawSPS);
                                nextScale = (lastScale + delta_scale + 256) % 256;
                            }
                            lastScale = (nextScale == 0) ? lastScale : nextScale;
                        }
                    }
                }
            }
        }
        else{ // Baseline profile: 66
            SPS.chroma_format_idc = 1;
            SPS.bit_depth_luma_minus8    = 8;
            SPS.bit_depth_chroma_minus8  = 8;
        }

        SPS.log2_max_frame_num_minus4 = ReadExponentialGolombCode(mRawSPS);
        SPS.pic_order_cnt_type = ReadExponentialGolombCode(mRawSPS);
        if( SPS.pic_order_cnt_type == 0 )
        {
            SPS.log2_max_pic_order_cnt_lsb_minus4 = ReadExponentialGolombCode(mRawSPS);
        }
        else if( SPS.pic_order_cnt_type == 1 ){
            SPS.delta_pic_order_always_zero_flag = ReadBits(mRawSPS, 1);
            SPS.offset_for_non_ref_pic = ReadSE(mRawSPS);
            SPS.offset_for_top_to_bottom_field = ReadSE(mRawSPS);
            SPS.num_ref_frames_in_pic_order_cnt_cycle = ReadExponentialGolombCode(mRawSPS);
            for(int i = 0; i < SPS.num_ref_frames_in_pic_order_cnt_cycle; i++ )
            {
                ReadSE(mRawSPS);
            }
        }
        else{
            if(SPS.pic_order_cnt_type != 2){
                Log.e(TAG, "Illegal pic_order_cnt_type = " + SPS.pic_order_cnt_type);
            }
        }

        SPS.num_ref_frames = ReadExponentialGolombCode(mRawSPS);
        SPS.gaps_in_frame_num_value_allowed_flag = ReadBit(mRawSPS);
        SPS.pic_width_in_mbs_minus1 = ReadExponentialGolombCode(mRawSPS);
        SPS.pic_height_in_map_units_minus1 = ReadExponentialGolombCode(mRawSPS);
        SPS.frame_mbs_only_flag = ReadBits(mRawSPS, 1);
        if( SPS.frame_mbs_only_flag == 0 )
        {
            SPS.mb_adaptive_frame_field_flag = ReadBits(mRawSPS, 1);
        }
        else{
            SPS.mb_adaptive_frame_field_flag = 0;
        }

        SPS.direct_8x8_inference_flag = ReadBits(mRawSPS, 1);
        SPS.frame_cropping_flag = ReadBits(mRawSPS, 1);
        if( SPS.frame_cropping_flag != 0 )
        {
            SPS.frame_crop_left_offset = ReadExponentialGolombCode(mRawSPS);
            SPS.frame_crop_right_offset = ReadExponentialGolombCode(mRawSPS);
            SPS.frame_crop_top_offset = ReadExponentialGolombCode(mRawSPS);
            SPS.frame_crop_bottom_offset = ReadExponentialGolombCode(mRawSPS);
        }
        else{
            SPS.frame_crop_left_offset = 0;
            SPS.frame_crop_right_offset = 0;
            SPS.frame_crop_top_offset = 0;
            SPS.frame_crop_bottom_offset = 0;
        }
        SPS.vui_parameters_present_flag = ReadBit(mRawSPS);

        SPS.print();
    }

    public int getWidth(){
        return SPS.getWidth();
    }

    public int getHeight(){
        return SPS.getHeight();
    }

    private int ReadBit(rawParameterSet rawParameterObj) // 讀出currentBit所在位置的bit value, currentBit = 0 for the first bit
    {
        int nIndex = rawParameterObj.currentBit / 8;
        int nOffset = rawParameterObj.currentBit % 8 + 1;
        rawParameterObj.currentBit++;
        //Log.i(TAG, "nIndex = " + nIndex + ", nOffect = " + nOffset);
        return (rawParameterObj.raw[nIndex] >> (8-nOffset)) & 0x01;
    }

    private int ReadBits(rawParameterSet rawParameterObj, int n) // 從讀出currentBit所在位置的bit讀出後n個bit之值
    {
        int r = 0;
        int i;
        for (i = 0; i < n; i++)
        {
            r |= ( ReadBit(rawParameterObj) << ( n - i - 1 ) );
        }
        return r;
    }

    int ReadExponentialGolombCode(rawParameterSet rawParameterObj)
    {
        int r = 0;
        int i = 0;

        while( (ReadBit(rawParameterObj) == 0) && (i < 32) )
        {
            i++;
        }

        r = ReadBits(rawParameterObj, i);
        r += (1 << i) - 1;
        return r;
    }

    int ReadSE(rawParameterSet rawParameterObj)
    {
        int r = ReadExponentialGolombCode(rawParameterObj);
        if ((r & 0x01) != 0x00)
        {
            r = (r+1)/2;
        }
        else
        {
            r = -(r/2);
        }
        return r;
    }

    class rawParameterSet {
        public byte[] raw;
        public int currentBit;

        public rawParameterSet(byte[] parameterArray){
            raw = parameterArray;
            currentBit = 0;
        }
    }

    class AnalyzedSPS { // sequence parameter set
        /* NAL */
        public int forbidden_zero_bit = -1; // u(1), should be 0
        public int nal_ref_idc = -1; // u(2), 3 means it is "important" (this is an SPS)
        public int nal_unit_type = -1; // u(5), 7 for SPS

        /* SPS */
        public int profile_idc = -1;
        public int constraint_set0_flag = -1;
        public int constraint_set1_flag = -1;
        public int constraint_set2_flag = -1;
        public int constraint_set3_flag = -1;
        public int constraint_set4_flag = -1;
        public int constraint_set5_flag = -1;
        public int reserved_zero_2bits = -1;
        public int level_idc = -1;
        public int seq_parameter_set_id = -1;

        int chroma_format_idc = -1;
        int residual_colour_transform_flag = -1;
        int bit_depth_luma_minus8 = -1;
        int bit_depth_chroma_minus8 = -1;
        int qpprime_y_zero_transform_bypass_flag = -1;
        int seq_scaling_matrix_present_flag = -1;

        int log2_max_frame_num_minus4 = -1;
        int pic_order_cnt_type = -1; // must be 0 or 1 or 2
        int log2_max_pic_order_cnt_lsb_minus4 = -1; // has value if pic_order_cnt_type == 0
        int delta_pic_order_always_zero_flag = -1; // has value if pic_order_cnt_type == 1
        int offset_for_non_ref_pic = -1; // has value if pic_order_cnt_type == 1
        int offset_for_top_to_bottom_field = -1; // has value if pic_order_cnt_type == 1
        int num_ref_frames_in_pic_order_cnt_cycle = -1; // has value if pic_order_cnt_type == 1

        int num_ref_frames = -1;
        int gaps_in_frame_num_value_allowed_flag = -1;
        int pic_width_in_mbs_minus1 = -1;
        int pic_height_in_map_units_minus1 = -1;
        int frame_mbs_only_flag = -1;
        int mb_adaptive_frame_field_flag = -1;

        int direct_8x8_inference_flag = -1;
        int frame_cropping_flag = -1;
        int frame_crop_left_offset = -1;
        int frame_crop_right_offset = -1;
        int frame_crop_top_offset = -1;
        int frame_crop_bottom_offset = -1;
        int vui_parameters_present_flag = -1;
        // Not Implementation for VUI and HRD

        public void print(){
            checkPrint("forbidden_zero_bit", forbidden_zero_bit);
            checkPrint("nal_ref_idc", nal_ref_idc);
            checkPrint("nal_unit_type", nal_unit_type);

            checkPrint("profile_idc", profile_idc);
            checkPrint("constraint_set0_flag", constraint_set0_flag);
            checkPrint("constraint_set1_flag", constraint_set1_flag);
            checkPrint("constraint_set2_flag", constraint_set2_flag);
            checkPrint("constraint_set3_flag", constraint_set3_flag);
            checkPrint("constraint_set4_flag", constraint_set4_flag);
            checkPrint("constraint_set5_flag", constraint_set5_flag);
            checkPrint("reserved_zero_2bits", reserved_zero_2bits);
            checkPrint("level_idc", level_idc);
            checkPrint("seq_parameter_set_id", seq_parameter_set_id);

            checkPrint("chroma_format_idc", chroma_format_idc);
            checkPrint("residual_colour_transform_flag", residual_colour_transform_flag);
            checkPrint("bit_depth_luma_minus8", bit_depth_luma_minus8);
            checkPrint("bit_depth_chroma_minus8", bit_depth_chroma_minus8);
            checkPrint("qpprime_y_zero_transform_bypass_flag", qpprime_y_zero_transform_bypass_flag);
            checkPrint("seq_scaling_matrix_present_flag", seq_scaling_matrix_present_flag);

            checkPrint("log2_max_frame_num_minus4", log2_max_frame_num_minus4);
            checkPrint("pic_order_cnt_type", pic_order_cnt_type);
            checkPrint("log2_max_pic_order_cnt_lsb_minus4", log2_max_pic_order_cnt_lsb_minus4);
            checkPrint("delta_pic_order_always_zero_flag", delta_pic_order_always_zero_flag);
            checkPrint("offset_for_non_ref_pic", offset_for_non_ref_pic);
            checkPrint("offset_for_top_to_bottom_field", offset_for_top_to_bottom_field);
            checkPrint("num_ref_frames_in_pic_order_cnt_cycle", num_ref_frames_in_pic_order_cnt_cycle);

            checkPrint("num_ref_frames", num_ref_frames);
            checkPrint("gaps_in_frame_num_value_allowed_flag", gaps_in_frame_num_value_allowed_flag);
            checkPrint("pic_width_in_mbs_minus1", pic_width_in_mbs_minus1);
            checkPrint("pic_height_in_map_units_minus1", pic_height_in_map_units_minus1);
            checkPrint("frame_mbs_only_flag", frame_mbs_only_flag);
            checkPrint("mb_adaptive_frame_field_flag", mb_adaptive_frame_field_flag);

            checkPrint("direct_8x8_inference_flag", direct_8x8_inference_flag);
            checkPrint("frame_cropping_flag", frame_cropping_flag);
            checkPrint("frame_crop_left_offset", frame_crop_left_offset);
            checkPrint("frame_crop_right_offset", frame_crop_right_offset);
            checkPrint("frame_crop_top_offset", frame_crop_top_offset);
            checkPrint("frame_crop_bottom_offset", frame_crop_bottom_offset);
            checkPrint("vui_parameters_present_flag", vui_parameters_present_flag);
        }

        public int getWidth(){
            return ((pic_width_in_mbs_minus1 +1)*16) - frame_crop_bottom_offset*2 - frame_crop_top_offset*2;
        }

        public int getHeight(){
            return ((2 - frame_mbs_only_flag)* (pic_height_in_map_units_minus1 +1) * 16) - (frame_crop_right_offset * 2) - (frame_crop_left_offset * 2);
        }

        private void checkPrint(String parameterName, int parameter){
            if(parameter != -1){
                Log.i(TAG, parameterName + " = " + parameter);
            }
            else{
                Log.d(TAG, parameterName + " not set");
            }
        }
    }

    private String byte2Hex(byte[] b) {
        String result = "";
        for (int i = 0 ; i < b.length ; i++) {
            result += "0x";
            result += Integer.toString((b[i] & 0xff) + 0x100, 16).substring(1);
            result += " ";
        }
        return result;
    }
}
