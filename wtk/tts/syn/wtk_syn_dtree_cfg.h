#ifndef WTK_TTS_SYN_WTK_SYN_DTREE_CFG
#define WTK_TTS_SYN_WTK_SYN_DTREE_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_syn_dtree_cfg wtk_syn_dtree_cfg_t;
struct wtk_syn_dtree_cfg
{
	char *dur_fn;
	char *lf0_fn;
	char *mcp_fn;
	char *bap_fn;
	char *lf0_gv_fn;
	char *mcp_gv_fn;
	char *bap_gv_fn;

	int dur_node_cnt;
	int lf0_ndoe_cnt;
	int mcp_node_cnt;
	int bap_node_cnt;
	int lf0_gv_node_cnt;
	int mcp_gv_node_cnt;
	int bap_gv_node_cnt;

	int dur_hash_hint;
	int lf0_hash_hint;
	int mcp_hash_hint;
	int bap_hash_hint;
	int lf0_gv_hash_hint;
	int mcp_gv_hash_hint;
	int bap_gv_hash_hint;

	unsigned use_bin:1;
};

int wtk_syn_dtree_cfg_init(wtk_syn_dtree_cfg_t *cfg);
int wtk_syn_dtree_cfg_clean(wtk_syn_dtree_cfg_t *cfg);
int wtk_syn_dtree_cfg_update_local(wtk_syn_dtree_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_syn_dtree_cfg_update(wtk_syn_dtree_cfg_t *cfg);
int wtk_syn_dtree_cfg_update2(wtk_syn_dtree_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
