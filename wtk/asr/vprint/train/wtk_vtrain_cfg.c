#include "wtk_vtrain_cfg.h" 

int wtk_vtrain_cfg_init(wtk_vtrain_cfg_t *cfg)
{
	cfg->mapTau=10;
	cfg->skip_frame=0;
	cfg->use_with_detect=0;
	return 0;
}

int wtk_vtrain_cfg_clean(wtk_vtrain_cfg_t *cfg)
{
	return 0;
}

int wtk_vtrain_cfg_update_local(wtk_vtrain_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_f(lc,cfg,mapTau,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,skip_frame,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_with_detect,v);
	return 0;
}

int wtk_vtrain_cfg_update(wtk_vtrain_cfg_t *cfg)
{
	return 0;
}

int wtk_vtrain_cfg_update2(wtk_vtrain_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return 0;
}
