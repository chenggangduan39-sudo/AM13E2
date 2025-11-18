#include "wtk_wakeup.h" 

wtk_wakeup_feat_t* wtk_wakeup_feat_new(int n)
{
	wtk_wakeup_feat_t *f;

	f=(wtk_wakeup_feat_t*)wtk_malloc(sizeof(wtk_wakeup_feat_t));
	f->index=0;
	f->v=(float*)wtk_malloc(sizeof(float)*n);
	return f;
}

void wtk_wakeup_feat_delete(wtk_wakeup_feat_t *feat)
{
	wtk_free(feat->v);
	wtk_free(feat);
}

wtk_wakeup_t* wtk_wakeup_new(wtk_wakeup_cfg_t *cfg)
{
	wtk_wakeup_t *wakeup;
	int i;

	wakeup=(wtk_wakeup_t*)wtk_malloc(sizeof(wtk_wakeup_t));
	wakeup->cfg=cfg;
	wakeup->wav_cnt=0;
	wtk_queue_init(&(wakeup->param_q));
	wakeup->parm=wtk_fextra_new(&(cfg->parm));
	//wtk_fextra_set_notify(wakeup->parm,(wtk_fextra_feature_notify_f),wakeup);
	wakeup->parm->output_queue=&(wakeup->param_q);
	wakeup->input_vec=wtk_vecf_new(cfg->parm.dnn.out_cols);
	wakeup->post_cfg=NULL;
	wakeup->vec_smooth=wtk_vecf_new(wakeup->cfg->parm.dnn.out_cols);
	wakeup->feat_robin=NULL;

	wakeup->check_valid=NULL;
	wakeup->check_valid_ths=NULL;
	wakeup->inst_pool=NULL;
	wakeup->pth_pool=NULL;
	wakeup->win_robin=NULL;
	if(cfg->use_win)
	{
		wakeup->win_robin=wtk_robin_new(cfg->max_win);
		for(i=0;i<wakeup->win_robin->nslot;++i)
		{
			wakeup->win_robin->r[i]=(float*)wtk_calloc(cfg->post.n_network,sizeof(float));
		}
	}else
	{
		wakeup->inst_pool=wtk_vpool_new(sizeof(wtk_wakeup_inst_t),cfg->inst_cache);
		wakeup->pth_pool=wtk_vpool_new(sizeof(wtk_wakeup_inst_t),cfg->pth_cache);
		wtk_queue_init(&(wakeup->inst_q));
	}
	wakeup->buf=wtk_strbuf_new(256,1);
	if(wakeup->cfg->log_wav)
	{
		wakeup->wav_buf=wtk_strbuf_new(10240,1);
	}else
	{
		wakeup->wav_buf=NULL;
	}
	wakeup->notify=NULL;
	wakeup->notify_ths=NULL;
	wtk_wakeup_reset(wakeup);
	return wakeup;
}

void wtk_wakeup_set_feature_notify(wtk_wakeup_t *wakeup,void *ths,wtk_wakeup_notify_feat_f notify)
{
	wakeup->notify=notify;
	wakeup->notify_ths=ths;
}

void wtk_wakeup_delete(wtk_wakeup_t *wakeup)
{
	int i;

	if(wakeup->wav_buf)
	{
		wtk_strbuf_delete(wakeup->wav_buf);
	}
	wtk_strbuf_delete(wakeup->buf);
	if(wakeup->feat_robin)
	{
		for(i=0;i<wakeup->feat_robin->nslot;++i)
		{
			wtk_wakeup_feat_delete((wtk_wakeup_feat_t*)(wakeup->feat_robin->r[i]));
		}
		wtk_robin_delete(wakeup->feat_robin);
	}
	wtk_vecf_delete(wakeup->vec_smooth);
	if(wakeup->inst_pool)
	{
		wtk_vpool_delete(wakeup->inst_pool);
		wtk_vpool_delete(wakeup->pth_pool);
	}else
	{
		for(i=0;i<wakeup->win_robin->nslot;++i)
		{
			wtk_free(wakeup->win_robin->r[i]);
		}
		wtk_robin_delete(wakeup->win_robin);
	}
	wtk_vecf_delete(wakeup->input_vec);
	wtk_fextra_delete(wakeup->parm);
	wtk_free(wakeup);
}

void wtk_wakeup_set_check_valid(wtk_wakeup_t *wakeup,void *ths,wtk_wakeup_check_valid_f check)
{
	wakeup->check_valid=check;
	wakeup->check_valid_ths=ths;
}

int wtk_wakeup_start(wtk_wakeup_t *wakeup)
{
	return wtk_wakeup_start2(wakeup,&(wakeup->cfg->post));
}

void wtk_wakeup_set_post(wtk_wakeup_t *wakeup,wtk_wakeup_post_cfg_t *post)
{
	if(post==wakeup->post_cfg){return;}
	wakeup->post_cfg=post;
}

int wtk_wakeup_start2(wtk_wakeup_t *wakeup,wtk_wakeup_post_cfg_t *post)
{
	int nx;
	int i;

	wakeup->post_cfg=post;
	wakeup->prob=0;
	//wtk_debug("================= smooth  left=%d right=%d\n",post->smooth_left,post->smooth_right);
	if(post->smooth_left>0 ||post->smooth_right>0)
	{
		nx=post->smooth_left+post->smooth_right+1;
		if(wakeup->feat_robin==NULL || (wakeup->feat_robin->nslot!=nx))
		{
			if(wakeup->feat_robin)
			{
				for(i=0;i<wakeup->feat_robin->nslot;++i)
				{
					wtk_wakeup_feat_delete((wtk_wakeup_feat_t*)(wakeup->feat_robin->r[i]));
				}
				wtk_robin_delete(wakeup->feat_robin);
			}
			wakeup->feat_robin=wtk_robin_new(nx);
			for(i=0;i<wakeup->feat_robin->nslot;++i)
			{
				wakeup->feat_robin->r[i]=wtk_wakeup_feat_new(wakeup->vec_smooth->len);
			}
		}
	}else
	{
		if(wakeup->feat_robin)
		{
			for(i=0;i<wakeup->feat_robin->nslot;++i)
			{
				wtk_wakeup_feat_delete((wtk_wakeup_feat_t*)(wakeup->feat_robin->r[i]));
			}
			wtk_robin_delete(wakeup->feat_robin);
			wakeup->feat_robin=NULL;
		}
		wakeup->feat_robin=NULL;
	}
	return 0;
}

int wtk_wakeup_reset(wtk_wakeup_t *wakeup)
{
	wakeup->skip_post=0;
	wakeup->get=0;
	wakeup->poped=0;
	//wtk_debug("=============== reset wake ===================\n");
	if(wakeup->wav_buf)
	{
		wtk_strbuf_reset(wakeup->wav_buf);
	}
	wtk_strbuf_reset(wakeup->buf);
	wakeup->thresh=0;
	wakeup->final_inst=NULL;
	wakeup->count=0;
	wakeup->idx=-1;
	if(wakeup->cfg->use_win)
	{
		wtk_robin_reset(wakeup->win_robin);
	}else
	{
		wtk_vpool_reset(wakeup->inst_pool);
		wtk_vpool_reset(wakeup->pth_pool);
		wtk_queue_init(&(wakeup->inst_q));
	}
	wtk_vecf_zero(wakeup->vec_smooth);
	if(wakeup->feat_robin)
	{
		wtk_robin_reset(wakeup->feat_robin);
	}
	//wtk_debug("=================================== reset wake parm\n");
	wtk_fextra_reset(wakeup->parm);
	//wtk_debug("=================================== reset wake2 parm\n");
	return 0;
}

wtk_wakeup_inst_t* wtk_wakeup_inst_new()
{
	wtk_wakeup_inst_t* inst;

	inst=(wtk_wakeup_inst_t*)wtk_malloc(sizeof(wtk_wakeup_inst_t));
	inst->node=NULL;
	inst->frame_s=0;
	inst->frame_e=0;
	inst->prob=0;
	inst->max_prob=0;
	inst->max_prob_frame=0;
	inst->pth=NULL;
	inst->pre_prob=0;
	return inst;
}

void wtk_wakeup_inst_init(wtk_wakeup_inst_t* inst)
{
	inst->node=NULL;
	inst->frame_s=0;
	inst->frame_e=0;
	inst->prob=0;
	inst->max_prob=0;
	inst->pth=NULL;
	inst->pre_prob=0;
	inst->rise_count=0;
	//inst->max_frame = 0;
}

void wtk_wakeup_inst_delete(wtk_wakeup_inst_t *inst)
{
	wtk_free(inst);
}

wtk_wakeup_inst_t* wtk_wakeup_pop_inst(wtk_wakeup_t *wakeup)
{
	wtk_wakeup_inst_t *inst;

	inst=(wtk_wakeup_inst_t*)wtk_vpool_pop(wakeup->inst_pool);
	//wtk_debug("pop inst=%p\n",inst);
	wtk_wakeup_inst_init(inst);
	return inst;
}

void wtk_wakeup_check_path(wtk_wakeup_inst_t *inst)
{
	wtk_wakeup_path_t *pth;
	static int ki=0;

	//++ki;
	pth=inst->pth;
	while(pth)
	{
		if(pth->max_prob_frame==15)
		{
			wtk_debug("v[%d] frame=%d prob=%f\n",++ki,pth->max_prob_frame,pth->prob);
			wtk_wakeup_path_print(inst->pth);
			//exit(0);
			if(ki==17)
			{
				exit(0);
			}
		}
		pth=pth->prev;
	}
}

void wtk_wakeup_push_inst(wtk_wakeup_t *wakeup,wtk_wakeup_inst_t *inst)
{
	//wtk_debug("push inst=%p\n",inst);
	//wtk_wakeup_check_path(inst);
	wtk_vpool_push(wakeup->inst_pool,inst);
}

wtk_wakeup_path_t* wtk_wakeup_path_new()
{
	wtk_wakeup_path_t *pth;

	pth=(wtk_wakeup_path_t*)wtk_malloc(sizeof(wtk_wakeup_path_t));
	pth->prev=NULL;
	pth->prob=0;
	pth->frame_s=0;
	pth->frame_e=0;
	return pth;
}

void wtk_wakeup_path_init(wtk_wakeup_path_t *pth)
{
	pth->prev=NULL;
	pth->prob=0;
	pth->frame_init=0;
	pth->frame_s=0;
	pth->frame_e=0;
}

void wtk_wakeup_path_delete(wtk_wakeup_path_t *pth)
{
	wtk_free(pth);
}

wtk_wakeup_path_t* wtk_wakeup_pop_path(wtk_wakeup_t *wakeup)
{
	wtk_wakeup_path_t *pth;

	pth=(wtk_wakeup_path_t*)wtk_vpool_pop(wakeup->pth_pool);
	wtk_wakeup_path_init(pth);
	return pth;
}

void wtk_wakeup_push_path(wtk_wakeup_t *wakeup,wtk_wakeup_path_t *pth)
{
	wtk_vpool_push(wakeup->pth_pool,pth);
}

int wtk_wakeup_path_depth(wtk_wakeup_path_t *pth)
{
	int d=0;

	while(pth)
	{
		++d;
		pth=pth->prev;
	}
	return d;
}

void wtk_wakeup_path_print(wtk_wakeup_path_t *pth)
{
	//wtk_debug("pth=%p\n",pth);
	if(!pth){return;}
	if(pth->prev)
	{
		wtk_wakeup_path_print(pth->prev);
	}
	wtk_debug("v[%d/%d-%d]=[%d=%f] pth=%p\n",wtk_wakeup_path_depth(pth),pth->frame_s,pth->frame_e,pth->max_prob_frame,pth->prob,pth);
}

int wtk_wakeup_path_first_frame(wtk_wakeup_path_t *pth)
{
	int v=0;

	while(pth)
	{
		wtk_debug("frame=%d\n",pth->max_prob_frame);
		if(!pth->prev)
		{
			v=pth->max_prob_frame;
		}
		pth=pth->prev;
	}
	return v;
}

int wtk_wakeup_path_frames(wtk_wakeup_path_t *pth)
{
	int ne;

	ne=pth->max_prob_frame;
	while(pth->prev)
	{
		pth=pth->prev;
	}
	return ne-pth->max_prob_frame;
}

wtk_wakeup_inst_t* wtk_wakeup_add_inst(wtk_wakeup_t *wakeup,wtk_wakeup_inst_t *prev_inst,float pth_prob,int start_frame)
{
	wtk_wakeup_inst_t *inst;
	wtk_wakeup_path_t *pth;
	int frame;
	float f;

	if(prev_inst->pth && prev_inst->node->pth_thresh2>0)
	{
		f=sqrt(prev_inst->pth->prob*prev_inst->max_prob);
		//wtk_debug("%f/%f=%f/%f\n",prev_inst->pth->prob,prev_inst->max_prob,f,prev_inst->node->pth_thresh2);
		if(f<prev_inst->node->pth_thresh2)
		{
			wtk_debug("%f/%f=%f/%f\n",prev_inst->pth->prob,prev_inst->max_prob,f,prev_inst->node->pth_thresh2);
			return NULL;
		}
	}
	pth=wtk_wakeup_pop_path(wakeup);
	pth->prev=prev_inst->pth;
	pth->frame_init = start_frame;
	pth->frame_s=prev_inst->frame_s;
	pth->frame_e=prev_inst->frame_e;
	//wtk_debug("%d/%d max=%d\n",pth->frame_s,pth->frame_e,prev_inst->node->max_frame);
	pth->max_prob=prev_inst->prob;
	pth->prob=prev_inst->max_prob;
	pth->max_prob_frame=prev_inst->max_prob_frame;

//	if(pth->frame_e-pth->max_prob_frame>prev_inst->node->max_frame)
//	{
//		wtk_debug("found bug\n");
//		wtk_wakeup_path_print(pth);
//		exit(0);
//	}


	//prev_inst->max_prob = 0.0;

//	wtk_debug("==========================\n");
//	wtk_wakeup_path_print(pth);

	inst=wtk_wakeup_pop_inst(wakeup);
	inst->pth=pth;
	inst->prob=0;
	inst->pre_prob=pth_prob;
	//wtk_debug("%d %d\n",prev_inst->node->depth,wakeup->cfg->n_network-1);
	if(prev_inst->node->depth==wakeup->post_cfg->n_network-1)
	{
		//wtk_wakeup_path_print(pth);
		inst->node=NULL;
		frame=wtk_wakeup_path_frames(pth);
		if(frame<wakeup->post_cfg->min_frame || frame>100)
		{
			wtk_wakeup_push_inst(wakeup,inst);
			return NULL;
		}else
		{
			return inst;
		}
	}
	//wtk_debug("v[%d]=%d/%f/%f\n",wakeup->frame,prev_inst->node->depth,pth_prob,prev_inst->max_prob);
	inst->node=wakeup->post_cfg->network+prev_inst->node->depth+1;
	inst->frame_s=inst->frame_e=wakeup->frame+1;
	inst->max_prob_frame=0;

	//wtk_debug("inst=%p prob=%f\n",inst,inst->prob);
	wtk_queue_push(&(wakeup->inst_q),&(inst->q_n));
//	if(wakeup->cfg->debug)
//	{
//		wtk_debug("add inst depth=%d prob=%f\n",inst->node->depth,prev_inst->max_prob);
//	}
	return inst;
}

/**
 * return 0 for prune, 1 for continue,2 for end
 */
int wtk_wakeup_update_inst(wtk_wakeup_t *wakeup,wtk_wakeup_inst_t *inst,wtk_vecf_t *vec)
{
	float f,f2;
	int n;
	wtk_wakeup_inst_t *inst2;
	int prev_frame,start_frame;
	int depth;
	int enter = 0;
	int b;

	start_frame = 0;
	depth  = inst->node->depth;
	f=vec->p[inst->node->idx];
	//wtk_debug("f=%f/%f\n",f,inst->max_prob);
	if(f>inst->max_prob){
		inst->rise_count++;
		//wtk_debug("%d\n",inst->rise_count);
	}else{
		if(inst->rise_count  >= wakeup->post_cfg->min_raise){
			if(depth == 0)
			{
				start_frame =  wakeup->frame - inst->rise_count;
			}
			inst->rise_count = 0;
			enter  = 1;
		}
		//inst->rise_count = 0;
		else{
			inst->max_prob = 0;
			inst->rise_count = 0;
			//wtk_wakeup_push_inst(wakeup,inst);
			//return 0;
		}
	}
	//}

	if(inst->frame_s==0)
	{
		inst->frame_s=wakeup->frame;
	}
	inst->frame_e=wakeup->frame;
	//wtk_debug("f[%d]=%f\n",wakeup->frame,f);
	n=inst->frame_e-inst->frame_s;
	//wtk_debug("f[%d]=%f/%f\n",n,f,inst->max_prob);
	if(f>inst->max_prob)
	{
		inst->max_prob=f;
		inst->max_prob_frame=wakeup->frame;
		//wtk_debug("inst=%p pre=%f max=%f pth=%p\n",inst,inst->pre_prob,inst->max_prob,inst->pth);
		if(inst->pth)
		{
			f=inst->pre_prob*inst->max_prob;
		}else
		{
			f=inst->max_prob;
		}
		//wtk_debug("inst=%p prob=%f\n",inst,f);
		//wtk_debug("f=%f\n",f);
		if(inst->node->depth>0)
		{
			f=pow(f,1.0/(inst->node->depth+1));
			//wtk_wakeup_path_print(inst->pth);
		}
		//wtk_debug("inst=%p prob=%f\n",inst,f);
		inst->prob=f;
		//wtk_debug("max=%f/%f pth=%f/%f depth=%d fs=%d\n",inst->max_prob,inst->node->cur_thresh,inst->prob,inst->node->pth_thresh,wtk_wakeup_path_depth(inst->pth),inst->frame_s);
		//wtk_wakeup_path_print(inst->pth);
		//wtk_debug("prob=%f/%f\n",inst->max_prob,inst->node->cur_thresh);
	}
	if(enter)
	{
		if(inst->pth)
		{
			wtk_wakeup_path_t *pth;

			pth=inst->pth;
			while(pth->prev)
			{
				pth=pth->prev;
			}
			prev_frame=pth->max_prob_frame;
		}else
		{
			prev_frame=inst->frame_s;
		}
		//prev_frame=inst->pth?inst->pth->max_prob_frame:inst->frame_s;
		//wtk_debug("depth=%d %d/%d %f/%f enter=%d\n",inst->node->depth,inst->frame_e-prev_frame,inst->node->min_frame,inst->max_prob,inst->node->cur_thresh,enter);
		if(((inst->frame_e-prev_frame)>inst->node->min_frame|| !inst->pth) && inst->max_prob>=inst->node->cur_thresh)
		{
			//wtk_debug("step next %f/%f\n",f,inst->node->pth_thresh);
			b=0;
			if(inst->prob>=inst->node->pth_thresh)
			{
				b=1;
			}
			//if(inst->prob>=inst->node->pth_thresh)
			if(b)
			{
				//wtk_debug("[%d,%d]step max=%f/%f depth=%d\n",inst->frame_s,inst->frame_e,inst->max_prob,inst->node->cur_thresh,wtk_wakeup_path_depth(inst->pth));
				//wtk_debug("%f    %f\n",inst->prob,inst->node->pth_thresh);
				//wtk_wakeup_path_print(inst->pth);
				//wtk_debug("f=%f\n",f);
				if(inst->pth)
				{
					//wtk_debug("%f\n",inst->pre_prob);
					f2=inst->pre_prob*inst->max_prob;
				}else
				{
					f2=inst->max_prob;
				}
				inst2=wtk_wakeup_add_inst(wakeup,inst,f2,start_frame);
				if(inst2 && !inst2->node)
				{
					//wtk_debug("prob=%f,f2=%f\n",inst->prob,f2);
					inst2->prob=inst->prob;
					wakeup->final_inst=inst2;
					return 2;
				}
			}
		}
	}
	//wtk_debug("n[%d]=%d/%d\n",inst->node->depth,n,inst->node->max_frame);
	//wtk_debug("%f 	deep:%d\n",inst->prob,inst->node->depth);
	//wtk_wakeup_path_print(inst->pth);
	//检查是否时间到了
	//wtk_debug("n=%d max=%d depth=%d\n",n,inst->node->max_frame,inst->node->depth);
	if(n>inst->node->max_frame && inst->node->depth != 0)
	{
		//wtk_debug("================= remove ==%d=============\n",inst->node->depth);
		//wtk_wakeup_path_print(inst->pth);
		//wtk_debug("clean depth=%d prob=%f\n",inst->node->depth,inst->max_prob);
		wtk_wakeup_push_inst(wakeup,inst);
		return 0;
	}else
	{
		//wtk_debug("inst[%d]=%p prob=%f\n",ki,inst,inst->prob);
		//wtk_debug("11111\n");
		//wtk_debug("%d/%d/%d\n",inst->frame_s,inst->frame_e,inst->max_prob_frame);

		if(inst->pth && ((inst->max_prob_frame-inst->pth->max_prob_frame)>inst->node->max_frame))
		{
//			if(inst->node->depth==3)
//			{
//				wtk_wakeup_path_print(inst->pth);
//			}
			wtk_wakeup_push_inst(wakeup,inst);
			return 0;
		}else
		{
			wtk_queue_push(&(wakeup->inst_q),&(inst->q_n));
			return 1;
		}
	}
}

