#include "wtk_kwake.h"

void wtk_kwake_on_feat(wtk_kwake_t *wake,wtk_kfeat_t *feat);
void wtk_kwake_on_fix_feat(wtk_kwake_t *wake,wtk_kfeat_t *feat);

wtk_robin_t* wtk_kwake_new_robin(int n,int nvec)
{
        wtk_robin_t *rb;
        int i;

        rb=wtk_robin_new(n);
        for(i=0;i<n;++i)
        {
                rb->r[i]=(float*)wtk_calloc(nvec,sizeof(float));
        }
        return rb;
}

wtk_robin_t* wtk_kwake_new_robin_fix(int n,int nvec)
{
	wtk_robin_t *rb;
	int i;

	rb=wtk_robin_new(n);
	for(i=0;i<n;++i)
	{
		rb->r[i]=(short*)wtk_calloc(nvec,sizeof(short));
	}
	return rb;
}

void wtk_kwake_delete_robin(wtk_robin_t *rb)
{
        int i;

        for(i=0;i<rb->nslot;++i)
        {
                wtk_free(rb->r[i]);
        }
        wtk_robin_delete(rb);
}

int wtk_kwake_bytes(wtk_kwake_t *wake)
{
	int bytes;

	bytes=sizeof(wtk_kwake_t);
	if(wake->parm)
	{
		bytes+=wtk_kxparm_bytes(wake->parm);
	}
	bytes+=wake->nvec*sizeof(float)*2;
	bytes+=wake->nvec*sizeof(float)*(wake->rb_smooth->nslot+wake->rb_win->nslot);
	bytes+=wtk_robin_bytes(wake->rb_win);
	bytes+=wtk_robin_bytes(wake->rb_smooth);
	bytes+=wake->cfg->post.nnet*sizeof(int);
	wtk_debug("wake: %.3fkb\n",bytes*1.0/1024);
	return bytes;
}

wtk_kwake_t* wtk_kwake_new(wtk_kwake_cfg_t *cfg)
{
	wtk_kwake_t *w;
	int nvec;

	w=(wtk_kwake_t*)wtk_malloc(sizeof(wtk_kwake_t));
	w->cfg=cfg;
	w->parm=wtk_kxparm_new(&(cfg->parm));
	//w->use_fxipoint=w->parm->knn->cfg->use_fixpoint;
	if(cfg->use_feature)
	{
		nvec=cfg->feature.nnode;
	}else
	{
		nvec=cfg->parm.knn.output_dim;
	}
	w->nvec=nvec;
	w->wake_slot=(int*)wtk_calloc(cfg->post.nnet,sizeof(int));
	w->idle_pos=0;
	w->idled=1;
	w->hint_pos=-1;
	//w->state=WTK_KWAKE_QUIET;
	w->state=WTK_KWAKE_NOISE;
	w->nhist=0;
	w->quick_prob=NULL;
	w->fix_quick_prob=NULL;
	w->smooth=NULL;
	w->fix_smooth=NULL;
	w->feat=NULL;
	w->fix_feat=NULL;
	if(cfg->use_fixpoint)
	{
		wtk_kxparm_set_notify(w->parm,w,(wtk_kxparm_notify_f)wtk_kwake_on_fix_feat);
		w->fix_quick_prob=(short*)wtk_calloc(cfg->post.nid,sizeof(short));
		w->fix_smooth=(short*)wtk_calloc(nvec,sizeof(short));
		w->fix_feat=(short*)wtk_calloc(nvec,sizeof(short));
		w->rb_smooth=wtk_kwake_new_robin_fix(cfg->smooth_left+cfg->smooth_right+1,nvec);
		w->rb_win=wtk_kwake_new_robin_fix(cfg->max_win,nvec);
		w->wsi=FLOAT2FIX_ANY(0.1,cfg->shift);
	}else
	{
		wtk_kxparm_set_notify(w->parm,w,(wtk_kxparm_notify_f)wtk_kwake_on_feat);
		//w->xinput=(float*)wtk_calloc(nvec,sizeof(float));
		w->quick_prob=(float*)wtk_calloc(cfg->post.nid,sizeof(float));
		w->smooth=(float*)wtk_calloc(nvec,sizeof(float));
		w->feat=(float*)wtk_calloc(nvec,sizeof(float));
		w->rb_smooth=wtk_kwake_new_robin(cfg->smooth_left+cfg->smooth_right+1,nvec);
		w->rb_win=wtk_kwake_new_robin(cfg->max_win,nvec);
		w->hist_prob=0;
	}
	if(cfg->post.use_rf)
	{
		w->rf=wtk_kwake_post_rf_inst_new(&(cfg->post.rf_cfg));
		w->rf->bkg_idx=w->cfg->bkg_idx;
		w->rf->sil_idx=w->cfg->sil_idx;
	}
	wtk_kwake_reset(w);
	return w;
}

wtk_kwake_t* wtk_kwake_new2(wtk_kwake_cfg_t *cfg,int output_dim)
{
	wtk_kwake_t *w;
	int nvec;

	w=(wtk_kwake_t*)wtk_malloc(sizeof(wtk_kwake_t));
	w->cfg=cfg;
	w->parm=NULL;//wtk_kxparm_new(&(cfg->parm));
	//w->use_fxipoint=w->parm->knn->cfg->use_fixpoint;
	if(cfg->use_feature)
	{
		nvec=cfg->feature.nnode;
	}else
	{
		nvec=output_dim;
	}
	w->nvec=nvec;
	w->wake_slot=(int*)wtk_calloc(cfg->post.nnet,sizeof(int));
	w->idle_pos=0;
	w->idled=1;
	w->hint_pos=-1;
	//w->state=WTK_KWAKE_QUIET;
	w->state=WTK_KWAKE_NOISE;
	w->nhist=0;
	w->quick_prob=NULL;
	w->fix_quick_prob=NULL;
	w->smooth=NULL;
	w->fix_smooth=NULL;
	w->feat=NULL;
	w->fix_feat=NULL;
	if(cfg->use_fixpoint)
	{
		//wtk_kxparm_set_notify(w->parm,w,(wtk_kxparm_notify_f)wtk_kwake_on_fix_feat);
		w->fix_quick_prob=(short*)wtk_calloc(cfg->post.nid,sizeof(short));
		w->fix_smooth=(short*)wtk_calloc(nvec,sizeof(short));
		w->fix_feat=(short*)wtk_calloc(nvec,sizeof(short));
		w->rb_smooth=wtk_kwake_new_robin_fix(cfg->smooth_left+cfg->smooth_right+1,nvec);
		w->rb_win=wtk_kwake_new_robin_fix(cfg->max_win,nvec);
		w->wsi=FLOAT2FIX_ANY(0.1,cfg->shift);
	}else
	{
		//wtk_kxparm_set_notify(w->parm,w,(wtk_kxparm_notify_f)wtk_kwake_on_feat);
		//w->xinput=(float*)wtk_calloc(nvec,sizeof(float));
		w->quick_prob=(float*)wtk_calloc(cfg->post.nid,sizeof(float));
		w->smooth=(float*)wtk_calloc(nvec,sizeof(float));
		w->feat=(float*)wtk_calloc(nvec,sizeof(float));
		w->rb_smooth=wtk_kwake_new_robin(cfg->smooth_left+cfg->smooth_right+1,nvec);
		w->rb_win=wtk_kwake_new_robin(cfg->max_win,nvec);
		w->hist_prob=0;
	}
	if(cfg->post.use_rf)
	{
		w->rf=wtk_kwake_post_rf_inst_new(&(cfg->post.rf_cfg));
		w->rf->bkg_idx=w->cfg->bkg_idx;
		w->rf->sil_idx=w->cfg->sil_idx;
	}
	wtk_kwake_reset(w);
	return w;
}

void wtk_kwake_delete(wtk_kwake_t *w)
{
	//wtk_free(w->xinput);
	if(w->cfg->use_fixpoint)
	{
		wtk_free(w->fix_quick_prob);
		wtk_free(w->fix_smooth);
		wtk_free(w->fix_feat);
	}else
	{
		wtk_free(w->quick_prob);
		wtk_free(w->smooth);
		wtk_free(w->feat);
	}
	wtk_kwake_delete_robin(w->rb_smooth);
	wtk_kwake_delete_robin(w->rb_win);
	wtk_free(w->wake_slot);
	if(w->parm)
	{
		wtk_kxparm_delete(w->parm);
	}
	if(w->cfg->post.use_rf)
	{
		wtk_kwake_post_rf_inst_delete(w->rf);
	}
	wtk_free(w);
}

void wtk_kwake_start(wtk_kwake_t *w)
{
	wtk_kwake_start2(w,0);
}

void wtk_kwake_set_echoing(wtk_kwake_t *w,int echoing)
{
	w->wk_prob=0;
	w->echoing=echoing;
	w->post_quiet=NULL;
	w->xpost=NULL;
	if(w->cfg->use_triger && w->idled)
	{
		w->post=&(w->cfg->triger);
		if(w->cfg->use_triger_quiet)
		{
			//wtk_debug("aaaaaasssssss\n");
			w->post_quiet=&(w->cfg->triger_quiet);
		}
	}else
	{
		if(w->cfg->use_full_echo ||(w->echoing && w->cfg->use_echo))
		{
			w->post=&(w->cfg->echo);
//			if(w->cfg->use_echo_quiet)
//			{
//				w->post_quiet=&(w->cfg->echo_quiet);
//			}
		}else
		{
			w->post=&(w->cfg->post);
//			if(w->cfg->use_post_quiet)
//			{
//				w->post_quiet=&(w->cfg->post_quiet);
//			}
		}
		if(w->cfg->use_triger && w->cfg->use_triger_quiet)
		{
			w->post_quiet=&(w->cfg->triger_quiet);
		}
	}
	w->post_normal=w->post;
	//wtk_debug("use triger=%d\n",w->cfg->use_triger);
}

void wtk_kwake_start2(wtk_kwake_t *w,int echoing)
{
//	w->state=WTK_KWAKE_QUIET;
//	w->nhist=0;
	wtk_kwake_set_echoing(w,echoing);
	if(w->post_quiet && w->state==WTK_KWAKE_QUIET)
	{
		w->post=w->post_quiet;
	}
	if(w->parm)
	{
		wtk_kxparm_start(w->parm);
	}
}

void wtk_kwake_reset(wtk_kwake_t *w)
{
	wtk_kwake_reset2(w,1);
}

void wtk_kwake_reset2(wtk_kwake_t *w,int reset_cmn)
{
	//wtk_debug("========== reset =================\n");
	w->quicked=0;
	w->first_n=0;
	w->second_n=0;
	w->quiet=0;
	w->echoing=0;
	w->input=0;

	wtk_robin_reset(w->rb_smooth);
	wtk_robin_reset(w->rb_win);
	if(w->cfg->use_fixpoint)
	{
		memset(w->fix_smooth,0,w->nvec*sizeof(short));
	}else
	{
		memset(w->smooth,0,w->nvec*sizeof(float));
	}
	if(w->parm)
	{
		wtk_kxparm_reset2(w->parm,reset_cmn);
	}
	w->smooth_idx=0;
	w->feat_idx=0;
	w->poped=0;
	w->waked=0;
	w->check_step=0;
	if(w->cfg->post.use_rf)
	{
		wtk_kwake_post_rf_inst_reset2(w->rf);
	}
	if(w->cfg->use_fixpoint)
	{
		w->fix_prob=0;
	}else
	{
		w->prob=0;
	}
}

void wtk_kwake_print_feat(wtk_kwake_t *wake,float *feat)
{
	wtk_kwake_post_cfg_t *post=wake->post;
	int i,id;
	int v[16]={0};
	static int ki=0;

	++ki;
	//wtk_debug("nnet=%d\n",post->nnet);
	printf("v[%d/%d]: %.3f/%.3f",ki,wake->feat_idx,feat[wake->cfg->sil_idx],feat[wake->cfg->bkg_idx]);
	if(ki>100)
	{
		//exit(0);
	}
	for(i=0;i<post->nnet;++i)
	{
		id=post->network[i].id;
		//wtk_debug("id=%d\n",id);
		if(v[id]==0)
		{
			printf("/%.3f",feat[id]);
			v[id]=1;
		}
	}
	printf("\n");
}

void wtk_kwake_print_win(wtk_kwake_t *wake)
{
	wtk_robin_t *rb=wake->rb_win;
	int i,j,id;
	float *pf;
	wtk_kwake_post_cfg_t *post=wake->xpost;
	int v[16]={0};
	int s,e;

	wtk_debug("==================== wake(quiet=%d triger=%d post=%d echo=%d) quicked=%d ===========================\n",
			wake->xpost==wake->post_quiet,wake->xpost==&(wake->cfg->triger),wake->xpost==&(wake->cfg->post),
			wake->xpost==&(wake->cfg->echo),wake->quicked);
	if(wake->prob>0)
	{
		printf("conf: %.3f\n",wake->prob);
		for(i=0;i<post->nnet;++i)
		{
			id=wake->wake_slot[i];
			pf=(float*)wtk_robin_at(rb,id);
			printf(" v[%d/%d]=%.3f",id+wake->poped,id,pf[post->network[i].id]);
		}
		printf("\n\n");
		s=wake->wake_slot[0];
		e=wake->wake_slot[post->nnet-1]+1;
	}else
	{
		s=0;
		e=rb->used;
	}
	for(i=s;i<e;++i)
	{
		pf=(float*)wtk_robin_at(rb,i);
		//print_float(pf,6);
		printf("v[%d/%d]: %.3f/%.3f",wake->poped+i,i,pf[wake->cfg->sil_idx],pf[wake->cfg->bkg_idx]);
		memset(v,0,16*sizeof(int));
		for(j=0;j<post->nnet;++j)
		{
			id=post->network[j].id;
			//wtk_debug("id=%d/%d\n",post->network[j].id,post->network[j].id2);
			if(v[id]==0)
			{
				printf("/%.3f",pf[id]);
				v[id]=1;
			}
		}
		printf("\n");
		//fflush(stdout);
		//exit(0);
	}
//	if(wake->xpost==wake->post_quiet)
//	{
//		exit(0);
//	}
}

//#define DEBUG_X
//#define DEBUG_NX 99

int wtk_kwake_quick_check(wtk_kwake_t *wake)
{
	wtk_robin_t *rb=wake->rb_win;
	wtk_kwake_post_cfg_t *post=wake->post;
	int *id=post->id;
	int nid=post->nid;
	int i,j;
	float *pf;
	float *prob=wake->quick_prob;//[16]={0};
	float f,sum;

	//wtk_debug("triger=%d\n",post==&(wake->cfg->triger));
	for(i=0;i<nid;++i)
	{
		prob[i]=0;
	}
	for(i=0;i<rb->used;++i)
	{
		pf=(float*)wtk_robin_at(rb,i);//(rb->r[i]);
		//print_float(pf,wake->nvec);
		for(j=0;j<nid;++j)
		{
			//wtk_debug("v[%d]=%d %f\n",j,id[j],pf[id[j]]);
			if(pf[id[j]]>prob[j])
			{
				prob[j]=pf[id[j]];
			}
		}
		//exit(0);
	}
	//exit(0);
	f=prob[0];
	sum=f;
	for(i=1;i<nid;++i)
	{
		sum*=prob[i];
		//wtk_debug("v[%d]=%f/%f\n",i,prob[i],sum);
		if(prob[i]>f)
		{
			f=prob[i];
		}
	}
	if(f<wake->cfg->quick_max_phn_prob)
	{
#ifdef DEBUG_X
		//print_float(prob,nid);
		wtk_debug("max phn failed %f/%f/%f\n",f,post->max_phn_prob,wake->cfg->quick_max_phn_prob);
#endif
		return 0;
	}
	wake->max_phn_prob=f;
	for(i=0;i<nid;++i)
	{
		if(prob[i]<post->id_prob[i])
		{
//#ifdef DEBUG_X
//			wtk_debug("v[%d]=%f/%f prob failed\n",i,prob[i],post->id_prob[i]);
//#endif
			return 0;
		}
	}
	//wtk_debug("f=%f nid=%d\n",sum,nid);
	//f=pow(sum,1.0/nid);
	//if(f<post->prob)
	if(sum<wake->cfg->quick_prob)
	{
#ifdef DEBUG_X
			wtk_debug("%f/%f sum prob failed\n",sum,wake->cfg->quick_prob);
#endif
		return 0;
	}
	return 1;
}

float wtk_kwake_check_inst(wtk_kwake_t *wake,float pth_prob,float pre_prob,int s,int depth,int *slot)
{
	wtk_kwake_post_cfg_t *post=wake->xpost;
	wtk_robin_t *rb=wake->rb_win;
	int i,e,nx,j,idx;
	float *pf,f;
	wtk_kwake_node_t *node=post->network+depth;
	int id=node->id;
	float pth_f,wrd_f,prob;
	float max_prob=0,next_prob;
	int bit1[10]={0};
	int bit2[10];

	e=rb->used-wtk_kwake_post_cfg_min_end(post,depth);
	if(depth>0)
	{
		e=min(e,s+node->max_frame);
	}
#ifdef DEBUG_X
	wtk_debug("id[%d]=[%d,%d] [%d,%d]\n",node->id,s,e,wake->poped+s,wake->poped+e);
#endif
	for(i=s;i<e;)
	{
		nx=min(wake->cfg->step,e-i);
		idx=i;
		i+=nx;
		pf=(float*)wtk_robin_at(rb,idx);
		f=pf[id];
		for(j=idx+1;j<i;++j)
		{
			pf=(float*)wtk_robin_at(rb,j);
			//wtk_debug("v[%d/%d]=%f/%f\n",j+wake->poped,id,pf[id],f);
			if(pf[id]>f)
			{
				f=pf[id];
				idx=j;
			}
		}
#ifdef DEBUG_X
		wtk_debug("v[%d/%d/%d,%d,%d]=%.3f/%.3f\n",depth,idx+wake->poped,id,wake->poped+i-nx,wake->poped+i,f,node->prob);
#endif
		if(f<node->prob)
		{
#ifdef DEBUG_X
		wtk_debug("v[%d/%d/%d,%d,%d]=%.3f/%.3f prob failed\n",depth,idx+wake->poped,id,wake->poped+i-nx,wake->poped+i,f,node->prob);
#endif
			continue;
		}
		if(node->check_edge)
		{
			if(idx>0)
			//if(idx>s)
			{
				pf=(float*)wtk_robin_at(rb,idx-1);
				if(pf[id]>f)
				{
	#ifdef DEBUG_X
					//wtk_debug("idx=%d\n",idx);
					wtk_debug("v[%d/%d/%d,%d,%d]=%.3f/%.3f pre failed\n",depth,idx+wake->poped,id,wake->poped+i-nx,wake->poped+i,pf[id],f);
					//exit(0);
	#endif
					continue;
				}
			}
			if(idx<rb->used-1)
			{
				pf=(float*)wtk_robin_at(rb,idx+1);
				if(pf[id]>f)
				{
	#ifdef DEBUG_X
			wtk_debug("v[%d/%d/%d,%d,%d]=%.3f/%.3f nxt failed\n",depth,idx+wake->poped,id,wake->poped+i-nx,wake->poped+i,f,node->prob);
	#endif
					continue;
				}
			}
		}
		if(node->wrd_prob >0)
		{
			wrd_f =f * pre_prob;
			if(wrd_f <node->wrd_prob)
			{
				continue;
			}
		}
		if(node->wrd_min_phn_prob>0)
		{
			prob = max(f,pre_prob);
			if(prob < node->wrd_min_phn_prob)
			{
				continue;
			}
		}
		pth_f=f*pth_prob;
		if(node->pth_prob>0)
		{
			if(pth_f<node->pth_prob)
			{
#ifdef DEBUG_X
		wtk_debug("v[%d/%d/%d,%d,%d]=%.3f/%.3f pth failed\n",depth,idx+wake->poped,id,wake->poped+i-nx,wake->poped+i,f,node->prob);
#endif
				continue;
			}
		}
#ifdef DEBUG_X
			wtk_debug("f[%d/%d]=%f\n",depth,post->nnet,f);
#endif
		if(depth==post->nnet-1)
		{
			next_prob=1;
		}else
		{
			next_prob=wtk_kwake_check_inst(wake,pth_f,f,idx+node[1].min_frame,depth+1,bit2);
		}
#ifdef DEBUG_X
			wtk_debug("f[%d]=%f/%f\n",depth,f,next_prob);
#endif
		if(next_prob>0 && f*next_prob>max_prob)
		{
			max_prob=f*next_prob;
			bit1[0]=idx;
			memcpy(bit1+1,bit2,(post->nnet-depth-1)*sizeof(int));
			//wtk_debug("f[%d]=%f/%f\n",depth,f,max_prob);
			//exit(0);
#ifdef DEBUG_X
			wtk_debug("f[%d]=%f/%f\n",depth,f,max_prob);
#endif
		}
	}
#ifdef DEBUG_X
	wtk_debug("depth[%d]=%f\n",depth,max_prob);
#endif
	if(max_prob>0)
	{
		memcpy(slot,bit1,(post->nnet-depth)*sizeof(int));
	}
	return max_prob;
}

int wtk_kwake_check_bg(wtk_kwake_t *wake,int s,int e)
{
	wtk_kwake_post_cfg_t *post=wake->xpost;
	wtk_robin_t *rb=wake->rb_win;
	float *pf;
	int i,j;
	float t=post->bkg_thresh;
	float t2=post->sil_thresh;
	int win=post->bkg_win;
	int cnt=0;
	int id,id2;
	float min_bg=1,f;
	wtk_kwake_peak_node_t *peak=post->peak;
	int npeak=post->npeak;
	int xcnt[8]={0};
	int xpeak[8]={0};
	int sil_cnt=0;
	int check_peak=0;
	int lcnt[8]={0};

	if(peak)
	{
		for(j=0;j<npeak;++j)
		{
			if(peak[j].max_count>0)
			{
				check_peak=1;
			}
		}
	}
	//wtk_debug("======== bg[%d,%d]\n",s,e);
	id=wake->cfg->bkg_idx;
	id2=wake->cfg->sil_idx;
	for(i=s;i<=e;++i)
	{
		pf=(float*)wtk_robin_at(rb,i);
		if(check_peak)
		{
			for(j=0;j<npeak;++j)
			{
				if(peak[j].max_count<=0)
				{
					continue;
				}
				f=pf[peak[j].id];
				if(peak[j].low_thresh>0)
				{
					//wtk_debug("f[%d]=%f/%f cnt=%d\n",i+wake->poped,f,peak[j].low_thresh,lcnt[j]);
					if(f<peak[j].low_thresh)
					{
						++lcnt[j];
					}
				}
				if(xpeak[j]==0)
				{
					if(f>peak[j].thresh_max)
					{
						++xcnt[j];
						xpeak[j]=1;
					}
				}else
				{
					if(f<peak[j].thresh_min)
					{
						xpeak[j]=0;
					}
				}
			}
		}
		if(pf[id]<min_bg)
		{
			min_bg=pf[id];
		}
		f=pf[id]+pf[id2];
		//wtk_debug("v[%d/%d]=%f/%f/%d [%d,%d] [%d,%d] echoing=%d\n",i+wake->poped,i,f,t,cnt,s,e,s+wake->poped,e+wake->poped,wake->echoing);
		if(f>=t)
		{
			++cnt;
			if(cnt>win)
			{
#ifdef DEBUG_X
				wtk_debug("win=%d/%d\n",cnt,win);
#endif
				return 0;
			}
		}else
		{
			cnt=0;
		}
		if(t2>0)
		{
			if(pf[id2]>t2)
			{
				++sil_cnt;
				if(sil_cnt>post->sil_win)
				{
#ifdef DEBUG_X
				wtk_debug("win=%d/%d\n",sil_cnt,post->sil_win);
#endif
					return 0;
				}
			}else
			{
				sil_cnt=0;
			}
		}
	}
	//wtk_debug("[%d,%d] min_bg=%f/%f sum=%f\n",s,e,min_bg,post->min_bkg,sum/(e-s));
	if(min_bg>post->min_bkg)
	{
#ifdef DEBUG_X
				wtk_debug("min bg=%f/%f\n",min_bg,post->min_bkg);
#endif
		return 0;
	}
	//wtk_debug("peak=%d/%d\n",xcnt[0],xcnt[1]);
	if(peak)
	{
		//wtk_debug("ff=%f/%f\n",ff[0],ff[1]);
		for(i=0;i<npeak;++i)
		{
			//wtk_debug("peak[%d]=%d/%d [%d,%d]\n",i,xcnt[i],peak[i].max_count,s,e);
			if(peak[i].max_count>0 && xcnt[i]>peak[i].max_count)
			{
				//wtk_debug("peak=%d/%d\n",xcnt[i],peak[i].max_count);
#ifdef DEBUG_X
				wtk_debug("peak=%d/%d\n",xcnt[i],peak[i].max_count);
#endif
				return 0;
			}
			if(peak[i].low_thresh>0 && lcnt[i]>=peak[i].low_cnt)
			{
#ifdef DEBUG_X
				wtk_debug("peak=%d/%d\n",xcnt[i],peak[i].max_count);
#endif
				return 0;
			}
			if(peak[i].node)
			{
				id=peak[i].id;
				if(peak[i].max_prob>0)
				{
					f=0;
					for(j=0;j<peak[i].nnode;++j)
					{
						s=wake->wake_slot[peak[i].node[j]];
						pf=(float*)wtk_robin_at(rb,s);
						if(pf[id]>f)
						{
							f=pf[id];
						}
					}
					if(f<peak[i].max_prob)
					{
						///wtk_debug("peak=%f/%f\n",f,peak[i].max_prob);
	#ifdef DEBUG_X
					wtk_debug("peak max=%f/%f\n",f,peak[i].max_prob);
	#endif
						return 0;
					}
				}
				if(peak[i].min_ratio>0)
				{
					int xs,xe;
					float prob;

					xs=wake->wake_slot[peak[i].node[0]];
					xe=wake->wake_slot[peak[i].node[peak[i].nnode-1]];
					pf=(float*)wtk_robin_at(rb,xs);
					prob=pf[id];
					pf=(float*)wtk_robin_at(rb,xe);
					if(pf[id]>prob)
					{
						prob=pf[id];
					}
					prob*=peak[i].min_ratio;
					f=1.0;
					for(j=xs+1;j<xe;++j)
					{
						pf=(float*)wtk_robin_at(rb,j);
						if(pf[id]<f)
						{
							f=pf[id];
							if(f<prob)
							{
								break;
							}
						}
					}
					//wtk_debug("f=%f/%f [%d,%d]\n",f,peak[i].min_prob,xs,xe);
					if(f>prob)
					{
						//wtk_debug("peak[%d]=%f/%f [%d,%d]\n",i,f,peak[i].min_prob,xs,xe);
#ifdef DEBUG_X
						wtk_debug("peak[%d]=%f/%f(%f) [%d,%d]\n",i,f,prob,peak[i].min_ratio,xs,xe);
#endif
						//wtk_debug("return\n");
						return 0;
					}
				}
			}
		}
	}
	return 1;
}

//#define DEBUG_X

int wtk_kwake_check_post(wtk_kwake_t *wake,wtk_kwake_post_cfg_t *post)
{
	//wtk_kwake_post_cfg_t *post=wake->post;
	wtk_kwake_node_t *network;
	wtk_robin_t *rb=wake->rb_win;
	wtk_kwake_node_t *node;
	float prob,t;
	int i,j;
	float *pf;
	float f;
	int id;
	int ret,win;
	int win_s;

	if(wake->prob<post->prob)
	{
#ifdef DEBUG_X
		wtk_debug("prob=%f/%f  failed\n",wake->prob,post->prob);
		//wtk_debug("prob=%f\n",wake->cfg->triger_low.prob);
#endif
		return 0;
	}
#ifdef DEBUG_X
	//wtk_kwake_print_win(wake);
#endif
	prob=0;
	for(i=0;i<post->nnet;++i)
	{
		pf=(float*)wtk_robin_at(rb,wake->wake_slot[i]);
		f=pf[post->network[i].id];
		if(f>prob)
		{
			prob=f;
		}
	}
	//wtk_debug("prob=%f/%f normal=%d\n",prob,post->max_phn_prob,post==wake->post_normal);
	wake->quicked=0;
	if(post->quick_network)
	{
		int j;
		int n=post->nnet/2;
		int s,e;
		float *pf;
		int id;
		float minf=post->quick_min_prob;
		int cnt;

		//n=1;
		cnt=0;
		for(i=0;i<n;++i)
		{
			id=post->network[i].id;
			s=wake->wake_slot[i]+1;
			e=wake->wake_slot[i+2];
			for(j=s;j<e;++j)
			{
				pf=wtk_robin_at(rb,j);
				if(pf[id]<minf)
				{
					++cnt;
					break;
				}
			}
		}
		//wtk_debug("cnt=%d/%d prob=%f/%f\n",cnt,n,prob,post->quick_max_phn_prob);
		if(cnt!=n)
		{
			if(prob<post->quick_max_phn_prob)
			{
		#ifdef DEBUG_X
				wtk_debug("quick max phn failed=%f/%f\n",prob,post->quick_max_phn_prob);
		#endif
				ret=0;
				goto end;
			}
			network=post->quick_network;
			wake->quicked=1;
			for(i=0;i<post->nnet;++i)
			{
				pf=wtk_robin_at(rb,wake->wake_slot[i]);
				if(pf[network[i].id]<network[i].prob)
				{
					ret=0;
					goto end;
				}
			}
		}else
		{
			if(prob<post->max_phn_prob)
			{
		#ifdef DEBUG_X
				wtk_debug("max phn failed=%f/%f\n",prob,post->max_phn_prob);
		#endif
				ret=0;
				goto end;
			}
			network=post->network;
		}
//		wtk_debug("============  use normal network=%d minf=%f normal=%d normal=%d =================\n",cnt==n,minf,
//				network==post->network?1:0,post==wake->post_normal);
	}else
	{
		if(prob<post->max_phn_prob)
		{
	#ifdef DEBUG_X
			wtk_debug("max phn failed=%f/%f\n",prob,post->max_phn_prob);
	#endif
			ret=0;
			goto end;
		}
		network=post->network;
	}
//	if(post->bkg_thresh>0)
//	{
//		i=wtk_kwake_check_bg(wake,wake->wake_slot[0],wake->wake_slot[post->nnet-1]);
//		if(i==0)
//		{
//#ifdef DEBUG_X
//			wtk_debug("bkg failed\n");
//#endif
//			ret=0;
//			goto end;
//		}
//	}
	id=network->id;
	win_s=wake->wake_slot[0];
	for(i=win_s;i>=0;--i)
	{
		pf=(float*)wtk_robin_at(rb,i);
		//wtk_debug("v[%d]=%f\n",i,pf[id]);
		if(pf[id]<0.1)
		{
			break;
		}
		win_s=i;
	}
	if(post->nnet==4)
	{
		if(1)
		{
			int j;
			int n=post->nnet/2;
			int s,e;
			float *pf;
			int id;
			int cnt,maxc;
			int win;
			float f;

			//wtk_debug("=============== check min =================\n");
			for(i=0;i<n;++i)
			{
				node=post->network+i;
				f=node->speech_inter_thresh;
				if(f<=0)
				{
					continue;
				}
				win=node->speech_inter_max_win;
				//wtk_debug("=============== check min[%d]: %f/%d =================\n",i,f,win);
				id=node->id;
				s=wake->wake_slot[i]+1;
				e=wake->wake_slot[i+2];
				cnt=0;
				maxc=0;
				for(j=s;j<e;++j)
				{
					pf=wtk_robin_at(rb,j);
					if(pf[id]<f)
					{
						++cnt;
					}else
					{
						if(cnt>maxc)
						{
							//wtk_debug("min=%d/%d\n",cnt,maxc);
							if(cnt>win)
							{
								ret=0;
								wtk_debug("failed min=%d/%d\n",cnt,maxc);
								goto end;
							}
							maxc=cnt;
						}
						cnt=0;
					}
				}
			}
		}
		if(post->half_scale1>0 && post->half_scale2>0)
		{
			pf=(float*)wtk_robin_at(rb,wake->wake_slot[0]);
			f=pf[network[0].id];
			pf=(float*)wtk_robin_at(rb,wake->wake_slot[1]);
			f+=pf[network[1].id];
			pf=(float*)wtk_robin_at(rb,wake->wake_slot[2]);
			t=pf[network[2].id];
			pf=(float*)wtk_robin_at(rb,wake->wake_slot[3]);
			t+=pf[network[3].id];
			//wtk_debug("f=%f t=%f ratio=%f\n",f,t,f/t);
			t=f/t;
			if(t>post->half_scale1 || t<post->half_scale2)
			{
	#ifdef DEBUG_X
				wtk_debug("half failed t=%f\n",t);
	#endif
				ret=0;
				goto end;
			}
		}
	}
	//wtk_debug("win_s=%d/%d\n",win_s,wake->wake_slot[0]);
	//win=wake->wake_slot[wake->post->nnet-1]-wake->wake_slot[0];
	win=wake->wake_slot[wake->post->nnet-1]-win_s;
	for(i=0;i<post->nnet;++i)
	{
		node=network+i;
		id=node->id;
		pf=(float*)wtk_robin_at(rb,wake->wake_slot[i]);
		f=pf[id];
		//wtk_debug("v[%d]=%f/%f\n",i,node->min_mid_ratio,node->min_mid_ratio_triger);
		if(node->min_mid_ratio>0 && f<node->min_mid_ratio_triger)
		{
			j=i-1;
			if(i<0)
			{
				j=post->nnet-1;
			}
			pf=(float*)wtk_robin_at(rb,wake->wake_slot[j]);
			t=pf[network[j].id];
			j=i+1;
			if(j>post->nnet-1)
			{
				j=0;
			}
			pf=(float*)wtk_robin_at(rb,wake->wake_slot[j]);
			t+=pf[network[j].id];
			//wtk_debug("mid[%d]=%f/%f/%f\n",i,t/2,f,(t/2)*node->min_mid_ratio);
			if((t/2)*(node->min_mid_ratio)>f)
			{
#ifdef DEBUG_X
				wtk_debug("min mid ratio v[%d]=%f/%f failed \n",i,t/2,f);
#endif
				ret=0;
				goto end;
			}
		}
		if(node->max_other_id>=0 && f<node->max_other_id_triger && pf[node->max_other_id]>node->max_other_id_triger_thresh)
		{
			//wtk_debug("f=%f/%f\n",f,pf[node->max_other_id]);
#ifdef DEBUG_X
			wtk_debug("max other failed\n");
#endif
			ret=0;
			goto end;
		}
		if(node->max_prev_triger>0 || node->max_prev_ratio_triger>0)
		{
			if(node->max_prev_node>=0)
			{
				pf=(float*)wtk_robin_at(rb,wake->wake_slot[node->max_prev_node]);
				prob=pf[network[node->max_prev_node].id];
			}else
			{
				pf=(float*)wtk_robin_at(rb,wake->wake_slot[i-1]);
				prob=pf[node[-1].id];
			}
			//wtk_debug("v[%d]=%f/%f/%f\n",i,prob,f,prob*node->max_prev_ratio);
			if(node->max_prev_triger>0 && prob>node->max_prev_triger && f<node->max_prev_triger_thresh)
			{
#ifdef DEBUG_X
			wtk_debug("max prev tirger failed\n");
#endif
				ret=0;
				goto end;
			}
			if(node->max_prev_ratio_triger>0 && prob>node->max_prev_ratio_triger && f<prob*node->max_prev_ratio)
			{
#ifdef DEBUG_X
				wtk_debug("max prev ratio failed v[%d]:%f/%f\n",i,f,prob);
#endif
				ret=0;
				goto end;
			}
		}
		if(node->max_next_triger>0 || node->max_next_ratio_triger>0)
		{
			if(node->max_next_node>=0)
			{
				pf=(float*)wtk_robin_at(rb,wake->wake_slot[node->max_next_node]);
				prob=pf[network[node->max_next_node].id];
			}else
			{
				pf=(float*)wtk_robin_at(rb,wake->wake_slot[i+1]);
				prob=pf[node[1].id];
			}
			//wtk_kwake_print_win(wake);
			//wtk_debug("f[%d]=%f/%f/%f\n",i,f,prob,prob*node->max_next_ratio);
			if(node->max_next_triger>0 && prob>node->max_next_triger && f<node->max_next_triger_thresh)
			{
#ifdef DEBUG_X
			wtk_debug("maxt next triger failed\n");
#endif
				ret=0;
				goto end;
			}
			if(node->max_next_ratio_triger>0 && prob>node->max_next_ratio_triger && f<prob*node->max_next_ratio)
			{
#ifdef DEBUG_X
			wtk_debug("maxt next ratio v[%d]=%f/%f failed\n",i,f,prob);
#endif
				ret=0;
				goto end;
			}
		}
		if(node->wrd_depth>0)
		{
			if(node->wrd_prob>0)
			{
				prob=f;
				for(j=1;j<node->wrd_depth;++j)
				{
					pf=(float*)wtk_robin_at(rb,wake->wake_slot[i-j]);
					prob*=pf[node[-j].id];
				}
				if(prob<node->wrd_prob)
				{
#ifdef DEBUG_X
					wtk_debug("wrd[%d]=%f/%f prob triger failed\n",i,prob,node->wrd_prob);
#endif
					ret=0;
					goto end;
				}
			}
			if(node->wrd_min_phn_prob>0)
			{
				prob=f;
				for(j=1;j<node->wrd_depth;++j)
				{
					//wtk_debug("v[%d/%d]=%d slot=%d/%d\n",i,j,i-j,wake->wake_slot[i-j],rb->used);
					pf=(float*)wtk_robin_at(rb,wake->wake_slot[i-j]);
					if(prob<pf[node[-j].id])
					{
						prob=pf[node[-j].id];
					}
				}
				if(prob<node->wrd_min_phn_prob)
				{
#ifdef DEBUG_X
					wtk_debug("wrd[%d]=%f/%f min phn prob failed\n",i,prob,node->wrd_min_phn_prob);
#endif
					ret=0;
					goto end;
				}
			}
		}
		if(node->char_depth>0)
		{
			float xf;

			prob=f;
			xf=f;
			for(j=1;j<node->char_depth;++j)
			{
				//wtk_debug("v[%d/%d]=%d slot=%d/%d\n",i,j,i-j,wake->wake_slot[i-j],rb->used);
				pf=(float*)wtk_robin_at(rb,wake->wake_slot[i-j]);
				t=pf[node[-j].id];
				xf*=t;
				if(prob<t)
				{
					prob=t;
				}
			}
			//wtk_debug("v[%d]=%f/%f\n",i,prob,node->char_min_phn_prob);
			if(node->char_prob>0 && xf<node->char_prob)
			{
#ifdef DEBUG_X
			wtk_debug("char failed\n");
#endif
				ret=0;
				goto end;
			}
			if(node->char_min_phn_prob>0 && prob<node->char_min_phn_prob)
			{
#ifdef DEBUG_X
				wtk_debug("wrd[%d]=%f/%f min phn prob failed quick=%d\n",i,prob,node->char_min_phn_prob,network==post->network?0:1);
#endif
				ret=0;
				goto end;
			}
		}

		if(node->max_other_triger>0 && node->max_other_triger_thresh>0 && f<node->max_other_triger_thresh)
		{
			pf=(float*)wtk_robin_at(rb,wake->wake_slot[i]);
			prob=0;
			for(j=0;j<post->nid;++j)
			{
				if(post->id[j]==id)
				{
					continue;
				}
				prob+=pf[post->id[j]];
			}
			if(prob>node->max_other_triger)
			{
#ifdef DEBUG_X
			wtk_debug("max other failed\n");
#endif
				ret=0;
				goto end;
			}
		}
		if(node->speech_dur_thresh>0 && f>node->speech_dur_thresh)
		{
			int tot;

			if(node->speech_dur_win>0)
			{
				tot=0;
				for(j=wake->wake_slot[i];j>=0;--j)
				{
					pf=(float*)wtk_robin_at(rb,j);
					///wtk_debug("v[%d/%d]=%f\n",wake->poped+j,j,pf[id]);
					if(pf[id]<node->speech_dur_min || pf[id]>f)
					{
						break;
					}else if(pf[id]>node->speech_dur_thresh)
					{
						tot=0;
					}else
					{
						++tot;
					}
				}
				if(tot<node->speech_dur_win)
				{
					ret=0;
					goto end;
				}
			}

			if(node->speech_dur_win>0)
			{
				tot=0;
				for(j=wake->wake_slot[i]+1;j<rb->used;++j)
				{
					pf=(float*)wtk_robin_at(rb,j);
					///wtk_debug("v[%d/%d]=%f\n",wake->poped+j,j,pf[id]);
					if(pf[id]<node->speech_dur_min || pf[id]>f)
					{
						break;
					}else if(pf[id]>node->speech_dur_thresh)
					{
						tot=0;
					}else
					{
						++tot;
					}
				}
				if(tot<node->speech_dur_win)
				{
					ret=0;
					wtk_debug("tot<node->speech_dur_win\n");
					goto end;
				}
			}
		}
		if(node->speech_thresh>0)
		{
			int tot=0;
			int n;

			if(i==0)
			{
				n=0;
			}else
			{
				n=wake->wake_slot[i-1];
			}
			for(j=wake->wake_slot[i];j>=n;--j)
			{
				pf=(float*)wtk_robin_at(rb,j);
				///wtk_debug("v[%d/%d]=%f\n",wake->poped+j,j,pf[id]);
				if(pf[id]<node->speech_thresh)
				{
					break;
				}
				++tot;
			}
//			if(i==post->nnet-1)
//			{
//				n=rb->used;
//			}else
//			{
//				n=wake->wake_slot[i+1];
//			}
			n=rb->used;
			for(j=wake->wake_slot[i]+1;j<n;++j)
			{
				pf=(float*)wtk_robin_at(rb,j);
				//wtk_debug("v[%d/%d]=%f\n",wake->poped+j,j,pf[id]);
				if(pf[id]<node->speech_thresh)
				{
					break;
				}
				++tot;
			}
//			wtk_debug("v[%d/%d]=%d/%d thresh=%f ratio=%f/%f [%d,%d]\n",wake->poped+i,i,tot,win,node->speech_thresh,tot*1.0/win,
//					node->speech_ratio,wake->wake_slot[0]+wake->poped,wake->wake_slot[wake->xpost->nnet-1]+wake->poped);
			if(node->speech_win>0 && tot>node->speech_win)
			{
#ifdef DEBUG_X
				wtk_debug("v[%d/%d]: %d/%d speech win failed\n",i,id,tot,node->speech_win);
#endif
				ret=0;
				goto end;
			}
			if(node->speech_min_win>0 && tot<node->speech_min_win)
			{
#ifdef DEBUG_X
			wtk_debug("speec min win failed %d/%d\n",tot,node->speech_min_win);
#endif
				ret=0;
				goto end;
			}
			if(node->speech_ratio>0 && tot*1.0/win>node->speech_ratio)
			{
#ifdef DEBUG_X
				wtk_debug("speech[%d] ratio failed tot=%d/%d %f\n",i,tot,win,tot*1.0/win);
#endif
				ret=0;
				goto end;
			}
		}
	}
	if(post->wake_s_thresh>0)
	{
		node=network;
		id=node->id;
		j=wake->wake_slot[0];
		for(i=wake->wake_slot[0]-1;i>=0;--i)
		{
			pf=(float*)wtk_robin_at(rb,i);
			if(pf[id]<post->wake_s_thresh)
			{
				//wake->wake_slot[0]=i+1;
				break;
			}else
			{
				j=i;
			}
		}
		//wake->wake_slot[0]=j;
		//wtk_debug("v=[%d,%d] %d\n",wake->wake_slot[0],wake->wake_slot[post->nnet-1],wake->wake_slot[post->nnet-1]-wake->wake_slot[0]);
		if(wake->wake_slot[post->nnet-1]-j<post->wake_min_win)
		{
#ifdef DEBUG_X
			wtk_debug("wake min failed[%d/%d]\n",wake->wake_slot[post->nnet-1]-j,post->wake_min_win);
#endif
			ret=0;
			goto end;
		}
	}
	if(post->bkg_thresh>0)
	{
		i=wtk_kwake_check_bg(wake,wake->wake_slot[0],wake->wake_slot[post->nnet-1]);
		if(i==0)
		{
#ifdef DEBUG_X
			wtk_debug("bkg failed\n");
#endif
			ret=0;
			goto end;
		}
	}
	ret=1;
end:
	return ret;
}


