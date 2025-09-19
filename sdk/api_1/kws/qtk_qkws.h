
#ifndef __QTK_API_QTK_QKWS__
#define __QTK_API_QTK_QKWS__

#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/asr/kws/qtk_kws.h"
#include "wtk/asr/kws/qtk_sond_cluster.h"
#include "qtk_qkws_cfg.h"
#include "sdk/qtk_api.h"
#include "wtk/asr/vad/wtk_vad.h"
#include "sdk/api_1/vboxebf/qtk_vboxebf.h"
#include "wtk/bfio/maskdenoise/cmask_pse/wtk_cmask_pse.h"
#include "sdk/api_1/aec/qtk_aec.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_qkws qtk_qkws_t;
struct qtk_qkws {
    qtk_qkws_cfg_t *cfg;
    qtk_kws_t *kws;
    wtk_cmask_pse_t *cmask_pse;
    wtk_vad_t *vad;
    qtk_sond_cluster_t *sc;
    qtk_vboxebf_t *vboxebf;
    qtk_aec_t *aec;
    qtk_session_t *session;
    wtk_strbuf_t *rlt_buf;
    wtk_strbuf_t *input_buf;
    wtk_strbuf_t *result_buf;
    wtk_strbuf_t *score_buf;
    wtk_strbuf_t *outbuf;
    qtk_engine_notify_f notify;
    void *this;
    FILE *efp;
    char *enroll_path;
    char *zdata;
    
    unsigned is_start:1;
    unsigned sil : 1;
    unsigned valid : 1;
    unsigned cancel : 1;
    unsigned is_set_number:1;
    unsigned is_start_enroll:1;
};

qtk_qkws_t *qtk_qkws_new(qtk_qkws_cfg_t *cfg, qtk_session_t *session);
void qtk_qkws_delete(qtk_qkws_t *qk);

int qtk_qkws_start(qtk_qkws_t *qk);
int qtk_qkws_feed(qtk_qkws_t *qk, char *data, int bytes, int is_end);
int qtk_qkws_reset(qtk_qkws_t *qk);
void qtk_qkws_cancel(qtk_qkws_t *qk);
void qtk_qkws_set_notify(qtk_qkws_t *qk, void *ths, qtk_engine_notify_f notify);

int qtk_qkws_set_enroll(qtk_qkws_t *qk, char *name, int len, int is_end);
void qtk_qkws_set_max_people(qtk_qkws_t *qk, int max_people);
int qtk_qkws_set_enroll_fn(qtk_qkws_t *qk, char *fn, int len);
void qtk_qkws_set_sv_thresh(qtk_qkws_t *qk, float thresh);
int qtk_qkws_set_vad_time(qtk_qkws_t *qk, float vs, float ve);
int qtk_qkws_set_spk_nums(qtk_qkws_t *qk, int num);
void qtk_qkws_set_result_dur(qtk_qkws_t *qk, int val);
void qtk_qkws_set_feat(qtk_qkws_t *qk, char *fn);

void qtk_qkws_clean(qtk_qkws_t *qk);
void qtk_qkws_reload(qtk_qkws_t *qk);
char* qtk_qkws_get_fn(qtk_qkws_t *qk);
float qtk_qkws_get_prob(qtk_qkws_t *qk);
void qtk_qkws_get_result(qtk_qkws_t *qk, qtk_var_t *var);

#ifdef __cplusplus
};
#endif
#endif
