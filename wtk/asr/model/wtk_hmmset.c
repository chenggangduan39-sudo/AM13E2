#include "wtk_hmmset.h"

wtk_hmmset_t* wtk_hmmset_new(wtk_label_t *l)
{
	return wtk_hmmset_new2(l,71);
}

wtk_hmmset_t* wtk_hmmset_new2(wtk_label_t *l,int hmmlist_hint)
{
	wtk_hmmset_t* hs;

	hs=(wtk_hmmset_t*)wtk_calloc(1,sizeof(*hs));
	wtk_hmmset_init(hs,l,hmmlist_hint);
	return hs;
}

int wtk_hmmset_delete(wtk_hmmset_t *hs)
{
	wtk_hmmset_clean(hs);
	free(hs);
	return 0;
}

int wtk_hmmset_bytes(wtk_hmmset_t *hs)
{
	int bytes=0;

	bytes+=wtk_str_hash_bytes(hs->hmm_hash);
	wtk_debug("bytes=%fM\n",bytes*1.0/(1024*1024));
	bytes+=wtk_str_hash_bytes(hs->mac_hash);
	wtk_debug("bytes=%fM\n",bytes*1.0/(1024*1024));
	bytes+=wtk_heap_bytes(hs->heap);
	wtk_debug("bytes=%fM\n",bytes*1.0/(1024*1024));
	bytes+=wtk_larray_bytes(hs->hmm_array);
	wtk_debug("bytes=%fM\n",bytes*1.0/(1024*1024));
	return bytes;
}

int wtk_hmmset_init(wtk_hmmset_t *hl,wtk_label_t *label,int hmmlist_hint)
{
	hl->cfg=NULL;
	hl->use_le=0;
	hl->load_hmm_from_hmmlist=1;
	hl->heap=wtk_heap_new(4096);
	hl->hmm_hash=wtk_str_hash_new2(hmmlist_hint,hl->heap);
	hl->mac_hash=wtk_str_hash_new2(hmmlist_hint,hl->heap);
	//wtk_label_init(&(hl->label),25007);
	hl->hmm_array=wtk_larray_new(hmmlist_hint,sizeof(wtk_hmm_t*));
	//hl->hmm_array=wtk_array_new_h(hl->heap,hmmlist_hint,sizeof(wtk_hmm_t*));
	hl->label=label;
	hl->allow_tmods=1;
	hl->num_phy_hmm=0;
	hl->num_phy_mix=0;
	hl->mix_log_exp=-log(-LZERO);
	hl->max_hmm_state=0;
	return 0;
}

int wtk_hmmset_clean(wtk_hmmset_t *hl)
{
	wtk_larray_delete(hl->hmm_array);
	wtk_str_hash_delete(hl->hmm_hash);
	wtk_str_hash_delete(hl->mac_hash);
	wtk_heap_delete(hl->heap);
	return 0;
}

void wtk_hmmset_set_hmm_load_type(wtk_hmmset_t *h,int load_hmm_from_hmmlist)
{
	h->load_hmm_from_hmmlist=load_hmm_from_hmmlist;
}

wtk_mixpdf_t* wtk_hmmset_new_mixpdf(wtk_hmmset_t *hl)
{
	wtk_mixpdf_t* pdf;
	wtk_heap_t *h;

	h=hl->heap;
	pdf=(wtk_mixpdf_t*)wtk_heap_malloc(h,sizeof(*pdf));
	pdf->used=0;
	pdf->fGconst=LZERO;
	++hl->num_phy_mix;
	pdf->index=hl->num_phy_mix;
	return pdf;
}

wtk_state_t* wtk_hmmset_new_state(wtk_hmmset_t *hl)
{
	wtk_state_t *s;

	s=(wtk_state_t*)wtk_heap_zalloc(hl->heap,sizeof(*s));
	s->index=-1;
	return s;
}

wtk_stream_t* wtk_hmmset_new_streams(wtk_hmmset_t *hl,int n)
{
	wtk_stream_t *s;

	s=(wtk_stream_t*)wtk_heap_malloc(hl->heap,n*sizeof(*s));
	return s;
}

wtk_mixture_t* wtk_hmmset_new_mixtures(wtk_hmmset_t *hl,int n)
{
	return (wtk_mixture_t*)wtk_heap_malloc(hl->heap,sizeof(wtk_mixture_t)*n);
}

