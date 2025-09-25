#include "qtk_cv_detection_face.h"

static const int steps[] = {8, 16, 32, 64, 128, 256};

qtk_cv_detection_face_t *qtk_cv_detection_face_new() {
    qtk_cv_detection_face_t *face =
        (qtk_cv_detection_face_t *)wtk_malloc(sizeof(qtk_cv_detection_face_t));
    face->input = (float *)wtk_malloc(544 * 960 * 3 * sizeof(float));
    face->sv_shape[0] = 1;
    face->sv_shape[1] = 3;
    face->sv_shape[2] = 544;
    face->sv_shape[3] = 960;
    face->face_desc.width = 960;
    face->face_desc.height = 544;
    for (int i = 0; i < 3; i++) {
        face->svd[i] = 127.5;
        face->std[i] = 128;
    }
    face->anchors_num = 2;
    face->len = 15;
    face->steps_num = 3;
    face->data0 = (uint8_t *)wtk_malloc(3840 * 2400 * 3);
    face->data1 = (uint8_t *)wtk_malloc(face->face_desc.width *
                                        face->face_desc.height * 3);
    return face;
}

void qtk_cv_detection_face_delete(qtk_cv_detection_face_t *face) {
    wtk_free(face->data0);
    wtk_free(face->data1);
    wtk_free(face->input);
    wtk_free(face);
    face = NULL;
}

static void qtk_face_data_process(qtk_cv_detection_face_t *face, float *preds) {
    const int len = face->len;
    for (int i = 0; i < face->steps_num; i++) {
        int face_x = face->face_desc.width / steps[i];
        int face_y = face->face_desc.height / steps[i];
        for (int n = 0; n < face_y; n++) {
            for (int m = 0; m < face_x; m++) {
                for (int k = 0; k < face->anchors_num; k++) {
                    float score = max(preds[0], 0);
                    if (score > face->iou) {
                        float cen_x = m * steps[i];
                        float cen_y = n * steps[i];
                        float x1 = cen_x - preds[1] * steps[i];
                        float y1 = cen_y - preds[2] * steps[i];
                        float x2 = cen_x + preds[3] * steps[i];
                        float y2 = cen_y + preds[4] * steps[i];
                        int num = face->shape.cnt;
                        qtk_cv_detection_onnx_box_push(&face->shape, x1, y1, x2,
                                                       y2, score);
                        for (int h = 0; h < 10; h++) {
                            if (h % 2 == 0)
                                face->shape.box[num].data[h / 2] =
                                    cen_x + preds[5 + h] * steps[i];
                            else
                                face->shape.box[num].data[h / 2 + 5] =
                                    cen_y + preds[5 + h] * steps[i];
                        }
                    }
                    preds += len;
                }
            }
        }
    }
}

static void qtk_face_data_nms(qtk_cv_detection_onnx_box_t *shape,
                              qtk_cv_detection_onnx_box_t *res, float iou,
                              qtk_image_desc_t dst_desc) {
    qsort(shape->box, shape->cnt, sizeof(shape->box[0]),
          qtk_cv_detection_onnx_box_cmp);
    float varea[1000] = {0};
    for (int i = 0; i < shape->cnt; i++) {
        varea[i] = (shape->box[i].roi.x2 - shape->box[i].roi.x1 + 1) *
                   (shape->box[i].roi.y2 - shape->box[i].roi.y1 + 1);
    }
    uint8_t isSuppressed[1000] = {0};
    for (int i = 0; i < shape->cnt; i++) {
        if (isSuppressed[i])
            continue;
        for (int j = i + 1; j < shape->cnt; j++) {
            if (isSuppressed[j])
                continue;
            float x1 = max(shape->box[i].roi.x1, shape->box[j].roi.x1);
            float y1 = max(shape->box[i].roi.y1, shape->box[j].roi.y1);
            float x2 = min(shape->box[i].roi.x2, shape->box[j].roi.x2);
            float y2 = min(shape->box[i].roi.y2, shape->box[j].roi.y2);
            float w = max(0, x2 - x1 + 1);
            float h = max(0, y2 - y1 + 1);
            float inter = w * h;
            float ovr = inter / (varea[i] + varea[j] - inter);
            if (ovr > iou)
                isSuppressed[j] = 1;
        }
    }
    int num = res->cnt;
    for (int i = 0; i < shape->cnt; i++) {
        if (isSuppressed[i])
            continue;
        res->box[num].roi.x1 = max(0, floor(shape->box[i].roi.x1));
        res->box[num].roi.y1 = max(0, floor(shape->box[i].roi.y1));
        res->box[num].roi.x2 = min(dst_desc.width, floor(shape->box[i].roi.x2));
        res->box[num].roi.y2 =
            min(dst_desc.height, floor(shape->box[i].roi.y2));
        memcpy(res->box[num].data, shape->box[i].data, 10 * sizeof(float));
        ++num;
    }
    res->cnt = num;
}

