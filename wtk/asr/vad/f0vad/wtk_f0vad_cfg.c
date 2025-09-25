#include "wtk_f0vad_cfg.h" 

int wtk_f0vad_cfg_init(wtk_f0vad_cfg_t *cfg)
{
	wtk_f0_cfg_init(&(cfg->f0));
	return 0;
}

int wtk_f0vad_cfg_clean(wtk_f0vad_cfg_t *cfg)
{
	wtk_f0_cfg_clean(&(cfg->f0));
	return 0;
}

int wtk_f0vad_cfg_update_local(wtk_f0vad_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;

	lc=wtk_local_cfg_find_lc_s(main,"f0");
	if(lc)
	{
		wtk_f0_cfg_update_local(&(cfg->f0),lc);
	}
	return 0;
}

int wtk_f0vad_cfg_update(wtk_f0vad_cfg_t *cfg)
{
	int ret;

	ret=wtk_f0_cfg_update(&(cfg->f0));
	if(ret!=0){goto end;}
	ret=0;
end:
	return ret;
}
