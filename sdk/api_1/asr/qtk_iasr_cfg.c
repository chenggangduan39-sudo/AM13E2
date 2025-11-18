#include "qtk_iasr_cfg.h"

int qtk_iasr_cfg_init(qtk_iasr_cfg_t *cfg) {
    qtk_cldebnf_cfg_init(&cfg->cldebnf);
    qtk_hw_cfg_init(&cfg->hw);
    cfg->use_cldebnf = 0;

    cfg->skip_comm = 0;

    cfg->type = QTK_IASR_SPX;

    wtk_string_set(&cfg->name, 0, 0);
    wtk_string_set(&cfg->dec_fn, 0, 0);
    wtk_string_set(&cfg->compile_fn, 0, 0);
    wtk_string_set(&cfg->usr_ebnf, 0, 0);
    wtk_string_set(&cfg->usr_xbnf, 0, 0);
    wtk_string_set(&cfg->keyword_fn, 0, 0);

    cfg->cfg.mcfg = NULL;
    cfg->eg_mcfg = NULL;
    cfg->eg_mbcfg = NULL;
    cfg->rbin = NULL;
    cfg->use_bin = 1;

    cfg->src_rate = 16000;
    cfg->dst_rate = 8000;
    cfg->resample_cache = 1024;
    cfg->use_resample = 0;

    cfg->use_hint = 0;
    cfg->use_lex = 1;
    cfg->use_punc = 1;
    cfg->max_hint=4;
    cfg->use_mode=1;
    cfg->idle_time=10;

    cfg->priority = 1;
    cfg->wait = 0;
    cfg->use_hw_upload = 0;
    cfg->use_hotword = 0;
    cfg->use_vad = 0;
    cfg->use_xbnf = 0;
    cfg->save_spx_wav = 0;
    cfg->use_timestamp = 0;
    cfg->use_general_asr = 0;
    return 0;
}

int qtk_iasr_cfg_clean(qtk_iasr_cfg_t *cfg) {
    switch (cfg->type) {
    case QTK_IASR_SPX:
        cfg->use_bin ? qtk_spx_cfg_delete_bin(cfg->cfg.scfg)
                     : qtk_spx_cfg_delete(cfg->cfg.scfg);
        if (cfg->use_cldebnf) {
            qtk_cldebnf_cfg_clean(&cfg->cldebnf);
        }
        break;

    case QTK_IASR_LC:
        if (cfg->use_bin) {
            qtk_decoder_wrapper_cfg_delete_bin(cfg->cfg.dwcfg);
        } else {
            qtk_decoder_wrapper_cfg_delete(cfg->cfg.mcfg);
        }
        break;

    case QTK_IASR_GR:
        if (cfg->use_bin) {
            qtk_decoder_wrapper_cfg_delete_bin(cfg->cfg.dwcfg);
        } else {
            qtk_decoder_wrapper_cfg_delete(cfg->cfg.mcfg);
        }
        if (cfg->eg_mcfg) {
            wtk_main_cfg_delete(cfg->eg_mcfg);
        }
        if (cfg->eg_mbcfg) {
            wtk_mbin_cfg_delete(cfg->eg_mbcfg);
        }
        break;
    case QTK_IASR_GR_NEW:
    	if (cfg->use_bin){
            qtk_asr_wrapper_cfg_delete_bin(cfg->cfg.awcfg);
    	}else {
    		// wtk_main_cfg_delete(cfg->cfg.mcfg);
            qtk_asr_wrapper_cfg_delete(cfg->cfg.mcfg);
    	}
    	break;

    default:
        break;
    }

    return 0;
}
//第一步 从配置文件中更新键值对
int qtk_iasr_cfg_update_local(qtk_iasr_cfg_t *cfg, wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    lc = main;
    wtk_local_cfg_update_cfg_string_v(lc, cfg, name, v);
    wtk_local_cfg_update_cfg_string_v(lc, cfg, dec_fn, v);
    wtk_local_cfg_update_cfg_string_v(lc, cfg, compile_fn, v);
    wtk_local_cfg_update_cfg_string_v(lc, cfg, usr_ebnf, v);
    wtk_local_cfg_update_cfg_string_v(lc, cfg, usr_xbnf, v);
    wtk_local_cfg_update_cfg_string_v(lc, cfg, keyword_fn, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, type, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, src_rate, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, dst_rate, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, resample_cache, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, priority, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, max_hint, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, use_mode, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, idle_time, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_bin, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_resample, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, wait, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_cldebnf, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, skip_comm, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_hint, v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_lex,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_punc,v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_hw_upload, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_hotword, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_vad, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_xbnf, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_timestamp, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, save_spx_wav, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_general_asr, v);
    if (cfg->type == QTK_IASR_SPX && cfg->use_cldebnf) {
        lc = wtk_local_cfg_find_lc_s(main, "cldebnf");
        if (lc) {
            qtk_cldebnf_cfg_update_local(&cfg->cldebnf, lc);
        }
    }
    if (cfg->type == QTK_IASR_SPX && cfg->use_hw_upload) {
        lc = wtk_local_cfg_find_lc_s(main, "hw");
        if (lc) {
            qtk_hw_cfg_update_local(&cfg->hw, lc);
        }
    }

    return 0;
}

int qtk_iasr_cfg_update(qtk_iasr_cfg_t *cfg) {

    switch (cfg->type) {
    case QTK_IASR_SPX:
        cfg->cfg.scfg = cfg->use_bin ? qtk_spx_cfg_new_bin(cfg->dec_fn.data, 0)
                                     : qtk_spx_cfg_new(cfg->dec_fn.data);
        if (cfg->use_cldebnf) {
            qtk_cldebnf_cfg_update(&cfg->cldebnf);
        }
        if (cfg->use_hw_upload) {
            qtk_hw_cfg_update(&cfg->hw);
        }
        break;

    case QTK_IASR_LC:
        wtk_debug(">>>>>%s\n", cfg->dec_fn.data);
        if (cfg->use_bin) {
            cfg->cfg.dwcfg = qtk_decoder_wrapper_cfg_new_bin(cfg->dec_fn.data);
            cfg->cfg.dwcfg->use_xbnf = cfg->use_xbnf;
        } else {
            cfg->cfg.mcfg = qtk_decoder_wrapper_cfg_new(cfg->dec_fn.data);
            qtk_decoder_wrapper_cfg_t *tmp_cfg =
                (qtk_decoder_wrapper_cfg_t *)cfg->cfg.mcfg;
            tmp_cfg->use_xbnf = cfg->use_xbnf;
        }
        break;

    case QTK_IASR_GR:
        wtk_debug(">>>>>%s  use_bin = %d\n", cfg->dec_fn.data, cfg->use_bin);
        if (cfg->use_bin) {
            cfg->cfg.dwcfg = qtk_decoder_wrapper_cfg_new_bin(cfg->dec_fn.data);
            cfg->cfg.dwcfg->use_xbnf = cfg->use_xbnf;
            cfg->eg_mbcfg = wtk_mbin_cfg_new_type(
                wtk_egram_cfg, cfg->compile_fn.data, "./main.cfg");
        } else {
            cfg->cfg.mcfg = qtk_decoder_wrapper_cfg_new(cfg->dec_fn.data);
            if (!cfg->cfg.mcfg) {
                wtk_debug(">>>>>decoder_wrapper cfg new failed.\n");
            }
            qtk_decoder_wrapper_cfg_t *tmp_cfg =
                (qtk_decoder_wrapper_cfg_t *)cfg->cfg.mcfg;
            tmp_cfg->use_xbnf = cfg->use_xbnf;
            cfg->eg_mcfg =
                wtk_main_cfg_new_type(wtk_egram_cfg, cfg->compile_fn.data);
        }
        break;
    case QTK_IASR_GR_NEW:
    	if (cfg->use_bin) {
    		cfg->cfg.awcfg = qtk_asr_wrapper_cfg_new_bin(cfg->dec_fn.data);
    	}else {
            cfg->cfg.mcfg = qtk_asr_wrapper_cfg_new(cfg->dec_fn.data);
            cfg->cfg.awcfg = (qtk_asr_wrapper_cfg_t *)(cfg->cfg.mcfg->cfg);
            if (!cfg->cfg.mcfg) {
                wtk_debug(">>>>>decoder_wrapper cfg new failed.\n");
            }
    	}
    	break;

    default:
        break;
    }

    return 0;
}

int qtk_iasr_cfg_update_spx(qtk_iasr_cfg_t *cfg, wtk_source_t *s) {
    int ret = 0;
    wtk_rbin2_item_t *item;

    item = (wtk_rbin2_item_t *)s->data;
    cfg->cfg.scfg = qtk_spx_cfg_new_bin(item->rb->fn, item->pos);
    if (!cfg->cfg.scfg) {
        ret = -1;
    }

    return ret;
}

int qtk_iasr_cfg_update_wrapper(qtk_iasr_cfg_t *cfg, wtk_source_t *s) {
    int ret = 0;
    wtk_rbin2_item_t *item;

    item = (wtk_rbin2_item_t *)s->data;
    //	wtk_debug("item fn = %s\n",item->rb->fn);
    cfg->cfg.dwcfg = qtk_decoder_wrapper_cfg_new_bin2(item->rb->fn, item->pos);
    if (!cfg->cfg.dwcfg) {
        ret = -1;
    }

    return ret;
}

