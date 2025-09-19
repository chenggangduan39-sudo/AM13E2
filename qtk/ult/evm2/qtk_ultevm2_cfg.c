#include "qtk/ult/evm2/qtk_ultevm2_cfg.h"

int qtk_ultevm2_cfg_init(qtk_ultevm2_cfg_t *cfg) {
    cfg->fc_est_len = 48000 * 2;
    cfg->fc_sig = 18e3;
    cfg->fc_search_width = 100;
    cfg->sample_rate = 48e3;
    cfg->ws1 = cfg->fc_sig - 1000;
    cfg->ws2 = cfg->fc_sig + 1000;
    cfg->winsize = 2048;
    cfg->fftsize = 4096;
    cfg->beta = 0.2;
    cfg->span = 20;
    cfg->passband = 400;
    cfg->median_filter_len = 4800;
    cfg->Q = 80;
    cfg->slide_dur = 0.1;
    cfg->frame_size = 128;
    cfg->tri_filter_gap = 3;
    cfg->tri_dropbin = 2;
    cfg->feat_max_diff_thresh = 2;
    cfg->feat_max_alpha = 0.5;
    cfg->active_enter_trap_dur = 1;
    cfg->active_leave_trap_dur = 1;
    return 0;
}

int qtk_ultevm2_cfg_clean(qtk_ultevm2_cfg_t *cfg) { return 0; }

int qtk_ultevm2_cfg_update(qtk_ultevm2_cfg_t *cfg) {
    cfg->sps = cfg->sample_rate / cfg->passband;
    cfg->fc_est_len = powf(2, ceilf(log2f(cfg->fc_est_len)));
    cfg->tri_numbin = floor((cfg->frame_size / 2.0) / cfg->tri_filter_gap) - 1;
    cfg->active_enter_trap = cfg->active_enter_trap_dur / cfg->slide_dur;
    cfg->active_leave_trap = cfg->active_leave_trap_dur / cfg->slide_dur;
    return 0;
}

int qtk_ultevm2_cfg_update2(qtk_ultevm2_cfg_t *cfg, wtk_source_loader_t *loader)
{
    return qtk_ultevm2_cfg_update(cfg);
}

int qtk_ultevm2_cfg_update_local(qtk_ultevm2_cfg_t *cfg, wtk_local_cfg_t *lc) {
    wtk_string_t *v;
    
    wtk_local_cfg_update_cfg_i(lc, cfg, fc_est_len, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, fc_sig, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, sample_rate, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, fc_search_width, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, winsize, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, fftsize, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, ws1, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, ws2, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, beta, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, span, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, passband, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, median_filter_len, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, Q, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, slide_dur, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, frame_size, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, tri_filter_gap, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, tri_dropbin, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, feat_max_diff_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, feat_max_alpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, active_enter_trap_dur, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, active_leave_trap_dur, v);
    return 0;
}
