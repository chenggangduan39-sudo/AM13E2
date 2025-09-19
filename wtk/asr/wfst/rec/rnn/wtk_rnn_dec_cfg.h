#ifndef WTK_FST_REC_REL_WTK_RNN_DEC_CFG
#define WTK_FST_REC_REL_WTK_RNN_DEC_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk_hs_tree.h"
#include "wtk_rnn_dec_syn.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rnn_dec_cfg wtk_rnn_dec_cfg_t;
struct wtk_rnn_dec_cfg
{
	wtk_rbin2_t *rbin;
	wtk_hs_tree_t *tree;
	wtk_rnn_dec_syn_t *syn;
	float max_hid_value;
	char *tree_fn;
	char *model_fn;
	unsigned use_tree_bin:1;
	unsigned use_fix:1;
};

int wtk_rnn_dec_cfg_init(wtk_rnn_dec_cfg_t *cfg);
int wtk_rnn_dec_cfg_clean(wtk_rnn_dec_cfg_t *cfg);
int wtk_rnn_dec_cfg_update_local(wtk_rnn_dec_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_rnn_dec_cfg_update(wtk_rnn_dec_cfg_t *cfg);
int wtk_rnn_dec_cfg_update2(wtk_rnn_dec_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
