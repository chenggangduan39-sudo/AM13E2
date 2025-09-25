#ifndef WTK_ASR_PARM_WTK_FIXPOINT
#define WTK_ASR_PARM_WTK_FIXPOINT
#include "wtk/core/wtk_type.h"
#include "wtk/core/math/wtk_math.h"
#ifdef USE_AUX
#include <immintrin.h>
#include <avx2intrin.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef int wtk_fix_t;
typedef int fixed32;
typedef float float32;

#ifndef DEFAULT_RADIX
#define DEFAULT_RADIX 12
#endif

/** Convert floating point to fixed point. */
#define FLOAT2FIX_ANY(x,radix) \
	(((x)<0.0) ? \
	((fixed32)((x)*(float32)(1<<(radix)) - 0.5)) \
	: ((fixed32)((x)*(float32)(1<<(radix)) + 0.5)))

#define INT2FIX_ANY(x,radix) \
	(((x)<0.0) ? \
	((fixed32)((x)*(float32)(1<<(radix)))) \
	: ((fixed32)((x)*(float32)(1<<(radix)))))


#define FLOAT2FIX_ANY2(x,radix) ((((int)(x))>>(radix)))
#define FIX2FLOAT_ANY2(x,radix) ((float)((x)<<(radix)))

#define FLOAT2FIX(x) FLOAT2FIX_ANY(x,DEFAULT_RADIX)
/** Convert fixed point to floating point. */
#define FIX2FLOAT_ANY(x,radix) ((float32)(x)/(1<<(radix)))
#define FIX2FLOAT(x) FIX2FLOAT_ANY(x,DEFAULT_RADIX)
#define FLOAT2COS(x) FLOAT2FIX_ANY(x,30)
typedef long long	   int64;
#define FIXMUL_ANY(a,b,radix) ((fixed32)(((int64)(a)*(b))>>(radix)))
#define FIXMUL(a,b) FIXMUL_ANY(a,b,DEFAULT_RADIX)
#define COSMUL(x,y) FIXMUL_ANY(x,y,30)

#define FIXMUL_TEST(a,b,radix)  (((fixed32)((a)*1.0*(b)))>>(radix))
#define FIXMUL_ANY2(a,b,radix) (fixed32)((((int64)(a))>>radix)*(b))


/* Various fixed-point logarithmic functions that we need. */
/** Minimum value representable in log format. */
#define MIN_FIXLOG -2829416  /* log(1e-300) * (1<<DEFAULT_RADIX) */
#define MIN_FIXLOG2 -4081985 /* log2(1e-300) * (1<<DEFAULT_RADIX) */
/** Fixed-point representation of log(2) */
#define FIXLN_2		((fixed32)(0.693147180559945 * (1<<DEFAULT_RADIX)))
/** Take natural logarithm of a fixedpoint number. */
#define FIXLN(x) (fixlog(x) - (FIXLN_2 * DEFAULT_RADIX))
#define FIX2FLOAT_ANY(x,radix) ((float32)(x)/(1<<(radix)))
#define FIX2FLOAT(x) FIX2FLOAT_ANY(x,DEFAULT_RADIX)


#ifndef HEXAGON
typedef unsigned int	uint32;
typedef int		int32;
#else
#include "AEEStdDef.h"
#endif


#define MIN_FIXLOG -2829416  /* log(1e-300) * (1<<DEFAULT_RADIX) */
#define MIN_INT -2147483600
#define MIN_SMALL -2047483600

/**
 * Take base-2 logarithm of an integer, yielding a fixedpoint number
 * with DEFAULT_RADIX as radix point.
 */
int32 fixlog2(uint32 x);
int fixlog(uint32 x);
fixed32 fe_log(float32 x);
fixed32 fe_log_add(fixed32 x, fixed32 y);

void wtk_fix_print_float(int *v,int len);
void wtk_fix_print(int *v,int len,int shift);
float wtk_fix_sum(int *v,int len,int shift);
void wtk_fix_print_short(short *v,int len,int shift);

int* wtk_fix_float2int(float *v,int n);

typedef struct {
   short m;
   short e;
}wtk_flt_t;

short wtk_flt_ilog2(int64 x);
wtk_flt_t wtk_flt(int x);
wtk_flt_t wtk_flt3(int shift);
wtk_flt_t wtk_flt2(int64 x);
int wtk_flt_toi(wtk_flt_t a);
int wtk_flt_toi2(wtk_flt_t a,int shift);
float wtk_flt_tof(wtk_flt_t a,int shift);
void wtk_flt_print(wtk_flt_t flt);
void wtk_flt_print2(wtk_flt_t flt,int shift);
int64 wtk_flt_extract64(wtk_flt_t a);
int wtk_flt_lt(wtk_flt_t a,wtk_flt_t b);
int wtk_flt_gt(wtk_flt_t a,wtk_flt_t b);
wtk_flt_t wtk_flt_add(wtk_flt_t a,wtk_flt_t b);
wtk_flt_t wtk_flt_sub(wtk_flt_t a,wtk_flt_t b);
wtk_flt_t wtk_flt_mul(wtk_flt_t a, wtk_flt_t b);
wtk_flt_t wtk_flt_mul2(wtk_flt_t a, int b,int shift);
wtk_flt_t wtk_flt_mul3(wtk_flt_t a,wtk_flt_t b,int shift);
wtk_flt_t wtk_flt_sqrt(wtk_flt_t a);
wtk_flt_t wtk_flt_divu(wtk_flt_t a, wtk_flt_t b);
wtk_flt_t wtk_flt_div32(int a,int b);
int wtk_flt_extract32(wtk_flt_t a);
short wtk_flt_extract16(wtk_flt_t a);
wtk_flt_t wtk_flt_shl(wtk_flt_t a,int b);

int wtk_fix_mul(int a,int b,int shift);
wtk_flt_t wtk_flt_mul_int(int a,int b,int shift);
wtk_flt_t wtk_flt_square(int a,int shift);
wtk_flt_t wtk_flt_square2(int a);
wtk_flt_t wtk_flt_square3(wtk_flt_t a);

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

//c=a*b;
#define WTK_FLT_MUL(c,a,b) \
{ \
	c.m = (short)((((int)a.m)*(b.m))>>15); \
	c.e = a.e+b.e+15; \
	if (c.m>0) \
	{\
		if (c.m<16384) \
		{ \
			c.m<<=1; \
			c.e-=1; \
		} \
	} else \
	{\
		if (c.m>-16384) \
		{ \
			c.m<<=1; \
			c.e-=1; \
		} \
	} \
}

#define WTK_FLT_ADD(c,a,b) \
{ \
	if(a.m==0) \
	{ \
		c.m=b.m; \
		c.e=b.e; \
	}else if(b.m==0)\
	{  \
		c.m=a.m;\
		c.e=a.e;\
	}else \
	{\
		if ((a).e > (b).e) \
		{\
		   c.m = ((a).m>>1) + ((b).m>>MIN(15,(a).e-(b).e+1)); \
		   c.e = (a).e+1; \
		}else \
		{ \
		   c.m = ((b).m>>1) + ((a).m>>MIN(15,(b).e-(a).e+1)); \
		   c.e = (b).e+1; \
		} \
		if (c.m>0) \
		{ \
		   if(c.m<16384) \
		   { \
		         c.m<<=1; \
		         c.e-=1;\
		   }\
		} else \
		{ \
		   if (c.m>-16384) \
		   { \
		      c.m<<=1; \
		      c.e-=1; \
		   }\
		} \
	}\
}

#define WTK_FLT_ADD2(a,b) \
{ \
	if(b.m==0)\
	{  \
	}else \
	{\
		if ((a).e > (b).e) \
		{\
		   a.m = ((a).m>>1) + ((b).m>>MIN(15,(a).e-(b).e+1)); \
		   a.e = (a).e+1; \
		}else \
		{ \
		   a.m = ((b).m>>1) + ((a).m>>MIN(15,(b).e-(a).e+1)); \
		   a.e = (b).e+1; \
		} \
		if (a.m>0) \
		{ \
		   if(a.m<16384) \
		   { \
		         a.m<<=1; \
		         a.e-=1;\
		   }\
		} else \
		{ \
		   if (a.m>-16384) \
		   { \
		      a.m<<=1; \
		      a.e-=1; \
		   }\
		} \
	}\
}

