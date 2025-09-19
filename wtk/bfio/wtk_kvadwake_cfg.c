#include "wtk_kvadwake_cfg.h"

int wtk_kvadwake_cfg_init(wtk_kvadwake_cfg_t *cfg) {

    cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	wtk_vad_cfg_init(&(cfg->vad));
	wtk_kwake_cfg_init(&(cfg->kwake));
	wtk_kwdec2_cfg_init(&(cfg->kwdec2));
	wtk_wdec_cfg_init(&(cfg->wdec));
	qtk_img_cfg_init(&(cfg->img));
    qtk_img_thresh_cfg_init(&(cfg->img_mod));
    qtk_img_thresh_cfg_init(&(cfg->img_mod_echo));
    qtk_img_thresh_cfg_init(&(cfg->img_no_mod));
    qtk_img_thresh_cfg_init(&(cfg->img_no_mod_echo));
    qtk_decoder_wrapper_cfg_init(&cfg->decoder);

    cfg->wake_right_fe = 0.4;

    cfg->vad_left_margin = 20;
    cfg->vad_right_margin = 20;

    cfg->rate = 16000;
    cfg->use_vad = 1;
    cfg->use_ivad = 0;
    cfg->use_vad_start = 0;
    cfg->use_wdec = 0;
    cfg->use_img = 0;
    cfg->use_kdec = 0;
    cfg->use_kwake = 0;
    cfg->use_kwdec2 = 0;
    cfg->xwrd = NULL;
    cfg->debug = 1;
    cfg->max_vad_tms = 10000;

    return 0;
}

int wtk_kvadwake_cfg_clean(wtk_kvadwake_cfg_t *cfg) {

	wtk_vad_cfg_clean(&(cfg->vad));
	wtk_kwake_cfg_clean(&(cfg->kwake));
	wtk_kwdec2_cfg_clean(&(cfg->kwdec2));
	wtk_wdec_cfg_clean(&(cfg->wdec));
	qtk_img_cfg_clean(&(cfg->img));
	qtk_decoder_wrapper_cfg_clean(&cfg->decoder);

    if (cfg->xwrd) {
        wtk_strbuf_delete(cfg->xwrd);
    }

    return 0;
}

int wtk_kvadwake_cfg_update_local(wtk_kvadwake_cfg_t *cfg,
                                  wtk_local_cfg_t *lc) {
    wtk_string_t *v;
    wtk_local_cfg_t *m;
    int ret;

    wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, vad_left_margin, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, vad_right_margin, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, wake_right_fe, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_vad, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_ivad, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_vad_start, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_wdec, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_kwake, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_kwdec2, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_img, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_kdec, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, max_vad_tms, v);
    m = lc;

    if (cfg->use_vad) {
        lc = wtk_local_cfg_find_lc_s(m, "vad");
        if (lc) {
            wtk_vad_cfg_update_local(&(cfg->vad), lc);
        }
    }
    if (cfg->use_kwake) {
        lc = wtk_local_cfg_find_lc_s(m, "kwake");
        if (lc) {
            wtk_kwake_cfg_update_local(&(cfg->kwake), lc);
        }
    }
    if (cfg->use_kwdec2) {
        lc = wtk_local_cfg_find_lc_s(m, "kwdec2");
        if (lc) {
            wtk_kwdec2_cfg_update_local(&(cfg->kwdec2), lc);
        }
    }
    if (cfg->use_wdec) {
        cfg->xwrd = wtk_strbuf_new(32, 1);
        lc = wtk_local_cfg_find_lc_s(m, "wdec");
        if (lc) {
            wtk_wdec_cfg_update_local(&(cfg->wdec), lc);
        }
    }
    if (cfg->use_img) {
        lc = wtk_local_cfg_find_lc_s(m, "img");
        if (lc) {
            qtk_img_cfg_update_local(&(cfg->img), lc);
        }
        lc = wtk_local_cfg_find_lc_s(m, "img_mod");
        if (lc) {
            ret = qtk_img_thresh_cfg_update_local(&(cfg->img_mod), lc);
            if (ret != 0) {
                goto end;
            }
        }
        lc = wtk_local_cfg_find_lc_s(m, "img_mod_echo");
        if (lc) {
            ret = qtk_img_thresh_cfg_update_local(&(cfg->img_mod_echo), lc);
            if (ret != 0) {
                goto end;
            }
        }
        lc = wtk_local_cfg_find_lc_s(m, "img_no_mod");
        if (lc) {
            ret = qtk_img_thresh_cfg_update_local(&(cfg->img_no_mod), lc);
            if (ret != 0) {
                goto end;
            }
        }
        lc = wtk_local_cfg_find_lc_s(m, "img_no_mod_echo");
        if (lc) {
            ret = qtk_img_thresh_cfg_update_local(&(cfg->img_no_mod_echo), lc);
            if (ret != 0) {
                goto end;
            }
        }
    }
    if (cfg->use_kdec) {
        lc = wtk_local_cfg_find_lc_s(m, "kdec");
        if (lc) {
            qtk_decoder_wrapper_cfg_update_local(&cfg->decoder, lc);
        }
    }

    ret = 0;
