//
// Created by CLIFF on 2017/2/9.
// Complement for videodev2.h on Android NDK
//

#ifndef MJPEGPLAYER_VIDEODEV2_COMPLMT_H
#define MJPEGPLAYER_VIDEODEV2_COMPLMT_H
/*
 *	F R A M E   S I Z E   E N U M E R A T I O N
 */
enum v4l2_frmsizetypes {
    V4L2_FRMSIZE_TYPE_DISCRETE	= 1,
    V4L2_FRMSIZE_TYPE_CONTINUOUS	= 2,
    V4L2_FRMSIZE_TYPE_STEPWISE	= 3,
};

/*
 *	F R A M E   R A T E   E N U M E R A T I O N
 */
enum v4l2_frmivaltypes {
    V4L2_FRMIVAL_TYPE_DISCRETE	= 1,
    V4L2_FRMIVAL_TYPE_CONTINUOUS	= 2,
    V4L2_FRMIVAL_TYPE_STEPWISE	= 3,
};


struct v4l2_frmsize_discrete {
    uint32_t			width;		/* Frame width [pixel] */
    uint32_t			height;		/* Frame height [pixel] */
};

struct v4l2_frmsize_stepwise {
    uint32_t			min_width;	/* Minimum frame width [pixel] */
    uint32_t			max_width;	/* Maximum frame width [pixel] */
    uint32_t			step_width;	/* Frame width step size [pixel] */
    uint32_t			min_height;	/* Minimum frame height [pixel] */
    uint32_t			max_height;	/* Maximum frame height [pixel] */
    uint32_t			step_height;	/* Frame height step size [pixel] */
};

struct v4l2_frmsizeenum {
    uint32_t			index;		/* Frame size number */
    uint32_t			pixel_format;	/* Pixel format */
    uint32_t			type;		/* Frame size type the device supports. */

    union {					/* Frame size */
        struct v4l2_frmsize_discrete	discrete;
        struct v4l2_frmsize_stepwise	stepwise;
    };

    uint32_t   reserved[2];			/* Reserved space for future use */
};

struct v4l2_frmival_stepwise {
    struct v4l2_fract	min;		/* Minimum frame interval [s] */
    struct v4l2_fract	max;		/* Maximum frame interval [s] */
    struct v4l2_fract	step;		/* Frame interval step size [s] */
};

struct v4l2_frmivalenum {
    uint32_t			index;		/* Frame format index */
    uint32_t			pixel_format;	/* Pixel format */
    uint32_t			width;		/* Frame width */
    uint32_t			height;		/* Frame height */
    uint32_t			type;		/* Frame interval type the device supports. */

    union {					/* Frame interval */
        struct v4l2_fract		discrete;
        struct v4l2_frmival_stepwise	stepwise;
    };

    __u32	reserved[2];			/* Reserved space for future use */
};

/*
 *	E V E N T S
 */

#define V4L2_EVENT_ALL				0
#define V4L2_EVENT_VSYNC			1
#define V4L2_EVENT_EOS				2
#define V4L2_EVENT_CTRL				3
#define V4L2_EVENT_FRAME_SYNC			4
#define V4L2_EVENT_PRIVATE_START		0x08000000

/* Payload for V4L2_EVENT_VSYNC */
struct v4l2_event_vsync {
    /* Can be V4L2_FIELD_ANY, _NONE, _TOP or _BOTTOM */
    __u8 field;
} __attribute__ ((packed));

/* Payload for V4L2_EVENT_CTRL */
#define V4L2_EVENT_CTRL_CH_VALUE		(1 << 0)
#define V4L2_EVENT_CTRL_CH_FLAGS		(1 << 1)
#define V4L2_EVENT_CTRL_CH_RANGE		(1 << 2)

struct v4l2_event_ctrl {
    __u32 changes;
    __u32 type;
    union {
        __s32 value;
        __s64 value64;
    };
    __u32 flags;
    __s32 minimum;
    __s32 maximum;
    __s32 step;
    __s32 default_value;
};

struct v4l2_event_frame_sync {
    __u32 frame_sequence;
};

struct v4l2_event {
    __u32				type;
    union {
        struct v4l2_event_vsync		vsync;
        struct v4l2_event_ctrl		ctrl;
        struct v4l2_event_frame_sync	frame_sync;
        __u8				data[64];
    } u;
    __u32				pending;
    __u32				sequence;
    struct timespec			timestamp;
    __u32				id;
    __u32				reserved[8];
};

#define V4L2_EVENT_SUB_FL_SEND_INITIAL		(1 << 0)
#define V4L2_EVENT_SUB_FL_ALLOW_FEEDBACK	(1 << 1)

struct v4l2_event_subscription {
    __u32				type;
    __u32				id;
    __u32				flags;
    __u32				reserved[5];
};


/* For Control UVC Device, copy from <linux/videodev2.h> */
#define VIDIOC_ENUM_FRAMESIZES	_IOWR('V', 74, struct v4l2_frmsizeenum)
#define VIDIOC_ENUM_FRAMEINTERVALS _IOWR('V', 75, struct v4l2_frmivalenum)
#define	VIDIOC_DQEVENT		 _IOR('V', 89, struct v4l2_event)
#define	VIDIOC_SUBSCRIBE_EVENT	 _IOW('V', 90, struct v4l2_event_subscription)

#endif //MJPEGPLAYER_VIDEODEV2_COMPLMT_H
