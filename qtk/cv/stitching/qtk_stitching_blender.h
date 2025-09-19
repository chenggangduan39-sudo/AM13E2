#ifndef G_519B85B9753B45CF990F1DA779800993
#define G_519B85B9753B45CF990F1DA779800993

#include "qtk/cv/stitching/qtk_stitching_blender_cfg.h"

typedef struct qtk_stitching_blender qtk_stitching_blender_t;

struct qtk_stitching_blender {
    qtk_stitching_blender_cfg_t *cfg;
    void *impl;
};

qtk_stitching_blender_t *
qtk_stitching_blender_new(qtk_stitching_blender_cfg_t *cfg);
void qtk_stitching_blender_delete(qtk_stitching_blender_t *b);
void qtk_stitching_blender_set_dst(qtk_stitching_blender_t *b, uint8_t *dst);
void qtk_stitching_blender_blend(qtk_stitching_blender_t *b);
void qtk_stitching_blender_reset(qtk_stitching_blender_t *b);
int qtk_stitching_blender_feed(qtk_stitching_blender_t *b, uint8_t **image);
int qtk_stitching_blender_feed_single(qtk_stitching_blender_t *b,
                                      uint8_t *image, int idx);
int qtk_stitching_blender_set_masks(qtk_stitching_blender_t *b, int nframe,
                                    uint8_t **masks, int *tls, int *sizes);
int qtk_stitching_blender_clear_masks(qtk_stitching_blender_t *b);

#endif