void wtk_wakeup_check(wtk_wakeup_t *wakeup)
{
	wtk_queue_node_t *qn;
	wtk_wakeup_inst_t *inst;

	wtk_debug("============== check inst=%d  ====================\n",wakeup->inst_q.length);
	for(qn=wakeup->inst_q.pop;qn;qn=qn->next)
	{
		inst=data_offset2(qn,wtk_wakeup_inst_t,q_n);
		wtk_debug("inst[%d,%d,%d]\n",inst->max_prob_frame,inst->frame_s,inst->frame_e);
		if(inst->pth)// && inst->node->depth==3)
		{
			wtk_wakeup_path_print(inst->pth);
		}
	}
	wtk_debug("==========================================\n");
}

/**
 * return 1 for end
 */
int wtk_wakeup_feed_vec(wtk_wakeup_t *wakeup,wtk_vecf_t *vec)
{
	wtk_queue_node_t *qn;
	wtk_wakeup_inst_t *inst;
	wtk_queue_t q;
	int b;
	int ret;

	if(wakeup->cfg->debug)
	{
		wtk_debug("==============> v[%d/%d]=%f/%f/%f/%f\n",wakeup->frame,wakeup->inst_q.length,vec->p[0],vec->p[1],vec->p[2],vec->p[3]);
	}
	b=1;
	q=wakeup->inst_q;
	wtk_queue_init(&(wakeup->inst_q));
	while(1)
	{
		//wtk_debug("111\n");
		//wtk_debug(" %d \n",wakeup->inst_q.length);
		qn=wtk_queue_pop(&(q));
		if(!qn){break;}
		inst=data_offset2(qn,wtk_wakeup_inst_t,q_n);
//		if(inst->pth)
//		{
//			wtk_debug("============ frame=%d inst=%d %f/%f/%f/%f =============\n",wakeup->frame,q.length,vec->p[0],vec->p[1],vec->p[2],vec->p[3]);
//			wtk_wakeup_path_print(inst->pth);
//		}
		//wtk_debug("inst depth:%d %d \n",inst->node->depth,wakeup->inst_q.length);
		ret=wtk_wakeup_update_inst(wakeup,inst,vec);
		if(ret==2)
		{
			wtk_queue_link(&(wakeup->inst_q),&(q));
			return 1;
		}else if(ret==1 && inst->node->depth==0)
		{
			b=0;
		}
	}
	if(b)
	{
		//wtk_debug("222\n");
		inst=wtk_wakeup_pop_inst(wakeup);
		inst->node=wakeup->post_cfg->network;
		//wtk_debug("%d\n",wakeup->inst_q.length);
		wtk_wakeup_update_inst(wakeup,inst,vec);
	}
	//wtk_wakeup_check(wakeup);
	return 0;
}
void wtk_wakeup_print_cfg(wtk_wakeup_t *wakeup){
	int i;
	wtk_wakeup_node_t *node;

	wtk_debug("min_rasie: %d\n",wakeup->post_cfg->min_raise);
	for(i = 0;i <4;i++)
	{
		node = wakeup->post_cfg->network + i;
		wtk_debug("%d  max_frame: %d\n",i,node->max_frame);
		wtk_debug("%d  cur_thresh: %f\n",i,node->cur_thresh);
		wtk_debug("%d  pth_thresh: %f\n",i,node->pth_thresh);
	}

}


int wtk_wakeup_quick_check_robin(wtk_wakeup_t *wakeup,wtk_robin_t *rb,int is_end)
{
	float *pf;
	int i;
	wtk_wakeup_node_t *node1;
	wtk_wakeup_node_t *node2;
	float f1,f2;
	int idx1,idx2;
	float sumf,sum2f,fx,t;
	wtk_wakeup_post_cfg_t *post_cfg=wakeup->post_cfg;
	int min_cnt;

	node1=post_cfg->network;
	node2=post_cfg->network+post_cfg->n_network-1;
	idx1=node1->idx;
	idx2=node2->idx;
	pf=wtk_robin_at(rb,0);
	f1=pf[idx1];
	f2=pf[idx2];
	//ki1=ki2=0;
	if(post_cfg->avg_thresh<=0)
	{
		for(i=0;i<rb->used;++i)
		{
			pf=wtk_robin_at(rb,i);
			//sum3f+=pf[2];
			if(f1<pf[idx1])
			{
				f1=pf[idx1];
			}
			if(f2<pf[idx2])
			{
				f2=pf[idx2];
			}
		}
	}else
	{
		sumf=0;
		sum2f=0;
		t=post_cfg->min_hint_thresh;
		min_cnt=0;
		if(t>0)
		{
			for(i=0;i<rb->used;++i)
			{
				pf=wtk_robin_at(rb,i);
				fx=pf[idx1]+pf[idx2];
				if(fx<t)
				{
					++min_cnt;
					if(min_cnt>post_cfg->min_hint_cnt)
					{
						return 0;
					}
				}else
				{
					min_cnt=0;
				}
				sumf+=fx;
				sum2f+=pf[0];//+pf[2];
				if(f1<pf[idx1])
				{
					f1=pf[idx1];
				}
				if(f2<pf[idx2])
				{
					f2=pf[idx2];
				}
			}
		}else
		{
			for(i=0;i<rb->used;++i)
			{
				pf=wtk_robin_at(rb,i);
				sumf+=pf[idx1]+pf[idx2];
				sum2f+=pf[0];//+pf[2];
				if(f1<pf[idx1])
				{
					f1=pf[idx1];
				}
				if(f2<pf[idx2])
				{
					f2=pf[idx2];
				}
			}
		}
		sumf/=rb->used;
		sum2f/=rb->used;
		//wtk_debug("%d,%d used=%d f1=%f f2=%f thresh=%f/%f\n",idx1,idx2,rb->used,f1,f2,node1->cur_thresh,node2->cur_thresh);
		//wtk_debug("%f/%f\n",sumf,sum2f);
		if((sumf<sum2f*post_cfg->speech_sum_scale) ||( (sumf)<post_cfg->avg_thresh))
		{
			return 0;
		}
	}
	if(f1>=node1->cur_thresh && f2>=node2->cur_thresh )
	{
		return 1;
	}else
	{
		return 0;
	}
}

#define USE_TRI 1

int wtk_wakeup_check_robin(wtk_wakeup_t *wakeup,wtk_robin_t *rb,wtk_wakeup_node_t *node,int s,int e,int depth,float pre,int ss,int is_end)
{
	float *pf,*pf2;
	int i;
	int ret;
	wtk_wakeup_node_t *nxt_node;
	wtk_wakeup_node_t *lst_node;
	float f,ppf;
	int s1,e1;
	int j;
	int xidx=0;
	int b;
	int xs,xe;
	int use_tri=USE_TRI;
	int win=wakeup->post_cfg->tri_win;
	//int bg_idx=wakeup->cfg->bg_idx;

//	if(wakeup->frame==218)
//	{
//		wtk_debug("check depth=%d s=%d e=%d\n",depth,s,e);
//	}
	if(depth==1)
	{
		nxt_node=NULL;
	}else
	{
		nxt_node=node+1;
	}
	if(node->depth==0)
	{
		lst_node=NULL;
	}else
	{
		lst_node=node-1;
	}
	xidx=nxt_node?nxt_node->idx:lst_node->idx;
	ppf=node->pth_pow_thresh;
	for(i=s;i<e;++i)
	{
		pf=wtk_robin_at(rb,i);
		//wtk_debug("v[%d,%d-%d,%d]\n",i+wakeup->poped,s+wakeup->poped,e+wakeup->poped,rb->used);
		if((pf[node->idx]-pf[xidx])<node->max_df)
		{
			//wtk_debug("v[%d/%d]=%f\n",i,node->idx,pf[node->idx]);
			continue;
		}
		if(depth<4 && (pf[2]>pf[node->idx]))//||(pf[1]+pf[3])<pf[0]))
		{
			continue;
		}
		//if(pf[2]>pf[node->idx] || pf[0]>pf[node->idx])
		if(use_tri&&(!is_end||(is_end && depth!=1)))
		{
			xs=i-win;
			if(xs<0)
			{
				xs=0;
			}
			xe=i+win;
			if(xe>(rb->used-1))
			{
				if(!is_end && depth!=1)
				{
					continue;
				}
				xe=rb->used-1;
			}
			//wtk_debug("==========  i=%d [%d,%d-%d] ==========\n",i+wakeup->poped,i,xs,xe);
			b=1;
			for(j=xs;j<=xe;++j)
			{
				if(j==i){continue;}
				pf2=wtk_robin_at(rb,j);
				//wtk_debug("v[%d/%d]=%f/%f\n",i+wakeup->poped,j+wakeup->poped,pf[node->idx],pf2[node->idx]);
				if(pf[node->idx]<pf2[node->idx])// || pf2[node->idx]>0.1)
				{
					b=0;
					break;
				}
			}
			if(b==0)
			{
				//wtk_debug("continue v=%d %f\n",i,pf[node->idx]);
				continue;
			}
		}
		//wtk_debug("continue v=%d %f/%f\n",i,pf[node->idx],node->cur_thresh);
		if(pf[node->idx]>=node->cur_thresh)
		{
			wakeup->cache[node->depth]=i;
			if(node->depth==0)
			{
				ss=i;
			}
			f=pre*pf[node->idx];
			//wtk_debug("continue v=%d %f/%f f=%f/%f prob=%f depth=%d\n",i,pf[node->idx],node->cur_thresh,f,ppf,pow(f,1.0/4),depth);
			if(f>=ppf)
			{
				if(depth==1)
				{
					if(i-ss>=node->min_frame2)
					{
						wakeup->prob=f;
						return 1;
					}else
					{
						continue;
					}
				}
				j=min(rb->used,i+nxt_node->max_frame);
				//wtk_debug("j=%d/%d\n",j,nxt_node->max_frame2);
				//j=min(j,nxt_node->max_frame2);
				e1=max(j,ss+nxt_node->min_frame2);
				if(e1<=rb->used)
				{
					//s1=max(i+nxt_node->min_frame,i-ss);
					s1=i+nxt_node->min_frame;
					//wtk_debug("i=%d s1=%d ss=%d j=%d\n",i,s1,ss,j);
					//wtk_debug("depth=%d/%d [%d,%d]\n",node->depth,depth,s1,e1);
					if(lst_node)
					{
						//wtk_debug("cache[%d,%d,%d,%d]\n",wakeup->cache[0],wakeup->cache[1],wakeup->cache[2],wakeup->cache[3]);
						j=wakeup->cache[lst_node->depth]+wakeup->post_cfg->min_tri_frame;
						s1=max(s1,j);
					}
//					if(wakeup->frame==218)
//					{
//						wtk_debug("check robin=%d depth=%d idx=%d %f max=%d\n",i,depth,node->idx,pf[node->idx],nxt_node->max_frame);
//					}
					ret=wtk_wakeup_check_robin(wakeup,rb,nxt_node,s1,e1,depth-1,f,ss,is_end);
					if(ret==1)
					{
						return 1;
					}
				}
			}
		}
	}
	return 0;
}


void wtk_wakeup_print_win(wtk_wakeup_t *wakeup)
{
	wtk_robin_t *rb=wakeup->win_robin;
	int i;
	float *pf;
	double t=1;

	wtk_debug("=========== win used=%d nslot=%d  ================\n",rb->used,rb->nslot);
	for(i=0;i<wakeup->post_cfg->n_network;++i)
	{
		pf=wtk_robin_at(rb,wakeup->cache[i]);
		wtk_debug("v[%d]=%d/%d/%d %f/%f/%f/%f\n",i,wakeup->cache[i]+wakeup->poped,wakeup->poped,wakeup->cache[i],pf[0],pf[1],pf[2],pf[3]);
		t=t*pf[wakeup->post_cfg->network[i].idx];
	}
	t=pow(t,1.0/wakeup->post_cfg->n_network);
	wtk_debug("======== prob=%f ===============\n",t);
}

int wtk_wakeup_post_check(wtk_wakeup_t *wakeup)
{
	wtk_robin_t *rb=wakeup->win_robin;
	float *pf;
	float sil,speech;

	pf=wtk_robin_at(rb,wakeup->tmp_cache[0]);
	sil=pf[2];
	speech=pf[0];
	pf=wtk_robin_at(rb,wakeup->tmp_cache[1]);
	sil*=pf[2];
	speech*=pf[0];
	pf=wtk_robin_at(rb,wakeup->tmp_cache[2]);
	sil*=pf[2];
	speech*=pf[0];
	pf=wtk_robin_at(rb,wakeup->tmp_cache[3]);
	sil*=pf[2];
	speech*=pf[0];
	wtk_debug("sil=%f speech=%f prob=%f\n",pow(sil,1.0/4),pow(speech,1.0/4),pow(wakeup->prob,1.0/4));
	sil=pow(wakeup->prob,1.0/4)-pow(speech,1.0/4);
	//return sil>0.1;
	return 0;
}


float wtk_wakeup_check_bg_sp_pass(wtk_wakeup_t *wakeup,int s,int pos1,int pos2,int e)
{
	wtk_robin_t *rb=wakeup->win_robin;
	float *pf;
	float prob,t,t1;
	int i,j,step;

	//wtk_debug("s=%d pos=%d/%d e=%d\n",s,pos1,pos2,e);
	pf=wtk_robin_at(rb,pos1);
	prob=pf[3];
	pf=wtk_robin_at(rb,pos2);
	prob*=pf[1];
	prob=sqrt(prob);
	//wtk_debug("prob=%f\n",prob);
	step=(int)((e-s+1)*1.0/2+0.5);
	//wtk_debug("step=%d\n", step);
	j=0;
	t=1;
	t1=0;
	for(i=s;i<=e;++i)
	{
		++j;
		pf=wtk_robin_at(rb,i);
		if(1)
		{
			t1+=pf[0];
			if(j==step || i==e)
			{
				//wtk_debug("v[%d]=%f/%f j=%d/%d\n",i,t1,t,j,step);
				t1/=j;
				t*=t1;
				t1=0;
				j=0;
			}
		}else
		{
			if(j==1)
			{
				t1=pf[0];
			}else
			{
				if(t1<pf[0])
				{
					t1=pf[0];
				}
				if(j==step || i==e)
				{
					t*=t1;
					//wtk_debug("v[%d]=%f/%f\n",i,t1,t);
					j=0;
				}
			}
		}
	}
	t=sqrt(t);
	t1=prob-t;
	return t1;
}

/**
 * return 1 for right;
 */
int wtk_wakeup_check_bg_sp(wtk_wakeup_t *wakeup)
{
	int n,b;
	float t1,t;
	int cnt=0;
	int nnet=wakeup->post_cfg->n_network;

	//print_int(wakeup->cache,4);
	//wtk_debug("%d/%d/%d/%d\n",wakeup->cache[0],wakeup->cache[1],wakeup->cache[2],wakeup->cache[3]);
	if(nnet==4)
	{
		n=(wakeup->cache[1]+wakeup->cache[2])/2;
		t1=wtk_wakeup_check_bg_sp_pass(wakeup,wakeup->cache[0],wakeup->cache[0],wakeup->cache[1],n);
		//wtk_debug("t1=%f\n",t1);
		if(t1>wakeup->post_cfg->bg_thresh)
		{
			++cnt;
		}else
		{
			return 0;
		}
		t=t1;
		//wtk_debug("t=%f/%f\n",t1,t);
		//if(b==0){goto end;}
		t1=wtk_wakeup_check_bg_sp_pass(wakeup,n,wakeup->cache[2],wakeup->cache[3],wakeup->cache[3]);
		//wtk_debug("t1=%f\n",t1);
		t+=t1;
		//wtk_debug("t=%f/%f\n",t1,t);
		if(t1>wakeup->post_cfg->bg_thresh)
		{
			++cnt;
		}
		b=(cnt==2 || t>0.3)?1:0;
		return b;
	}else
	{
		t1=wtk_wakeup_check_bg_sp_pass(wakeup,wakeup->cache[0],wakeup->cache[0],wakeup->cache[1],wakeup->cache[1]);
		//wtk_debug("t1=%f\n",t1);
		if(t1>wakeup->post_cfg->bg_thresh)
		{
			return 1;
		}else
		{
			return 0;
		}
	}
}

/**
 * return 1 for right;
 */
int wtk_wakeup_check_bg_sp2(wtk_wakeup_t *wakeup)
{
	wtk_wakeup_post_cfg_t *post_cfg=wakeup->post_cfg;
	wtk_robin_t *rb=wakeup->win_robin;
	float t,t1;
	int i,j,step;
	float *pf;
	float prob;

	prob=pow(wakeup->prob,0.25);
	step=(int)((wakeup->cache[3]-wakeup->cache[0]+1)*0.25+0.5);
	//wtk_debug("step=%d\n",step);
	j=0;
	t=1;
	t1=0;
	for(i=wakeup->cache[0];i<=wakeup->cache[3];++i)
	{
		++j;
		pf=wtk_robin_at(rb,i);
		if(j==1)
		{
			t1=pf[0];
		}else
		{
			if(t1<pf[0])
			{
				t1=pf[0];
			}
			if(j==step || i==wakeup->cache[3])
			{
				t*=t1;
				wtk_debug("v[%d]=%f/%f\n",i,t1,t);
				j=0;
			}
		}
	}
	t=pow(t,0.25);
	t1=prob-t;
	wtk_debug("wake bg=%f/%f\n",prob,t);
	if(t1<post_cfg->bg_thresh)
	{
		return 0;
	}
	j=0;
	t=1;
	t1=0;
	for(i=wakeup->cache[0];i<=wakeup->cache[3];++i)
	{
		++j;
		pf=wtk_robin_at(rb,i);
		if(j==1)
		{
			t1=pf[2];
		}else
		{
			if(t1<pf[2])
			{
				t1=pf[2];
			}
			if(j==step || i==wakeup->cache[3])
			{
				t*=t1;
				wtk_debug("v[%d]=%f/%f\n",i,t1,t);
				j=0;
			}
		}
	}
	t=pow(t,0.25);
	t1=prob-t;
	wtk_debug("wake sil=%f/%f\n",prob,t);
	if(t1<post_cfg->sil_thresh)
	{
		return 0;
	}else
	{
		return 1;
	}
}



/**
 * return 1 for right;
 */
int wtk_wakeup_process_check2(wtk_wakeup_t *wakeup)
{
	wtk_wakeup_post_cfg_t *post_cfg=wakeup->post_cfg;
	wtk_robin_t *rb=wakeup->win_robin;
	float *pf;
	wtk_wakeup_node_t *node;
	float t;
	int b=1;
	float tf;
	float t1,t2;

	if(post_cfg->use_bg_check)
	{
		b=wtk_wakeup_check_bg_sp(wakeup);
		if(b==0)
		{
			goto end;
		}
	}
	if(post_cfg->use_half_check)
	{
		tf=wakeup->post_cfg->pass_thresh;
		if(tf<=0){goto end;}
		node=wakeup->post_cfg->network;
		pf=wtk_robin_at(rb,wakeup->cache[0]);
		t=pf[node[0].idx];
		pf=wtk_robin_at(rb,wakeup->cache[1]);
		t1=sqrt(t*pf[node[1].idx]);
		//wtk_debug("t=%f/%f\n",t,node[3].pth_thresh);
		if(t1<tf)
		{
			b=0;
			goto end;
		}
		pf=wtk_robin_at(rb,wakeup->cache[2]);
		t=pf[node[2].idx];
		pf=wtk_robin_at(rb,wakeup->cache[3]);
		t2=sqrt(t*pf[node[3].idx]);
		//wtk_debug("t=%f/%f\n",t,node[3].pth_thresh);
		if(t2<tf)
		{
			b=0;
			goto end;
		}
	}
	b=1;
end:
	return b;
}

int wtk_wakeup_process_post(wtk_wakeup_t *wakeup)
{
	wtk_robin_t *rb=wakeup->win_robin;
	float *pf,*pf2;
	int i;
	int idx,nxt_idx;
	wtk_wakeup_node_t *node;
	float t;
	int b=1;
	int j;

//	for(i=0;i<rb->used;++i)
//	{
//		pf=wtk_robin_at(rb,i);
//		wtk_debug("v[%d/%d]=%f/%f/%f/%f\n",i+wakeup->poped,i,pf[0],pf[1],pf[2],pf[3]);
//	}
	node=wakeup->post_cfg->network;
	//wtk_wakeup_print_win(wakeup);
	//check front
	idx=wakeup->cache[0];
	//wtk_debug("idx=%d/%d %d\n",idx,nxt_idx,node->idx);
	if(USE_TRI==0)
	{
		nxt_idx=wakeup->cache[1]-wakeup->post_cfg->network[1].min_frame;
		pf=wtk_robin_at(rb,idx);
		for(i=idx+1;i<nxt_idx;++i)
		{
			pf2=wtk_robin_at(rb,i);
			if(pf2[node->idx]>pf[node->idx])
			{
				pf=pf2;
				//wtk_debug("i=%d %f/%f/%f/%f\n",i,pf2[0],pf2[1],pf2[2],pf2[3]);
				wakeup->cache[0]=i;
			}
		}
	}else
	{
		pf2=wtk_robin_at(rb,wakeup->cache[0]);
		t=pf2[(wakeup->post_cfg->network)->idx]*wakeup->post_cfg->scale_s;
		//wtk_debug("t=%f\n",t);
		t=min(t,wakeup->post_cfg->min_s);
		for(i=idx;i>=0;--i)
		{
			pf=wtk_robin_at(rb,i);
			if(pf[node->idx]<t)
			{
				wakeup->cache[0]=i;
				break;
			}else
			{
				wakeup->cache[0]=i;
			}
		}
	}

	if(USE_TRI)
	{
		int nnet;

		if(wakeup->post_cfg->max_end_win>0)
		{
			nnet=wakeup->post_cfg->n_network;
			idx=wakeup->cache[nnet-1];
			node=wakeup->post_cfg->network+nnet-1;
			pf2=wtk_robin_at(rb,idx);
			t=pf2[node->idx]*wakeup->post_cfg->scale_e;
			t=min(t,wakeup->post_cfg->min_e);
			for(i=idx+1,j=0;i<rb->used;++i,++j)
			{
				pf2=wtk_robin_at(rb,i);
				//wtk_debug("v[%d]=%f/%f\n",i,pf2[node->idx],t);
				if(pf2[node->idx]<t)
				{
					wakeup->cache[nnet-1]=i;
					b=0;
					break;
				}else
				{
					wakeup->cache[nnet-1]=i;
				}
				if(j>=wakeup->post_cfg->max_end_win)
				{
					b=0;
					break;
				}
			}
		}else
		{
			b=0;
		}
	}else
	{
		b=0;
	}
//	if(wakeup->post_cfg->use_bg_check)
//	{
//		b=wtk_wakeup_check_bg_sp(wakeup);
//		b=b==1?0:1;
//	}
	//wtk_wakeup_print_win(wakeup);
	//exit(0);
	return b;
}



void wtk_wakeup_print_raw_win(wtk_wakeup_t *wakeup)
{
	wtk_robin_t *rb=wakeup->win_robin;
	float *pf;
	int i,j;
	int nnet=wakeup->post_cfg->n_network;

	printf("===>prob: %f frame=%d\n",pow(wakeup->prob,1.0/nnet),wakeup->frame);
	printf("===> wakeup start ");
	for(i=0;i<nnet;++i)
	{
		//j=wakeup->cache[i];
		j=wakeup->tmp_cache[i];
		pf=wtk_robin_at(rb,j);
		printf(" [%d]=%f",j,pf[wakeup->post_cfg->network[i].idx]);
	}
	printf("\n");
	for(i=wakeup->cache[0];i<=wakeup->cache[nnet-1];++i)
	//for(i=wakeup->tmp_cache[0];i<=wakeup->tmp_cache[nnet-1];++i)
	{
		pf=wtk_robin_at(rb,i);
		wtk_debug("v[%d/%d]=%f/%f/%f/%f\n",i+wakeup->poped,i,pf[0],pf[1],pf[2],pf[3]);
	}
	printf("===> wakeup end\n");
}

//#define DEBUG_X 1

void wtk_wakeup_print_robin(wtk_wakeup_t *wakeup)
{
	wtk_robin_t *rb=wakeup->win_robin;
	float *pf;
	int i;

	for(i=0;i<rb->used;++i)
	{
		pf=wtk_robin_at(rb,i);
		wtk_debug("v[%d/%d]=%f/%f/%f/%f\n",i+wakeup->poped,i,pf[0],pf[1],pf[2],pf[3]);
	}
}

/**
 *  |bkg qi sil you|
 * return 1 for end
 */
int wtk_wakeup_feed_vec2(wtk_wakeup_t *wakeup,wtk_vecf_t *v,int is_end,int idx)
{
	wtk_robin_t *rb=wakeup->win_robin;
	float *pf;
	int i;
	int depth=wakeup->post_cfg->n_network;
	wtk_wakeup_node_t *node;
	int b;
	int ret;

	//if(wakeup->post_cfg->de)
	//wtk_debug("v[%d/%d]=%f/%f/%f/%f used=%d\n",idx,wakeup->frame,v->p[0],v->p[1],v->p[2],v->p[3],rb->used);
	if(wakeup->post_cfg->debug || wakeup->cfg->print_v)
	{
		wtk_debug("v[%d]=%f/%f/%f/%f used=%d\n",idx,v->p[0],v->p[1],v->p[2],v->p[3],rb->used);
	}
	if(wakeup->notify)
	{
		wakeup->notify(wakeup->notify_ths,idx,v->p);
	}
#ifdef DEBUG_X
	wtk_debug("v[%d]=%f/%f/%f/%f used=%d\n",wakeup->frame,v->p[0],v->p[1],v->p[2],v->p[3],rb->used);
#endif
//	if(idx==213)
//	{
//		wtk_wakeup_print_robin(wakeup);
//	}
	//return 0;
	if(wakeup->get)
	{
		return 1;
	}
	if(wakeup->skip_post)
	{
		return 0;
	}
	if(rb->used==0)
	{
		node=wakeup->post_cfg->network;
		if(v->p[node->idx]<node->cur_thresh)
		{
			++wakeup->poped;
			return 0;
		}
	}
	//wtk_debug("v[%d]=%f/%f/%f/%f\n",wakeup->frame,v->p[0],v->p[1],v->p[2],v->p[3]);
	//wtk_debug("used=%d/%d\n",rb->used,rb->nslot);
	pf=wtk_robin_next(rb);
	memcpy(pf,v->p,v->len*sizeof(float));
	//wtk_debug("used=%d/%d\n",rb->used,wakeup->cfg->min_win);
	if(rb->used>=wakeup->post_cfg->min_win)
	{
		//wtk_debug("v[%d]=%f/%f/%f/%f\n",wakeup->frame,v->p[0],v->p[1],v->p[2],v->p[3]);
		i=wtk_wakeup_quick_check_robin(wakeup,rb,is_end);
		//wtk_debug("frame=%d i=%d\n",wakeup->frame,i);
		if(i==1)
		{
			node=wakeup->post_cfg->network;
			i=wtk_wakeup_check_robin(wakeup,rb,node,0,rb->used,depth,1,0,is_end);
			if(i==1)
			{
				i=wtk_wakeup_process_check2(wakeup);
			}
			if(i==1)
			{
#ifdef DEBUG_X
				wtk_wakeup_print_win(wakeup);
#endif
				//wtk_wakeup_print_win(wakeup);
				//wtk_wakeup_print_robin(wakeup);
				for(i=0;i<wakeup->post_cfg->n_network;++i)
				{
					wakeup->tmp_cache[i]=wakeup->cache[i];
				}
				b=wtk_wakeup_process_post(wakeup);
				//wtk_debug("=============== get waked=%d prob=%f ==================\n",b,pow(wakeup->prob,1.0/4));
				if(b==1 && !is_end)
				{
					ret=0;
					goto end;
				}
				//wtk_debug("============== waked ==================\n");
				//wtk_debug("[%d,%d]\n",wakeup->tmp_cache[0]+wakeup->poped,wakeup->tmp_cache[3]+wakeup->poped);
				if(wakeup->check_valid)
				{
					b=wakeup->check_valid(wakeup->check_valid_ths,
							(wakeup->tmp_cache[0]+wakeup->poped)*wakeup->parm->cfg->frame_dur,
							(wakeup->tmp_cache[3]+wakeup->poped)*wakeup->parm->cfg->frame_dur
							);
					if(b==0)
					{
						ret=0;
						goto end;
					}
				}
#ifdef DEBUG_X
			wtk_debug("is_end=%d b=%d frame=%d\n",is_end,b,wakeup->frame);
			wtk_wakeup_print_win(wakeup);
#endif
				//wtk_wakeup_print_win(wakeup);
				//exit(0);
				//wtk_wakeup_print_win(wakeup);
				//exit(0);
				wakeup->get=1;
				return 1;
			}
		}else
		{
			//wtk_debug("break check frame=%d\n",wakeup->frame);
		}
	}
	ret=0;
end:
	if(rb->used>=rb->nslot)
	{
		node=wakeup->post_cfg->network;
		//wtk_debug("========clean...\n");
		wtk_robin_pop(rb);
		++wakeup->poped;
		for(i=0;i<rb->used;++i)
		{
			pf=wtk_robin_at(rb,i);
			//wtk_debug("v[%d]=%f/%f/%f/%f %f\n",i,pf[0],pf[1],pf[2],pf[3],node->cur_thresh);
			if(pf[node->idx]>=node->cur_thresh)
			{
				break;
			}
		}
		//wtk_debug("========clean i=%d...\n",i);
		//exit(0);
		if(i>0)
		{
			wakeup->poped+=i;
			wtk_robin_pop2(rb,i);
		}
	}
	return ret;
}

int wtk_wakeup_feed_feature_vec(wtk_wakeup_t *wakeup,wtk_vecf_t *vec2,int is_end,int idx)
{
//	static int ki=0;
//
//	++ki;
//	wtk_debug("v[%d/%d]=%f/%f/%f/%f\n",idx,ki,vec2->p[0],vec2->p[1],vec2->p[2],vec2->p[3]);
//	if(ki!=idx)
//	{
//		exit(0);
//	}
	//exit(0);
	if(wakeup->cfg->use_win)
	{
		return wtk_wakeup_feed_vec2(wakeup,vec2,is_end,idx);
	}else
	{
		return wtk_wakeup_feed_vec(wakeup,vec2);
	}
}

int wtk_wakeup_feed_feature(wtk_wakeup_t *wakeup,wtk_feat_t *f,int is_end)
{
	wtk_wakeup_feat_t *f2;
	wtk_robin_t *rbin=wakeup->feat_robin;
	wtk_vecf_t *vec;
	wtk_vecf_t *vec2=wakeup->input_vec;
	int i;
	int b;
	int ret=0;
	int n,j,k;
	wtk_wakeup_post_cfg_t *post=wakeup->post_cfg;

	//wtk_debug("=========== indx=%d is_end=%d ============\n",f->index,is_end);
	//wtk_debug("v[%d/%d]=%f/%f/%f/%f used=%d/%d\n",f->index,wakeup->frame,f->rv[1],f->rv[2],f->rv[3],f->rv[4],rbin->used,rbin->nslot);
	//print_cfg(wakeup);
	wakeup->frame=f->index;
	//exit(0);
	if(rbin)
	{
		if(post->smooth_right==0)
		{
			vec=wakeup->vec_smooth;
			//wtk_vector_print(f->rv);
			b=rbin->used==rbin->nslot;
			//wtk_debug("%d/%d/%d\n",b,rbin->used,rbin->nslot);
			f2=(wtk_wakeup_feat_t*)wtk_robin_next(rbin);
			if(b)
			{
				for(i=0;i<vec->len;++i)
				{
					vec->p[i]-=f2->v[i];
				}
			}
			for(i=0;i<vec->len;++i)
			{
				vec->p[i]+=f2->v[i]=expf(f->rv[i+1]);
				//wtk_debug("v[%d]=%f/%f/%f/%f\n",f->index,vec->p[0],vec->p[1],vec->p[2],vec->p[3]);
			}
			//wtk_debug("used=%d\n",rbin->used);
			for(i=0;i<vec2->len;++i)
			{
				//wtk_debug("v[%d]=%f/%f/%f/%f\n",f->index,vec2->p[0],vec2->p[1],vec2->p[2],vec2->p[3]);
				vec2->p[i]=vec->p[i]/rbin->used;
				//wtk_debug("v[%d]=%f/%f/%f/%f\n",f->index,vec2->p[0],vec2->p[1],vec2->p[2],vec2->p[3]);
				//wtk_debug("v[%d/%d]=%f/%f\n",ki,i,vec->p[i]/rbin->used,vec2->p[i]);
			}
			return wtk_wakeup_feed_feature_vec(wakeup,vec2,is_end,f->index);
		}else
		{
			vec=wakeup->vec_smooth;
			//wtk_vector_print(f->rv);
			b=rbin->used==rbin->nslot;
			//wtk_debug("%d/%d/%d\n",b,rbin->used,rbin->nslot);
			f2=(wtk_wakeup_feat_t*)wtk_robin_next(rbin);
			if(b)
			{
				for(i=0;i<vec->len;++i)
				{
					vec->p[i]-=f2->v[i];
				}
			}
			f2->index=f->index;
			for(i=0;i<vec->len;++i)
			{
				vec->p[i]+=f2->v[i]=expf(f->rv[i+1]);
			}
			//wtk_debug("v[%d]=%f/%f/%f/%f\n",f->index,f2->v[0],f2->v[1],f2->v[2],f2->v[3]);
			if((rbin->used<rbin->nslot)&&!is_end)
			{
				return 0;
			}
			//wtk_debug("=========== indx=%d %d/%d ============\n",f->index,rbin->used,rbin->nslot);
			if(!b)
			{
				n=min(post->smooth_left,rbin->used);
				for(i=0;i<n;++i)
				{
					//wtk_debug("i=%d\n",i);
					memset(vec2->p,0,vec2->len*sizeof(float));
					for(j=i-post->smooth_left;j<=i+post->smooth_right;++j)
					{
						if(j<0)
						{
							k=0;
						}else if(j>=rbin->used)
						{
							k=rbin->used-1;
						}else
						{
							k=j;
						}
						f2=(wtk_wakeup_feat_t*)wtk_robin_at(rbin,k);
						//wtk_debug("j=%d index=%d %f/%f/%f/%f\n",j,f2->index,f2->v[0],f2->v[1],f2->v[2],f2->v[3]);
						for(k=0;k<vec2->len;++k)
						{
							vec2->p[k]+=f2->v[k];
						}
					}
					for(k=0;k<vec2->len;++k)
					{
						vec2->p[k]=vec2->p[k]/rbin->nslot;
					}
					//print_float(vec2->p,vec2->len);
					wtk_wakeup_feed_feature_vec(wakeup,vec2,0,i+1);
					if(wakeup->get)
					{
						break;
					}
				}
				//exit(0);
			}
			//wtk_debug("used=%d\n",rbin->used);
			if(rbin->used==rbin->nslot && !wakeup->get)
			{
				for(i=0;i<vec2->len;++i)
				{
					//wtk_debug("v[%d]=%f/%f/%f/%f\n",f->index,vec2->p[0],vec2->p[1],vec2->p[2],vec2->p[3]);
					vec2->p[i]=vec->p[i]/rbin->used;
					//wtk_debug("v[%d]=%f/%f/%f/%f\n",f->index,vec2->p[0],vec2->p[1],vec2->p[2],vec2->p[3]);
					//wtk_debug("v[%d/%d]=%f/%f\n",ki,i,vec->p[i]/rbin->used,vec2->p[i]);
				}
				f2=(wtk_wakeup_feat_t*)wtk_robin_at(rbin,rbin->nslot/2);
				ret=wtk_wakeup_feed_feature_vec(wakeup,vec2,is_end,f2->index);
				if(ret==1 && wakeup->cfg->print_v==0)
				{
					return 1;
				}
			}
			if(is_end &&!wakeup->get)
			{
				int s;

				n=min(post->smooth_left+1,rbin->used);
				//wtk_debug("n=%d\n",rbin->used);
				f2=(wtk_wakeup_feat_t*)wtk_robin_at(rbin,n);
				s=f2->index;
				for(i=n;i<rbin->used;++i)
				{
					//wtk_debug("i=%d\n",i);
					memset(vec2->p,0,vec2->len*sizeof(float));
					for(j=i-post->smooth_left;j<=i+post->smooth_right;++j)
					{
						if(j<0)
						{
							k=0;
						}else if(j>=rbin->used)
						{
							k=rbin->used-1;
						}else
						{
							k=j;
						}
						f2=(wtk_wakeup_feat_t*)wtk_robin_at(rbin,k);
						//wtk_debug("j=%d index=%d %f/%f/%f/%f\n",j,f2->index,f2->v[0],f2->v[1],f2->v[2],f2->v[3]);
						for(k=0;k<vec2->len;++k)
						{
							vec2->p[k]+=f2->v[k];
						}
					}
					for(k=0;k<vec2->len;++k)
					{
						vec2->p[k]=vec2->p[k]/rbin->nslot;
					}
					//print_float(vec2->p,vec2->len);
					//exit(0);
					ret=wtk_wakeup_feed_feature_vec(wakeup,vec2,0,s+i-n);
					if(ret==1 && wakeup->cfg->print_v==0)
					{
						return 1;
					}
				}
				//exit(0);
			}
		}
	}else
	{
		for(i=0;i<vec2->len;++i)
		{
			vec2->p[i]=expf(f->rv[i+1]);
			//wtk_debug("v[%d]=%f\n",i,vec2->p[i]);
		}
		return wtk_wakeup_feed_feature_vec(wakeup,vec2,is_end,f->index);
		//wtk_debug("v[%d]=%f/%f/%f/%f\n",f->index,vec2->p[0],vec2->p[1],vec2->p[2],vec2->p[3]);
	}
	return ret;
}

