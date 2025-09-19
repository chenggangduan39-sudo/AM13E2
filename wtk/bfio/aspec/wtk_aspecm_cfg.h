#ifndef WTK_BFIO_ASPEC_WTK_ASPECM_CFG_H
#define WTK_BFIO_ASPEC_WTK_ASPECM_CFG_H
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/aspec/wtk_aspec.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_aspecm_cfg wtk_aspecm_cfg_t;

struct wtk_aspecm_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    int rate;
	int nbin;
    int nmic;
    float **mic_pos;

    wtk_qmmse_cfg_t qmmse;
    wtk_aspec_cfg_t aspec;

    float theta;
    float theta_range;
    float specsum_fs;
    float specsum_fe;
    int specsum_ns;
    int specsum_ne;

    int q_nf;
    int right_nf;
    float min_speccrest;
    float envelope_thresh;
    float right_min_thresh;
    float q_alpha;

    unsigned use_qmmse:1;
    unsigned use_sqenvelope:1;
    unsigned use_line:1;
};

int wtk_aspecm_cfg_init(wtk_aspecm_cfg_t *cfg);
int wtk_aspecm_cfg_clean(wtk_aspecm_cfg_t *cfg);
int wtk_aspecm_cfg_update(wtk_aspecm_cfg_t *cfg);
int wtk_aspecm_cfg_update2(wtk_aspecm_cfg_t *cfg, wtk_source_loader_t *sl);
int wtk_aspecm_cfg_update_local(wtk_aspecm_cfg_t *cfg, wtk_local_cfg_t *lc);

wtk_aspecm_cfg_t* wtk_aspecm_cfg_new(char *fn);
void wtk_aspecm_cfg_delete(wtk_aspecm_cfg_t *cfg);
wtk_aspecm_cfg_t* wtk_aspecm_cfg_new_bin(char *fn);
void wtk_aspecm_cfg_delete_bin(wtk_aspecm_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
