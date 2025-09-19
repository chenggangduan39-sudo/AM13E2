#include "qtk_cv_detection_body.h"

static const int stride[5] = {8, 16, 32, 64, 128};

static void qtk_detection_person_process(qtk_cv_detection_body_t *body,
                                         float *preds) {
    const int len = body->reg_max;
    for (int i = 0; i < body->num_stages; i++) {
        int body_x = body->body_desc.width / stride[i];
        if (body->body_desc.width % stride[i] != 0)
            body_x += 1;
        int body_y = body->body_desc.height / stride[i];
        if (body->body_desc.height % stride[i] != 0)
            body_y += 1;
        for (int n = 0; n < body_y; n++) {
            for (int m = 0; m < body_x; m++) {
                float score = preds[0];
                if (score > body->iou) {
                    float cen_x = m * stride[i];
                    float cen_y = n * stride[i];
                    float x1 = cen_x - preds[1] * stride[i];
                    float x2 = cen_x + preds[3] * stride[i];
                    float y1 = cen_y - preds[2] * stride[i];
                    float y2 = cen_y + preds[4] * stride[i];
                    qtk_cv_detection_onnx_box_push(&body->shape, x1, y1, x2, y2,
                                                   score);
                }
                preds += len;
            }
        }
    }
}

qtk_cv_detection_body_t *qtk_cv_detection_body_new() {
    qtk_cv_detection_body_t *body =
        (qtk_cv_detection_body_t *)wtk_malloc(sizeof(qtk_cv_detection_body_t));
    body->input = (float *)wtk_malloc(1024 * 640 * 3 * sizeof(float));
    body->body_desc.width = 1024;
    body->body_desc.height = 640;
    body->sv_shape[0] = 1;
    body->sv_shape[1] = 3;
    body->sv_shape[2] = 640;
    body->sv_shape[3] = 1024;
    for (int i = 0; i < 3; i++) {
        body->svd[i] = 127.5;
        body->std[i] = 128;
    }
    body->reg_max = 5;
    body->num_stages = 5;
    body->data0 = (uint8_t *)wtk_malloc(3840 * 3840 * 3);
    body->data1 = (uint8_t *)wtk_malloc(body->body_desc.width *
                                        body->body_desc.height * 3*10);
    return body;
}

void qtk_cv_detection_body_detele(qtk_cv_detection_body_t *body) {
    wtk_free(body->input);
    wtk_free(body->data1);
    wtk_free(body->data0);
    wtk_free(body);
    body = NULL;
}

void qtk_cv_detection_body_process(qtk_cv_detection_body_t *body,
                                   uint8_t *imagedata, invoke_t invoke) {
    float *output;
    body->res.cnt = 0;
    body->shape.cnt = 0;
    invoke(body->input, &output, imagedata, body->svd, body->std,
           body->sv_shape, body->invo, 13640 * 5);
    qtk_detection_person_process(body, output);
    qtk_cv_detection_onnx_nms(&body->shape, &body->res, body->conf,
                              body->body_desc);
}

void qtk_cv_detection_body_process_data(uint8_t *imagedata,
                                        qtk_cv_detection_body_t *body,
                                        invoke_t invoke, qtk_image_desc_t *desc,
                                        resize_t resize) {

    const float xy = (float)desc->width / (float)desc->height;
    if (desc->width >= desc->height) {
        body->img_desc.width = body->body_desc.width;
        body->img_desc.height = (int)floor((float)body->img_desc.width / xy);
        const int t = (desc->height - body->img_desc.height) / 2;
        if (desc->width == body->img_desc.width) {
            memset(body->data1, 127,
                   body->body_desc.width * body->body_desc.height * 3);
            memcpy(body->data1 + t * desc->width * 3, imagedata,
                   desc->width * desc->height * 3);
        } else {
            resize(imagedata, desc, body->data0, &body->img_desc);
            memset(body->data1, 127,
                   body->body_desc.width * body->body_desc.height * 3);
            int k = (body->body_desc.height - body->img_desc.height) / 2;
            if(k<0){
                k=0;
            }
            // printf(">>>>>>>>>>>>>>>>>>>%d %d %d %d %d\n",body->body_desc.height,body->body_desc.width,body->img_desc.width,body->img_desc.height,k);
            memmove(body->data1 + k * body->body_desc.width * 3, body->data0,
                   body->img_desc.width * body->img_desc.height * 3);
        }
        qtk_cv_detection_body_process(body, body->data1, invoke);
        const int k = (body->body_desc.height - body->img_desc.height) / 2;
        for (int i = 0; i < body->res.cnt; i++) {
            body->res.box[i].roi.y2 =
                min(body->img_desc.height + k, body->res.box[i].roi.y2);
            body->res.box[i].roi.y2 -= k;
            body->res.box[i].roi.y1 -= k;
        }
        float w_dio = (float)desc->width / (float)body->img_desc.width;
        float h_dio = (float)desc->height / (float)body->img_desc.height;
        for (int i = 0; i < body->res.cnt; i++) {
            body->res.box[i].roi.x1 =
                max(floor(body->res.box[i].roi.x1 * w_dio), 0);
            body->res.box[i].roi.y1 =
                max(floor(body->res.box[i].roi.y1 * h_dio), 0);
            body->res.box[i].roi.x2 =
                min(floor(body->res.box[i].roi.x2 * w_dio), desc->width);
            body->res.box[i].roi.y2 =
                min(floor(body->res.box[i].roi.y2 * h_dio), desc->height);
        }
        return;
    } else {
        wtk_debug("Temporarily not supported .");
        return;
    }
}
