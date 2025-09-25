#ifndef WTK_BFIO_DEREVERB_WTK_DEREVERB2_CFG
#define WTK_BFIO_DEREVERB_WTK_DEREVERB2_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/fft/wtk_stft2.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_dereverb2_cfg wtk_dereverb2_cfg_t;
struct wtk_dereverb2_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    wtk_stft2_cfg_t stft;
    wtk_bf_cfg_t bf;
    wtk_qmmse_cfg_t qmmse;

    int rate;

    float preemph;
	float notch_radius;
	float notch_radius_den;

    int D;
	int L;
	float sigma;
	float p;
    float lambda;

    float wx_alpha;
    float coh_alpha;
    float leak_scale;

    float theta;
    float phi;

    unsigned use_post:1;
};

int wtk_dereverb2_cfg_init(wtk_dereverb2_cfg_t *cfg);
int wtk_dereverb2_cfg_clean(wtk_dereverb2_cfg_t *cfg);
int wtk_dereverb2_cfg_update_local(wtk_dereverb2_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_dereverb2_cfg_update(wtk_dereverb2_cfg_t *cfg);
int wtk_dereverb2_cfg_update2(wtk_dereverb2_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_dereverb2_cfg_t* wtk_dereverb2_cfg_new(char *fn);
void wtk_dereverb2_cfg_delete(wtk_dereverb2_cfg_t *cfg);
wtk_dereverb2_cfg_t* wtk_dereverb2_cfg_new_bin(char *fn);
void wtk_dereverb2_cfg_delete_bin(wtk_dereverb2_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif