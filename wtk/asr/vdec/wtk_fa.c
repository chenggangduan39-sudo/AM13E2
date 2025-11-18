#include "wtk_fa.h"

//static wtk_string_t sil=wtk_string("sil");


wtk_faphn_t* wtk_fawrd_get_phn(wtk_fawrd_t *wrd,int index)
{
	wtk_faphn_t *phn=0;

	if(!wrd->phones || index>= wrd->phones->nslot){goto end;}
	phn=((wtk_faphn_t**)wrd->phones->slot)[index];
end:
	return phn;
}

wtk_fawrd_t* wtk_fawrd_new_h(wtk_heap_t *heap,wtk_string_t *name,int nphn)
{
	wtk_fawrd_t *wrd;

	wrd=(wtk_fawrd_t*)wtk_heap_malloc(heap,sizeof(*wrd));
	wrd->name=name;
	wrd->start=wrd->end=wrd->dur=wrd->score=0;

	wrd->sil=(wtk_string_equal_s(name,"!SENT_START") || wtk_string_equal_s(name,"!SENT_END")||wtk_string_equal_s(name,"sil") || wtk_string_equal_s(name,"sp"));
	wrd->embed=0;
	//wtk_debug("%*.*s %d\n",name->len,name->len,name->data,wrd->sil);
	if(nphn>0)
	{
		wrd->phones=wtk_array_new_h(heap,nphn,sizeof(wtk_faphn_t*));
	}else
	{
		wrd->phones=0;
	}
	return wrd;
}

wtk_faphn_t* wtk_faphn_new_h(wtk_heap_t *heap,wtk_lab_t *lab)
{
	wtk_faphn_t *phn;
	wtk_string_t *name;

	phn=(wtk_faphn_t*)wtk_heap_malloc(heap,sizeof(*phn));
	phn->lab=lab;
	phn->score=0;
	name=lab->name;
	phn->sil=(wtk_string_equal_s(name,"sil") || wtk_string_equal_s(name,"sp"));
	phn->embed=0;
	phn->start=lab->start;
	phn->dur=lab->end-lab->start;
	return phn;
}

double wtk_fawrd_update(wtk_fawrd_t *w)
{
	wtk_faphn_t **phns,*p;
	int j,nphn;

	nphn=wtk_fa_word_nphn(w);
	if(nphn>0)
	{
		phns=(wtk_faphn_t**)w->phones->slot;
		w->start=phns[0]->lab->start;
		w->end=phns[nphn-1]->lab->end;
		w->dur=0;
		for(j=0;j<nphn;++j)
		{
			p=phns[j];
			if(p->sil){continue;}
			w->dur+=p->dur;
		}
		if(w->dur==0)
		{
			w->sil=1;
		}
	}else
	{
		w->dur=w->start=w->end=0;
	}
	return w->dur;
}

void wtk_fa_update(wtk_fa_t *fa)
{
	wtk_fawrd_t **wrds;
	int i,nwrd;

	wrds=(wtk_fawrd_t**)fa->words->slot;
	nwrd=fa->words->nslot;
	if(nwrd>0)
	{
		fa->dur=0;
		for(i=0;i<nwrd;++i)
		{
			fa->dur+=wtk_fawrd_update(wrds[i]);
		}
		if(nwrd>1)
		{
			fa->start=wrds[1]->start;
		}else
		{
			fa->start=0;
		}
		if(fa->valid_wrds>=1)
		{
			fa->end=wrds[fa->valid_wrds-1]->end;
		}else
		{
			fa->end=0;
		}
	}else
	{
		fa->start=fa->end=fa->dur=0;
	}
}

