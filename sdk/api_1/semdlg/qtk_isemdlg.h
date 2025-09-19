#ifndef QTK_API_1_SEMDLG_QTK_ISEMDLG
#define QTK_API_1_SEMDLG_QTK_ISEMDLG

#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/core/wtk_model.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/semdlg/wtk_semdlg.h"

#include "qtk_isemdlg_cfg.h"
#include "sdk/session/qtk_session.h"
#include "sdk/spx/qtk_spx.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_isemdlg qtk_isemdlg_t;
struct qtk_isemdlg {
    wtk_queue_node_t q_n;
    qtk_isemdlg_cfg_t *cfg;
    qtk_session_t *session;
    union {
        wtk_semdlg_t *semdlg;
        qtk_spx_t *spx;
    } ins;
    wtk_json_parser_t *parser;
    wtk_strbuf_t *input;
    wtk_strbuf_t *result;
    wtk_model_item_t *semdlg_item;
    unsigned syn : 1;
    unsigned use_json : 1;
};

qtk_isemdlg_t *qtk_isemdlg_new(qtk_isemdlg_cfg_t *cfg, qtk_session_t *session,
                               wtk_model_t *model);
void qtk_isemdlg_delete(qtk_isemdlg_t *i);

wtk_string_t qtk_isemdlg_process(qtk_isemdlg_t *i, char *data, int bytes,
                                 int use_json);

void qtk_isemdlg_set_syn(qtk_isemdlg_t *i, int syn);
void qtk_isemdlg_set_env(qtk_isemdlg_t *i, char *env, int bytes);
void qtk_isemdlg_set_coreType(qtk_isemdlg_t *i, char *data, int len);
void qtk_isemdlg_set_semRes(qtk_isemdlg_t *i, char *data, int len);

#ifdef __cplusplus
};
#endif
#endif
