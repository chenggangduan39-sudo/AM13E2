//-----------------------------------------------------------------------------
// Copyright 2012-2016 Masanori Morise. All Rights Reserved.
// Author: mmorise [at] yamanashi.ac.jp (Masanori Morise)
//-----------------------------------------------------------------------------
#ifndef WORLD_COMMON_H_
#define WORLD_COMMON_H_

#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
// #include "fft.h"
#include "macrodefinitions.h"
#include "wtk/core/math/wtk_math.h"

#define wtk_world_debug(...) printf("%s:%d:  ",__FUNCTION__,__LINE__);printf(__VA_ARGS__);fflush(stdout);

WORLD_BEGIN_C_DECLS

//-----------------------------------------------------------------------------
// Structs on FFT
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// GetSuitableFFTSize() calculates the suitable FFT size.
// The size is defined as the minimum length whose length is longer than
// the input sample.
// Input:
//   sample     : Length of the input signal
// Output:
//   Suitable FFT size
//-----------------------------------------------------------------------------
int GetSuitableFFTSize(int sample);

//-----------------------------------------------------------------------------
// These four functions are simple max() and min() function
// for "int" and "double" type.
//-----------------------------------------------------------------------------
// inline int MyMaxInt(int x, int y) {
//   return x > y ? x : y;
// }

// inline double MyMaxDouble(double x, double y) {
//   return x > y ? x : y;
// }

// inline int MyMinInt(int x, int y) {
//   return x < y ? x : y;
// }

// inline double MyMinDouble(double x, double y) {
//   return x < y ? x : y;
// }

//-----------------------------------------------------------------------------
// These functions are used in at least two different .cpp files
// DCCorrection is used in CheapTrick() and D4C().
void DCCorrection(const double *input, double current_f0, int fs, int fft_size,
    double *output);

// LinearSmoothing is used in CheapTrick() and D4C().
void LinearSmoothing(const double *input, double width, int fs, int fft_size,
    double *output);

// NuttallWindow is used in Dio() and D4C().
void NuttallWindow(int y_length, double *y);

//-----------------------------------------------------------------------------
// These functions are used to speed up the processing.

WORLD_END_C_DECLS

#endif  // WORLD_COMMON_H_
