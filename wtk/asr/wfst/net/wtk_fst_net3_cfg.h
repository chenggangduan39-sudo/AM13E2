#ifndef WTK_FST_NET3_WTK_FST_NET3_CFG_H_
#define WTK_FST_NET3_WTK_FST_NET3_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/segmenter/wtk_prune.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fst_net3_cfg wtk_fst_net3_cfg_t;
struct wtk_fst_net3_cfg
{
	wtk_prune_cfg_t hg;
	int node_cache;
	int trans_cache;
	int inst_cache;
	int prune_frames;
	float hg_nodes_per_frame;
	int hg_nodes_per_frame_b;
	int hg_fix_nodes;
	int hg_min_node_thresh;
	int node_prune_thresh;
	int depth;
	float prune_thresh;
	float prune_beam;
	float prune_end_beam;
	float lm_scale;
	unsigned use_wrd_align:1;
	unsigned use_wrd_cmp:1;
	unsigned use_hg:1;
	unsigned use_fix_nodes:1;
	unsigned use_hot_word:1;
};

int wtk_fst_net3_cfg_init(wtk_fst_net3_cfg_t *cfg);
int wtk_fst_net3_cfg_clean(wtk_fst_net3_cfg_t *cfg);
int wtk_fst_net3_cfg_update_local(wtk_fst_net3_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_fst_net3_cfg_update(wtk_fst_net3_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
