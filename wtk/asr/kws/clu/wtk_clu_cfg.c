#include "wtk/asr/kws/clu/wtk_clu_cfg.h"

int wtk_clu_cfg_init(wtk_clu_cfg_t *cfg) {
    cfg->hop_s = 0.75;
    cfg->hop_num_byte = 24000;
    cfg->chunk_s = 1.5;
    cfg->chunk_num_byte = 48000;
    cfg->vprint_s = 300;
    cfg->vprint_num_byte = 400;
    cfg->vprint_size = 256;
    cfg->sample_rate = 16000;
    cfg->byte_per_sample = 2;
    cfg->chunk_dur = 2;
    wtk_nnet3_xvector_compute_cfg_init(&cfg->xvector);
    return 0;
}

int wtk_clu_cfg_clean(wtk_clu_cfg_t *cfg) {
    wtk_nnet3_xvector_compute_cfg_clean(&cfg->xvector);
    return 0;
}

int wtk_clu_cfg_update(wtk_clu_cfg_t *cfg) {
    wtk_nnet3_xvector_compute_cfg_update(&cfg->xvector);
    cfg->hop_num_byte =
        (int)ceil(cfg->hop_s * cfg->sample_rate * cfg->byte_per_sample);
    cfg->chunk_num_byte =
        (int)ceil(cfg->chunk_s * cfg->sample_rate * cfg->byte_per_sample);
    cfg->vprint_num_byte = (int)ceil(cfg->vprint_s * cfg->chunk_s);

    return 0;
}

int wtk_clu_cfg_update2(wtk_clu_cfg_t *cfg, wtk_source_loader_t *sl) {
    int ret = 0;

    ret = wtk_nnet3_xvector_compute_cfg_update2(&cfg->xvector, sl);
    cfg->hop_num_byte =
        (int)ceil(cfg->hop_s * cfg->sample_rate * cfg->byte_per_sample);
    cfg->chunk_num_byte =
        (int)ceil(cfg->chunk_s * cfg->sample_rate * cfg->byte_per_sample);
    cfg->vprint_num_byte = (int)ceil(cfg->vprint_s / cfg->chunk_s);

    return ret;
}

int wtk_clu_cfg_update_local(wtk_clu_cfg_t *cfg, wtk_local_cfg_t *main) {
    int ret = 0;
    wtk_string_t *v = NULL;
    wtk_local_cfg_t *lc = main;
    wtk_local_cfg_t *lc2 = NULL;

    wtk_local_cfg_update_cfg_f(lc, cfg, hop_s, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, chunk_s, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, vprint_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, sample_rate, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, chunk_dur, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, byte_per_sample, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, vprint_size, v);

    lc2 = wtk_local_cfg_find_lc_s(lc, "xvector");
    if (lc2) {
        wtk_nnet3_xvector_compute_cfg_update_local(&cfg->xvector, lc2);
    }

    return ret;
}
