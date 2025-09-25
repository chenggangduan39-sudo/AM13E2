#ifndef WTK_MER_MIN_MAX_NORM_H_
#define WTK_MER_MIN_MAX_NORM_H_
#include "tts-mer/wtk_mer_common.h"
#ifdef __cplusplus
extern "C" {
#endif

void wtk_mer_mean_variance_norm(wtk_matf_t *src, wtk_vecf_t *min_max);
wtk_matf_t* wtk_mer_normalise_data(wtk_matf_t *matf, wtk_vecf_t *min_max_vec);

#ifdef __cplusplus
}
#endif
#endif
