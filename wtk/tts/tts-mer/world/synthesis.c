//-----------------------------------------------------------------------------
// Copyright 2012-2016 Masanori Morise. All Rights Reserved.
// Author: mmorise [at] yamanashi.ac.jp (Masanori Morise)
//
// Voice synthesis based on f0, spectrogram and aperiodicity.
// forward_real_fft, inverse_real_fft and minimum_phase are used to speed up.
//-----------------------------------------------------------------------------
#include "common.h"
#include "synthesis.h"
#include "synthesis_requiem.h"
#include "constantnumbers.h"
#include "matlabfunctions.h"

static void GetTemporalParametersForTimeBase(const float *f0, int f0_length, int fs,
  int y_length, float frame_period,
  double *coarse_time_axis, double *coarse_f0, double *coarse_vuv) 
{
  // for (int i = 0; i < y_length; ++i)
  //   time_axis[i] = i / static_cast<double>(fs);
  int i;
  float tmp = 0;
  for (i = 0; i < f0_length; ++i)
  {
    // coarse_time_axis[i] = i * frame_period;
    coarse_time_axis[i] = tmp;
    tmp += frame_period;

    coarse_f0[i] = f0[i];
    coarse_vuv[i] = f0[i] == 0.0 ? 0.0 : 1.0;
  }
  coarse_f0[f0_length] = coarse_f0[f0_length - 1] * 2 - coarse_f0[f0_length - 2];
  coarse_vuv[f0_length] = coarse_vuv[f0_length - 1] * 2 - coarse_vuv[f0_length - 2];
  // for (int i = 0; i < f0_length; ++i)
  //   coarse_f0[i] = f0[i];
  // coarse_f0[f0_length] = coarse_f0[f0_length - 1] * 2 -
  //   coarse_f0[f0_length - 2];
  // for (int i = 0; i < f0_length; ++i)
  //   coarse_vuv[i] = f0[i] == 0.0 ? 0.0 : 1.0;
  // coarse_vuv[f0_length] = coarse_vuv[f0_length - 1] * 2 -
  //   coarse_vuv[f0_length - 2];
}


static int GetPulseLocationsForTimeBase(const float *interpolated_f0,
    const double *time_axis, int y_length, int fs,
    int *pulse_locations_index) 
{/* 这个函数对精度要求较高,保持double */
  int i;
  double 
    pi2 = 2.0 * PI,
    pi2fs = pi2 / fs;
  double 
    // *total_phase = new double[y_length],
    // *wrap_phase = new double[y_length],
    *total_phase = malloc(sizeof(*total_phase)*y_length),
    *wrap_phase = malloc(sizeof(*wrap_phase)*y_length),
    wrap_phase_abs;
    // *wrap_phase_abs = new float[y_length];

  // total_phase[0] = 2.0 * world::kPi * interpolated_f0[0] / fs;
  total_phase[0] = pi2fs * interpolated_f0[0];
  // printf("%lf %lf \n", total_phase[0], interpolated_f0[0]);
  for (i = 1; i < y_length; ++i)
  {
    // total_phase[i] = total_phase[i - 1] + 2.0 * world::kPi * interpolated_f0[i] / fs;
    total_phase[i] = total_phase[i - 1] + pi2fs * interpolated_f0[i];
  }

  
  for (i = 0; i < y_length; ++i)
    wrap_phase[i] = fmod(total_phase[i], pi2);

  int number_of_pulses = 0;
  for (i = 0; i < y_length - 1; ++i)
  {
    wrap_phase_abs = fabs(wrap_phase[i + 1] - wrap_phase[i]);
    if (wrap_phase_abs > PI) {
      // pulse_locations[number_of_pulses] = time_axis[i];
      // pulse_locations_index[number_of_pulses] = static_cast<int>(matlab_round(pulse_locations[number_of_pulses] * fs)) + 1;
      pulse_locations_index[number_of_pulses] = (int)(matlab_round(time_axis[i] * fs));
      /* python 版这里是有 +1 的  */
      ++number_of_pulses;
    }
  }

  
  // for (int i = 0; i < y_length - 1; ++i) {
  //   if (wrap_phase_abs[i] > world::kPi) {
  //     // pulse_locations[number_of_pulses] = time_axis[i];
  //     // pulse_locations_index[number_of_pulses] = static_cast<int>(matlab_round(pulse_locations[number_of_pulses] * fs)) + 1;
  //     pulse_locations_index[number_of_pulses] = static_cast<int>(matlab_round(time_axis[i] * fs)) + 1;
  //     /* python 版这里是有 +1 的  */
  //     ++number_of_pulses;
  //   }
  // }

  // delete[] wrap_phase;
  // delete[] total_phase;
  free(wrap_phase);
  free(total_phase);

  return number_of_pulses;
}

int GetTimeBase(const float *f0, int f0_length, int fs,
    double frame_period, double *time_axis, int y_length,
    int *pulse_locations_index, float *interpolated_vuv)
{
  // double *coarse_time_axis = new double[f0_length + 1];
  // double *coarse_f0 = new double[f0_length + 1];
  // double *coarse_vuv = new double[f0_length + 1];
  // float *interpolated_f0 = new float[y_length];
  double *coarse_time_axis = calloc( (f0_length + 1), sizeof(*coarse_time_axis));
  double *coarse_f0 = calloc( (f0_length + 1), sizeof(*coarse_f0));
  double *coarse_vuv = calloc( (f0_length + 1), sizeof(*coarse_vuv));
  float *interpolated_f0 = calloc( y_length, sizeof(*interpolated_f0));
  int i;

  GetTemporalParametersForTimeBase(f0, f0_length, fs, y_length, frame_period, coarse_time_axis, coarse_f0, coarse_vuv);

  interp1(coarse_time_axis, coarse_f0, f0_length + 1,
      time_axis, y_length, interpolated_f0);
  interp1(coarse_time_axis, coarse_vuv, f0_length + 1,
      time_axis, y_length, interpolated_vuv);
  // for (int i = 0; i < y_length; ++i)
  // {
  //   interpolated_vuv[i] = interpolated_vuv[i] > 0.5 ? 1.0 : 0.0;
  // }
  for (i = 0; i < y_length; ++i)
  {
    // interpolated_f0[i] =
      // interpolated_vuv[i] == 0.0 ? world::kCeilF0 : interpolated_f0[i];
    if (interpolated_vuv[i] > 0.5) {
    } else {
      // interpolated_f0[i] = world::kCeilF0;
      interpolated_f0[i] = 500.0;
    }
    // interpolated_f0[i] =
    //   interpolated_vuv[i] > 0.5 ? interpolated_f0[i]: world::kCeilF0;
  }
  int number_of_pulses = GetPulseLocationsForTimeBase(interpolated_f0,
      time_axis, y_length, fs, pulse_locations_index);

  // delete[] coarse_vuv;
  // delete[] coarse_f0;
  // delete[] coarse_time_axis;
  // delete[] interpolated_f0;

  free(coarse_vuv);
  free(coarse_f0);
  free(coarse_time_axis);
  free(interpolated_f0);

  return number_of_pulses;
}
