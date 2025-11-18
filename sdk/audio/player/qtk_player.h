#ifndef SDK_AUDIO_QTK_PLAYER
#define SDK_AUDIO_QTK_PLAYER

#include "qtk_player_cfg.h"
#include "sdk/audio/util/qtk_asound_card.h"
#include "qtk_player_module.h"
#include "wtk/os/wtk_log.h"
#include "sdk/session/qtk_session.h"
#ifdef MTK8516
#include "include/u_gpio.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
#ifdef MTK8516
#define SPEAKER_ENABLE_PIN (GPIO_BASE_VALUE + GPIO33)
#endif
typedef struct qtk_player qtk_player_t;
typedef void(*qtk_player_notify_f)(void *err_notify_ths,int err);

#pragma pack(1)
typedef struct
{
	unsigned char	zero;		// 保留
	unsigned char	indx;		// 音频通道编号
	unsigned short	data;		// 有效音频数据
}audio_frame_t;
#pragma pack()
#define AUDIO_DATA_CHANNEL (8)

struct qtk_player
{
	qtk_player_cfg_t *cfg;
	wtk_log_t *log;
	qtk_player_module_t plyer_module;
	qtk_player_notify_f err_notify_func;
	wtk_strbuf_t *buf;
	audio_frame_t *frames;
    short *data_8ch;
    int frames_len;
	int step;
	void *err_notify_ths;
	int write_fail_times;
};

qtk_player_t* qtk_player_new(qtk_player_cfg_t *cfg,qtk_session_t *session,void *notify_ths,qtk_player_notify_f notify_func);
int qtk_player_delete(qtk_player_t *p);
void qtk_player_set_err_notify(qtk_player_t *p,
		void *err_notify_ths,
		qtk_player_notify_f err_notify_func
		);
void qtk_player_set_callback(qtk_player_t *p,
		void *handler,
		qtk_player_start_func start_func,
		qtk_player_stop_func  stop_func,
		qtk_player_write_func write_func,
		qtk_player_clean_func clean_func
		);

int qtk_player_start(qtk_player_t *p,
		char *snd_name,
		int sample_rate,
		int channel,
		int bytes_per_sample
		);
void qtk_player_stop(qtk_player_t *p);
int qtk_player_write(qtk_player_t *p, char *data, int bytes);
int qtk_player_write2(qtk_player_t *p, char *data, int bytes, int channel);
void qtk_player_clean(qtk_player_t *p);
int qtk_player_isErr(qtk_player_t *p);
int qtk_player_get_sound_card(char *card_name);


#ifdef __cplusplus
};
#endif
#endif
