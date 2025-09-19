#ifndef __SDK_ENGINE_EAEC_QTK_ENGINEAEC_H__
#define __SDK_ENGINE_EAEC_QTK_ENGINEAEC_H__

#include "qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/api_1/aec/qtk_aec.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_engineaec qtk_engineaec_t;

struct qtk_engineaec
{
	qtk_session_t *session;
	qtk_aec_t *eqform;
	qtk_engine_param_t param;
	qtk_engine_thread_t *thread;
	qtk_engine_callback_t *callback;
	void *notify_ths;
	qtk_engine_notify_f notify;
	qtk_aec_cfg_t *cfg;
	wtk_wavfile_t *wav;
};


qtk_engineaec_t* qtk_engineaec_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_engineaec_delete(qtk_engineaec_t *e);
int qtk_engineaec_start(qtk_engineaec_t *e);
int qtk_engineaec_feed(qtk_engineaec_t *e, char *data, int bytes, int is_end);
int qtk_engineaec_reset(qtk_engineaec_t *e);
int qtk_engineaec_cancel(qtk_engineaec_t *e);
void qtk_engineaec_set_notify(qtk_engineaec_t *e, void *ths, qtk_engine_notify_f notify_f);
int qtk_engineaec_set(qtk_engineaec_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
