#include "wtk_fixfft.h" 
#include "wtk/core/wtk_str.h"
#include "wtk/core/wtk_alloc.h"

#define K1 8192
#define K2 -4096
#define K3 340
#define K4 -10

short wtk_fix_cos(short x)
{
   short x2;

   if (x<12868)
   {
      x2 = MULT16_16_P13(x,x);
      return ADD32(K1, MULT16_16_P13(x2, ADD32(K2, MULT16_16_P13(x2, ADD32(K3, MULT16_16_P13(K4, x2))))));
   } else {
      x = SUB16(25736,x);
      x2 = MULT16_16_P13(x,x);
      return SUB32(-K1, MULT16_16_P13(x2, ADD32(K2, MULT16_16_P13(x2, ADD32(K3, MULT16_16_P13(K4, x2))))));
   }
}

short wtk_fixnlms_ilog4(unsigned int x)
{
   int r=0;

   if (x>=(int)65536)
   {
      x >>= 16;
      r += 8;
   }
   if (x>=256)
   {
      x >>= 8;
      r += 4;
   }
   if (x>=16)
   {
      x >>= 4;
      r += 2;
   }
   if (x>=4)
   {
      r += 1;
   }
   return r;
}

/* sqrt(x) ~= 0.22178 + 1.29227*x - 0.77070*x^2 + 0.25659*x^3 (for .25 < x < 1) */
#define C0 3634
#define C1 21173
#define C2 -12627
#define C3 4204

short wtk_fix_sqrt(int x)
{
   int k;
   int  rt;

   k = wtk_fixnlms_ilog4(x)-6;
   x = VSHR32(x, (k<<1));
   rt = ADD16(C0, MULT16_16_Q14(x, ADD16(C1, MULT16_16_Q14(x, ADD16(C2, MULT16_16_Q14(x, (C3)))))));
   rt = VSHR32(rt,7-k);
   return rt;
}

short wtk_fix_ilogl(wtk_fix64_t x)
{
   int r=0;

   if (x>=(wtk_fix64_t)4294967296)
   {
      x >>= 32;
      r += 32;
   }
   if (x>=(int)65536)
   {
      x >>= 16;
      r += 16;
   }
   if (x>=256)
   {
      x >>= 8;
      r += 8;
   }
   if (x>=16)
   {
      x >>= 4;
      r += 4;
   }
   if (x>=4)
   {
      x >>= 2;
      r += 2;
   }
   if (x>=2)
   {
      r += 1;
   }
   return r;
}



static inline wtk_fix16_t _spx_cos_pi_2(wtk_fix16_t x)
{
	wtk_fix16_t x2;

   x2 = MULT16_16_P15(x,x);
   return ADD16(1,MIN16(32766,ADD32(SUB16(L1,x2), MULT16_16_P15(x2, ADD32(L2, MULT16_16_P15(x2, ADD32(L3, MULT16_16_P15(L4, x2))))))));
}

static inline wtk_fix16_t spx_cos_norm(wtk_fix32_t x)
{
   x = x&0x0001ffff;
   if (x>SHL32(EXTEND32(1), 16))
   {
      x = SUB32(SHL32(EXTEND32(1), 17),x);
   }
   if (x&0x00007fff)
   {
      if (x<SHL32(EXTEND32(1), 15))
      {
         return _spx_cos_pi_2(EXTRACT16(x));
      } else
      {
         return NEG32(_spx_cos_pi_2(EXTRACT16(65536-x)));
      }
   } else
   {
      if (x&0x0000ffff)
      {
         return 0;
      }else if (x&0x0001ffff)
      {
         return -32767;
      }else
      {
         return 32767;
      }
   }
}

static void kf_factor(int n,int * facbuf)
{
    int p=4;

    /*factor out powers of 4, powers of 2, then any remaining primes */
    do {
        while (n % p) {
            switch (p) {
                case 4: p = 2; break;
                case 2: p = 3; break;
                default: p += 2; break;
            }
            if (p>32000 || (wtk_fix32_t)p*(wtk_fix32_t)p > n)
                p = n;          /* no more factors, skip to end */
        }
        n /= p;
        *facbuf++ = p;
        *facbuf++ = n;
    } while (n > 1);
}


