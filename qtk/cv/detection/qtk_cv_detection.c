#include "qtk/cv/detection/qtk_cv_detection.h"
#include "qtk/cv/qtk_cv_bbox.h"
#include "qtk/math/qtk_math.h"

typedef struct {
    qtk_cv_bbox_t box;
    float conf;
    float *keypoint;
} bbox_info_t;

static int bbox_conf_cmp_(bbox_info_t *a, bbox_info_t *b) {
    if (a->conf < b->conf) {
        return 1;
    }
    if (a->conf > b->conf) {
        return -1;
    }
    return 0;
}

qtk_cv_detection_t *qtk_cv_detection_new(qtk_cv_detection_cfg_t *cfg) {

    qtk_cv_detection_t *fd =
        (qtk_cv_detection_t *)malloc(sizeof(qtk_cv_detection_t));
    fd->src_imgheight = 0;
    fd->src_imgwidth = 0;
    fd->x_factor = 0;
    fd->y_factor = 0;
    fd->rat[0] = 0.0;
    fd->rat[1] = 0.0;
    fd->cfg = cfg;
    fd->nnrt = qtk_nnrt_new(&cfg->nnrt);
#ifdef QTK_NNRT_RKNPU
    // if(fd->nnrt->cfg->use_rknpu){
    //     wtk_debug("set detect RKNN_NPU_CORE_2 mask\n");
    //     rknn_core_mask core_mask2 = RKNN_NPU_CORE_2;
    //     // rknn_core_mask core_mask1 = RKNN_NPU_CORE_AUTO;
    //     int ret = rknn_set_core_mask(fd->nnrt->ctx, core_mask2);
    //     if(ret != 0) wtk_debug("set core2 mask failed\n");
    // }
#endif
    fd->input = wtk_malloc(sizeof(float) * 3 * cfg->height * cfg->width);
    fd->heap = wtk_heap_new(1024);
    qtk_min_heap_init2(&fd->mheap, sizeof(bbox_info_t), 32,
                       (qtk_min_heap_cmp_f)bbox_conf_cmp_);
    if (!(cfg->nnrt.use_ss_ipu || cfg->nnrt.use_rknpu)) {
        int64_t shape[4] = {1, 3, fd->cfg->height, fd->cfg->width};
        fd->input_val = qtk_nnrt_value_create_external(
            fd->nnrt, QTK_NNRT_VALUE_ELEM_F32, shape, 4, fd->input);
    }
    return fd;
}

void qtk_cv_detection_delete(qtk_cv_detection_t *fd) {
    if (!(fd->cfg->nnrt.use_ss_ipu || fd->cfg->nnrt.use_rknpu)) {
        qtk_nnrt_value_release(fd->nnrt, fd->input_val);
    }
    qtk_nnrt_delete(fd->nnrt);
    wtk_free(fd->input);
    qtk_min_heap_clean2(&fd->mheap);
    wtk_heap_delete(fd->heap);
    wtk_free(fd);
}

static int cv_post_(qtk_cv_detection_t *fd, float *output,
                 qtk_cv_detection_result_t **result) {
    int stride = 15;
    int i, j, k, n, kn;
    bbox_info_t candidate;
    int nresult = 0;
    qtk_min_heap_reset(&fd->mheap);
    for (i = 0; i < fd->cfg->nstride; i++) {
        int grid_w = fd->cfg->width / fd->cfg->stride[i];
        int grid_h = fd->cfg->height / fd->cfg->stride[i];

        for (j = 0; j < grid_h; j++) {
            for (k = 0; k < grid_w; k++) {
                for (n = 0; n < fd->cfg->nanchor; n++, output += stride) {
                    float center_x, center_y;
                    
                    if (output[0] < fd->cfg->thresh) {
                        continue;
                    }
                    center_x = k * fd->cfg->stride[i];
                    center_y = j * fd->cfg->stride[i];
                    candidate.conf = output[0];

                    candidate.keypoint =
                        wtk_heap_malloc(fd->heap, sizeof(float) * 10);
                    for (kn = 0; kn < 5; kn++) {
                        candidate.keypoint[kn * 2 + 0] =
                            output[5 + kn * 2 + 0] * fd->cfg->stride[i] +
                            center_x;

                        candidate.keypoint[kn * 2 + 1] =
                            output[5 + kn * 2 + 1] * fd->cfg->stride[i] +
                            center_y;
                            
                    }
                    candidate.box.x1 =
                        center_x - output[1] * fd->cfg->stride[i];
                    candidate.box.y1 =
                        center_y - output[2] * fd->cfg->stride[i];
                    candidate.box.x2 =
                        center_x + output[3] * fd->cfg->stride[i];
                    candidate.box.y2 =
                        center_y + output[4] * fd->cfg->stride[i];

                    qtk_min_heap_push(&fd->mheap, &candidate);
                }
            }
        }
    }
    *result = wtk_heap_malloc(fd->heap, sizeof(qtk_cv_detection_result_t) *
                                            fd->mheap.nelem);
    while (qtk_min_heap_pop(&fd->mheap, &candidate) == 0) {
        int keep = 1;
        for (i = 0; i < nresult; i++) {
            if (qtk_cv_bbox_jaccard_overlap(&(*result)[i].box, &candidate.box) >
                fd->cfg->nms_thresh) {
                keep = 0;
                break;
            }
        }
        if (keep) {
            (*result)[nresult].box = candidate.box;
            (*result)[nresult].keypoints = candidate.keypoint;
            (*result)[nresult].conf = candidate.conf;
            nresult++;
        }
    }
    return nresult;
}


static int ssp_post(qtk_cv_detection_t *fd, float *output, qtk_cv_detection_result_t **result){
    float *pScore = NULL;
    int nresult = 0;
    // const float conf_threshold = 0.5f;
    wtk_debug("============ conf_threshold = %f\n", fd->cfg->conf_threshold);
    float cx, cy, cw, ch;
    for(size_t i = 0; i < 5040; i++){
        pScore = output + 4*5040 + i;
        if(*pScore > fd->cfg->conf_threshold){
            // wtk_debug("*pScore = %f\n", *pScore);
            cx = output[i];
            cy = output[1*5040+i];
            cw = output[2*5040+i];
            ch = output[3*5040+i];

            qtk_cv_bbox_t box;
            cx = cx - fd->rat[0];
            cy = cy - fd->rat[1];

            box.x1 = max(0, (int)((cx - cw / 2) / fd->x_factor));
            box.y1 = max(0, (int)((cy - ch / 2) / fd->y_factor));
            int width = (int)(cw / fd->x_factor);
            int height =(int)(ch / fd->y_factor);

            int right = min(fd->src_imgwidth, box.x1 + width);
            int bottom = min(fd->src_imgheight, box.y1 + height);
            width = right - box.x1;
            height = bottom - box.y1;

            box.x2 = box.x1 + width;
            box.y2 = box.y1 + height;
            qtk_min_heap_push(&fd->mheap,  &(qtk_cv_bbox_result_t){*pScore, 0, box});
        }
    }
        {
        qtk_cv_bbox_result_t cur_box;
        *result = wtk_heap_malloc(fd->heap, sizeof(qtk_cv_detection_result_t) *
                                            fd->mheap.nelem);
        while (0 == qtk_min_heap_pop(&fd->mheap, &cur_box)) {
            int keep = 1;
            for (int i = 0; i < nresult; i++) {
                float iou = qtk_cv_bbox_jaccard_overlap(&cur_box.box,
                                                        &(*result)[i].box);
                // wtk_debug("iou = %f\n", iou);
                if (iou > 0.5) {
                    keep = 0;
                }
            }
            if (keep) {
                (*result)[nresult].box.x1 = qtk_roundf(cur_box.box.x1);
                (*result)[nresult].box.x2 = qtk_roundf(cur_box.box.x2);
                (*result)[nresult].box.y1 = qtk_roundf(cur_box.box.y1);
                (*result)[nresult].box.y2 = qtk_roundf(cur_box.box.y2);
                (*result)[nresult].conf = cur_box.conf;
                //(*result)[nresult].clsid = cur_box.clsid

                nresult++;
            }
        }
    }
    return nresult;
}

static int post_(qtk_cv_detection_t *fd, float *output, qtk_cv_detection_result_t **result){
    int ret = -1;
    if(fd->cfg->stand_sit_phone == 1){
        ret = ssp_post(fd, output, result);
    }else{
        ret = cv_post_(fd, output, result);
    }
    return ret;
}

static float deqnt_affine_to_f32(int8_t qnt, int32_t zp, float scale) { return ((float)qnt - (float)zp) * scale; }
int qtk_cv_detection_detect(qtk_cv_detection_t *fd, uint8_t *image,
                            qtk_cv_detection_result_t **result) {
    int i, ret;
    int n = fd->cfg->width * fd->cfg->height;
    qtk_nnrt_value_t input, output;
    float *output_val;
    int64_t shape[4] = {1, 3, fd->cfg->height, fd->cfg->width};

    wtk_heap_reset(fd->heap);

    if (fd->nnrt->cfg->use_ss_ipu) {
        input = qtk_nnrt_value_create_external(fd->nnrt, QTK_NNRT_VALUE_ELEM_U8, shape, 4, image) ;
    } else if (fd->nnrt->cfg->use_rknpu) {
        input = qtk_nnrt_value_create_external(fd->nnrt, QTK_NNRT_VALUE_ELEM_U8, shape, 4, image) ;
    } else {
        for (i = 0; i < n; i++) {
            fd->input[i] = (image[i * 3 + 0] - fd->cfg->mean[0]) / fd->cfg->std[0];
            fd->input[i + n] = (image[i * 3 + 1] - fd->cfg->mean[1]) / fd->cfg->std[1];
            fd->input[i + n * 2] = (image[i * 3 + 2] - fd->cfg->mean[2]) / fd->cfg->std[2];
        }
        input = fd->input_val;
    }

#ifdef QTK_NNRT_RKNPU
    if (fd->nnrt->cfg->use_rknpu) {
        qtk_nnrt_feed_image(fd->nnrt, input, 0);
    } else {
        qtk_nnrt_feed(fd->nnrt, input, 0);
    }
#else
    qtk_nnrt_feed(fd->nnrt, input, 0);
#endif
    // double start_time = time_get_ms();
    qtk_nnrt_run(fd->nnrt);
    // double end_time = time_get_ms() - start_time;
    // printf("detect qtk_nnrt_run time = %f ms\n", end_time);
    qtk_nnrt_get_output(fd->nnrt, &output, 0);
    output_val = qtk_nnrt_value_get_data(fd->nnrt, output);
    
    ret = post_(fd, output_val, result);

    if (fd->nnrt->cfg->use_rknpu || fd->nnrt->cfg->use_ss_ipu) {
        qtk_nnrt_value_release(fd->nnrt, input);
    }
    qtk_nnrt_value_release(fd->nnrt, output);
    qtk_nnrt_reset(fd->nnrt);

    return ret;
}
