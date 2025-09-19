#ifndef QTK_ASR_WRAPPER_H_
#define QTK_ASR_WRAPPER_H_

#include "qtk_asr_wrapper_cfg.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/strlike/wtk_chnlike.h"
#include "wtk/asr/vad/wtk_vad.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_asr_wrapper qtk_asr_wrapper_t;
typedef struct qtk_asr_sourcer qtk_asr_sourcer_t;

typedef void (*qtk_asr_wrapper_asr_notify_t)(void *upval, char *res, int len);
typedef void (*qtk_asr_wrapper_wakeup_notify_t)(void *upval, float fs, float fe);

struct qtk_asr_sourcer{
	void* (*qtk_asr_new)(void *);
	void (*qtk_asr_reset)(void *);
	void (*qtk_asr_delete)(void *);
	int (*qtk_asr_start)(void *);
	int (*qtk_asr_start2)(void *,char *data, int bytes);
	int (*qtk_asr_feed)(void *, char *data, int bytes, int is_end);
	void (*qtk_asr_get_result)(void *,wtk_string_t *v);
	void (*qtk_asr_get_hint_result)(void *,wtk_string_t *v);
	int (*qtk_asr_set_context)(void *,char *data, int bytes);
};

struct qtk_asr_wrapper
{
	qtk_asr_wrapper_cfg_t *cfg;
	wtk_chnlike_t *chnlike;
	wtk_lex_t *lex;

	void *dec;
	qtk_asr_sourcer_t sourcer;

	wtk_vad_t *vad;
	qtk_k2_dec_t *wakeup;
	wtk_vframe_state_t last_vframe_state;
	wtk_queue_t vad_q;

	void *notify_wakeup_ths;
	qtk_asr_wrapper_wakeup_notify_t notify_wakeup;
	void *notify_asr_ths;
	qtk_asr_wrapper_asr_notify_t notify_asr;
	wtk_strbuf_t *wav_cache;//cache wav for wakeup+asr
	wtk_strbuf_t *keywords2;
	wtk_heap_t *heap;

	long unsigned int idle_time;
	long unsigned int start_frame;
	long unsigned int idle_bytes;
	int reset_bytes;
	int mode;
	int is_end;
	int frame_bytes;
	unsigned int waked:1;
	unsigned int asr:1;
	unsigned int asr_start:1;
};

qtk_asr_wrapper_t* qtk_asr_wrapper_new(qtk_asr_wrapper_cfg_t* cfg);
int qtk_asr_wrapper_start(qtk_asr_wrapper_t* wrapper);
int qtk_asr_wrapper_start2(qtk_asr_wrapper_t *wrapper, char *data, int len);
int qtk_asr_wrapper_feed(qtk_asr_wrapper_t* wrapper,char *data,int bytes,int is_end);
void qtk_asr_wrapper_reset(qtk_asr_wrapper_t* wrapper);
void qtk_asr_wrapper_delete(qtk_asr_wrapper_t* wrapper);
void qtk_asr_wrapper_get_result(qtk_asr_wrapper_t *wrapper,wtk_string_t *v);
void qtk_asr_wrapper_get_hint_result(qtk_asr_wrapper_t *wrapper,wtk_string_t *v);
void qtk_asr_wrapper_get_vad_result(qtk_asr_wrapper_t *wrapper,wtk_string_t *v);
int qtk_asr_wrapper_set_context(qtk_asr_wrapper_t *wrapper,char *data,int bytes);
int qtk_asr_wrapper_set_context_asr(qtk_asr_wrapper_t *wrapper,char *data,int bytes);
int qtk_asr_wrapper_set_context_wakeup(qtk_asr_wrapper_t *wrapper,char *data,int bytes);

void qtk_asr_wrapper_set_sourcer(qtk_asr_wrapper_t *wrapper,
		void* (*qtk_asr_new)(void *),
		void (*qtk_asr_reset)(void *),
		void (*qtk_asr_delete)(void *),
		int (*qtk_asr_start)(void *),
		int (*qtk_asr_start2)(void *,char *data, int bytes),
		int (*qtk_asr_feed)(void *, char *data, int bytes, int is_end),
		void (*qtk_asr_get_result)(void *,wtk_string_t *v),
		void (*qtk_asr_get_hint_result)(void *,wtk_string_t *v),
		int (*qtk_asr_set_context)(void *,char *data, int bytes)
		);

float qtk_asr_wrapper_set_vadindex(qtk_asr_wrapper_t * wrapper, int index);
void qtk_asr_wrapper_get_wake_time(qtk_asr_wrapper_t *wrapper,float *fs,float *fe);
float qtk_asr_wrapper_get_conf(qtk_asr_wrapper_t *wrapper);

void qtk_asr_wrapper_set_asr_notify(qtk_asr_wrapper_t *wrapper,void *upval, qtk_asr_wrapper_asr_notify_t notify);
void qtk_asr_wrapper_set_wakeup_notify(qtk_asr_wrapper_t *wrapper,void *upval, qtk_asr_wrapper_wakeup_notify_t notify);
void qtk_asr_wrapper_set_idle_time(qtk_asr_wrapper_t *wrapper, long unsigned int val);
void qtk_asr_wrapper_set_mode(qtk_asr_wrapper_t *wrapper, int mode);
int qtk_asr_wrapper_set_contacts(qtk_asr_wrapper_t *wrapper, char *data, int len);
int qtk_asr_wrapper_set_text(qtk_asr_wrapper_t *wrapper, char *data, int len);
#ifdef __cplusplus
}
;
#endif
#endif