int wtk_fixfft_substate_bytes(wtk_fixfft_substate_t *s)
{
	int v;

	v=sizeof(wtk_fixfft_substate_t);
	v+=s->nfft*sizeof(wtk_fixcpx_t);
	return v;
}

wtk_fixfft_substate_t* wtk_fixfft_substate_new(int nfft,int inverse)
{
	wtk_fixfft_substate_t *st;
	int i,phase;

	st=(wtk_fixfft_substate_t*)wtk_malloc(sizeof(wtk_fixfft_substate_t));
	nfft=nfft/2;
	st->nfft=nfft;
	st->inverse=inverse;
	st->twiddles=(wtk_fixcpx_t*)wtk_calloc(nfft,sizeof(wtk_fixcpx_t));
	for(i=0;i<nfft;++i)
	{
		if(st->inverse)
		{
			phase=i;
		}else
		{
			phase=-i;
		}
		phase=DIV32(SHL32(phase,17),nfft);
		st->twiddles[i].r=spx_cos_norm(phase);
		st->twiddles[i].i=spx_cos_norm(phase-32768);
		//wtk_debug("v[%d]=%d+j%d phase=%d\n",i,st->twiddles[i].r,st->twiddles[i].i,phase);
	}
	//exit(0);
    kf_factor(nfft,st->factors);
	return st;
}


void wtk_fixfft_substate_delete(wtk_fixfft_substate_t *st)
{
	wtk_free(st->twiddles);
	wtk_free(st);
}

int wtk_fixfft_state_bytes(wtk_fixfft_state_t *s)
{
	int v;

	v=sizeof(wtk_fixfft_state_t);
	v+=s->nfft*sizeof(wtk_fixcpx_t)*2;
	v+=wtk_fixfft_substate_bytes(s->substate);
	return v;
}

wtk_fixfft_state_t* wtk_fixfft_state_new(int nfft,int inverse_fft)
{
	wtk_fixfft_state_t *st;
	int i;

	st=(wtk_fixfft_state_t*)wtk_malloc(sizeof(wtk_fixfft_state_t));
	st->nfft=nfft;
	st->substate=wtk_fixfft_substate_new(nfft,inverse_fft);
	st->tmpbuf=(wtk_fixcpx_t*)wtk_calloc(nfft,sizeof(wtk_fixcpx_t));
	st->super_twiddles=(wtk_fixcpx_t*)wtk_calloc(nfft,sizeof(wtk_fixcpx_t));
	nfft/=2;
    for (i=0;i<nfft;++i)
    {
       int phase = i+(nfft>>1);
       if (!inverse_fft)
       {
          phase = -phase;
       }
       ///kf_cexp2(st->super_twiddles+i, DIV32(SHL32(phase,16),nfft));
       phase=DIV32(SHL32(phase,16),nfft);
		st->super_twiddles[i].r=spx_cos_norm(phase);
		st->super_twiddles[i].i=spx_cos_norm(phase-32768);
		//wtk_debug("v[%d]=%d+j%d\n",i,st->super_twiddles[i].r,st->super_twiddles[i].i);
    }
   // exit(0);
	return st;
}

void  wtk_fixfft_state_delete(wtk_fixfft_state_t *st)
{
	wtk_fixfft_substate_delete(st->substate);
	wtk_free(st->super_twiddles);
	wtk_free(st->tmpbuf);
	wtk_free(st);
}

wtk_fixfft_t* wtk_fixfft_new(int n)
{
	wtk_fixfft_t *fft;

	fft=(wtk_fixfft_t*)wtk_malloc(sizeof(wtk_fixfft_t));
	fft->N=n;
	fft->forward=wtk_fixfft_state_new(n,0);
	fft->backward=wtk_fixfft_state_new(n,1);
	return fft;
}

int wtk_fixfft_bytes(wtk_fixfft_t *f)
{
	int v;

	v=sizeof(wtk_fixfft_t);
	v+=wtk_fixfft_state_bytes(f->forward);
	v+=wtk_fixfft_state_bytes(f->backward);
	return v;
}

