#ifndef QTK_WAKEUP_WRAPPER_H_
#define QTK_WAKEUP_WRAPPER_H_

#include "qtk_wakeup_wrapper_cfg.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/asr/vad/wtk_vad2.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_wakeup_wrapper qtk_wakeup_wrapper_t;
typedef struct qtk_wakeup_sourcer qtk_wakeup_sourcer_t;

typedef void (*qtk_wakeup_wrapper_notify_t)(void *upval, float fs, float fe);

struct qtk_wakeup_sourcer{
	void* (*qtk_wakeup_new)(void *);
	void (*qtk_wakeup_reset)(void *);
	void (*qtk_wakeup_delete)(void *);
	int (*qtk_wakeup_start)(void *);
	int (*qtk_wakeup_feed)(void *, char *data, int bytes, int is_end);
	int (*qtk_wakeup_set_context)(void *,char *data, int bytes);
	float (*qtk_wakeup_get_conf)(void *);
	void (*qtk_wakeup_get_wake_time)(void *,float *fs, float *fe);
};

struct qtk_wakeup_wrapper
{
	qtk_wakeup_wrapper_cfg_t *cfg;
	void *dec;
	qtk_wakeup_sourcer_t sourcer;
	wtk_vad_t *vad;
	wtk_vad2_t *vad2;
	wtk_vframe_state_t last_vframe_state;
	wtk_queue_t vad_q;

	void *notify_wakeup_ths;
	qtk_wakeup_wrapper_notify_t notify_wakeup;

	long unsigned int idle_time;
	long unsigned int start_frame;
	long unsigned int idle_bytes;
	int reset_bytes;
	int is_end;
	int frame_bytes;
	unsigned int waked:1;
};

qtk_wakeup_wrapper_t* qtk_wakeup_wrapper_new(qtk_wakeup_wrapper_cfg_t* cfg);
int qtk_wakeup_wrapper_start(qtk_wakeup_wrapper_t* wrapper);
int qtk_wakeup_wrapper_feed(qtk_wakeup_wrapper_t* wrapper,char *data,int bytes,int is_end);
void qtk_wakeup_wrapper_reset(qtk_wakeup_wrapper_t* wrapper);
void qtk_wakeup_wrapper_delete(qtk_wakeup_wrapper_t* wrapper);
void qtk_wakeup_wrapper_get_result(qtk_wakeup_wrapper_t *wrapper,wtk_string_t *v);
void qtk_wakeup_wrapper_get_hint_result(qtk_wakeup_wrapper_t *wrapper,wtk_string_t *v);
void qtk_wakeup_wrapper_get_vad_result(qtk_wakeup_wrapper_t *wrapper,wtk_string_t *v);
int qtk_wakeup_wrapper_set_context(qtk_wakeup_wrapper_t *wrapper,char *data,int bytes);
int qtk_wakeup_wrapper_set_context_wakeup(qtk_wakeup_wrapper_t *wrapper,char *data,int bytes);

void qtk_wakeup_wrapper_set_sourcer(qtk_wakeup_wrapper_t *wrapper,
		void* (*qtk_wakeup_new)(void *),
		void (*qtk_wakeup_reset)(void *),
		void (*qtk_wakeup_delete)(void *),
		int (*qtk_wakeup_start)(void *),
		int (*qtk_wakeup_feed)(void *, char *data, int bytes, int is_end),
		int (*qtk_wakeup_set_context)(void *,char *data, int bytes),
		float (*qtk_wakeup_get_conf)(void *),
		void (*qtk_wakeup_get_wake_time)(void *,float *fs, float *fe)
		);

float qtk_wakeup_wrapper_set_vadindex(qtk_wakeup_wrapper_t * wrapper, int index);
void qtk_wakeup_wrapper_get_wake_time(qtk_wakeup_wrapper_t *wrapper,float *fs,float *fe);
float qtk_wakeup_wrapper_get_conf(qtk_wakeup_wrapper_t *wrapper);

void qtk_wakeup_wrapper_set_wakeup_notify(qtk_wakeup_wrapper_t *wrapper,void *upval, qtk_wakeup_wrapper_notify_t notify);
void qtk_wakeup_wrapper_set_idle_time(qtk_wakeup_wrapper_t *wrapper, long unsigned int val);
#ifdef __cplusplus
}
;
#endif
#endif

