#ifndef WTK_BFIO_WTK_BFIO5_CFG
#define WTK_BFIO_WTK_BFIO5_CFG
#include "wtk/asr/vad/wtk_vad2.h"
#include "wtk/asr/wfst/kaldifst/qtk_decoder_wrapper_cfg.h"
#include "wtk/bfio/aec/wtk_aec.h"
#include "wtk/bfio/maskdenoise/wtk_gainnet_denoise.h"
#include "wtk/bfio/qform/wtk_qform2.h"
#include "wtk/bfio/qform/wtk_qform3.h"
#include "wtk/bfio/qform/wtk_qform9.h"
#include "wtk/bfio/ssl/wtk_ssl.h"
#include "wtk/bfio/ssl/wtk_ssl2.h"
#include "wtk/bfio/ssl/wtk_maskssl2.h"
#include "wtk/bfio/masknet/wtk_gainnet2.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/afilter/wtk_rls.h"
#include "wtk/bfio/wtk_kvadwake.h"
#include "wtk/bfio/wtk_wbf.h"
#include "wtk/bfio/wtk_wbf2.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/fft/wtk_stft2.h"
#ifdef __cplusplus
extern "C" {`
#endif

typedef struct wtk_bfio5_cfg wtk_bfio5_cfg_t;
struct wtk_bfio5_cfg
{
	int in_channel;
	int *mic_channel;
	int nmicchannel;
	int *mic_channel2;
	int nmicchannel2;
	int *sp_channel;
	int nspchannel;

    wtk_stft2_cfg_t stft2;
    wtk_stft2_cfg_t stft2_2;
    wtk_stft2_cfg_t sp_stft2;

    wtk_aec_cfg_t aec;

    wtk_wbf_cfg_t wbf;
    wtk_wbf2_cfg_t wbf2;
    wtk_gainnet_denoise_cfg_t gdenoise;

    wtk_kvadwake_cfg_t vwake;
    wtk_kvadwake_cfg_t vwake2;
    wtk_kvadwake_cfg_t vwake3;

    float aec_wake_fs;
    float aec_wake_fe;

    wtk_ssl_cfg_t ssl;
    wtk_ssl2_cfg_t ssl2;
    float wake_ssl_fs;
    float wake_ssl_fe;

    wtk_qform9_cfg_t qform9;
    wtk_qform2_cfg_t qform2;
    wtk_qform3_cfg_t qform3;
    qtk_decoder_wrapper_cfg_t decoder;

    wtk_vad_cfg_t vad;

    wtk_bankfeat_cfg_t bankfeat;
    int featm_lm;
    int featsp_lm;

    char *aecmdl_fn;
    wtk_gainnet2_cfg_t *gainnet2;
    wtk_maskssl2_cfg_t maskssl2;
    wtk_maskssl2_cfg_t maskssl2_2;
    wtk_qmmse_cfg_t qmmse;
	wtk_rls_cfg_t echo_rls;

    float spenr_thresh;
    int spenr_cnt;

    int wins;
    int rate;

    int stft2_hist;

    int vad_left_margin;
    int vad_right_margin;

    void *hook;

    float sil_delay;
    float speech_delay;

    int de_wake_len;
    float energy_conf;
    int aec_wake_len;

    int ressl_range;

    float low_fs;
    float low_fe;
    int low_fs_idx;
    int low_fe_idx;
    float low_thresh;

    float *mic_scale;

    float dup_time;
    float gd_dup_time;
    float wake_scale;

    unsigned use_preemph:1;
    unsigned use_vad_start:1;
    unsigned use_wbf:1;
    unsigned use_wbf2:1;
    unsigned use_qform2:1;
    unsigned use_qform3:1;
    unsigned use_qform9:1;
    unsigned use_asr:1;
    unsigned use_offline_asr:1;
    unsigned use_aec:1;
    unsigned debug : 1;
    unsigned use_raw_audio : 1;
    unsigned use_all_raw_audio : 1;
    unsigned use_mic2_raw_audio : 1;
    unsigned use_gdenoise : 1;
    unsigned use_ssl2 : 1;
    unsigned use_one_shot : 1;
    unsigned use_line : 1;
    unsigned use_kvadwake : 1;
    unsigned use_kvadwake2 : 1;
    unsigned use_vad : 1;
    unsigned use_en_trick : 1;
    unsigned use_aec_wake : 1;
    unsigned use_low_trick : 1;
    unsigned use_rbin_res:1;
    unsigned use_maskssl2:1;
    unsigned use_maskssl2_2:1;
    unsigned use_qmmse:1;
    unsigned use_rls:1;
    unsigned use_mic_scale;
    unsigned use_stft2_2:1;
    unsigned use_nmicchannel:1;
};

int wtk_bfio5_cfg_init(wtk_bfio5_cfg_t *cfg);
int wtk_bfio5_cfg_clean(wtk_bfio5_cfg_t *cfg);
int wtk_bfio5_cfg_update_local(wtk_bfio5_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_bfio5_cfg_update(wtk_bfio5_cfg_t *cfg);
int wtk_bfio5_cfg_update2(wtk_bfio5_cfg_t *cfg, wtk_source_loader_t *sl);

void wtk_bfio5_cfg_set_wakeword(wtk_bfio5_cfg_t *bfio5, char *wrd);

wtk_bfio5_cfg_t* wtk_bfio5_cfg_new(char *cfg_fn);
void wtk_bfio5_cfg_delete(wtk_bfio5_cfg_t *cfg);
wtk_bfio5_cfg_t* wtk_bfio5_cfg_new_bin(char *bin_fn);
void wtk_bfio5_cfg_delete_bin(wtk_bfio5_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
