#ifndef WTK_BFIO_QFORM_WTK_QFORM9_CFG
#define WTK_BFIO_QFORM_WTK_QFORM9_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/aspec/wtk_aspec.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/core/fft/wtk_stft2.h"
#include "wtk_qmmse.h"
#include "wtk_covm.h"
#include "wtk_qenvelope.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_qform9_cfg wtk_qform9_cfg_t;
struct wtk_qform9_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    wtk_stft2_cfg_t stft2;
    int rate;

    wtk_aspec_cfg_t aspec;

    float *mic_class[2];
    wtk_aspec_cfg_t aspec_class1;
    wtk_aspec_cfg_t aspec_class2;

    wtk_covm_cfg_t covm;

    wtk_bf_cfg_t bf;

    int theta_range;

    int ntheta_range;
    float *ntheta_center;
    int ntheta_num;

    int theta_range_class;
    int theta_range_class1;
    int theta_range_class2;
    float theta_center_class1;
    float theta_center_class2;
    float theta_center_class3;

    int lf;
    int lt;

    float specsum_fs;
    float specsum_fe;
    int specsum_ns;
    int specsum_ne;

    float cohv_thresh;
    float specsum_bl;

    wtk_qenvelope_cfg_t qenvl;
    wtk_qenvelope_cfg_t qenvl2;
    wtk_qenvelope_cfg_t qenvl3;

    wtk_qmmse_cfg_t qmmse;

    float **t_r_qenvl;
    int t_r_number;
    float qenvel_alpha;
    int noise_debug_cnt;

    float sym;
    int cnon_clip_s;
    int cnon_clip_e;

    int delay_nf;

    float ncohv_alpha;
    float scohv_alpha;
    float nscohv_scale;
    float nscohv_scale2;
    float nscohv_scale3;
    float entropy_thresh;
    int entropy_in_cnt;
    int entropy_cnt;
    int cohv_init_frame;

    int sil_in_cnt;
    int sil_out_cnt;
    int sil_max_range;
    float sil_noise_suppress;

    float max_out;
    int out_agc_level;

    unsigned use_line:1;
    unsigned use_post:1;
    unsigned use_noiseblock:1;
    unsigned use_noiseblock2:1;
    unsigned use_two_aspecclass:1;
    unsigned use_cline1:1;
    unsigned use_cline2:1;
    unsigned use_cline3:1;
    unsigned use_twojoin:1;

    unsigned use_qenvelope:1;
    unsigned use_nqenvelope:1;
    unsigned use_sqenvelope:1;
    unsigned use_t_r_qenvelope:1;
    unsigned use_simple_qenvelope:1;
    unsigned use_noise_qenvelope:1;
    unsigned use_specsum_bl:1;
    unsigned use_preemph:1;
    unsigned use_two_channel:1;
    unsigned use_howl_suppression:1;
    unsigned debug:1;
    unsigned use_noise_debug:1;
    unsigned use_sound_debug:1;
    unsigned use_cnon:1;
    unsigned use_cohv_cnt:1;
    unsigned use_qmmse_param:1;
    unsigned use_bs:1;
    unsigned use_bs_win:1;
};

int wtk_qform9_cfg_init(wtk_qform9_cfg_t *cfg);
int wtk_qform9_cfg_clean(wtk_qform9_cfg_t *cfg);
int wtk_qform9_cfg_update_local(wtk_qform9_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_qform9_cfg_update(wtk_qform9_cfg_t *cfg);
int wtk_qform9_cfg_update2(wtk_qform9_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_qform9_cfg_t* wtk_qform9_cfg_new(char *cfg_fn);
void wtk_qform9_cfg_delete(wtk_qform9_cfg_t *cfg);
wtk_qform9_cfg_t* wtk_qform9_cfg_new_bin(char *bin_fn);
void wtk_qform9_cfg_delete_bin(wtk_qform9_cfg_t *cfg);

void wtk_qform9_cfg_set_theta_range(wtk_qform9_cfg_t *cfg,float theta_range);
void wtk_qform9_cfg_set_noise_suppress(wtk_qform9_cfg_t *cfg,float noise_suppress);
#ifdef __cplusplus
};
#endif
#endif
