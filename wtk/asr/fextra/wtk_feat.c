#include "wtk_fextra_cfg.h"
#include "wtk_feat.h"
#include "wtk/core/math/wtk_math.h"

wtk_feat_t* wtk_feat_new2(wtk_feat_cfg_t *cfg, int xf_size)
{
	wtk_feat_t *f;

	f=(wtk_feat_t*)wtk_calloc(1, sizeof(*f));
	f->cfg=cfg;
	f->send_hook=f->app_hook=NULL;
	f->used=0;f->index=0;
	//f=wtk_feature_new(p->cfg->align,p->feature_cols,p->xform_rows,p->fmpe?1:0);
	f->v=wtk_vector_new(cfg->sig_size);
	if(xf_size>0)
	{
		f->rv=f->xf_v=wtk_vector_new(xf_size);
	}else
	{
		f->xf_v=0;
		f->rv=f->v;
	}
	if(cfg->use_dnn)
	{
		f->dnn_v=wtk_vector_new(cfg->dnn_size);
		f->rv=f->dnn_v;
	}else
	{
		f->dnn_v=0;
	}
	return f;
}

wtk_feat_t* wtk_feat_new(wtk_feat_cfg_t *cfg)
{
	return wtk_feat_new2(cfg, 0);
}

int wtk_feat_bytes2(wtk_fextra_cfg_t *cfg,int xf_size)
{
	int bytes=0;

	bytes+=sizeof(wtk_feat_t);
	bytes+=wtk_vector_bytes(cfg->feature_cols);
	if(xf_size>0)
	{
		bytes+=wtk_vector_bytes(xf_size);
	}
	if(cfg->use_dnn)
	{
		bytes+=wtk_vector_bytes(cfg->dnn.out_cols);
	}
	return bytes;
}

int wtk_feat_bytes(wtk_fextra_cfg_t *cfg)
{
	return wtk_feat_bytes2(cfg, 0);
}

int wtk_feat_delete(wtk_feat_t *f)
{
	//wtk_debug("%p\n",f);
	if(f->v)
	{
		wtk_vector_delete(f->v);
	}
	if(f->xf_v)
	{
		wtk_vector_delete(f->xf_v);
	}
	if(f->dnn_v)
	{
		wtk_vector_delete(f->dnn_v);
	}
	wtk_free(f);
	return 0;
}


int wtk_feat_send(wtk_feat_t *f)
{
	return f->send(f->send_hook,f);
}

void wtk_feat_push_back(wtk_feat_t *f)
{
	--f->used;
	//wtk_debug("push feature back %p/%d used=%d\n",f,f->index,f->used);
	if(f->send)
	{
		f->send(f->send_hook,f);
	}
}

void wtk_feat_use_dec(wtk_feat_t *f)
{
	--f->used;
}

void wtk_feat_print(wtk_feat_t *f)
{
	//printf("addr=%p,ref=%d\n",f,f->ref);
	printf("============ feature(%p,%d) ================\n",f,f->index);
	wtk_vector_print(f->rv);
	//exit(0);
}

void wtk_feat_print2(wtk_feat_t *f)
{
	FILE* log=stdout;
	wtk_vector_t *v=f->rv;
	int i,n;

	n=wtk_vector_size(v);
	for(i=1;i<=n;++i)
	{
		if(i>1)
		{
			fprintf(log," ");
		}
		fprintf(log,"%e",v[i]);
	}
	fprintf(log,"\n");
}
