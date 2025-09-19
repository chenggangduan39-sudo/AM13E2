#ifndef AC3D5D0F_63AB_F6E6_B1ED_BE4463FCEB1B
#define AC3D5D0F_63AB_F6E6_B1ED_BE4463FCEB1B

#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_cv_face_rec_alignment_cfg qtk_cv_face_rec_alignment_cfg_t;

struct qtk_cv_face_rec_alignment_cfg {
    int dst_width;
    int dst_height;

    float ref_ptr_scale;
};

int qtk_cv_face_rec_alignment_cfg_init(qtk_cv_face_rec_alignment_cfg_t *cfg);
int qtk_cv_face_rec_alignment_cfg_clean(qtk_cv_face_rec_alignment_cfg_t *cfg);
int qtk_cv_face_rec_alignment_cfg_update(qtk_cv_face_rec_alignment_cfg_t *cfg);
int qtk_cv_face_rec_alignment_cfg_update_local(
    qtk_cv_face_rec_alignment_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_cv_face_rec_alignment_cfg_update2(qtk_cv_face_rec_alignment_cfg_t *cfg,
                                          wtk_source_loader_t *sl);

#endif /* AC3D5D0F_63AB_F6E6_B1ED_BE4463FCEB1B */
