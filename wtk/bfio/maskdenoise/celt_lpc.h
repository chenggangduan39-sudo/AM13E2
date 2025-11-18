#ifndef WTK_BFIO_MASKDENOISE_CELT_PLC
#define WTK_BFIO_MASKDENOISE_CELT_PLC
#include "pitch.h"
#ifdef __cplusplus
extern "C" {
#endif
void _celt_lpc(float *_lpc, const float *ac, int p);

void celt_fir(
         const float *x,
         const float *num,
         float *y,
         int N,
         int ord);

void celt_iir(const float *x,
         const float *den,
         float *y,
         int N,
         int ord,
         float *mem);

int _celt_autocorr(const float *x, float *ac,
         const float *window, int overlap, int lag, int n);

#ifdef __cplusplus
};
#endif
#endif
