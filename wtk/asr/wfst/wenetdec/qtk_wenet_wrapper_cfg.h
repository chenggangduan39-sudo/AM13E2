#ifndef QTK_WENET_WRAPPER_CFG_H_
#define QTK_WENET_WRAPPER_CFG_H_
#include "wtk/asr/fextra/kparm/wtk_kxparm.h"
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#include "wtk/asr/fextra/wtk_fextra_cfg.h"
#include "wtk/asr/vad/wtk_vad.h"
#include "wtk/asr/wfst/egram/wtk_egram.h"
#include "wtk/asr/wfst/egram/xbnf/wtk_xbnf_rec.h"
#include "wtk/asr/wfst/kaldifst/qtk_kwfstdec_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_version_cfg.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/strlike/wtk_chnlike_cfg.h"
#include "wtk/lex/wtk_lex.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_wenet_wrapper_cfg qtk_wenet_wrapper_cfg_t;


struct qtk_wenet_wrapper_cfg
{
#ifdef ONNX_DEC
	qtk_onnxruntime_cfg_t onnx;
#endif
	qtk_kwfstdec_cfg_t kwfstdec;
        wtk_egram_cfg_t egram;
        wtk_lex_cfg_t lex;
        wtk_vad_cfg_t vad;
	wtk_kxparm_cfg_t parm;
	wtk_chnlike_cfg_t chnlike;	
	wtk_xbnf_rec_cfg_t xbnf;
	wtk_version_cfg_t version;
	wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
	char *lex_fn;
	char *prior_fn;
	wtk_string_t res;
    unsigned int use_vad:1;
	unsigned int use_lex:1;
	unsigned int use_lite:1;
	unsigned int use_xbnf:1;
        unsigned int speedup : 1;
        unsigned int use_context : 1; // context biasing
                                      // wtk_vector_t **vec;
};

int qtk_wenet_wrapper_cfg_init(qtk_wenet_wrapper_cfg_t *cfg);
int qtk_wenet_wrapper_cfg_clean(qtk_wenet_wrapper_cfg_t *cfg);
int qtk_wenet_wrapper_cfg_update_local(qtk_wenet_wrapper_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_wenet_wrapper_cfg_update(qtk_wenet_wrapper_cfg_t *cfg);
int qtk_wenet_wrapper_cfg_update2(qtk_wenet_wrapper_cfg_t *cfg,wtk_source_loader_t *sl);
int qtk_wenet_wrapper_cfg_update3(qtk_wenet_wrapper_cfg_t *cfg,wtk_source_loader_t *sl,wtk_rbin2_t *rb);
int qtk_wenet_wrapper_cfg_bytes(qtk_wenet_wrapper_cfg_t *cfg);

wtk_main_cfg_t* qtk_wenet_wrapper_cfg_new(char *cfg_fn);
void qtk_wenet_wrapper_cfg_delete(wtk_main_cfg_t *main_cfg);
qtk_wenet_wrapper_cfg_t* qtk_wenet_wrapper_cfg_new_bin(char *bin_fn);
qtk_wenet_wrapper_cfg_t* qtk_wenet_wrapper_cfg_new_bin2(char *bin_fn,unsigned int seek_pos);
void qtk_wenet_wrapper_cfg_delete_bin(qtk_wenet_wrapper_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
