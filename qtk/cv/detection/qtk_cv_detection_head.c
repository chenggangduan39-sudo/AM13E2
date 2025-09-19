#include "qtk_cv_detection_head.h"
static const int steps[] = {8, 16, 32, 64, 128};

qtk_cv_detection_head_t *qtk_cv_detection_head_new() {
    qtk_cv_detection_head_t *head =
        (qtk_cv_detection_head_t *)wtk_malloc(sizeof(qtk_cv_detection_head_t));
    head->input = (float *)wtk_malloc(544 * 960 * 3 * sizeof(float));
    head->sv_shape[0] = 1;
    head->sv_shape[1] = 3;
    head->sv_shape[2] = 544;
    head->sv_shape[3] = 960;
    head->head_desc.width = 960;
    head->head_desc.height = 544;
    for (int i = 0; i < 3; i++) {
        head->std[i] = 128;
        head->svd[i] = 127.5;
    }
    head->len = 5;
    head->anchors_num = 2;
    head->steps_nums = 3;
    return head;
}

void qtk_cv_detection_head_delete(qtk_cv_detection_head_t *head) {
    wtk_free(head->input);
    wtk_free(head);
    head=NULL;
}

static void qtk_head_data_process(qtk_cv_detection_head_t *head, float *preds) {
    const int len = head->len;
    for (int i=0;i<head->steps_nums;i++){
        int head_x=head->head_desc.width/steps[i];
        int head_y=head->head_desc.height/steps[i];
        for (int n=0;n<head_y;n++){
            for (int m=0;m<head_x;m++){
                for (int k=0;k<head->anchors_num;k++){
                    float score=max(0,preds[0]);
                    if (score>head->iou){
                        float cen_x=m*steps[i];
                        float cen_y=n*steps[i];
                        float x1=cen_x-preds[1]*steps[i];
                        float x2=cen_x+preds[3]*steps[i];
                        float y1=cen_y-preds[2]*steps[i];
                        float y2=cen_y+preds[4]*steps[i];
                        qtk_cv_detection_onnx_box_push(&head->shape, x1, y1, x2,
                                                       y2, score);
                    }
                    preds+=len;
                }
            }
        }
    }
}

void qtk_cv_detection_head_process(qtk_cv_detection_head_t *head,
                                   uint8_t *imagedata, invoke_t invoke) {
    head->shape.cnt=0;
    head->res.cnt=0;
    float *output;
    invoke(head->input, &output, imagedata, head->svd, head->std,
           head->sv_shape, head->invo, 21420 * 5);
    qtk_head_data_process(head, output);
    qtk_cv_detection_onnx_nms(&head->shape, &head->res, head->conf,
                              head->head_desc);
    float w_dio=(float)head->img_desc.width/(float)head->head_desc.width;
    float h_dio=(float)head->img_desc.height/(float)head->head_desc.height;
    for (int i=0;i<head->res.cnt;i++){
        head->res.box[i].roi.x1=max(0,floor(head->res.box[i].roi.x1*w_dio));
        head->res.box[i].roi.y1=max(0,floor(head->res.box[i].roi.y1*h_dio));
        head->res.box[i].roi.x2=min(head->img_desc.width,floor(head->res.box[i].roi.x2*w_dio));
        head->res.box[i].roi.y2=min(head->img_desc.height,floor(head->res.box[i].roi.y2*h_dio));
    }
    // qtk_cv_detection_onnx_box_square(&head->res, head->img_desc);
}
