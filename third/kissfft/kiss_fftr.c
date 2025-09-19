/*
 *  Copyright (c) 2003-2004, Mark Borgerding. All rights reserved.
 *  This file is part of KISS FFT - https://github.com/mborgerding/kissfft
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  See COPYING file for more information.
 */

#include "qdm_kiss_fftr.h"
#include "_qdm_kiss_fft_guts.h"

struct qdm_kiss_fftr_state{
    qdm_kiss_fft_cfg substate;
    qdm_kiss_fft_cpx * tmpbuf;
    qdm_kiss_fft_cpx * super_twiddles;
#ifdef USE_SIMD
    void * pad;
#endif
};

#ifdef KISS_FFT_ALLINONE
qdm_kiss_fftr_cfg qdm_kiss_fftr_alloc(int nfft,void * mem,size_t * lenmem)
#else
qdm_kiss_fftr_cfg qdm_kiss_fftr_alloc(int nfft,int inverse_fft,void * mem,size_t * lenmem)
#endif
{
	KISS_FFT_ALIGN_CHECK(mem)

    int i;
    qdm_kiss_fftr_cfg st = NULL;
    size_t subsize = 0, memneeded;

    if (nfft & 1) {
        KISS_FFT_ERROR("Real FFT optimization must be even.");
        return NULL;
    }
    nfft >>= 1;

#ifdef KISS_FFT_ALLINONE
    qdm_kiss_fft_alloc (nfft, NULL, &subsize);
#else
    qdm_kiss_fft_alloc (nfft, inverse_fft, NULL, &subsize);
#endif
    memneeded = sizeof(struct qdm_kiss_fftr_state) + subsize + sizeof(qdm_kiss_fft_cpx) * ( nfft * 3 / 2);

    if (lenmem == NULL) {
        st = (qdm_kiss_fftr_cfg) KISS_FFT_MALLOC (memneeded);
    } else {
        if (*lenmem >= memneeded)
            st = (qdm_kiss_fftr_cfg) mem;
        *lenmem = memneeded;
    }
    if (!st)
        return NULL;

    st->substate = (qdm_kiss_fft_cfg) (st + 1); /*just beyond qdm_kiss_fftr_state struct */
    st->tmpbuf = (qdm_kiss_fft_cpx *) (((char *) st->substate) + subsize);
    st->super_twiddles = st->tmpbuf + nfft;
#ifdef KISS_FFT_ALLINONE
    qdm_kiss_fft_alloc(nfft, st->substate, &subsize);
#else
    qdm_kiss_fft_alloc(nfft, inverse_fft, st->substate, &subsize);
#endif

    for (i = 0; i < nfft/2; ++i) {
        double phase =
            -3.14159265358979323846264338327 * ((double) (i+1) / nfft + .5);
#ifndef KISS_FFT_ALLINONE
        if (inverse_fft)
            phase *= -1;
#endif
        kf_cexp (st->super_twiddles+i,phase);
    }
    return st;
}

#ifdef KISS_FFT_ALLINONE
qdm_kiss_fftr_cfg KISS_FFT_API qdm_kiss_fftr_alloc_ex(int nfft,void * mem, size_t * lenmem, qdm_kiss_fft_cpx *twiddle)
#else
qdm_kiss_fftr_cfg KISS_FFT_API qdm_kiss_fftr_alloc_ex(int nfft,int inverse_fft,void * mem, size_t * lenmem, qdm_kiss_fft_cpx *twiddle)
#endif
{
	KISS_FFT_ALIGN_CHECK(mem)

    qdm_kiss_fftr_cfg st = NULL;
    size_t subsize = 0, memneeded;

    if (nfft & 1) {
        KISS_FFT_ERROR("Real FFT optimization must be even.");
        return NULL;
    }
    nfft >>= 1;

#ifdef KISS_FFT_ALLINONE
    qdm_kiss_fft_alloc_ex(nfft, NULL, &subsize, twiddle + nfft / 2);
#else
    qdm_kiss_fft_alloc_ex(nfft, inverse_fft, NULL, &subsize, twiddle + nfft / 2);
#endif
    memneeded = sizeof(struct qdm_kiss_fftr_state) + subsize;

    if (lenmem == NULL) {
        st = (qdm_kiss_fftr_cfg) KISS_FFT_MALLOC (memneeded);
    } else {
        if (*lenmem >= memneeded)
            st = (qdm_kiss_fftr_cfg) mem;
        *lenmem = memneeded;
    }
    if (!st)
        return NULL;

    st->substate = (qdm_kiss_fft_cfg) (st + 1); /*just beyond qdm_kiss_fftr_state struct */
    st->tmpbuf = NULL;
    st->super_twiddles = twiddle;
#ifdef KISS_FFT_ALLINONE
    qdm_kiss_fft_alloc_ex(nfft, st->substate, &subsize, twiddle + nfft/2);
#else
    qdm_kiss_fft_alloc_ex(nfft, inverse_fft, st->substate, &subsize, twiddle + nfft/2);
#endif

    return st;
}

