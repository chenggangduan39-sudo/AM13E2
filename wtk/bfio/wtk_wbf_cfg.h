#ifndef WTK_BFIO_WTK_WBF_CFG
#define WTK_BFIO_WTK_WBF_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/qform/wtk_bf.h" 
#include "wtk/bfio/aspec/wtk_aspec.h" 
#include "wtk/bfio/qform/wtk_qmmse.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wbf_cfg wtk_wbf_cfg_t;
struct wtk_wbf_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;
    
    wtk_stft2_cfg_t stft2;
    wtk_aspec_cfg_t aspec;
    wtk_bf_cfg_t bf;

    float ncov_alpha;
    int init_ncovnf;
    float scov_alpha;
    int init_scovnf;

    int lf;
    int lt;

    int wbf_cnt;
    int range_interval;

    int flushbfgap;

    wtk_qmmse_cfg_t qmmse;

    int rate;
    float specsum_fs;
    float specsum_fe;
    int specsum_ns;
    int specsum_ne;
    float specsum_bl;

    unsigned use_specsum_bl:1;
    unsigned use_line:1;
    unsigned use_post:1;
    unsigned debug:1;
};

int wtk_wbf_cfg_init(wtk_wbf_cfg_t *cfg);
int wtk_wbf_cfg_clean(wtk_wbf_cfg_t *cfg);
int wtk_wbf_cfg_update_local(wtk_wbf_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_wbf_cfg_update(wtk_wbf_cfg_t *cfg);
int wtk_wbf_cfg_update2(wtk_wbf_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_wbf_cfg_t* wtk_wbf_cfg_new(char *fn);
void wtk_wbf_cfg_delete(wtk_wbf_cfg_t *cfg);
wtk_wbf_cfg_t* wtk_wbf_cfg_new_bin(char *fn);
void wtk_wbf_cfg_delete_bin(wtk_wbf_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif