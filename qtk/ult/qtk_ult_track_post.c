#include "qtk/ult/qtk_ult_track_post.h"

qtk_ult_track_post_t *qtk_ult_track_post_new(qtk_ult_track_post_cfg_t *cfg) {
    int i;
    qtk_ult_track_post_t *p =
        wtk_malloc(sizeof(qtk_ult_track_post_t) +
                   sizeof(qtk_ult_track_result_t) * cfg->win_sz);
    p->cfg = cfg;
    p->win = wtk_robin_new(cfg->win_sz);
    p->heap = wtk_heap_new(1024);
    for (i = 0; i < cfg->win_sz; i++) {
        p->win->r[i] = &((qtk_ult_track_result_t *)(p + 1))[i];
    }
    p->stand = 0;
    p->startup = 1;
    return p;
}

void update_win_statistics_(qtk_ult_track_post_t *p, float *mean, float *std) {
    float sum;
    int i;
    float *height = wtk_heap_malloc(p->heap, sizeof(float) * p->win->used);
    for (i = 0, sum = 0; i < p->win->used; i++) {
        qtk_ult_track_result_t *res = wtk_robin_at(p->win, i);
        height[i] = res->r * sinf(res->y_theta / 180.0 * M_PI);
        sum += height[i];
    }
    *mean = sum / p->win->used;
    for (i = 0, sum = 0; i < p->win->used; i++) {
        sum += (height[i] - *mean) * (height[i] - *mean);
    }
    *std = sqrtf(sum / p->win->used);
}

int qtk_ult_track_post_feed(qtk_ult_track_post_t *p,
                            qtk_ult_track_result_t *res) {
    float mean, std;
    *(qtk_ult_track_result_t *)wtk_robin_next(p->win) = *res;
    if (wtk_robin_is_full(p->win)) {
        update_win_statistics_(p, &mean, &std);
        wtk_robin_pop2(p->win, p->cfg->win_step);
        p->startup = 0;
        p->stand = std > p->cfg->height_std_thresh;
    } else {
        if (p->startup) {
            update_win_statistics_(p, &mean, &std);
            p->stand = std > p->cfg->height_std_thresh;
        }
    }
    wtk_heap_reset(p->heap);
    return 0;
}

void qtk_ult_track_post_delete(qtk_ult_track_post_t *p) {
    wtk_robin_delete(p->win);
    wtk_heap_delete(p->heap);
    wtk_free(p);
}
