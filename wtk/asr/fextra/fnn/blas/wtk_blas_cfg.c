#ifdef USE_BLAS
#include "wtk_blas_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include <stdlib.h>

void wtk_blas_vector_mult(wtk_blas_vector_t* src,wtk_blas_vector_t *dst)
{
#ifdef USE_SIMPLE
	int i;

	for(i=0;i<src->len;++i)
	{
		src->v[i]*=dst->v[i];
	}
#else
	float *s0,*e0;
	float *s1;

	s0=src->v-1;
	e0=s0+src->len;
	s1=dst->v-1;
	while(s0<e0)
	{
		*(++s0)*=*(++s1);
	}
#endif
}

void wtk_blas_vector_mult2(float* src,int len,wtk_blas_vector_t *dst)
{
	float *s0,*e0;
	float *s1;

	s0=src;
	e0=s0+len;
	s1=dst->v-1;
	while(s0<e0)
	{
		*(s0)*=*(++s1);
		s0++;
	}
}

/*
void wtk_dnn_trans_process(wtk_dnn_trans_t *l,wtk_vector_t *v)
{
	int n;
	float *pv,*pve,*pb,*pm;

	n=wtk_vector_size(v);
	pv=&(v[1]);
	pve=pv+n;
	pb=&(l->b[1]);
	pm=&(l->m[1]);
	while(pv<pve)
	{
		*pv=(*pv+*(pb++))*(*(pm++));
		++pv;
	}
	//exit(0);
}
*/

wtk_blas_vector_t* wtk_blas_vector_new(int align,int n)
{
	wtk_blas_vector_t *v;
	int ret;

	v=(wtk_blas_vector_t*)wtk_malloc(sizeof(*v));
	v->len=n;
	v->bytes=v->len*sizeof(float);
	//v->v=(float*)memalign(align,n*sizeof(float));
	v->v=0;
#ifdef _WIN32
        ret = 0;
        v->v = wtk_malloc(n * sizeof(floaat));
#else
	ret=posix_memalign((void**)&(v->v), align, n*sizeof(float));
#endif
	if(ret!=0)
	{
		wtk_free(v);
		v=0;
	}
	return v;
}

void wtk_blas_vector_delete(wtk_blas_vector_t *v)
{
	wtk_free(v->v);
	wtk_free(v);
}

int wtk_blas_vector_bytes(wtk_blas_vector_t *v)
{
	int bytes=sizeof(wtk_blas_vector_t);

	bytes+=v->len*sizeof(float);
	return bytes;
}

void wtk_blas_vector_print(wtk_blas_vector_t *v)
{
	int i;

	wtk_debug("================ blas vector ================\n");
	for(i=0;i<v->len;++i)
	{
		printf("v[%d]=%f\n",i,v->v[i]);
	}
}

wtk_blas_matrix_t* wtk_blas_matrix_new(int align,int row,int col)
{
	wtk_blas_matrix_t *m;
	int ret;

	m=wtk_malloc(sizeof(*m));
	m->row=row;
	m->col=col;
    //m->m=(float*)memalign(align,row*col*sizeof(float));
	m->m=0;
#ifdef _WIN32
        m->m = wtk_malloc(row * col * sizeof(float));
#else
	ret=posix_memalign((void**)&(m->m), align, row*col*sizeof(float));
	if(ret!=0)
	{
		wtk_free(m);
		m=0;
	}
#endif
	return m;
}

int wtk_blas_matrix_bytes(wtk_blas_matrix_t *m)
{
	int bytes=sizeof(wtk_blas_matrix_t);

	bytes+=m->row*m->col*sizeof(float);
	return bytes;
}

void wtk_blas_matrix_delete(wtk_blas_matrix_t *m)
{
	wtk_free(m->m);
	wtk_free(m);
}

/*
double wtk_blas_matrix_get(wtk_blas_matrix_t *m,int row,int col)
{
	int index;

	index=m->col*row+col;
	return m->m[index];
}*/

void wtk_blas_matrix_transpose(wtk_blas_matrix_t *src,wtk_blas_matrix_t *dst)
{
	int i,j;

	for(i=0;i<dst->row;++i)
	{
		for(j=0;j<dst->col;++j)
		{
			dst->m[wtk_blas_matrix_index(dst,i,j)]=src->m[wtk_blas_matrix_index(src,j,i)];
		}
	}
}

