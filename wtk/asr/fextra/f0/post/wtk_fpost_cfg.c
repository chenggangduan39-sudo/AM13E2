#include "wtk_fpost_cfg.h"

void wtk_splitf0_cfg_init(wtk_splitf0_cfg_t *cfg,int min_voice_dur,int pls_thresh,
		float energy_thresh,float energy_ratio,float unvoice_thresh,
		float glb_mean,float glb_var,float noise_var_ratio)
{
	cfg->min_voice_dur=min_voice_dur;
	cfg->pls_thresh=pls_thresh;
	cfg->energy_thresh=energy_thresh;
	cfg->energy_ratio=energy_ratio;
	cfg->unvoice_thresh=unvoice_thresh;
	cfg->glb_mean=glb_mean;
	cfg->glb_var=glb_var;
	cfg->energy_ratio=energy_ratio;
	cfg->noise_var_ratio=noise_var_ratio;
}

int wtk_splitf0_cfg_update_local(wtk_splitf0_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_i(lc,cfg,min_voice_dur,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pls_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,energy_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,energy_ratio,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,unvoice_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,glb_mean,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,glb_var,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,noise_var_ratio,v);
	return 0;

}

int wtk_fpost_cfg_init(wtk_fpost_cfg_t *cfg)
{
	wtk_splitf0_cfg_init(&(cfg->ctone),5,30,100,0.2,0,-8.931563e-04,4.415637e-02,0.1);
	wtk_splitf0_cfg_init(&(cfg->wtone),5,30,25,0.1,0,-1.631121e-03,7.200515e-02,0.1);
	return 0;
}

int wtk_fpost_cfg_clean(wtk_fpost_cfg_t *cfg)
{
	return 0;
}

int wtk_fpost_cfg_update_local(wtk_fpost_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	int ret=0;

	lc=wtk_local_cfg_find_lc_s(main,"ctone");
	if(lc)
	{
		ret=wtk_splitf0_cfg_update_local(&(cfg->ctone),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"wtone");
	if(lc)
	{
		ret=wtk_splitf0_cfg_update_local(&(cfg->wtone),lc);
		if(ret!=0){goto end;}
	}
end:
	return ret;
}

int wtk_fpost_cfg_update(wtk_fpost_cfg_t *cfg)
{
	return 0;
}
