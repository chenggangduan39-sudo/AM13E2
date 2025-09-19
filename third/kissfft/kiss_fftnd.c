/*
 *  Copyright (c) 2003-2004, Mark Borgerding. All rights reserved.
 *  This file is part of KISS FFT - https://github.com/mborgerding/kissfft
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  See COPYING file for more information.
 */

#include "qdm_kiss_fftnd.h"
#include "_qdm_kiss_fft_guts.h"

struct qdm_kiss_fftnd_state{
    int dimprod; /* dimsum would be mighty tasty right now */
    int ndims;
    int *dims;
    qdm_kiss_fft_cfg *states; /* cfg states for each dimension */
    qdm_kiss_fft_cpx * tmpbuf; /*buffer capable of hold the entire input */
};

#ifdef KISS_FFT_ALLINONE
qdm_kiss_fftnd_cfg qdm_kiss_fftnd_alloc(const int *dims,int ndims,void*mem,size_t*lenmem)
#else
qdm_kiss_fftnd_cfg qdm_kiss_fftnd_alloc(const int *dims,int ndims,int inverse_fft,void*mem,size_t*lenmem)
#endif
{
    KISS_FFT_ALIGN_CHECK(mem)

    qdm_kiss_fftnd_cfg st = NULL;
    int i;
    int dimprod=1;
    size_t memneeded = KISS_FFT_ALIGN_SIZE_UP(sizeof(struct qdm_kiss_fftnd_state));
    char * ptr = NULL;

    for (i=0;i<ndims;++i) {
        size_t sublen=0;
#ifdef KISS_FFT_ALLINONE
        qdm_kiss_fft_alloc (dims[i], NULL, &sublen);
#else
        qdm_kiss_fft_alloc (dims[i], inverse_fft, NULL, &sublen);
#endif
        memneeded += sublen;   /* st->states[i] */
        dimprod *= dims[i];
    }
    memneeded += KISS_FFT_ALIGN_SIZE_UP(sizeof(int) * ndims);/*  st->dims */
    memneeded += KISS_FFT_ALIGN_SIZE_UP(sizeof(void*) * ndims);/* st->states  */
    memneeded += KISS_FFT_ALIGN_SIZE_UP(sizeof(qdm_kiss_fft_cpx) * dimprod); /* st->tmpbuf */

    if (lenmem == NULL) {/* allocate for the caller*/
        ptr = (char *) malloc (memneeded);
    } else { /* initialize supplied buffer if big enough */
        if (*lenmem >= memneeded)
            ptr = (char *) mem;
        *lenmem = memneeded; /*tell caller how big struct is (or would be) */
    }
    if (!ptr)
        return NULL; /*malloc failed or buffer too small */

    st = (qdm_kiss_fftnd_cfg) ptr;
    st->dimprod = dimprod;
    st->ndims = ndims;
    ptr += KISS_FFT_ALIGN_SIZE_UP(sizeof(struct qdm_kiss_fftnd_state));

    st->states = (qdm_kiss_fft_cfg *)ptr;
    ptr += KISS_FFT_ALIGN_SIZE_UP(sizeof(void*) * ndims);

    st->dims = (int*)ptr;
    ptr += KISS_FFT_ALIGN_SIZE_UP(sizeof(int) * ndims);

    st->tmpbuf = (qdm_kiss_fft_cpx*)ptr;
    ptr += KISS_FFT_ALIGN_SIZE_UP(sizeof(qdm_kiss_fft_cpx) * dimprod);

    for (i=0;i<ndims;++i) {
        size_t len;
        st->dims[i] = dims[i];
#ifdef KISS_FFT_ALLINONE
        qdm_kiss_fft_alloc (st->dims[i], NULL, &len);
        st->states[i] = qdm_kiss_fft_alloc (st->dims[i], ptr,&len);
#else
        qdm_kiss_fft_alloc (st->dims[i], inverse_fft, NULL, &len);
        st->states[i] = qdm_kiss_fft_alloc (st->dims[i], inverse_fft, ptr,&len);
#endif
        ptr += len;
    }
    /*
Hi there!

If you're looking at this particular code, it probably means you've got a brain-dead bounds checker
that thinks the above code overwrites the end of the array.

It doesn't.

-- Mark

P.S.
The below code might give you some warm fuzzies and help convince you.
       */
    if ( ptr - (char*)st != (int)memneeded ) {
        fprintf(stderr,
                "################################################################################\n"
                "Internal error! Memory allocation miscalculation\n"
                "################################################################################\n"
               );
    }
    return st;
}

