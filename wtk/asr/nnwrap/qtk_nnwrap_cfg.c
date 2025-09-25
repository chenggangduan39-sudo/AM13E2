#include "qtk_nnwrap_cfg.h"
int qtk_nnwrap_cfg_init(qtk_nnwrap_cfg_t *cfg)
{
	int ret;
	cfg->cfile = 0;
	cfg->rbin = 0;
	cfg->hook = 0;
	cfg->encoder_fn = 0;
	cfg->age_fn = 0;
	cfg->gender_fn = 0;
	cfg->pool_fn = 0;
	cfg->use_vad = 0;
    wtk_kxparm_cfg_init(&(cfg->kxparm));
    wtk_kvad_cfg_init(&(cfg->vad));
	ret=0;
	return ret;
}

int qtk_nnwrap_cfg_clean(qtk_nnwrap_cfg_t *cfg)
{
    wtk_kxparm_cfg_clean(&(cfg->kxparm));
    wtk_kvad_cfg_clean(&(cfg->vad));

    if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
	if(cfg->cfile)
	{
		wtk_cfg_file_delete(cfg->cfile);
	}
	return 0;
}

int qtk_nnwrap_cfg_update_local(qtk_nnwrap_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret=0;

	lc=main;
    wtk_local_cfg_update_cfg_str(lc,cfg,encoder_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,age_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,gender_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,pool_fn,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_vad,v);

    lc=wtk_local_cfg_find_lc_s(main,"kxparm");
    if(lc)
    {
        ret=wtk_kxparm_cfg_update_local(&(cfg->kxparm),lc);
    }

    lc=wtk_local_cfg_find_lc_s(main,"kvad");
    if(lc)
    {
        ret=wtk_kvad_cfg_update_local(&(cfg->vad),lc);
    }

	ret=0;
	return ret;
}

int qtk_nnwrap_cfg_update(qtk_nnwrap_cfg_t *cfg)
{
	cfg->sl.hook=0;
	cfg->sl.vf=wtk_source_load_file_v;
	return qtk_nnwrap_cfg_update2(cfg,&cfg->sl);
}

int qtk_nnwrap_cfg_update2(qtk_nnwrap_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	cfg->sl = *sl;
    ret=wtk_kxparm_cfg_update2(&(cfg->kxparm),sl);
	if(ret!=0){goto end;}

	if(cfg->use_vad)
	{
		ret=wtk_kvad_cfg_update2(&(cfg->vad),sl);
		if(ret!=0){goto end;}
	}

end:
	return ret;
}

qtk_nnwrap_cfg_t* qtk_nnwrap_cfg_new_bin(char *bin_fn,char *cfg_fn)
{
	wtk_mbin_cfg_t *cfg;
	qtk_nnwrap_cfg_t *vc;

	//wtk_debug("%s/%s\n",bin_fn,cfg_fn);
	cfg=wtk_mbin_cfg_new_type(qtk_nnwrap_cfg,bin_fn,cfg_fn);
	vc=(qtk_nnwrap_cfg_t*)(cfg->cfg);
	vc->hook=cfg;
	return vc;
}

qtk_nnwrap_cfg_t* qtk_nnwrap_cfg_new_bin2(char *bin_fn)
{
	return qtk_nnwrap_cfg_new_bin(bin_fn,"./kws.cfg");
}

qtk_nnwrap_cfg_t* qtk_nnwrap_cfg_new_bin3(char *bin_fn,unsigned int seek_pos)
{
	qtk_nnwrap_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	char *cfg_fn="./kws.cfg";
	int ret;

	cfg=(qtk_nnwrap_cfg_t*)wtk_malloc(sizeof(qtk_nnwrap_cfg_t));
	qtk_nnwrap_cfg_init(cfg);
	cfg->rbin=wtk_rbin2_new();
	ret=wtk_rbin2_read2(cfg->rbin,bin_fn,seek_pos);
	if(ret!=0){
		wtk_debug("read failed\n");
		goto end;
	}
	item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	if(!item)
	{
		wtk_debug("%s not found %s\n",cfg_fn,bin_fn);
		ret=-1;goto end;
	}
	cfg->cfile=wtk_cfg_file_new();
	//wtk_debug("f=%p\n",cfg->rbin->f);
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	ret=wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	if(ret!=0){goto end;}
	ret=qtk_nnwrap_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}
	cfg->sl.hook=cfg->rbin;
	cfg->sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=qtk_nnwrap_cfg_update2(cfg,&(cfg->sl));
	if(ret!=0){goto end;}
end:
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		qtk_nnwrap_cfg_delete_bin2(cfg);
		cfg=NULL;
	}
	return cfg;
}

int qtk_nnwrap_cfg_delete_bin(qtk_nnwrap_cfg_t *cfg)
{
	wtk_mbin_cfg_delete((wtk_mbin_cfg_t*)(cfg->hook));
	return 0;
}

int qtk_nnwrap_cfg_delete_bin2(qtk_nnwrap_cfg_t *cfg)
{
	qtk_nnwrap_cfg_clean(cfg);
	wtk_free(cfg);
	return 0;
}

qtk_nnwrap_cfg_t* qtk_nnwrap_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_nnwrap_cfg_t *vc;

	main_cfg=wtk_main_cfg_new_type(qtk_nnwrap_cfg,cfg_fn);
	vc=(qtk_nnwrap_cfg_t*)(main_cfg->cfg);
	vc->hook=main_cfg;
	return vc;
}

void qtk_nnwrap_cfg_delete(qtk_nnwrap_cfg_t *cfg)
{
	wtk_main_cfg_delete((wtk_main_cfg_t*)(cfg->hook));
}
