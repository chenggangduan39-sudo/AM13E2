#ifndef __SDK_ENGINE_TINYBF_QTK_ETINY_BF_H__
#define __SDK_ENGINE_TINYBF_QTK_ETINY_BF_H__

#include "qtk_etinybf_cfg.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "wtk/bfio/qform/wtk_qform9.h"
//#include "wtk/bfio/vbox/wtk_gainnet_bf_stereo.h"
#include "wtk/bfio/maskform/wtk_gainnet_bf_stereo.h"
#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_etinybf qtk_etinybf_t;

struct qtk_etinybf {
    qtk_session_t *session;
    qtk_engine_param_t param;
    void *ths;
    qtk_engine_notify_f notify;
    wtk_qform9_t *qform9;
    qtk_etinybf_cfg_t *cfg;
    wtk_strbuf_t *buf;
    int cur_channel;
    int input_cap;
    short **input;

    //wtk_gainnet_bf_stereo_t *vboxebf;
    wtk_gainnet_bf_stereo_t *vboxebf;
};

qtk_etinybf_t *qtk_etinybf_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_etinybf_delete(qtk_etinybf_t *bf);
int qtk_etinybf_start(qtk_etinybf_t *bf);
int qtk_etinybf_reset(qtk_etinybf_t *bf);
int qtk_etinybf_feed(qtk_etinybf_t *bf, char *data, int len, int is_end);
int qtk_etinybf_set_notify(qtk_etinybf_t *bf, void *ths,
                           qtk_engine_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif