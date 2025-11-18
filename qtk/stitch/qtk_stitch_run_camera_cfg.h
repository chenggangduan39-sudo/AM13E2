#ifndef __QTK_STITCH_RUN_CAMERA_CFG_H__
#define __QTK_STITCH_RUN_CAMERA_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_array.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float aspect;
    float focal;
    float ppx;
    float ppy;
    char *Rt_path;
    wtk_array_t *Rt_shape;
} qtk_stitch_run_camera_cfg_t;

int qtk_stitch_run_camera_cfg_init(qtk_stitch_run_camera_cfg_t *cfg);
int qtk_stitch_run_camera_cfg_clean(qtk_stitch_run_camera_cfg_t *cfg);
int qtk_stitch_run_camera_cfg_update_local(qtk_stitch_run_camera_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_stitch_run_camera_cfg_update(qtk_stitch_run_camera_cfg_t *cfg);

#ifdef __cplusplus
};
#endif


#endif