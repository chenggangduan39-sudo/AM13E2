#include "wtk_drc_cfg.h"

int wtk_drc_cfg_init(wtk_drc_cfg_t *cfg)
{

    cfg->rate = 16000;
    cfg->pregain = 0.0f;
    cfg->threshold = -24.0f;
    cfg->knee = 30.0f;
    cfg->ratio = 12.0f;
    cfg->attack = 0.003f;
    cfg->release = 0.25f;
    
    cfg->predelay = 0.006f;
    cfg->releasezone1 = 0.09f;
    cfg->releasezone2 = 0.16f;
    cfg->releasezone3 = 0.42f;
    cfg->releasezone4 = 0.98f;
    cfg->postgain = 0.0f;
    cfg->wet = 1.0f;

    cfg->size = 128;
    cfg->numchannels = 1;

	return 0;
}

int wtk_drc_cfg_clean(wtk_drc_cfg_t *cfg)
{
	return 0;
}

int wtk_drc_cfg_update_local(wtk_drc_cfg_t *cfg,wtk_local_cfg_t *m)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;
	wtk_array_t *a;
	int i;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,pregain,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,threshold,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,knee,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,ratio,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,attack,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,release,v);
    
    wtk_local_cfg_update_cfg_f(lc,cfg,predelay,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,releasezone1,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,releasezone2,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,releasezone3,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,releasezone4,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,postgain,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,wet,v);

    wtk_local_cfg_update_cfg_i(lc,cfg,size,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,numchannels,v);

	ret=0;
end:
	return ret;
}

int wtk_drc_cfg_update(wtk_drc_cfg_t *cfg)
{
	int ret;

	ret=0;
end:
	return ret;
}

int wtk_drc_cfg_update2(wtk_drc_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	ret=0;
end:
	return ret;
}

wtk_drc_cfg_t* wtk_drc_cfg_new(char *fn)
{
    wtk_main_cfg_t *main_cfg;
    wtk_drc_cfg_t *cfg;

    main_cfg=wtk_main_cfg_new_type(wtk_drc_cfg,fn);
    if(!main_cfg)
    {
        return NULL;
    }
    cfg=(wtk_drc_cfg_t*)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void wtk_drc_cfg_delete(wtk_drc_cfg_t *cfg)
{
    wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_drc_cfg_t* wtk_drc_cfg_new_bin(char *fn)
{
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_drc_cfg_t *cfg;

    mbin_cfg=wtk_mbin_cfg_new_type(wtk_drc_cfg,fn,"./cfg");
    if(!mbin_cfg)
    {
        return NULL;
    }
    cfg=(wtk_drc_cfg_t*)mbin_cfg->cfg;
    cfg->mbin_cfg=mbin_cfg;
    return cfg;
}

void wtk_drc_cfg_delete_bin(wtk_drc_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}