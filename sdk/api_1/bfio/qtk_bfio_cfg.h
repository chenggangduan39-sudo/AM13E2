#ifndef __QTK_BFIO_CFG_H__
#define __QTK_BFIO_CFG_H__

#include "wtk/bfio/wtk_bfio.h"
#include "wtk/bfio/wtk_bfio5_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct qtk_bfio_cfg{
	wtk_string_t cfg_fn;
	wtk_string_t soundscreen_fn;
	wtk_string_t vboxebf_fn;
	wtk_string_t wake_word;
	wtk_bfio_cfg_t *qform_cfg;
	wtk_bfio5_cfg_t *bfio5_cfg;
	wtk_main_cfg_t *main_cfg;
	wtk_rbin2_t *rbin;
	wtk_cfg_file_t *cfile;

    float **mic_pos;
    int nmic;

	int mics;
	float mic_shift;
	float echo_shift;
	float fix_theta;
	float phi;
	float theta_range;
	float noise_suppress;
	float low_thresh;
	unsigned int use_manual:1;
	unsigned int use_bfio:1;
	unsigned int use_bfio5:1;
	unsigned int use_soundscreen:1;
	unsigned int use_vboxebf:1;
	unsigned int use_bin:1;
}qtk_bfio_cfg_t;

int qtk_bfio_cfg_init(qtk_bfio_cfg_t *cfg);
int qtk_bfio_cfg_clean(qtk_bfio_cfg_t *cfg);
int qtk_bfio_cfg_update_local(qtk_bfio_cfg_t  *cfg, wtk_local_cfg_t *main);
int qtk_bfio_cfg_update(qtk_bfio_cfg_t *cfg);
int qtk_bfio_cfg_update2(qtk_bfio_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_bfio_cfg_t *qtk_bfio_cfg_new(char *fn);
void qtk_bfio_cfg_delete(qtk_bfio_cfg_t *cfg);
qtk_bfio_cfg_t *qtk_bfio_cfg_new_bin(char *bin_fn);
void qtk_bfio_cfg_delete_bin(qtk_bfio_cfg_t *cfg);
#ifdef __cplusplus
};
#endif



#endif