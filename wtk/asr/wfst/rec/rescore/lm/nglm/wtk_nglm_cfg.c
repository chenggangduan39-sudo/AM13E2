#include "wtk_nglm_cfg.h"

int wtk_nglm_cfg_init(wtk_nglm_cfg_t *cfg)
{
	//wtk_string_set(&(cfg->name),0,0);
	cfg->lm_fn=NULL;
	cfg->max_order=4;
	cfg->debug=0;
	cfg->load_all=0;
	cfg->rbin=NULL;
	cfg->reset_max_bytes=-1;
	cfg->use_dynamic_reset=0;
	return 0;
}

int wtk_nglm_cfg_clean(wtk_nglm_cfg_t *cfg)
{
	return 0;
}

int wtk_nglm_cfg_update_local(wtk_nglm_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	//wtk_local_cfg_print(lc);
	wtk_local_cfg_update_cfg_str(lc,cfg,lm_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_order,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,load_all,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,reset_max_bytes,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dynamic_reset,v);
	//wtk_local_cfg_update_cfg_string_v(lc,cfg,name,v);
	return 0;
}

int wtk_nglm_cfg_update(wtk_nglm_cfg_t *cfg)
{
	cfg->reset_max_bytes*=1024*1024;
	return 0;
}

