//
// Created by Cliff Wu on 2016/8/22.
//

#include <com_example_v002060_mjpegplayer_MainActivity.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>

#include <uvc_host.h>

#include <android/native_window.h>
#include <android/log.h>
#define LOG_TAG "mjpeg_player.c"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO , LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN , LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define RGB565_RED   (31 << 11)
#define RGB565_GREEN (63 << 5)
#define RGB565_BLUE  (31 << 0)

jclass MainActivity;
jobject mMainActivity;

char *y_data;
char *uv_data;
int renderer_lock = 0;

int fill_native_window(__uint16_t rgb565_value){

}

void release_native_window(){

}

UvcDeviceInfo gUvcDeviceInfo;
// Testing for surface operation from native C code
JNIEXPORT void JNICALL Java_com_example_v002060_mjpegplayer_MainActivity_testSurface(JNIEnv *env, jobject activity, jobject surface, jbyteArray buffer){



    char uvc_device_name[16] = "/dev/video0";
    init_uvc_device_info(&gUvcDeviceInfo, uvc_device_name, sizeof(uvc_device_name), 1920, 1080);
    int ret = open_uvc_device(&gUvcDeviceInfo); // open and show the UVC device capability
    if(ret < 0){
        LOGE("UVC open failed...");
    }
    else{
        LOGI("UVC open success");

            ret = count_uvc_format_number(&gUvcDeviceInfo);
            if(ret < 0){
                LOGE("count_uvc_format_number");
            }

            ret = store_uvc_format(&gUvcDeviceInfo);
            if(ret < 0){
                LOGE("store_uvc_format");
            }
            //print_support_format(&gUvcDeviceInfo);

            ret = set_uvc_format(&gUvcDeviceInfo, gUvcDeviceInfo.width, gUvcDeviceInfo.height, V4L2_PIX_FMT_MJPEG);
            if(ret < 0){
                LOGE("set_uvc_format");
            }

            ret = set_uvc_fps(&gUvcDeviceInfo, 1, 30);
            if(ret < 0){
                LOGE("set_uvc_fps");
            }

            ret = create_uvc_buf(&gUvcDeviceInfo);
            if(ret < 0){
                LOGE("create_uvc_buf");
            }

            ret = start_uvc_capture(&gUvcDeviceInfo);
            if(ret < 0){
                LOGE("start_uvc_capture");
            }

            ret = get_vuc_frame(&gUvcDeviceInfo);
            if(ret < 0){
                LOGE("get_vuc_frame");
            }
            else{
                write_jpg_file(gUvcDeviceInfo.jpeg_buf, gUvcDeviceInfo.jpeg_buf_used_size, "/storage/sdcard0/UVC_CAPTURE.jpg");
            }

    }

    char* classname = "com/example/v002060/mjpegplayer/MainActivity";
    jclass dpclazz = (*env)->FindClass(env,classname);
    if(dpclazz == 0){
        LOGE("Class: %s not found", classname);
        return;
    }
    jmethodID methodID = (*env)->GetMethodID(env,dpclazz,"decodeCallback","([B)V");
    if(methodID == 0){
        LOGE("Method: decodeCallback not found");
        return;
    }

    //jbyte a[] = {1,2,3,4,5,6}; // (equal to char a[]?)
    int length = 6;
    //unsigned char *a = malloc(length);
    unsigned char *a = (unsigned char *) (*env)->GetPrimitiveArrayCritical(env, buffer, 0);
    a[0] = 1;
    a[3] = 4;
    //jbyteArray ret = (*env)->NewByteArray(env, length);
    //(*env)->SetByteArrayRegion (env, ret, 0, length, a);
    //(*env)->CallVoidMethod(env, activity, methodID, ret);
    (*env)->ReleasePrimitiveArrayCritical(env, buffer, a, 0);

    ANativeWindow       *mANativeWindow;
    ANativeWindow_Buffer nwBuffer;
    int width, height;

    // initialize the native windows
    LOGI("ANativeWindow_fromSurface");
    mANativeWindow = (ANativeWindow*)ANativeWindow_fromSurface(env, surface);
    if (mANativeWindow == NULL){
        LOGE("Error: ANativeWindow_fromSurface()");
        return;
    }

    width = ANativeWindow_getWidth(mANativeWindow);
    height = ANativeWindow_getHeight(mANativeWindow);
    LOGI("Width = %d, Height = %d", width, height);

    LOGI("ANativeWindow_lock");
    if (0 != ANativeWindow_lock(mANativeWindow, &nwBuffer, 0)) {
        LOGE("ANativeWindow_lock error");
        return;
    }

    LOGI("Check nwBuffer->format");
    if (nwBuffer.format == WINDOW_FORMAT_RGB_565) {
        LOGI("nwBuffer->format == WINDOW_FORMAT_RGB_565");
        LOGI("Painted red");
        int y, x;
        __uint16_t * line = (__uint16_t *) nwBuffer.bits;
        // modify pixels with image processing algorithm
        for (y = 0; y < nwBuffer.height; y++) {
            for (x = 0; x < nwBuffer.width; x++) {
                line[x] = RGB565_RED;
            }
            line = line + nwBuffer.stride;
        }
    }

    LOGI("ANativeWindow_unlockAndPost");
    LOGI(" ");
    if(0 !=ANativeWindow_unlockAndPost(mANativeWindow)){
        LOGE("ANativeWindow_unlockAndPost error");
        return;
    }
    sleep(1);

    LOGI("ANativeWindow_lock");
    if (0 != ANativeWindow_lock(mANativeWindow, &nwBuffer, 0)) {
        LOGE("ANativeWindow_lock error");
        return;
    }

    LOGI("Check nwBuffer->format");
    if (nwBuffer.format == WINDOW_FORMAT_RGB_565) {
        LOGI("nwBuffer->format == WINDOW_FORMAT_RGB_565");
        LOGI("Painted green");
        int y, x;
        __uint16_t * line = (__uint16_t *) nwBuffer.bits;
        // modify pixels with image processing algorithm
        for (y = 0; y < nwBuffer.height; y++) {
            for (x = 0; x < nwBuffer.width; x++) {
                line[x] = RGB565_GREEN;
            }
            line = line + nwBuffer.stride;
        }
    }

    LOGI("ANativeWindow_unlockAndPost");
    LOGI(" ");
    if(0 !=ANativeWindow_unlockAndPost(mANativeWindow)){
        LOGE("ANativeWindow_unlockAndPost error");
        return;
    }
    sleep(1);

    LOGI("ANativeWindow_lock");
    if (0 != ANativeWindow_lock(mANativeWindow, &nwBuffer, 0)) {
        LOGE("ANativeWindow_lock error");
        return;
    }

    LOGI("Check nwBuffer->format");
    if (nwBuffer.format == WINDOW_FORMAT_RGB_565) {
        int y, x;
        LOGI("nwBuffer->format == WINDOW_FORMAT_RGB_565");
        LOGI("Painted blue");
        __uint16_t * line = (__uint16_t *) nwBuffer.bits;
        // modify pixels with image processing algorithm
        for (y = 0; y < nwBuffer.height; y++) {
            for (x = 0; x < nwBuffer.width; x++) {
                line[x] = RGB565_BLUE;
            }
            line = line + nwBuffer.stride;
        }
    }

    LOGI("ANativeWindow_unlockAndPost");
    LOGI(" ");
    if(0 !=ANativeWindow_unlockAndPost(mANativeWindow)){
        LOGE("ANativeWindow_unlockAndPost error");
        return;
    }

    if (nwBuffer.format == WINDOW_FORMAT_RGBX_8888 ) {
        LOGI("nwBuffer->format == WINDOW_FORMAT_RGBX_8888 ");
    }

    LOGI("ANativeWindow_release");
    ANativeWindow_release(mANativeWindow);
}

