#include "qtk_tts_segwrd_cfg.h"

int qtk_tts_segwrd_cfg_init(qtk_tts_segwrd_cfg_t *cfg)
{
    memset(cfg,0,sizeof(qtk_tts_segwrd_cfg_t));
    cfg->wordmax = 6;
    cfg->wordmin = 2;
    cfg->scalar = 1.0f;
    return 0;
}

int qtk_tts_segwrd_cfg_clean(qtk_tts_segwrd_cfg_t *cfg)
{
    if(cfg->feature_idx_dict)
        wtk_kdict_delete(cfg->feature_idx_dict);
    if(cfg->unigram_dict)
        wtk_kdict_delete(cfg->unigram_dict);
    if(cfg->post_cut_dict)
        wtk_kdict_delete(cfg->post_cut_dict);
    return 0;
}

int qtk_tts_segwrd_cfg_update_local(qtk_tts_segwrd_cfg_t *cfg,wtk_local_cfg_t *main_cfg)
{
    wtk_string_t *v = NULL;
    cfg->norm_letter = wtk_local_cfg_find_array_s(main_cfg,"norm_letter");
    cfg->norm_num = wtk_local_cfg_find_array_s(main_cfg,"norm_num");
    wtk_local_cfg_update_cfg_str(main_cfg,cfg,feature_idx_fn,v);
    wtk_local_cfg_update_cfg_str(main_cfg,cfg,unigram_fn,v);
    wtk_local_cfg_update_cfg_str(main_cfg,cfg,modelw_fn,v);
    wtk_local_cfg_update_cfg_str(main_cfg,cfg,post_cut_fn,v);
    wtk_local_cfg_update_cfg_b(main_cfg,cfg,use_pre,v);
    wtk_local_cfg_update_cfg_b(main_cfg,cfg,use_post,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,feature_idx_hint,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,wordmax,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,wordmin,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,unigram_hint,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,n_feature,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,n_tag,v);
    wtk_local_cfg_update_cfg_f(main_cfg,cfg,scalar,v);
    wtk_local_cfg_update_cfg_f(main_cfg,cfg,post_cut_hint,v);
    return 0;
}

int qtk_tts_segwrd_cfg_update(qtk_tts_segwrd_cfg_t *cfg)
{
    if(cfg->feature_idx_fn){
        cfg->feature_idx_dict = wtk_kdict_new(NULL,cfg->feature_idx_hint,NULL,NULL);
        wtk_kdict_load_file(cfg->feature_idx_dict,cfg->feature_idx_fn);
    }
    if(cfg->unigram_fn){
        cfg->unigram_dict = wtk_kdict_new(NULL,cfg->unigram_hint,NULL,NULL);
        wtk_kdict_load_file(cfg->unigram_dict,cfg->unigram_fn);
    }
    if(cfg->post_cut_fn){
        cfg->post_cut_dict = wtk_kdict_new(NULL,cfg->post_cut_hint,NULL,NULL);
        wtk_kdict_load_file(cfg->post_cut_dict,cfg->post_cut_fn);
    }
    return 0;
}

int qtk_tts_segwrd_cfg_update2(qtk_tts_segwrd_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret=0;

    if(cfg->feature_idx_fn){
        cfg->feature_idx_dict = wtk_kdict_new(NULL,cfg->feature_idx_hint,NULL,NULL);
		ret=wtk_source_loader_load(sl,cfg->feature_idx_dict,(wtk_source_load_handler_t)wtk_kdict_load,cfg->feature_idx_fn);
		if(ret!=0){goto end;}
    }
    if(cfg->unigram_fn){
        cfg->unigram_dict = wtk_kdict_new(NULL,cfg->unigram_hint,NULL,NULL);
		ret=wtk_source_loader_load(sl,cfg->unigram_dict,(wtk_source_load_handler_t)wtk_kdict_load,cfg->unigram_fn);
		if(ret!=0){goto end;}
    }
    if(cfg->post_cut_fn){
        cfg->post_cut_dict = wtk_kdict_new(NULL,cfg->post_cut_hint,NULL,NULL);
		ret=wtk_source_loader_load(sl,cfg->post_cut_dict,(wtk_source_load_handler_t)wtk_kdict_load,cfg->post_cut_fn);
		if(ret!=0){goto end;}
    }

end:
    return ret;
}
