#ifndef __SDK_ENGINE_EAGC_QTK_ENGINEAGC_H__
#define __SDK_ENGINE_EAGC_QTK_ENGINEAGC_H__

#include "qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/api_1/agc/qtk_agc.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_engineagc qtk_engineagc_t;

struct qtk_engineagc
{
	qtk_session_t *session;
	qtk_agc_t *eqform;
	qtk_engine_param_t param;
	qtk_engine_thread_t *thread;
	qtk_engine_callback_t *callback;
	void *notify_ths;
	qtk_engine_notify_f notify;
	qtk_agc_cfg_t *cfg;
	wtk_wavfile_t *wav;
};


qtk_engineagc_t* qtk_engineagc_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_engineagc_delete(qtk_engineagc_t *e);
int qtk_engineagc_start(qtk_engineagc_t *e);
int qtk_engineagc_feed(qtk_engineagc_t *e, char *data, int bytes, int is_end);
int qtk_engineagc_feed2(qtk_engineagc_t *e, char *input, int in_bytes, char *output, int *out_bytes, int is_end);
int qtk_engineagc_reset(qtk_engineagc_t *e);
int qtk_engineagc_cancel(qtk_engineagc_t *e);
void qtk_engineagc_set_notify(qtk_engineagc_t *e, void *ths, qtk_engine_notify_f notify_f);
int qtk_engineagc_set(qtk_engineagc_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
