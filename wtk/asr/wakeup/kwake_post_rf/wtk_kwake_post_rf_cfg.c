#include "wtk_kwake_post_rf_cfg.h"

int wtk_kwake_post_rf_cfg_init(wtk_kwake_post_rf_cfg_t *cfg)
{
    cfg->rf_win_left_pad = 0;
    cfg->rf_win_right_pad = 0;
    cfg->state_array = NULL;
    cfg->state_cnt = 0;
    cfg->rf_thresh=0.5;
    cfg->bkg_sil_ratio=9;
    return 0;
}

int wtk_kwake_post_rf_cfg_clean(wtk_kwake_post_rf_cfg_t *cfg)
{
	if(cfg->state_array)
	{
		wtk_free(cfg->state_array);
	}
    return 0;
}

int wtk_kwake_post_rf_cfg_update(wtk_kwake_post_rf_cfg_t *cfg)
{
    return 0;
}

int wtk_kwake_post_rf_cfg_update_local(wtk_kwake_post_rf_cfg_t *cfg, wtk_local_cfg_t *lc)
{
    wtk_string_t *v;
    wtk_array_t *a;
    int i;

    wtk_local_cfg_update_cfg_f(lc,cfg,rf_thresh,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,rf_win_left_pad,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,rf_win_right_pad,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,bkg_sil_ratio,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,win,v);
    a=wtk_local_cfg_find_array_s(lc,"node_state");
	if(a)
	{
		cfg->state_array=(int*)wtk_calloc(a->nslot,sizeof(int));
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)(a->slot))[i];
			cfg->state_array[i]=wtk_str_atoi(v->data,v->len);
			//wtk_debug("node =%d\n",cfg->state_array[i]);
		}
		cfg->state_cnt = a->nslot;
	}
    return 0;
}