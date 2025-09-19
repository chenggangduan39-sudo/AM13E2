#ifndef QTK_K2_DEC_H_
#define QTK_K2_DEC_H_

#include "qtk_k2_wrapper_cfg.h"
#include "qtk_beam_searcher.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/asr/wfst/kaldifst/qtk_kwfstdec.h"
#include "wtk/asr/wfst/wtk_wfstenv_cfg.h"
#include "wtk/core/json/wtk_json.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_k2_dec qtk_k2_dec_t;
typedef struct qtk_k2_dec_instance qtk_k2_dec_instance_t;
typedef struct qtk_k2_encoder_helper qtk_k2_encoder_helper_t;

struct qtk_k2_encoder_helper
{
#ifdef ONNX_DEC
    OrtValue **in;
#endif
    int64_t **in_shape;
    int *in_shape_len;
    int *in_dim;

    float *input1;
    int64_t *input2;
};

struct qtk_k2_dec_instance
{
    qtk_beam_searcher_t *searcher;
#ifdef ONNX_DEC
    qtk_onnxruntime_t *decoder;
    qtk_onnxruntime_t *joiner;
#endif
    int64_t decoder_shape[2];
    int64_t decoder_input[8];
    float *prob;
};


struct qtk_k2_dec
{
    qtk_k2_wrapper_cfg_t *cfg;
    wtk_queue_t parm_q;
    wtk_kxparm_t *kxparm;
    qtk_k2_dec_instance_t ins[2];
    qtk_beam_searcher_t *searcher;
    qtk_punctuation_prediction_t *pp;
    wtk_cfg_file_t *env_parser;
    wtk_wfstenv_cfg_t env;

    wtk_strbuf_t *res_buf;
    wtk_strbuf_t *json_buf;
    wtk_strbuf_t *feat;
    wtk_strbuf_t *hint_buf;
    wtk_strbuf_t *timestamp_buf;
    qtk_kwfstdec_t *dec;
    wtk_json_t *json;
    char *stream_val;
#ifdef ONNX_DEC
    qtk_onnxruntime_t *encoder;
    qtk_onnxruntime_t *decoder;
    qtk_onnxruntime_t *joiner;
#endif
    qtk_k2_encoder_helper_t *ehelp;

    int64_t decoder_shape[2];
    int64_t decoder_input[8];
    int64_t dj_shape[2];
    int64_t je_shape[2];

    int num_feats;
    int frame;
    int num_toks;
    int chunk_nums;

    int pad_len;
    int chunk_len;
    int ret_len;
    int tail_len;
    int index;
    int hint_len;
    int need_detect;
    int last_timestampe;

    float recommand_conf;
    float conf;
    float st;
    float ed;
    float *prob;
    int found;
    int idle;
    int64_t vad_index;
    int64_t valid_index;
};

qtk_k2_dec_t* qtk_k2_dec_new(qtk_k2_wrapper_cfg_t* cfg);
int qtk_k2_dec_start(qtk_k2_dec_t* wrapper);
int qtk_k2_dec_start2(qtk_k2_dec_t *wrapper,char *data,int bytes);
int qtk_k2_dec_feed(qtk_k2_dec_t* wrapper,char *data,int bytes,int is_end);
void qtk_k2_dec_reset(qtk_k2_dec_t* wrapper);
void qtk_k2_dec_reset2(qtk_k2_dec_t* wrapper);
void qtk_k2_dec_delete(qtk_k2_dec_t* wrapper);
float qtk_k2_dec_get_conf(qtk_k2_dec_t *wrapper);
void qtk_k2_dec_get_result(qtk_k2_dec_t *wrapper,wtk_string_t *v);
void qtk_k2_dec_get_hint_result(qtk_k2_dec_t *wrapper,wtk_string_t *v);
int qtk_k2_dec_set_hotwords(qtk_k2_dec_t *wrapper,char *data, int len);
int qtk_k2_dec_set_keywords(qtk_k2_dec_t *wrapper,char *data, int len);
int qtk_k2_dec_set_keywords_wakeup(qtk_k2_dec_t *wrapper,char *data, int len);
int qtk_k2_dec_set_keywords_asr(qtk_k2_dec_t *wrapper,char *data, int len);
float qtk_k2_dec_set_vadindex(qtk_k2_dec_t * wrapper, int index);
void qtk_k2_dec_get_wake_time(qtk_k2_dec_t *dec,float *fs,float *fe);
float qtk_k2_dec_get_conf(qtk_k2_dec_t *dec);
int qtk_k2_dec_set_contacts(qtk_k2_dec_t *dec, char *data, int len);
void qtk_k2_dec_enter_norm_state(qtk_k2_dec_t *dec);
void qtk_k2_dec_enter_idle_state(qtk_k2_dec_t * wrapper);
#ifdef ONNX_DEC
void* qtk_k2_dec_get_state_val(qtk_k2_dec_t *wrapper,long unsigned int *len);
int qtk_k2_dec_set_state_val(qtk_k2_dec_t *wrapper,void *val,long unsigned int len);
#endif
#ifdef __cplusplus
};
#endif
#endif

