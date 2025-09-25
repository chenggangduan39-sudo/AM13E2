#include "qtk/cv/qtk_cv_bbox.h"
#include "qtk/core/qtk_type.h"
#include "qtk/math/qtk_math.h"

float qtk_cv_bbox_jaccard_overlap(qtk_cv_bbox_t *box1, qtk_cv_bbox_t *box2) {
    qtk_cv_bbox_t intersect;
    float intersect_area;

    if (!qtk_cv_bbox_intersect(box1, box2, &intersect)) {
        return 0.0;
    }
    intersect_area = qtk_cv_bbox_area(&intersect);
    return intersect_area /
           (qtk_cv_bbox_area(box1) + qtk_cv_bbox_area(box2) - intersect_area);
}

int qtk_cv_bbox_intersect(qtk_cv_bbox_t *box1, qtk_cv_bbox_t *box2,
                          qtk_cv_bbox_t *result) {

    result->x1 = max(box1->x1, box2->x1);
    result->y1 = max(box1->y1, box2->y1);
    result->x2 = min(box1->x2, box2->x2);
    result->y2 = min(box1->y2, box2->y2);
    if (result->y2 <= result->y1 || result->x2 <= result->x1) {
        return 0;
    }
    return 1;
}

float qtk_cv_bbox_distance(qtk_cv_bbox_t *box1, qtk_cv_bbox_t *box2) {
    float cx1, cy1, cx2, cy2;

    cx1 = (box1->x1 + box1->x2) / 2;
    cy1 = (box1->y1 + box1->y2) / 2;

    cx2 = (box2->x1 + box2->x2) / 2;
    cy2 = (box2->y1 + box2->y2) / 2;

    return qtk_sqrtf((cx1 - cx2) * (cx1 - cx2) + (cy1 - cy2) * (cy1 - cy2));
}

int qtk_cv_bbox_fix_ratio(qtk_cv_bbox_t *box, qtk_cv_bbox_t *res, float ratio,
                          int width, int height) {
    float extend, extend_half, max_extend1, max_extend2;
    float w = box->x2 - box->x1;
    float h = box->y2 - box->y1;
    float cur_ratio = w / h;
    float extend_top = 0, extend_bottom = 0, extend_left = 0, extend_right = 0;
    if (cur_ratio > ratio) {
        extend = w / ratio - h;
        extend_half = extend / 2;
        max_extend1 = box->y1;
        max_extend2 = height - box->y2;
        if (max_extend1 >= extend_half && max_extend2 >= extend_half) {
            extend_top = extend_half;
            extend_bottom = extend_half;
        } else if (max_extend1 < extend_half) {
            extend_top = max_extend1;
            extend_bottom = min(max_extend2, extend - max_extend1);
        } else {
            extend_bottom = max_extend2;
            extend_top = min(max_extend1, extend - max_extend2);
        }
    } else {
        extend = h * ratio - w;
        extend_half = extend / 2;
        max_extend1 = box->x1;
        max_extend2 = width - box->x2;
        if (max_extend1 >= extend_half && max_extend2 >= extend_half) {
            extend_left = extend_half;
            extend_right = extend_half;
        } else if (max_extend1 < extend_half) {
            extend_left = max_extend1;
            extend_right = min(max_extend2, extend - max_extend1);
        } else {
            extend_right = max_extend2;
            extend_left = min(max_extend1, extend - max_extend2);
        }
    }

    res->x1 = box->x1 - extend_left;
    res->x2 = box->x2 + extend_right;
    res->y1 = box->y1 - extend_top;
    res->y2 = box->y2 + extend_bottom;

    return 0;
}

void qtk_cv_bbox_clip2int(qtk_cv_bbox_t *s, qtk_cv_bbox_t *d, int w, int h) {
    int rw = qtk_roundf(s->x2 - s->x1);
    int rh = qtk_roundf(s->y2 - s->y1);
    int x1 = qtk_roundf(s->x1);
    int y1 = qtk_roundf(s->y1);
    x1 = max(0, x1);
    y1 = max(0, y1);
    rw = min(w - x1, rw);
    rh = min(h - y1, rh);
    d->x1 = x1;
    d->y1 = y1;
    d->x2 = x1 + rw;
    d->y2 = y1 + rh;
}
