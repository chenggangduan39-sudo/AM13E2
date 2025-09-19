#ifndef WTK_BFIO_MASKDENOISE_PITCH
#define WTK_BFIO_MASKDENOISE_PITCH
#include "assert.h"
#include "celt_lpc.h"
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif

void pitch_downsample(float *x[], float *x_lp,
      int len, int C);

void pitch_search(const float *x_lp, float *y,
                  int len, int max_pitch, int *pitch);

void pitch_search2(const float *x_lp, float *y,
                  int len, int max_pitch, int *pitch,  float *pitch_corr);

float remove_doubling(float *x, int maxperiod, int minperiod,
      int N, int *T0, int prev_period, float prev_gain);


/* OPT: This is the kernel you really want to optimize. It gets used a lot
   by the prefilter and by the PLC. */
static inline void xcorr_kernel(const float * x, const float * y, float sum[4], int len)
{
   int j;
   float y_0, y_1, y_2, y_3;
   
   assert(len>=3);
   y_3=0; /* gcc doesn't realize that y_3 can't be used uninitialized */
   y_0=*y++;
   y_1=*y++;
   y_2=*y++;
   for (j=0;j<len-3;j+=4)
   {
      float tmp;
      tmp = *x++;
      y_3=*y++;
      sum[0] += tmp*y_0;
      sum[1] += tmp*y_1;
      sum[2] += tmp*y_2;
      sum[3] += tmp*y_3;
      tmp=*x++;
      y_0=*y++;
      sum[0] += tmp*y_1;
      sum[1] += tmp*y_2;
      sum[2] += tmp*y_3;
      sum[3] += tmp*y_0;
      tmp=*x++;
      y_1=*y++;
      sum[0] += tmp*y_2;
      sum[1] += tmp*y_3;
      sum[2] += tmp*y_0;
      sum[3] += tmp*y_1;
      tmp=*x++;
      y_2=*y++;
      sum[0] += tmp*y_3;
      sum[1] += tmp*y_0;
      sum[2] += tmp*y_1;
      sum[3] += tmp*y_2;
   }
   if (j++<len)
   {
      float tmp = *x++;
      y_3=*y++;
      sum[0] += tmp*y_0;
      sum[1] += tmp*y_1;
      sum[2] += tmp*y_2;
      sum[3] += tmp*y_3;
   }
   if (j++<len)
   {
      float tmp=*x++;
      y_0=*y++;
      sum[0] += tmp*y_1;
      sum[1] += tmp*y_2;
      sum[2] += tmp*y_3;
      sum[3] += tmp*y_0;
   }
   if (j<len)
   {
      float tmp=*x++;
      y_1=*y++;
      sum[0] += tmp*y_2;
      sum[1] += tmp*y_3;
      sum[2] += tmp*y_0;
      sum[3] += tmp*y_1;
   }
}

static inline void dual_inner_prod(const float *x, const float *y01, const float *y02,
      int N, float *xy1, float *xy2)
{
   int i;
   float xy01=0;
   float xy02=0;
   for (i=0;i<N;i++)
   {
      xy01 += x[i]*y01[i];
      xy02 += x[i]*y02[i];
   }
   *xy1 = xy01;
   *xy2 = xy02;
}

/*We make sure a C version is always available for cases where the overhead of
  vectorization and passing around an arch flag aren't worth it.*/
static inline float celt_inner_prod(const float *x,
      const float *y, int N)
{
   int i;
   float xy=0;

   for (i=0;i<N;i++)
      xy += x[i]*y[i];

   return xy;
}

void celt_pitch_xcorr(const float *_x, const float *_y,
      float *xcorr, int len, int max_pitch);


#ifdef __cplusplus
};
#endif
#endif
