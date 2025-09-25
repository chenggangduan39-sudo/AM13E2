#include "wtk_qlas_cfg.h" 

wtk_qlas_trans_t* wtk_qlas_trans_new(int n)
{
	wtk_qlas_trans_t  *t;

	t=(wtk_qlas_trans_t*)wtk_malloc(sizeof(wtk_qlas_trans_t));
	t->bias=wtk_vecf_new(n);
	t->win=wtk_vecf_new(n);
	return t;
}

void wtk_qlas_trans_delete(wtk_qlas_trans_t *t)
{
		if(t->bias)
		{
			wtk_vecf_delete(t->bias);
		}
		if(t->win)
		{
			wtk_vecf_delete(t->win);
		}
		wtk_free(t);
}

wtk_qlas_layer_t*  wtk_qlas_layer_new_fixchar(wtk_fnn_post_type_t type,float scale,int row,int col)
{
	wtk_qlas_layer_t *layer;

	layer=(wtk_qlas_layer_t*)wtk_malloc(sizeof(wtk_qlas_layer_t));
	layer->type=type;
	layer->win=NULL;
	layer->fixwin.bwin=wtk_matb_new(row,col);
	layer->fixwin.bwin->scale=scale;
	layer->bias=wtk_vecf_new(row);
    layer->tmp=NULL;

    if(layer->type==wtk_fnn_relu)
    {
        layer->tmp=wtk_vecf_new(layer->bias->len);
    }

	return layer;
}

wtk_qlas_layer_t*  wtk_qlas_layer_new_fixshort(wtk_fnn_post_type_t type,float scale,int row,int col)
{
	wtk_qlas_layer_t *layer;

	layer=(wtk_qlas_layer_t*)wtk_malloc(sizeof(wtk_qlas_layer_t));
	layer->type=type;
	layer->win=NULL;
	layer->fixwin.swin=wtk_mats_new(row,col);
	layer->fixwin.swin->scale=scale;
	layer->bias=wtk_vecf_new(row);
	layer->tmp=NULL;

	if(layer->type==wtk_fnn_sigmoid_normal)
	{
        layer->tmp=wtk_vecf_new(layer->bias->len);
	}
	return layer;
}

void wtk_qlas_layer_delete(wtk_qlas_layer_t *layer)
{
	if(layer->win)
	{
		wtk_matf_delete(layer->win);
	}
	if(layer->bias)
	{
		wtk_vecf_delete(layer->bias);
	}
    if(layer->tmp)
    {
        wtk_vecf_delete(layer->tmp);
    }
	wtk_free(layer);
}


int wtk_qlas_cfg_update_fix(wtk_qlas_cfg_t *cfg)
{
	wtk_queue_node_t *qn;
	wtk_qlas_layer_t *l;

	for(qn=cfg->layer_q.pop;qn;qn=qn->next)
	{
		l=data_offset2(qn,wtk_qlas_layer_t,q_n);
		if(cfg->use_char)
		{
			l->fixwin.bwin=wtk_matb_new(l->win->row,l->win->col);
			wtk_matb_fix_matf(l->fixwin.bwin,l->win,cfg->max_w);
		}else
		{
			l->fixwin.swin=wtk_mats_new(l->win->row,l->win->col);
			wtk_mats_fix_matf(l->fixwin.swin,l->win,cfg->max_w);
		}
		//wtk_matb_print(l->cwin);
		//exit(0);
	}
	return 0;
}

int wtk_qlas_cfg_init(wtk_qlas_cfg_t *cfg)
{
	cfg->use_bin=0;
	cfg->net_fn=NULL;
	cfg->trans_fn=NULL;
	cfg->transf=NULL;
	cfg->max_w=1024;
	cfg->max_w=127;
	cfg->use_char=1;
	cfg->use_fix_res=0;
	cfg->use_fix=0;
	cfg->cache_size=1;
	wtk_queue_init(&(cfg->layer_q));
	return 0;
}


int wtk_qlas_cfg_clean(wtk_qlas_cfg_t *cfg)
{
	wtk_queue_node_t *qn;
	wtk_qlas_layer_t *l;

	if(cfg->transf)
	{
		wtk_qlas_trans_delete(cfg->transf);
	}
	while(1)
	{
		qn=wtk_queue_pop(&(cfg->layer_q));
		if(!qn){break;}
		l=data_offset2(qn,wtk_qlas_layer_t,q_n);
		if(cfg->use_fix)
		{
			if(cfg->use_char)
			{
				wtk_matb_delete(l->fixwin.bwin);
			}else
			{
				wtk_mats_delete(l->fixwin.swin);
			}
		}
		wtk_qlas_layer_delete(l);
	}
	return 0;
}