wtk_fa_t* wtk_fa_new_h(wtk_heap_t *heap,wtk_transcription_t *fa_trans,
		wtk_transcription_t* phn_trans,double frame_dur)
{
	wtk_lablist_t *ll;
	wtk_queue_node_t *n;
	wtk_lab_t *lab;
	wtk_fa_t *f;
	wtk_faphn_t *phn;
	wtk_fawrd_t *wrd=0;
	int words;
	int ret=0;
	int i;

//#define	DEBUG_MLF
#ifdef DEBUG_MLF
	wtk_debug("================== FA ==============\n");
	wtk_transcription_print(fa_trans);
	wtk_debug("================== phn =============\n");
	//wtk_transcription_print(phn_trans);
#endif
	f=(wtk_fa_t*)wtk_heap_malloc(heap,sizeof(*f));
	if(fa_trans->forceout || phn_trans->forceout )
	{
		if(fa_trans->forceout)
		{
			if(phn_trans->forceout)
			{
				f->force_out=wtk_both_forceout;
			}else
			{
				f->force_out=wtk_fa_forceout;
			}
		}else if(phn_trans->forceout)
		{
			f->force_out=wtk_phn_forceout;
		}
	}else
	{
		f->force_out=0;
	}
	f->frame_dur=frame_dur*1.0E7;
	f->start=f->end=f->dur=0;
	f->score=0;
	ll=data_offset(fa_trans->lab_queue.pop,wtk_lablist_t,trans_n);
	words=ll->lable_queue.length;
	f->words=wtk_array_new_h(heap,words,sizeof(wtk_fawrd_t*));
	f->n_ref_words=0;
	for(i=0,n=ll->lable_queue.pop;n;n=n->next)
	{
		lab=data_offset(n,wtk_lab_t,lablist_n);
		if(lab->aux_lab[2])
		{
			wrd=wtk_fawrd_new_h(heap,lab->aux_lab[2],4);
			if(wrd->sil==0)
			{
				++f->n_ref_words;
			}
			((wtk_fawrd_t**)wtk_array_push(f->words))[0]=wrd;
			++i;
		}
		if(!wrd){ret=-1;goto end;}
		phn=wtk_faphn_new_h(heap,lab);
		((wtk_faphn_t**)wtk_array_push(wrd->phones))[0]=phn;
	}
	f->valid_wrds=i;
	f->is_forceout=0;
	wtk_fa_update(f);
	wtk_fa_cal_gop(f,phn_trans);
end:
	if(ret!=0){f=0;}
	return f;
}

int wtk_fawrd_nsilphn(wtk_fawrd_t *w)
{
	int i,n;
	wtk_faphn_t **phns,*p;
	int nphn;

	n=0;
	nphn=wtk_fa_word_nphn(w);
	if(nphn>0)
	{
		phns=(wtk_faphn_t**)w->phones->slot;
	}
	for(i=0;i<nphn;++i)
	{
		p=phns[i];
		n+=p->sil;
	}
	return n;
}

int wtk_fa_nsilphn(wtk_fa_t *fa)
{
	int i,n;
	wtk_fawrd_t **wrds,*w;

	n=0;
	wrds=(wtk_fawrd_t**)fa->words->slot;
	for(i=0;i<fa->words->nslot;++i)
	{
		w=wrds[i];
		if(w->sil){continue;}
		n+=wtk_fawrd_nsilphn(w);
	}
	return n;
}

double wtk_fa_calc_phn_gop(wtk_fa_t* fa,wtk_faphn_t *p,wtk_transcription_t *phn_trans)
{
	wtk_lablist_t *ll;
	wtk_lab_t *lab,*l;
	double overlap,delta;

	lab=p->lab;
	// sum of score*duration of overlap
	overlap=wtk_transcription_get_overlap_prob(phn_trans,lab);
	ll=data_offset(phn_trans->lab_queue.pop,wtk_lablist_t,trans_n);
	if(ll)
	{
		l=data_offset(ll->lable_queue.push,wtk_lab_t,lablist_n);
		if(l)
		{
			p->embed=wtk_float_round(lab->end/1E4)<=wtk_float_round(l->end/1E4);
			//wtk_debug("end=%f,%f\n",lab->end,l->end);
		}
		else
		{
			p->embed=0;
		}
	}else
	{
		p->embed=0;
	}
	if(p->embed)
	{
		delta=overlap - lab->score;
		//wtk_debug("delat=%f\n",delta);
		if(delta<0){delta=0;}
		p->score=delta*fa->frame_dur/p->dur;
	}else
	{
		p->score=0;
		delta=0;
	}
	//wtk_debug("embed=%d, %.*s: overlap=%f,delta=%f,score=%f,gop=%f\n",p->embed,lab->name->len,lab->name->data,overlap,delta,p->score,p->gop_score);
//#define DEBUG_SCORE
#ifdef DEBUG_SCORE
	print_data(p->lab->name->data,p->lab->name->len);
	wtk_debug("s=%f,gop=%f,delta=%f,%f,dur=%f,overlab=%f,score=%f\n",p->score,p->gop_score,delta,delta*fa->frame_dur,
			p->dur,overlap,lab->score);
#endif
	return delta;
}

