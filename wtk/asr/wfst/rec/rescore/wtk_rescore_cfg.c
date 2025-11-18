#include "wtk_rescore_cfg.h"

int wtk_rescore_cfg_init(wtk_rescore_cfg_t *cfg)
{
	wtk_lm_rescore_cfg_init(&(cfg->lm));
	cfg->nbest_max_search=15000;
	cfg->use_hist=0;
	return 0;
}

int wtk_rescore_cfg_clean(wtk_rescore_cfg_t *cfg)
{
	wtk_lm_rescore_cfg_set_sym_out(&(cfg->lm),NULL);
	wtk_lm_rescore_cfg_clean(&(cfg->lm));
	return 0;
}

int wtk_rescore_cfg_update_local(wtk_rescore_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	int ret;

	wtk_local_cfg_update_cfg_b(lc,cfg,use_hist,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nbest_max_search,v);
	ret=wtk_lm_rescore_cfg_update_local(&(cfg->lm),lc);
	if(ret!=0){goto end;}
	ret=0;
end:
	return 0;
}


int wtk_rescore_cfg_update(wtk_rescore_cfg_t *cfg)
{
	int ret;

	ret=wtk_lm_rescore_cfg_update(&(cfg->lm));
	if(ret!=0){goto end;}
	ret=0;
end:
	return ret;
}

int wtk_rescore_cfg_update2(wtk_rescore_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	ret=wtk_lm_rescore_cfg_update2(&(cfg->lm),sl);
	if(ret!=0){goto end;}
	ret=0;
end:
	return ret;
}

void wtk_rescore_set_sym_out(wtk_rescore_cfg_t *cfg,wtk_fst_sym_t *sym_out)
{
	wtk_lm_rescore_cfg_set_sym_out(&(cfg->lm),sym_out);
}
