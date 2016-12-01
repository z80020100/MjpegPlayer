#include <uvc_host.h>

#include <android/log.h>
#define LOG_TAG "uvc_host.c"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO , LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN , LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

int print_v4l2_cap(struct v4l2_capability *cap)
{
    /*
    struct v4l2_capability {
        __u8    driver[16];     // i.e. "bttv"
        __u8    card[32];       // i.e. "Hauppauge WinTV"
        __u8    bus_info[32];   // "PCI:" + pci_name(pci_dev)
        __u32   version;        // should use KERNEL_VERSION()
        __u32   capabilities;   // Device capabilities
        __u32   reserved[4];
    };
     */
    int cap_chk = -2;

    LOGI("driver      : %s", cap->driver);
    LOGI("card        : %s", cap->card);
    LOGI("bus_info    : %s", cap->bus_info);
    LOGI("version     : %d", cap->version);
    LOGI("capabilities: 0x%08X", cap->capabilities);
	
	if(cap->capabilities & V4L2_CAP_VIDEO_CAPTURE){
		LOGI("Support V4L2_CAP_VIDEO_CAPTURE\n");
        cap_chk++;
	}
	
	if(cap->capabilities & V4L2_CAP_STREAMING){
		LOGI("Support V4L2_CAP_STREAMING\n");
        cap_chk++;
	}

    return cap_chk;
}

int is_huffman(unsigned char *buf)
{
    unsigned char *ptbuf;
    int i = 0;
    ptbuf = buf;
    while(((ptbuf[0] << 8) | ptbuf[1]) != 0xffda) {
        if(i++ > 2048)
            return 0;
        if(((ptbuf[0] << 8) | ptbuf[1]) == 0xffc4)
            return 1;
        ptbuf++;
    }
    return 0;
}

void write_jpg_file(unsigned char *buf, int size, char *file_name){
	FILE * pFile;
	pFile = fopen(file_name, "wb");
	
    if(!is_huffman(buf)){
	    unsigned char *ptdeb, *ptlimit, *ptcur = buf;
        int sizein;
	
	    ptdeb = ptcur = buf;
        ptlimit = buf + size;
        while((((ptcur[0] << 8) | ptcur[1]) != 0xffc0) && (ptcur < ptlimit))
            ptcur++;
    
        sizein = ptcur - ptdeb;

        fwrite(buf, 1, sizein, pFile);
        fwrite(dht_data, 1, sizeof(dht_data), pFile);
        fwrite(ptcur, 1, size - sizein, pFile);	
	}
	else{
        fwrite(buf, 1, size, pFile);
	}
	fclose(pFile);
}

int write_jpg_memory(unsigned char *buf, int size, char *memory_ptr){
	if(!is_huffman(buf)){
	    unsigned char *ptdeb, *ptlimit, *ptcur = buf;
        int sizein, pos = 0;
	
	    ptdeb = ptcur = buf;
        ptlimit = buf + size;
        while((((ptcur[0] << 8) | ptcur[1]) != 0xffc0) && (ptcur < ptlimit))
            ptcur++;
    
        sizein = ptcur - ptdeb;
	
	    memcpy(memory_ptr + pos, buf, sizein); pos += sizein;
        memcpy(memory_ptr + pos, dht_data, sizeof(dht_data)); pos += sizeof(dht_data);
        memcpy(memory_ptr + pos, ptcur, size - sizein); pos += size - sizein;
	
	    return pos;
    }
    else{
        memcpy(memory_ptr, buf, size);
        return size;
    }
}

int open_uvc_device(UvcDeviceInfo *uvc_device_info){

    int ret, status = -1;
    
    if(uvc_device_info->stage == UVC_DEVICE_STAGE_INIT){
        uvc_device_info->uvc_dev_fd = open(uvc_device_info->uvc_dev_path, O_RDWR);
        if(uvc_device_info->uvc_dev_fd == -1){
            LOGE("Open UVC Device: %s fail!", uvc_device_info->uvc_dev_path);
            //perror("Error");
            uvc_device_info->stage = UVC_DEVICE_STAGE_IDEL;
        }
        else{
            LOGI("Open UVC Device: %s succeed!", uvc_device_info->uvc_dev_path);
            uvc_device_info->stage = UVC_DEVICE_STAGE_OPEN;
            // Show the UVC device capability
            ret = ioctl(uvc_device_info->uvc_dev_fd, VIDIOC_QUERYCAP, &uvc_device_info->cap);
            if(ret < 0){
                LOGE("Query VIDIOC_QUERYCAP fail!");
                return status;
            }
        
            ret = print_v4l2_cap(&uvc_device_info->cap);
            if(ret < 0){
                LOGI("UVC Device: %s can't capture and streaming", uvc_device_info->uvc_dev_path);
                return status;
            }
            status = 0;        
        }

    }
    else{
        LOGE("Error: open UVC device fail, UVC device is on stage %d", uvc_device_info->stage);
    }

    return status;
}

void close_uvc_device(UvcDeviceInfo *uvc_device_info){
    int ret, i;
    LOGI("Start: close UVC device: %s", uvc_device_info->uvc_dev_path);
    switch(uvc_device_info->stage){
        case UVC_DEVICE_STAGE_START_CAPTURE:
            ret = stop_uvc_capture(uvc_device_info);
            
        case UVC_DEVICE_STAGE_JPEG_BUF:
            free(uvc_device_info->jpeg_buf);
            LOGI("Free buffer for JPEG data");
            
        case UVC_DEVICE_STAGE_MAP_BUF:
            for(i = 0; i < uvc_device_info->req_buf.count; i++){
                munmap(uvc_device_info->memory[i], uvc_device_info->buf.length);
            }
            LOGI("Unmap buffers");

            
        case UVC_DEVICE_STAGE_REQ_BUF:
        case UVC_DEVICE_STAGE_SET_FPS:
            free(uvc_device_info->set_fps);
            LOGI("Free struct v4l2_streamparm (for set FPS)");
        
        case UVC_DEVICE_STAGE_SET_FORMAT:
        case UVC_DEVICE_STAGE_STORE_FORMAT:
            free(uvc_device_info->sup_format);
            LOGI("Free UvcDeviceFormat buffer (for store support format)");

        case UVC_DEVICE_STAGE_GET_FORMAT_NUMBER:
        case UVC_DEVICE_STAGE_OPEN:
            close(uvc_device_info->uvc_dev_fd);
            LOGI("Close UVC Device: %s", uvc_device_info->uvc_dev_path);
            
        case UVC_DEVICE_STAGE_INIT:
        case UVC_DEVICE_STAGE_IDEL:
            break;

        default:
            LOGI("Unknown UVC Device Stage: %d", uvc_device_info->stage);
    }
    uvc_device_info->stage = UVC_DEVICE_STAGE_IDEL;
    LOGI("End: close UVC device: %s", uvc_device_info->uvc_dev_path);
    
}

void init_uvc_device_info(UvcDeviceInfo *uvc_device_info, char *dev_name, int dev_name_length, int width, int height){
    if((uvc_device_info->stage == UVC_DEVICE_STAGE_IDEL) || uvc_device_info->stage == UVC_DEVICE_STAGE_INIT){
        memset(uvc_device_info, 0, sizeof(UvcDeviceInfo));
        memcpy(uvc_device_info->uvc_dev_path, dev_name, dev_name_length);
        LOGI("Set UVC Device Node: %s", uvc_device_info->uvc_dev_path);
        uvc_device_info->width = width;
        uvc_device_info->height = height;
        LOGI("Set UVC Resolution: %d x %d", uvc_device_info->width, uvc_device_info->height);
        uvc_device_info->fps_show_interval = FPS_SHOW_INTERVAL;
        LOGI("Set UVC Device FPS Print Interval: %ds", uvc_device_info->fps_show_interval);
        uvc_device_info->stage = UVC_DEVICE_STAGE_INIT;
    }
    else{
        LOGE("Error: init UVC device fail, UVC device is on stage %d", uvc_device_info->stage);
    }
}

