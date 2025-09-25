#ifndef QTK_ASR_WRAPPER_CFG_H_
#define QTK_ASR_WRAPPER_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_version_cfg.h"
#include "wtk/core/strlike/wtk_chnlike_cfg.h"
#include "wtk/asr/vad/wtk_vad.h"
#include "wtk/lex/wtk_lex.h"
#include "wtk/asr/wfst/k2dec/qtk_k2_dec.h"
#include "wtk/asr/wfst/wenetdec/qtk_wenet_wrapper.h"
#include "wtk/asr/wfst/kaldifst/qtk_decoder_wrapper.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_asr_wrapper_cfg qtk_asr_wrapper_cfg_t;

typedef enum
{
	QTK_K2=0,
	QTK_KALDI,
	QTK_WENET
}qtk_asr_decoder_type;

struct qtk_asr_wrapper_cfg
{
	wtk_lex_cfg_t lex;
	wtk_vad_cfg_t vad;
	wtk_chnlike_cfg_t chnlike;	
	wtk_version_cfg_t version;
	wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
	qtk_k2_wrapper_cfg_t k2;
	qtk_wenet_wrapper_cfg_t wenet;
	qtk_decoder_wrapper_cfg_t kaldi;
	qtk_k2_wrapper_cfg_t kwake;//k2 wakeup

	char *lex_fn;
	char *asr_type;
	char *asr_fn;
	char *wakeup_fn;

	qtk_asr_decoder_type type;
	wtk_string_t res;
	wtk_strbuf_t *asr_buf;
	wtk_strbuf_t *wakeup_buf;
	void *hook;

	wtk_array_t *contact_e;
	wtk_array_t *contact_b;
	wtk_array_t *place_e;
	wtk_array_t *place_b;

	unsigned int use_vad:1;
	unsigned int use_wake:1;
	unsigned int use_lex:1;
	unsigned int use_hc_asr:1;
};

int qtk_asr_wrapper_cfg_init(qtk_asr_wrapper_cfg_t *cfg);
int qtk_asr_wrapper_cfg_clean(qtk_asr_wrapper_cfg_t *cfg);
int qtk_asr_wrapper_cfg_update_local(qtk_asr_wrapper_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_asr_wrapper_cfg_update(qtk_asr_wrapper_cfg_t *cfg);
int qtk_asr_wrapper_cfg_update2(qtk_asr_wrapper_cfg_t *cfg,wtk_source_loader_t *sl);
int qtk_asr_wrapper_cfg_update3(qtk_asr_wrapper_cfg_t *cfg,wtk_source_loader_t *sl,wtk_rbin2_t *rb);
int qtk_asr_wrapper_cfg_bytes(qtk_asr_wrapper_cfg_t *cfg);

wtk_main_cfg_t* qtk_asr_wrapper_cfg_new(char *cfg_fn);
void qtk_asr_wrapper_cfg_delete(wtk_main_cfg_t *main_cfg);
qtk_asr_wrapper_cfg_t* qtk_asr_wrapper_cfg_new_bin(char *bin_fn);
qtk_asr_wrapper_cfg_t* qtk_asr_wrapper_cfg_new_bin2(char *bin_fn,unsigned int seek_pos);
void qtk_asr_wrapper_cfg_delete_bin(qtk_asr_wrapper_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