#include "wtk/core/wtk_wavfile.h"

int wtk_wakeup_feed(wtk_wakeup_t *wakeup,char *data,int bytes,int is_end)
{
	wtk_queue_t *q=&(wakeup->param_q);
	wtk_queue_node_t *qn;
	wtk_feat_t *feat;//,*feat2;
	int ret;
	char buf[256];
//	static wtk_wavfile_t *log=NULL;

	if(wakeup->wav_buf)
	{
		if(bytes>0)
		{
			wtk_strbuf_push(wakeup->wav_buf,data,bytes);
		}
		if(is_end)
		{
			wakeup->wav_cnt++;
			sprintf(buf,"wakeup_log/%d.wav",wakeup->wav_cnt);
			wave_write_file2(buf,1,16000,wakeup->wav_buf->data,wakeup->wav_buf->pos);
		}
	}
//	wtk_debug("len=%d end=%d\n",bytes,is_end)
//	if(!log)
//	{
//		log=wtk_wavfile_new(16000);
//		wtk_wavfile_open2(log,"wk");
//		log->max_pend=-1;
//	}
//	wtk_wavfile_write(log,data,bytes);
//	if(is_end)
//	{
//		wtk_debug("============ close ============\n");
//		wtk_wavfile_close(log);
//		wtk_wavfile_delete(log);
//		log=NULL;
//	}
	//wtk_debug("bytes=%d is_end=%d len=%d\n",bytes,is_end,wakeup->inst_q.length);
	//print_data(data,min(bytes,32));
	wakeup->count+=bytes;
	wtk_fextra_feed2(wakeup->parm,data,bytes,is_end);
	//wtk_debug("len=%d\n",wakeup->param_q.length);
	ret=0;
	while(1)
	{
		qn=wtk_queue_pop(q);
		if(!qn){break;}
		feat=data_offset2(qn,wtk_feat_t,queue_n);
		//wtk_debug("index=%d\n",feat->index);
		//wtk_debug("index=%d queue=%d bytes=%f\n",feat->index,wakeup->param_q.length,wakeup->count*1.0/32000);
		if(wakeup->cfg->parm.dnn.skip_frame == 0||(wakeup->cfg->parm.dnn.skip_frame != 0 && feat->index %2 == 1))
		{
			ret=wtk_wakeup_feed_feature(wakeup,feat,is_end?(qn->next?0:1):0);
			//wtk_debug("%f\n",feat->v[1]);
			//wtk_debug("v[%d]\n",feat->index);
			wtk_fextra_reuse_feature(wakeup->parm,feat);
			if(ret==1 && wakeup->cfg->print_v==0)
			{
				//exit(0);
				//wtk_fextra_get_cache(wakeup->parm);
				//wtk_vector_print(wakeup->parm->zpost.zmean->cur);
				wakeup->idx=feat->index;
				//wtk_debug("index=%d len=%d\n",feat->index,wakeup->param_q.length);
				//is_end;
				break;
			}
		}
	}
	//wtk_debug("pend=%d free=%d used=%d\n",wakeup->param_q.length,wakeup->parm->feature_hoard.cur_free,wakeup->parm->feature_hoard.use_length);
	return ret;
}

float wtk_wakeup_get_cur_valid_start_time(wtk_wakeup_t *wakeup)
{
	wtk_queue_node_t *qn;
	wtk_wakeup_inst_t *inst;
	wtk_queue_t q;
	int ret = 0;

	q=wakeup->inst_q;
	for(qn=q.pop;qn;qn=qn->next)
	{
		inst=data_offset(qn,wtk_wakeup_inst_t,q_n);
		//wtk_wakeup_path_print(inst->pth);
		if(inst->node->depth == 1&&(ret == 0 || inst->pth->frame_init < ret))
		{
			ret = inst->pth->frame_init;
		}
	}
	return ret*wakeup->parm->cfg->frame_dur;
}

int wtk_wakeup_get_ssl_time(wtk_wakeup_t *wakeup,float *fs,float *fe)
{
	wtk_wakeup_inst_t  *inst=wakeup->final_inst;
	wtk_wakeup_path_t *pth;
	int e,s;

	if(wakeup->cfg->use_win)
	{
		if(wakeup->get==0)
		{
			return -1;
		}
		//wtk_debug("%d/%d %d/%d poped=%d\n",wakeup->raw_cache[0],wakeup->raw_cache[3],wakeup->cache[0],wakeup->cache[3],wakeup->poped);
		s=wakeup->cache[0]+wakeup->poped;
		e=wakeup->cache[3]+wakeup->poped;
		if(fs)
		{
			*fs=s*wakeup->parm->cfg->frame_dur;
		}
		if(fe)
		{
			*fe=e*wakeup->parm->cfg->frame_dur;
		}
		return 0;
	}
	if(inst)
	{
		pth=inst->pth;
//		wtk_debug("prob=%f\n",inst->prob);
//		wtk_wakeup_path_print(pth);
		//wtk_debug("=========== %f/%f\n",inst->prob,pth->prob);
		e=pth->max_prob_frame+wakeup->post_cfg->right_win;
		if(e>pth->frame_e)
		{
			e=pth->frame_e;
		}
		while(pth->prev)
		{
			pth=pth->prev;
		}
		s=pth->max_prob_frame-wakeup->post_cfg->left_win;
		if(s<0)
		{
			s=0;
		}
		if(fs)
		{
			*fs=s*wakeup->parm->cfg->frame_dur;
		}
		if(fe)
		{
			*fe=e*wakeup->parm->cfg->frame_dur;
		}
		//wtk_debug("pth=%p\n",inst->pth);
		//wtk_debug("frame=%d\n",wakeup->frame);
		//wtk_wakeup_path_print(inst->pth);
		return 0;
	}else
	{
		return -1;
	}
}

int wtk_wakeup_get_raw_final_time(wtk_wakeup_t *wakeup,float *fs,float *fe)
{
	int e,s;

	if(wakeup->get==0)
	{
		*fs=0;
		*fe=0;
		return -1;
	}
	s=wakeup->tmp_cache[0]+wakeup->poped;
	e=wakeup->tmp_cache[wakeup->post_cfg->n_network-1]+wakeup->poped;
	if(fs)
	{
		*fs=s*wakeup->parm->cfg->frame_dur;
	}
	if(fe)
	{
		*fe=e*wakeup->parm->cfg->frame_dur;
	}
	return 0;
}

int wtk_wakeup_get_final_hint_time(wtk_wakeup_t *wakeup,float *fs,float *fe)
{
	wtk_robin_t *rb=wakeup->win_robin;
	int e,s;
	int i;
	float *pf,sumf;
	int idx;

	if(wakeup->get==0)
	{
		return -1;
	}
	//wtk_debug("%d/%d\n",wakeup->cache[0],rb->used);
	idx=wakeup->cache[0];
	for(i=wakeup->cache[0];i>=0;--i)
	{
		pf=(float*)wtk_robin_at(rb,i);
		sumf=pf[1]+pf[3];
		//wtk_debug("v[%d]=%f\n",i,sumf);
		if(sumf<0.5)
		{
			break;
		}else
		{
			idx=i;
		}
	}
	s=idx+wakeup->poped;
	e=wakeup->cache[wakeup->post_cfg->n_network-1]+wakeup->poped;
	if(fs)
	{
		*fs=s*wakeup->parm->cfg->frame_dur;
	}
	if(fe)
	{
		*fe=e*wakeup->parm->cfg->frame_dur;
	}
	return 0;
}

int wtk_wakeup_get_final_time(wtk_wakeup_t *wakeup,float *fs,float *fe)
{
	wtk_wakeup_inst_t  *inst=wakeup->final_inst;
	wtk_wakeup_path_t *pth;
	int e,s;

	if(wakeup->cfg->use_win)
	{
		if(wakeup->get==0)
		{
			return -1;
		}
		s=wakeup->cache[0]+wakeup->poped;
		e=wakeup->cache[wakeup->post_cfg->n_network-1]+wakeup->poped;
		if(fs)
		{
			*fs=s*wakeup->parm->cfg->frame_dur;
		}
		if(fe)
		{
			*fe=e*wakeup->parm->cfg->frame_dur;
		}
		return 0;
	}
	if(inst)
	{
		pth=inst->pth;
//		wtk_debug("prob=%f\n",inst->prob);
//		wtk_wakeup_path_print(pth);
		//wtk_debug("=========== %f/%f\n",inst->prob,pth->prob);
		e=pth->max_prob_frame+wakeup->post_cfg->right_win;
		if(e>pth->frame_e)
		{
			e=pth->frame_e;
		}
		while(pth->prev)
		{
			pth=pth->prev;
		}
		s=pth->max_prob_frame-wakeup->post_cfg->left_win;
		if(s<0)
		{
			s=0;
		}
		if(fs)
		{
			*fs=s*wakeup->parm->cfg->frame_dur;
		}
		if(fe)
		{
			*fe=e*wakeup->parm->cfg->frame_dur;
		}
		//wtk_debug("pth=%p\n",inst->pth);
		//wtk_debug("frame=%d\n",wakeup->frame);
		//wtk_wakeup_path_print(inst->pth);
		return 0;
	}else
	{
		return -1;
	}
}


