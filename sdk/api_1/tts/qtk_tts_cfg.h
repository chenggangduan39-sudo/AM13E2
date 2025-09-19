
#ifndef QTK_API_TTSC_QTK_TTS_CFG
#define QTK_API_TTSC_QTK_TTS_CFG

#include "cldtts/qtk_cldtts_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/tts/wtk_tts_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_tts_cfg qtk_tts_cfg_t;
struct qtk_tts_cfg {
    qtk_cldtts_cfg_t cldtts;
    wtk_tts_cfg_t *tts;
    char *tts_fn;
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;
    unsigned use_cldtts : 1;
    unsigned use_bin : 1;
};

int qtk_tts_cfg_init(qtk_tts_cfg_t *cfg);
int qtk_tts_cfg_clean(qtk_tts_cfg_t *cfg);
int qtk_tts_cfg_update_local(qtk_tts_cfg_t *cfg, wtk_local_cfg_t *main);
int qtk_tts_cfg_update(qtk_tts_cfg_t *cfg);
int qtk_tts_cfg_update2(qtk_tts_cfg_t *cfg, wtk_source_loader_t *sl);

void qtk_tts_cfg_update_option(qtk_tts_cfg_t *cfg, qtk_option_t *opt);
void qtk_tts_cfg_update_params(qtk_tts_cfg_t *cfg, wtk_local_cfg_t *params);

qtk_tts_cfg_t *qtk_tts_cfg_new(char *cfg_fn);
void qtk_tts_cfg_delete(qtk_tts_cfg_t *cfg);

qtk_tts_cfg_t *qtk_tts_cfg_new_bin(char *bin_fn);
void qtk_tts_cfg_delete_bin(qtk_tts_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