wtk_blas_layer_t* wtk_blas_layer_new()
{
	wtk_blas_layer_t *l;

	l=(wtk_blas_layer_t*)wtk_malloc(sizeof(*l));
	l->w=0;
	l->b=0;
	l->rescale=0;
	l->in_dim=0;
	l->out_dim=0;
	return l;
}

void wtk_blas_layer_delete(wtk_blas_layer_t *l)
{
	if(l->w)
	{
		wtk_blas_matrix_delete(l->w);
	}
	if(l->b)
	{
		wtk_blas_vector_delete(l->b);
		//wtk_blas_matrix_delete(l->b);
	}
	if(l->rescale)
	{
		wtk_blas_vector_delete(l->rescale);
	}
	wtk_free(l);
}

int wtk_blas_layer_bytes(wtk_blas_layer_t *l)
{
	int bytes=sizeof(wtk_blas_layer_t);

	if(l->w)
	{
		bytes+=wtk_blas_matrix_bytes(l->w);
	}
	if(l->b)
	{
		bytes+=wtk_blas_vector_bytes(l->b);
	}
	return bytes;
}

void wtk_blas_layer_print(wtk_blas_layer_t *l)
{
	wtk_debug("================ layer =================\n");
	printf("type: %s\n",wtk_fnn_post_type_str(l->type));
	printf("bias: [%d]\n",l->b->len);
	printf("win: [%d,%d]\n",l->w->row,l->w->col);
}

wtk_blas_trans_t*  wtk_blas_trans_new()
{
	wtk_blas_trans_t *t;

	t=wtk_malloc(sizeof(*t));
	t->b=0;
	t->w=0;
	return t;
}

void wtk_blas_trans_delete(wtk_blas_trans_t *t)
{
	if(t->b)
	{
		wtk_blas_vector_delete(t->b);
	}
	if(t->w)
	{
		wtk_blas_vector_delete(t->w);
	}
	wtk_free(t);
}

int wtk_blas_trans_bytes(wtk_blas_trans_t *t)
{
	int bytes=0;

	if(t->b)
	{
		bytes+=wtk_blas_vector_bytes(t->b);
	}
	if(t->w)
	{
		bytes+=wtk_blas_vector_bytes(t->w);
	}
	return bytes;
}


int wtk_blas_cfg_init(wtk_blas_cfg_t *cfg)
{
	cfg->net_fn=0;
	cfg->trans_fn=0;
    cfg->last_trans_fn=0;
	cfg->align=4096;
	cfg->trans=0;
	cfg->expand_trans=0;
	cfg->cache_size=1;
	cfg->in_cols=0;
	cfg->max_row=0;
	cfg->max_col=0;
	cfg->is_bin=0;
    cfg->last_trans=0;
	wtk_queue_init(&(cfg->layer_q));
	return 0;
}

int wtk_blas_cfg_clean(wtk_blas_cfg_t *cfg)
{
	wtk_queue_node_t *n;
	wtk_blas_layer_t *l;

	while(1)
	{
		n=wtk_queue_pop(&(cfg->layer_q));
		if(!n){break;}
		l=data_offset(n,wtk_blas_layer_t,q_n);
		wtk_blas_layer_delete(l);
	}
	if(cfg->trans)
	{
		wtk_blas_trans_delete(cfg->trans);
	}
	if(cfg->expand_trans)
	{
		wtk_blas_trans_delete(cfg->expand_trans);
	}
    if(cfg->last_trans)
    {
        wtk_vector_delete(cfg->last_trans);
    }
	return 0;
}

int wtk_blas_cfg_bytes(wtk_blas_cfg_t *cfg)
{
	int bytes=0;
	wtk_queue_node_t *n;
	wtk_blas_layer_t *l;

	for(n=cfg->layer_q.pop;n;n=n->next)
	{
		l=data_offset(n,wtk_blas_layer_t,q_n);
		bytes+=wtk_blas_layer_bytes(l);
	}
	if(cfg->trans)
	{
		bytes+=wtk_blas_trans_bytes(cfg->trans);
	}
	if(cfg->expand_trans)
	{
		bytes+=wtk_blas_trans_bytes(cfg->expand_trans);
	}
	return bytes;
}

int wtk_blas_cfg_update_local(wtk_blas_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,net_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,trans_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,last_trans_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,align,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,cache_size,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,is_bin,v);
	return 0;
}

