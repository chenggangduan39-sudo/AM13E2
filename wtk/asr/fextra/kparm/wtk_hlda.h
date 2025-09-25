#ifndef WTK_ASR_PARM_WTK_HLDA
#define WTK_ASR_PARM_WTK_HLDA
#include "wtk/core/wtk_type.h" 
#include "wtk_hlda_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_hlda wtk_hlda_t;

struct wtk_hlda
{
	wtk_hlda_cfg_t *cfg;
	float *vec;
};

int wtk_hlda_bytes(wtk_hlda_t *hlda);
wtk_hlda_t* wtk_hlda_new(wtk_hlda_cfg_t *cfg);
void wtk_hlda_delete(wtk_hlda_t *hlda);
void wtk_hlda_calc(wtk_hlda_t *hlda,float *pv);
void wtk_hlda_calc_fix(wtk_hlda_t *hlda,int *pv);
#ifdef __cplusplus
};
#endif
#endif
