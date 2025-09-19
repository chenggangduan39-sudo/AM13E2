#include "wtk_tts_parser.h" 

wtk_tts_parser_t* wtk_tts_parser_new(wtk_tts_parser_cfg_t *cfg,wtk_rbin2_t *rbin)
{
	wtk_tts_parser_t *p;

	p=(wtk_tts_parser_t*)wtk_malloc(sizeof(wtk_tts_parser_t));
	p->cfg=cfg;
	p->heap=wtk_heap_new(4096);
	p->buf=wtk_strbuf_new(256,1);
	p->norm=wtk_tts_norm_new(&(cfg->norm),rbin);
	wtk_tts_segsnt_init(&(p->segsnt),&(cfg->segsnt));
	wtk_tts_segwrd_init(&(p->segwrd),&(cfg->segwrd),rbin);
	if (cfg->use_pos)
		wtk_tts_pos_init(&(p->pos),&(cfg->pos),rbin);
	p->polyphn=wtk_polyphn_new(&(cfg->polyphn));
	wtk_tts_phn_init(&(p->phn),&(cfg->phn));
	wtk_tts_parser_reset(p);
	return p;
}
int wtk_tts_parser_bytes(wtk_tts_parser_t *p)
{
	int bytes;

	bytes=wtk_heap_bytes(p->heap);
	bytes+=wtk_strbuf_bytes(p->buf);
	bytes+=wtk_tts_norm_bytes(p->norm);
	bytes+=wtk_polyphn_bytes(p->polyphn);
	//wtk_debug("bytes=%f M\n",bytes*1.0/(1024*1024));
//	{
//		double m;
//
//		m=wtk_proc_mem();
//		wtk_debug("m=%f\n",m);
//	}
	//getchar();
	//exit(0);
	return bytes;
}
void wtk_tts_parser_delete(wtk_tts_parser_t *p)
{
	wtk_tts_norm_delete(p->norm);
	wtk_strbuf_delete(p->buf);
	if (p->cfg->use_pos)
		wtk_tts_pos_clean(&(p->pos));
	wtk_tts_segsnt_clean(&(p->segsnt));
	wtk_tts_segwrd_clean(&(p->segwrd));
	wtk_polyphn_delete(p->polyphn);
	wtk_tts_phn_clean(&(p->phn));
	wtk_heap_delete(p->heap);
	wtk_free(p);
}


void wtk_tts_parser_reset(wtk_tts_parser_t *p)
{
	wtk_tts_norm_reset(p->norm);
	if (p->cfg->use_pos)
		wtk_tts_pos_reset(&(p->pos));
	wtk_tts_segsnt_reset(&(p->segsnt));
	wtk_tts_segwrd_reset(&(p->segwrd));
	wtk_polyphn_reset(p->polyphn);
	wtk_tts_phn_reset(&(p->phn));
	wtk_heap_reset(p->heap);
	p->lab=NULL;
}

//static char* ps[]={
//		"nil^nil-pau+w=ao@1-1&nil-nil/A:nil_nil_nil/B:nil_1@1_1&nil#nil$nil/C:nil-2-3/D:nil_nil/E:nil_1!1_5/F:o_1/G:6-5@0/H:5&5@5@5&5!5/I:nil+nil&nil+nil!nil!nil#nil",
//		"nil^pau-w+ao=z@1-2&0-0/A:nil_1_nil/B:nil_2@1_1&3#1$0/C:nil-3-4/D:nil_1/E:o_1!2_4/F:o_1/G:6-5@0/H:5&p@5@1&5!1/I:H+A&nil+H!nil!H#nil",
//		"pau^w-ao+z=ay@2-1&1-1/A:nil_1_nil/B:nil_2@1_1&3#1$0/C:nil-3-4/D:nil_1/E:o_1!2_4/F:o_1/G:6-5@0/H:p&1@5@1&5!1/I:T+A&nil+H!nil!H#nil",
//		"w^ao-z+ay=ib@1-3&0-0/A:nil_2_3/B:nil_3@1_1&4#0$0/C:nil-2-1/D:o_1/E:o_1!3_3/F:n_2/G:6-5@0/H:1&p@1@1&1!1/I:H+A&nil+M!nil!M#nil",
//		"ao^z-ay+ib=s@2-2&1-1/A:nil_2_3/B:nil_3@1_1&4#0$0/C:nil-2-1/D:o_1/E:o_1!3_3/F:n_2/G:6-5@0/H:p&p@1@1&1!1/I:M+A&nil+M!nil!M#nil",
//		"z^ay-ib+s=uw@3-1&1-1/A:nil_2_3/B:nil_3@1_1&4#0$0/C:nil-2-1/D:o_1/E:o_1!3_3/F:n_2/G:6-5@0/H:p&1@1@1&1!1/I:T+A&nil+M!nil!M#nil",
//		"ay^ib-s+uw=zh@1-2&0-0/A:nil_3_4/B:nil_2@1_2&1#0$0/C:nil-3-1/D:o_1/E:n_2!4_2/F:nil_1/G:6-5@0/H:1&p@1@0&1!5/I:H+H&nil+T!nil!T#nil",
//		"ib^s-uw+zh=oh@2-1&1-1/A:nil_3_4/B:nil_2@1_2&1#0$0/C:nil-3-1/D:o_1/E:n_2!4_2/F:nil_1/G:6-5@0/H:p&0@1@0&1!5/I:T+H&nil+T!nil!T#nil",
//		"s^uw-zh+oh=ub@1-3&0-0/A:nil_2_1/B:nil_3@2_1&1#0$0/C:nil-1-nil/D:o_1/E:n_2!4_2/F:nil_1/G:6-5@0/H:0&p@0@5&1!5/I:H+T&nil+T!nil!T#nil",
//		"uw^zh-oh+ub=pau@2-2&1-1/A:nil_2_1/B:nil_3@2_1&1#0$0/C:nil-1-nil/D:o_1/E:n_2!4_2/F:nil_1/G:6-5@0/H:p&p@0@5&1!5/I:M+T&nil+T!nil!T#nil",
//		"zh^oh-ub+pau=nil@3-1&1-1/A:nil_2_1/B:nil_3@2_1&1#0$0/C:nil-1-nil/D:o_1/E:n_2!4_2/F:nil_1/G:6-5@0/H:p&5@0@5&1!5/I:T+T&nil+T!nil!T#nil",
//		"oh^ub-pau+nil=nil@1-1&nil-nil/A:nil_3_1/B:nil_1@1_1&nil#nil$nil/C:nil-nil-nil/D:n_2/E:nil_1!5_1/F:nil_nil/G:6-5@0/H:5&5@5@5&5!5/I:nil+nil&nil+nil!nil!nil#nil",
//};

