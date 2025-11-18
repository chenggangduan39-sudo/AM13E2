#ifndef WTK_BFIO_QFORM_WTK_QFORM_PICKUP2_CFG
#define WTK_BFIO_QFORM_WTK_QFORM_PICKUP2_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/fft/wtk_stft.h"
#include "wtk_qmmse.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_qform_pickup2_cfg wtk_qform_pickup2_cfg_t;

struct wtk_qform_pickup2_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    wtk_stft_cfg_t stft;

    int rate;

    float preemph;
	float notch_radius;
	float notch_radius_den;

    wtk_qmmse_cfg_t qmmse;
};

int wtk_qform_pickup2_cfg_init(wtk_qform_pickup2_cfg_t *cfg);
int wtk_qform_pickup2_cfg_clean(wtk_qform_pickup2_cfg_t *cfg);
int wtk_qform_pickup2_cfg_update_local(wtk_qform_pickup2_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_qform_pickup2_cfg_update(wtk_qform_pickup2_cfg_t *cfg);
int wtk_qform_pickup2_cfg_update2(wtk_qform_pickup2_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_qform_pickup2_cfg_t* wtk_qform_pickup2_cfg_new(char *cfg_fn);
void wtk_qform_pickup2_cfg_delete(wtk_qform_pickup2_cfg_t *cfg);
wtk_qform_pickup2_cfg_t* wtk_qform_pickup2_cfg_new_bin(char *bin_fn);
void wtk_qform_pickup2_cfg_delete_bin(wtk_qform_pickup2_cfg_t *cfg);

void wtk_qform_pickup2_cfg_set_noise_suppress(wtk_qform_pickup2_cfg_t *cfg,float noise_suppress);

#ifdef __cplusplus
};
#endif
#endif
