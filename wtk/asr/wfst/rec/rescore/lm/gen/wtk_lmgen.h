#ifndef WTK_FST_LM_NGLM_WTK_LMGEN
#define WTK_FST_LM_NGLM_WTK_LMGEN
#include "wtk/core/wtk_type.h" 
#include "wtk_lmgen_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lmgen wtk_lmgen_t;


struct wtk_lmgen
{
	wtk_lmgen_cfg_t *cfg;
	wtk_lmgen_rec_t *backward;
	wtk_lmgen_rec_t *forward;
};


wtk_lmgen_t* wtk_lmgen_new(wtk_lmgen_cfg_t *cfg);
void wtk_lmgen_delete(wtk_lmgen_t *gen);
void wtk_lmgen_reset(wtk_lmgen_t *gen);
void wtk_lmgen_test(wtk_lmgen_t *gen);
#ifdef __cplusplus
};
#endif
#endif
