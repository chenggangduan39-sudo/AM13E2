#include <ctype.h>
#include "wtk_polyphn.h" 

void wtk_polyphn_soft_tone_snt(wtk_polyphn_t *p,wtk_tts_snt_t *s);
wtk_polyphn_t* wtk_polyphn_new(wtk_polyphn_cfg_t *cfg)
{
	wtk_polyphn_t *p;

	p=(wtk_polyphn_t*)wtk_malloc(sizeof(wtk_polyphn_t));
	p->cfg=cfg;
	p->buf1=wtk_strbuf_new(256,1);
	p->buf2=wtk_strbuf_new(256,1);
	//p->buf3=wtk_strbuf_new(256,1);
	p->use_defpron = cfg->use_defpron;
	if (p->use_defpron)
		p->defpron=wtk_defpron_new();
	else
		p->defpron=NULL;
	return p;
}

int wtk_polyphn_bytes(wtk_polyphn_t* p)
{
  int bytes;

  bytes=sizeof(*p);
  bytes+=wtk_strbuf_bytes(p->buf1);
  bytes+=wtk_strbuf_bytes(p->buf2);
  if(p->defpron){
	  bytes+=wtk_defpron_bytes(p->defpron);
  }
  //wtk_debug("bytes=%f M\n",bytes*1.0/(1024*1024));

  return bytes;
}
void wtk_polyphn_delete(wtk_polyphn_t *p)
{
	if (p->defpron){
		wtk_defpron_delete(p->defpron);
	}
	wtk_strbuf_delete(p->buf1);
	wtk_strbuf_delete(p->buf2);
	//wtk_strbuf_delete(p->buf3);
	wtk_free(p);
}

void wtk_polyphn_reset(wtk_polyphn_t *p)
{
	if (p->defpron){
		wtk_defpron_reset(p->defpron);
	}
}

wtk_tts_wrd_pron_t* wtk_polyphn_process_poly(wtk_polyphn_t *p,wtk_tts_info_t *info,wtk_tts_wrd_t **wrds,int nwrd,int index,wtk_polyphn_wrd_t *w)
{
	wtk_strbuf_t *wrd=p->buf1;
	wtk_strbuf_t *pos=p->buf2;
	//wtk_strbuf_t *idx=p->buf3;
	wtk_queue_node_t *qn,*qn2;
	wtk_polyphn_expr_item_t *item;
	wtk_polyphn_expr_item_if_t *xif;
	wtk_tts_wrd_t *cur;
	int b;
	int i,n;

	//wtk_polyphn_wrd_print(w);
	//exit(0);
	//wtk_debug("len=%d [%.*s]\n",w->item_q.length,w->wrd->len,w->wrd->data);
	for(qn=w->item_q.pop;qn;qn=qn->next)
	{
		item=data_offset2(qn,wtk_polyphn_expr_item_t,q_n);
		b=1;
		for(qn2=item->if_q.pop;qn2;qn2=qn2->next)
		{
			xif=data_offset2(qn2,wtk_polyphn_expr_item_if_t,q_n);
			//wtk_debug("start=%d end=%d\n",xif->start,xif->end);
			//wtk_polyphn_expr_item_if_print(xif);
			i=index+xif->start;
			//wtk_debug("i=%d\n",i);
			if(i<0){b=0;break;}
			n=xif->end-xif->start+i;
			//wtk_debug("i=%d n=%d %d/%d\n",i,n,xif->start,xif->end);
			wtk_strbuf_reset(wrd);
			wtk_strbuf_reset(pos);
			//wtk_strbuf_reset(idx);
			for(;i<=n && i<nwrd;++i)
			{
				cur=wrds[i];
				wtk_strbuf_push(wrd,cur->v->data,cur->v->len);
				wtk_strbuf_push(pos,cur->pos->data,cur->pos->len);
			}
//			//for idx by dmd 20180406
//			if(index==0) wtk_strbuf_push_s(idx,"s");
//			else if(index==nwrd-1) wtk_strbuf_push_s(idx,"e");
//			else{
//				if (wrds[index+1]->sil==1){
//					wtk_strbuf_push_s(idx,"me");
//				}else if(wrds[index-1]->sil==1){
//					wtk_strbuf_push_s(idx,"ms");
//				}else
//					wtk_strbuf_push_s(idx,"m");
//			}
			//wtk_debug("[%.*s] [%.*s] [%.*s]\n",wrd->pos,wrd->data,pos->pos,pos->data,wrds[nwrd-1]->pos->len,wrds[nwrd-1]->pos->data);
			//exit(0);
			b=wtk_polyphn_expr_item_if_match(xif,wrd->data,wrd->pos,pos->data,pos->pos);
//			b=wtk_polyphn_expr_item_if_match2(xif,wrd->data,wrd->pos,pos->data,pos->pos, idx->data, idx->pos);
			if(!b)
			{
				break;
			}
		}
		if(b)
		{
//			wtk_debug("b=%d ki=%d\n",b,ki);
//			wtk_polyphn_expr_item_print(item);
//			wtk_debug("[%.*s]\n",wrds[index]->pos->len,wrds[index]->pos->data);
			wrds[index]->pos=item->pos;
//			wtk_debug("[%.*s]\n",wrds[index]->pos->len,wrds[index]->pos->data);
//			exit(0);
			return item->pron;
		}
	}
	return NULL;
}

void wtk_polyphn_change_tone33_2(wtk_polyphn_t *p,wtk_tts_lab_t *lab)
{
	wtk_tts_snt_t **snt,*s;
	wtk_tts_wrd_t **wrds,*w;
	wtk_tts_xsyl_t *xsyl;
	int i,j;

	snt=(wtk_tts_snt_t**)lab->snts->slot;
	for(i=0;i<lab->snts->nslot;++i)
	{
		s=snt[i];
		wrds=(wtk_tts_wrd_t**)s->wrds->slot;
		for(j=0;j<s->wrds->nslot-1;)
		{
			w=wrds[j];
			if(!w->pron->pron || w->pron->pron->nsyl>1)
			{
				++j;
				continue;
			}
			xsyl=w->pron->xsyl;
			//wtk_debug("[%.*s]=%d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->syl->tone);
			if(xsyl->tone==3)
			{
				w=wrds[j+1];
				if(w->pron->pron && w->pron->pron->nsyl==1 && w->pron->xsyl->tone==3)
				{
					xsyl->tone=2;
					//wtk_debug("change %.*s-%d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->tone);
					j+=2;
				}else
				{
					++j;
				}
			}else
			{
				++j;
			}
		}
	}
}

void wtk_polyphn_change_tone33(wtk_polyphn_t *p,wtk_tts_lab_t *lab)
{
	wtk_tts_snt_t **snt,*s;
	wtk_tts_xsyl_t **syls;
	wtk_tts_xsyl_t *xsyl;
	int i,j;

	snt=(wtk_tts_snt_t**)lab->snts->slot;
	for(i=0;i<lab->snts->nslot;++i)
	{
		s=snt[i];
		if(!s->syls||s->syls->nslot==0){continue;}
		syls=(wtk_tts_xsyl_t**)s->syls->slot;
		for(j=0;j<s->syls->nslot-1;)
		{
			xsyl=syls[j];
			if(xsyl->tone==3 && syls[j+1]->tone==3)
			{
				xsyl->tone=2;
				//wtk_debug("set tone [%.*s]\n",xsyl->syl->v->len,xsyl->syl->v->data);
				++j;
			}else
			{
				++j;
			}
		}
	}
}

/*
 * http://zh.wiktionary.org/zh/%E9%99%84%E5%BD%95:%E2%80%9C%E4%B8%80%E2%80%9D%E3%80%81%E2%80%9C%E4%B8%8D%E2%80%9D%E7%9A%84%E5%8F%98%E8%B0%83
*/
void wtk_polyphn_change_tone_yi2(wtk_polyphn_t *p,wtk_tts_lab_t *lab)
{
	wtk_tts_snt_t **snt,*s;
	wtk_tts_wrd_t **wrds,*w;
	wtk_tts_xsyl_t *xsyl;
	int i,j;

	snt=(wtk_tts_snt_t**)lab->snts->slot;
	for(i=0;i<lab->snts->nslot;++i)
	{
		s=snt[i];
		wrds=(wtk_tts_wrd_t**)s->wrds->slot;
		for(j=0;j<s->wrds->nslot-1;)
		{
			w=wrds[j];
			if(!w->pron->pron || w->pron->pron->nsyl>1)
			{
				++j;
				continue;
			}
			xsyl=w->pron->xsyl;
			//wtk_debug("[%.*s]=%d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->syl->tone);
			if((wtk_string_cmp_s(w->v,"一")==0))
			{
				w=wrds[j+1];
				if(w->pron->pron && w->pron->pron->nsyl==1)
				{
					if(j>0 && (wtk_string_cmp(wrds[j-1]->v,w->v->data,w->v->len)==0) && wrds[j-1]->pron->pron
							&& (wrds[j-1]->pron->pron->nsyl==1))
					{
						xsyl->tone=5;
					}else
					{
						if(w->pron->xsyl->tone==4)
						{
							xsyl->tone=2;
						}else
						{
							xsyl->tone=4;
						}
					}
					j+=2;
				}else
				{
					++j;
				}
			}else
			{
				++j;
			}
		}
	}
	//exit(0);
}


void wtk_polyphn_change_tone_yi(wtk_polyphn_t *p,wtk_tts_lab_t *lab)
{
	wtk_tts_snt_t **snt,*s;
	wtk_tts_xsyl_t **syls;
	wtk_tts_xsyl_t *xsyl;
	int i,j;

	snt=(wtk_tts_snt_t**)lab->snts->slot;
	for(i=0;i<lab->snts->nslot;++i)
	{
		s=snt[i];
		if(!s->syls){continue;}
		syls=(wtk_tts_xsyl_t**)s->syls->slot;
		for(j=0;j<s->syls->nslot;)
		{
			xsyl=syls[j];
			//wtk_debug("%.*s-%d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->tone);
			if(wtk_string_cmp_s(xsyl->wrd->v,"一")==0)
			{
				if(j==s->syls->nslot-1)
				{
					//句末
					xsyl->tone=1;
				}else
				{
					if(j>0 && (wtk_string_cmp(syls[j-1]->syl->v,syls[j+1]->syl->v->data,syls[j+1]->syl->v->len)==0))
					{
						//处在叠声动词之间
						xsyl->tone=5;
					}else if(syls[j+1]->tone==4)
					{
						//处在第四声（去声）字前
						xsyl->tone=2;
					}else if(syls[j+1]->tone==1 || syls[j+1]->tone==2 ||syls[j+1]->tone==3 )
					{
						//处在第一声（阴平）、第二声（阳平）、第三声（上声）字前
						xsyl->tone=4;
						//wtk_debug("tone=%d/%d\n",xsyl->syl->tone,xsyl->tone);
					}
				}
				++j;
			}else
			{
				++j;
			}
			//wtk_debug("%.*s-%d/%d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->tone,xsyl->syl->tone);
		}
	}
}

void wtk_polyphn_change_tone_bu2(wtk_polyphn_t *p,wtk_tts_lab_t *lab)
{
	wtk_tts_snt_t **snt,*s;
	wtk_tts_wrd_t **wrds,*w;
	wtk_tts_xsyl_t *xsyl;
	int i,j;

	snt=(wtk_tts_snt_t**)lab->snts->slot;
	for(i=0;i<lab->snts->nslot;++i)
	{
		s=snt[i];
		wrds=(wtk_tts_wrd_t**)s->wrds->slot;
		for(j=0;j<s->wrds->nslot;)
		{
			w=wrds[j];
			if(!w->pron->pron || w->pron->pron->nsyl>1)
			{
				++j;
				continue;
			}
			xsyl=w->pron->xsyl;
			//wtk_debug("[%.*s]=%d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->syl->tone);
			if((wtk_string_cmp_s(w->v,"不")==0))
			{
				if(j==s->wrds->nslot-1)
				{
					xsyl->tone=4;
					++j;
				}else
				{
					w=wrds[j+1];
					if(w->pron->pron && w->pron->pron->nsyl==1)
					{
						if(j>0 && (wtk_string_cmp(wrds[j-1]->v,w->v->data,w->v->len)==0) && wrds[j-1]->pron->pron
								&& (wrds[j-1]->pron->pron->nsyl==1))
						{
							xsyl->tone=5;
						}else if(w->pron->xsyl->tone==4)
						{
							xsyl->tone=2;
						}else
						{
							xsyl->tone=4;
						}
						j+=2;
					}else
					{
						xsyl->tone=4;
						j+=2;
					}
				}
			}else
			{
				++j;
			}
		}
	}
	//exit(0);
}

void wtk_polyphn_change_tone_bu(wtk_polyphn_t *p,wtk_tts_lab_t *lab)
{
	wtk_tts_snt_t **snt,*s;
	wtk_tts_xsyl_t **syls;
	wtk_tts_xsyl_t *xsyl;
	int i,j;

	snt=(wtk_tts_snt_t**)lab->snts->slot;
	for(i=0;i<lab->snts->nslot;++i)
	{
		s=snt[i];
		if(!s->syls){continue;}
		syls=(wtk_tts_xsyl_t**)s->syls->slot;
		for(j=0;j<s->syls->nslot;)
		{
			xsyl=syls[j];
			//wtk_debug("%.*s-%d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->tone);
			//if(wtk_string_cmp_s(xsyl->syl->v,"bu")==0)
			//if(wtk_string_cmp_s(xsyl->wrd->v,"不")==0)
			if(wtk_string_cmp_s(xsyl->syl->v,"bu")==0 && wtk_string_str_s(xsyl->wrd->v, "不") >=0)
			{
				if(j==s->syls->nslot-1)
				{
					//句末
					xsyl->tone=4;
				}else
				{
					if(j>0 && (wtk_string_cmp(syls[j-1]->syl->v,syls[j+1]->syl->v->data,syls[j+1]->syl->v->len)==0))
					{
						xsyl->tone=5;
					}else if(syls[j+1]->tone==4)
					{
						xsyl->tone=2;
					}else if(syls[j+1]->tone==1 || syls[j+1]->tone==2 ||syls[j+1]->tone==3)
					{
						xsyl->tone=4;
					}
				}
				++j;
			}else
			{
				++j;
			}
			//wtk_debug("%.*s-%d/%d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->tone,xsyl->syl->tone);
		}
	}
}

void wtk_polyphn_soft_tone(wtk_polyphn_t *p,wtk_tts_lab_t *lab)
{
	wtk_tts_snt_t **snt,*s;
	int i;

	snt=(wtk_tts_snt_t**)lab->snts->slot;
	for(i=0;i<lab->snts->nslot;++i)
	{
		s=snt[i];
		if(!s->syls){continue;}
		wtk_polyphn_soft_tone_snt(p, s);
	}
}

void wtk_polyphn_change_tone(wtk_polyphn_t *p,wtk_tts_lab_t *lab)
{
	//wtk_debug("======================\n");
	wtk_polyphn_change_tone33(p,lab);
	wtk_polyphn_change_tone_yi(p,lab);
	wtk_polyphn_change_tone_bu(p,lab);
	wtk_polyphn_soft_tone(p,lab);
	//wtk_tts_lab_print_syl(lab);
	//exit(0);
}

void wtk_polyphn_change_tone33_snt(wtk_polyphn_t *p,wtk_tts_snt_t *s)
{
	wtk_tts_xsyl_t **syls;
	wtk_tts_xsyl_t *xsyl;
	int j;

	if(!s->syls || s->syls->nslot<=0){return;}
	syls=(wtk_tts_xsyl_t**)s->syls->slot;
	for(j=0;j<s->syls->nslot-1;)
	{
		xsyl=syls[j];
		if(xsyl->tone==3 && syls[j+1]->tone==3)
		{
			xsyl->tone=2;
			if(wtk_string_cmp_s(syls[j+1]->syl->v,"hao")==0)
			{
				//只要小米对我好, 我就喜欢他!
				//syls[j+1]->tone=5;            //注释: 影响"身体好",发音 dmd123. 20160816
				
				//wtk_debug("set tone [%.*s]\n",xsyl->syl->v->len,xsyl->syl->v->data);
			}
			//wtk_debug("set tone [%.*s]\n",xsyl->syl->v->len,xsyl->syl->v->data);
			//wtk_debug("set tone [%.*s]\n",xsyl->syl->v->len,xsyl->syl->v->data);
			++j;
		}else
		{
			++j;
		}
	}
}

void wtk_polyphn_change_tone_yi_snt(wtk_polyphn_t *p,wtk_tts_snt_t *s)
{
	wtk_tts_xsyl_t **syls;
	wtk_tts_xsyl_t *xsyl;
	int j;

	if(!s->syls){return;}
	syls=(wtk_tts_xsyl_t**)s->syls->slot;
	for(j=0;j<s->syls->nslot;)
	{
		xsyl=syls[j];
		//wtk_debug("%.*s-%d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->tone);
		if(wtk_string_cmp_s(xsyl->wrd->v,"一")==0)
		{
			if(j==s->syls->nslot-1)
			{
				//句末
				xsyl->tone=1;
			}else
			{
				if(j>0 && (wtk_string_cmp(syls[j-1]->syl->v,syls[j+1]->syl->v->data,syls[j+1]->syl->v->len)==0))
				{
					//处在叠声动词之间
					xsyl->tone=5;
				}else if(syls[j+1]->tone==4)
				{
					//处在第四声（去声）字前
					xsyl->tone=2;
				}else if(syls[j+1]->tone==1 || syls[j+1]->tone==2 ||syls[j+1]->tone==3 )
				{
					//处在第一声（阴平）、第二声（阳平）、第三声（上声）字前
					xsyl->tone=4;
				}
			}
			++j;
		}else
		{
			++j;
		}
		//wtk_debug("%.*s-%d/%d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->tone,xsyl->syl->tone);
	}
}

void wtk_polyphn_change_tone_bu_snt(wtk_polyphn_t *p,wtk_tts_snt_t *s)
{
	wtk_tts_xsyl_t **syls;
	wtk_tts_xsyl_t *xsyl;
	int j;

	if(!s->syls){return;}
	syls=(wtk_tts_xsyl_t**)s->syls->slot;
	for(j=0;j<s->syls->nslot;)
	{
		xsyl=syls[j];
		//wtk_debug("%.*s-%d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->tone);
		// dmd123 20160816
		// here still leave a bug. syl and word should in same position.
		// it'll absolutely correct when "不" and other "bu" don't appear in same that condition should be not appear in linguistics.
		if(wtk_string_cmp_s(xsyl->syl->v,"bu")==0 && wtk_string_str_s(xsyl->wrd->v, "不") >=0)
		{
			if(j==s->syls->nslot-1)
			{
				//sentence end
				xsyl->tone=4;
			}else
			{
				if(j>0 && (wtk_string_cmp(syls[j-1]->syl->v,syls[j+1]->syl->v->data,syls[j+1]->syl->v->len)==0))
				{
					//pl: hao3-bu5-hao3
					xsyl->tone=5;
				}else if(syls[j+1]->tone==4)
				{
					//pl: bu2-bian4
					xsyl->tone=2;
				}else if(syls[j+1]->tone==1 || syls[j+1]->tone==2 ||syls[j+1]->tone==3)
				{
					//pl: bu4-duo1, bu4-tong2,bu4-xiang3
					xsyl->tone=4;
				}
			}
			++j;
		}else
		{
			++j;
		}
		//wtk_debug("%.*s-%d/%d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->tone,xsyl->syl->tone);
	}
}

