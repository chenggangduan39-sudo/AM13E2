#include "wtk_vscore_cfg.h" 

int wtk_vscore_cfg_init(wtk_vscore_cfg_t *cfg)
{
	wtk_fextra_cfg_init(&(cfg->parm));
	wtk_hmmset_cfg_init(&(cfg->hmmset));
	wtk_vrec_cfg_init(&(cfg->fa));
	wtk_vrec_cfg_init(&(cfg->loop));
	cfg->label=NULL;
	cfg->cfg=NULL;
	cfg->bin_cfg=NULL;
	return 0;
}

int wtk_vscore_cfg_clean(wtk_vscore_cfg_t *cfg)
{
	wtk_fextra_cfg_clean(&(cfg->parm));
	wtk_hmmset_cfg_clean(&(cfg->hmmset));
	wtk_vrec_cfg_clean(&(cfg->fa));
	wtk_vrec_cfg_clean(&(cfg->loop));
	if(cfg->label)
	{
		wtk_label_delete(cfg->label);
	}
	return 0;
}

int wtk_vscore_cfg_update_local(wtk_vscore_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	int ret;

	lc=wtk_local_cfg_find_lc_s(main,"parm");
	if(lc)
	{
		ret=wtk_fextra_cfg_update_local(&(cfg->parm),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"hmmset");
	if(lc)
	{
		ret=wtk_hmmset_cfg_update_local(&(cfg->hmmset),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"fa");
	if(lc)
	{
		ret=wtk_vrec_cfg_update_local(&(cfg->fa),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"loop");
	if(lc)
	{
		ret=wtk_vrec_cfg_update_local(&(cfg->loop),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_vscore_cfg_update(wtk_vscore_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_vscore_cfg_update2(cfg,&sl);
}

int wtk_vscore_cfg_update2(wtk_vscore_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	cfg->label=wtk_label_new(250007);
	ret=wtk_fextra_cfg_update(&(cfg->parm));
	if(ret!=0)
	{
		wtk_debug("update parm failed.\n");
		goto end;
	}
	ret=wtk_hmmset_cfg_update2(&(cfg->hmmset),cfg->label,sl);
	if(ret!=0)
	{
		wtk_debug("update hmmset failed\n");
		goto end;
	}
	ret=wtk_vrec_cfg_update2(&(cfg->fa),sl,cfg->label,cfg->hmmset.hmmset);
	if(ret!=0)
	{
		goto end;
	}
	ret=wtk_vrec_cfg_update2(&(cfg->loop),sl,cfg->label,cfg->hmmset.hmmset);
	if(ret!=0)
	{
		goto end;
	}
	wtk_wfst_dnn_cfg_attach_hmmset(&(cfg->fa.rec.dnn),cfg->hmmset.hmmset);
	wtk_wfst_dnn_cfg_attach_hmmset(&(cfg->loop.rec.dnn),cfg->hmmset.hmmset);
	ret=0;
end:
	//wtk_debug("ret=%d\n",ret);
	return ret;
}


wtk_vscore_cfg_t* wtk_vscore_cfg_new(char *fn)
{
	wtk_main_cfg_t *cfg;
	wtk_vscore_cfg_t *tcfg;

	cfg=wtk_main_cfg_new_type(wtk_vscore_cfg,fn);
	if(cfg)
	{
		tcfg=(wtk_vscore_cfg_t*)(cfg->cfg);
		tcfg->cfg=cfg;
		return tcfg;
	}else
	{
		return NULL;
	}
}

void wtk_vscore_cfg_delete(wtk_vscore_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->cfg);
}
