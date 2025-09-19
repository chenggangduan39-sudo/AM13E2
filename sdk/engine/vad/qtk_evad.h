#ifndef QTK_ENGINE_VAD_QTK_EVAD
#define QTK_ENGINE_VAD_QTK_EVAD

#include "wtk/asr/vad/wtk_vad.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"

#include "sdk/engine/comm/qtk_engine_ogg.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_evad qtk_evad_t;

typedef enum {
    QTK_EVAD_SPEECH_START,
    QTK_EVAD_SPEECH_DATA_OGG,
    QTK_EVAD_SPEECH_DATA_PCM,
    QTK_EVAD_SPEECH_END,
} qtk_evad_type_t;

struct qtk_evad {
    qtk_session_t *session;
    qtk_engine_param_t param;
    wtk_vad_cfg_t *cfg;
    wtk_vad_t *v;
    qtk_engine_notify_f notify;
    void *notify_ths;
    qtk_engine_thread_t *thread;
    qtk_engine_callback_t *callback;
    qtk_engine_ogg_t *ogg;
    wtk_queue_t vad_q;
    int cancel;
    unsigned sil : 1;
};

qtk_evad_t *qtk_evad_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_evad_start(qtk_evad_t *e);
int qtk_evad_feed(qtk_evad_t *e, char *data, int bytes, int is_end);
int qtk_evad_reset(qtk_evad_t *e);
int qtk_evad_delete(qtk_evad_t *e);

int qtk_evad_cancel(qtk_evad_t *e);
void qtk_evad_set_notify(qtk_evad_t *e, void *notify_ths,
                         qtk_engine_notify_f notify);

int qtk_evad_set(qtk_evad_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
