#include "wtk_vdec_cfg.h"


int wtk_vdec_cfg_init(wtk_vdec_cfg_t *cfg)
{
	wtk_fextra_cfg_init(&(cfg->parm));
	wtk_hmmset_cfg_init(&(cfg->hmmset));
	wtk_net_cfg_init(&(cfg->net));
	wtk_rec_cfg_init(&(cfg->rec));
	cfg->ebnf=NULL;
	cfg->dict_fn=NULL;
	cfg->net_fn=NULL;
	cfg->label=NULL;
	cfg->lat_set=NULL;
	cfg->dict=NULL;
	cfg->ebnf_fn=NULL;
	cfg->use_ebnf=0;
	cfg->cfile=NULL;
	cfg->rbin=NULL;
	return 0;
}

int wtk_vdec_cfg_clean(wtk_vdec_cfg_t *cfg)
{
	if(cfg->ebnf)
	{
		wtk_ebnf_delete(cfg->ebnf);
	}
	wtk_fextra_cfg_clean(&(cfg->parm));
	wtk_hmmset_cfg_clean(&(cfg->hmmset));
	wtk_net_cfg_clean(&(cfg->net));
	wtk_rec_cfg_clean(&(cfg->rec));
	if(cfg->dict)
	{
		wtk_dict_delete(cfg->dict);
	}
	if(cfg->lat_set)
	{
		wtk_latset_delete(cfg->lat_set);
	}
	if(cfg->label)
	{
		wtk_label_delete(cfg->label);
	}
	if(cfg->cfile)
	{
		wtk_cfg_file_delete(cfg->cfile);
	}
	if(cfg->rbin)
	{
		wtk_rbin2_delete(cfg->rbin);
	}
	return 0;
}

int wtk_vdec_cfg_update_local(wtk_vdec_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_str(lc,cfg,net_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,dict_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,ebnf_fn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ebnf,v);
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
	lc=wtk_local_cfg_find_lc_s(main,"net");
	if(lc)
	{
		ret=wtk_net_cfg_update_local(&(cfg->net),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"rec");
	if(lc)
	{
		ret=wtk_rec_cfg_update_local(&(cfg->rec),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_vdec_cfg_load_ebnf(wtk_vdec_cfg_t *cfg,char *fn)
{
	wtk_ebnf_t *ebnf=cfg->ebnf;
	char *data;
	int len;
	int ret=-1;
	//double t;

	data=file_read_buf(fn,&len);
	if(!data)
	{
		wtk_debug("load ebnf[%s] failed.\n",fn);
		goto end;
	}
	//t=time_get_ms();
	ret=wtk_ebnf_feed(ebnf,data,len);
	if(ret!=0){goto end;}
	//wtk_debug("%f\n",time_get_ms()-t);
	ret=wtk_latset_expand_lat(cfg->lat_set,ebnf->lat);
	if(ret!=0){goto end;}
	//wtk_debug("%f\n",time_get_ms()-t);
	ret=0;
end:
	if(data)
	{
		wtk_free(data);
	}
	return ret;
}

int wtk_vdec_cfg_update2(wtk_vdec_cfg_t *cfg,wtk_source_loader_t *sl)
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
	ret=wtk_net_cfg_update(&(cfg->net));
	if(ret!=0)
	{
		wtk_debug("update net failed\n");
		goto end;
	}
	ret=wtk_rec_cfg_update(&(cfg->rec));
	if(ret!=0)
	{
		wtk_debug("update rec failed\n");
		goto end;
	}
	cfg->dict=wtk_dict_new(cfg->label,0);
	//wtk_debug("%s\n",cfg->dict_fn);
	if(cfg->dict_fn)
	{
		ret=wtk_source_loader_load(sl,cfg->dict,(wtk_source_load_handler_t)wtk_dict_load,cfg->dict_fn);
		//ret=wtk_source_load_file(cfg->dict,(wtk_source_load_handler_t)wtk_dict_load,cfg->dict_fn);
		if(ret!=0)
		{
			wtk_debug("update dict failed\n");
			goto end;
		}
	}
	cfg->lat_set=wtk_latset_new(&(cfg->net),cfg->dict,cfg->hmmset.hmmset,
			(wtk_dict_word_find_f)wtk_dict_find_word,cfg->dict);
	/*
	{
		wtk_ebnf_t *ebnf;
		//char *s="(\\<s\\> ( male | female ) \\</s\\>)";
		char *s="(sil ( male | female ) sil)";

		ebnf=wtk_ebnf_new(0,cfg->dict,(wtk_dict_word_find_f)wtk_dict_find_word,cfg->dict);
		ret=wtk_ebnf_feed(ebnf,s,strlen(s));
		wtk_debug("ret=%d\n",ret);
		ret=wtk_latset_expand_lat(cfg->lat_set,ebnf->lat);
		cfg->lat_set->main=ebnf->lat;
		wtk_debug("ret=%d\n",ret);
	}*/
	if(cfg->use_ebnf)
	{
		cfg->ebnf=wtk_ebnf_new(NULL,cfg->dict,(wtk_dict_word_find_f)wtk_dict_find_word,cfg->dict);
		ret=wtk_vdec_cfg_load_ebnf(cfg,cfg->ebnf_fn);
		if(ret!=0){goto end;}
	}else
	{
		if(cfg->net_fn)
		{
			ret=wtk_source_loader_load(sl,cfg->lat_set,(wtk_source_load_handler_t)wtk_latset_load,cfg->net_fn);
			//ret=wtk_source_load_file(cfg->lat_set,(wtk_source_load_handler_t)wtk_latset_load,cfg->net_fn);
			if(ret!=0)
			{
				wtk_debug("update latset failed\n");
				goto end;
			}
			ret=wtk_latset_expand(cfg->lat_set);
			if(ret!=0)
			{
				wtk_debug("expand latset failed\n");
				goto end;
			}
		}
	}
	ret=0;
end:
	//wtk_debug("ret=%d\n",ret);
	return ret;
}

int wtk_vdec_cfg_update(wtk_vdec_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_vdec_cfg_update2(cfg,&(sl));
}

void wtk_vdec_cfg_delete_bin(wtk_vdec_cfg_t *cfg)
{
	wtk_vdec_cfg_clean(cfg);
	wtk_free(cfg);
}

wtk_vdec_cfg_t* wtk_vdec_cfg_new_bin(char *bin_fn)
{
	wtk_vdec_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./vdec.cfg";
	int ret;

	//wtk_debug("read bin %s\n",bin_fn);
	cfg=(wtk_vdec_cfg_t*)wtk_malloc(sizeof(wtk_vdec_cfg_t));
	wtk_vdec_cfg_init(cfg);
	cfg->rbin=wtk_rbin2_new();
	ret=wtk_rbin2_read(cfg->rbin,bin_fn);
	if(ret!=0)
	{
		wtk_debug("read failed %s\n",bin_fn);
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
	ret=wtk_vdec_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}

	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=wtk_vdec_cfg_update2(cfg,&sl);
	if(ret!=0){goto end;}
end:
	if(ret!=0)
	{
		wtk_vdec_cfg_delete_bin(cfg);
		cfg=NULL;
	}
	return cfg;
}
