/*
 * qtk_eresample.h
 *
 *  Created on: Jul 7, 2023
 *      Author: root
 */

#ifndef SDK_ENGINE_RESAMPLE_QTK_ERESAMPLE_H_
#define SDK_ENGINE_RESAMPLE_QTK_ERESAMPLE_H_
#include "wtk/bfio/resample/wtk_resample.h"
#include "speex/speex_resampler.h"
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
typedef struct qtk_eresample qtk_eresample_t;
#define QTK_EBOXEBF_FEED_STEP (20 * 2 * 16)
struct qtk_eresample {
    qtk_session_t *session;
    qtk_engine_param_t param;
    wtk_resample_t *rsp;
    SpeexResamplerState *srsp;
    qtk_engine_thread_t *thread;
    qtk_engine_callback_t *callback;
    void *notify_ths;
    char *outresample;
    int outresample_size;
    qtk_engine_notify_f notify;
    unsigned feedend : 1;
};

qtk_eresample_t *qtk_eresample_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_eresample_delete(qtk_eresample_t *e);

int qtk_eresample_start(qtk_eresample_t *e);
int qtk_eresample_feed(qtk_eresample_t *e, char *data, int bytes, int is_end);
int qtk_eresample_reset(qtk_eresample_t *e);

int qtk_eresample_cancel(qtk_eresample_t *e);
void qtk_eresample_set_notify(qtk_eresample_t *e, void *ths,
                          qtk_engine_notify_f notify_f);

int qtk_eresample_set(qtk_eresample_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif /* SDK_ENGINE_VBOXEBF_QTK_Eresample_H_ */
