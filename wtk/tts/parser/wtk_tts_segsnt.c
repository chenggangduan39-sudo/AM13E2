#include "wtk_tts_segsnt.h"
#include "wtk/core/wtk_os.h"
#include <math.h>
#include <ctype.h>


wtk_tts_lab_t* wtk_tts_lab_new(wtk_heap_t *heap,int hint)
{
	wtk_tts_lab_t *lab;

	lab=(wtk_tts_lab_t*)wtk_heap_malloc(heap,sizeof(wtk_tts_lab_t));
	lab->snts=wtk_array_new_h(heap,hint,sizeof(void*));
	lab->speech_speed=1.0;
	return lab;
}

void wtk_tts_segsnt_init(wtk_tts_segsnt_t *snt,wtk_tts_segsnt_cfg_t *cfg)
{
	snt->cfg=cfg;
}

void wtk_tts_segsnt_clean(wtk_tts_segsnt_t *snt)
{
}

void wtk_tts_segsnt_reset(wtk_tts_segsnt_t *snt)
{
}

void wtk_tts_segsnt_update2(wtk_tts_segsnt_t *seg,wtk_tts_snt_t *snt, wtk_string_t *v)
{
	wtk_string_t xv;
	// 先根据句末标点判断
	if(wtk_array_str_has(seg->cfg->ques_sym,v->data,v->len))
	{
		//wtk_debug("[%.*s]\n",snt->snt->len,snt->snt->data);
		if(wtk_array_str_end_with(seg->cfg->ques_char2,snt->snt->data,snt->snt->len-v->len))
		//if(wtk_array_str_has(seg->cfg->ques_char2,snt->snt->data,snt->snt->len-v->len))
		{
			snt->type=WTK_TTS_SNT_QUES2;
		}else
		{
			snt->type=WTK_TTS_SNT_QUES;
		}
	}else if(wtk_array_str_has(seg->cfg->sigh_sym,v->data,v->len))
	{
		snt->type=WTK_TTS_SNT_SIGH;
	}
	if(snt->type==WTK_TTS_SNT_NORM)
	{
		if(wtk_array_str_has(seg->cfg->end_tok,v->data,v->len))
		{
			wtk_string_set(&(xv),snt->snt->data,snt->snt->len-v->len);
		}else
		{
			wtk_string_set(&(xv),snt->snt->data,snt->snt->len);
		}
		//wtk_debug("[%.*s]\n",xv.len,xv.data);
		if(wtk_array_str_end_with(seg->cfg->sigh_char,xv.data,xv.len))
		{
			snt->type=WTK_TTS_SNT_SIGH;
		}else
		{
			if(wtk_array_str_end_with(seg->cfg->ques_char2,xv.data,xv.len))
			{
				snt->type=WTK_TTS_SNT_QUES2;
			}else if(wtk_array_str_end_with(seg->cfg->ques_char,xv.data,xv.len))
			{
				snt->type=WTK_TTS_SNT_QUES;
			}else if(wtk_array_str_start_with(seg->cfg->ques_start2,snt->snt->data,snt->snt->len))
			{
				snt->type=WTK_TTS_SNT_QUES2;
			}
		}
	}
}

void wtk_tts_segsnt_update(wtk_tts_segsnt_t *seg,wtk_tts_snt_t *snt)
{
	wtk_string_t **strs,*v;

	strs=(wtk_string_t **)snt->chars->slot;
	v=strs[snt->chars->nslot-1];
	wtk_tts_segsnt_update2(seg, snt, v);
}

void wtk_tts_segsnt_update_random_tone(wtk_tts_segsnt_t *snt,wtk_tts_lab_t *lab)
{
	wtk_tts_snt_t **snts,*s;
	int i;
	int j,nxti;
	unsigned long t;

	//wtk_debug("use_random_tone\n");
	snts=(wtk_tts_snt_t**)lab->snts->slot;
	srand(time_get_ms());
	for(i=0;i<lab->snts->nslot;)
	{
		s=snts[i];
		//wtk_debug("[%.*s]\n",s->snt->len,s->snt->data);
		if(s->type==WTK_TTS_SNT_NORM)
		{
			//wtk_debug("%d\n",i);
			nxti=i;
			for(j=i+1;j<lab->snts->nslot;++j)
			{
				if(snts[j]->type!=WTK_TTS_SNT_NORM)
				{
					break;
				}else
				{
					nxti=j;
				}
			}
			if(nxti>i)
			{
				int last_set=0;

				for(j=i;j<=nxti;++j)
				{
					if(last_set==0)
					{
						t=rand(); //random();//+time_get_ms()*10000;
						t=(t)%(snt->cfg->rand_step);
						snts[j]->type=t==0?WTK_TTS_SNT_SIGH:WTK_TTS_SNT_NORM;
						last_set=1;
					}else
					{
						snts[j]->type=WTK_TTS_SNT_NORM;
						last_set=0;
					}
//					switch(t)
//					{
//					case 0:
//						snts[j]->type=WTK_TTS_SNT_SIGH;
//						break;
//					case 1:
//						snts[j]->type=WTK_TTS_SNT_QUES;
//						break;
//					case 2:
//						snts[j]->type=WTK_TTS_SNT_QUES2;
//						break;
//					default:
//						snts[j]->type=WTK_TTS_SNT_NORM;
//						break;
//					}
				}
				i=nxti+1;
			}else
			{
				++i;
			}
		}else
		{
			++i;
		}
	}
//	for(i=0;i<lab->snts->nslot;++i)
//	{
//		s=snts[i];
//		wtk_debug("v[%d]=%d [%.*s]\n",i,s->type,s->snt->len,s->snt->data);
//	}
}

