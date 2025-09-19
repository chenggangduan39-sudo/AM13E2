#include "wtk_tts_phn.h" 


void wtk_tts_phn_init(wtk_tts_phn_t *phn,wtk_tts_phn_cfg_t *cfg)
{
	phn->cfg=cfg;
}

void wtk_tts_phn_clean(wtk_tts_phn_t *phn)
{
}

void wtk_tts_phn_reset(wtk_tts_phn_t *phn)
{
}

int wtk_tts_phn_process_snt(wtk_tts_phn_t *phn,wtk_tts_info_t *info,wtk_tts_snt_t *s)
{
	static wtk_string_t ps=wtk_string("pau");
	static wtk_string_t *pps=&ps;
	static wtk_tts_sylphn_t pau={
			&pps,
			1
	};
	wtk_tts_wrd_t **wrds,*w;
	wtk_tts_xsyl_t *syl;
	wtk_tts_wrd_xpron_t *pron;
	wtk_tts_xphn_t *xphn;
	int j,k,m;

	if(s->wrds->nslot<=0)
	{
		return 0;
	}
	//wtk_debug("s->wrds->nslot=%d\n",s->wrds->nslot);
	wrds=(wtk_tts_wrd_t**)s->wrds->slot;
	s->phns=wtk_array_new_h(info->heap,s->wrds->nslot*6,sizeof(void*));
	for(j=0;j<s->wrds->nslot;++j)
	{
		w=wrds[j];
		//wtk_debug("wrd[%d]=[%.*s]\n",j,w->v->len,w->v->data);
		pron=w->pron;
		//wtk_tts_wrd_xpron_print(pron);
		for(k=0,syl=pron->xsyl;k<pron->pron->nsyl;++k,++syl)
		{
			//wtk_debug("syl[%d]=[%.*s/%d]\n",k,syl->syl->v->len,syl->syl->v->data,syl->syl->tone);
			if(wtk_string_cmp_s(syl->syl->v,"pau")==0)
			{
				syl->phn=&pau;
			}else
			{
				syl->phn=wtk_kdict_get(phn->cfg->dict,syl->syl->v->data,syl->syl->v->len);
			}
			syl->phns=wtk_array_new_h(info->heap,syl->phn->nphn,sizeof(void*));
			//wtk_tts_sylphn_print(syl->phn);
			for(m=0;m<syl->phn->nphn;++m)
			{
				//wtk_debug("phn[%d]=%.*s\n",m,syl->phn->phns[m]->len,syl->phn->phns[m]->data);
				if(s->phns->nslot==0 && wtk_string_cmp_s(syl->phn->phns[m],"pau")!=0)
				{
					xphn=wtk_tts_xphn_new(info->heap);
					xphn->wrd=NULL;
					xphn->pron=NULL;
					xphn->syl=NULL;
					xphn->snt=NULL;
					xphn->phn=&ps;
					xphn->sil=1;
					xphn->lab=NULL;
					xphn->bound=WTK_TTS_BOUND_SEG;
					wtk_array_push2(s->phns,&(xphn));
				}
				xphn=wtk_tts_xphn_new(info->heap);
				xphn->snt=s;
				xphn->wrd=w;
				xphn->pron=pron;
				xphn->syl=syl;
				xphn->phn=syl->phn->phns[m];
				//wtk_debug("%p=>%p\n",xphn,xphn->phn);
				xphn->sil=wtk_string_cmp_s(xphn->phn,"pau")==0?1:0;
				if(m==syl->phn->nphn-1)
				{
					xphn->bound=w->pron->xsyl[k].bound;
				}else
				{
					xphn->bound=WTK_TTS_BOUND_PHONE;
				}
				//wtk_debug("phn[%.*s]=%d\n",xphn->phn->len,xphn->phn->data,xphn->bound);
				//wtk_debug("bound=%d\n",xphn->bound);
				wtk_array_push2(syl->phns,&(xphn));
				wtk_array_push2(s->phns,&(xphn));
			}
		}
		//exit(0);
	}
	xphn=((wtk_tts_xphn_t**)s->phns->slot)[s->phns->nslot-1];
	if(wtk_string_cmp_s(xphn->phn,"pau")!=0)
	{
		xphn=wtk_tts_xphn_new(info->heap);
		xphn->wrd=NULL;
		xphn->pron=NULL;
		xphn->snt=NULL;
		xphn->syl=NULL;
		xphn->phn=&ps;
		xphn->sil=1;
		xphn->bound=WTK_TTS_BOUND_SEG;
		wtk_array_push2(s->phns,&(xphn));
	}
	else
	{
		//xphn->wrd->sil=1;
		xphn->syl=NULL;
		xphn->wrd=NULL;
		xphn->pron=NULL;
		xphn->snt=NULL;
		xphn->syl=NULL;
		xphn->phn=&ps;
		xphn->sil=1;
		xphn->bound=WTK_TTS_BOUND_SEG;
		s->sil_wrd_end=1;
	}
//		{
//			for(j=0;j<s->phns->nslot;++j)
//			{
//				xphn=((wtk_tts_xphn_t**)s->phns->slot)[j];
//				wtk_debug("v[%d]=%.*s %d\n",j,xphn->phn->len,xphn->phn->data,xphn->bound);
//			}
//		}
	return 0;
}


