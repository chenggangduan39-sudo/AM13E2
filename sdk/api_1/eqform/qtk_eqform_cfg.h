#ifndef __QTK_EQFORM_CFG_H__
#define __QTK_EQFORM_CFG_H__

#include "wtk/bfio/wtk_eqform_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct qtk_eqform_cfg{
	wtk_string_t cfg_fn;
	wtk_eqform_cfg_t *eqform_cfg;
	wtk_main_cfg_t *main_cfg;
	wtk_rbin2_t *rbin;
	wtk_cfg_file_t *cfile;

    float **mic_pos;
    int nmic;

	int mics;
	float mic_shift;
	float echo_shit;
	float fix_theta;
	float phi;
	float theta_range;
	float noise_suppress;
	unsigned int use_manual:1;
	unsigned int use_bin:1;	
}qtk_eqform_cfg_t;

int qtk_eqform_cfg_init(qtk_eqform_cfg_t *cfg);
int qtk_eqform_cfg_clean(qtk_eqform_cfg_t *cfg);
int qtk_eqform_cfg_update_local(qtk_eqform_cfg_t  *cfg, wtk_local_cfg_t *main);
int qtk_eqform_cfg_update(qtk_eqform_cfg_t *cfg);
int qtk_eqform_cfg_update2(qtk_eqform_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_eqform_cfg_t *qtk_eqform_cfg_new(char *fn);
void qtk_eqform_cfg_delete(qtk_eqform_cfg_t *cfg);
qtk_eqform_cfg_t *qtk_eqform_cfg_new_bin(char *bin_fn);
void qtk_eqform_cfg_delete_bin(qtk_eqform_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif