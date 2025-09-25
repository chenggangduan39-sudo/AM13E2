#ifndef WTK_BFIO_WTK_EQFORM_CFG
#define WTK_BFIO_WTK_EQFORM_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/qform/wtk_qform9.h"
#include "wtk/bfio/qform/wtk_qform11.h"
#include "wtk/bfio/qform/wtk_qform3.h"
#include "wtk/bfio/aec/wtk_aec.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_eqform_cfg wtk_eqform_cfg_t;

struct wtk_eqform_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    wtk_aec_cfg_t aec;
    wtk_qform9_cfg_t qform9;
    wtk_qform11_cfg_t qform11;
    wtk_qform3_cfg_t qform3;
    int enrcheck_hist;
    float enr_thresh;
    int rate;

    unsigned use_post_form:1;
    unsigned use_aec:1;
    unsigned use_enrcheck:1;
    unsigned use_qform9:1;
    unsigned use_qform11:1;
    unsigned use_qform3:1;
};

int wtk_eqform_cfg_init(wtk_eqform_cfg_t *cfg);
int wtk_eqform_cfg_clean(wtk_eqform_cfg_t *cfg);
int wtk_eqform_cfg_update_local(wtk_eqform_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_eqform_cfg_update(wtk_eqform_cfg_t *cfg);
int wtk_eqform_cfg_update2(wtk_eqform_cfg_t *cfg,wtk_source_loader_t *sl);


wtk_eqform_cfg_t* wtk_eqform_cfg_new(char *cfg_fn);
void wtk_eqform_cfg_delete(wtk_eqform_cfg_t *cfg);
wtk_eqform_cfg_t* wtk_eqform_cfg_new_bin(char *bin_fn);
void wtk_eqform_cfg_delete_bin(wtk_eqform_cfg_t *cfg);


void wtk_eqform_cfg_set_theta_range(wtk_eqform_cfg_t *cfg,float theta_range);
void wtk_eqform_cfg_set_noise_suppress(wtk_eqform_cfg_t *cfg,float noise_suppress);

#ifdef __cplusplus
};
#endif
#endif