int wtk_wakeup_get_unsee_sample(wtk_wakeup_t *wakeup)
{
	int cnt;

	//wtk_debug("count=%f/%d\n",wakeup->count*1.0/32000,wakeup->idx);
	cnt=(wakeup->count>>1)-wakeup->idx*wakeup->parm->cfg->frame_dur*16000;
	return cnt;
}
float wtk_wakeup_get_result(wtk_wakeup_t *wakeup,wtk_string_t *v)
{
	wtk_strbuf_t *buf=wakeup->buf;
	float f;

	if(wakeup->final_inst)
	{
		f=wakeup->final_inst->prob;
	}else
	{
		f=0;
	}
	if(v)
	{
		wtk_strbuf_reset(buf);
		wtk_strbuf_push_f(buf,"{\"rec\":\"%.*s\",\"conf\":%f}",wakeup->cfg->word.len,wakeup->cfg->word.data,f);
		wtk_string_set((v),buf->data,buf->pos);
	}
	return f;
}

float wtk_wakeup_get_result2(wtk_wakeup_t *wakeup,wtk_string_t *v, int *res,int detail)
{
	wtk_strbuf_t *buf=wakeup->buf;
	float f,cur_thresh,pth_thresh;
	int i;

	cur_thresh=wakeup->post_cfg->network->cur_thresh;
	pth_thresh=wakeup->post_cfg->network->pth_thresh;
	if(wakeup->final_inst)
	{
		if(detail == 1){
			f=wakeup->final_inst->pth->prev->prev->prev->prob;
			printf(" %f",f);
			f=wakeup->final_inst->pth->prev->prev->prob;
			printf(" %f",f);
			f=wakeup->final_inst->pth->prev->prob;
			printf(" %f",f);
			f=wakeup->final_inst->pth->prob;
			printf(" %f",f);
			printf(" %f",cur_thresh);

			f=wakeup->final_inst->pth->prev->prev->prev->max_prob;
			printf(" %f",f);
			f=wakeup->final_inst->pth->prev->prev->max_prob;
			printf(" %f",f);
			f=wakeup->final_inst->pth->prev->max_prob;
			printf(" %f",f);
			f=wakeup->final_inst->pth->max_prob;
			printf(" %f",f);
			printf(" %f\n",pth_thresh);

		}else{
			f=wakeup->final_inst->prob;
			printf(" %f	OK!\n",f);
			//printf(" %d\n",wakeup->final_inst->pth->frame_e);
			//printf(" %d\n",wakeup->final_inst->pth->prev->prev->prev->frame_init);
		}
	}else
	{
		f=0;
		if(detail == 1){
			for(i = 0;i<=3;i++){
				printf(" %f",f);
			}
			printf(" %f",cur_thresh);
			for(i = 0;i<=3;i++){
				printf(" %f",f);
			}
			printf(" %f\n",pth_thresh);
		}else{
			printf(" %f	NO OK!\n",f);
		}
	}
	if(f>0){
		*res = 1;
	}
	if(v != NULL)
	{
		wtk_strbuf_reset(buf);
		wtk_strbuf_push_f(buf,"%f",f);
		wtk_string_set((v),buf->data,buf->pos);
	}
	return f;
}

void wtk_wakeup_print(wtk_wakeup_t *wakeup)
{
	if(wakeup->cfg->use_win)
	{
		wtk_wakeup_print_win(wakeup);
	}else
	{
		wtk_debug("inst: use=%d free=%d\n",wakeup->inst_pool->hoard.use_length,wakeup->inst_pool->hoard.cur_free);
		wtk_debug("path: use=%d free=%d\n",wakeup->pth_pool->hoard.use_length,wakeup->pth_pool->hoard.cur_free);
		if(wakeup->final_inst)
		{
			wtk_debug("final=%f\n",wakeup->final_inst->prob);
			wtk_wakeup_path_print(wakeup->final_inst->pth);
		}
	}
}

void wtk_wakeup_update_config(wtk_wakeup_t *wakeup,int smooth,int  min_raise,int max_frame,float cur_thresh,float pth_thresh)
{
	int i;
	wtk_wakeup_node_t *node;

	wakeup->post_cfg->min_raise = min_raise;
	//wakeup->post_cfg->wsmooth = smooth;
	for(i = 0;i <4;i++)
	{
		node = wakeup->post_cfg->network + i;
		node->cur_thresh = cur_thresh;
		node->pth_thresh = pth_thresh;
		node->max_frame = max_frame;
	}
}
void wtk_wakeup_set_max_frame(wtk_wakeup_t *wakeup,int max_frame){
	int i;
	wtk_wakeup_node_t *node;

	for(i = 0;i <4;i++)
	{
		node = wakeup->post_cfg->network + i;
		node->max_frame = max_frame;
	}
}
void wtk_wakeup_set_cur_thresh(wtk_wakeup_t *wakeup,float cur_thresh){
	int i;
	wtk_wakeup_node_t *node;

	for(i = 0;i <4;i++)
	{
		node = wakeup->post_cfg->network + i;
		node->cur_thresh = cur_thresh;
	}
}

void wtk_wakeup_set_pth_thresh(wtk_wakeup_t *wakeup,float pth_thresh)
{
	int i;
	wtk_wakeup_node_t *node;

	for(i = 0;i <4;i++)
	{
		node = wakeup->post_cfg->network + i;
		node->pth_thresh = pth_thresh;
	}
}

void wtk_wakeup_set_smooth(wtk_wakeup_t *wakeup,int smooth)
{
	int i;

	//wakeup->post_cfg->wsmooth = smooth;
	if(wakeup->feat_robin)
	{
		for(i=0;i<wakeup->feat_robin->nslot;++i)
		{
			wtk_wakeup_feat_delete((wtk_wakeup_feat_t*)(wakeup->feat_robin->r[i]));
		}
		wtk_robin_delete(wakeup->feat_robin);
		wtk_vecf_delete(wakeup->vec_smooth);
	}
	wakeup->vec_smooth=wtk_vecf_new(wakeup->cfg->parm.dnn.out_cols);
	//wakeup->feat_robin=wtk_robin_new(wakeup->post_cfg->wsmooth);
	for(i=0;i<wakeup->feat_robin->nslot;++i)
	{
		wakeup->feat_robin->r[i]=wtk_wakeup_feat_new(wakeup->vec_smooth->len);
	}
}
void wtk_wakeup_set_min_raise(wtk_wakeup_t *wakeup,int min_raise)
{
	wakeup->post_cfg->min_raise = min_raise;
}


int wtk_wakeup_test_file2(wtk_wakeup_t *wakeup,char *fn,float *pfs,float *pfe)
{
	wtk_riff_t *riff;
	char *p;
	int nx,ret;
	int cnt;
	int b=0;
	float fs,fe;
	int get=-1;

	riff=wtk_riff_new();
	wtk_riff_open(riff,fn);
	nx=riff->fmt.channels*2;
	p=(char*)wtk_malloc(nx);
	cnt=0;
	//t=time_get_ms();
	while(1)
	{
		ret=wtk_riff_read(riff,p,nx);
		if(ret!=nx){break;}
		++cnt;
		ret=wtk_wakeup_feed(wakeup,p,2,0);
		if(ret==1)
		{
			b=1;
			break;
		}
	}
	if(b==0)
	{
		ret=wtk_wakeup_feed(wakeup,NULL,0,1);
		if(ret==1)
		{
			b=1;
		}
	}
	if(b)
	{
		ret=wtk_wakeup_get_final_time(wakeup,&fs,&fe);
		if(ret==0)
		{
			//printf("==> %f %f\n",fs,fe);
			*pfs=fs;
			*pfe=fe;
			get=1;
		}
	}
	//t=time_get_ms()-t;
	//prob=wtk_wakeup_get_result(wakeup,NULL);
	//wtk_debug("time=%f rate=%f\n",t,t/(cnt*1.0/16));
	//wtk_debug("prob=%f\n",prob);
	wtk_free(p);
	wtk_riff_delete(riff);
	wtk_wakeup_reset(wakeup);
	return get;
}


int wtk_wakeup_test_file(char *cfg,char *wav,float *fs,float *fe)
{
	wtk_wakeup_cfg_t *wcfg;
	//wtk_main_cfg_t *main_cfg;
	wtk_wakeup_t *wakeup;
	int ret=-1;

	//main_cfg=wtk_main_cfg_new_type(wtk_wakeup_cfg,cfg);
	//if(!main_cfg){goto end;}
	//wakeup=wtk_wakeup_new((wtk_wakeup_cfg_t*)(main_cfg->cfg));
	wcfg=wtk_wakeup_cfg_new_bin2(cfg);
	if(!wcfg){goto end;}
	wakeup=wtk_wakeup_new(wcfg);
	ret=wtk_wakeup_test_file2(wakeup,wav,fs,fe);
	wtk_wakeup_delete(wakeup);
end:
//	if(main_cfg)
//	{
//		wtk_main_cfg_delete(main_cfg);
//	}
	return ret;
}

float wtk_wakeup_get_delay(wtk_wakeup_t *wakeup)
{
	return (wakeup->parm->n_frame_index-wakeup->frame)*wakeup->parm->cfg->frame_dur;
}
