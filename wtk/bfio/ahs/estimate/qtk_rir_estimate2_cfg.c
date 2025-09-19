#include "qtk_rir_estimate2_cfg.h"

int qtk_rir_estimate2_cfg_init(qtk_rir_estimate2_cfg_t *cfg)
{
	cfg->distance = 0.0;
	cfg->max_val = 429.020265;
	cfg->rt60 = 0.6;
	cfg->rir_duration = 10;
	cfg->lookahead = 50.0;
        // cfg->st = 1.0;//s
        cfg->st = 0.0; // s
        cfg->rate = 16000;

	cfg->channel=2;
    cfg->max_freq = 23000;
    cfg->min_freq = 19000;
    cfg->code_tms = 0.2;
    cfg->win_tms = 0.02;
    cfg->energy_shift = 1.0;
    cfg->use_hanning = 0;
	cfg->rir_lookahead_thresh = 0.7;
	cfg->rir_lookahead_len = 1000;
    return 0;
}
int qtk_rir_estimate2_cfg_clean(qtk_rir_estimate2_cfg_t *cfg)
{
    return 0;
}
int qtk_rir_estimate2_cfg_update_local(qtk_rir_estimate2_cfg_t *cfg, wtk_local_cfg_t *m)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret = 0;

	lc=m;
	wtk_local_cfg_update_cfg_f(lc,cfg,distance,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_val,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,rt60,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,lookahead,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,st,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,rir_duration,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,hop_size,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,max_freq,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,min_freq,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,code_tms,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,win_tms,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,energy_shift,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,rir_lookahead_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,rir_lookahead_len,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_hanning,v);
    return ret;
}

int qtk_rir_estimate2_cfg_update(qtk_rir_estimate2_cfg_t *cfg)
{
	int ret;
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;

    ret = qtk_rir_estimate2_cfg_update2(cfg,&sl);
    // if(ret != 0){
    //     goto end;
    // }
    cfg->code_len = floor(cfg->code_tms * cfg->rate);
    cfg->win_len = floor(cfg->win_tms * cfg->rate);

	return ret;
}
int qtk_rir_estimate2_cfg_update2(qtk_rir_estimate2_cfg_t *cfg, wtk_source_loader_t *sl)
{
	int ret = 0;

	cfg->code_len = floor(cfg->code_tms * cfg->rate);
	cfg->win_len = floor(cfg->win_tms * cfg->rate);
	return ret;
}

qtk_rir_estimate2_cfg_t* qtk_rir_estimate2_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_rir_estimate2_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(qtk_rir_estimate2_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(qtk_rir_estimate2_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void qtk_rir_estimate2_cfg_delete(qtk_rir_estimate2_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_rir_estimate2_cfg_t* qtk_rir_estimate2_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	qtk_rir_estimate2_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(qtk_rir_estimate2_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(qtk_rir_estimate2_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void qtk_rir_estimate2_cfg_delete_bin(qtk_rir_estimate2_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
