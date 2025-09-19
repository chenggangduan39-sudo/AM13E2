
#ifndef QTK_ENGINE_EVAL_QTK_EEVAL
#define QTK_ENGINE_EVAL_QTK_EEVAL

#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"

#include "sdk/api_1/eval/qtk_eval.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_eeval qtk_eeval_t;

struct qtk_eeval {
    qtk_session_t *session;
    qtk_engine_param_t param;
    qtk_eval_cfg_t *cfg;
    qtk_eval_t *eval;
    qtk_engine_notify_f notify_f;
    void *notify_ths;
    qtk_engine_thread_t *thread;
    qtk_engine_callback_t *callback;
    wtk_strbuf_t *evaltxt;
    int cancel;
};

qtk_eeval_t *qtk_eeval_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_eeval_delete(qtk_eeval_t *e);

int qtk_eeval_start(qtk_eeval_t *e);
int qtk_eeval_feed(qtk_eeval_t *e, char *data, int bytes, int is_end);
int qtk_eeval_reset(qtk_eeval_t *e);

int qtk_eeval_cancel(qtk_eeval_t *e);
void qtk_eeval_set_notify(qtk_eeval_t *e, void *notify_ths,
                          qtk_engine_notify_f notify_f);

int qtk_eeval_set(qtk_eeval_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
