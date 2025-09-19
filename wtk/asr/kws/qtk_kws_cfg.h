#ifndef QTK_KWS_CFG_H_
#define QTK_KWS_CFG_H_
#include "wtk/asr/fextra/kparm/wtk_kxparm.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/wakeup/wtk_kwake.h"
#include "wtk/asr/img/qtk_img.h"
#include "wtk_svprint.h"
#include "wtk/asr/vad/kvad/wtk_kvad.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_kws_cfg qtk_kws_cfg_t;

struct qtk_kws_cfg
{
    void *hook;
    wtk_cfg_file_t *cfile;
    wtk_rbin2_t *rbin;
    wtk_kxparm_cfg_t kxparm;
    wtk_kwake_cfg_t kwake;
    wtk_kvad_cfg_t vad;
    wtk_vad_cfg_t vad2;
    wtk_svprint_cfg_t svprint;
    qtk_img_cfg_t img;
    char *wake_fn;
    char *e2e_fn;
    char *svprint_fn;
    wtk_source_loader_t sl;
    float section_secs;
    unsigned int use_wake1:1;
    unsigned int use_wake2:1;
    unsigned int use_svprint:1;
    unsigned int use_vad:1;
    unsigned int use_vad2:1;
    unsigned int use_img:1;
    unsigned int use_window_pad:1;

    unsigned int use_sliding_window:1;
};

int qtk_kws_cfg_init(qtk_kws_cfg_t *cfg);
int qtk_kws_cfg_clean(qtk_kws_cfg_t *cfg);
int qtk_kws_cfg_update_local(qtk_kws_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_kws_cfg_update(qtk_kws_cfg_t *cfg);

/**
 * @brief used for bin loader;
 */
int qtk_kws_cfg_update2(qtk_kws_cfg_t *cfg,wtk_source_loader_t *sl);

qtk_kws_cfg_t* qtk_kws_cfg_new_bin(char *bin_fn,char *cfg_fn);
qtk_kws_cfg_t* qtk_kws_cfg_new_bin2(char *bin_fn);
qtk_kws_cfg_t* qtk_kws_cfg_new_bin3(char *bin_fn,unsigned int seek_pos);
int qtk_kws_cfg_delete_bin(qtk_kws_cfg_t *cfg);
int qtk_kws_cfg_delete_bin2(qtk_kws_cfg_t *cfg);
qtk_kws_cfg_t* qtk_kws_cfg_new(char *cfg_fn);
void qtk_kws_cfg_delete(qtk_kws_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
