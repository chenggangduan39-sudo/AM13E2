#include "qtk_cv_detection_hand.h"

qtk_cv_detection_hand_t *qtk_cv_detection_hand_new() {
    qtk_cv_detection_hand_t *hand =
        (qtk_cv_detection_hand_t *)wtk_malloc(sizeof(qtk_cv_detection_hand_t));
    hand->input = (float *)wtk_malloc(512 * 512 * 3 * sizeof(float));
    hand->hand_desc.width = 512;
    hand->hand_desc.height = 512;
    hand->sv_shape[0] = 1;
    hand->sv_shape[1] = 3;
    hand->sv_shape[2] = 512;
    hand->sv_shape[3] = 512;
    hand->num_stages = 5376;
    for (int i = 0; i < 3; i++) {
        hand->svd[i] = 0;
        hand->std[i] = 255;
    }
    hand->data0 = (uint8_t *)wtk_malloc(3840 * 2160 * 3);
    hand->data1 = (uint8_t *)wtk_malloc(3840 * 2160 * 3);
    hand->data2 = (uint8_t *)wtk_malloc(hand->hand_desc.width *
                                        hand->hand_desc.height * 3);
    return hand;
}

void qtk_cv_detection_hand_delete(qtk_cv_detection_hand_t *hand) {
    wtk_free(hand->data0);
    wtk_free(hand->data1);
    wtk_free(hand->data2);
    wtk_free(hand->input);
    wtk_free(hand);
    hand = NULL;
}

static void qtk_hand_process(qtk_cv_detection_hand_t *hand, float *preds) {
    const int len = 5;
    for (int i = 0; i < hand->num_stages; i++) {
        float score = preds[4];
        if (score > hand->iou) {
            float width = preds[2] / 2.0;
            float height = preds[3] / 2.0;
            float x1 = preds[0] - width;
            float x2 = preds[0] + width;
            float y1 = preds[1] - height;
            float y2 = preds[1] + height;
            qtk_cv_detection_onnx_box_push(&hand->shape, x1, y1, x2, y2, score);
        }
        preds += len;
    }
}

void qtk_cv_detection_hand_show(int t) {
    switch (t) {
    case 0:
        printf("bg\n");
        break;
    case 1:
        printf("fist\n");
        break;
    case 2:
        printf("like\n");
        break;
    case 3:
        printf("neg_hand\n");
        break;
    case 4:
        printf("ok\n");
        break;
    case 5:
        printf("palm\n");
        break;
    case 6:
        printf("peace\n");
        break;
    default:
        break;
    }
}

// std::string qtk_cv_detection_hand_show3(int t) {
//     switch (t) {
//     case 0:
//         return "bg";
//     case 1:
//         return "fist";
//     case 2:
//         return "like";
//     case 3:
//         return "neg_hand";
//     case 4:
//         return "ok";
//     case 5:
//         return "palm";
//     case 6:
//         return "peace";
//     default:
//         return "";
//     }
// }


// string qtk_cv_detection_hand_show2(int t) {
//     switch (t) {
//     case 0:
//         return("bg\n");
//     case 1:
//         return("fist\n");
//     case 2:
//         return("like\n");
//     case 3:
//         return("neg_hand\n");
//     case 4:
//         return("ok\n");
//     case 5:
//         return("palm\n");
//     case 6:
//         return("peace\n");        
//     }
// }


void qtk_cv_detection_hand_process(qtk_cv_detection_hand_t *hand,
                                   uint8_t *imagedata, invoke_t invoke) {
    hand->res.cnt = 0;
    hand->shape.cnt = 0;
    float *output;
    invoke(hand->input, &output, imagedata, hand->svd, hand->std,
           hand->sv_shape, hand->invo, 5376 * 5);
    qtk_hand_process(hand, output);
    qtk_cv_detection_onnx_nms(&hand->shape, &hand->res, hand->conf,
                              hand->hand_desc);
    float w_dio = (float)hand->img_desc.width / (float)hand->hand_desc.width;
    float h_dio = (float)hand->img_desc.height / (float)hand->hand_desc.height;
    for (int i = 0; i < hand->res.cnt; i++) {
        hand->res.box[i].roi.x1 =
            max(0, floor(hand->res.box[i].roi.x1 * w_dio));
        hand->res.box[i].roi.x2 =
            min(hand->img_desc.width, floor(hand->res.box[i].roi.x2 * w_dio));
        hand->res.box[i].roi.y1 =
            max(0, floor(hand->res.box[i].roi.y1 * h_dio));
        hand->res.box[i].roi.y2 =
            min(hand->img_desc.height, floor(hand->res.box[i].roi.y2 * h_dio));
    }
    qtk_cv_detection_onnx_box_square(&hand->res, hand->img_desc);
}

qtk_cv_detection_hand_gesture_t *qtk_cv_detection_hand_gesture_new() {
    qtk_cv_detection_hand_gesture_t *ges =
        (qtk_cv_detection_hand_gesture_t *)wtk_malloc(
            sizeof(qtk_cv_detection_hand_gesture_t));
    ges->input = (float *)wtk_malloc(128 * 128 * 3 * sizeof(float));
    ges->desc.width = 128;
    ges->desc.height = 128;
    ges->sv_shape[0] = 1;
    ges->sv_shape[1] = 3;
    ges->sv_shape[2] = 128;
    ges->sv_shape[3] = 128;
    for (int i = 0; i < 3; i++) {
        ges->svd[i] = 0;
        ges->std[i] = 255;
    }
    ges->data0 = (uint8_t *)wtk_malloc(3840 * 2160 * 3);
    ges->data1 = (uint8_t *)wtk_malloc(ges->desc.width * ges->desc.height * 3);
    ges->data2 = (uint8_t *)wtk_malloc(ges->desc.width * ges->desc.height * 3);
    return ges;
}

