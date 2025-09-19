#include "wtk_cmvn_cfg.h" 

int wtk_cmvn_cfg_clean(wtk_cmvn_cfg_t *cfg)
{
	return 0;
}

int wtk_cmvn_cfg_update(wtk_cmvn_cfg_t *cfg)
{
	cfg->fix_alpha=cfg->alpha*100;
	return 0;
}

int wtk_cmvn_cfg_init(wtk_cmvn_cfg_t *cfg)
{
	cfg->alpha=0.99;
	cfg->use_mean=1;
	cfg->use_var=0;
	cfg->use_online=0;
	cfg->use_hist=1;
        cfg->use_array = 0;
        cfg->left_frame=200;
	cfg->right_frame=20;
	cfg->init_frame=20;
	cfg->use_save_cmn=0;
	cfg->alpha_frame=100;
	cfg->use_sliding=0;
	return 0;
}

int wtk_cmvn_cfg_update_local(wtk_cmvn_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_b(lc,cfg,use_sliding,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_save_cmn,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,alpha,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,alpha_frame,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_mean,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_var,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_online,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_hist,v);
        wtk_local_cfg_update_cfg_b(lc, cfg, use_array, v);
        wtk_local_cfg_update_cfg_i(lc,cfg,init_frame,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,left_frame,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,right_frame,v);
	//wtk_debug("online=%d\n",cfg->use_online);
	return 0;
}
