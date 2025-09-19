#ifndef QTK_API_1_ASR_QTK_ASR
#define QTK_API_1_ASR_QTK_ASR

#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/core/wtk_str_encode.h"
#include "wtk/os/wtk_lock.h"
#include "wtk/lex/wtk_lex_cfg.h"

#include "qtk_asr_cfg.h"
#include "qtk_iasr_route.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    QTK_ASR_NOT_CREDIBLE,
    QTK_ASR_INVALID_AUDIO,
} qtk_asr_err_t;

typedef struct qtk_asr qtk_asr_t;
struct qtk_asr {
    wtk_lex_t* lex;
    wtk_strbuf_t* lex_buf;
    qtk_asr_cfg_t *cfg;
    qtk_session_t *session;
    wtk_strbuf_t *cld_ebnf;
    qtk_iasr_t **iasrs;
    qtk_iasr_route_t **routes;
    wtk_strbuf_t *rec;
    wtk_json_parser_t *parser;
    void *hint_ths;
    qtk_iasr_set_hint_notify_f hint_notify;
    void *spxfinal_ths;
    qtk_iasr_set_spxfinal_notify_f spxfinal_notify;
    void *restult_ths;
    qtk_iasr_set_result_f result_notify;
    wtk_lock_t lock;
    wtk_lock_t lex_lock;
    qtk_asr_err_t err;
    int wait;
    int waited;
    int notifyed;
    int rec_priority;
    int usrec_priority;
    int n_iasrs;
    unsigned valid : 1;
    unsigned usrec : 1;
    unsigned skip_space : 1;
};

qtk_asr_t *qtk_asr_new(qtk_asr_cfg_t *cfg, qtk_session_t *session);
void qtk_asr_delete(qtk_asr_t *asr);
int qtk_asr_start(qtk_asr_t *asr);
int qtk_asr_feed(qtk_asr_t *asr, char *data, int bytes, int is_end);
wtk_string_t qtk_asr_get_result(qtk_asr_t *asr);
void qtk_asr_reset(qtk_asr_t *asr);
void qtk_asr_cancel(qtk_asr_t *asr);
void qtk_asr_set_hint_notify(qtk_asr_t *asr, void *ths,
                             qtk_iasr_set_hint_notify_f notify);
void qtk_asr_set_spxfinal_notify(qtk_asr_t *asr, void *ths,
                             qtk_iasr_set_spxfinal_notify_f notify);
void qtk_asr_set_result_notify(qtk_asr_t *asr, void *ths,
                             qtk_iasr_set_result_f notify);
int qtk_asr_hw_upload(qtk_asr_t *asr, char *hw_fn, int flag);
int qtk_asr_hw_update(qtk_asr_t *asr, char *hw_fn, int flag);
int qtk_asr_hw_get_hotword(qtk_asr_t *asr);
void qtk_asr_set_hw_notify(qtk_asr_t *asr, void *ths,
                           qtk_iasr_set_hw_notify_f notify_f);
void qtk_asr_set_res(qtk_asr_t *asr, char *data, int len);
void qtk_asr_set_coreType(qtk_asr_t *asr, char *data, int len);
void qtk_asr_set_skip_space(qtk_asr_t *asr, int skip_space);
void qtk_asr_set_xbnf(qtk_asr_t *asr, char *xbnf, int len);
void qtk_asr_set_idle_time(qtk_asr_t *asr, int itime);

int qtk_asr_update_cmds(qtk_asr_t* asr,char* words,int len);
#ifdef __cplusplus
};
#endif
#endif
