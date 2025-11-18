#ifndef WTK_BFIO_QFORM_BEAMNET_WTK_beam_ft_CFG_H
#define WTK_BFIO_QFORM_BEAMNET_WTK_beam_ft_CFG_H
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WTK_BEAM_FEATURE_AVNET_REGION,
} wtk_beam_ft_model_type_t;

typedef struct wtk_beam_ft_cfg wtk_beam_ft_cfg_t;

struct wtk_beam_ft_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    int wins;
    int nbin;
    int rate;
    float sv;
    int channel;
    int nmic;
    float **mic_pos;

    int out_channels;
    int ref_channel;

    float theta_step;
    float min_theta;
    float max_theta;
    int n_theta;

    unsigned int use_line : 1;
    unsigned int use_csn : 1;
};

int wtk_beam_ft_cfg_init(wtk_beam_ft_cfg_t *cfg);
int wtk_beam_ft_cfg_clean(wtk_beam_ft_cfg_t *cfg);
int wtk_beam_ft_cfg_update(wtk_beam_ft_cfg_t *cfg);
int wtk_beam_ft_cfg_update2(wtk_beam_ft_cfg_t *cfg, wtk_source_loader_t *sl);
int wtk_beam_ft_cfg_update_local(wtk_beam_ft_cfg_t *cfg, wtk_local_cfg_t *lc);

wtk_beam_ft_cfg_t *wtk_beam_ft_cfg_new(char *fn);
void wtk_beam_ft_cfg_delete(wtk_beam_ft_cfg_t *cfg);
wtk_beam_ft_cfg_t *wtk_beam_ft_cfg_new_bin(char *fn);
void wtk_beam_ft_cfg_delete_bin(wtk_beam_ft_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
