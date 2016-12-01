#include <pthread.h>

#include <com_example_v002060_mjpegplayer_MainActivity.h>
#include <uvc_host.h>

#include <uvc_device.h>
#include "uvc_gadget.h"

#include <android/log.h>
#define LOG_TAG "uvc_device.c"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO , LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN , LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

int stream_flag = 0;
int uvc_device_flag = 1;

extern UvcDeviceInfo gUvcDeviceInfo;
extern pthread_mutex_t lock;

static struct uvc_device *
uvc_open(const char *devname)
{
	struct uvc_device *dev;
	struct v4l2_capability cap;
	int ret;
	int fd;

	fd = open(devname, O_RDWR /*| O_NONBLOCK*/);
	if (fd == -1) {
		LOGE("v4l2 open failed: %s (%d)\n", strerror(errno), errno);
		return NULL;
	}

	LOGI("open succeeded, file descriptor = %d\n", fd);

	ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
	if (ret < 0) {
		LOGE("unable to query device: %s (%d)\n", strerror(errno),
			errno);
		close(fd);
		return NULL;
        }

	LOGI("device is %s on bus %s\n", cap.card, cap.bus_info);

	dev = malloc(sizeof *dev);
	if (dev == NULL) {
		close(fd);
		return NULL;
	}

	memset(dev, 0, sizeof *dev);
	dev->fd = fd;

	return dev;
}

static void
uvc_close(struct uvc_device *dev)
{
	close(dev->fd);
	free(dev->imgdata);
	free(dev->mem);
	free(dev);
}

/* ---------------------------------------------------------------------------
 *  * Video streaming
 *   */

static void
uvc_video_fill_buffer(struct uvc_device *dev, struct v4l2_buffer *buf)
{
	//LOGI("======>uvc_video_fill_buffer");
	unsigned int bpl;
	unsigned int i;

	switch (dev->fcc) {
	case V4L2_PIX_FMT_YUYV:
		/* Fill the buffer with video data. */
		bpl = dev->width * 2;
		for (i = 0; i < dev->height; ++i)
			memset(dev->mem[buf->index] + i*bpl, dev->color++, bpl);

		buf->bytesused = bpl * dev->height;
		break;

	case V4L2_PIX_FMT_MJPEG:
	    pthread_mutex_lock(&lock);
		//memcpy(dev->mem[buf->index], dev->imgdata, dev->imgsize);
		memcpy(dev->mem[buf->index], gUvcDeviceInfo.jpeg_buf, gUvcDeviceInfo.jpeg_buf_used_size);
		//buf->bytesused = dev->imgsize;
		buf->bytesused = gUvcDeviceInfo.jpeg_buf_used_size;
		pthread_mutex_unlock(&lock);
		break;
	}
}

static int
uvc_video_process(struct uvc_device *dev)
{
	struct v4l2_buffer buf;
	int ret;

	//LOGI("######>uvc_video_process");
	memset(&buf, 0, sizeof buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
#if 0
	buf.memory = V4L2_MEMORY_USERPTR;
	buf.m.userptr = (unsigned long)buffers[0].start;
	buf.length = buffers[0].length;

#else
	buf.memory = V4L2_MEMORY_MMAP;
#endif
	if ((ret = ioctl(dev->fd, VIDIOC_DQBUF, &buf)) < 0) {
		LOGE("1.Unable to dequeue buffer: %s (%d).", strerror(errno),errno);
		usleep(1000000);
		return ret;
	}


	uvc_video_fill_buffer(dev, &buf);

	if ((ret = ioctl(dev->fd, VIDIOC_QBUF, &buf)) < 0)
	{
		LOGE("3.Unable to requeue buffer: %s (%d).", strerror(errno),
			errno);
		return ret;
	}

	return 0;
}

static int
uvc_video_reqbufs(struct uvc_device *dev, int nbufs)
{
	struct v4l2_requestbuffers rb;
	struct v4l2_buffer buf;
	unsigned int i;
	int ret;

	LOGI("======>uvc_video_reqbufs\r\n");
	for (i = 0; i < dev->nbufs; ++i)
		munmap(dev->mem[i], dev->bufsize);

	free(dev->mem);
	dev->mem = 0;
	dev->nbufs = 0;

	memset(&rb, 0, sizeof rb);
	rb.count = nbufs;
	rb.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	rb.memory = V4L2_MEMORY_MMAP;

	ret = ioctl(dev->fd, VIDIOC_REQBUFS, &rb);
	if (ret < 0)
	{
		LOGE("Unable to allocate buffers: %s (%d).",
			strerror(errno), errno);
		return ret;
	}

	LOGI("%u buffers allocated.", rb.count);

	/* Map the buffers. */
	dev->mem = malloc(rb.count * sizeof dev->mem[0]);
    if (dev->mem == NULL)
	{
        LOGE("Unable to allocate memory for request buffer");
        return -1;
    }


	for (i = 0; i < rb.count; ++i)
	{
LOGI("{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		memset(&buf, 0, sizeof buf);
		buf.index = i;
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(dev->fd, VIDIOC_QUERYBUF, &buf);
		if (ret < 0)
		{
LOGI("{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
			LOGE("Unable to query buffer %u: %s (%d).\n", i,
				strerror(errno), errno);
			return -1;
		}
		LOGI("length: %u offset: %u\n", buf.length, buf.m.offset);

		dev->mem[i] = mmap(0, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, dev->fd, buf.m.offset);
		if (dev->mem[i] == MAP_FAILED)
		{
LOGI("{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
			LOGE("Unable to map buffer %u: %s (%d)\n", i,
				strerror(errno), errno);
			return -1;
		}
		LOGI("Buffer %u mapped at address %p.\n", i, dev->mem[i]);
	}

	dev->bufsize = buf.length;
	dev->nbufs = rb.count;

	return 0;
}

static int
uvc_video_stream(struct uvc_device *dev, int enable)
{
	struct v4l2_buffer buf;
	unsigned int i;
	int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	int ret;

	LOGI("======>uvc_video_stream\r\n");
	if (!enable) {
		LOGE("Stopping video stream.\n");
		ioctl(dev->fd, VIDIOC_STREAMOFF, &type);
LOGE("WOODY:STREAMOFF{%s}{%d}\r\n", __FILE__, __LINE__);
		return 0;
	}

	LOGI(" ============ Starting video stream.=============\n");

	for (i = 0; i < dev->nbufs; ++i) {
		memset(&buf, 0, sizeof buf);

		buf.index = i;
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_MMAP;

		uvc_video_fill_buffer(dev, &buf);

		LOGI("Queueing buffer %u.\n", i);
		if ((ret = ioctl(dev->fd, VIDIOC_QBUF, &buf)) < 0) {
			LOGE("Unable to queue buffer: %s (%d).\n",
				strerror(errno), errno);
			break;
		}
	}
LOGI("WOODY:STREAMON{%s}{%d}\r\n", __FILE__, __LINE__);
	ioctl(dev->fd, VIDIOC_STREAMON, &type);

	stream_flag = 1;

	return ret;
}

static int
uvc_video_set_format(struct uvc_device *dev)
{
	struct v4l2_format fmt;
	int ret;

	LOGI("======>uvc_video_set_format to 0x%08x %ux%u\n",
		dev->fcc, dev->width, dev->height);

	memset(&fmt, 0, sizeof fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fmt.fmt.pix.width = dev->width;
	fmt.fmt.pix.height = dev->height;
	fmt.fmt.pix.pixelformat = dev->fcc;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	if (dev->fcc == V4L2_PIX_FMT_MJPEG)
		fmt.fmt.pix.sizeimage = dev->imgsize * 1.5;

LOGI("{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
	if ((ret = ioctl(dev->fd, VIDIOC_S_FMT, &fmt)) < 0)
	{
		LOGE("Unable to set format: %s (%d).\n",
			strerror(errno), errno);
	}
LOGI("{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
	return ret;
}

static int
uvc_video_init(struct uvc_device *dev __attribute__((__unused__)))
{
	return 0;
}

/* ---------------------------------------------------------------------------
 *  * Request processing
 *   */

struct uvc_frame_info
{
	unsigned int width;
	unsigned int height;
	unsigned int intervals[8];
};

struct uvc_format_info
{
	unsigned int fcc;
	const struct uvc_frame_info *frames;
};

static const struct uvc_frame_info uvc_frames_yuyv[] = {
	{  640, 360, { 666666, 10000000, 50000000, 0 }, },
	//{ 1280, 720, { 50000000, 0 }, },
	{ 1920, 1080, { 50000000, 0 }, },
	{ 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_mjpeg[] = {
	{  640, 360, { 666666, 10000000, 50000000, 0 }, },
	//{ 1280, 720, { 50000000, 0 }, },
	{ 1920, 1080, { 50000000, 0 }, },
	{ 0, 0, { 0, }, },
};

static const struct uvc_format_info uvc_formats[] = {
	{ V4L2_PIX_FMT_YUYV, uvc_frames_yuyv },
	{ V4L2_PIX_FMT_MJPEG, uvc_frames_mjpeg },
};

static void
uvc_fill_streaming_control(struct uvc_device *dev,
			   struct uvc_streaming_control *ctrl,
			   int iframe, int iformat)
{
	LOGI("======>uvc_fill_streaming_control\r\n");
	const struct uvc_format_info *format;
	const struct uvc_frame_info *frame;
	unsigned int nframes;

	if (iformat < 0)
	{
LOGE("{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		iformat = ARRAY_SIZE(uvc_formats) + iformat;
	}
	if (iformat < 0 || iformat >= (int)ARRAY_SIZE(uvc_formats))
	{
LOGE("{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		return;
	}
	format = &uvc_formats[iformat];

	nframes = 0;
	while (format->frames[nframes].width != 0)
	{
LOGE("{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		++nframes;
	}

	if (iframe < 0)
	{
LOGE("{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		iframe = nframes + iframe;
	}
	if (iframe < 0 || iframe >= (int)nframes)
	{
LOGE("{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		return;
	}
	frame = &format->frames[iframe];

	memset(ctrl, 0, sizeof *ctrl);

	ctrl->bmHint = 1;
	ctrl->bFormatIndex = iformat + 1;
	ctrl->bFrameIndex = iframe + 1;
	ctrl->dwFrameInterval = /*666666;//*/frame->intervals[0];
	switch (format->fcc) {
	case V4L2_PIX_FMT_YUYV:
LOGI("V4L2_PIX_FMT_YUYV{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
		break;
	case V4L2_PIX_FMT_MJPEG:
LOGI("V4L2_PIX_FMT_MJPEG{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		ctrl->dwMaxVideoFrameSize = dev->imgsize;
		break;
	}
	ctrl->dwMaxPayloadTransferSize = 512;	/* TODO this should be filled by the driver. */
	ctrl->bmFramingInfo = 3;
	ctrl->bPreferedVersion = 1;
	ctrl->bMaxVersion = 1;

	LOGI("====== streaming control value ====== top");
	LOGI(" ctrl->bmHint= %d", ctrl->bmHint);
	LOGI(" ctrl->bFormatIndex= %d", ctrl->bFormatIndex);
	LOGI(" ctrl->bFrameIndex= %d", ctrl->bFrameIndex);
	LOGI(" ctrl->dwFrameInterval= %d", ctrl->dwFrameInterval);
	LOGI(" ctrl->dwMaxVideoFrameSize= %d", ctrl->dwMaxVideoFrameSize);
	LOGI(" ctrl->dwMaxPayloadTransferSize= %d", ctrl->dwMaxPayloadTransferSize);
	LOGI(" ctrl->bmFramingInfo= %d", ctrl->bmFramingInfo);
	LOGI(" ctrl->bPreferedVersion= %d", ctrl->bPreferedVersion);
	LOGI(" ctrl->bMaxVersion= %d", ctrl->bMaxVersion);
	LOGI("====== streaming control value ====== end");
}

static void
uvc_events_process_standard(struct uvc_device *dev, struct usb_ctrlrequest *ctrl,
			    struct uvc_request_data *resp)
{
	LOGI("======>uvc_events_process_standard\r\n");
	(void)dev;
	(void)ctrl;
	resp->length = 0;
	//(void)resp;
}

static void
uvc_events_process_control(struct uvc_device *dev, uint8_t req, uint8_t cs,
			   struct uvc_request_data *resp)
{
	LOGI("======>uvc_events_process_control (req %02x cs %02x)\n", req, cs);
	(void)dev;
	//(void)resp;
	resp->length = 0;
}

static void
uvc_events_process_streaming(struct uvc_device *dev, uint8_t req, uint8_t cs,
			     struct uvc_request_data *resp)
{
	struct uvc_streaming_control *ctrl;

	LOGI("======>uvc_events_process_streaming (req %02x cs %02x)\n", req, cs);

	if (cs != UVC_VS_PROBE_CONTROL && cs != UVC_VS_COMMIT_CONTROL)
	{
LOGE("{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		return;
	}
	ctrl = (struct uvc_streaming_control *)&resp->data;
	resp->length = sizeof *ctrl;

	switch (req) {
	case UVC_SET_CUR:
LOGI("UVC_SET_CUR{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		dev->control = cs;
		resp->length = 34;
		break;

	case UVC_GET_CUR:
LOGI("UVC_GET_CUR{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		if (cs == UVC_VS_PROBE_CONTROL)
		{
LOGI("{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
			memcpy(ctrl, &dev->probe, sizeof *ctrl);
		}
		else
		{
LOGI("{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
			memcpy(ctrl, &dev->commit, sizeof *ctrl);
		}
		break;

	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_DEF:
		if(req == UVC_GET_MIN)
			LOGI("UVC_GET_MIN{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		else if(req == UVC_GET_MAX)
			LOGI("UVC_GET_MAX{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		else if(req == UVC_GET_DEF)
			LOGI("UVC_GET_DEF{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		uvc_fill_streaming_control(dev, ctrl, req == UVC_GET_MAX ? -1 : 0, req == UVC_GET_MAX ? -1 : 0);
		//uvc_fill_streaming_control(dev, ctrl, 0, 1);
		break;

	case UVC_GET_RES:
LOGI("UVC_GET_RES{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		memset(ctrl, 0, sizeof *ctrl);
		break;

	case UVC_GET_LEN:
LOGI("UVC_GET_LEN{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		resp->data[0] = 0x00;
		resp->data[1] = 0x22;
		resp->length = 2;
		break;

	case UVC_GET_INFO:
LOGI("UVC_GET_INFO{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		resp->data[0] = 0x03;
		resp->length = 1;
		break;
	}
}

static void
uvc_events_process_class(struct uvc_device *dev, struct usb_ctrlrequest *ctrl,
			 struct uvc_request_data *resp)
{
	LOGI("======>uvc_events_process_class\r\n");
	if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
		return;

	switch (ctrl->wIndex & 0xff) {
	case UVC_INTF_CONTROL:
		uvc_events_process_control(dev, ctrl->bRequest, ctrl->wValue >> 8, resp);
LOGI("UVC_INTF_CONTROL{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		break;

	case UVC_INTF_STREAMING:
		uvc_events_process_streaming(dev, ctrl->bRequest, ctrl->wValue >> 8, resp);
LOGI("UVC_INTF_STREAMING{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		break;

	default:
LOGI("{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		break;
	}
}

static void
uvc_events_process_setup(struct uvc_device *dev, struct usb_ctrlrequest *ctrl,
			 struct uvc_request_data *resp)
{
	LOGI("======>uvc_events_process_setup\r\n");
	dev->control = 0;

	LOGI("bRequestType %02x bRequest %02x wValue %04x wIndex %04x "
		"wLength %04x\n", ctrl->bRequestType, ctrl->bRequest,
		ctrl->wValue, ctrl->wIndex, ctrl->wLength);

	switch (ctrl->bRequestType & USB_TYPE_MASK) {
	case USB_TYPE_STANDARD:
LOGI("USB_TYPE_STANDARD{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		uvc_events_process_standard(dev, ctrl, resp);
		break;

	case USB_TYPE_CLASS:
LOGI("USB_TYPE_CLASS{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		uvc_events_process_class(dev, ctrl, resp);
		break;

	default:
LOGI("{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		break;
	}
}

static void
uvc_events_process_data(struct uvc_device *dev, struct uvc_request_data *data)
{
	LOGI("======>uvc_events_process_data\r\n");
	struct uvc_streaming_control *target;
	struct uvc_streaming_control *ctrl;
	const struct uvc_format_info *format;
	const struct uvc_frame_info *frame;
	const unsigned int *interval;
	unsigned int iformat, iframe;
	unsigned int nframes;

	switch (dev->control) {
	case UVC_VS_PROBE_CONTROL:
		LOGI("setting probe control, length = %d\n", data->length);
		target = &dev->probe;
		break;

	case UVC_VS_COMMIT_CONTROL:
		LOGI("setting commit control, length = %d\n", data->length);
		target = &dev->commit;
		break;

	default:
		LOGI("setting unknown control, length = %d\n", data->length);
		return;
	}

	ctrl = (struct uvc_streaming_control *)&data->data;
	iformat = clamp((unsigned int)ctrl->bFormatIndex, 1U,
			(unsigned int)ARRAY_SIZE(uvc_formats));
	format = &uvc_formats[iformat-1];
	//format = &uvc_formats[iformat];

	nframes = 0;
	while (format->frames[nframes].width != 0)
		++nframes;

	iframe = clamp((unsigned int)ctrl->bFrameIndex, 1U, nframes);
	frame = &format->frames[iframe-1];
	interval = frame->intervals;

	while (interval[0] < ctrl->dwFrameInterval && interval[1])
		++interval;

	target->bFormatIndex = iformat;
	target->bFrameIndex = iframe;
	switch (format->fcc) {
	case V4L2_PIX_FMT_YUYV:
		LOGI("======>V4L2_PIX_FMT_YUYV\r\n");
		target->dwMaxVideoFrameSize = frame->width * frame->height * 2;
		break;
	case V4L2_PIX_FMT_MJPEG:
		LOGI("======>V4L2_PIX_FMT_MJPEG\r\n");
		if (dev->imgsize == 0)
			LOGE("WARNING: MJPEG requested and no image loaded.\n");
		target->dwMaxVideoFrameSize = dev->imgsize;
		break;
	}
	target->dwFrameInterval = *interval;

	if (dev->control == UVC_VS_COMMIT_CONTROL) {
		dev->fcc = format->fcc;
		dev->width = frame->width;
		dev->height = frame->height;

		uvc_video_set_format(dev);
		if (dev->bulk)
			uvc_video_stream(dev, 1);
	}
}

static void
uvc_events_process(struct uvc_device *dev)
{
	LOGI("######>uvc_events_process\r\n");
	struct v4l2_event v4l2_event;
	struct uvc_event *uvc_event = (void *)&v4l2_event.u.data;
	struct uvc_request_data resp;
	int ret;

	ret = ioctl(dev->fd, VIDIOC_DQEVENT, &v4l2_event);
	if (ret < 0) {
		LOGE("VIDIOC_DQEVENT failed: %s (%d)\n", strerror(errno),
			errno);
		return;
	}

	memset(&resp, 0, sizeof resp);
	resp.length = -EL2HLT;

	switch (v4l2_event.type) {
	case UVC_EVENT_CONNECT:
		LOGI("#UVC_EVENT_CONNECT{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
	case UVC_EVENT_DISCONNECT:
		LOGI("#UVC_EVENT_DISCONNECT{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		return;

	case UVC_EVENT_SETUP:
		LOGI("#UVC_EVENT_SETUP{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		uvc_events_process_setup(dev, &uvc_event->req, &resp);
		break;

	case UVC_EVENT_DATA:
		LOGI("#UVC_EVENT_DATA{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		uvc_events_process_data(dev, &uvc_event->data);
		return;

	case UVC_EVENT_STREAMON:
		LOGI("#UVC_EVENT_STREAMON{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		uvc_video_reqbufs(dev, 4);
		uvc_video_stream(dev, 1);
		break;

	case UVC_EVENT_STREAMOFF:
		LOGI("#UVC_EVENT_STREAMOFF{%s}{%s}{%d}\r\n", __FILE__, __FUNCTION__, __LINE__);
		uvc_video_stream(dev, 0);
		uvc_video_reqbufs(dev, 0);
		break;
	}

	ioctl(dev->fd, UVCIOC_SEND_RESPONSE, &resp);
	if (ret < 0) {
		LOGE("UVCIOC_S_EVENT failed: %s (%d)\n", strerror(errno),
			errno);
		return;
	}
}

static void
uvc_events_init(struct uvc_device *dev)
{
	LOGI("======>uvc_events_init\r\n");
	struct v4l2_event_subscription sub;

	uvc_fill_streaming_control(dev, &dev->probe, 0, 0);
	uvc_fill_streaming_control(dev, &dev->commit, 0, 0);
	//uvc_fill_streaming_control(dev, &dev->probe, 0, 1);
	//uvc_fill_streaming_control(dev, &dev->commit, 0, 1);

	if (dev->bulk) {
		/* FIXME Crude hack, must be negotiated with the driver. */
		dev->probe.dwMaxPayloadTransferSize = 16 * 1024;
		dev->commit.dwMaxPayloadTransferSize = 16 * 1024;
		LOGI("WOODY:bulk payload size 16k {%s}{%d}\r\n", __FUNCTION__, __LINE__);
	}


	memset(&sub, 0, sizeof sub);
	sub.type = UVC_EVENT_SETUP;
	ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	sub.type = UVC_EVENT_DATA;
	ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	sub.type = UVC_EVENT_STREAMON;
	ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	sub.type = UVC_EVENT_STREAMOFF;
	ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
}

static void image_load(struct uvc_device *dev, const char *img)
{
	LOGI("======>image_load\r\n");
	int fd = -1;

	if (img == NULL)
		return;

	fd = open(img, O_RDONLY);
	if (fd == -1) {
		LOGE("Unable to open MJPEG image '%s'\n", img);
		return;
	}

	dev->imgsize = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	dev->imgdata = malloc(dev->imgsize);
	if (dev->imgdata == NULL) {
		LOGE("Unable to allocate memory for MJPEG image\n");
		dev->imgsize = 0;
		return;
	}

	int ret = read(fd, dev->imgdata, dev->imgsize);
	if (ret != dev->imgsize)
	{
		LOGE("WOODY: READ IMAGE DATA FAIL!!!\r\n");
	}

	close(fd);
}

int uvc_device_start(){
    char *device = "/dev/video0";
	struct uvc_device *dev;
	char *mjpeg_image = "/sdcard/1080p1.jpg";
	int bulk_mode = 0;
	fd_set fds;
	int ret;

    dev = uvc_open(device);
    if (dev == NULL)
    {
        LOGE("Open %s, failed", device);
        return -1;
    }

    LOGI("Open %s success", device);

    image_load(dev, mjpeg_image);

    dev->bulk = bulk_mode;

    uvc_events_init(dev);
    uvc_video_init(dev);


    FD_ZERO(&fds);
    FD_SET(dev->fd, &fds);

    while (1) {
    	fd_set efds = fds;
    	fd_set wfds = fds;

    	ret = select(dev->fd + 1, NULL, &wfds, &efds, NULL);
    	if (FD_ISSET(dev->fd, &efds))
    		uvc_events_process(dev);
    	if (FD_ISSET(dev->fd, &wfds)){
    		if(stream_flag)
    			uvc_video_process(dev);
    	}
    }

    uvc_close(dev);
    return 0;
}

JNIEXPORT void JNICALL Java_com_example_v002060_mjpegplayer_MainActivity_startUvcDevice(JNIEnv *env, jobject activity){
    char *device = "/dev/video0";
	struct uvc_device *dev;
	char *mjpeg_image = "/sdcard/1080p2.jpg";
	int bulk_mode = 0;
	fd_set fds;
	int ret;

    dev = uvc_open(device);
    if (dev == NULL)
    {
        LOGE("Open %s, failed", device);
        return -1;
    }

    LOGI("Open %s success", device);

    image_load(dev, mjpeg_image);

    dev->bulk = bulk_mode;

    uvc_events_init(dev);
    uvc_video_init(dev);


    FD_ZERO(&fds);
    FD_SET(dev->fd, &fds);

    while (uvc_device_flag) {
    	fd_set efds = fds;
    	fd_set wfds = fds;

    	ret = select(dev->fd + 1, NULL, &wfds, &efds, NULL);
    	if (FD_ISSET(dev->fd, &efds))
    		uvc_events_process(dev);
    	if (FD_ISSET(dev->fd, &wfds)){
    		if(stream_flag)
    			uvc_video_process(dev);
    	}
    }

    uvc_close(dev);
    return 0;
}