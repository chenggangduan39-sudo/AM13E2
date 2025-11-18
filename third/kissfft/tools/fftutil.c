/*
 *  Copyright (c) 2003-2004, Mark Borgerding. All rights reserved.
 *  This file is part of KISS FFT - https://github.com/mborgerding/kissfft
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  See COPYING file for more information.
 */

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "qdm_kiss_fft.h"
#include "qdm_kiss_fftndr.h"

static
void fft_file(FILE * fin,FILE * fout,int nfft,int isinverse)
{
    qdm_kiss_fft_cfg st;
    qdm_kiss_fft_cpx * buf;
    qdm_kiss_fft_cpx * bufout;

    buf = (qdm_kiss_fft_cpx*)malloc(sizeof(qdm_kiss_fft_cpx) * nfft );
    bufout = (qdm_kiss_fft_cpx*)malloc(sizeof(qdm_kiss_fft_cpx) * nfft );
#ifdef KISS_FFT_ALLINONE
    st = qdm_kiss_fft_alloc( nfft,0,0);
#else
    st = qdm_kiss_fft_alloc( nfft ,isinverse ,0,0);
#endif

    while ( fread( buf , sizeof(qdm_kiss_fft_cpx) * nfft ,1, fin ) > 0 ) {
#ifdef KISS_FFT_ALLINONE
        if (isinverse) {
            kiss_ifft( st , buf ,bufout);
        } else {
            qdm_kiss_fft( st , buf ,bufout);
        }
#else
        qdm_kiss_fft( st , buf ,bufout);
#endif
        fwrite( bufout , sizeof(qdm_kiss_fft_cpx) , nfft , fout );
    }
    free(st);
    free(buf);
    free(bufout);
}

static
void fft_filend(FILE * fin,FILE * fout,int *dims,int ndims,int isinverse)
{
    qdm_kiss_fftnd_cfg st;
    qdm_kiss_fft_cpx *buf;
    int dimprod=1,i;
    for (i=0;i<ndims;++i) 
        dimprod *= dims[i];

    buf = (qdm_kiss_fft_cpx *) malloc (sizeof (qdm_kiss_fft_cpx) * dimprod);
#ifdef KISS_FFT_ALLINONE
    st = qdm_kiss_fftnd_alloc (dims, ndims, 0, 0);
#else
    st = qdm_kiss_fftnd_alloc (dims, ndims, isinverse, 0, 0);
#endif

    while (fread (buf, sizeof (qdm_kiss_fft_cpx) * dimprod, 1, fin) > 0) {
#ifdef KISS_FFT_ALLINONE
        if (isinverse) {
            kiss_ifftnd (st, buf, buf);
        } else {
            qdm_kiss_fftnd (st, buf, buf);
        }
#else
        qdm_kiss_fftnd (st, buf, buf);
#endif
        fwrite (buf, sizeof (qdm_kiss_fft_cpx), dimprod, fout);
    }
    free (st);
    free (buf);
}



static
void fft_filend_real(FILE * fin,FILE * fout,int *dims,int ndims,int isinverse)
{
    int dimprod=1,i;
    qdm_kiss_fftndr_cfg st;
    void *ibuf;
    void *obuf;
    int insize,outsize; // size in bytes

    for (i=0;i<ndims;++i) 
        dimprod *= dims[i];
    insize = outsize = dimprod;
    int rdim = dims[ndims-1];

    if (isinverse)
        insize = insize*2*(rdim/2+1)/rdim;
    else
        outsize = outsize*2*(rdim/2+1)/rdim;

    ibuf = malloc(insize*sizeof(qdm_kiss_fft_scalar));
    obuf = malloc(outsize*sizeof(qdm_kiss_fft_scalar));

#ifdef KISS_FFT_ALLINONE
    st = qdm_kiss_fftndr_alloc(dims, ndims, 0, 0);
#else
    st = qdm_kiss_fftndr_alloc(dims, ndims, isinverse, 0, 0);
#endif

    while ( fread (ibuf, sizeof(qdm_kiss_fft_scalar), insize,  fin) > 0) {
        if (isinverse) {
            qdm_kiss_fftndri(st,
                    (qdm_kiss_fft_cpx*)ibuf,
                    (qdm_kiss_fft_scalar*)obuf);
        }else{
            qdm_kiss_fftndr(st,
                    (qdm_kiss_fft_scalar*)ibuf,
                    (qdm_kiss_fft_cpx*)obuf);
        }
        fwrite (obuf, sizeof(qdm_kiss_fft_scalar), outsize,fout);
    }
    free(st);
    free(ibuf);
    free(obuf);
}

static
void fft_file_real(FILE * fin,FILE * fout,int nfft,int isinverse)
{
    qdm_kiss_fftr_cfg st;
    qdm_kiss_fft_scalar * rbuf;
    qdm_kiss_fft_cpx * cbuf;

    rbuf = (qdm_kiss_fft_scalar*)malloc(sizeof(qdm_kiss_fft_scalar) * nfft );
    cbuf = (qdm_kiss_fft_cpx*)malloc(sizeof(qdm_kiss_fft_cpx) * (nfft/2+1) );
#ifdef KISS_FFT_ALLINONE
    st = qdm_kiss_fftr_alloc( nfft ,0,0);
#else
    st = qdm_kiss_fftr_alloc( nfft ,isinverse ,0,0);
#endif

    if (isinverse==0) {
        while ( fread( rbuf , sizeof(qdm_kiss_fft_scalar) * nfft ,1, fin ) > 0 ) {
            qdm_kiss_fftr( st , rbuf ,cbuf);
            fwrite( cbuf , sizeof(qdm_kiss_fft_cpx) , (nfft/2 + 1) , fout );
        }
    }else{
        while ( fread( cbuf , sizeof(qdm_kiss_fft_cpx) * (nfft/2+1) ,1, fin ) > 0 ) {
            qdm_kiss_fftri( st , cbuf ,rbuf);
            fwrite( rbuf , sizeof(qdm_kiss_fft_scalar) , nfft , fout );
        }
    }
    free(st);
    free(rbuf);
    free(cbuf);
}

static
int get_dims(char * arg,int * dims)
{
    char *p0;
    int ndims=0;

    do{
        p0 = strchr(arg,',');
        if (p0)
            *p0++ = '\0';
        dims[ndims++] = atoi(arg);
//         fprintf(stderr,"dims[%d] = %d\n",ndims-1,dims[ndims-1]); 
        arg = p0;
    }while (p0);
    return ndims;
}

int main(int argc,char ** argv)
{
    int isinverse=0;
    int isreal=0;
    FILE *fin=stdin;
    FILE *fout=stdout;
    int ndims=1;
    int dims[32];
    dims[0] = 1024; /*default fft size*/

    while (1) {
        int c=getopt(argc,argv,"n:iR");
        if (c==-1) break;
        switch (c) {
            case 'n':
                ndims = get_dims(optarg,dims);
                break;
            case 'i':isinverse=1;break;
            case 'R':isreal=1;break;
            case '?':
                     fprintf(stderr,"usage options:\n"
                            "\t-n d1[,d2,d3...]: fft dimension(s)\n"
                            "\t-i : inverse\n"
                            "\t-R : real input samples, not complex\n");
                     exit (1);
            default:fprintf(stderr,"bad %c\n",c);break;
        }
    }

    if ( optind < argc ) {
        if (strcmp("-",argv[optind]) !=0)
            fin = fopen(argv[optind],"rb");
        ++optind;
    }

    if ( optind < argc ) {
        if ( strcmp("-",argv[optind]) !=0 ) 
            fout = fopen(argv[optind],"wb");
        ++optind;
    }

    if (ndims==1) {
        if (isreal)
            fft_file_real(fin,fout,dims[0],isinverse);
        else
            fft_file(fin,fout,dims[0],isinverse);
    }else{
        if (isreal)
            fft_filend_real(fin,fout,dims,ndims,isinverse);
        else
            fft_filend(fin,fout,dims,ndims,isinverse);
    }

    if (fout!=stdout) fclose(fout);
    if (fin!=stdin) fclose(fin);

    return 0;
}
