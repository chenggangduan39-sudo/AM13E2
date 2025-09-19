#ifndef __QTK_STITCH_FEATURE_MATCHER_H__
#define __QTK_STITCH_FEATURE_MATCHER_H__

#include "qtk_stitch_def.h"
#include "wtk/core/wtk_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_stitch_feature_matcher{
    void *match;
    int matcher_type;
    int range_width;
    int try_use_gpu;
    int match_conf;
    void *pairwise_matches; //list
}qtk_stitch_feature_matcher_t;

qtk_stitch_feature_matcher_t* qtk_stitch_feature_matcher_new(int matcher_type, 
                                                            int range_width,
                                                            float match_conf);
void qtk_stitch_feature_matcher_delete(qtk_stitch_feature_matcher_t* matcher);
float qtk_stitch_feature_get_match_conf(float match_conf, int type);
void qtk_stitch_feature_matcher_match_feature(qtk_stitch_feature_matcher_t* matcher,wtk_queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif