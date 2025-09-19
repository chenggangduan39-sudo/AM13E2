#include "wtk_ebnfdec_cfg.h" 

int wtk_ebnfdec_cfg_init(wtk_ebnfdec_cfg_t *cfg)
{
	cfg->egram_bin_cfg=NULL;
	cfg->egram_cfg=NULL;
	cfg->dec_cfg=NULL;
	cfg->dec_bin_cfg=NULL;
	cfg->vad_cfg=NULL;
	cfg->dec_fn=NULL;
	cfg->compile_fn=NULL;
	cfg->vad_fn=NULL;
	cfg->use_bin=1;
	return 0;
}

int wtk_ebnfdec_cfg_clean(wtk_ebnfdec_cfg_t *cfg)
{
	if(!cfg->use_bin)
	{
		if(cfg->egram_cfg)
		{
			wtk_main_cfg_delete(cfg->egram_cfg);
		}
		if(cfg->dec_cfg)
		{
			wtk_wfstdec_cfg_delete(cfg->dec_cfg);
		}
	}else
	{
		if(cfg->egram_bin_cfg)
		{
			wtk_mbin_cfg_delete(cfg->egram_bin_cfg);
		}
		if(cfg->dec_bin_cfg)
		{
			wtk_wfstdec_cfg_delete_bin(cfg->dec_bin_cfg);
		}
	}
	if(cfg->vad_cfg)
	{
		wtk_vad_cfg_delete_bin(cfg->vad_cfg);
	}
	return 0;
}

int wtk_ebnfdec_cfg_update_local(wtk_ebnfdec_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_b(lc,cfg,use_bin,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,dec_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,compile_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,vad_fn,v);
	return 0;
}

int wtk_ebnfdec_cfg_update(wtk_ebnfdec_cfg_t *cfg)
{
	if(cfg->vad_fn)
	{
		cfg->vad_cfg=wtk_vad_cfg_new_bin2(cfg->vad_fn);
	}
	if(cfg->use_bin)
	{
		if(cfg->compile_fn)
		{
			cfg->egram_bin_cfg=wtk_mbin_cfg_new_type(wtk_egram_cfg,cfg->compile_fn,"./egram.cfg.r");
		}
		if(cfg->dec_fn)
		{
			cfg->dec_bin_cfg=wtk_wfstdec_cfg_new_bin(cfg->dec_fn);
		}
	}else
	{
		if(cfg->compile_fn)
		{
			cfg->egram_cfg=wtk_main_cfg_new_type(wtk_egram_cfg,cfg->compile_fn);
		}
		if(cfg->dec_fn)
		{
			cfg->dec_cfg=wtk_wfstdec_cfg_new(cfg->dec_fn);
		}
	}
	return 0;
}
