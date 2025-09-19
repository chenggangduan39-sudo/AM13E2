#ifndef WTK_BFIO_MASKNET_WTK_GAINNET_CFG
#define WTK_BFIO_MASKNET_WTK_GAINNET_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk_grunet.h"
#include "wtk_lstmnet.h"
#include "wtk_dnnnet.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_cfg wtk_gainnet_cfg_t;

//denoise

struct wtk_gainnet_cfg
{
    char *model_fn;
    int ifeat_len;
    
    wtk_dnnnet_layer_t *in_dnn;
    wtk_grunet_layer_t *vad_gru;
    wtk_lstmnet_layer_t *vad_lstm;
    // wtk_dnnnet_layer_t *vad_odnn;

    wtk_grunet_layer_t *noise_gru;
    wtk_lstmnet_layer_t *noise_lstm;
    wtk_grunet_layer_t *denoise_gru;
    wtk_lstmnet_layer_t *denoise_lstm;
    wtk_dnnnet_layer_t *denoise_odnn;

    wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
    void *hook;

    unsigned use_lstm:1;
};

int wtk_gainnet_cfg_init(wtk_gainnet_cfg_t *cfg);
int wtk_gainnet_cfg_clean(wtk_gainnet_cfg_t *cfg);
int wtk_gainnet_cfg_update_local(wtk_gainnet_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_gainnet_cfg_update(wtk_gainnet_cfg_t *cfg);
int wtk_gainnet_cfg_update2(wtk_gainnet_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_gainnet_cfg_t* wtk_gainnet_cfg_new_bin2(char *bin_fn);
int wtk_gainnet_cfg_delete_bin2(wtk_gainnet_cfg_t *cfg);
wtk_gainnet_cfg_t* wtk_gainnet_cfg_new_bin3(char *bin_fn,unsigned int seek_pos);
int wtk_gainnet_cfg_delete_bin3(wtk_gainnet_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif