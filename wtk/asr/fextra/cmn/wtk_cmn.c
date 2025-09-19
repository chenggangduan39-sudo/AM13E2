#include "wtk_cmn.h"
#include "../wtk_fextra.h"

wtk_cmn_t* wtk_cmn_new(wtk_cmn_cfg_t *cfg,struct wtk_fextra *parm)
{
	wtk_cmn_t *z;

	z=(wtk_cmn_t*)wtk_malloc(sizeof(*z));
	z->cfg=cfg;
	z->frames=0;
	z->parm=parm;
	//z->vec_size=parm->feature_cols;//wtk_vector_size(cfg->cmn_def);
	if(cfg->cmn_def)
	{
		z->vec_size=wtk_vector_size(cfg->cmn_def);
	}else
	{
		z->vec_size=parm->cfg->static_feature_cols;
	}
	if(cfg->cmn_def)
	{
		z->cur=wtk_vector_dup(cfg->cmn_def);
	}else
	{
		z->cur=wtk_vector_new(z->vec_size);
		wtk_vector_zero(z->cur);
	}
	z->save_cmn=0;
	z->use_post2=0;
	wtk_queue_init(&(z->cmn_q));
	z->buf=wtk_vector_new(z->vec_size);
	wtk_vector_zero(z->buf);
	wtk_cmn_reset(z);
	return z;
}

int wtk_cmn_delete(wtk_cmn_t *z)
{
	wtk_vector_delete(z->cur);
	wtk_vector_delete(z->buf);
	wtk_free(z);
	return 0;
}

wtk_cmn_feat_t*  wtk_cmn_new_feat(int idx,float *pv,int size)
{
	wtk_cmn_feat_t *feat;

	feat=(wtk_cmn_feat_t*)wtk_malloc(sizeof(wtk_cmn_feat_t));
	feat->idx=idx;
	feat->feat=(float*)wtk_calloc(size,sizeof(float));
	memcpy(feat->feat,pv,size*sizeof(float));
	return feat;
}

void  wtk_cmn_feat_delete(wtk_cmn_feat_t *f)
{
	wtk_free(f->feat);
	wtk_free(f);
}

void wtk_cmn_clean_cmn_q(wtk_cmn_t *cmn)
{
	wtk_queue_node_t *qn;
	wtk_cmn_feat_t *feat;

	while(1)
	{
		qn=wtk_queue_pop(&(cmn->cmn_q));
		if(!qn){break;}
		feat=data_offset2(qn,wtk_cmn_feat_t,q_n);
		wtk_cmn_feat_delete(feat);
	}
}

int wtk_cmn_reset(wtk_cmn_t *z)
{
	if(z->cmn_q.length>0)
	{
		wtk_cmn_clean_cmn_q(z);
	}
	wtk_queue_init(&(z->post_feature_q));
	//wtk_queue_init(&(z->pend_q));
	if(!z->cfg->save_cmn)
	{
		z->frames=0;
		if(z->cfg->cmn_def)
		{
			wtk_vector_cpy(z->cfg->cmn_def,z->cur);
		}else
		{
			wtk_vector_zero(z->cur);
		}
		if(z->buf)
		{
			wtk_vector_zero(z->buf);
		}
		z->use_post2=0;
	}else
	{
	}
	//wtk_debug("===========================================> frames=%d\n",z->frames);
	return 0;
}

void wtk_cmn_dup_hist(wtk_cmn_t *dst,wtk_cmn_t *src)
{
	dst->frames=src->frames;
	if(dst->cfg->max_cmn_frame>0 && dst->frames>dst->cfg->max_cmn_frame)
	{
		dst->frames=dst->cfg->max_cmn_frame;
	}
	dst->use_post2=src->use_post2;
	wtk_vector_cpy(src->cur,dst->cur);
	wtk_vector_cpy(src->buf,dst->buf);
}

#ifdef USE_RAW

void wtk_cmn_update_buf(wtk_cmn_t *z,wtk_vector_t *v)
{
	wtk_vector_t *buf=z->buf;
	int i,n=z->vec_size;

	++z->frames;
	for(i=1;i<=n;++i)
	{
		buf[i]+=v[i];
	}
}

void wtk_cmn_update_cmn(wtk_cmn_t *z)
{
	wtk_vector_t *buf=z->buf;
	int i,n=z->vec_size;
	wtk_vector_t *cur=z->cur;

	for(i=1;i<=n;++i)
	{
		cur[i]=buf[i]/z->frames;
	}
}

#else

//void wtk_cmn_update_buf(wtk_cmn_t *z,wtk_vector_t *v)
//{
//	wtk_vector_t *buf=z->buf;
//	int i,n=z->vec_size;
//	float f1,f2;
//
//	f1=0.99;
//	f2=1-f1;
//	for(i=1;i<=n;++i)
//	{
//		//buf[i]=(buf[i]*last_n+v[i])/z->frames;
//		buf[i]=buf[i]*f1+v[i]*f2;
//	}
//}

void wtk_cmn_update_buf(wtk_cmn_t *z,wtk_vector_t *v)
{
	wtk_vector_t *buf=z->buf;
	int i,n=z->vec_size;
	int last_n;
	float t;

	last_n=z->frames;
	if(z->cfg->max_cmn_frame>0 && z->frames>z->cfg->max_cmn_frame)
	{
		last_n=z->cfg->max_cmn_frame;
		if(z->cfg->post_update_frame2>=0)
		{
			z->use_post2=1;
		}
	}
	t=1.0/(last_n+1);
	++z->frames;
	for(i=1;i<=n;++i)
	{
		//buf[i]=(buf[i]*last_n+v[i])/z->frames;
		buf[i]=(buf[i]*last_n+v[i])*t;
	}
}

void wtk_cmn_update_cmn(wtk_cmn_t *z)
{
	wtk_vector_t *buf=z->buf;
	int n=z->vec_size;
	wtk_vector_t *cur=z->cur;

	memcpy(cur+1,buf+1,n*sizeof(float));
//	for(i=1;i<=n;++i)
//	{
//		cur[i]=buf[i];
//		//printf("%d: v[%d]=%f\n",z->frames,i,cur[i]);
//	}
//	if(z->cfg->max_cmn_frame>0 && z->frames>z->cfg->max_cmn_frame)
//	{
//		z->frames=z->cfg->max_cmn_frame;
//	}
}
#endif

void wtk_cmn_update_cmn_q(wtk_cmn_t *z,int idx)
{
	wtk_queue_t *q=&(z->cmn_q);
	wtk_vector_t *cmn=z->cur;
	wtk_queue_node_t *qn;
	wtk_cmn_feat_t *feat;
	int s,e;
	int i;
	float f;
	int cnt;

	s=max(idx-z->cfg->post_left_frame,1);
	e=idx+z->cfg->post_update_frame;
	wtk_vector_zero(cmn);
	cnt=0;
	for(qn=q->pop;qn;qn=qn->next)
	{
		feat=data_offset2(qn,wtk_cmn_feat_t,q_n);
		if(feat->idx>=s && feat->idx<=e)
		{
			++cnt;
			//wtk_debug("idx=[%d,%d] [%d,%d] [%d,%d]\n",idx,feat->idx,s,e,z->cfg->post_left_frame,z->cfg->post_update_frame);
			for(i=0;i<z->vec_size;++i)
			{
				cmn[i+1]+=feat->feat[i];
			}
		}
	}
	//wtk_debug("idx=%d[%d,%d]\n",idx,min_s,min_e);
	//wtk_vector_print(cmn);
	if(z->cfg->alpha>0)
	{
		f=(1-z->cfg->alpha)/cnt;
		for(i=1;i<=z->vec_size;++i)
		{
			cmn[i]=z->buf[i]*z->cfg->alpha+f*cmn[i];
			//cmn[i]=(z->buf[i]*z->frames-cmn[i]*cnt)*z->cfg->alpha/(z->frames-cnt)+(1-z->cfg->alpha)*cmn[i];
		}
	}else
	{
		f=1.0/cnt;
		for(i=1;i<=z->vec_size;++i)
		{
			cmn[i]*=f;
		}
	}
	s=idx-z->cfg->post_left_frame;
	if(s>0)
	{
		while(1)
		{
			qn=q->pop;
			if(!qn){break;}
			feat=data_offset2(qn,wtk_cmn_feat_t,q_n);
			if(feat->idx<=s)
			{
				wtk_queue_pop(q);
				//wtk_debug("pop idx=%d s=%d\n",feat->idx,s);
				wtk_cmn_feat_delete(feat);
			}else
			{
				break;
			}
		}
	}
}

