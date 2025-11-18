#ifndef WTK_BFIO_QFORM_WTK_qform2_CFG
#define WTK_BFIO_QFORM_WTK_qform2_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/core/fft/wtk_stft2.h"
#include "wtk/bfio/afilter/wtk_nlms.h"
#include "wtk/bfio/afilter/wtk_rls3.h"
#include "wtk/bfio/dereverb/wtk_admm2.h"
#include "wtk_qmmse.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_qform2_cfg wtk_qform2_cfg_t;
struct wtk_qform2_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    wtk_stft2_cfg_t stft2;
    wtk_bf_cfg_t bf;
    wtk_rls3_cfg_t rls3;
    wtk_nlms_cfg_t nlms;
    wtk_admm2_cfg_t admm;

    int rate;
    int XN;

    float coh_alpha;

    wtk_qmmse_cfg_t qmmse;         

    unsigned use_preemph:1;
    unsigned use_post:1;
    unsigned use_admm2:1;
    unsigned use_nlms:1;
    unsigned debug:1;
};

int wtk_qform2_cfg_init(wtk_qform2_cfg_t *cfg);
int wtk_qform2_cfg_clean(wtk_qform2_cfg_t *cfg);
int wtk_qform2_cfg_update_local(wtk_qform2_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_qform2_cfg_update(wtk_qform2_cfg_t *cfg);
int wtk_qform2_cfg_update2(wtk_qform2_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_qform2_cfg_t* wtk_qform2_cfg_new(char *cfg_fn);
void wtk_qform2_cfg_delete(wtk_qform2_cfg_t *cfg);
wtk_qform2_cfg_t* wtk_qform2_cfg_new_bin(char *bin_fn);
void wtk_qform2_cfg_delete_bin(wtk_qform2_cfg_t *cfg);

void wtk_qform2_cfg_set_noise_suppress(wtk_qform2_cfg_t *cfg,float noise_suppress);

#ifdef __cplusplus
};
#endif
#endif