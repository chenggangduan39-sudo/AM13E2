#include "denoise.h"
#include "YIN.h"
#include "config.h"
#if (SAMPLE_RATE == 16000)
#if (NB_BANDS == 34)
int nfftborder[] = {0,  2,  4,  6,  8,  10, 12,  14,  16,  18, 20, 22,
                    24, 26, 28, 30, 32, 34, 36,  38,  40,  42, 44, 46,
                    48, 55, 63, 72, 83, 95, 108, 123, 140, 160};

#elif (NB_BANDS == 22)
int nfftborder[] = {0,  2,  4,  6,  8,  10, 12, 14, 16,  18,  20,
                    22, 24, 30, 37, 46, 57, 70, 86, 106, 130, 160};
#else
#error "Unsupported NB_BANDS value. Must be 22 or 34."
#endif
#endif
CommonState common;

SignalState *init_SignalState() {
  SignalState *st;
  st = (SignalState *)malloc(sizeof(SignalState));
  memset(st, 0, sizeof(*st));
  return st;
}

void delete_RNNState(RNNState *rnn){
  free(rnn->first_conv1d_state);
  free(rnn->second_conv1d_state);
  free(rnn->gru1_state);
  free(rnn->gru2_state);
  free(rnn->third_conv1d_state);
  free(rnn->gru3_state);
  free(rnn->gru4_state);
}

RNNState *init_RNNState() {
  RNNState *rnnstate;
  rnnstate = (RNNState *)malloc(sizeof(RNNState));

  extern const RNNModel model_lite;
  rnnstate->model = &model_lite;
  rnnstate->first_conv1d_state =
      (float *)calloc(sizeof(float), rnnstate->model->conv1->kernel_size *
                                         rnnstate->model->conv1->nb_inputs);
  rnnstate->second_conv1d_state =
      (float *)calloc(sizeof(float), rnnstate->model->conv2->kernel_size *
                                         rnnstate->model->conv2->nb_inputs);
  rnnstate->gru1_state =
      (float *)calloc(sizeof(float), rnnstate->model->gru1->nb_neurons);
  rnnstate->gru2_state =
      (float *)calloc(sizeof(float), rnnstate->model->gru2->nb_neurons);

#if WITH_REFERENCE
  rnnstate->third_conv1d_state =
      (float *)calloc(sizeof(float), rnnstate->model->conv3->kernel_size *
                                         rnnstate->model->conv3->nb_inputs);

  rnnstate->gru3_state =
      (float *)calloc(sizeof(float), rnnstate->model->gru3->nb_neurons);
  rnnstate->gru4_state =
      (float *)calloc(sizeof(float), rnnstate->model->gru4->nb_neurons);

#endif
  return rnnstate;
};

void compute_band_energy(float *bandE, const kiss_fft_cpx *X) {
  int i;
  float sum[NB_BANDS] = {0};

  for (i = 0; i < NB_BANDS - 1; i++) {
    int j;
    int band_size;
    band_size = (nfftborder[i + 1] - nfftborder[i]);
    for (j = 0; j < band_size; j++) {
      float tmp;
      float frac = (float)j / band_size;
      tmp = SQUARE(X[(nfftborder[i]) + j].r);
      tmp += SQUARE(X[(nfftborder[i]) + j].i);
      sum[i] += (1 - frac) * tmp;
      sum[i + 1] += frac * tmp;
    }
  }
  for (i = 0; i < NB_BANDS; i++) {
    bandE[i] = sum[i];
  }
}

void compute_pitch_coherence(float *coherence, const kiss_fft_cpx *X,
                             const kiss_fft_cpx *P, const float *X_erb,
                             const float *P_erb) {
  int i;
  float sum[NB_BANDS] = {0};
  for (i = 0; i < NB_BANDS - 1; i++) {
    int j;
    int band_size;
    band_size = (nfftborder[i + 1] - nfftborder[i]);
    for (j = 0; j < band_size; j++) {
      float tmp;
      float frac = (float)j / band_size;
      tmp = X[(nfftborder[i]) + j].r * P[(nfftborder[i]) + j].r;
      tmp += X[(nfftborder[i]) + j].i * P[(nfftborder[i]) + j].i;
      sum[i] += (1 - frac) * tmp;
      sum[i + 1] += frac * tmp;
    }
  }

  for (i = 0; i < NB_BANDS; i++) {
    coherence[i] = sum[i] / sqrt(1e-15 + X_erb[i] * P_erb[i]);
  }
}

