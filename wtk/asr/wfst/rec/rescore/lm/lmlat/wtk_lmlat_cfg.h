#ifndef WTK_FST_LM_WTK_LMLAT_CFG_H_
#define WTK_FST_LM_WTK_LMLAT_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/wfst/rec/rescore/lm/nglm/wtk_nglm.h"
#include "wtk/asr/wfst/rec/rescore/lm/wtk_lmexpand_dict.h"
#include "wtk/asr/wfst/net/wtk_fst_net_cfg.h"
#include "wtk/asr/wfst/rec/wtk_wfst_kv.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lmlat_cfg wtk_lmlat_cfg_t;

struct wtk_lmlat_cfg
{
	wtk_wfst_kv_cfg_t kv;
	//wtk_nglm_cfg_t nglm;
	wtk_fst_net_cfg_t output_net;
	wtk_lmexpand_dict_t *dict;
	char *dict_fn;
	float lmscale;
	float wrdpen;
	float beam;
	float state_beam;
	float unk_pen;
	int ntok;
	int state_ntok;
	unsigned use_dict:1;
};

int wtk_lmlat_cfg_init(wtk_lmlat_cfg_t *cfg);
int wtk_lmlat_cfg_clean(wtk_lmlat_cfg_t *cfg);
int wtk_lmlat_cfg_update_local(wtk_lmlat_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_lmlat_cfg_update(wtk_lmlat_cfg_t *cfg);
int wtk_lmlat_cfg_update2(wtk_lmlat_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
