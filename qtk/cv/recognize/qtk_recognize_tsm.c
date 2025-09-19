#include "qtk/cv/recognize/qtk_recognize_tsm.h"
#include "qtk/math/qtk_vector.h"
#include "wtk/core/wtk_alloc.h"

int qtk_recognize_tcm_post_init(qtk_recognize_tcm_post_t *post, int ncls,
                                int likelyhood_nhist) {
    post->likelyhood_hist =
        qtk_cyclearray_new(likelyhood_nhist, sizeof(float) * ncls);
    post->cls_hist = qtk_cyclearray_new(3, sizeof(int));
    post->ncls = ncls;
    post->likelyhood_nhist = likelyhood_nhist;
    post->stale_likelyhood = cast(float *, (wtk_malloc(sizeof(float) * ncls)));
    post->likelyhood_sum = cast(float *, wtk_calloc(sizeof(float), ncls));
    return 0;
}

void qtk_recognize_tcm_post_clean(qtk_recognize_tcm_post_t *post) {
    wtk_free(post->likelyhood_hist);
    wtk_free(post->cls_hist);
    wtk_free(post->stale_likelyhood);
    wtk_free(post->likelyhood_sum);
}

int qtk_recognize_tcm_post_process(qtk_recognize_tcm_post_t *post,
                                   const float *likelyhood) {
    int cls_id;
    float max_val;
    qtk_vector_add_elewise(post->likelyhood_sum, cast(float *, likelyhood),
                           post->likelyhood_sum, post->ncls);
    if (qtk_cyclearray_push(post->likelyhood_hist, cast(float *, (likelyhood)),
                            post->stale_likelyhood)) {
        qtk_vector_sub_elewise(post->likelyhood_sum, post->stale_likelyhood,
                               post->likelyhood_sum, post->ncls);
    }
    qtk_vector_argmax(post->likelyhood_sum, post->ncls, &max_val, &cls_id);
    return cls_id;
}