void interp_band_gain(float *g, const float *bandE) {
  int i;
  memset(g, 0, FREQ_SIZE);
  for (i = 0; i < NB_BANDS - 1; i++) {
    int j;
    int band_size;
    band_size = (nfftborder[i + 1] - nfftborder[i]);
    for (j = 0; j < band_size; j++) {
      float frac = (float)j / band_size;
      g[(nfftborder[i]) + j] = (1 - frac) * bandE[i] + frac * bandE[i + 1];
    }
  }
}

void check_init() {
  int i;
  float temp_sum = 0;
  if (common.init)
    return;
  common.kfft = opus_fft_alloc_twiddles(2 * FRAME_SIZE, NULL, NULL, NULL, 0);

  for (i = 0; i < FRAME_SIZE; i++)
    common.half_window[i] =
        sin(.5 * M_PI * sin(.5 * M_PI * (i + .5) / FRAME_SIZE) *
            sin(.5 * M_PI * (i + .5) / FRAME_SIZE));

  for (i = 1; i < COMB_M + 2; i++) {
    common.comb_hann_window[i - 1] =
        0.5 - 0.5 * cos(2.0 * M_PI * i / (COMB_M * 2 + 2));
    temp_sum += common.comb_hann_window[i - 1];
  }
  for (i = 1; i < COMB_M + 2; i++) {
    common.comb_hann_window[i - 1] /= temp_sum;
  }

  common.init = 1;
}

void denoise_clean(){
  if (common.init){
    free(common.kfft->bitrev);
    free(common.kfft->twiddles);
    free(common.kfft);
  }
}

void apply_window(float *x) {
  int i;
  check_init();
  for (i = 0; i < FRAME_SIZE; i++) {
    x[i] *= common.half_window[i];
    x[WINDOW_SIZE - 1 - i] *= common.half_window[i];
  }
}

void forward_transform(kiss_fft_cpx *out, const float *in) {
  int i;
  kiss_fft_cpx x[WINDOW_SIZE];
  kiss_fft_cpx y[WINDOW_SIZE];
  check_init();
  for (i = 0; i < WINDOW_SIZE; i++) {
    x[i].r = in[i];
    x[i].i = 0;
  }

  opus_fft(common.kfft, x, y, 0);
  for (i = 0; i < FREQ_SIZE; i++) {
    out[i].r = y[i].r * WINDOW_SIZE;
    out[i].i = y[i].i * WINDOW_SIZE;
  }
}

void inverse_transform(float *out, const kiss_fft_cpx *in) {
  int i;
  kiss_fft_cpx x[WINDOW_SIZE];
  kiss_fft_cpx y[WINDOW_SIZE];
  check_init();
  for (i = 0; i < FREQ_SIZE; i++) {
    x[i] = in[i];
  }
  for (; i < WINDOW_SIZE; i++) {
    x[i].r = x[WINDOW_SIZE - i].r;
    x[i].i = -x[WINDOW_SIZE - i].i;
  }
  opus_fft(common.kfft, x, y, 0);
  /* output in reverse order for IFFT. */
  out[0] = y[0].r;
  for (i = 1; i < WINDOW_SIZE; i++) {
    out[i] = y[WINDOW_SIZE - i].r;
  }
}

void frame_analysis(SignalState *st, kiss_fft_cpx *X, float *Ex,
                    const float *in) {
  int i;
  float x[WINDOW_SIZE];
  RNN_COPY(x, st->analysis_mem, FRAME_SIZE);
  for (i = 0; i < FRAME_SIZE; i++)
    x[FRAME_SIZE + i] = in[i];
  RNN_COPY(st->analysis_mem, in, FRAME_SIZE);
  apply_window(x);
  forward_transform(X, x);
  compute_band_energy(Ex, X);
}