wtk_hmm_t* wtk_hmmset_new_hmm(wtk_hmmset_t *hl,char* n,int nl)
{
	wtk_hmm_t *hmm;
	wtk_hmm_t **ph;

	//wtk_debug("%*.*s\n",nl,nl,n);
	hmm=(wtk_hmm_t*)wtk_heap_zalloc(hl->heap,sizeof(*hmm));
	hmm->name=wtk_hmmset_find_name(hl,n,nl);
	//hmm->sil=wtk_string_cmp_s(hmm->name,"sil")==0?1:0;
	//hmm->name=wtk_heap_dup_string(hl->heap,n,nl);
	hmm->tIdx=hl->num_phy_hmm++;
	//hmm->max_trans=LZERO;
	//wtk_str_hash_add(hl->hmm_hash,hmm->name->data,hmm->name->len,hmm);
	ph=(wtk_hmm_t**)wtk_larray_push(hl->hmm_array);
	//hmm->seIndex=NULL;
	*ph=hmm;
	//wtk_debug("[%.*s]=%d,%p\n",nl,n,hl->hmm_array->nslot,hmm);
	return hmm;
}

wtk_macro_t* wtk_hmmset_find_macro(wtk_hmmset_t *hl,char type,char* n,int nl)
{
	wtk_str_hash_t *h=hl->mac_hash;
	uint32_t index;
	wtk_queue_t *q;
	wtk_queue_node_t *qn;
	hash_str_node_t *hn;
	wtk_macro_t *m;

	//wtk_debug("c=%c,[%.*s]\n",type,nl,n);
	index=hash_string_value_len(n,nl,h->nslot);
	q=h->slot[index];
	if(!q)// || q->length==0)
	{
		return NULL;
	}
	for(qn=q->pop;qn;qn=qn->next)
	{
		hn=(hash_str_node_t*)data_offset2(qn,hash_str_node_t,n);
		if(hn->key.len==nl && strncmp(hn->key.data,n,nl)==0)
		{
			m=(wtk_macro_t*)hn->value;
			if(m->type==type)
			{
				return m;
			}
		}
	}
	return NULL;
}



wtk_macro_t* wtk_hmmset_find_macro2(wtk_hmmset_t *hl,char type,char* n,int nl)
{
	wtk_macro_t *m;
	wtk_str_hash_t *h=hl->mac_hash;
	wtk_string_t name;
	wtk_macro_t t;
	int ret;

	name.data=n;name.len=nl;
	t.name=&(name);
	t.type=type;m=0;
	ret=wtk_str_hash_findc(h,n,nl,(wtk_cmp_handler_t)wtk_macro_cmp,(void*)&t,(void**)&m);
	return ret==0?m:0;
}

void* wtk_hmmset_find_macro_hook(wtk_hmmset_t *hl,char type,char* n,int nl)
{
	wtk_macro_t *m;

	m=wtk_hmmset_find_macro(hl,type,n,nl);
	return m?m->hook:0;
}

wtk_string_t* wtk_hmmset_find_name(wtk_hmmset_t *hl,char *n,int nl)
{
	wtk_name_t *ln;

	ln=wtk_label_find((hl->label),n,nl,1);
	return ln->name;
}

int wtk_hmmset_add_macro2(wtk_hmmset_t *hl,char type,char* n,int nl,void *hook,wtk_macro_t** pm)
{
	wtk_macro_t *m;
	wtk_str_hash_t *h=hl->mac_hash;
	wtk_name_t *ln;
	int ret;

	//wtk_debug("%*.*s\n",nl,nl,n);
	m=(wtk_macro_t*)wtk_str_hash_malloc(h,sizeof(*m));
	m->type=type;
	ln=wtk_label_find((hl->label),n,nl,1);
	m->name=ln->name;
	m->hook=hook;
	ret=wtk_str_hash_add(h,m->name->data,m->name->len,m);
	if(pm){*pm=m;}
	return ret;
}

int wtk_hmmset_add_macro(wtk_hmmset_t *hl,char type,char* n,int nl,void *hook)
{
	return wtk_hmmset_add_macro2(hl,type,n,nl,hook,0);
}

#ifdef USE_MAC

