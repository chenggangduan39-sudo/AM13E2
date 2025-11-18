#ifndef QBL_CV_QBL_CV_BBOX_H
#define QBL_CV_QBL_CV_BBOX_H
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x1;
    float x2;
    float y1;
    float y2;
} qtk_cv_bbox_t;

#define qtk_cv_bbox_width(b) ((b)->x2 - (b)->x1)
#define qtk_cv_bbox_height(b) ((b)->y2 - (b)->y1)
#define qtk_cv_bbox_area(b) (qtk_cv_bbox_width(b) * qtk_cv_bbox_height(b))

typedef struct {
    float conf;
    qtk_cv_bbox_t box;
} qtk_cv_bbox_conf_t;

typedef struct {
    float conf;
    int clsid;
    qtk_cv_bbox_t box;
} qtk_cv_bbox_result_t;

float qtk_cv_bbox_jaccard_overlap(qtk_cv_bbox_t *box1, qtk_cv_bbox_t *box2);
int qtk_cv_bbox_intersect(qtk_cv_bbox_t *box1, qtk_cv_bbox_t *box2,
                          qtk_cv_bbox_t *result);
float qtk_cv_bbox_distance(qtk_cv_bbox_t *box1, qtk_cv_bbox_t *box2);
int qtk_cv_bbox_fix_ratio(qtk_cv_bbox_t *box, qtk_cv_bbox_t *res, float ratio,
                          int width, int height);
void qtk_cv_bbox_clip2int(qtk_cv_bbox_t *s, qtk_cv_bbox_t *d, int w, int h);

#ifdef __cplusplus
};
#endif
#endif
