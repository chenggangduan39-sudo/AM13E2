#include "qtk_wake_e2e_cfg.h"

int qtk_wake_e2e_cfg_init(qtk_wake_e2e_cfg_t *cfg)
{
	int ret;

	cfg->cfile=0;
	cfg->rbin=0;
	cfg->hook=0;
        ret = 0;
        return ret;
}

int qtk_wake_e2e_cfg_clean(qtk_wake_e2e_cfg_t *cfg)
{
	if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
	if(cfg->cfile)
	{
		wtk_cfg_file_delete(cfg->cfile);
	}
	return 0;
}

int qtk_wake_e2e_cfg_update_local(qtk_wake_e2e_cfg_t *cfg,wtk_local_cfg_t *main)
{
        return 0;
}

int qtk_wake_e2e_cfg_update(qtk_wake_e2e_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return qtk_wake_e2e_cfg_update2(cfg,&sl);
}

int qtk_wake_e2e_cfg_update2(qtk_wake_e2e_cfg_t *cfg,wtk_source_loader_t *sl)
{
        return 0;
}

qtk_wake_e2e_cfg_t* qtk_wake_e2e_cfg_new_bin(char *bin_fn,char *cfg_fn)
{
	wtk_mbin_cfg_t *cfg;
	qtk_wake_e2e_cfg_t *vc;

	//wtk_debug("%s/%s\n",bin_fn,cfg_fn);
	cfg=wtk_mbin_cfg_new_type(qtk_wake_e2e_cfg,bin_fn,cfg_fn);
	vc=(qtk_wake_e2e_cfg_t*)(cfg->cfg);
	vc->hook=cfg;
	return vc;
}

qtk_wake_e2e_cfg_t* qtk_wake_e2e_cfg_new_bin2(char *bin_fn)
{
	return qtk_wake_e2e_cfg_new_bin(bin_fn,"./kws.cfg");
}

qtk_wake_e2e_cfg_t* qtk_wake_e2e_cfg_new_bin3(char *bin_fn,unsigned int seek_pos)
{
	qtk_wake_e2e_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./kws.cfg";
	int ret;

	cfg=(qtk_wake_e2e_cfg_t*)wtk_malloc(sizeof(qtk_wake_e2e_cfg_t));
	qtk_wake_e2e_cfg_init(cfg);
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
	ret=qtk_wake_e2e_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=qtk_wake_e2e_cfg_update2(cfg,&sl);
	if(ret!=0){goto end;}
end:
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		qtk_wake_e2e_cfg_delete_bin2(cfg);
		cfg=NULL;
	}
	return cfg;
}

int qtk_wake_e2e_cfg_delete_bin(qtk_wake_e2e_cfg_t *cfg)
{
	wtk_mbin_cfg_delete((wtk_mbin_cfg_t*)(cfg->hook));
	return 0;
}

int qtk_wake_e2e_cfg_delete_bin2(qtk_wake_e2e_cfg_t *cfg)
{
	qtk_wake_e2e_cfg_clean(cfg);
	wtk_free(cfg);
	return 0;
}

qtk_wake_e2e_cfg_t* qtk_wake_e2e_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_wake_e2e_cfg_t *vc;

	main_cfg=wtk_main_cfg_new_type(qtk_wake_e2e_cfg,cfg_fn);
	vc=(qtk_wake_e2e_cfg_t*)(main_cfg->cfg);
	vc->hook=main_cfg;
	return vc;
}

void qtk_wake_e2e_cfg_delete(qtk_wake_e2e_cfg_t *cfg)
{
	wtk_main_cfg_delete((wtk_main_cfg_t*)(cfg->hook));
}
