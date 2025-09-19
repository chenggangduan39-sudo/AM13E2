#ifndef F940E030_1A09_0F19_E777_093428C50225
#define F940E030_1A09_0F19_E777_093428C50225

#include "qtk/core/qtk_min_heap.h"
#include "qtk/cv/detection/qtk_cv_detection_cfg.h"
#include "qtk/cv/qtk_cv_bbox.h"
#include "qtk/nnrt/qtk_nnrt.h"

typedef struct qtk_cv_detection qtk_cv_detection_t;

typedef struct {
    float *keypoints;
    qtk_cv_bbox_t box;
    float conf;
} qtk_cv_detection_result_t;

struct qtk_cv_detection {
    int src_imgwidth;
    int src_imgheight;
    float x_factor;
    float y_factor;
    float rat[2];
    qtk_cv_detection_cfg_t *cfg;
    qtk_nnrt_t *nnrt;
    float *input;
    qtk_min_heap_t mheap;
    qtk_nnrt_value_t *input_val;
    wtk_heap_t *heap;
};

qtk_cv_detection_t *qtk_cv_detection_new(qtk_cv_detection_cfg_t *cfg);
void qtk_cv_detection_delete(qtk_cv_detection_t *fd);
int qtk_cv_detection_detect(qtk_cv_detection_t *fd, uint8_t *image,
                            qtk_cv_detection_result_t **result);

#endif /* F940E030_1A09_0F19_E777_093428C50225 */