int wtk_tts_parser_finish_snt(wtk_tts_parser_t *p,wtk_tts_snt_t *s)
{
	wtk_tts_wrd_t **wrds,*wrd;
	wtk_tts_xphn_t **phns,*phn;
	wtk_tts_xsyl_t *syl;
	wtk_strbuf_t * buf=p->buf;
	int j;
	int nsyl,vi,v1,nwrd;
	wtk_tts_bound_t bound_pl,bound_pr,bound_syll,bound_sylr,bound_wrdl,bound_wrdr;
	wtk_string_t *v;

	{
		if(s->wrds->nslot<=0)
		{
			return 0;
		}
		//wtk_tts_snt_print(s);
		wrds=(wtk_tts_wrd_t**)s->wrds->slot;
		nsyl=wtk_tts_snt_get_nsyl(s);
		nwrd=wtk_tts_snt_get_nwrd(s);
		if(s->sil_wrd_end)           // current snt only with pau? dmd
		{
			nwrd+=1;
			nsyl+=1;
		}else                        // current snt contain words and first/last pau? dmd
		{
			nwrd+=2;
			nsyl+=2;
		}
		phns=(wtk_tts_xphn_t**)s->phns->slot;
		for(j=0;j<s->phns->nslot;++j)
		{
			wtk_strbuf_reset(buf);
			if(j>=2)
			{
				phn=phns[j-2];
				wtk_strbuf_push(buf,phn->phn->data,phn->phn->len);
			}else
			{
				wtk_strbuf_push_s(buf,"nil");
			}
			wtk_strbuf_push_c(buf,'^');
			if(j>=1)
			{
				phn=phns[j-1];
				wtk_strbuf_push(buf,phn->phn->data,phn->phn->len);
			}else
			{
				wtk_strbuf_push_s(buf,"nil");
			}
			wtk_strbuf_push_c(buf,'-');
			phn=phns[j];
			//wtk_debug("[%.*s] [%.*s]\n",phn->phn->len,phn->phn->data,phn->syl?phn->syl->wrd->v->len:0,phn->syl?phn->syl->wrd->v->data:"");
//			if(phn->wrd)
//			{
//				wtk_debug("[%.*s:%.*s]\n",phn->phn->len,phn->phn->data,phn->wrd->v->len,phn->wrd->v->data);
//			}
			wtk_strbuf_push(buf,phn->phn->data,phn->phn->len);
			wtk_strbuf_push_c(buf,'+');
			if(j<s->phns->nslot-1)
			{
				phn=phns[j+1];
				wtk_strbuf_push(buf,phn->phn->data,phn->phn->len);
			}else
			{
				wtk_strbuf_push_s(buf,"nil");
			}
			//wtk_debug("%d/%d\n",bound_pl,bound_pr);
			//exit(0);
			wtk_strbuf_push_c(buf,'=');
			if((j+2)<(s->phns->nslot))
			{
				//wtk_debug("j=%d/%d\n",j,s->phns->nslot-2);
				phn=phns[j+2];
				if(phn)
				{
					wtk_strbuf_push(buf,phn->phn->data,phn->phn->len);
				}
			}else
			{
				wtk_strbuf_push_s(buf,"nil");
			}
			wtk_strbuf_push_c(buf,'@');
			phn=phns[j];
			//wtk_debug("[%.*s]\n",phn->phn->len,phn->phn->data);
			//current phone forward position in syllable
			//current phone backward position in syllable
			if(!phn->sil)
			{
				//wtk_debug("found %.*s\n",buf->pos,buf->data);
				vi=wtk_tts_xphn_get_forward_syl_pos(phn);
				v1=wtk_tts_xphn_get_backward_syl_pos(phn);
				//wtk_debug("vi=%d/%d\n",vi,v1);
				wtk_strbuf_push_f(buf,"%d-%d",vi,v1);
				//exit(0);
			}else
			{
				wtk_strbuf_push_s(buf,"1-1");
			}
			wtk_strbuf_push_c(buf,'&');
			//whether current phone is shengmu, values:0/1(1->true)
			//whether current phone is vowel, values:0/1 (1-true),define is diff form coder ???
			if(!phn->sil)
			{
				vi=wtk_tts_xphn_is_shengmu(phn);
				vi=vi==1?0:1;
				v1=wtk_tts_is_vowel(phn->phn);
				v1=v1==0?0:1;
				//wtk_debug("vi=%d,%d\n",vi,v1);
				wtk_strbuf_push_f(buf,"%d-%d",vi,v1);
				//wtk_debug("%.*s\n",buf->pos,buf->data);
				//exit(0);
			}else
			{
				wtk_strbuf_push_s(buf,"nil-nil");
			}
			wtk_strbuf_push_s(buf, "/A:nil");
			//number of phones in preview syllable
			//tone of preview syllable
			if(j>0)
			{
				//wtk_debug("%.*s\n",buf->pos,buf->data);
				//wtk_debug("%.*s syl=%p\n",phn->phn->len,phn->phn->data,phn->syl);
				if(phn->syl)
				{
					vi=wtk_tts_xphn_get_syl_snt_pos(phn);
					if(vi==0)
					{
						wtk_strbuf_push_s(buf,"_1_nil");
					}else
					{
						syl=wtk_tts_snt_get_syl(s,vi-1);
						vi=syl->phns->nslot;
						if(vi==1 && syl->wrd->sil)
						{
							wtk_strbuf_push_s(buf,"_1_nil");
						}else
						{
							wtk_strbuf_push_f(buf,"_%d_%d",vi,syl->tone);
						}
					}
				}else
				{
					int idx;

					if(s->sil_wrd_end)
					{
						idx=s->syls->nslot-2;
					}else
					{
						idx=s->syls->nslot-1;
					}
					//wtk_debug("j=%d n=%d\n",j,s->syls->nslot);
					if(idx<0){idx=0;}
					syl=wtk_tts_snt_get_syl(s,idx);
					//wtk_debug("[%.*s]\n",syl->syl->v->len,syl->syl->v->data);
					vi=syl->phns->nslot;
					//wtk_debug("vi=%d\n",vi);
					wtk_strbuf_push_f(buf,"_%d_%d",vi,syl->tone);
					//wtk_strbuf_push_s(buf, "_1_nil");
				}
			}else
			{
				wtk_strbuf_push_s(buf, "_nil_nil");
			}
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
//			if(j>=3)
//			{
//				//exit(0);
//			}
			wtk_strbuf_push_s(buf, "/B:nil");
			phn=phns[j];
			if(!phn->sil)
			{
				//number of phones current syllable
				syl=phn->syl;
				wtk_strbuf_push_f(buf,"_%d",syl->phns->nslot);
				//forward position of current syllable in word
				vi=wtk_tts_xsyl_get_forward_wrd_pos(syl);
				wtk_strbuf_push_f(buf,"@%d",vi);
				//backward position of current syllable in word
				vi=wtk_tts_xsyl_get_backward_wrd_pos(syl);
				wtk_strbuf_push_f(buf,"_%d",vi);
				//tone of current syllable
				wtk_strbuf_push_f(buf,"&%d",syl->tone);
				//current syllable contain zero_shengmu,value:0,1
				//wtk_debug("%.*s:%d\n",syl->syl->v->len,syl->syl->v->data,syl->tone);
				vi=wtk_tts_is_zero_init(syl->phn->phns[0]);
				wtk_strbuf_push_f(buf,"#%d",vi);
				//current syllable erhua, value:0,1(1->erhua)
				//vi=wtk_tts_is_erhua(syl->phn->phns[syl->phn->nphn-1]);
				vi=wtk_tts_is_erhua2(syl->phn);
				wtk_strbuf_push_f(buf,"$%d",vi);
			}else
			{
				wtk_strbuf_push_s(buf,"_1@1_1&nil#nil$nil");
			}
			wtk_strbuf_push_s(buf,"/C:nil");
			//number of phones next syllable
			//tone of next syllable
			phn=phns[j];
			//wtk_debug("phn=%p\n",phn->syl);
			if(phn->syl)
			{
				syl=wtk_tts_snt_get_nxt_syl(s,phn->syl);
				if(syl && wtk_string_cmp_s(syl->syl->v,"pau")==0)
				{
					syl=NULL;
				}
			}else
			{
				if(j==0)
				{
					syl=wtk_tts_snt_get_syl(s,0);
				}else
				{
					syl=NULL;
				}
			}
			if(syl)
			{
				//wtk_debug("syl=%.*s:%d/%d\n",syl->syl->v->len,syl->syl->v->data,syl->tone,syl->syl->tone);
				wtk_strbuf_push_f(buf,"-%d-%d",syl->phn->nphn,syl->tone);
			}else
			{
				if(phn->syl)
				{
					wtk_strbuf_push_s(buf,"-1-nil");
				}else
				{
					wtk_strbuf_push_s(buf,"-nil-nil");
				}
			}
			//wtk_debug("v[%d]=[%.*s]\n",j,buf->pos,buf->data);
			//exit(0);
			//pos(part of speech) of preview word
			//number of syllables in preview word
			if(j!=0)
			{
				wtk_strbuf_push_s(buf,"/D:");
				//wtk_debug("index=%d\n",phn->wrd->index);
				//wtk_debug("[%.*s]=%p\n",phn->phn->len,phn->phn->data,phn->wrd);
				if(phn->wrd)
				{
					wrd=wtk_tts_snt_get_wrd(s,phn->wrd->index-2);
				}else
				{
					if(s->sil_wrd_end)
					{
						wrd=wtk_tts_snt_get_wrd(s,s->wrds->nslot-2);
					}else
					{
						wrd=wtk_tts_snt_get_wrd(s,s->wrds->nslot-1);
					}
					//wrd=wtk_tts_snt_get_wrd(s,s->wrds->nslot-1);
				}
				if(wrd && !wrd->sil)
				{
					//wtk_debug("[%.*s]\n",wrd->v->len,wrd->v->data);
					//wtk_debug("j=%d [%.*s] pos=%.*s\n",j,wrd->v->len,wrd->v->data,wrd->pos->len,wrd->pos->data);
					if(p->cfg->use_pos)
					{
						wtk_strbuf_push(buf,wrd->pos->data,wrd->pos->len);
					}else
					{
						wtk_strbuf_push(buf,"x",1);   //default pos symbal. by dmd
					}
					wtk_strbuf_push_f(buf,"_%d",wrd->pron->pron->nsyl);
				}else
				{
					wtk_strbuf_push_s(buf,"nil_1");
				}
				//wtk_debug("%.*s\n",buf->pos,buf->data);
			}else
			{
				wtk_strbuf_push_s(buf,"/D:nil_nil");
			}
//			wtk_debug("[%.*s]\n",buf->pos,buf->data);
//			if(j>=5)
//			{
//				//exit(0);
//			}
			phn=phns[j];
			if(phn->wrd)
			{
				//pos(part of speech) of current word
				//number of syllables in current word
				wrd=phn->wrd;
				if(wrd->sil)
				{
					wtk_strbuf_push_s(buf, "/E:nil_1");
				}else
				{
					wtk_strbuf_push_s(buf,"/E:");
					if(p->cfg->use_pos)
					{
						wtk_strbuf_push(buf,phn->wrd->pos->data,phn->wrd->pos->len);
					}else
					{
						wtk_strbuf_push(buf,"x",1);   //default x: unknow by dmd
					}
					wtk_strbuf_push_f(buf,"_%d",phn->wrd->pron->pron->nsyl);
				}
			}else
			{
				wtk_strbuf_push_s(buf, "/E:nil_1");
			}
			//wtk_debug("%.*s\n",buf->pos,buf->data);
			if(phn->wrd)
			{
				//forward position of current word in snt
				//backward position of current word in snt
				//wtk_debug("pos=%d\n",phn->wrd->valid_pos);
				wtk_strbuf_push_f(buf,"!%d_%d",phn->wrd->index+1,nwrd-phn->wrd->index);
			}else
			{
				if(j==0)
				{
					wtk_strbuf_push_f(buf,"!1_%d",nwrd);
				}else
				{
					wtk_strbuf_push_f(buf,"!%d_1",nwrd);
				}
			}
//			wtk_debug("[%.*s]\n",buf->pos,buf->data);
//			if(j>=5)
//			{
//				exit(0);
//			}
			//pos(part of speech) of next word
			//number of syllables in next word
			//wtk_debug("[%.*s]=%p\n",phn->phn->len,phn->phn->data,phn->wrd);
			if(phn->wrd)
			{
				wrd=wtk_tts_snt_get_wrd(s,phn->wrd->index);
			}else
			{
				if(j==0)
				{
					wrd=wrds[0];
				}else
				{
					wrd=NULL;
				}
			}
			//wtk_debug("wrd=%p %.*s wrd=%p\n",wrd,phn->phn->len,phn->phn->data,phn->wrd);
			//wtk_tts_wrd_print(wrd);
			if(wrd)
			{
				if(wrd->sil)
				{
					wtk_strbuf_push_s(buf,"/F:nil_1");
				}else
				{
					vi=wtk_tts_wrd_nsyl(wrd);
					if(p->cfg->use_pos)
					{
						wtk_strbuf_push_f(buf,"/F:%.*s_%d",wrd->pos->len,wrd->pos->data,vi);
					}else
					{
						wtk_strbuf_push_f(buf,"/F:%.*s_%d",1,"x",vi);   //default x:unknown. by dmd
					}
				}
			}else
			{
				if(phn->sil)
				{
					wtk_strbuf_push_s(buf,"/F:nil_nil");
				}else
				{
					wtk_strbuf_push_s(buf,"/F:nil_1");
				}
			}
			//wtk_debug("v[%d]:%.*s\n",j,buf->pos,buf->data);
			//number of syllables in current snt
			//number of words int current snt
			//snt-type(sentence pattern) of current snt
			wtk_strbuf_push_f(buf,"/G:%d-%d@%d",nsyl,nwrd,s->type);
			//left bound type of phone
			//right bound type of phone
			if(j<=0)
			{
				bound_pl=WTK_TTS_BOUND_SEG;
			}else
			{
				if(j==s->phns->nslot-1 && !phns[j]->syl)
				{
					//if have a stop(ting-dun) in end
					bound_pl=WTK_TTS_BOUND_SEG;
				}else
				{
					bound_pl=phns[j-1]->bound;
				}
				//bound_pl=phns[j-1]->bound;
			}
			//wtk_debug("pl=%d\n",bound_pl);
			//wtk_debug("bound_pl=%d\n",bound_pl);
//			if(j>=s->phns->nslot-1)
//			{
//				bound_pr=WTK_TTS_BOUND_SEG;
//			}else
//			{
//				if(j==0)
//				{
//					bound_pr=WTK_TTS_BOUND_SEG;
//				}else
//				{
//					bound_pr=phns[j+1]->bound;
//					wtk_debug("[%.*s]=%d\n",phns[j+1]->phn->len,phns[j+1]->phn->data,bound_pr)
//				}
//			}

			bound_pr=phns[j]->bound;
			if(j==s->phns->nslot-2)
			{
				if(phns[j+1]->sil)
				{
					bound_pr=WTK_TTS_BOUND_SEG;
				}
			}
			//wtk_debug("pr=%d\n",bound_pr);

			//left bound type of syllable
			if(phn->syl)
			{
				vi=wtk_tts_snt_get_syl_snt_pos(s,phn->syl);
				if(vi<1)
				{
					bound_syll=WTK_TTS_BOUND_SEG;
				}else
				{
					syl=wtk_tts_snt_get_syl(s,vi-1);
					bound_syll=syl->bound;
				}
			}else
			{
				if(j==0)
				{
					bound_syll=WTK_TTS_BOUND_SEG;
				}else
				{
					syl=wtk_tts_snt_get_syl(s,s->syls->nslot-1);
					bound_syll=syl->bound;
				}
			}
			//right bound type of syllable
//			if(phn->syl)
//			{
//				vi=wtk_tts_snt_get_syl_snt_pos(s,phn->syl);
//				if(vi>=s->syls->nslot-1)
//				{
//					bound_sylr=WTK_TTS_BOUND_SEG;
//				}else
//				{
//					syl=wtk_tts_snt_get_syl(s,vi+1);
//					wtk_debug("syl=%p\n",syl);
//					bound_sylr=syl->bound;
//				}
//			}else
//			{
//				bound_sylr=WTK_TTS_BOUND_SEG;
//			}
			if(phn->syl)
			{
				if(s->sil_wrd_end && (phn->syl==((wtk_tts_xsyl_t**)(s->syls->slot))[s->syls->nslot-2]))
				{
					bound_sylr=WTK_TTS_BOUND_SEG;
				}else
				{
					bound_sylr=phn->syl->bound;
				}
			}else
			{
				bound_sylr=WTK_TTS_BOUND_SEG;
			}
//			wtk_debug("bound =%d\n",bound_sylr);
//			if(j>=12)
//			{
//				exit(0);
//			}
			//left bound type of word
			if(phn->wrd)
			{
				if(phn->wrd->index<=1)
				{
					bound_wrdl=WTK_TTS_BOUND_CSEG; //sil
				}else
				{
					wrd=wtk_tts_snt_get_wrd(s,phn->wrd->index-2);
					bound_wrdl=wrd->bound;
				}
			}else
			{
				if(j==0)
				{
					bound_wrdl=WTK_TTS_BOUND_SEG;
				}else
				{
					wrd=wtk_tts_snt_get_wrd(s,s->wrds->nslot-1);
					bound_wrdl=wrd->bound;
				}
			}
			//wtk_debug("bound_wrdl=%d\n",bound_wrdl);
			//right bound type of word
			if(phn->wrd)
			{
//				if(phn->wrd->index>=s->wrds->nslot-1)
//				{
//					bound_wrdr=WTK_TTS_BOUND_SEG;
//				}else
//				{
//					wrd=wtk_tts_snt_get_wrd(s,phn->wrd->index);
//					bound_wrdr=wrd->bound;
//				}
				if((phn->wrd->index-1)>=s->wrds->nslot-1 ||(s->sil_wrd_end && (phn->wrd->index-1)>=s->wrds->nslot-2))
				{
					bound_wrdr=WTK_TTS_BOUND_SEG;
				}else
				{
					bound_wrdr=phn->wrd->bound;
				}
//				wtk_debug("[%.*s]=%d/%d wrd=%d/%d\n",phn->wrd->v->len,phn->wrd->v->data,bound_wrdr,phn->wrd->bound,phn->wrd->index,
//						s->wrds->nslot-1);
			}else
			{
				bound_wrdr=WTK_TTS_BOUND_SEG;
			}
//			wtk_debug("bound_wrdr=%d wrd=%p\n",bound_wrdr,phn->wrd);
//			if(j==4)
//			{
//				wtk_debug("found bug\n");
//				exit(0);
//			}
			//wtk_debug("%d %d %d %d %d %d\n",bound_pl,bound_pr,bound_syll,bound_sylr,bound_wrdl,bound_wrdr);
			if(bound_pl==WTK_TTS_BOUND_CSEG)
			{
				bound_pl=WTK_TTS_BOUND_SEG;
			}
			if(bound_pr==WTK_TTS_BOUND_CSEG)
			{
				bound_pr=WTK_TTS_BOUND_SEG;
			}
			if(bound_syll==WTK_TTS_BOUND_CSEG)
			{
				bound_syll=WTK_TTS_BOUND_SEG;
			}
			if(bound_sylr==WTK_TTS_BOUND_CSEG)
			{
				bound_sylr=WTK_TTS_BOUND_SEG;
			}
			if(bound_wrdl==WTK_TTS_BOUND_CSEG)
			{
				bound_wrdl=WTK_TTS_BOUND_SEG;
			}
			if(bound_wrdr==WTK_TTS_BOUND_CSEG)
			{
				bound_wrdr=WTK_TTS_BOUND_SEG;
			}
			if(!p->cfg->use_hi)
			{
				phn->lab=wtk_heap_dup_string(p->heap,buf->data,buf->pos);
				continue;
			}
			//wtk_debug("[%d]\n",p->cfg->use_hi);
			wtk_strbuf_push_s(buf,"/H:");
			//h1&h2@h3@h4&h5!h6
			//h1: left bound type of phone
			//h2: right bound type of phone
			//h3: left bound type of syllable
			//h4: right bound type of syllable
			//h5: left bound type of word
			//h6: right bound type of word
			//wtk_debug("bound_pl[%d]=%d\n",j,bound_pl);
			if(bound_pl==WTK_TTS_BOUND_PHONE)
			{
				wtk_strbuf_push_s(buf,"p");
			}else
			{
				wtk_strbuf_push_f(buf,"%d",bound_pl);
			}
			wtk_strbuf_push_s(buf,"&");
			if(bound_pr==WTK_TTS_BOUND_PHONE)
			{
				wtk_strbuf_push_s(buf,"p");
			}else
			{
				wtk_strbuf_push_f(buf,"%d",bound_pr);
			}
			wtk_strbuf_push_f(buf,"@%d@%d&%d!%d",bound_syll,bound_sylr,bound_wrdl,bound_wrdr);
//			wtk_debug("%.*s\n",phn->phn->len,phn->phn->data);
//			wtk_debug("%.*s\n",buf->pos,buf->data);
//			if(j>=11)
//			{
//				exit(0);
//			}
			//wtk_debug("bound_sylr=%d bound_wrdr=%d\n",bound_sylr,bound_wrdr);

			wtk_strbuf_push_s(buf,"/I:");
			//i1+i2&i3+i4!i5!i6#i7
			phn=phns[j];
			//relative(xiang-dui,response absolute) position of phone in syllable
			v=wtk_tts_xphn_get_phn_in_syl_pos(phn);
			//wtk_debug("[%.*s]\n",v->len,v->data);
			wtk_strbuf_push(buf,v->data,v->len);
			wtk_strbuf_push_s(buf,"+");
			//position of syllable in word
			v=wtk_tts_xphn_get_syl_in_wrd_pos(phn);
			//wtk_debug("[%.*s]\n",v->len,v->data);
			wtk_strbuf_push(buf,v->data,v->len);
			wtk_strbuf_push_s(buf,"&");
			//position of syllable in phrase
			v=wtk_tts_xphn_get_syl_in_phase_pos(phn);
			wtk_strbuf_push(buf,v->data,v->len);
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			wtk_strbuf_push_s(buf,"+");
			//position of syllable in snt
			v=wtk_tts_xphn_get_syl_in_seg_pos(phn);
			wtk_strbuf_push(buf,v->data,v->len);
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			wtk_strbuf_push_s(buf,"!");
			//position of word in phrase
			v=wtk_tts_xphn_get_wrd_in_phase_pos(phn);
			wtk_strbuf_push(buf,v->data,v->len);
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			wtk_strbuf_push_s(buf,"!");
			//position of word in snt
			v=wtk_tts_xphn_get_wrd_in_seg_pos(phn);
			wtk_strbuf_push(buf,v->data,v->len);
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			wtk_strbuf_push_s(buf,"#");
			//position of phrase in snt
			v=wtk_tts_xphn_get_phase_in_seg_pos(phn);
			wtk_strbuf_push(buf,v->data,v->len);
			//printf("v[%d]: %.*s\n",j+1,buf->pos,buf->data);
			//exit(0);
			phn=phns[j];
			phn->lab=wtk_heap_dup_string(p->heap,buf->data,buf->pos);
			//wtk_debug("[%.*s]\n",phn->lab->len,phn->lab->data);
//			if(!wtk_str_equal(buf->data,buf->pos,ps[j],strlen(ps[j])))
//			{
//				wtk_debug("not equal\n");
//				printf("%.*s\n",buf->pos,buf->data);
//				printf("%s\n",ps[j]);
//				exit(0);
//			}
			//printf("v[%d]=%.*s \n",j,buf->pos,buf->data);
		}
	}
	//exit(0);
	return 0;
}

int wtk_tts_parser_finish(wtk_tts_parser_t *p,wtk_tts_lab_t *lab)
{
	wtk_tts_snt_t **snt,*s;
	int i;

	//wtk_tts_lab_print_bound(lab);
	snt=(wtk_tts_snt_t**)lab->snts->slot;
	//wtk_debug("n=%d\n",lab->snts->nslot);
	for(i=0;i<lab->snts->nslot;++i)
	{
		s=snt[i];
		if(s->wrds->nslot<=0)
		{
			continue;
		}
		wtk_tts_parser_finish_snt(p, s);
	}
	return 0;
}

wtk_tts_lab_t* wtk_tts_parser_to_snt(wtk_tts_parser_t *p,char *s,int s_bytes)
{
	return wtk_tts_parser_to_sntm(p, s, s_bytes, 0);
}

wtk_tts_lab_t* wtk_tts_parser_to_sntm(wtk_tts_parser_t *p,char *s,int s_bytes,int use_m)
{
	wtk_tts_lab_t* lab;
	wtk_tts_info_t info;
	wtk_string_t v;

	info.heap=p->heap;
	info.buf=p->buf;
	if(p->cfg->segwrd.use_sp)
	{
		v.data=s;
		v.len=s_bytes;
	}else
	{
		v=wtk_tts_norm_process(p->norm,s,s_bytes);
	}
	//wtk_array_print_string(p->norm->lex->lexr->wrds);
	//wtk_debug("[%.*s] use_sp=%d\n",v.len,v.data,p->cfg->segwrd.use_sp);
	if (use_m){
		lab=wtk_tts_segsnt_processm(&(p->segsnt),&info,v.data, v.len);
	}else{
		lab=wtk_tts_segsnt_process(&(p->segsnt),&info,v.data,v.len);
	}

	if(!lab || lab->snts->nslot<=0)
	{
		lab=NULL;
	}
	return lab;
}

