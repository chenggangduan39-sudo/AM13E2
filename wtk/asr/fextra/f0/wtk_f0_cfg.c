#include "wtk_f0_cfg.h"

int wtk_f0_cfg_init(wtk_f0_cfg_t *cfg)
{
	wtk_fpost_cfg_init(&(cfg->post));
	wtk_favg_cfg_init(&(cfg->avg));
	cfg->use_post=0;
	cfg->use_avg=0;
	cfg->frame_dur=0.01;
	return 0;
}

int wtk_f0_cfg_clean(wtk_f0_cfg_t *cfg)
{
	if(cfg->use_post)
	{
		wtk_fpost_cfg_clean(&(cfg->post));
	}
	if(cfg->use_avg)
	{
		wtk_favg_cfg_clean(&(cfg->avg));
	}
	return 0;
}

int wtk_f0_cfg_update_local(wtk_f0_cfg_t *cfg,wtk_local_cfg_t* main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc=main;
	int ret=0;

	//wtk_local_cfg_print(lc);
	wtk_local_cfg_update_cfg_f(lc,cfg,frame_dur,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_post,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_avg,v);
	if(cfg->use_post)
	{
		lc=wtk_local_cfg_find_lc_s(main,"post");
		if(lc)
		{
			ret=wtk_fpost_cfg_update_local(&(cfg->post),lc);
			if(ret!=0){goto end;}
		}
	}
	if(cfg->use_avg)
	{
		lc=wtk_local_cfg_find_lc_s(main,"avg");
		if(lc)
		{
			ret=wtk_favg_cfg_update_local(&(cfg->avg),lc);
			if(ret!=0){goto end;}
		}
	}
end:
	return ret;
}

int wtk_f0_cfg_update(wtk_f0_cfg_t *cfg)
{
	int ret=0;

	if(cfg->use_post)
	{
		ret=wtk_fpost_cfg_update(&(cfg->post));
		if(ret!=0){goto end;}
	}
	if(cfg->use_avg)
	{
		ret=wtk_favg_cfg_update(&(cfg->avg));
		if(ret!=0){goto end;}
	}
end:
	return ret;
}

int wtk_f0_cfg_update_set_example(wtk_f0_cfg_t *cfg)
{
	return 0;
}

void wtk_f0_cfg_print(wtk_f0_cfg_t *cfg)
{
	printf("------------ F0 -----------------\n");
}