void qtk_cv_detection_face_process(qtk_cv_detection_face_t *face,
                                   uint8_t *imagedata, invoke_t invoke) {
    float *output;
    face->shape.cnt = 0;
    face->res.cnt = 0;
    invoke(face->input, &output, imagedata, face->svd, face->std,
           face->sv_shape, face->invo, 21420 * 15);
    qtk_face_data_process(face, output);
    qtk_face_data_nms(&face->shape, &face->res, face->conf, face->face_desc);
    float img = (float)face->img_desc.width / (float)face->img_desc.height;
    float dst = (float)face->face_desc.width / (float)face->face_desc.height;
    if (img == dst) {
        float w_dio =
            (float)face->img_desc.width / (float)face->face_desc.width;
        float h_dio =
            (float)face->img_desc.height / (float)face->face_desc.height;
        for (int i = 0; i < face->res.cnt; i++) {
            face->res.box[i].roi.x1 =
                max(0, floor(face->res.box[i].roi.x1 * w_dio));
            face->res.box[i].roi.y1 =
                max(0, floor(face->res.box[i].roi.y1 * h_dio));
            face->res.box[i].roi.x2 = min(
                face->img_desc.width, floor(face->res.box[i].roi.x2 * w_dio));
            face->res.box[i].roi.y2 = min(
                face->img_desc.height, floor(face->res.box[i].roi.y2 * h_dio));
            for (int j = 0; j < 5; j++) {
                face->res.box[i].data[j] =
                    floor(face->res.box[i].data[j] * w_dio);
                face->res.box[i].data[j + 5] =
                    floor(face->res.box[i].data[j] * h_dio);
            }
        }
    } else {
        float xy = (float)face->img_desc.width / (float)face->img_desc.height;
        int height = (int)(face->face_desc.width / xy);
        int x = (int)(face->face_desc.height - height) / 2;
        for (int i = 0; i < face->res.cnt; i++) {
            face->res.box[i].roi.y2 =
                min(face->img_desc.height + x, face->res.box[i].roi.y2);
            face->res.box[i].roi.y2 -= x;
            face->res.box[i].roi.y1 -= x;
            for (int j = 0; j < 5; j++) {
                face->res.box[i].data[j + 5] = min(face->res.box[i].data[j + 5],
                                                   face->img_desc.height + x);
                face->res.box[i].data[j + 5] -= x;
            }
        }
        float xw_dio =
            (float)face->img_desc.width / (float)face->face_desc.width;
        float xh_dio = (float)face->img_desc.height / (float)height;
        for (int i = 0; i < face->res.cnt; i++) {
            face->res.box[i].roi.x1 =
                max(0, floor(face->res.box[i].roi.x1 * xw_dio));
            face->res.box[i].roi.y1 =
                max(0, floor(face->res.box[i].roi.y1 * xh_dio));
            face->res.box[i].roi.x2 = min(
                face->img_desc.width, floor(face->res.box[i].roi.x2 * xw_dio));
            face->res.box[i].roi.y2 = min(
                face->img_desc.height, floor(face->res.box[i].roi.y2 * xh_dio));
            for (int j = 0; j < 5; j++) {
                face->res.box[i].data[j] =
                    floor(face->res.box[i].data[j] * xw_dio);
                face->res.box[i].data[j + 5] =
                    floor(face->res.box[i].data[j] * xh_dio);
            }
        }
    }

    qtk_cv_detection_onnx_box_square(&face->res, face->img_desc);
}

void qtk_cv_detection_face_process_data(qtk_cv_detection_face_t *face,
                                        uint8_t *imagedata,
                                        qtk_image_desc_t *desc, resize_t resize,
                                        invoke_t invoke) {

    const float xy = (float)desc->width / (float)desc->height;
    if (desc->width >= desc->height) {
        face->img_desc.width = face->face_desc.width;
        face->img_desc.height = (int)floor(face->img_desc.width / xy);
        const int t = (desc->height - face->img_desc.height) / 2;
        memset(face->data0, 127, 3840 * 2400 * 3 * sizeof(uint8_t));
        if (desc->width == face->img_desc.width) {
            memcpy(face->data0 + t * desc->width * 3, imagedata,
                   desc->width * desc->height * 3);
        } else {
            resize(imagedata, desc, face->data0, &face->img_desc);
            memset(face->data1, 127,
                   face->face_desc.width * face->face_desc.height * 3);
            int k = (face->face_desc.height - face->img_desc.height);
            memcpy(face->data1 + k * face->face_desc.width * 3, face->data0,
                   face->img_desc.width * face->img_desc.height * 3);
        }
        qtk_cv_detection_face_process(face, face->data1, invoke);
        float w_dio = (float)desc->width / (float)face->img_desc.width;
        float h_dio = (float)desc->height / (float)face->img_desc.height;
        for (int i = 0; i < face->res.cnt; i++) {
            face->res.box[i].roi.x1 =
                max(0, floor(face->res.box[i].roi.x1 * w_dio));
            face->res.box[i].roi.y1 =
                max(0, floor(face->res.box[i].roi.y1 * h_dio));
            face->res.box[i].roi.x2 =
                min(desc->width, floor(face->res.box[i].roi.x2 * w_dio));
            face->res.box[i].roi.y2 =
                min(desc->height, floor(face->res.box[i].roi.y2 * h_dio));
            for (int j = 0; j < 5; j++) {
                face->res.box[i].data[j] =
                    floor(face->res.box[i].data[j] * w_dio);
                face->res.box[i].data[j + 5] =
                    floor(face->res.box[i].data[j] * h_dio);
            }
        }
    } else {
        wtk_debug("Temporarily not supported .");
        return;
    }
}
