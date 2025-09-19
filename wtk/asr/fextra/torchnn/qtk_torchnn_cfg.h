#ifndef QTK_ASR_TORCH_NN_CFG
#define QTK_ASR_TORCH_NN_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_torchnn_cfg qtk_torchnn_cfg_t;
struct qtk_torchnn_cfg
{
	char *torchnn_fn;
};

int qtk_torchnn_cfg_init(qtk_torchnn_cfg_t *cfg);
int qtk_torchnn_cfg_clean(qtk_torchnn_cfg_t *cfg);
int qtk_torchnn_cfg_update_local(qtk_torchnn_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_torchnn_cfg_update(qtk_torchnn_cfg_t *cfg);
int qtk_torchnn_cfg_update2(qtk_torchnn_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