int wtk_qlas_cfg_update_local(wtk_qlas_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	int ret;

	wtk_local_cfg_update_cfg_f(lc,cfg,max_w,v);
	//wtk_debug("%d\n",cfg->max_w);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_char,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fix_res,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fix,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,net_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,trans_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,cache_size,v);
	ret=0;
	return ret;
}

int wtk_qlas_cfg_update(wtk_qlas_cfg_t *cfg)
{
	wtk_source_loader_t file_sl;

	file_sl.hook=0;
	file_sl.vf=wtk_source_load_file_v;
	return wtk_qlas_cfg_update2(cfg,&file_sl);
}

wtk_vecf_t* wtk_qlas_cfg_load_trans_vector(wtk_source_t *src,wtk_strbuf_t *buf,char *name,int is_bin)
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

void wtk_qlas_cfg_update_transf_bias(wtk_qlas_trans_t *transf)
{
	float *pf1,*pf2;
	int i,n;

	pf1=transf->bias->p;
	pf2=transf->win->p;
	n=transf->bias->len;
	for(i=0;i<n;++i)
	{
		pf1[i]*=pf2[i];
	}
}


int wtk_qlas_cfg_load_file_transf(wtk_qlas_cfg_t *cfg,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	int ret=-1;
	wtk_qlas_trans_t *transf=NULL;

	src->swap=0;
	buf=wtk_strbuf_new(256,1);
	transf=(wtk_qlas_trans_t*)wtk_malloc(sizeof(wtk_qlas_trans_t));
	transf->bias=NULL;
	transf->win=NULL;
	transf->bias=wtk_qlas_cfg_load_trans_vector(src,buf,"<bias>",cfg->use_bin);
	//wtk_debug("%d\n",transf->bias->len);
	if(!transf->bias){goto end;}
	transf->win=wtk_qlas_cfg_load_trans_vector(src,buf,"<window>",cfg->use_bin);
	if(!transf->win){goto end;}
	cfg->transf=transf;
	wtk_qlas_cfg_update_transf_bias(cfg->transf);
	transf=NULL;
	ret=0;
end:
	if(transf)
	{
		wtk_qlas_trans_delete(transf);
	}
	wtk_strbuf_delete(buf);
	return ret;
}

wtk_qlas_layer_t* wtk_qlas_layer_load(wtk_qlas_cfg_t *cfg,wtk_source_t *src,wtk_strbuf_t *buf,int is_bin,wtk_qlas_layer_t *last_layer)
{
	wtk_qlas_layer_t *layer;
	int row,col;
	int ret;

	layer=(wtk_qlas_layer_t*)wtk_malloc(sizeof(wtk_qlas_layer_t));
	layer->bias=NULL;
	layer->win=NULL;
	layer->tmp=NULL;
	ret=wtk_source_read_string(src,buf);
	if(ret!=0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	//read  <biasedlinearity> 256 396
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if(wtk_str_equal_s(buf->data,buf->pos,"<Normalize>"))
	{
		wtk_source_skip_sp(src,NULL);
		ret=wtk_source_read_int(src,&row,1,is_bin);
		if(ret!=0){goto end;}
		ret=wtk_source_read_int(src,&col,1,is_bin);
		//wtk_debug("row=%d, col=%d\n",row,col);
		wtk_source_read_string(src,buf);
		if(last_layer&&last_layer->type==wtk_fnn_sigmoid)
		{
			//wtk_debug("normal\n");
			last_layer->type=wtk_fnn_sigmoid_normal;
		}else
		{
			ret=-1;
			goto end;
		}
	}else if(!wtk_str_equal_s(buf->data,buf->pos,"<biasedlinearity>"))
	{
		ret=-1;
		goto end;
	}
	wtk_source_skip_sp(src,NULL);
	ret=wtk_source_read_int(src,&row,1,is_bin);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&col,1,is_bin);
	if(ret!=0){goto end;}
	//read m 256 396
	ret=wtk_source_read_string(src,buf);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&row,1,is_bin);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&col,1,is_bin);
	if(ret!=0){goto end;}
	layer->win=wtk_matf_new(row,col);
	ret=wtk_source_read_float(src,layer->win->p,row*col,is_bin);
	if(ret!=0){goto end;}
	ret=wtk_source_read_string(src,buf);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&row,1,is_bin);
	if(ret!=0){goto end;}
	layer->bias=wtk_vecf_new(row);
	ret=wtk_source_read_float(src,layer->bias->p,row,is_bin);
	if(ret!=0){goto end;}
	ret=wtk_source_read_string(src,buf);
	if(ret!=0){goto end;}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
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
	layer->max_bias=wtk_float_abs_max(layer->bias->p,layer->bias->len);
	wtk_source_skip_sp(src,NULL);
	ret=wtk_source_read_int(src,&row,1,is_bin);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&row,1,is_bin);
	if(ret!=0){goto end;}
	ret=0;
