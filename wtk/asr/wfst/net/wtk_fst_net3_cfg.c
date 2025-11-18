#include "wtk_fst_net3_cfg.h"

int wtk_fst_net3_cfg_init(wtk_fst_net3_cfg_t *cfg)
{
	cfg->node_cache=10240;
	cfg->trans_cache=10240;
	cfg->inst_cache=10240;
	cfg->prune_thresh=0.9;
	cfg->prune_frames=100;
	cfg->prune_beam=0.0;
	cfg->prune_end_beam=0.0;
	cfg->use_wrd_align=0;
	cfg->use_wrd_cmp=1;
	cfg->depth=0;
	cfg->use_hg=0;
	cfg->use_fix_nodes=0;
	cfg->hg_fix_nodes=0;
	cfg->hg_nodes_per_frame=20;
	cfg->hg_min_node_thresh=100;
	cfg->hg_nodes_per_frame_b=0;
	cfg->node_prune_thresh=-1;
	cfg->lm_scale=0;
	cfg->use_hot_word=0;
	wtk_prune_cfg_init(&(cfg->hg));
	return 0;
}

int wtk_fst_net3_cfg_clean(wtk_fst_net3_cfg_t *cfg)
{
	wtk_prune_cfg_clean(&(cfg->hg));
	return 0;
}

int wtk_fst_net3_cfg_update_local(wtk_fst_net3_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;

	lc=main;
	wtk_local_cfg_update_cfg_i(lc,cfg,node_cache,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,trans_cache,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,prune_frames,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,prune_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,prune_beam,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,prune_end_beam,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,inst_cache,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_wrd_align,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_wrd_cmp,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_hg,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fix_nodes,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_hot_word,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,hg_fix_nodes,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,hg_nodes_per_frame,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,hg_min_node_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,hg_nodes_per_frame_b,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,node_prune_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,depth,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,lm_scale,v);
	lc=wtk_local_cfg_find_lc_s(main,"hg");
	if(lc)
	{
		wtk_prune_cfg_update_local(&(cfg->hg),lc);
	}
	return 0;
}

int wtk_fst_net3_cfg_update(wtk_fst_net3_cfg_t *cfg)
{
	wtk_prune_cfg_update(&(cfg->hg));
	return 0;
}
