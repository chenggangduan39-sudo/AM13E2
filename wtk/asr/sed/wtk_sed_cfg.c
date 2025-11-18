#include "wtk/asr/sed/wtk_sed_cfg.h"

int wtk_sed_event_params_init(wtk_sed_event_params_t *ep) {
    ep->high_threshold = 0.2;
    ep->low_threshold = 0.05;
    ep->high_n_smooth = 10;
    ep->low_n_smooth = 1;
    ep->n_salt = 10;

    return 0;
}

int wtk_sed_event_params_clean(wtk_sed_event_params_t *ep) { return 0; }

int wtk_sed_event_params_update_local(wtk_sed_event_params_t *cfg,
                                      wtk_local_cfg_t *lc) {
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_f(lc, cfg, low_threshold, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, high_threshold, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, high_n_smooth, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, low_n_smooth, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, n_salt, v);

    return 0;
}

int wtk_sed_event_params_update(wtk_sed_event_params_t *ep) { return 0; }

int wtk_sed_cfg_init(wtk_sed_cfg_t *cfg) {
    wtk_sed_event_params_init(&(cfg->shout_params));
    wtk_sed_event_params_init(&(cfg->cry_params));
    cfg->sample_duration = 8;
    cfg->sample_rate = 16000;
    cfg->byte_per_sample = 2;
    cfg->wav_step = 1;
    cfg->onnx_output_index = 0;
    cfg->seconds_per_index = 0.01;
    cfg->index_per_second = 100;
    cfg->wav_cross = 7;

#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_init(&(cfg->onnx));
#endif

    return 0;
}

int wtk_sed_cfg_clean(wtk_sed_cfg_t *cfg) {
    wtk_sed_event_params_clean(&(cfg->shout_params));
    wtk_sed_event_params_clean(&(cfg->cry_params));
#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_clean(&(cfg->onnx));
#endif

    return 0;
}

int wtk_sed_cfg_update_local(wtk_sed_cfg_t *cfg, wtk_local_cfg_t *main) {
    int ret = 0;
    wtk_string_t *v;
    wtk_local_cfg_t *lc = main;

    wtk_local_cfg_update_cfg_i(lc, cfg, wav_step, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, sample_rate, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, sample_duration, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, byte_per_sample, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, onnx_output_index, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, index_per_second, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, seconds_per_index, v);

    lc = wtk_local_cfg_find_lc_s(main, "shout_params");
    if (lc) {
        ret = wtk_sed_event_params_update_local(&(cfg->shout_params), lc);
    }

    lc = wtk_local_cfg_find_lc_s(main, "cry_params");
    if (lc) {
        ret = wtk_sed_event_params_update_local(&(cfg->cry_params), lc);
    }

#ifdef ONNX_DEC
    lc = wtk_local_cfg_find_lc_s(lc, "onnx");
    if (lc) {
        ret = qtk_onnxruntime_cfg_update_local(&(cfg->onnx), lc);
        if (ret != 0) {
            wtk_debug("update local onnx failed\n");
            goto end;
        }
    }
end:
#endif

    return ret;
}

int wtk_sed_cfg_update(wtk_sed_cfg_t *cfg) {
    cfg->wav_cross = cfg->sample_duration - cfg->wav_step;

    return 0;
}

int wtk_sed_cfg_update2(wtk_sed_cfg_t *cfg, wtk_source_loader_t *sl) {
    int ret = 0;
#ifdef ONNX_DEC
    ret = qtk_onnxruntime_cfg_update2(&(cfg->onnx), sl->hook);
    if (ret != 0) {
        wtk_debug("update onnx failed\n");
        goto end;
    }
end:
#endif

    return ret;
}
