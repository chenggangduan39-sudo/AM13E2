#include "wtk_rec.h"

double wtk_lat_larc_like(wtk_lat_t *lat,wtk_larc_t *la,wtk_dict_word_t *null_word)
{
	return la->aclike*lat->acscale+la->lmlike*lat->lmscale+la->prlike*lat->prscale
	+((!la->end->info.word || la->end->info.word==null_word)?0:lat->wdpenalty);
}

double wtk_lat_larc_lmlike(wtk_lat_t *lat,wtk_larc_t *la,wtk_dict_word_t *null_word)
{
	return la->lmlike*lat->lmscale+((!la->end->info.word || la->end->info.word==null_word)?0:lat->wdpenalty);
}

void wtk_lat_add_nbe(wtk_lat_t *lat,wtk_nbestentry_t* head,wtk_nbestentry_t* best,wtk_lnode_t *ln,wtk_heap_t *heap,wtk_dict_word_t *null_word)
{
	wtk_nbestentry_t	*newNBE,*pos;
	wtk_larc_t *la;
	//wtk_heap_t *heap=rec->local_heap;//rec->heap;
	//wtk_dict_word_t *null_word=rec->dict->null_word;
	double like,score;

	for(la=ln->foll;la;la=la->farc)
	{
		like=wtk_lat_larc_like(lat,la,null_word);
		//wtk_debug("like=%f end=%f ac=%f lm=%f\n",like,la->end->score,la->aclike,la->lmlike);
		if(best)
		{
			//wtk_debug("best=%f\n",best->like);
			like+=best->like;
		}
		score=like+la->end->score;
		//wtk_debug("score=%f like=%f end=%f\n",score,like,la->end->score);
		if(score<LSMALL){continue;}
		newNBE=(wtk_nbestentry_t*)wtk_heap_malloc(heap,sizeof(*newNBE));
		newNBE->score=score;
		newNBE->like=like;
		newNBE->lnode=la->end;
		newNBE->larc=la;
		newNBE->path_prev=best;
		for(pos=head->next;score<pos->score;pos=pos->next);
		newNBE->prev=pos->prev;
		newNBE->next=pos;
		newNBE->prev->next=newNBE->next->prev=newNBE;
	}
}

int wtk_nbe_word_match(wtk_nbestentry_t* cmp,wtk_nbestentry_t* ans)
{
	if(cmp==ans)
	{
		return 1;
	}else if( !cmp || !ans)
	{
		return 0;
	}else if(cmp->larc->end->info.word != ans->larc->end->info.word)
	{
		return 0;
	}else
	{
		return wtk_nbe_word_match(cmp->path_prev,ans->path_prev);
	}
}

wtk_transcription_t* wtk_lat_to_transcription(wtk_lat_t *lat,wtk_heap_t *heap,wtk_dict_t *dict,int N,int trace_model,int trace_state)
{
	wtk_dict_pron_t *pron;
	wtk_dict_word_t *null_word,*word;
	wtk_transcription_t *trans;
	wtk_lab_t *lab;
	wtk_nbestentry_t **ans,*pos,*best,head,tail;
	wtk_queue_node_t *qn;
	wtk_lnode_t *ln;
	wtk_larc_t *la;
	wtk_lalign_t *lal;
	double score,lm,modlk,start,end;
	int *order;
	int i,j,n,naux;
	wtk_lablist_t *ll;
	wtk_string_t *model;
	//float tscale=1.0E7;

	naux=trace_model+trace_state;
	if(naux>0)
	{
		naux=2;
	}
	if(dict)
	{
		null_word=dict->null_word;
	}else
	{
		null_word=NULL;
	}
	ans=(wtk_nbestentry_t**)wtk_heap_malloc(heap,sizeof(wtk_nbestentry_t*)*N);
	--ans;
	for(i=0,ln=lat->lnodes;i<lat->nn;++i,++ln)
	{
		ln->score=ln->foll?LZERO:0;
		ln->n=-1;
	}
	for(i=0,n=0,ln=lat->lnodes;i<lat->nn;++i,++ln)
	{
		if(ln->n==-1)
		{
			wtk_lnode_mark_back(ln,&n);
		}
	}
	order=(int*)wtk_heap_malloc(heap,sizeof(int)*lat->nn);
	for(i=0,ln=lat->lnodes;i<lat->nn;++i,++ln)
	{
		order[ln->n]=i;
	}
	for(i=lat->nn-1;i>0;--i)
	{
		ln=lat->lnodes+order[i];
		for(la=ln->pred;la;la=la->parc)
		{
			score=ln->score+wtk_lat_larc_like(lat,la,null_word);
			//wtk_debug("i=%d,score=%f,raw=%f,ln=%f\n",i,score,la->start->score,ln->score);
			//store score to start of arc
			if(score>la->start->score)
			{
				la->start->score=score;
			}
		}
	}
	head.next=&tail;head.prev=0;
	tail.next=0;tail.prev=&head;
	head.score=tail.score=LZERO;
	//add best path head node.
	for(i=0,ln=lat->lnodes;i<lat->nn;++i,++ln)
	{
		if(ln->pred){continue;}
		wtk_lat_add_nbe(lat,&head,0,ln,heap,null_word);
	}
	for(n=0,best=head.next;n<N && best!=&tail;best=head.next)
	{
		best->next->prev=best->prev;
		best->prev->next=best->next;
		if(best->lnode->foll)
		{
			wtk_lat_add_nbe(lat,&head,best,best->lnode,heap,null_word);
			continue;
		}
		for(i=1;i<=n;++i)
		{
			if(wtk_nbe_word_match(best,ans[i]))
			{
				best=0;
				break;
			}
		}
		if(best)
		{
			//wtk_nbestentry_print(best);
			//exit(0);
			ans[++n]=best;
		}
	}
	trans=wtk_transcription_new_h(heap);
	for(i=1;i<=n;++i)
	{
#ifdef DEBUG_STATE
		states=models=0;
		for(pos=ans[i]->path_prev;pos;pos=pos->path_prev)
		{
			if(pos->larc->end.info.word==null_word){continue;}
			if(!pos->larc->lalign)
			{
				states=models=0;
				break;
			}
			for(j=0,lal=pos->larc->lalign;j<pos->larc->nalign;++j,++lal)
			{
				if(lal->trace_state<0)
				{
					models=1;
				}else if(lal->trace_state>0)
				{
					states=1;
				}
			}
		}
		naux=states+models;
#else
		//naux=rec->;
#endif
		ll=wtk_lablist_new_h(heap,naux);
		if(naux>0)
		{
			for(pos=ans[i]->path_prev;pos;pos=pos->path_prev)
			{
				la=pos->larc;
				lal=la->lalign;
				word=la->end->info.word;
				if(word==null_word)
				{
					continue;
				}
				lm=wtk_lat_larc_lmlike(lat,la,null_word);
				start=la->start->time*1.0E7;//(int)(la->start->time*1.0E7+0.5);
				//wtk_debug("%f,%f,%d\n",start,la->start->time,(int)(start+0.5));
				model=0;modlk=0.0;
				qn=0;
				//wtk_debug("word: %.*s,score=%f,lm=%f,a=%f\n",word->name->len,word->name->data,la->score,la->lmlike,la->aclike);
				for(j=0,lal=la->lalign;j<la->nalign;++j,++lal)
				{
					//wtk_debug("%*.*s:\t%d,%d\n",lal->name->len,lal->name->len,lal->name->data,j,lal->state);
					if(lal->state<0 && trace_state)
					{
						model=lal->name;
						modlk=lal->like;
						lab=0;
						continue;
					}
					lab=wtk_lab_new_h(heap,ll->max_aux_lab);
					lab->name=lal->name;
					lab->score=lal->like;
					end=start+lal->dur*1.0E7;//(int)(start+lal->dur*1.0E7+0.5);
					lab->start=start;lab->end=end;
					start=end;
					lab->aux_lab[naux]=word?word->name:0;
					lab->aux_score[naux]=lm;
					word=0;lm=0;
					lab->aux_lab[1]=model;
					lab->aux_score[1]=modlk;
					modlk=0;model=0;
					if(!qn)
					{
						wtk_queue_push_front(&(ll->lable_queue),&(lab->lablist_n));
						qn=&(lab->lablist_n);
					}else
					{
						wtk_queue_insert_to(&(ll->lable_queue),qn,&(lab->lablist_n));
						qn=&(lab->lablist_n);
					}
				}
			}
		}else
		{
			for(pos=ans[i]->path_prev;pos;pos=pos->path_prev)
			{
				la=pos->larc;
				for(pron=la->end->info.word->pron_list;pron;pron=pron->next)
				{
					if(pron->pnum==la->end->v)
					{
						break;
					}
				}
				if(!pron || !pron->outsym || pron->outsym->len<=0)
				{
					continue;
				}
				if(la->end->info.word==dict->null_word)
				{
					continue;
				}
				lab=wtk_lab_new_h(heap,ll->max_aux_lab);
				lab->name=pron->outsym;
				lab->score=wtk_lat_larc_like(lat,la,null_word);
				lab->start=la->start->time*1.0e7;
				lab->end=la->end->time*1.0e7;
				wtk_queue_push_front(&(ll->lable_queue),&(lab->lablist_n));
			}
		}
		wtk_queue_push(&(trans->lab_queue),&(ll->trans_n));
	}
	return trans;
}

wtk_transcription_t* wtk_rec_transcription_from_lat(wtk_rec_t *rec,wtk_lat_t *lat)
{
	wtk_transcription_t* trans=0;

	trans=wtk_lat_to_transcription(lat,rec->local_heap,rec->dict,rec->cfg->nbest,rec->cfg->model,rec->cfg->state);
	if(trans)
	{
		trans->forceout=rec->is_forceout;
	}
	return trans;
}
