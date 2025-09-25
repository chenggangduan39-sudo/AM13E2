#ifndef QTK_ENGINE_TTS_QTK_ETTS
#define QTK_ENGINE_TTS_QTK_ETTS

#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"

#include "sdk/api_1/tts/qtk_tts.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_etts qtk_etts_t;

struct qtk_etts {
    qtk_session_t *session;
    qtk_engine_param_t param;
    qtk_tts_cfg_t *cfg;
    qtk_tts_t *t;
    void *notify_ths;
    qtk_engine_notify_f notify;
    qtk_engine_thread_t *thread;
    qtk_engine_callback_t *callback;
    unsigned ended : 1;
};

qtk_etts_t *qtk_etts_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_etts_start(qtk_etts_t *e);
int qtk_etts_feed(qtk_etts_t *e, char *data, int bytes, int is_end);
int qtk_etts_reset(qtk_etts_t *e);
int qtk_etts_delete(qtk_etts_t *e);

int qtk_etts_cancel(qtk_etts_t *e);
void qtk_etts_set_notify(qtk_etts_t *e, void *notify_ths,
                         qtk_engine_notify_f notify);

int qtk_etts_set(qtk_etts_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