end:
	if(ret!=0)
	{
		wtk_qlas_layer_delete(layer);
		layer=NULL;
	}
	return layer;
}

int wtk_qlas_cfg_load_file_net(wtk_qlas_cfg_t *cfg,wtk_source_t *src,int is_bin)
{
	wtk_strbuf_t *buf;
	wtk_qlas_layer_t *layer,*last_layer;
	int ret=-1;

	src->swap=0;
	buf=wtk_strbuf_new(256,1);
	last_layer=NULL;
	while(1)
	{
		layer=wtk_qlas_layer_load(cfg,src,buf,cfg->use_bin,last_layer);
		if(!layer){ret=0;goto end;}
		last_layer=layer;
		wtk_queue_push(&(cfg->layer_q),&(layer->q_n));
	}
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

int wtk_qlas_cfg_load_fix_res(wtk_qlas_cfg_t *cfg,wtk_source_t *src);



void wtk_qlas_trans_print(wtk_qlas_trans_t *transf)
{
	print_float(transf->win->p,transf->win->len);
	print_float(transf->bias->p,transf->bias->len);
}

void wtk_qlas_layer_print(wtk_qlas_layer_t *layer)
{
	print_float(layer->bias->p,layer->bias->len);
	print_short2(layer->fixwin.swin->p,layer->fixwin.swin->row*layer->fixwin.swin->col);
	exit(0);
}

void wtk_qlas_cfg_print(wtk_qlas_cfg_t *cfg)
{
	wtk_queue_node_t *qn;
	wtk_qlas_layer_t *layer;

	wtk_qlas_trans_print(cfg->transf);
	for(qn=cfg->layer_q.pop;qn;qn=qn->next)
	{
		layer=data_offset2(qn,wtk_qlas_layer_t,q_n);
		wtk_qlas_layer_print(layer);
	}
}

int wtk_qlas_cfg_load_file(wtk_qlas_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	if(cfg->use_fix_res)
	{
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_qlas_cfg_load_fix_res,cfg->net_fn);
		if(ret!=0){goto end;}
	}else
	{
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_qlas_cfg_load_file_transf,cfg->trans_fn);
		if(ret!=0){goto end;}
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_qlas_cfg_load_file_net,cfg->net_fn);
		if(ret!=0){goto end;}
		if(cfg->use_fix)
		{
			ret=wtk_qlas_cfg_update_fix(cfg);
			if(ret!=0){goto end;}
		}
	}
	//wtk_qlas_cfg_print(cfg);
end:
	//exit(0);
	return ret;
}

int wtk_qlas_cfg_update2(wtk_qlas_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	ret=wtk_qlas_cfg_load_file(cfg,sl);
	if(ret!=0){goto end;}
end:
	return ret;
}

int wtk_qlas_cfg_out_cols(wtk_qlas_cfg_t *cfg)
{
	wtk_qlas_layer_t *l;

	if(cfg->layer_q.length<=0){return 0;}
	l=data_offset(cfg->layer_q.push,wtk_qlas_layer_t,q_n);
	return  l->bias->len;
}

void wtk_qlas_cfg_write_float(FILE *f,float *data,int len)
{
	fwrite((char*)&len,4,1,f);
	fwrite((char*)data,4,len,f);
}

void wtk_qlas_cfg_write_matb(FILE *f,wtk_matb_t *mb)
{
	fwrite(&(mb->scale),4,1,f);
	fwrite(&(mb->row),4,1,f);
	fwrite(&(mb->col),4,1,f);
	fwrite(mb->p,1,mb->row*mb->col,f);
	//fwrite((char*)&len,1,4,f);
	//fwrite((char*)data,4,len,f);
}

void wtk_qlas_cfg_write_mats(FILE *f,wtk_mats_t *ms)
{
	fwrite(&(ms->scale),4,1,f);
	fwrite(&(ms->row),4,1,f);
	fwrite(&(ms->col),4,1,f);
	fwrite(ms->p,2,ms->row*ms->col,f);
	//print_short(ms->p,20);//ms->row*ms->col);
	//exit(0);
}

