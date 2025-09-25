#include "qtk_cv_detection_dynamic.h"
#include "qtk_cv_detection_onnx.h"

qtk_cv_detection_dynamic_t *qtk_cv_detection_dynamic_new() {
    qtk_cv_detection_dynamic_t *dy = (qtk_cv_detection_dynamic_t *)wtk_malloc(
        sizeof(qtk_cv_detection_dynamic_t));
    for (int i = 0; i < DYNAMIC_NUM; i++) {
        dy->sv_shape[i] = (int64_t *)wtk_malloc(4 * sizeof(int64_t));
        dy->sv_shape[i][0] = 1;
    }

    dy->sv_shape[0][1] = 3;
    dy->sv_shape[0][2] = 224;
    dy->sv_shape[0][3] = 224;

    dy->sv_shape[1][1] = 3;
    dy->sv_shape[1][2] = 56;
    dy->sv_shape[1][3] = 56;

    dy->sv_shape[2][1] = 7;
    dy->sv_shape[2][2] = 28;
    dy->sv_shape[2][3] = 28;

    dy->sv_shape[3][1] = 7;
    dy->sv_shape[3][2] = 28;
    dy->sv_shape[3][3] = 28;

    dy->sv_shape[4][1] = 7;
    dy->sv_shape[4][2] = 28;
    dy->sv_shape[4][3] = 28;

    dy->sv_shape[5][1] = 14;
    dy->sv_shape[5][2] = 28;
    dy->sv_shape[5][3] = 28;

    dy->sv_shape[6][1] = 14;
    dy->sv_shape[6][2] = 14;
    dy->sv_shape[6][3] = 14;

    dy->sv_shape[7][1] = 14;
    dy->sv_shape[7][2] = 14;
    dy->sv_shape[7][3] = 14;

    dy->sv_shape[8][1] = 14;
    dy->sv_shape[8][2] = 14;
    dy->sv_shape[8][3] = 14;

    dy->sv_shape[9][1] = 14;
    dy->sv_shape[9][2] = 14;
    dy->sv_shape[9][3] = 14;

    dy->sv_shape[10][1] = 14;
    dy->sv_shape[10][2] = 14;
    dy->sv_shape[10][3] = 14;

    dy->sv_shape[11][1] = 14;
    dy->sv_shape[11][2] = 14;
    dy->sv_shape[11][3] = 14;

    dy->sv_shape[12][1] = 14;
    dy->sv_shape[12][2] = 14;
    dy->sv_shape[12][3] = 14;

    dy->sv_shape[13][1] = 29;
    dy->sv_shape[13][2] = 14;
    dy->sv_shape[13][3] = 14;

    dy->sv_shape[14][1] = 29;
    dy->sv_shape[14][2] = 7;
    dy->sv_shape[14][3] = 7;

    dy->sv_shape[15][1] = 29;
    dy->sv_shape[15][2] = 7;
    dy->sv_shape[15][3] = 7;

    dy->sv_shape[16][1] = 29;
    dy->sv_shape[16][2] = 7;
    dy->sv_shape[16][3] = 7;

    dy->input[0] = (float *)wtk_malloc(dy->sv_shape[0][2] * dy->sv_shape[0][3] *
                                       3 * sizeof(float));
    dy->output[0] = (float *)wtk_malloc(6 * sizeof(float));
    for (int i = 1; i < DYNAMIC_NUM; i++) {
        dy->input[i] =
            (float *)wtk_malloc(dy->sv_shape[i][1] * dy->sv_shape[i][2] *
                                dy->sv_shape[i][3] * sizeof(float));
        memset(dy->input[i], 0,
               dy->sv_shape[i][1] * dy->sv_shape[i][2] * dy->sv_shape[i][3] *
                   sizeof(float));
        dy->output[i] =
            (float *)wtk_malloc(dy->sv_shape[i][1] * dy->sv_shape[i][2] *
                                dy->sv_shape[i][3] * sizeof(float));
    }
    dy->svd[0] = 123.675;
    dy->svd[1] = 116.25;
    dy->svd[2] = 103.53;
    dy->std[0] = 58.395;
    dy->std[1] = 57.12;
    dy->std[2] = 57.375;
    dy->dynamic_desc.width = 224;
    dy->dynamic_desc.height = 224;
    qtk_recognize_tcm_post_init(&dy->post, 6, 4);
    dy->data0 = (uint8_t *)wtk_malloc(3840 * 3840 * 3);
    dy->data1 = (uint8_t *)wtk_malloc(dy->dynamic_desc.width *
                                      dy->dynamic_desc.height * 3);
    return dy;
}

void qtk_cv_detection_dynamic_delete(qtk_cv_detection_dynamic_t *dy) {
    for (int i = 0; i < 17; i++) {
        wtk_free(dy->input[i]);
        wtk_free(dy->output[i]);
        wtk_free(dy->sv_shape[i]);
    }
    qtk_recognize_tcm_post_clean(&dy->post);
    wtk_free(dy->data0);
    wtk_free(dy->data1);
    wtk_free(dy);
    dy = NULL;
}

void qtk_cv_detection_dynamic_process(qtk_cv_detection_dynamic_t *dy,
                                      uint8_t *imagedata, invoke_mum invoke) {

    dy->num = 5;

    invoke(dy->input, dy->output, imagedata, dy->svd, dy->std, dy->sv_shape,
           dy->invo, 6, DYNAMIC_NUM);
    dy->output[0][3] *= 1.25;
    int max = 0;
    for (int i = 1; i < 6; i++) {
        if (dy->output[0][max] < dy->output[0][i])
            max = i;
    }
    if (dy->output[0][max] < dy->score)
        max = 5;
    dy->num = max;
}

void qtk_cv_detection_dynamic_process_data(qtk_cv_detection_dynamic_t *dy,
                                           uint8_t *imagedata,
                                           qtk_image_desc_t *desc,
                                           resize_t resize, invoke_mum invoke) {
    if (desc->width == desc->height) {
        resize(imagedata, desc, dy->data1, &dy->dynamic_desc);
        qtk_cv_detection_dynamic_process(dy, dy->data1, invoke);
    } else if (desc->width > desc->height) {
        memset(dy->data0, 127, 3840 * 3840 * 3);
        int t = (desc->width - desc->height) / 2;
        memcpy(dy->data0 + t * desc->width * 3, imagedata,
               desc->width * desc->height * 3);
        dy->img_desc.width = desc->width;
        dy->img_desc.height = desc->width;
        dy->img_desc.channel = 3;

        resize(dy->data0, &dy->img_desc, dy->data1, &dy->dynamic_desc);
        qtk_cv_detection_dynamic_process(dy, dy->data1, invoke);
    } else {
        wtk_debug("Temporarily not supported .");
        return;
    }
}

void qtk_cv_detection_dynamic_show(int t) {
    switch (t) {
    case 0:
        printf("left\n");
        break;
    case 1:
        printf("rigth\n");
        break;
    case 2:
        printf("up\n");
        break;
    case 3:
        printf("down\n");
        break;
    case 4:
        printf("edit\n");
        break;
    case 5:
        printf("other\n");
        break;
    default:
        break;
    }
}