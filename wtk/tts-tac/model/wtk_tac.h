#ifndef WTK_TAC_H_
#define WTK_TAC_H_
#include "wtk/tts-tac/wtk_tac_common.h"
#ifdef __cplusplus
extern "C" {
#endif

void wtk_tac_conv1d(wtk_matf_t *kernel, int kernel_size, wtk_vecf_t *bias, wtk_vecf_t *gamma, wtk_vecf_t *beta, wtk_vecf_t *moving_mean, wtk_vecf_t *moving_variance, float epsilon, void activation(float*, int), int is_after, wtk_matf_t *in, wtk_matf_t *dst);
void wtk_tac_conv1d2(wtk_matf_t *kernel, int kernel_size, wtk_vecf_t *bias, wtk_vecf_t *gamma, wtk_vecf_t *beta, wtk_vecf_t *moving_mean, wtk_vecf_t *moving_variance, float epsilon, void activation(float*, int), int is_after, wtk_matf_t *in, wtk_matf_t *dst);
void wtk_tac_conv1d3(wtk_matf_t *kernel, int kernel_size, wtk_vecf_t *bias, void activation(float*, int), wtk_matf_t *in, wtk_matf_t *dst);
#ifdef __cplusplus
}
#endif
#endif