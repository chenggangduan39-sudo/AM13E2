#ifndef WTK_KSR_VAD_KVAD_WTK_KVAD_CFG
#define WTK_KSR_VAD_KVAD_WTK_KVAD_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/asr/fextra/kparm/wtk_kxparm.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_kvad_cfg wtk_kvad_cfg_t;
struct wtk_kvad_cfg
{
	wtk_kxparm_cfg_t parm;
	int speech_trap;
	int sil_trap;
	int left_margin;
	int right_margin;
	float speech_enter_prob;
	float speech_leave_prob;
	float speech_thresh;

	int fix_speech_enter_prob;
	int fix_speech_leave_prob;
	int fix_speech_thresh;
	int shift;
	int cache;
	unsigned use_fixpoint:1;
	unsigned use_bin:1;
	unsigned use_prob:1;
	unsigned use_nnprob:1;
	wtk_rbin2_t *rbin;
};

int wtk_kvad_cfg_bytes(wtk_kvad_cfg_t *cfg);
int wtk_kvad_cfg_init(wtk_kvad_cfg_t *cfg);
int wtk_kvad_cfg_clean(wtk_kvad_cfg_t *cfg);
int wtk_kvad_cfg_update_local(wtk_kvad_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_kvad_cfg_update(wtk_kvad_cfg_t *cfg);
int wtk_kvad_cfg_update2(wtk_kvad_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif


