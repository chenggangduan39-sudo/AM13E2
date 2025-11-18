#include "wtk/bfio/maskdenoise/cmask_pse/wtk_cmask_pse2_cfg.h"

int wtk_cmask_pse2_cfg_init(wtk_cmask_pse2_cfg_t *cfg) {
    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;

    cfg->channel = 0;
    cfg->nmicchannel = 0;
    cfg->mic_channel = NULL;
    cfg->nspchannel = 0;
    cfg->sp_channel = NULL;
    cfg->nbfchannel = 0;

    cfg->wins = 1024;
    cfg->rate = 16000;

    cfg->emb_len = 0;
    cfg->gb_len = 0;
    cfg->emb_feat_len = NULL;
    cfg->nemb_feat = 0;

#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_init(&(cfg->emb));
#endif
    cfg->num_frame = 1;
    wtk_fbank_cfg_init(&(cfg->fbank));
    qtk_nnrt_cfg_init(&cfg->pse_rt);
    wtk_mask_bf_cfg_init(&(cfg->mask_bf));

    cfg->clip_s = 0;
    cfg->clip_e = 8000;

    cfg->sym = 1e-2;
    cfg->cnon_clip_s = 7000;
    cfg->cnon_clip_e = 8000;

    cfg->spenr_thresh = 100;
    cfg->spenr_cnt = 10;
    cfg->micenr_thresh = 300;
    cfg->micenr_cnt = 10;

    cfg->max_bs_out = 32000;

    cfg->use_onnx = 1;
    cfg->use_cnon = 0;
    cfg->use_bf = 0;

    return 0;
}

int wtk_cmask_pse2_cfg_clean(wtk_cmask_pse2_cfg_t *cfg) {
    if (cfg->mic_channel) {
        wtk_free(cfg->mic_channel);
    }
    if (cfg->sp_channel) {
        wtk_free(cfg->sp_channel);
    }
    if (cfg->emb_feat_len) {
        wtk_free(cfg->emb_feat_len);
    }
#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_clean(&(cfg->emb));
#endif
    wtk_fbank_cfg_clean(&(cfg->fbank));
    qtk_nnrt_cfg_clean(&cfg->pse_rt);
    wtk_mask_bf_cfg_clean(&(cfg->mask_bf));

    return 0;
}

