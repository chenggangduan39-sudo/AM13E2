#include "wtk_delta.h" 
#include "wtk_kparm.h"

float wtk_delta_win2sigma(int win)
{
	int i;
	float sigma;

	sigma=0;
	for(i=1;i<=win;++i)
	{
		sigma+=i*i;
	}
	sigma*=2;
	//wtk_debug("sigma=%f\n",sigma);
	return 1.0/sigma;
}

wtk_delta_t* wtk_delta_new(wtk_delta_cfg_t *cfg,int vec_size)
{
	wtk_delta_t *delta;
	int i;

	delta=(wtk_delta_t*)wtk_malloc(sizeof(wtk_delta_t));
	delta->cfg=cfg;
	delta->vec_size=vec_size;
	delta->robin=(wtk_robin_t**)wtk_calloc(cfg->order,sizeof(wtk_robin_t*));
	for(i=0;i<cfg->order;++i)
	{
		delta->robin[i]=wtk_robin_new(cfg->win*2+1);
		delta->sigma[i]=wtk_delta_win2sigma(cfg->win);
		delta->fix_sigma[i]=FLOAT2FIX(delta->sigma[i]);
	}
	return delta;
}

int wtk_delta_bytes(wtk_delta_t *delta)
{
	int bytes;
	int i;

	bytes=sizeof(wtk_delta_t);
	bytes+=delta->cfg->order*sizeof(wtk_robin_t*);
	for(i=0;i<delta->cfg->order;++i)
	{
		bytes+=wtk_robin_bytes(delta->robin[i]);
	}
	return bytes;
}

void wtk_delta_delete(wtk_delta_t *delta)
{
	int i;

	for(i=0;i<delta->cfg->order;++i)
	{
		wtk_robin_delete(delta->robin[i]);
	}
	wtk_free(delta->robin);
	wtk_free(delta);
}

void wtk_delta_reset(wtk_delta_t *delta)
{
	int i;

	for(i=0;i<delta->cfg->order;++i)
	{
		delta->pos[i]=-1;
		if(delta->robin[i]->used>0)
		{
			exit(0);
		}
	}
}

void wtk_delta_diff(float **pv,int win,int vec_size,float sigma)
{
	int i,j;
	float *p,*n,*v;

	v=pv[win]+vec_size;
	for(i=1;i<=win;++i)
	{
		p=pv[win-i];
		n=pv[win+i];
#ifndef USE_X
		if(i==1)
		{
			if(i==win)
			{
				for(j=0;j<vec_size;++j)
				{
					v[j]=(n[j]-p[j])*sigma;
				}
			}else
			{
				for(j=0;j<vec_size;++j)
				{
					v[j]=n[j]-p[j];
				}
			}
		}else if(i==win)
		{
			for(j=0;j<vec_size;++j)
			{
				v[j]=(v[j]+i*(n[j]-p[j]))*sigma;
			}
		}else
		{
			for(j=0;j<vec_size;++j)
			{
				v[j]+=i*(n[j]-p[j]);
			}
		}
#else
		for(j=0;j<vec_size;++j)
		{
			if(i==1)
			{
				v[j]=n[j]-p[j];
			}else
			{
				v[j]+=i*(n[j]-p[j]);
			}
			if(j==0)
			{
				wtk_debug("v[%d]=%f/%f/%f\n",j,n[j],p[j],v[j]);
			}
			if(i==win)
			{
				v[j]*=sigma;
			}
		}
#endif
	}
}


/*
 *remove the front feature in robin.
 */
void wtk_delta_flush_robin(wtk_delta_t *p,wtk_robin_t *r)
{
	wtk_kfeat_t *f;

	f=(wtk_kfeat_t*)wtk_robin_pop(r);
	--f->used;
	//wtk_debug("flush feat=%d f=%d\n",f->index,f->used);
	wtk_kparm_push_feat(p->parm,f);
}

