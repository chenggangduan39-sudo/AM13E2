/*
 *  Copyright (c) 2003-2004, Mark Borgerding. All rights reserved.
 *  This file is part of KISS FFT - https://github.com/mborgerding/kissfft
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  See COPYING file for more information.
 */

#ifndef KISS_FTR_H
#define KISS_FTR_H

#include "qdm_kiss_fft.h"
#ifdef __cplusplus
extern "C" {
#endif

    
/* 
 
 Real optimized version can save about 45% cpu time vs. complex fft of a real seq.

 
 
 */

typedef struct qdm_kiss_fftr_state *qdm_kiss_fftr_cfg;


#ifdef KISS_FFT_ALLINONE
qdm_kiss_fftr_cfg KISS_FFT_API qdm_kiss_fftr_alloc(int nfft,void * mem, size_t * lenmem);
#else
qdm_kiss_fftr_cfg KISS_FFT_API qdm_kiss_fftr_alloc(int nfft,int inverse_fft,void * mem, size_t * lenmem);
#endif
/*
 nfft must be even

 If you don't care to allocate space, use mem = lenmem = NULL 
*/

#ifdef KISS_FFT_ALLINONE
qdm_kiss_fftr_cfg KISS_FFT_API qdm_kiss_fftr_alloc_ex(int nfft,void * mem, size_t * lenmem, qdm_kiss_fft_cpx *twiddle);
#else
qdm_kiss_fftr_cfg KISS_FFT_API qdm_kiss_fftr_alloc_ex(int nfft,int inverse_fft,void * mem, size_t * lenmem, qdm_kiss_fft_cpx *twiddle);
#endif


void KISS_FFT_API qdm_kiss_fftr(qdm_kiss_fftr_cfg cfg,const qdm_kiss_fft_scalar *timedata,qdm_kiss_fft_cpx *freqdata);

void KISS_FFT_API qdm_kiss_fftr_ex(qdm_kiss_fftr_cfg cfg,const qdm_kiss_fft_scalar *timedata,qdm_kiss_fft_cpx *freqdata, qdm_kiss_fft_cpx *tmpbuf);

/*
 input timedata has nfft scalar points
 output freqdata has nfft/2+1 complex points
*/

void KISS_FFT_API qdm_kiss_fftri(qdm_kiss_fftr_cfg cfg,const qdm_kiss_fft_cpx *freqdata,qdm_kiss_fft_scalar *timedata);

void KISS_FFT_API qdm_kiss_fftri_ex(qdm_kiss_fftr_cfg cfg,const qdm_kiss_fft_cpx *freqdata,qdm_kiss_fft_scalar *timedata, qdm_kiss_fft_cpx *tmpbuf);
/*
 input freqdata has  nfft/2+1 complex points
 output timedata has nfft scalar points
*/

#define qdm_kiss_fftr_free KISS_FFT_FREE

#ifdef __cplusplus
}
#endif
#endif