double wtk_fa_calc_wrd_gop(wtk_fa_t *fa,wtk_fawrd_t *w,wtk_transcription_t *phn_trans,double *valid_dur)
{
	wtk_faphn_t **phns,*p;
	int i,nphn;
	double delta=0;
	double dur=0;

	phns=(wtk_faphn_t**)w->phones->slot;
	nphn=w->phones->nslot;
	if(nphn>0)
	{
		w->embed=1;
		for(i=0;i<nphn;++i)
		{
			p=phns[i];
			if(p->sil){continue;}
			delta+=wtk_fa_calc_phn_gop(fa,p,phn_trans);
			if(!p->embed)
			{
				w->embed=0;
			}else
			{
				dur+=p->dur;
			}
		}
		if(w->embed)
		{
			w->score=delta*fa->frame_dur/w->dur;
		}else
		{
			for(i=0;i<nphn;++i)
			{
				p=phns[i];
				if(p->sil){continue;}
				p->score=0;
			}
			w->score=0;
		}
	}else
	{
		w->score=0;
	}
	if(valid_dur)
	{
		*valid_dur=dur;
	}
//	wtk_debug("wrd = %.*s,gop: %f\n",w->name->len, w->name->data, w->gop_score);
	return delta;
}

void wtk_fa_cal_gop(wtk_fa_t *fa,wtk_transcription_t* phn_trans)
{
	wtk_fawrd_t **wrds,*w;
	int i,nwrd;
	double delta=0;
	int nx=0;
	double dur=0,t;

	wrds=(wtk_fawrd_t**)fa->words->slot;
	nwrd=fa->words->nslot;
	for(i=0;i<nwrd;++i)
	{
		w=wrds[i];
		if(w->sil || wtk_fa_word_nphn(w)<=0){continue;}
		delta+=wtk_fa_calc_wrd_gop(fa,w,phn_trans,&t);
		dur+=t;
		if(w->embed)
		{
			++nx;
		}
	}
	//wtk_debug("delta: %f\n",delta);
	if(dur>0)
	{
		fa->score=delta*(fa->frame_dur/dur);
	}else
	{
		fa->score=0;
	}
}

void wtk_fa_get_rec(wtk_fa_t *fa,wtk_strbuf_t *buf)
{
	wtk_fawrd_t **wrds,*w;
	int i;

	wrds=(wtk_fawrd_t**)fa->words->slot;
	wtk_strbuf_reset(buf);
	for(i=0;i<fa->words->nslot;++i)
	{
		w=wrds[i];
		if(w->sil){continue;}
		if(buf->pos>0)
		{
			wtk_strbuf_push_c(buf,' ');
		}
		wtk_strbuf_push(buf,w->name->data,w->name->len);
	}
	wtk_strbuf_push_c(buf,0);
}


void wtk_fa_print(wtk_fa_t *fa)
{
	wtk_fawrd_t **wrds,*w;
	wtk_faphn_t **phns,*p;
	int i,j;

	wrds=(wtk_fawrd_t**)fa->words->slot;
	for(i=0;i<fa->words->nslot;++i)
	{
		w=wrds[i];
		if(w->sil){continue;}
		printf("%*.*s:\t score=%f \n",w->name->len,w->name->len,w->name->data,w->score);
		if(!w->phones){continue;}
		phns=(wtk_faphn_t**)w->phones->slot;
		for(j=0;j<w->phones->nslot;++j)
		{
			p=phns[j];
			if(p->sil){continue;}
			printf("\t%*.*s:\tscore=%f  dur=%.0f\n",p->lab->name->len,p->lab->name->len,p->lab->name->data,p->score,p->dur/fa->frame_dur);
		}
	}
	printf("Overall:\tscore=%f\n",fa->score);
}



