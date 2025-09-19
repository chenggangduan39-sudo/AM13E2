#ifndef __qtk_mod_am13e2_CFG_H__
#define __qtk_mod_am13e2_CFG_H__
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "qtk/record/qtk_record_cfg.h"
#include "qtk/play/qtk_play_cfg.h"
#include "sdk/api_1/vboxebf/qtk_vboxebf_cfg.h"
#include "sdk/api_1/gainnetbf/qtk_gainnetbf_cfg.h"
#include "wtk/bfio/consist/wtk_mic_check_cfg.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_mod_am13e2_cfg{
	qtk_record_cfg_t rcd;
	qtk_record_cfg_t rcd2;
	qtk_record_cfg_t rcd3;
	qtk_record_cfg_t rcd4;
	qtk_play_cfg_t usbaudio;
	qtk_play_cfg_t lineout;
	qtk_play_cfg_t speaker;
	wtk_string_t cache_path;
	wtk_string_t gain_path;
	wtk_string_t linein_check_path;
	wtk_string_t lineout_check_path;

	wtk_string_t gainnetbf_fn;
	
	wtk_string_t mic_check_rcd_fn;
	wtk_string_t mic_check_play_fn;

	wtk_string_t vboxebf_fn;
	wtk_string_t array_vbox_fn;
	wtk_string_t denoise_vbox_fn;

	qtk_vboxebf_cfg_t *denoisebf_cfg;
	qtk_vboxebf_cfg_t *vboxebf_cfg;
	qtk_vboxebf_cfg_t *avboxebf_cfg;
	qtk_gainnetbf_cfg_t *gainnetbf_cfg;
	wtk_mic_check_cfg_t *mic_check_rcd_cfg;
	wtk_mic_check_cfg_t *mic_check_play_cfg;

	int nmic;
	int sleep_time;
	int sil_time;
	int aspk_channel;
	int linein_channel;
	int use_out_mode;// 1:uac 2:line in/out
	int aspk_mode;
	int ch0_shifttm;
	int ch1_shifttm;
	int ch2_shifttm;
	int ch3_shifttm;
	int ch4_shifttm;
	int ch5_shifttm;
	int ch6_shifttm;
	int ch7_shifttm;
	int cha_shifttm;
	float mic_sum;
	float mic_shift;
	float echo_shift;
	unsigned int use_usbaudio:1;
	unsigned int use_lineout:1;
	unsigned int use_speaker:1;
	unsigned int debug:1;
	unsigned int use_log:1;
	unsigned int use_log_wav:1;
	unsigned int use_out_resample:1;
	unsigned int use_dev:1;
	unsigned int use_test3a:1;
	unsigned int use_3abfio:1;
	unsigned int use_array:1;
	unsigned int use_linein_mic:1;
	unsigned int use_linein_courseware:1;
	unsigned int use_linein_check:1;
	unsigned int use_rcd3:1;
	unsigned int use_rcd:1;
	unsigned int use_rcd4:1;
	unsigned int use_line_in:1;
	unsigned int use_spkout:1;
	unsigned int use_wooferout:1;
	unsigned int use_headset:1;
	unsigned int use_speaker_left:1;
	unsigned int use_speaker_right:1;
	unsigned int use_linein_courseware_touac:1;
	unsigned int use_mainlineout:1;
	unsigned int use_wooflineout:1;
	unsigned int use_meetinglineout:1;
	unsigned int use_expandlineout:1;
}qtk_mod_am13e2_cfg_t;

int qtk_mod_am13e2_cfg_init(qtk_mod_am13e2_cfg_t *cfg);
int qtk_mod_am13e2_cfg_clean(qtk_mod_am13e2_cfg_t *cfg);
int qtk_mod_am13e2_cfg_update_local(qtk_mod_am13e2_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_mod_am13e2_cfg_update(qtk_mod_am13e2_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
