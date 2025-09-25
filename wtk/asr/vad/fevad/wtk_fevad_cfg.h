#ifndef WTK_ASR_VAD_FEVAD_WTK_FEVAD_CFG
#define WTK_ASR_VAD_FEVAD_WTK_FEVAD_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/fft/wtk_stft.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fevad_cfg wtk_fevad_cfg_t;
struct wtk_fevad_cfg
{
	wtk_stft_cfg_t stft;
	int rate;
	float sil_time;
	float min_sil_time;
	float min_speech_time;
	int min_start_frame;
	int min_sil_frame;
	int min_speech_frame;
	float e_thresh;
	float f_thresh;
	float fv_thresh;
	float sf_thresh;

	float e_ratio;
	float e_alpha;

	unsigned debug:1;
};

int wtk_fevad_cfg_init(wtk_fevad_cfg_t *cfg);
int wtk_fevad_cfg_clean(wtk_fevad_cfg_t *cfg);
int wtk_fevad_cfg_update_local(wtk_fevad_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_fevad_cfg_update(wtk_fevad_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
