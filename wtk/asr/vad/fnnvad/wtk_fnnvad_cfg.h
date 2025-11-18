#ifndef WTK_VAD_FNNVAD_WTK_FNNVAD_CFG_H_
#define WTK_VAD_FNNVAD_WTK_FNNVAD_CFG_H_
#include "wtk/core/wtk_type.h"
#include "wtk/asr/fextra/wtk_fextra.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fnnvad_cfg wtk_fnnvad_cfg_t;
struct wtk_fnnvad_cfg
{
	wtk_fextra_cfg_t parm;
	int cache;
	int win;
	int siltrap;
	int speechtrap;
	float sil_thresh;
	float speech_energe_thresh;
	float echo_speech_thresh;
	float high_speech_thresh_rate;
	int high_speech_min_frame;
	float detect_sil_go_high_thresh_rate;
	float detect_high_go_low_thresh_rate;
	int detect_min_high_frame;
	int detect_min_low_frame;
	unsigned use_speech_end_detect:1;
};

int wtk_fnnvad_cfg_init(wtk_fnnvad_cfg_t *cfg);
int wtk_fnnvad_cfg_clean(wtk_fnnvad_cfg_t *cfg);
int wtk_fnnvad_cfg_update_local(wtk_fnnvad_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_fnnvad_cfg_update(wtk_fnnvad_cfg_t *cfg);
int wtk_fnnvad_cfg_update2(wtk_fnnvad_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
