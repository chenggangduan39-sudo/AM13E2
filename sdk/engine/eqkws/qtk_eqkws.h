
#ifndef __QTK_ENGINE_QKWS_QTK_EQKWS__
#define __QTK_ENGINE_QKWS_QTK_EQKWS__

#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/core/wtk_strbuf.h"

#include "sdk/api_1/kws/qtk_qkws.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_eqkws qtk_eqkws_t;

struct qtk_eqkws {
    qtk_session_t *session;
    qtk_engine_param_t param;
    qtk_qkws_cfg_t *cfg;
    qtk_qkws_t *qkws;
    qtk_engine_notify_f notify_f;
    void *notify_ths;
    qtk_engine_thread_t *thread;
    qtk_engine_callback_t *callback;
    wtk_strbuf_t *outbuf;
    int cancel;
    char *enrollfn;
};

qtk_eqkws_t *qtk_eqkws_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_eqkws_delete(qtk_eqkws_t *e);

int qtk_eqkws_start(qtk_eqkws_t *e);
int qtk_eqkws_feed(qtk_eqkws_t *e, char *data, int bytes, int is_end);
int qtk_eqkws_reset(qtk_eqkws_t *e);

int qtk_eqkws_cancel(qtk_eqkws_t *e);
void qtk_eqkws_set_notify(qtk_eqkws_t *e, void *notify_ths,
                          qtk_engine_notify_f notify_f);

int qtk_eqkws_set(qtk_eqkws_t *e, char *data, int bytes);
char *qtk_eqkws_get_fn(qtk_eqkws_t *e);
float qtk_eqkws_get_prob(qtk_eqkws_t *e);
void qtk_eqkws_get_result(qtk_eqkws_t *e, qtk_var_t *var);

#ifdef __cplusplus
};
#endif
#endif