typedef long long wtk_int64_t;

wtk_flt_t wtk_fltf(wtk_int64_t x);
short wtk_flt_ilogl(wtk_int64_t x);

typedef struct
{
	int *pv;
	int len;
	int shift;
}wtk_fixvec_t;

wtk_fixvec_t* wtk_fixvec_new(int len);
void wtk_fixvec_delete(wtk_fixvec_t *v);
wtk_fixvec_t* wtk_fixvec_new2(float *pf,int len);
wtk_fixvec_t* wtk_fixvec_new3(float *pf,int len,int shiftx);

typedef struct
{
	unsigned char *pv;
	//short *pv;
	int len;
	int shift;
	short min;
}wtk_fixvecc_t;

wtk_fixvecc_t* wtk_fixvecc_new(int len);
wtk_fixvecc_t* wtk_fixvecc_new2(float *pf,int len);
void wtk_fixvecc_delete(wtk_fixvecc_t *v);
int wtk_fixvecc_bytes(wtk_fixvecc_t *v);

typedef struct
{
	int row;
	int col;
	unsigned char *pv;
	short *min;
	//short *pv;
	unsigned char *shift;
}wtk_fixmatc_t;

int wtk_fixmatc_bytes(wtk_fixmatc_t *m);
wtk_fixmatc_t* wtk_fixmatc_new2(int row,int col);
wtk_fixmatc_t* wtk_fixmatc_new(float *pf,int row,int col);
void wtk_fixmatc_delete(wtk_fixmatc_t *m);

typedef struct
{
	int row;
	int col;
	short *pv;
	unsigned char *shift;
}wtk_fixmats_t;
int wtk_fixmats_bytes(wtk_fixmats_t *m);
wtk_fixmats_t* wtk_fixmats_new2(int row,int col);
void wtk_fixmats_delete(wtk_fixmats_t* m);
wtk_fixmats_t* wtk_fixmats_new(float *pf,int row,int col,int min_shift);

typedef struct
{
	short *pv;
	int len;
	int shift;
}wtk_fixvecs_t;

int wtk_fixvecs_bytes(wtk_fixvecs_t *vec);
wtk_fixvecs_t* wtk_fixvecs_new(float *pf,int len,int min_shift);
wtk_fixvecs_t* wtk_fixvecs_new2(int len);
void wtk_fixvecs_delete(wtk_fixvecs_t *v);


/**
 * -128 120
 *  0.001 15
 *	[-16,15]*(e6)/e6
 *	-16*2^3 15*2^3
 */
typedef struct
{
	char m:5;
	unsigned char e:3;
}wtk_fltchar_t;
#define WTK_FLTCHAR_STEP 3

int wtk_fltchar_base(float f);
void wtk_fltchar_print(wtk_fltchar_t c);
wtk_fltchar_t wtk_fltchar_init(float f);

#define WTK_FLTCHAR_MUL(c,a,b,v) \
	c.e=a.e+b.e-6;\
	v=((int)(a.m)*(b.m));\
	if(v>15) \
	{ \
		do\
		{\
			v>>=WTK_FLTCHAR_STEP;\
			++c.e;\
		}while(v>15);\
	}else if(v<-16) \
	{ \
		do \
		{\
			v>>=WTK_FLTCHAR_STEP;\
			++c.e;\
		}while(v<-16);\
	}

wtk_fltchar_t wtk_fltchar_mul(wtk_fltchar_t a,wtk_fltchar_t b);
void wtk_fltchar_test(void);

int wtk_fix_exp(int v);
void wtk_fix_print_exp(int input_shift,int output_shift);

#ifdef USE_AUX
void wtk_fix_test_mul(void);
#endif

#ifdef __cplusplus
};
#endif
#endif
