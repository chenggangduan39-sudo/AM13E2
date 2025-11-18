#ifndef SDK_AUDIO_QTK_AUDIO
#define SDK_AUDIO_QTK_AUDIO

#include "wtk/core/wtk_model.h"

#include "qtk_audio_cfg.h"
#include "player/qtk_player.h"
#include "recorder/qtk_recorder.h"
#include "sdk/audio/usb/qtk_usb.h"
#include "qtk_auin.h"
#include "qtk_auout.h"
#include "sdk/session/qtk_session.h"
#include "daemon/qtk_audio_daemon.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_audio qtk_audio_t;

typedef void (*qtk_audio_rcd_notify_f)(void *ths,char *data,int len);
typedef void (*qtk_audio_ply_notify_f)(void *ths,qtk_auout_data_state_t state,char *data,int bytes);
typedef void (*qtk_audio_daemon_notify_f)(void *ths,qtk_audio_daemon_cmd_t cmd);
typedef void (*qtk_audio_notify_f)(void *ths,int is_err);

typedef enum
{
	QTK_AUDIO_AUDIO,
	QTK_AUDIO_USB,
}qtk_audio_type_t;

struct qtk_audio
{
	int vid;
	int pid;
	qtk_audio_cfg_t *cfg;
	qtk_session_t *session;
	union{
		qtk_usb_t *u;
		struct{
			qtk_player_t *p;
			qtk_recorder_t *r;
		}audio;
	}v;
	qtk_audio_type_t atype;
	qtk_auin_t *auin;
	qtk_auout_t *auout;
	qtk_audio_daemon_t *ad;
	wtk_model_t *model;
	wtk_model_item_t *talk_item;
	qtk_audio_rcd_notify_f rcd_notify;
	void *rcd_notify_ths;
	qtk_audio_ply_notify_f ply_notify;
	void *ply_notify_ths;
	qtk_audio_daemon_notify_f daemon_notify;
	void *daemon_notify_ths;
	int channel;
	int zero_wait_bytes;
	int zero_waited_bytes;
	unsigned talking:1;
	unsigned run:1;
	unsigned ply_flg:1;
	unsigned rcd_flg:1;
};

qtk_audio_t* qtk_audio_new(qtk_audio_cfg_t *cfg,qtk_session_t *session);
int qtk_audio_delete(qtk_audio_t *a);

void qtk_audio_set_model(qtk_audio_t *a,wtk_model_t *model);
void qtk_audio_set_callback(qtk_audio_t *a,
		void *user_data,
		qtk_recorder_start_func recorder_start,
		qtk_recorder_read_func  recorder_read,
		qtk_recorder_stop_func  recorder_stop,
		qtk_recorder_clean_func recorder_clean,
		qtk_player_start_func   player_start,
		qtk_player_write_func   player_write,
		qtk_player_stop_func    player_stop,
		qtk_player_clean_func   player_clean
		);
void qtk_audio_set_daemon_notify(qtk_audio_t *a,void *daemon_notify_ths,qtk_audio_daemon_notify_f daemon_notify);

/**
 * recorder
 */

void qtk_audio_rcder_set_notify(qtk_audio_t *a,void *rcd_notify_ths,qtk_audio_rcd_notify_f rcd_notify);
int qtk_audio_rcder_start(qtk_audio_t *a);
int qtk_audio_rcder_stop(qtk_audio_t *a);

int qtk_audio_rcder_get_bufSize(qtk_audio_t *a);
int qtk_audio_rcder_get_bufTime(qtk_audio_t *a);
int qtk_audio_rcder_get_channel(qtk_audio_t *a);
int qtk_audio_rcder_get_sampleRate(qtk_audio_t *a);
int qtk_audio_rcder_get_bytes(qtk_audio_t *a);

/**
 * player
 */
void qtk_audio_plyer_set_notify(qtk_audio_t *a,void *ply_notify_ths,qtk_audio_ply_notify_f ply_notify);
int qtk_audio_plyer_start(qtk_audio_t *a);
int qtk_audio_plyer_stop(qtk_audio_t *a);

int qtk_audio_plyer_play_file(qtk_audio_t *a,char *fn,int syn);
int qtk_audio_plyer_play_mp3(qtk_audio_t *a,char *fn);
void qtk_audio_plyer_play_start(qtk_audio_t *a,int sample_rate,int channel,int bytes_per_sample);
void qtk_audio_plyer_play_data(qtk_audio_t *a,char *data,int bytes);
void qtk_audio_plyer_play_end(qtk_audio_t *a,int syn);
void qtk_audio_plyer_stop_play(qtk_audio_t *a);
int qtk_audio_plyer_is_playing(qtk_audio_t *a);

float qtk_audio_plyer_set_volume(qtk_audio_t *a,float volume);
float qtk_audio_plyer_inc_volume(qtk_audio_t *a);
float qtk_audio_plyer_dec_volume(qtk_audio_t *a);

float qtk_audio_plyer_set_pitch(qtk_audio_t *a,float pitch);
float qtk_audio_plyer_inc_pitch(qtk_audio_t *a);
float qtk_audio_plyer_dec_pitch(qtk_audio_t *a);

#ifdef __cplusplus
};
#endif
#endif
