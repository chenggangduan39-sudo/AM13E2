#include "wtk_tts_pos.h" 

void wtk_tts_pos_init(wtk_tts_pos_t *pos,wtk_tts_pos_cfg_t *cfg,wtk_rbin2_t *rbin)
{
	pos->cfg=cfg;
	if(cfg->use_voc_bin)
	{
		if(rbin)
		{
			pos->fkv=wtk_fkv_new4(rbin,cfg->voc_fn,1703);
		}else
		{
			pos->fkv=wtk_fkv_new3(cfg->voc_fn);
		}
		pos->oov=wtk_fkv_get_int_s(pos->fkv,"OOV",NULL);
	}else
	{
		pos->oov=-1;
		pos->fkv=NULL;
	}
}

void wtk_tts_pos_clean(wtk_tts_pos_t *pos)
{
	if(pos->fkv)
	{
		wtk_fkv_delete(pos->fkv);
	}
}

void wtk_tts_pos_reset(wtk_tts_pos_t *pos)
{
	if(pos->fkv)
	{
		wtk_fkv_reset(pos->fkv);
	}
}


int* wtk_tts_pos_get_index(wtk_tts_pos_t *pos,wtk_tts_snt_t *snt,wtk_heap_t *heap)
{
	wtk_tts_wrd_t **wrds,*w;
	int *v;
	int i;

	v=(int*)wtk_heap_malloc(heap,sizeof(int)*snt->wrds->nslot);
	wrds=(wtk_tts_wrd_t**)snt->wrds->slot;
	for(i=0;i<snt->wrds->nslot;++i)
	{
		w=wrds[i];
		if(pos->fkv)
		{
			v[i]=wtk_fkv_get_int(pos->fkv,w->v->data,w->v->len,NULL);
			if(v[i]<0)
			{
				v[i]=pos->oov;
			}
		}else
		{
			v[i]=wtk_tts_poshmm_get_idx(pos->cfg->hmm,w->v->data,w->v->len);
		}
		//wtk_debug("v[%d/%d]=[%.*s]\n",i,v[i],w->v->len,w->v->data);
	}
	return v;
}

int* wtk_tts_pos_decode(wtk_tts_pos_t *p,wtk_heap_t *heap,int *v,int n)
{
	wtk_tts_poshmm_t *hmm=p->cfg->hmm;
	wtk_matrix_t *biot,*delta,*psi;
	int i,j,k,pos;
	float maxval,val,f;

	biot=wtk_matrix_newh(heap,hmm->N,n);
	for(i=1;i<=hmm->N;++i)
	{
		for(j=1;j<=n;++j)
		{
			//f=hmm->mat[i][v[j-1]];
			f=wtk_sparsem_get(hmm->mat,i-1,v[j-1]-1);
			//wtk_debug("v[%d][%d]=%f\n",i-1,v[j-1]-1,f);
			if(f!=0.0)
			{
				//wtk_debug("%f\n",hmm->mat[i][v[j-1]]);
				biot[i][j]=f;
			}else
			{
				biot[i][j]=LZERO;
			}
		}
	}
	delta=wtk_matrix_newh(heap,n,hmm->N);
	psi=wtk_matrix_newh(heap,n,hmm->N);
	for(i=1;i<=hmm->N;++i)
	{
		delta[1][i]=hmm->pi[i]+biot[i][1];
		//wtk_debug("v[%d]=%f %f/%f\n",i,delta[1][i],hmm->pi[i],biot[i][1]);
	}
	for(i=2;i<=n;++i)
	{
		for(j=1;j<=hmm->N;++j)
		{
			maxval=LZERO;
			pos=0;
			for(k=1;k<=hmm->N;++k)
			{
				val=delta[i-1][k]+hmm->A[k][j];
				if(val>maxval)
				{
					maxval=val;
					pos=k;
				}
			}
			delta[i][j]=maxval+biot[j][i];
			psi[i][j]=pos;
			//wtk_mati_at(psi,i-1,j-1)=pos;
		}
	}
	maxval=LZERO;
	pos=0;
	for(i=1;i<=hmm->N;++i)
	{
		if(delta[n][i]>maxval)
		{
			maxval=delta[n][i];
			pos=i;
		}
	}
	v[n-1]=pos;

	for(i=n-2;i>=0;--i)
	{
		v[i]=psi[i+2][v[i+1]];
		//wtk_debug("v[%d]=%d\n",i,v[i]);
	}
	//exit(0);
	return v;
}

static int wtk_nstring_has(wtk_string_t *s,int n,wtk_string_t *v)
{
	int i;

	for(i=0;i<n;++i,++s)
	{
		if(wtk_string_cmp(s,v->data,v->len)==0)
		{
			return 1;
		}
	}
	return 0;
}

