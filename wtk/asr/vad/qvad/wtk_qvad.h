#ifndef WTK_VAD_QVAD_WTK_QVAD
#define WTK_VAD_QVAD_WTK_QVAD
#include "wtk/core/wtk_type.h" 
#include "wtk_qvad_cfg.h"
#include "wb_vad.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_qvad wtk_qvad_t;
struct wtk_qvad
{
	wtk_qvad_cfg_t *cfg;
	VadVars *inst;
};

wtk_qvad_t* wtk_qvad_new(wtk_qvad_cfg_t *cfg);
void wtk_qvad_delete(wtk_qvad_t *v);
void wtk_qvad_reset(wtk_qvad_t *v);
void wtk_qvad_feed(wtk_qvad_t *v,short *data,int len);



#ifdef __cplusplus
};
#endif
#endif
