#ifndef WTK_BFIO_QFORM_WTK_QMMSE_CFG
#define WTK_BFIO_QFORM_WTK_QMMSE_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_qmmse_cfg wtk_qmmse_cfg_t;

struct wtk_qmmse_cfg
{
	int rate;
	int step;

	int nbands;
	float noise_suppress;
	float beta;
	float init_sym;
	int max_range;
	float noise_prob;

	float echo_alpha;
	float sed_alpha;

	float atcoe;

	float agc_level;
	float max_increase_step;
	float max_decrease_step;
	float max_gain;
	float max_out_gain;
	int agc_init_frame;
	float max_init_increase_step;
	float max_init_decrease_step;
	float init_max_alpha;
	float loudness_pow;

	float echo_suppress;
	float echo_suppress_active;
	float io_alpha;

	float init_tgt_eng;
	int smooth_frame;
	float smooth_scale;
	float tgt_gain_scale;
	float smooth_percent;
	float max_smooth_gain;
	float min_smooth_gain;
	
	float down_percent;
	float down_frame;
	float down_scale;
	float max_down_gain;
	float min_down_gain;
	float down_thresh;
	float down_cnt;

	float agc_mask_thresh;
	float agc_mask_scale;
	float agc_mask_scale2;
	float agc_pframe_thresh;
	float echo_agc_pframe_thresh;

	float loudness_thresh;
	float rate_scale;
	int loudness_frame;

	float entropy_thresh;
	float b_agc_scale;
	float loudness_thresh2;
	int loudness_frame2;

	float agc_mean_mask_thresh;
	float echo_agc_mean_mask_thresh;
	int agc_mask_cnt;
	int echo_agc_mask_cnt;

	unsigned use_echonoise:1;
	unsigned use_bank:1;
	unsigned use_logmmse:1;
	unsigned use_imcra_org:1;
	unsigned use_agc:1;
	unsigned use_cnon:1;
	unsigned use_sed:1;
	unsigned use_sed2:1;
	unsigned use_agc_smooth:1;
	unsigned use_down_agc:1;
	unsigned use_agc_mask:1;
};

int wtk_qmmse_cfg_init(wtk_qmmse_cfg_t *cfg);
int wtk_qmmse_cfg_clean(wtk_qmmse_cfg_t *cfg);
int wtk_qmmse_cfg_update_local(wtk_qmmse_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_qmmse_cfg_update(wtk_qmmse_cfg_t *cfg);
int wtk_qmmse_cfg_update2(wtk_qmmse_cfg_t *cfg,wtk_source_loader_t *sl);

void wtk_qmmse_cfg_set_noise_suppress(wtk_qmmse_cfg_t *cfg,float noise_suppress);

#ifdef __cplusplus
};
#endif
#endif
