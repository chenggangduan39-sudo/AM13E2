#include "wtk_wdec_cfg.h" 

void wtk_wdec_post_cfg_init(wtk_wdec_post_cfg_t *cfg)
{
	cfg->min_wrd_conf=NULL;
	cfg->min_wrd_conf2=1.5;
	cfg->min_conf=2.0;
	cfg->min_step_conf=0.3;
	cfg->min_hmm_conf=0.1;

}

void wtk_wdec_post_cfg_clean(wtk_wdec_post_cfg_t *cfg)
{
	if(cfg->min_wrd_conf)
	{
		wtk_free(cfg->min_wrd_conf);
	}
}

int wtk_wdec_post_cfg_update_local(wtk_wdec_post_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	wtk_array_t *a;
	int i;

	wtk_local_cfg_update_cfg_f(lc,cfg,min_hmm_conf,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_step_conf,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_conf,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_wrd_conf2,v);
	a=wtk_local_cfg_find_array_s(lc,"min_wrd_conf");
	if(a)
	{
		cfg->min_wrd_conf=(float*)wtk_calloc(a->nslot,sizeof(float));
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)(a->slot))[i];
			cfg->min_wrd_conf[i]=wtk_str_atof(v->data,v->len);
		}
	}
	return 0;
}

void wtk_wdec_post_cfg_update(wtk_wdec_post_cfg_t *cfg)
{
}

int wtk_wdec_cfg_init(wtk_wdec_cfg_t *cfg)
{
	cfg->cfg.main_cfg=NULL;
	wtk_prune_cfg_init(&(cfg->prune));
	wtk_fextra_cfg_init(&(cfg->fextra));
	wtk_wfst_dnn_cfg_init(&(cfg->dnn));
	wtk_wdec_net_cfg_init(&(cfg->net));
	cfg->step=1;
	cfg->add_path=0;
	wtk_wdec_post_cfg_init(&(cfg->post));
	return 0;
}

int wtk_wdec_cfg_clean(wtk_wdec_cfg_t *cfg)
{
	wtk_prune_cfg_clean(&(cfg->prune));
	wtk_wdec_net_cfg_clean(&(cfg->net));
	wtk_fextra_cfg_clean(&(cfg->fextra));
	wtk_wfst_dnn_cfg_clean(&(cfg->dnn));
	if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
	if(cfg->cfile)
	{
		wtk_cfg_file_delete(cfg->cfile);
	}
	wtk_wdec_post_cfg_clean(&(cfg->post));

	return 0;
}

int wtk_wdec_cfg_update_local(wtk_wdec_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_b(lc,cfg,add_path,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,step,v);
	lc=wtk_local_cfg_find_lc_s(main,"post");
	if(lc)
	{
		wtk_wdec_post_cfg_update_local(&(cfg->post),lc);
	}
	lc=wtk_local_cfg_find_lc_s(main,"prune");

	if(lc)
	{
		ret=wtk_prune_cfg_update_local(&(cfg->prune),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"net");
	if(lc)
	{
		ret=wtk_wdec_net_cfg_update_local(&(cfg->net),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"fextra");
	if(lc)
	{
		ret=wtk_fextra_cfg_update_local(&(cfg->fextra),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"dnn");
	if(lc)
	{
		ret=wtk_wfst_dnn_cfg_update_local(&(cfg->dnn),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_wdec_cfg_update(wtk_wdec_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_wdec_cfg_update2(cfg,&sl);
}


int wtk_wdec_cfg_update2(wtk_wdec_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	//wtk_wdec_cfg_print_phn(cfg);
	wtk_wdec_post_cfg_update(&(cfg->post));
	ret=wtk_prune_cfg_update(&(cfg->prune));
	if(ret!=0){goto end;}
	ret=wtk_wdec_net_cfg_update2(&(cfg->net),sl);
	if(ret!=0){goto end;}
	ret=wtk_fextra_cfg_update2(&(cfg->fextra),sl);
	if(ret!=0){goto end;}
	ret=wtk_wfst_dnn_cfg_update(&(cfg->dnn),sl);
	if(ret!=0){goto end;}
	wtk_wfst_dnn_cfg_attach_hmmset(&(cfg->dnn),cfg->net.hmmset.hmmset);
	wtk_hmmset_transpose_trans_matrix(cfg->net.hmmset.hmmset);
end:
	return ret;
}

wtk_wdec_cfg_t* wtk_wdec_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_wdec_cfg_t *cfg=NULL;

	main_cfg=wtk_main_cfg_new_type(wtk_wdec_cfg,fn);
	if(!main_cfg){goto end;}
	cfg=(wtk_wdec_cfg_t*)main_cfg->cfg;
	cfg->cfg.main_cfg=main_cfg;
end:
	return cfg;
}

void wtk_wdec_cfg_delete(wtk_wdec_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->cfg.main_cfg);
}


wtk_wdec_cfg_t* wtk_wdec_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *bin_cfg;
	wtk_wdec_cfg_t *cfg=NULL;

	bin_cfg=wtk_mbin_cfg_new_type(wtk_wdec_cfg,fn,"./wdec.cfg");
	if(!bin_cfg){goto end;}
	cfg=(wtk_wdec_cfg_t*)bin_cfg->cfg;
	cfg->cfg.bin_cfg=bin_cfg;

end:
	return cfg;
}

void wtk_wdec_cfg_delete_bin(wtk_wdec_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->cfg.bin_cfg);
}

wtk_wdec_cfg_t* wtk_wdec_cfg_new_bin3(char *bin_fn,unsigned int seek_pos)
{
	wtk_wdec_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./wdec.cfg";
	int ret;

	cfg=(wtk_wdec_cfg_t*)wtk_malloc(sizeof(wtk_wdec_cfg_t));
	wtk_wdec_cfg_init(cfg);
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
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	ret=wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	if(ret!=0){goto end;}
	ret=wtk_wdec_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=wtk_wdec_cfg_update2(cfg,&sl);
	if(ret!=0){goto end;}

end:
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		wtk_wdec_cfg_delete_bin2(cfg);
		cfg=NULL;
	}
	return cfg;
}

void wtk_wdec_cfg_delete_bin2(wtk_wdec_cfg_t *cfg)
{
		wtk_wdec_cfg_clean(cfg);
		wtk_free(cfg);
}