/*
 This works by tackling one dimension at a time.

 In effect,
 Each stage starts out by reshaping the matrix into a DixSi 2d matrix.
 A Di-sized fft is taken of each column, transposing the matrix as it goes.

Here's a 3-d example:
Take a 2x3x4 matrix, laid out in memory as a contiguous buffer
 [ [ [ a b c d ] [ e f g h ] [ i j k l ] ]
   [ [ m n o p ] [ q r s t ] [ u v w x ] ] ]

Stage 0 ( D=2): treat the buffer as a 2x12 matrix
   [ [a b ... k l]
     [m n ... w x] ]

   FFT each column with size 2.
   Transpose the matrix at the same time using qdm_kiss_fft_stride.

   [ [ a+m a-m ]
     [ b+n b-n]
     ...
     [ k+w k-w ]
     [ l+x l-x ] ]

   Note fft([x y]) == [x+y x-y]

Stage 1 ( D=3) treats the buffer (the output of stage D=2) as an 3x8 matrix,
   [ [ a+m a-m b+n b-n c+o c-o d+p d-p ] 
     [ e+q e-q f+r f-r g+s g-s h+t h-t ]
     [ i+u i-u j+v j-v k+w k-w l+x l-x ] ]

   And perform FFTs (size=3) on each of the columns as above, transposing 
   the matrix as it goes.  The output of stage 1 is 
       (Legend: ap = [ a+m e+q i+u ]
                am = [ a-m e-q i-u ] )
   
   [ [ sum(ap) fft(ap)[0] fft(ap)[1] ]
     [ sum(am) fft(am)[0] fft(am)[1] ]
     [ sum(bp) fft(bp)[0] fft(bp)[1] ]
     [ sum(bm) fft(bm)[0] fft(bm)[1] ]
     [ sum(cp) fft(cp)[0] fft(cp)[1] ]
     [ sum(cm) fft(cm)[0] fft(cm)[1] ]
     [ sum(dp) fft(dp)[0] fft(dp)[1] ]
     [ sum(dm) fft(dm)[0] fft(dm)[1] ]  ]

Stage 2 ( D=4) treats this buffer as a 4*6 matrix,
   [ [ sum(ap) fft(ap)[0] fft(ap)[1] sum(am) fft(am)[0] fft(am)[1] ]
     [ sum(bp) fft(bp)[0] fft(bp)[1] sum(bm) fft(bm)[0] fft(bm)[1] ]
     [ sum(cp) fft(cp)[0] fft(cp)[1] sum(cm) fft(cm)[0] fft(cm)[1] ]
     [ sum(dp) fft(dp)[0] fft(dp)[1] sum(dm) fft(dm)[0] fft(dm)[1] ]  ]

   Then FFTs each column, transposing as it goes.

   The resulting matrix is the 3d FFT of the 2x3x4 input matrix.

   Note as a sanity check that the first element of the final 
   stage's output (DC term) is 
   sum( [ sum(ap) sum(bp) sum(cp) sum(dp) ] )
   , i.e. the summation of all 24 input elements. 

*/
void qdm_kiss_fftnd(qdm_kiss_fftnd_cfg st,const qdm_kiss_fft_cpx *fin,qdm_kiss_fft_cpx *fout)
{
    int i,k;
    const qdm_kiss_fft_cpx * bufin=fin;
    qdm_kiss_fft_cpx * bufout;

    /*arrange it so the last bufout == fout*/
    if ( st->ndims & 1 ) {
        bufout = fout;
        if (fin==fout) {
            memcpy( st->tmpbuf, fin, sizeof(qdm_kiss_fft_cpx) * st->dimprod );
            bufin = st->tmpbuf;
        }
    }else
        bufout = st->tmpbuf;

    for ( k=0; k < st->ndims; ++k) {
        int curdim = st->dims[k];
        int stride = st->dimprod / curdim;

        for ( i=0 ; i<stride ; ++i ) 
            qdm_kiss_fft_stride( st->states[k], bufin+i , bufout+i*curdim, stride );

        /*toggle back and forth between the two buffers*/
        if (bufout == st->tmpbuf){
            bufout = fout;
            bufin = st->tmpbuf;
        }else{
            bufout = st->tmpbuf;
            bufin = fout;
        }
    }
}

#ifdef KISS_FFT_ALLINONE
void kiss_ifftnd(qdm_kiss_fftnd_cfg st,const qdm_kiss_fft_cpx *fin,qdm_kiss_fft_cpx *fout)
{
    int i,k;
    const qdm_kiss_fft_cpx * bufin=fin;
    qdm_kiss_fft_cpx * bufout;

    /*arrange it so the last bufout == fout*/
    if ( st->ndims & 1 ) {
        bufout = fout;
        if (fin==fout) {
            memcpy( st->tmpbuf, fin, sizeof(qdm_kiss_fft_cpx) * st->dimprod );
            bufin = st->tmpbuf;
        }
    }else
        bufout = st->tmpbuf;

    for ( k=0; k < st->ndims; ++k) {
        int curdim = st->dims[k];
        int stride = st->dimprod / curdim;

        for ( i=0 ; i<stride ; ++i )
            kiss_ifft_stride( st->states[k], bufin+i , bufout+i*curdim, stride );

        /*toggle back and forth between the two buffers*/
        if (bufout == st->tmpbuf){
            bufout = fout;
            bufin = st->tmpbuf;
        }else{
            bufout = st->tmpbuf;
            bufin = fout;
        }
    }
}
#endif
