#ifndef QTK_API_1_ASR_QTK_IASR_CFG
#define QTK_API_1_ASR_QTK_IASR_CFG

#include "wtk/asr/wfst/kaldifst/qtk_decoder_wrapper_cfg.h"
#include "wtk/asr/wfst/qtk_asr_wrapper_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"

#include "ebnf/qtk_cldebnf_cfg.h"
#include "hw/qtk_hw_cfg.h"
#include "sdk/spx/qtk_spx_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_iasr_cfg qtk_iasr_cfg_t;

typedef enum {
    QTK_IASR_SPX = 0, // spx asr
    QTK_IASR_LC = 1,  // local common asr
    QTK_IASR_GR = 2,  // local grammar asr
	QTK_IASR_GR_NEW = 3, // local grammer asr new.
} qtk_iasr_type_t;

struct qtk_iasr_cfg {
    qtk_cldebnf_cfg_t cldebnf;
    qtk_hw_cfg_t hw;
    qtk_iasr_type_t type;
    wtk_string_t name;
    wtk_string_t dec_fn;
    wtk_string_t compile_fn;
    wtk_string_t usr_ebnf;
    wtk_string_t usr_xbnf;
    wtk_string_t keyword_fn;
    union {
        wtk_main_cfg_t *mcfg;
        qtk_spx_cfg_t *scfg;
        qtk_decoder_wrapper_cfg_t *dwcfg;
        qtk_asr_wrapper_cfg_t *awcfg;
    } cfg;
    wtk_main_cfg_t *eg_mcfg;
    wtk_mbin_cfg_t *eg_mbcfg;
    wtk_rbin2_t *rbin;
    int src_rate;
    int dst_rate;
    int resample_cache;
    int priority;
    int max_hint;
    int use_mode;
    int idle_time;
    unsigned use_bin : 1;
    unsigned use_resample : 1;
    unsigned use_cldebnf : 1;
    unsigned skip_comm : 1;
    unsigned use_timestamp:1;
    unsigned use_hint : 1;
    unsigned use_lex:1;
	unsigned use_punc:1;
    unsigned use_vad : 1;
    unsigned wait : 1;
    unsigned use_hw_upload : 1;
    unsigned use_hotword : 1;
    unsigned use_xbnf : 1;
    unsigned save_spx_wav : 1;
    unsigned use_general_asr : 1;
};

int qtk_iasr_cfg_init(qtk_iasr_cfg_t *cfg);
int qtk_iasr_cfg_clean(qtk_iasr_cfg_t *cfg);
int qtk_iasr_cfg_update_local(qtk_iasr_cfg_t *cfg, wtk_local_cfg_t *main);
int qtk_iasr_cfg_update(qtk_iasr_cfg_t *cfg);
int qtk_iasr_cfg_update2(qtk_iasr_cfg_t *cfg, wtk_source_loader_t *sl);

void qtk_iasr_cfg_update_params(qtk_iasr_cfg_t *cfg, wtk_local_cfg_t *params);
void qtk_iasr_cfg_update_option(qtk_iasr_cfg_t *cfg, qtk_option_t *opt);
#ifdef __cplusplus
};
#endif
#endif
