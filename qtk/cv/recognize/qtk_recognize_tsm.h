#ifndef G_4N014G7B_97EV_ZF03_ZUOA_1R3A34KQA1HU
#define G_4N014G7B_97EV_ZF03_ZUOA_1R3A34KQA1HU
#pragma once
#include "qtk/core/qtk_cyclearray.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_recognize_tcm_post qtk_recognize_tcm_post_t;

struct qtk_recognize_tcm_post {
    qtk_cyclearray_t *likelyhood_hist;
    qtk_cyclearray_t *cls_hist;
    float *stale_likelyhood;
    float *likelyhood_sum;
    int ncls;
    int likelyhood_nhist;
};

int qtk_recognize_tcm_post_init(qtk_recognize_tcm_post_t *post, int ncls,
                                int likelyhood_nhist);
void qtk_recognize_tcm_post_clean(qtk_recognize_tcm_post_t *post);
int qtk_recognize_tcm_post_process(qtk_recognize_tcm_post_t *post,
                                   const float *likelyhood);

#ifdef __cplusplus
};
#endif
#endif /* G_4N014G7B_97EV_ZF03_ZUOA_1R3A34KQA1HU */
