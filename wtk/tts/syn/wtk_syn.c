#include <math.h>
#include "wtk_syn.h"

static void MedianFilter(float *data, int stidx, int endidx, int windowSize);

wtk_syn_t* wtk_syn_new(wtk_syn_cfg_t *cfg)
{
	wtk_syn_t *s;

	s=(wtk_syn_t*)wtk_malloc(sizeof(wtk_syn_t));
	s->cfg=cfg;
	s->heap=wtk_heap_new(4096);
	s->glb_heap=wtk_heap_new(4096);
	s->lc=wtk_syn_lc_new(s->glb_heap,s->cfg->hmm);
	s->fixf0_item=wtk_syn_dtree_search_qs_s(s->cfg->tree,WTK_SYN_DTREE_TREE_LF0,"C-Voiced");
	if(!s->fixf0_item)
	{
		s->fixf0_item=wtk_syn_dtree_search_qs_s(s->cfg->tree,WTK_SYN_DTREE_TREE_LF0,"C-Vowel");
	}
	s->sigp=wtk_syn_sigp_new(cfg,(cfg->hmm->mcepvsize/cfg->win.mcp->cfg->fn->nslot)-1);
	s->snt_idx=0;
	s->buf=wtk_strbuf_new(256,1);
	return s;
}

int wtk_syn_bytes(wtk_syn_t* s)
{
	int bytes;

	bytes=wtk_heap_bytes(s->heap);
	bytes+=wtk_heap_bytes(s->glb_heap);
	bytes+=wtk_strbuf_bytes(s->buf);
	bytes+=wtk_syn_sigp_bytes(s->sigp);
	//wtk_debug("bytes=%f M\n",bytes*1.0/(1024*1024));

	return bytes;
}

void wtk_syn_delete(wtk_syn_t *s)
{
	wtk_strbuf_delete(s->buf);
	wtk_syn_sigp_delete(s->sigp);
	wtk_heap_delete(s->heap);
	wtk_heap_delete(s->glb_heap);
	wtk_free(s);
}

void wtk_syn_reset(wtk_syn_t *s)
{
	s->syn_cur=NULL;
	s->syn_nxt=NULL;
	wtk_syn_lc_reset(s->lc);
	wtk_heap_reset(s->heap);
	s->snt_idx=0;
}

void wtk_syn_set_volume_scale(wtk_syn_t *s,float scale)
{
	wtk_syn_sigp_set_volume_scale(s->sigp,scale);
}


void wtk_syn_fixf0(wtk_syn_t *s,wtk_syn_lc_t *q)
{
	wtk_queue_node_t *qn;
	wtk_syn_hmm_lc_t *lc,*lc_next;
	int nstate;
	int i;
	wtk_syn_qs_item_t *item;

	if(!s->fixf0_item){return;}
	item=s->fixf0_item;
	nstate=s->cfg->hmm->nstate-1;
	for(qn=q->lc_q.pop;qn && qn->next;qn=qn->next)
	{
		lc=data_offset2(qn,wtk_syn_hmm_lc_t,q_n);
		lc_next=data_offset2(qn->next,wtk_syn_hmm_lc_t,q_n);
		//wtk_debug("v[%d]=%d\n",++i,lc->voiced[nstate]);
		//wtk_debug("lc->name=%.*s lc_next->name=%.*s\n",lc->name->len, lc->name->data,lc_next->name->len,lc_next->name->data);
		if(!lc->voiced[nstate] && wtk_syn_qs_item_match(item,lc->name->data,lc->name->len))
		{
			if(lc_next->voiced[0] || wtk_syn_qs_item_match(item,lc_next->name->data,lc_next->name->len))
			{
				//wtk_debug("%d\n",lc_next->voiced[0]);
				for(i=1;i<=nstate;++i)
				{
					//wtk_debug("v[%d]=%d\n",i,lc->voiced[i]);
					if(!lc->voiced[i])
					{
						lc->voiced[i]=1;
					}
				}
			}
		}
		if(lc->voiced[nstate] && wtk_syn_qs_item_match(item,lc_next->name->data,lc_next->name->len))
		{
			//wtk_debug("[%d]\n",lc->voiced[nstate]);
			for(i=0;i<nstate;++i)
			{
				if(!lc_next->voiced[i])
				{
					//wtk_debug("v[%d]\n",i);
					lc_next->voiced[i]=1;
				}
			}
		}
	}
	return;
}


void wtk_syn_get_dur(wtk_syn_t *s,wtk_syn_lc_t *q)
{
	wtk_queue_node_t *qn;
	wtk_syn_hmm_lc_t *lc;
	float dur;
	int i=0;
	float f;
	//int ki=0;

	f=s->cfg->hmm_cfg.fperiod*1.0/s->cfg->hmm_cfg.rate;
	dur=0;
	for(qn=q->lc_q.pop;qn;qn=qn->next)
	{
		lc=data_offset2(qn,wtk_syn_hmm_lc_t,q_n);
		i+=lc->totaldur;
		dur=i*f;
		//wtk_debug("dur=%f\n",dur);
		//wtk_debug("v[%d]=%d,%f tot=%d\n",++ki,i,dur,lc->totaldur);
	}
	q->totframe=(int)dur;
}



#define INFTY   ((double) 1.0e+38)
#define INFTY2  ((double) 1.0e+19)
#define INVINF  ((double) 1.0e-38)
#define INVINF2 ((double) 1.0e-19)

float finv(float x)
{
	if (x >= INFTY2)
		return 0.0;
	if (x <= -INFTY2)
		return 0.0;
	if (x <= INVINF2 && x >= 0)
		return INFTY;
	if (x >= -INVINF2 && x < 0)
		return -INFTY;

	return (1.0 / x);
}

int wtk_syn_tot_dur(wtk_syn_lc_t *lc)
{
	wtk_syn_hmm_lc_t *lh;
	wtk_queue_node_t *qn;
	int step=0;

	for(qn=lc->lc_q.pop;qn;qn=qn->next)
	{
		lh=data_offset2(qn,wtk_syn_hmm_lc_t,q_n);
		step+=lh->totaldur;
	}
	return step;
}

void wtk_syn_towav(wtk_syn_t *s,wtk_syn_lc_t *lc,wtk_syn_pstream_t *lf0pst,
		wtk_syn_pstream_t *mceppst,wtk_syn_pstream_t *bappst,char *v)
{
	wtk_syn_sigp_t *sp;
	wtk_syn_vector_t *f0v;
	wtk_matf_t *mcp;
	wtk_matf_t *bap=NULL;
	wtk_queue_node_t *qn;
	wtk_syn_hmm_lc_t *lh,*nlh;
	int step=0;
	int lk,mk;
	int mcepframe;
	int pad;
	int tot,next,count;

	tot=lc->totframe;
	lk=0;mk=0;
	count=0;
	sp=s->sigp;
	for(qn=lc->lc_q.pop;qn;qn=qn->next)
	{
		lh=data_offset2(qn,wtk_syn_hmm_lc_t,q_n);
		step+=lh->totaldur;
		count+=lh->totaldur;
		//wtk_debug("%.*s\n",lh->phn->lab->len,lh->phn->lab->data);
		if(qn->next)
		{
			nlh=data_offset2(qn->next,wtk_syn_hmm_lc_t,q_n);
			if(!nlh->phn->syl)
			{
				continue;
			}
			if(s->cfg->stream_min_dur>0 && step<s->cfg->stream_min_dur)
			{
				continue;
			}
		}
		if(qn->next)
		{
			next=tot-count;
			if(next<s->cfg->steam_min_left_dur)
			{
				continue;
			}
		}
		//wtk_debug("%.*s\n",lh->phn->lab->len,lh->phn->lab->data);
		if(wtk_tts_xphn_is_last_in_syl(lh->phn) || !qn->next)
		{
			pad=qn->next?s->cfg->stream_pad_sil_dur:0;
			//wtk_debug("step=%d\n",step);
			f0v=wtk_syn_vector_new(step+pad,0);
			mcp=wtk_matf_new(step+pad,mceppst->dim);
			if(bappst)
			{
				bap=wtk_matf_new(step+pad,bappst->dim);
			}
			for(mcepframe=0;mcepframe<step;++mcepframe,++mk)
			{
				//wtk_debug("%d\n",mceppst->T);
				if(v[mk])
				{
					f0v->data[mcepframe]=(double) s->cfg->hmm_cfg.f0_std
							* exp(*wtk_matf_at(lf0pst->par,lk,0)) + s->cfg->hmm_cfg.f0_mean;

					++lk;
				}else
				{
					f0v->data[mcepframe]=0;
				}
				//wtk_debug("v[%d]=%d %f\n",mcepframe,v[mcepframe],f0v->data[mcepframe]);
				memcpy(wtk_matf_row(mcp,mcepframe),wtk_matf_row(mceppst->par,mk),mceppst->dim*sizeof(float));
				if(bappst)
				{
					memcpy(wtk_matf_row(bap,mcepframe),wtk_matf_row(bappst->par,mk),bappst->dim*sizeof(float));
				}
				//exit(0);
			}
			wtk_syn_sigp_reset(sp);
			wtk_syn_sigp_process(sp,f0v,mcp,bap,s->cfg->hmm_cfg.sigp);
			//wtk_heap_free(heap,f0v);
			wtk_syn_vector_delete(f0v);
			wtk_matrix_delete(mcp);
			if(bap)
			{
				wtk_matrix_delete(bap);
			}
			step=0;
		}
	}
}

void wtk_syn_towav3(wtk_syn_t *s,wtk_syn_lc_t *lc,wtk_syn_pstream_t *lf0pst,
		wtk_syn_pstream_t *mceppst,wtk_syn_pstream_t *bappst,char *v)
{
	wtk_syn_sigp_t *sp;
	wtk_syn_vector_t *f0v;
	wtk_matf_t *mcp;
	wtk_matf_t *bap=NULL;
	wtk_queue_node_t *qn;
	wtk_syn_hmm_lc_t *lh,*nlh;
	int step=0;
	int lk,mk;
	int mcepframe;
	int tot,next,count;
	wtk_syn_vector_t fx;
	wtk_matf_t mx;
	wtk_matf_t bx;
	int vx;

	lk=0;mk=0;
	sp=s->sigp;
	step=lc->totframe;
	f0v=wtk_syn_vector_new(step,0);
	mcp=wtk_matf_new(step,mceppst->dim);
	if(bappst)
	{
		bap=wtk_matf_new(step,bappst->dim);
	}
	for(mcepframe=0;mcepframe<step;++mcepframe,++mk)
	{
		//wtk_debug("%d\n",mceppst->T);
		if(v[mk])
		{
			f0v->data[mcepframe]=(double) s->cfg->hmm_cfg.f0_std
					* exp(*wtk_matf_at(lf0pst->par,lk,0)) + s->cfg->hmm_cfg.f0_mean;

			++lk;
		}else
		{
			f0v->data[mcepframe]=0;
		}
		//wtk_debug("v[%d]=%d %f\n",mcepframe,v[mcepframe],f0v->data[mcepframe]);
		memcpy(wtk_matf_row(mcp,mcepframe),wtk_matf_row(mceppst->par,mk),mceppst->dim*sizeof(float));
		if(bappst)
		{
			memcpy(wtk_matf_row(bap,mcepframe),wtk_matf_row(bappst->par,mk),bappst->dim*sizeof(float));
		}
		//exit(0);
	}
	//wtk_debug("===================> [%d]",sp->cfg->f0_norm_win);
	if(sp->cfg->f0_norm_win>0)
	{
		//wtk_debug("step=%d\n",step);
		MedianFilter(f0v->data,0,step-1,sp->cfg->f0_norm_win);
	}
	step=0;
	count=0;
	tot=lc->totframe;
	s->syn_cur=NULL;
	for(qn=lc->lc_q.pop;qn;qn=qn->next)
	{
		if(!s->syn_cur)
		{
			s->syn_cur=qn;
		}
		lh=data_offset2(qn,wtk_syn_hmm_lc_t,q_n);
		step+=lh->totaldur;
		count+=lh->totaldur;
//		if(lh->phn->syl)
//		{
//			wtk_debug("[%.*s] %d/%d\n",lh->phn->syl->syl->v->len,lh->phn->syl->syl->v->data,lh->totaldur,count);
//		}else
//		{
//			wtk_debug("sil: %d/%d\n",lh->totaldur,count);
//		}
		//wtk_debug("%.*s\n",lh->phn->lab->len,lh->phn->lab->data);
		//wtk_debug("step=%d count=%d\n",step,count);
		if(qn->next)
		{
			nlh=data_offset2(qn->next,wtk_syn_hmm_lc_t,q_n);
			if(!nlh->phn->syl)
			{
				continue;
			}
			if(s->cfg->stream_min_dur>0 && step<s->cfg->stream_min_dur)
			{
				continue;
			}
		}
		if(qn->next)
		{
			next=tot-count;
			if(next<s->cfg->steam_min_left_dur)
			{
				continue;
			}
		}
		//wtk_debug("%.*s\n",lh->phn->lab->len,lh->phn->lab->data,lh->name->len,lh->name->data,lh->totaldur);
		//wtk_debug("%.*s:%d %d\n",lh->phn->phn->len,lh->phn->phn->data,lh->totaldur,count);//lh->phn->syl->syl->v->len,lh->phn->syl->syl->v->data);

		if(wtk_tts_xphn_is_last_in_syl(lh->phn) || !qn->next)
		{
			s->syn_nxt=qn;
			vx=count-step;
			//wtk_debug("raise step=%d\n",step);
			fx.len=step;
			fx.data=f0v->data+vx;
			fx.imag=f0v->imag+vx;
			mx.row=step;
			mx.col=mcp->col;
			mx.p=mcp->p+mcp->col*vx;
			//wtk_debug("step=%d,count=%d %d/%d tot=%d\n",step,count,count+step,f0v->len,tot);
			wtk_syn_sigp_reset(sp);
			if(!qn->next)
			{
				s->is_snt_end=1;
			}
			if(bap)
			{
				bx.row=step;
				bx.col=bap->col;
				bx.p=bap->p+bap->col*vx;
				wtk_syn_sigp_process(sp,&fx,&mx,&bx,s->cfg->hmm_cfg.sigp);
			}else
			{
				wtk_syn_sigp_process(sp,&fx,&mx,0,s->cfg->hmm_cfg.sigp);
			}
			s->syn_cur=NULL;
			//wtk_syn_sigp_process(sp,f0v,mcp,bap,s->cfg->hmm_cfg.sigp);
			step=0;
		}
	}
	//wtk_debug("tot=%d\n",tot);
//	wtk_syn_sigp_reset(sp);
//	wtk_syn_sigp_process(sp,f0v,mcp,bap,s->cfg->hmm_cfg.sigp);
	wtk_syn_vector_delete(f0v);
	wtk_matrix_delete(mcp);
	if(bap)
	{
		wtk_matrix_delete(bap);
	}
	//exit(0);
}

