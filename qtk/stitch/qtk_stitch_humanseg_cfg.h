#ifndef __QTK_STITCH_HUMANSEG_CFG_H__
#define __QTK_STITCH_HUMANSEG_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"

#ifndef IPU_DEC
#include "qtk/nnrt/qtk_nnrt_cfg.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif 

typedef struct qtk_stitch_humanseg_cfg{
    int in_w;
    int in_h;
#ifndef IPU_DEC
    qtk_nnrt_cfg_t nnrt;
#endif
    char *fn;
}qtk_stitch_humanseg_cfg_t;

int qtk_stitch_humanseg_cfg_init(qtk_stitch_humanseg_cfg_t *cfg);
int qtk_stitch_humanseg_cfg_clean(qtk_stitch_humanseg_cfg_t *cfg);
int qtk_stitch_humanseg_cfg_update_local(qtk_stitch_humanseg_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_stitch_humanseg_cfg_update(qtk_stitch_humanseg_cfg_t *cfg);

#ifdef __cplusplus
}
#endif

#endif