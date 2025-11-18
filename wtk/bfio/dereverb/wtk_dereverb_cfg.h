#ifndef WTK_BFIO_DEREVERB_WTK_DEREVERB_CFG
#define WTK_BFIO_DEREVERB_WTK_DEREVERB_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk_admm.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/qform/wtk_covm.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_dereverb_cfg wtk_dereverb_cfg_t;
struct wtk_dereverb_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;
	wtk_stft2_cfg_t stft;
	wtk_covm_cfg_t covm;
	wtk_bf_cfg_t bf;
	wtk_admm_cfg_t admm;

	float theta;
	float theta2;
	float phi;
	float fft_scale;

	unsigned use_fixtheta:1;
	unsigned use_admm:1;
	unsigned use_exlsty:1;
};

int wtk_dereverb_cfg_init(wtk_dereverb_cfg_t *cfg);
int wtk_dereverb_cfg_clean(wtk_dereverb_cfg_t *cfg);
int wtk_dereverb_cfg_update_local(wtk_dereverb_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_dereverb_cfg_update(wtk_dereverb_cfg_t *cfg);
int wtk_dereverb_cfg_update2(wtk_dereverb_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_dereverb_cfg_t* wtk_dereverb_cfg_new(char *fn);
void wtk_dereverb_cfg_delete(wtk_dereverb_cfg_t *cfg);
wtk_dereverb_cfg_t* wtk_dereverb_cfg_new_bin(char *fn);
void wtk_dereverb_cfg_delete_bin(wtk_dereverb_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
