#include "qtk/ult/fmcw/qtk_ult_fmcw.h"

static void mask_cir_(qtk_ult_fmcw_t *fmcw, wtk_complex_t *samples) {
    int i;
    float dis_start = fmcw->cfg->dis_start / 2;
    float dis_stop = fmcw->cfg->dis_end / 2;
    int start_idx = (int)(dis_start / fmcw->cfg->range_unit) + 1;
    int stop_idx = (int)(dis_stop / fmcw->cfg->range_unit);
    if (start_idx > 1) {
        for (i = 0; i < start_idx; i++) {
            samples[i].a = 0;
            samples[i].b = 0;
        }
    }
    for (i = stop_idx; i < fmcw->cfg->nsamples; i++) {
        samples[i].a = 0;
        samples[i].b = 0;
    }
}

qtk_ult_fmcw_t *qtk_ult_fmcw_new(qtk_ult_fmcw_cfg_t *cfg, void *upval,
                                 qtk_ult_fmcw_notifier_t notifier) {
    int i;
    qtk_ult_fmcw_t *fmcw = wtk_malloc(sizeof(qtk_ult_fmcw_t));
    fmcw->cfg = cfg;
    fmcw->conv = wtk_malloc(cfg->channel * sizeof(wtk_conv2_cpx_t *));
    for (i = 0; i < cfg->channel; i++) {
        fmcw->conv[i] =
            wtk_conv2_cpx_new(cfg->filter_params, cfg->nsymbols, cfg->period);
    }
    fmcw->samples =
        wtk_calloc(sizeof(wtk_complex_t), cfg->nsamples * cfg->channel);
    fmcw->cir = wtk_malloc(sizeof(wtk_complex_t) * cfg->period * cfg->channel);
    fmcw->notifier = notifier;
    fmcw->upval = upval;
    fmcw->state = QTK_ULT_FMCW_INIT;
    fmcw->frames =
        wtk_malloc(sizeof(wtk_complex_t) * cfg->period * cfg->channel);
    fmcw->pos = 0;
    fmcw->sample_pos = 0;
    fmcw->cur_peak = 0;
    fmcw->nframe = 0;
    return fmcw;
}

void qtk_ult_fmcw_delete(qtk_ult_fmcw_t *fmcw) {
    int i;
    for (i = 0; i < fmcw->cfg->channel; i++) {
        wtk_conv2_cpx_delete(fmcw->conv[i]);
    }
    wtk_free(fmcw->conv);
    wtk_free(fmcw->samples);
    wtk_free(fmcw->cir);
    wtk_free(fmcw->frames);
    wtk_free(fmcw);
}

static void feed_sample_(qtk_ult_fmcw_t *fmcw) {
    int i, c, c_idx, s_idx;
    for (c = 0, c_idx = 0, s_idx = 0; c < fmcw->cfg->channel;
         c++, c_idx += fmcw->cfg->period, s_idx += fmcw->cfg->nsamples) {
        for (i = 0; i < fmcw->cfg->nsamples; i++) {
            fmcw->cir[s_idx + i] =
                fmcw->cir[c_idx + i * fmcw->cfg->subsample_factor];
        }
        mask_cir_(fmcw, fmcw->cir + s_idx);
    }
    fmcw->notifier(fmcw->upval, fmcw->cir);
}

static void feed_frame_(qtk_ult_fmcw_t *fmcw) {
    int i, c, len;
    wtk_complex_t *conv_result[100];
    int refchan = 0;
    uint32_t sync_pos, diff;
    for (c = 0; c < fmcw->cfg->channel; c++) {
        wtk_conv2_cpx_feed_frame(fmcw->conv[c],
                                 fmcw->frames + c * fmcw->cfg->period,
                                 &conv_result[c]);
    }
    switch (fmcw->state) {
    case QTK_ULT_FMCW_INIT:
        for (i = 0; i < fmcw->cfg->period; i++) {
            float cur = conv_result[refchan][i].a * conv_result[refchan][i].a +
                        conv_result[refchan][i].b * conv_result[refchan][i].b;
            if (cur > fmcw->cfg->peak_thresh) {
                if (cur > fmcw->cur_peak) {
                    fmcw->cur_peak = cur;
                } else {
                    wtk_debug("aligned\n");
                    fmcw->pre_sync_pos = fmcw->nframe * fmcw->cfg->period + i;
                    break;
                }
            }
        }
        if (i < fmcw->cfg->period) {
            fmcw->sample_pos = fmcw->cfg->period - i;
            for (c = 0; c < fmcw->cfg->channel; c++) {
                memcpy(fmcw->cir + c * fmcw->cfg->period, conv_result[c] + i,
                       sizeof(wtk_complex_t) * fmcw->sample_pos);
            }
            fmcw->state = QTK_ULT_FMCW_ALIGNED;
        }
        break;
    case QTK_ULT_FMCW_ALIGNED:
        diff = (fmcw->nframe + 1) * fmcw->cfg->period - fmcw->pre_sync_pos;
        for (i = 0; i < fmcw->cfg->period; i++) {
            float cur = conv_result[refchan][i].a * conv_result[refchan][i].a +
                        conv_result[refchan][i].b * conv_result[refchan][i].b;
            if (cur > fmcw->cfg->peak_thresh) {
                if (cur > fmcw->cur_peak) {
                    fmcw->cur_peak = cur;
                } else {
                    sync_pos = fmcw->nframe * fmcw->cfg->period + i;
                    diff = sync_pos - fmcw->pre_sync_pos;
                    fmcw->pre_sync_pos = sync_pos;
                    break;
                }
            }
        }
        diff = diff > fmcw->cfg->period ? diff - fmcw->cfg->period : fmcw->cfg->period - diff;
        if (diff > fmcw->cfg->sync_pos_tolerance) {
            wtk_debug("sync error realigned\n");
            fmcw->state = QTK_ULT_FMCW_INIT;
            goto err;
        }
        len = fmcw->cfg->period;
        while (len > 0) {
            int need = min(len, fmcw->cfg->period - fmcw->sample_pos);
            for (c = 0; c < fmcw->cfg->channel; c++) {
                memcpy(fmcw->cir + c * fmcw->cfg->period + fmcw->sample_pos,
                       conv_result[c], sizeof(wtk_complex_t) * need);
                conv_result[c] += need;
            }
            fmcw->sample_pos += need;
            len -= need;
            if (fmcw->sample_pos == fmcw->cfg->period) {
                feed_sample_(fmcw);
                fmcw->sample_pos = 0;
            }
        }
        break;
    }
err:
    fmcw->nframe++;
}

int qtk_ult_fmcw_feed(qtk_ult_fmcw_t *fmcw, short **wav, int len) {
    while (len > 0) {
        int c, i;
        int pos = fmcw->pos;
        int need = min(fmcw->cfg->period - pos, len);
        for (c = 0; c < fmcw->cfg->channel; c++) {
            for (i = 0; i < need; i++) {
                int idx = i + pos;
                fmcw->frames[c * fmcw->cfg->period + idx].a =
                    wav[c][i] * fmcw->cfg->carrier[idx].a;
                fmcw->frames[c * fmcw->cfg->period + idx].b =
                    wav[c][i] * fmcw->cfg->carrier[idx].b;
            }
            wav[c] += need;
        }
        fmcw->pos += need;
        len -= need;
        if (fmcw->pos == fmcw->cfg->period) {
            feed_frame_(fmcw);
            fmcw->pos = 0;
        }
    }
    return 0;
}
