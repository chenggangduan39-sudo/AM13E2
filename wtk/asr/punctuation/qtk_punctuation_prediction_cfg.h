#ifndef QTK_PUNCTUATION_PREDICTION_CFG_H_
#define QTK_PUNCTUATION_PREDICTION_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#include "wtk/asr/wfst/net/wtk_fst_sym.h"
#include "wtk/core/wtk_label.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_punctuation_prediction_cfg qtk_punctuation_prediction_cfg_t;

struct qtk_punctuation_prediction_cfg{
#ifdef ONNX_DEC
	qtk_onnxruntime_cfg_t model;
#endif
    wtk_label_t *label;
	wtk_rbin2_t *rbin;
    wtk_fst_sym_t *sym;
	wtk_cfg_file_t *cfile;
	char *sym_fn;
};

int qtk_punctuation_prediction_cfg_init(qtk_punctuation_prediction_cfg_t *cfg);
int qtk_punctuation_prediction_cfg_clean(qtk_punctuation_prediction_cfg_t *cfg);
int qtk_punctuation_prediction_cfg_update_local(qtk_punctuation_prediction_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_punctuation_prediction_cfg_update(qtk_punctuation_prediction_cfg_t *cfg);
int qtk_punctuation_prediction_cfg_update2(qtk_punctuation_prediction_cfg_t *cfg,wtk_source_loader_t *sl);
int qtk_punctuation_prediction_cfg_update3(qtk_punctuation_prediction_cfg_t *cfg,wtk_source_loader_t *sl,wtk_rbin2_t *rb);
int qtk_punctuation_prediction_cfg_bytes(qtk_punctuation_prediction_cfg_t *cfg);

wtk_main_cfg_t* qtk_punctuation_prediction_cfg_new(char *cfg_fn);
void qtk_punctuation_prediction_cfg_delete(wtk_main_cfg_t *main_cfg);
qtk_punctuation_prediction_cfg_t* qtk_punctuation_prediction_cfg_new_bin(char *bin_fn);
qtk_punctuation_prediction_cfg_t* qtk_punctuation_prediction_cfg_new_bin2(char *bin_fn,unsigned int seek_pos);
void qtk_punctuation_prediction_cfg_delete_bin(qtk_punctuation_prediction_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
