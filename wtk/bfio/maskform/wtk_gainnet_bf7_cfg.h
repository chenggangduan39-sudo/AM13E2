#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_BF7_CFG
#define WTK_BFIO_MASKFORM_WTK_GAINNET_BF7_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/masknet/wtk_masknet.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#include "wtk/bfio/aspec/wtk_aspec.h"
#include "wtk/bfio/qform/wtk_qenvelope.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_bf7_cfg wtk_gainnet_bf7_cfg_t;
struct wtk_gainnet_bf7_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;
    int rate;
    float speed;
	float **mic_pos;
	int channel;

    int features_len;

    char *mdl_fn;
	wtk_masknet_cfg_t *masknet;

    wtk_covm_cfg_t covm;
	wtk_bf_cfg_t bf;

    unsigned use_line:1;
    unsigned use_rbin_res:1;
    unsigned use_preemph:1;
    unsigned use_bf:1;
};

int wtk_gainnet_bf7_cfg_init(wtk_gainnet_bf7_cfg_t *cfg);
int wtk_gainnet_bf7_cfg_clean(wtk_gainnet_bf7_cfg_t *cfg);
int wtk_gainnet_bf7_cfg_update_local(wtk_gainnet_bf7_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_gainnet_bf7_cfg_update(wtk_gainnet_bf7_cfg_t *cfg);
int wtk_gainnet_bf7_cfg_update2(wtk_gainnet_bf7_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_gainnet_bf7_cfg_t* wtk_gainnet_bf7_cfg_new(char *fn);
void wtk_gainnet_bf7_cfg_delete(wtk_gainnet_bf7_cfg_t *cfg);
wtk_gainnet_bf7_cfg_t* wtk_gainnet_bf7_cfg_new_bin(char *fn);
void wtk_gainnet_bf7_cfg_delete_bin(wtk_gainnet_bf7_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
