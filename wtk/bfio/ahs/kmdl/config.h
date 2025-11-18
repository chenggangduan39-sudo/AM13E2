#ifndef CONFIG_H
#define CONFIG_H
#define FRAME_SIZE 160
#define WINDOW_SIZE (2 * FRAME_SIZE)
#define FREQ_SIZE (FRAME_SIZE + 1)
#define SAMPLE_RATE 16000
#define ENVELOPE_POSTFILTERING_BETA 0.02f
#define COMB_BUF_SIZE (COMB_M * FRAME_SIZE + WINDOW_SIZE)


/*config 2: low latency and low complexity*/
#define NB_BANDS 22
#define COMB_M 5
#define PITCH_MIN_PERIOD 40  // 400 Hz
#define PITCH_MAX_PERIOD 200 // 80 Hz
#define PITH_NORM_FACTOR ((float)(PITCH_MAX_PERIOD))
#define KALMAN_DELAY 32


#define tau_min PITCH_MIN_PERIOD
#define tau_max PITCH_MAX_PERIOD


#define harmo_th 0.2
#define YIN_lowAmp 0.1
#define YIN_frameSize  ((int)(1.5 * tau_max))
#define YIN_prob_smooth_factor  0.7

#define SAVE_FEAT_WHILE_INFER 0
#define WITH_REFERENCE 1
#endif
