#include "wtk_vrec_cfg.h" 

int wtk_vrec_cfg_init(wtk_vrec_cfg_t *cfg)
{
	wtk_net_cfg_init(&(cfg->net));
	wtk_rec_cfg_init(&(cfg->rec));
	cfg->dict=NULL;
	cfg->dict_fn=NULL;
	cfg->net_fn=NULL;
	cfg->use_ebnf=0;
	cfg->latset=NULL;
	cfg->ebnf=NULL;
	return 0;
}

int wtk_vrec_cfg_clean(wtk_vrec_cfg_t *cfg)
{
	if(cfg->ebnf)
	{
		wtk_ebnf_delete(cfg->ebnf);
	}
	if(cfg->latset)
	{
		wtk_latset_delete(cfg->latset);
	}
	wtk_net_cfg_clean(&(cfg->net));
	wtk_rec_cfg_clean(&(cfg->rec));
	if(cfg->dict)
	{
		wtk_dict_delete(cfg->dict);
	}
	return 0;
}

int wtk_vrec_cfg_update_local(wtk_vrec_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_str(lc,cfg,net_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,dict_fn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ebnf,v);
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

int wtk_vrec_cfg_load_ebnf(wtk_vrec_cfg_t *cfg,char *fn)
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
	ret=wtk_latset_expand_lat(cfg->latset,ebnf->lat);
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


int wtk_vrec_cfg_update2(wtk_vrec_cfg_t *cfg,wtk_source_loader_t *sl,wtk_label_t *label,wtk_hmmset_t *hmmset)
{
	int ret;

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
	cfg->dict=wtk_dict_new(label,0);
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
	cfg->latset=wtk_latset_new(&(cfg->net),cfg->dict,hmmset,
			(wtk_dict_word_find_f)wtk_dict_find_word,cfg->dict);
	if(cfg->use_ebnf)
	{
		cfg->ebnf=wtk_ebnf_new(NULL,cfg->dict,(wtk_dict_word_find_f)wtk_dict_find_word,cfg->dict);
		ret=wtk_vrec_cfg_load_ebnf(cfg,cfg->net_fn);
		if(ret!=0){goto end;}
	}else
	{
		if(cfg->net_fn)
		{
			ret=wtk_source_loader_load(sl,cfg->latset,(wtk_source_load_handler_t)wtk_latset_load,cfg->net_fn);
			//ret=wtk_source_load_file(cfg->lat_set,(wtk_source_load_handler_t)wtk_latset_load,cfg->net_fn);
			if(ret!=0)
			{
				wtk_debug("update latset failed\n");
				goto end;
			}
			ret=wtk_latset_expand(cfg->latset);
			if(ret!=0)
			{
				wtk_debug("expand latset failed\n");
				goto end;
			}
		}
	}

end:
	//wtk_debug("ret=%d\n",ret);
	return ret;
}
