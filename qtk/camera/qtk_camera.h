#ifndef QBL_CAMERA_QBL_CAMERA_H
#define QBL_CAMERA_QBL_CAMERA_H
#pragma once
#include "qtk/image/qtk_image.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *data;
    size_t len;
    double timestamp;
    int index;
} qtk_camera_frame_t;

typedef void *qtk_camera_t;

qtk_camera_t qtk_camera_new(const char *device_name, int blocking,
                            int num_buffers);
void qtk_camera_delete(qtk_camera_t cam);
int qtk_camera_set_resolution(qtk_camera_t cam, int width, int height);
int qtk_camera_set_fmt(qtk_camera_t cam, qtk_image_fmt_t fmt);
int qtk_camera_set_fps(qtk_camera_t cam, int fps);
int qtk_camera_cap_frame(qtk_camera_t cam, qtk_camera_frame_t *frame);
int qtk_camera_release_frame(qtk_camera_t cam, int index);
int qtk_camera_start(qtk_camera_t cam);
int qtk_camera_stop(qtk_camera_t cam);

#ifdef __cplusplus
};
#endif
#endif
