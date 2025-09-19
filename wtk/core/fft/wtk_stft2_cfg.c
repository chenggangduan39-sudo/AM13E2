#include "wtk_stft2_cfg.h" 

int wtk_stft2_cfg_init(wtk_stft2_cfg_t *cfg)
{
	cfg->channel=4;
	cfg->win=512;
	cfg->overlap=0.75;
	cfg->use_sine=0;
	cfg->use_hamming=0;
	cfg->use_hann=0;
	cfg->use_conj_window=0;

	cfg->use_synthesis_window=0;
	
	cfg->use_fftscale=1;

	cfg->step=0;

	return 0;
}

int wtk_stft2_cfg_clean(wtk_stft2_cfg_t *cfg)
{
	return 0;
}

int wtk_stft2_cfg_update_local(wtk_stft2_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_b(lc,cfg,use_synthesis_window,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_fftscale,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_sine,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_hamming,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_hann,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_conj_window,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,win,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,overlap,v);
	return 0;
}

int wtk_stft2_cfg_update(wtk_stft2_cfg_t *cfg)
{
	cfg->step=cfg->win*(1-cfg->overlap);

	return 0;
}