void nv21_copy(AVFrame *pFrame, AVCodecContext *pCodecCtx, unsigned char *y_dest, unsigned char *uv_dest){
    //FILE * pfout = fopen ("/storage/emulated/0/test_copy.yuv420p" , "wb");
    int i = 0;

    int width = pCodecCtx->width, height = pCodecCtx->height;
    int height_half = height / 2, width_half = width / 2;
    int y_wrap = pFrame->linesize[0];
    int u_wrap = pFrame->linesize[1];
    int v_wrap = pFrame->linesize[2];

    unsigned char *y_buf = pFrame->data[0];
    unsigned char *u_buf = pFrame->data[1];
    unsigned char *v_buf = pFrame->data[2];

    int dest_offset = 0;
    //save y
    for (i = 0; i < height; i++){
        //fwrite(y_buf + i * y_wrap, 1, width, pfout);
        memcpy(y_dest + dest_offset, y_buf + i * y_wrap, width);
        dest_offset = dest_offset + width;
    }

    // change to NV21 arrangement
    int j = 0;
    dest_offset = 0;
    for (i = 0; i < height_half; i++){
        for(j = 0; j < width_half; j++){
            *(uv_dest + dest_offset) = *(v_buf + i * v_wrap + j);
            dest_offset++;
            *(uv_dest + dest_offset) = *(u_buf + i * u_wrap + j);
            dest_offset++;
        }
    }

    //LOGI("Copy %d bytes data", dest_offset);
    //fwrite(dest, 1, dest_offset, pfout);


    //fflush(pfout);
    //fclose(pfout);
}

