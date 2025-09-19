#ifndef __QBL_SCI_QBL_WINDOW_H__
#define __QBL_SCI_QBL_WINDOW_H__
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// tmp_buf.shape = col
void qtk_window_hanning_2d(float *dst, int row, int col, float *tmp_buf);
void qtk_window_hann(int len, float *dst);

#ifdef __cplusplus
};
#endif
#endif
