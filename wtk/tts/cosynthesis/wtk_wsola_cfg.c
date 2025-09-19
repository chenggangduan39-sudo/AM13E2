#include "wtk_wsola_cfg.h"

int wtk_wsola_cfg_init_win(wtk_wsola_cfg_t *cfg)
{
    //int ret;
    double a;
    int i,len;
    len = cfg->win_sz * 2;
    a= M_2PI/(len-1);
    cfg->window=(float*)wtk_calloc(len,sizeof(float));
    for(i=0;i<len;++i)
    {
        cfg->window[i]=0.5*(1-cos(a*i)); //hanning
    }
    return 0;
}

int wtk_wsola_cfg_init(wtk_wsola_cfg_t *cfg)
{
    cfg->win_sz = 200;
    wtk_wsola_cfg_init_win(cfg);
    return 0;
}

int wtk_wsola_cfg_update_local(wtk_wsola_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v;
    wtk_local_cfg_update_cfg_i(lc, cfg, win_sz, v);
    return 0;
}

int wtk_wsola_cfg_clean(wtk_wsola_cfg_t *cfg)
{
	if (cfg->window)
		wtk_free(cfg->window);
    return 0;
}

int wtk_wsola_cfg_update(wtk_wsola_cfg_t *cfg)
{
    return 0;
}
int wtk_wsola_cfg_update2(wtk_wsola_cfg_t *cfg,wtk_source_loader_t *sl)
{
    return 0;
}
