#ifndef G_DD2401F7E9C04582BD4CAF1EF44F21E8
#define G_DD2401F7E9C04582BD4CAF1EF44F21E8
#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_stitching_blender_cfg qtk_stitching_blender_cfg_t;

struct qtk_stitching_blender_cfg {
    int nband;
    int dst_height;
    int dst_width;
};

int qtk_stitching_blender_cfg_init(qtk_stitching_blender_cfg_t *cfg);
int qtk_stitching_blender_cfg_clean(qtk_stitching_blender_cfg_t *cfg);
int qtk_stitching_blender_cfg_update(qtk_stitching_blender_cfg_t *cfg);
int qtk_stitching_blender_cfg_update_local(qtk_stitching_blender_cfg_t *cfg,
                                           wtk_local_cfg_t *lc);
int qtk_stitching_blender_cfg_update2(qtk_stitching_blender_cfg_t *cfg,
                                      wtk_source_loader_t *sl);

#endif
