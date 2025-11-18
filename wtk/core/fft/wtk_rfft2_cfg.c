#include "wtk_rfft2_cfg.h" 

int wtk_rfft2_cfg_init(wtk_rfft2_cfg_t *cfg)
{
	cfg->winf=NULL;
	cfg->wint="hann";
	cfg->win=256;
	cfg->overlap=0.5;
	return 0;
}

int wtk_rfft2_cfg_clean(wtk_rfft2_cfg_t *cfg)
{
	if(cfg->winf)
	{
		wtk_free(cfg->winf);
	}
	return 0;
}

int wtk_rfft2_cfg_update_local(wtk_rfft2_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,wint,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,win,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,overlap,v);
	return 0;
}

int wtk_rfft2_cfg_update(wtk_rfft2_cfg_t *cfg)
{
	cfg->winf=wtk_math_create_win(cfg->wint,cfg->win);
	cfg->step=cfg->win*(1-cfg->overlap);
	return 0;
}
