#include "wtk_svm.h"
#include "wtk/core/wtk_larray.h"

wtk_svm_t* wtk_svm_new()
{
	wtk_svm_t *s;

	s=(wtk_svm_t*)calloc(1,sizeof(*s));
	s->heap=wtk_heap_new(4096);
	return s;
}

void svm_destroy_model(struct svm_model* model)
{
	int i;

	if(model->free_sv && model->l > 0)
		free((void *)(model->SV[0]));
	for(i=0;i<model->nr_class-1;i++)
		free(model->sv_coef[i]);
	free(model->SV);
	free(model->sv_coef);
	free(model->rho);
	free(model->label);
	free(model->probA);
	free(model->probB);
	free(model->nSV);
	free(model);
}


int wtk_svm_delete(wtk_svm_t *s)
{
	wtk_heap_delete(s->heap);
	if(s->model)
	{
		svm_destroy_model(s->model);
	}
	free(s);
	return 0;
}

wtk_string_t svm_type[]=
{
		wtk_string("c_svc"),
		wtk_string("nu_svc"),
		wtk_string("one_class"),
		wtk_string("epsilon_svr"),
		wtk_string("nu_svr")
};

wtk_string_t kernel_type[]=
{
		wtk_string("linear"),
		wtk_string("polynomial"),
		wtk_string("rbf"),
		wtk_string("sigmoid"),
		wtk_string("precomputed")
};

int wtk_svm_load_kernel_type(wtk_svm_t *svm,wtk_source_t *s,wtk_strbuf_t *b)
{
	int i,ret;

	ret=wtk_source_read_string(s,b);
	if(ret!=0){goto end;}
	ret=-1;
	for(i=0;i<sizeof(kernel_type)/sizeof(wtk_string_t);++i)
	{
		if(wtk_string_cmp(&kernel_type[i],b->data,b->pos)==0)
		{
			svm->model->param.kernel_type=i;
			ret=0;
			break;
		}
	}
end:
	return ret;
}

int wtk_svm_load_svm_type(wtk_svm_t *svm,wtk_source_t *s,wtk_strbuf_t *b)
{
	int i,ret;

	ret=wtk_source_read_string(s,b);
	if(ret!=0){goto end;}
	ret=-1;
	for(i=0;i<sizeof(svm_type)/sizeof(wtk_string_t);++i)
	{
		if(wtk_string_cmp(&svm_type[i],b->data,b->pos)==0)
		{
			svm->model->param.svm_type=i;
			ret=0;
			break;
		}
	}
end:
	return ret;
}

int wtk_svm_load_degree(wtk_svm_t *svm,wtk_source_t *s,wtk_strbuf_t *b)
{
	float v;
	int ret;

	ret=wtk_source_read_float(s,&v,1,0);
	if(ret!=0){goto end;}
	svm->model->param.degree=v;
end:
	return ret;
}

int wtk_svm_load_gamma(wtk_svm_t *svm,wtk_source_t *s,wtk_strbuf_t *b)
{
	float v;
	int ret;

	ret=wtk_source_read_float(s,&v,1,0);
	if(ret!=0){goto end;}
	svm->model->param.gamma=v;
end:
	return ret;
}

int wtk_svm_load_coef0(wtk_svm_t *svm,wtk_source_t *s,wtk_strbuf_t *b)
{
	float v;
	int ret;

	ret=wtk_source_read_float(s,&v,1,0);
	if(ret!=0){goto end;}
	svm->model->param.coef0=v;
end:
	return ret;
}

int wtk_svm_load_nrclass(wtk_svm_t *svm,wtk_source_t *s,wtk_strbuf_t *b)
{
	float v;
	int ret;

	ret=wtk_source_read_float(s,&v,1,0);
	if(ret!=0){goto end;}
	svm->model->nr_class=v;
end:
	return ret;
}


int wtk_svm_load_totsv(wtk_svm_t *svm,wtk_source_t *s,wtk_strbuf_t *b)
{
	float v;
	int ret;

	ret=wtk_source_read_float(s,&v,1,0);
	if(ret!=0){goto end;}
	svm->model->l=v;
end:
	return ret;
}

