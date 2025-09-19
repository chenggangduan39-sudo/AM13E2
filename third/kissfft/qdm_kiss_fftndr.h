/*
 *  Copyright (c) 2003-2004, Mark Borgerding. All rights reserved.
 *  This file is part of KISS FFT - https://github.com/mborgerding/kissfft
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  See COPYING file for more information.
 */

#ifndef KISS_NDR_H
#define KISS_NDR_H

#include "qdm_kiss_fft.h"
#include "qdm_kiss_fftr.h"
#include "qdm_kiss_fftnd.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct qdm_kiss_fftndr_state *qdm_kiss_fftndr_cfg;


#ifdef KISS_FFT_ALLINONE
qdm_kiss_fftndr_cfg KISS_FFT_API qdm_kiss_fftndr_alloc(const int *dims,int ndims,void*mem,size_t*lenmem);
#else
qdm_kiss_fftndr_cfg KISS_FFT_API qdm_kiss_fftndr_alloc(const int *dims,int ndims,int inverse_fft,void*mem,size_t*lenmem);
#endif
/*
 dims[0] must be even

 If you don't care to allocate space, use mem = lenmem = NULL 
*/


void KISS_FFT_API qdm_kiss_fftndr(
        qdm_kiss_fftndr_cfg cfg,
        const qdm_kiss_fft_scalar *timedata,
        qdm_kiss_fft_cpx *freqdata);
/*
 input timedata has dims[0] X dims[1] X ... X  dims[ndims-1] scalar points
 output freqdata has dims[0] X dims[1] X ... X  dims[ndims-1]/2+1 complex points
*/

void KISS_FFT_API qdm_kiss_fftndri(
        qdm_kiss_fftndr_cfg cfg,
        const qdm_kiss_fft_cpx *freqdata,
        qdm_kiss_fft_scalar *timedata);
/*
 input and output dimensions are the exact opposite of qdm_kiss_fftndr
*/


#define qdm_kiss_fftndr_free free

#ifdef __cplusplus
}
#endif

#endif
