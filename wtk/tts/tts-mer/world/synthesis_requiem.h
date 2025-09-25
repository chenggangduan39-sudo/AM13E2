#ifndef WORLD_SYNTHESIS_REQUIEM_H_
#define WORLD_SYNTHESIS_REQUIEM_H_
#include "common.h"
#include "synthesis.h"
#include "matlabfunctions.h"
#include "constantnumbers.h"
#include "wtk_world_plural.h"

typedef struct wtk_rfft wtk_rfft_t;

void wtk_world_synthesis_requiem(wtk_rfft_t *rf, float *win, int fft_size, float *f0, int f0_len, int fs, float *y, int y_len, float *ap, int ap_len, float *sp, double frame_period);
#endif