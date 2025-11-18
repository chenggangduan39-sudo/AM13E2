#include "wtk_tts_def.h" 

void wtk_tts_xphn_print(wtk_tts_xphn_t *phn)
{
	wtk_debug("[%.*s]\n",phn->phn->len,phn->phn->data);
}

void wtk_tts_sylphn_print(wtk_tts_sylphn_t *p)
{
	int i;

	for(i=0;i<p->nphn;++i)
	{
		wtk_debug("v[%d]=%.*s\n",i,p->phns[i]->len,p->phns[i]->data);
	}
}

wtk_tts_wrd_t* wtk_tts_wrd_new(wtk_heap_t *heap,wtk_string_t *v)
{
	wtk_tts_wrd_t *wrd;

	wrd=(wtk_tts_wrd_t*)wtk_heap_malloc(heap,sizeof(wtk_tts_wrd_t));
	wrd->v=wtk_heap_dup_string(heap,v->data,v->len);
	wrd->pos=NULL;
	wrd->pron=NULL;
	wrd->sil=0;
	wrd->index=0;
	wrd->valid_pos=0;
	return wrd;
}

void wtk_tts_wrd_pron_print(wtk_tts_wrd_pron_t *p)
{
	wtk_tts_syl_t *s;
	int i;

	if(p == NULL)
		return;

	if(p->next)
	{
		wtk_tts_wrd_pron_print(p->next);
	}
	for(i=0,s=p->syls; s!=NULL && i<p->nsyl;++i,++s)
	{
		printf("v[%d]=%.*s:%d\n",i,s->v->len,s->v->data,s->tone);
	}
}

int wtk_tts_wrd_nsyl(wtk_tts_wrd_t *wrd)
{
	return wrd->pron->pron->nsyl;
}

void wtk_tts_wrd_print(wtk_tts_wrd_t *wrd)
{
	//wtk_tts_wrd_pron_t *pron;
	wtk_tts_xsyl_t *xsyl;
	int i;

	wtk_debug("%.*s\n",wrd->v->len,wrd->v->data);
	if(wrd->pos)
	{
		printf("pos: %.*s\n",wrd->pos->len,wrd->pos->data);
	}
	printf("bound: %d\n",wrd->bound);
	if (!wrd->pron || !wrd->pron->xsyl){
		wtk_debug("Warming: pron/xsyl is null\n");
		return;
	}
	printf("nsyl: %d\n",wrd->pron->pron->nsyl);
	for(i=0,xsyl=wrd->pron->xsyl;i<wrd->pron->pron->nsyl;++i,++xsyl)
	{
		printf("v[%d]: syl: %.*s\n",i,xsyl->syl->v->len,xsyl->syl->v->data);
		printf("v[%d]: bound: %d\n",i,xsyl->bound);
	}
	/*
	pron=wrd->pron->pron;
	while(pron)
	{
		wtk_tts_wrd_pron_print(pron);
		pron=pron->next;
	}*/
}


wtk_tts_snt_t* wtk_tts_snt_new(wtk_heap_t *heap,int hint)
{
	wtk_tts_snt_t *s;

	s=(wtk_tts_snt_t*)wtk_heap_malloc(heap,sizeof(wtk_tts_snt_t));
	s->chars=wtk_array_new_h(heap,hint,sizeof(void*));
	s->wrds=NULL;
	s->type=WTK_TTS_SNT_NORM;
	s->snt=NULL;
	s->phns=NULL;
	s->syls=NULL;
//	s->n_valid_wrd=0;
//	s->n_valid_syl=0;
	s->sil_wrd_end=0;
	//s->nsyl=0;
	s->is_ctx_pick=0;
	return s;
}

char* wtk_tts_snt_type_to_str(wtk_tts_snt_t *snt)
{
	static char* type[]={
			"NORM",
			"QUES",
			"QUES2",
			"SIGH"
	};

	return type[snt->type];
}

