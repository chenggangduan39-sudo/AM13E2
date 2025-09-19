#include "wtk/bfio/qtk_fixed_angle_wasr.h"

static void _on_qform2(qtk_fixed_angle_wasr_t *w, short *d, int len,
                       int is_end) {
    if (len > 0) {
        qtk_wasr_feed(w->wasr, d, len);
    }
    if (is_end) {
        qtk_wasr_feed_end(w->wasr);
    }
}

static void _on_aec(qtk_fixed_angle_wasr_t *w, short **data, int len,
                    int is_end) {
    wtk_qform2_feed(w->qform2, data, len, is_end);
}

qtk_fixed_angle_wasr_t *
qtk_fixed_angle_wasr_new(qtk_fixed_angle_wasr_cfg_t *cfg) {
    qtk_fixed_angle_wasr_t *w;
    w = wtk_malloc(sizeof(qtk_fixed_angle_wasr_t));
    w->cfg = cfg;
    w->wasr = qtk_wasr_new(&cfg->wasr);
    w->qform2 = wtk_qform2_new(&cfg->qform2);
    wtk_qform2_set_notify(w->qform2, w, cast(wtk_qform2_notify_f, _on_qform2));
    wtk_qform2_start(w->qform2, cfg->theta, cfg->phi);
    if (cfg->use_aec) {
        w->aec = wtk_aec_new(&cfg->aec);
        wtk_aec_set_notify(w->aec, w, cast(wtk_aec_notify_f, _on_aec));
    } else {
        w->aec = NULL;
    }
    return w;
}

void qtk_fixed_angle_wasr_delete(qtk_fixed_angle_wasr_t *w) {
    qtk_wasr_delete(w->wasr);
    wtk_qform2_delete(w->qform2);
    if (w->cfg->use_aec) {
        wtk_aec_delete(w->aec);
    }
    wtk_free(w);
}

int qtk_fixed_angle_wasr_feed(qtk_fixed_angle_wasr_t *w, short **d, int len) {
    if (w->cfg->use_aec) {
        wtk_aec_feed(w->aec, d, len, 0);
    } else {
        wtk_qform2_feed(w->qform2, d, len, 0);
    }
    return 0;
}

int qtk_fixed_angle_wasr_feed_end(qtk_fixed_angle_wasr_t *w) {
    if (w->cfg->use_aec) {
        wtk_aec_feed(w->aec, NULL, 0, 1);
    } else {
        wtk_qform2_feed(w->qform2, NULL, 0, 1);
    }
    return 0;
}

void qtk_fixed_angle_wasr_set_wasr_notify(qtk_fixed_angle_wasr_t *w,
                                          qtk_wasr_notify_t notify,
                                          void *upval) {
    qtk_wasr_set_notify(w->wasr, notify, upval);
}
