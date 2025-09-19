#ifndef QTK_UTIL_SPX_QTK_SPX
#define QTK_UTIL_SPX_QTK_SPX

#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_thread.h"

#include "qtk_spx_cfg.h"
#include "sdk/api_1/asr/qtk_iasr_cfg.h"
#include "qtk_spx_msg.h"
#include "sdk/codec/oggenc/qtk_oggenc.h"
#include "sdk/session/cldhub/qtk_cldhub.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_spx qtk_spx_t;

typedef void (*qtk_spx_add_hdr_func)(void *add_hdr_ths, wtk_json_t *json,
                                     wtk_json_item_t *item);
typedef void (*qtk_spx_notify_func)(void *ths, qtk_spx_msg_t *msg);

struct qtk_spx {
    qtk_iasr_cfg_t* iasr_cfg;
    qtk_spx_cfg_t *cfg;
    qtk_session_t *session;
    qtk_cldhub_t *cldhub;
    qtk_oggenc_t *oggenc;
    wtk_strbuf_t *buf;
    wtk_strbuf_t *env;
    wtk_strbuf_t *hook;
    wtk_string_t *id;
    wtk_json_parser_t *parser;
    qtk_spx_add_hdr_func add_hdr_func;
    void *add_hdr_ths;
    void *ths;
    qtk_spx_notify_func notify_func;
    wtk_string_t *res;
    wtk_string_t *semRes;
    wtk_string_t *coreType;
    wtk_lockhoard_t msg_hoard;
    wtk_lock_t lock;
    unsigned long counter;
};

qtk_spx_t *qtk_spx_new(qtk_spx_cfg_t *cfg, qtk_session_t *session);
void qtk_spx_delete(qtk_spx_t *spx);
void qtk_spx_set_add_hdr(qtk_spx_t *spx, void *add_hdr_ths,
                         qtk_spx_add_hdr_func add_hdr_func);
void qtk_spx_set_notify(qtk_spx_t *spx, void *ths,
                        qtk_spx_notify_func notify_func);
int qtk_spx_start(qtk_spx_t *spx, char *txt, int bytes, int use_json);
int qtk_spx_feed(qtk_spx_t *spx, char *data, int bytes, int is_end);
void qtk_spx_reset(qtk_spx_t *spx);
void qtk_spx_cancel(qtk_spx_t *spx);

void qtk_spx_set_env(qtk_spx_t *spx, char *env, int bytes);
void qtk_spx_clean_env(qtk_spx_t *spx);

qtk_spx_msg_t *qtk_spx_get_result(qtk_spx_t *spx);
qtk_spx_msg_t *qtk_spx_pop_msg(qtk_spx_t *spx);
void qtk_spx_push_msg(qtk_spx_t *spx, qtk_spx_msg_t *msg);

void qtk_spx_set_coreType(qtk_spx_t *spx, char *data, int len);
void qtk_spx_set_res(qtk_spx_t *spx, char *data, int len);
void qtk_spx_set_semRes(qtk_spx_t *spx, char *data, int len);
void qtk_spx_set_useStream(qtk_spx_t *spx, int useStream);
void qtk_spx_set_skip_space(qtk_spx_t *spx, int skip_space);
void qtk_spx_set_speed(qtk_spx_t *spx, float speed);
void qtk_spx_set_volume(qtk_spx_t *spx, float volume);
void qtk_spx_set_pitch(qtk_spx_t *spx, float pitch);

#ifdef __cplusplus
};
#endif
#endif