int wtk_cmask_pse2_cfg_update(wtk_cmask_pse2_cfg_t *cfg) {
    int ret;

    ret = wtk_fbank_cfg_update(&(cfg->fbank));
    if (ret != 0) {
        goto end;
    }
    if (cfg->channel < cfg->nmicchannel + cfg->nspchannel) {
        cfg->channel = cfg->nmicchannel + cfg->nspchannel;
    }
    if (cfg->nbfchannel == 0) {
        cfg->nbfchannel = cfg->nmicchannel;
    }
    cfg->clip_s = (cfg->clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->clip_e = (cfg->clip_e * 1.0 * cfg->wins) / cfg->rate;
    cfg->cnon_clip_s = (cfg->cnon_clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->cnon_clip_e = (cfg->cnon_clip_e * 1.0 * cfg->wins) / cfg->rate;

#ifdef ONNX_DEC
    if (cfg->use_onnx) {
        ret = qtk_onnxruntime_cfg_update(&(cfg->emb));
        if (ret != 0) {
            wtk_debug("update onnx failed\n");
            goto end;
        }
    }
#endif
    wtk_fbank_cfg_update(&(cfg->fbank));
    qtk_nnrt_cfg_update(&cfg->pse_rt);
    {
        int emb_maybe_len[] = {cfg->emb_len, cfg->gb_len};
        int nmaybe = sizeof(emb_maybe_len) / sizeof(emb_maybe_len[0]);
        cfg->nemb_feat = 0;
        cfg->emb_feat_len = (int *)wtk_malloc(sizeof(int) * nmaybe);
        for (int i = 0; i < nmaybe; ++i) {
            if (emb_maybe_len[i] > 0) {
                cfg->emb_feat_len[cfg->nemb_feat++] = emb_maybe_len[i];
            }
        }
    }
    ret = wtk_mask_bf_cfg_update(&(cfg->mask_bf));
    if (ret != 0) {
        goto end;
    }
    ret = 0;
end:
    return ret;
}

int wtk_cmask_pse2_cfg_update2(wtk_cmask_pse2_cfg_t *cfg,
                               wtk_source_loader_t *sl) {
    int ret;

    ret = wtk_fbank_cfg_update2(&(cfg->fbank), sl->hook);
    if (ret != 0) {
        goto end;
    }
    if (cfg->channel < cfg->nmicchannel + cfg->nspchannel) {
        cfg->channel = cfg->nmicchannel + cfg->nspchannel;
    }
    if (cfg->nbfchannel == 0) {
        cfg->nbfchannel = cfg->nmicchannel;
    }
    cfg->clip_s = (cfg->clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->clip_e = (cfg->clip_e * 1.0 * cfg->wins) / cfg->rate;
    cfg->cnon_clip_s = (cfg->cnon_clip_s * 1.0 * cfg->wins) / cfg->rate;
    cfg->cnon_clip_e = (cfg->cnon_clip_e * 1.0 * cfg->wins) / cfg->rate;

#ifdef ONNX_DEC
    if (cfg->use_onnx) {
        ret = qtk_onnxruntime_cfg_update2(&(cfg->emb), sl->hook);
        if (ret != 0) {
            wtk_debug("update onnx failed\n");
            goto end;
        }
    }
#endif
    wtk_fbank_cfg_update2(&(cfg->fbank), sl->hook);
    qtk_nnrt_cfg_update2(&cfg->pse_rt, sl);
    {
        int emb_maybe_len[] = {cfg->emb_len, cfg->gb_len};
        int nmaybe = sizeof(emb_maybe_len) / sizeof(emb_maybe_len[0]);
        cfg->nemb_feat = 0;
        cfg->emb_feat_len = (int *)wtk_malloc(sizeof(int) * nmaybe);
        for (int i = 0; i < nmaybe; ++i) {
            if (emb_maybe_len[i] > 0) {
                cfg->emb_feat_len[cfg->nemb_feat++] = emb_maybe_len[i];
            }
        }
    }
    ret = wtk_mask_bf_cfg_update2(&(cfg->mask_bf), sl);
    if (ret != 0) {
        goto end;
    }
    ret = 0;
end:
    return ret;
}

int wtk_cmask_pse2_cfg_update_local(wtk_cmask_pse2_cfg_t *cfg,
                                    wtk_local_cfg_t *m) {
    wtk_string_t *v;
    wtk_local_cfg_t *lc;
    int ret;
    wtk_array_t *a;
    int i;

    lc = m;
    wtk_local_cfg_update_cfg_i(lc, cfg, wins, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, emb_len, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, gb_len, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, clip_e, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sym, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, cnon_clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, cnon_clip_e, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, spenr_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, spenr_cnt, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, micenr_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, micenr_cnt, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, max_bs_out, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, num_frame, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_onnx, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_cnon, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_bf, v);

    a = wtk_local_cfg_find_array_s(lc, "mic_channel");
    if (a) {
        cfg->mic_channel = (int *)wtk_malloc(sizeof(int) * a->nslot);
        cfg->nmicchannel = a->nslot;
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->mic_channel[i] = wtk_str_atoi(v->data, v->len);
        }
    }

    a = wtk_local_cfg_find_array_s(lc, "sp_channel");
    if (a) {
        cfg->sp_channel = (int *)wtk_malloc(sizeof(int) * a->nslot);
        cfg->nspchannel = a->nslot;
        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->sp_channel[i] = wtk_str_atoi(v->data, v->len);
        }
    }
#ifdef ONNX_DEC
    lc = wtk_local_cfg_find_lc_s(m, "emb");
    if (lc) {
        ret = qtk_onnxruntime_cfg_update_local(&(cfg->emb), lc);
        if (ret != 0) {
            wtk_debug("update local onnx failed\n");
            goto end;
        }
    }
#endif
    lc = wtk_local_cfg_find_lc_s(m, "fbank");
    if (lc) {
        ret = wtk_fbank_cfg_update_local(&(cfg->fbank), lc);
        if (ret != 0) {
            wtk_debug("update local fbank failed\n");
            goto end;
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "pse_rt");
    if (lc) {
        ret = qtk_nnrt_cfg_update_local(&(cfg->pse_rt), lc);
        if (ret != 0) {
            wtk_debug("update local nnrt failed\n");
            goto end;
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "mask_bf");
    if (lc) {
        ret = wtk_mask_bf_cfg_update_local(&(cfg->mask_bf), lc);
        if (ret != 0) {
            goto end;
        }
    }
    ret = 0;
end:
    return ret;
}

wtk_cmask_pse2_cfg_t *wtk_cmask_pse2_cfg_new(char *fn) {
    wtk_main_cfg_t *main_cfg;
    wtk_cmask_pse2_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(wtk_cmask_pse2_cfg, fn);
    if (!main_cfg) {
        return NULL;
    }
    cfg = (wtk_cmask_pse2_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void wtk_cmask_pse2_cfg_delete(wtk_cmask_pse2_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_cmask_pse2_cfg_t *wtk_cmask_pse2_cfg_new_bin(char *fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_cmask_pse2_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(wtk_cmask_pse2_cfg, fn, "./cfg");
    if (!mbin_cfg) {
        return NULL;
    }
    cfg = (wtk_cmask_pse2_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void wtk_cmask_pse2_cfg_delete_bin(wtk_cmask_pse2_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