//#ifdef DEBUG_X
//#undef DEBUG_X
//#endif

int wtk_kwake_check(wtk_kwake_t *wake)
{
	//wtk_kwake_post_cfg_t *post=wake->post;
	wtk_robin_t *rb=wake->rb_win;
	float prob;
	int i;
	int ret;

#ifdef DEBUG_X
#ifdef DEBUG_NX
	if(wake->feat_idx<DEBUG_NX)
	{
		return 0;
	}
#endif
#endif
	//wtk_kwake_print_win(wake);
	//if((wake->cfg->use_triger==0 || wake->idled==0) && wake->cfg->use_quick && wake->max_phn_prob>wake->cfg->quick_max_phn_prob)
	//wtk_debug("################### check quick prob=%f\n",wake->max_phn_prob);
	if(wake->max_phn_prob>wake->cfg->quick_max_phn_prob && (wake->cfg->use_quick||(wake->echoing&&wake->cfg->use_echo_quick)))
	{
		if(wake->echoing)
		{
			wake->xpost=&(wake->cfg->echo_quick);
		}else
		{
			wake->xpost=&(wake->cfg->quick);
		}
		//wtk_kwake_print_win(wake);
		prob=wtk_kwake_check_inst(wake,1,0,1,0,wake->wake_slot);
		//wtk_debug("================> check quick prob=%f\n",prob);
		if(prob>0)
		{
			//wtk_debug("################### check quick prob=%f\n",prob);
			if(wake->wake_slot[0]<30)// && wake->wake_slot[0]>0)
			{
				int n;
				float *pf;
				int id;
				int b;

				b=1;
				if(0)
				{
					n=wake->wake_slot[0];
					id=wake->xpost->network[1].id;
					for(i=0;i<n;++i)
					{
						pf=wtk_robin_at(rb,i);
						if(pf[id]>0.5)
						{
							b=0;
							break;
						}
					}
				}
				//wtk_debug("b=%d\n",b);
				if(b)
				{
					n=rb->used-1;
					b=0;
					id=wake->xpost->network[wake->xpost->nnet-1].id;
					for(i=wake->wake_slot[wake->xpost->nnet-1]+1;i<n;++i)
					{
						pf=wtk_robin_at(rb,i);
						//wtk_debug("v[%d]=%f\n",i,pf[id]);
						if(pf[id]<0.1)
						{
							b=1;
							break;
						}
					}
				}
				if(b && rb->used>(wake->wake_slot[wake->xpost->nnet-1]+3))
				{
					n=rb->used-1;
					id=wake->xpost->network[0].id;
					ret=0;
					for(i=wake->wake_slot[wake->xpost->nnet-1]+1;i<n;++i)
					{
						pf=wtk_robin_at(rb,i);
						//wtk_debug("v[%d]=%f\n",i,pf[id]);
						if(pf[id]>0.1)
						{
							++ret;
						}
					}
//					wtk_debug("wake=%d/%d ret=%d/%d\n",wake->wake_slot[0]+wake->poped,wake->wake_slot[1]+wake->poped,
//							ret,n-(wake->wake_slot[wake->xpost->nnet-1]+1));
					if(ret>5)//ret*1.0/(n-(wake->wake_slot[wake->xpost->nnet-1]+1))>0.5)
					{
						b=0;
					}
				}
				//wtk_debug("b=%d\n",b);
				if(b)
				{
					wake->prob=pow(prob,1.0/wake->xpost->nnet);
					//wtk_debug("################################\n");
					ret=wtk_kwake_check_post(wake,wake->xpost);
					//wtk_debug("################### check quick prob=%f ret=%d\n",prob,ret);
					//exit(0);
					if(ret==1)
					{
						goto end1;
					}
				}
			}
		}
		if(wake->cfg->use_full_quick)
		{
			return  0;
		}
		//exit(0);
	}
	wake->xpost=wake->post;
			//wtk_debug("sssssssssssssssssssssssssssssss\n");
	prob=wtk_kwake_check_inst(wake,1,0,1,0,wake->wake_slot);
	//wtk_kwake_print_win(wake);
	if(prob<=0)
	{
#ifdef DEBUG_X
		wtk_debug("prob=%f failed quiet=%d\n",prob,wake->post==wake->post_quiet);
#endif
		return 0;
	}
	if(wake->xpost->use_rf) 
	{
		// wtk_debug("%d\n",wake->poped);
		wake->rf->thresh = wake->post->rf_cfg.rf_thresh;
        return wtk_kwake_check_post_rf(wake->rf,wake->rb_win,wake->wake_slot,wake->cfg->post.nnet);
    }
	prob=pow(prob,1.0/wake->post->nnet);
#ifdef DEBUG_X
	wtk_debug("prob=%f/%f\n",prob,wake->post->prob);
	wtk_kwake_print_win(wake);
#endif
//wtk_kwake_print_win(wake);
	wake->prob=prob;
	ret=wtk_kwake_check_post(wake,wake->xpost);
	//wtk_debug("ret=%d triger=%d\n",ret,wake->post==&(wake->cfg->triger));
	if(ret==0)
	{
		if(wake->idled && wake->cfg->use_triger && wake->cfg->use_full_triger==0)
		{
			ret=wtk_kwake_check_post(wake,&(wake->cfg->post));
//			wtk_debug("=========== xfound %f idled=%d %d/%d ret=%d ==================\n",wake->idle_pos*1.0/8000,wake->idled,wake->cfg->use_triger,
//					wake->cfg->use_full_triger,ret);
			//wtk_debug("use post ret=%d\n",ret);
			if(ret==0){goto end;}
			//wtk_debug("=========== xfound %f ==================\n",wake->idle_pos*1.0/8000);
			ret=wake->idle_pos-wake->hint_pos;
			//wtk_debug("ret=%d hint=%d pos=%d/%d\n",ret,wake->hint_pos,wake->poped+wake->wake_slot[wake->post->nnet-1],wake->feat_idx);
			if((wake->hint_pos>=0 && ret>0.5*wake->cfg->parm.parm.rate &&  ret<3*wake->cfg->parm.parm.rate))
			{
				ret=1;
			}else
			{
				wake->hint_pos=wake->idle_pos;
				ret=0;
				goto end;
			}
		}else
		{
			goto end;
		}
	}
//	if(wake->idled==0)
//	{
//		exit(0);
//	}
end1:
	//wtk_debug("used=%d/%d\n",rb->used,wake->wake_slot[wake->xpost->nnet-1]);
	if((rb->used-wake->wake_slot[wake->xpost->nnet-1])>30)
	{
		//wtk_debug("%d\n",rb->used-wake->wake_slot[wake->xpost->nnet-1]);
		i=rb->used/2;
		if(i>0)
		{
			wake->poped+=i;
			wtk_robin_pop2(rb,i);
		}
		ret=0;
	}else
	{
		ret=1;
	}
end:
#ifdef DEBUG_X
		wtk_debug("check ret=%d\n",ret);
#endif
	return ret;
}

void wtk_kwake_update_quiet_state(wtk_kwake_t *wake,wtk_kfeat_t *feat)
{
	float *input;
	float f;

	input=feat->v;
	//wtk_debug("v[%d]=%.3f/%.3f/%.3f/%.3f\n",feat->index,pf[0],pf[1],pf[2],pf[3]);
	switch(wake->state)
	{
	case WTK_KWAKE_QUIET:
		f=input[wake->cfg->bkg_idx];
		if(f>0.5)
		{
#ifdef DEBUG_T
			wtk_debug("goto noise\n");
#endif
			wake->nhist=0;
			wake->hist_prob=0;
			wake->state=WTK_KWAKE_NOISE;
			wake->post=wake->post_normal;
		}else
		{
			f=input[wake->cfg->sil_idx];
			if(f>0.90)
			{
				wake->nhist=0;
				wake->hist_prob=0;
				wake->wk_prob=0;
			}else
			{
				wake->hist_prob+=input[wake->cfg->bkg_idx];
				wake->wk_prob+=input[wake->post->network->id];
				++wake->nhist;
				if(wake->nhist>10)
				{
					//wtk_debug("init=%f/%f\n",wake->hist_prob/wake->nhist,wake->wk_prob/wake->nhist);
					wake->nhist=0;
					//wake->hist_prob=0;
					if((wake->hist_prob*0.5)<wake->wk_prob || wake->wk_prob>0.1)
					{
						//wtk_debug("goto speech\n");
						wake->state=WTK_KWAKE_SPEECH;
						wake->wk_prob=0;
						wake->hist_prob=0;
#ifdef DEBUG_T
						wtk_debug("goto speech\n");
#endif
					}else
					{

						//wtk_debug("goto noise\n");
#ifdef DEBUG_T
						wtk_debug("goto noise\n");
#endif
						wake->wk_prob=0;
						wake->hist_prob=0;
						wake->state=WTK_KWAKE_NOISE;
						wake->post=wake->post_normal;
					}
				}
			}
		}
		break;
	case WTK_KWAKE_SPEECH:
		f=input[wake->cfg->bkg_idx];
		if(f>0.2)
		{
			++wake->nhist;
			if(wake->nhist>50)
			{
#ifdef DEBUG_T
				wtk_debug("goto noise\n");
#endif
				wake->nhist=0;
				wake->hist_prob=0;
				wake->state=WTK_KWAKE_NOISE;
				wake->post=wake->post_normal;
			}
		}else
		{
			wake->nhist=0;
			if(input[wake->cfg->sil_idx]>0.95)
			{
#ifdef DEBUG_T
				wtk_debug("goto sil\n");
#endif
				wake->hist_prob=0;
				wake->wk_prob=0;
				wake->state=WTK_KWAKE_QUIET;
			}
		}
		break;
	case WTK_KWAKE_NOISE:
		f=input[wake->cfg->sil_idx];
		if(f>0.92)
		{
			++wake->nhist;
			if(wake->nhist>150)
			{
#ifdef DEBUG_T
				wtk_debug("goto sil nhist=%d\n",wake->nhist);
#endif
				wake->nhist=0;
				wake->hist_prob=0;
				wake->state=WTK_KWAKE_QUIET;
				wake->post=wake->post_quiet;
			}
		}else
		{
			wake->nhist=0;
		}
		break;
	}
}

