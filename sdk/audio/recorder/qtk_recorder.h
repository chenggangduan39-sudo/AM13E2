#ifndef SDK_AUDIO_QTK_RECORDER
#define SDK_AUDIO_QTK_RECORDER

#include "sdk/audio/util/qtk_asound_card.h"
#include "qtk_recorder_module.h"
#include "qtk_recorder_cfg.h"
#include "wtk/os/wtk_log.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_recorder qtk_recorder_t;

typedef void (*qtk_recorder_notify_f)(void *ths,int is_err);

struct qtk_recorder
{
	qtk_recorder_cfg_t *cfg;
	wtk_log_t *log;
	wtk_strbuf_t *buf;
	qtk_recorder_module_t rcder_module;
	qtk_recorder_notify_f err_notify_func;
	void *err_notify_ths;
	unsigned int sample_rate;
	unsigned int channel;
	unsigned int bytes_per_sample;
	unsigned int read_fail_times;
};

qtk_recorder_t* qtk_recorder_new(qtk_recorder_cfg_t *cfg,qtk_session_t *session,void * notify_ths,qtk_recorder_notify_f notify_func);
void qtk_recorder_delete(qtk_recorder_t *r);
void qtk_recorder_set_err_notify(qtk_recorder_t *r,void *err_notify_ths,qtk_recorder_notify_f err_notify_func);
void qtk_recorder_set_fmt(qtk_recorder_t *r,int sample_rate,int channel,int bytes_per_sample);

int qtk_recorder_start(qtk_recorder_t *r);
wtk_strbuf_t* qtk_recorder_read(qtk_recorder_t *r);
void qtk_recorder_stop(qtk_recorder_t *r);
void qtk_recorder_clean(qtk_recorder_t *r);
int qtk_recorder_isErr(qtk_recorder_t *r);

void qtk_recorder_set_callback(qtk_recorder_t *r,
		void *handler,
		qtk_recorder_start_func start_func,
		qtk_recorder_stop_func  stop_func,
		qtk_recorder_read_func  read_func,
		qtk_recorder_clean_func clean_func
		);

#ifdef __cplusplus
};
#endif
#endif
