#include "wtk_mer_cfg.h"

wtk_mer_cfg_t* wtk_mer_cfg_new(char *cfg_fn, int is_bin, int seek_pos)
{
	wtk_cfg_macro_check();
    void *main_cfg;
    wtk_mer_cfg_t *mer_cfg=NULL;
	if (is_bin)
	{
		main_cfg = wtk_mbin_cfg_new_type2(seek_pos,wtk_mer_cfg,cfg_fn,"./cfg");
		if(!main_cfg){wtk_debug("加载配置文件失败!\n");goto end;}
		mer_cfg=(wtk_mer_cfg_t*)(((wtk_mbin_cfg_t*)main_cfg)->cfg);
		mer_cfg->bin_cfg=main_cfg;
	} else {
		main_cfg=wtk_main_cfg_new_type(wtk_mer_cfg, cfg_fn);
		if(!main_cfg){wtk_debug("加载配置文件失败!\n");goto end;}
		mer_cfg=(wtk_mer_cfg_t*)(((wtk_main_cfg_t*)main_cfg)->cfg);
		mer_cfg->main_cfg=main_cfg;
	}
end:
	return mer_cfg;
}
int wtk_mer_cfg_init(wtk_mer_cfg_t *cfg)
{
	cfg->main_cfg = NULL;
	cfg->bin_cfg = NULL;
	wtk_tts_parser_cfg_init(&(cfg->parser));
	wtk_mer_cfg_syn_init(&(cfg->syn));
	wtk_syn_cfg_init(&(cfg->syn_tts_cfg));
	return 0;
}
int wtk_mer_cfg_update_local(wtk_mer_cfg_t *cfg, wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	int ret;

	lc=main;
	lc=wtk_local_cfg_find_lc_s(main, "parser");
	if(lc)
	{
		ret=wtk_tts_parser_cfg_update_local(&(cfg->parser), lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main, "syn_tts");
	if(lc)
	{
		ret=wtk_syn_cfg_update_local(&(cfg->syn_tts_cfg), lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main, "syn");
	if(lc)
	{
		ret=wtk_mer_cfg_syn_update_local(&(cfg->syn), lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}
int wtk_mer_cfg_update(wtk_mer_cfg_t *cfg)
{
	wtk_source_loader_t sl;
	
	wtk_source_loader_init_file( &(sl));
	return wtk_mer_cfg_update2(cfg, &(sl));
	// return 0;
}
int wtk_mer_cfg_update2(wtk_mer_cfg_t *cfg, wtk_source_loader_t *sl)
{
	wtk_syn_cfg_t *tts_syn = &(cfg->syn_tts_cfg);
	wtk_strpool_t *pool;
	int ret;

	// wtk_rbin2_print(sl->hook);
	if(cfg->parser.segwrd.use_bin)
	{
		pool=wtk_strpool_new(1507);
	}else
	{
		pool=wtk_strpool_new(15007);
	}
	cfg->pool = pool;
	wtk_mer_cfg_syn_update2(&(cfg->syn), sl);
	ret=wtk_tts_parser_cfg_update3(&(cfg->parser), sl, cfg->pool);
	if(ret!=0){goto end;}
	// ret=wtk_syn_cfg_update3(tts_syn, sl, pool);
	ret=wtk_mer_cfg_tts_syn_update3(tts_syn, sl, pool);
	if(ret!=0){goto end;}
	ret=0;
end:
	return ret;
}
int wtk_mer_cfg_tts_syn_update3(wtk_syn_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool)
{
	int ret;
	if(!pool)
	{
		pool=wtk_strpool_new(2048);
	}
	ret=wtk_syn_win_cfg_update2(&(cfg->win),sl,pool);
	if(ret!=0){goto end;}
	// cfg->tree=wtk_syn_dtree_new(&(cfg->tree_cfg),sl,pool);
	cfg->tree=wtk_mer_syn_dtree_new(&(cfg->tree_cfg),sl,pool);
	if(!cfg->tree){goto end;}
	// cfg->hmm=wtk_syn_hmm_new(&(cfg->hmm_cfg),sl,pool,cfg->tree);
	cfg->hmm=wtk_mer_syn_hmm_new(&(cfg->hmm_cfg),sl,pool,cfg->tree);
	if(!cfg->hmm){goto end;}
	ret=0;
end:
	return ret;
}
int wtk_mer_cfg_clean(wtk_mer_cfg_t *cfg)
{
	wtk_tts_parser_cfg_clean(&(cfg->parser));
	wtk_mer_cfg_syn_clean(&(cfg->syn));
	wtk_syn_cfg_clean(&(cfg->syn_tts_cfg));
	if(cfg->pool)
	{
		wtk_strpool_delete(cfg->pool);
	}
	return 0;
}
void wtk_mer_cfg_delete(wtk_mer_cfg_t *cfg)
{
	if (cfg->bin_cfg)
	{ wtk_mbin_cfg_delete(cfg->bin_cfg); }
	else
	{ wtk_main_cfg_delete(cfg->main_cfg); }
}