void wtk_kwake_update_quiet_state2(wtk_kwake_t *wake,float  *input)
{
	wtk_kwake_post_cfg_t *post=wake->post;
	wtk_kwake_cfg_t *cfg=wake->cfg;
	int i;
	float f;

	//wtk_kwake_print_feat(wake,input);
	switch(wake->state)
	{
	case WTK_KWAKE_QUIET:
		f=input[wake->cfg->bkg_idx];
		if(f>cfg->normal_min_prob)
		{
			++wake->nhist;
			wake->hist_prob+=f;
			if(wake->nhist>cfg->normal_hist)
			{
				//wtk_debug("############### goto quiet normal=%d\n",wake->nhist);
				wake->nhist=0;
				wake->hist_prob=0;
				wake->state=WTK_KWAKE_NOISE;
				wake->post=wake->post_normal;
				//exit(0);
			}
		}else
		{
			wake->nhist=0;
			wake->hist_prob=0;
		}
		break;
	case WTK_KWAKE_SPEECH:
		exit(0);
		break;
	case WTK_KWAKE_NOISE:
		f=0;
		for(i=0;i<post->nid;++i)
		{
			f+=input[post->id[i]];
		}
		//wtk_debug("f=%f n=%d\n",f,wake->nhist);
		if(f>cfg->quiet_max_prob || input[wake->cfg->bkg_idx]>0.3)
		{
			wake->nhist=0;
			wake->hist_prob=0;
		}else
		{
			wake->hist_prob+=f;
			++wake->nhist;
			if(wake->nhist>cfg->quiet_hist)
			{
				//wtk_debug("############### goto quiet nhist=%d\n",wake->nhist);
				wake->nhist=0;
				wake->hist_prob=0;
				wake->state=WTK_KWAKE_QUIET;
				if(wake->cfg->use_triger_quiet)
				{
					wake->post=wake->post_quiet;
				}
			}
		}
		break;
	}
}

void wtk_kwake_on_featf(wtk_kwake_t *wake,float *feat, int index)
{
//#define DEBUG_T
// return;
	wtk_kwake_cfg_t *cfg=wake->cfg;
	wtk_robin_t *rb=wake->rb_smooth;
	int nvec=wake->nvec;
	float *pf,f,*xf;
	int i,j,b;
	float *smooth=wake->smooth;
	float *input=wake->feat;
	int ret;
	wtk_kwake_node_t *node;
	if(wake->waked){return;}
	b=rb->used==rb->nslot;
	//wtk_debug("%d/%d/%d\n",b,rbin->used,rbin->nslot);
	pf=(float*)wtk_robin_next(rb);
	if(b)
	{
		for(i=0;i<nvec;++i)
		{
			smooth[i]-=pf[i];
		}
	}
	xf=feat;
	if(wake->cfg->use_feature)
	{
		wtk_kwake_feature_cfg_t *feat=&(wake->cfg->feature);
		int *in;
		int nin;

		for(i=0;i<nvec;++i)
		{
			nin=feat->nodes[i].nin;
			in=feat->nodes[i].in;
			f=0;
			for(j=0;j<nin;++j)
			{
				f+=xf[in[j]];
			}
			smooth[i]+=pf[i]=f;
			//wtk_debug("v[%d]=%f/%f/%f/%f\n",f->index,vec->p[0],vec->p[1],vec->p[2],vec->p[3]);
		}
	}else
	{
		for(i=0;i<nvec;++i)
		{
			smooth[i]+=pf[i]=xf[i];
			//wtk_debug("v[%d]=%f/%f/%f/%f\n",f->index,vec->p[0],vec->p[1],vec->p[2],vec->p[3]);
		}
	}
	if((rb->used-wake->smooth_idx-1)<cfg->smooth_right)
	{
		return;
	}
	for(i=0;i<nvec;++i)
	{
		input[i]=smooth[i]/rb->used;
		if(input[i]<0)
		{
			input[i]=-input[i];
		}
	}
	if(wake->post_quiet)
	{
		wtk_kwake_update_quiet_state2(wake,input);
	}
	++wake->feat_idx;
#ifdef DEBUG_X
#ifdef	DEBUG_NX
	if(wake->feat_idx>DEBUG_NX)
	{
		exit(0);
	}
#endif
#endif
	//if(cfg->debug)
	{
		//wtk_kwake_print_feat(wake,input);
		// return;
	}
	rb=wake->rb_win;
	node=wake->post->network;
	if(rb->used>=rb->nslot)
	{
		wtk_robin_pop(rb);
		++wake->poped;
		//删除前面低频
		for(i=0;i<rb->used;++i)
		{
			pf=(float*)wtk_robin_at(rb,i);
			//wtk_debug("v[%d]=%f/%f/%f/%f %f\n",i,pf[0],pf[1],pf[2],pf[3],node->cur_thresh);
			if(pf[node->id]>=node->prob)
			{
				break;
			}
		}
		//wtk_debug("poped=%d\n",i);
		if(i>0)
		{
			wake->poped+=i;
			wtk_robin_pop2(rb,i);
		}
	}
	if(rb->used==0)
	{
		if(input[node->id]<node->prob)
		{
			++wake->poped;
			return;
		}
		wake->first_n=-1;
		wake->second_n=-1;
	}
	//wtk_debug("%.3f/%.3f/%.3f/%.3f\n",wake->xinput[0],wake->xinput[1],wake->xinput[2],wake->xinput[2]);
	if(input[node[1].id]>=node[1].prob)
	{
		wake->second_n=index;
	}
	if(input[node->id]>=node->prob)
	{
		wake->first_n=index;
	}else if(wake->second_n<0)
	{
		if((index-wake->first_n)>node[1].max_frame)
		{
			//wtk_debug("reset sencod\n");
			wake->poped+=rb->used+1;
			wtk_robin_reset(rb);
			return;
		}
	}
	//wtk_debug("fist=%d secnod=%d t=%d feat=%d\n",wake->first_n,wake->second_n,wake->third_n,feat->index);
	//wtk_debug("%d/%f\n",rb->used,input[wake->post->network->id]);
	pf=(float*)wtk_robin_next(rb);
	memcpy(pf,input,nvec*sizeof(float));
	//wtk_debug("=============yes %d/%d =============\n",rb->used,cfg->min_win);
	if(rb->used<cfg->min_win)
	{
		return;
	}else if(cfg->check_step>1 && rb->used<rb->nslot)
	{
#ifdef DEBUG_X
#else
		++wake->check_step;
		if(wake->check_step>cfg->check_step)
		{
			wake->check_step=0;
		}else
		{
			//wtk_debug("check=%d\n",wake->check_step);
			return;
		}
#endif
	}
	//wtk_debug("check=%d\n",cfg->check_step);
	ret=wtk_kwake_quick_check(wake);
	//wtk_debug("quick[%d/%d]=%d\n",wake->feat_idx,rb->used,ret);
	if(ret==1)
	{
		ret=wtk_kwake_check(wake);
		// wtk_debug("ret=%d\n",ret);
		if(ret==1)
		{
			wake->waked=1;
			//wtk_debug("======== waked ===============\n");
			//wtk_kwake_print_win(wake);
			//exit(0);
			return;
		}
	}
}

void wtk_kwake_on_feat(wtk_kwake_t *wake,wtk_kfeat_t *feat)
{
//#define DEBUG_T
// return;
	wtk_kwake_cfg_t *cfg=wake->cfg;
	wtk_robin_t *rb=wake->rb_smooth;
	int nvec=wake->nvec;
	float *pf,f,*xf;
	int i,j,b;
	float *smooth=wake->smooth;
	float *input=wake->feat;
	int ret;
	wtk_kwake_node_t *node;
	if(wake->waked){return;}
	b=rb->used==rb->nslot;
	//wtk_debug("%d/%d/%d\n",b,rbin->used,rbin->nslot);
	pf=(float*)wtk_robin_next(rb);
	if(b)
	{
		for(i=0;i<nvec;++i)
		{
			smooth[i]-=pf[i];
		}
	}
	xf=feat->v;
	if(wake->cfg->use_feature)
	{
		wtk_kwake_feature_cfg_t *feat=&(wake->cfg->feature);
		int *in;
		int nin;

		for(i=0;i<nvec;++i)
		{
			nin=feat->nodes[i].nin;
			in=feat->nodes[i].in;
			f=0;
			for(j=0;j<nin;++j)
			{
				f+=xf[in[j]];
			}
			smooth[i]+=pf[i]=f;
			//wtk_debug("v[%d]=%f/%f/%f/%f\n",f->index,vec->p[0],vec->p[1],vec->p[2],vec->p[3]);
		}
	}else
	{
		for(i=0;i<nvec;++i)
		{
			smooth[i]+=pf[i]=xf[i];
			//wtk_debug("v[%d]=%f/%f/%f/%f\n",f->index,vec->p[0],vec->p[1],vec->p[2],vec->p[3]);
		}
	}
	if((rb->used-wake->smooth_idx-1)<cfg->smooth_right)
	{
		return;
	}
	for(i=0;i<nvec;++i)
	{
		input[i]=smooth[i]/rb->used;
		if(input[i]<0)
		{
			input[i]=-input[i];
		}
	}
	if(wake->post_quiet)
	{
		wtk_kwake_update_quiet_state2(wake,input);
	}
	++wake->feat_idx;
#ifdef DEBUG_X
#ifdef	DEBUG_NX
	if(wake->feat_idx>DEBUG_NX)
	{
		exit(0);
	}
#endif
#endif
	if(cfg->debug)
	{
		wtk_kwake_print_feat(wake,input);
		// return;
	}
	rb=wake->rb_win;
	node=wake->post->network;
	if(rb->used>=rb->nslot)
	{
		wtk_robin_pop(rb);
		++wake->poped;
		//删除前面低频
		for(i=0;i<rb->used;++i)
		{
			pf=(float*)wtk_robin_at(rb,i);
			//wtk_debug("v[%d]=%f/%f/%f/%f %f\n",i,pf[0],pf[1],pf[2],pf[3],node->cur_thresh);
			if(pf[node->id]>=node->prob)
			{
				break;
			}
		}
		//wtk_debug("poped=%d\n",i);
		if(i>0)
		{
			wake->poped+=i;
			wtk_robin_pop2(rb,i);
		}
	}
	if(rb->used==0)
	{
		if(input[node->id]<node->prob)
		{
			++wake->poped;
			return;
		}
		wake->first_n=-1;
		wake->second_n=-1;
	}
	//wtk_debug("%.3f/%.3f/%.3f/%.3f\n",wake->xinput[0],wake->xinput[1],wake->xinput[2],wake->xinput[2]);
	if(input[node[1].id]>=node[1].prob)
	{
		wake->second_n=feat->index;
	}
	if(input[node->id]>=node->prob)
	{
		wake->first_n=feat->index;
	}else if(wake->second_n<0)
	{
		if((feat->index-wake->first_n)>node[1].max_frame)
		{
			//wtk_debug("reset sencod\n");
			wake->poped+=rb->used+1;
			wtk_robin_reset(rb);
			return;
		}
	}
	//wtk_debug("fist=%d secnod=%d t=%d feat=%d\n",wake->first_n,wake->second_n,wake->third_n,feat->index);
	//wtk_debug("%d/%f\n",rb->used,input[wake->post->network->id]);
	pf=(float*)wtk_robin_next(rb);
	memcpy(pf,input,nvec*sizeof(float));
	//wtk_debug("=============yes %d/%d =============\n",rb->used,cfg->min_win);
	if(rb->used<cfg->min_win)
	{
		return;
	}else if(cfg->check_step>1 && rb->used<rb->nslot)
	{
#ifdef DEBUG_X
#else
		++wake->check_step;
		if(wake->check_step>cfg->check_step)
		{
			wake->check_step=0;
		}else
		{
			//wtk_debug("check=%d\n",wake->check_step);
			return;
		}
#endif
	}
	//wtk_debug("check=%d\n",cfg->check_step);
	ret=wtk_kwake_quick_check(wake);
	//wtk_debug("quick[%d/%d]=%d\n",wake->feat_idx,rb->used,ret);
	if(ret==1)
	{
		ret=wtk_kwake_check(wake);
		// wtk_debug("ret=%d\n",ret);
		if(ret==1)
		{
			wake->waked=1;
			//wtk_debug("======== waked ===============\n");
			//wtk_kwake_print_win(wake);
			//exit(0);
			return;
		}
	}
}

int wtk_kwake_quick_check_fix(wtk_kwake_t *wake)
{
	wtk_robin_t *rb=wake->rb_win;
	wtk_kwake_post_cfg_t *post=wake->post;
	int *id=post->id;
	int nid=post->nid;
	int i,j;
	short *pf;
	short *prob=wake->fix_quick_prob;//[16]={0};
	int f;
	int sum;
	int shift=wake->cfg->shift;

	for(i=0;i<nid;++i)
	{
		prob[i]=0;
	}
	for(i=0;i<rb->used;++i)
	{
		pf=(short*)wtk_robin_at(rb,i);//(rb->r[i]);
		//print_float(pf,wake->nvec);
		for(j=0;j<nid;++j)
		{
			//wtk_debug("v[%d]=%d %f\n",j,id[j],pf[id[j]]);
			if(pf[id[j]]>prob[j])
			{
				prob[j]=pf[id[j]];
			}
		}
		//exit(0);
	}
	//exit(0);
	f=prob[0];
	sum=f;
	for(i=1;i<nid;++i)
	{
		sum=(sum*prob[i])>>shift;
		//wtk_debug("v[%d]=%f/%f\n",i,prob[i],sum);
		if(prob[i]>f)
		{
			f=prob[i];
		}
	}
	if(f<wake->cfg->fix_quick_max_phn_prob)
	{
#ifdef DEBUG_X
		//print_float(prob,nid);
		//wtk_debug("max phn failed %f/%f/%f\n",f,post->max_phn_prob,wake->cfg->quick_max_phn_prob);
#endif
		return 0;
	}
	wake->fix_max_phn_prob=f;
	for(i=0;i<nid;++i)
	{
		if(prob[i]<post->fix->id_prob[i])
		{
//#ifdef DEBUG_X
//			wtk_debug("v[%d]=%f/%f prob failed\n",i,prob[i],post->id_prob[i]);
//#endif
			return 0;
		}
	}
	//wtk_debug("f=%f nid=%d\n",sum,nid);
	//f=pow(sum,1.0/nid);
	//if(f<post->prob)
	//wtk_debug("sum=%d/%d\n",sum,wake->cfg->fix_quick_prob,FIX2FLOAT_AN);
	if(sum<wake->cfg->fix_quick_prob)
	{
#ifdef DEBUG_X
			//wtk_debug("%f/%f sum prob failed\n",f,post->prob);
#endif
		return 0;
	}
	return 1;
}

void wtk_kwake_print_fix_win(wtk_kwake_t *wake)
{
	wtk_robin_t *rb=wake->rb_win;
	int i,j,id;
	short *pf;
	wtk_kwake_post_cfg_t *post=wake->xpost;
	int v[16]={0};
	int s,e;
	int shift=wake->cfg->shift;

	if(!post)
	{
		post=wake->post;
	}
	wtk_debug("==================== wake(quiet=%d triger=%d post=%d echo=%d) ===========================\n",
			wake->xpost==wake->post_quiet,wake->xpost==&(wake->cfg->triger),wake->xpost==&(wake->cfg->post),
			wake->xpost==&(wake->cfg->echo));
	if(wake->fix_prob>0)
	{
		printf("conf: %.3f\n",FIX2FLOAT_ANY(wake->fix_prob,shift));
		for(i=0;i<post->nnet;++i)
		{
			id=wake->wake_slot[i];
			pf=(short*)wtk_robin_at(rb,id);
			printf(" v[%d/%d]=%.3f",id+wake->poped,id,FIX2FLOAT_ANY(pf[post->network[i].id],shift));
		}
		printf("\n\n");
		s=wake->wake_slot[0];
		e=wake->wake_slot[post->nnet-1]+1;
	}else
	{
		s=0;
		e=rb->used;
	}
	for(i=s;i<e;++i)
	{
		pf=(short*)wtk_robin_at(rb,i);
		//print_float(pf,6);
		printf("v[%d/%d]: %.3f/%.3f",wake->poped+i,i,FIX2FLOAT_ANY(pf[wake->cfg->sil_idx],shift),FIX2FLOAT_ANY(pf[wake->cfg->bkg_idx],shift));
		memset(v,0,16*sizeof(int));
		for(j=0;j<post->nnet;++j)
		{
			id=post->network[j].id;
			//wtk_debug("id=%d/%d\n",post->network[j].id,post->network[j].id2);
			if(v[id]==0)
			{
				printf("/%.3f",FIX2FLOAT_ANY(pf[id],shift));
				v[id]=1;
			}
		}
		printf("\n");
		//fflush(stdout);
		//exit(0);
	}
//	if(wake->xpost==wake->post_quiet)
//	{
//		exit(0);
//	}
}

//#define DEBUG_X
//#define DEBUG_NX 50

int wtk_kwake_check_inst_fix(wtk_kwake_t *wake,int pth_prob,int pre_prob,int s,int depth,int *slot)
{
	wtk_kwake_post_cfg_t *post=wake->xpost;
	wtk_robin_t *rb=wake->rb_win;
	int i,e,nx,j,idx;
	short *pf;
	int  f,wrd_f,prob;
	wtk_kwake_node_t *node=post->network+depth;
	int id=node->id;
	int pth_f;
	int max_prob=0,next_prob;
	int bit1[10]={0};
	int bit2[10];
	int shift=wake->cfg->shift;

	e=rb->used-wtk_kwake_post_cfg_min_end(post,depth);
	if(depth>0)
	{
		e=min(e,s+node->max_frame);
	}
#ifdef DEBUG_X
	wtk_debug("id[%d]=[%d,%d] [%d,%d]\n",node->id,s,e,wake->poped+s,wake->poped+e);
#endif
	for(i=s;i<e;)
	{
		nx=min(wake->cfg->step,e-i);
		idx=i;
		i+=nx;
		pf=(short*)wtk_robin_at(rb,idx);
		f=pf[id];
		for(j=idx+1;j<i;++j)
		{
			pf=(short*)wtk_robin_at(rb,j);
			//wtk_debug("v[%d/%d]=%f/%f\n",j+wake->poped,id,pf[id],f);
			if(pf[id]>f)
			{
				f=pf[id];
				idx=j;
			}
		}
#ifdef DEBUG_X
		wtk_debug("v[%d/%d/%d,%d,%d]=%.3f/%.3f\n",depth,idx+wake->poped,id,wake->poped+i-nx,wake->poped+i,
				FIX2FLOAT_ANY(f,shift),FIX2FLOAT_ANY(node->prob,shift));
#endif
		if(f<node->fix->prob)
		{
#ifdef DEBUG_X
		wtk_debug("v[%d/%d/%d,%d,%d]=%.3f/%.3f prob failed\n",depth,idx+wake->poped,id,wake->poped+i-nx,wake->poped+i,
				FIX2FLOAT_ANY(f,shift),FIX2FLOAT_ANY(node->prob,shift));
#endif
			continue;
		}
		if(node->check_edge)
		{
			pf=(short*)wtk_robin_at(rb,idx-1);
			if(pf[id]>f)
			{
#ifdef DEBUG_X
				wtk_debug("v[%d/%d/%d,%d,%d]=%.3f/%.3f pre failed\n",depth,idx+wake->poped,id,wake->poped+i-nx,wake->poped+i,
						FIX2FLOAT_ANY(pf[id],shift),FIX2FLOAT_ANY(f,shift));
#endif
				continue;
			}
			if(idx<rb->used-1)
			{
				pf=(short*)wtk_robin_at(rb,idx+1);
				if(pf[id]>f)
				{
	#ifdef DEBUG_X
			wtk_debug("v[%d/%d/%d,%d,%d]=%.3f/%.3f nxt failed\n",depth,idx+wake->poped,id,wake->poped+i-nx,wake->poped+i,
					FIX2FLOAT_ANY(f,shift),FIX2FLOAT_ANY(node->prob,shift));
	#endif
					continue;
				}
			}
		}
        if (node->fix->wrd_prob > 0) {
            wrd_f = (f * pre_prob) >> shift;
            if (wrd_f < node->fix->wrd_prob) {
                continue;
            }
        }
        if (node->fix->wrd_min_phn_prob > 0) {
            prob = max(f, pre_prob);
            if (prob < node->fix->wrd_min_phn_prob) {
                continue;
            }
        }
		pth_f=(f*pth_prob)>>shift;
		if(node->fix->pth_prob>0)
		{
			if(pth_f<node->fix->pth_prob)
			{
#ifdef DEBUG_X
				wtk_debug("v[%d/%d/%d,%d,%d]=%.3f/%.3f pth failed\n",depth,idx+wake->poped,id,wake->poped+i-nx,wake->poped+i,
						FIX2FLOAT_ANY(f,shift),FIX2FLOAT_ANY(node->prob,shift));
#endif
				continue;
			}
		}
#ifdef DEBUG_X
		wtk_debug("depth[%d/%d]=%f\n",depth,post->nnet,FIX2FLOAT_ANY(f,shift));
#endif
		if(depth==post->nnet-1)
		{
			next_prob=1<<shift;
		}else
		{
			next_prob=wtk_kwake_check_inst_fix(wake,pth_f,f,idx+node[1].min_frame,depth+1,bit2);
		}
#ifdef DEBUG_X
		wtk_debug("depth[%d]=%f next=%f\n",depth,FIX2FLOAT_ANY(f,shift),FIX2FLOAT_ANY(next_prob,shift));
#endif
		if(next_prob>0 && ((f*next_prob)>>shift)>max_prob)
		{
			max_prob=(f*next_prob)>>shift;
			bit1[0]=idx;
			memcpy(bit1+1,bit2,(post->nnet-depth-1)*sizeof(int));
			//wtk_debug("f[%d]=%f/%f\n",depth,f,max_prob);
			//exit(0);
#ifdef DEBUG_X
			wtk_debug("depth update max[%d]=%f/%f\n",depth,FIX2FLOAT_ANY(f,shift),FIX2FLOAT_ANY(max_prob,shift));
#endif
		}
	}
#ifdef DEBUG_X
	wtk_debug("depth final[%d]=%f\n",depth,FIX2FLOAT_ANY(max_prob,shift));
#endif
	if(max_prob>0)
	{
		memcpy(slot,bit1,(post->nnet-depth)*sizeof(int));
	}
	return max_prob;
}

//static int WSI=FLOAT2FIX_ANY(0.1,shift);


int wtk_kwake_check_bg_fix(wtk_kwake_t *wake,int s,int e)
{
	wtk_kwake_post_cfg_t *post=wake->xpost;
	wtk_robin_t *rb=wake->rb_win;
	short *pf;
	int i;
	int t=post->fix->bkg_thresh;
	int t2=post->fix->sil_thresh;
	int win=post->bkg_win;
	int cnt=0;
	int id,id2;
	int shift=wake->cfg->shift;
	int min_bg=0,f;
	int sil_cnt=0;

	min_bg=1<<shift;
	//wtk_debug("======== bg[%d,%d]\n",s,e);
	id=wake->cfg->bkg_idx;
	id2=wake->cfg->sil_idx;
	for(i=s;i<=e;++i)
	{
		pf=(short*)wtk_robin_at(rb,i);
		if(pf[id]<min_bg)
		{
			min_bg=pf[id];
		}
		f=pf[id]+pf[id2];
		//wtk_debug("v[%d/%d]=%f/%f/%d [%d,%d] [%d,%d] echoing=%d\n",i+wake->poped,i,f,t,cnt,s,e,s+wake->poped,e+wake->poped,wake->echoing);
		if(f>=t)
		{
			++cnt;
			if(cnt>win)
			{
#ifdef DEBUG_X
				wtk_debug("win=%d/%d\n",cnt,win);
#endif
				return 0;
			}
		}else
		{
			cnt=0;
		}
		if(t2>0)
		{
			if(pf[id2]>t2)
			{
				++sil_cnt;
				if(sil_cnt>post->sil_win)
				{
#ifdef DEBUG_X
				wtk_debug("win=%d/%d\n",sil_cnt,post->sil_win);
#endif
					return 0;
				}
			}else
			{
				sil_cnt=0;
			}
		}
	}
	//wtk_debug("[%d,%d] min_bg=%f/%f sum=%f\n",s,e,min_bg,post->min_bkg,sum/(e-s));
	if(min_bg>post->fix->min_bkg)
	{
#ifdef DEBUG_X
		wtk_debug("min bg=%f/%f\n",FIX2FLOAT_ANY(min_bg,shift),FIX2FLOAT_ANY(post->min_bkg,shift));
#endif
		return 0;
	}
	return 1;
}

