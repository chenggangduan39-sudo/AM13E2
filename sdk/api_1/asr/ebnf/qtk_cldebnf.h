#ifndef QTK_API_1_ASR_EBNF_QTK_CLDEBNF
#define QTK_API_1_ASR_EBNF_QTK_CLDEBNF

#include "wtk/core/json/wtk_json_parse.h"

#include "qtk_cldebnf_cfg.h"
#include "sdk/httpc/qtk_httpc.h"
#include "sdk/session/qtk_session.h"

#include "md5.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_cldebnf qtk_cldebnf_t;

typedef enum {
    QTK_CLDEBNF_INIT,
    QTK_CLDEBNF_VALID,
    QTK_CLDEBNF_INVALID,
} qtk_cldebnf_state_t;

struct qtk_cldebnf {
    qtk_cldebnf_cfg_t *cfg;
    qtk_session_t *session;
    qtk_httpc_t *httpc;
    wtk_string_t *ebnf_data;
    wtk_string_t *md5Id;
    wtk_strbuf_t *url;
    wtk_thread_t thread;
    wtk_sem_t sem;
    qtk_cldebnf_state_t state;
    unsigned run : 1;
};

qtk_cldebnf_t *qtk_cldebnf_new(qtk_cldebnf_cfg_t *cfg, qtk_session_t *session);
void qtk_cldebnf_delete(qtk_cldebnf_t *cnf);

void qtk_cldebnf_process(qtk_cldebnf_t *cnf, char *data, int len);
wtk_string_t *qtk_cldebnf_check(qtk_cldebnf_t *cnf);

#ifdef __cplusplus
};
#endif
#endif
