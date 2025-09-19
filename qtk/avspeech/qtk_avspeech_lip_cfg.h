#ifndef G_4CD68C2481324E9884F6448CC116BAAE
#define G_4CD68C2481324E9884F6448CC116BAAE
#include "qtk/cv/detection/qtk_cv_detection_cfg.h"
#include "qtk/cv/tracking/qtk_mot_sort_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_avspeech_lip_cfg qtk_avspeech_lip_cfg_t;
typedef enum {
    QTK_AVSPEECH_LIP_SELECTOR_POLICY_ALL,
    QTK_AVSPEECH_LIP_SELECTOR_POLICY_CENTRAL,
    QTK_AVSPEECH_LIP_SELECTOR_POLICY_AT_MOST,
    QTK_AVSPEECH_LIP_SELECTOR_POLICY_MAX,
} qtk_avspeech_lip_selector_policy_t;

struct qtk_avspeech_lip_cfg {
    qtk_cv_detection_cfg_t face_det;
    qtk_nnrt_cfg_t landmark;
    qtk_mot_sort_cfg_t mot;
    qtk_avspeech_lip_selector_policy_t
        selector_policy; // "all", "central", "at_most"
    int policy_n;
    int H;
    int W;
    int num_landmarks;
    int nskip;
    int border;
    float camera_angle;

    /* policy central parameters */
    float theta_tolerance;
    float switch_enter_trap;

    unsigned use_landmark : 1;
};

int qtk_avspeech_lip_cfg_init(qtk_avspeech_lip_cfg_t *cfg);
int qtk_avspeech_lip_cfg_clean(qtk_avspeech_lip_cfg_t *cfg);
int qtk_avspeech_lip_cfg_update(qtk_avspeech_lip_cfg_t *cfg);
int qtk_avspeech_lip_cfg_update_local(qtk_avspeech_lip_cfg_t *cfg,
                                      wtk_local_cfg_t *lc);
int qtk_avspeech_lip_cfg_update2(qtk_avspeech_lip_cfg_t *cfg,
                                 wtk_source_loader_t *sl);

#endif
