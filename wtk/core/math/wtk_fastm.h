#ifndef WTK_CORE_MATH_WTK_FASTM
#define WTK_CORE_MATH_WTK_FASTM
#include "wtk/core/wtk_type.h" 
#ifdef __cplusplus
extern "C" {
#endif

float wtk_fast_log(float val);
float wtk_fast_log2(float val);
double wtk_fast_exp(double y);
float wtk_fast_exp2(float y);
float wtk_fast_pow(float a, float b);
float wtk_fast_sqrt(float x);

#ifdef __cplusplus
};
#endif
#endif
