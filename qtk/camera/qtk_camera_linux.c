
#include "qtk/camera/qtk_camera.h"
#include "qtk/image/qtk_image.h"
#include "wtk/core/wtk_alloc.h"
#include "wtk/core/wtk_str.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static int xioctl(int fd, int request, void *arg) {
    int ret;
    while ((ret = ioctl(fd, request, arg)) == -1 && errno == EINTR)
        ;
    return ret;
}

typedef struct {
    int width;
    int height;
    qtk_image_fmt_t fmt;
    int fd;
    wtk_string_t *buffers;
    int num_buffers;
    int fps;
} impl_v4l2_t;

int qtk_camera_set_fps(qtk_camera_t cam, int fps) {
    impl_v4l2_t *impl = cast(impl_v4l2_t *, cam);

    struct v4l2_streamparm setfps = {0};
    setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    setfps.parm.capture.timeperframe.numerator = 1;
    setfps.parm.capture.timeperframe.denominator = fps;

    if (xioctl(impl->fd, VIDIOC_S_PARM, &setfps)) {
        goto err;
    }

    impl->fps = fps;

    return 0;
err:
    return -1;
}

qtk_camera_t qtk_camera_new(const char *device_name, int blocking,
                            int num_buffers) {
    struct v4l2_capability cap;
    impl_v4l2_t *impl = wtk_malloc(sizeof(impl_v4l2_t));
    if (!impl) {
        goto err;
    }

    impl->width = 1280;
    impl->height = 720;
    impl->fmt = QBL_IMAGE_NV12;
    impl->fd = -1;
    impl->num_buffers = num_buffers;
    impl->buffers = wtk_malloc(sizeof(wtk_string_t) * num_buffers);
    impl->fps = -1;

    if (blocking) {
        impl->fd = open(device_name, O_RDWR);
    } else {
        impl->fd = open(device_name, O_RDWR | O_NONBLOCK);
    }
    if (impl->fd < 0) {
        goto err;
    }

    if (xioctl(impl->fd, VIDIOC_QUERYCAP, &cap)) {
        goto err;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        goto err;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        goto err;
    }

    if (qtk_camera_set_fps(impl, 10)) {
        goto err;
    }

    return cast(qtk_camera_t, impl);
err:
    if (impl) {
        qtk_camera_delete(cast(qtk_camera_t, impl));
    }
    return NULL;
}

void qtk_camera_delete(qtk_camera_t cam) {
    impl_v4l2_t *impl = cast(impl_v4l2_t *, cam);

    if (impl->fd > 0) {
        close(impl->fd);
        impl->fd = -1;
    }
    wtk_free(cam);
}

int qtk_camera_set_resolution(qtk_camera_t cam, int width, int height) {
    impl_v4l2_t *impl = cast(impl_v4l2_t *, cam);

    impl->width = width;
    impl->height = height;

    return 0;
}

int qtk_camera_set_fmt(qtk_camera_t cam, qtk_image_fmt_t fmt) {
    impl_v4l2_t *impl = cast(impl_v4l2_t *, cam);
    struct v4l2_fmtdesc fmtdesc = {0};
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    uint32_t fmt_fourcc = 0;

    switch (fmt) {
    case QBL_IMAGE_MJPEG:
        fmt_fourcc = v4l2_fourcc('M', 'J', 'P', 'G');
        break;
    case QBL_IMAGE_NV12:
        fmt_fourcc = v4l2_fourcc('N', 'V', '1', '2');
        break;
    case QBL_IMAGE_RGB24:
        fmt_fourcc = v4l2_fourcc('R', 'G', 'B', '3');
        break;
    default:;
    }

    while (0 == xioctl(impl->fd, VIDIOC_ENUM_FMT, &fmtdesc)) {
        if (fmtdesc.pixelformat == fmt_fourcc) {
            impl->fmt = fmt;
            return 0;
        }
        fmtdesc.index++;
    }

    return -1;
}

int qtk_camera_cap_frame(qtk_camera_t cam, qtk_camera_frame_t *frame) {
    struct v4l2_buffer buf = {0};
    impl_v4l2_t *impl = cast(impl_v4l2_t *, cam);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (xioctl(impl->fd, VIDIOC_DQBUF, &buf)) {
        wtk_debug("cap failed\n");
        return -1;
    }
    frame->data = (uint8_t *)impl->buffers[buf.index].data;
    frame->len = impl->buffers[buf.index].len;
    frame->index = buf.index;
    frame->timestamp = buf.timestamp.tv_sec + buf.timestamp.tv_usec * 1e-6;
    return 0;
}

int qtk_camera_release_frame(qtk_camera_t cam, int index) {
    struct v4l2_buffer buf = {0};
    impl_v4l2_t *impl = cast(impl_v4l2_t *, cam);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = index;

    int ret = xioctl(impl->fd, VIDIOC_QBUF, &buf);
    return ret;
}

int qtk_camera_start(qtk_camera_t cam) {
    impl_v4l2_t *impl = cast(impl_v4l2_t *, cam);
    struct v4l2_format fmt = {0};
    struct v4l2_buffer buf = {0};
    struct v4l2_requestbuffers req = {0};
    int i;

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = impl->width;
    fmt.fmt.pix.height = impl->height;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    switch (impl->fmt) {
    case QBL_IMAGE_MJPEG:
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        break;
    case QBL_IMAGE_NV12:
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
        break;
    case QBL_IMAGE_RGB24:
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
        break;
    default:;
    }

    if (xioctl(impl->fd, VIDIOC_S_FMT, &fmt)) {
        goto err;
    }

    req.count = impl->num_buffers;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (xioctl(impl->fd, VIDIOC_REQBUFS, &req)) {
        goto err;
    }
    for (i = 0; i < impl->num_buffers; i++) {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (xioctl(impl->fd, VIDIOC_QUERYBUF, &buf)) {
            goto err;
        }
        impl->buffers[i].len = buf.length;
        impl->buffers[i].data = mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                     MAP_SHARED, impl->fd, buf.m.offset);
        if (qtk_camera_release_frame(impl, i)) {
            goto err;
        }
    }
    if (xioctl(impl->fd, VIDIOC_STREAMON, &buf.type)) {
        goto err;
    }
    return 0;
err:
    return -1;
}

int qtk_camera_stop(qtk_camera_t cam) {
    impl_v4l2_t *impl = cast(impl_v4l2_t *, cam);
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int i;
    if (ioctl(impl->fd, VIDIOC_STREAMOFF, &type)) {
        goto err;
    }
    for (i = 0; i < impl->num_buffers; i++) {
        munmap(impl->buffers[i].data, impl->buffers[i].len);
    }
err:
    return 0;
}