void wtk_cmn_process_cmn(wtk_cmn_t *z,wtk_vector_t *feat,int idx)
{
	wtk_vector_t *cmn;
	int i,n=z->vec_size;

	if(!z->cfg->use_hist)
	{
		wtk_cmn_update_cmn_q(z,idx);
	}
	cmn=z->cur;
	for(i=1;i<=n;++i)
	{
		feat[i]-=cmn[i];
		//wtk_debug("%p,v[%d]=%f,%f,vs=%d\n",feat,i,feat[i],cmn[i],z->vec_size);
	}
}
void wtk_cmn_raise(wtk_cmn_t *cmn,wtk_feat_t *f);
void wtk_cmn_static_post_feed2(wtk_cmn_t *z,wtk_feat_t *f)
{
	wtk_cmn_feat_t *cmn_feat;
	wtk_feat_t *feat;
	wtk_vector_t *buf=z->buf;
	int i,n=z->vec_size;
	wtk_queue_t* q = &z->cmn_q;
	wtk_queue_t* q2 = &z->post_feature_q;
	wtk_queue_node_t *qn,*qn2;

	cmn_feat=wtk_cmn_new_feat(f->index,f->v+1,z->vec_size);
	wtk_queue_push(&(z->cmn_q),&(cmn_feat->q_n));

	++f->used;
	wtk_queue_push(&(z->post_feature_q),&(f->queue_n));

	for(i=1;i<=n;++i)
	{
		buf[i]+=f->v[i];
	}
	//wtk_debug("%d\n",f->index);
	//print_double(db,65);
	//wtk_vector_print(buf);
	if(f->index < 300)
	{
		z->frames++;
	}else if(f->index == 300)
	{
		z->frames++;
		for(i=1;i<=n;++i)
		{
			z->cur[i] = buf[i]/z->frames;
		}
		//print_double(db,65);
		while(1)
		{
			qn=wtk_queue_pop(q2);
			if(!qn){break;}
			feat=data_offset(qn,wtk_feat_t,queue_n);
			for(i=1;i<=n;++i)
			{
				feat->v[i] -= z->cur[i];
			}
			--feat->used;
			i=feat->index;
			wtk_cmn_raise(z,feat);
			if(i == 151){break;}
		}
	}else
	{
		qn=wtk_queue_pop(q2);
		feat=data_offset(qn,wtk_feat_t,queue_n);

		qn2=wtk_queue_pop(q);
		cmn_feat=data_offset(qn2,wtk_cmn_feat_t,q_n);

		for(i=1;i<=n;++i)
		{
			buf[i]=buf[i]-cmn_feat->feat[i-1];
		}
		for(i=1;i<=n;++i)
		{
			z->cur[i] = buf[i]/z->frames;
			feat->v[i]-=z->cur[i];
		}
		wtk_cmn_feat_delete(cmn_feat);
		--f->used;
		wtk_cmn_raise(z,feat);
	}
}