int wtk_hmmset_add_hmm(wtk_hmmset_t *hl,char* l,int lb,char* p,int pb)
{
	wtk_macro_t *lm,*pm;
	wtk_hmm_t *hmm;
	int ret;

	//wtk_debug("l=%.*s,p=%.*s.\n",lb,l,pb,p);
	ret=-1;
	lm=wtk_hmmset_find_macro(hl,MACRO_TYPE_L,l,lb);
	if(lm){goto end;}
	pm=wtk_hmmset_find_macro(hl,MACRO_TYPE_H,p,pb);
	if(!pm)
	{
		hmm=wtk_hmmset_new_hmm(hl,p,pb);
		wtk_hmmset_add_macro(hl,MACRO_TYPE_H,p,pb,hmm);
	}else
	{
		hmm=(wtk_hmm_t*)pm->hook;
	}
	//wtk_str_hash_add(hl->hmm_hash,m->name->data,m->name->len,hmm);
	ret=wtk_hmmset_add_macro2(hl,MACRO_TYPE_L,l,lb,hmm,&lm);
	if(ret==0)
	{
		wtk_str_hash_add(hl->hmm_hash,lm->name->data,lm->name->len,lm);
	}
end:
	return ret;
}

wtk_hmm_t* wtk_hmmset_find_hmm(wtk_hmmset_t *hl,char* n,int nl)
{
	wtk_macro_t* m;

	m=(wtk_macro_t*)wtk_str_hash_find(hl->hmm_hash,n,nl);
	return m ? (wtk_hmm_t*)m->hook:0;
}

#else

int wtk_hmmset_add_hmm(wtk_hmmset_t *hl,char* l,int lb,char* p,int pb)
{
	wtk_hmm_t *hmm = NULL;
	wtk_string_t *v;

	//wtk_debug("l=%.*s,p=%.*s.\n",lb,l,pb,p);
	if(lb>0)
	{
		hmm=(wtk_hmm_t*)wtk_str_hash_find(hl->hmm_hash,l,lb);
		if(hmm){goto end;}
	}
	if(pb>0)
	{
		hmm=(wtk_hmm_t*)wtk_str_hash_find(hl->hmm_hash,p,pb);
		if(!hmm)
		{
			hmm=wtk_hmmset_new_hmm(hl,p,pb);
			wtk_str_hash_add(hl->hmm_hash,hmm->name->data,hmm->name->len,hmm);
		}
	}
	if(lb>0)
	{
		v=wtk_heap_dup_string(hl->heap,l,lb);
		wtk_str_hash_add(hl->hmm_hash,v->data,v->len,hmm);

        wtk_label_find((hl->label),l,lb,1);  // jfyuan 20140312 add
	}
end:
	return 0;
}

int wtk_hmmset_add_hmm2(wtk_hmmset_t *hl,char* p,int pb)
{
	wtk_hmm_t *hmm;

	hmm=wtk_hmmset_new_hmm(hl,p,pb);
	wtk_str_hash_add(hl->hmm_hash,hmm->name->data,hmm->name->len,hmm);
	return 0;
}

wtk_hmm_t* wtk_hmmset_find_hmm(wtk_hmmset_t *hl,char* n,int nl)
{
	return (wtk_hmm_t*)wtk_str_hash_find(hl->hmm_hash,n,nl);
}
#endif

double wtk_log_add(double x, double y,double min_log_exp)
{
	double temp,diff,z;

   if (x<y)
   {
      temp = x; x = y; y = temp;
   }
   diff = y-x;
   if (diff<min_log_exp)
   {
      return  (x<LSMALL)?LZERO:x;
   }
   else
   {
      z = exp(diff);
      return x+log(1.0+z);
   }
}

float wtk_mixpdf_calc_dia_prob1(wtk_mixpdf_t *pdf,wtk_vector_t *obs)
{
	double prob=pdf->fGconst;
	double mean;
	int i,n;

	n=wtk_vector_size(obs);
	for(i=1;i<=n;++i)
	{
		mean=obs[i]-pdf->mean[i];
		prob+=(mean*mean)*(pdf->variance[i]);
		//wtk_debug("%d: %f,%f,%f,%f\n",i,prob,obs[i],pdf->mean[i],pdf->variance[i]);
	}
	//return (float)(prob*-0.5f);
	return prob;
}

float wtk_mixpdf_calc_dia_prob(wtk_mixpdf_t *pdf,wtk_vector_t *obs)
{
	register float *pm,*pv,*po;
	register float *e;
	register float prob=pdf->fGconst;
	register float mean;

	//wtk_vector_print(obs);
	pm=pdf->mean;pv=pdf->variance;po=obs;
	//wtk_debug("sf=%d,pm=%d,pv=%d,po=%d\n",(int)(sizeof(float)),(int)((long)pm%sizeof(long)),(int)((long)pv%sizeof(long)),(int)((long)po%sizeof(long)));
	e=po+wtk_vector_size(obs);
	while(po<e)
	{
		mean=*(++po)-*(++pm);
		prob+=(mean*mean)*(*(++pv));
	}
	//return (float)(prob*-0.5f);
	return prob;
}

double wtk_hmmset_calc_prob_fix(wtk_hmmset_t *hl,wtk_state_t *state,wtk_fixi_t *fix,float scale)
{
	wtk_mixture_t *mix;
	wtk_stream_t *stream;
	int streams,i,j;
	double outp=0,weight,mixprob,streamprob;

	//wtk_vector_print(obs);
	streams=hl->stream_width[0];
	for(i=0,stream=state->pStream;i<streams;++i,++stream)
	{
		streamprob=LZERO;
		for(j=0,mix=stream->pmixture;j<stream->nMixture;++j,++mix)
		{
			//weight=log(mix->fWeight);
			weight=mix->fWeight;
			mixprob=wtk_mixpdf_calc_dia_prob_fix(mix->pdf,fix,scale);
			streamprob=wtk_log_add(streamprob,weight+mixprob,hl->mix_log_exp);
		}
		if(streams==1)
		{
			outp=streamprob;
		}else
		{
			outp+=state->pfStreamWeight[i]*streamprob;
		}
	}
	return outp;
}



float wtk_mixpdf_calc_dia_prob_fix(wtk_mixpdf_t *pdf,wtk_fixi_t *fix,float scale)
{
	float prob;
	register int v;
	register int *pi,*pe;
	register int *pmean;
	register int *pvar;
	register int mean;

	v=0;
	pi=fix->p-1;
	pe=pi+fix->col;
	pmean=(int*)(pdf->mean);
	pvar=(int*)(pdf->variance);
	while((pi+4)<=pe)
	{
		mean=(*(++pi))-*(++pmean);
		v+=(mean*mean)*(*(++pvar));

		mean=(*(++pi))-*(++pmean);
		v+=(mean*mean)*(*(++pvar));

		mean=(*(++pi))-*(++pmean);
		v+=(mean*mean)*(*(++pvar));

		mean=(*(++pi))-*(++pmean);
		v+=(mean*mean)*(*(++pvar));
	}
	while(pi<pe)
	{
		mean=(*(++pi))-*(++pmean);
		v+=(mean*mean)*(*(++pvar));
	}
	//wtk_debug("v=%d\n",v);
	prob=pdf->fGconst+v/scale;
	//prob=pdf->fGconst+v*rec->fix_scale;//v/rec->fix_scale;//(fix->scale*rec->hmmset->cfg->var_scale*fix->scale);
	//wtk_debug("prob=%f v=%d scale=%f/%f\n",prob,v,fix->scale,rec->hmmset->cfg->var_scale);
	return prob;
}



void wtk_mixpdf_post_process(wtk_hmmset_t *set,wtk_mixpdf_t *pdf)
{
	int n;
	float *s,*e;

	n=wtk_vector_size(pdf->variance);
	s=pdf->variance+1;
	e=s+n;
	//wtk_debug("%d/%d\n",(int)(e-s),n);
	while(s<e)
	{
		//wtk_debug("%d/%d\n",(int)(e-s),n);
		*(s)*=-0.5;
		++s;
	}
	//exit(0);
	pdf->fGconst*=-0.5f;
	if(set->cfg && set->cfg->use_fix)
	{
#ifdef DEBUG_MAX
		static float max1=0,max2=0;
		float f;

		f=wtk_vector_max_abs(pdf->variance);
		if(f>max1)
		{
			max1=f;
		}
		f=wtk_vector_max_abs(pdf->mean);
		if(f>max2)
		{
			max2=f;
		}
		wtk_debug("var=%f mean=%f\n",max1,max2);
		//exit(0);
#endif
		wtk_vector_fix_scale(pdf->variance,set->cfg->var_scale);
		wtk_vector_fix_scale(pdf->mean,set->cfg->mean_scale);
	}
}

