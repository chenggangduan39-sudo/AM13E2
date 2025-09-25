#ifndef WTK_BFIO_VBOX_WTK_VBOXBF_STEREO_CFG
#define WTK_BFIO_VBOX_WTK_VBOXBF_STEREO_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/masknet/wtk_gainnet7.h"
#include "wtk/bfio/masknet/wtk_gainnet2.h"
#include "wtk/bfio/masknet/wtk_gainnet.h"
#include "wtk/bfio/afilter/wtk_rls.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#include "wtk/bfio/maskdenoise/wtk_bankfeat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vboxebf_stereo_cfg wtk_vboxebf_stereo_cfg_t;
struct wtk_vboxebf_stereo_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;
    int rate;
    
	wtk_rls_cfg_t echo_rls;

    wtk_qmmse_cfg_t qmmse;

    float spenr_thresh;
    int spenr_cnt;

    float micenr_thresh;
    int micenr_cnt;

    wtk_bankfeat_cfg_t bankfeat;

    int featm_lm;
    int featsp_lm;

    char *aecmdl_fn;
    wtk_gainnet2_cfg_t *gainnet2;

    float g_minthresh;

    char *agcmdl_fn;
	wtk_gainnet_cfg_t *agc_gainnet;
    float agc_a;
    float agc_b;
    float eagc_a;
    float eagc_b;
    float g2_min;
    float g2_max;
    float agcaddg;

    float gbias;

	int channel;
	int *mic_channel;
	int nmicchannel;
	int *sp_channel;
	int nspchannel;

    float ralpha;
    float ralpha2;
    float echo_ralpha;
    float echo_ralpha2;

    int clip_s;
    int clip_e;

	float **omic_pos;
	int **omic_channel;
    int nomicchannel;
    int *orls_channel;
    int noutchannel;
	float eye;
    float speed;

    float **otheta;

	wtk_equalizer_cfg_t eq;

    unsigned use_rbin_res:1;
    unsigned use_eq:1;
    unsigned use_qmmse:1;
};

int wtk_vboxebf_stereo_cfg_init(wtk_vboxebf_stereo_cfg_t *cfg);
int wtk_vboxebf_stereo_cfg_clean(wtk_vboxebf_stereo_cfg_t *cfg);
int wtk_vboxebf_stereo_cfg_update_local(wtk_vboxebf_stereo_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_vboxebf_stereo_cfg_update(wtk_vboxebf_stereo_cfg_t *cfg);
int wtk_vboxebf_stereo_cfg_update2(wtk_vboxebf_stereo_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_vboxebf_stereo_cfg_t* wtk_vboxebf_stereo_cfg_new(char *fn);
void wtk_vboxebf_stereo_cfg_delete(wtk_vboxebf_stereo_cfg_t *cfg);
wtk_vboxebf_stereo_cfg_t* wtk_vboxebf_stereo_cfg_new_bin(char *fn);
void wtk_vboxebf_stereo_cfg_delete_bin(wtk_vboxebf_stereo_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
