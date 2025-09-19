#include "wtk/asr/fextra/pitch/audio/qtk_dsp.h"

void qtk_dsp_preemph(float *sig, size_t len, float coeff) {
    size_t idx;

    for (idx = len - 1; idx > 0; idx--) {
        sig[idx] -= coeff * sig[idx - 1];
    }
    sig[0] *= (1.0 - coeff);
}