double wtk_hmmset_calc_prob(wtk_hmmset_t *hl,wtk_state_t *state,wtk_vector_t *obs)
{
	wtk_mixture_t *mix;
	wtk_stream_t *stream;
	int streams,i,j;
	double outp=0,weight,mixprob,streamprob;

	//wtk_vector_print(obs);
	streams=hl->stream_width[0];
	for(i=0,stream=state->pStream;i<streams;++i,++stream)
	{
		streamprob=LZERO;
		for(j=0,mix=stream->pmixture;j<stream->nMixture;++j,++mix)
		{
			//weight=log(mix->fWeight);
			weight=mix->fWeight;
			mixprob=wtk_mixpdf_calc_dia_prob(mix->pdf,obs);
			streamprob=wtk_log_add(streamprob,weight+mixprob,hl->mix_log_exp);
		}
		if(streams==1)
		{
			outp=streamprob;
		}else
		{
			outp+=state->pfStreamWeight[i]*streamprob;
		}
	}
	return outp;
}

int wtk_hmmlist_print_macro(wtk_hmmset_t *hl,hash_str_node_t *h)
{
	wtk_macro_print((wtk_macro_t*)h->value);
	return 0;
}

int wtk_hmmlist_print_hmm(wtk_hmmset_t *hl,hash_str_node_t *h)
{
	wtk_macro_t *macro=(wtk_macro_t*)h->value;
	wtk_hmm_t *hmm=(wtk_hmm_t*)macro->hook;

	printf("%*.*s(state=%d)\n",hmm->name->len,hmm->name->len,hmm->name->data,hmm->num_state);
	return 0;
}

void wtk_hmmset_print(wtk_hmmset_t *hl)
{
	//wtk_str_hash_walk(hl->mac_hash,(wtk_walk_handler_t)wtk_hmmlist_print_macro,hl);
	wtk_str_hash_walk(hl->hmm_hash,(wtk_walk_handler_t)wtk_hmmlist_print_hmm,hl);
}

void wtk_inputxform_print(wtk_inputxform_t* xf)
{
	int i;

	for(i=1;i<=wtk_vector_size(xf->xform->block_size);++i)
	{
		wtk_matrix_print(xf->xform->xform[i]);
	}
}

void wtk_hmm_print(wtk_hmm_t *hmm)
{
	printf("################### hmm #########################\n");
	printf("name:\t%*.*s\n",hmm->name->len,hmm->name->len,hmm->name->data);
	printf("state:\t%d\n",hmm->num_state);
	printf("#################################################\n");
}


wtk_stream_t* wtk_stream_dup(wtk_stream_t *src,wtk_heap_t *heap)
{
	wtk_stream_t *dst;
	wtk_mixpdf_t *p1,*p2;
	int i;

	dst=(wtk_stream_t*)wtk_heap_malloc(heap,sizeof(wtk_stream_t));
	dst->nMixture=src->nMixture;
	dst->pmixture=(wtk_mixture_t*)wtk_heap_malloc(heap,sizeof(wtk_mixture_t)*dst->nMixture);
	for(i=0;i<dst->nMixture;++i)
	{
		dst->pmixture[i].fWeight=src->pmixture[i].fWeight;
		dst->pmixture[i].pdf=(wtk_mixpdf_t*)wtk_heap_malloc(heap,sizeof(wtk_mixpdf_t));
		p1=dst->pmixture[i].pdf;
		p2=src->pmixture[i].pdf;
		p1->fGconst=p2->fGconst;
		p1->used=0;
		p1->index=0;
		p1->mean=wtk_svector_dup(heap,p2->mean);
		p1->variance=wtk_svector_dup(heap,p2->variance);
	}
	return dst;
}


void wtk_mixpdf_print(wtk_mixpdf_t *pdf)
{
	wtk_debug("============ pdf=%p ===================\n",pdf);
	printf("gconst: %f\n",pdf->fGconst);
	print_float(pdf->mean,wtk_vector_size(pdf->mean));
	print_float(pdf->variance,wtk_vector_size(pdf->variance));
}
