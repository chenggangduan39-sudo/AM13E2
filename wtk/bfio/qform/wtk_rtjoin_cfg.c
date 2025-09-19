#include "wtk_rtjoin_cfg.h"

int wtk_rtjoin_cfg_init(wtk_rtjoin_cfg_t *cfg)
{
	cfg->channel=4;
    cfg->wins=320;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->rate=16000;

	wtk_equalizer_cfg_init(&(cfg->eq));
	cfg->use_eq=0;

	cfg->M=4;
	cfg->M2=2;
	cfg->power_alpha=0.1;

	cfg->mufb=0.1;

	cfg->clip_s=0;
	cfg->clip_e=8000;

	cfg->power_alpha2=0.1;
	cfg->power_alpha3=0.9;
	cfg->use_choicech=1;

	cfg->power_thresh=50;

	cfg->chioce_thresh=1e4;
	cfg->change_frame_len=100;
	cfg->change_frame_num=7;
	cfg->change_frame_num2=4;
	cfg->change_frame_delay=100;
	cfg->init_change_frame=100;
	cfg->change_thresh=1.2;
	cfg->change_thresh2=1.8;
	cfg->mix_scale=1.2;
	cfg->use_control_bs=0;

	cfg->csd_thresh=1e13;
	cfg->csd_in_cnt=2;
	cfg->csd_out_cnt=80;
	cfg->use_csd=0;

	cfg->mean_nchenr_thresh=-1;
	cfg->mean_nchenr_cnt=40;
	cfg->sil_power_alpha2=0.1;
	cfg->sound_power_alpha2=0.9;
	cfg->nchenr_cnt=5;
	cfg->change_delay=10;
	cfg->nlms_change_init=40;
	cfg->nlms_align_thresh=10;
	cfg->nlms_align_thresh2=1;
	cfg->nlms_change_cnt=4;
	cfg->nlms_mix_thresh=500;

	return 0;
}

int wtk_rtjoin_cfg_clean(wtk_rtjoin_cfg_t *cfg)
{
	wtk_equalizer_cfg_clean(&(cfg->eq));

	return 0;
}

int wtk_rtjoin_cfg_update_local(wtk_rtjoin_cfg_t *cfg,wtk_local_cfg_t *m)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,M,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,M2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,power_alpha,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,mufb,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,clip_e,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_eq,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_choicech,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,power_alpha2,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,power_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,chioce_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,change_frame_len,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,change_frame_num,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,change_frame_num2,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,change_frame_delay,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,init_change_frame,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,change_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,change_thresh2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,mix_scale,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_control_bs,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,csd_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,csd_in_cnt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,csd_out_cnt,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_csd,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,mean_nchenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,mean_nchenr_cnt,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sil_power_alpha2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sound_power_alpha2,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nchenr_cnt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,change_delay,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nlms_change_init,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,nlms_align_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,nlms_align_thresh2,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nlms_change_cnt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nlms_mix_thresh,v);

	lc=wtk_local_cfg_find_lc_s(m,"eq");
	if(lc)
	{
        ret=wtk_equalizer_cfg_update_local(&(cfg->eq),lc);
        if(ret!=0){goto end;}
    }

	ret=0;
end:
	return ret;
}

int wtk_rtjoin_cfg_update(wtk_rtjoin_cfg_t *cfg)
{
	int ret;

	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_s=max(1,cfg->clip_s);
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=min(cfg->wins/2,cfg->clip_e);

	ret=wtk_equalizer_cfg_update(&(cfg->eq));
	if(ret!=0){goto end;}

	ret=0;
end:
	return ret;
}

int wtk_rtjoin_cfg_update2(wtk_rtjoin_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return wtk_rtjoin_cfg_update(cfg);
}

wtk_rtjoin_cfg_t* wtk_rtjoin_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_rtjoin_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_rtjoin_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_rtjoin_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_rtjoin_cfg_delete(wtk_rtjoin_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_rtjoin_cfg_t* wtk_rtjoin_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_rtjoin_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_rtjoin_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_rtjoin_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_rtjoin_cfg_delete_bin(wtk_rtjoin_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

void wtk_rtjoin_cfg_set_channel(wtk_rtjoin_cfg_t *cfg, int channel)
{
	cfg->channel=channel;
}