#ifndef WTK_BFIO_AEC_WTK_AEC_CFG
#define WTK_BFIO_AEC_WTK_AEC_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/fft/wtk_stft2.h"
#include "wtk/bfio/afilter/wtk_nlms.h"
#include "wtk/bfio/afilter/wtk_rls.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_aec_cfg wtk_aec_cfg_t;
struct wtk_aec_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_stft2_cfg_t stft;
    wtk_nlms_cfg_t nlms;
    wtk_rls_cfg_t rls;
    wtk_qmmse_cfg_t qmmse;

    float leak_scale;
    float coh_alpha;

    int spchannel;

    float spenr_thresh;
    int spenr_cnt;

    unsigned use_preemph:1;
    unsigned use_aec_post:1;
    unsigned use_post:1;
    unsigned use_rls:1;
    unsigned use_nlms:1;
};

int wtk_aec_cfg_init(wtk_aec_cfg_t *cfg);
int wtk_aec_cfg_clean(wtk_aec_cfg_t *cfg);
int wtk_aec_cfg_update_local(wtk_aec_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_aec_cfg_update(wtk_aec_cfg_t *cfg);
int wtk_aec_cfg_update2(wtk_aec_cfg_t *cfg,wtk_source_loader_t *sl);
wtk_aec_cfg_t* wtk_aec_cfg_new(char *fn);
void wtk_aec_cfg_delete(wtk_aec_cfg_t *cfg);
wtk_aec_cfg_t* wtk_aec_cfg_new_bin(char *fn);
void wtk_aec_cfg_delete_bin(wtk_aec_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