void wtk_fixfft_delete(wtk_fixfft_t *fft)
{
	wtk_fixfft_state_delete(fft->forward);
	wtk_fixfft_state_delete(fft->backward);
	wtk_free(fft);
}

int wtk_fixfft_range(short *in,short *out,int bound,int len)
{
	short max_val=0;
	int i,shift;

	for(i=0;i<len;++i)
	{
		//max_val=in[i]>max_val?in[i]:((-in[i]>max_val)?in[i]:max_val);
		if(in[i]>max_val)
		{
			max_val=in[i];
		}else if(-in[i]>max_val)
		{
			max_val=-in[i];
		}
	}
	//wtk_debug("max_val=%d\n",max_val);
	//exit(0);
	shift=0;
	bound>>=1;
	while(max_val<=(bound) && max_val!=0)
	{
		max_val<<=1;
		shift++;
	}
	for(i=0;i<len;++i)
	{
		out[i]=in[i]<<shift;
	}
	return shift;
}


void wtk_fixfft_shuffle(wtk_fixcpx_t *Fout,wtk_fixcpx_t *f,int fstride,int in_stride,int * factors)
{
   int p=*factors++; /* the radix  */
   int m=*factors++; /* stage's fft length/p */
   int v;
   int j;

   v=fstride*in_stride;
   //wtk_debug("v=%d %d+j%d stride=%d/%d\n",v,f->r,f->i,fstride,in_stride);
   if (m==1)
   {
      for (j=0;j<p;j++)
      {
         Fout[j] = *f;
         f += v;//fstride*in_stride;
      }
   }else
   {
      for (j=0;j<p;j++)
      {
    	  wtk_fixfft_shuffle( Fout , f, fstride*p, in_stride, factors);
         f += v;//fstride*in_stride;
         Fout += m;
      }
   }
}

# define FRACBITS 15
# define SAMPPROD int
#define SAMP_MAX 32767

#   define smul(a,b) ( (SAMPPROD)(a)*(b) )
#   define sround( x )  (short)( ( (x) + (1<<(FRACBITS-1)) ) >> FRACBITS )

#   define C_MUL(m,a,b) \
      do{ (m).r = sround( smul((a).r,(b).r) - smul((a).i,(b).i) ); \
          (m).i = sround( smul((a).r,(b).i) + smul((a).i,(b).r) ); }while(0)

#ifndef CHECK_OVERFLOW_OP
#  define CHECK_OVERFLOW_OP(a,op,b) /* noop */
#endif

#define  C_SUB( res, a,b)\
    do { \
	    CHECK_OVERFLOW_OP((a).r,-,(b).r)\
	    CHECK_OVERFLOW_OP((a).i,-,(b).i)\
	    (res).r=(a).r-(b).r;  (res).i=(a).i-(b).i; \
    }while(0)

#define C_ADDTO( res , a)\
    do { \
	    CHECK_OVERFLOW_OP((res).r,+,(a).r)\
	    CHECK_OVERFLOW_OP((res).i,+,(a).i)\
	    (res).r += (a).r;  (res).i += (a).i;\
    }while(0)

#define  C_ADD( res, a,b)\
    do { \
	    CHECK_OVERFLOW_OP((a).r,+,(b).r)\
	    CHECK_OVERFLOW_OP((a).i,+,(b).i)\
	    (res).r=(a).r+(b).r;  (res).i=(a).i+(b).i; \
    }while(0)

#   define C_MUL4(m,a,b) \
               do{ (m).r = PSHR32( smul((a).r,(b).r) - smul((a).i,(b).i),17 ); \
               (m).i = PSHR32( smul((a).r,(b).i) + smul((a).i,(b).r),17 ); }while(0)

#   define DIVSCALAR(x,k) \
	(x) = sround( smul(  x, SAMP_MAX/k ) )

#  define HALF_OF(x) ((x)>>1)

#   define C_FIXDIV(c,div) \
	do {    DIVSCALAR( (c).r , div);  \
		DIVSCALAR( (c).i  , div); }while (0)

