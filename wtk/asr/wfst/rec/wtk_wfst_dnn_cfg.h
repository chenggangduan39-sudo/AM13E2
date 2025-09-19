#ifndef WTK_VITE_REC_DNN_WTK_WFST_DNN_CFG_H_
#define WTK_VITE_REC_DNN_WTK_WFST_DNN_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/asr/model/wtk_hmmset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wfst_dnn_cfg wtk_wfst_dnn_cfg_t;

typedef struct
{
	wtk_string_t *name;
	int  index;
	float gconst;
}wtk_dnn_state_t;

struct wtk_wfst_dnn_cfg
{
	float conf_min;
	float conf_max;
	float conf_bias;
	float scale;
	char *state_fn;
	wtk_str_hash_t *hash;
	unsigned use_bin:1;
};

int wtk_wfst_dnn_cfg_init(wtk_wfst_dnn_cfg_t *cfg);
int wtk_wfst_dnn_cfg_clean(wtk_wfst_dnn_cfg_t *cfg);
int wtk_wfst_dnn_cfg_update_local(wtk_wfst_dnn_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_wfst_dnn_cfg_update(wtk_wfst_dnn_cfg_t *cfg,wtk_source_loader_t *sl);
wtk_dnn_state_t* wtk_wfst_dnn_cfg_find(wtk_wfst_dnn_cfg_t *cfg,wtk_string_t *name);
int wtk_wfst_dnn_cfg_attach_hmmset(wtk_wfst_dnn_cfg_t *cfg,wtk_hmmset_t *set);
int wtk_wfst_dnn_cfg_bytes(wtk_wfst_dnn_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
