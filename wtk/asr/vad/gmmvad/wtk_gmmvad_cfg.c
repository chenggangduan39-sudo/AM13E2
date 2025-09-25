#include "wtk_gmmvad_cfg.h" 

int wtk_gmmvad_cfg_init(wtk_gmmvad_cfg_t *cfg)
{
	cfg->frame_size=160;	//10ms
	cfg->mode=3;
	return 0;
}

int wtk_gmmvad_cfg_clean(wtk_gmmvad_cfg_t *cfg)
{
	return 0;
}

int wtk_gmmvad_cfg_update_local(wtk_gmmvad_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_i(lc,cfg,mode,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,frame_size,v);
	return 0;
}

int wtk_gmmvad_cfg_update(wtk_gmmvad_cfg_t *cfg)
{
	return 0;
}