void wtk_cmn_static_post_feed(wtk_cmn_t *z,wtk_feat_t *f)
{
	//wtk_debug("index=%d frames=%d\n",f->index,z->frames);
	//print_float(f->v,24);wtk_vector_size(f->v));
	if(z->cfg->use_hist)
	{
		wtk_cmn_update_buf(z,f->v);
		if(z->frames>=z->cfg->start_min_frame)
		{
			wtk_cmn_update_cmn(z);
		}
	}else
	{
		wtk_cmn_feat_t *feat;

		++z->frames;
		wtk_cmn_update_buf(z,f->v);
		feat=wtk_cmn_new_feat(f->index,f->v+1,z->vec_size);
		wtk_queue_push(&(z->cmn_q),&(feat->q_n));
	}
}

void wtk_cmn_raise(wtk_cmn_t *cmn,wtk_feat_t *f)
{
	if(cmn->parm->cfg->use_cmn2)
	{
		//wtk_fextra_feed_static_input_feature(cmn->parm,f);
		//wtk_vector_print(f->v);
		wtk_fextra_output_feature(cmn->parm,f);
	}else
	{
		wtk_fextra_output_feature(cmn->parm,f);
	}
}

void wtk_zmean_flush_queue(wtk_cmn_t *z,wtk_queue_t *q)
{
	wtk_queue_node_t *n;
	wtk_feat_t *f;

	while(1)
	{
		n=wtk_queue_pop(q);
		if(!n){break;}
		f=data_offset(n,wtk_feat_t,queue_n);
		wtk_cmn_process_cmn(z,f->v,f->index);
		//wtk_debug("v[%d]=%d %p\n",f->index,f->used,f);
		--f->used;
		wtk_cmn_raise(z,f);
	}
}

void wtk_cmn_flush_extra_post_queue(wtk_cmn_t *z)
{
	if(z->cfg->use_hist && z->frames<z->cfg->start_min_frame)
	{
		//cmn not update for min frame is long enough;
		wtk_cmn_update_cmn(z);
	}
	//wtk_zmean_flush_queue(z,&(z->pend_q));
	wtk_zmean_flush_queue(z,&(z->post_feature_q));
}

void wtk_cmn_flush_extra_post_queue2(wtk_cmn_t *z)
{
	wtk_queue_node_t *n;
	wtk_feat_t *f;
	wtk_queue_t *q = &(z->post_feature_q);
	int i,j;
	int size=wtk_vector_size(z->cur);
	wtk_vector_t *cmn=z->cur;
	j = z->vec_size;

	if(z->frames<300)
	{
		for(i=1;i<=size;++i)
		{
			z->cur[i] = z->buf[i]/z->frames;
		}
	}

	while(1)
	{
		n=wtk_queue_pop(q);
		if(!n){break;}
		f=data_offset(n,wtk_feat_t,queue_n);
		for(i=1;i<=j;++i)
		{
			f->v[i]-=cmn[i];
		}
		--f->used;
		wtk_cmn_raise(z,f);
	}
}

void wtk_zmean_flush_one_feature(wtk_cmn_t *z)
{
	wtk_queue_t *q=&(z->post_feature_q);
	wtk_queue_node_t *n;
	wtk_feat_t *f;

	while(1)
	{
		n=wtk_queue_pop(q);
		if(!n){break;}
		f=data_offset(n,wtk_feat_t,queue_n);
		wtk_cmn_process_cmn(z,f->v,f->index);
		--f->used;
		wtk_cmn_raise(z,f);
		//wtk_fextra_output_feature(z->parm,f);
		break;
	}
}

