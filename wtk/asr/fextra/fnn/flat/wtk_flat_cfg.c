#include <math.h>
#include "wtk_flat_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_math.h"


wtk_dnn_fix_layer_t* wtk_dnn_fix_layer_new2()
{
	wtk_dnn_fix_layer_t *l;

	l=(wtk_dnn_fix_layer_t*)wtk_malloc(sizeof(wtk_dnn_fix_layer_t));
	//l->max=0;
	l->scale=0;
	l->b=NULL;
	l->w.c=NULL;
	l->uc=NULL;
	return l;
}

wtk_dnn_fix_layer_t* wtk_dnn_fix_layer_new(wtk_flat_cfg_t *cfg,wtk_dnn_layer_t *layer,int use_c,float max_w,float max_b)
{
	wtk_dnn_fix_layer_t *d;
	float min,max,scale;
	wtk_matrix_t *m;
//#define DEBUG_TX

	d=(wtk_dnn_fix_layer_t*)wtk_malloc(sizeof(wtk_dnn_fix_layer_t));

	m=layer->w;
	//wtk_debug("row=%d col=%d\n",wtk_matrix_rows(m),wtk_matrix_cols(m));
	min=wtk_matrix_min(m);
	max=wtk_matrix_max(m);
	//wtk_debug("min=%f max=%f\n",min,max);
	min=fabs(min);
	//wtk_debug("v[%d]: min=%f max=%f\n",layer->index,min,max);
	max=max>=min?max:min;
	//wtk_debug("max=%f\n",max);
	//scale=127.0/max;
	scale=max_w/max;
	d->scale=scale;
	//wtk_debug("max=%f\n",max);
	//wtk_debug("scale=%f max=%f\n",scale,max);
#ifdef DEBUG_TX
	wtk_debug("scale=%f max=%f\n",scale,max);
	wtk_debug("v[0]=%f\n",m[1][1]);
	wtk_debug("v[1]=%f\n",m[1][2]);
	wtk_debug("v[2]=%f\n",m[1][3]);
#endif
	if(use_c)
	{
		if(cfg->use_lazy_out && layer->type==wtk_fnn_softmax)
		{
			//d->w.c=wtk_matc_new2(m,scale);
			//wtk_debug("new 3\n");
			d->w.c=wtk_matc_new3(m,scale);
		}else
		{
			d->w.c=wtk_matc_new2(m,scale);
		}
#ifdef DEBUG_TX
		wtk_debug("v[0]=%d\n",d->w.c->p[0]);
		wtk_debug("v[1]=%d\n",d->w.c->p[1]);
		wtk_debug("v[2]=%d\n",d->w.c->p[2]);
		wtk_debug("v[4]=%d\n",d->w.c->p[3]);
		wtk_debug("v[5]=%d\n",d->w.c->p[4]);
#endif
	}else
	{
		if(cfg->use_lazy_out && layer->type==wtk_fnn_softmax)
		{
			d->w.i=wtk_mati_new3(m,scale);
		}else
		{
			d->w.i=wtk_mati_new2(m,scale);
			//wtk_mati_print(d->w.i);
			if(cfg->use_fix_trans_matrix)
			{
				int vi;
				wtk_mati_t *m;

				m=d->w.i;
				d->w.i=wtk_mati_transpose(d->w.i);
				vi=d->w.i->row;
				d->w.i->row=d->w.i->col;
				d->w.i->col=vi;
				wtk_mati_delete(m);
			}
		}
	}
	//exit(0);
	//wtk_debug("%f,%f=[%d]\n",m[1][1],m[1][1]*scale,d->w->p[0]);
	//wtk_debug("%f,%f=[%d]\n",m[2][1],m[2][1]*scale,d->w->p[1]);
	//exit(0);
	if(layer->b)
	{
		//scale*=255.0;
		scale*=max_b;

		d->b=wtk_mati_new2(layer->b,scale);
		//wtk_debug("row=%d col=%d\n",d->b->row,d->b->col);
		//wtk_debug("%f=%d\n",layer->b[1][1],d->b->p[0]);
		//exit(0);
	}else
	{
		d->b=NULL;
	}
	return d;
}

void wtk_matxuc_delete(wtk_matxuc_t *uc)
{
	wtk_matuc_delete(uc->uc);
	wtk_free(uc);
}

void  wtk_dnn_fix_layer_delete(wtk_dnn_fix_layer_t *l,int use_c)
{
	if(use_c)
	{
		if(l->w.c)
		{
			wtk_matc_delete(l->w.c);
		}
	}else
	{
		if(l->w.i)
		{
			wtk_mati_delete(l->w.i);
		}
	}
	if(l->uc)
	{
		wtk_matxuc_delete(l->uc);
	}
	if(l->b)
	{
		wtk_mati_delete(l->b);
	}
	wtk_free(l);
}

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

void wtk_dnn_trans_process2(wtk_dnn_trans_t *l,wtk_vector_t *v)
{
	int i;
	int n;

	n=wtk_vector_size(v);
	for(i=1;i<=n;++i)
	{
		//wtk_debug("v=%f,b=%f,m=%f\n",v[i],l->b[i],l->m[i]);
		v[i]=(v[i]+l->b[i])*l->m[i];
		//wtk_debug("trans[%d]=%f,%f,%f\n",i,v[i],l->b[i],l->m[i]);
		//exit(0);
	}
	//exit(0);
}


wtk_dnn_layer_t* wtk_dnn_layer_new()
{
	wtk_dnn_layer_t *layer;

	layer=(wtk_dnn_layer_t*)wtk_malloc(sizeof(*layer));
	layer->w=layer->b=0;
	layer->r_alpha=layer->r_beta=0;
	layer->fix_wb=NULL;
	return layer;
}

void wtk_dnn_layer_delete(wtk_dnn_layer_t *l,int use_c)
{
	if(l->w)
	{
		wtk_matrix_delete(l->w);
	}
	if(l->b)
	{
		wtk_matrix_delete(l->b);
	}
	if(l->r_alpha)
	{
		wtk_matrix_delete(l->r_alpha);
	}
	if(l->r_beta)
	{
		wtk_matrix_delete(l->r_beta);
	}
	if(l->fix_wb)
	{
		wtk_dnn_fix_layer_delete(l->fix_wb,use_c);
	}
	wtk_free(l);
}

void wtk_dnn_trans_delete(wtk_dnn_trans_t *t)
{
	wtk_vector_delete(t->b);
	wtk_vector_delete(t->m);
	wtk_free(t);
}

void wtk_dnn_layer_transpose(wtk_dnn_layer_t *layer)
{
	wtk_matrix_t *m;

	m=wtk_matrix_transpose2(layer->w);
	wtk_matrix_delete(layer->w);
	layer->w=m;

	/*
	if(layer->b)
	{
		m=wtk_matrix_transpose2(layer->b);
		wtk_matrix_delete(layer->b);
		layer->b=m;
	}*/
}