int wtk_kwake_check_post_fix(wtk_kwake_t *wake,wtk_kwake_post_cfg_t *post)
{
	//wtk_kwake_post_cfg_t *post=wake->post;
	wtk_robin_t *rb=wake->rb_win;
	wtk_kwake_node_t *node;
	int prob,t;
	int i,j;
	short *pf;
	int f;
	int id;
	int ret,win;
	int win_s;
	int shift=wake->cfg->shift;
	short wsi=wake->wsi;
	if(wake->fix_prob<post->fix->prob)
	{
#ifdef DEBUG_X
		wtk_debug("prob=%f\n",FIX2FLOAT_ANY(wake->fix_prob,shift));
#endif
		return 0;
	}
#ifdef DEBUG_X
	//wtk_kwake_print_fix_win(wake);
#endif
	prob=0;
	for(i=0;i<post->nnet;++i)
	{
		pf=(short*)wtk_robin_at(rb,wake->wake_slot[i]);
		f=pf[post->network[i].id];
		if(f>prob)
		{
			prob=f;
		}
	}
	//wtk_debug("prob=%f/%f\n",prob,post->max_phn_prob);
	if(prob<post->fix->max_phn_prob)
	{
#ifdef DEBUG_X
		wtk_debug("max phn failed=%f/%f\n",FIX2FLOAT_ANY(prob,shift),FIX2FLOAT_ANY(post->max_phn_prob,shift));
#endif
		ret=0;
		goto end;
	}
	id=post->network->id;
	win_s=wake->wake_slot[0];
	for(i=win_s;i>=0;--i)
	{
		pf=(short*)wtk_robin_at(rb,i);
		//wtk_debug("v[%d]=%f\n",i,pf[id]);
		if(pf[id]<wsi)
		{
			break;
		}
		win_s=i;
	}
	if(post->nnet==4 && post->fix->half_scale1>0 && post->fix->half_scale2>0)
	{
		pf=(short*)wtk_robin_at(rb,wake->wake_slot[0]);
		f=pf[post->network[0].id];
		pf=(short*)wtk_robin_at(rb,wake->wake_slot[1]);
		f+=pf[post->network[1].id];
		pf=(short*)wtk_robin_at(rb,wake->wake_slot[2]);
		t=pf[post->network[2].id];
		pf=(short*)wtk_robin_at(rb,wake->wake_slot[3]);
		t+=pf[post->network[3].id];
		//wtk_debug("half=%f/%f\n",FIX2FLOAT_ANY(f,shift),FIX2FLOAT_ANY(t,shift));
		//wtk_debug("f=%f t=%f ratio=%f\n",f,t,f/t);
		t=(f<<wake->cfg->shift)/t;
		//wtk_debug("half failed t=%f\n",FIX2FLOAT_ANY(t,shift));
		if(t>post->fix->half_scale1 || t<post->fix->half_scale2)
		{
#ifdef DEBUG_X
			wtk_debug("half failed t=%f\n",FIX2FLOAT_ANY(t,shift));
#endif
			ret=0;
			goto end;
		}
	}
	//wtk_debug("win_s=%d/%d\n",win_s,wake->wake_slot[0]);
	//win=wake->wake_slot[wake->post->nnet-1]-wake->wake_slot[0];
	win=wake->wake_slot[wake->post->nnet-1]-win_s;
	for(i=0;i<post->nnet;++i)
	{
		node=post->network+i;
		id=node->id;
		pf=(short*)wtk_robin_at(rb,wake->wake_slot[i]);
		f=pf[id];
		//wtk_debug("v[%d]=%f/%f\n",i,node->min_mid_ratio,node->min_mid_ratio_triger);
		if(node->fix->min_mid_ratio>0 && f<node->fix->min_mid_ratio_triger)
		{
			j=i-1;
			if(i<0)
			{
				j=post->nnet-1;
			}
			pf=(short*)wtk_robin_at(rb,wake->wake_slot[j]);
			t=pf[post->network[j].id];
			j=i+1;
			if(j>post->nnet-1)
			{
				j=0;
			}
			pf=(short*)wtk_robin_at(rb,wake->wake_slot[j]);
			t+=pf[post->network[j].id];
			//wtk_debug("mid[%d]=%f/%f/%f\n",i,t/2,f,(t/2)*node->min_mid_ratio);
			if(((t*node->fix->min_mid_ratio)>>(shift+1))>f)
			{
#ifdef DEBUG_X
				wtk_debug("min mid ratio v[%d]=%f/%f failed \n",i,FIX2FLOAT_ANY(t/2,shift),FIX2FLOAT_ANY(f,shift));
#endif
				ret=0;
				goto end;
			}
		}
		if(node->max_other_id>=0 && f<node->fix->max_other_id_triger && pf[node->max_other_id]>node->fix->max_other_id_triger_thresh)
		{
			//wtk_debug("f=%f/%f\n",f,pf[node->max_other_id]);
#ifdef DEBUG_X
			wtk_debug("max other failed\n");
#endif
			ret=0;
			goto end;
		}
		if(node->fix->max_prev_triger>0 || node->fix->max_prev_ratio_triger>0)
		{
			if(node->max_prev_node>=0)
			{
				pf=(short*)wtk_robin_at(rb,wake->wake_slot[node->max_prev_node]);
				prob=pf[post->network[node->max_prev_node].id];
			}else
			{
				pf=(short*)wtk_robin_at(rb,wake->wake_slot[i-1]);
				prob=pf[node[-1].id];
			}
			//wtk_debug("v[%d]=%f/%f/%f\n",i,prob,f,prob*node->max_prev_ratio);
			if(node->fix->max_prev_triger>0 && prob>node->fix->max_prev_triger && f<node->fix->max_prev_triger_thresh)
			{
#ifdef DEBUG_X
			wtk_debug("max prev tirger failed\n");
#endif
				ret=0;
				goto end;
			}
			if(node->fix->max_prev_ratio_triger>0 && prob>node->fix->max_prev_ratio_triger && f<((prob*node->fix->max_prev_ratio)>>shift))
			{
#ifdef DEBUG_X
				wtk_debug("max prev ratio failed v[%d]:%f/%f\n",i,FIX2FLOAT_ANY(f,shift),FIX2FLOAT_ANY(prob,shift));
#endif
				ret=0;
				goto end;
			}
		}
		if(node->fix->max_next_triger>0 || node->fix->max_next_ratio_triger>0)
		{
			if(node->max_next_node>=0)
			{
				pf=(short*)wtk_robin_at(rb,wake->wake_slot[node->max_next_node]);
				prob=pf[post->network[node->max_next_node].id];
			}else
			{
				pf=(short*)wtk_robin_at(rb,wake->wake_slot[i+1]);
				prob=pf[node[1].id];
			}
			//wtk_kwake_print_win(wake);
			//wtk_debug("f[%d]=%f/%f/%f\n",i,f,prob,prob*node->max_next_ratio);
			if(node->fix->max_next_triger>0 && prob>node->fix->max_next_triger && f<node->fix->max_next_triger_thresh)
			{
#ifdef DEBUG_X
			wtk_debug("maxt next triger failed\n");
#endif
				ret=0;
				goto end;
			}
			if(node->fix->max_next_ratio_triger>0 && prob>node->fix->max_next_ratio_triger && f<((prob*node->fix->max_next_ratio)>>shift))
			{
#ifdef DEBUG_X
			wtk_debug("maxt next ratio v[%d]=%f/%f failed\n",i,FIX2FLOAT_ANY(f,shift),FIX2FLOAT_ANY(prob,shift));
#endif
				ret=0;
				goto end;
			}
		}
		if(node->wrd_depth>0)
		{
			if(node->fix->wrd_prob>0)
			{
				prob=f;
				for(j=1;j<node->wrd_depth;++j)
				{
					pf=(short*)wtk_robin_at(rb,wake->wake_slot[i-j]);
					prob=(prob*pf[node[-j].id])>>shift;
				}
				if(prob<node->fix->wrd_prob)
				{
#ifdef DEBUG_X
					wtk_debug("wrd[%d]=%f/%f prob triger failed\n",i,FIX2FLOAT_ANY(prob,shift),FIX2FLOAT_ANY(node->wrd_prob,shift));
#endif
					ret=0;
					goto end;
				}
			}
			if(node->fix->wrd_min_phn_prob>0)
			{
				prob=f;
				for(j=1;j<node->wrd_depth;++j)
				{
					//wtk_debug("v[%d/%d]=%d slot=%d/%d\n",i,j,i-j,wake->wake_slot[i-j],rb->used);
					pf=(short*)wtk_robin_at(rb,wake->wake_slot[i-j]);
					if(prob<pf[node[-j].id])
					{
						prob=pf[node[-j].id];
					}
				}
				if(prob<node->fix->wrd_min_phn_prob)
				{
#ifdef DEBUG_X
					wtk_debug("wrd[%d]=%f/%f min phn prob failed\n",i,FIX2FLOAT_ANY(prob,shift),
							FIX2FLOAT_ANY(node->fix->wrd_min_phn_prob,shift));
#endif
					ret=0;
					goto end;
				}
			}
		}
		if(node->char_depth>0)
		{
			int xf;

			prob=f;
			xf=f;
			for(j=1;j<node->char_depth;++j)
			{
				//wtk_debug("v[%d/%d]=%d slot=%d/%d\n",i,j,i-j,wake->wake_slot[i-j],rb->used);
				pf=(short*)wtk_robin_at(rb,wake->wake_slot[i-j]);
				t=pf[node[-j].id];
				xf=(xf*t)>>shift;
				if(prob<t)
				{
					prob=t;
				}
			}
			//wtk_debug("v[%d]=%f/%f\n",i,prob,node->char_min_phn_prob);
			if(node->fix->char_prob>0 && xf<node->fix->char_prob)
			{
#ifdef DEBUG_X
			wtk_debug("char failed\n");
#endif
				ret=0;
				goto end;
			}
			if(node->fix->char_min_phn_prob>0 && prob<node->fix->char_min_phn_prob)
			{
#ifdef DEBUG_X
				wtk_debug("wrd[%d]=%f/%f min phn prob failed\n",i,FIX2FLOAT_ANY(prob,shift),FIX2FLOAT_ANY(node->char_min_phn_prob,shift));
#endif
				ret=0;
				goto end;
			}
		}

		if(node->fix->max_other_triger>0 && node->fix->max_other_triger_thresh>0 && f<node->fix->max_other_triger_thresh)
		{
			pf=(short*)wtk_robin_at(rb,wake->wake_slot[i]);
			prob=0;
			for(j=0;j<post->nid;++j)
			{
				if(post->id[j]==id)
				{
					continue;
				}
				prob+=pf[post->id[j]];
			}
			if(prob>node->fix->max_other_triger)
			{
#ifdef DEBUG_X
			wtk_debug("max other failed\n");
#endif
				ret=0;
				goto end;
			}
		}
		if(node->fix->speech_dur_thresh>0 && f>node->fix->speech_dur_thresh)
		{
			int tot;

			if(node->speech_dur_win>0)
			{
				tot=0;
				for(j=wake->wake_slot[i];j>=0;--j)
				{
					pf=(short*)wtk_robin_at(rb,j);
					///wtk_debug("v[%d/%d]=%f\n",wake->poped+j,j,pf[id]);
					if(pf[id]<node->fix->speech_dur_min || pf[id]>f)
					{
						break;
					}else if(pf[id]>node->fix->speech_dur_thresh)
					{
						tot=0;
					}else
					{
						++tot;
					}
				}
				if(tot<node->speech_dur_win)
				{
					ret=0;
					goto end;
				}
			}

			if(node->speech_dur_win>0)
			{
				tot=0;
				for(j=wake->wake_slot[i]+1;j<rb->used;++j)
				{
					pf=(short*)wtk_robin_at(rb,j);
					///wtk_debug("v[%d/%d]=%f\n",wake->poped+j,j,pf[id]);
					if(pf[id]<node->fix->speech_dur_min || pf[id]>f)
					{
						break;
					}else if(pf[id]>node->fix->speech_dur_thresh)
					{
						tot=0;
					}else
					{
						++tot;
					}
				}
				if(tot<node->speech_dur_win)
				{
					ret=0;
					goto end;
				}
			}
		}
		if(node->fix->speech_thresh>0)
		{
			int tot=0;
			int n;

			if(i==0)
			{
				n=0;
			}else
			{
				n=wake->wake_slot[i-1];
			}
			for(j=wake->wake_slot[i];j>=n;--j)
			{
				pf=(short*)wtk_robin_at(rb,j);
				///wtk_debug("v[%d/%d]=%f\n",wake->poped+j,j,pf[id]);
				if(pf[id]<node->fix->speech_thresh)
				{
					break;
				}
				++tot;
			}
			if(i==post->nnet-1)
			{
				n=rb->used;
			}else
			{
				n=wake->wake_slot[i+1];
			}
			for(j=wake->wake_slot[i]+1;j<n;++j)
			{
				pf=(short*)wtk_robin_at(rb,j);
				//wtk_debug("v[%d/%d]=%f\n",wake->poped+j,j,pf[id]);
				if(pf[id]<node->fix->speech_thresh)
				{
					break;
				}
				++tot;
			}
//			wtk_debug("v[%d/%d]=%d/%d thresh=%f ratio=%f/%f [%d,%d]\n",wake->poped+i,i,tot,win,node->speech_thresh,tot*1.0/win,
//					node->speech_ratio,wake->wake_slot[0]+wake->poped,wake->wake_slot[wake->xpost->nnet-1]+wake->poped);
			//wtk_debug("v[%d]:%d/%d\n",i,tot,node->speech_win);
			if(node->speech_win>0 && tot>node->speech_win)
			{
#ifdef DEBUG_X
				wtk_debug("v[%d/%d]: %d/%d speech win failed\n",i,id,tot,node->speech_win);
#endif
				ret=0;
				goto end;
			}
			if(node->speech_min_win>0 && tot<node->speech_min_win)
			{
#ifdef DEBUG_X
			wtk_debug("speec min win failed %d/%d\n",tot,node->speech_min_win);
#endif
				ret=0;
				goto end;
			}
			if(node->fix->speech_ratio>0 && (tot<<shift)/win>node->fix->speech_ratio)
			{
#ifdef DEBUG_X
				wtk_debug("speech[%d] ratio failed tot=%d/%d %f\n",i,tot,win,tot*1.0/win);
#endif
				ret=0;
				goto end;
			}
		}
	}
	if(post->fix->wake_s_thresh>0)
	{
		node=post->network;
		id=node->id;
		j=wake->wake_slot[0];
		for(i=wake->wake_slot[0]-1;i>=0;--i)
		{
			pf=(short*)wtk_robin_at(rb,i);
			if(pf[id]<post->fix->wake_s_thresh)
			{
				//wake->wake_slot[0]=i+1;
				break;
			}else
			{
				j=i;
			}
		}
		//wake->wake_slot[0]=j;
		//wtk_debug("v=[%d,%d] %d\n",wake->wake_slot[0],wake->wake_slot[post->nnet-1],wake->wake_slot[post->nnet-1]-wake->wake_slot[0]);
		if(wake->wake_slot[post->nnet-1]-j<post->wake_min_win)
		{
#ifdef DEBUG_X
			wtk_debug("wake min failed[%d/%d]\n",wake->wake_slot[post->nnet-1]-j,post->wake_min_win);
#endif
			ret=0;
			goto end;
		}
	}
	if(post->fix->bkg_thresh>0)
	{
		i=wtk_kwake_check_bg_fix(wake,wake->wake_slot[0],wake->wake_slot[post->nnet-1]);
		if(i==0)
		{
#ifdef DEBUG_X
			wtk_debug("bkg failed\n");
#endif
			ret=0;
			goto end;
		}
	}
	ret=1;
end:
	return ret;
}

