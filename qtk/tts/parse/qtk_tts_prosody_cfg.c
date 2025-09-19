#include "qtk_tts_prosody_cfg.h"

int qtk_tts_prosody_cfg_init(qtk_tts_prosody_cfg_t *cfg)
{
    memset(cfg,0,sizeof(*cfg));
    cfg->use_start_sil = 0;
    cfg->use_end_sil = 1;
    cfg->use_tail_sym = 1;
    cfg->split_sym = "_ ";
    cfg->use_segwrd_sym = 0;
    qtk_tts_ncrf_cfg_init(&cfg->ncrf);
    return 0;
}

int qtk_tts_prosody_cfg_clean(qtk_tts_prosody_cfg_t *cfg)
{
    if(cfg->dict)
        wtk_kdict_delete(cfg->dict);
    if(cfg->use_ncrf){
        qtk_tts_ncrf_cfg_clean(&cfg->ncrf);
    }
    return 0;
}

int qtk_tts_prosody_cfg_update_local(qtk_tts_prosody_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v = NULL;
    wtk_local_cfg_t *main_cfg = lc;

    wtk_local_cfg_update_cfg_b(main_cfg,cfg,use_ncrf,v);
    wtk_local_cfg_update_cfg_b(main_cfg,cfg,use_start_sil,v);
    wtk_local_cfg_update_cfg_b(main_cfg,cfg,use_end_sil,v);
    wtk_local_cfg_update_cfg_b(main_cfg,cfg,use_tail_sym,v);
    wtk_local_cfg_update_cfg_b(main_cfg,cfg,use_segwrd_sym,v);
    cfg->symbols2=wtk_local_cfg_find_int_array_s(main_cfg,"symbols2");
    cfg->symbols3=wtk_local_cfg_find_int_array_s(main_cfg,"symbols3");
    cfg->symbols4=wtk_local_cfg_find_int_array_s(main_cfg,"symbols4");
    cfg->symbols5=wtk_local_cfg_find_int_array_s(main_cfg,"symbols5");
    cfg->filter = wtk_local_cfg_find_array_s(main_cfg,"filter");
    lc = wtk_local_cfg_find_lc_s(main_cfg,"ncrf");
    if(lc && cfg->use_ncrf){
        qtk_tts_ncrf_cfg_update_local(&cfg->ncrf,lc);
    }
    wtk_local_cfg_update_cfg_str(main_cfg,cfg,dict_fn,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,dict_hint,v);
    wtk_local_cfg_update_cfg_str(main_cfg,cfg, split_sym,v);

    return 0;
}

int qtk_tts_prosody_cfg_update(qtk_tts_prosody_cfg_t *cfg)
{
    if(cfg->dict_fn){
        cfg->dict = wtk_kdict_new(NULL,cfg->dict_hint,NULL,NULL);
        wtk_kdict_load_file(cfg->dict,cfg->dict_fn);
    }
    if(cfg->use_ncrf){
        qtk_tts_ncrf_cfg_update(&cfg->ncrf);
    }
    return 0;
}

int qtk_tts_prosody_cfg_update2(qtk_tts_prosody_cfg_t *cfg, wtk_source_loader_t *sl)
{
	int ret=0;

    if(cfg->dict_fn){
        cfg->dict = wtk_kdict_new(NULL,cfg->dict_hint,NULL,NULL);
		ret=wtk_source_loader_load(sl,cfg->dict,(wtk_source_load_handler_t)wtk_kdict_load,cfg->dict_fn);
		if(ret!=0){goto end;}
    }
    if(cfg->use_ncrf){
        ret=qtk_tts_ncrf_cfg_update(&cfg->ncrf);
        if(ret!=0){goto end;}
    }

end:
    return ret;
}
