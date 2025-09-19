#ifndef __SDK_ENGINE_AEC_QTK_EAEC_H__
#define __SDK_ENGINE_AEC_QTK_EAEC_H__

#include "qtk_eaec_cfg.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "wtk/bfio/aec/wtk_aec.h"
#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_eaec qtk_eaec_t;

struct qtk_eaec {
    qtk_session_t *session;
    qtk_engine_param_t param;
    void *ths;
    qtk_engine_notify_f notify;
    wtk_aec_t *aec;
    qtk_eaec_cfg_t *cfg;
    wtk_strbuf_t *audio;
    int cur_channel;
    int input_cap;
    short **input;
};

qtk_eaec_t *qtk_eaec_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_eaec_delete(qtk_eaec_t *bf);
int qtk_eaec_start(qtk_eaec_t *bf);
int qtk_eaec_reset(qtk_eaec_t *bf);
int qtk_eaec_feed(qtk_eaec_t *bf, char *data, int len, int is_end);
int qtk_eaec_set_notify(qtk_eaec_t *bf, void *ths, qtk_engine_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif