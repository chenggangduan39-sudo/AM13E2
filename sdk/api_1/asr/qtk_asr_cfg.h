#ifndef QTK_API_1_ASR_QTK_ASR_CFG
#define QTK_API_1_ASR_QTK_ASR_CFG

#include "qtk_iasr_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_asr_cfg qtk_asr_cfg_t;
struct qtk_asr_cfg {
    qtk_iasr_cfg_t grammar;
    qtk_iasr_cfg_t *iasrs;
    int *iasrs_valid;
    wtk_string_t lex_fn;
    wtk_lex_cfg_t lex_cfg;
    wtk_main_cfg_t *main_cfg;
    wtk_rbin2_t *rbin;
    wtk_cfg_file_t *cfile;
    int skip_iasrs;
    int n_iasrs;
    float rec_min_conf;
    float gr_min_conf;
    unsigned use_grammar : 1;
    unsigned use_json : 1;
    //	unsigned skip_space:1;
    unsigned use_lex : 1;
};

int qtk_asr_cfg_init(qtk_asr_cfg_t *cfg);
int qtk_asr_cfg_clean(qtk_asr_cfg_t *cfg);
int qtk_asr_cfg_update_local(qtk_asr_cfg_t *cfg, wtk_local_cfg_t *main);
int qtk_asr_cfg_update(qtk_asr_cfg_t *cfg);
int qtk_asr_cfg_update2(qtk_asr_cfg_t *cfg, wtk_source_loader_t *sl);

void qtk_asr_cfg_update_params(qtk_asr_cfg_t *cfg, wtk_local_cfg_t *params);
void qtk_asr_cfg_update_option(qtk_asr_cfg_t *cfg, qtk_option_t *opt);

qtk_asr_cfg_t *qtk_asr_cfg_new(char *fn);
void qtk_asr_cfg_delete(qtk_asr_cfg_t *cfg);

qtk_asr_cfg_t *qtk_asr_cfg_new_bin(char *bin_fn);
void qtk_asr_cfg_delete_bin(qtk_asr_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