typedef struct _dLink2{
	float value;
	int idx;   // idx of the point in the original data array, also used to save windowSize in mHead
	struct _dLink2 *next;
}dLink2;

static dLink2 *InitialiseMedianFilter(dLink2 *head, float *data, int stidx)
{

	int i, size;
	dLink2 *ptrTmp, *ptr;

	size = head->idx;

	/* initialise */
	for(i=stidx;i<stidx+size;i++){
		ptrTmp = (dLink2 *)malloc(sizeof(dLink2));
		ptrTmp->value = data[i];
		ptrTmp->idx = i;
		/* find the position to insert, ascending order */
		ptr = head;
		while(ptr->next && ptr->next->value < ptrTmp->value)
			ptr = ptr->next;
		ptrTmp->next = ptr->next;
		ptr->next = ptrTmp;
	}

	/* find median */
	size = (int)(size/2);
	ptr = head->next;
	for(i=0;i<size;i++)
		ptr = ptr->next;

	return ptr;
}

static dLink2 *UpdateMedianFilter(dLink2 *head, int delIdx, int addIdx, float *data)
{

	dLink2 *ptr, *takeOut;
	int i, hfWnd;

	/* take out link with delIdx */
	ptr = head;
	while(ptr->next && ptr->next->idx != delIdx)
		ptr = ptr->next;
	if(!ptr->next){
		fprintf(stderr, "Error: idx %d not in median filter",delIdx);
		//exit(1);
		return 0;
	}
	takeOut = ptr->next;
	ptr->next = ptr->next->next;

	/* add the new data in - median filter in ascending order */
	takeOut->idx = addIdx;
	takeOut->value = data[addIdx];
	//wtk_debug("addIdx=%d\n",addIdx);
	ptr = head;
	while(ptr->next && ptr->next->value < takeOut->value)
		ptr = ptr->next;
	takeOut->next = ptr->next;
	ptr->next = takeOut;

	/* find median */
	hfWnd = (int)(head->idx/2); // head->idx is actually windowSize
	ptr = head->next;
	for(i=0;i<hfWnd;i++)
		ptr = ptr->next;

	return ptr;

}

static void FreeMedianFilter(dLink2 *head)
{
	dLink2 *ptr, *toFree;
	int size, i;

	ptr = head;
	size = head->idx;
	i = 0;
	while(ptr){
		toFree = ptr;
		ptr = ptr->next;
		free(toFree);
		i++;
	}

	if(i!=size+1){
		fprintf(stderr, "Error: free %d elements, but size+head is %d\n",i,size+1);
	}

	return;
}

/* calulate median value for points from stidx to endidx with windowSize neighbouring points */
static void MedianFilter(float *data, int stidx, int endidx, int windowSize)
{
	float *tmp;
	dLink2 *mHead, *mMedian;
	int hfWnd, i, j;

    if(endidx-stidx<5) return;
	if(windowSize<2 || windowSize>100){
		fprintf(stderr, "Wrong window size %d in segment %d to %d\n",windowSize,stidx,endidx);
		exit(1);
	}
	if(windowSize>endidx-stidx+1) return;
	i = windowSize%2;
	if(i==0){
		fprintf(stderr, "Wrong median filter window size %d. Must be odd!\n",windowSize);
		exit(1);
	}
	hfWnd = (int)(windowSize/2);

	/* get tmp array, only smooth those points away from boundaries */
	/*                                                             */
	/* |--- hfWnd ---|---- data to be filtered ----|--- hfWnd ---| */
	/*                                                             */
	tmp = (float *)malloc((endidx-stidx+2-windowSize)*sizeof(float));
	mHead = (dLink2 *)malloc(sizeof(dLink2));
	mHead->idx = windowSize;
	mHead->value = -1000.0;
	mHead->next = NULL;

	/* do median smoothing */
	mMedian = InitialiseMedianFilter(mHead,data,stidx);
	tmp[0] = mMedian->value;
	for(i=stidx+hfWnd+1;i<endidx-hfWnd+1;i++){
		mMedian = UpdateMedianFilter(mHead,i-hfWnd-1,i+hfWnd,data);
		tmp[i-stidx-hfWnd] = mMedian->value;
	}

	/* copy boundary values to head and tail points and update data */
	for(i=stidx;i<=endidx;i++){
		j = i-stidx-hfWnd;
		if( j < 0 )
			data[i] = tmp[0];
		else if ( j > endidx-stidx+1-windowSize )
			data[i] = tmp[endidx-stidx+1-windowSize];
		else
            data[i] = tmp[j];
	}

	FreeMedianFilter(mHead);

	free(tmp);

	return;

}

