#ifndef __QTK_API_QTK_MASKBFNET_CFG_H__
#define __QTK_API_QTK_MASKBFNET_CFG_H__
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/maskbfnet/wtk_mask_bf_net_cfg.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct qtk_maskbfnet_cfg{
	wtk_string_t cfg_fn;
	wtk_string_t cache_fn;
	wtk_mask_bf_net_cfg_t *maskbfnet_cfg;
	wtk_main_cfg_t *main_cfg;
	wtk_rbin2_t *rbin;
	wtk_cfg_file_t *cfile;

    float **mic_pos;
    int nmic;

	int channel;
	int denoise_enable;

	float noise_suppress;
	float mic_shift;
	float echo_shift;

	unsigned int use_log_wav:1;
	unsigned int use_manual:1;
	unsigned int use_bin:1;
	unsigned int use_cache_mode:1;
}qtk_maskbfnet_cfg_t;

int qtk_maskbfnet_cfg_init(qtk_maskbfnet_cfg_t *cfg);
int qtk_maskbfnet_cfg_clean(qtk_maskbfnet_cfg_t *cfg);
int qtk_maskbfnet_cfg_update_local(qtk_maskbfnet_cfg_t  *cfg, wtk_local_cfg_t *main);
int qtk_maskbfnet_cfg_update(qtk_maskbfnet_cfg_t *cfg);
int qtk_maskbfnet_cfg_update2(qtk_maskbfnet_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_maskbfnet_cfg_t *qtk_maskbfnet_cfg_new(char *fn);
void qtk_maskbfnet_cfg_delete(qtk_maskbfnet_cfg_t *cfg);
qtk_maskbfnet_cfg_t *qtk_maskbfnet_cfg_new_bin(char *bin_fn);
void qtk_maskbfnet_cfg_delete_bin(qtk_maskbfnet_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
