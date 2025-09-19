#include "wtk_lmgen2_cfg.h" 

int wtk_lmgen2_cfg_init(wtk_lmgen2_cfg_t *cfg)
{
	wtk_lm_dict_cfg_init(&(cfg->dict));
	wtk_prune_cfg_init(&(cfg->prune));
	wtk_prune_cfg_init(&(cfg->predict_prune));
	cfg->depth=2;
	cfg->skip_pen=0;
	cfg->bow_pen=0;
	cfg->step_ppl_thresh=50;
	cfg->nbest=10;
	cfg->step_ppl_thresh2=20;
	cfg->first_ppl_thresh=250;
	cfg->predict_nwrd=8;
	cfg->predict_nbest=10;
	cfg->predict_step_prob=-3.0;
	return 0;
}

int wtk_lmgen2_cfg_clean(wtk_lmgen2_cfg_t *cfg)
{
	wtk_lm_dict_cfg_clean(&(cfg->dict));
	wtk_prune_cfg_clean(&(cfg->prune));
	wtk_prune_cfg_clean(&(cfg->predict_prune));
	return 0;
}

int wtk_lmgen2_cfg_update_local(wtk_lmgen2_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_i(lc,cfg,depth,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,step_ppl_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,step_ppl_thresh2,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nbest,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,first_ppl_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,skip_pen,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,bow_pen,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,predict_nwrd,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,predict_nbest,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,predict_step_prob,v);
	lc=wtk_local_cfg_find_lc_s(main,"dict");
	if(lc)
	{
		ret=wtk_lm_dict_cfg_update_local(&(cfg->dict),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"lm");
	if(lc)
	{
		ret=wtk_nglm_cfg_update_local(&(cfg->lm),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"prune");
	if(lc)
	{
		ret=wtk_prune_cfg_update_local(&(cfg->prune),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"predict_prune");
	if(lc)
	{
		ret=wtk_prune_cfg_update_local(&(cfg->predict_prune),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_lmgen2_cfg_update(wtk_lmgen2_cfg_t *cfg)
{
	int ret;

	ret=wtk_prune_cfg_update(&(cfg->predict_prune));
	if(ret!=0){goto end;}
	ret=wtk_prune_cfg_update(&(cfg->prune));
	if(ret!=0){goto end;}
	ret=wtk_nglm_cfg_update(&(cfg->lm));
	if(ret!=0){goto end;}
	ret=wtk_lm_dict_cfg_update(&(cfg->dict));
	if(ret!=0){goto end;}
end:
	return ret;
}

int wtk_lmgen2_cfg_update2(wtk_lmgen2_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	ret=wtk_prune_cfg_update(&(cfg->predict_prune));
	if(ret!=0){goto end;}
	ret=wtk_prune_cfg_update(&(cfg->prune));
	if(ret!=0){goto end;}
	ret=wtk_nglm_cfg_update(&(cfg->lm));
	if(ret!=0){goto end;}
	ret=wtk_lm_dict_cfg_update2(&(cfg->dict),sl);
	if(ret!=0){goto end;}
end:
	cfg->lm.rbin=(wtk_rbin2_t*)(sl->hook);
	return ret;
}
