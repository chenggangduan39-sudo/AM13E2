#include "wtk_lda.h" 
#include "wtk_kparm.h"

int wtk_lda_bytes(wtk_lda_t *lda,int v)
{
	int bytes;

	bytes=sizeof(wtk_lda_t);
	bytes+=wtk_robin_bytes(lda->robin);
	bytes+=lda->robin->nslot*v*sizeof(float);
	return bytes;
}

wtk_lda_t* wtk_lda_new(wtk_lda_cfg_t *cfg,int v)
{
	wtk_lda_t *lda;

	lda=(wtk_lda_t*)wtk_malloc(sizeof(wtk_lda_t));
	lda->cfg=cfg;
	lda->vec_size=v;
	lda->robin=wtk_robin_new(cfg->win*2+1);
	lda->parm=NULL;
	lda->input_vec=(float*)wtk_calloc(lda->robin->nslot*v,sizeof(float));
	wtk_lda_reset(lda);
	return lda;
}

void wtk_lda_delete(wtk_lda_t *lda)
{
	wtk_free(lda->input_vec);
	wtk_robin_delete(lda->robin);
	wtk_free(lda);
}

void wtk_lda_reset(wtk_lda_t *lda)
{

}

/*
 *remove the front feature in robin.
 */
void wtk_lda_flush_robin(wtk_lda_t *p,wtk_robin_t *r)
{
	wtk_kfeat_t *f;

	f=(wtk_kfeat_t*)wtk_robin_pop(r);
	--f->used;
	wtk_kparm_push_feat(p->parm,f);
}

wtk_kfeat_t* wtk_lda_feed2(wtk_lda_t *lda,wtk_kfeat_t *feat,int is_end)
{
	wtk_robin_t *rb=lda->robin;
	int win=lda->cfg->win;
	int i,j,n;
	int vec_size=lda->vec_size;
	float *mf;
	float f;
	float *input_vec=lda->input_vec;
	float *pf;

	//wtk_debug("index=%d\n",feat->index);
	if(rb->used>0)
	{
		memmove(lda->input_vec,lda->input_vec+vec_size,(rb->nslot-1)*vec_size*sizeof(float));
	}
	if(feat)
	{
		//wtk_debug("feat[%d]=%f used=%d\n",feat->index,feat->v[0],rb->used);
		j=vec_size*sizeof(float);
		if(rb->used==0)
		{
			for(i=0;i<rb->nslot;++i)
			{
				memcpy(input_vec+i*vec_size,feat->v,j);
			}
		}else
		{
			memcpy(input_vec+(rb->nslot-1)*vec_size,feat->v,j);
		}
		++feat->used;
		wtk_robin_push(rb,feat);
	}else if(is_end)
	{
		j=vec_size*sizeof(float);
		memcpy(input_vec+(rb->nslot-1)*vec_size,input_vec+(rb->nslot-2)*vec_size,j);
	}
//	for(i=0;i<rb->nslot;++i)
//	{
//		wtk_debug("v[%d/%d]=%f\n",feat?feat->index:-1,i,input_vec[i*vec_size]);
//	}
	if(rb->used<=win)
	{
		return NULL;
	}
	mf=lda->cfg->lda->p;
	n=rb->nslot*vec_size;
//	{
//		for(i=0;i<rb->used;++i)
//		{
//			wtk_kfeat_t *f;
//
//			f=(wtk_kfeat_t*)wtk_robin_at(rb,i);
//			wtk_debug("v[%d]=%d\n",i,f->index);
//		}
//	}
	if(rb->used==rb->nslot)
	{
		feat=(wtk_kfeat_t*)wtk_robin_at(rb,win);
	}else
	{
		if(is_end)
		{
			feat=(wtk_kfeat_t*)wtk_robin_at(rb,win);
			//wtk_debug("index=%d used=%d/%d\n",feat->index,rb->used,rb->nslot);
			//exit(0);
		}else
		{
			feat=(wtk_kfeat_t*)wtk_robin_at(rb,rb->used-win-1);
			//feat=(wtk_kfeat_t*)(rb->r[rb->used-win-1]);
			//wtk_debug("used=%d/%d\n",rb->used,rb->nslot);
			//exit(0);
		}
	}
	pf=feat->v;
	//wtk_debug("index=%d used=%d/%d\n",feat->index,rb->used,rb->nslot);
	for(i=0;i<lda->cfg->lda->row;++i)
	{
		f=0;
		for(j=0;j<n;++j)
		{
			f+=input_vec[j]*(*(mf++));
		}
		//wtk_debug("v[%d]=%f\n",i,f);
		pf[i]=f;
	}
	if(rb->nslot==rb->used || is_end)
	{
		//if robin is full or got end hint, remove the front feature in the robin.
		wtk_lda_flush_robin(lda,rb);
		//wtk_debug("pop rb=%d\n",rb->used);
	}
//	{
//		static int ki=0;
//		++ki;
//		if(ki!=feat->index)
//		{
//			exit(0);
//		}
//	}
	//wtk_kparm_raise(lda->parm,feat);
	//exit(0);
	return feat;
}


void wtk_lda_feed(wtk_lda_t *lda,wtk_kfeat_t *feat,int is_end)
{
	feat=wtk_lda_feed2(lda,feat,is_end);
	if(feat)
	{
		wtk_kparm_raise(lda->parm,feat);
	}
	//exit(0);
}

void wtk_lda_flush(wtk_lda_t *lda)
{
	wtk_kfeat_t *feat;

	while(1)
	{
		feat=wtk_lda_feed2(lda,NULL,1);
		if(!feat){break;}
		wtk_kparm_raise(lda->parm,feat);
	}
	while(lda->robin->used>0)
	{
		wtk_lda_flush_robin(lda,lda->robin);
	}
}
