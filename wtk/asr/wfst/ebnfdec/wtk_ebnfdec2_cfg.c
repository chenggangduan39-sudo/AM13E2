#include "wtk/asr/wfst/wtk_wfstdec.h"
#include "wtk_ebnfdec2_cfg.h" 


int wtk_ebnfdec2_cfg_init(wtk_ebnfdec2_cfg_t *cfg)
{
	cfg->egram_bin_cfg=NULL;
	cfg->egram_cfg=NULL;
	cfg->dec_cfg=NULL;
	cfg->dec_bin_cfg=NULL;
	cfg->dec_fn=NULL;
	cfg->compile_fn=NULL;
	cfg->ebnf_fn=NULL;
	cfg->use_bin=1;
	cfg->usr_bin=NULL;
	return 0;
}

int wtk_ebnfdec2_cfg_clean(wtk_ebnfdec2_cfg_t *cfg)
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
	return 0;
}

int wtk_ebnfdec2_cfg_update_local(wtk_ebnfdec2_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_b(lc,cfg,use_bin,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,usr_bin,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,dec_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,compile_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,ebnf_fn,v);
	return 0;
}

int wtk_ebnfdec2_cfg_update(wtk_ebnfdec2_cfg_t *cfg)
{
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
	if(cfg->ebnf_fn)
	{
		wtk_egram_t *egram;
		wtk_wfstdec_cfg_t *dec_cfg;

		if(cfg->use_bin)
		{
			egram=wtk_egram_new2(cfg->egram_bin_cfg);
			dec_cfg=cfg->dec_bin_cfg;
		}else
		{
			egram=wtk_egram_new((wtk_egram_cfg_t*)(cfg->egram_cfg->cfg),NULL);
			dec_cfg=(wtk_wfstdec_cfg_t*)(cfg->dec_cfg->cfg);
		}
		wtk_egram_ebnf2fst2(egram,cfg->ebnf_fn);
		//wtk_egram_write_txt(egram,"x.out","x.fsm");
		wtk_egram_write(egram,cfg->usr_bin);
		wtk_wfstdec_cfg_set_ebnf_net(dec_cfg,cfg->usr_bin);
		wtk_egram_delete(egram);
	}
	return 0;
}