#define Malloc(x,y) wtk_calloc(y,sizeof(x))

int wtk_svm_load_rho(wtk_svm_t *svm,wtk_source_t *s,wtk_strbuf_t *b)
{
	float *v;
	int i,ret;
	int n;

	n=svm->model->nr_class*(svm->model->nr_class-1)/2;
	v=(float*)malloc(sizeof(float)*n);
	ret=wtk_source_read_float(s,v,n,0);
	if(ret!=0){goto end;}
	svm->model->rho=Malloc(double,n);
	for(i=0;i<n;++i)
	{
		svm->model->rho[i]=v[i];
	}
end:
	free(v);
	return ret;
}

int wtk_svm_load_label(wtk_svm_t *svm,wtk_source_t *s,wtk_strbuf_t *b)
{
	float *v;
	int i,ret;
	int n;

	n=svm->model->nr_class;
	v=(float*)malloc(sizeof(float)*n);
	ret=wtk_source_read_float(s,v,n,0);
	if(ret!=0){goto end;}
	svm->model->label=Malloc(int,n);
	for(i=0;i<n;++i)
	{
		svm->model->label[i]=v[i];
	}
end:
	free(v);
	return ret;
}

int wtk_svm_load_proba(wtk_svm_t *svm,wtk_source_t *s,wtk_strbuf_t *b)
{
	float *v;
	int i,ret;
	int n;

	n=svm->model->nr_class*(svm->model->nr_class-1)/2;
	v=(float*)malloc(sizeof(float)*n);
	ret=wtk_source_read_float(s,v,n,0);
	if(ret!=0){goto end;}
	svm->model->probA=Malloc(double,n);
	for(i=0;i<n;++i)
	{
		svm->model->probA[i]=v[i];
	}
end:
	free(v);
	return ret;
}

int wtk_svm_load_probb(wtk_svm_t *svm,wtk_source_t *s,wtk_strbuf_t *b)
{
	float *v;
	int i,ret;
	int n;

	n=svm->model->nr_class*(svm->model->nr_class-1)/2;
	v=(float*)malloc(sizeof(float)*n);
	ret=wtk_source_read_float(s,v,n,0);
	if(ret!=0){goto end;}
	svm->model->probB=Malloc(double,n);
	for(i=0;i<n;++i)
	{
		svm->model->probB[i]=v[i];
	}
end:
	free(v);
	return ret;
}

int wtk_svm_load_nrsv(wtk_svm_t *svm,wtk_source_t *s,wtk_strbuf_t *b)
{
	float *v;
	int i,ret;
	int n;

	n=svm->model->nr_class;
	v=(float*)malloc(sizeof(float)*n);
	ret=wtk_source_read_float(s,v,n,0);
	if(ret!=0){goto end;}
	svm->model->nSV=Malloc(int,n);
	for(i=0;i<n;++i)
	{
		svm->model->nSV[i]=v[i];
	}
end:
	free(v);
	return ret;
}


typedef int (*wtk_svm_loader)(wtk_svm_t* svm,wtk_source_t *s,wtk_strbuf_t*b);
typedef struct
{
	wtk_string_t name;
	wtk_svm_loader loader;
}wtk_svm_handler_t;

wtk_svm_handler_t head_type[]=
{
		{wtk_string("svm_type"),wtk_svm_load_svm_type},
		{wtk_string("kernel_type"),wtk_svm_load_kernel_type},
		{wtk_string("degree"),wtk_svm_load_degree},
		{wtk_string("gamma"),wtk_svm_load_gamma},
		{wtk_string("coef0"),wtk_svm_load_coef0},
		{wtk_string("nr_class"),wtk_svm_load_nrclass},
		{wtk_string("total_sv"),wtk_svm_load_totsv},
		{wtk_string("rho"),wtk_svm_load_rho},
		{wtk_string("label"),wtk_svm_load_label},
		{wtk_string("probA"),wtk_svm_load_proba},
		{wtk_string("probB"),wtk_svm_load_probb},
		{wtk_string("nr_sv"),wtk_svm_load_nrsv},
		{wtk_string("SV"),0}
};

