#include "qtk_spx_cfg.h"

int qtk_spx_cfg_init(qtk_spx_cfg_t *cfg) {
    qtk_oggenc_cfg_init(&cfg->oggenc);
    wtk_string_set(&cfg->coreType, 0, 0);
    wtk_string_set(&cfg->res, 0, 0);
    wtk_string_set(&cfg->env, 0, 0);
    wtk_string_set(&cfg->semRes, 0, 0);
    wtk_string_set(&cfg->synRes, 0, 0);

    cfg->volume = 5.0f;
    cfg->speed = 1.0f;
    cfg->pitch = 1.2f;

    cfg->iType = -1;

    cfg->rate = 16000;
    cfg->channel = 1;
    cfg->bytes_per_sample = 2;

    cfg->use_ogg = 1;
    cfg->useStream = 1;

    cfg->timeout = 20000;
    cfg->cache = 10;
    cfg->bufsize = 1024;
    cfg->lua_bufsize = 256;
    cfg->use_luabuf = 0;

    cfg->max_size = 10;

    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;
    cfg->use_hint = 0;
    cfg->use_hotword = 0;
    cfg->skip_space = 1;
    return 0;
}

int qtk_spx_cfg_clean(qtk_spx_cfg_t *cfg) {
    if (cfg->use_ogg) {
        qtk_oggenc_cfg_clean(&cfg->oggenc);
    }
    return 0;
}

int qtk_spx_cfg_update_local(qtk_spx_cfg_t *cfg, wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    lc = main;
    wtk_local_cfg_update_cfg_string_v(lc, cfg, coreType, v);
    wtk_local_cfg_update_cfg_string_v(lc, cfg, res, v);
    wtk_local_cfg_update_cfg_string_v(lc, cfg, env, v);
    wtk_local_cfg_update_cfg_string_v(lc, cfg, semRes, v);
    wtk_local_cfg_update_cfg_string_v(lc, cfg, synRes, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, volume, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, speed, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, pitch, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, iType, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, bytes_per_sample, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, timeout, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, cache, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, bufsize, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, lua_bufsize, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, max_size, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_ogg, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, useStream, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_luabuf, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_hint, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, skip_space, v);

    if (cfg->use_ogg) {
        lc = wtk_local_cfg_find_lc_s(main, "oggenc");
        if (lc) {
            qtk_oggenc_cfg_update_local(&cfg->oggenc, lc);
        }
    }

    return 0;
}

int qtk_spx_cfg_update(qtk_spx_cfg_t *cfg) {
    if (cfg->use_ogg) {
        qtk_oggenc_cfg_update(&cfg->oggenc);
    }
    return 0;
}

int qtk_spx_cfg_update2(qtk_spx_cfg_t *cfg, wtk_source_loader_t *sl) {
    return qtk_spx_cfg_update(cfg);
}

void qtk_spx_cfg_update_params(qtk_spx_cfg_t *cfg, wtk_local_cfg_t *params) {
    wtk_string_t *v;

    v = wtk_local_cfg_find_string(params, "tts_volume", strlen("tts_volume"));
    if (v) {
        cfg->volume = atof(v->data);
    }
    v = wtk_local_cfg_find_string(params, "tts_speed", strlen("tts_speed"));
    if (v) {
        cfg->speed = atof(v->data);
    }
    v = wtk_local_cfg_find_string(params, "tts_pitch", strlen("tts_pitch"));
    if (v) {
        cfg->pitch = atof(v->data);
    }
    v = wtk_local_cfg_find_string(params, "coreType", strlen("coreType"));
    if (v) {
        wtk_string_set(&cfg->coreType, v->data, v->len);
    }
    v = wtk_local_cfg_find_string(params, "res", strlen("res"));
    if (v) {
        wtk_string_set(&cfg->res, v->data, v->len);
    }
    v = wtk_local_cfg_find_string(params, "semRes", strlen("semRes"));
    if (v) {
        wtk_string_set(&cfg->semRes, v->data, v->len);
    }
    v = wtk_local_cfg_find_string(params, "useStream", strlen("useStream"));
    if (v) {
        cfg->useStream = atoi(v->data);
    }
    v = wtk_local_cfg_find_string(params, "skip_space", strlen("skip_space"));
    if (v) {
        cfg->skip_space = atoi(v->data);
    }
    v = wtk_local_cfg_find_string(params, "use_hint", strlen("use_hint"));
    if (v) {
        cfg->use_hint = atoi(v->data);
    }
}

void qtk_spx_cfg_update_option(qtk_spx_cfg_t *cfg, qtk_option_t *option) {
    cfg->timeout = option->timeout;
}

qtk_spx_cfg_t *qtk_spx_cfg_new(char *fn) {
    wtk_main_cfg_t *main_cfg;
    qtk_spx_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(qtk_spx_cfg, fn);
    cfg = (qtk_spx_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;

    return cfg;
}

void qtk_spx_cfg_delete(qtk_spx_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_spx_cfg_t *qtk_spx_cfg_new_bin(char *bin_fn, int seek_pos) {
    wtk_mbin_cfg_t *mbin_cfg;
    qtk_spx_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type2(seek_pos, qtk_spx_cfg, bin_fn, "./cfg");
    cfg = (qtk_spx_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;

    return cfg;
}

void qtk_spx_cfg_delete_bin(qtk_spx_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
