#ifndef __QTK_ECLOUD_H__
#define __QTK_ECLOUD_H__

#include "sdk/engine/comm/qtk_engine_hdr.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/engine/qtk_engine.h"
#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "sdk/spx/qtk_spx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*qtk_ewakeup_notify_f)(void *ths, char *data, int bytes);
typedef struct qtk_ecloud qtk_ecloud_t;
struct qtk_ecloud {
    qtk_session_t *session;
    qtk_engine_param_t param;
    qtk_spx_t *spx;
    qtk_spx_cfg_t *spx_cfg;
    wtk_strbuf_t *evaltxt;
    wtk_json_parser_t *parser;
    wtk_strbuf_t *rlt_buf;
    qtk_engine_notify_f notify_f;
    void *notify_ths;
    unsigned use_phone_ml : 1;
};

qtk_ecloud_t *qtk_ecloud_new(qtk_session_t *s, wtk_local_cfg_t *params);
int qtk_ecloud_start(qtk_ecloud_t *e);
void qtk_ecloud_delete(qtk_ecloud_t *e);
void qtk_ecloud_reset(qtk_ecloud_t *e);
int qtk_ecloud_feed(qtk_ecloud_t *e, char *data, int bytes, int is_end);
void qtk_ecloud_set(qtk_ecloud_t *e, char *data, int bytes);
void qtk_ecloud_set_notify(qtk_ecloud_t *e, void *ths,
                           qtk_engine_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif
