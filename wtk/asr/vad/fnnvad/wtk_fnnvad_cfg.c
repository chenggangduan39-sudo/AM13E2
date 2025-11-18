#include "wtk_fnnvad_cfg.h"

int wtk_fnnvad_cfg_init(wtk_fnnvad_cfg_t *cfg)
{
	wtk_fextra_cfg_init(&(cfg->parm));
	cfg->cache=100;
	cfg->win=2;
	cfg->siltrap=8;
	cfg->speechtrap=8;
	cfg->speech_energe_thresh=0;
	cfg->echo_speech_thresh=-0.8;
	cfg->use_speech_end_detect=0;
	cfg->sil_thresh=-0.693;
	cfg->high_speech_thresh_rate=3;
	cfg->high_speech_min_frame=3;

	cfg->detect_sil_go_high_thresh_rate=10.0;
	cfg->detect_high_go_low_thresh_rate=0.01;
	cfg->detect_min_high_frame=5;
	cfg->detect_min_low_frame=15;
	return 0;
}

int wtk_fnnvad_cfg_clean(wtk_fnnvad_cfg_t *cfg)
{
	//wtk_debug("dvcfg delete 2 fo=%d\n",cfg->use_f0);
	wtk_fextra_cfg_clean(&(cfg->parm));
	return 0;
}

int wtk_fnnvad_cfg_update_local(wtk_fnnvad_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_b(lc,cfg,use_speech_end_detect,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,cache,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,win,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,siltrap,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,speechtrap,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,speech_energe_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,echo_speech_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,high_speech_thresh_rate,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,high_speech_min_frame,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,detect_sil_go_high_thresh_rate,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,detect_min_high_frame,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,detect_high_go_low_thresh_rate,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,detect_min_low_frame,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,sil_thresh,v);
	lc=wtk_local_cfg_find_lc_s(main,"fextra");
	if(!lc)
	{
		lc=wtk_local_cfg_find_lc_s(main,"parm");
	}
	if(lc)
	{
		ret=wtk_fextra_cfg_update_local(&(cfg->parm),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:

	//wtk_local_cfg_print(main);
	//exit(0);
	return ret;
}

int wtk_fnnvad_cfg_update(wtk_fnnvad_cfg_t *cfg)
{
	int ret;

	ret=wtk_fextra_cfg_update(&(cfg->parm));
	if(ret!=0){goto end;}
	if(cfg->use_speech_end_detect)
	{
		cfg->parm.use_e=1;
	}
	ret=0;
end:
	return ret;
}

int wtk_fnnvad_cfg_update2(wtk_fnnvad_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	ret=wtk_fextra_cfg_update2(&(cfg->parm),sl);
	if(ret!=0){goto end;}
	if(cfg->use_speech_end_detect)
	{
		cfg->parm.use_e=1;
	}
	ret=0;
end:
	return ret;
}