wtk_blas_matrix_t* wtk_blas_load_matrix(wtk_source_t *src,wtk_strbuf_t *buf,int align,int row,int col,int bin)
{
	wtk_blas_matrix_t *m,*m2;
	int ret;

	m=wtk_blas_matrix_new(align,row,col);
	ret=wtk_source_read_float(src,m->m,row*col,bin);
	if(ret!=0)
	{
		wtk_debug("read float failed(r=%d c=%d)\n",row,col);
		goto end;
	}
	m2=wtk_blas_matrix_new(align,col,row);
	wtk_blas_matrix_transpose(m,m2);
	wtk_blas_matrix_delete(m);
	m=m2;
end:
	if(ret!=0 && m)
	{
		wtk_blas_matrix_delete(m);
		m=0;
	}
	return m;
}


wtk_blas_layer_t* wtk_blas_cfg_load_layer(wtk_blas_cfg_t *cfg,wtk_source_t *src)
{
	wtk_blas_layer_t *l=0;
	wtk_strbuf_t *buf;
	int row,col,v;
	int ret=-1;
	int is_bin=cfg->is_bin;

	buf=wtk_strbuf_new(256,1);
	l=wtk_blas_layer_new();
	//<biasedlinearity> 2048 429
	ret=wtk_source_read_string(src,buf);
	if(ret!=0){goto end;}
	if(!wtk_str_equal_s(buf->data,buf->pos,"<biasedlinearity>"))
	{
		wtk_debug("[%.*s] not support.\n",buf->pos,buf->data);
		ret=-1;
		goto end;
	}
	if(is_bin)
	{
		wtk_source_skip_sp3(src,NULL);
	}else
	{
		wtk_source_skip_sp(src,NULL);
	}
	ret=wtk_source_read_int(src,&row,1,is_bin);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&col,1,is_bin);
	if(ret!=0){goto end;}
	//wtk_debug("row=%d,col=%d\n",row,col);
	ret=wtk_source_read_string(src,buf);
	if(ret!=0){goto end;}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if(!wtk_str_equal_s(buf->data,buf->pos,"m"))
	{
		wtk_debug("[%.*s] not support.\n",buf->pos,buf->data);
		ret=-1;
		goto end;
	}
	if(is_bin)
	{
		wtk_source_skip_sp3(src,NULL);
	}else
	{
		wtk_source_skip_sp(src,NULL);
	}
	ret=wtk_source_read_int(src,&v,1,is_bin);
	if(ret!=0){goto end;}
	//wtk_debug("v=%d\n",v);
	if(v!=row)
	{
		wtk_debug("row[%d]!=v[%d]\n",row,v);
		ret=-1;
		goto end;
	}
	ret=wtk_source_read_int(src,&v,1,is_bin);
	if(ret!=0){goto end;}
	//wtk_debug("v=%d\n",v);
	if(v!=col)
	{
		wtk_debug("col[%d]!=v[%d]\n",col,v);
		ret=-1;
		goto end;
	}
	if(row>cfg->max_row)
	{
		cfg->max_row=row;
	}
	if(col>cfg->max_col)
	{
		cfg->max_col=col;
	}
	l->w=wtk_blas_load_matrix(src,buf,cfg->align,row,col,is_bin);
	if(!l->w)
	{
		wtk_debug("load matrix failed.\n");
		goto end;
	}
	wtk_source_read_string(src,buf);
	if(!wtk_str_equal_s(buf->data,buf->pos,"v"))
	{
		wtk_debug("[%.*s] not support.\n",buf->pos,buf->data);
		ret=-1;goto end;
	}
	if(is_bin)
	{
		wtk_source_skip_sp3(src,NULL);
	}else
	{
		wtk_source_skip_sp(src,NULL);
	}
	ret=wtk_source_read_int(src,&v,1,is_bin);
	if(ret!=0){goto end;}
	if(v!=row){ret=-1;goto end;}
	//wtk_debug("v=%d\n",v);
	l->b=wtk_blas_vector_new(cfg->align,v);
	ret=wtk_source_read_float(src,l->b->v,v,is_bin);
	if(ret!=0){goto end;}
	wtk_source_read_string(src,buf);
	if(is_bin)
	{
		wtk_source_skip_sp3(src,NULL);
	}else
	{
		wtk_source_skip_sp(src,NULL);
	}
	if(wtk_str_equal_s(buf->data,buf->pos,"<sigmoid>"))
	{
		l->type=wtk_fnn_sigmoid;
	}else if(wtk_str_equal_s(buf->data,buf->pos,"<softmax>"))
	{
		l->type=wtk_fnn_softmax;
	}else if(wtk_str_equal_s(buf->data,buf->pos,"<linear>"))
	{
		l->type=wtk_fnn_linear;
	}else if(wtk_str_equal_s(buf->data,buf->pos,"<Pnorm>"))
	{
//		wtk_debug("PNORM \n");
		l->type=wtk_fnn_pnorm;
		ret=wtk_source_read_int(src,&v,1,is_bin);
		l->out_dim=v;
		if(ret!=0){goto end;}
		ret=wtk_source_read_int(src,&v,1,is_bin);
		l->in_dim=v;
		wtk_source_read_string(src,buf);
		if(!wtk_str_equal_s(buf->data,buf->pos,"<Normalize>"))
		{
			ret=-1;
			goto end;
		}
	}else if(wtk_str_equal_s(buf->data,buf->pos,"<Rescale>"))
	{
		l->type=wtk_fnn_rescale;
		ret=wtk_source_read_int(src,&v,1,is_bin);
		if(ret!=0){goto end;}
		ret=wtk_source_read_int(src,&v,1,is_bin);
		if(ret!=0){goto end;}

	//	l->rescale=wtk_blas_vector_new(cfg->align,v);
	//	ret=wtk_source_read_float(src,l->rescale->v,v,is_bin);

		wtk_source_read_string(src,buf);
		if(!wtk_str_equal_s(buf->data,buf->pos,"<softmax>"))
		{
			ret=-1;
			goto end;
		}
		ret=wtk_source_read_int(src,&v,1,is_bin);
		if(ret!=0){goto end;}
		ret=wtk_source_read_int(src,&v,1,is_bin);
		if(ret!=0){goto end;}
		
		l->rescale=wtk_blas_vector_new(cfg->align,v);
		ret=wtk_source_read_float(src,l->rescale->v,v,is_bin);
		goto end;
	}else
	{
		wtk_debug("[%.*s] not support\n",buf->pos,buf->data);
		ret=-1;
		goto end;
	}
	ret=wtk_source_read_int(src,&v,1,is_bin);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&v,1,is_bin);
	if(ret!=0){goto end;}
