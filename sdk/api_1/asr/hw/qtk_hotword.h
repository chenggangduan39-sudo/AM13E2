#ifndef __QTK_HOTWORD_H__
#define __QTK_HOTWORD_H__

#include "jwt/qtk_jwt.h"
#include "qtk_hw_cfg.h"
#include "sdk/httpc/qtk_httpc.h"
#include "sdk/session/qtk_session.h"
#include "wtk/core/json/wtk_json.h"
#include "wtk/core/wtk_base64.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QTK_HOTWORD_URL "/api/ahive/v1/hotword"
#define QTK_HOTWORD_TOKEN_HEADER "{\"typ\":\"JWT\",\"alg\":\"HS256\"}"
#define QTK_HOTWORD_TOKEN_SIGNATURE "AhiveQdreamer"

typedef void (*qtk_hw_notify_f)(void *ths, char *data, int bytes);
typedef struct qtk_hw qtk_hw_t;
struct qtk_hw {
    qtk_hw_cfg_t *cfg;
    qtk_session_t *session;
    qtk_httpc_t *httpc;
    wtk_json_t *json;
    wtk_string_t token_payload;
    wtk_strbuf_t *token;
    wtk_strbuf_t *upload_res;
    wtk_string_t *update_res;
    void *ths;
    qtk_hw_notify_f notify;
};

qtk_hw_t *qtk_hw_new(qtk_hw_cfg_t *cfg, qtk_session_t *session);
int qtk_hw_delete(qtk_hw_t *hw);
int qtk_hw_upload(qtk_hw_t *hw, char *res_fn, int flag);
int qtk_hw_update(qtk_hw_t *hw, char *res_fn, int flag);
int qtk_hw_get_hotword(qtk_hw_t *hw);
void qtk_hw_set_notify(qtk_hw_t *hw, void *ths, qtk_hw_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif