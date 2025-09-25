#ifndef __SDK_MOD_CFG_H__
#define __SDK_MOD_CFG_H__
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifndef __ANDROID__
#include "qtk/record/qtk_record_cfg.h"
#include "qtk/play/qtk_play_cfg.h"
#endif
#ifdef USE_LED
#include "sdk/dev/led/qtk_led_cfg.h"
#endif
#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_mod_cfg{
#ifndef __ANDROID__
    qtk_record_cfg_t rcd;
	qtk_play_cfg_t usbaudio;
	qtk_play_cfg_t lineout;
#endif

#ifdef USE_LED
	qtk_led_cfg_t led;
#endif
	wtk_string_t uart_fn;
    wtk_string_t vboxebf_fn;
	wtk_string_t qform_fn;
	wtk_string_t soundscreen_fn;
	wtk_string_t kws_fn;
	wtk_string_t enroll_fn;
	wtk_string_t cache_path;
	int audio_type; //1 vboxebf  2 gainnetbf 3 ssl 4 eqform
	int audio2_type; //1 vboxebf  2 gainnetbf 3 ssl 4 eqform
	int audio3_type; //1 vboxebf  2 gainnetbf 3 ssl 4 eqform
	int max_add_count;
	int max_play_count;
	int max_output_length;
	int sleep_time;
	int resample_rate;
	int spknum;
	int skip_head_tm;
	float restart_time;
    float echo_shift;
	float mic_shift;
	float spk_shift;
	float lineout_shift;
	unsigned int use_lineout:1;
	unsigned int use_usbaudio:1;
	unsigned int use_uart:1;
	unsigned int debug:1;
	unsigned int use_log:1;
	unsigned int use_log_wav:1;
	unsigned int use_resample:1;
	unsigned int use_in_resample:1;
}qtk_mod_cfg_t;

int qtk_mod_cfg_init(qtk_mod_cfg_t *cfg);
int qtk_mod_cfg_clean(qtk_mod_cfg_t *cfg);
int qtk_mod_cfg_update_local(qtk_mod_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_mod_cfg_update(qtk_mod_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
