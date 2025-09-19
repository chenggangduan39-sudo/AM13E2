#ifndef __SDK_ENGINE_GDENOISE_QTK_GDENOISE__
#define __SDK_ENGINE_GDENOISE_QTK_GDENOISE__

#include "qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/api_1/gdenoise/qtk_gdenoise.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_egdenoise qtk_egdenoise_t;

struct qtk_egdenoise
{
	qtk_session_t *session;
	qtk_gdenoise_t *eqform;
	qtk_engine_param_t param;
	qtk_engine_thread_t *thread;
	qtk_engine_callback_t *callback;
	void *notify_ths;
	qtk_engine_notify_f notify;
	qtk_gdenoise_cfg_t *cfg;
};


qtk_egdenoise_t* qtk_egdenoise_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_egdenoise_delete(qtk_egdenoise_t *e);
int qtk_egdenoise_start(qtk_egdenoise_t *e);
int qtk_egdenoise_feed(qtk_egdenoise_t *e, char *data, int bytes, int is_end);
int qtk_egdenoise_reset(qtk_egdenoise_t *e);
int qtk_egdenoise_cancel(qtk_egdenoise_t *e);
void qtk_egdenoise_set_notify(qtk_egdenoise_t *e, void *ths, qtk_engine_notify_f notify_f);
int qtk_egdenoise_set(qtk_egdenoise_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
