#include "qtk/vprint/qtk_speaker_diarization_cfg.h"

int qtk_speaker_diarization_cfg_init(qtk_speaker_diarization_cfg_t *cfg) {
    wtk_nnet3_xvector_compute_cfg_init(&cfg->xvector);
    wtk_kvad_cfg_init(&cfg->kvad);
    wtk_kxparm_cfg_init(&cfg->kxparm);
    cfg->sliding_step_frames = 24;
    cfg->sliding_win_frames = 144;
    cfg->emb_len = 256;
    cfg->feat_dim = 80;
    cfg->feat_frame_ms = 10;
    cfg->vad_frame_ms = 10;
    cfg->spec_pval = 0.2;
    return 0;
}

int qtk_speaker_diarization_cfg_clean(qtk_speaker_diarization_cfg_t *cfg) {
    wtk_nnet3_xvector_compute_cfg_clean(&cfg->xvector);
    wtk_kvad_cfg_clean(&cfg->kvad);
    wtk_kxparm_cfg_clean(&cfg->kxparm);
    return 0;
}

int qtk_speaker_diarization_cfg_update(qtk_speaker_diarization_cfg_t *cfg) {
    wtk_nnet3_xvector_compute_cfg_update(&cfg->xvector);
    wtk_kvad_cfg_update(&cfg->kvad);
    wtk_kxparm_cfg_update(&cfg->kxparm);
    return 0;
}

int qtk_speaker_diarization_cfg_update_local(qtk_speaker_diarization_cfg_t *cfg,
                                             wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;
    lc = wtk_local_cfg_find_lc_s(main, "xvector");
    if (lc) {
        wtk_nnet3_xvector_compute_cfg_update_local(&cfg->xvector, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "kvad");
    if (lc) {
        wtk_kvad_cfg_update_local(&cfg->kvad, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "kxparm");
    if (lc) {
        wtk_kxparm_cfg_update_local(&cfg->kxparm, lc);
    }

    wtk_local_cfg_update_cfg_i(main, cfg, sliding_win_frames, v);
    wtk_local_cfg_update_cfg_i(main, cfg, sliding_step_frames, v);
    wtk_local_cfg_update_cfg_f(main, cfg, spec_pval, v);

    return 0;
}
