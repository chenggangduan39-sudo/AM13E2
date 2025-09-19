//-----------------------------------------------------------------------------
// Copyright 2012-2016 Masanori Morise. All Rights Reserved.
// Author: mmorise [at] yamanashi.ac.jp (Masanori Morise)
//
// .wav input/output functions were modified for compatibility with C language.
// Since these functions (wavread() and wavwrite()) are roughly implemented,
// we recommend more suitable functions provided by other organizations.
// This file is independent of WORLD project and for the test.cpp.
//-----------------------------------------------------------------------------
#include "audioio.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tts-mer/syn/wtk_mer_sound.h"

#if (defined (__WIN32__) || defined (_WIN32)) && !defined (__MINGW32__)
#pragma warning(disable : 4996)
#endif

static inline int MyMaxInt(int x, int y) {
  return x > y ? x : y;
}

static inline int MyMinInt(int x, int y) {
  return x < y ? x : y;
}

static void foutput(void *src, size_t size, int n, void **dst, char type)
{
    switch (type)
    {
      case 'w':
      {
        FILE *fp;
        fp = (FILE*)(*dst);
        fwrite(src, size, n, fp);
        break;
      }
      case 'm':
      {
        int s= size*n;
        memcpy(*dst, src, s);
        (*dst) = (char*)(*dst) + s;
        break;
      }
    }
}

void wtk_world_wavwrite(const float *x, int x_length, int fs, int nbit,
    const char *filename, void *dst) {
  void *fp;
  char type = 'm';
  int i;

  if (filename==NULL) {
    fp = dst;
  } else {
    fp = fopen(filename, "wb");
    type = 'w';
  }
  if (NULL == fp) {
    printf("File cannot be opened.\n");
    return;
  }

  if (filename!=NULL){
	  char text[4] = {'R', 'I', 'F', 'F'};
	  uint32_t long_number = 36 + x_length * 2;
	  foutput(text, 1, 4, &fp, type);
	  foutput(&long_number, 4, 1, &fp, type);

	  text[0] = 'W';
	  text[1] = 'A';
	  text[2] = 'V';
	  text[3] = 'E';
	  foutput(text, 1, 4, &fp, type);
	  text[0] = 'f';
	  text[1] = 'm';
	  text[2] = 't';
	  text[3] = ' ';
	  foutput(text, 1, 4, &fp, type);

	  long_number = 16;
	  foutput(&long_number, 4, 1, &fp, type);
	  int16_t short_number = 1;
	  foutput(&short_number, 2, 1, &fp, type);
	  short_number = 1;
	  foutput(&short_number, 2, 1, &fp, type);
	  long_number = fs;
	  foutput(&long_number, 4, 1, &fp, type);
	  long_number = fs * 2;
	  foutput(&long_number, 4, 1, &fp, type);
	  short_number = 2;
	  foutput(&short_number, 2, 1, &fp, type);
	  short_number = 16;
	  foutput(&short_number, 2, 1, &fp, type);

	  text[0] = 'd';
	  text[1] = 'a';
	  text[2] = 't';
	  text[3] = 'a';
	  foutput(text, 1, 4, &fp, type);
	  long_number = x_length * 2;
	  foutput(&long_number, 4, 1, &fp, type);
  }

  double max_x;
  // max_x=0.001;
  max_x=5;
  for (i = 0; i < x_length; ++i) {
	  if (fabs(x[i])>max_x)max_x=fabs(x[i]);
  }
  // 为了平滑分段合成导致的音量忽大忽小问题
  // max_x = (max_x + wtk_world_sound_get(max_x))/2.0;
  // wtk_world_sound_set(max_x);
  // printf("max_x: %lf \n", max_x);
  int16_t tmp_signal;
  for (i = 0; i < x_length; ++i) {
    if(max_x > 1.0) {
      tmp_signal = (short)(MyMaxInt(-32768,
        MyMinInt(32767, (int)(x[i]/max_x * 32767))));
    }
    else {
      tmp_signal = (short)(MyMaxInt(-32768,
        MyMinInt(32767,  (int)(x[i] * 32767))));
    }
    // tmp_signal = static_cast<int16_t>(MyMaxInt(-32768,
        // MyMinInt(32767, static_cast<int>(x[i] * 32767))));
    foutput(&tmp_signal, 2, 1, &fp, type);
  }
  if (type == 'w') fclose((FILE*)fp);
}
