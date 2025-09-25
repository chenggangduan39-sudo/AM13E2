#ifndef WTK_TAC_GRIFFIN_LIM_H_
#define WTK_TAC_GRIFFIN_LIM_H_
#include "wtk/tts-tac/wtk_tac_common.h"
#ifdef __cplusplus
extern "C" {
#endif

void wtk_tac_griffin_lim(wtk_tac_hparams_t *hp, wtk_mer_wav_stream_t *wav, wtk_rfft_t *rf, float *hann_win, float *hann_win_sum, wtk_matf_t *linear);

#ifdef __cplusplus
}
#endif
#endif