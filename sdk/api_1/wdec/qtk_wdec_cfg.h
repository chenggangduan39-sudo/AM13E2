#ifndef QTK_API_WDEC_QTK_TTS_CFG
#define QTK_API_WDEC_QTK_TTS_CFG

#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/tts/wtk_tts_cfg.h"
#include "wtk/asr/wdec/wtk_wdec_cfg.h"
#include "wtk/asr/wfst/kwdec2/wtk_kwdec2_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_wdec_cfg qtk_wdec_cfg_t;
struct qtk_wdec_cfg {
    wtk_wdec_cfg_t *wdec_cfg;
    wtk_kwdec2_cfg_t *kwdec2_cfg;
    wtk_vad_cfg_t *vad_cfg;
    wtk_string_t wdec_fn;
    wtk_string_t vad_fn;
    wtk_string_t words;
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;
	int left_margin;
	int right_margin;
    unsigned use_bin : 1;
    unsigned use_wdec : 1;
    unsigned use_kwdec2 : 1;
    unsigned use_vad : 1;
};

int qtk_wdec_cfg_init(qtk_wdec_cfg_t *cfg);
int qtk_wdec_cfg_clean(qtk_wdec_cfg_t *cfg);
int qtk_wdec_cfg_update_local(qtk_wdec_cfg_t *cfg, wtk_local_cfg_t *main);
int qtk_wdec_cfg_update(qtk_wdec_cfg_t *cfg);
int qtk_wdec_cfg_update2(qtk_wdec_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_wdec_cfg_t *qtk_wdec_cfg_new(char *cfg_fn);
void qtk_wdec_cfg_delete(qtk_wdec_cfg_t *cfg);

qtk_wdec_cfg_t *qtk_wdec_cfg_new_bin(char *bin_fn);
void qtk_wdec_cfg_delete_bin(qtk_wdec_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
