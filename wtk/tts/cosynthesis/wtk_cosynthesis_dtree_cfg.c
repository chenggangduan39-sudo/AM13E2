#include "wtk_cosynthesis_dtree_cfg.h"

int wtk_cosynthesis_dtree_cfg_init(wtk_cosynthesis_dtree_cfg_t *cfg)
{
	cfg->dur_fn=NULL;
	cfg->lf0_fn=NULL;
	cfg->mcp_fn=NULL;
	cfg->bap_fn=NULL;
	cfg->lf0_gv_fn=NULL;
	cfg->mcp_gv_fn=NULL;
	cfg->bap_gv_fn=NULL;
	cfg->conca_lf0_fn = NULL;
	cfg->conca_mcp_fn = NULL;
	
	cfg->dur_node_cnt=1;
	cfg->lf0_ndoe_cnt=1;
	cfg->mcp_node_cnt=1;
	cfg->bap_node_cnt=1;
	cfg->lf0_gv_node_cnt=1;
	cfg->mcp_gv_node_cnt=1;
	cfg->bap_gv_node_cnt=1;

	cfg->dur_hash_hint=0;
	cfg->lf0_hash_hint=0;
	cfg->mcp_hash_hint=0;
	cfg->bap_hash_hint=0;
	cfg->lf0_gv_hash_hint=0;
	cfg->mcp_gv_hash_hint=0;
	cfg->bap_gv_hash_hint=0;

	cfg->use_bin=0;
	return 0;
}

int wtk_cosynthesis_dtree_cfg_clean(wtk_cosynthesis_dtree_cfg_t *cfg)
{
	return 0;
}

int wtk_cosynthesis_dtree_cfg_update_local(wtk_cosynthesis_dtree_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,dur_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,lf0_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,mcp_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,bap_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,lf0_gv_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,mcp_gv_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,bap_gv_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,conca_lf0_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,conca_mcp_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,dur_node_cnt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,lf0_ndoe_cnt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,mcp_node_cnt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bap_node_cnt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,lf0_gv_node_cnt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,mcp_gv_node_cnt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bap_gv_node_cnt,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,dur_hash_hint,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,lf0_hash_hint,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,mcp_hash_hint,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bap_hash_hint,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,lf0_gv_hash_hint,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,mcp_gv_hash_hint,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bap_gv_hash_hint,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_bin,v);
	//wtk_debug("%p:%d\n",cfg,cfg->dur_node_cnt);
	return 0;
}

int wtk_cosynthesis_dtree_cfg_update(wtk_cosynthesis_dtree_cfg_t *cfg)
{
	return 0;
}

int wtk_cosynthesis_dtree_cfg_update2(wtk_cosynthesis_dtree_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return 0;
}