int wtk_tts_segsnt_solewrds(char *s, char *e)
{
	int cnt;
	int len;

	len=0;
	while(s < e){
		cnt=wtk_utf8_bytes(*s);
		if (cnt==1 && (isalpha(*s)||*s=='\'')){
			//check english
			len++;
		}else{
			break;
		}
		s++;
	}

	return len;
}
wtk_tts_lab_t* wtk_tts_segsnt_processm(wtk_tts_segsnt_t *snt,wtk_tts_info_t *info,char *data,int data_bytes)
{
	wtk_heap_t *heap=info->heap;
	wtk_strbuf_t *buf=info->buf;
	wtk_strbuf_t *tbf;
	char *s,*e;
	int cnt,i;
	wtk_string_t *v;
	wtk_tts_lab_t *lab;
	wtk_tts_snt_t *st;
	int is_pick;

	tbf=wtk_strbuf_new(128,1);
	cnt=0;
	lab=wtk_tts_lab_new(heap,5);
	st=wtk_tts_snt_new(heap,data_bytes);
	s=data;e=s+data_bytes;
	wtk_strbuf_reset(buf);
	while(s<e)
	{
		cnt=wtk_utf8_bytes(*s);
		is_pick=0;
		wtk_strbuf_reset(tbf);
		if (cnt==1 && snt->cfg->pick_h && isalpha(*s)){
				cnt=wtk_tts_segsnt_solewrds(s,e);
				for (i=0; i < cnt; i++)
					wtk_strbuf_push_c(tbf, tolower(s[i]));
				if (wtk_str_hash_find(snt->cfg->pick_h, tbf->data, tbf->pos)!=NULL)is_pick=1;
		}else{
			wtk_strbuf_push(tbf, s, cnt);
		}
		v=wtk_heap_dup_string(heap,tbf->data, tbf->pos);

		if (!is_pick){
			wtk_strbuf_push(buf,v->data,v->len);
			wtk_array_push2(st->chars,&v);
		}
		s+=cnt;
		if(s>=e || wtk_array_str_has(snt->cfg->end_tok,v->data,v->len) || (snt->cfg->max_word>0 && st->chars->nslot>snt->cfg->max_word) || is_pick)
		{
			wtk_strbuf_strip(buf);
			if(wtk_array_str_in(snt->cfg->end_tok,buf->data,buf->pos))
			{
				wtk_strbuf_reset(buf);
			}else
			{
				//wtk_debug("[%.*s]\n",buf->pos,buf->data);
				//wtk_debug("%d/%d\n",snt->cfg->max_word,st->chars->nslot);
				if (buf->pos > 0){
					st->snt=wtk_heap_dup_string(heap,buf->data,buf->pos);
					wtk_tts_segsnt_update(snt,st);
					wtk_strbuf_reset(buf);
					// wtk_tts_snt_print(st);
					wtk_array_push2(lab->snts,&(st));
					if(s<e)
					{
						st=wtk_tts_snt_new(heap,e-s);
					}
				}
				if (is_pick){
					st->is_ctx_pick=1;
					wtk_strbuf_reset(buf);
					wtk_strbuf_push(buf,v->data,v->len);
					wtk_array_push2(st->chars,&v);
					st->snt=wtk_heap_dup_string(heap,buf->data, buf->pos);
					wtk_tts_segsnt_update(snt,st);
					wtk_strbuf_reset(buf);
					wtk_array_push2(lab->snts,&(st));
					if(s<e)
					{
						st=wtk_tts_snt_new(heap,e-s);
					}
				}
			}
		}
	}
	//wtk_debug("random=%d\n",snt->cfg->use_random_tone);
	//exit(0);
	if(snt->cfg->use_random_tone)
	{
		wtk_tts_segsnt_update_random_tone(snt,lab);
	}
	//exit(0);
	wtk_strbuf_delete(tbf);

	return lab;
}

wtk_tts_lab_t* wtk_tts_segsnt_process(wtk_tts_segsnt_t *snt,wtk_tts_info_t *info,char *data,int data_bytes)
{
	wtk_heap_t *heap=info->heap;
	wtk_strbuf_t *buf=info->buf;
	char *s,*e;
	int cnt;
	wtk_string_t *v;
	wtk_tts_lab_t *lab;
	wtk_tts_snt_t *st;

	//add by dmd 2017.05.31
	//for max_word adjust. hope split in symbol, such in end_cand_tok.
	wtk_strbuf_t *tbuf;
	wtk_array_t *tarr=NULL;
	wtk_string_t **tv;
	int idx_v, idx_c, k, end_t;

	//wtk_debug("%.*s\n",data_bytes,data);
	lab=wtk_tts_lab_new(heap,5);
	st=wtk_tts_snt_new(heap,data_bytes);
	s=data;e=s+data_bytes;
	wtk_strbuf_reset(buf);

	idx_v=idx_c=0;
	tbuf=wtk_strbuf_new(256,1);

	//Note: maybe diff meaning of characters for diff tts-version-task, here still old meaning.
	while(s<e)
	{
		cnt=wtk_utf8_bytes(*s);
		//wtk_debug("[%.*s]\n",cnt,s);
		v=wtk_heap_dup_string(heap,s,cnt);
		wtk_strbuf_push(buf,v->data,v->len);
		wtk_array_push2(st->chars,&v);
		//memory symbol of segmented-snt cand-pot for back when over length sentence
		if (snt->cfg->end_cand_tok && wtk_array_str_has(snt->cfg->end_cand_tok,v->data,v->len)){
			idx_v=buf->pos;
			idx_c=st->chars->nslot;
		}
		s+=cnt;
		//wtk_debug("%d/%d\n",snt->cfg->max_word,st->chars->nslot);
		if(s>=e || wtk_array_str_has(snt->cfg->end_tok,v->data,v->len) || (snt->cfg->max_word>0 && st->chars->nslot>snt->cfg->max_word))
		{
			end_t=0;
			//here two condition: symbol(best selection) or over length.
			if (!(s>=e || wtk_array_str_has(snt->cfg->end_tok,v->data,v->len))){
				end_t=1;
			}
			wtk_strbuf_strip(buf);
			if(wtk_array_str_in(snt->cfg->end_tok,buf->data,buf->pos))   //only symbol for a sentence, give up.
			{
				wtk_strbuf_reset(buf);
			}else
			{
				//remedy buf and st->chars. by dmd 2017.05.31
				if (end_t==1 && idx_v > 0)
				{
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_push(tbuf, buf->data+idx_v, buf->pos-idx_v);
					buf->pos = idx_v;
					//wtk_debug("buf=%.*s\n",buf->pos, buf->data);
					//wtk_debug("tbuf=%.*s\n",tbuf->pos, tbuf->data);
					tarr=wtk_array_new_h(heap,idx_c,sizeof(void*));
					//copy st->chars into tarr
					tv=(wtk_string_t**)st->chars->slot;
					for (k=idx_c; k < st->chars->nslot; k++){
						wtk_array_push2(tarr,&(tv[k]));
					}
					// should reset st->chars and set values
//					wtk_array_reset(st->chars);
//					tv=(wtk_string_t**)tarr->slot;
//					for (k=0; k < tarr->nslot; k++){
//						wtk_array_push2(tarr,tv);
//					}
					//here direct adjust index.
					st->chars->nslot=idx_c;
				}
				//wtk_debug("[%.*s]\n",buf->pos,buf->data);
				//wtk_debug("%d/%d\n",snt->cfg->max_word,st->chars->nslot);
				st->snt=wtk_heap_dup_string(heap,buf->data,buf->pos);
				// wtk_debug("%.*s\n",st->snt->len,st->snt->data);
				wtk_strbuf_reset(buf);
				wtk_tts_segsnt_update(snt,st);
				//wtk_tts_snt_print(st);
				wtk_array_push2(lab->snts,&(st));

				if(s<e)
				{
					st=wtk_tts_snt_new(heap,e-s);
					//add rest after remedy buf and st->chars , by dmd 2017.05.31
					if (end_t==1 && tbuf->pos>0){
						wtk_strbuf_reset(buf);
						wtk_strbuf_push(buf, tbuf->data, tbuf->pos);
						wtk_strbuf_reset(tbuf);
						tv=(wtk_string_t**)tarr->slot;
						for (k=0; k < tarr->nslot; k++){
							wtk_array_push2(st->chars,tv);
						}
					}
					idx_v=0;
					idx_c=0;
				}
			}
		}
	}
	//wtk_debug("random=%d\n",snt->cfg->use_random_tone);
	//set sentence type 对句子进行划分  分成 感叹句 陈述句 疑问句之类的类似类型 情感合成  现在未开发
	// if(snt->cfg->use_random_tone)
	// {
	// 	wtk_tts_segsnt_update_random_tone(snt,lab);
	// }
	wtk_strbuf_delete(tbuf);
	return lab;
}
