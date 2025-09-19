#include "wtk_bfio_cfg.h"

int wtk_bfio_cfg_init(wtk_bfio_cfg_t *cfg) {
    wtk_stft2_cfg_init(&(cfg->stft2));
    wtk_stft2_cfg_init(&(cfg->sp_stft2));

    wtk_aec_cfg_init(&(cfg->aec));
    wtk_wbf_cfg_init(&(cfg->wbf));
    wtk_wbf2_cfg_init(&(cfg->wbf2));
    wtk_gainnet_denoise_cfg_init(&(cfg->gdenoise));
    wtk_kvadwake_cfg_init(&(cfg->vwake));
    wtk_kvadwake_cfg_init(&(cfg->vwake2));
    wtk_kvadwake_cfg_init(&(cfg->vwake3));

    wtk_ssl_cfg_init(&(cfg->ssl));
    wtk_ssl2_cfg_init(&(cfg->ssl2));
    wtk_qform9_cfg_init(&(cfg->qform9));
    wtk_qform2_cfg_init(&(cfg->qform2));
    wtk_qform3_cfg_init(&(cfg->qform3));
    qtk_decoder_wrapper_cfg_init(&cfg->decoder);

    wtk_vad_cfg_init(&(cfg->vad));

    cfg->stft2_hist = 1800; // ms

    cfg->hook = NULL;

    cfg->rate = 16000;

    cfg->use_vad_start = 0;

    cfg->vad_left_margin = 20;
    cfg->vad_right_margin = 20;

    cfg->use_asr = 1;
    cfg->use_aec = 1;
    cfg->use_offline_asr = 0;

    cfg->wake_ssl_fs = 0.1;
    cfg->wake_ssl_fe = -0.3;

    cfg->use_wbf = 1;
    cfg->use_wbf2 = 0;
    cfg->use_qform2 = 0;
    cfg->use_qform3 = 0;
    cfg->use_qform9 = 1;
    cfg->use_gdenoise = 0;

    cfg->use_preemph = 0;
    cfg->use_raw_audio = 0;
    cfg->use_all_raw_audio = 0;
    cfg->debug = 1;
    cfg->use_ssl2 = 0;
    cfg->use_one_shot = 0;
    cfg->sil_delay = 3000;
    cfg->speech_delay = 5000;
    cfg->use_line = 1;
    cfg->use_kvadwake = 1;
    cfg->use_kvadwake2 = 0;
    cfg->use_vad = 1;

    cfg->use_en_trick = 0;
    cfg->de_wake_len = 5000*16*2;
    cfg->energy_conf = 1e-5;

    cfg->use_aec_wake = 0;
    cfg->aec_wake_fs = -2.0;
    cfg->aec_wake_fe = 0.3;
    cfg->aec_wake_len = 5000*16*2;
    cfg->ressl_range = 15;

    cfg->low_fs = 150;
    cfg->low_fe = 2000;
    cfg->low_thresh=15000;
    cfg->use_low_trick = 0;

    return 0;
}

int wtk_bfio_cfg_clean(wtk_bfio_cfg_t *cfg) {
    wtk_stft2_cfg_clean(&(cfg->stft2));
    wtk_stft2_cfg_clean(&(cfg->sp_stft2));
    wtk_aec_cfg_clean(&(cfg->aec));
    wtk_vad_cfg_clean(&(cfg->vad));
    wtk_wbf_cfg_clean(&(cfg->wbf));
    wtk_wbf2_cfg_clean(&(cfg->wbf2));
    wtk_gainnet_denoise_cfg_clean(&(cfg->gdenoise));
    wtk_kvadwake_cfg_clean(&(cfg->vwake));
    wtk_kvadwake_cfg_clean(&(cfg->vwake2));
    wtk_kvadwake_cfg_clean(&(cfg->vwake3));
    wtk_ssl_cfg_clean(&(cfg->ssl));
    wtk_ssl2_cfg_clean(&(cfg->ssl2));
    wtk_qform2_cfg_clean(&(cfg->qform2));
    wtk_qform3_cfg_clean(&(cfg->qform3));
    wtk_qform9_cfg_clean(&(cfg->qform9));
    qtk_decoder_wrapper_cfg_clean(&cfg->decoder);

    return 0;
}

int wtk_bfio_cfg_update_local(wtk_bfio_cfg_t *cfg, wtk_local_cfg_t *lc) {
    wtk_string_t *v;
    wtk_local_cfg_t *m;
    int ret;

    wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, stft2_hist, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_vad_start, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, vad_left_margin, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, vad_right_margin, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_asr, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_offline_asr, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_aec, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_wbf, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_wbf2, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_qform2, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_qform3, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_qform9, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, wake_ssl_fs, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, wake_ssl_fe, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_preemph, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, debug, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_raw_audio, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_all_raw_audio, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_ssl2, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_one_shot, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sil_delay, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, speech_delay, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_gdenoise, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_line, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_kvadwake, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_kvadwake2, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_vad, v);
    
    wtk_local_cfg_update_cfg_b(lc, cfg, use_en_trick, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, de_wake_len, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, energy_conf, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_aec_wake, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, aec_wake_fs, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, aec_wake_fe, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, aec_wake_len, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, ressl_range, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, low_fs, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, low_fe, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, low_thresh, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_low_trick, v);
    
    m = lc;
    lc = wtk_local_cfg_find_lc_s(m, "stft2");
    if (lc) {
        ret = wtk_stft2_cfg_update_local(&(cfg->stft2), lc);
        if (ret != 0) {
            goto end;
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "sp_stft2");
    if (lc) {
        ret = wtk_stft2_cfg_update_local(&(cfg->sp_stft2), lc);
        if (ret != 0) {
            goto end;
        }
    }
    if(cfg->use_kvadwake){
        lc = wtk_local_cfg_find_lc_s(m, "kvadwake");
        if (lc) {
            ret = wtk_kvadwake_cfg_update_local(&(cfg->vwake), lc);
            if (ret != 0) {
                goto end;
            }
        }
    }
    if(cfg->use_kvadwake2){
        lc = wtk_local_cfg_find_lc_s(m, "kvadwake2");
        if (lc) {
            ret = wtk_kvadwake_cfg_update_local(&(cfg->vwake2), lc);
            if (ret != 0) {
                goto end;
            }
        }
    }
    if(cfg->use_aec_wake){
        if(!cfg->use_kvadwake){
            wtk_debug("error: use_aec_wake must be used with use_kvadwake\n");
            ret = -1;
            goto end;
        }
        lc = wtk_local_cfg_find_lc_s(m, "aec_wake");
        if (lc) {
            ret = wtk_kvadwake_cfg_update_local(&(cfg->vwake3), lc);
            if (ret != 0) {
                goto end;
            }
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "ssl");
    if (lc) {
        ret = wtk_ssl_cfg_update_local(&(cfg->ssl), lc);
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_aec) {
        lc = wtk_local_cfg_find_lc_s(m, "aec");
        if (lc) {
            ret = wtk_aec_cfg_update_local(&(cfg->aec), lc);
            if (ret != 0) {
                goto end;
            }
        }
    }
    if (cfg->use_wbf) {
        lc = wtk_local_cfg_find_lc_s(m, "wbf");
        if (lc) {
            ret = wtk_wbf_cfg_update_local(&(cfg->wbf), lc);
            if (ret != 0) {
                goto end;
            }
        }
    }
    if (cfg->use_wbf2) {
        lc = wtk_local_cfg_find_lc_s(m, "wbf2");
        if (lc) {
            ret = wtk_wbf2_cfg_update_local(&(cfg->wbf2), lc);
            if (ret != 0) {
                goto end;
            }
        }
    }
    if (cfg->use_gdenoise) {
        lc = wtk_local_cfg_find_lc_s(m, "gdenoise");
        if (lc) {
            ret = wtk_gainnet_denoise_cfg_update_local(&(cfg->gdenoise), lc);
            if (ret != 0) {
                goto end;
            }
        }
    }
    if (cfg->use_ssl2) {
        lc = wtk_local_cfg_find_lc_s(m, "ssl2");
        if (lc) {
            ret = wtk_ssl2_cfg_update_local(&(cfg->ssl2), lc);
            if (ret != 0) {
                goto end;
            }
        }
    }
    if (cfg->use_qform2) {
        lc = wtk_local_cfg_find_lc_s(m, "qform2");
        if (lc) {
            ret = wtk_qform2_cfg_update_local(&(cfg->qform2), lc);
            if (ret != 0) {
                goto end;
            }
        }
    }
    if (cfg->use_qform3) {
        lc = wtk_local_cfg_find_lc_s(m, "qform3");
        if (lc) {
            ret = wtk_qform3_cfg_update_local(&(cfg->qform3), lc);
            if (ret != 0) {
                goto end;
            }
        }
    }
    if (cfg->use_qform9) {
        lc = wtk_local_cfg_find_lc_s(m, "qform9");
        if (lc) {
            ret = wtk_qform9_cfg_update_local(&(cfg->qform9), lc);
            if (ret != 0) {
                goto end;
            }
        }
    }
    if(cfg->use_asr && cfg->use_vad){
        lc = wtk_local_cfg_find_lc_s(m, "vad");
        if (lc) {
            ret = wtk_vad_cfg_update_local(&(cfg->vad), lc);
            if (ret != 0) {
                goto end;
            }
        }
    }
    if (cfg->use_offline_asr) {
        lc = wtk_local_cfg_find_lc_s(m, "asr");
        if (lc) {
            ret = qtk_decoder_wrapper_cfg_update_local(&cfg->decoder, lc);
            if (ret != 0) {
                goto end;
            }
        }
    }

    ret = 0;
end:
    return ret;
}

int wtk_bfio_cfg_update(wtk_bfio_cfg_t *cfg) {
    int ret;

    ret = wtk_stft2_cfg_update(&(cfg->stft2));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_stft2_cfg_update(&(cfg->sp_stft2));
    if (ret != 0) {
        goto end;
    }
    if(cfg->use_kvadwake){
        ret = wtk_kvadwake_cfg_update(&(cfg->vwake));
        if (ret != 0) {
            goto end;
        }
    }
    if(cfg->use_kvadwake2){
        ret = wtk_kvadwake_cfg_update(&(cfg->vwake2));
        if (ret != 0) {
            goto end;
        }
        cfg->vwake2.debug = cfg->debug;
    }
    if(cfg->use_aec_wake){
        ret = wtk_kvadwake_cfg_update(&(cfg->vwake3));
        if (ret != 0) {
            goto end;
        }
        cfg->vwake3.debug = cfg->debug;
    }
    ret = wtk_ssl_cfg_update(&(cfg->ssl));
    if (ret != 0) {
        goto end;
    }
    if (cfg->use_aec) {
        ret = wtk_aec_cfg_update(&(cfg->aec));
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_wbf) {
        ret = wtk_wbf_cfg_update(&(cfg->wbf));
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_wbf2) {
        ret = wtk_wbf2_cfg_update(&(cfg->wbf2));
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_ssl2) {
        ret = wtk_ssl2_cfg_update(&(cfg->ssl2));
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_qform9) {
        ret = wtk_qform9_cfg_update(&(cfg->qform9));
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_qform2) {
        ret = wtk_qform2_cfg_update(&(cfg->qform2));
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_qform3) {
        ret = wtk_qform3_cfg_update(&(cfg->qform3));
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_gdenoise) {
        ret = wtk_gainnet_denoise_cfg_update(&(cfg->gdenoise));
        if (ret != 0) {
            goto end;
        }
    }

    if (cfg->use_asr && cfg->use_vad) {
        ret = wtk_vad_cfg_update(&(cfg->vad));
        if (ret != 0) {
            goto end;
        }
    }
    cfg->stft2_hist = cfg->stft2_hist / 1000.0 * cfg->rate / cfg->stft2.step;
    if (cfg->use_offline_asr && cfg->use_asr == 0) {
        wtk_debug("!!! Err: use_offline_asr depend on use_asr\n");
        ret = -1;
        goto end;
    }
    if (cfg->use_offline_asr) {
        qtk_decoder_wrapper_cfg_update(&cfg->decoder);
    }
    cfg->vwake.debug = cfg->debug;
    if(cfg->use_qform3){
        cfg->use_line = cfg->qform3.use_line;
    }
    if(cfg->use_qform9){
        cfg->use_line = cfg->qform9.use_line;
    }
    cfg->low_fs_idx = ceil(cfg->low_fs / (cfg->rate / 2)*cfg->stft2.step);
    cfg->low_fe_idx = floor(cfg->low_fe / (cfg->rate / 2)*cfg->stft2.step);

    ret = 0;
end:
    return ret;
}

int wtk_bfio_cfg_update2(wtk_bfio_cfg_t *cfg, wtk_source_loader_t *sl) {
    int ret;

    ret = wtk_stft2_cfg_update(&(cfg->stft2));
    if (ret != 0) {
        goto end;
    }
    ret = wtk_stft2_cfg_update(&(cfg->sp_stft2));
    if (ret != 0) {
        goto end;
    }
    if(cfg->use_kvadwake){
        ret = wtk_kvadwake_cfg_update2(&(cfg->vwake), sl);
        if (ret != 0) {
            goto end;
        }
    }
    if(cfg->use_kvadwake2){
        ret = wtk_kvadwake_cfg_update2(&(cfg->vwake2), sl);
        if (ret != 0) {
            goto end;
        }
        cfg->vwake2.debug = cfg->debug;
    }
    if(cfg->use_aec_wake){
        ret = wtk_kvadwake_cfg_update2(&(cfg->vwake3), sl);
        if (ret != 0) {
            goto end;
        }
        cfg->vwake3.debug = cfg->debug;
    }
    ret = wtk_ssl_cfg_update(&(cfg->ssl));
    if (ret != 0) {
        goto end;
    }
    if (cfg->use_aec) {
        ret = wtk_aec_cfg_update2(&(cfg->aec), sl);
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_wbf) {
        ret = wtk_wbf_cfg_update2(&(cfg->wbf), sl);
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_wbf2) {
        ret = wtk_wbf2_cfg_update2(&(cfg->wbf2), sl);
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_ssl2) {
        ret = wtk_ssl2_cfg_update(&(cfg->ssl2));
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_qform9) {
        ret = wtk_qform9_cfg_update2(&(cfg->qform9), sl);
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_qform2) {
        ret = wtk_qform2_cfg_update2(&(cfg->qform2), sl);
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_qform3) {
        ret = wtk_qform3_cfg_update2(&(cfg->qform3), sl);
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_gdenoise) {
        ret = wtk_gainnet_denoise_cfg_update2(&(cfg->gdenoise), sl);
        if (ret != 0) {
            goto end;
        }
    }

    if (cfg->use_asr && cfg->use_vad) {
        ret = wtk_vad_cfg_update2(&(cfg->vad), sl);
        if (ret != 0) {
            goto end;
        }
    }
    cfg->stft2_hist = cfg->stft2_hist / 1000.0 * cfg->rate / cfg->stft2.step;
    if (cfg->use_offline_asr && cfg->use_asr == 0) {
        wtk_debug("!!! Err: use_offline_asr depend on use_asr\n");
        ret = -1;
        goto end;
    }
    if (cfg->use_offline_asr) {
        qtk_decoder_wrapper_cfg_update2(&cfg->decoder, sl);
    }
    cfg->vwake.debug = cfg->debug;
    if(cfg->use_qform3){
        cfg->use_line = cfg->qform3.use_line;
    }
    if(cfg->use_qform9){
        cfg->use_line = cfg->qform9.use_line;
    }
    cfg->low_fs_idx = ceil(cfg->low_fs / (cfg->rate / 2));
    cfg->low_fe_idx = floor(cfg->low_fe / (cfg->rate / 2));

    ret = 0;
end:
    return ret;
}

void wtk_bfio_cfg_set_wakeword(wtk_bfio_cfg_t *bfio, char *wrd) {
    if(bfio->use_kvadwake){
        wtk_kvadwake_cfg_set_wakeword(&(bfio->vwake), wrd);
    }
    if(bfio->use_kvadwake2){
        wtk_kvadwake_cfg_set_wakeword(&(bfio->vwake2), wrd);
    }
    if(bfio->use_aec_wake){
        wtk_kvadwake_cfg_set_wakeword(&(bfio->vwake3), wrd);
    }
}

wtk_bfio_cfg_t *wtk_bfio_cfg_new(char *cfg_fn) {
    wtk_main_cfg_t *main_cfg;
    wtk_bfio_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(wtk_bfio_cfg, cfg_fn);
    cfg = (wtk_bfio_cfg_t *)main_cfg->cfg;
    cfg->hook = main_cfg;
    return cfg;
}

void wtk_bfio_cfg_delete(wtk_bfio_cfg_t *cfg) {
    wtk_main_cfg_delete((wtk_main_cfg_t *)cfg->hook);
}

wtk_bfio_cfg_t *wtk_bfio_cfg_new_bin(char *bin_fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_bfio_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(wtk_bfio_cfg, bin_fn, "./bfio.cfg");
    cfg = (wtk_bfio_cfg_t *)mbin_cfg->cfg;
    cfg->hook = mbin_cfg;
    return cfg;
}

void wtk_bfio_cfg_delete_bin(wtk_bfio_cfg_t *cfg) {
    wtk_mbin_cfg_delete((wtk_mbin_cfg_t *)cfg->hook);
}