void normalize_band_E(float *Erb_band, float *BandE) {
  float ref_level_db = 10.f;
  float min_level_db = -50.f;
  float band_E;
  for (int i = 0; i < NB_BANDS; i++) {
    band_E = 10 * log10(Erb_band[i] + 1e-7) - ref_level_db;
    BandE[i] = ((band_E - min_level_db) / (-min_level_db));
  }
}

int compute_frame_features(SignalState *st, PitchState *ps, kiss_fft_cpx *X,
                           float *Ex, kiss_fft_cpx *P, float *Ep, float *Exp,
                           float *features, const float *in) {
  int i, k;
  float E = 0;
  float p[WINDOW_SIZE];
  float yin_frame[FRAME_SIZE];

  // compute current frame's erb band
  RNN_MOVE(st->comb_buf, &st->comb_buf[FRAME_SIZE], COMB_BUF_SIZE - FRAME_SIZE);
  RNN_COPY(&st->comb_buf[COMB_BUF_SIZE - FRAME_SIZE], in, FRAME_SIZE);
  for (int i = 0; i < FRAME_SIZE; i++) {
    celt_assert(st->comb_buf[COMB_BUF_SIZE - FRAME_SIZE + i] == in[i]);
  }

  frame_analysis(st, X, Ex, &st->comb_buf[COMB_BUF_SIZE - FRAME_SIZE]);

  // compute_current_band_energy
  float BandE[NB_BANDS] = {0};
  normalize_band_E(Ex, BandE);
  for (i = 0; i < NB_BANDS; i++) {
    features[i] = BandE[i];
  }

  // compute pitch
  for (i = 0; i < FRAME_SIZE; i++) {
    yin_frame[i] = (st->comb_buf[COMB_BUF_SIZE - FRAME_SIZE + i]);
  }
  RNN_MOVE(ps->YIN_BUFF, &ps->YIN_BUFF[FRAME_SIZE], YIN_frameSize - FRAME_SIZE);
  RNN_COPY(&ps->YIN_BUFF[YIN_frameSize - FRAME_SIZE], yin_frame, FRAME_SIZE);

  for (int i = 0; i < FRAME_SIZE; i++) {
    celt_assert(YIN_BUFF[YIN_frameSize - FRAME_SIZE + i] == yin_frame[i]);
  }

  Pyin_process(ps, YIN_frameSize);

  if (ps->pitch_period <= PITCH_MIN_PERIOD) {
    ps->pitch_period = PITCH_MIN_PERIOD;
  }
  if (ps->pitch_period >= PITCH_MAX_PERIOD) {
    ps->pitch_period = PITCH_MAX_PERIOD;
  }

  features[2 * NB_BANDS] = ps->pitch_period / PITH_NORM_FACTOR;
  features[2 * NB_BANDS + 1] = ps->pitch_confidence;

  // compute pitch coherence
  for (i = 0; i < WINDOW_SIZE; i++) {
    p[i] = 0;
  }

  for (k = -COMB_M; k < 1; k++) {
    for (i = 0; i < WINDOW_SIZE; i++)
      p[i] += st->comb_buf[COMB_BUF_SIZE - WINDOW_SIZE +
                           ((int)(ps->pitch_period * k)) + i] *
              common.comb_hann_window[k + COMB_M];
  }
  apply_window(p);
  forward_transform(P, p);
  compute_band_energy(Ep, P);

  compute_pitch_coherence(Exp, X, P, Ex, Ep);
  for (i = 0; i < NB_BANDS; i++) {
    features[i + NB_BANDS] = Exp[i];
  }

  for (i = 0; i < NB_BANDS; i++) {
    E += Ex[i];
  }
  return E < 0.1;
}

