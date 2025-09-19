#ifndef __SDK_ENGINE_ESTIMATE_QTK_ESTIMATE__
#define __SDK_ENGINE_ESTIMATE_QTK_ESTIMATE__

#include "qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/api_1/estimate/qtk_estimate.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_eestimate qtk_eestimate_t;

struct qtk_eestimate
{
	qtk_session_t *session;
	qtk_estimate_t *eqform;
	qtk_engine_param_t param;
	qtk_engine_thread_t *thread;
	qtk_engine_callback_t *callback;
	void *notify_ths;
	qtk_engine_notify_f notify;
	qtk_estimate_cfg_t *cfg;
};


qtk_eestimate_t* qtk_eestimate_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_eestimate_delete(qtk_eestimate_t *e);
int qtk_eestimate_start(qtk_eestimate_t *e);
int qtk_eestimate_feed(qtk_eestimate_t *e, char *data, int bytes, int is_end);
int qtk_eestimate_reset(qtk_eestimate_t *e);
int qtk_eestimate_cancel(qtk_eestimate_t *e);
void qtk_eestimate_set_notify(qtk_eestimate_t *e, void *ths, qtk_engine_notify_f notify_f);
int qtk_eestimate_set(qtk_eestimate_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