int wtk_tts_phn_process(wtk_tts_phn_t *phn,wtk_tts_info_t *info,wtk_tts_lab_t *lab)
{
	static wtk_string_t ps=wtk_string("pau");
	static wtk_string_t *pps=&ps;
	static wtk_tts_sylphn_t pau={
			&pps,
			1
	};
	wtk_tts_snt_t **snt,*s;
	wtk_tts_wrd_t **wrds,*w;
	wtk_tts_xsyl_t *syl;
	wtk_tts_wrd_xpron_t *pron;
	wtk_tts_xphn_t *xphn;
	int i,j,k,m;

	snt=(wtk_tts_snt_t**)lab->snts->slot;
	//wtk_debug("n=%d\n",lab->snts->nslot);
	for(i=0;i<lab->snts->nslot;++i)
	{
		s=snt[i];
		if(s->wrds->nslot<=0)
		{
			continue;
		}
		//wtk_debug("i=%d\n",i);
		wrds=(wtk_tts_wrd_t**)s->wrds->slot;
		s->phns=wtk_array_new_h(info->heap,s->wrds->nslot*6,sizeof(void*));
		for(j=0;j<s->wrds->nslot;++j)
		{
			w=wrds[j];
			//wtk_debug("wrd[%d]=[%.*s]\n",j,w->v->len,w->v->data);
			pron=w->pron;
			//wtk_tts_wrd_xpron_print(pron);
			for(k=0,syl=pron->xsyl;k<pron->pron->nsyl;++k,++syl)
			{
				//wtk_debug("syl[%d]=[%.*s]\n",k,syl->syl->v->len,syl->syl->v->data);
				if(wtk_string_cmp_s(syl->syl->v,"pau")==0)
				{
					syl->phn=&pau;
				}else
				{
					syl->phn=wtk_kdict_get(phn->cfg->dict,syl->syl->v->data,syl->syl->v->len);
				}
				syl->phns=wtk_array_new_h(info->heap,syl->phn->nphn,sizeof(void*));
				//wtk_tts_sylphn_print(syl->phn);
				for(m=0;m<syl->phn->nphn;++m)
				{
					//wtk_debug("phn[%d]=%.*s\n",m,syl->phn->phns[m]->len,syl->phn->phns[m]->data);
					if(s->phns->nslot==0 && wtk_string_cmp_s(syl->phn->phns[m],"pau")!=0)
					{
						xphn=wtk_tts_xphn_new(info->heap);
						xphn->wrd=NULL;
						xphn->pron=NULL;
						xphn->syl=NULL;
						xphn->snt=NULL;
						xphn->phn=&ps;
						xphn->sil=1;
						xphn->lab=NULL;
						xphn->bound=WTK_TTS_BOUND_SEG;
						wtk_array_push2(s->phns,&(xphn));
					}
					xphn=wtk_tts_xphn_new(info->heap);
					xphn->snt=s;
					xphn->wrd=w;
					xphn->pron=pron;
					xphn->syl=syl;
					xphn->phn=syl->phn->phns[m];
					//wtk_debug("%p=>%p\n",xphn,xphn->phn);
					xphn->sil=wtk_string_cmp_s(xphn->phn,"pau")==0?1:0;
					if(m==syl->phn->nphn-1)
					{
						xphn->bound=w->pron->xsyl[k].bound;
					}else
					{
						xphn->bound=WTK_TTS_BOUND_PHONE;
					}
					//wtk_debug("phn[%.*s]=%d\n",xphn->phn->len,xphn->phn->data,xphn->bound);
					//wtk_debug("bound=%d\n",xphn->bound);
					wtk_array_push2(syl->phns,&(xphn));
					wtk_array_push2(s->phns,&(xphn));
				}
			}
			//exit(0);
		}
		xphn=((wtk_tts_xphn_t**)s->phns->slot)[s->phns->nslot-1];
		if(wtk_string_cmp_s(xphn->phn,"pau")!=0)
		{
			xphn=wtk_tts_xphn_new(info->heap);
			xphn->wrd=NULL;
			xphn->pron=NULL;
			xphn->snt=NULL;
			xphn->syl=NULL;
			xphn->phn=&ps;
			xphn->sil=1;
			xphn->bound=WTK_TTS_BOUND_SEG;
			wtk_array_push2(s->phns,&(xphn));
		}
		else
		{
			//xphn->wrd->sil=1;
			xphn->syl=NULL;
			xphn->wrd=NULL;
			xphn->pron=NULL;
			xphn->snt=NULL;
			xphn->syl=NULL;
			xphn->phn=&ps;
			xphn->sil=1;
			xphn->bound=WTK_TTS_BOUND_SEG;
			s->sil_wrd_end=1;
		}
//		{
//			for(j=0;j<s->phns->nslot;++j)
//			{
//				xphn=((wtk_tts_xphn_t**)s->phns->slot)[j];
//				wtk_debug("v[%d]=%.*s %d\n",j,xphn->phn->len,xphn->phn->data,xphn->bound);
//			}
//		}
	}

	//exit(0);
	return 0;
}
