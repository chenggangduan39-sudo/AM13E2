#include "qtk_stitch_run_camera_cfg.h"

int qtk_stitch_run_camera_cfg_init(qtk_stitch_run_camera_cfg_t *cfg)
{
    cfg->aspect = 0;
    cfg->focal = 0;
    cfg->ppx = 0;
    cfg->ppy = 0;
    cfg->Rt_path = NULL;
    cfg->Rt_shape = NULL;
    return 0;
}

int qtk_stitch_run_camera_cfg_clean(qtk_stitch_run_camera_cfg_t *cfg)
{
    return 0;
}
int qtk_stitch_run_camera_cfg_update_local(qtk_stitch_run_camera_cfg_t *cfg,wtk_local_cfg_t *main_lc)
{
    wtk_string_t *v = NULL;
    cfg->Rt_shape = wtk_local_cfg_find_int_array_s(main_lc,"Rt_shape");
    wtk_local_cfg_update_cfg_str(main_lc,cfg,Rt_path,v);
    wtk_local_cfg_update_cfg_f(main_lc,cfg,aspect,v);
    wtk_local_cfg_update_cfg_f(main_lc,cfg,focal,v);
    wtk_local_cfg_update_cfg_f(main_lc,cfg,ppx,v);
    wtk_local_cfg_update_cfg_f(main_lc,cfg,ppy,v);

    return 0;
}

int qtk_stitch_run_camera_cfg_update(qtk_stitch_run_camera_cfg_t *cfg)
{
    return 0;
}