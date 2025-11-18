#include "wtk_gainnet_cfg.h" 

int wtk_gainnet_cfg_init(wtk_gainnet_cfg_t *cfg)
{
	cfg->model_fn=NULL;

	cfg->in_dnn=NULL;
	cfg->vad_gru=NULL;
	cfg->vad_lstm=NULL;
	// cfg->vad_odnn=NULL;

	cfg->denoise_gru=NULL;
	cfg->denoise_lstm=NULL;
	cfg->noise_gru=NULL;
	cfg->noise_lstm=NULL;
	cfg->denoise_odnn=NULL;

	cfg->ifeat_len=0;
	cfg->use_lstm=0;

	return 0;
}

int wtk_gainnet_cfg_clean(wtk_gainnet_cfg_t *cfg)
{
	if(cfg->in_dnn)
	{
		wtk_dnnnet_layer_delete(cfg->in_dnn);
	}
	if(cfg->vad_gru)
	{
		wtk_grunet_layer_delete(cfg->vad_gru);
	}
	if(cfg->vad_lstm)
	{
		wtk_lstmnet_layer_delete(cfg->vad_lstm);
	}
	// if(cfg->vad_odnn)
	// {
	// 	wtk_dnnnet_layer_delete(cfg->vad_odnn);
	// }
	if(cfg->noise_gru)
	{
		wtk_grunet_layer_delete(cfg->noise_gru);
	}
	if(cfg->denoise_gru)
	{
		wtk_grunet_layer_delete(cfg->denoise_gru);
	}
	if(cfg->noise_lstm)
	{
		wtk_lstmnet_layer_delete(cfg->noise_lstm);
	}
	if(cfg->denoise_lstm)
	{
		wtk_lstmnet_layer_delete(cfg->denoise_lstm);
	}
	if(cfg->denoise_odnn)
	{
		wtk_dnnnet_layer_delete(cfg->denoise_odnn);
	}

	return 0;
}

int wtk_gainnet_cfg_update_local(wtk_gainnet_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,model_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,ifeat_len,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_lstm,v);

	return 0;
}

