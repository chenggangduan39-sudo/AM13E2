#ifndef __SDK_MODULE_QTK_MAVSPEECH_H__
#define __SDK_MODULE_QTK_MAVSPEECH_H__

#include "qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "qtk/avspeech/qtk_avspeech_separator.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_mavspeech qtk_mavspeech_t;

struct qtk_mavspeech
{
	qtk_session_t *session;
	qtk_avspeech_separator_t *sep;
	qtk_engine_param_t param;
	qtk_engine_thread_t *thread;
	qtk_engine_callback_t *callback;
	void *notify_ths;
	qtk_engine_notify_f notify;
	qtk_avspeech_separator_cfg_t *cfg;
	wtk_main_cfg_t *main_cfg;
	wtk_wavfile_t *wav;
	int channel;
};


qtk_mavspeech_t* qtk_mavspeech_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_mavspeech_delete(qtk_mavspeech_t *e);
int qtk_mavspeech_start(qtk_mavspeech_t *e);
int qtk_mavspeech_feed_audio(qtk_mavspeech_t *e, char *data, int bytes, int is_end);
int qtk_mavspeech_feed_image(qtk_mavspeech_t *e, char *data);
int qtk_mavspeech_reset(qtk_mavspeech_t *e);
int qtk_mavspeech_cancel(qtk_mavspeech_t *e);
void qtk_mavspeech_set_notify(qtk_mavspeech_t *e, void *ths, qtk_engine_notify_f notify_f);
int qtk_mavspeech_set(qtk_mavspeech_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
