#ifndef __WTK_WSOLA_CFG_H__
#define __WTK_WSOLA_CFG_H__
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif
#ifndef M_2PI
#define M_2PI 6.283185307179586476925286766559005
#endif
typedef struct 
{
    float * window;
    int win_sz;
}wtk_wsola_cfg_t;

int wtk_wsola_cfg_init(wtk_wsola_cfg_t *cfg);
int wtk_wsola_cfg_update_local(wtk_wsola_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_wsola_cfg_clean(wtk_wsola_cfg_t *cfg);
int wtk_wsola_cfg_update(wtk_wsola_cfg_t *cfg);
int wtk_wsola_cfg_update2(wtk_wsola_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif

#endif