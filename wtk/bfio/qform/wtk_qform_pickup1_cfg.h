#ifndef WTK_BFIO_QFORM_WTK_QFORM_PICKUP1_CFG
#define WTK_BFIO_QFORM_WTK_QFORM_PICKUP1_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/aspec/wtk_aspec.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/core/fft/wtk_stft2.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_qform_pickup1_cfg wtk_qform_pickup1_cfg_t;
struct wtk_qform_pickup1_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    wtk_stft2_cfg_t stft2;
    int rate;

    float preemph;
	float notch_radius;
	float notch_radius_den;

    wtk_aspec_cfg_t aspec;

    float *mic_class[2];
    wtk_aspec_cfg_t aspec_class1;
    wtk_aspec_cfg_t aspec_class2;

    wtk_bf_cfg_t bf;

    int theta_range;

    int ntheta_range;
    float *ntheta_center;
    int ntheta_num;

    int lf;
    int lt;

    float specsum_fs;
    float specsum_fe;
    int specsum_ns;
    int specsum_ne;

    float ncov_alpha;
    int init_ncovnf;

    float scov_alpha;
    int init_scovnf;

    int flushcohvgap;
    float cohv_thresh;

    int flushcovgap;
    int cov_hist;
    int batch;

    unsigned use_line:1;
    unsigned use_covhist:1;
    unsigned use_noiseblock:1;
    unsigned use_noiseblock2:1;
    unsigned use_two_aspecclass:1;
    unsigned use_cline1:1;
    unsigned use_cline2:1;
    unsigned debug:1;
};

int wtk_qform_pickup1_cfg_init(wtk_qform_pickup1_cfg_t *cfg);
int wtk_qform_pickup1_cfg_clean(wtk_qform_pickup1_cfg_t *cfg);
int wtk_qform_pickup1_cfg_update_local(wtk_qform_pickup1_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_qform_pickup1_cfg_update(wtk_qform_pickup1_cfg_t *cfg);
int wtk_qform_pickup1_cfg_update2(wtk_qform_pickup1_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_qform_pickup1_cfg_t* wtk_qform_pickup1_cfg_new(char *cfg_fn);
void wtk_qform_pickup1_cfg_delete(wtk_qform_pickup1_cfg_t *cfg);
wtk_qform_pickup1_cfg_t* wtk_qform_pickup1_cfg_new_bin(char *bin_fn);
void wtk_qform_pickup1_cfg_delete_bin(wtk_qform_pickup1_cfg_t *cfg);

void wtk_qform_pickup1_cfg_set_theta_range(wtk_qform_pickup1_cfg_t *cfg,float theta_range);
#ifdef __cplusplus
};
#endif
#endif