int wtk_tts_pos_process_snt(wtk_tts_pos_t *pos,wtk_tts_info_t *info,wtk_tts_snt_t *s)
{
	static wtk_string_t sn[]={
			wtk_string("nr"),wtk_string("ns"),wtk_string("nt"),wtk_string("nz"),
			wtk_string("nx"),wtk_string("Ng")
	};
	static wtk_string_t sa[]={
			wtk_string("ad"),wtk_string("an"),wtk_string("Ag")
	};
	static wtk_string_t sv[]={
			wtk_string("vd"),wtk_string("vn"),wtk_string("Vg")
	};
	static wtk_string_t sx[]={
			wtk_string("n"),wtk_string("a"),wtk_string("v"),wtk_string("m"),wtk_string("y"),
			wtk_string("q"),wtk_string("d"),wtk_string("w"),wtk_string("l")
	};
	static wtk_string_t xn=wtk_string("n");
	static wtk_string_t xa=wtk_string("a");
	static wtk_string_t xv=wtk_string("v");
	static wtk_string_t xm=wtk_string("m");
	static wtk_string_t xo=wtk_string("o");
	wtk_string_t **strs;
	wtk_tts_wrd_t **wrds,*w;
	int j;
	int *v;

	strs=(wtk_string_t**)pos->cfg->attr->slot;
	if(s->wrds->nslot<=0)
	{
		return 0;
	}
	//wtk_tts_snt_print(s);
	v=wtk_tts_pos_get_index(pos,s,info->heap);//(int*)wtk_heap_malloc(info->heap,sizeof(int)*s->wrds->nslot);
	//wtk_debug("nslot=%d\n",s->wrds->nslot);
	v=wtk_tts_pos_decode(pos,info->heap,v,s->wrds->nslot);
	//exit(0);
	wrds=(wtk_tts_wrd_t**)s->wrds->slot;
	for(j=0;j<s->wrds->nslot;++j)
	{
		w=wrds[j];
		w->pos=strs[v[j]-1];
		//wtk_debug("[%.*s]=%d\n",w->v->len,w->v->data,v[j]);
		if(wtk_nstring_has(sn,sizeof(sn)/sizeof(wtk_string_t),w->pos))
		{
			w->pos=&(xn);
		}else if(wtk_nstring_has(sa,sizeof(sa)/sizeof(wtk_string_t),w->pos))
		{
			w->pos=&(xa);
		}else if(wtk_nstring_has(sv,sizeof(sv)/sizeof(wtk_string_t),w->pos))
		{
			w->pos=&(xv);
		}else if(wtk_string_cmp_s(w->pos,"Mg")==0)
		{
			w->pos=&(xm);
		}else if(wtk_nstring_has(sx,sizeof(sx)/sizeof(wtk_string_t),w->pos))
		{

		}else
		{
			w->pos=&xo;
		}
		//wtk_debug("v[%d]=%.*s %.*s\n",j,w->v->len,w->v->data,w->pos->len,w->pos->data);
	}
	return 0;
}

