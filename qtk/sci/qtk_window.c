#include "qtk/sci/qtk_window.h"
#include "qtk/math/qtk_math.h"

void qtk_window_hanning_2d(float *dst, int row, int col, float *tmp_buf) {
    const float coeff0 = 2.0 * QTK_PI / (col - 1);
    const float coeff1 = 2.0 * QTK_PI / (row - 1);

    for (int i = 0; i < col; i++) {
        tmp_buf[i] = 0.5 * (1.0 - qtk_cosf(coeff0 * i));
    }
    for (int i = 0; i < row; i++) {
        const float r = 0.5 * (1.0 - qtk_cosf(coeff1 * i));
        for (int j = 0; j < col; j++) {
            *dst++ = r * tmp_buf[j];
        }
    }
}

void qtk_window_hann(int len, float *dst) {
    int i;
    const float coeff = 2.0 * QTK_PI / len;
    for (i = 0; i < len; i++) {
        dst[i] = 0.5 * (1.0 - cos(coeff * i));
    }
}
