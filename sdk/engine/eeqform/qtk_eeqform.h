#ifndef __SDK_ENGINE_EQFORM_QTK_EQFORM__
#define __SDK_ENGINE_EQFORM_QTK_EQFORM__

#include "qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/api_1/eqform/qtk_eqform.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_eeqform qtk_eeqform_t;

struct qtk_eeqform
{
	qtk_session_t *session;
	qtk_eqform_t *qform;
	qtk_eqform_cfg_t *cfg;
	qtk_engine_param_t param;
	qtk_engine_thread_t *thread;
	qtk_engine_callback_t *callback;
	void *notify_ths;
	qtk_engine_notify_f notify;
};


qtk_eeqform_t* qtk_eeqform_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_eeqform_delete(qtk_eeqform_t *e);
int qtk_eeqform_start(qtk_eeqform_t *e);
int qtk_eeqform_feed(qtk_eeqform_t *e, char *data, int bytes, int is_end);
int qtk_eeqform_reset(qtk_eeqform_t *e);
int qtk_eeqform_cancel(qtk_eeqform_t *e);
void qtk_eeqform_set_notify(qtk_eeqform_t *e, void *ths, qtk_engine_notify_f notify_f);
int qtk_eeqform_set(qtk_eeqform_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
