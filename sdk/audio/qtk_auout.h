#ifndef SDK_AUDIO_QTK_AUOUT
#define SDK_AUDIO_QTK_AUOUT

#include "wtk/core/pitch/wtk_pitch.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_log.h"

// #include "qtk/util/mp3dec/wtk_mp3dec2.h"
#include "qtk_auout_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_auout qtk_auout_t;

typedef enum {
	QTK_AUOUT_DATA_START,
	QTK_AUOUT_DATA_WRITE,
	QTK_AUOUT_DATA_END,
}qtk_auout_data_state_t;

typedef void(*qtk_auout_data_notify_func)(void *ths,qtk_auout_data_state_t state,char *data,int bytes);
typedef void(*qtk_auout_msg_notify_func)(void *notify_ths);

typedef enum {
	QTK_AUOUT_MSG_START,
	QTK_AUOUT_MSG_DATA,
	QTK_AUOUT_MSG_END,
	QTK_AUOUT_MSG_STOP,
}qtk_auout_msg_type_t;

typedef struct {
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	qtk_auout_msg_type_t type;
	wtk_strbuf_t *buf;
	int sample_rate;
	int channel;
	int bytes_per_sample;
	void *msg_notify_ths;
	qtk_auout_msg_notify_func msg_notify_func;
}qtk_auout_msg_t;


typedef int(*qtk_auout_start_f) (void *ths,char *sd_name,int sample_rate,int channel,int bytes_per_sample);
typedef int(*qtk_auout_write_f) (void *ths,char *data,int bytes);
typedef void(*qtk_auout_stop_f) (void *ths);
typedef void(*qtk_auout_clean_f)(void *ths);

typedef struct {
	qtk_auout_start_f auout_start;
	qtk_auout_write_f auout_write;
	qtk_auout_stop_f  auout_stop;
	qtk_auout_clean_f auout_clean;
	void *action_ths;
}qtk_auout_action_t;

typedef enum {
	QTK_AUOUT_INIT,
	QTK_AUOUT_PLAY,
}qtk_auout_state_t;

struct qtk_auout
{
	qtk_auout_cfg_t *cfg;
	// wtk_mp3dec2_t *mp3dec;
	wtk_log_t *log;
	wtk_pitch_t *pitch;
	wtk_strbuf_t *buf;
	wtk_thread_t thread;
	wtk_blockqueue_t input_q;
	wtk_lockhoard_t msg_hoard;
	qtk_auout_action_t actions;
	qtk_auout_data_notify_func notify_func;
	void *notify_ths;
	wtk_lock_t lock;
	float pitch_shift;
	float volume_shift;
	int step;
	int buf_time;
	int sample_rate;
	int channel;
	int bytes_per_sample;
	qtk_auout_state_t state;
	// unsigned mp3dec_active:1;
	unsigned run:1;
	unsigned stop_hint:1;
};

qtk_auout_t* qtk_auout_new(qtk_auout_cfg_t *cfg,wtk_log_t *log);
void qtk_auout_delete(qtk_auout_t *auout);

void qtk_auout_set_callback(qtk_auout_t *auout,
		void *action_ths,
		qtk_auout_start_f auout_start,
		qtk_auout_write_f auout_write,
		qtk_auout_stop_f  auout_stop,
		qtk_auout_clean_f auout_clean
		);
void qtk_auout_set_notify(qtk_auout_t *auout,void *notify_ths,qtk_auout_data_notify_func notify_func);
void qtk_auout_set_bufTime(qtk_auout_t *auout,int buf_time);

int qtk_auout_start(qtk_auout_t *auout);
void qtk_auout_stop(qtk_auout_t *auout);
/**
 * use when device plug out
 */
void qtk_auout_stop2(qtk_auout_t *auout);

void qtk_auout_play_start(qtk_auout_t *auout,int sample_rate,int channel,int bytes_per_sample);
void qtk_auout_play_feed_data(qtk_auout_t *auout,char *data,int bytes);
void qtk_auout_play_data(qtk_auout_t *auout,char *data,int bytes);
void qtk_auout_play_end(qtk_auout_t *auout,int syn);
int qtk_auout_play_file(qtk_auout_t *auout,char *fn,int syn);
int qtk_auout_play_mp3(qtk_auout_t *auout,char *fn);
void qtk_auout_stop_play(qtk_auout_t *auout);

float qtk_auout_set_volume(qtk_auout_t *auout,float volume);
float qtk_auout_inc_volume(qtk_auout_t *auout);
float qtk_auout_dec_volume(qtk_auout_t *auout);

float qtk_auout_set_pitch(qtk_auout_t *auout,float pitch);
float qtk_auout_inc_pitch(qtk_auout_t *auout);
float qtk_auout_dec_pitch(qtk_auout_t *auout);

#ifdef __cplusplus
};
#endif
#endif