int wtk_gainnet_cfg_update_mdl(wtk_gainnet_cfg_t *cfg, wtk_source_t *src)
{
	int ret;
	wtk_strbuf_t *buf;

	buf = wtk_strbuf_new(256, 1);

	ret = wtk_source_read_string(src, buf);
	if(ret!=0)
	{
		wtk_debug("error: empty net\n");
		goto end;
	}
	if(wtk_str_equal_s(buf->data, buf->pos,"dnn"))
	{
		cfg->in_dnn=wtk_dnnnet_layer_new(src, buf);
		if(!cfg->in_dnn)
		{
			ret=-1;goto end;
		}
	}else
	{
		wtk_debug("error: no dnn   %s \n", buf->data);goto end;
	}	

	ret = wtk_source_read_string(src, buf);
	if(ret!=0)
	{
		wtk_debug("error: empty net\n");
		goto end;
	}
	if(cfg->use_lstm)
	{
		if(wtk_str_equal_s(buf->data, buf->pos,"lstm"))
		{
			cfg->vad_lstm=wtk_lstmnet_layer_new(src, buf);
			if(!cfg->vad_lstm)
			{
				ret=-1;goto end;
			}
		}else
		{
			wtk_debug("error: no lstm   %s \n", buf->data);goto end;
		}	
	}else
	{
		if(wtk_str_equal_s(buf->data, buf->pos,"gru"))
		{
			cfg->vad_gru=wtk_grunet_layer_new(src, buf);
			if(!cfg->vad_gru)
			{
				ret=-1;goto end;
			}
		}else
		{
			wtk_debug("error: no gru   %s \n", buf->data);goto end;
		}	
	}

	// ret = wtk_source_read_string(src, buf);
	// if(ret!=0)
	// {
	// 	wtk_debug("error: empty net\n");
	// 	goto end;
	// }
	// if(wtk_str_equal_s(buf->data, buf->pos,"dnn"))
	// {
	// 	cfg->vad_odnn=wtk_dnnnet_layer_new(src, buf);
	// 	if(!cfg->vad_odnn)
	// 	{
	// 		ret=-1;goto end;
	// 	}
	// }else
	// {
	// 	wtk_debug("error: no dnn   %s \n", buf->data);goto end;
	// }	

	ret = wtk_source_read_string(src, buf);
	if(ret!=0)
	{
		wtk_debug("error: empty net\n");
		goto end;
	}
	if(cfg->use_lstm)
	{
		if(wtk_str_equal_s(buf->data, buf->pos,"lstm"))
		{
			cfg->noise_lstm=wtk_lstmnet_layer_new(src, buf);
			if(!cfg->noise_lstm)
			{
				ret=-1;	goto end;
			}
		}else
		{
			wtk_debug("error: no lstm   %s \n", buf->data);goto end;
		}	
	}else
	{
		if(wtk_str_equal_s(buf->data, buf->pos,"gru"))
		{
			cfg->noise_gru=wtk_grunet_layer_new(src, buf);
			if(!cfg->noise_gru)
			{
				ret=-1;	goto end;
			}
		}else
		{
			wtk_debug("error: no gru   %s \n", buf->data);goto end;
		}	
	}


	ret = wtk_source_read_string(src, buf);
	if(ret!=0)
	{
		wtk_debug("error: empty net\n");
		goto end;
	}
	if(cfg->use_lstm)
	{
		if(wtk_str_equal_s(buf->data, buf->pos,"lstm"))
		{
			cfg->denoise_lstm=wtk_lstmnet_layer_new(src, buf);
			if(!cfg->denoise_lstm)
			{
				ret=-1;goto end;
			}
		}else
		{
			wtk_debug("error: no gru   %s \n", buf->data);goto end;
		}	
	}else
	{
		if(wtk_str_equal_s(buf->data, buf->pos,"gru"))
		{
			cfg->denoise_gru=wtk_grunet_layer_new(src, buf);
			if(!cfg->denoise_gru)
			{
				ret=-1;goto end;
			}
		}else
		{
			wtk_debug("error: no gru   %s \n", buf->data);goto end;
		}	
	}
	

	ret = wtk_source_read_string(src, buf);
	if(ret!=0)
	{
		wtk_debug("error: empty net\n");
		goto end;
	}
	if(wtk_str_equal_s(buf->data, buf->pos,"dnn"))
	{
		cfg->denoise_odnn=wtk_dnnnet_layer_new(src, buf);
		if(!cfg->denoise_odnn)
		{
			ret=-1;goto end;
		}
	}else
	{
		wtk_debug("error: no dnn   %s \n", buf->data);goto end;
	}	

	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

int wtk_gainnet_cfg_update(wtk_gainnet_cfg_t *cfg)
{
	int ret;
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
    ret = wtk_source_loader_load(&sl, cfg, (wtk_source_load_handler_t) wtk_gainnet_cfg_update_mdl, cfg->model_fn);
	if(ret!=0){goto end;}

	ret=0;
end:
	return ret;
}

int wtk_gainnet_cfg_update2(wtk_gainnet_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;
	wtk_source_loader_t file_sl;
	
	if(!sl)
	{
		file_sl.hook=0;
		file_sl.vf=wtk_source_load_file_v;
		sl=&(file_sl);
	}
    ret = wtk_source_loader_load(sl, cfg, (wtk_source_load_handler_t) wtk_gainnet_cfg_update_mdl, cfg->model_fn);
    if(ret!=0){goto end;}

	ret=0;
end:
	return ret;
}

wtk_gainnet_cfg_t* wtk_gainnet_cfg_new_bin(char *bin_fn,char *cfg_fn)
{
	wtk_mbin_cfg_t *cfg;
	wtk_gainnet_cfg_t *vc;

	//wtk_debug("%s/%s\n",bin_fn,cfg_fn);
	cfg=wtk_mbin_cfg_new_type(wtk_gainnet_cfg,bin_fn,cfg_fn);
	vc=(wtk_gainnet_cfg_t*)(cfg->cfg);
	vc->hook=cfg;
	return vc;
}

wtk_gainnet_cfg_t* wtk_gainnet_cfg_new_bin2(char *bin_fn)
{
	return wtk_gainnet_cfg_new_bin(bin_fn,"./gainnet.cfg");
}

int wtk_gainnet_cfg_delete_bin2(wtk_gainnet_cfg_t *cfg)
{
	wtk_mbin_cfg_delete((wtk_mbin_cfg_t*)(cfg->hook));
	return 0;
}

wtk_gainnet_cfg_t* wtk_gainnet_cfg_new_bin3(char *bin_fn,unsigned int seek_pos)
{
	wtk_gainnet_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./gainnet.cfg";
	int ret;

	cfg=(wtk_gainnet_cfg_t*)wtk_malloc(sizeof(wtk_gainnet_cfg_t));
	wtk_gainnet_cfg_init(cfg);
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
	ret=wtk_gainnet_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=wtk_gainnet_cfg_update2(cfg,&sl);
	if(ret!=0){goto end;}
end:
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		wtk_gainnet_cfg_delete_bin2(cfg);
		cfg=NULL;
	}
	return cfg;
}

int wtk_gainnet_cfg_delete_bin3(wtk_gainnet_cfg_t *cfg)
{
    wtk_rbin2_delete(cfg->rbin);
    wtk_cfg_file_delete(cfg->cfile);
	wtk_gainnet_cfg_clean(cfg);
	wtk_free(cfg);
	return 0;
}
