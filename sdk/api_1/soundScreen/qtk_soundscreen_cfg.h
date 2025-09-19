#ifndef __QTK_SOUNDSCREEN_CFG_H__
#define __QTK_SOUNDSCREEN_CFG_H__
#include "wtk/bfio/qform/wtk_qform9_cfg.h"
#include "wtk/bfio/qform/beamnet/wtk_beamnet2_cfg.h"
#include "wtk/bfio/qform/beamnet/wtk_beamnet3_cfg.h"
#include "wtk/bfio/qform/beamnet/wtk_beamnet4_cfg.h"
#include "wtk/bfio/aec/wtk_aec_cfg.h"
#include "wtk/bfio/vbox/wtk_vboxebf3_cfg.h"
#include "wtk/bfio/qform/cmask_bfse/wtk_cmask_bfse_cfg.h"

//#define USE_DESAIXIWEI

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_soundscreen_cfg{
    wtk_string_t cfg_fn;
    wtk_string_t cfg2_fn;
    wtk_string_t cfg3_fn;
    wtk_string_t cfg4_fn;
    wtk_string_t aec_fn;
    wtk_string_t vbox_fn;
    wtk_qform9_cfg_t *qform9_cfg;
    wtk_qform9_cfg_t *qform9_cfg2;
    wtk_beamnet2_cfg_t *beamnet2_cfg;
    wtk_beamnet3_cfg_t *beamnet3_cfg;
    wtk_beamnet4_cfg_t *beamnet4_cfg;
    wtk_beamnet4_cfg_t *beamnet4_cfg2;
    wtk_beamnet4_cfg_t *beamnet4_cfg3;
    wtk_beamnet4_cfg_t *beamnet4_cfg4;
    wtk_cmask_bfse_cfg_t *cmask_bfse_cfg;
    wtk_aec_cfg_t *aec_cfg;
    wtk_vboxebf3_cfg_t *vboxebf3_cfg;
    wtk_main_cfg_t *main_cfg;
    wtk_rbin2_t *rbin;
	wtk_cfg_file_t *cfile;
	int *thetas;
	int n_theta;
    int theta_range;
    
    float **mic_pos;
    int nmic;

    int theta_range_class1;
    int theta_range_class2;
    float noise_suppress;

    unsigned int use_manual:1;
    unsigned int use_bin:1;
    unsigned int use_aec_bin:1;
    unsigned int use_vbox_bin:1;
    unsigned int use_center:1;
    unsigned int use_aec:1;
    unsigned int use_vboxebf:1;
    unsigned int use_qform9:1;
    unsigned int use_beamnet2:1;
    unsigned int use_beamnet3:1;
    unsigned int use_beamnet4:1;
    unsigned int use_cmask_bfse:1;
}qtk_soundscreen_cfg_t;

int qtk_soundscreen_cfg_init(qtk_soundscreen_cfg_t *cfg);
int qtk_soundscreen_cfg_clean(qtk_soundscreen_cfg_t *cfg);
int qtk_soundscreen_cfg_update_local(qtk_soundscreen_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_soundscreen_cfg_update(qtk_soundscreen_cfg_t *cfg);
int qtk_soundscreen_cfg_update2(qtk_soundscreen_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_soundscreen_cfg_t *qtk_soundscreen_cfg_new(char *fn);
void qtk_soundscreen_cfg_delete(qtk_soundscreen_cfg_t *cfg);
qtk_soundscreen_cfg_t *qtk_soundscreen_cfg_new_bin(char *bin_fn);
void qtk_soundscreen_cfg_delete_bin(qtk_soundscreen_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
