#ifndef WTK_FST_REC_REL_WTK_WFST_KV_CFG
#define WTK_FST_REC_REL_WTK_WFST_KV_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/asr/wfst/rec/rescore/lm/nglm/wtk_nglm.h"
#include "wtk/asr/wfst/rec/rnn/wtk_rnn_dec.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wfst_kv_cfg wtk_wfst_kv_cfg_t;
struct wtk_wfst_kv_cfg
{
	wtk_nglm_cfg_t nglm;
	wtk_lm_dict_cfg_t dict;
	wtk_rnn_dec_cfg_t rnn;
	float oov_pen;
	int nslot;
	float rnn_scale;
	float ngram_scale;
	unsigned int use_rnn:1;
	unsigned int use_ngram:1;
};

int wtk_wfst_kv_cfg_init(wtk_wfst_kv_cfg_t *cfg);
int wtk_wfst_kv_cfg_clean(wtk_wfst_kv_cfg_t *cfg);
int wtk_wfst_kv_cfg_update_local(wtk_wfst_kv_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_wfst_kv_cfg_update(wtk_wfst_kv_cfg_t *cfg);
int wtk_wfst_kv_cfg_update2(wtk_wfst_kv_cfg_t *cfg,wtk_source_loader_t *sl);
void wtk_wfst_kv_cfg_set_rbin(wtk_wfst_kv_cfg_t *cfg,wtk_rbin2_t *rbin);
#ifdef __cplusplus
};
#endif
#endif
