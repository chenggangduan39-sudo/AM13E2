#ifndef __QTK_STITCH_RUN_RECTANGLE_H__
#define __QTK_STITCH_RUN_RECTANGLE_H__

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_array.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int x;
    int y;
    int width;
    int height;
}qtk_stitch_run_rectangle_cfg_t;

int qtk_stitch_run_rectangle_cfg_init(qtk_stitch_run_rectangle_cfg_t *cfg);
int qtk_stitch_run_rectangle_cfg_clean(qtk_stitch_run_rectangle_cfg_t *cfg);
int qtk_stitch_run_rectangle_cfg_update_local(qtk_stitch_run_rectangle_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_stitch_run_rectangle_cfg_update(qtk_stitch_run_rectangle_cfg_t *cfg);

#ifdef  __cplusplus
};
#endif

#endif