wtk_dnn_layer_t* wtk_dnn_layer_load(wtk_flat_cfg_t *cfg,wtk_source_t *src,wtk_strbuf_t *buf,wtk_dnn_layer_t *last_layer)
{
	int ret;
	int row,col;
	int v;
	wtk_matrix_t *m=0;
	wtk_dnn_layer_t *layer;
	int is_bin;

	is_bin=cfg->is_bin;
	layer=(wtk_dnn_layer_t*)wtk_malloc(sizeof(*layer));
	layer->w=layer->b=layer->r_alpha=layer->r_beta=0;
	layer->fix_wb=NULL;
	//wtk_debug("layer=%p\n",layer);
	ret=wtk_source_read_string(src,buf);
	if(ret!=0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
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
			exit(0);
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
	m=wtk_matrix_new2(row,col);
	ret=wtk_source_read_matrix(src,m,is_bin);
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		wtk_debug("read window failed\n");
		goto end;
	}
	//wtk_matrix_print(m);
//	{
//		wtk_matrix_t *t;
//
//		t=wtk_matrix_new2(col,row);
//		wtk_matrix_transpose(t,m);
//		wtk_matrix_delete(m);
//		m=t;
//	}
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
	/*
	if(v!=row)
	{
		wtk_debug("v!=row %d=%d\n",v,row);
		ret=-1;goto end;
	}*/
	if(cfg->use_transpose)
	{
		m=wtk_matrix_new2(1,row);
	}else
	{
		m=wtk_matrix_new2(1,col);
	}
	ret=wtk_source_read_matrix(src,m,is_bin);
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		wtk_debug("read bias failed\n");
		goto end;
	}
	layer->b=m;
	m=0;
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
	}else if(wtk_str_equal_s(buf->data,buf->pos,"<ParametricRelu>"))
	{
		layer->type=wtk_fnn_relu;
		ret=wtk_source_read_int(src,&v,1,is_bin);
		ret=wtk_source_read_int(src,&v,1,is_bin);
		wtk_source_read_string(src,buf);
		if(!wtk_str_equal_s(buf->data,buf->pos,"v"))
		{
			wtk_debug("invalid [%.*s] must v\n",buf->pos,buf->data);
			ret=-1;goto end;
		}
		ret=wtk_source_read_int(src,&v,1,is_bin);
		//wtk_debug("ret=%d,v=%d\n",ret,v);
		if(ret!=0){goto end;}
		m=wtk_matrix_new2(1,v);
		ret=wtk_source_read_matrix(src,m,is_bin);
		layer->r_alpha=m;
		m=0;

		wtk_source_read_string(src,buf);
		if(!wtk_str_equal_s(buf->data,buf->pos,"v"))
		{
			wtk_debug("invalid [%.*s] must v\n",buf->pos,buf->data);
			ret=-1;goto end;
		}
		ret=wtk_source_read_int(src,&v,1,is_bin);
		//wtk_debug("ret=%d,v=%d\n",ret,v);
		if(ret!=0){goto end;}
		m=wtk_matrix_new2(1,v);
		ret=wtk_source_read_matrix(src,m,is_bin);
		layer->r_beta=m;
		m=0;
		goto end;
	}else if(wtk_str_equal_s(buf->data,buf->pos,"<ParameterRelu>"))
	{
		layer->type=wtk_fnn_relu;
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
			wtk_matrix_delete(m);
		}
		//this is no well;
		//wtk_debug("layer=%p\n",layer);
		wtk_dnn_layer_delete(layer,cfg->use_c);
		layer=0;
	}else
	{
		if(cfg->use_transpose)
		{
			wtk_dnn_layer_transpose(layer);
		}
	}
	return layer;
}



void wtk_flat_cfg_fixpoint_layer(wtk_flat_cfg_t *cfg,wtk_dnn_layer_t *layer)
{
	wtk_dnn_fix_layer_t *fix;

	fix=wtk_dnn_fix_layer_new(cfg,layer,cfg->use_c,cfg->max_w,cfg->max_b);
	layer->fix_wb=fix;
}

int wtk_flat_cfg_load_net(wtk_flat_cfg_t *cfg,wtk_source_t *src)
{
	wtk_dnn_layer_t *layer,*last_layer;
	wtk_strbuf_t *buf;
	int ret;
	int index=0;

	src->swap=0;
	buf=wtk_strbuf_new(256,1);
	last_layer=NULL;
	while(1)
	{
		layer=wtk_dnn_layer_load(cfg,src,buf,last_layer);
		last_layer=layer;
		//wtk_debug("layer=%p\n",layer);
		if(!layer)
		{
			//wtk_debug("break \n");
			ret=0;goto end;
		}
		layer->index=index;
		//wtk_debug("v[%d]=%d\n",layer->index,layer->type);
		if(cfg->use_fix_float)
		{
			if(cfg->use_fix_0_layer || index>0)
			{
				layer->float_type=WTK_DNN_FIX_FLOAT;
				wtk_flat_cfg_fixpoint_layer(cfg,layer);
			}
		}else
		{
			layer->float_type=WTK_DNN_FLOAT;
		}
		wtk_queue_push(&(cfg->layer_q),&(layer->q_n));
		++index;
	}
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}


