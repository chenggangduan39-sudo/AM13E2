#include "qtk/math/qtk_vector.h"

void qtk_signal_normalize(float *sample, int len, float desired_rms,
                          float *normalizer) {
    int i;
    double rms = 0;
    float ratio;
    for (i = 0; i < len; i++) {
        rms += sample[i] * sample[i];
    }
    rms = max(sqrt(rms / len), 1e-4);
    ratio = desired_rms / rms;
    *normalizer = rms / desired_rms;
    qtk_vector_scale(sample, sample, len, ratio);
}
