#ifndef B0E395A1_9D4C_F462_B97C_1F64206014D5
#define B0E395A1_9D4C_F462_B97C_1F64206014D5

#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_ult_track_post_cfg qtk_ult_track_post_cfg_t;
struct qtk_ult_track_post_cfg {
    int win_sz;
    int win_step;
    float height_std_thresh;
};

int qtk_ult_track_post_cfg_init(qtk_ult_track_post_cfg_t *cfg);
int qtk_ult_track_post_cfg_clean(qtk_ult_track_post_cfg_t *cfg);
int qtk_ult_track_post_cfg_update(qtk_ult_track_post_cfg_t *cfg);
int qtk_ult_track_post_cfg_update_local(qtk_ult_track_post_cfg_t *cfg,
                                        wtk_local_cfg_t *lc);

#endif /* B0E395A1_9D4C_F462_B97C_1F64206014D5 */