void wtk_syn_smooth_f0()
{
}


void wtk_syn_towav2(wtk_syn_t *s,wtk_syn_lc_t *lc,wtk_syn_pstream_t *lf0pst,
		wtk_syn_pstream_t *mceppst,wtk_syn_pstream_t *bappst,char *v)
{
	wtk_syn_sigp_t *sp;
	wtk_syn_vector_t *f0v;
	wtk_matf_t *mcp;
	wtk_matf_t *bap=NULL;
	int step=0;
	int lk,mk;
	int mcepframe;

	s->syn_cur=NULL;
	lk=0;mk=0;
	sp=s->sigp;
	step=lc->totframe;
	f0v=wtk_syn_vector_new(step,0);
	mcp=wtk_matf_new(step,mceppst->dim);
	if(bappst)
	{
		bap=wtk_matf_new(step,bappst->dim);
	}
	wtk_syn_sigp_reset(sp);
	for(mcepframe=0;mcepframe<step;++mcepframe,++mk)
	{
		//wtk_debug("%d\n",mceppst->T);
		if(v[mk])
		{
			f0v->data[mcepframe]=(double) s->cfg->hmm_cfg.f0_std
					* exp(*wtk_matf_at(lf0pst->par,lk,0)) + s->cfg->hmm_cfg.f0_mean;

			++lk;
		}else
		{
			f0v->data[mcepframe]=0;
		}
		//wtk_debug("v[%d]=%d %f\n",mcepframe,v[mcepframe],f0v->data[mcepframe]);
//		printf("f0[%d]=%3.0f\n",mcepframe,f0v->data[mcepframe]);
//		float* tv=wtk_matf_row(mceppst->par,mk);
//		int tn;
//		for (tn=0; tn < mceppst->dim; ++tn)
//		{
//			printf("mcep[%d][%d]=%6.5f\n", mcepframe,tn,tv[tn]);
//		}
		memcpy(wtk_matf_row(mcp,mcepframe),wtk_matf_row(mceppst->par,mk),mceppst->dim*sizeof(float));
		if(bappst)
		{
//			float* tv=wtk_matf_row(bappst->par,mk);
//			int tn;
//			for (tn=0; tn < bappst->dim; ++tn)
//			{
//				printf("bap[%d][%d]=%6.5f\n", mcepframe,tn,tv[tn]);
//			}
			memcpy(wtk_matf_row(bap,mcepframe),wtk_matf_row(bappst->par,mk),bappst->dim*sizeof(float));
		}
		//exit(0);
	}
	if(sp->cfg->f0_norm_win>0)
	{
		//wtk_debug("step=%d\n",step);
		MedianFilter(f0v->data,0,step-1,sp->cfg->f0_norm_win);
	}
	wtk_syn_sigp_process(sp,f0v,mcp,bap,s->cfg->hmm_cfg.sigp);
	//wtk_heap_free(heap,f0v);
	wtk_syn_vector_delete(f0v);
	wtk_matrix_delete(mcp);
	if(bap)
	{
		wtk_matrix_delete(bap);
	}
	step=0;
}

//wtk_syn_pstream_t *lf0pst,*mceppst,*bappst=NULL;
void wtk_syn_pdf2speech(wtk_syn_t *s,wtk_syn_lc_t *lc)
{
	wtk_queue_node_t *qn;
	wtk_syn_hmm_t *hmm;
	wtk_syn_pstream_t *lf0pst=NULL,*mceppst=NULL,*bappst=NULL;
	wtk_syn_hmm_lc_t *lh;
	int mcepframe=0,lf0frame=0;
	int i,j,k,n;
	int lw,rw;
	char *v;
	int nobound;
	int ci;
#ifdef DEBUG_X
	double t;
#endif

	hmm=lc->hmm;
	v=(char*)wtk_heap_zalloc(s->heap,lc->totframe);
	for(qn=lc->lc_q.pop;qn;qn=qn->next)
	{
		lh=data_offset2(qn,wtk_syn_hmm_lc_t,q_n);
		for(i=0;i<hmm->nstate;++i)
		{
			//wtk_debug("v[%d]=%d lf0frame=%d dur=%d i=%d\n",++ki,lh->voiced[i],lf0frame,lh->dur[i],i);
			for(j=0;j<lh->dur[i];++j)
			{
				v[mcepframe++]=lh->voiced[i];
				if(lh->voiced[i])
				{
					++lf0frame;
				}
			}
		}
	}
	//printf("hmm->nstate=%d\n",hmm->nstate);
	//printf("lf0frame=%d\n",lf0frame);
	//printf("mcepframe=%d\n",mcepframe);
	if(NULL==lf0pst)
		lf0pst=wtk_syn_pstream_new(hmm->lf0stream,lf0frame,s->cfg->win.lf0,lc->lf0gv);
	if(NULL == mceppst)
		mceppst=wtk_syn_pstream_new(hmm->mcepvsize,mcepframe,s->cfg->win.mcp,lc->mcegv);
	/* the following remedy by dmd at 2018.02.24
	   raw: "if (lc->bapgv) " will cause bap unused
	   new: if(lc->hmm->bappdf) */
	//if(lc->hmm->bappdf)
	if(lc->hmm->cfg->bap_fn) //for support bin res, use bap_fn will fit. remedy by dm 2018.03.07
	{
		bappst=wtk_syn_pstream_new(hmm->bapvsize,mcepframe,s->cfg->win.bap,lc->bapgv);
	}
	mcepframe=0;
	lf0frame=0;
	//wtk_debug("%f\n",lf0pst->par[1][1]);
	for(ci=0,qn=lc->lc_q.pop;qn;qn=qn->next,++ci)
	{
		lh=data_offset2(qn,wtk_syn_hmm_lc_t,q_n);
		//wtk_debug("v[%d]=%p\n",ci,lh);
		for(i=0;i<hmm->nstate;++i)
		{
			for(j=0;j<lh->dur[i];++j)
			{
				for(k=1;k<=hmm->mcepvsize;++k)
				{
					*wtk_matf_at(mceppst->mseq,mcepframe,k-1)=lh->mcepmean[i+1][k];
					*wtk_matf_at(mceppst->ivseq,mcepframe,k-1)=finv(lh->mcepvariance[i+1][k]);
					//wtk_debug("j=%d v[%d][%d]=%f\n",j,i,k,lh->mcepmean[i][k]);
				}
				//if(lc->bapgv)
				//if(lc->hmm->bappdf) //same to above by dm 2018.02.24
				if(lc->hmm->cfg->bap_fn) //for support bin res, use bap_fn will fit. remedy by dm 2018.03.07
				{
					for(k=1;k<=hmm->bapvsize;++k)
					{
						*wtk_matf_at(bappst->mseq,mcepframe,k-1)=lh->bapmean[i+1][k];
						*wtk_matf_at(bappst->ivseq,mcepframe,k-1)=finv(lh->bapvariance[i+1][k]);
					}
				}
				for(k=0;k<hmm->lf0stream;++k)
				{
					lw=s->cfg->win.lf0->width[k][0];
					rw=s->cfg->win.lf0->width[k][1];
					/* check current frame is UV boundary or not */
					nobound=1;
					//wtk_debug("lw=%d rw=%d\n",lw,rw);
					for(n=lw;n<=rw;++n)
					{
						//wtk_debug("%d %d %d\n",mcepframe+n,lc->totframe,(mcepframe+n));
						if(mcepframe+n<0 || lc->totframe<=(mcepframe+n))
						{
							nobound=0;
						}else
						{
							nobound=nobound & v[mcepframe+n];
						}
					}
					//wtk_debug("nobound=%d\n",nobound);
					//wtk_debug("v[%d]: nobound=%d v=%d\n",++kx,nobound,v[mcepframe]);
					if(v[mcepframe])
					{
						//wtk_debug("i=%d k=%d lh=%p mean=%p\n",i,k,lh,lh->lf0mean);
						*wtk_matf_at(lf0pst->mseq,lf0frame,k)=lh->lf0mean[i+1][k+1];
						if(nobound || k==0)
						{
							*wtk_matf_at(lf0pst->ivseq,lf0frame,k)=finv(lh->lf0variance[i+1][k+1]);
						}else
						{
							*wtk_matf_at(lf0pst->ivseq,lf0frame,k)=0;
						}
						//exit(0);
					}
				}
				if (v[mcepframe])
				{
					++lf0frame;
				}
				++mcepframe;
				//wtk_debug("v[%d][%d/%d/%d/%d]=%d/%d\n",++kx,ci,i,j,lh->dur[i],lf0frame,mcepframe);
			}
			//exit(0);
		}
		//exit(0);
	}
	//wtk_debug("%f/%f \n",mceppst->par->p[0],mceppst->par->p[1]);
	//wtk_debug("%f/%f\n",mceppst->sm->b[1],mceppst->sm->b[2]);
	//wtk_debug("mceppst=%p\n",mceppst);
#ifdef DEBUG_X
	t=time_get_ms();
#endif
	wtk_syn_pstream_mlpg_grannw(mceppst,10000, 0.05, 1.0e-2, -1.0, 1, 1);
#ifdef DEBUG_X
	t=time_get_ms()-t;
	wtk_debug("t=%f\n",t);
#endif

#ifdef DEBUG_X
	t=time_get_ms();
#endif
	// parameter generation for bap
	if(bappst)
	{
		//wtk_debug("mceppst=%p\n",bappst);
		wtk_syn_pstream_mlpg_grannw(bappst,10000, 0.05, 1.0e-2, -1.0, 1, 1);
	}
	if(lf0frame>0)
	{
		//wtk_debug("mceppst=%p\n",mceppst);
		//wtk_debug("post=%d\n",lf0frame);
		wtk_syn_pstream_mlpg_grannw(lf0pst,10000, 0.01, 1.0e-2, -1.0, 1, 1);
	}
#ifdef DEBUG_X
	t=time_get_ms()-t;
	wtk_debug("t=%f\n",t);
#endif
//	double tx1=0, tx2=0;
//	tx1=time_get_ms();
//	wtk_debug("use stream=%d\n",s->cfg->use_stream);
	if(s->cfg->use_stream)
	{
		//wtk_syn_towav(s,lc,lf0pst,mceppst,bappst,v);
		wtk_syn_towav3(s,lc,lf0pst,mceppst,bappst,v);
	}else
	{
		wtk_syn_towav2(s,lc,lf0pst,mceppst,bappst,v);
	}
//    tx2=time_get_ms();
//    wtk_debug("process tx= %f\n",tx2-tx1);
//    wtk_debug("process lf0frame= %d\n", lf0frame);
	wtk_syn_pstream_delete(lf0pst);
	wtk_syn_pstream_delete(mceppst);
	if(bappst)
	{
		wtk_syn_pstream_delete(bappst);
	}
}

void wtk_syn_pdf2speech3(wtk_syn_t *s,wtk_syn_lc_t *lc)
{
	wtk_queue_node_t *qn;
	wtk_syn_hmm_lc_t *lh;
	wtk_syn_lc_t tlc;

	//for(qn=lc->lc_q.pop;qn;qn=qn->next)
	while(1)
	{
		qn=wtk_queue_pop(&(lc->lc_q));
		if(!qn){break;}
		lh=data_offset2(qn,wtk_syn_hmm_lc_t,q_n);
		tlc=*lc;
		tlc.totframe=lh->totaldur;
		wtk_queue_init(&(tlc.lc_q));
		wtk_queue_push(&(tlc.lc_q),&(lh->q_n));
		wtk_syn_pdf2speech(s,&(tlc));
	}
}

int wtk_syn_process_snt(wtk_syn_t *s,wtk_tts_snt_t *snt,float rho)
{
	wtk_tts_xphn_t **phns,*phn;
	int idx[2];
	int ret;
	//float rho=s->cfg->hmm_cfg.rho;
	wtk_syn_hmm_lc_t *lc;
	int i;
	float diff=0;
	wtk_syn_lc_t* l;

	s->is_snt_end=0;
	if(!snt->phns || snt->phns->nslot<=0){return 0;}
	l=s->lc;
	wtk_syn_lc_reset(l);
	phns=(wtk_tts_xphn_t**)snt->phns->slot;
	for(i=0;i<snt->phns->nslot;++i)
	{
		phn=phns[i];
		lc=wtk_syn_hmm_lc_new(s->cfg->hmm,s->heap,phn->lab->data,phn->lab->len);
		lc->phn=phn;
		//wtk_debug("v[%d]=[%.*s]\n",i,phn->lab->len,phn->lab->data);
		ret=wtk_syn_dtree_search(s->cfg->tree,WTK_SYN_DTREE_TREE_DUR,2,phn->lab->data,phn->lab->len,idx);
		if(ret!=0){goto end;}
		wtk_syn_hmm_lc_find_durpdf(lc,rho,idx,&diff);
		//wtk_syn_hmm_lc_print(lc);
		//printf("dur:%.*s %d %d\n",phn->phn->len,phn->phn->data, l->totframe, l->totframe+lc->totaldur);
		l->totframe+=lc->totaldur;
		wtk_syn_hmm_lc_set_output_pdfs(lc,phn->lab->data,phn->lab->len);
		//wtk_debug("v[%d]=lc=%p mean=%p\n",l->lc_q.length,lc,lc->lf0mean);
		wtk_queue_push(&(l->lc_q),&(lc->q_n));
		wtk_syn_lc_set_gv_pdfs(l,phn->lab->data,phn->lab->len);
	}
	//wtk_debug("total frame=%d\n", l->totframe);
	wtk_syn_fixf0(s,l);
	wtk_syn_pdf2speech(s,l);
	ret=0;
end:
	return ret;
}


int wtk_syn_process(wtk_syn_t *s,wtk_tts_lab_t *lab)
{
	wtk_tts_snt_t **snt;
	int i;
	int ret;

	snt=(wtk_tts_snt_t**)lab->snts->slot;
	//wtk_debug("n=%d\n",lab->snts->nslot);
	s->snt_idx=0;
	for(i=0;i<lab->snts->nslot;++i)
	{
		if(snt[i]->wrds->nslot<=0)
		{
			continue;
		}
		s->snt_idx=i+1;
		ret=wtk_syn_process_snt(s,snt[i],lab->speech_speed);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_syn_get_cur_snt_index(wtk_syn_t *s)
{
	return s->snt_idx;
}

#include "wtk/core/json/wtk_json.h"
wtk_string_t wtk_syn_get_cur_timeinfo(wtk_syn_t *s)
{
	wtk_string_t v;
	wtk_strbuf_t *buf=s->buf;
	wtk_queue_node_t *qn;
	wtk_syn_hmm_lc_t *lh;
	wtk_json_t *json;
	wtk_json_item_t *m,*item = NULL,*item2=NULL,*item3;
	wtk_json_item_t *x;
	wtk_tts_xsyl_t *last_syl=NULL;
	int tot=0;

	wtk_strbuf_reset(buf);
	if(!s->syn_cur){goto end;}
	json=wtk_json_new();
	x=wtk_json_new_object(json);
	wtk_json_obj_add_str2_ss(json,x,"cmd","tts");
	m=wtk_json_new_array(json);
	wtk_json_obj_add_item2_s(json,x,"tts",m);
	for(qn=s->syn_cur;;qn=qn->next)
	{
		lh=data_offset2(qn,wtk_syn_hmm_lc_t,q_n);
		if(lh->phn->syl)
		{
			//wtk_debug("[%.*s] %d/%d\n",lh->phn->syl->syl->v->len,lh->phn->syl->syl->v->data,lh->totaldur,count);
			if(!last_syl || lh->phn->syl!=last_syl)
			{
				if(last_syl)
				{
					wtk_json_obj_add_ref_number_s(json,item,"dur",tot);
				}
				last_syl=lh->phn->syl;
				item=wtk_json_new_object(json);
				wtk_json_array_add_item(json,m,item);
				wtk_json_obj_add_ref_str_s(json,item,"word",lh->phn->syl->syl->v);
				item2=wtk_json_new_array(json);
				wtk_json_obj_add_item2_s(json,item,"phn",item2);
				tot=0;
			}
			item3=wtk_json_new_object(json);
			wtk_json_array_add_item(json,item2,item3);
			wtk_json_obj_add_ref_str_s(json,item3,"phn",lh->phn->phn);
			wtk_json_obj_add_ref_number_s(json,item3,"dur",lh->totaldur);
			tot+=lh->totaldur;
		}else
		{
			if(last_syl)
			{
				wtk_json_obj_add_ref_number_s(json,item,"dur",tot);
			}
			last_syl=NULL;
			//wtk_debug("sil: %d/%d\n",lh->totaldur,count);
			item=wtk_json_new_object(json);
			wtk_json_obj_add_ref_str_s(json,item,"word",lh->phn->phn);
			wtk_json_obj_add_ref_number_s(json,item,"dur",lh->totaldur);
			item2=wtk_json_new_array(json);
			wtk_json_obj_add_item2_s(json,item,"phn",item2);
			item3=wtk_json_new_object(json);
			wtk_json_array_add_item(json,item2,item3);
			wtk_json_obj_add_ref_str_s(json,item3,"phn",lh->phn->phn);
			wtk_json_obj_add_ref_number_s(json,item3,"dur",lh->totaldur);
			//void wtk_json_obj_add_str2(wtk_json_t *json,wtk_json_item_t *obj,char *key,int key_len,char *v,int v_len);
			wtk_json_array_add_item(json,m,item);
		}
		if(qn==s->syn_nxt){break;}
	}
	if(last_syl)
	{
		wtk_json_obj_add_ref_number_s(json,item,"dur",tot);
	}
	wtk_json_item_print(x,buf);
	wtk_json_delete(json);
end:
	wtk_string_set(&(v),buf->data,buf->pos);
	//wtk_debug("[%.*s]\n",v.len,v.data);
//	{
//		static int ki=0;
//
//		++ki;
//		if(ki==2)
//		{
//			exit(0);
//		}
//	}
	//exit(0);
	return v;
}


//---------------------------------------------


void wtk_syn_test(wtk_syn_t *s)
{
	char *ps[]=
	{
			"nil^nil-pau+z=oe@1-1&nil-nil/A:nil_nil_nil/B:nil_1@1_1&nil#nil$nil/C:nil-3-1/D:nil_nil/E:nil_1!1_11/F:v_2/G:15-11@0/H:5&5@5@5&5!5/I:nil+nil&nil+nil!nil!nil#nil",
			"nil^pau-z+oe=ng@1-3&0-0/A:nil_1_nil/B:nil_3@1_2&1#0$0/C:nil-4-2/D:nil_1/E:v_2!2_10/F:o_1/G:15-11@0/H:5&p@5@0&5!1/I:H+H&nil+H!nil!H#nil",
			"pau^z-oe+ng=q@2-2&1-1/A:nil_1_nil/B:nil_3@1_2&1#0$0/C:nil-4-2/D:nil_1/E:v_2!2_10/F:o_1/G:15-11@0/H:p&p@5@0&5!1/I:M+H&nil+H!nil!H#nil",
			"z^oe-ng+q=il@3-1&1-0/A:nil_1_nil/B:nil_3@1_2&1#0$0/C:nil-4-2/D:nil_1/E:v_2!2_10/F:o_1/G:15-11@0/H:p&0@5@0&5!1/I:T+H&nil+H!nil!H#nil",
			"oe^ng-q+il=ah@1-4&0-0/A:nil_3_1/B:nil_4@2_1&2#0$0/C:nil-2-5/D:nil_1/E:v_2!2_10/F:o_1/G:15-11@0/H:0&p@0@1&5!1/I:H+T&nil+H!nil!H#nil",
			"ng^q-il+ah=ng@2-3&1-1/A:nil_3_1/B:nil_4@2_1&2#0$0/C:nil-2-5/D:nil_1/E:v_2!2_10/F:o_1/G:15-11@0/H:p&p@0@1&5!1/I:M+T&nil+H!nil!H#nil",
			"q^il-ah+ng=l@3-2&1-1/A:nil_3_1/B:nil_4@2_1&2#0$0/C:nil-2-5/D:nil_1/E:v_2!2_10/F:o_1/G:15-11@0/H:p&p@0@1&5!1/I:T+T&nil+H!nil!H#nil",
			"il^ah-ng+l=ea@4-1&1-0/A:nil_3_1/B:nil_4@2_1&2#0$0/C:nil-2-5/D:nil_1/E:v_2!2_10/F:o_1/G:15-11@0/H:p&1@0@1&5!1/I:T+T&nil+H!nil!H#nil",
			"ah^ng-l+ea=vw@1-2&0-0/A:nil_4_2/B:nil_2@1_1&5#0$0/C:nil-1-2/D:v_2/E:o_1!3_9/F:o_1/G:15-11@0/H:1&p@1@1&1!1/I:H+A&nil+H!nil!H#nil",
			"ng^l-ea+vw=w@2-1&1-1/A:nil_4_2/B:nil_2@1_1&5#0$0/C:nil-1-2/D:v_2/E:o_1!3_9/F:o_1/G:15-11@0/H:p&1@1@1&1!1/I:T+A&nil+H!nil!H#nil",
			"l^ea-vw+w=ao@1-1&1-1/A:nil_2_5/B:nil_1@1_1&2#1$0/C:nil-2-3/D:o_1/E:o_1!4_8/F:o_1/G:15-11@0/H:1&1@1@1&1!1/I:A+A&nil+H!nil!H#nil",
			"ea^vw-w+ao=h@1-2&0-0/A:nil_1_2/B:nil_2@1_1&3#1$0/C:nil-2-2/D:o_1/E:o_1!5_7/F:v_2/G:15-11@0/H:1&p@1@1&1!1/I:H+A&nil+M!nil!M#nil",
			"vw^w-ao+h=ea@2-1&1-1/A:nil_1_2/B:nil_2@1_1&3#1$0/C:nil-2-2/D:o_1/E:o_1!5_7/F:v_2/G:15-11@0/H:p&1@1@1&1!1/I:T+A&nil+M!nil!M#nil",
			"w^ao-h+ea=z@1-2&0-0/A:nil_2_3/B:nil_2@1_2&2#0$0/C:nil-3-4/D:o_1/E:v_2!6_6/F:o_1/G:15-11@0/H:1&p@1@0&1!1/I:H+H&nil+M!nil!M#nil",
			"ao^h-ea+z=ul@2-1&1-1/A:nil_2_3/B:nil_2@1_2&2#0$0/C:nil-3-4/D:o_1/E:v_2!6_6/F:o_1/G:15-11@0/H:p&0@1@0&1!1/I:T+H&nil+M!nil!M#nil",
			"h^ea-z+ul=ao@1-3&0-0/A:nil_2_2/B:nil_3@2_1&4#0$0/C:nil-2-5/D:o_1/E:v_2!6_6/F:o_1/G:15-11@0/H:0&p@0@1&1!1/I:H+T&nil+M!nil!M#nil",
			"ea^z-ul+ao=d@2-2&1-1/A:nil_2_2/B:nil_3@2_1&4#0$0/C:nil-2-5/D:o_1/E:v_2!6_6/F:o_1/G:15-11@0/H:p&p@0@1&1!1/I:M+T&nil+M!nil!M#nil",
			"z^ul-ao+d=ea@3-1&1-1/A:nil_2_2/B:nil_3@2_1&4#0$0/C:nil-2-5/D:o_1/E:v_2!6_6/F:o_1/G:15-11@0/H:p&1@0@1&1!1/I:T+T&nil+M!nil!M#nil",
			"ul^ao-d+ea=x@1-2&0-0/A:nil_3_4/B:nil_2@1_1&5#0$0/C:nil-3-4/D:v_2/E:o_1!7_5/F:n_2/G:15-11@0/H:1&p@1@1&1!1/I:H+A&nil+M!nil!M#nil",
			"ao^d-ea+x=ih@2-1&1-1/A:nil_3_4/B:nil_2@1_1&5#0$0/C:nil-3-4/D:v_2/E:o_1!7_5/F:n_2/G:15-11@0/H:p&1@1@1&1!1/I:T+A&nil+M!nil!M#nil",
			"d^ea-x+ih=n@1-3&0-0/A:nil_2_5/B:nil_3@1_2&4#0$0/C:nil-3-1/D:o_1/E:n_2!8_4/F:o_1/G:15-11@0/H:1&p@1@0&1!1/I:H+H&nil+T!nil!T#nil",
			"ea^x-ih+n=x@2-2&1-1/A:nil_2_5/B:nil_3@1_2&4#0$0/C:nil-3-1/D:o_1/E:n_2!8_4/F:o_1/G:15-11@0/H:p&p@1@0&1!1/I:M+H&nil+T!nil!T#nil",
			"x^ih-n+x=ih@3-1&1-0/A:nil_2_5/B:nil_3@1_2&4#0$0/C:nil-3-1/D:o_1/E:n_2!8_4/F:o_1/G:15-11@0/H:p&0@1@0&1!1/I:T+H&nil+T!nil!T#nil",
			"ih^n-x+ih=n@1-3&0-0/A:nil_3_4/B:nil_3@2_1&1#0$0/C:nil-2-2/D:o_1/E:n_2!8_4/F:o_1/G:15-11@0/H:0&p@0@1&1!1/I:H+T&nil+T!nil!T#nil",
			"n^x-ih+n=h@2-2&1-1/A:nil_3_4/B:nil_3@2_1&1#0$0/C:nil-2-2/D:o_1/E:n_2!8_4/F:o_1/G:15-11@0/H:p&p@0@1&1!1/I:M+T&nil+T!nil!T#nil",
			"x^ih-n+h=ea@3-1&1-0/A:nil_3_4/B:nil_3@2_1&1#0$0/C:nil-2-2/D:o_1/E:n_2!8_4/F:o_1/G:15-11@0/H:p&1@0@1&1!1/I:T+T&nil+T!nil!T#nil",
			"ih^n-h+ea=j@1-2&0-0/A:nil_3_1/B:nil_2@1_1&2#0$0/C:nil-3-2/D:n_2/E:o_1!9_3/F:n_2/G:15-11@0/H:1&p@1@1&1!1/I:H+A&nil+T!nil!T#nil",
			"n^h-ea+j=vl@2-1&1-1/A:nil_3_1/B:nil_2@1_1&2#0$0/C:nil-3-2/D:n_2/E:o_1!9_3/F:n_2/G:15-11@0/H:p&1@1@1&1!1/I:T+A&nil+T!nil!T#nil",
			"h^ea-j+vl=ee@1-3&0-0/A:nil_2_2/B:nil_3@1_2&2#0$0/C:nil-3-1/D:o_1/E:n_2!10_2/F:nil_1/G:15-11@0/H:1&p@1@0&1!5/I:H+H&nil+T!nil!T#nil",
			"ea^j-vl+ee=x@2-2&1-1/A:nil_2_2/B:nil_3@1_2&2#0$0/C:nil-3-1/D:o_1/E:n_2!10_2/F:nil_1/G:15-11@0/H:p&p@1@0&1!5/I:M+H&nil+T!nil!T#nil",
			"j^vl-ee+x=ih@3-1&1-1/A:nil_2_2/B:nil_3@1_2&2#0$0/C:nil-3-1/D:o_1/E:n_2!10_2/F:nil_1/G:15-11@0/H:p&0@1@0&1!5/I:T+H&nil+T!nil!T#nil",
			"vl^ee-x+ih=n@1-3&0-0/A:nil_3_2/B:nil_3@2_1&1#0$0/C:nil-1-nil/D:o_1/E:n_2!10_2/F:nil_1/G:15-11@0/H:0&p@0@5&1!5/I:H+T&nil+T!nil!T#nil",
			"ee^x-ih+n=pau@2-2&1-1/A:nil_3_2/B:nil_3@2_1&1#0$0/C:nil-1-nil/D:o_1/E:n_2!10_2/F:nil_1/G:15-11@0/H:p&p@0@5&1!5/I:M+T&nil+T!nil!T#nil",
			"x^ih-n+pau=nil@3-1&1-0/A:nil_3_2/B:nil_3@2_1&1#0$0/C:nil-1-nil/D:o_1/E:n_2!10_2/F:nil_1/G:15-11@0/H:p&5@0@5&1!5/I:T+T&nil+T!nil!T#nil",
			"ih^n-pau+nil=nil@1-1&nil-nil/A:nil_3_1/B:nil_1@1_1&nil#nil$nil/C:nil-nil-nil/D:n_2/E:nil_1!11_1/F:nil_nil/G:15-11@0/H:5&5@5@5&5!5/I:nil+nil&nil+nil!nil!nil#nil"
	};
	int idx[2];
	int ret;
	float rho=1.0;
	wtk_syn_hmm_lc_t *lc;
	int i;
	char *data;
	int bytes;
	float diff=0;
	wtk_syn_lc_t* l;

	//wtk_debug("%ld\n",sizeof(ps)/sizeof(char*));
	//.exit(0);
	l=wtk_syn_lc_new(s->heap,s->cfg->hmm);
	//wtk_debug("tot=%d\n",l->totframe);
	for(i=0;i<sizeof(ps)/sizeof(char*);++i)
	{
		data=ps[i];
		bytes=strlen(data);
		lc=wtk_syn_hmm_lc_new(s->cfg->hmm,s->heap,data,bytes);
		//wtk_debug("[%.*s]\n",bytes,data);
		ret=wtk_syn_dtree_search(s->cfg->tree,WTK_SYN_DTREE_TREE_DUR,2,data,bytes,idx);
		if(ret!=0)
		{
			return;
		}
		//wtk_debug("ret=%d len=%f rho=%f\n",ret,s->cfg->length,s->cfg->rho);
		//wtk_debug("idx=%d/%d\n",idx[0],idx[1]);
		//wtk_syn_hmm_find_durpdf(s->cfg->hmm,rho,idx,lc,&diff);
		wtk_syn_hmm_lc_find_durpdf(lc,rho,idx,&diff);
		//wtk_syn_hmm_lc_print(lc);
		l->totframe+=lc->totaldur;
		wtk_syn_hmm_lc_set_output_pdfs(lc,data,bytes);
		//wtk_debug("v[%d]=lc=%p mean=%p\n",l->lc_q.length,lc,lc->lf0mean);
		wtk_queue_push(&(l->lc_q),&(lc->q_n));
//		if(i==1)
//		{
//			wtk_syn_hmm_lc_print(lc);
//			exit(0);
//		}
		wtk_syn_lc_set_gv_pdfs(l,data,bytes);
	}
	wtk_debug("tot=%d\n",l->totframe);

	wtk_syn_fixf0(s,l);
	//wtk_debug("tot=%d\n",l->totframe);
	//wtk_syn_get_dur(s,l);
	//wtk_debug("tot=%d\n",l->totframe);
	//exit(0);
	wtk_syn_pdf2speech(s,l);
	//wtk_debug("test end\n");
}