void calc_ideal_gain(float *X, float *Y, float *g) {
  for (int i = 0; i < NB_BANDS; ++i) {
    g[i] = sqrt(X[i] / (1e-7 + Y[i]));
    if (g[i] > 1)
      g[i] = 1;
    if (g[i] < 0)
      g[i] = 0;
  }
}

int train(int argc, char **argv) {
  int i;
  int count = 0;
  int maxCount;

  if (argc != 7) {
    printf("# of argc is %d\n", argc);
    fprintf(stderr,
            "usage: %s <target> <input>  <count> <output> <estimated echo>  "
            "<mic>\n",
            argv[0]);
    return 1;
  }
  float target[FRAME_SIZE];
  float input[FRAME_SIZE];
  float reference[FRAME_SIZE];
  float mic[FRAME_SIZE];

  SignalState *state_target;
  SignalState *state_input;
  SignalState *state_reference;
  SignalState *state_mic;

  state_target = init_SignalState();
  state_input = init_SignalState();
  state_reference = init_SignalState();
  state_mic = init_SignalState();

  FILE *f1, *f2, *f3, *f4, *f5;

  f1 = fopen(argv[1], "rb"); // target audio
  f2 = fopen(argv[2], "rb"); // input
  f3 = fopen(argv[4], "wb"); // output feat
  f4 = fopen(argv[5], "rb"); // reference which could be estimated echo or
                             // reference of kalman filter
  f5 = fopen(argv[6], "rb"); // mic, which is noisy input

  maxCount = atoi(argv[3]);
  PitchState *pst;
  pst = init_pitchstate();

  while (1) {
    short tmp[FRAME_SIZE];
    if (count == maxCount)
      break;
    // target clean
    fread(tmp, sizeof(short), FRAME_SIZE, f1);
    if (feof(f1)) {
      rewind(f1);
      fread(tmp, sizeof(short), FRAME_SIZE, f1);
    }
    for (i = 0; i < FRAME_SIZE; i++)
      target[i] = ((float)tmp[i]) / 32768.f;

    // noisy input
    fread(tmp, sizeof(short), FRAME_SIZE, f2);
    if (feof(f2)) {
      rewind(f2);
      fread(tmp, sizeof(short), FRAME_SIZE, f2);
    }
    for (i = 0; i < FRAME_SIZE; i++)
      input[i] = ((float)tmp[i]) / 32768.f;

    // echo estimated by kalman
    fread(tmp, sizeof(short), FRAME_SIZE, f4);
    if (feof(f4)) {
      rewind(f4);
      fread(tmp, sizeof(short), FRAME_SIZE, f4);
    }
    for (i = 0; i < FRAME_SIZE; i++)
      reference[i] = ((float)tmp[i]) / 32768.f;

    // mic
    fread(tmp, sizeof(short), FRAME_SIZE, f5);
    if (feof(f5)) {
      rewind(f5);
      fread(tmp, sizeof(short), FRAME_SIZE, f5);
    }
    for (i = 0; i < FRAME_SIZE; i++)
      mic[i] = ((float)tmp[i]) / 32768.f;

    // compute feature of main branch
    float features[NB_BANDS * 2 + 2];
    float input_erb[NB_BANDS];
    kiss_fft_cpx INPUT[FREQ_SIZE];
    kiss_fft_cpx P[FREQ_SIZE];
    float Ep[NB_BANDS];
    float Exp[NB_BANDS];

    compute_frame_features(state_input, pst, INPUT, input_erb, P, Ep, Exp,
                           features, input);
    fwrite(features, sizeof(float), NB_BANDS * 2 + 2, f3);

    // comput feature of side branch
    float reference_erb[NB_BANDS] = {0};
    float reference_erb_normed[NB_BANDS] = {0};
    kiss_fft_cpx REFERENCE[FREQ_SIZE];
    frame_analysis(state_reference, REFERENCE, reference_erb, reference);
    normalize_band_E(reference_erb, reference_erb_normed);
    fwrite(reference_erb_normed, sizeof(float), NB_BANDS, f3);

    float mic_erb[NB_BANDS] = {0};
    float mic_erb_normed[NB_BANDS] = {0};
    kiss_fft_cpx KALMAN_OUT[FREQ_SIZE];
    frame_analysis(state_mic, KALMAN_OUT, mic_erb, mic);
    normalize_band_E(mic_erb, mic_erb_normed);
    fwrite(mic_erb_normed, sizeof(float), NB_BANDS, f3);

    // compute IRM
    float target_erb[NB_BANDS];
    kiss_fft_cpx TARGET[FREQ_SIZE];
    frame_analysis(state_target, TARGET, target_erb, target);

    float gb[NB_BANDS];
    calc_ideal_gain(target_erb, input_erb, gb);
    fwrite(gb, sizeof(float), NB_BANDS, f3);

    count++;
  }
  fclose(f1);
  fclose(f2);
  fclose(f3);
  fclose(f4);
  fclose(f5);
  free(state_target);
  free(pst);
  free(state_input);
  free(state_reference);
  return 0;
}