wtk_kfeat_t* wtk_delta_feed_order(wtk_delta_t *delta,wtk_kfeat_t *feat,int index,int is_end)
{
	wtk_robin_t *rb=delta->robin[index];
	int win=delta->cfg->win;
	float *pf[16];
	wtk_kfeat_t *fv[16];
	int pad;
	int i,j;
	int pos;

//	if(feat)
//	{
//		wtk_debug("============= feat[%d] index=%d ==================\n",feat->index,index);
//	}
	//print_float(feat->v,12);
	if(feat)
	{
		++feat->used;
		wtk_robin_push(rb,feat);
	}
	if(rb->used<=win)
	{
		return NULL;
	}
	pad=rb->nslot-rb->used;
	i=0;
	pos=index*delta->vec_size;
//	wtk_debug("################# index=%d used=%d/%d pos=%d feat=%d sigma=%f ######################\n",index,rb->used,win,pos,
//			feat->index,delta->sigma[index]);
	if(pad>0 && !is_end)
	{
		//if not end, add pad to front.
		// * |f0|f1|f2|0|0|  => |f0|f0|f0|f1|f2|
		// * |f0|f1|f2|f3|0| => |f0|f0|f1|f2|f3|
		feat=(wtk_kfeat_t*)wtk_robin_at(rb,0);
		for(;i<pad;++i)
		{
			pf[i]=feat->v+pos;
			fv[i]=feat;
		}
	}
	for(j=0;j<rb->used;++i,++j)
	{
		feat=((wtk_kfeat_t*)wtk_robin_at(rb,j));
		pf[i]=feat->v+pos;
		fv[i]=feat;
	}
	if(pad>0 && is_end)
	{
		//if is end and pad to the end.
		//|f0|f1|f2|f3|0| => |f0|f1|f2|f3|f3|
		//|f0|f1|f2|0|0| => |f0|f1|f2|f2|f2|
		feat=(wtk_kfeat_t*)wtk_robin_at(rb,rb->used-1);
		for(j=0;j<pad;++i,++j)
		{
			pf[i]=feat->v+pos;
			fv[i]=feat;
			//wtk_debug("v[%d]=%d,%p\n",i,f->index,f);
		}
	}
	wtk_delta_diff(pf,win,delta->vec_size,delta->sigma[index]);
	feat=fv[win];
//	for(i=0;i<(win*2+1);++i)
//	{
//		wtk_debug("v[%d]=%d\n",i,fv[i]->index);
//	}
//	print_float(feat->v+delta->vec_size*(index+1),delta->vec_size);
	//f=wtk_feature_v_to_f(pv[win]);
	if(rb->nslot==rb->used || is_end)
	{
		//if robin is full or got end hint, remove the front feature in the robin.
		wtk_delta_flush_robin(delta,rb);
	}
	++index;
	if(index<delta->cfg->order)
	{
		return wtk_delta_feed_order(delta,feat,index,is_end);
	}
	return feat;
}

void wtk_delta_feed(wtk_delta_t *delta,wtk_kfeat_t *feat)
{
	feat=wtk_delta_feed_order(delta,feat,0,0);
	if(feat)
	{
		wtk_kparm_on_delta(delta->parm,feat);
	}
}

void wtk_delta_flush(wtk_delta_t *delta)
{
	int i;
	wtk_kfeat_t *feat;
	wtk_robin_t *rb;

	for(i=0;i<delta->cfg->order;++i)
	{
		while(1)
		{
			feat=wtk_delta_feed_order(delta,NULL,i,1);
			if(!feat)
			{
				break;
			}
			wtk_kparm_on_delta(delta->parm,feat);
		}
		rb=delta->robin[i];
		while(rb->used>0)
		{
			wtk_delta_flush_robin(delta,rb);
		}
		//wtk_debug("v[%d]=%d\n",i,delta->robin[i]->used);
	}
}

void wtk_delta_diff_fix(int **pv,int win,int vec_size,int sigma)
{
	int i,j;
	int *p,*n,*v;

	v=pv[win]+vec_size;
	for(i=1;i<=win;++i)
	{
		p=pv[win-i];
		n=pv[win+i];
		for(j=0;j<vec_size;++j)
		{
			if(i==1)
			{
				v[j]=n[j]-p[j];
			}else
			{
				v[j]+=i*(n[j]-p[j]);
			}
			if(i==win)
			{
				v[j]=FIXMUL(v[j],sigma);
			}
		}
	}
}