void yuv420p_copy(AVFrame *pFrame, AVCodecContext *pCodecCtx, unsigned char *dest){
    //FILE * pfout = fopen ("/storage/emulated/0/test_copy.yuv420p" , "wb");
    int i = 0;

    int width = pCodecCtx->width, height = pCodecCtx->height;
    int height_half = height / 2, width_half = width / 2;
    int y_wrap = pFrame->linesize[0];
    int u_wrap = pFrame->linesize[1];
    int v_wrap = pFrame->linesize[2];

    unsigned char *y_buf = pFrame->data[0];
    unsigned char *u_buf = pFrame->data[1];
    unsigned char *v_buf = pFrame->data[2];

    int dest_offset = 0;
    //save y
    for (i = 0; i < height; i++){
        //fwrite(y_buf + i * y_wrap, 1, width, pfout);
        memcpy(dest + dest_offset, y_buf + i * y_wrap, width);
        dest_offset = dest_offset + width;
    }

    //LOGI("===>copy Y success\n");
    //save u
    for (i = 0; i < height_half; i++){
        //fwrite(u_buf + i * u_wrap, 1, width_half, pfout);
        memcpy(dest + dest_offset, u_buf + i * u_wrap, width_half);
        dest_offset = dest_offset + width_half;
    }

    //LOGI("===>copy U success\n");
    //save v
    for (i = 0; i < height_half; i++){
        //fwrite(v_buf + i * v_wrap, 1, width_half, pfout);
        memcpy(dest + dest_offset, v_buf + i * v_wrap, width_half);
        dest_offset = dest_offset + width_half;
    }

    //LOGI("===>copy V success\n");

    //LOGI("Copy %d bytes data", dest_offset);
    //fwrite(dest, 1, dest_offset, pfout);


    //fflush(pfout);
    //fclose(pfout);
}

void yuv420p_save(AVFrame *pFrame, AVCodecContext *pCodecCtx)
{
    FILE * pfout = fopen ("/storage/sdcard0/test.yuv420p" , "wb");
    int i = 0;

    int width = pCodecCtx->width, height = pCodecCtx->height;
    int height_half = height / 2, width_half = width / 2;
    int y_wrap = pFrame->linesize[0];
    int u_wrap = pFrame->linesize[1];
    int v_wrap = pFrame->linesize[2];

    unsigned char *y_buf = pFrame->data[0];
    unsigned char *u_buf = pFrame->data[1];
    unsigned char *v_buf = pFrame->data[2];

    //save y
    for (i = 0; i < height; i++)
        fwrite(y_buf + i * y_wrap, 1, width, pfout);
    LOGI("===>save Y success\n");
    //save u
    for (i = 0; i < height_half; i++)
        fwrite(u_buf + i * u_wrap, 1, width_half, pfout);
    LOGI("===>save U success\n");
    //save v
    for (i = 0; i < height_half; i++)
        fwrite(v_buf + i * v_wrap, 1, width_half, pfout);
    LOGI("===>save V success\n");

    fflush(pfout);
    fclose(pfout);
}

JNIEXPORT void JNICALL Java_com_example_v002060_mjpegplayer_MainActivity_startPlay(JNIEnv *env, jobject activity, jobject surface, jbyteArray jbuffer){
    	LOGI("Enter: naMain");

    char* classname = "com/example/v002060/mjpegplayer/MainActivity";
    jclass dpclazz = (*env)->FindClass(env,classname);
    if(dpclazz == 0){
        LOGE("Class: %s not found", classname);
        return;
    }
    jmethodID methodID = (*env)->GetMethodID(env,dpclazz,"decodeCallback","()V");
    if(methodID == 0){
        LOGE("Method: decodeCallback not found");
        return;
    }

    //jbyte a[] = {1,2,3,4,5,6}; // (equal to char a[]?)
    int length = 3840*2160*3/2;
    //unsigned char *yuvFrameData = malloc(length);
    //unsigned char *yuvFrameData;
    //jbyteArray frameByteArray = (*env)->NewByteArray(env, length);
    //(*env)->SetByteArrayRegion (env, frameByteArray, 0, length, yuvFrameData);
    //(*env)->CallVoidMethod(env, activity, methodID, frameByteArray);

            unsigned char *yuvFrameData = malloc(3804*2160*3/2);
            jbyteArray frameByteArray = (*env)->NewByteArray(env, 3804*2160*3/2);




    unsigned short *rgb565_value;

/*
    ANativeWindow_Buffer nwBuffer;

    LOGI("ANativeWindow_fromSurface");
    ANativeWindow *mANativeWindow = ANativeWindow_fromSurface(env, surface);
    if (mANativeWindow == NULL) {
        LOGE("ANativeWindow_fromSurface error");
        return;
    }
*/

    //LOGI("Width = %d, Height = %d", ANativeWindow_getWidth (mANativeWindow), ANativeWindow_getHeight (mANativeWindow));

    	AVFormatContext *pFormatCtx = NULL;
    	int             i, videoStream;
    	AVCodecContext  *pCodecCtx = NULL;
    	AVCodec         *pCodec = NULL;
    	AVFrame         *pFrame = NULL;
    	AVFrame         *pFrameRGBA = NULL;
    	AVPacket        packet;
    	int             frameFinished;
    	jobject			bitmap;
    	void* 			buffer;

    	AVDictionary    *optionsDict = NULL;
    	//struct SwsContext      *sws_ctx = NULL;
    	char *videoFileName;

    	unsigned int fps_count = 0;
    	struct timeval tv;
    	unsigned long fps_count_start_time_us;
        unsigned long fps_count_cur_time_us;
        int fps_show_interval = 3;

    	// Register all formats and codecs
    	av_register_all();
    	LOGI("Register all formats and codecs");

    	//get C string from JNI jstring
    	//videoFileName = (char *)(*pEnv)->GetStringUTFChars(pEnv, pFileName, NULL);
    	//videoFileName = "/storage/emulated/legacy/MJPEG_Q2_4K_24FPS.mkv";
    	videoFileName = "/storage/sdcard0/MJPEG_Q2_4K_24FPS.mkv";
        LOGI("Video file name: %s", videoFileName);

    	// Open video file
    	if(avformat_open_input(&pFormatCtx, videoFileName, NULL, NULL)!=0)
    		return ; // Couldn't open file
    	LOGI("Open video file");

    	// Retrieve stream information
    	if(avformat_find_stream_info(pFormatCtx, NULL)<0)
    		return; // Couldn't find stream information
    	LOGI("Retrieve stream information");

    	// Dump information about file onto standard error
    	av_dump_format(pFormatCtx, 0, videoFileName, 0);
        LOGI("Dump information about file onto standard error");

    	// Find the first video stream
    	videoStream=-1;
    	for(i=0; i<pFormatCtx->nb_streams; i++) {
    		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
    			videoStream=i;
    			break;
    		}
    	}
    	if(videoStream==-1)
    		return ; // Didn't find a video stream
        LOGI("Find the first video stream");

    	// Get a pointer to the codec context for the video stream
    	pCodecCtx=pFormatCtx->streams[videoStream]->codec;
    	LOGI("Get a pointer to the codec context for the video stream");

    	// Find the decoder for the video stream
    	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    	if(pCodec==NULL) {
    		fprintf(stderr, "Unsupported codec!\n");
    		return ; // Codec not found
    	}
    	LOGI("Find the decoder for the video stream");
    	LOGI("pCodecCtx->codec_id = %d", pCodecCtx->codec_id);
    	if(pCodecCtx->codec_id == AV_CODEC_ID_MPEG4){
    	    LOGI("pCodecCtx->codec_id = AV_CODEC_ID_MPEG4");
    	}
    	if(pCodecCtx->codec_id == AV_CODEC_ID_MJPEG){
    	    LOGI("pCodecCtx->codec_id = AV_CODEC_ID_MJPEG");
    	    pCodecCtx->thread_count = 4;
    	    pCodecCtx->thread_type = FF_THREAD_FRAME;
    	}

    	// Open codec
    	if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
    		return ; // Could not open codec
        LOGI("Open codec");

    	// Allocate video frame
    	pFrame=av_frame_alloc();
    	if(pFrame ==NULL)
    	    return ;
    	LOGI("Allocate video frame");

    	// Allocate an AVFrame structure
    	pFrameRGBA=av_frame_alloc();
    	if(pFrameRGBA==NULL)
    		return ;
    	LOGI("Allocate an AVFrame structure");

    	//get the scaling context
    	/*
    	sws_ctx = sws_getContext
        (
            pCodecCtx->width,
            pCodecCtx->height,
            pCodecCtx->pix_fmt,
            ANativeWindow_getWidth (mANativeWindow),
            ANativeWindow_getHeight (mANativeWindow),
            AV_PIX_FMT_RGB565BE,
            SWS_BILINEAR,
            NULL,
            NULL,
            NULL
        );
        */

    	// Assign appropriate parts of bitmap to image planes in pFrameRGBA
    	// Note that pFrameRGBA is an AVFrame, but AVFrame is a superset
    	// of AVPicture
    	//avpicture_fill((AVPicture *)pFrameRGBA, buffer, AV_PIX_FMT_RGBA,
    	//	 pCodecCtx->width, pCodecCtx->height);
        //LOGI("Assign appropriate parts of bitmap to image planes in pFrameRGBA");


                        uint8_t *src_data[4], *dst_data[4];
                        int src_linesize[4], dst_linesize[4];
                        int src_w = pCodecCtx->width, src_h = pCodecCtx->height, dst_w = 3840, dst_h = 2160;
                        enum AVPixelFormat src_pix_fmt = AV_PIX_FMT_YUV420P, dst_pix_fmt = AV_PIX_FMT_RGB565LE;//AV_PIX_FMT_RGB565LE
                        const char *dst_filename = NULL;
                        FILE *dst_file;
                        int dst_bufsize;
                        struct SwsContext *sws_ctx;
                        int ret, j;
                        unsigned short *test_value;
    	// Read frames and save first five frames to disk
    	i=0;
    	                        /* create scaling context */
                                sws_ctx = sws_getContext(src_w, src_h, src_pix_fmt,
                                                         dst_w, dst_h, dst_pix_fmt,
                                                         SWS_BILINEAR, NULL, NULL, NULL);
                                if (!sws_ctx) {
                                    fprintf(stderr,
                                            "Impossible to create scale context for the conversion "
                                            "fmt:%s s:%dx%d -> fmt:%s s:%dx%d\n",
                                            av_get_pix_fmt_name(src_pix_fmt), src_w, src_h,
                                            av_get_pix_fmt_name(dst_pix_fmt), dst_w, dst_h);
                                    ret = AVERROR(EINVAL);
                                    //goto end;
                                }
                                                        if ((ret = av_image_alloc(dst_data, dst_linesize,
                                                                                  dst_w, dst_h, dst_pix_fmt, 1)) < 0) {
                                                            fprintf(stderr, "Could not allocate destination image\n");
                                                            //goto end;
                                                        }
                                                        dst_bufsize = ret;
    	while(av_read_frame(pFormatCtx, &packet)>=0) {
    		// Is this a packet from the video stream?
    		if(packet.stream_index==videoStream) {
    			// Decode video frame
    			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,
    			   &packet);
    			// Did we get a video frame?
    			if(frameFinished) {

    			    //LOGI("ANativeWindow_lock");
                    //if (0 != ANativeWindow_lock(mANativeWindow, &nwBuffer, 0)) {
                        //LOGE("ANativeWindow_lock error");
                        //return;
                    //}



    			    //LOGI("Frame %d decode complete", i);
    				// Convert the image from its native format to RGBA

                    if(1){
                        //LOGI("yuv420p_save: frame %d", i);
                        //yuv420p_save(pFrame, pCodecCtx);





                        /* allocate source and destination image buffers
                        if ((ret = av_image_alloc(src_data, src_linesize,
                                                  src_w, src_h, src_pix_fmt, 16)) < 0) {
                            fprintf(stderr, "Could not allocate source image\n");
                            //goto end;
                        }*/

                        /* buffer is going to be written to rawvideo file, no alignment */


                        //LOGI("dst_bufsize = %d", dst_bufsize);



                            /* convert to destination format */
                            //sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                                      //pFrame->linesize, 0, src_h, dst_data, dst_linesize);

                                //yuvFrameData = (*env)->GetPrimitiveArrayCritical(env, jbuffer, 0);
                                //yuv420p_copy(pFrame, pCodecCtx, yuvFrameData);
                                //(*env)->SetByteArrayRegion (env, frameByteArray, 0, length, yuvFrameData);
                                //(*env)->ReleasePrimitiveArrayCritical(env, jbuffer, yuvFrameData, JNI_ABORT);
                                //memcpy(y_data, yuvFrameData, 3840*2160);
                                //memcpy(y_data, yuvFrameData+3840*2160, 3840*2160/2);
                                //LOGI("renderer_lock: %p", &renderer_lock);

                                nv21_copy(pFrame, pCodecCtx, y_data, uv_data);
                                (*env)->CallVoidMethod(env, activity, methodID);



                            /* write scaled image to file */
                            if(0){
                                dst_filename = "/storage/emulated/0/test.nv21";
                                dst_file = fopen(dst_filename, "wb");
                                if (!dst_file) {
                                    fprintf(stderr, "Could not open destination file %s\n", dst_filename);
                                }

                                fwrite(dst_data[0], 1, dst_bufsize, dst_file);
                                fclose(dst_file);

                                yuv420p_save(pFrame, pCodecCtx);
                                //yuv420p_copy(pFrame, pCodecCtx, yuvFrameData);
                                //(*env)->CallVoidMethod(env, activity, methodID, frameByteArray);
                            }
                            //test_value = dst_data[0];


/*
                            for(j = 0; j < dst_bufsize; j++){
                                unsigned char *test_value = dst_data[0]+j;
                                if(*(test_value) != 0){
                                    LOGI("j = %d, value = %x", j, *test_value);
                                    break;
                                }
                            }
*/








                    }

                    /*
    				sws_scale
    				(
    					sws_ctx,
    					(uint8_t const * const *)pFrame->data,
    					pFrame->linesize,
    					0,
    					pCodecCtx->height,
    					pFrameRGBA->data,
    					pFrameRGBA->linesize
    				);
    				*/


                    /*
                    //LOGI("Check nwBuffer->format");
                    if (nwBuffer.format == WINDOW_FORMAT_RGB_565) {
                        int y, x;
                        //LOGI("nwBuffer->format == WINDOW_FORMAT_RGB_565");
                        //LOGI("Painted blue");
                        __uint16_t * line = (__uint16_t *) nwBuffer.bits;
                        // modify pixels with image processing algorithm
                        //LOGI("nwBuffer.height = %d, nwBuffer.width = %d", nwBuffer.height, nwBuffer.width);

                        for (y = 0; y < 1080; y++) {
                            for (x = 0; x < 1920; x++) {
                                line[x] = *test_value;
                                test_value++;
                                //LOGI("value = 0x%X", RGB565_RED);
                            }
                            line = line + nwBuffer.stride;
                        }
                    }
                    */


                                            //av_freep(&src_data[0]);


            // show FPS
            if(fps_count == 0){
                gettimeofday(&tv,NULL);
                fps_count_start_time_us = 1000000 * tv.tv_sec + tv.tv_usec;
            }

            gettimeofday(&tv, NULL);
            fps_count_cur_time_us = 1000000 * tv.tv_sec + tv.tv_usec;
            fps_count++;

            if((fps_count_cur_time_us - fps_count_start_time_us) >= fps_show_interval*1000000){
                //printf("UVC: %s, Name: %s, Resolution: %d x %d, FPS = %d\n", uvc_device_info->uvc_dev_path, uvc_device_info->cap.card, uvc_device_info->set_format.fmt.pix.width, uvc_device_info->set_format.fmt.pix.height, uvc_device_info->fps_count/uvc_device_info->fps_show_interval);
                LOGI("Decode FPS = %d", fps_count/fps_show_interval);
                fps_count = 0;
            }

                //LOGI("ANativeWindow_unlockAndPost");
                //LOGI(" ");
                //if(0 !=ANativeWindow_unlockAndPost(mANativeWindow)){
                    //LOGE("ANativeWindow_unlockAndPost error");
                    //return;
                //}

    			}
    		}
    		// Free the packet that was allocated by av_read_frame
    		//av_free_packet(&packet);
    		av_packet_unref(&packet);
    		i++;
    	}
    	LOGI("Decode complete");



    	// Free the RGB image
    	av_free(pFrameRGBA);

    	// Free the YUV frame
    	av_free(pFrame);

    	// Close the codec
    	avcodec_close(pCodecCtx);

    	// Close the video file
    	avformat_close_input(&pFormatCtx);

    //LOGI("ANativeWindow_release");
    //ANativeWindow_release(mANativeWindow);

                                                av_freep(&dst_data[0]);
                                                sws_freeContext(sws_ctx);

    free(yuvFrameData);
        LOGI("Exit: naMain");
}