end:
	if(ret!=0)
	{
		wtk_blas_layer_delete(l);
		l=0;
	}
	wtk_strbuf_delete(buf);
	return l;
}


int wtk_blas_cfg_load_net(wtk_blas_cfg_t *cfg,wtk_source_t *src)
{
	wtk_blas_layer_t *l;
	int ret=0;

	src->swap=0;
	while(1)
	{
		//wtk_debug("read layer\n");
		l=wtk_blas_cfg_load_layer(cfg,src);
		if(!l){goto end;}
		//wtk_blas_layer_print(l);
		wtk_queue_push(&(cfg->layer_q),&(l->q_n));
	}
end:
	//exit(0);
	return ret;
}

wtk_blas_vector_t* wtk_blas_load_trans_vector(wtk_source_t *src,wtk_strbuf_t *buf,
		char *name,int align,int bin)
{
	wtk_blas_vector_t *v=0;
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
	if(bin)
	{
		wtk_source_skip_sp3(src,NULL);
	}else
	{
		wtk_source_skip_sp(src,NULL);
	}
	ret=wtk_source_read_int(src,&i,1,bin);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&i,1,bin);
	if(ret!=0){goto end;}
	ret=wtk_source_read_string(src,buf);
	if(ret!=0){goto end;}
	if(bin)
	{
		wtk_source_skip_sp3(src,NULL);
	}else
	{
		wtk_source_skip_sp(src,NULL);
	}
	ret=wtk_source_read_int(src,&i,1,bin);
	if(ret!=0){goto end;}
	//wtk_debug("i=%d\n",i);
	v=wtk_blas_vector_new(align,i);
	ret=wtk_source_read_float(src,v->v,v->len,bin);
	if(ret!=0){goto end;}
	//v->bytes=v->len*sizeof(float);
end:
	if(ret!=0 && v)
	{
		wtk_blas_vector_delete(v);
		v=0;
	}
	return v;
}

int wtk_blas_cfg_load_last_trans(wtk_blas_cfg_t *cfg,wtk_source_t *src)
{
    int ret=-1;
    int num;
    //wtk_debug("load last trans\n");
    ret=wtk_source_read_int(src,&num,1,cfg->is_bin);
    cfg->last_trans=wtk_vector_new(num);
    ret=wtk_source_read_vector(src,cfg->last_trans,cfg->is_bin);

    return ret;
}

int wtk_blas_cfg_load_trans(wtk_blas_cfg_t *cfg,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	wtk_blas_trans_t *trans;
	int ret=-1;

	src->swap=0;
	trans=wtk_blas_trans_new();
	buf=wtk_strbuf_new(256,1);
	//wtk_debug("buf=%p\n",buf);
	trans->b=wtk_blas_load_trans_vector(src,buf,"<bias>",cfg->align,cfg->is_bin);
	if(!trans->b)
	{
		goto end;
	}
	trans->w=wtk_blas_load_trans_vector(src,buf,"<window>",cfg->align,cfg->is_bin);
	if(!trans->w){goto end;}
	cfg->trans=trans;
	wtk_blas_vector_mult(trans->b,trans->w);
	ret=0;
end:
	//wtk_debug("ret=%d,bias=%d,window=%d\n",ret,trans->b->len,trans->w->len);
	if(ret!=0 && trans)
	{
		wtk_blas_trans_delete(trans);
	}
	wtk_strbuf_delete(buf);
	return ret;
}

void wtk_blas_cfg_update_trans(wtk_blas_cfg_t *cfg)
{
	wtk_blas_trans_t *t;
	int i;

	if(cfg->cache_size==1)
	{
		cfg->expand_trans=cfg->trans;
		cfg->trans=0;
	}else if(cfg->cache_size>1)
	{
		t=wtk_blas_trans_new();
		t->b=wtk_blas_vector_new(cfg->align,cfg->cache_size*cfg->trans->b->len);
		t->w=wtk_blas_vector_new(cfg->align,cfg->cache_size*cfg->trans->w->len);
		for(i=0;i<cfg->cache_size;++i)
		{
			memcpy(t->b->v+i*cfg->trans->b->len,cfg->trans->b->v,sizeof(float)*cfg->trans->b->len);
			memcpy(t->w->v+i*cfg->trans->w->len,cfg->trans->w->v,sizeof(float)*cfg->trans->w->len);
		}
		wtk_blas_trans_delete(cfg->trans);
		cfg->trans=0;
		cfg->expand_trans=t;
	}
}

int wtk_blas_cfg_update2(wtk_blas_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	if(cfg->cache_size<=0)
	{
		cfg->cache_size=1;
	}
	if(!cfg->net_fn || !cfg->trans_fn)
	{
		ret=-1;goto end;
	}
	ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_blas_cfg_load_trans,cfg->trans_fn);
	if(ret!=0){goto end;}
	ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_blas_cfg_load_net,cfg->net_fn);
	if(ret!=0){goto end;}
    if(cfg->last_trans_fn)
    {
	    ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_blas_cfg_load_last_trans,cfg->last_trans_fn);
    }
	wtk_blas_cfg_update_trans(cfg);
	cfg->in_cols=wtk_blas_cfg_in_rows(cfg);
	cfg->in_col_bytes=cfg->in_cols*sizeof(float);
end:
	return ret;
}

int wtk_blas_cfg_update(wtk_blas_cfg_t *cfg)
{
	wtk_source_loader_t file_sl;
	int ret;

	file_sl.hook=0;
	file_sl.vf=wtk_source_load_file_v;
	ret=wtk_blas_cfg_update2(cfg,&(file_sl));
	return ret;
}

int wtk_blas_cfg_out_cols(wtk_blas_cfg_t *cfg)
{
	wtk_blas_layer_t *l;

	if(cfg->layer_q.length<=0){return 0;}
	l=data_offset(cfg->layer_q.push,wtk_blas_layer_t,q_n);
	return l->b->len;
}

int wtk_blas_cfg_in_rows(wtk_blas_cfg_t *cfg)
{
	wtk_blas_layer_t *l;

	if(cfg->layer_q.length<=0){return 0;}
	l=data_offset(cfg->layer_q.pop,wtk_blas_layer_t,q_n);
	return l->w->row;
}

int wtk_blas_cfg_in_cols(wtk_blas_cfg_t *cfg)
{
	wtk_blas_layer_t *l;

	if(cfg->layer_q.length<=0){return 0;}
	l=data_offset(cfg->layer_q.pop,wtk_blas_layer_t,q_n);
	return l->w->col;
}
#endif
