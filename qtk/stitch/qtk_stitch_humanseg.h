#ifndef __QTK_STITCH_HUMANSEG_H__
#define __QTK_STITCH_HUMANSEG_H__

#include "qtk_stitch_humanseg_cfg.h"
#ifndef IPU_DEC
#include "qtk/nnrt/qtk_nnrt.h"
#else
#include "qtk/qtk_ipu.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_stitch_humanseg{
    qtk_stitch_humanseg_cfg_t *cfg;
#ifndef IPU_DEC
    qtk_nnrt_t *rt;
#else
    qtk_ipu_t *ipu;
#endif
    float *in;
    void *humansegs; //std::vector<cv::Mat>
}qtk_stitch_humanseg_t;

qtk_stitch_humanseg_t* qtk_stitch_humanseg_new(qtk_stitch_humanseg_cfg_t *cfg);
int qtk_stitch_humanseg_delete(qtk_stitch_humanseg_t *seg);
int qtk_stitch_humanseg_feed(qtk_stitch_humanseg_t *seg,void *data,void *out);

#ifdef __cplusplus
}
#endif

#endif