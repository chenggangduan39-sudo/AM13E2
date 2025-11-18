/*
 *  Copyright (c) 2003-2004, Mark Borgerding. All rights reserved.
 *  This file is part of KISS FFT - https://github.com/mborgerding/kissfft
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  See COPYING file for more information.
 */

#include "qdm_kiss_fftndr.h"
#include "_qdm_kiss_fft_guts.h"
#define MAX(x,y) ( ( (x)<(y) )?(y):(x) )

struct qdm_kiss_fftndr_state
{
    int dimReal;
    int dimOther;
    qdm_kiss_fftr_cfg cfg_r;
    qdm_kiss_fftnd_cfg cfg_nd;
    void * tmpbuf;
};

static int prod(const int *dims, int ndims)
{
    int x=1;
    while (ndims--) 
        x *= *dims++;
    return x;
}

#ifdef KISS_FFT_ALLINONE
qdm_kiss_fftndr_cfg qdm_kiss_fftndr_alloc(const int *dims,int ndims,void*mem,size_t*lenmem)
#else
qdm_kiss_fftndr_cfg qdm_kiss_fftndr_alloc(const int *dims,int ndims,int inverse_fft,void*mem,size_t*lenmem)
#endif
{
    KISS_FFT_ALIGN_CHECK(mem)

    qdm_kiss_fftndr_cfg st = NULL;
    size_t nr=0 , nd=0,ntmp=0;
    int dimReal = dims[ndims-1];
    int dimOther = prod(dims,ndims-1);
    size_t memneeded;
    char * ptr = NULL;

#ifdef KISS_FFT_ALLINONE
    (void)qdm_kiss_fftr_alloc(dimReal,NULL,&nr);
    (void)qdm_kiss_fftnd_alloc(dims,ndims-1,NULL,&nd);
#else
    (void)qdm_kiss_fftr_alloc(dimReal,inverse_fft,NULL,&nr);
    (void)qdm_kiss_fftnd_alloc(dims,ndims-1,inverse_fft,NULL,&nd);
#endif
    ntmp =
        MAX( 2*dimOther , dimReal+2) * sizeof(qdm_kiss_fft_scalar)  // freq buffer for one pass
        + dimOther*(dimReal+2) * sizeof(qdm_kiss_fft_scalar);  // large enough to hold entire input in case of in-place

    memneeded = KISS_FFT_ALIGN_SIZE_UP(sizeof( struct qdm_kiss_fftndr_state )) + KISS_FFT_ALIGN_SIZE_UP(nr) + KISS_FFT_ALIGN_SIZE_UP(nd) + KISS_FFT_ALIGN_SIZE_UP(ntmp);

    if (lenmem==NULL) {
        ptr = (char*) malloc(memneeded);
    }else{
        if (*lenmem >= memneeded)
            ptr = (char *)mem;
        *lenmem = memneeded; 
    }
    if (ptr==NULL)
        return NULL;
    
    st = (qdm_kiss_fftndr_cfg) ptr;
    memset( st , 0 , memneeded);
    ptr += KISS_FFT_ALIGN_SIZE_UP(sizeof(struct qdm_kiss_fftndr_state));
    
    st->dimReal = dimReal;
    st->dimOther = dimOther;
#ifdef KISS_FFT_ALLINONE
    st->cfg_r = qdm_kiss_fftr_alloc( dimReal,ptr,&nr);
#else
    st->cfg_r = qdm_kiss_fftr_alloc( dimReal,inverse_fft,ptr,&nr);
#endif
    ptr += KISS_FFT_ALIGN_SIZE_UP(nr);
#ifdef KISS_FFT_ALLINONE
    st->cfg_nd = qdm_kiss_fftnd_alloc(dims,ndims-1, ptr,&nd);
#else
    st->cfg_nd = qdm_kiss_fftnd_alloc(dims,ndims-1,inverse_fft, ptr,&nd);
#endif
    ptr += KISS_FFT_ALIGN_SIZE_UP(nd);
    st->tmpbuf = ptr;

    return st;
}

void qdm_kiss_fftndr(qdm_kiss_fftndr_cfg st,const qdm_kiss_fft_scalar *timedata,qdm_kiss_fft_cpx *freqdata)
{
    int k1,k2;
    int dimReal = st->dimReal;
    int dimOther = st->dimOther;
    int nrbins = dimReal/2+1;

    qdm_kiss_fft_cpx * tmp1 = (qdm_kiss_fft_cpx*)st->tmpbuf; 
    qdm_kiss_fft_cpx * tmp2 = tmp1 + MAX(nrbins,dimOther);

    // timedata is N0 x N1 x ... x Nk real

    // take a real chunk of data, fft it and place the output at correct intervals
    for (k1=0;k1<dimOther;++k1) {
        qdm_kiss_fftr( st->cfg_r, timedata + k1*dimReal , tmp1 ); // tmp1 now holds nrbins complex points
        for (k2=0;k2<nrbins;++k2)
           tmp2[ k2*dimOther+k1 ] = tmp1[k2];
    }

    for (k2=0;k2<nrbins;++k2) {
        qdm_kiss_fftnd(st->cfg_nd, tmp2+k2*dimOther, tmp1);  // tmp1 now holds dimOther complex points
        for (k1=0;k1<dimOther;++k1) 
            freqdata[ k1*(nrbins) + k2] = tmp1[k1];
    }
}

void qdm_kiss_fftndri(qdm_kiss_fftndr_cfg st,const qdm_kiss_fft_cpx *freqdata,qdm_kiss_fft_scalar *timedata)
{
    int k1,k2;
    int dimReal = st->dimReal;
    int dimOther = st->dimOther;
    int nrbins = dimReal/2+1;
    qdm_kiss_fft_cpx * tmp1 = (qdm_kiss_fft_cpx*)st->tmpbuf; 
    qdm_kiss_fft_cpx * tmp2 = tmp1 + MAX(nrbins,dimOther);

    for (k2=0;k2<nrbins;++k2) {
        for (k1=0;k1<dimOther;++k1) 
            tmp1[k1] = freqdata[ k1*(nrbins) + k2 ];
        qdm_kiss_fftnd(st->cfg_nd, tmp1, tmp2+k2*dimOther);
    }

    for (k1=0;k1<dimOther;++k1) {
        for (k2=0;k2<nrbins;++k2)
            tmp1[k2] = tmp2[ k2*dimOther+k1 ];
        qdm_kiss_fftri( st->cfg_r,tmp1,timedata + k1*dimReal);
    }
}
