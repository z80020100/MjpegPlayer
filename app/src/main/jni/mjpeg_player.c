//
// Created by Cliff Wu on 2016/8/22.
//

#include <com_example_v002060_mjpegplayer_MainActivity.h>

#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>

#include <android/native_window.h>
#include <android/log.h>
#define LOG_TAG "mjpeg_player.c"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define RGB565_RED   (31 << 11)
#define RGB565_GREEN (63 << 5)
#define RGB565_BLUE  (31 << 0)

// Testing for surface operation from native C code
JNIEXPORT void JNICALL Java_com_example_v002060_mjpegplayer_MainActivity_testSurface(JNIEnv *env, jobject activity, jobject surface){

    ANativeWindow_Buffer nwBuffer;

    LOGI("ANativeWindow_fromSurface");
    ANativeWindow *mANativeWindow = ANativeWindow_fromSurface(env, surface);
    if (mANativeWindow == NULL) {
        LOGE("ANativeWindow_fromSurface error");
        return;
    }

    LOGI("Width = %d, Height = %d", ANativeWindow_getWidth (mANativeWindow), ANativeWindow_getHeight (mANativeWindow));

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

void yuv420p_save(AVFrame *pFrame, AVCodecContext *pCodecCtx)
{
    FILE * pfout = fopen ("/storage/emulated/0/test.yuv420p" , "wb");
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

JNIEXPORT void JNICALL Java_com_example_v002060_mjpegplayer_MainActivity_startPlay(JNIEnv *env, jobject activity, jobject surface){
    	LOGI("Enter: naMain");

    unsigned short *rgb565_value;

    ANativeWindow_Buffer nwBuffer;

    LOGI("ANativeWindow_fromSurface");
    ANativeWindow *mANativeWindow = ANativeWindow_fromSurface(env, surface);
    if (mANativeWindow == NULL) {
        LOGE("ANativeWindow_fromSurface error");
        return;
    }

    LOGI("Width = %d, Height = %d", ANativeWindow_getWidth (mANativeWindow), ANativeWindow_getHeight (mANativeWindow));

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
    	videoFileName = "/storage/emulated/0/MJPEG_Q15_1080P_60FPS.mkv";
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
                        int src_w = pCodecCtx->width, src_h = pCodecCtx->height, dst_w = 1920, dst_h = 1080;
                        enum AVPixelFormat src_pix_fmt = AV_PIX_FMT_YUV420P, dst_pix_fmt = AV_PIX_FMT_NV21;//AV_PIX_FMT_RGB565LE
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
                    if (0 != ANativeWindow_lock(mANativeWindow, &nwBuffer, 0)) {
                        LOGE("ANativeWindow_lock error");
                        return;
                    }



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

                            /* write scaled image to file */
                            if(i == 120){
                                dst_filename = "/storage/emulated/0/test.nv21";
                                dst_file = fopen(dst_filename, "wb");
                                if (!dst_file) {
                                    fprintf(stderr, "Could not open destination file %s\n", dst_filename);
                                }

                                fwrite(dst_data[0], 1, dst_bufsize, dst_file);
                                fclose(dst_file);

                                yuv420p_save(pFrame, pCodecCtx);
                            }
                            test_value = dst_data[0];


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
                                //line[x] = *test_value;
                                //test_value++;
                                //LOGI("value = 0x%X", RGB565_RED);
                            }
                            line = line + nwBuffer.stride;
                        }
                    }


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
                LOGI("FPS = %d", fps_count/fps_show_interval);
                fps_count = 0;
            }

                //LOGI("ANativeWindow_unlockAndPost");
                //LOGI(" ");
                if(0 !=ANativeWindow_unlockAndPost(mANativeWindow)){
                    LOGE("ANativeWindow_unlockAndPost error");
                    return;
                }

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

    LOGI("ANativeWindow_release");
    ANativeWindow_release(mANativeWindow);

                                                av_freep(&dst_data[0]);
                                                sws_freeContext(sws_ctx);

        LOGI("Exit: naMain");
}