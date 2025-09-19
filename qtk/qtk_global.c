#ifdef QTK_WITH_TRACY
#include "qtk/tracy/qtk_tracy.h"
#include "wtk/core/cfg/wtk_main_cfg.h"

static wtk_main_cfg_t *tracy_cfg;
qtk_tracy_t *glb_tracy;
#endif

int qtk_global_init(void) {
#ifdef QTK_WITH_TRACY
    if (glb_tracy) {
        return 0;
    }
    char *cfn = getenv("QTK_TRACY_CFG");
    tracy_cfg = wtk_main_cfg_new_type(qtk_tracy_cfg, cfn);
    glb_tracy = qtk_tracy_new((qtk_tracy_cfg_t *)tracy_cfg->cfg);
    qtk_tracy_start(glb_tracy);
#endif
    return 0;
}

int qtk_global_clean(void) {
#ifdef QTK_WITH_TRACY
    qtk_tracy_stop(glb_tracy);
    qtk_tracy_delete(glb_tracy);
    wtk_main_cfg_delete(tracy_cfg);
    glb_tracy = NULL;
    tracy_cfg = NULL;
#endif
    return 0;
}