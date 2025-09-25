#include "wtk_masknet_cfg.h" 

wtk_masknet_layer_t *wtk_masknet_layer_new(wtk_masknet_layer_type_t type)
{
	wtk_masknet_layer_t *ml;

	ml=(wtk_masknet_layer_t *)wtk_malloc(sizeof(wtk_masknet_layer_t));
	ml->type=type;

	ml->cnn=NULL;
	ml->lstm=NULL;
	ml->tdnn=NULL;
	ml->dnn=NULL;
	ml->gru=NULL;

	return ml;
}

void wtk_masknet_layer_delete(wtk_masknet_layer_t *ml)
{
	wtk_free(ml);
}

int wtk_masknet_cfg_init(wtk_masknet_cfg_t *cfg)
{
	cfg->layer_depth=0;
	wtk_queue_init(&(cfg->layer_q));
	cfg->model_fn=NULL;

	return 0;
}

int wtk_masknet_cfg_clean(wtk_masknet_cfg_t *cfg)
{
	wtk_queue_node_t *qn;
	wtk_masknet_layer_t *ml;

	while(cfg->layer_q.length>0)
	{
		qn=wtk_queue_pop(&(cfg->layer_q));
		ml=(wtk_masknet_layer_t *)data_offset2(qn, wtk_masknet_layer_t, q_n);
		switch(ml->type)
		{
			case WTK_CNNNET_LAYER:
				wtk_cnnnet_layer_delete(ml->cnn);
				break;
    		case WTK_LSTMNET_LAYER:
				wtk_lstmnet_layer_delete(ml->lstm);
				break;
    		case WTK_TDNNNET_LAYER:
				wtk_tdnnnet_layer_delete(ml->tdnn);
				break;
    		case WTK_DNNNET_LAYER:
				wtk_dnnnet_layer_delete(ml->dnn);
				break;
			case WTK_GRUNET_LAYER:
				wtk_grunet_layer_delete(ml->gru);
				break;
		}
		wtk_masknet_layer_delete(ml);
	}
	return 0;
}

int wtk_masknet_cfg_update_local(wtk_masknet_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,model_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,ifeat_len,v);
	
	return 0;
}

int wtk_masknet_cfg_update_mdl(wtk_masknet_cfg_t *cfg, wtk_source_t *src)
{
	int ret;
	wtk_strbuf_t *buf;
	wtk_masknet_layer_t *layer;
	wtk_cnnnet_layer_t *cnn;
	wtk_lstmnet_layer_t *lstm;
	wtk_tdnnnet_layer_t *tdnn;
	wtk_dnnnet_layer_t *dnn;
	wtk_grunet_layer_t *gru;

	buf = wtk_strbuf_new(256, 1);
	while(1)
	{
		ret = wtk_source_read_string(src, buf);
		if(ret != 0 && cfg->layer_depth==0)
		{
			wtk_debug("error: empty net\n");
			goto end;
		}else if(ret!=0)
		{
			break;
		}

		if(wtk_str_equal_s(buf->data, buf->pos,"cnn"))
		{
			cnn=wtk_cnnnet_layer_new(src, buf);
			if(cnn)
			{
				layer=wtk_masknet_layer_new(WTK_CNNNET_LAYER);
				layer->cnn=cnn;
				wtk_queue_push(&(cfg->layer_q), &(layer->q_n));
			}else
			{
				ret=-1;
				goto end;
			}
			++cfg->layer_depth;
		}else if(wtk_str_equal_s(buf->data, buf->pos,"lstm"))
		{
			lstm=wtk_lstmnet_layer_new(src, buf);
			if(lstm)
			{
				layer=wtk_masknet_layer_new(WTK_LSTMNET_LAYER);
				layer->lstm=lstm;
				wtk_queue_push(&(cfg->layer_q), &(layer->q_n));
			}else
			{
				ret=-1;
				goto end;
			}
			++cfg->layer_depth;
		}else if(wtk_str_equal_s(buf->data, buf->pos,"gru"))
		{
			gru=wtk_grunet_layer_new(src, buf);
			if(gru)
			{
				layer=wtk_masknet_layer_new(WTK_GRUNET_LAYER);
				layer->gru=gru;
				wtk_queue_push(&(cfg->layer_q), &(layer->q_n));
			}else
			{
				ret=-1;
				goto end;
			}
			++cfg->layer_depth;
		}else if(wtk_str_equal_s(buf->data, buf->pos,"tdnn"))
		{
			tdnn=wtk_tdnnnet_layer_new(src, buf);
			if(tdnn)
			{
				layer=wtk_masknet_layer_new(WTK_TDNNNET_LAYER);
				layer->tdnn=tdnn;
				wtk_queue_push(&(cfg->layer_q), &(layer->q_n));
			}else
			{
				ret=-1;
				goto end;
			}
			++cfg->layer_depth;
		}else if(wtk_str_equal_s(buf->data, buf->pos,"dnn"))
		{
			dnn=wtk_dnnnet_layer_new(src, buf);
			if(dnn)
			{
				layer=wtk_masknet_layer_new(WTK_DNNNET_LAYER);
				layer->dnn=dnn;
				wtk_queue_push(&(cfg->layer_q), &(layer->q_n));
			}else
			{
				ret=-1;
				goto end;
			}
			++cfg->layer_depth;
		}else
		{
			wtk_debug("error: no  cnn / lstm / tdnn / dnn / gru   %s \n", buf->data);
			ret=-1;
			goto end;
		}	
	}

	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

int wtk_masknet_cfg_update(wtk_masknet_cfg_t *cfg)
{
	int ret;
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
    ret = wtk_source_loader_load(&sl, cfg, (wtk_source_load_handler_t) wtk_masknet_cfg_update_mdl, cfg->model_fn);
	if(ret!=0){goto end;}

	ret=0;
end:
	return ret;
}

int wtk_masknet_cfg_update2(wtk_masknet_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;
	wtk_source_loader_t file_sl;
	
	if(!sl)
	{
		file_sl.hook=0;
		file_sl.vf=wtk_source_load_file_v;
		sl=&(file_sl);
	}
    ret = wtk_source_loader_load(sl, cfg, (wtk_source_load_handler_t) wtk_masknet_cfg_update_mdl, cfg->model_fn);
    if(ret!=0){goto end;}

	ret=0;
end:
	return ret;
}

wtk_masknet_cfg_t* wtk_masknet_cfg_new_bin(char *bin_fn,char *cfg_fn)
{
	wtk_mbin_cfg_t *cfg;
	wtk_masknet_cfg_t *vc;

	//wtk_debug("%s/%s\n",bin_fn,cfg_fn);
	cfg=wtk_mbin_cfg_new_type(wtk_masknet_cfg,bin_fn,cfg_fn);
	vc=(wtk_masknet_cfg_t*)(cfg->cfg);
	vc->hook=cfg;
	return vc;
}

wtk_masknet_cfg_t* wtk_masknet_cfg_new_bin2(char *bin_fn)
{
	return wtk_masknet_cfg_new_bin(bin_fn,"./masknet.cfg");
}

int wtk_masknet_cfg_delete_bin2(wtk_masknet_cfg_t *cfg)
{
	wtk_mbin_cfg_delete((wtk_mbin_cfg_t*)(cfg->hook));
	return 0;
}

wtk_masknet_cfg_t* wtk_masknet_cfg_new_bin3(char *bin_fn,unsigned int seek_pos)
{
	wtk_masknet_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./masknet.cfg";
	int ret;

	cfg=(wtk_masknet_cfg_t*)wtk_malloc(sizeof(wtk_masknet_cfg_t));
	wtk_masknet_cfg_init(cfg);
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
	ret=wtk_masknet_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=wtk_masknet_cfg_update2(cfg,&sl);
	if(ret!=0){goto end;}
end:
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		wtk_masknet_cfg_delete_bin2(cfg);
		cfg=NULL;
	}
	return cfg;
}

int wtk_masknet_cfg_delete_bin3(wtk_masknet_cfg_t *cfg)
{
    wtk_rbin2_delete(cfg->rbin);
    wtk_cfg_file_delete(cfg->cfile);
	wtk_masknet_cfg_clean(cfg);
	wtk_free(cfg);
	return 0;
}
