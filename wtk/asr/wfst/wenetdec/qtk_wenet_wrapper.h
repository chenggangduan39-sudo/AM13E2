#ifndef QTK_WENET_WRAPPER_H_
#define QTK_WENET_WRAPPER_H_

#include "wtk/asr/wfst/kaldifst/qtk_kwfstdec.h"
#include "wtk/asr/wfst/kaldifst/qtk_kwfstdec_lite.h"
#include "qtk_wenet_wrapper_cfg.h"
#include "wtk/core/json/wtk_json.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/asr/wfst/rec/wtk_wfstr.h"
#include "wtk/asr/wfst/wtk_wfstdec.h"
#include "wtk/asr/wfst/wtk_wfstenv_cfg.h"
#include "wtk/core/strlike/wtk_chnlike.h"
#include "wtk/asr/vad/wtk_vad.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_wenet_wrapper qtk_wenet_wrapper_t;

struct qtk_wenet_wrapper
{
    wtk_queue_t vad_q;
	wtk_chnlike_t *chnlike;
	wtk_xbnf_rec_t *xbnf;
	qtk_wenet_wrapper_cfg_t *cfg;
	wtk_cfg_file_t *env_parser;
	wtk_wfstenv_cfg_t env;
	wtk_queue_t parm_q;
	wtk_kxparm_t *kxparm;
	qtk_kwfstdec_t *dec;
	wtk_egram_t *egram;
	qtk_kwfstdec_lite_t *dec_lite;
	wtk_strbuf_t *res_buf;
	wtk_strbuf_t *json_buf;
	wtk_strbuf_t *hint_buf;
	wtk_strbuf_t *vad_buf;
	wtk_strbuf_t *feat;
	wtk_json_t *json;
    wtk_vad_t *vad;
	wtk_lex_t *lex;
    wtk_vframe_state_t last_vframe_state;
#ifdef ONNX_DEC
    qtk_onnxruntime_t *onnx;
#endif

	double time_stop;
	int wav_bytes;
	int num_feats;
	int rec_wav_bytes;
	int index;
	int64_t shape[3];
	int64_t shape2;
	unsigned int data_left:1;
	unsigned int run:1;
};

qtk_wenet_wrapper_t* qtk_wenet_wrapper_new(qtk_wenet_wrapper_cfg_t* cfg);
int qtk_wenet_wrapper_start(qtk_wenet_wrapper_t* wrapper);
int qtk_wenet_wrapper_start2(qtk_wenet_wrapper_t* wrapper,char *data,int bytes);
int qtk_wenet_wrapper_feed(qtk_wenet_wrapper_t* wrapper,char *data,int bytes,int is_end);
void qtk_wenet_wrapper_reset(qtk_wenet_wrapper_t* wrapper);
void qtk_wenet_wrapper_delete(qtk_wenet_wrapper_t* wrapper);
void qtk_wenet_wrapper_get_result(qtk_wenet_wrapper_t *wrapper,wtk_string_t *v);
void qtk_wenet_wrapper_get_hint_result(qtk_wenet_wrapper_t *wrapper,wtk_string_t *v);
void qtk_wenet_wrapper_get_vad_result(qtk_wenet_wrapper_t *wrapper,wtk_string_t *v);
void qtk_wenet_wrapper_set_xbnf(qtk_wenet_wrapper_t *wrapper,char *buf, int len);
int qtk_wenet_wrapper_set_context_fn(qtk_wenet_wrapper_t *wrapper, char *fn);
int qtk_wenet_wrapper_set_context_str(qtk_wenet_wrapper_t *wrapper, char *buf,
                                      int len);
#ifdef __cplusplus
}
;
#endif
#endif

