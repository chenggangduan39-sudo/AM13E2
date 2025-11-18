//-----------------------------------------------------------------------------
// Copyright 2012-2016 Masanori Morise. All Rights Reserved.
// Author: mmorise [at] yamanashi.ac.jp (Masanori Morise)
//-----------------------------------------------------------------------------
#ifndef WORLD_AUDIOIO_H_
#define WORLD_AUDIOIO_H_

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// wavwrite() write a .wav file.
// Input:
//   x          : Input signal
//   x_ength : Signal length of x [sample]
//   fs         : Sampling frequency [Hz]
//   nbit       : Quantization bit [bit]
//   filename   : Name of the output signal.
// Caution:
//   The variable nbit is not used in this function.
//   This function only supports the 16 bit.
//-----------------------------------------------------------------------------
void wtk_world_wavwrite(const float *x, int x_length, int fs, int nbit,
  const char *filename, void *dst);

#ifdef __cplusplus
}
#endif

#endif  // WORLD_AUDIOIO_H_
