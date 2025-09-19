#include "wtk_polyphn_cfg.h" 

int wtk_polyphn_cfg_init(wtk_polyphn_cfg_t *cfg)
{
	cfg->lex_fn=NULL;
	cfg->polyphn=NULL;
	cfg->soft_wrds_end=NULL;
	cfg->use_soft_end=0;
	cfg->use_defpron=0;

	return 0;
}

int wtk_polyphn_cfg_clean(wtk_polyphn_cfg_t *cfg)
{
	if(cfg->polyphn)
	{
		wtk_polyphn_lex_delete(cfg->polyphn);
	}
	return 0;
}

int wtk_polyphn_cfg_update_local(wtk_polyphn_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,lex_fn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_soft_end,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_defpron,v);
	if (cfg->use_soft_end){
		cfg->soft_wrds_end=wtk_local_cfg_find_array_s(lc,"soft_wrds_end");
	}

	return 0;
}

int wtk_polyphn_cfg_update(wtk_polyphn_cfg_t *cfg)
{
	if(cfg->lex_fn)
	{
		cfg->polyphn=wtk_polyphn_lex_new();
		wtk_polyphn_lex_load(cfg->polyphn,cfg->lex_fn);
	}
	//wtk_debug("lex_fn=%s\n",cfg->lex_fn);
	return 0;
}

int wtk_polyphn_cfg_update2(wtk_polyphn_cfg_t *cfg,wtk_source_loader_t *sl)
{
	if(cfg->lex_fn)
	{
		cfg->polyphn=wtk_polyphn_lex_new();
		wtk_source_loader_load(sl,cfg->polyphn,(wtk_source_load_handler_t)wtk_polyphn_lex_load2,cfg->lex_fn);
	}
	//wtk_debug("lex_fn=%s\n",cfg->lex_fn);
	return 0;
}
