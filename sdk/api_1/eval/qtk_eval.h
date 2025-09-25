
#ifndef QTK_API_EVALC_QTK_EVAL
#define QTK_API_EVALC_QTK_EVAL

#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/edu/eval/engsnt/wtk_engsnt.h"

#include "spx/qtk_spx.h"
#include "qtk_eval_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_eval qtk_eval_t;
struct qtk_eval {
    qtk_eval_cfg_t *cfg;
    qtk_session_t *session;
    union {
        qtk_spx_t *spx;
        wtk_engsnt_t *engsnt;
    } ins;
    wtk_strbuf_t *rlt_buf;
    wtk_json_parser_t *parser;
};

qtk_eval_t *qtk_eval_new(qtk_eval_cfg_t *cfg, qtk_session_t *session);
void qtk_eval_delete(qtk_eval_t *eval);

int qtk_eval_start(qtk_eval_t *eval, char *evaltxt, int len);
int qtk_eval_feed(qtk_eval_t *eval, char *data, int bytes, int is_end);
wtk_string_t qtk_eval_get_result(qtk_eval_t *eval);
int qtk_eval_reset(qtk_eval_t *eval);

void qtk_eval_cancel(qtk_eval_t *eval);
void qtk_eval_set_coreType(qtk_eval_t *eval, char *data, int len);
void qtk_eval_set_res(qtk_eval_t *eval, char *data, int len);

#ifdef __cplusplus
};
#endif
#endif