void qdm_kiss_fftr(qdm_kiss_fftr_cfg st,const qdm_kiss_fft_scalar *timedata,qdm_kiss_fft_cpx *freqdata)
{
    /* input buffer timedata is stored row-wise */
    int k,ncfft;
    qdm_kiss_fft_cpx fpnk,fpk,f1k,f2k,tw,tdc;

#ifndef KISS_FFT_ALLINONE
    if ( st->substate->inverse) {
        KISS_FFT_ERROR("kiss fft usage error: improper alloc");
        return;/* The caller did not call the correct function */
    }
#endif

    ncfft = st->substate->nfft;

    /*perform the parallel fft of two real signals packed in real,imag*/
    qdm_kiss_fft( st->substate , (const qdm_kiss_fft_cpx*)timedata, st->tmpbuf );
    /* The real part of the DC element of the frequency spectrum in st->tmpbuf
     * contains the sum of the even-numbered elements of the input time sequence
     * The imag part is the sum of the odd-numbered elements
     *
     * The sum of tdc.r and tdc.i is the sum of the input time sequence.
     *      yielding DC of input time sequence
     * The difference of tdc.r - tdc.i is the sum of the input (dot product) [1,-1,1,-1...
     *      yielding Nyquist bin of input time sequence
     */

    tdc.r = st->tmpbuf[0].r;
    tdc.i = st->tmpbuf[0].i;
    C_FIXDIV(tdc,2);
    CHECK_OVERFLOW_OP(tdc.r ,+, tdc.i);
    CHECK_OVERFLOW_OP(tdc.r ,-, tdc.i);
    freqdata[0].r = tdc.r + tdc.i;
    freqdata[ncfft].r = tdc.r - tdc.i;
#ifdef USE_SIMD
    freqdata[ncfft].i = freqdata[0].i = _mm_set1_ps(0);
#else
    freqdata[ncfft].i = freqdata[0].i = 0;
#endif

    for ( k=1;k <= ncfft/2 ; ++k ) {
        fpk    = st->tmpbuf[k];
        fpnk.r =   st->tmpbuf[ncfft-k].r;
        fpnk.i = - st->tmpbuf[ncfft-k].i;
        C_FIXDIV(fpk,2);
        C_FIXDIV(fpnk,2);

        C_ADD( f1k, fpk , fpnk );
        C_SUB( f2k, fpk , fpnk );
        C_MUL( tw , f2k , st->super_twiddles[k-1]);

        freqdata[k].r = HALF_OF(f1k.r + tw.r);
        freqdata[k].i = HALF_OF(f1k.i + tw.i);
        freqdata[ncfft-k].r = HALF_OF(f1k.r - tw.r);
        freqdata[ncfft-k].i = HALF_OF(tw.i - f1k.i);
    }
}

void KISS_FFT_API qdm_kiss_fftr_ex(qdm_kiss_fftr_cfg cfg,const qdm_kiss_fft_scalar *timedata,qdm_kiss_fft_cpx *freqdata, qdm_kiss_fft_cpx *tmpbuf){
    cfg->tmpbuf = tmpbuf;
    qdm_kiss_fftr(cfg, timedata, freqdata);
}

void qdm_kiss_fftri(qdm_kiss_fftr_cfg st,const qdm_kiss_fft_cpx *freqdata,qdm_kiss_fft_scalar *timedata)
{
    /* input buffer timedata is stored row-wise */
    int k, ncfft;

#ifndef KISS_FFT_ALLINONE
    if (st->substate->inverse == 0) {
        KISS_FFT_ERROR("kiss fft usage error: improper alloc");
        return;/* The caller did not call the correct function */
    }
#endif

    ncfft = st->substate->nfft;

    st->tmpbuf[0].r = freqdata[0].r + freqdata[ncfft].r;
    st->tmpbuf[0].i = freqdata[0].r - freqdata[ncfft].r;
    C_FIXDIV(st->tmpbuf[0],2);

    for (k = 1; k <= ncfft / 2; ++k) {
        qdm_kiss_fft_cpx fk, fnkc, fek, fok, tmp;
        fk = freqdata[k];
        fnkc.r = freqdata[ncfft - k].r;
        fnkc.i = -freqdata[ncfft - k].i;
        C_FIXDIV( fk , 2 );
        C_FIXDIV( fnkc , 2 );

        C_ADD (fek, fk, fnkc);
        C_SUB (tmp, fk, fnkc);
#ifdef KISS_FFT_ALLINONE
        C_CONJ_MUL (fok, tmp, st->super_twiddles[k-1]);
#else
        C_MUL (fok, tmp, st->super_twiddles[k-1]);
#endif
        C_ADD (st->tmpbuf[k],     fek, fok);
        C_SUB (st->tmpbuf[ncfft - k], fek, fok);
#ifdef USE_SIMD
        st->tmpbuf[ncfft - k].i *= _mm_set1_ps(-1.0);
#else
        st->tmpbuf[ncfft - k].i *= -1;
#endif
    }
#ifdef KISS_FFT_ALLINONE
    kiss_ifft (st->substate, st->tmpbuf, (qdm_kiss_fft_cpx *) timedata);
#else
    qdm_kiss_fft (st->substate, st->tmpbuf, (qdm_kiss_fft_cpx *) timedata);
#endif
}


void KISS_FFT_API qdm_kiss_fftri_ex(qdm_kiss_fftr_cfg cfg,const qdm_kiss_fft_cpx *freqdata,qdm_kiss_fft_scalar *timedata, qdm_kiss_fft_cpx *tmpbuf)
{
    cfg->tmpbuf = tmpbuf;
    qdm_kiss_fftri(cfg, freqdata, timedata);
}
