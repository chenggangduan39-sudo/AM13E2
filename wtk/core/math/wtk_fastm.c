#include "wtk_fastm.h" 

#include <math.h>

float wtk_fast_log(float val)
{
    union { float val; int32_t x; } u = { val };
    register float log_2 = (float)(((u.x >> 23) & 255) - 128);
    u.x   &= ~(255 << 23);
    u.x   += 127 << 23;
    log_2 += ((-0.3358287811f) * u.val + 2.0f) * u.val  -0.65871759316667f;
    return (log_2);
}

float wtk_fast_log2(float val)
{
    int* exp_ptr = (int*)(&val);
    int x = *exp_ptr;
    int log_2 = ((x >> 23) & 255) - 128;
    x &= ~(255 << 23);
    x += 127 << 23;
    *exp_ptr = x;
    //val = ((-1.0f/3) * val + 2) * val - 2.0f/3;   // (1)
    //wtk_debug("f1=%f f2=%f\n",f1,f2);
    //exit(0);
    //val = (-0.333333 * val + 2) * val -0.666667;   // (1)
    return ((-0.333333 * val + 2) * val -0.666667 + log_2)* 0.69314718f;
}

//see wtk_matrix.c   
/* 
double wtk_fast_exp(double y)
{
#define EXP_A (1048576/M_LN2)
#define EXP_C 60801
	union {
		double d;
		struct {
			int j, i;
		} n;
	} d2i;

	d2i.n.j=0;
	d2i.n.i = EXP_A*(y)+(1072693248-EXP_C);
	return d2i.d;
}

float wtk_fast_exp2(float y)
{
#define EXP_A (1048576/M_LN2)
#define EXP_C 60801
	union {
		double d;
		struct {
			int j, i;
		} n;
	} d2i;

	d2i.n.j=0;
	d2i.n.i = EXP_A*(y)+(1072693248-EXP_C);
	return d2i.d;
}*/

float wtk_fast_pow(float a, float b)
{
    union {
        double d;
        int x[2];
    } u = { a };
    u.x[1] = (int)(b * (u.x[1] - 1072632447) + 1072632447);
    u.x[0] = 0;
    return u.d;
}

float wtk_fast_sqrt(float x)
{
    union {
        float d;
        unsigned int i;
    } u = { x};

    u.i+=127 << 23;
    u.i>>=1;
    return u.d;
}

//
//float wtk_fast_sqrt(float x)
//{
//  unsigned int i = *((unsigned int*)(&x));
//
//  // adjust bias
//  i  += 127 << 23;
//  // approximation of square root
//  i >>= 1;
//  return *(float*) &i;
//}
