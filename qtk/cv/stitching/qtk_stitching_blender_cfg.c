#include "qtk/cv/stitching/qtk_stitching_blender_cfg.h"

int qtk_stitching_blender_cfg_init(qtk_stitching_blender_cfg_t *cfg) {
    cfg->nband = 5;
    cfg->dst_height = 0;
    cfg->dst_width = 0;
    return 0;
}

int qtk_stitching_blender_cfg_clean(qtk_stitching_blender_cfg_t *cfg) {
    return 0;
}

int qtk_stitching_blender_cfg_update(qtk_stitching_blender_cfg_t *cfg) {
    return 0;
}

int qtk_stitching_blender_cfg_update_local(qtk_stitching_blender_cfg_t *cfg,
                                           wtk_local_cfg_t *lc) {
    wtk_string_t *v;
    wtk_local_cfg_update_cfg_i(lc, cfg, nband, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, dst_height, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, dst_width, v);
    return 0;
}
int qtk_stitching_blender_cfg_update2(qtk_stitching_blender_cfg_t *cfg,
                                      wtk_source_loader_t *sl) {
    return 0;
}
