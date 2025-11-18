#include "wtk/bfio/qtk_wasr_cfg.h"

int qtk_wasr_cfg_init(qtk_wasr_cfg_t *cfg) {
    cfg->audio_cache_ms = 1000;
    cfg->audio_sample_rate = 16000;
    cfg->wake_fe_adjust = 0;
    cfg->use_qmmse = 0;
    cfg->use_vad_start = 0;
    if (wtk_vad_cfg_init(&cfg->vad) ||
        qtk_decoder_wrapper_cfg_init(&cfg->decoder) ||
        wtk_kwake_cfg_init(&cfg->wake) || wtk_qmmse_cfg_init(&cfg->qmmse) ||
        wtk_stft2_cfg_init(&cfg->stft2)) {
        return -1;
    }
    return 0;
}

int qtk_wasr_cfg_clean(qtk_wasr_cfg_t *cfg) {
    if (wtk_vad_cfg_clean(&cfg->vad) ||
        qtk_decoder_wrapper_cfg_clean(&cfg->decoder) ||
        wtk_kwake_cfg_clean(&cfg->wake) || wtk_qmmse_cfg_clean(&cfg->qmmse) ||
        wtk_stft2_cfg_clean(&cfg->stft2)) {
        return -1;
    }
    return 0;
}

int qtk_wasr_cfg_update(qtk_wasr_cfg_t *cfg) {
    if (wtk_vad_cfg_update(&cfg->vad) ||
        qtk_decoder_wrapper_cfg_update(&cfg->decoder) ||
        wtk_kwake_cfg_update(&cfg->wake) || wtk_qmmse_cfg_update(&cfg->qmmse) ||
        wtk_stft2_cfg_update(&cfg->stft2)) {
        return -1;
    }
    return 0;
}

int qtk_wasr_cfg_update_local(qtk_wasr_cfg_t *cfg, wtk_local_cfg_t *m) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    lc = wtk_local_cfg_find_lc_s(m, "vad");
    if (lc) {
        if (wtk_vad_cfg_update_local(&cfg->vad, lc)) {
            goto err;
        }
    }

    lc = wtk_local_cfg_find_lc_s(m, "wakeup");
    if (lc) {
        if (wtk_kwake_cfg_update_local(&cfg->wake, lc)) {
            goto err;
        }
    }

    lc = wtk_local_cfg_find_lc_s(m, "asr");
    if (lc) {
        if (qtk_decoder_wrapper_cfg_update_local(&cfg->decoder, lc)) {
            goto err;
        }
    }

    wtk_local_cfg_update_cfg_f(m, cfg, wake_fe_adjust, v);
    wtk_local_cfg_update_cfg_i(m, cfg, audio_cache_ms, v);
    wtk_local_cfg_update_cfg_i(m, cfg, audio_sample_rate, v);
    wtk_local_cfg_update_cfg_b(m, cfg, use_qmmse, v);
    wtk_local_cfg_update_cfg_b(m, cfg, use_vad_start, v);

    if (cfg->use_qmmse) {
        lc = wtk_local_cfg_find_lc_s(m, "stft2");
        if (lc) {
            if (wtk_stft2_cfg_update_local(&cfg->stft2, lc)) {
                goto err;
            }
        }

        cfg->qmmse.step = cfg->stft2.win / 2;
        lc = wtk_local_cfg_find_lc_s(m, "qmmse");
        if (lc) {
            if (wtk_qmmse_cfg_update_local(&cfg->qmmse, lc)) {
                goto err;
            }
        }
    }

    return 0;
err:
    return -1;
}
