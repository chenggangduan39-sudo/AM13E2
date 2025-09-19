#ifndef WTK_BFIO_QFORM_WTK_QFORM_PICHUP_CFG
#define WTK_BFIO_QFORM_WTK_QFORM_PICKUP_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/core/fft/wtk_stft.h"
#include "wtk_qmmse.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_qform_pickup_cfg wtk_qform_pickup_cfg_t;

struct wtk_qform_pickup_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    wtk_stft_cfg_t stft;

    int rate;

    float preemph;
	float notch_radius;
	float notch_radius_den;

    wtk_bf_cfg_t bf;
    float ncov_alpha;
    int init_ncovnf;

    float scov_alpha;
    int init_scovnf;

    float cohv_thresh;
    float theta_range;

    int noise_block;
    float *ntheta;
    float ntheta_range;

    float batch_tm;
    int nbatch;

    int fs;
    int fe;

    wtk_qmmse_cfg_t qmmse;

    unsigned use_cohvsum:1;
    unsigned use_line:1;
    unsigned use_post:1;
    unsigned debug:1;
};

int wtk_qform_pickup_cfg_init(wtk_qform_pickup_cfg_t *cfg);
int wtk_qform_pickup_cfg_clean(wtk_qform_pickup_cfg_t *cfg);
int wtk_qform_pickup_cfg_update_local(wtk_qform_pickup_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_qform_pickup_cfg_update(wtk_qform_pickup_cfg_t *cfg);
int wtk_qform_pickup_cfg_update2(wtk_qform_pickup_cfg_t *cfg,wtk_source_loader_t *sl);


wtk_qform_pickup_cfg_t* wtk_qform_pickup_cfg_new(char *cfg_fn);
void wtk_qform_pickup_cfg_delete(wtk_qform_pickup_cfg_t *cfg);
wtk_qform_pickup_cfg_t* wtk_qform_pickup_cfg_new_bin(char *bin_fn);
void wtk_qform_pickup_cfg_delete_bin(wtk_qform_pickup_cfg_t *cfg);

void wtk_qform_pickup_cfg_set_theta_range(wtk_qform_pickup_cfg_t *cfg,float theta_range);
void wtk_qform_pickup_cfg_set_noise_suppress(wtk_qform_pickup_cfg_t *cfg,float noise_suppress);

#ifdef __cplusplus
};
#endif
#endif
