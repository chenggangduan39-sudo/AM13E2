/*
 * wtk_mtts_cfg.c
 *
 *  Created on: Dec 17, 2016
 *      Author: dm
 */

#include "wtk_mtts_cfg.h"

int wtk_mtts_cfg_init(wtk_mtts_cfg_t *cfg)
{
	cfg->cn=NULL;
	cfg->cncfg=NULL;
	cfg->use_cnbin=0;
	cfg->swav=NULL;
	cfg->svol=1.0f;
	cfg->sshift=0.0f;
	cfg->r_shift=1.0f;
	cfg->mix_sil_time=50*16;
	return 0;
}

int wtk_mtts_cfg_clean(wtk_mtts_cfg_t *cfg)
{
	return 0;
}

int wtk_mtts_cfg_update_local(wtk_mtts_cfg_t *cfg, wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	ret=0;
	lc=main;
	wtk_local_cfg_update_cfg_f(lc,cfg,svol,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sshift,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_cnbin,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,mix_sil_time,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,cn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,swav,v);

	return ret;
}

int wtk_mtts_cfg_update(wtk_mtts_cfg_t *cfg)
{
	int ret=-1;

	if (cfg->use_cnbin){
		cfg->cncfg=wtk_tts_cfg_new_bin(cfg->cn);
	}else{
		cfg->cncfg=wtk_tts_cfg_new(cfg->cn);
	}
	if (cfg->cncfg)ret=0;

	return ret;
}

wtk_mtts_cfg_t* wtk_mtts_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_mtts_cfg_t *mtts=NULL;

	main_cfg=wtk_main_cfg_new_type(wtk_mtts_cfg,cfg_fn);
	if(!main_cfg){goto end;}
	mtts=(wtk_mtts_cfg_t*)(main_cfg->cfg);
	if (mtts==NULL){
		wtk_main_cfg_delete(main_cfg);
		goto end;
	}
	mtts->main_cfg=main_cfg;
end:

	return  mtts;
}

void wtk_mtts_cfg_delete(wtk_mtts_cfg_t *cfg)
{
	if (cfg->cncfg){
		if(cfg->use_cnbin)
		{
			wtk_tts_cfg_delete_bin(cfg->cncfg);
		}else
		{
			wtk_tts_cfg_delete(cfg->cncfg);
		}
	}
	if (cfg->main_cfg)
		wtk_main_cfg_delete(cfg->main_cfg);
}


