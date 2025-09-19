#ifndef QTK_API_1_ASR_QTK_IASR
#define QTK_API_1_ASR_QTK_IASR

#include "wtk/asr/wfst/kaldifst/qtk_decoder_wrapper.h"
#include "wtk/asr/wfst/qtk_asr_wrapper.h"
#include "wtk/bfio/resample/wtk_resample.h"

#include "ebnf/qtk_cldebnf.h"
#include "hw/qtk_hotword.h"
#include "qtk_iasr_cfg.h"
#include "sdk/spx/qtk_spx.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_iasr qtk_iasr_t;
typedef void (*qtk_iasr_set_hint_notify_f)(void *ths, char *data, int bytes);
typedef void (*qtk_iasr_set_spxfinal_notify_f)(void *ths, char *data, int bytes);
typedef void (*qtk_iasr_set_hw_notify_f)(void *ths, char *data, int bytes);
typedef void (*qtk_iasr_set_result_f)(void *ths, char *data, int bytes, float fs, float fe, int type);

struct qtk_iasr {
    int index;
    wtk_wavfile_t* spx_wav;
    qtk_iasr_cfg_t *cfg;
    qtk_session_t *session;
    union {
        qtk_spx_t *spx;
        qtk_decoder_wrapper_t *dw;
        qtk_asr_wrapper_t *aw;
    } ins;
    qtk_hw_t *hw;
    wtk_egram_t *egram;
    // wtk_fst_net_t *net;
    wtk_resample_t *resample;
    wtk_strbuf_t *buf;
    qtk_cldebnf_t *cnf;

    void *hint_ths;
    qtk_iasr_set_hint_notify_f hint_notify;
    void *spxfinal_ths;
    qtk_iasr_set_spxfinal_notify_f spxfinal_notify;
    void *hw_ths;
    qtk_iasr_set_hw_notify_f hw_notify;
    void *result_ths;
    qtk_iasr_set_result_f result_notify;

    char *usr_ebnf_data;
    int usr_ebnf_len;
    unsigned int end_flag : 1;
};

qtk_iasr_t *qtk_iasr_new(qtk_iasr_cfg_t *cfg, qtk_session_t *session);
void qtk_iasr_delete(qtk_iasr_t *iasr);

int qtk_iasr_start(qtk_iasr_t *iasr);
int qtk_iasr_feed(qtk_iasr_t *iasr, char *data, int bytes, int is_end);
void qtk_iasr_cancel(qtk_iasr_t *iasr);
void qtk_iasr_reset(qtk_iasr_t *iasr);
wtk_string_t qtk_iasr_get_result(qtk_iasr_t *iasr);
int qtk_iasr_set_ebnf(qtk_iasr_t *iasr, char *ebnf, int bytes);
int qtk_iasr_set_xbnf(qtk_iasr_t *iasr, char *xbnf, int bytes);
void qtk_iasr_set_res(qtk_iasr_t *iasr, char *data, int len);
void qtk_iasr_set_coreType(qtk_iasr_t *iasr, char *data, int len);
void qtk_iasr_set_skip_space(qtk_iasr_t *iasr, int skip_space);
void qtk_iasr_set_idle_time(qtk_iasr_t *iasr, int val);
void qtk_iasr_set_hint_notify(qtk_iasr_t *iasr, void *ths,
                              qtk_iasr_set_hint_notify_f notify);
void qtk_iasr_set_spxfinal_notify(qtk_iasr_t *iasr, void *ths,
                              qtk_iasr_set_spxfinal_notify_f notify);
void qtk_iasr_set_result_notify(qtk_iasr_t *iasr, void *ths, qtk_iasr_set_result_f notify);

// hotword 接口
int qtk_iasr_hw_upload(qtk_iasr_t *iasr, char *res_fn, int flag);
int qtk_iasr_hw_update(qtk_iasr_t *iasr, char *res_fn, int flag);
int qtk_iasr_get_hotword(qtk_iasr_t *iasr);
void qtk_iasr_set_hw_notify(qtk_iasr_t *iasr, void *ths,
                            qtk_iasr_set_hw_notify_f notify_f);

int qtk_iasr_update_cmds(qtk_iasr_t *iasr,char* words,int len);
#ifdef __cplusplus
};
#endif

#endif
