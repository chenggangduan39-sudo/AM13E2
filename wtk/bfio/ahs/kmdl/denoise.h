#include "YIN.h"
#include "common.h"
#include "config.h"
#include "kiss_fft.h"
#include "backbone.h"
#include "wtk/core/math/wtk_math.h"
#ifndef DENOISE_H
#define DENOISE_H
#define SQUARE(x) ((x) * (x))

typedef struct {
  int init;
  kiss_fft_state *kfft;
  float half_window[FRAME_SIZE];
  float comb_hann_window[COMB_M + 1];
} CommonState;

typedef struct {
  float analysis_mem[FRAME_SIZE];
  float synthesis_mem[FRAME_SIZE];
  float comb_buf[COMB_BUF_SIZE];
} SignalState;

SignalState *init_SignalState();
RNNState *init_RNNState();
void delete_RNNState(RNNState *rnn);
#if SAVE_FEAT_WHILE_INFER
void rnnoise_process_frame(RNNState *rnn_state, PitchState *pst,
                           const float *in, SignalState *state_input,
                           const float *reference, SignalState *state_reference,float *out,
                           FILE *f_feat);

#else
void rnnoise_process_frame(RNNState *rnn_state, PitchState *pst,
                           const float *in, SignalState *state_input,
                           const float *reference, SignalState *state_reference,
                           float *out);
#endif

int train(int argc, char **argv);
void denoise_clean();
#endif
