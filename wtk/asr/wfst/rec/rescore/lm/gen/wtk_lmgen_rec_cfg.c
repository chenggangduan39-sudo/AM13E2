#include "wtk_lmgen_rec_cfg.h" 

int wtk_lmgen_rec_cfg_init(wtk_lmgen_rec_cfg_t *cfg)
{
	wtk_nglm_cfg_init(&(cfg->lm));
	cfg->ntok=5;
	cfg->step_ppl_beam=100;
	cfg->end_ppl_beam=50;
	cfg->hit_scale=1.0;
	cfg->hit_scale2=1.0;
	cfg->has_hit_scale=0.0;
	cfg->max_tok=100;
	cfg->max_depth=10;
	cfg->nbest=5;
	cfg->end_thresh=-1;
	cfg->stop_scale=0;
	return 0;
}

int wtk_lmgen_rec_cfg_clean(wtk_lmgen_rec_cfg_t *cfg)
{
	wtk_nglm_cfg_clean(&(cfg->lm));
	return 0;
}

int wtk_lmgen_rec_cfg_update_local(wtk_lmgen_rec_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_i(lc,cfg,ntok,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_tok,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_depth,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,step_ppl_beam,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,end_ppl_beam,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,hit_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,has_hit_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,hit_scale2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,stop_scale,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,end_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nbest,v);
	lc=wtk_local_cfg_find_lc_s(main,"lm");
	if(lc)
	{
		ret=wtk_nglm_cfg_update_local(&(cfg->lm),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_lmgen_rec_cfg_update(wtk_lmgen_rec_cfg_t *cfg)
{
	int ret;

	ret=wtk_nglm_cfg_update(&(cfg->lm));
	if(ret!=0){goto end;}
	ret=0;
end:
	return ret;
}
