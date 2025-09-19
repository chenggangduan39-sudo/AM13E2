#ifndef __QTK_API_IMG_CFG_H__
#define __QTK_API_IMG_CFG_H__
#include "sdk/api_1/asr/qtk_asr_cfg.h"
#include "wtk/asr/img/qtk_img_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_api_img_cfg qtk_api_img_cfg_t;
struct qtk_api_img_cfg
{
	wtk_main_cfg_t *img_mcfg;
	qtk_img_cfg_t *img;
	wtk_vad_cfg_t *vad;
	char* img_fn;
	char* vad_fn;
	int left_margin;
	int right_margin;
	void *hook;
	unsigned int img_use_bin:1;
	unsigned int log_wav:1;
};

int qtk_api_img_cfg_init(qtk_api_img_cfg_t *cfg);
int qtk_api_img_cfg_clean(qtk_api_img_cfg_t *cfg);
int qtk_api_img_cfg_update_local(qtk_api_img_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_api_img_cfg_update(qtk_api_img_cfg_t *cfg);
int qtk_api_img_cfg_update2(qtk_api_img_cfg_t *cfg,wtk_source_loader_t *sl);

void qtk_api_img_cfg_update_params(qtk_api_img_cfg_t *cfg,wtk_local_cfg_t *params);

qtk_api_img_cfg_t* qtk_api_img_cfg_new(char *cfg_fn);
void qtk_api_img_cfg_delete(qtk_api_img_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
