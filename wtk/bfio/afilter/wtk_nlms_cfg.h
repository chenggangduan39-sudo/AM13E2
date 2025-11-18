#ifndef WTK_BFIO_AFILTER_WTK_NLMS_CFG
#define WTK_BFIO_AFILTER_WTK_NLMS_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_nlms_cfg wtk_nlms_cfg_t;
struct wtk_nlms_cfg
{
    int channel;
    int M;
    int N;
    int L;
    float coh_alpha;
    float x_alpha;
    float max_u;
    float min_u;

    float leak_scale;

    float orth_m;
    float orth_s;
    float orth2_m;
    float orth2_s;
    float power_l_scale;
    unsigned int use_en_step;
    unsigned int use_sec_iter;
};

int wtk_nlms_cfg_init(wtk_nlms_cfg_t *cfg);
int wtk_nlms_cfg_clean(wtk_nlms_cfg_t *cfg);
int wtk_nlms_cfg_update_local(wtk_nlms_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_nlms_cfg_update(wtk_nlms_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
