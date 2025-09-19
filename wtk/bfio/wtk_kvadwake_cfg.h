#ifndef WTK_BFIO_WTK_KVADWAKE_CFG
#define WTK_BFIO_WTK_KVADWAKE_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/wakeup/wtk_kwake.h"
#include "wtk/asr/wdec/wtk_wdec.h"
#include "wtk/asr/wfst/kwdec2/wtk_kwdec2.h"
#include "wtk/asr/img/qtk_img.h"
#include "wtk/asr/vad/wtk_vad2.h"
#include "wtk/asr/wfst/kaldifst/qtk_decoder_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_kvadwake_cfg wtk_kvadwake_cfg_t;
struct wtk_kvadwake_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	wtk_vad_cfg_t vad;
    wtk_kwake_cfg_t kwake;
    wtk_wdec_cfg_t wdec;
    wtk_strbuf_t *xwrd;
    wtk_kwdec2_cfg_t kwdec2;
    qtk_img_cfg_t img;
    qtk_img_thresh_cfg_t img_mod;
    qtk_img_thresh_cfg_t img_mod_echo;
    qtk_img_thresh_cfg_t img_no_mod;
    qtk_img_thresh_cfg_t img_no_mod_echo;

    qtk_decoder_wrapper_cfg_t decoder;

    int rate;

    float wake_right_fe;
    
    int vad_left_margin;
    int vad_right_margin;
    int max_vad_len;
    int max_vad_tms;

    unsigned use_vad:1;
    unsigned use_ivad:1;
    unsigned use_vad_start:1;
    unsigned use_wdec:1;
    unsigned use_img:1;
    unsigned use_kdec:1;
    unsigned use_kwdec2:1;
    unsigned use_kwake:1;
    unsigned debug : 1;
};

int wtk_kvadwake_cfg_init(wtk_kvadwake_cfg_t *cfg);
int wtk_kvadwake_cfg_clean(wtk_kvadwake_cfg_t *cfg);
int wtk_kvadwake_cfg_update_local(wtk_kvadwake_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_kvadwake_cfg_update(wtk_kvadwake_cfg_t *cfg);
int wtk_kvadwake_cfg_update2(wtk_kvadwake_cfg_t *cfg,wtk_source_loader_t *sl);

int wtk_kvadwake_cfg_set_wakeword(wtk_kvadwake_cfg_t *cfg,char *wrd);

wtk_kvadwake_cfg_t* wtk_kvadwake_cfg_new(char *fn);
void wtk_kvadwake_cfg_delete(wtk_kvadwake_cfg_t *cfg);
wtk_kvadwake_cfg_t* wtk_kvadwake_cfg_new_bin(char *fn);
void wtk_kvadwake_cfg_delete_bin(wtk_kvadwake_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
