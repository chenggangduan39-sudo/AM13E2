#include "wtk_fevad_cfg.h" 

int wtk_fevad_cfg_init(wtk_fevad_cfg_t *cfg)
{
	wtk_stft_cfg_init(&(cfg->stft));
	cfg->rate=16000;
	cfg->min_start_frame=30;
	cfg->e_thresh=40;
	cfg->f_thresh=185;
	cfg->sf_thresh=5;
	cfg->sil_time=300;
	cfg->min_sil_time=100;
	cfg->min_speech_time=50;
	cfg->e_ratio=20.0;
	cfg->e_alpha=0.7;

	cfg->fv_thresh=50;
	cfg->debug=0;
	return 0;
}

int wtk_fevad_cfg_clean(wtk_fevad_cfg_t *cfg)
{
	wtk_stft_cfg_clean(&(cfg->stft));
	return 0;
}

int wtk_fevad_cfg_update_local(wtk_fevad_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,fv_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,e_ratio,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,e_alpha,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sil_time,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_sil_time,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_speech_time,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,e_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,f_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sf_thresh,v);
	lc=wtk_local_cfg_find_lc_s(main,"stft");
	if(lc)
	{
		ret=wtk_stft_cfg_update_local(&(cfg->stft),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_fevad_cfg_update(wtk_fevad_cfg_t *cfg)
{
	int ret;

	ret=wtk_stft_cfg_update(&(cfg->stft));
	if(ret!=0){goto end;}
	cfg->min_start_frame=cfg->sil_time*cfg->rate/(cfg->stft.win*1000);
	cfg->min_sil_frame=cfg->min_sil_time*cfg->rate/(cfg->stft.win*1000);
	cfg->min_speech_frame=cfg->min_speech_time*cfg->rate/(cfg->stft.win*1000);
	ret=0;
end:
	return ret;
}