int count_uvc_format_number(UvcDeviceInfo *uvc_device_info){
    //float frame_rate;
    uvc_device_info->format_num = 0;

    if(uvc_device_info->stage == UVC_DEVICE_STAGE_OPEN){

        // get format
        uvc_device_info->fmt.index = 0;
        uvc_device_info->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        while (ioctl(uvc_device_info->uvc_dev_fd, VIDIOC_ENUM_FMT, &uvc_device_info->fmt) == 0){
            /*printf("{ format index = %d, pixelformat = \"%c%c%c%c\", description = \"%s\" }\n", uvc_device_info->fmt.index,
                uvc_device_info->fmt.pixelformat & 0xFF, (uvc_device_info->fmt.pixelformat >> 8) & 0xFF, (uvc_device_info->fmt.pixelformat >> 16) & 0xFF, (uvc_device_info->fmt.pixelformat >> 24) & 0xFF, 
                uvc_device_info->fmt.description);*/

            // get resolution for one format
            uvc_device_info->fsize.index = 0;
            uvc_device_info->fsize.pixel_format = uvc_device_info->fmt.pixelformat;
            while(ioctl(uvc_device_info->uvc_dev_fd, VIDIOC_ENUM_FRAMESIZES, &uvc_device_info->fsize) == 0){
                if (uvc_device_info->fsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                    /*printf("  { frame size index = %d, discrete: width = %u, height = %u }\n", uvc_device_info->fsize.index,
                        uvc_device_info->fsize.discrete.width, uvc_device_info->fsize.discrete.height);*/

                    // get frame rate for one format, one resolution
                    uvc_device_info->fival.index = 0;
                    uvc_device_info->fival.pixel_format = uvc_device_info->fmt.pixelformat;
                    uvc_device_info->fival.width = uvc_device_info->fsize.discrete.width;
                    uvc_device_info->fival.height = uvc_device_info->fsize.discrete.height;
                    while(ioctl(uvc_device_info->uvc_dev_fd, VIDIOC_ENUM_FRAMEINTERVALS, &uvc_device_info->fival) == 0){
                        if (uvc_device_info->fival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                            //printf("frame interval: %u/%u\n", uvc_device_info->fival.discrete.numerator, uvc_device_info->fival.discrete.denominator); // output is a fraction
                            //frame_rate = (float)uvc_device_info->fival.discrete.denominator/uvc_device_info->fival.discrete.numerator;
                            //printf("    { frame interval index = %d, frame rate = %.3f }\n", uvc_device_info->fival.index, frame_rate);

                            uvc_device_info->format_num++;
                        }

                        uvc_device_info->fival.index++;
                    }

                    uvc_device_info->fsize.index++;
                }
            }

            uvc_device_info->fmt.index++;
        }

        printf("Number of Support Format: %d\n", uvc_device_info->format_num);
        uvc_device_info->stage = UVC_DEVICE_STAGE_GET_FORMAT_NUMBER;
        return uvc_device_info->format_num;
    }
    else{
        LOGE("Error: count format number fail, UVC device is on stage %d\n", uvc_device_info->stage);
        return -1;
    }
}

int store_uvc_format(UvcDeviceInfo *uvc_device_info){
    int format_index = 0;
    if(uvc_device_info->stage == UVC_DEVICE_STAGE_GET_FORMAT_NUMBER){
        uvc_device_info->sup_format = malloc(sizeof(UvcDeviceFormat) * uvc_device_info->format_num);
        memset(uvc_device_info->sup_format, 0, sizeof(UvcDeviceFormat) * uvc_device_info->format_num);
        
        // get format
        uvc_device_info->fmt.index = 0;
        uvc_device_info->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        while (ioctl(uvc_device_info->uvc_dev_fd, VIDIOC_ENUM_FMT, &uvc_device_info->fmt) == 0){
            /*printf("{ format index = %d, pixelformat = \"%c%c%c%c\", description = \"%s\" }\n", uvc_device_info->fmt.index,
                uvc_device_info->fmt.pixelformat & 0xFF, (uvc_device_info->fmt.pixelformat >> 8) & 0xFF, (uvc_device_info->fmt.pixelformat >> 16) & 0xFF, (uvc_device_info->fmt.pixelformat >> 24) & 0xFF, 
                uvc_device_info->fmt.description);*/

            // get resolution for one format
            uvc_device_info->fsize.index = 0;
            uvc_device_info->fsize.pixel_format = uvc_device_info->fmt.pixelformat;
            while(ioctl(uvc_device_info->uvc_dev_fd, VIDIOC_ENUM_FRAMESIZES, &uvc_device_info->fsize) == 0){
                if (uvc_device_info->fsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                    /*printf("  { frame size index = %d, discrete: width = %u, height = %u }\n", uvc_device_info->fsize.index,
                        uvc_device_info->fsize.discrete.width, uvc_device_info->fsize.discrete.height);*/

                    // get frame rate for one format, one resolution
                    uvc_device_info->fival.index = 0;
                    uvc_device_info->fival.pixel_format = uvc_device_info->fmt.pixelformat;
                    uvc_device_info->fival.width = uvc_device_info->fsize.discrete.width;
                    uvc_device_info->fival.height = uvc_device_info->fsize.discrete.height;
                    while(ioctl(uvc_device_info->uvc_dev_fd, VIDIOC_ENUM_FRAMEINTERVALS, &uvc_device_info->fival) == 0){
                        if (uvc_device_info->fival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {

                            uvc_device_info->sup_format[format_index].index  = format_index;
                            uvc_device_info->sup_format[format_index].fival.pixel_format = uvc_device_info->fmt.pixelformat;
                            uvc_device_info->sup_format[format_index].fival.width = uvc_device_info->fsize.discrete.width;
                            uvc_device_info->sup_format[format_index].fival.height = uvc_device_info->fsize.discrete.height;
                            uvc_device_info->sup_format[format_index].fival.discrete.numerator = uvc_device_info->fival.discrete.numerator;
                            uvc_device_info->sup_format[format_index].fival.discrete.denominator = uvc_device_info->fival.discrete.denominator;
                            
                            format_index++;
                        }

                        uvc_device_info->fival.index++;
                    }

                    uvc_device_info->fsize.index++;
                }
            }

            uvc_device_info->fmt.index++;
        }
        uvc_device_info->stage = UVC_DEVICE_STAGE_STORE_FORMAT;
        return format_index;
    }
    else{
        LOGE("Error: store format fail, UVC device is on stage %d\n", uvc_device_info->stage);
        return -1;
    }
}

void print_support_format(UvcDeviceInfo *uvc_device_info){
    int i;
    if(uvc_device_info->stage >= UVC_DEVICE_STAGE_STORE_FORMAT){
        for(i = 0; i < uvc_device_info->format_num; i++){
            printf("Index: %d F: %c%c%c%c, W: %d, H: %d, FPS: %.3f\n",
                uvc_device_info->sup_format[i].index,
                uvc_device_info->sup_format[i].fival.pixel_format & 0xFF,
                (uvc_device_info->sup_format[i].fival.pixel_format >> 8) & 0xFF,
                (uvc_device_info->sup_format[i].fival.pixel_format >> 16) & 0xFF,
                (uvc_device_info->sup_format[i].fival.pixel_format >> 24) & 0xFF,
                uvc_device_info->sup_format[i].fival.width,
                uvc_device_info->sup_format[i].fival.height,
                (float)uvc_device_info->sup_format[i].fival.discrete.denominator/(float)uvc_device_info->sup_format[i].fival.discrete.numerator);
        }
    }
    else{
        LOGE("Error: print support format fail, UVC device is on stage %d\n", uvc_device_info->stage);
    }
}

int set_uvc_format(UvcDeviceInfo *uvc_device_info, int width, int height, int format){
    int ret, status = -1;
    
    if(uvc_device_info->stage >= UVC_DEVICE_STAGE_OPEN){
        uvc_device_info->set_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        uvc_device_info->set_format.fmt.pix.width = width;
        uvc_device_info->set_format.fmt.pix.height = height;
        uvc_device_info->set_format.fmt.pix.pixelformat = format;
        uvc_device_info->set_format.fmt.pix.field = V4L2_FIELD_ANY;
            
        ret = ioctl(uvc_device_info->uvc_dev_fd, VIDIOC_S_FMT, &uvc_device_info->set_format);
        if(ret < 0){
            printf("Query %s VIDIOC_S_FMT fail!\n", uvc_device_info->uvc_dev_path);
            perror("Error");
        }
        else{
            status = 0;
            uvc_device_info->stage = UVC_DEVICE_STAGE_SET_FORMAT;

            /* Check format */
            if((uvc_device_info->set_format.fmt.pix.width != width) || (uvc_device_info->set_format.fmt.pix.height != height)){
                printf("Driver: reset width = %d, height = %d\n", uvc_device_info->set_format.fmt.pix.width, uvc_device_info->set_format.fmt.pix.height);
            }
            else{
                printf("Set width = %d, height = %d\n", uvc_device_info->set_format.fmt.pix.width, uvc_device_info->set_format.fmt.pix.height);
            }
                
            if(uvc_device_info->set_format.fmt.pix.pixelformat != V4L2_PIX_FMT_MJPEG){
                if(uvc_device_info->set_format.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG){
                    printf("Not support YUYV\n");
                }
                else if(uvc_device_info->set_format.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV){
                    printf("Not support MJPEG\n");
                }
            }
            else{
                if(uvc_device_info->set_format.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG){
                    printf("Set format = MJPEG\n");
                }
                else if(uvc_device_info->set_format.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV){
                    printf("Set format = YUYV\n");
                }
            }
        }
    }
    else{
        printf("Error: set format fail, UVC device is on stage %d\n", uvc_device_info->stage);
    }

    return status;
}

int set_uvc_fps(UvcDeviceInfo *uvc_device_info, int frame_interval_numerator, int frame_interval_denominator){
    int ret, status = -1;
    if(uvc_device_info->stage == UVC_DEVICE_STAGE_SET_FORMAT){
		/* Set frame rate */
		uvc_device_info->set_fps = (struct v4l2_streamparm *) calloc(1, sizeof(struct v4l2_streamparm));
		memset(uvc_device_info->set_fps, 0, sizeof(struct v4l2_streamparm));
		uvc_device_info->set_fps->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		uvc_device_info->set_fps->parm.capture.timeperframe.numerator = frame_interval_numerator;
		uvc_device_info->set_fps->parm.capture.timeperframe.denominator = frame_interval_denominator;
		
		ret = ioctl(uvc_device_info->uvc_dev_fd, VIDIOC_G_PARM, uvc_device_info->set_fps);
		if(ret < 0){
			printf("Query %s VIDIOC_G_PARM (frame rate setting) fail!\n", uvc_device_info->uvc_dev_path);
		}
		else{
            status = 0;
            uvc_device_info->stage = UVC_DEVICE_STAGE_SET_FPS;
            
			/* Check frame rate */
			float frame_rate = (float)uvc_device_info->set_fps->parm.capture.timeperframe.denominator/uvc_device_info->set_fps->parm.capture.timeperframe.numerator;
			if((uvc_device_info->set_fps->parm.capture.timeperframe.numerator != frame_interval_numerator) || (uvc_device_info->set_fps->parm.capture.timeperframe.denominator != frame_interval_denominator)){
				frame_rate = (float)uvc_device_info->set_fps->parm.capture.timeperframe.denominator/uvc_device_info->set_fps->parm.capture.timeperframe.numerator;
				printf("Driver: reset frame rate = %.3f\n", frame_rate);
			}
			else{
				printf("Set frame rate = %.3f\n", frame_rate);
			}
		}
    }
    else{
        printf("Error: set format fail, UVC device is on stage %d\n", uvc_device_info->stage);
    }

    return status;
}

int create_uvc_buf(UvcDeviceInfo *uvc_device_info){
    int i, ret, status = -1;
    if(uvc_device_info->stage == UVC_DEVICE_STAGE_SET_FPS){
		/* Request buffers */
		memset(&uvc_device_info->req_buf, 0, sizeof(struct v4l2_requestbuffers ));
		uvc_device_info->req_buf.count = NUMBER_OF_BUFFERS;
		uvc_device_info->req_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		uvc_device_info->req_buf.memory = V4L2_MEMORY_MMAP;
		
		ret = ioctl(uvc_device_info->uvc_dev_fd, VIDIOC_REQBUFS, &uvc_device_info->req_buf);
		if(ret < 0){
			printf("Query %s VIDIOC_REQBUFS (allocate buffers) fail!\n", uvc_device_info->uvc_dev_path);
            perror("Error");
            return status;
		}
		else{
			printf("Number of request buffers = %d\n", uvc_device_info->req_buf.count);
			uvc_device_info->stage = UVC_DEVICE_STAGE_REQ_BUF;
            
			/* Map buffers */
			printf("Start map buffers\n");
			for(i = 0; i < uvc_device_info->req_buf.count; i++){
				memset(&uvc_device_info->buf, 0, sizeof(struct v4l2_buffer));
				uvc_device_info->buf.index = i;
				uvc_device_info->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				uvc_device_info->buf.memory = V4L2_MEMORY_MMAP;
				ret = ioctl(uvc_device_info->uvc_dev_fd, VIDIOC_QUERYBUF, &uvc_device_info->buf);
				if(ret < 0) {
					printf("Query %s VIDIOC_QUERYBUF (query buffer) fail!\n", uvc_device_info->uvc_dev_path);
                    perror("Error");
                    return status;
				}
				else{
					/* Show buffer info */
					printf("Buffer index: %d, length: %u, offset: %u\n", uvc_device_info->buf.index, uvc_device_info->buf.length, uvc_device_info->buf.m.offset);
					
					// Map
					uvc_device_info->memory[i] = mmap(0 /* start anywhere */ ,
                          uvc_device_info->buf.length,
						  PROT_READ | PROT_WRITE,
						  MAP_SHARED,
						  uvc_device_info->uvc_dev_fd,
                          uvc_device_info->buf.m.offset);
					
					if(uvc_device_info->memory[i] == MAP_FAILED){
						printf("Error: map buffer fail!\n");
                        return status;
					}

					printf("Memory[%d] address = %p\n", i, uvc_device_info->memory[i]);
				}
			}
            uvc_device_info->stage = UVC_DEVICE_STAGE_MAP_BUF;
            
			uvc_device_info->jpeg_buf = (char*)malloc(uvc_device_info->buf.length*2);
			printf("Allocate buffer for JPEG data: %d bytes\n", uvc_device_info->buf.length*2);
            uvc_device_info->stage = UVC_DEVICE_STAGE_JPEG_BUF;
		}

		/* Queue buffer */
		for(i = 0; i < uvc_device_info->req_buf.count; i++) {
			memset(&uvc_device_info->buf, 0, sizeof(struct v4l2_buffer));
			uvc_device_info->buf.index = i;
			uvc_device_info->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			uvc_device_info->buf.memory = V4L2_MEMORY_MMAP;
			
			ret = ioctl(uvc_device_info->uvc_dev_fd, VIDIOC_QBUF, &uvc_device_info->buf);
			if(ret < 0) {
				printf("Query %s VIDIOC_QBUF (queue buffer) fail!\n", uvc_device_info->uvc_dev_path);
                return status;
			}
		}
        printf("Queue all buffer first\n");
        status = 0;
    }
    else{
        printf("Error: create buffer fail, UVC device is on stage %d\n", uvc_device_info->stage);
    }

    return status;
}

int start_uvc_capture(UvcDeviceInfo *uvc_device_info){
    int ret, status = -1;
    if(uvc_device_info->stage == UVC_DEVICE_STAGE_JPEG_BUF){
		int buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ret = ioctl(uvc_device_info->uvc_dev_fd, VIDIOC_STREAMON, &buf_type);
		if(ret < 0){
			printf("Query %s VIDIOC_STREAMON (start capture) fail!\n", uvc_device_info->uvc_dev_path);
		}
        else{
            status = 0;
            printf("Start capture\n");
            uvc_device_info->stage = UVC_DEVICE_STAGE_START_CAPTURE;
        }
    }
    else{
        printf("Error: start capture fail, UVC device is on stage %d\n", uvc_device_info->stage);
    }

    return status;
}

int stop_uvc_capture(UvcDeviceInfo *uvc_device_info){
    int ret, status = -1;
    if(uvc_device_info->stage == UVC_DEVICE_STAGE_START_CAPTURE){
		int buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ret = ioctl(uvc_device_info->uvc_dev_fd, VIDIOC_STREAMOFF, &buf_type);
		if(ret < 0){
			printf("Query %s VIDIOC_STREAMOFF (stop capture) fail!\n", uvc_device_info->uvc_dev_path);
		}
        else{
            status = 0;
            printf("Stop capture\n");
            uvc_device_info->stage = UVC_DEVICE_STAGE_JPEG_BUF;
        }
    }
    else{
        printf("Error: stop capture fail, UVC device is on stage %d\n", uvc_device_info->stage);
    }

    return status;
}

int get_vuc_frame(UvcDeviceInfo *uvc_device_info){
    int ret, status = -1;
    if(uvc_device_info->stage == UVC_DEVICE_STAGE_START_CAPTURE){
        memset(&uvc_device_info->buf, 0, sizeof(struct v4l2_buffer));
            uvc_device_info->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            uvc_device_info->buf.memory = V4L2_MEMORY_MMAP;
        
            ret = ioctl(uvc_device_info->uvc_dev_fd, VIDIOC_DQBUF, &uvc_device_info->buf);
            if(ret < 0){
                printf("Query %s VIDIOC_DQBUF (dequeue buffer) fail!\n", uvc_device_info->uvc_dev_path);
                return status;
            }
            
            unsigned long time_in_micros;
            gettimeofday(&uvc_device_info->tv,NULL);
            time_in_micros = 1000000 * uvc_device_info->tv.tv_sec + uvc_device_info->tv.tv_usec;
            //printf("buf.index = %02d, buf.bytesused = %d, timestamp = %ld\n", uvc_device_info->buf.index, uvc_device_info->buf.bytesused, (time_in_micros)/1000);
            
            uvc_device_info->jpeg_buf_used_size = write_jpg_memory(uvc_device_info->memory[uvc_device_info->buf.index], uvc_device_info->buf.bytesused, uvc_device_info->jpeg_buf);

        /* Requeue buffer */
        ret = ioctl(uvc_device_info->uvc_dev_fd, VIDIOC_QBUF, &uvc_device_info->buf);
        if(ret < 0) {
            printf("Query %s VIDIOC_QBUF (requeue buffer) fail!\n", uvc_device_info->uvc_dev_path);
            return status;
        }
        status = 0;

        // show FPS
        if(uvc_device_info->fps_count == 0){
            gettimeofday(&uvc_device_info->tv,NULL);
            uvc_device_info->fps_count_start_time_us = 1000000 * uvc_device_info->tv.tv_sec + uvc_device_info->tv.tv_usec;
        }
        
        gettimeofday(&uvc_device_info->tv, NULL);
        uvc_device_info->fps_count_cur_time_us = 1000000 * uvc_device_info->tv.tv_sec + uvc_device_info->tv.tv_usec;
        uvc_device_info->fps_count++;
        
        if((uvc_device_info->fps_count_cur_time_us - uvc_device_info->fps_count_start_time_us) >= uvc_device_info->fps_show_interval*1000000){
            LOGI("UVC: %s, Name: %s, Resolution: %d x %d, FPS = %d", uvc_device_info->uvc_dev_path, uvc_device_info->cap.card, uvc_device_info->set_format.fmt.pix.width, uvc_device_info->set_format.fmt.pix.height, uvc_device_info->fps_count/uvc_device_info->fps_show_interval);
            uvc_device_info->fps_count = 0;
        }

    }
    else{
        LOGE("Error: get frame fail, UVC device is on stage %d\n", uvc_device_info->stage);
    }

    return status;
}