end:
    return ret;
}

int wtk_kvadwake_cfg_update(wtk_kvadwake_cfg_t *cfg) {
    int ret;

    if (cfg->use_vad) {
        ret = wtk_vad_cfg_update(&(cfg->vad));
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_kwake) {
		ret = wtk_kwake_cfg_update(&(cfg->kwake));
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_kwdec2) {
		ret = wtk_kwdec2_cfg_update(&(cfg->kwdec2));
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_wdec) {
		ret = wtk_wdec_cfg_update(&(cfg->wdec));
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_img) {
		ret = qtk_img_cfg_update(&(cfg->img));
        if (ret != 0) {
            goto end;
        }
    }

    if (cfg->use_kdec) {
        qtk_decoder_wrapper_cfg_update(&cfg->decoder);
    }
    cfg->max_vad_len =cfg->max_vad_tms*16;

    ret = 0;
end:
    return ret;
}

int wtk_kvadwake_cfg_update2(wtk_kvadwake_cfg_t *cfg, wtk_source_loader_t *sl) {
    int ret;

    if (cfg->use_vad) {
        ret = wtk_vad_cfg_update2(&(cfg->vad), sl);
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_kwake) {
		ret = wtk_kwake_cfg_update2(&(cfg->kwake), sl);
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_kwdec2) {
		ret = wtk_kwdec2_cfg_update2(&(cfg->kwdec2), sl);
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_wdec) {
		ret = wtk_wdec_cfg_update2(&(cfg->wdec), sl);
        if (ret != 0) {
            goto end;
        }
    }
    if (cfg->use_img) {
		ret = qtk_img_cfg_update2(&(cfg->img), sl);
        if (ret != 0) {
            goto end;
        }
    }

    if (cfg->use_kdec) {
        qtk_decoder_wrapper_cfg_update2(&cfg->decoder, sl);
    }
    cfg->max_vad_len =cfg->max_vad_tms*16;

    ret = 0;
end:
    return ret;
}

int wtk_kvadwake_cfg_set_wakeword(wtk_kvadwake_cfg_t *cfg, char *wrd) {
    wtk_strbuf_t *buf = cfg->xwrd;
    int ret = -1;

    if (!cfg->use_wdec || !cfg->use_wdec || !wrd) {
        goto end;
    }
    wtk_strbuf_reset(buf);
    wtk_strbuf_push(buf, wrd, strlen(wrd));
    wtk_strbuf_push_c(buf, 0);
    wrd = buf->data;
    cfg->wdec.net.word = wrd;
    ret = 0;
end:
    return ret;
}

wtk_kvadwake_cfg_t* wtk_kvadwake_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_kvadwake_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_kvadwake_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_kvadwake_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_kvadwake_cfg_delete(wtk_kvadwake_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_kvadwake_cfg_t* wtk_kvadwake_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_kvadwake_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_kvadwake_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_kvadwake_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_kvadwake_cfg_delete_bin(wtk_kvadwake_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