void wtk_tts_snt_print(wtk_tts_snt_t *snt)
{
	wtk_string_t **strs;
	wtk_tts_wrd_t **wrds;
	//char *s;
	int i;

	//s=wtk_tts_snt_type_to_str(snt);
	if(snt->wrds)
	{
		wrds=(wtk_tts_wrd_t**)snt->wrds->slot;
		for(i=0;i<snt->wrds->nslot;++i)
		{
			wtk_debug("v[%d]=[%.*s] %.*s\n",i,wrds[i]->v->len,wrds[i]->v->data,wrds[i]->pos->len,wrds[i]->pos->data);
		}
	}else
	{
		strs=(wtk_string_t **)snt->chars->slot;
		for(i=0;i<snt->chars->nslot;++i)
		{
			wtk_debug("v[%d]=[%.*s]\n",i,strs[i]->len,strs[i]->data);
		}
	}
}

wtk_string_t* wtk_tts_snt_get_sylphn(wtk_tts_snt_t *s,int idx)
{
	wtk_tts_wrd_t **wrds,*w;
	wtk_tts_wrd_xpron_t *pron;
	wtk_tts_xsyl_t *syl;
	int i,k,j;
	int cnt=0;

	wrds=(wtk_tts_wrd_t**)s->wrds->slot;
	for(i=0;i<s->wrds->nslot;++i)
	{
		w=wrds[i];
		pron=w->pron;
		for(k=0,syl=pron->xsyl;k<pron->pron->nsyl;++k,++syl)
		{
			for(j=0;j<syl->phn->nphn;++j)
			{
				if(cnt==idx)
				{
					return syl->phn->phns[j];
				}
				++cnt;
			}
		}
	}
	return NULL;
}

int wtk_tts_snt_get_nsyl(wtk_tts_snt_t *s)
{
	return s->syls->nslot;
	//return s->n_valid_syl;
//	wtk_tts_xsyl_t *syl;
//	int cnt;
//
//	//return s->syls->nslot;
//	cnt=s->syls->nslot;
//	if(cnt>0)
//	{
//		syl=((wtk_tts_xsyl_t**)s->syls->slot)[cnt-1];
//		if(wtk_string_cmp_s(syl->syl->v,"pau")==0)
//		{
//			cnt-=1;
//		}
//	}
//	return cnt;
}

int wtk_tts_snt_get_nwrd(wtk_tts_snt_t *s)
{
	return s->wrds->nslot;
	//return s->n_valid_wrd;
//	wtk_tts_wrd_t *w;
//	int cnt;
//
//	cnt=s->wrds->nslot;
//	if(cnt>0)
//	{
//		w=((wtk_tts_wrd_t**)(s->wrds->slot))[cnt-1];
//		if(w->sil)
//		{
//			--cnt;
//		}
//	}
//	//wtk_debug("cnt=%d\n",cnt);
//	//exit(0);
//	return cnt;
}

wtk_tts_xsyl_t* wtk_tts_snt_get_syl(wtk_tts_snt_t *snt,int idx)
{
	if(idx<0 || idx>(snt->syls->nslot-1))
	{
		return NULL;
	}
	return ((wtk_tts_xsyl_t**)snt->syls->slot)[idx];
}

int wtk_tts_snt_get_syl_snt_pos(wtk_tts_snt_t *snt,wtk_tts_xsyl_t *syl)
{
	wtk_tts_xsyl_t **syls;
	int i;

	syls=(wtk_tts_xsyl_t**)snt->syls->slot;
	for(i=0;i<snt->syls->nslot;++i)
	{
		if(syls[i]==syl)
		{
			return i;
		}
	}
	return -1;
}

wtk_tts_xsyl_t* wtk_tts_snt_get_nxt_syl(wtk_tts_snt_t *snt,wtk_tts_xsyl_t *syl)
{
	int pos;

	pos=wtk_tts_snt_get_syl_snt_pos(snt,syl);
	if(pos<0){return NULL;}
	return wtk_tts_snt_get_syl(snt,pos+1);
}

wtk_tts_wrd_t* wtk_tts_snt_get_wrd(wtk_tts_snt_t *snt,int index)
{
	if(index<0 || index>snt->wrds->nslot-1)
	{
		return NULL;
	}
	return ((wtk_tts_wrd_t**)snt->wrds->slot)[index];
}

void wtk_tts_lab_print_bound(wtk_tts_lab_t *lab)
{
	wtk_tts_snt_t **snt,*s;
	wtk_tts_xphn_t **phns,*phn;
	int i,j;

	snt=(wtk_tts_snt_t**)lab->snts->slot;
	//wtk_debug("n=%d\n",lab->snts->nslot);
	for(i=0;i<lab->snts->nslot;++i)
	{
		s=snt[i];
		phns=(wtk_tts_xphn_t**)s->phns->slot;
		for(j=0;j<s->phns->nslot;++j)
		{
			phn=phns[j];
			wtk_debug("v[%d]: %.*s %d\n",j,phn->phn->len,phn->phn->data,phn->bound);
		}
	}
}

