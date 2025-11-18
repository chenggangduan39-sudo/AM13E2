/*
 *  Copyright (c) 2003-2010, Mark Borgerding. All rights reserved.
 *  This file is part of KISS FFT - https://github.com/mborgerding/kissfft
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  See COPYING file for more information.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/times.h>
#include <unistd.h>
#include "qdm_kiss_fft.h"
#include "qdm_kiss_fftr.h"
#include "qdm_kiss_fftnd.h"
#include "qdm_kiss_fftndr.h"

#include "pstats.h"

static
int getdims(int * dims, char * arg)
{
    char *s;
    int ndims=0;
    while ( (s=strtok( arg , ",") ) ) {
        dims[ndims++] = atoi(s);
        //printf("%s=%d\n",s,dims[ndims-1]);
        arg=NULL;
    }
    return ndims;
}

int main(int argc,char ** argv)
{
    int k;
    int nfft[32];
    int ndims = 1;
    int isinverse=0;
    int numffts=1000,i;
    qdm_kiss_fft_cpx * buf;
    qdm_kiss_fft_cpx * bufout;
    int real = 0;

    nfft[0] = 1024;// default

    while (1) {
        int c = getopt (argc, argv, "n:ix:r");
        if (c == -1)
            break;
        switch (c) {
            case 'r':
                real = 1;
                break;
            case 'n':
                ndims = getdims(nfft, optarg );
                if (nfft[0] != qdm_kiss_fft_next_fast_size(nfft[0]) ) {
                    int ng = qdm_kiss_fft_next_fast_size(nfft[0]);
                    fprintf(stderr,"warning: %d might be a better choice for speed than %d\n",ng,nfft[0]);
                }
                break;
            case 'x':
                numffts = atoi (optarg);
                break;
            case 'i':
                isinverse = 1;
                break;
        }
    }
    int nbytes = sizeof(qdm_kiss_fft_cpx);
    for (k=0;k<ndims;++k)
        nbytes *= nfft[k];

#ifdef USE_SIMD        
    numffts /= 4;
    fprintf(stderr,"since SIMD implementation does 4 ffts at a time, numffts is being reduced to %d\n",numffts);
#endif

    buf=(qdm_kiss_fft_cpx*)KISS_FFT_MALLOC(nbytes);
    bufout=(qdm_kiss_fft_cpx*)KISS_FFT_MALLOC(nbytes);
    memset(buf,0,nbytes);

    pstats_init();

    if (ndims==1) {
        if (real) {
#ifdef KISS_FFT_ALLINONE
            qdm_kiss_fftr_cfg st = qdm_kiss_fftr_alloc( nfft[0] ,0,0);
#else
            qdm_kiss_fftr_cfg st = qdm_kiss_fftr_alloc( nfft[0] ,isinverse ,0,0);
#endif
            if (isinverse)
                for (i=0;i<numffts;++i)
                    qdm_kiss_fftri( st ,(qdm_kiss_fft_cpx*)buf,(qdm_kiss_fft_scalar*)bufout );
            else
                for (i=0;i<numffts;++i)
                    qdm_kiss_fftr( st ,(qdm_kiss_fft_scalar*)buf,(qdm_kiss_fft_cpx*)bufout );
            free(st);
        }else{
#ifdef KISS_FFT_ALLINONE
            qdm_kiss_fft_cfg st = qdm_kiss_fft_alloc( nfft[0] ,0,0);
            if (isinverse) {
                for (i=0;i<numffts;++i)
                    kiss_ifft( st ,buf,bufout );
            } else {
                for (i=0;i<numffts;++i)
                    qdm_kiss_fft( st ,buf,bufout );
            }
#else
            qdm_kiss_fft_cfg st = qdm_kiss_fft_alloc( nfft[0] ,isinverse ,0,0);
            for (i=0;i<numffts;++i)
                qdm_kiss_fft( st ,buf,bufout );
#endif
            free(st);
        }
    }else{
        if (real) {
#ifdef KISS_FFT_ALLINONE
            qdm_kiss_fftndr_cfg st = qdm_kiss_fftndr_alloc( nfft,ndims,0,0);
#else
            qdm_kiss_fftndr_cfg st = qdm_kiss_fftndr_alloc( nfft,ndims ,isinverse ,0,0);
#endif
            if (isinverse)
                for (i=0;i<numffts;++i)
                    qdm_kiss_fftndri( st ,(qdm_kiss_fft_cpx*)buf,(qdm_kiss_fft_scalar*)bufout );
            else
                for (i=0;i<numffts;++i)
                    qdm_kiss_fftndr( st ,(qdm_kiss_fft_scalar*)buf,(qdm_kiss_fft_cpx*)bufout );
            free(st);
        }else{
#ifdef KISS_FFT_ALLINONE
            qdm_kiss_fftnd_cfg st= qdm_kiss_fftnd_alloc(nfft,ndims,0,0);
            if (isinverse) {
                for (i=0;i<numffts;++i)
                    kiss_ifftnd( st ,buf,bufout );
            } else {
                for (i=0;i<numffts;++i)
                    qdm_kiss_fftnd( st ,buf,bufout );
            }
#else
            qdm_kiss_fftnd_cfg st= qdm_kiss_fftnd_alloc(nfft,ndims,isinverse ,0,0);
            for (i=0;i<numffts;++i)
                qdm_kiss_fftnd( st ,buf,bufout );
#endif
            free(st);
        }
    }

    free(buf); free(bufout);

    fprintf(stderr,"KISS\tnfft=");
    for (k=0;k<ndims;++k)
        fprintf(stderr, "%d,",nfft[k]);
    fprintf(stderr,"\tnumffts=%d\n" ,numffts);
    pstats_report();

    qdm_kiss_fft_cleanup();

    return 0;
}

