#ifndef B040F2B9_4967_3AEC_BD8D_AEB4CEF595A8
#define B040F2B9_4967_3AEC_BD8D_AEB4CEF595A8

#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_mot_sort_cfg qtk_mot_sort_cfg_t;
struct qtk_mot_sort_cfg {
    int max_age;
    int min_hits;
    float speed_noise;
    float delta_t;

    union {
        struct {
            float iou_threshold;
        };
        struct {
            float distance_threshold;
        };
    };

    unsigned use_distance : 1;
};

int qtk_mot_sort_cfg_init(qtk_mot_sort_cfg_t *cfg);
int qtk_mot_sort_cfg_clean(qtk_mot_sort_cfg_t *cfg);
int qtk_mot_sort_cfg_update(qtk_mot_sort_cfg_t *cfg);
int qtk_mot_sort_cfg_update_local(qtk_mot_sort_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_mot_sort_cfg_update2(qtk_mot_sort_cfg_t *cfg, wtk_source_loader_t *sl);

#endif /* B040F2B9_4967_3AEC_BD8D_AEB4CEF595A8 */
