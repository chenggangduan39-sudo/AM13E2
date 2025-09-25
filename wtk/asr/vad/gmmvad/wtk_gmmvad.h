#ifndef WTK_VAD_GMMVAD_WTK_GMMVAD
#define WTK_VAD_GMMVAD_WTK_GMMVAD
#include "wtk/core/wtk_type.h" 
#include "wtk_gmmvad_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gmmvad wtk_gmmvad_t;

struct wtk_gmmvad
{
	wtk_gmmvad_cfg_t *cfg;
	int frame_counter;
	int downsampling_filter_state[4];
	int16_t over_hang;
	int16_t num_of_speech;
    int16_t noise_means[kTableSize];
    int16_t speech_means[kTableSize];
    int16_t noise_stds[kTableSize];
    int16_t speech_stds[kTableSize];
    int16_t index_vector[16 * kNumChannels];
    int16_t low_value_vector[16 * kNumChannels];
    int16_t mean_value[kNumChannels];
    int16_t upper_state[5];
    int16_t lower_state[5];
    int16_t hp_filter_state[4];
    int16_t over_hang_max_1[3];
    int16_t over_hang_max_2[3];
    int16_t individual[3];
    int16_t total[3];
    int16_t speechNB[240]; // Downsampled speech frame: 480 samples (30ms in WB)
};

wtk_gmmvad_t* wtk_gmmvad_new(wtk_gmmvad_cfg_t *cfg);
void wtk_gmmvad_delete(wtk_gmmvad_t *v);
void wtk_gmmvad_reset(wtk_gmmvad_t *v);
int wtk_gmmvad_set_mode(wtk_gmmvad_t *v,int mode);

/**
 * return 1 for speech 0 for noise
 */
int wtk_gmmvad_process(wtk_gmmvad_t *v,short *sample,int len);
#ifdef __cplusplus
};
#endif
#endif
