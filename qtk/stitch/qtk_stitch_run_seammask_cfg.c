#include "qtk_stitch_run_seammask_cfg.h"

int qtk_stitch_run_seammask_cfg_init(qtk_stitch_run_seammask_cfg_t *cfg)
{
    cfg->seam_mask_path = NULL;
    cfg->shape = NULL;
    return 0;
}

int qtk_stitch_run_seammask_cfg_clean(qtk_stitch_run_seammask_cfg_t *cfg)
{
    return 0;
}

int qtk_stitch_run_seammask_cfg_update_local(qtk_stitch_run_seammask_cfg_t *cfg,wtk_local_cfg_t *main_lc)
{
    wtk_string_t *v = NULL;

    cfg->shape = wtk_local_cfg_find_int_array_s(main_lc,"shape");
    wtk_local_cfg_update_cfg_str(main_lc,cfg,seam_mask_path,v);

    return 0;
}

int qtk_stitch_run_seammask_cfg_update(qtk_stitch_run_seammask_cfg_t *cfg)
{
    return 0;
}