#include "wtk_mlat_cfg.h" 

int wtk_mlat_cfg_init(wtk_mlat_cfg_t *cfg)
{
	cfg->trans=NULL;
	wtk_queue_init(&(cfg->layer_q));
	cfg->net_fn=NULL;
	cfg->trans_fn=NULL;
	cfg->nthread=2;
	cfg->skip_frame=0;

	cfg->max_w=127.0;
	cfg->max_b=255.0;
	cfg->max_0_w=2048.0;
	cfg->use_c=1;
	cfg->use_fix_res=0;
	cfg->is_bin=0;
	cfg->use_int=0;
	cfg->use_transpose=0;
	return 0;
}

void wtk_mlat_fix_layer_delete(wtk_mlat_fix_layer_t *layer,int use_c)
{
	if(use_c)
	{
		wtk_matc_delete(layer->w.c);
	}else
	{
		wtk_mati_delete(layer->w.i);
	}
	wtk_mati_delete(layer->b);
	wtk_free(layer);
}

void wtk_mlat_layer_delete(wtk_mlat_layer_t *layer,int use_c)
{
	if(layer->w)
	{
		wtk_matf_delete(layer->w);
	}
	if(layer->b)
	{
		wtk_vecf_delete(layer->b);
	}
	if(layer->fix_wb)
	{
		wtk_mlat_fix_layer_delete(layer->fix_wb,use_c);
	}
	wtk_free(layer);
}

void wtk_mlat_trans_delete(wtk_mlat_trans_t *t)
{
	wtk_vecf_delete(t->w);
	wtk_vecf_delete(t->b);
	wtk_free(t);

}

int wtk_mlat_cfg_clean(wtk_mlat_cfg_t *cfg)
{
	wtk_queue_node_t *qn;
	wtk_mlat_layer_t *layer;

	while(1)
	{
		qn=wtk_queue_pop(&(cfg->layer_q));
		if(!qn){break;}
		layer=data_offset2(qn,wtk_mlat_layer_t,q_n);
		wtk_mlat_layer_delete(layer,cfg->use_c);
	}
	if(cfg->trans)
	{
		wtk_mlat_trans_delete(cfg->trans);
	}
	return 0;
}

int wtk_mlat_cfg_update_local(wtk_mlat_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,net_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,trans_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nthread,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,skip_frame,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,is_bin,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_transpose,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_c,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fix_res,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_int,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_w,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_b,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_0_w,v);
	return 0;
}

int wtk_mlat_cfg_update(wtk_mlat_cfg_t *cfg)
{
	wtk_source_loader_t sl;
	int ret;

	wtk_source_loader_init_file(&sl);
	ret=wtk_mlat_cfg_update2(cfg,&sl);
	return ret;
}


wtk_vecf_t* wtk_mflat_load_trans_vector(wtk_source_t *src,wtk_strbuf_t *buf,char *name,int is_bin)
{
	wtk_vecf_t *v=0;
	int i;
	int ret;

	ret=wtk_source_read_string(src,buf);
	if(ret!=0){goto end;}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if(strncmp(name,buf->data,buf->pos)!=0)
	{
		wtk_debug("[%s]!=[%.*s]\n",name,buf->pos,buf->data);
		ret=-1;goto end;
	}
	wtk_source_skip_sp(src,NULL);
	ret=wtk_source_read_int(src,&i,1,is_bin);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&i,1,is_bin);
	if(ret!=0){goto end;}
	ret=wtk_source_read_string(src,buf);
	if(ret!=0){goto end;}
	wtk_source_skip_sp(src,NULL);
	ret=wtk_source_read_int(src,&i,1,is_bin);
	if(ret!=0){goto end;}
	//wtk_debug("i=%d\n",i);
	v=wtk_vecf_new(i);
	ret=wtk_source_read_float(src,v->p,v->len,is_bin);
	if(ret!=0){goto end;}
	//wtk_vector_print(v);
end:
	if(ret!=0 && v)
	{
		wtk_vecf_delete(v);
		v=0;
	}
	return v;
}

int wtk_mlat_cfg_load_trans(wtk_mlat_cfg_t *cfg,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	wtk_mlat_trans_t *trans;
	int ret=-1;

	src->swap=0;
	buf=wtk_strbuf_new(256,1);
	trans=(wtk_mlat_trans_t*)wtk_malloc(sizeof(*trans));
	trans->b=wtk_mflat_load_trans_vector(src,buf,"<bias>",cfg->is_bin);
	if(!trans->b){goto end;}
	trans->w=wtk_mflat_load_trans_vector(src,buf,"<window>",cfg->is_bin);
	if(!trans->w){goto end;}
	cfg->trans=trans;
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

wtk_mlat_layer_t* wtk_mlat_layer_load(wtk_mlat_cfg_t *cfg,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	int row,col;
	int v;
	wtk_matf_t *m=0;
	wtk_vecf_t *vf;
	wtk_mlat_layer_t *layer;
	int is_bin;

	is_bin=cfg->is_bin;
	layer=(wtk_mlat_layer_t*)wtk_calloc(1,sizeof(*layer));
	layer->w=0;
	layer->b=0;
	//wtk_debug("layer=%p\n",layer);
	ret=wtk_source_read_string(src,buf);
	if(ret!=0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if(!wtk_str_equal_s(buf->data,buf->pos,"<biasedlinearity>"))
	{
		ret=-1;
		goto end;
	}
	wtk_source_skip_sp(src,NULL);
	ret=wtk_source_read_int(src,&row,1,is_bin);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&col,1,is_bin);
	//wtk_debug("row=%d, col=%d\n",row,col);
	wtk_source_read_string(src,buf);
	wtk_source_skip_sp(src,NULL);
	//wtk_source_read_line(src,buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret=wtk_source_read_int(src,&v,1,is_bin);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&v,1,is_bin);
	if(ret!=0){goto end;}
	//wtk_debug("row=%d col=%d\n",row,col);
	m=wtk_matf_new(row,col);
	ret=wtk_source_read_float(src,m->p,row*col,is_bin);
	if(ret!=0)
	{
		wtk_debug("read window failed\n");
		goto end;
	}
	layer->w=m;
	m=0;
	wtk_source_read_string(src,buf);
	//print_data(buf->data,buf->pos);
	//print_hex(buf->data,buf->pos);
	if(!wtk_str_equal_s(buf->data,buf->pos,"v"))
	{
		wtk_debug("invalid [%.*s] must v\n",buf->pos,buf->data);
		ret=-1;goto end;
	}
	wtk_source_skip_sp(src,NULL);
	//wtk_debug("row=%d col=%d\n",row,col);
	ret=wtk_source_read_int(src,&v,1,is_bin);
	//wtk_debug("ret=%d,v=%d\n",ret,v);
	if(ret!=0){goto end;}
	//wtk_debug("v=%d\n",v);
	vf=wtk_vecf_new(row);
	ret=wtk_source_read_float(src,vf->p,vf->len,is_bin);
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		wtk_debug("read bias failed\n");
		goto end;
	}
	layer->b=vf;
	wtk_source_read_string(src,buf);
	//wtk_debug("[%.*s]=%d\n",buf->pos,buf->data,wtk_dnn_sigmoid);
	if(wtk_str_equal_s(buf->data,buf->pos,"<sigmoid>"))
	{
		layer->type=wtk_fnn_sigmoid;
	}else if(wtk_str_equal_s(buf->data,buf->pos,"<softmax>"))
	{
		layer->type=wtk_fnn_softmax;
	}else if(wtk_str_equal_s(buf->data,buf->pos,"<linear>"))
	{
		layer->type=wtk_fnn_linear;
	}else
	{
		wtk_debug("[%.*s] not support\n",buf->pos,buf->data);
		ret=-1;
		goto end;
	}
	wtk_source_skip_sp(src,NULL);
	ret=wtk_source_read_int(src,&v,1,is_bin);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&v,1,is_bin);
	if(ret!=0){goto end;}
end:
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		if(m)
		{
			wtk_matf_delete(m);
		}
		layer=0;
	}
	return layer;
}


int wtk_mflat_cfg_load_net(wtk_mlat_cfg_t *cfg,wtk_source_t *src)
{
	wtk_mlat_layer_t *layer;
	wtk_strbuf_t *buf;
	int ret;
	int index=0;

	src->swap=0;
	buf=wtk_strbuf_new(256,1);
	while(1)
	{
		layer=wtk_mlat_layer_load(cfg,src,buf);
		//wtk_debug("layer=%p\n",layer);
		if(!layer)
		{
			//wtk_debug("break \n");
			ret=0;goto end;
		}
		layer->index=index;
		wtk_queue_push(&(cfg->layer_q),&(layer->q_n));
		++index;
	}
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

wtk_mlat_layer_t* wtk_mlat_layer_new()
{
	wtk_mlat_layer_t *layer;

	layer=(wtk_mlat_layer_t*)wtk_calloc(1,sizeof(wtk_mlat_layer_t));
	layer->type=WTK_DNN_FLOAT;
	return layer;
}

wtk_mlat_layer_t* wtk_mlat_cfg_read_fix_0_layer(wtk_mlat_cfg_t *cfg,wtk_strbuf_t *buf,wtk_source_t *src)
{
	wtk_mlat_layer_t *layer;
	int ret;
	int rc[2];
	char b;
	int t;

	layer=NULL;
	ret=wtk_source_read_int(src,rc,2,1);
	if(ret!=0){goto end;}
	layer=wtk_mlat_layer_new();
	//wtk_debug("rc=%d/%d\n",rc[0],rc[1]);
	//exit(0);
	layer->w=wtk_matf_new(rc[0],rc[1]);
	ret=wtk_source_read_float(src,layer->w->p,layer->w->row*layer->w->col,1);
	if(ret!=0)
	{
		wtk_debug("read window failed\n");
		goto end;
	}
	b=wtk_source_get(src);
	if(b!=0)
	{
		if(cfg->use_transpose)
		{
			t=rc[1];
		}else
		{
			t=rc[0];
		}
		layer->b=wtk_vecf_new(t);
		ret=wtk_source_read_float(src,layer->b->p,layer->b->len,1);
		if(ret!=0)
		{
			wtk_debug("read bias failed\n");
			goto end;
		}
	}
	ret=wtk_source_read_int(src,rc,1,1);
	if(ret!=0){goto end;}
	//wtk_debug("r=%d\n",rc[0]);
	ret=wtk_source_fill(src,buf->data,rc[0]);
	if(ret!=0){goto end;}
	buf->pos=rc[0];
	//wtk_debug("[%.*s]\n",rc[0],buf->data);
	if(wtk_str_equal_s(buf->data,buf->pos,"<sigmoid>"))
	{
		layer->type=wtk_fnn_sigmoid;
	}else if(wtk_str_equal_s(buf->data,buf->pos,"<softmax>"))
	{
		layer->type=wtk_fnn_softmax;
	}else if(wtk_str_equal_s(buf->data,buf->pos,"<linear>"))
	{
		layer->type=wtk_fnn_linear;
	}else
	{
		wtk_debug("[%.*s] not support\n",buf->pos,buf->data);
		ret=-1;
		goto end;
	}
	//wtk_debug("row=%d col=%d\n",rc[0],rc[1]);
end:
	return layer;
}

wtk_mlat_fix_layer_t* wtk_mlat_fix_layer_new()
{
	wtk_mlat_fix_layer_t *layer;

	layer=(wtk_mlat_fix_layer_t*)wtk_calloc(1,sizeof(wtk_mlat_fix_layer_t));
	return layer;
}

/**
 *	float(1): scale
 *	int(2): row,col
 *	char(row*col): matrix
 *	char(1): has transpose
 *	int(col): transpose
 *	int(1): type string length
 *	char(int):string
 */
wtk_mlat_layer_t* wtk_mlat_cfg_read_fix_char_layer(wtk_mlat_cfg_t *cfg,wtk_strbuf_t *buf,wtk_source_t *src)
{
	wtk_mlat_fix_layer_t *fix_layer;
	wtk_mlat_layer_t *layer;
	int ret;
	int rc[2];
	char b;
	float scale;
	int len;
	int t;

	layer=NULL;
	ret=wtk_source_read_float(src,&scale,1,1);
	if(ret!=0){goto end;}
	layer=wtk_mlat_layer_new();
	//wtk_debug("scale=%f\n",scale);
	fix_layer=wtk_mlat_fix_layer_new();
	fix_layer->scale=scale;
	ret=wtk_source_read_int(src,rc,2,1);
	if(ret!=0){goto end;}
	//wtk_debug("row=%d col=%d\n",rc[0],rc[1]);
	fix_layer->w.c=wtk_matc_new(rc[0],rc[1]);
	len=rc[0]*rc[1];
	ret=wtk_source_fill(src,(char*)(fix_layer->w.c->p),len);
	if(ret!=0){goto end;}
	if(cfg->use_int)
	{
		wtk_matc_t *matc;
		int i;

		matc=fix_layer->w.c;
		fix_layer->w.i=wtk_mati_new(matc->row,matc->col);
		for(i=0;i<len;++i)
		{
			fix_layer->w.i->p[i]=matc->p[i];
		}
		wtk_matc_delete(matc);
	}
	/*
	print_hex((char*)(fix_layer->w.c->p),10);//rc[0]*rc[1]);
	wtk_debug("v[0]=%d\n",fix_layer->w.c->p[0]);
	wtk_debug("v[1]=%d\n",fix_layer->w.c->p[1]);
	wtk_debug("v[2]=%d\n",fix_layer->w.c->p[2]);
	wtk_debug("v[3]=%d\n",fix_layer->w.c->p[3]);
	exit(0);
	*/

	b=wtk_source_get(src);
	if(b!=0)
	{
		t=cfg->use_transpose?rc[1]:rc[0];
		fix_layer->b=wtk_mati_new(1,t);
		ret=wtk_source_fill(src,(char*)(fix_layer->b->p),t*sizeof(int));
		if(ret!=0)
		{
			wtk_debug("read bias failed\n");
			goto end;
		}
	}
	layer->fix_wb=fix_layer;
	ret=wtk_source_read_int(src,rc,1,1);
	if(ret!=0){goto end;}
	//wtk_debug("r=%d/%d\n",rc[0],rc[1]);
	ret=wtk_source_fill(src,buf->data,rc[0]);
	if(ret!=0){goto end;}
	buf->pos=rc[0];
	//wtk_debug("[%.*s]\n",rc[0],buf->data);
	if(wtk_str_equal_s(buf->data,buf->pos,"<sigmoid>"))
	{
		layer->type=wtk_fnn_sigmoid;
	}else if(wtk_str_equal_s(buf->data,buf->pos,"<softmax>"))
	{
		layer->type=wtk_fnn_softmax;
	}else if(wtk_str_equal_s(buf->data,buf->pos,"<linear>"))
	{
		layer->type=wtk_fnn_linear;
	}else
	{
		wtk_debug("[%.*s] not support\n",buf->pos,buf->data);
		ret=-1;
		goto end;
	}
	//wtk_debug("row=%d col=%d\n",rc[0],rc[1]);
end:
	return layer;
}

/**
 *	float(1): scale
 *	int(2): row,col
 *	short(row*col): matrix
 *	char(1): has transpose
 *	int(col): transpose
 *	int(1): type string length
 *	char(int):string
 */
wtk_mlat_layer_t* wtk_mlat_cfg_read_fix_short_layer(wtk_mlat_cfg_t *cfg,wtk_strbuf_t *buf,wtk_source_t *src)
{
	wtk_mlat_fix_layer_t *fix_layer;
	wtk_mlat_layer_t *layer;
	int ret;
	int rc[2];
	char b;
	float scale;
	short *si=NULL,*ps;
	int *pi;
	int i;

	layer=NULL;
	ret=wtk_source_read_float(src,&scale,1,1);
	if(ret!=0){goto end;}
	layer=wtk_mlat_layer_new();
	//wtk_debug("scale=%f\n",scale);
	fix_layer=wtk_mlat_fix_layer_new();
	fix_layer->scale=scale;
	ret=wtk_source_read_int(src,rc,2,1);
	if(ret!=0){goto end;}
	//wtk_debug("row=%d col=%d\n",rc[0],rc[1]);
	fix_layer->w.i=wtk_mati_new(rc[0],rc[1]);
	ret=sizeof(short)*rc[0]*rc[1];
	si=(short*)wtk_malloc(ret);
	ret=wtk_source_fill(src,(char*)si,ret);
	if(ret!=0){goto end;}
	ret=rc[0]*rc[1];
	pi=fix_layer->w.i->p;
	ps=si;
	for(i=0;i<ret;++i)
	{
		*(pi++)=*(ps++);
	}
	/*
	print_hex((char*)(fix_layer->w.c->p),10);//rc[0]*rc[1]);
	wtk_debug("v[0]=%d\n",fix_layer->w.c->p[0]);
	wtk_debug("v[1]=%d\n",fix_layer->w.c->p[1]);
	wtk_debug("v[2]=%d\n",fix_layer->w.c->p[2]);
	wtk_debug("v[3]=%d\n",fix_layer->w.c->p[3]);
	exit(0);
	*/

	b=wtk_source_get(src);
	if(b!=0)
	{
		fix_layer->b=wtk_mati_new(1,rc[1]);
		ret=wtk_source_fill(src,(char*)(fix_layer->b->p),rc[1]*sizeof(int));
		if(ret!=0)
		{
			wtk_debug("read bias failed\n");
			goto end;
		}
	}
	layer->fix_wb=fix_layer;
	ret=wtk_source_read_int(src,rc,1,1);
	if(ret!=0){goto end;}
	//wtk_debug("r=%d/%d\n",rc[0],rc[1]);
	ret=wtk_source_fill(src,buf->data,rc[0]);
	if(ret!=0){goto end;}
	buf->pos=rc[0];
	//wtk_debug("[%.*s]\n",rc[0],buf->data);
	if(wtk_str_equal_s(buf->data,buf->pos,"<sigmoid>"))
	{
		layer->type=wtk_fnn_sigmoid;
	}else if(wtk_str_equal_s(buf->data,buf->pos,"<softmax>"))
	{
		layer->type=wtk_fnn_softmax;
	}else if(wtk_str_equal_s(buf->data,buf->pos,"<linear>"))
	{
		layer->type=wtk_fnn_linear;
	}else
	{
		wtk_debug("[%.*s] not support\n",buf->pos,buf->data);
		ret=-1;
		goto end;
	}
	//wtk_debug("row=%d col=%d\n",rc[0],rc[1]);
end:
	if(si)
	{
		wtk_free(si);
	}
	return layer;
}

int wtk_mlat_cfg_load_fix_net(wtk_mlat_cfg_t *cfg,wtk_source_t *src)
{
	wtk_mlat_layer_t *layer;
	wtk_strbuf_t *buf;
	int ret;
	//int index;
	int i;
	int cnt;

	src->swap=0;
	buf=wtk_strbuf_new(256,1);
	ret=wtk_source_read_int(src,&(cnt),1,1);
	if(ret!=0){goto end;}
	//wtk_debug("cnt=%d\n",cnt);
	//wtk_debug("use_fix0=%d\n",cfg->use_fix_0_layer);
	layer=wtk_mlat_cfg_read_fix_0_layer(cfg,buf,src);
	if(!layer){ret=-1;goto end;}
	layer->float_type=WTK_DNN_FLOAT;
	wtk_queue_push(&(cfg->layer_q),&(layer->q_n));
	for(i=1;i<cnt;++i)
	{
		//wtk_debug("use_c=%d\n",cfg->use_c);
		if(cfg->use_c)
		{
			layer=wtk_mlat_cfg_read_fix_char_layer(cfg,buf,src);
		}else
		{
			layer=wtk_mlat_cfg_read_fix_short_layer(cfg,buf,src);
		}
		if(!layer){ret=-1;goto end;}
		layer->float_type=WTK_DNN_FIX_FLOAT;
		wtk_queue_push(&(cfg->layer_q),&(layer->q_n));
	}
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}



int wtk_mlat_cfg_update2(wtk_mlat_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	if(!cfg->net_fn || !cfg->trans_fn)
	{
		ret=-1;
		goto end;
	}
	ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_mlat_cfg_load_trans,cfg->trans_fn);
	if(ret!=0){goto end;}
	if(cfg->use_fix_res)
	{
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_mlat_cfg_load_fix_net,cfg->net_fn);
	}else
	{
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_mflat_cfg_load_net,cfg->net_fn);
	}
	if(ret!=0){goto end;}
//	//wtk_debug("load trans\n");
	cfg->input_size=cfg->trans->b->len;
	cfg->output_size=wtk_mlat_cfg_out_cols(cfg);
	ret=0;
end:
	if(cfg->skip_frame>0)
	{
		++cfg->skip_frame;
	}
	return ret;
}

int wtk_mlat_cfg_out_cols(wtk_mlat_cfg_t *cfg)
{
	wtk_mlat_layer_t *l;

	if(cfg->layer_q.length<=0){return 0;}
	l=data_offset(cfg->layer_q.push,wtk_mlat_layer_t,q_n);
	if(cfg->use_fix_res)
	{
		//wtk_debug("[%d/%d]\n",l->fix_wb->b->row,l->fix_wb->b->col);
		return l->fix_wb->b->col;
	}else
	{
		return l->b->len;
	}
}
