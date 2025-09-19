#ifndef QTK_K2_WRAPPER_CFG_H_
#define QTK_K2_WRAPPER_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/asr/wfst/kaldifst/qtk_kwfstdec_cfg.h"
#include "wtk/asr/fextra/wtk_fextra_cfg.h"
#include "wtk/core/cfg/wtk_version_cfg.h"
#include "wtk/asr/wfst/egram/xbnf/wtk_xbnf_rec.h"
#include "wtk/asr/wfst/egram/xbnf/wtk_xbnfnet.h"
#include "wtk/core/strlike/wtk_chnlike_cfg.h"
#include "wtk/asr/vad/wtk_vad.h"
#include "wtk/asr/fextra/kparm/wtk_kxparm.h"
#include "wtk/lex/wtk_lex.h"
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#include "wtk/asr/wfst/net/wtk_fst_sym.h"
#include "wtk/asr/wfst/net/wtk_fst_net2.h"
#include "wtk/core/wtk_label.h"
#include "wtk/asr/punctuation/qtk_punctuation_prediction.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_k2_wrapper_cfg qtk_k2_wrapper_cfg_t;
typedef enum {
	QTK_K2_BEAM_SEARCH = 0,
	QTK_K2_WFST_SEARCH,
	QTK_K2_KEYWORD_SEARCH,
	QTK_K2_KWS_ASR
}qtk_k2_wrapper_search_method;


struct qtk_k2_wrapper_cfg
{
#ifdef ONNX_DEC
	qtk_onnxruntime_cfg_t encoder;
	qtk_onnxruntime_cfg_t decoder;
	qtk_onnxruntime_cfg_t joiner;
#endif
	wtk_label_t *label;
	wtk_label_t *label2;
	qtk_kwfstdec_cfg_t kwfstdec;
	wtk_lex_cfg_t lex;
	wtk_vad_cfg_t vad;
	wtk_kxparm_cfg_t parm;
	wtk_chnlike_cfg_t chnlike;
	wtk_xbnf_rec_cfg_t xbnf;
	qtk_punctuation_prediction_cfg_t pp;
	wtk_version_cfg_t version;
	wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
	wtk_rbin2_t *rbin2;
	wtk_fst_sym_t *sym;
	wtk_fst_sym_t *sym2;//for keyword detect
	qtk_k2_wrapper_search_method method;
	wtk_fst_net_cfg_t net;
	wtk_dict_t *dict;
	wtk_xbnfnet_cfg_t xbnfnet;
	char *dict_fn;
	char *lex_fn;
	char *prior_fn;
	char *sym_fn;
	char *sym2_fn;
	wtk_string_t res;
	float blank_penalty;
	float hot_gain;

	//thresh for keyword detect
	float conf;
	float wrd_speed;
	float aver_amprob;
	float interval;

	int phn_hash_hint;
	int wrd_hash_hint;
	int search_method;
	int beam;
	int chunk;
	int subsample;
	int right_context;
	int left_context;
	int hint_len;

	int idle_hint;
	float idle_conf;
	float norm_conf;
	int last_outid;

	unsigned int use_trick:1;
	unsigned int use_last_state:1;
	unsigned int use_vad:1;
	unsigned int use_lex:1;
	unsigned int use_lite:1;
	unsigned int use_xbnf:1;
	unsigned int use_context:1;//context search in normal asr
	unsigned int keyword_detect:1;//keyword detect phone model
	unsigned int use_stream:1;
	unsigned int filter_result:1;
	unsigned int use_eps_feat:1;
	unsigned int use_pp:1;
	unsigned int need_reset:1;
	unsigned int use_hc_wakeup:1;//TODO for hc wakeup
	unsigned int use_hc_asr:1;//TODO for hc asr
	unsigned int use_ebnf:1;
};

int qtk_k2_wrapper_cfg_init(qtk_k2_wrapper_cfg_t *cfg);
int qtk_k2_wrapper_cfg_clean(qtk_k2_wrapper_cfg_t *cfg);
int qtk_k2_wrapper_cfg_update_local(qtk_k2_wrapper_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_k2_wrapper_cfg_update(qtk_k2_wrapper_cfg_t *cfg);
int qtk_k2_wrapper_cfg_update2(qtk_k2_wrapper_cfg_t *cfg,wtk_source_loader_t *sl);
int qtk_k2_wrapper_cfg_update3(qtk_k2_wrapper_cfg_t *cfg,wtk_source_loader_t *sl,wtk_rbin2_t *rb);
int qtk_k2_wrapper_cfg_bytes(qtk_k2_wrapper_cfg_t *cfg);

wtk_main_cfg_t* qtk_k2_wrapper_cfg_new(char *cfg_fn);
void qtk_k2_wrapper_cfg_delete(wtk_main_cfg_t *main_cfg);
qtk_k2_wrapper_cfg_t* qtk_k2_wrapper_cfg_new_bin(char *bin_fn);
qtk_k2_wrapper_cfg_t* qtk_k2_wrapper_cfg_new_bin2(char *bin_fn,unsigned int seek_pos);
void qtk_k2_wrapper_cfg_delete_bin(qtk_k2_wrapper_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
