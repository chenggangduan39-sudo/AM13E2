#ifndef __SDK_ENGINE_QFORM_QTK_QFORM__
#define __SDK_ENGINE_QFORM_QTK_QFORM__

#include "qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/api_1/qform/qtk_qform.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_eqform9 qtk_eqform9_t;

struct qtk_eqform9
{
	qtk_session_t *session;
	qtk_qform_t *qform;
	qtk_qform_cfg_t *cfg;
	qtk_engine_param_t param;
	qtk_engine_thread_t *thread;
	qtk_engine_callback_t *callback;
	void *notify_ths;
	qtk_engine_notify_f notify;
};


qtk_eqform9_t* qtk_eqform9_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_eqform9_delete(qtk_eqform9_t *e);
int qtk_eqform9_start(qtk_eqform9_t *e);
int qtk_eqform9_feed(qtk_eqform9_t *e, char *data, int bytes, int is_end);
int qtk_eqform9_reset(qtk_eqform9_t *e);
int qtk_eqform9_cancel(qtk_eqform9_t *e);
void qtk_eqform9_set_notify(qtk_eqform9_t *e, void *ths, qtk_engine_notify_f notify_f);
int qtk_eqform9_set(qtk_eqform9_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