#   define C_MULBYSCALAR( c, s ) \
    do{ (c).r =  sround( smul( (c).r , s ) ) ;\
        (c).i =  sround( smul( (c).i , s ) ) ; }while(0)

#   define S_MUL(a,b) sround( smul(a,b) )

void wtk_fixfft_bfly2(
		wtk_fixfft_substate_t *st,
		wtk_fixcpx_t * Fout,
        int fstride,
        int m,
        int N,
        int mm
        )
{
	wtk_fixcpx_t * Fout2;
	wtk_fixcpx_t * tw1;
	wtk_fixcpx_t t;
    if (!st->inverse){
       int i,j;
       wtk_fixcpx_t * Fout_beg = Fout;
       for (i=0;i<N;i++)
       {
          Fout = Fout_beg + i*mm;
          Fout2 = Fout + m;
          tw1 = st->twiddles;
          for(j=0;j<m;j++)
          {
             /* Almost the same as the code path below, except that we divide the input by two
              (while keeping the best accuracy possible) */
             int tr, ti;
             tr = SHR32(SUB32(MULT16_16(Fout2->r , tw1->r),MULT16_16(Fout2->i , tw1->i)), 1);
             ti = SHR32(ADD32(MULT16_16(Fout2->i , tw1->r),MULT16_16(Fout2->r , tw1->i)), 1);
             tw1 += fstride;
             Fout2->r = PSHR32(SUB32(SHL32(EXTEND32(Fout->r), 14), tr), 15);
             Fout2->i = PSHR32(SUB32(SHL32(EXTEND32(Fout->i), 14), ti), 15);
             Fout->r = PSHR32(ADD32(SHL32(EXTEND32(Fout->r), 14), tr), 15);
             Fout->i = PSHR32(ADD32(SHL32(EXTEND32(Fout->i), 14), ti), 15);
             ++Fout2;
             ++Fout;
          }
       }
    } else {
       int i,j;
       wtk_fixcpx_t * Fout_beg = Fout;
       for (i=0;i<N;i++)
       {
          Fout = Fout_beg + i*mm;
          Fout2 = Fout + m;
          tw1 = st->twiddles;
          for(j=0;j<m;j++)
          {
             C_MUL (t,  *Fout2 , *tw1);
             tw1 += fstride;
             C_SUB( *Fout2 ,  *Fout , t );
             C_ADDTO( *Fout ,  t );
             ++Fout2;
             ++Fout;
          }
       }
    }
}

