#ifndef WTK_ASR_VDEC_WTK_VREC_CFG
#define WTK_ASR_VDEC_WTK_VREC_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/asr/fextra/wtk_fextra.h"
#include "wtk/asr/model/wtk_hmmset.h"
#include "wtk/asr/vdec/rec/wtk_rec.h"
#include "wtk/asr/net/wtk_latset.h"
#include "wtk/asr/net/wtk_ebnf.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vrec_cfg wtk_vrec_cfg_t;
struct wtk_vrec_cfg
{
	wtk_net_cfg_t net;
	wtk_rec_cfg_t rec;
	wtk_dict_t *dict;
	wtk_latset_t *latset;
	wtk_ebnf_t *ebnf;
	char *dict_fn;
	char *net_fn;
	unsigned use_ebnf:1;
};

int wtk_vrec_cfg_init(wtk_vrec_cfg_t *cfg);
int wtk_vrec_cfg_clean(wtk_vrec_cfg_t *cfg);
int wtk_vrec_cfg_update_local(wtk_vrec_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_vrec_cfg_update2(wtk_vrec_cfg_t *cfg,wtk_source_loader_t *sl,wtk_label_t *label,wtk_hmmset_t *hmmset);
#ifdef __cplusplus
};
#endif
#endif