void wtk_zmean_post_feed2(wtk_cmn_t *z,wtk_feat_t *f)
{
	wtk_queue_t *q=&(z->post_feature_q);

	++f->used;
	wtk_queue_push(q,&(f->queue_n));
	//wtk_debug("len=%d,frame=%d.\n",q->length,v->cfg->post_update_frame);
	if(z->use_post2)
	{
		if((z->frames>=z->cfg->start_min_frame) && (z->cfg->post_update_frame2>=0) && (q->length>=z->cfg->post_update_frame2))
		{
			//wtk_zmean_flush_parm_post_queue(z);
			wtk_zmean_flush_one_feature(z);
		}
	}else
	{
		if((z->frames>=z->cfg->start_min_frame) && (z->cfg->post_update_frame>=0) && (q->length>=z->cfg->post_update_frame))
		{
			//wtk_zmean_flush_parm_post_queue(z);
			wtk_zmean_flush_one_feature(z);
		}
	}
}

void wtk_cmn_post_feed(wtk_cmn_t *z,wtk_feat_t *f)
{
	//wtk_debug("frames=%d idx=%d q=%d/%d\n",z->frames,f->index,z->cmn_q.length,z->post_feature_q.length);
	if(z->frames>=z->cfg->start_min_frame&&!z->cfg->use_whole)
	{
//	wtk_debug("frames=%d idx=%d q=%d/%d\n",z->frames,f->index,z->cmn_q.length,z->post_feature_q.length);
		/*
		if(z->pend_q.length>0)
		{
			//for start
			++f->used;
			wtk_queue_push(&(z->pend_q),&(f->queue_n));
			wtk_zmean_flush_queue(z,&(z->pend_q));
		}else
		*/
		{

			if(z->cfg->left_seek_frame>0 && z->frames>=z->cfg->left_seek_frame)
			{
				if(z->cfg->min_flush_frame<=0)
				{
					if(z->post_feature_q.length>0)
					{
						wtk_zmean_flush_queue(z,&(z->post_feature_q));
					}
					wtk_cmn_process_cmn(z,f->v,f->index);
					wtk_cmn_raise(z,f);
					//wtk_fextra_output_feature(z->parm,f);
				}else
				{
					++f->used;
					wtk_queue_push(&(z->post_feature_q),&(f->queue_n));
				}
			}else
			{
				++f->used;
				wtk_queue_push(&(z->post_feature_q),&(f->queue_n));
				if(z->use_post2)
				{
					if(((z->cfg->post_update_frame<=0) || (z->post_feature_q.length>=z->cfg->post_update_frame2)))
					{
						//wtk_zmean_flush_parm_post_queue(z);
						if(z->cfg->smooth)
						{
							wtk_zmean_flush_one_feature(z);
						}else
						{
							wtk_zmean_flush_queue(z,&(z->post_feature_q));
						}
					}
				}else
				{
					if(((z->cfg->post_update_frame<=0) || (z->post_feature_q.length>=z->cfg->post_update_frame)))
					{
						//wtk_zmean_flush_parm_post_queue(z);
						if(z->cfg->smooth)
						{
							wtk_zmean_flush_one_feature(z);
						}else
						{
							wtk_zmean_flush_queue(z,&(z->post_feature_q));
						}
					}
				}
			}
		}
	}else
	{
		++f->used;
		wtk_queue_push(&(z->post_feature_q),&(f->queue_n));
	}
}

void wtk_cmn_flush(wtk_cmn_t *z,int force)
{
	int min_frame;
	int b;

	if(force)
	{
		b=1;
	}else
	{
		min_frame=z->cfg->min_flush_frame;
		if(min_frame>0 && z->frames>=z->cfg->start_min_frame && z->cfg->left_seek_frame>0
			&& z->frames>=z->cfg->left_seek_frame && z->post_feature_q.length>=min_frame)
		{
			b=1;
		}else
		{
			b=0;
		}
	}
	if(b)
	{
		wtk_zmean_flush_queue(z,&(z->post_feature_q));
	}
}

int wtk_cmn_can_flush_all(wtk_cmn_t *z)
{
	int b;

	if(z->cfg->left_seek_frame>0 && z->frames>=z->cfg->left_seek_frame)
	{
		b=1;
	}else
	{
		b=0;
	}
	return b;
}


