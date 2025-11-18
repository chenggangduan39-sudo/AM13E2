#include "wtk_vprint_cfg.h" 

int wtk_vprint_cfg_init(wtk_vprint_cfg_t *cfg)
{
	wtk_vparm_cfg_init(&(cfg->vparm));
	wtk_vtrain_cfg_init(&(cfg->train));
	wtk_vdetect_cfg_init(&(cfg->detect));
	cfg->detect.hmmset=&(cfg->vparm.hmmset);
	cfg->rbin=0;
	cfg->cfile=0;
	cfg->use_share_vparm=0;
	cfg->main_cfg=NULL;
	cfg->train_update_cnt=3;
	return 0;
}

int wtk_vprint_cfg_clean(wtk_vprint_cfg_t *cfg)
{
	wtk_vparm_cfg_clean(&(cfg->vparm));
	wtk_vtrain_cfg_clean(&(cfg->train));
	wtk_vdetect_cfg_clean(&(cfg->detect));
	if(cfg->cfile){wtk_cfg_file_delete(cfg->cfile);}
	if(cfg->rbin){wtk_rbin2_delete(cfg->rbin);}
	return 0;
}

int wtk_vprint_cfg_update_local(wtk_vprint_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_b(lc,cfg,use_share_vparm,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,train_update_cnt,v);
	lc=wtk_local_cfg_find_lc_s(main,"vparm");
	if(lc)
	{
		ret=wtk_vparm_cfg_update_local(&(cfg->vparm),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"train");
	if(lc)
	{
		ret=wtk_vtrain_cfg_update_local(&(cfg->train),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"detect");
	if(lc)
	{
		ret=wtk_vdetect_cfg_update_local(&(cfg->detect),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_vprint_cfg_update(wtk_vprint_cfg_t *cfg)
{
	int ret;

	ret=wtk_vparm_cfg_update(&(cfg->vparm));
	if(ret!=0){goto end;}
	ret=wtk_vtrain_cfg_update(&(cfg->train));
	if(ret!=0){goto end;}
	ret=wtk_vdetect_cfg_update(&(cfg->detect));
	if(ret!=0){goto end;}
	ret=0;
end:
	return ret;
}

int wtk_vprint_cfg_update2(wtk_vprint_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret=0;

	ret=wtk_vparm_cfg_update2(&(cfg->vparm),sl);
	if(ret!=0){goto end;}
	ret=wtk_vtrain_cfg_update2(&(cfg->train),sl);
	if(ret!=0){goto end;}
	ret=wtk_vdetect_cfg_update2(&(cfg->detect),sl);	//not complete
	if(ret!=0){goto end;}

end:
	return ret;
}

wtk_vprint_cfg_t* wtk_vprint_cfg_new(char *cfg)
{
	wtk_main_cfg_t *main_cfg;
	wtk_vprint_cfg_t *vc=NULL;

	main_cfg=wtk_main_cfg_new_type(wtk_vprint_cfg,cfg);
	if(!main_cfg){goto end;}
	vc=(wtk_vprint_cfg_t*)main_cfg->cfg;
	vc->main_cfg=main_cfg;
end:
	return vc;
}

void wtk_vprint_cfg_delete(wtk_vprint_cfg_t *cfg)
{
	if(cfg->main_cfg)
	{
		wtk_main_cfg_delete(cfg->main_cfg);
	}
}

void wtk_vprint_cfg_delete_bin(wtk_vprint_cfg_t *cfg)
{
	wtk_vprint_cfg_clean(cfg);
	wtk_free(cfg);
}

wtk_vprint_cfg_t* wtk_vprint_cfg_new_bin(char *vprint_fn)
{
	wtk_vprint_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./vprint.cfg.r";
	int ret;

	cfg=(wtk_vprint_cfg_t*)wtk_malloc(sizeof(wtk_vprint_cfg_t));
	memset(cfg,0,sizeof(*cfg));
	wtk_vprint_cfg_init(cfg);
	cfg->rbin=wtk_rbin2_new();
	ret=wtk_rbin2_read(cfg->rbin,vprint_fn);
	if(ret!=0){wtk_debug("read vprint rbin failed\n");ret=-1;goto end;}
	item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	if(!item){wtk_debug("%s not found\n",cfg_fn);ret=-1;goto end;}
	cfg->cfile=wtk_cfg_file_new();
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	ret=wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	if(ret!=0){goto end;}
	ret=wtk_vprint_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}
	{
		wtk_string_t v;
		wtk_strbuf_t *buf;

		buf=wtk_strbuf_new(256,1);
		v=wtk_dir_name2(vprint_fn,strlen(vprint_fn),'/');
		wtk_strbuf_push(buf,v.data,v.len);
		wtk_strbuf_push_s(buf,"/usr");
		wtk_strbuf_push_c(buf,0);
		wtk_heap_fill_string(cfg->cfile->heap,&(cfg->detect.usr_dn),buf->data,buf->pos);
		cfg->detect.usr_dn.len-=1;
		cfg->detect.load=1;
		wtk_strbuf_delete(buf);
	}
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=wtk_vprint_cfg_update2(cfg,&sl);
	if(ret!=0){goto end;}
	//cfg->detect.usr_dn.
end:
	if(ret!=0){
		wtk_vprint_cfg_delete_bin(cfg);
		cfg=0;
	}
	return cfg;
}

wtk_vprint_cfg_t* wtk_vprint_cfg_new_bin2(char *vprint_fn,unsigned int seek_pos)
{
	wtk_vprint_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./vprint.cfg.r";
	int ret;

	cfg=(wtk_vprint_cfg_t*)wtk_malloc(sizeof(wtk_vprint_cfg_t));
	memset(cfg,0,sizeof(*cfg));
	wtk_vprint_cfg_init(cfg);
	cfg->rbin=wtk_rbin2_new();
	ret=wtk_rbin2_read2(cfg->rbin,vprint_fn,seek_pos);
	if(ret!=0){wtk_debug("read vprint rbin failed\n");ret=-1;goto end;}
	item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	if(!item){wtk_debug("%s not found\n",cfg_fn);ret=-1;goto end;}
	cfg->cfile=wtk_cfg_file_new();
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	ret=wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	if(ret!=0){goto end;}
	ret=wtk_vprint_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=wtk_vprint_cfg_update2(cfg,&sl);
	if(ret!=0){goto end;}

end:
	if(ret!=0){
		wtk_vprint_cfg_delete_bin(cfg);
		cfg=0;
	}
	return cfg;
}
