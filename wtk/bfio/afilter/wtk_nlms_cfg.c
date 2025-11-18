#include "wtk_nlms_cfg.h" 

int wtk_nlms_cfg_init(wtk_nlms_cfg_t *cfg)
{
	cfg->channel=1;
    cfg->M=4;
	cfg->N=1;
    cfg->coh_alpha=0.93;
    cfg->x_alpha=0.1;

	cfg->max_u=0.4;
	cfg->min_u=0.1;

    cfg->leak_scale=1.0;
    cfg->orth_m=1.0;
    cfg->orth_s=0.5;
    cfg->orth2_m=1.0;
    cfg->orth2_s=0.1;
	cfg->use_en_step=0;
	cfg->use_sec_iter=0;

	cfg->power_l_scale=-1;

	return 0;
}

int wtk_nlms_cfg_clean(wtk_nlms_cfg_t *cfg)
{
	return 0;
}

int wtk_nlms_cfg_update_local(wtk_nlms_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;

	lc=main;
	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,coh_alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,x_alpha,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,M,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,max_u,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_u,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,leak_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,orth_m,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,orth_s,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,orth2_m,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,orth2_s,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_en_step,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_sec_iter,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,power_l_scale,v);

	return 0;
}

int wtk_nlms_cfg_update(wtk_nlms_cfg_t *cfg)
{
	cfg->L=cfg->M*cfg->N;
	return 0;
}