void wtk_tts_lab_print_syl(wtk_tts_lab_t *lab)
{
	wtk_tts_snt_t **snt,*s;
	wtk_tts_xsyl_t **syls;
	wtk_tts_xsyl_t *xsyl;
	int i,j;

	snt=(wtk_tts_snt_t**)lab->snts->slot;
	//wtk_debug("n=%d\n",lab->snts->nslot);
	for(i=0;i<lab->snts->nslot;++i)
	{
		s=snt[i];
		syls=(wtk_tts_xsyl_t**)s->syls->slot;
		syls=(wtk_tts_xsyl_t**)s->syls->slot;
		for(j=0;j<s->syls->nslot;++j)
		{
			xsyl=syls[j];
			wtk_debug("%.*s-%d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->tone);
		}
	}
}

void wtk_tts_snt_print_syl(wtk_tts_snt_t *s)
{
	wtk_tts_xsyl_t **syls;
	wtk_tts_xsyl_t *xsyl;
	int j;

	syls=(wtk_tts_xsyl_t**)s->syls->slot;
	syls=(wtk_tts_xsyl_t**)s->syls->slot;
	for(j=0;j<s->syls->nslot;++j)
	{
		xsyl=syls[j];
		wtk_debug("%.*s-%d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->tone);
	}
}

void wtk_tts_lab_print(wtk_tts_lab_t *lab)
{
	wtk_tts_snt_t **snt,*s;
	wtk_tts_xphn_t **phns,*phn;
	int i,j;

	snt=(wtk_tts_snt_t**)lab->snts->slot;
	//wtk_debug("n=%d\n",lab->snts->nslot);
	for(i=0;i<lab->snts->nslot;++i)
	{
		s=snt[i];
		phns=(wtk_tts_xphn_t**)s->phns->slot;
		for(j=0;j<s->phns->nslot;++j)
		{
			phn=phns[j];
			wtk_debug("v[%d]: %.*s\n",j,phn->lab->len,phn->lab->data);
		}
	}
}

wtk_tts_xphn_t* wtk_tts_xphn_new(wtk_heap_t *heap)
{
	wtk_tts_xphn_t *xphn;

	xphn=(wtk_tts_xphn_t*)wtk_heap_malloc(heap,sizeof(wtk_tts_xphn_t));
	xphn->wrd=NULL;
	xphn->pron=NULL;
	xphn->syl=NULL;
	xphn->snt=NULL;
	xphn->phn=NULL;
	xphn->sil=1;
	xphn->lab=NULL;
	xphn->bound=WTK_TTS_BOUND_SEG;
	return xphn;
}

int wtk_tts_xphn_is_last_in_syl(wtk_tts_xphn_t *p)
{
	if(!p->syl)
	{
		return 1;
	}
	//wtk_debug("%.*s\n",p->wrd->v->len,p->wrd->v->data);
	if(p->syl!=(p->wrd->pron->xsyl+p->wrd->pron->pron->nsyl-1))
	{
		return 0;
	}
	return (((wtk_tts_xphn_t**)p->syl->phns->slot)[p->syl->phns->nslot-1]==p)?1:0;
}

wtk_tts_bound_t wtk_tts_xphn_get_syl_left_bound(wtk_tts_xphn_t *phn)
{
	if(!phn->syl)
	{
		return WTK_TTS_BOUND_SEG;
	}
	wtk_tts_xphn_print(phn);
	exit(0);
	return 0;
}

wtk_tts_bound_t wtk_tts_xphn_get_syl_right_bound(wtk_tts_xphn_t *phn)
{
	if(!phn->syl)
	{
		return WTK_TTS_BOUND_SEG;
	}
	wtk_tts_xphn_print(phn);
	exit(0);
	return 0;
}

wtk_tts_bound_t wtk_tts_xphn_get_wrd_left_bound(wtk_tts_xphn_t *phn)
{
	if(!phn->wrd)
	{
		return WTK_TTS_BOUND_SEG;
	}
	wtk_tts_xphn_print(phn);
	exit(0);
	return 0;
}

wtk_tts_bound_t wtk_tts_xphn_get_wrd_right_bound(wtk_tts_xphn_t *phn)
{
	if(!phn->wrd)
	{
		return WTK_TTS_BOUND_SEG;
	}
	wtk_tts_xphn_print(phn);
	exit(0);
	return 0;
}

/*
const char * get_rel_pos(int i, int sum)
{
    if(i <= 0 || sum <= 0)
        return "nil";
    else if(sum == 1)
        return "A";
    else if(sum == 2)
    {
        if(i == 1)
            return "H";
        else
            return "T";
    }
    else if(i/(float)sum <= 1/(float)3)
        return "H";
    else if(i/(float)sum <= 2/(float)3)
        return "M";
    else
        return "T";

    return "nil";
}*/

static wtk_string_t phn_pos[]={
		wtk_string("nil"),
		wtk_string("A"),
		wtk_string("M"),
		wtk_string("H"),
		wtk_string("T"),
};

wtk_string_t* wtk_tts_get_rel(int i, int sum)
{
    if(i <= 0 || sum <= 0)
    {
        return phn_pos+0;	//"nil";
    }else if(sum == 1)
    {
        return phn_pos+1;	//"A"
    }else if(sum == 2)
    {
        if(i == 1)
        {
            return phn_pos+3;//"H";
        }else
        {
            return phn_pos+4;//"T";
        }
    }
// remedy by dmd at 2018.09.28
//// follow old
//    else if(i/(float)sum <= 1/(float)3)
//    {
//        return phn_pos+3;//"H";
//    }else if(i/(float)sum <= 2/(float)3)
//    {
//        return phn_pos+2;//"M";
//    }else
//    {
//        return phn_pos+4;//"T";
//    }
////  follow new
    else{
//    	printf("pos i=%d sum=%d\n",i,sum);
    	if (i==1)
    		return phn_pos+3;//"H";
    	else if (i==sum)
    		return phn_pos+4;//"T";
    	else
    		return phn_pos+2;//"M";
    }

    return phn_pos+0;
}


wtk_string_t* wtk_tts_xphn_get_phn_in_syl_pos(wtk_tts_xphn_t *phn)
{
	wtk_tts_xphn_t **phns;
	int i,index;

	if(!phn->syl || phn->sil)
	{
		return &(phn_pos[0]);
	}
	phns=(wtk_tts_xphn_t**)phn->syl->phns->slot;
	index=-1;
	for(i=0;i<phn->syl->phns->nslot;++i)
	{
		//wtk_debug("phn=%p %p\n",phn,phns[i]);
		//wtk_debug("[%.*s]=[%.*s]\n",phn->phn->len,phn->phn->data,phns[i]->phn->len,phns[i]->phn->data);
		if(phn==phns[i])
		{
			index=i;
			break;
		}
	}
	if(index<0)
	{
		wtk_debug("error: phn not in syl(%d)\n",phn->syl->phns->nslot);
		return NULL;
	}
	return wtk_tts_get_rel(index+1,phn->syl->phns->nslot);
}

wtk_string_t* wtk_tts_xphn_get_syl_in_wrd_pos(wtk_tts_xphn_t *phn)
{
	wtk_tts_xsyl_t *syl;
	int i,index;

	if(!phn->syl || phn->sil)
	{
		return &(phn_pos[0]);
	}
	index=-1;
	for(i=0,syl=phn->wrd->pron->xsyl;i<phn->wrd->pron->pron->nsyl;++i,++syl)
	{
		if(syl==phn->syl)
		{
			index=i;
			break;
		}
	}
	if(index<0)
	{
		wtk_debug("error: syl not in word\n");
		return NULL;
	}
	return wtk_tts_get_rel(index+1,phn->wrd->pron->pron->nsyl);
}

wtk_string_t* wtk_tts_xphn_get_syl_in_phase_pos(wtk_tts_xphn_t *phn)
{
	if(!phn->syl || phn->sil)
	{
		return &(phn_pos[0]);
	}
	return &(phn_pos[0]);
	//return NULL;
}

wtk_string_t* wtk_tts_xphn_get_syl_in_seg_pos(wtk_tts_xphn_t *phn)
{
	int index;

	if(!phn->syl || phn->sil)
	{
		return &(phn_pos[0]);
	}
	index=wtk_tts_snt_get_syl_snt_pos(phn->snt,phn->syl);
	if(index<0)
	{
		return &(phn_pos[0]);
	}
	if (phn->snt->sil_wrd_end==1){     //if tail exist symbol, should drop
		return wtk_tts_get_rel(index+1,phn->snt->syls->nslot-1);
	}else
	    return wtk_tts_get_rel(index+1,phn->snt->syls->nslot);
}

wtk_string_t* wtk_tts_xphn_get_wrd_in_phase_pos(wtk_tts_xphn_t *phn)
{
	if(!phn->syl || phn->sil)
	{
		return &(phn_pos[0]);
	}
	return &(phn_pos[0]);
	//return NULL;
}

wtk_string_t* wtk_tts_xphn_get_wrd_in_seg_pos(wtk_tts_xphn_t *phn)
{
	if(!phn->wrd || phn->sil)
	{
		return &(phn_pos[0]);
	}
	//wtk_debug("index=%d/%d\n",phn->wrd->index,phn->snt->wrds->nslot);
	if (phn->snt->sil_wrd_end==1){     //if tail exist symbol, should drop
		return wtk_tts_get_rel(phn->wrd->index,phn->snt->wrds->nslot-1);
	}else
		return wtk_tts_get_rel(phn->wrd->index,phn->snt->wrds->nslot);
}

wtk_string_t* wtk_tts_xphn_get_phase_in_seg_pos(wtk_tts_xphn_t *phn)
{
	if(!phn->syl || phn->sil)
	{
		return &(phn_pos[0]);
	}
	return &(phn_pos[0]);
}

wtk_tts_wrd_t* wtk_tts_xphn_get_next_word(wtk_tts_xphn_t *phn)
{
	return NULL;
}

int wtk_tts_xphn_get_forward_syl_pos(wtk_tts_xphn_t *phn)
{
	wtk_tts_xphn_t **phns;
	int i;

	phns=(wtk_tts_xphn_t**)phn->syl->phns->slot;
	for(i=0;i<phn->syl->phns->nslot;++i)
	{
		if(phns[i]==phn)
		{
			return i+1;
		}
	}
	wtk_debug("error: phn is not in syl\n");
	return -1;
}


int wtk_tts_xphn_get_backward_syl_pos(wtk_tts_xphn_t *phn)
{
	wtk_tts_xphn_t **phns;
	int i,j;

	phns=(wtk_tts_xphn_t**)phn->syl->phns->slot;
	for(j=0,i=phn->syl->phns->nslot-1;i>=0;--i,++j)
	{
		if(phns[i]==phn)
		{
			return j+1;
		}
	}
	wtk_debug("error: phn is not in syl\n");
	return -1;
}

static wtk_string_t tts_chn_pre[]={
	wtk_string("b"),
	wtk_string("c"),
	wtk_string("ch"),
	wtk_string("d"),
	wtk_string("f"),
	wtk_string("g"),
	wtk_string("h"),
	wtk_string("j"),
	wtk_string("k"),
	wtk_string("l"),
	wtk_string("m"),
	wtk_string("n"),
	wtk_string("p"),
	wtk_string("q"),
	wtk_string("r"),
	wtk_string("s"),
	wtk_string("sh"),
	wtk_string("t"),
	wtk_string("w"),
	wtk_string("x"),
	wtk_string("y"),
	wtk_string("z"),
	wtk_string("zh")
};

int wtk_tts_is_chnpre(wtk_string_t *v)
{
	int i;

	for(i=0;i<sizeof(tts_chn_pre)/sizeof(wtk_string_t);++i)
	{
		if(wtk_string_cmp(v,tts_chn_pre[i].data,tts_chn_pre[i].len)==0)
		{
			return 1;
		}
	}
	return 0;
}

int wtk_tts_xphn_is_shengmu(wtk_tts_xphn_t *phn)
{
	int ret;

	ret=wtk_tts_is_chnpre(phn->phn);
	if(ret==0){return 0;}
	ret=wtk_tts_xphn_get_forward_syl_pos(phn);
	return ret==1?1:0;
}


static wtk_string_t tts_vowel[]={
	wtk_string("aa"),
	wtk_string("aar"),
	wtk_string("ae"),
	wtk_string("ah"),
	wtk_string("ao"),
	wtk_string("ar"),
	wtk_string("arn"),
	wtk_string("ax"),
	wtk_string("ay"),
	wtk_string("ea"),
	wtk_string("ee"),
	wtk_string("eh"),
	wtk_string("ei"),
	wtk_string("er"),
	wtk_string("ern"),
	wtk_string("ib"),
	wtk_string("ih"),
	wtk_string("il"),
	wtk_string("iy"),
	wtk_string("iz"),
	wtk_string("izh"),
	wtk_string("oe"),
	wtk_string("oh"),
	wtk_string("or"),
	wtk_string("orn"),
	wtk_string("ub"),
	wtk_string("ul"),
	wtk_string("ur"),
	wtk_string("uw"),
	wtk_string("vl"),
	wtk_string("vw"),
//support shengyunmu
	wtk_string("a"),
	wtk_string("i"),
	wtk_string("ii"),
	wtk_string("iii"),
	wtk_string("u"),
	wtk_string("e"),
	wtk_string("ea"),
	wtk_string("o"),
	wtk_string("v"),
	wtk_string("ic"),
	wtk_string("ih"),
	wtk_string("er"),
	wtk_string("ai"),
	wtk_string("ei"),
	wtk_string("ao"),
	wtk_string("ou"),
	wtk_string("ia"),
	wtk_string("ie"),
	wtk_string("ua"),
	wtk_string("uo"),
	wtk_string("ve"),
	wtk_string("iao"),
	wtk_string("iou"),
	wtk_string("uai"),
	wtk_string("uei"),
	wtk_string("an"),
	wtk_string("ian"),
	wtk_string("uan"),
	wtk_string("van"),
	wtk_string("en"),
	wtk_string("in"),
	wtk_string("uen"),
	wtk_string("vn"),
	wtk_string("ang"),
	wtk_string("iang"),
	wtk_string("uang"),
	wtk_string("eng"),
	wtk_string("ing"),
	wtk_string("ueng"),
	wtk_string("ong"),
	wtk_string("iong")

};

int wtk_tts_is_vowel(wtk_string_t *v)
{
	int i;

	for(i=0;i<sizeof(tts_vowel)/sizeof(wtk_string_t);++i)
	{
		if(wtk_string_cmp(v,tts_vowel[i].data,tts_vowel[i].len)==0)
		{
			return 1;
		}
	}
	return 0;
}


static wtk_string_t  tts_zero_init[]={
	wtk_string("aa"),
	wtk_string("aar"),
	wtk_string("ae"),
	wtk_string("ah"),
	wtk_string("ao"),
	wtk_string("ar"),
	wtk_string("arn"),
	wtk_string("ax"),
	wtk_string("ay"),
	wtk_string("ea"),
	wtk_string("ei"),
	wtk_string("er"),
	wtk_string("ern"),
	wtk_string("ih"),
	wtk_string("iy"),
	wtk_string("oe"),
	wtk_string("oh"),
	wtk_string("or"),
	wtk_string("orn"),
	wtk_string("ur"),
	wtk_string("uw"),
	wtk_string("vl"),
	wtk_string("vw"),
	wtk_string("w"),
	wtk_string("y"),
//support shengyunmu
	wtk_string("a"),
	wtk_string("i"),
	wtk_string("ii"),
	wtk_string("iii"),
	wtk_string("u"),
	wtk_string("e"),
	wtk_string("ea"),
	wtk_string("o"),
	wtk_string("v"),
	wtk_string("ic"),
	wtk_string("ih"),
	wtk_string("er"),
	wtk_string("ai"),
	wtk_string("ei"),
	wtk_string("ao"),
	wtk_string("ou"),
	wtk_string("ia"),
	wtk_string("ie"),
	wtk_string("ua"),
	wtk_string("uo"),
	wtk_string("ve"),
	wtk_string("iao"),
	wtk_string("iou"),
	wtk_string("uai"),
	wtk_string("uei"),
	wtk_string("an"),
	wtk_string("ian"),
	wtk_string("uan"),
	wtk_string("van"),
	wtk_string("en"),
	wtk_string("in"),
	wtk_string("uen"),
	wtk_string("vn"),
	wtk_string("ang"),
	wtk_string("iang"),
	wtk_string("uang"),
	wtk_string("eng"),
	wtk_string("ing"),
	wtk_string("ueng"),
	wtk_string("ong"),
	wtk_string("iong")
};

int wtk_tts_is_zero_init(wtk_string_t *v)
{
	int i;

	for(i=0;i<sizeof(tts_zero_init)/sizeof(wtk_string_t);++i)
	{
		if(wtk_string_cmp(v,tts_zero_init[i].data,tts_zero_init[i].len)==0)
		{
			return 1;
		}
	}
	return 0;
}

static wtk_string_t  tts_erhua[]={
    wtk_string("er"),
    wtk_string("ar"),
    wtk_string("or"),
    wtk_string("orn"),
    wtk_string("ern"),
    wtk_string("arn"),
    wtk_string("ur")
};

int wtk_tts_is_erhua(wtk_string_t *v)
{
	int i;

	for(i=0;i<sizeof(tts_erhua)/sizeof(wtk_string_t);++i)
	{
		if(wtk_string_cmp(v,tts_erhua[i].data,tts_erhua[i].len)==0)
		{
			return 1;
		}
	}
	return 0;
}

int wtk_tts_is_erhua2(wtk_tts_sylphn_t *phn)
{
	int i;

	for(i=0;i<sizeof(tts_erhua)/sizeof(wtk_string_t);++i)
	{
		if(phn->nphn > 1 &&(wtk_string_cmp(phn->phns[phn->nphn-1],tts_erhua[i].data,tts_erhua[i].len)==0 || wtk_str_end_with_s(phn->phns[phn->nphn-1]->data,phn->phns[phn->nphn-1]->len,"r")==0))
		{
			return 1;
		}
	}
	return 0;
}

int wtk_tts_xphn_get_syl_snt_pos(wtk_tts_xphn_t *phn)
{
	wtk_tts_xsyl_t **syls;
	int i;

	syls=(wtk_tts_xsyl_t**)phn->snt->syls->slot;
	for(i=0;i<phn->snt->syls->nslot;++i)
	{
		if(syls[i]==phn->syl)
		{
			return i;
		}
	}
	return -1;
}


int wtk_tts_xsyl_get_forward_wrd_pos(wtk_tts_xsyl_t *syl)
{
	wtk_tts_xsyl_t *ts;
	int i;

	for(i=0,ts=syl->wrd->pron->xsyl;i<syl->wrd->pron->pron->nsyl;++i,++ts)
	{
		if(syl==ts)
		{
			return i+1;
		}
	}
	wtk_debug("error: syl not in word\n");
	return -1;
}

int wtk_tts_xsyl_get_backward_wrd_pos(wtk_tts_xsyl_t *syl)
{
	int pos;

	pos=wtk_tts_xsyl_get_forward_wrd_pos(syl);
	return syl->wrd->pron->pron->nsyl-pos+1;
}

void wtk_tts_xsyl_print(wtk_tts_xsyl_t *syl)
{
}


void wtk_tts_wrd_xpron_print(wtk_tts_wrd_xpron_t *pron)
{
	// wtk_debug("bound=%d\n",pron->bound);
	wtk_tts_wrd_pron_print(pron->pron);
	wtk_tts_xsyl_print(pron->xsyl);
}

void wtk_tts_segsnt_print(wtk_tts_lab_t *lab)
{
	int i = 0,n = 0;
    wtk_tts_snt_t **snts = NULL,*snt = NULL;
    n = lab->snts->nslot;
    snts = lab->snts->slot;
    for(i = 0; i < n; ++i){
        snt = snts[i];
        printf("snt[%d]:[%.*s] \n",i,snt->snt->len,snt->snt->data);
    }
}

void wtk_tts_lab_segwrd_print(wtk_tts_lab_t *lab)
{
	int i = 0,n = 0;
	int j = 0,k = 0;
	wtk_tts_snt_t **snts = NULL,*snt = NULL;
	wtk_tts_wrd_t **wrds,*wrd;
	n = lab->snts->nslot;
	snts = lab->snts->slot;

	for(i = 0; i<n;++i){
		snt = snts[i];
		k = snt->wrds->nslot;
		wrds = snt->wrds->slot;
		for(j = 0; j < k; j++){
			wrd = wrds[j];
			printf("%.*s\n",wrd->v->len,wrd->v->data);
			wtk_tts_wrd_xpron_print(wrd->pron);
		}
	}
}