#ifndef __QTK_GAINNET_DENOISE_CFG_H__
#define __QTK_GAINNET_DENOISE_CFG_H__
#include "wtk/bfio/maskdenoise/wtk_gainnet_denoise_cfg.h"
#include "wtk/bfio/aec/wtk_aec_cfg.h"
// #include "wtk/bfio/aec/wtk_gainnet_aec3_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct qtk_gdenoise_cfg{
	wtk_string_t cfg_fn;
	wtk_string_t aec_cfg_fn;
	wtk_gainnet_denoise_cfg_t *gdenoise_cfg;
	wtk_aec_cfg_t *aec_cfg;
	// wtk_gainnet_aec3_cfg_t *gaec3_cfg;
	wtk_main_cfg_t *main_cfg;
	wtk_rbin2_t *rbin;
	wtk_cfg_file_t *cfile;
	int *skip_channels;
	float mic_shift;
	float echo_shift;
	int nskip;
	int mics;
	unsigned int use_aec:1;
	// unsigned int use_gainnet_aec3:1;
	unsigned int use_bin:1;
	unsigned int use_aec_bin:1;
	unsigned int use_manual:1;
}qtk_gdenoise_cfg_t;

int qtk_gdenoise_cfg_init(qtk_gdenoise_cfg_t *cfg);
int qtk_gdenoise_cfg_clean(qtk_gdenoise_cfg_t *cfg);
int qtk_gdenoise_cfg_update_local(qtk_gdenoise_cfg_t  *cfg, wtk_local_cfg_t *main);
int qtk_gdenoise_cfg_update(qtk_gdenoise_cfg_t *cfg);
int qtk_gdenoise_cfg_update2(qtk_gdenoise_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_gdenoise_cfg_t *qtk_gdenoise_cfg_new(char *fn);
void qtk_gdenoise_cfg_delete(qtk_gdenoise_cfg_t *cfg);
qtk_gdenoise_cfg_t *qtk_gdenoise_cfg_new_bin(char *bin_fn);
void qtk_gdenoise_cfg_delete_bin(qtk_gdenoise_cfg_t *cfg);
#ifdef __cplusplus
};
#endif



#endif