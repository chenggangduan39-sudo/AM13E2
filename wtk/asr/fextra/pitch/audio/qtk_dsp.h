#ifndef __QTK_AUDIO_QTK_DSP_H__
#define __QTK_AUDIO_QTK_DSP_H__
#include "wtk/core/wtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif

void qtk_dsp_preemph(float *sig, size_t len, float coeff);

#ifdef __cplusplus
};
#endif
#endif