int wtk_tts_parser_process_snt(wtk_tts_parser_t *p,wtk_tts_snt_t *snt)
{
	wtk_tts_info_t info;
	int ret=-1;

	info.heap=p->heap;
	info.buf=p->buf;
	//句子分词
	ret=wtk_tts_segwrd_process_snt(&(p->segwrd),&info,snt);
	if(ret!=0){goto end;}
	//词性处理
	if (p->cfg->use_pos)
	{
		ret=wtk_tts_pos_process_snt(&(p->pos),&info,snt);
		if(ret!=0){goto end;}
	}
	//多音字处理
	ret=wtk_polyphn_process_snt(p->polyphn,&info,snt);
	if(ret!=0){goto end;}
	//音素处理
	ret=wtk_tts_phn_process_snt(&(p->phn),&info,snt);
	if(ret!=0){goto end;}
	//生成合成文本序列
	ret=wtk_tts_parser_finish_snt(p,snt);
	if(ret!=0){goto end;}
	ret=0;
end:
	//wtk_debug("ret=%d\n",ret);
	return ret;
}

int wtk_tts_parser_process(wtk_tts_parser_t *p,char *s,int s_bytes)
{
	wtk_tts_lab_t* lab;
	wtk_tts_info_t info;
	int ret=-1;
	wtk_string_t v;

	info.heap=p->heap;
	info.buf=p->buf;
	if(p->cfg->segwrd.use_sp)
	{
		wtk_string_set(&(v),s,s_bytes);
	}else
	{
		v=wtk_tts_norm_process(p->norm,s,s_bytes);
	}
	//wtk_debug("[%.*s]\n",v.len,v.data);
	lab=wtk_tts_segsnt_process(&(p->segsnt),&info,v.data,v.len);
	if(!lab || lab->snts->nslot<=0){goto end;}
	ret=wtk_tts_segwrd_process(&(p->segwrd),&info,lab);
	if(ret!=0){goto end;}
	ret=wtk_tts_pos_process(&(p->pos),&info,lab);
	if(ret!=0){goto end;}
	ret=wtk_polyphn_process(p->polyphn,&info,lab);
	if(ret!=0){goto end;}
	ret=wtk_tts_phn_process(&(p->phn),&info,lab);
	if(ret!=0){goto end;}
	ret=wtk_tts_parser_finish(p,lab);
	if(ret!=0){goto end;}
	p->lab=lab;
	ret=0;
end:
	//wtk_debug("ret=%d\n",ret);
	return ret;
}

