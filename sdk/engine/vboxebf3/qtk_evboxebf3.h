/*
 * qtk_evboxebf3.h
 *
 *  Created on: Jul 7, 2023
 *      Author: root
 */

#ifndef SDK_ENGINE_VBOXEBF_QTK_EVBOXEBF3_H_
#define SDK_ENGINE_VBOXEBF_QTK_EVBOXEBF3_H_
#include "wtk/bfio/vbox/wtk_vboxebf3.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"

#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/session/qtk_session.h"
#include "sdk/qtk_api.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_evboxebf3 qtk_evboxebf3_t;
#define QTK_EBOXEBF_FEED_STEP (20 * 2 * 16)
struct qtk_evboxebf3 {
    qtk_session_t *session;
    qtk_engine_param_t param;
    wtk_vboxebf3_cfg_t *cfg;
    wtk_vboxebf3_t *b;
    qtk_engine_thread_t *thread;
    qtk_engine_callback_t *callback;
    void *notify_ths;
    qtk_engine_notify_f notify;
    unsigned feedend : 1;
};

qtk_evboxebf3_t *qtk_evboxebf3_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_evboxebf3_delete(qtk_evboxebf3_t *e);

int qtk_evboxebf3_start(qtk_evboxebf3_t *e);
int qtk_evboxebf3_feed(qtk_evboxebf3_t *e, char *data, int bytes, int is_end);
int qtk_evboxebf3_reset(qtk_evboxebf3_t *e);

int qtk_evboxebf3_cancel(qtk_evboxebf3_t *e);
void qtk_evboxebf3_set_notify(qtk_evboxebf3_t *e, void *ths,
                          qtk_engine_notify_f notify_f);

int qtk_evboxebf3_set(qtk_evboxebf3_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif /* SDK_ENGINE_VBOXEBF_QTK_EVBOXEBF3_H_ */
