#ifndef QTK_API_1_SEMDLG_QTK_SEMDLG
#define QTK_API_1_SEMDLG_QTK_SEMDLG

#include "qtk_isemdlg.h"
#include "qtk_semdlg_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_semdlg qtk_semdlg_t;
struct qtk_semdlg {
    wtk_queue_t semdlg_q;
};

qtk_semdlg_t *qtk_semdlg_new(qtk_semdlg_cfg_t *cfg, qtk_session_t *session,
                             wtk_model_t *model);
void qtk_semdlg_delete(qtk_semdlg_t *semdlg);

wtk_string_t qtk_semdlg_process(qtk_semdlg_t *semdlg, char *data, int bytes,
                                int use_json);

void qtk_semdlg_set_env(qtk_semdlg_t *semdlg, char *env, int bytes);
void qtk_semdlg_set_coreType(qtk_semdlg_t *semdlg, char *data, int len);
void qtk_semdlg_set_semRes(qtk_semdlg_t *semdlg, char *data, int len);

#ifdef __cplusplus
};
#endif
#endif
