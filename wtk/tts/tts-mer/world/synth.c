//-----------------------------------------------------------------------------
// 
// Author: Zhizheng Wu (wuzhizheng@gmail.com)
// Date: 11-03-2016
//
// To generate waveform given F0, band aperiodicities and spectrum with WORLD vocoder
//
// This is modified based on Msanori Morise's test.cpp. Low-dimensional band aperiodicities are used as suggested by Oliver.
//
// synth FFT_length sampling_rate F0_file spectrogram_file aperiodicity_file output_waveform
//
//-----------------------------------------------------------------------------

#include "common.h"

#if (defined (__WIN32__) || defined (_WIN32)) && !defined (__MINGW32__)
#include <conio.h>
#include <windows.h>
#pragma comment(lib, "winmm.lib")
#pragma warning(disable : 4996)
#endif
#if (defined (__linux__) || defined(__CYGWIN__) || defined(__APPLE__))
#include <stdint.h>
#include <sys/time.h>
#endif

// For .wav input/output functions.
#include "audioio.h"
#include "synthesis_requiem.h"

// WORLD core functions.
// Note: win.sln uses an option in Additional Include Directories.
// To compile the program, the option "-I $(SolutionDir)..\src" was set.
#include "synthesis.h"
#include "synth.h"
#include "wtk/core/wtk_type.h"
// #include "tts-mer/wtk-extend/wtk_audio.h"

// Frame shift [msec]
#define FRAMEPERIOD 5.0

#if (defined (__linux__) || defined(__CYGWIN__) || defined(__APPLE__))
// Linux porting section: implement timeGetTime() by gettimeofday(),
#ifndef DWORD
#define DWORD uint32_t
#endif
#ifdef USE_DEBUG
static DWORD timeGetTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  DWORD ret = (DWORD)(tv.tv_usec / 1000 + tv.tv_sec * 1000);
  return ret;
}
#endif
#endif

//-----------------------------------------------------------------------------
// struct for WORLD
// This struct is an option.
// Users are NOT forced to use this struct.
//-----------------------------------------------------------------------------
typedef struct {
  double frame_period;
  int fs;

  float *f0;
  // double *time_axis;
  int f0_length;

  float *spectrogram;
  // double **aperiodicity;
  int fft_size;

  float *ap;
  int ap_len;

  float *sp;
} WorldParameters;

static void WaveformSynthesis(wtk_rfft_t *rf, float *win, WorldParameters *world_parameters, int fs,
    int y_length, float *y)
{
#ifdef USE_DEBUG
  DWORD elapsed_time;
  elapsed_time = timeGetTime();
#endif
  int f0_len = world_parameters->f0_length
    , ap_len = world_parameters->ap_len
    , fft_size = world_parameters->fft_size;
  double 
    frame_period = world_parameters->frame_period;
  float
    *f0 = world_parameters->f0,
    *ap = world_parameters->ap,
    *sp = world_parameters->spectrogram;
  
  wtk_world_synthesis_requiem(rf, win, fft_size, f0, f0_len, fs, y, y_length, ap, ap_len, sp, frame_period);
#ifdef USE_DEBUG
  wtk_debug("WORLD: %d [msec]\n", timeGetTime() - elapsed_time);
#endif
}

void DestroyMemory(WorldParameters *world_parameters) {
  // delete[] world_parameters->time_axis;
  // delete[] world_parameters->f0;
  // delete[] world_parameters->spectrogram;
  free(world_parameters->spectrogram);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int wtk_world_synth(wtk_rfft_t *rf, float *win, int fft_size, int fs, float frame_period, float *f0, int f0_length, float *sp, int sp_len, float *ap, int ap_len, char *fn_out, void *outio)
{
  WorldParameters world_parameters;
  // You must set fs and frame_period before analysis/synthesis.
  world_parameters.fs = fs;

  // 5.0 ms is the default value.
  // Generally, the inverse of the lowest F0 of speech is the best.
  // However, the more elapsed time is required.
  world_parameters.frame_period = frame_period;
  world_parameters.fft_size = fft_size;
  
  world_parameters.ap = ap;
  world_parameters.ap_len = ap_len;
  
  world_parameters.f0 = f0;
  world_parameters.f0_length = f0_length;

  // world_parameters.spectrogram = new double *[world_parameters.f0_length];
  // for (int i = 0; i < world_parameters.f0_length; ++i) {
  //   world_parameters.spectrogram[i] = new double[fft_size / 2 + 1];
  // }
  // world_parameters.spectrogram = new float[(fft_size/2+1)*f0_length];
  world_parameters.spectrogram = malloc(sizeof(float)*(fft_size/2+1)*f0_length);

  int k = fft_size/2+1
    , i, j;
  for (i = 0; i < f0_length; i++) {
    for (j=0; j< k; j++)
    {
      // world_parameters.spectrogram[i][j] = (sp+i*(fft_size/2+1))[j];
      world_parameters.spectrogram[j*f0_length+i] = (sp+i*k)[j];
    }
  }
  
  //---------------------------------------------------------------------------
  // Synthesis part
  //---------------------------------------------------------------------------
  // The length of the output waveform
  int y_length = (int)((world_parameters.f0_length - 1) *
    frame_period / 1000.0 * fs) + 1;
  float *y = malloc(sizeof(float)*y_length);
  // Synthesis
  WaveformSynthesis(rf, win, &world_parameters, fs, y_length, y);
  
  // Output
  wtk_world_wavwrite(y, y_length, fs, sizeof(short), fn_out, outio);
  // wtk_mer_wav_stream_write_float( );

  free(y);
  DestroyMemory(&world_parameters);

  return y_length*sizeof(short);
}
