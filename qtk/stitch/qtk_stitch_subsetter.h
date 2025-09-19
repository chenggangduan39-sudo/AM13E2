#ifndef __QTK_STITCH_SUBSETTER_H__
#define __QTK_STITCH_SUBSETTER_H__

#include "wtk/core/wtk_queue.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_stitch_subsetter{
    float confidece_threshold;
    char *save_file;
    void *indices; //得到相干排序
}qtk_stitch_subsetter_t;

qtk_stitch_subsetter_t* qtk_stitch_subsetter_new(float confidece_threshold, char *save_file);
void qtk_stitch_subsetter_subset(qtk_stitch_subsetter_t *subset,
                                wtk_queue_t *queue,
                                void *paiwise_watches);
void qtk_stitch_subsetter_delete(qtk_stitch_subsetter_t *subset);
#ifdef __cplusplus
}
#endif

#endif