wtk_dnn_layer_t* wtk_flat_cfg_read_fix_0_layer(wtk_flat_cfg_t *cfg,wtk_strbuf_t *buf,wtk_source_t *src)
{
	wtk_dnn_layer_t *layer;
	wtk_matrix_t *m;
	int ret;
	int rc[2];
	char b;

	layer=NULL;
	ret=wtk_source_read_int(src,rc,2,1);
	if(ret!=0){goto end;}
	//wtk_debug("v=%d/%d\n",rc[0],rc[1]);
	layer=wtk_dnn_layer_new();
	m=wtk_matrix_new(rc[0],rc[1]);
	ret=wtk_source_read_matrix(src,m,1);
	if(ret!=0)
	{
		wtk_debug("read window failed\n");
		goto end;
	}
//	if(1)
//	{
//		wtk_matrix_t *t;
//
//		t=wtk_matrix_new2(rc[1],rc[0]);
//		wtk_matrix_transpose(t,m);
//		wtk_matrix_delete(m);
//		m=t;
//	}
	layer->w=m;
	b=wtk_source_get(src);
	if(b!=0)
	{
		m=wtk_matrix_new(1,rc[1]);
		ret=wtk_source_read_matrix(src,m,1);
		if(ret!=0)
		{
			wtk_debug("read bias failed\n");
			goto end;
		}
		layer->b=m;
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

wtk_matuc_t* wtk_matuc_new4(wtk_matc_t *src,int *pmin)
{
	wtk_matuc_t *uc;
	int min=1000;
	int i,j;
	signed char *p;
	unsigned char *c;

	uc=wtk_matuc_new_align(src->row,src->col,4);
	uc->row=src->row;
	uc->col=src->col;
	min=1000;
	//wtk_debug("row=%d col=%d col_bytes=%d/%d\n",src->row,src->col,src->col_bytes,uc->col_bytes);
	//exit(0);
	for(i=0;i<src->row;++i)
	{
		p=src->p+i*src->col_bytes;
		for(j=0;j<src->col;++j)
		{
			if(p[j]<min)
			{
				min=p[j];
			}
		}
	}
	for(i=0;i<src->row;++i)
	{
		p=src->p+i*src->col_bytes;
		c=uc->p+i*uc->col_bytes;
		for(j=0;j<src->col;++j)
		{
			c[j]=p[j]-min;
		}
	}
	//exit(0);
	*pmin=min;
	return uc;
}


wtk_matxuc_t *wtk_flat_cfg_mat_transpose2(wtk_matc_t *m)
{
	wtk_matc_t *mc;
	wtk_matxuc_t *uc;

	//wtk_debug("row=%d col=%d\n",m->row,m->col);
	mc=wtk_matc_new_align(m->col,m->row,4);
	wtk_matc_init_transpose(mc,m);
	uc=(wtk_matxuc_t*)wtk_malloc(sizeof(wtk_matxuc_t));
	uc->uc=wtk_matuc_new4(mc,&uc->min);
	wtk_matc_delete(mc);
	return uc;
}


void wtk_matuc_init_matc(wtk_matuc_t *uc,wtk_matc_t *m,int *pmin)
{
	int min;
	int i,j;
	signed char *p;
	unsigned char *c;

	min=1000;
	for(i=0;i<m->row;++i)
	{
		p=m->p+i*m->col_bytes;
		for(j=0;j<m->col;++j)
		{
			if(p[j]<min)
			{
				min=p[j];
			}
		}
	}
	for(i=0;i<m->row;++i)
	{
		p=m->p+i*m->col_bytes;
		c=uc->p+i*uc->col_bytes;
		for(j=0;j<m->col;++j)
		{
			c[j]=p[j]-min;
		}
	}
	*pmin=min;
}


wtk_matxuc_t *wtk_flat_cfg_mat_transpose2_x(wtk_matc_t *m)
{
	wtk_matxuc_t *uc;

	uc=(wtk_matxuc_t*)wtk_malloc(sizeof(wtk_matxuc_t));
	uc->uc=wtk_matuc_new_align(m->row,m->col,4);
	wtk_matuc_init_matc(uc->uc,m,&(uc->min));
	return uc;
}


wtk_matc_t *wtk_flat_cfg_mat_transpose(wtk_matc_t *m)
{
	wtk_matc_t *mc;

	//wtk_debug("row=%d col=%d\n",m->row,m->col);
	mc=wtk_matc_new_align(m->col,m->row,4);
	wtk_matc_init_transpose(mc,m);
	//wtk_matc_print(m);
	//wtk_matc_print(mc);
	//mc=wtk_matc_transpose(m);
	//wtk_matc_print(mc);
	//exit(0);
	return mc;
}

wtk_matc_t* wtk_flat_cfg_mat_transpose_x(wtk_matc_t *m)
{
	wtk_matc_t *mc;

	//wtk_debug("row=%d col=%d\n",m->row,m->col);
	mc=wtk_matc_new_align(m->row,m->col,4);
	wtk_matc_cpy(mc,m);
	return mc;
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
wtk_dnn_layer_t* wtk_flat_cfg_read_fix_char_layer(wtk_flat_cfg_t *cfg,wtk_strbuf_t *buf,wtk_source_t *src)
{
	wtk_dnn_fix_layer_t *fix_layer;
	wtk_dnn_layer_t *layer;
	int ret;
	int rc[2];
	char b;
	float scale;
	int col;

	layer=NULL;
	ret=wtk_source_read_float(src,&scale,1,1);
	if(ret!=0)
	{
		wtk_debug("read scale failed ret=%d\n",ret);
		goto end;
	}
	layer=wtk_dnn_layer_new();
	//wtk_debug("scale=%f\n",scale);
	fix_layer=wtk_dnn_fix_layer_new2();
	fix_layer->scale=scale;
	ret=wtk_source_read_int(src,rc,2,1);
	if(ret!=0){goto end;}
	//wtk_debug("%p row=%d col=%d\n",fix_layer,rc[0],rc[1]);
	fix_layer->w.c=wtk_matc_new(rc[0],rc[1]);
	ret=wtk_source_fill(src,(char*)(fix_layer->w.c->p),rc[0]*rc[1]);
	if(ret!=0)
	{
		wtk_debug("read matrix failed\n");
		goto end;
	}
	b=wtk_source_get(src);
	if(b!=0)
	{
		col=cfg->use_transpose?rc[1]:rc[0];
		fix_layer->b=wtk_mati_new(1,col);
		ret=wtk_source_fill(src,(char*)(fix_layer->b->p),col*sizeof(int));
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
	//wtk_debug("fix=%p %d/%d\n",layer->fix_wb,layer->fix_wb->w.c->row,layer->fix_wb->w.c->col);
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
wtk_dnn_layer_t* wtk_flat_cfg_read_fix_short_layer(wtk_flat_cfg_t *cfg,wtk_strbuf_t *buf,wtk_source_t *src)
{
	wtk_dnn_fix_layer_t *fix_layer;
	wtk_dnn_layer_t *layer;
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
	layer=wtk_dnn_layer_new();
	//wtk_debug("scale=%f\n",scale);
	fix_layer=wtk_dnn_fix_layer_new2();
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

wtk_dnn_layer_t* wtk_flat_cfg_read_fix_0_char_layer(wtk_flat_cfg_t *cfg,wtk_strbuf_t *buf,wtk_source_t *src)
{
	wtk_dnn_fix_layer_t *fix_layer;
	wtk_dnn_layer_t *layer;
	wtk_matrix_t *m;
	int ret;
	int rc[2];
	float scale;
	int col;

	layer=NULL;
	ret=wtk_source_read_float(src,&scale,1,1);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,rc,2,1);
	if(ret!=0){goto end;}
	//wtk_debug("scale=%f row=%d col=%d\n",scale,rc[0],rc[1]);
	layer=wtk_dnn_layer_new();
	layer->type=WTK_DNN_FIX0;
	fix_layer=wtk_dnn_fix_layer_new2();
	fix_layer->scale=scale;
	layer->fix_wb=fix_layer;
	fix_layer->w.c=wtk_matc_new(rc[0],rc[1]);
	ret=wtk_source_fill(src,(char*)(fix_layer->w.c->p),rc[0]*rc[1]);
	//wtk_mati_print2(fix_layer->w.i,10);
	//wtk_mati_print(fix_layer->w.i);
	//exit(0);
	if(ret!=0){goto end;}
	col=cfg->use_transpose?rc[1]:rc[0];
	m=wtk_matrix_new(1,col);
	//wtk_debug("col=%d\n",col);
	ret=wtk_source_read_matrix(src,m,1);
	if(ret!=0)
	{
		wtk_debug("read bias failed\n");
		goto end;
	}
	layer->b=m;
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
	//wtk_debug("============= found end\n");
	//exit(0);
	return layer;
}

wtk_dnn_layer_t* wtk_flat_cfg_read_fix_0_int_layer(wtk_flat_cfg_t *cfg,wtk_strbuf_t *buf,wtk_source_t *src)
{
	wtk_dnn_fix_layer_t *fix_layer;
	wtk_dnn_layer_t *layer;
	wtk_matrix_t *m;
	int ret;
	int rc[2];
	float scale;

	layer=NULL;
	ret=wtk_source_read_float(src,&scale,1,1);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,rc,2,1);
	if(ret!=0){goto end;}
	//wtk_debug("scale=%f %d-%d\n",scale,rc[0],rc[1]);
	layer=wtk_dnn_layer_new();
	layer->type=WTK_DNN_FIX0;
	fix_layer=wtk_dnn_fix_layer_new2();
	fix_layer->scale=scale;
	layer->fix_wb=fix_layer;
	fix_layer->w.i=wtk_mati_new(rc[0],rc[1]);
	ret=wtk_source_fill(src,(char*)(fix_layer->w.i->p),rc[0]*rc[1]*sizeof(int));
	//wtk_mati_print2(fix_layer->w.i,10);
	//wtk_mati_print(fix_layer->w.i);
	//exit(0);
	if(ret!=0){goto end;}
	m=wtk_matrix_new(1,rc[1]);
	ret=wtk_source_read_matrix(src,m,1);
	if(ret!=0)
	{
		wtk_debug("read bias failed\n");
		goto end;
	}
	layer->b=m;
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

int wtk_flat_cfg_load_fix_net(wtk_flat_cfg_t *cfg,wtk_source_t *src)
{
	wtk_dnn_layer_t *layer;
	wtk_strbuf_t *buf;
	int ret;
	//int index;
	int i;
	int cnt;
	wtk_queue_node_t *qn;
	int uc;
	wtk_matxuc_t *xuc;
	wtk_matc_t *mc;

	src->swap=0;
	buf=wtk_strbuf_new(256,1);
	ret=wtk_source_read_int(src,&(cnt),1,1);
	if(ret!=0){goto end;}
	//wtk_debug("cnt=%d\n",cnt);
	//wtk_debug("use_fix0=%d\n",cfg->use_fix_0_layer);
	if(cfg->use_fix_0_layer)
	{
		if(cfg->use_fix_0_c)
		{
			layer=wtk_flat_cfg_read_fix_0_char_layer(cfg,buf,src);
		}else
		{
			layer=wtk_flat_cfg_read_fix_0_int_layer(cfg,buf,src);
		}
		if(!layer)
		{
			wtk_debug("load fix 0 layer failed.\n");
			ret=-1;goto end;
		}
		wtk_queue_push(&(cfg->layer_q),&(layer->q_n));
	}else
	{
		layer=wtk_flat_cfg_read_fix_0_layer(cfg,buf,src);
		if(!layer){ret=-1;goto end;}
		layer->float_type=WTK_DNN_FLOAT;
		wtk_queue_push(&(cfg->layer_q),&(layer->q_n));
	}
	for(i=1;i<cnt;++i)
	{
		//wtk_debug("use_c=%d\n",cfg->use_c);
		if(cfg->use_c)
		{
			layer=wtk_flat_cfg_read_fix_char_layer(cfg,buf,src);
		}else
		{
			layer=wtk_flat_cfg_read_fix_short_layer(cfg,buf,src);
		}
		//wtk_debug("v[%d]=%p row=%d/%d\n",i,layer->fix_wb->uc,layer->fix_wb->uc->uc->row,layer->fix_wb->uc->uc->col);
		if(!layer)
		{
			wtk_debug("load fix %d layer failed.\n",i);
			ret=-1;goto end;
		}
		layer->float_type=WTK_DNN_FIX_FLOAT;
		wtk_queue_push(&(cfg->layer_q),&(layer->q_n));
	}
	if(cfg->use_c && cfg->use_fix_0_cx)
	{
		uc=1;
		for(i=0,qn=cfg->layer_q.pop;qn;qn=qn->next,++i)
		{
			layer=data_offset2(qn,wtk_dnn_layer_t,q_n);
			//wtk_debug("v[%d] row=%d col=%d\n",i,layer->fix_wb->w.c->row,layer->fix_wb->w.c->col);
			if(cfg->use_transpose)
			{
				if(uc==1)
				{
					xuc=wtk_flat_cfg_mat_transpose2(layer->fix_wb->w.c);
					layer->fix_wb->uc=xuc;
				}else
				{
					mc=wtk_flat_cfg_mat_transpose(layer->fix_wb->w.c);
					mc->row=layer->fix_wb->w.c->row;
					mc->col=layer->fix_wb->w.c->col;
					wtk_matc_delete(layer->fix_wb->w.c);
					layer->fix_wb->w.c=mc;
				}
			}else
			{
				if(uc)
				{
					xuc=wtk_flat_cfg_mat_transpose2_x(layer->fix_wb->w.c);
					layer->fix_wb->uc=xuc;
					wtk_matc_delete(layer->fix_wb->w.c);
					layer->fix_wb->w.c=NULL;
				}else
				{
					mc=wtk_flat_cfg_mat_transpose_x(layer->fix_wb->w.c);
					wtk_matc_delete(layer->fix_wb->w.c);
					layer->fix_wb->w.c=mc;
				}
			}
			switch(layer->type)
			{
			case wtk_fnn_sigmoid:
				//wtk_debug("sigmoid\n");
				uc=0;
				break;
			case wtk_fnn_relu:
				//TODO
				break;
			case wtk_fnn_pnorm:
				//TODO
				break;
			case wtk_fnn_rescale:
				//TODO
				break;
			case wtk_fnn_softmax:
				//wtk_debug("softmax\n");
				uc=0;
				break;
			case wtk_fnn_linear:
				uc=1;
				break;
			default:
				break;
			}
		}
	}
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

wtk_vector_t* wtk_flat_load_trans_vector(wtk_source_t *src,wtk_strbuf_t *buf,char *name,int is_bin)
{
	wtk_vector_t *v=0;
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
	v=wtk_vector_new(i);
	ret=wtk_source_read_vector(src,v,is_bin);
	if(ret!=0){goto end;}
	//wtk_vector_print(v);
end:
	if(ret!=0 && v)
	{
		wtk_vector_delete(v);
		v=0;
	}
	return v;
}

int wtk_flat_cfg_load_trans(wtk_flat_cfg_t *cfg,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	wtk_dnn_trans_t *trans;
	int ret=-1;

	src->swap=0;
	buf=wtk_strbuf_new(256,1);
	trans=(wtk_dnn_trans_t*)wtk_malloc(sizeof(*trans));
	trans->b=wtk_flat_load_trans_vector(src,buf,"<bias>",cfg->is_bin);
	if(!trans->b){goto end;}
	trans->m=wtk_flat_load_trans_vector(src,buf,"<window>",cfg->is_bin);
	if(!trans->m){goto end;}
	cfg->trans=trans;
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

int wtk_flat_cfg_update2(wtk_flat_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	if(!cfg->net_fn || !cfg->trans_fn)
	{
		ret=-1;
		goto end;
	}
	if(cfg->use_fix_res)
	{
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_flat_cfg_load_fix_net,cfg->net_fn);
	}else
	{
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_flat_cfg_load_net,cfg->net_fn);
	}
	if(ret!=0){goto end;}
	ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_flat_cfg_load_trans,cfg->trans_fn);
	if(ret!=0){goto end;}
	//wtk_debug("ret=%d\n",ret);
	//exit(0);
	//wtk_debug("load trans\n");
	ret=0;
end:
	return ret;
}


int wtk_flat_cfg_init(wtk_flat_cfg_t *cfg)
{
	cfg->net_fn=0;
	cfg->trans_fn=0;
	cfg->trans=0;
	cfg->is_bin=0;
	cfg->use_fix_float=0;
	cfg->cache_size=10;
	cfg->nx=4;
	cfg->use_transpose=1;
	cfg->max_w=127.0;
	cfg->max_b=255.0;
	cfg->max_0_w=2048.0;
	cfg->use_c=1;
	cfg->use_lazy_out=0;
	cfg->use_fix_res=0;
	cfg->use_fix_trans_matrix=0;
	cfg->min_avg_scale=0;
	cfg->min_avg_v=0;
	cfg->min_trans_avg_scale=0;
	cfg->min_trans_avg_v=0;
	cfg->use_fix_0_layer=0;
	cfg->use_fix_0_c=0;
	cfg->use_mt=0;
	wtk_queue_init(&(cfg->layer_q));
	cfg->use_fix_0_cx=0;
	return 0;
}

int wtk_flat_cfg_clean(wtk_flat_cfg_t *cfg)
{
	wtk_queue_node_t *n;
	wtk_dnn_layer_t  *l;

	while(1)
	{
		n=wtk_queue_pop(&(cfg->layer_q));
		if(!n){break;}
		l=data_offset(n,wtk_dnn_layer_t,q_n);
		wtk_dnn_layer_delete(l,cfg->use_c);
	}
	if(cfg->trans)
	{
		wtk_dnn_trans_delete(cfg->trans);
	}
	return 0;
}

int wtk_flat_cfg_update_local(wtk_flat_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,net_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,trans_fn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,is_bin,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fix_float,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fix_0_cx,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,cache_size,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nx,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_avg_scale,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_avg_v,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_trans_avg_scale,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_trans_avg_v,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_transpose,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_c,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_w,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_b,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_0_w,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_lazy_out,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fix_res,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fix_trans_matrix,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fix_0_layer,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fix_0_c,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_mt,v);
	return 0;
}

int wtk_flat_cfg_update(wtk_flat_cfg_t *cfg)
{
	wtk_source_loader_t file_sl;
	int ret;

	file_sl.hook=0;
	file_sl.vf=wtk_source_load_file_v;
	ret=wtk_flat_cfg_update2(cfg,&(file_sl));
	return ret;
}

int wtk_flat_cfg_in_cols(wtk_flat_cfg_t *cfg)
{
	wtk_dnn_layer_t *l;
	int n;

	if(cfg->layer_q.length<=0)
	{
		n=0;
		goto end;
	}
	l=data_offset(cfg->layer_q.pop,wtk_dnn_layer_t,q_n);
	if(cfg->use_transpose)
	{
		if(l->w)
		{
			n=wtk_matrix_rows(l->w);
			//return wtk_matrix_cols(l->w);
		}else
		{
			if(cfg->use_c)
			{
				if(l->fix_wb->uc)
				{
					n=l->fix_wb->uc->uc->row;
				}else
				{
					n=l->fix_wb->w.c->row;
				}
			}else
			{
				n=l->fix_wb->w.i->row;
			}
		}
	}else
	{
		if(l->w)
		{
			n=wtk_matrix_cols(l->w);
			//return wtk_matrix_cols(l->w);
		}else
		{
			if(cfg->use_c)
			{
				if(l->fix_wb->uc)
				{
					n=l->fix_wb->uc->uc->col;
				}else
				{
					n=l->fix_wb->w.c->col;
				}
			}else
			{
				n=l->fix_wb->w.i->col;
			}
		}
	}
end:
	return n;
}

int wtk_flat_cfg_out_cols(wtk_flat_cfg_t *cfg)
{
	wtk_dnn_layer_t *l;
	int n;

	if(cfg->layer_q.length<=0)
	{
		n=0;
		goto end;
	}
	l=data_offset(cfg->layer_q.push,wtk_dnn_layer_t,q_n);
	if(cfg->use_transpose)
	{
		if(l->w)
		{
			n=wtk_matrix_cols(l->w);
		}else
		{
			if(cfg->use_c)
			{
				if(l->fix_wb->uc)
				{
					n=l->fix_wb->uc->uc->col;
				}else
				{
					n=l->fix_wb->w.c->col;
				}
			}else
			{
				n=l->fix_wb->w.i->col;
			}
		}
	}else
	{
		if(l->w)
		{
			n=wtk_matrix_rows(l->w);
		}else
		{
			if(cfg->use_c)
			{
				if(l->fix_wb->uc)
				{
					n=l->fix_wb->uc->uc->row;
				}else
				{
					n=l->fix_wb->w.c->row;
				}
			}else
			{
				n=l->fix_wb->w.i->row;
			}
		}
	}
end:
	return n;
}
