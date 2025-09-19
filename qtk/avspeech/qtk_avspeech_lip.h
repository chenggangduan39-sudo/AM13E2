#ifndef G_D44E85396FA54079B86039E0C9F35C84
#define G_D44E85396FA54079B86039E0C9F35C84

#include "qtk/avspeech/qtk_avspeech_lip_cfg.h"
#include "qtk/cv/detection/qtk_cv_detection.h"
#include "qtk/cv/tracking/qtk_mot_sort.h"
#include "wtk/core/wtk_heap.h"
#include "wtk/core/wtk_hoard.h"

typedef enum {
    QTK_AVSPEECH_LIP_START,
    QTK_AVSPEECH_LIP_DATA,
    QTK_AVSPEECH_LIP_END,
    QTK_AVSPEECH_LIP_EMPTY,
} qtk_avspeech_lip_result_state_t;

typedef struct {
    qtk_avspeech_lip_result_state_t state;
    uint8_t *lip_image;
    uint8_t *face_patch;
    int face_H;
    int face_W;
    int id;
    uint32_t frame_idx;
    qtk_cv_bbox_t roi;
} qtk_avspeech_lip_result_t;

typedef struct qtk_avspeech_lip qtk_avspeech_lip_t;
typedef int (*qtk_avspeech_lip_notifier_t)(void *upval,
                                           qtk_avspeech_lip_result_t *result);

struct qtk_avspeech_lip {
    qtk_avspeech_lip_cfg_t *cfg;
    qtk_cv_detection_t *face_det;
    qtk_nnrt_t *landmark;
    qtk_avspeech_lip_notifier_t notifier;
    qtk_mot_sort_t mot;
    wtk_heap_t *heap;
    wtk_hoard_t tracklet_hist;
    qtk_cv_detection_result_t *cur_faces;
    uint8_t *lip_patch;
    uint8_t *landmark_patch;
    uint8_t *detect_patch;
    void *upval;
    uint32_t frame_idx;

    void *selector;
};

qtk_avspeech_lip_t *qtk_avspeech_lip_new(qtk_avspeech_lip_cfg_t *cfg,
                                         void *upval,
                                         qtk_avspeech_lip_notifier_t notifier);
void qtk_avspeech_lip_delete(qtk_avspeech_lip_t *m);
int qtk_avspeech_lip_feed(qtk_avspeech_lip_t *lip, uint8_t *image, int H,
                          int W);
int qtk_avspeech_lip_reset(qtk_avspeech_lip_t *lip);
#endif
