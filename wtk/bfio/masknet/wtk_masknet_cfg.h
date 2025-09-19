#ifndef WTK_BFIO_MASKNET_WTK_MASKNET_CFG
#define WTK_BFIO_MASKNET_WTK_MASKNET_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk_cnnnet.h"
#include "wtk_lstmnet.h"
#include "wtk_tdnnnet.h"
#include "wtk_dnnnet.h"
#include "wtk_grunet.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_masknet_cfg wtk_masknet_cfg_t;

typedef enum
{
    WTK_CNNNET_LAYER,
    WTK_LSTMNET_LAYER,
    WTK_TDNNNET_LAYER,
    WTK_DNNNET_LAYER,
    WTK_GRUNET_LAYER,
}wtk_masknet_layer_type_t;

typedef struct
{
    wtk_queue_node_t q_n;

    wtk_cnnnet_layer_t *cnn;
    wtk_lstmnet_layer_t *lstm;
    wtk_tdnnnet_layer_t *tdnn;
    wtk_dnnnet_layer_t *dnn;
    wtk_grunet_layer_t *gru;

    wtk_masknet_layer_type_t type;
}wtk_masknet_layer_t;

struct wtk_masknet_cfg
{
    char *model_fn;
    wtk_queue_t layer_q;
    int layer_depth;
    
    int ifeat_len;

    wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
    void *hook;
};

int wtk_masknet_cfg_init(wtk_masknet_cfg_t *cfg);
int wtk_masknet_cfg_clean(wtk_masknet_cfg_t *cfg);
int wtk_masknet_cfg_update_local(wtk_masknet_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_masknet_cfg_update(wtk_masknet_cfg_t *cfg);
int wtk_masknet_cfg_update2(wtk_masknet_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_masknet_cfg_t* wtk_masknet_cfg_new_bin2(char *bin_fn);
int wtk_masknet_cfg_delete_bin2(wtk_masknet_cfg_t *cfg);
wtk_masknet_cfg_t* wtk_masknet_cfg_new_bin3(char *bin_fn,unsigned int seek_pos);
int wtk_masknet_cfg_delete_bin3(wtk_masknet_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif