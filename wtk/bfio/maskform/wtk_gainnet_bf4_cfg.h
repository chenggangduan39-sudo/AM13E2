#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_BF4_CFG
#define WTK_BFIO_MASKFORM_WTK_GAINNET_BF4_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/masknet/wtk_gainnet5.h"
#include "wtk/bfio/masknet/wtk_gainnet.h"
#include "wtk/bfio/masknet/wtk_gainnet3.h"
#include "wtk/bfio/afilter/wtk_rls.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#include "wtk/bfio/aspec/wtk_aspec.h"
#include "wtk/bfio/qform/wtk_qenvelope.h"
#include "wtk/bfio/ssl/wtk_ssl2.h"
#include "wtk/bfio/ssl/wtk_maskssl2.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_bf4_cfg wtk_gainnet_bf4_cfg_t;
struct wtk_gainnet_bf4_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;
    int rate;

	int nb_bands;
    int ceps_mem;
    int nb_delta_ceps;

    int nb_features;
    int nb_features_x;

    float spenr_thresh;
    int spenr_cnt;

    float micenr_thresh;
    int micenr_cnt;

    float micnenr;
    float spnenr;

    char *mdl_fn;
	wtk_gainnet5_cfg_t *gainnet;

    wtk_qmmse_cfg_t preqmmse;

    float agc_a;
    float agc_b;

	int channel;
	int *mic_channel;
	int nmicchannel;
	int *sp_channel;
	int nspchannel;
    float fft_scale;

    float theta;
	float phi;

    wtk_aspec_cfg_t aspec;
    wtk_covm_cfg_t covm;
	wtk_bf_cfg_t bf;
	wtk_rls_cfg_t echo_rls;

    wtk_qenvelope_cfg_t qenvl;

    wtk_qmmse_cfg_t qmmse;
    wtk_maskssl2_cfg_t maskssl2;

    int theta_range;

    int lf;
    int lt;

    int specsum_fs;
    int specsum_fe;

    int pframe_fs;
    int pframe_fe;
    float pframe_thresh;
    float pframe_alpha;

    float **t_r_qenvl;
    int t_r_number;
    float qenvel_alpha;

    unsigned use_line:1;
    unsigned use_rbin_res:1;
    unsigned use_preemph:1;
    unsigned use_postsingle:1;
    unsigned use_miccnon:1;
    unsigned use_spcnon:1;
    unsigned use_ceps:1;
    unsigned use_preqmmse:1;
    unsigned use_qmmse:1;
    unsigned use_qenvelope:1;
    unsigned use_sqenvelope:1;
    unsigned use_t_r_qenvelope:1;
    unsigned use_simple_qenvelope:1;
    unsigned use_fftsbf:1;
    unsigned use_maskssl2:1;
    unsigned use_ssl_delay:1;
};

int wtk_gainnet_bf4_cfg_init(wtk_gainnet_bf4_cfg_t *cfg);
int wtk_gainnet_bf4_cfg_clean(wtk_gainnet_bf4_cfg_t *cfg);
int wtk_gainnet_bf4_cfg_update_local(wtk_gainnet_bf4_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_gainnet_bf4_cfg_update(wtk_gainnet_bf4_cfg_t *cfg);
int wtk_gainnet_bf4_cfg_update2(wtk_gainnet_bf4_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_gainnet_bf4_cfg_t* wtk_gainnet_bf4_cfg_new(char *fn);
void wtk_gainnet_bf4_cfg_delete(wtk_gainnet_bf4_cfg_t *cfg);
wtk_gainnet_bf4_cfg_t* wtk_gainnet_bf4_cfg_new_bin(char *fn);
void wtk_gainnet_bf4_cfg_delete_bin(wtk_gainnet_bf4_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
