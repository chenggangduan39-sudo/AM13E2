#ifndef WTK_BFIO_MASKNET_WTK_GAINNET6_CFG
#define WTK_BFIO_MASKNET_WTK_GAINNET6_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk_grunet.h"
#include "wtk_dnnnet.h"
#include "wtk_cnnnet.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet6_cfg wtk_gainnet6_cfg_t;

struct wtk_gainnet6_cfg
{
    char *model_fn;

    wtk_cnnnet_layer_t *de_cnn;
    wtk_dnnnet_layer_t *de_dnn;
    wtk_grunet_layer_t *de_gru;
    wtk_grunet_layer_t *de_gru2;
    wtk_grunet_layer_t *de_gru3;

    wtk_cnnnet_layer_t *der_cnn;
    wtk_dnnnet_layer_t *der_dnn;
    wtk_grunet_layer_t *der_gru;
    wtk_grunet_layer_t *der_gru2;
    wtk_grunet_layer_t *der_gru3;
    wtk_dnnnet_layer_t *der_odnn;

    wtk_dnnnet_layer_t *agc_dnn;
    wtk_grunet_layer_t *agc_gru;
    wtk_grunet_layer_t *agc_gru2;
    wtk_grunet_layer_t *agc_gru3;
    wtk_dnnnet_layer_t *agc_odnn;

    wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
    void *hook;

    unsigned use_agc:1;
};

int wtk_gainnet6_cfg_init(wtk_gainnet6_cfg_t *cfg);
int wtk_gainnet6_cfg_clean(wtk_gainnet6_cfg_t *cfg);
int wtk_gainnet6_cfg_update_local(wtk_gainnet6_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_gainnet6_cfg_update(wtk_gainnet6_cfg_t *cfg);
int wtk_gainnet6_cfg_update2(wtk_gainnet6_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_gainnet6_cfg_t* wtk_gainnet6_cfg_new_bin2(char *bin_fn);
int wtk_gainnet6_cfg_delete_bin2(wtk_gainnet6_cfg_t *cfg);
wtk_gainnet6_cfg_t* wtk_gainnet6_cfg_new_bin3(char *bin_fn,unsigned int seek_pos);
int wtk_gainnet6_cfg_delete_bin3(wtk_gainnet6_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif