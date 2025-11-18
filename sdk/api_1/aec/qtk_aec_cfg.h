#ifndef __QTK_API_QTK_AEC_CFG_H__
#define __QTK_API_QTK_AEC_CFG_H__
#include "wtk/bfio/aec/wtk_aec_cfg.h"
#include "wtk/bfio/aec/cmask_aec/wtk_cmask_aec_cfg.h"
#include "wtk/bfio/aec/cmask_aec/wtk_cmask_aec2_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct qtk_aec_cfg{
	wtk_string_t cfg_fn;
	wtk_string_t cache_fn;
	wtk_aec_cfg_t *aec_cfg;
	wtk_cmask_aec_cfg_t *caec_cfg;
	wtk_cmask_aec_cfg_t *caec2_cfg;
	wtk_main_cfg_t *main_cfg;
	wtk_rbin2_t *rbin;
	wtk_cfg_file_t *cfile;

    float **mic_pos;
    int nmic;

	int channel;
	int spk_channel;

	float noise_suppress;
	float mic_shift;
	float echo_shift;
	float spk_shift;
	float energy_sum;

	unsigned int use_log_wav:1;
	unsigned int use_manual:1;
	unsigned int use_cmask_aec:1;
	unsigned int use_cmask_aec2:1;
	unsigned int use_bin:1;
	unsigned int use_cache_mode:1;
}qtk_aec_cfg_t;

int qtk_aec_cfg_init(qtk_aec_cfg_t *cfg);
int qtk_aec_cfg_clean(qtk_aec_cfg_t *cfg);
int qtk_aec_cfg_update_local(qtk_aec_cfg_t  *cfg, wtk_local_cfg_t *main);
int qtk_aec_cfg_update(qtk_aec_cfg_t *cfg);
int qtk_aec_cfg_update2(qtk_aec_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_aec_cfg_t *qtk_aec_cfg_new(char *fn);
void qtk_aec_cfg_delete(qtk_aec_cfg_t *cfg);
qtk_aec_cfg_t *qtk_aec_cfg_new_bin(char *bin_fn);
void qtk_aec_cfg_delete_bin(qtk_aec_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