wtk_kfeat_t* wtk_delta_feed_fix_order(wtk_delta_t *delta,wtk_kfeat_t *feat,int index,int is_end)
{
	wtk_robin_t *rb=delta->robin[index];
	int win=delta->cfg->win;
	int *pf[16];
	wtk_kfeat_t *fv[16];
	int pad;
	int i,j;
	int pos;
	wtk_kfeat_t *f;
	int set=1;

//	if(feat)
//	{
//		wtk_debug("feat[%d]: index=%d used=%d rb=%d end=%d\n",feat->index,index,feat->used,rb->used,is_end);
//		for(i=0;i<rb->used;++i)
//		{
//			wtk_kfeat_t *f;
//			f=(wtk_kfeat_t*)wtk_robin_at(rb,i);
//			wtk_debug("v[%d]=%d\n",i,f->index);
//		}
//	}
	//print_float(feat->v,12);
	if(feat)
	{
		++feat->used;
		wtk_robin_push(rb,feat);
	}
	if(rb->used<=win)
	{
		return NULL;
	}
	pad=rb->nslot-rb->used;
	i=0;
	pos=index*delta->vec_size;
//	wtk_debug("################# index=%d used=%d/%d pos=%d feat=%d sigma=%f ######################\n",index,rb->used,win,pos,
//			feat->index,delta->sigma[index]);
	//wtk_debug("pos=%d/%d\n",delta->pos[index],win-delta->pos[index]);
	if(pad>0)
	{
		if(!is_end)
		{
			//if not end, add pad to front.
			// * |f0|f1|f2|0|0|  => |f0|f0|f0|f1|f2|
			// * |f0|f1|f2|f3|0| => |f0|f0|f1|f2|f3|
			feat=(wtk_kfeat_t*)wtk_robin_at(rb,0);
			for(;i<pad;++i)
			{
				pf[i]=((int*)feat->v)+pos;
				fv[i]=feat;
			}
		}else if(delta->pos[index]<win-1)
		{
			set=0;
			j=win-delta->pos[index]-1;
			//wtk_debug("win=%d j=%d\n",win,j);
			feat=(wtk_kfeat_t*)wtk_robin_at(rb,0);
			for(;i<j;++i)
			{
				pf[i]=((int*)feat->v)+pos;
				fv[i]=feat;
			}
			//exit(0);
		}
	}
	for(j=0;j<rb->used;++i,++j)
	{
		feat=((wtk_kfeat_t*)wtk_robin_at(rb,j));
		pf[i]=(int*)(feat->v)+pos;
		fv[i]=feat;
	}
	if(pad>0 && is_end)
	{
		//if is end and pad to the end.
		//|f0|f1|f2|f3|0| => |f0|f1|f2|f3|f3|
		//|f0|f1|f2|0|0| => |f0|f1|f2|f2|f2|
		feat=(wtk_kfeat_t*)wtk_robin_at(rb,rb->used-1);
		for(j=0;j<pad;++i,++j)
		{
			pf[i]=(int*)(feat->v)+pos;
			fv[i]=feat;
			//wtk_debug("v[%d]=%d,%p\n",i,f->index,f);
		}
	}
	wtk_delta_diff_fix(pf,win,delta->vec_size,delta->fix_sigma[index]);
	feat=fv[win];
//	wtk_debug("================== xx delta[%d]=%d =====================\n",index,delta->pos[index]);
//	for(i=0;i<(win*2+1);++i)
//	{
//		wtk_debug("v[%d]=%d\n",i,fv[i]->index);
//	}
//	wtk_debug("feed feat=%d\n",feat->index);
	if(delta->pos[index]<(win-1))
	{
		for(i=0;i<rb->used;++i)
		{
			f=(wtk_kfeat_t*)wtk_robin_at(rb,i);
			//wtk_debug("v[%d]=%d\n",i,f->index);
			if(f==feat)
			{
				//wtk_debug("feat=%d pos=%d\n",feat->index,i);
				delta->pos[index]=i;
				break;
			}
		}
	}
//	for(i=0;i<(win*2+1);++i)
//	{
//		wtk_debug("v[%d]=%d\n",i,fv[i]->index);
//	}
//	print_float(feat->v+delta->vec_size*(index+1),delta->vec_size);
	//f=wtk_feature_v_to_f(pv[win]);
	if(rb->nslot==rb->used || (is_end && set))
	{
		//if robin is full or got end hint, remove the front feature in the robin.
		wtk_delta_flush_robin(delta,rb);
	}
	++index;
	if(index<delta->cfg->order)
	{
		return wtk_delta_feed_fix_order(delta,feat,index,is_end);
	}
//	wtk_debug("return feat=%d used=%d\n",feat->index,feat->used);
	return feat;
}

void wtk_delta_feed_fix(wtk_delta_t *delta,wtk_kfeat_t *feat)
{
	//wtk_debug("feat fix feat=%d/%d\n",feat->index,feat->used);
	feat=wtk_delta_feed_fix_order(delta,feat,0,0);
	if(feat)
	{
		wtk_kparm_on_delta(delta->parm,feat);
	}
}


void wtk_delta_flush_fix(wtk_delta_t *delta)
{
	int i;
	wtk_kfeat_t *feat;
	wtk_robin_t *rb;

//	wtk_debug("============= flush fix order=%d ===========\n",delta->cfg->order);
//	for(i=0;i<delta->cfg->order;++i)
//	{
//		{
//			int j;
//			wtk_kfeat_t *f;
//
//			rb=delta->robin[i];
//			for(j=0;j<rb->used;++j)
//			{
//				f=(wtk_kfeat_t*)wtk_robin_at(rb,j);
//				wtk_debug("v[%d]=%d/%d\n",i,f->index,f->used);
//			}
//		}
//	}
//	wtk_debug("============= flush fix2 ===========\n");
	for(i=0;i<delta->cfg->order;++i)
	{
//		{
//			int j;
//			wtk_kfeat_t *f;
//
//			rb=delta->robin[i];
//			for(j=0;j<rb->used;++j)
//			{
//				f=(wtk_kfeat_t*)wtk_robin_at(rb,j);
//				wtk_debug("v[%d]=%d\n",i,f->index);
//			}
//		}
		while(1)
		{
			feat=wtk_delta_feed_fix_order(delta,NULL,i,1);
			if(!feat)
			{
				break;
			}
			wtk_kparm_on_delta(delta->parm,feat);
		}
		rb=delta->robin[i];
//		{
//			int j;
//			wtk_kfeat_t *f;
//
//			rb=delta->robin[i];
//			for(j=0;j<rb->used;++j)
//			{
//				f=(wtk_kfeat_t*)wtk_robin_at(rb,j);
//				wtk_debug("+++ v[%d]=%d\n",i,f->index);
//			}
//		}
		while(rb->used>0)
		{
			wtk_delta_flush_robin(delta,rb);
		}
		//wtk_debug("v[%d]=%d\n",i,delta->robin[i]->used);
	}
}
