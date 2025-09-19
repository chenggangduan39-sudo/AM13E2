/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: fft transform
 last mod: $Id: smallft.h,v 1.3 2003/09/16 18:35:45 jm Exp $

 ********************************************************************/
/**
   @file smallft.h
   @brief Discrete Rotational Fourier Transform (DRFT)
*/

#ifndef WTK_BFIO_MASKDENOISE_WTK_DRFT
#define WTK_BFIO_MASKDENOISE_WTK_DRFT
#include <string.h>
#include "wtk/core/fft/wtk_rfft.h"
#include "wtk/core/fft/pffft.h"
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct drft_lookup wtk_drft_t;

/** Discrete Rotational Fourier Transform lookup */
struct drft_lookup{
  int n;
  float *trigcache;
  int *splitcache;
  wtk_rfft_t *rfft;
  PFFFT_Setup *pffft;
  float *xtmp;
  unsigned use_fft:1;
};

wtk_drft_t* wtk_drft_new(int n);
void wtk_drft_delete(wtk_drft_t *d);
void wtk_drft_fft(wtk_drft_t *d,float *data);
void wtk_drft_fft2(wtk_drft_t *d,float *input,wtk_complex_t *out);
void wtk_drft_fft2_x(wtk_drft_t *d,float *input,wtk_complex_t *out);
void wtk_drft_fft3(wtk_drft_t *d,float *input,wtk_complex_t *out);
void wtk_drft_ifft(wtk_drft_t *d,float *data);
void wtk_drft_ifft2(wtk_drft_t *d,wtk_complex_t *input,float *output);
void wtk_drft_ifft2_x(wtk_drft_t *d,wtk_complex_t *input,float *output);
void wtk_drft_ifft3(wtk_drft_t *d,wtk_complex_t *input,float *output);


void wtk_drft_init_synthesis_window(float *synthesis_window, float *analysis_window, int wins);
void wtk_drft_frame_analysis(wtk_drft_t* rfft, float *rfft_in, float *analysis_mem, wtk_complex_t *fft,  float *in, int wins, float *window);
void wtk_drft_frame_analysis2(wtk_drft_t* rfft, float *rfft_in, float *analysis_mem, wtk_complex_t *fft,  short *in, int wins, float *window);
void wtk_drft_frame_synthesis(wtk_drft_t* rfft,  float *rfft_in, float *synthesis_mem, wtk_complex_t *fft, float *out, int wins, float *synthesis_window);
void wtk_drft_frame_synthesis2(wtk_drft_t* rfft,  float *rfft_in, float *synthesis_mem, wtk_complex_t *fft, short *out, int wins, float *synthesis_window);

void wtk_drft_stft(wtk_drft_t *drft, float *rfft_in, float *analysis_mem, wtk_complex_t *fft, float *in, int wins, float *window);
void wtk_drft_istft(wtk_drft_t *drft, float *rfft_in, float *synthesis_mem, wtk_complex_t *fft, float *out, int wins, float *synthesis_window);
wtk_drft_t* wtk_drft_new2(int n);
void wtk_drft_delete2(wtk_drft_t *d);
void wtk_drft_fft22(wtk_drft_t *d,float *input,wtk_complex_t *out);
void wtk_drft_ifft22(wtk_drft_t *d,wtk_complex_t *input,float *output);
void wtk_drft_frame_analysis22(wtk_drft_t* rfft, float *rfft_in, float *analysis_mem, wtk_complex_t *fft,  short *in, int wins, float *window);
void wtk_drft_frame_synthesis22(wtk_drft_t* rfft,  float *rfft_in, float *synthesis_mem, wtk_complex_t *fft, float *out, int wins, float *synthesis_window);
void wtk_drft_frame_synthesis3(wtk_drft_t* rfft,  float *rfft_in, float *synthesis_mem, wtk_complex_t *fft, float *out, int wins, float *synthesis_window);
void wtk_drft_frame_synthesis4(wtk_drft_t* rfft,  float *rfft_in, wtk_complex_t *fft, float *out, int wins, float *synthesis_window);
void wtk_drft_frame_analysis22_float(wtk_drft_t* rfft, float *rfft_in, float *analysis_mem, wtk_complex_t *fft,  float *in, int wins, float *window);

void wtk_drft_fft23(wtk_drft_t *d,float *input,wtk_complex_t *out);
void wtk_drft_ifft23(wtk_drft_t *d,wtk_complex_t *input,float *output);
void wtk_drft_stft2(wtk_drft_t *drft, float *rfft_in, float *analysis_mem, wtk_complex_t *fft, float *in, int wins, float *window);
void wtk_drft_istft2(wtk_drft_t *drft, float *rfft_in, float *synthesis_mem, wtk_complex_t *fft, float *out, int wins, float *synthesis_window);

#ifdef __cplusplus
}
#endif

#endif
