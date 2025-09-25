#ifndef __SDK_ENGINE_ESOUNDSCREEN_QTK_ESOUNDSCREEN__
#define __SDK_ENGINE_ESOUNDSCREEN_QTK_ESOUNDSCREEN__

#include "qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/api_1/soundScreen/qtk_soundscreen.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_esoundscreen qtk_esoundscreen_t;

struct qtk_esoundscreen
{
	qtk_session_t *session;
	qtk_soundscreen_t *soundscreen;
	qtk_engine_param_t param;
	qtk_engine_thread_t *thread;
	qtk_engine_callback_t *callback;
	void *notify_ths;
	qtk_engine_notify_f notify;
	qtk_soundscreen_cfg_t *cfg;
};


qtk_esoundscreen_t* qtk_esoundscreen_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_esoundscreen_delete(qtk_esoundscreen_t *e);
int qtk_esoundscreen_start(qtk_esoundscreen_t *e);
int qtk_esoundscreen_feed(qtk_esoundscreen_t *e, char *data, int bytes, int is_end);
int qtk_esoundscreen_reset(qtk_esoundscreen_t *e);
int qtk_esoundscreen_cancel(qtk_esoundscreen_t *e);
void qtk_esoundscreen_set_notify(qtk_esoundscreen_t *e, void *ths, qtk_engine_notify_f notify_f);
int qtk_esoundscreen_set(qtk_esoundscreen_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
