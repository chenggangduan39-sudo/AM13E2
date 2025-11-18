#ifndef WTK_BFIO_MASKNET_WTK_BBONENET_CFG
#define WTK_BFIO_MASKNET_WTK_BBONENET_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk_grunet.h"
#include "wtk_cnn1dnet.h"
#include "wtk_lstmnet.h"
#include "wtk_dnnnet.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_bbonenet_cfg wtk_bbonenet_cfg_t;

struct wtk_bbonenet_cfg
{
    char *model_fn;
    int ifeat_len;

    wtk_dnnnet_layer_t *idnn;
    wtk_cnn1dnet_layer_t *cnn1;
    wtk_cnn1dnet_layer_t *cnn2;
    wtk_grunet_layer_t *gru1;
    wtk_grunet_layer_t *gru2;
    wtk_grunet_layer_t *gru3;
    wtk_dnnnet_layer_t *odnn;

    wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
    void *hook;
};

int wtk_bbonenet_cfg_init(wtk_bbonenet_cfg_t *cfg);
int wtk_bbonenet_cfg_clean(wtk_bbonenet_cfg_t *cfg);
int wtk_bbonenet_cfg_update_local(wtk_bbonenet_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_bbonenet_cfg_update(wtk_bbonenet_cfg_t *cfg);
int wtk_bbonenet_cfg_update2(wtk_bbonenet_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_bbonenet_cfg_t* wtk_bbonenet_cfg_new_bin2(char *bin_fn);
int wtk_bbonenet_cfg_delete_bin2(wtk_bbonenet_cfg_t *cfg);
wtk_bbonenet_cfg_t* wtk_bbonenet_cfg_new_bin3(char *bin_fn,unsigned int seek_pos);
int wtk_bbonenet_cfg_delete_bin3(wtk_bbonenet_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif