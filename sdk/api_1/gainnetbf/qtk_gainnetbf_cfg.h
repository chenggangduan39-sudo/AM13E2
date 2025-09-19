#ifndef __QTK_GAINNET_BF_CFG_H__
#define __QTK_GAINNET_BF_CFG_H__
#include "wtk/bfio/maskform/wtk_gainnet_bf_cfg.h"
#include "wtk/bfio/maskform/wtk_gainnet_bf3_cfg.h"
#include "wtk/bfio/maskform/wtk_gainnet_bf4_cfg.h"
#include "wtk/bfio/maskform/wtk_gainnet_bf6_cfg.h"
#include "wtk/bfio/qform/wtk_rtjoin2_cfg.h"
#include "wtk/bfio/qform/wtk_mix_speech_cfg.h"

#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct qtk_gainnetbf_cfg{
	wtk_string_t cfg_fn;
	wtk_gainnet_bf_cfg_t *gainnet_bf_cfg;
	wtk_gainnet_bf3_cfg_t *gainnet_bf3_cfg;
	wtk_gainnet_bf4_cfg_t *gainnet_bf4_cfg;
	wtk_gainnet_bf6_cfg_t *gainnet_bf6_cfg;
	wtk_rtjoin2_cfg_t *rtjoin2_cfg;
	wtk_mix_speech_cfg_t *mix_speech_cfg;
	wtk_main_cfg_t *main_cfg;
	wtk_rbin2_t *rbin;
	wtk_cfg_file_t *cfile;
	int *skip_channels;
	float mic_shift;
	float echo_shift;
	int nskip;
	int mics;
	float theta;
	float phi;
	int theta_range;

	float agca;
	float gbais;

	int max_extp;
	int online_tms;
	int continue_count;
	float energy_sum;
	float zero_sum;
	
	int energy_thr_count;
	int energy_thr_time;
	float energy_thr;
	int left_count;
	int use_ssl_delay;
	int cache_len;

	int join_channel;
	int main_channel;
	float mic_scale;

	char *input_fn;
	char *out_fn;

	unsigned int use_ssl_filter:1;
	unsigned int use_fftsbf:1;
	unsigned int use_log_wav:1;
	unsigned int use_energy_debug:1;
	unsigned int use_min_sil:1;
	unsigned int use_cachebuf:1;
	unsigned int use_bin:1;
	unsigned int use_manual:1;
	unsigned int use_gainnet_bf:1;
	unsigned int use_gainnet_bf3:1;
	unsigned int use_gainnet_bf4:1;
	unsigned int use_gainnet_bf6:1;
	unsigned int use_rtjoin2:1;
	unsigned int use_mix_speech:1;
}qtk_gainnetbf_cfg_t;

int qtk_gainnetbf_cfg_init(qtk_gainnetbf_cfg_t *cfg);
int qtk_gainnetbf_cfg_clean(qtk_gainnetbf_cfg_t *cfg);
int qtk_gainnetbf_cfg_update_local(qtk_gainnetbf_cfg_t  *cfg, wtk_local_cfg_t *main);
int qtk_gainnetbf_cfg_update(qtk_gainnetbf_cfg_t *cfg);
int qtk_gainnetbf_cfg_update2(qtk_gainnetbf_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_gainnetbf_cfg_t *qtk_gainnetbf_cfg_new(char *fn);
void qtk_gainnetbf_cfg_delete(qtk_gainnetbf_cfg_t *cfg);
qtk_gainnetbf_cfg_t *qtk_gainnetbf_cfg_new_bin(char *bin_fn);
void qtk_gainnetbf_cfg_delete_bin(qtk_gainnetbf_cfg_t *cfg);
#ifdef __cplusplus
};
#endif



#endif
