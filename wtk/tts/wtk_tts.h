#ifndef WTK_TFIRE_KEL_TTS_WTK_TTS_H_
#define WTK_TFIRE_KEL_TTS_WTK_TTS_H_
#include "wtk/core/wtk_type.h"
#include "wtk_tts_cfg.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_blockqueue.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts wtk_tts_t;
typedef int (*wtk_pitch_callback_f)(void *ths);
struct wtk_tts
{
	wtk_tts_cfg_t *cfg;
	wtk_tts_parser_t *parser;
	wtk_syn_t *syn;
	wtk_pitch_t *pitch;
	void *ths;
	wtk_tts_notify_f notify;
	void *notify_info_ths;
	wtk_tts_notify_f notify_info;

	//tts start/end by dmd 2017.05.09
	void *notify_start_ths;
	wtk_tts_notify_f notify_start;
	void *notify_end_ths;
	wtk_tts_notify_f notify_end;

	float pitch_shit;
	float speed;//0.8 1.0 1.2

	wtk_strbuf_t *sil_buf;
	wtk_strbuf_t *sil_speech_buf;
	wtk_blockqueue_t msg_q;
	wtk_thread_t thread;
	wtk_sem_t wait_sem;
	wtk_sem_t pause_sem;
	wtk_strbuf_t *pitch_buf;
	wtk_pitch_callback_f pitch_start;
	wtk_pitch_callback_f pitch_end;
	void* pitch_ths;
	int snt_sil_time;
	int min_sil_time;
	unsigned stop_hint:1;
	unsigned pause_hint:1;
	unsigned run:1;
};

typedef enum
{
	WTK_TTS_MSG_AUDIO,
	WTK_TTS_MSG_STOP,
}wtk_tts_msg_type_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_tts_msg_type_t type;
	wtk_string_t *data;
}wtk_tts_msg_t;
wtk_tts_msg_t* wtk_tts_msg_new(char *data,int len);
void wtk_tts_msg_delete(wtk_tts_msg_t *msg);

wtk_tts_t *wtk_tts_new(wtk_tts_cfg_t *cfg);
wtk_tts_t *wtk_tts_new2(wtk_tts_cfg_t *cfg,wtk_pitch_callback_f start, wtk_pitch_callback_f end, void*ths);
int wtk_tts_delete(wtk_tts_t *tts);
int wtk_tts_reset(wtk_tts_t *tts);
int wtk_tts_pause(wtk_tts_t *tts);
int wtk_tts_resume(wtk_tts_t *tts);
void wtk_tts_set_volume_scale(wtk_tts_t *tts,float scale);
void wtk_tts_set_minsil_time(wtk_tts_t* tts, int time);
void wtk_tts_set_sntsil_time(wtk_tts_t* tts, int time);
void wtk_tts_set_stop_hint(wtk_tts_t *tts);
void wtk_tts_set_notify(wtk_tts_t *tts,void *ths,wtk_tts_notify_f notify);
void wtk_tts_set_info_notify(wtk_tts_t *tts,void *ths,wtk_tts_notify_f notify);
void wtk_tts_set_start_notify(wtk_tts_t *tts,void *ths,wtk_tts_notify_f notify);
void wtk_tts_set_end_notify(wtk_tts_t *tts,void *ths,wtk_tts_notify_f notify);
int wtk_tts_process(wtk_tts_t *tts,char *txt,int txt_bytes);
void wtk_tts_set_speed(wtk_tts_t *tts,float speed);
void wtk_tts_set_pitch(wtk_tts_t *tts,float pitch);
int wtk_tts_get_current_syn_snt_index(wtk_tts_t *tts);
int wtk_tts_process2(wtk_tts_t *tts,char *txt,int txt_bytes);
wtk_string_t wtk_tts_get_cur_timeinfo(wtk_tts_t *s);
void wtk_tts_notify(wtk_tts_t *t,char *data,int len);
void wtk_tts_defpron_setwrd(wtk_tts_t* t, wtk_string_t* k, wtk_string_t* v);
int wtk_tts_bytes(wtk_tts_t *tts);
#ifdef __cplusplus
};
#endif
#endif