int wtk_tts_parser_process2(wtk_tts_parser_t *p,char *txt,int txt_bytes)
{
	wtk_tts_snt_t **snt,*s;
	wtk_tts_lab_t* lab;
	int i;
	int ret=-1;
	wtk_strbuf_t *buf=wtk_strbuf_new(256,1);

	wtk_strbuf_push(buf,txt,txt_bytes);
	wtk_strbuf_strip(buf);
	txt=buf->data;
	txt_bytes=buf->pos;

	lab=wtk_tts_parser_to_snt(p,txt,txt_bytes);
	if(!lab){goto end;}
	snt=(wtk_tts_snt_t**)lab->snts->slot;
	for(i=0;i<lab->snts->nslot ;++i)
	{
		s=snt[i];
		ret=wtk_tts_parser_process_snt(p,s);
		if(ret!=0){continue;}
	}
	p->lab=lab;
end:
	//wtk_debug("ret=%d\n",ret);
	wtk_strbuf_delete(buf);
	return ret;
}

void wtk_tts_parser_print(wtk_tts_parser_t *p)
{
	wtk_tts_lab_print(p->lab);
}

void wtk_tts_parser_defpron_wrd(wtk_tts_parser_t*p, wtk_string_t* k, wtk_string_t* v)
{
	if (p->polyphn->use_defpron)
		wtk_defpron_setwrd(p->polyphn->defpron, k, v);
}
