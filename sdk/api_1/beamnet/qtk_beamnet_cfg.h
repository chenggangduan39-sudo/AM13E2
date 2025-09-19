#ifndef __QTK_BEAMNET_CFG_H__
#define __QTK_BEAMNET_CFG_H__

#include "wtk/bfio/qform/beamnet/wtk_beamnet_cfg.h"
#include "wtk/bfio/qform/beamnet/wtk_beamnet2_cfg.h"
#include "wtk/bfio/qform/beamnet/wtk_beamnet3_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct qtk_beamnet_cfg{
	wtk_string_t cfg_fn;
	wtk_beamnet_cfg_t *qform_cfg;
	wtk_beamnet2_cfg_t *qform2_cfg;
	wtk_beamnet3_cfg_t *qform3_cfg;
	wtk_main_cfg_t *main_cfg;
	wtk_rbin2_t *rbin;
	wtk_cfg_file_t *cfile;

    float **mic_pos;
    int nmic;

	int mics;
	float mic_shift;
	float echo_shitt;
	float fix_theta;
	float phi;
	float theta_range;
	float noise_suppress;
	unsigned int use_manual:1;
	unsigned int use_beamnet:1;
	unsigned int use_beamnet2:1;
	unsigned int use_beamnet3:1;
	unsigned int use_bin:1;
}qtk_beamnet_cfg_t;

int qtk_beamnet_cfg_init(qtk_beamnet_cfg_t *cfg);
int qtk_beamnet_cfg_clean(qtk_beamnet_cfg_t *cfg);
int qtk_beamnet_cfg_update_local(qtk_beamnet_cfg_t  *cfg, wtk_local_cfg_t *main);
int qtk_beamnet_cfg_update(qtk_beamnet_cfg_t *cfg);
int qtk_beamnet_cfg_update2(qtk_beamnet_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_beamnet_cfg_t *qtk_beamnet_cfg_new(char *fn);
void qtk_beamnet_cfg_delete(qtk_beamnet_cfg_t *cfg);
qtk_beamnet_cfg_t *qtk_beamnet_cfg_new_bin(char *bin_fn);
void qtk_beamnet_cfg_delete_bin(qtk_beamnet_cfg_t *cfg);
#ifdef __cplusplus
};
#endif



#endif