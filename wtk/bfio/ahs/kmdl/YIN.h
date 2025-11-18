#pragma once
#include "config.h"
#ifndef YIN_H
#define YIN_H
typedef struct  {
  float YIN_BUFF[YIN_frameSize];
  float pitch_period ;
  float pitch_confidence;
  float prev_pitch_period;
  float pitch_confidence_context[3];
} PitchState;
void yin_clean();
PitchState *init_pitchstate();
void Pyin_process(PitchState *ps,int yin_frameSize);
#endif