int uvc_preview_flag = 0;

JNIEXPORT void JNICALL Java_com_example_v002060_mjpegplayer_MainActivity_startUvcPreview(JNIEnv *env, jobject activity, jstring jDevice, jint width, jint height){

    // Set UVC Device
    char *uvc_device_name = (*env)->GetStringUTFChars(env, jDevice, 0);
    init_uvc_device_info(&gUvcDeviceInfo, uvc_device_name, strlen(uvc_device_name), width, height);
    (*env)->ReleaseStringUTFChars(env, jDevice, uvc_device_name);
    int ret = open_uvc_device(&gUvcDeviceInfo); // open and show the UVC device capability
    if(ret < 0){
        LOGE("UVC open failed...");
        goto ERROR_EXIT;
    }

    ret = count_uvc_format_number(&gUvcDeviceInfo);
    if(ret < 0){
        LOGE("count_uvc_format_number");
        goto ERROR_EXIT;
    }

    ret = store_uvc_format(&gUvcDeviceInfo);
    if(ret < 0){
        LOGE("store_uvc_format");
        goto ERROR_EXIT;
    }
    print_support_format(&gUvcDeviceInfo);

    ret = set_uvc_format(&gUvcDeviceInfo, gUvcDeviceInfo.width, gUvcDeviceInfo.height, V4L2_PIX_FMT_MJPEG);
    if(ret < 0){
        LOGE("set_uvc_format");
        goto ERROR_EXIT;
    }

    ret = set_uvc_fps(&gUvcDeviceInfo, 1, 30);
    if(ret < 0){
        LOGE("set_uvc_fps");
        goto ERROR_EXIT;
    }

    ret = create_uvc_buf(&gUvcDeviceInfo);
    if(ret < 0){
        LOGE("create_uvc_buf");
        goto ERROR_EXIT;
    }

    ret = start_uvc_capture(&gUvcDeviceInfo);
    if(ret < 0){
        LOGE("start_uvc_capture");
        goto ERROR_EXIT;
    }

    ret = get_vuc_frame(&gUvcDeviceInfo);
    if(ret < 0){
        LOGE("get_vuc_frame");
        goto ERROR_EXIT;
    }

    write_jpg_file(gUvcDeviceInfo.jpeg_buf, gUvcDeviceInfo.jpeg_buf_used_size, "/storage/sdcard0/UVC_CAPTURE.jpg");
    LOGI("Save a frame to file");

    // Set JNI frame update callback function
    char* classname = "com/example/v002060/mjpegplayer/MainActivity";
    jclass dpclazz = (*env)->FindClass(env, classname);
    if(dpclazz == 0){
        LOGE("Class: %s not found", classname);
        return;
    }
    jmethodID methodID = (*env)->GetMethodID(env, dpclazz, "decodeCallback", "()V");
    if(methodID == 0){
        LOGE("Method: decodeCallback not found");
        return;
    }

    // Set FFmpeg Decoder
    AVCodecContext *pCodecCtx = NULL;
    AVCodec        *pCodec = NULL;
    AVFrame        *pFrame = NULL;
    AVPacket        packet;
    int frameFinished;

    // Register all formats and codecs
    av_register_all();
    LOGI("Register all formats and codecs");

    pCodec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
    pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_get_context_defaults3(pCodecCtx, pCodec);
    pCodecCtx->thread_count = 4;
    pCodecCtx->thread_type = FF_THREAD_FRAME;

    if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0){
        LOGE("Could not open codec");
        return;
    }
    LOGI("Open codec");

    // Allocate video frame
    pFrame = av_frame_alloc();
    if(pFrame == NULL){
        LOGE("Could not allocate video frame");
        return;
    }
    LOGI("Allocate video frame");
    uvc_preview_flag = 1;

    int i = 0;
    while(uvc_preview_flag){
        ret = get_vuc_frame(&gUvcDeviceInfo);
        if(ret < 0){
            LOGE("get_vuc_frame");
            goto ERROR_EXIT;
        }

        packet.data = gUvcDeviceInfo.jpeg_buf;
        packet.size = gUvcDeviceInfo.jpeg_buf_used_size;

        avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
    	// Did we get a video frame?
    	if(frameFinished) {
            nv21_copy(pFrame, pCodecCtx, y_data, uv_data);
            (*env)->CallVoidMethod(env, activity, methodID);

            if(i == 0){
                LOGI("pFrame->format = %d", pFrame->format);

                //yuv420p_save(pFrame, pCodecCtx);
                 uint8_t *src_data[4], *dst_data[4];
                int src_linesize[4], dst_linesize[4];
                int src_w = pCodecCtx->width, src_h = pCodecCtx->height, dst_w = 3840, dst_h = 2160;
                enum AVPixelFormat src_pix_fmt = AV_PIX_FMT_YUV422P, dst_pix_fmt = AV_PIX_FMT_NV21;//AV_PIX_FMT_RGB565LE, AV_PIX_FMT_NV21
                const char *dst_filename = NULL;
                FILE *dst_file;
                int dst_bufsize;
                struct SwsContext *sws_ctx;
                int ret;
                unsigned short *test_value;


                /* create scaling context */
                sws_ctx = sws_getContext(src_w, src_h, src_pix_fmt,
                    dst_w, dst_h, dst_pix_fmt,
                    SWS_BILINEAR, NULL, NULL, NULL);
                    if (!sws_ctx) {
                        LOGE(
                            "Impossible to create scale context for the conversion, fmt:%s s:%dx%d -> fmt:%s s:%dx%d",
                            av_get_pix_fmt_name(src_pix_fmt), src_w, src_h,
                            av_get_pix_fmt_name(dst_pix_fmt), dst_w, dst_h);
                        ret = AVERROR(EINVAL);
                    }
                    if ((ret = av_image_alloc(dst_data, dst_linesize, dst_w, dst_h, dst_pix_fmt, 1)) < 0) {
                        LOGE("Could not allocate destination image");
                    }
                    dst_bufsize = ret;
                    LOGI("dst_bufsize = %d", dst_bufsize);

                    sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                        pFrame->linesize, 0, src_h, dst_data, dst_linesize);

                    dst_file = fopen("/storage/sdcard0/test.nv21", "wb");
                    fwrite(dst_data[0], 1, dst_bufsize, dst_file);
                    fclose(dst_file);
            }
            i++;
    	}
    }

    memset(y_data, 0, 3840*2160);
    memset(uv_data, 128, 3840*2160/2);
    (*env)->CallVoidMethod(env, activity, methodID);

    // Free the YUV frame
    free(pFrame);

    // Close the codec
    avcodec_close(pCodecCtx);


ERROR_EXIT:
    close_uvc_device(&gUvcDeviceInfo);
    return;
}

JNIEXPORT void JNICALL Java_com_example_v002060_mjpegplayer_MainActivity_stopUvcPreview(JNIEnv *env, jobject activity){
    uvc_preview_flag = 0;
}