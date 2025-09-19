#ifndef __SDK_ENGINE_EMASKBFNET_QTK_ENGINMASKBFNET_H__
#define __SDK_ENGINE_EMASKBFNET_QTK_ENGINMASKBFNET_H__

#include "qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/api_1/maskbfnet/qtk_maskbfnet.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_emaskbfnet qtk_emaskbfnet_t;

struct qtk_emaskbfnet
{
	qtk_session_t *session;
	qtk_maskbfnet_t *eqform;
	qtk_engine_param_t param;
	qtk_engine_thread_t *thread;
	qtk_engine_callback_t *callback;
	void *notify_ths;
	qtk_engine_notify_f notify;
	qtk_maskbfnet_cfg_t *cfg;
	wtk_wavfile_t *wav;
};


qtk_emaskbfnet_t* qtk_emaskbfnet_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_emaskbfnet_delete(qtk_emaskbfnet_t *e);
int qtk_emaskbfnet_start(qtk_emaskbfnet_t *e);
int qtk_emaskbfnet_feed(qtk_emaskbfnet_t *e, char *data, int bytes, int is_end);
int qtk_emaskbfnet_feed2(qtk_emaskbfnet_t *e, char *input, int in_bytes, char *output, int *out_bytes, int is_end);
int qtk_emaskbfnet_reset(qtk_emaskbfnet_t *e);
int qtk_emaskbfnet_cancel(qtk_emaskbfnet_t *e);
void qtk_emaskbfnet_set_notify(qtk_emaskbfnet_t *e, void *ths, qtk_engine_notify_f notify_f);
int qtk_emaskbfnet_set(qtk_emaskbfnet_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