svm_node_t* wtk_svm_load_line_node_g(wtk_heap_t *heap,wtk_larray_t *array,char *data,int bytes)
{
	svm_node_t *ns=0,*n;
	wtk_source_t src;
	int ret,index;
	float v;

	wtk_larray_reset(array);
	wtk_source_init_str(&(src),data,bytes);
	while(1)
	{
		ret=wtk_source_read_int(&(src),&index,1,0);
		if(ret!=0)
		{
			ret=0;break;
		}
		wtk_source_get(&src);
		ret=wtk_source_read_float(&src,&v,1,0);
		if(ret!=0)
		{
			wtk_debug("read float failed.\n");
			goto end;
		}
		n=(svm_node_t*)wtk_larray_push(array);
		n->index=index;
		n->value=v;
	}
	n=(svm_node_t*)wtk_larray_push(array);
	n->index=-1;
	ns=(svm_node_t*)wtk_heap_dup_data(heap,(char*)array->slot,array->slot_size*array->nslot);
end:
	wtk_source_clean_str(&(src));
	return ns;
}

int wtk_svm_load(wtk_svm_t *svm,wtk_source_t *s)
{
	wtk_strbuf_t *buf=wtk_strbuf_new(256,1);
	svm_model_t *m;
	svm_node_t *node;
	int ret,i,k,n;
	float v;
	wtk_larray_t *array;
	wtk_heap_t *heap=svm->heap;

	array=wtk_larray_new(20,sizeof(svm_node_t));
	svm->model=m=Malloc(svm_model_t,1);
	memset(m,0,sizeof(*m));
	while(1)
	{
		ret=wtk_source_read_string(s,buf);
		if(ret!=0)
		{
			wtk_debug("read header feaild.\n");
			goto end;
		}
		ret=-1;
		//wtk_debug("%.*s\n",buf->pos,buf->data);
		for(i=0;i<sizeof(head_type)/sizeof(wtk_svm_handler_t);++i)
		{
			if(wtk_string_cmp(&head_type[i].name,buf->data,buf->pos)==0)
			{
				//wtk_debug("v[%d]=%.*s\n",i,buf->pos,buf->data);
				if(head_type[i].loader)
				{
					ret=head_type[i].loader(svm,s,buf);
				}else
				{
					ret=1;
				}
				break;
			}
		}
		if(ret<0)
		{
			wtk_debug("read %.*s failed\n",buf->pos,buf->data);
			goto end;
		}
		if(ret==1){break;}
	}
	n=m->nr_class-1;
	m->sv_coef=Malloc(double*,n);
	for(i=0;i<n;++i)
	{
		m->sv_coef[i]=calloc(m->l,sizeof(double));//Malloc(double,x);
	}
	m->SV=Malloc(svm_node_t*,m->l);
	for(i=0;i<m->l;++i)
	{
		ret=wtk_source_read_float(s,&v,1,0);
		if(ret!=0)
		{
			wtk_debug("read float failed.\n");
			break;
		}
		m->sv_coef[0][i]=v;
		for(k=1;k<n;++k)
		{
			ret=wtk_source_read_float(s,&v,1,0);
			if(ret!=0)
			{
				wtk_debug("read float failed.\n");
				break;
			}
			m->sv_coef[k][i]=v;
		}
		ret=wtk_source_read_line(s,buf);
		if(ret!=0){goto end;}
		//wtk_debug("%.*s\n",buf->pos,buf->data);
		node=wtk_svm_load_line_node_g(heap,array,buf->data,buf->pos);
		m->SV[i]=node;
	}
end:
	wtk_larray_delete(array);
	wtk_strbuf_delete(buf);
	//wtk_debug("%d,%d\n",ret,j);
	return ret;
}

