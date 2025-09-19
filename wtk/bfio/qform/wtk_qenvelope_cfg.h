#ifndef WTK_BFIO_QFORM_WTK_QENVELOPE_CFG
#define WTK_BFIO_QFORM_WTK_QENVELOPE_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_qenvelope_cfg wtk_qenvelope_cfg_t;
struct wtk_qenvelope_cfg
{
    int envelope_nf;
    float envelope_thresh;
    
    float min_speccrest;
    int left_nf;
    int right_nf;
    float right_min_thresh;
    int max_rise_nf;

    int idx;

    unsigned use_envelope2:1;
    unsigned debug:1;
};

int wtk_qenvelope_cfg_init(wtk_qenvelope_cfg_t *cfg);
int wtk_qenvelope_cfg_clean(wtk_qenvelope_cfg_t *cfg);
int wtk_qenvelope_cfg_update_local(wtk_qenvelope_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_qenvelope_cfg_update(wtk_qenvelope_cfg_t *cfg);
int wtk_qenvelope_cfg_set_idx(wtk_qenvelope_cfg_t *cfg, int idx);
#ifdef __cplusplus
};
#endif
#endif