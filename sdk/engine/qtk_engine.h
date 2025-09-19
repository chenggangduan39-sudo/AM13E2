#ifndef QTK_QTK_ENGINE
#define QTK_QTK_ENGINE

#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/wtk_str_hash.h"
#include "wtk/os/wtk_log.h"

#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_engine qtk_engine_t;

typedef void *(*qtk_engine_new_func)(qtk_session_t *session,
                                     wtk_local_cfg_t *params);
typedef int (*qtk_engine_del_func)(void *handler);
typedef int (*qtk_engine_start_func)(void *handler);
typedef int (*qtk_engine_feed_func)(void *handler, char *data, int bytes,
                                    int is_end);
typedef int (*qtk_engine_feed2_func)(void *handler, char *intput,int in_bytes,
                                    char *output, int *out_bytes, int is_end);
typedef int (*qtk_engine_reset_func)(void *handler);
typedef int (*qtk_engine_cancel_func)(void *handler);
typedef int (*qtk_engine_set_func)(void *handler, char *data, int bytes);
typedef int (*qtk_engine_set_xbnf_func)(void *handler, char *xbnf, int bytes);
typedef char *(*qtk_engine_get_fn_func)(void *handler);
typedef float (*qtk_engine_get_prob_func)(void *handler);
typedef void (*qtk_engine_get_result_func)(void *handler, qtk_var_t *var);
typedef void (*qtk_engine_set_notify_func)(void *handler, void *notify_ths,
                                           qtk_engine_notify_f notify_f);

typedef struct {
    qtk_engine_new_func new_func;
    qtk_engine_del_func del_func;
    qtk_engine_start_func start_func;
    qtk_engine_feed_func feed_func;
    qtk_engine_feed2_func feed2_func;
    qtk_engine_reset_func reset_func;
    qtk_engine_cancel_func cancel_func;
    qtk_engine_set_notify_func set_notify_func;
    qtk_engine_set_func set_func;
    qtk_engine_set_xbnf_func set_xbnf_func;
    qtk_engine_get_fn_func get_fn_func;
    qtk_engine_get_prob_func get_prob_func;
    qtk_engine_get_result_func get_result_func;
} qtk_engine_action_t;

typedef enum {
    QTK_ENGINE_ASR,
    QTK_ENGINE_BFIO,
    QTK_ENGINE_CSR,
    QTK_ENGINE_EVAL,
    QTK_ENGINE_SEMDLG,
    QTK_ENGINE_TTS,
    QTK_ENGINE_VAD,
    QTK_ENGINE_WAKEUP,
    QTK_ENGINE_KVAD,
    QTK_ENGINE_TINYBF,
    QTK_ENGINE_AEC,
    QTK_ENGINE_VBOXEBF3,
    QTK_ENGINE_WDEC,
	QTK_ENGINE_VBOXEBF,
	QTK_ENGINE_GDENOISE,
	QTK_ENGINE_GAINNETBF,
	QTK_ENGINE_CONSIST,
	QTK_ENGINE_SSL,
	QTK_ENGINE_SOUNDSCREEN,
	QTK_ENGINE_QFORM,
    QTK_ENGINE_QKWS,
    QTK_ENGINE_CSRSC,
    QTK_ENGINE_EQFORM,
    QTK_ENGINE_ESTIMATE,
    QTK_ENGINE_RESAMPLE,
    QTK_ENGINE_EIMG,
    QTK_ENGINE_BFIO2,
    QTK_ENGINE_AGC,
    QTK_ENGINE_MASKBFNET,
} qtk_engine_type_t;

struct qtk_engine {
    qtk_session_t *session;
    wtk_cfg_file_t *params;
    qtk_engine_action_t *actions;
    void *handler;
    qtk_engine_type_t type;
};

#ifdef __cplusplus
};
#endif
#endif