void wtk_fixfft_bfly4(
		wtk_fixfft_substate_t *st,
		wtk_fixcpx_t * Fout,
        int fstride,
        int m,
        int N,
        int mm
        )
{
	wtk_fixcpx_t *tw1,*tw2,*tw3;
	wtk_fixcpx_t scratch[6];
    int m2=2*m;
    int m3=3*m;
    int i, j;
	wtk_fixcpx_t * Fout_beg = Fout;

    if (st->inverse)
    {
       for (i=0;i<N;i++)
       {
          Fout = Fout_beg + i*mm;
          tw3 = tw2 = tw1 = st->twiddles;
          for (j=0;j<m;j++)
          {
             C_MUL(scratch[0],Fout[m] , *tw1 );
             C_MUL(scratch[1],Fout[m2] , *tw2 );
             C_MUL(scratch[2],Fout[m3] , *tw3 );

             C_SUB( scratch[5] , *Fout, scratch[1] );
             C_ADDTO(*Fout, scratch[1]);
             C_ADD( scratch[3] , scratch[0] , scratch[2] );
             C_SUB( scratch[4] , scratch[0] , scratch[2] );
             C_SUB( Fout[m2], *Fout, scratch[3] );
             tw1 += fstride;
             tw2 += fstride*2;
             tw3 += fstride*3;
             C_ADDTO( *Fout , scratch[3] );

             Fout[m].r = scratch[5].r - scratch[4].i;
             Fout[m].i = scratch[5].i + scratch[4].r;
             Fout[m3].r = scratch[5].r + scratch[4].i;
             Fout[m3].i = scratch[5].i - scratch[4].r;
             ++Fout;
          }
       }
    } else
    {
       for (i=0;i<N;i++)
       {
          Fout = Fout_beg + i*mm;
          tw3 = tw2 = tw1 = st->twiddles;
          for (j=0;j<m;j++)
          {
             C_MUL4(scratch[0],Fout[m] , *tw1 );
             C_MUL4(scratch[1],Fout[m2] , *tw2 );
             C_MUL4(scratch[2],Fout[m3] , *tw3 );

             Fout->r = PSHR16(Fout->r, 2);
             Fout->i = PSHR16(Fout->i, 2);
             C_SUB( scratch[5] , *Fout, scratch[1] );

             //wtk_debug("vd=%d+j%d %d+j%d %d+j%d\n",scratch[5].r,scratch[5].i,Fout->r,Fout->i,scratch[1].r,scratch[1].i);

             C_ADDTO(*Fout, scratch[1]);
             C_ADD( scratch[3] , scratch[0] , scratch[2] );
             C_SUB( scratch[4] , scratch[0] , scratch[2] );
             Fout[m2].r = PSHR16(Fout[m2].r, 2);
             Fout[m2].i = PSHR16(Fout[m2].i, 2);
             C_SUB( Fout[m2], *Fout, scratch[3] );
             tw1 += fstride;
             tw2 += fstride*2;
             tw3 += fstride*3;
             C_ADDTO( *Fout , scratch[3] );

             //wtk_debug("fstride=%d tw1=%d+j%d tw2=%d+j%d tw3=%d+j%d\n",fstride,tw1->r,tw1->i,tw2->r,tw2->i,tw3->r,tw3->i);

             //wtk_debug("vx1[%d/%d %d/%d]=%d+j%d %d+j%d\n",i,j,m,m3,Fout[m].r,Fout[m].i,Fout[m3].r,Fout[m3].i);

             Fout[m].r = scratch[5].r + scratch[4].i;
             Fout[m].i = scratch[5].i - scratch[4].r;
             Fout[m3].r = scratch[5].r - scratch[4].i;
             Fout[m3].i = scratch[5].i + scratch[4].r;

             //wtk_debug("vx2[%d/%d %d/%d]=%d+j%d %d+j%d\n",i,j,m,m3,Fout[m].r,Fout[m].i,Fout[m3].r,Fout[m3].i);
             ++Fout;
          }
       }
    }
}

void wtk_fixfft_bfly3(wtk_fixfft_substate_t *st,wtk_fixcpx_t * Fout,int fstride,int m)
{
     int k=m;
     int m2 = 2*m;
     wtk_fixcpx_t *tw1,*tw2;
     wtk_fixcpx_t scratch[5];
     wtk_fixcpx_t epi3;
     epi3 = st->twiddles[fstride*m];

     tw1=tw2=st->twiddles;

     do{
        if (!st->inverse) {
         C_FIXDIV(*Fout,3); C_FIXDIV(Fout[m],3); C_FIXDIV(Fout[m2],3);
	}

         C_MUL(scratch[1],Fout[m] , *tw1);
         C_MUL(scratch[2],Fout[m2] , *tw2);

         C_ADD(scratch[3],scratch[1],scratch[2]);
         C_SUB(scratch[0],scratch[1],scratch[2]);
         tw1 += fstride;
         tw2 += fstride*2;

         Fout[m].r = Fout->r - HALF_OF(scratch[3].r);
         Fout[m].i = Fout->i - HALF_OF(scratch[3].i);

         C_MULBYSCALAR( scratch[0] , epi3.i );

         C_ADDTO(*Fout,scratch[3]);

         Fout[m2].r = Fout[m].r + scratch[0].i;
         Fout[m2].i = Fout[m].i - scratch[0].r;

         Fout[m].r -= scratch[0].i;
         Fout[m].i += scratch[0].r;

         ++Fout;
     }while(--k);
}

void wtk_fixfft_bfly5(wtk_fixfft_substate_t *st,wtk_fixcpx_t * Fout,int fstride,int m)
{
	wtk_fixcpx_t *Fout0,*Fout1,*Fout2,*Fout3,*Fout4;
    int u;
    wtk_fixcpx_t scratch[13];
    wtk_fixcpx_t * twiddles = st->twiddles;
    wtk_fixcpx_t *tw;
    wtk_fixcpx_t ya,yb;
    ya = twiddles[fstride*m];
    yb = twiddles[fstride*2*m];

    Fout0=Fout;
    Fout1=Fout0+m;
    Fout2=Fout0+2*m;
    Fout3=Fout0+3*m;
    Fout4=Fout0+4*m;

    tw=st->twiddles;
    for ( u=0; u<m; ++u ) {
        if (!st->inverse) {
        C_FIXDIV( *Fout0,5); C_FIXDIV( *Fout1,5); C_FIXDIV( *Fout2,5); C_FIXDIV( *Fout3,5); C_FIXDIV( *Fout4,5);
	}
        scratch[0] = *Fout0;

        C_MUL(scratch[1] ,*Fout1, tw[u*fstride]);
        C_MUL(scratch[2] ,*Fout2, tw[2*u*fstride]);
        C_MUL(scratch[3] ,*Fout3, tw[3*u*fstride]);
        C_MUL(scratch[4] ,*Fout4, tw[4*u*fstride]);

        C_ADD( scratch[7],scratch[1],scratch[4]);
        C_SUB( scratch[10],scratch[1],scratch[4]);
        C_ADD( scratch[8],scratch[2],scratch[3]);
        C_SUB( scratch[9],scratch[2],scratch[3]);

        Fout0->r += scratch[7].r + scratch[8].r;
        Fout0->i += scratch[7].i + scratch[8].i;

        scratch[5].r = scratch[0].r + S_MUL(scratch[7].r,ya.r) + S_MUL(scratch[8].r,yb.r);
        scratch[5].i = scratch[0].i + S_MUL(scratch[7].i,ya.r) + S_MUL(scratch[8].i,yb.r);

        scratch[6].r =  S_MUL(scratch[10].i,ya.i) + S_MUL(scratch[9].i,yb.i);
        scratch[6].i = -S_MUL(scratch[10].r,ya.i) - S_MUL(scratch[9].r,yb.i);

        C_SUB(*Fout1,scratch[5],scratch[6]);
        C_ADD(*Fout4,scratch[5],scratch[6]);

        scratch[11].r = scratch[0].r + S_MUL(scratch[7].r,yb.r) + S_MUL(scratch[8].r,ya.r);
        scratch[11].i = scratch[0].i + S_MUL(scratch[7].i,yb.r) + S_MUL(scratch[8].i,ya.r);
        scratch[12].r = - S_MUL(scratch[10].i,yb.i) + S_MUL(scratch[9].i,ya.i);
        scratch[12].i = S_MUL(scratch[10].r,yb.i) - S_MUL(scratch[9].r,ya.i);

        C_ADD(*Fout2,scratch[11],scratch[12]);
        C_SUB(*Fout3,scratch[11],scratch[12]);

        ++Fout0;++Fout1;++Fout2;++Fout3;++Fout4;
    }
}

/* perform the butterfly for one stage of a mixed radix FFT */
void wtk_fixfft_bfly_generic(
		wtk_fixfft_substate_t *st,
		wtk_fixcpx_t * Fout,
        int fstride,
        int m,
        int p
        )
{
    int u,k,q1,q;
    wtk_fixcpx_t * twiddles = st->twiddles;
    wtk_fixcpx_t t;
    wtk_fixcpx_t scratchbuf[17];
    int Norig = st->nfft;

    /*CHECKBUF(scratchbuf,nscratchbuf,p);*/
//    if (p>17)
//       speex_fatal("KissFFT: max radix supported is 17");

    for ( u=0; u<m; ++u ) {
        k=u;
        for ( q1=0 ; q1<p ; ++q1 ) {
            scratchbuf[q1] = Fout[ k  ];
        if (!st->inverse) {
            C_FIXDIV(scratchbuf[q1],p);
	}
            k += m;
        }

        k=u;
        for ( q1=0 ; q1<p ; ++q1 ) {
            int twidx=0;
            Fout[ k ] = scratchbuf[0];
            for (q=1;q<p;++q ) {
                twidx += fstride * k;
                if (twidx>=Norig) twidx-=Norig;
                C_MUL(t,scratchbuf[q] , twiddles[twidx] );
                C_ADDTO( Fout[ k ] ,t);
            }
            k += m;
        }
    }
}

void wtk_fixfft_work(wtk_fixfft_substate_t *st,wtk_fixcpx_t * Fout,wtk_fixcpx_t * f,int fstride,int in_stride,int * factors,int N,int s2,int m2)
{
   int i;
   wtk_fixcpx_t * Fout_beg=Fout;
    const int p=*factors++; /* the radix  */
    const int m=*factors++; /* stage's fft length/p */

   // wtk_debug("%d+j%d %d/%d/%d p=%d m=%d\n",f->r,f->i,N,s2,m2,p,m);
    /*printf ("fft %d %d %d %d %d %d %d\n", p*m, m, p, s2, fstride*in_stride, N, m2);*/
    if (m==1)
    {
       /*for (i=0;i<N;i++)
       {
          int j;
          Fout = Fout_beg+i*m2;
          const kiss_fft_cpx * f2 = f+i*s2;
          for (j=0;j<p;j++)
          {
             *Fout++ = *f2;
             f2 += fstride*in_stride;
          }
       }*/
    }else
    {
    	wtk_fixfft_work( st,Fout , f, fstride*p, in_stride, factors, N*p, fstride*in_stride, m);
    }
       switch (p)
       {
          case 2:
        	  wtk_fixfft_bfly2(st,Fout,fstride,m, N, m2);
        	  break;
          case 3:
        	  for (i=0;i<N;i++)
        	  {
        		  Fout=Fout_beg+i*m2;
        		  wtk_fixfft_bfly3(st,Fout,fstride,m);
        	  }
        	  break;
          case 4:
        	  wtk_fixfft_bfly4(st,Fout,fstride,m, N, m2);
        	  //exit(0);
        	  break;
          case 5:
        	  for (i=0;i<N;i++)
        	  {
        		  Fout=Fout_beg+i*m2;
        		  wtk_fixfft_bfly5(st,Fout,fstride,m);
        	  }
        	  break;
          default:
        	  for (i=0;i<N;i++)
        	  {
        		  Fout=Fout_beg+i*m2;
        		  wtk_fixfft_bfly_generic(st,Fout,fstride,m,p);
        	  }
        	  break;
    }
}

static void wtk_fixfft_renorm_range(wtk_fix16_t *in, wtk_fix16_t *out, int shift, int len)
{
   int i;

   for (i=0;i<len;i++)
   {
      out[i] = PSHR16(in[i], shift);
   }
}


void wtk_fixfft_fft(wtk_fixfft_t *fft,short *in,short *out)
{
	int shift;
	wtk_fixfft_state_t *st=fft->forward;
	int k,ncfft,nx;
	wtk_fixcpx_t tdc;
	int f1kr, f1ki, twr, twi;
	int f2kr,f2ki;
	int i,j,m;
	wtk_fixcpx_t *tmpbuf=st->tmpbuf,*super_twiddles=st->super_twiddles;

	shift=wtk_fixfft_range(in,in,32000,fft->N);
	wtk_fixfft_shuffle(tmpbuf,(wtk_fixcpx_t*)in,1,1,st->substate->factors);
	wtk_fixfft_work(st->substate,tmpbuf,(wtk_fixcpx_t*)in,1,1,st->substate->factors,1,1,1);
	ncfft=st->substate->nfft;
	tdc.r = tmpbuf[0].r;
	tdc.i = tmpbuf[0].i;
	C_FIXDIV(tdc,2);
	CHECK_OVERFLOW_OP(tdc.r ,+, tdc.i);
	CHECK_OVERFLOW_OP(tdc.r ,-, tdc.i);
	out[0] = tdc.r + tdc.i;
	out[2*ncfft-1] = tdc.r - tdc.i;
	nx=ncfft/2;
	for(k=1,i=ncfft-k,j=1,m=2*i-1;k<=nx;++k,--i)
	{
	      f2kr = SHR32(SUB32(EXTEND32(tmpbuf[k].r), EXTEND32(tmpbuf[i].r)),1);
	      f2ki = PSHR32(ADD32(EXTEND32(tmpbuf[k].i), EXTEND32(tmpbuf[i].i)),1);

	      f1kr = SHL32(ADD32(EXTEND32(tmpbuf[k].r), EXTEND32(tmpbuf[i].r)),13);
	      f1ki = SHL32(SUB32(EXTEND32(tmpbuf[k].i), EXTEND32(tmpbuf[i].i)),13);

	      twr = SHR32(SUB32(MULT16_16(f2kr,super_twiddles[k].r),MULT16_16(f2ki,super_twiddles[k].i)), 1);
	      twi = SHR32(ADD32(MULT16_16(f2ki,super_twiddles[k].r),MULT16_16(f2kr,super_twiddles[k].i)), 1);

	      out[j] = PSHR32(f1kr + twr, 15);
	      out[j+1] = PSHR32(f1ki + twi, 15);
	      j+=2;
	      out[m] = PSHR32(f1kr - twr, 15);
	      out[m+1] = PSHR32(twi - f1ki, 15);
	      m-=2;

	     // wtk_debug("v[%d]=%d/%d/%d/%d\n",k,out[2*k-1],out[2*k],out[2*(ncfft-k)-1],out[2*(ncfft-k)]);
	}
	//exit(0);
	wtk_fixfft_renorm_range(in,in,shift,fft->N);
	wtk_fixfft_renorm_range(out,out,shift,fft->N);
	//wtk_fixfft_ifft(fft,out,in);
}

void wtk_fixfft_ifft(wtk_fixfft_t *fft,short *in,short *out)
{
	wtk_fixfft_state_t *st=fft->backward;
	int k, ncfft;
	wtk_fixcpx_t *tmpbuf=st->tmpbuf,*super_twiddles=st->super_twiddles;
	wtk_fixcpx_t fk, fnkc, fek, fok, tmp;
	int nc;
	int i,j,m;

	ncfft = st->substate->nfft;
	tmpbuf[0].r = in[0] + in[2*ncfft-1];
	tmpbuf[0].i = in[0] - in[2*ncfft-1];
	nc=ncfft / 2;
   for (k = 1,m=ncfft-k,i=2*k-1,j=2*(ncfft - k)-1; k <= nc; ++k,--m)
   {
	  fk.r = in[i];
	  fk.i = in[i+1];
	  i+=2;
	  fnkc.r = in[j];
	  fnkc.i = -in[j+1];
	  j-=2;
		/*C_FIXDIV( fk , 2 );
	  C_FIXDIV( fnkc , 2 );*/

	  C_ADD (fek, fk, fnkc);
	  C_SUB (tmp, fk, fnkc);
	  C_MUL (fok, tmp, super_twiddles[k]);
	  C_ADD (tmpbuf[k],     fek, fok);
	  C_SUB (tmpbuf[m], fek, fok);
#ifdef USE_SIMD
	  st->tmpbuf[ncfft - k].i *= _mm_set1_ps(-1.0);
#else
	  tmpbuf[m].i *= -1;
#endif
   }
   wtk_fixfft_shuffle((wtk_fixcpx_t*)out,tmpbuf,1,1,st->substate->factors);
   wtk_fixfft_work(st->substate,(wtk_fixcpx_t*)out,tmpbuf,1,1,st->substate->factors,1,1,1);
}

void wtk_fixfft_print_fft(short *f,int n)
{
	int i,j;
	float ta,tb;

	ta=tb=0;
	printf("----------------- fft --------------------------\n");
	for(i=0,j=0;i<n;++j)
	{
		if(i==0 || i==n-1)
		{
			ta+=f[i];
			wtk_debug("v[%d]=%d\n",j,f[i]);
			++i;
		}else
		{
			ta+=f[i];
			tb+=f[i+1];
			wtk_debug("v[%d]=%d+%di\n",j,f[i],f[i+1]);
			i+=2;
		}
	}
	wtk_debug("tot=%e+%ei\n",ta,tb);
}





