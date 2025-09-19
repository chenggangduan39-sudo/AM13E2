#ifndef WTK_BFIO3_WTK_BFIO3_CFG
#define WTK_BFIO3_WTK_BFIO3_CFG
#include "wtk/core/cfg/wtk_main_cfg.h" 
#include "wtk/asr/wfst/kaldifst/qtk_decoder_wrapper_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h" 
#include "wtk/core/fft/wtk_stft2.h"
#include "wtk/bfio/wtk_kvadwake.h" 
#include "wtk/bfio/qform/wtk_qform9.h" 
#include "wtk/bfio/qform/wtk_qform2.h" 
#include "wtk/bfio/ssl/wtk_ssl.h" 
#include "wtk/asr/vad/wtk_vad2.h" 
#include "wtk/bfio/aec/wtk_aec.h" 
#include "wtk/bfio/wtk_wbf.h" 
#include "wtk/bfio/wtk_wbf2.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_bfio3_cfg wtk_bfio3_cfg_t;
struct wtk_bfio3_cfg
{
    wtk_stft2_cfg_t stft2;
    wtk_stft2_cfg_t sp_stft2;

    wtk_aec_cfg_t aec;

    wtk_wbf_cfg_t wbf;
    wtk_wbf2_cfg_t wbf2;

    wtk_kvadwake_cfg_t vwake;

    wtk_ssl_cfg_t ssl;
    float wake_ssl_fs;
    float wake_ssl_fe;

    wtk_qform9_cfg_t qform9;
    wtk_qform2_cfg_t qform2;
    qtk_decoder_wrapper_cfg_t decoder;

    char *vad_fn;
    wtk_vad_cfg_t *vad;

    int rate;

    int stft2_hist;

    int vad_left_margin;
    int vad_right_margin;

    void *hook;

    float idle_conf;
    float norm_conf;

    unsigned use_preemph:1;
    unsigned use_rbin_res:1;
    unsigned use_vad_start:1;
    unsigned use_wbf2:1;
    unsigned use_qform2:1;
    unsigned use_asr:1;
    unsigned use_offline_asr:1;
    unsigned use_aec:1;
    unsigned debug : 1;
    unsigned vad_fn_use_bin : 1;
    unsigned use_raw_audio : 1;
    unsigned use_trick : 1;
    unsigned use_thread:1;
};

int wtk_bfio3_cfg_init(wtk_bfio3_cfg_t *cfg);
int wtk_bfio3_cfg_clean(wtk_bfio3_cfg_t *cfg);
int wtk_bfio3_cfg_update_local(wtk_bfio3_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_bfio3_cfg_update(wtk_bfio3_cfg_t *cfg);
int wtk_bfio3_cfg_update2(wtk_bfio3_cfg_t *cfg, wtk_source_loader_t *sl);

void wtk_bfio3_cfg_set_wakeword(wtk_bfio3_cfg_t *bfio3, char *wrd);

wtk_bfio3_cfg_t* wtk_bfio3_cfg_new(char *cfg_fn);
void wtk_bfio3_cfg_delete(wtk_bfio3_cfg_t *cfg);
wtk_bfio3_cfg_t* wtk_bfio3_cfg_new_bin(char *bin_fn);
void wtk_bfio3_cfg_delete_bin(wtk_bfio3_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
