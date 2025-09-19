#ifndef __QTK_ENGINE_KVAD_H__
#define __QTK_ENGINE_KVAD_H__

#include "sdk/engine/comm/qtk_engine_ogg.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "wtk/asr/vad/kvad/wtk_kvad.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_ekvad qtk_ekvad_t;

typedef enum {
    qtk_ekvad_SPEECH_START,
    qtk_ekvad_SPEECH_DATA_OGG,
    qtk_ekvad_SPEECH_DATA_PCM,
    qtk_ekvad_SPEECH_END,
} qtk_ekvad_type_t;

struct qtk_ekvad {
    qtk_session_t *session;
    qtk_engine_param_t param;
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_main_cfg_t *main_cfg;
    wtk_kvad_cfg_t *cfg;
    wtk_kvad_t *v;
    qtk_engine_notify_f notify;
    void *notify_ths;
    qtk_engine_thread_t *thread;
    qtk_engine_callback_t *callback;
    qtk_engine_ogg_t *ogg;
    wtk_queue_t vad_q;
    int cancel;
    unsigned sil : 1;
};

qtk_ekvad_t *qtk_ekvad_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_ekvad_start(qtk_ekvad_t *e);
int qtk_ekvad_feed(qtk_ekvad_t *e, char *data, int bytes, int is_end);
int qtk_ekvad_reset(qtk_ekvad_t *e);
int qtk_ekvad_delete(qtk_ekvad_t *e);

int qtk_ekvad_cancel(qtk_ekvad_t *e);
void qtk_ekvad_set_notify(qtk_ekvad_t *e, void *notify_ths,
                          qtk_engine_notify_f notify);

int qtk_ekvad_set(qtk_ekvad_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
