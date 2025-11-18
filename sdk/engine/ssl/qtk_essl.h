#ifndef __SDK_ENGINE_SSL_QTK_SSL__
#define __SDK_ENGINE_SSL_QTK_SSL__

#include "qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/api_1/ssl/qtk_ssl.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_essl qtk_essl_t;

struct qtk_essl
{
	qtk_session_t *session;
	qtk_ssl_t *eqform;
	qtk_engine_param_t param;
	qtk_engine_thread_t *thread;
	qtk_engine_callback_t *callback;
	void *notify_ths;
	qtk_engine_notify_f notify;
	qtk_ssl_cfg_t *cfg;
};


qtk_essl_t* qtk_essl_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_essl_delete(qtk_essl_t *e);
int qtk_essl_start(qtk_essl_t *e);
int qtk_essl_feed(qtk_essl_t *e, char *data, int bytes, int is_end);
int qtk_essl_reset(qtk_essl_t *e);
int qtk_essl_cancel(qtk_essl_t *e);
void qtk_essl_set_notify(qtk_essl_t *e, void *ths, qtk_engine_notify_f notify_f);
int qtk_essl_set(qtk_essl_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
