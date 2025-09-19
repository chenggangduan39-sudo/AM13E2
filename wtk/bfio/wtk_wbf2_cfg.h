#ifndef WTK_BFIO_WTK_WBF2_CFG
#define WTK_BFIO_WTK_WBF2_CFG
#include "wtk/bfio/qform/wtk_bf.h" 
#include "wtk/bfio/qform/wtk_qform2.h"
#include "wtk/bfio/qform/wtk_qform3.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wbf2_cfg wtk_wbf2_cfg_t;
struct wtk_wbf2_cfg
{
    wtk_stft2_cfg_t stft2;
    wtk_qform2_cfg_t qform2;
    wtk_qform3_cfg_t qform3;

    int wbf2_cnt;
    int *theta;

    unsigned use_line:1;
    unsigned use_qform3:1;
};

int wtk_wbf2_cfg_init(wtk_wbf2_cfg_t *cfg);
int wtk_wbf2_cfg_clean(wtk_wbf2_cfg_t *cfg);
int wtk_wbf2_cfg_update_local(wtk_wbf2_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_wbf2_cfg_update(wtk_wbf2_cfg_t *cfg);
int wtk_wbf2_cfg_update2(wtk_wbf2_cfg_t *cfg,wtk_source_loader_t *sl);

#ifdef __cplusplus
};
#endif
#endif