static int* wtk_tts_pos_get_index2(wtk_tts_pos_t *pos,wtk_array_t *wrds_a,wtk_heap_t *heap)
{
	wtk_tts_wrd_t **wrds, *w;
	int *v;
	int i;

	v=(int*)wtk_heap_malloc(heap,sizeof(int)*wrds_a->nslot);
	wrds=(wtk_tts_wrd_t**)wrds_a->slot;
	for(i=0;i<wrds_a->nslot;++i)
	{
		w=wrds[i];
		if(pos->fkv)
		{
			v[i]=wtk_fkv_get_int(pos->fkv,w->v->data,w->v->len,NULL);
			if(v[i]<0)
			{
				v[i]=pos->oov;
			}
		}else
		{
			v[i]=wtk_tts_poshmm_get_idx(pos->cfg->hmm,w->v->data,w->v->len);
		}
	}
	return v;
}
int wtk_tts_pos_process_snt2(wtk_tts_pos_t *pos,wtk_tts_info_t *info,wtk_array_t*wrds_a)
{
	static wtk_string_t sn[]={
			wtk_string("nr"),wtk_string("ns"),wtk_string("nt"),wtk_string("nz"),
			wtk_string("nx"),wtk_string("Ng")
	};
	static wtk_string_t sa[]={
			wtk_string("ad"),wtk_string("an"),wtk_string("Ag")
	};
	static wtk_string_t sv[]={
			wtk_string("vd"),wtk_string("vn"),wtk_string("Vg")
	};
	static wtk_string_t sx[]={
			wtk_string("n"),wtk_string("a"),wtk_string("v"),wtk_string("m"),wtk_string("y"),
			wtk_string("q"),wtk_string("d"),wtk_string("w"),wtk_string("l")
	};
	static wtk_string_t xn=wtk_string("n");
	static wtk_string_t xa=wtk_string("a");
	static wtk_string_t xv=wtk_string("v");
	static wtk_string_t xm=wtk_string("m");
	static wtk_string_t xo=wtk_string("o");
	wtk_string_t **strs;
	wtk_tts_wrd_t **wrds,*w;
	int j;
	int *v;

	strs=(wtk_string_t**)pos->cfg->attr->slot;
	if(wrds_a->nslot<=0)
	{
		goto end;
	}
	v=wtk_tts_pos_get_index2(pos,wrds_a,info->heap);//(int*)wtk_heap_malloc(info->heap,sizeof(int)*s->wrds->nslot);
	v=wtk_tts_pos_decode(pos,info->heap,v,wrds_a->nslot);
	wrds=(wtk_tts_wrd_t**)wrds_a->slot;
	for(j=0;j<wrds_a->nslot;++j)
	{
		w=wrds[j];
		w->pos=strs[v[j]-1];
		//wtk_debug("[%.*s]=%d\n",w->v->len,w->v->data,v[j]);
		if(wtk_nstring_has(sn,sizeof(sn)/sizeof(wtk_string_t),w->pos))
		{
			w->pos=&(xn);
		}else if(wtk_nstring_has(sa,sizeof(sa)/sizeof(wtk_string_t),w->pos))
		{
			w->pos=&(xa);
		}else if(wtk_nstring_has(sv,sizeof(sv)/sizeof(wtk_string_t),w->pos))
		{
			w->pos=&(xv);
		}else if(wtk_string_cmp_s(w->pos,"Mg")==0)
		{
			w->pos=&(xm);
		}else if(wtk_nstring_has(sx,sizeof(sx)/sizeof(wtk_string_t),w->pos))
		{

		}else
		{
			w->pos=&xo;
		}
	}
end:
	return 0;
}
int wtk_tts_pos_process(wtk_tts_pos_t *pos,wtk_tts_info_t *info,wtk_tts_lab_t *lab)
{
	static wtk_string_t sn[]={
			wtk_string("nr"),wtk_string("ns"),wtk_string("nt"),wtk_string("nz"),
			wtk_string("nx"),wtk_string("Ng")
	};
	static wtk_string_t sa[]={
			wtk_string("ad"),wtk_string("an"),wtk_string("Ag")
	};
	static wtk_string_t sv[]={
			wtk_string("vd"),wtk_string("vn"),wtk_string("Vg")
	};
	static wtk_string_t sx[]={
			wtk_string("n"),wtk_string("a"),wtk_string("v"),wtk_string("m"),wtk_string("y"),
			wtk_string("q"),wtk_string("d"),wtk_string("w"),wtk_string("l")
	};
	static wtk_string_t xn=wtk_string("n");
	static wtk_string_t xa=wtk_string("a");
	static wtk_string_t xv=wtk_string("v");
	static wtk_string_t xm=wtk_string("m");
	static wtk_string_t xo=wtk_string("o");
	wtk_tts_snt_t **snt,*s;
	wtk_string_t **strs;
	wtk_tts_wrd_t **wrds,*w;
	int i,j;
	int *v;

	snt=(wtk_tts_snt_t**)lab->snts->slot;
	strs=(wtk_string_t**)pos->cfg->attr->slot;
//	for(i=0;i<pos->cfg->attr->nslot;++i)
//	{
//		wtk_debug("v[%d]=%.*s\n",i,strs[i]->len,strs[i]->data);
//	}
	for(i=0;i<lab->snts->nslot;++i)
	{
		s=snt[i];
		if(s->wrds->nslot<=0)
		{
			continue;
		}
		//wtk_tts_snt_print(s);
		v=wtk_tts_pos_get_index(pos,s,info->heap);//(int*)wtk_heap_malloc(info->heap,sizeof(int)*s->wrds->nslot);
		//wtk_debug("nslot=%d\n",s->wrds->nslot);
		v=wtk_tts_pos_decode(pos,info->heap,v,s->wrds->nslot);
		//exit(0);
		wrds=(wtk_tts_wrd_t**)s->wrds->slot;
		for(j=0;j<s->wrds->nslot;++j)
		{
			w=wrds[j];
			w->pos=strs[v[j]-1];
			//wtk_debug("[%.*s]=%d\n",w->v->len,w->v->data,v[j]);
			if(wtk_nstring_has(sn,sizeof(sn)/sizeof(wtk_string_t),w->pos))
			{
				w->pos=&(xn);
			}else if(wtk_nstring_has(sa,sizeof(sa)/sizeof(wtk_string_t),w->pos))
			{
				w->pos=&(xa);
			}else if(wtk_nstring_has(sv,sizeof(sv)/sizeof(wtk_string_t),w->pos))
			{
				w->pos=&(xv);
			}else if(wtk_string_cmp_s(w->pos,"Mg")==0)
			{
				w->pos=&(xm);
			}else if(wtk_nstring_has(sx,sizeof(sx)/sizeof(wtk_string_t),w->pos))
			{

			}else
			{
				w->pos=&xo;
			}
			//wtk_debug("v[%d]=%.*s %.*s\n",j,w->v->len,w->v->data,w->pos->len,w->pos->data);
		}
		//exit(0);
	}
	//exit(0);
	return 0;
}
