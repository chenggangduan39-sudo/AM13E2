#include "wtk_rnn_dec_cfg.h" 

int wtk_rnn_dec_cfg_init(wtk_rnn_dec_cfg_t *cfg)
{
	cfg->rbin=NULL;
	cfg->max_hid_value=2048;
	cfg->tree=NULL;
	cfg->syn=NULL;
	cfg->tree_fn=NULL;
	cfg->model_fn=NULL;
	cfg->use_tree_bin=0;
	cfg->use_fix=0;
	return 0;
}

int wtk_rnn_dec_cfg_clean(wtk_rnn_dec_cfg_t *cfg)
{
	if(cfg->tree)
	{
		wtk_hs_tree_delete(cfg->tree);
	}
	if(cfg->syn)
	{
		wtk_rnn_dec_syn_delete(cfg->syn);
	}
	return 0;
}

int wtk_rnn_dec_cfg_update_local(wtk_rnn_dec_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_i(lc,cfg,max_hid_value,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_tree_bin,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fix,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,model_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,tree_fn,v);
	return 0;
}

int wtk_rnn_dec_cfg_update(wtk_rnn_dec_cfg_t *cfg)
{
	cfg->tree=wtk_hs_tree_new(cfg->tree_fn,cfg->use_tree_bin,NULL);
	cfg->syn=wtk_rnn_dec_syn_new(cfg->use_fix,cfg->model_fn,NULL);
	return 0;
}

int wtk_rnn_dec_cfg_update2(wtk_rnn_dec_cfg_t *cfg,wtk_source_loader_t *sl)
{
	cfg->tree=wtk_hs_tree_new(cfg->tree_fn,cfg->use_tree_bin,sl);
	cfg->syn=wtk_rnn_dec_syn_new(cfg->use_fix,cfg->model_fn,sl);
	return 0;
}
