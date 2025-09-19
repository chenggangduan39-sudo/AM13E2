#ifndef QTK_ENGINE_COMM_QTK_ENGINE_PARAM
#define QTK_ENGINE_COMM_QTK_ENGINE_PARAM

#include "sdk/session/qtk_session.h"
#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_engine_param qtk_engine_param_t;
struct qtk_engine_param {
    qtk_session_t *session;
	wtk_string_t *log_wav_path;
    wtk_string_t *wakewrd;
    float **mic_array;
    char *cfg;
    char *playfn;
	char *consist_fn;
	char *inputpcm_fn;
    int channel;
	int spk_channel;
	int theta;
	int theta_range;
	int online_tms;
	int specsum_fs;
	int specsum_fe;
	int lf;
	int theta_step;
	int use_fftsbf;
	int use_cnon;
    int use_erlssingle;
	int online_frame_step;
    int resample_in_rate;
    int resample_out_rate;

    int mic;
    int winStep;
    int left_margin;
    int right_margin;
    int idle_time;
    int asr_min_time;
    int asr_max_time;
    int timealive;
    int bfio_reset_time;
	int use_ssl;
	int use_maskssl;
    int use_maskssl2;
    int use_resample;
    int out_channel;

    float tts_volume;
    float tts_speed;
    float tts_pitch;
	float gbias;
	float agca;
	float mic_shift;
	float spk_shift;
	float echo_shift;
	float bfmu;
	float echo_bfmu;
	float energy_sum;
	float spenr_thresh;
	float sym;
    float noise_suppress;
	float echo_suppress;
	float echo_suppress_active;
    float eq_offset;
    float nil_er;
    float mic_corr_er;
    float mic_corr_aver;
    float mic_energy_er;
    float mic_energy_aver;
    float spk_corr_er;
    float spk_corr_aver;
    float spk_energy_er;
    float spk_energy_aver;
    float wake_conf;

	unsigned use_xcorr:1;
	unsigned use_equal:1;

    unsigned use_bin : 1;
    unsigned use_thread : 1;
    unsigned use_json : 1;
    unsigned active : 1;
    unsigned idle : 1;
    unsigned use_oneMic : 1;
    unsigned output_ogg : 1;
    unsigned syn : 1;
    unsigned wake_logwav : 1;

    unsigned use_hotword : 1;
    unsigned use_hw_upload;
    unsigned use_hint;
    unsigned use_hw_json;
    unsigned use_bfio_reset : 1;
	unsigned use_rearrange:1;
	unsigned use_logwav:1;
	unsigned use_inputpcm:1;
	unsigned use_cfg:1;
};

void qtk_engine_param_init(qtk_engine_param_t *param);
void qtk_engine_param_set_session(qtk_engine_param_t *param,
                                  qtk_session_t *session);
int qtk_engine_param_feed(qtk_engine_param_t *param, wtk_local_cfg_t *lc);
void qtk_engine_param_clean(qtk_engine_param_t *param);

void qtk_engine_param_print(qtk_engine_param_t *param);

#ifdef __cplusplus
};
#endif
#endif