void wtk_polyphn_soft_tone_snt(wtk_polyphn_t *p,wtk_tts_snt_t *s)
{
	wtk_tts_xsyl_t **syls;
	wtk_tts_xsyl_t *xsyl;
	wtk_string_t *v1,*v2;
	int i, m, n;

	if(!s->syls || !p->cfg->use_soft_end || !p->cfg->soft_wrds_end){return;}
	syls=(wtk_tts_xsyl_t**)s->syls->slot;
	n = s->syls->nslot;
	v1=v2=0;
	while(n>1){
		xsyl=syls[n-1];
		//printf("syl=%.*s\n", xsyl->syl->v->len, xsyl->syl->v->data);
		if (wtk_string_cmp_s(xsyl->syl->v, "pau")==0){
			n--;
		}else{
			break;
		}
	}
	if (n > 1){
		xsyl=syls[n-1];
		v1 = xsyl->wrd->v;
		v2 = syls[n-2]->wrd->v;
		//printf("v1=%.*s v2=%.*s\n",v1->len, v1->data, v2->len, v2->data);
		if (xsyl->tone!=5 && wtk_array_str_in(p->cfg->soft_wrds_end,v1->data,v1->len)){
			if (wtk_string_cmp2(v1,v2)!=0 && wtk_string_cmp_s(v2, "pau")!=0){   //can't be continue and same words and.
				xsyl->tone=5;
			}
		}
	}

	//whole sentence for soft tone exclude last word. by dmd at 20180326
	for(i=1,m=n-1; i < m; i++){
		xsyl=syls[i];
		if (wtk_string_cmp_s(syls[i+1]->syl->v, "pau")==0){
			v1 = xsyl->wrd->v;
			v2 = syls[i-1]->wrd->v;
			if (xsyl->tone!=5 && wtk_array_str_has(p->cfg->soft_wrds_end,v1->data,v1->len)){
				if (wtk_string_cmp2(v1,v2)!=0 && wtk_string_cmp_s(v2, "pau")!=0){ //can't be continue and same words.
					xsyl->tone=5;
				}
			}
		}
	}
}

void wtk_polyphn_change_tone_snt(wtk_polyphn_t *p,wtk_tts_snt_t *s)
{
	wtk_polyphn_change_tone33_snt(p,s);
	wtk_polyphn_change_tone_yi_snt(p,s);
	wtk_polyphn_change_tone_bu_snt(p,s);
	wtk_polyphn_soft_tone_snt(p,s);
}

