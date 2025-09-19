#include "wtk_feat_cfg.h" 

int wtk_feat_cfg_init(wtk_feat_cfg_t *cfg)
{
	cfg->sig_size=13;
	cfg->dnn_size=0;
	cfg->use_dnn=0;
	return 0;
}

int wtk_feat_cfg_clean(wtk_feat_cfg_t *cfg)
{
	return 0;
}

int wtk_feat_cfg_update_local(wtk_feat_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_i(lc,cfg,sig_size,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,dnn_size,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dnn,v);
	return 0;
}

int wtk_feat_cfg_update(wtk_feat_cfg_t *cfg)
{
	return 0;
}
