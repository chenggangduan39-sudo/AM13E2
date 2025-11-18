/*
 *  Copyright (c) 2003-2004, Mark Borgerding. All rights reserved.
 *  This file is part of KISS FFT - https://github.com/mborgerding/kissfft
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  See COPYING file for more information.
 */

#ifndef KISS_FFTND_H
#define KISS_FFTND_H

#include "qdm_kiss_fft.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qdm_kiss_fftnd_state * qdm_kiss_fftnd_cfg;
    
#ifdef KISS_FFT_ALLINONE
qdm_kiss_fftnd_cfg KISS_FFT_API qdm_kiss_fftnd_alloc(const int *dims,int ndims,void*mem,size_t*lenmem);
#else
qdm_kiss_fftnd_cfg KISS_FFT_API qdm_kiss_fftnd_alloc(const int *dims,int ndims,int inverse_fft,void*mem,size_t*lenmem);
#endif
void KISS_FFT_API qdm_kiss_fftnd(qdm_kiss_fftnd_cfg  cfg,const qdm_kiss_fft_cpx *fin,qdm_kiss_fft_cpx *fout);
#ifdef KISS_FFT_ALLINONE
void KISS_FFT_API kiss_ifftnd(qdm_kiss_fftnd_cfg  cfg,const qdm_kiss_fft_cpx *fin,qdm_kiss_fft_cpx *fout);
#endif

#ifdef __cplusplus
}
#endif
#endif