int wtk_polyphn_process_snt(wtk_polyphn_t *p,wtk_tts_info_t *info,wtk_tts_snt_t *s)
{
	wtk_tts_wrd_t **wrds,*w;
	wtk_tts_xsyl_t *xsyl;
	wtk_polyphn_wrd_t *pw;
	int j,k;
	wtk_tts_wrd_pron_t *pron;

	wrds=(wtk_tts_wrd_t**)s->wrds->slot;
	s->syls=wtk_array_new_h(info->heap,s->wrds->nslot*2,sizeof(void*));
	for(j=0;j<s->wrds->nslot;++j)
	{
		w=wrds[j];
		if(!w->pron->pron)
		{
			continue;
		}
		pron=NULL;
		//wtk_debug("[%.*s]\n",w->v->len,w->v->data);
		if (p->use_defpron){
			pron=wtk_defpron_find(p->defpron, w->v->data, w->v->len);
		}
		if (!pron){
			//if(w->pron->pron->nsyl==1)// && 0)
			if(w->pron->pron->nsyl==1 || w->pron->pron->nsyl==2)
			{
				pw=wtk_polyphn_lex_find(p->cfg->polyphn,w->v->data,w->v->len);
				if(pw)
				{
					pron=wtk_polyphn_process_poly(p,info,wrds,s->wrds->nslot,j,pw);
				}
			}
			if(!pron)
			{
				pron=w->pron->pron;
			}
		}

		w->pron->xsyl=(wtk_tts_xsyl_t*)wtk_heap_malloc(info->heap,sizeof(wtk_tts_xsyl_t)*pron->nsyl);
		//wtk_debug("bound=[%.*s] %d pos=[%.*s]\n",w->v->len,w->v->data,w->bound,w->pos->len,w->pos->data);
		for(k=0,xsyl=w->pron->xsyl;k<pron->nsyl;++k,++xsyl)
		{
			xsyl->syl=pron->syls+k;
			xsyl->tone=xsyl->syl->tone;
			xsyl->wrd=w;
			if(k==pron->nsyl-1)
			{
				xsyl->bound=w->bound;
			}else
			{
				xsyl->bound=WTK_TTS_BOUND_SYL;
			}
			//wtk_debug("[%.*s-%d]=%d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->tone,xsyl->bound);
			wtk_array_push2(s->syls,&xsyl);
			//wtk_debug("bound=[%.*s:%d] %d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->tone,xsyl->bound);
		}
	}
	wtk_polyphn_change_tone_snt(p,s);
	//exit(0);
	//wtk_tts_snt_print_syl(s);
	return 0;
}


void wtk_polyphn_print(wtk_polyphn_t *p)
{
}

int wtk_polyphn_process(wtk_polyphn_t *p,wtk_tts_info_t *info,wtk_tts_lab_t *lab)
{
	wtk_tts_snt_t **snt,*s;
	wtk_tts_wrd_t **wrds,*w;
	wtk_tts_xsyl_t *xsyl;
	wtk_polyphn_wrd_t *pw;
	int i,j,k;
	wtk_tts_wrd_pron_t *pron;

	snt=(wtk_tts_snt_t**)lab->snts->slot;
	for(i=0;i<lab->snts->nslot;++i)
	{
		s=snt[i];
		wrds=(wtk_tts_wrd_t**)s->wrds->slot;
		s->syls=wtk_array_new_h(info->heap,s->wrds->nslot*2,sizeof(void*));
		for(j=0;j<s->wrds->nslot;++j)
		{
			w=wrds[j];
			if(!w->pron->pron)
			{
				continue;
			}
			pron=NULL;
			if(w->pron->pron->nsyl==1) // && 0)         //dmd123 2016.08.16
			{
				pw=wtk_polyphn_lex_find(p->cfg->polyphn,w->v->data,w->v->len);
				if(pw)
				{
					pron=wtk_polyphn_process_poly(p,info,wrds,s->wrds->nslot,j,pw);
				}
			}
			if(!pron)
			{
				pron=w->pron->pron;
			}
			w->pron->xsyl=(wtk_tts_xsyl_t*)wtk_heap_malloc(info->heap,sizeof(wtk_tts_xsyl_t)*pron->nsyl);
			//wtk_debug("bound=[%.*s] %d pos=[%.*s]\n",w->v->len,w->v->data,w->bound,w->pos->len,w->pos->data);
			for(k=0,xsyl=w->pron->xsyl;k<pron->nsyl;++k,++xsyl)
			{
				xsyl->syl=pron->syls+k;
				xsyl->tone=xsyl->syl->tone;
				xsyl->wrd=w;
				if(k==pron->nsyl-1)
				{
					xsyl->bound=w->bound;
				}else
				{
					xsyl->bound=WTK_TTS_BOUND_SYL;
				}
				//wtk_debug("[%.*s-%d]=%d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->tone,xsyl->bound);
				wtk_array_push2(s->syls,&xsyl);
				//wtk_debug("bound=[%.*s] %d\n",xsyl->syl->v->len,xsyl->syl->v->data,xsyl->bound);
			}
		}
	}
	wtk_polyphn_change_tone(p,lab);
	//exit(0);

	return 0;
}