int qtk_iasr_cfg_update_new(qtk_iasr_cfg_t *cfg, wtk_source_t *s) {
    int ret = 0;
    wtk_rbin2_item_t *item;

    item = (wtk_rbin2_item_t *)s->data;
    //	wtk_debug("item fn = %s\n",item->rb->fn);
    cfg->cfg.awcfg = qtk_asr_wrapper_cfg_new_bin2(item->rb->fn, item->pos);
    if (!cfg->cfg.awcfg) {
        ret = -1;
    }

    return ret;
}
int qtk_iasr_cfg_update2(qtk_iasr_cfg_t *cfg, wtk_source_loader_t *sl) {
    int ret;

    switch (cfg->type) {
    case QTK_IASR_SPX:
        ret = wtk_source_loader_load(
            sl, cfg, (wtk_source_load_handler_t)qtk_iasr_cfg_update_spx,
            cfg->dec_fn.data);
        if (ret != 0) {
            goto end;
        }
        if (cfg->use_cldebnf) {
            qtk_cldebnf_cfg_update(&cfg->cldebnf);
        }
        break;

    case QTK_IASR_LC:
        ret = wtk_source_loader_load(
            sl, cfg, (wtk_source_load_handler_t)qtk_iasr_cfg_update_wrapper,
            cfg->dec_fn.data);
        if (ret != 0) {
            goto end;
        }
        break;

    case QTK_IASR_GR:
        ret = wtk_source_loader_load(
            sl, cfg, (wtk_source_load_handler_t)qtk_iasr_cfg_update_wrapper,
            cfg->dec_fn.data);
        if (ret != 0) {
            goto end;
        }
        break;
    case QTK_IASR_GR_NEW:
        ret = wtk_source_loader_load(
            sl, cfg, (wtk_source_load_handler_t)qtk_iasr_cfg_update_new,
            cfg->dec_fn.data);
        if (ret != 0) {
            goto end;
        }
    	break;

    default:
        break;
    }

    ret = 0;
end:
    return ret;
}
//第二步，再从engine_new的params参数中更新键值对
void qtk_iasr_cfg_update_params(qtk_iasr_cfg_t *cfg, wtk_local_cfg_t *params) {
    wtk_string_t *v;

    // wtk_debug(">>>>>>>>>>>>>>update_params  type = %d\n", cfg->type);
    if (cfg->type == QTK_IASR_GR) {
        v = wtk_local_cfg_find_string_s(params, "usr_ebnf");
        if (v) {
            cfg->usr_ebnf = *v;
            cfg->rbin = NULL;
        }
    }
    if (cfg->type != QTK_IASR_SPX) {
        v = wtk_local_cfg_find_string_s(params, "usr_xbnf");
        if (v) {
            // wtk_debug("****%.*s\n", v->len, v->data);
            cfg->usr_xbnf = *v;
        }
    }
    if(cfg->type == QTK_IASR_GR_NEW)
    {
        v = wtk_local_cfg_find_string_s(params, "keyword_fn");
        if (v) {
            cfg->keyword_fn = *v;
        } 
    }
    if (cfg->type == QTK_IASR_SPX) {
        v = wtk_local_cfg_find_string_s(params, "use_hotword");
        if (v) {
            cfg->use_hotword = atoi(v->data);
        }
        v = wtk_local_cfg_find_string_s(params, "use_hw_upload");
        if (v) {
            cfg->use_hw_upload = atoi(v->data);
        }
        v = wtk_local_cfg_find_string_s(params, "use_hint");
        if (v) {
            cfg->use_hint = atoi(v->data);
        }
        v = wtk_local_cfg_find_string_s(params, "use_lex");
		if(v) {
			cfg->use_lex = atoi(v->data);
		}
		v = wtk_local_cfg_find_string_s(params, "use_punc");
		if(v) {
			cfg->use_punc = atoi(v->data);
		}
        v = wtk_local_cfg_find_string_s(params, "max_hint");
		if(v) {
			cfg->max_hint = atoi(v->data);
		}
        v = wtk_local_cfg_find_string_s(params, "use_vad");
        if (v) {
            cfg->use_vad = atoi(v->data);
        }
        v = wtk_local_cfg_find_string_s(params, "save_spx_wav");
        if (v) {
            cfg->save_spx_wav = atoi(v->data);
        }
        qtk_spx_cfg_update_params(cfg->cfg.scfg, params);
    }
}

void qtk_iasr_cfg_update_option(qtk_iasr_cfg_t *cfg, qtk_option_t *opt) {
    if (cfg->type == QTK_IASR_SPX) {
        qtk_spx_cfg_update_option(cfg->cfg.scfg, opt);
    }
}
