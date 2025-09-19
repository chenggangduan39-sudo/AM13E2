#ifndef __QTK_STITCH_RUN_SEAMMASK_CFG_H__
#define __QTK_STITCH_RUN_SEAMMASK_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_array.h"

typedef struct {
    wtk_array_t *shape;
    char *seam_mask_path;
}qtk_stitch_run_seammask_cfg_t;

int qtk_stitch_run_seammask_cfg_init(qtk_stitch_run_seammask_cfg_t *cfg);
int qtk_stitch_run_seammask_cfg_clean(qtk_stitch_run_seammask_cfg_t *cfg);
int qtk_stitch_run_seammask_cfg_update_local(qtk_stitch_run_seammask_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_stitch_run_seammask_cfg_update(qtk_stitch_run_seammask_cfg_t *cfg);

#endif