#include "qtk_cv_detection_onnx.h"
#include "qtk/cv/qtk_cv_bbox.h"

void qtk_cv_detection_onnx_box_push(qtk_cv_detection_onnx_box_t *shape,
                                    float x1, float y1, float x2, float y2,
                                    float score) {
    int k = shape->cnt;
    if (k == 1000)
        return;
    shape->box[k].roi.x1 = x1;
    shape->box[k].roi.x2 = x2;
    shape->box[k].roi.y1 = y1;
    shape->box[k].roi.y2 = y2;
    shape->box[k].score = score;
    shape->cnt += 1;
}

int qtk_cv_detection_onnx_box_cmp(const void *a, const void *b) {
    return ((*(qtk_cv_detection_res_t *)a).score <
            (*(qtk_cv_detection_res_t *)b).score);
}

void qtk_cv_detection_onnx_box_square(qtk_cv_detection_onnx_box_t *box,
                                      qtk_image_desc_t desc) {
    for (int i = 0; i < box->cnt; i++) {
        int width = (int)(box->box[i].roi.x2 - box->box[i].roi.x1);
        int height = (int)(box->box[i].roi.y2 - box->box[i].roi.y1);
        if (width == height)
            continue;
        if (width > height) {
            int t = (width - height) / 2;
            box->box[i].roi.y1 = floor(max(box->box[i].roi.y1 - t, 0));
            box->box[i].roi.y2 =
                floor(min(box->box[i].roi.y2 + t, desc.height));
        } else {
            int t = (height - width) / 2;
            box->box[i].roi.x1 = floor(max(0, box->box[i].roi.x1 - t));
            box->box[i].roi.x2 = floor(min(desc.width, box->box[i].roi.x2 + t));
        }
    }
}

void qtk_cv_detection_onnx_nms(qtk_cv_detection_onnx_box_t *shape,
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
            if (ovr >= iou)
                isSuppressed[j] = 1;
        }
    }
    int num = res->cnt;
    for (int i = 0; i < shape->cnt; i++) {
        if (isSuppressed[i])
            continue;
        res->box[num].roi.x1 = max(floor(shape->box[i].roi.x1), 0);
        res->box[num].roi.y1 = max(floor(shape->box[i].roi.y1), 0);
        res->box[num].roi.x2 = min(floor(shape->box[i].roi.x2), dst_desc.width);
        res->box[num].roi.y2 =
            min(floor(shape->box[i].roi.y2), dst_desc.height);
        res->box[num].score = shape->box[i].score;
        num++;
    }
    res->cnt = num;
}

void qtk_cv_detection_onnx_softmax(const float *x, float *y, int length) {
    float sum = 0;
    int i = 0;
    for (i = 0; i < length; i++) {
        y[i] = exp(x[i]);
        sum += y[i];
    }
    for (i = 0; i < length; i++) {
        y[i] /= sum;
    }
}

void qtk_cv_detection_onnx_image_rect(uint8_t *imagedata,
                                      qtk_image_desc_t *desc, uint8_t *data,
                                      qtk_cv_bbox_t *box) {
    const int width = (int)box->x2 - (int)box->x1;
    const int height = (int)box->y2 - (int)box->y1;
    const int x1 = (int)box->x1;
    const int y1 = (int)box->y1;
    uint8_t *p = imagedata + y1 * desc->width * 3 + x1 * 3;
    for (int i = 0; i < height; i++) {
        memcpy(data + i * width * 3, p + i * desc->width * 3, width * 3);
    }
}

void qtk_cv_detection_onnx_image_BGRtoRGB(uint8_t *imagedata, uint8_t *data,
                                          const int size) {
    uint8_t *p = imagedata;
    uint8_t *q = data;
    for (int i = 0; i < size; i++) {
        q[0] = p[2];
        q[1] = p[1];
        q[2] = p[0];
        q += 3;
        p += 3;
    }
}

void qtk_cv_detection_onnx_image_BGRAtoBGR(uint8_t *imagedata, uint8_t *data,
                                           const int size) {
    uint8_t *p = imagedata;
    uint8_t *q = data;
    for (int i = 0; i < size; i++) {
        q[0] = p[0];
        q[1] = p[1];
        q[2] = p[2];
        q += 3;
        p += 4;
    }
}

void qtk_cv_detection_onnx_image_BGRAtoRGB(uint8_t *imagedata, uint8_t *data,
                                           const int size) {
    uint8_t *p = imagedata;
    uint8_t *q = data;
    for (int i = 0; i < size; i++) {
        q[0] = p[2];
        q[1] = p[1];
        q[2] = p[0];
        q += 3;
        p += 4;
    }
}

void qtk_cv_detection_onnx_box_stillness(qtk_cv_detection_onnx_box_t *res,
                                         float iou) {
    static int cnt = 0;
    static qtk_cv_bbox_t roi[1000];
    qtk_cv_bbox_t box[1000];
    int box_num = 0;
    if (cnt == 0) {
        for (int i = 0; i < res->cnt; i++) {
            roi[i].x1 = res->box[i].roi.x1;
            roi[i].x2 = res->box[i].roi.x2;
            roi[i].y1 = res->box[i].roi.y1;
            roi[i].y2 = res->box[i].roi.y2;
        }
        cnt = res->cnt;
        res->cnt = 0;
        return;
    } else {
        for (int i = 0; i < res->cnt; i++) {
            for (int j = 0; j < cnt; j++) {
                float k =
                    qtk_cv_bbox_jaccard_overlap(&res->box[i].roi, &roi[j]);
                if (k > iou) {
                    box[box_num].x1 = res->box[i].roi.x1;
                    box[box_num].y1 = res->box[i].roi.y1;
                    box[box_num].x2 = res->box[i].roi.x2;
                    box[box_num].y2 = res->box[i].roi.y2;
                    box_num++;
                    break;
                }
            }
        }
    }
    for (int i = 0; i < res->cnt; i++) {
        roi[i].x1 = res->box[i].roi.x1;
        roi[i].x2 = res->box[i].roi.x2;
        roi[i].y1 = res->box[i].roi.y1;
        roi[i].y2 = res->box[i].roi.y2;
    }
    cnt = res->cnt;
    for (int i = 0; i < box_num; i++) {
        res->box[i].roi.x1 = box[i].x1;
        res->box[i].roi.x2 = box[i].x2;
        res->box[i].roi.y1 = box[i].y1;
        res->box[i].roi.y2 = box[i].y2;
    }
    res->cnt = box_num;
}