void wtk_qlas_cfg_write_fix_bin(wtk_qlas_cfg_t *cfg,char *fn)
{
	wtk_qlas_trans_t *transf=cfg->transf;
	wtk_queue_node_t *qn;
	wtk_qlas_layer_t *layer;
	FILE *f;
	int i;

	wtk_debug("use_char=%d max_w=%f\n",cfg->use_char,cfg->max_w);
	f=fopen(fn,"wb");
	i=cfg->use_char;
	fwrite(&i,4,1,f);
	fwrite(&(cfg->max_w),4,1,f);
	wtk_qlas_cfg_write_float(f,transf->win->p,transf->win->len);
	wtk_qlas_cfg_write_float(f,transf->bias->p,transf->bias->len);
	fwrite((char*)&(cfg->layer_q.length),4,1,f);
	for(qn=cfg->layer_q.pop;qn;qn=qn->next)
	{
		layer=data_offset2(qn,wtk_qlas_layer_t,q_n);
		i=layer->type;
		fwrite(&i,4,1,f);
		wtk_debug("type=%d %f %d/%d\n",i,layer->fixwin.bwin->scale,layer->fixwin.bwin->row,layer->fixwin.bwin->col);
		if(cfg->use_char)
		{
			wtk_qlas_cfg_write_matb(f,layer->fixwin.bwin);
		}else
		{
			wtk_qlas_cfg_write_mats(f,layer->fixwin.swin);
		}
		wtk_qlas_cfg_write_float(f,layer->bias->p,layer->bias->len);
	}
	fclose(f);
}

int wtk_qlas_cfg_load_fix_res(wtk_qlas_cfg_t *cfg,wtk_source_t *src)
{
	int ret=-1;
	int i,n,x;
	wtk_fnn_post_type_t type;
	float scale;
	int row,col;
	wtk_qlas_layer_t *layer;

	src->swap=0;
	cfg->use_fix=1;
	ret=wtk_source_fill(src,(char*)&i,4);
	if(ret!=0){goto end;}
	cfg->use_char=i;
	ret=wtk_source_fill(src,(char*)&scale,4);
	if(ret!=0){goto end;}
	cfg->max_w=scale;
	//wtk_debug("use_char=%d max=%f\n",cfg->use_char,cfg->max_w);
	//printf("use_char=%d max=%f\n",cfg->use_char,cfg->max_w);
	ret=wtk_source_fill(src,(char*)&i,4);
	if(ret!=0){goto end;}
	cfg->transf=wtk_qlas_trans_new(i);
	ret=wtk_source_fill(src,(char*)(cfg->transf->win->p),i*sizeof(float));
	if(ret!=0){goto end;}
	ret=wtk_source_fill(src,(char*)&i,4);
	if(ret!=0){goto end;}
	ret=wtk_source_fill(src,(char*)(cfg->transf->bias->p),i*sizeof(float));
	if(ret!=0){goto end;}
	ret=wtk_source_fill(src,(char*)&i,4);
	if(ret!=0){goto end;}
	n=i;
	//wtk_debug("n=%d\n",n);
	for(i=0;i<n;++i)
	{
		ret=wtk_source_fill(src,(char*)&x,4);
		if(ret!=0){goto end;}
		type=x;
		ret=wtk_source_fill(src,(char*)&scale,4);
		if(ret!=0){goto end;}
		ret=wtk_source_fill(src,(char*)&row,4);
		if(ret!=0){goto end;}
		ret=wtk_source_fill(src,(char*)&col,4);
		if(ret!=0){goto end;}
		//wtk_debug("type=%d sclae=%f row=%d col=%d\n",type,scale,row,col);
		if(cfg->use_char)
		{
			layer=wtk_qlas_layer_new_fixchar(type,scale,row,col);
			ret=wtk_source_fill(src,(char*)(layer->fixwin.bwin->p),row*col);
		}else
		{
			layer=wtk_qlas_layer_new_fixshort(type,scale,row,col);
			ret=wtk_source_fill(src,(char*)(layer->fixwin.swin->p),row*col*2);
		}
		if(ret!=0){goto end;}
		//print_short2(layer->fixwin.swin->p,row*col*2);
		//exit(0);
		wtk_queue_push(&(cfg->layer_q),&(layer->q_n));
		ret=wtk_source_fill(src,(char*)&x,4);
		if(ret!=0){goto end;}
		ret=wtk_source_fill(src,(char*)(layer->bias->p),x*sizeof(float));
		if(ret!=0){goto end;}
		layer->max_bias=wtk_float_abs_max(layer->bias->p,layer->bias->len);
		//wtk_debug("ret=%d\n",ret);
	}
end:
	//exit(0);
	return ret;
}
