//-----------------------------------------------------------------------------
// Copyright 2012-2016 Masanori Morise. All Rights Reserved.
// Author: mmorise [at] yamanashi.ac.jp (Masanori Morise)
//-----------------------------------------------------------------------------
#ifndef WORLD_SYNTHESIS_H_
#define WORLD_SYNTHESIS_H_

#include "macrodefinitions.h"

WORLD_BEGIN_C_DECLS

//-----------------------------------------------------------------------------
// Synthesis() synthesize the voice based on f0, spectrogram and
// aperiodicity (not excitation signal).
// Input:
//   f0                   : f0 contour
//   f0_length            : Length of f0
//   spectrogram          : Spectrogram estimated by CheapTrick
//   fft_size             : FFT size
//   aperiodicity         : Aperiodicity spectrogram based on D4C
//   frame_period         : Temporal period used for the analysis
//   fs                   : Sampling frequency
//   y_length             : Length of the output signal (Memory of y has been
//                          allocated in advance)
// Output:
//   y                    : Calculated speech
//-----------------------------------------------------------------------------

int GetTimeBase(const float *f0, int f0_length, int fs,
    double frame_period, double *time_axis, int y_length,
    int *pulse_locations_index, float *interpolated_vuv);

WORLD_END_C_DECLS

#endif  // WORLD_SYNTHESIS_H_