int wtk_kwake_check_fix(wtk_kwake_t *wake)
{
	//wtk_kwake_post_cfg_t *post=wake->post;
	wtk_robin_t *rb=wake->rb_win;
	int prob;
	int i;
	int ret;
	int shift=wake->cfg->shift;

#ifdef DEBUG_X
#ifdef DEBUG_NX
	if(wake->feat_idx<DEBUG_NX)
	{
		return 0;
	}
#endif
#endif
	//wtk_kwake_print_win(wake);
	//if((wake->cfg->use_triger==0 || wake->idled==0) && wake->cfg->use_quick && wake->max_phn_prob>wake->cfg->quick_max_phn_prob)
	wake->xpost=wake->post;
	prob=wtk_kwake_check_inst_fix(wake,1<<shift,0,1,0,wake->wake_slot);
	if(prob<=0)
	{
#ifdef DEBUG_X
		wtk_debug("prob=%f failed quiet=%d\n",FIX2FLOAT_ANY(prob,shift),wake->post==wake->post_quiet);
#endif
		return 0;
	}
	if(wake->xpost->use_rf) 
	{
		wake->rf->thresh = wake->post->rf_cfg.rf_thresh;
        return wtk_kwake_check_post_rf_fix(wake->rf,wake->rb_win,wake->wake_slot,wake->cfg->post.nnet,wake->cfg->shift);
    }
	wake->fix_prob=prob;
	//wtk_debug("prob=%d/%f/%f\n",prob,FIX2FLOAT_ANY(prob,shift),pow(FIX2FLOAT_ANY(prob,shift),1.0/wake->post->nnet));
#ifdef DEBUG_X
	wtk_debug("prob=%d/%f/%f\n",prob,FIX2FLOAT_ANY(prob,shift),pow(FIX2FLOAT_ANY(prob,shift),1.0/wake->post->nnet));
	wtk_kwake_print_fix_win(wake);
#endif
	ret=wtk_kwake_check_post_fix(wake,wake->xpost);
	//wtk_debug("ret=%d triger=%d\n",ret,wake->post==&(wake->cfg->triger));
	if(ret==0)
	{
		if(wake->idled && wake->cfg->use_triger && wake->cfg->use_full_triger==0)
		{
			ret=wtk_kwake_check_post_fix(wake,&(wake->cfg->post));
//			wtk_debug("=========== xfound %f idled=%d %d/%d ret=%d ==================\n",wake->idle_pos*1.0/8000,wake->idled,wake->cfg->use_triger,
//					wake->cfg->use_full_triger,ret);
			//wtk_debug("use post ret=%d\n",ret);
			if(ret==0){goto end;}
			//wtk_debug("=========== xfound %f ==================\n",wake->idle_pos*1.0/8000);
			ret=wake->idle_pos-wake->hint_pos;
			//wtk_debug("ret=%d hint=%d pos=%d/%d\n",ret,wake->hint_pos,wake->poped+wake->wake_slot[wake->post->nnet-1],wake->feat_idx);
			if((wake->hint_pos>=0 && ret>(wake->cfg->parm.parm.rate/2) &&  ret<3*wake->cfg->parm.parm.rate))
			{
				ret=1;
			}else
			{
				wake->hint_pos=wake->idle_pos;
				ret=0;
				goto end;
			}
		}else
		{
			goto end;
		}
	}
	//wtk_debug("used=%d/%d\n",rb->used,wake->wake_slot[wake->xpost->nnet-1]);
	if((rb->used-wake->wake_slot[wake->xpost->nnet-1])>20)
	{
		i=rb->used/2;
		if(i>0)
		{
			wake->poped+=i;
			wtk_robin_pop2(rb,i);
		}
		ret=0;
	}else
	{
		ret=1;
	}
end:
#ifdef DEBUG_X
		wtk_debug("check ret=%d\n",ret);
#endif
	return ret;
}


void wtk_kwake_print_feat_fix(wtk_kwake_t *wake,wtk_kfeat_t *feat)
{
	wtk_kwake_post_cfg_t *post=wake->post;
	int i,id;
	int v[16]={0};
	short *sv=(short*)(feat->v);
	int shift=wake->cfg->shift;

	//wtk_debug("nnet=%d\n",post->nnet);
	printf("v[%d]: %.3f/%.3f",feat->index,FIX2FLOAT_ANY(sv[wake->cfg->sil_idx],shift),FIX2FLOAT_ANY(sv[wake->cfg->bkg_idx],shift));
	for(i=0;i<post->nnet;++i)
	{
		id=post->network[i].id;
		//wtk_debug("id=%d\n",id);
		if(v[id]==0)
		{
			printf("/%.3f",FIX2FLOAT_ANY(sv[id],shift));
			v[id]=1;
		}
	}
	printf("\n");
}
void wtk_kwake_on_fix_feat(wtk_kwake_t *wake,wtk_kfeat_t *feat)
{
//#define DEBUG_T
	wtk_kwake_cfg_t *cfg=wake->cfg;
	wtk_robin_t *rb=wake->rb_smooth;
	int nvec=wake->nvec;
	short *pf;
	int i,b;
	short *smooth=wake->fix_smooth;
	short *input=wake->fix_feat;
	int ret;
	wtk_kwake_node_t *node;
	short *fv=(short*)(feat->v);

	//wtk_fix_print(fv,nvec,shift);
	if(wake->cfg->debug)
	{
		wtk_kwake_print_feat_fix(wake,feat);
	}
	if(wake->waked){return;}
	b=rb->used==rb->nslot;
	//wtk_debug("%d/%d/%d\n",b,rbin->used,rbin->nslot);
	pf=(short*)wtk_robin_next(rb);
	if(b)
	{
		for(i=0;i<nvec;++i)
		{
			smooth[i]-=pf[i];
		}
	}
	for(i=0;i<nvec;++i)
	{
		smooth[i]+=pf[i]=fv[i];
		//wtk_debug("v[%d]=%f/%f/%f/%f\n",f->index,vec->p[0],vec->p[1],vec->p[2],vec->p[3]);
	}
	if((rb->used-wake->smooth_idx-1)<cfg->smooth_right)
	{
		return;
	}
	for(i=0;i<nvec;++i)
	{
		input[i]=smooth[i]/rb->used;
	}
	++wake->feat_idx;
#ifdef DEBUG_X
#ifdef	DEBUG_NX
	if(wake->feat_idx>DEBUG_NX)
	{
		exit(0);
	}
#endif
#endif
	rb=wake->rb_win;
	node=wake->post->network;
	if(rb->used>=rb->nslot)
	{
		wtk_robin_pop(rb);
		++wake->poped;
		//删除前面低频
		for(i=0;i<rb->used;++i)
		{
			pf=(short*)wtk_robin_at(rb,i);
			//wtk_debug("v[%d]=%f/%f/%f/%f %f\n",i,pf[0],pf[1],pf[2],pf[3],node->cur_thresh);
			if(pf[node->id]>=node->fix->prob)
			{
				break;
			}
		}
		//wtk_debug("poped=%d\n",i);
		if(i>0)
		{
			wake->poped+=i;
			wtk_robin_pop2(rb,i);
		}
	}
    //wtk_debug("---------------> %p \n",wake->post->network);
	if(rb->used==0)
	{
		if(input[node->id]<node->fix->prob)
		{
			++wake->poped;
			return;
		}
	}
	//wtk_debug("fist=%d secnod=%d t=%d feat=%d\n",wake->first_n,wake->second_n,wake->third_n,feat->index);
	//wtk_debug("%d/%f\n",rb->used,input[wake->post->network->id]);
	pf=(short*)wtk_robin_next(rb);
	memcpy(pf,input,nvec*sizeof(short));
	//wtk_debug("=============yes %d/%d =============\n",rb->used,cfg->min_win);
	if(rb->used<cfg->min_win)
	{
		return;
	}else if(cfg->check_step>1 && rb->used<rb->nslot)
	{
#ifdef DEBUG_X
#else
		++wake->check_step;
		if(wake->check_step>cfg->check_step)
		{
			wake->check_step=0;
		}else
		{
			//wtk_debug("check=%d\n",wake->check_step);
			return;
		}
#endif
	}
	//wtk_debug("check=%d\n",cfg->check_step);
	ret=wtk_kwake_quick_check_fix(wake);
	//wtk_debug("quick[%d/%d]=%d\n",wake->feat_idx,rb->used,ret);
	if(ret==1)
	{
		//wtk_kwake_print_fix_win(wake);
		ret=wtk_kwake_check_fix(wake);
		if(ret==1)
		{
			wake->waked=1;
			wtk_debug("======== waked ===============\n");
			//wtk_kwake_print_fix_win(wake);
			//exit(0);
			return;
		}
	}
}

#include "wtk/core/wtk_wavfile.h"

int wtk_kwake_feed(wtk_kwake_t *w,short *data,int len,int is_end)
{
#ifdef USE_LOG
	static wtk_wavfile_t *log=NULL;

	{
		if(!log)
		{
			static int ki=0;
			char tmp[256];

			++ki;
			sprintf(tmp,"vad%d.wav",ki);
			log=wtk_wavfile_new(8000);
			wtk_wavfile_open(log,tmp);
			log->max_pend=0;
		}
		wtk_wavfile_write(log,(char*)data,len*2);
		if(log && (is_end||w->waked))
		{
			wtk_wavfile_close(log);
			wtk_wavfile_delete(log);
			log=NULL;
		}
	}
#endif
	w->input+=len;
	//wtk_debug("len=%d idled=%d\n",len,w->idled);
	if(w->waked){return 1;}
	if(w->cfg->use_triger && len>0)
	{
		w->idle_pos+=len;
		if(w->idle_pos>w->cfg->triger_len*2)
		{
			w->idle_pos=w->cfg->triger_len;
			w->hint_pos=-1;
		}
		if(w->idled==0 && w->idle_pos>w->cfg->triger_len)
		{
			//wtk_debug("============== retrigered ===================\n");
			//exit(0);
			w->hint_pos=-1;
			w->idle_pos=0;
			w->idled=1;
			w->post=&(w->cfg->triger);
		}
	}
	wtk_kxparm_feed(w->parm,data,len,is_end);
	if(w->cfg->use_full_triger==0 && w->cfg->use_triger && w->waked)
	{
		w->hint_pos=-1;
		w->idle_pos=0;
		w->idled=0;
		//exit(0);
		w->post=&(w->cfg->post);
	}
	return w->waked;
}

void wtk_kwake_get_wake_time2(wtk_kwake_t *w,float *fs,float *fe,float dur)
{
	//wtk_debug("id=%d dur=%f\n",w->poped+w->wake_slot[0],dur);
	*fs=(w->poped+w->wake_slot[0])*dur;
	*fe=(w->poped+w->wake_slot[w->xpost->nnet-1])*dur;
}

void wtk_kwake_get_wake_time(wtk_kwake_t *w,float *fs,float *fe)
{
	float dur;//=w->parm->cfg->parm.frame_step_ms;

	//wtk_debug("echoing=%d/%d\n",w->echoing,w->post==&(w->cfg->echo));
	if(w->parm->cfg->use_htk)
	{
		dur=w->parm->cfg->htk.frame_dur;
	}else
	{
		dur=w->parm->cfg->parm.frame_step_ms/1000;
	}

	//wtk_debug("id=%d dur=%f\n",w->poped+w->wake_slot[0],dur);
	*fs=(w->poped+w->wake_slot[0])*dur;
	*fe=(w->poped+w->wake_slot[w->xpost->nnet-1])*dur;
}

void wtk_kwake_print(wtk_kwake_t *w)
{
	if(w->cfg->use_fixpoint)
	{
		wtk_kwake_print_fix_win(w);
	}else
	{
		wtk_kwake_print_win(w);
	}
}

float wtk_kwake_get_conf(wtk_kwake_t *w)
{
	return w->prob;
}

void wtk_kwake_set_notify(wtk_kwake_t *kwake,void *ths,wtk_kwake_notify_f notify)
{
	kwake->notify=notify;
	kwake->ths=ths;
}

void wtk_kwake_get_fix_wake_time(wtk_kwake_t *w,int *fs,int *fe)
{
	wtk_int64_t step;//=w->parm->cfg->parm.frame_step_ms;
	int rate;

	//wtk_debug("echoing=%d/%d\n",w->echoing,w->post==&(w->cfg->echo));
	if(w->parm->cfg->use_htk)
	{
		step=w->parm->cfg->htk.frame_step;
		rate=wtk_fextra_cfg_get_sample_rate(&(w->parm->cfg->htk));
	}else
	{
		step=w->parm->cfg->parm.frame_step;
		rate=w->parm->cfg->parm.rate;
	}

	//wtk_debug("id=%d dur=%f\n",w->poped+w->wake_slot[0],dur);
	*fs=(step*(w->poped+w->wake_slot[0])*1000)/rate;
	*fe=(step*(w->poped+w->wake_slot[w->xpost->nnet-1])*1000)/rate;
}
