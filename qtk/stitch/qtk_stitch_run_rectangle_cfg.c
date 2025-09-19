#include "qtk_stitch_run_rectangle_cfg.h"

int qtk_stitch_run_rectangle_cfg_init(qtk_stitch_run_rectangle_cfg_t *cfg)
{
    cfg->x = 0;
    cfg->y = 0;
    cfg->width = 0;
    cfg->height = 0;

    return 0;
}

int qtk_stitch_run_rectangle_cfg_clean(qtk_stitch_run_rectangle_cfg_t *cfg)
{
    return 0;
}

int qtk_stitch_run_rectangle_cfg_update_local(qtk_stitch_run_rectangle_cfg_t *cfg,wtk_local_cfg_t *main_lc)
{
    wtk_string_t *v;
    wtk_local_cfg_update_cfg_i(main_lc,cfg,x,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,y,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,width,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,height,v);
    // wtk_debug("%d %d %d %d\n",cfg->x,cfg->y,cfg->width,cfg->height);
    return 0;
}

int qtk_stitch_run_rectangle_cfg_update(qtk_stitch_run_rectangle_cfg_t *cfg)
{
    return 0;
}