void frame_synthesis(SignalState *st, float *out, const kiss_fft_cpx *y) {
  float x[WINDOW_SIZE];
  int i;
  inverse_transform(x, y);
  apply_window(x);
  for (i = 0; i < FRAME_SIZE; i++)
    out[i] = x[i] + st->synthesis_mem[i];
  RNN_COPY(st->synthesis_mem, &x[FRAME_SIZE], FRAME_SIZE);
}

void biquad(float *y, float mem[2], const float *x, const float *b,
            const float *a, int N) {
  int i;
  for (i = 0; i < N; i++) {
    float xi, yi;
    xi = x[i];
    yi = x[i] + mem[0];
    mem[0] = mem[1] + (b[0] * (double)xi - a[0] * (double)yi);
    mem[1] = (b[1] * (double)xi - a[1] * (double)yi);
    y[i] = yi;
  }
}

#if SAVE_FEAT_WHILE_INFER
void rnnoise_process_frame(RNNState *rnn_state, PitchState *pst,
                           const float *input, SignalState *state_input,
                           const float *reference, SignalState *state_reference,
                           float *out, FILE *f_feat)

#else
void rnnoise_process_frame(RNNState *rnn_state, PitchState *pst,
                           const float *input, SignalState *state_input,
                           const float *reference, SignalState *state_reference,
                           float *out)
#endif
{
  int i;
  float features[NB_BANDS * 2 + 2];
  float gb[NB_BANDS];
  kiss_fft_cpx Y[FREQ_SIZE];
  float Y_erb[NB_BANDS] = {0};
  kiss_fft_cpx P_hat[FREQ_SIZE];
  float Phat_erb[NB_BANDS];
  float Exp[NB_BANDS];

  // compute main branch's features
  compute_frame_features(state_input, pst, Y, Y_erb, P_hat, Phat_erb, Exp,
                         features, input);

  // compute side branch's features
  float reference_erb[NB_BANDS] = {0};
  float reference_erb_normed[NB_BANDS] = {0};
  kiss_fft_cpx REFERENCE[FREQ_SIZE];
  frame_analysis(state_reference, REFERENCE, reference_erb, reference);
  normalize_band_E(reference_erb, reference_erb_normed);

  float features_full[3 * NB_BANDS + 2] = {0};
  for (i = 0; i < 2 * NB_BANDS + 2; i++) {
    features_full[i] = features[i];
  };
  for (i = 0; i < NB_BANDS; i++) {
    features_full[i + 2 * NB_BANDS + 2] = reference_erb_normed[i];
  };

  compute_rnn_lite(rnn_state, gb, features_full);

#if SAVE_FEAT_WHILE_INFER
  fwrite(features_full, sizeof(float), NB_BANDS * 3 + 2, f_feat);
  fwrite(gb, sizeof(float), NB_BANDS, f_feat);
#endif

  float gf[FREQ_SIZE] = {1};
  interp_band_gain(gf, gb);

  for (i = 0; i < FREQ_SIZE; i++) {
    Y[i].r *= gf[i];
    Y[i].i *= gf[i];
  };

  frame_synthesis(state_input, out, Y);
};