void qtk_cv_detection_hand_gesture_delete(
    qtk_cv_detection_hand_gesture_t *ges) {
    wtk_free(ges->data0);
    wtk_free(ges->data1);
    wtk_free(ges->data2);
    wtk_free(ges->input);
    wtk_free(ges);
    ges = NULL;
}

void qtk_cv_detection_gesture_process(qtk_cv_detection_hand_gesture_t *ges,
                                      uint8_t *imagedata, invoke_t invoke) {
    ges->max = 3;
    float *output;
    invoke(ges->input, &output, imagedata, ges->svd, ges->std, ges->sv_shape,
           ges->invo, 7);
    int max = 0;
    for (int i = 1; i < 7; i++) {
        if (output[max] < output[i])
            max = i;
    }
    ges->max = max;
    
}

void qtk_cv_detection_body_res_hand_process(qtk_cv_detection_res_t box,
                                        qtk_cv_detection_hand_t *hand,
                                        uint8_t *imagedata,
                                        qtk_image_desc_t *desc, resize_t resize,
                                        invoke_t invoke) {
        const int width =
            (int)(box.roi.x2 - box.roi.x1);
        const int height =
            (int)(box.roi.y2 - box.roi.y1);
        hand->img_desc.width = width;
        hand->img_desc.height = height;
        hand->img_desc.channel = 3;
        qtk_cv_detection_onnx_image_rect(imagedata, desc, hand->data0,
                                         &(box.roi));
        qtk_cv_detection_onnx_image_BGRtoRGB(hand->data0, hand->data1,
                                             width * height);
        resize(hand->data1, &hand->img_desc, hand->data2, &hand->hand_desc);     
        qtk_cv_detection_hand_process(hand, hand->data2, invoke);
        for (int j = 0; j < hand->res.cnt; j++) {
            int x1 =
                (int)hand->res.box[j].roi.x1 + (int)box.roi.x1;
            int y1 =
                (int)hand->res.box[j].roi.y1 + (int)box.roi.y1;
            int hand_width =
                (int)hand->res.box[j].roi.x2 - (int)hand->res.box[j].roi.x1;
            int hand_height =
                (int)hand->res.box[j].roi.y2 - (int)hand->res.box[j].roi.y1;
            hand->res.box[j].roi.x1 = x1;
            hand->res.box[j].roi.y1 = y1;
            hand->res.box[j].roi.x2 = x1 + hand_width;
            hand->res.box[j].roi.y2 = y1 + hand_height;
        }
}

void qtk_cv_detection_body_hand_process(qtk_cv_detection_body_t *body,
                                        qtk_cv_detection_hand_t *hand,
                                        uint8_t *imagedata,
                                        qtk_image_desc_t *desc, resize_t resize,
                                        invoke_t invoke) {
    for (int i = 0; i < body->res.cnt; i++) {
        const int width =
            (int)(body->res.box[i].roi.x2 - body->res.box[i].roi.x1);
        const int height =
            (int)(body->res.box[i].roi.y2 - body->res.box[i].roi.y1);
        hand->img_desc.width = width;
        hand->img_desc.height = height;
        hand->img_desc.channel = 3;
        qtk_cv_detection_onnx_image_rect(imagedata, desc, hand->data0,
                                         &body->res.box[i].roi);
        qtk_cv_detection_onnx_image_BGRtoRGB(hand->data0, hand->data1,
                                             width * height);
        resize(hand->data1, &hand->img_desc, hand->data2, &hand->hand_desc);
        qtk_cv_detection_hand_process(hand, hand->data2, invoke);
        for (int j = 0; j < hand->res.cnt; j++) {
            int x1 =
                (int)hand->res.box[j].roi.x1 + (int)body->res.box[i].roi.x1;
            int y1 =
                (int)hand->res.box[j].roi.y1 + (int)body->res.box[i].roi.y1;
            int hand_width =
                (int)hand->res.box[j].roi.x2 - (int)hand->res.box[j].roi.x1;
            int hand_height =
                (int)hand->res.box[j].roi.y2 - (int)hand->res.box[j].roi.y1;
            hand->res.box[j].roi.x1 = x1;
            hand->res.box[j].roi.y1 = y1;
            hand->res.box[j].roi.x2 = x1 + hand_width;
            hand->res.box[j].roi.y2 = y1 + hand_height;
        }
    }
}

void qtk_cv_detection_hand_gesture_process(qtk_cv_detection_hand_t *hand,
                                           qtk_cv_detection_hand_gesture_t *ges,
                                           uint8_t *imagedata,
                                           qtk_image_desc_t *desc,
                                           resize_t resize, invoke_t invoke) {
    for (int i = 0; i < hand->res.cnt; i++) {
        int width = (int)(hand->res.box[i].roi.x2 - hand->res.box[i].roi.x1);
        int height = (int)(hand->res.box[i].roi.y2 - hand->res.box[i].roi.y1);
        ges->img_desc.width = width;
        ges->img_desc.height = height;
        ges->img_desc.channel = 3;
        qtk_cv_detection_onnx_image_rect(imagedata, desc, ges->data0,
                                         &hand->res.box[i].roi);
        resize(ges->data0, &ges->img_desc, ges->data1, &ges->desc);
        qtk_cv_detection_gesture_process(ges, ges->data1, invoke);
        hand->res.box[i].classify = ges->max;
    }
}
