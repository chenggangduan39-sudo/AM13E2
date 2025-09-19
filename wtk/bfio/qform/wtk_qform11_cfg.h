#ifndef WTK_BFIO_QFORM_WTK_QFORM11_CFG
#define WTK_BFIO_QFORM_WTK_QFORM11_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/aspec/wtk_aspec.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/core/fft/wtk_stft2.h"
#include "wtk_qmmse.h"
#include "wtk_covm.h"
#include "wtk_qenvelope.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_qform11_cfg wtk_qform11_cfg_t;
struct wtk_qform11_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    wtk_stft2_cfg_t stft2;
    int rate;

    wtk_aspec_cfg_t aspec;
    wtk_covm_cfg_t covm;
    wtk_bf_cfg_t bf;

	int nmulchannel;
    float **mul_theta_range;
    float *aspec_theta;
    int naspec;
    float tstep;
    int out_len;

    int theta_range;

    int lf;
    int lt;

    float specsum_fs;
    float specsum_fe;
    int specsum_ns;
    int specsum_ne;

    float cohv_thresh;

    wtk_qenvelope_cfg_t qenvl;
    wtk_qmmse_cfg_t qmmse;

    unsigned use_line:1;
    unsigned use_post:1;

    unsigned use_qenvelope:1;
    unsigned use_sqenvelope:1;
    unsigned use_preemph:1;
    unsigned debug:1;
};

int wtk_qform11_cfg_init(wtk_qform11_cfg_t *cfg);
int wtk_qform11_cfg_clean(wtk_qform11_cfg_t *cfg);
int wtk_qform11_cfg_update_local(wtk_qform11_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_qform11_cfg_update(wtk_qform11_cfg_t *cfg);
int wtk_qform11_cfg_update2(wtk_qform11_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_qform11_cfg_t* wtk_qform11_cfg_new(char *cfg_fn);
void wtk_qform11_cfg_delete(wtk_qform11_cfg_t *cfg);
wtk_qform11_cfg_t* wtk_qform11_cfg_new_bin(char *bin_fn);
void wtk_qform11_cfg_delete_bin(wtk_qform11_cfg_t *cfg);

void wtk_qform11_cfg_set_theta_range(wtk_qform11_cfg_t *cfg,float theta_range);
void wtk_qform11_cfg_set_noise_suppress(wtk_qform11_cfg_t *cfg,float noise_suppress);
#ifdef __cplusplus
};
#endif
#endif
