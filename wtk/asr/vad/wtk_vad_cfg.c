#include "wtk_vad_cfg.h"

int wtk_vad_cfg_init(wtk_vad_cfg_t *cfg)
{
	int ret;

	cfg->type=WTK_GMM_VAD;
	cfg->left_margin=cfg->right_margin=0;
	cfg->min_speech=0;
	cfg->use_margin_check=1;
	cfg->use_ann=0;
	cfg->use_dnn=0;
	cfg->cfile=0;
	cfg->rbin=0;
	cfg->use_gmm2=0;
	cfg->use_fe=0;
	cfg->use_k=0;
	cfg->margin_proc=0;
	cfg->prob_proc=0;
	ret=wtk_annvad_cfg_init(&(cfg->annvad));
	if(ret!=0){goto end;}
	ret=wtk_fnnvad_cfg_init(&(cfg->dnnvad));
	if(ret!=0){goto end;}
	wtk_gmmvad_cfg_init(&(cfg->gmmvad2));
	wtk_fevad_cfg_init(&(cfg->fevad));
	wtk_kvad_cfg_init(&(cfg->kvad));
	ret=0;
end:
	return ret;
}

int wtk_vad_cfg_clean(wtk_vad_cfg_t *cfg)
{
	if(cfg->use_ann)
	{
		wtk_annvad_cfg_clean(&(cfg->annvad));
	}else if(cfg->use_dnn)
	{
		wtk_fnnvad_cfg_clean(&(cfg->dnnvad));
	}else if(cfg->use_gmm2)
	{
		wtk_gmmvad_cfg_clean(&(cfg->gmmvad2));
	}else if(cfg->use_fe)
	{
		wtk_fevad_cfg_clean(&(cfg->fevad));
	}else if(cfg->use_k)
	{
		wtk_kvad_cfg_clean(&(cfg->kvad));
	}
	if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
	if(cfg->cfile)
	{
		wtk_cfg_file_delete(cfg->cfile);
	}
	return 0;
}

int wtk_vad_cfg_update_local(wtk_vad_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret=0;

	lc=main;
	wtk_local_cfg_update_cfg_i(lc,cfg,left_margin,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,right_margin,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_speech,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_margin_check,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ann,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dnn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_gmm2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fe,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_k,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,margin_proc,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,prob_proc,v);
	if(cfg->use_ann)
	{
		lc=wtk_local_cfg_find_lc_s(main,"annvad");
		if(lc)
		{
			ret=wtk_annvad_cfg_update_local(&(cfg->annvad),lc);
			if(ret!=0){goto end;}
		}
	}	
	if(cfg->use_dnn)
	{
		lc=wtk_local_cfg_find_lc_s(main,"dnnvad");
		if(lc)
		{
			ret=wtk_fnnvad_cfg_update_local(&(cfg->dnnvad),lc);
			if(ret!=0){goto end;}
		}
	}
	if(cfg->use_gmm2)
	{
		lc=wtk_local_cfg_find_lc_s(main,"gmmvad2");
		if(lc)
		{
			ret=wtk_gmmvad_cfg_update_local(&(cfg->gmmvad2),lc);
			if(ret!=0){goto end;}
		}
	}
	if(cfg->use_fe)
	{
		lc=wtk_local_cfg_find_lc_s(main,"fevad");
		if(lc)
		{
			ret=wtk_fevad_cfg_update_local(&(cfg->fevad),lc);
			if(ret!=0){goto end;}
		}
	}
	if(cfg->use_k)
	{
		lc=wtk_local_cfg_find_lc_s(main,"kvad");
		if(lc)
		{
			ret=wtk_kvad_cfg_update_local(&(cfg->kvad),lc);
			if(ret!=0){goto end;}
		}
	}
	ret=0;
end:
	return ret;
}

int wtk_vad_cfg_update(wtk_vad_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_vad_cfg_update2(cfg,&sl);
}

int wtk_vad_cfg_update2(wtk_vad_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;
	//wtk_debug("%d %d\n",cfg->use_dnn,cfg->use_ann);
	if(cfg->use_ann)
	{
		cfg->type=WTK_ANN_VAD;
		ret=wtk_annvad_cfg_update(&(cfg->annvad),sl);
	}else if(cfg->use_dnn)
	{
		cfg->type=WTK_DNN_VAD;
		ret=wtk_fnnvad_cfg_update2(&(cfg->dnnvad),sl);
	}else if(cfg->use_gmm2)
	{
		cfg->type=WTK_GMM_VAD;
		ret=wtk_gmmvad_cfg_update(&(cfg->gmmvad2));
	}else if(cfg->use_fe)
	{
		cfg->type=WTK_FE_VAD;
		ret=wtk_fevad_cfg_update(&(cfg->fevad));
	}else if(cfg->use_k)
	{
		cfg->type=WTK_K_VAD;
		ret=wtk_kvad_cfg_update2(&(cfg->kvad),sl);
	}else
	{
		ret=-1;
	}
	if(ret!=0){goto end;}
end:
	return ret;
}

wtk_vad_cfg_t* wtk_vad_cfg_new_bin(char *bin_fn,char *cfg_fn)
{
	wtk_mbin_cfg_t *cfg;
	wtk_vad_cfg_t *vc;

	//wtk_debug("%s/%s\n",bin_fn,cfg_fn);
	cfg=wtk_mbin_cfg_new_type(wtk_vad_cfg,bin_fn,cfg_fn);
	vc=(wtk_vad_cfg_t*)(cfg->cfg);
	vc->hook=cfg;
	return vc;
}

wtk_vad_cfg_t* wtk_vad_cfg_new_bin2(char *bin_fn)
{
	return wtk_vad_cfg_new_bin(bin_fn,"./vad.cfg.r");
}

wtk_vad_cfg_t* wtk_vad_cfg_new_bin3(char *bin_fn,unsigned int seek_pos)
{
	wtk_vad_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./vad.cfg.r";
	int ret;

	cfg=(wtk_vad_cfg_t*)wtk_malloc(sizeof(wtk_vad_cfg_t));
	wtk_vad_cfg_init(cfg);
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
	ret=wtk_vad_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=wtk_vad_cfg_update2(cfg,&sl);
	if(ret!=0){goto end;}
end:
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		wtk_vad_cfg_delete_bin2(cfg);
		cfg=NULL;
	}
	return cfg;
}

int wtk_vad_cfg_delete_bin(wtk_vad_cfg_t *cfg)
{
	wtk_mbin_cfg_delete((wtk_mbin_cfg_t*)(cfg->hook));
	return 0;
}

int wtk_vad_cfg_delete_bin2(wtk_vad_cfg_t *cfg)
{
	wtk_vad_cfg_clean(cfg);
	wtk_free(cfg);
	return 0;
}

wtk_vad_cfg_t* wtk_vad_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_vad_cfg_t *vc;

	main_cfg=wtk_main_cfg_new_type(wtk_vad_cfg,cfg_fn);
	vc=(wtk_vad_cfg_t*)(main_cfg->cfg);
	vc->hook=main_cfg;
	return vc;
}

void wtk_vad_cfg_delete(wtk_vad_cfg_t *cfg)
{
	wtk_main_cfg_delete((wtk_main_cfg_t*)(cfg->hook));
}
