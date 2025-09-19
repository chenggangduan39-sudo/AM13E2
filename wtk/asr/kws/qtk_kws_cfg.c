#include "qtk_kws_cfg.h"

int qtk_kws_cfg_init(qtk_kws_cfg_t *cfg)
{
	int ret;
	cfg->cfile = 0;
	cfg->rbin = 0;
	cfg->hook = 0;
	cfg->wake_fn = 0;
	cfg->e2e_fn = 0;
	cfg->svprint_fn = 0;
	cfg->use_vad = 0;
	cfg->use_vad2 = 0;
	cfg->use_img = 0;
	cfg->use_svprint = 0;
	cfg->use_wake1 = 0;
	cfg->use_wake2 = 0;
	cfg->use_window_pad = 0;
	cfg->section_secs = -1;
	cfg->use_sliding_window = 0;
	wtk_kxparm_cfg_init(&(cfg->kxparm));
	wtk_kwake_cfg_init(&(cfg->kwake));
	wtk_svprint_cfg_init(&(cfg->svprint));
	wtk_kvad_cfg_init(&(cfg->vad));
	wtk_vad_cfg_init(&(cfg->vad2));
	qtk_img_cfg_init(&(cfg->img));
	ret=0;
	return ret;
}

int qtk_kws_cfg_clean(qtk_kws_cfg_t *cfg)
{
	wtk_kxparm_cfg_clean(&(cfg->kxparm));
	wtk_kwake_cfg_clean(&(cfg->kwake));
	wtk_svprint_cfg_clean(&(cfg->svprint));
	wtk_kvad_cfg_clean(&(cfg->vad));
	qtk_img_cfg_clean(&(cfg->img));

	if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
	if(cfg->cfile)
	{
		wtk_cfg_file_delete(cfg->cfile);
	}

    if(cfg->use_vad2)
    {
        wtk_vad_cfg_clean(&(cfg->vad2));
    }

	return 0;
}

int qtk_kws_cfg_update_local(qtk_kws_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret=0;

	lc=main;
    wtk_local_cfg_update_cfg_str(lc,cfg,wake_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,e2e_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,svprint_fn,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_wake1,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_wake2,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_svprint,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_vad,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_vad2,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_img,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_window_pad,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,section_secs,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_sliding_window,v);

	lc=wtk_local_cfg_find_lc_s(main,"kxparm");
	if(lc)
	{
		ret=wtk_kxparm_cfg_update_local(&(cfg->kxparm),lc);
	}
	lc=wtk_local_cfg_find_lc_s(main,"kwake");
	if(lc)
	{
		ret=wtk_kwake_cfg_update_local(&(cfg->kwake),lc);
	}

	lc=wtk_local_cfg_find_lc_s(main,"svprint");
	if(lc)
	{
		ret=wtk_svprint_cfg_update_local(&(cfg->svprint),lc);
	}

	lc=wtk_local_cfg_find_lc_s(main,"kvad");
	if(lc)
	{
		ret=wtk_kvad_cfg_update_local(&(cfg->vad),lc);
	}

	if(cfg->use_vad2){
		lc=wtk_local_cfg_find_lc_s(main,"vad");
		if(!lc)
		{
			lc=wtk_local_cfg_find_lc_s(main,"vad2");
		}
		if(lc)
		{
			ret=wtk_vad_cfg_update_local(&(cfg->vad2),lc);
		}
	}

	lc=wtk_local_cfg_find_lc_s(main,"img");
	if(lc)
	{
		ret=qtk_img_cfg_update_local(&(cfg->img),lc);
	}

	ret=0;
	return ret;
}

int qtk_kws_cfg_update(qtk_kws_cfg_t *cfg)
{
	cfg->sl.hook=0;
	cfg->sl.vf=wtk_source_load_file_v;
	return qtk_kws_cfg_update2(cfg,&cfg->sl);
}

int qtk_kws_cfg_update2(qtk_kws_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	cfg->sl = *sl;
    ret=wtk_kxparm_cfg_update2(&(cfg->kxparm),sl);
	if(ret!=0){goto end;}

	if(cfg->use_wake1)
	{
		ret=wtk_kwake_cfg_update2(&(cfg->kwake),sl);
		if(ret!=0){goto end;}
	}

	if(cfg->use_svprint)
	{
		ret=wtk_svprint_cfg_update2(&(cfg->svprint),sl);
		if(ret!=0){goto end;}
	}

	if(cfg->use_vad)
	{
		ret=wtk_kvad_cfg_update2(&(cfg->vad),sl);
		if(ret!=0){goto end;}
	}

	if(cfg->use_vad2)
	{
		ret=wtk_vad_cfg_update2(&(cfg->vad2),sl);
		if(ret!=0)
		{
			wtk_debug("update vad2 failed\n");
			goto end;
		}
	}

	if(cfg->use_img)
	{
		ret=qtk_img_cfg_update2(&(cfg->img),sl);
		if(ret!=0){goto end;}
	}
end:
	return ret;
}

qtk_kws_cfg_t* qtk_kws_cfg_new_bin(char *bin_fn,char *cfg_fn)
{
	wtk_mbin_cfg_t *cfg;
	qtk_kws_cfg_t *vc;

	//wtk_debug("%s/%s\n",bin_fn,cfg_fn);
	cfg=wtk_mbin_cfg_new_type(qtk_kws_cfg,bin_fn,cfg_fn);
	vc=(qtk_kws_cfg_t*)(cfg->cfg);
	vc->hook=cfg;
	return vc;
}

qtk_kws_cfg_t* qtk_kws_cfg_new_bin2(char *bin_fn)
{
	return qtk_kws_cfg_new_bin(bin_fn,"./kws.cfg");
}

qtk_kws_cfg_t* qtk_kws_cfg_new_bin3(char *bin_fn,unsigned int seek_pos)
{
	qtk_kws_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	char *cfg_fn="./kws.cfg";
	int ret;

	cfg=(qtk_kws_cfg_t*)wtk_malloc(sizeof(qtk_kws_cfg_t));
	qtk_kws_cfg_init(cfg);
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
	ret=qtk_kws_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}
	cfg->sl.hook=cfg->rbin;
	cfg->sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=qtk_kws_cfg_update2(cfg,&(cfg->sl));
	if(ret!=0){goto end;}
end:
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		qtk_kws_cfg_delete_bin2(cfg);
		cfg=NULL;
	}
	return cfg;
}

int qtk_kws_cfg_delete_bin(qtk_kws_cfg_t *cfg)
{
	wtk_mbin_cfg_delete((wtk_mbin_cfg_t*)(cfg->hook));
	return 0;
}

int qtk_kws_cfg_delete_bin2(qtk_kws_cfg_t *cfg)
{
	qtk_kws_cfg_clean(cfg);
	wtk_free(cfg);
	return 0;
}

qtk_kws_cfg_t* qtk_kws_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_kws_cfg_t *vc;

	main_cfg=wtk_main_cfg_new_type(qtk_kws_cfg,cfg_fn);
	vc=(qtk_kws_cfg_t*)(main_cfg->cfg);
	vc->hook=main_cfg;
	return vc;
}

void qtk_kws_cfg_delete(qtk_kws_cfg_t *cfg)
{
	wtk_main_cfg_delete((wtk_main_cfg_t*)(cfg->hook));
}
