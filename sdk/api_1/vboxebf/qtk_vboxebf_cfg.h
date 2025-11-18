#ifndef __QTK_VBOXEBF_CFG_H__
#define __QTK_VBOXEBF_CFG_H__
#include "wtk/bfio/vbox/wtk_vboxebf3_cfg.h"
#include "wtk/bfio/vbox/wtk_vboxebf4_cfg.h"
#include "wtk/bfio/vbox/wtk_vboxebf6_cfg.h"
#include "wtk/bfio/maskbfnet/wtk_mask_bf_net_cfg.h"
#ifdef USE_AHS
#include "wtk/bfio/ahs/qtk_ahs_cfg.h"
#endif

#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct qtk_vboxebf_cfg{
	wtk_string_t cfg_fn;
	wtk_string_t cache_fn;
	wtk_vboxebf3_cfg_t *vebf3_cfg;
	wtk_vboxebf4_cfg_t *vebf4_cfg;
	wtk_vboxebf6_cfg_t *vebf6_cfg;
	wtk_mask_bf_net_cfg_t *mask_bf_net_cfg;
#ifdef USE_AHS
	qtk_ahs_cfg_t *ahs_cfg;
#endif
	wtk_main_cfg_t *main_cfg;
	wtk_rbin2_t *rbin;
	wtk_cfg_file_t *cfile;
	wtk_string_t theta_fn;
	int *skip_channels;
	float mic_shift;
	float spk_shift;
	float echo_shift;
	int channel;
	int spk_channel;
	int nskip;
	int mics;
	int max_extp;
	int online_tms;
	int theta_range;
	int continue_count;
	int theta_step;
	int online_frame_step;
	int specsum_fs;
	int specsum_fe;
	int lf;
	int use_ssl;
	int use_maskssl;
	int use_maskssl2;
	int agc_enable;
	int echo_enable;
	int denoise_enable;
	int ssl_enable;
	int use_erlssingle;
	int aec_level;
	int ans_level;
	float suppress;
	float gbias;
	float energy_sum;
	float zero_sum;
	float agca;
	float bfmu;
	float echo_bfmu;
	float noise_suppress;
	float echo_suppress;
	float echo_suppress_active;
	float spenr_thresh;
	float sym;
	float mic_volume;
	float agc_level;
	int use_cnon;
	int use_fftsbf;
	int use_ssl_delay;

	unsigned AGC:1;
	unsigned AEC:1;
	unsigned ANS:1;
	float mic_shift2;
	float agc_a;

	unsigned use_log_wav:1;
	// unsigned use_vboxebf:1;
	// unsigned use_vboxebf2:1;
	unsigned use_vboxebf3:1;
	unsigned use_vboxebf4:1;
	unsigned use_vboxebf6:1;
	unsigned use_ahs:1;
	unsigned use_mask_bf_net:1;
	// unsigned use_vboxebf4_multi:1;
	// unsigned use_vboxebf5:1;
	unsigned use_bin:1;
	unsigned use_ssl_and_qform:1;
	unsigned use_manual:1;
	unsigned use_thread:1;
	unsigned use_ssl_filter:1;
	unsigned use_cache_mode:1;
}qtk_vboxebf_cfg_t;

int qtk_vboxebf_cfg_init(qtk_vboxebf_cfg_t *cfg);
int qtk_vboxebf_cfg_clean(qtk_vboxebf_cfg_t *cfg);
int qtk_vboxebf_cfg_update_local(qtk_vboxebf_cfg_t  *cfg, wtk_local_cfg_t *main);
int qtk_vboxebf_cfg_update(qtk_vboxebf_cfg_t *cfg);
int qtk_vboxebf_cfg_update2(qtk_vboxebf_cfg_t *cfg, wtk_source_loader_t *sl);
void qtk_vboxebf_cfg_set_agcanc(qtk_vboxebf_cfg_t *cfg);

qtk_vboxebf_cfg_t *qtk_vboxebf_cfg_new(char *fn);
void qtk_vboxebf_cfg_delete(qtk_vboxebf_cfg_t *cfg);
qtk_vboxebf_cfg_t *qtk_vboxebf_cfg_new_bin(char *bin_fn);
void qtk_vboxebf_cfg_delete_bin(qtk_vboxebf_cfg_t *cfg);
#ifdef __cplusplus
};
#endif



#endif
