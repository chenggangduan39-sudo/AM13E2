#include "qtk_stitch_humanseg_cfg.h"

int qtk_stitch_humanseg_cfg_init(qtk_stitch_humanseg_cfg_t *cfg)
{
    cfg->in_h = 192;
    cfg->in_w = 192;
    cfg->fn = NULL;
#ifndef IPU_DEC
    qtk_nnrt_cfg_init(&cfg->nnrt);
#endif
    return 0;
}

int qtk_stitch_humanseg_cfg_clean(qtk_stitch_humanseg_cfg_t *cfg)
{
#ifndef IPU_DEC
    qtk_nnrt_cfg_clean(&cfg->nnrt);
#endif
    return 0;
}

int qtk_stitch_humanseg_cfg_update_local(qtk_stitch_humanseg_cfg_t *cfg,wtk_local_cfg_t *main_lc)
{
    wtk_string_t *v = NULL;
    wtk_local_cfg_t  *lc = NULL;
    wtk_local_cfg_update_cfg_i(main_lc,cfg,in_h,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,in_w,v);
    wtk_local_cfg_update_cfg_str(main_lc,cfg,fn,v);
#ifndef IPU_DEC
    lc = wtk_local_cfg_find_lc_s(main_lc,"nnrt");
    if(lc){
        qtk_nnrt_cfg_update_local(&cfg->nnrt,lc);
    }
#endif
    return 0;
}

int qtk_stitch_humanseg_cfg_update(qtk_stitch_humanseg_cfg_t *cfg)
{
#ifndef IPU_DEC
    qtk_nnrt_cfg_update(&cfg->nnrt);
#endif
    return 0;
}