#include <ctype.h>
#include "wtk_tts_segwrd_cfg.h" 

int wtk_tts_segwrd_cfg_init(wtk_tts_segwrd_cfg_t *cfg)
{
	wtk_segmenter_cfg_init(&(cfg->seg));
	cfg->dict_fn=NULL;
	cfg->dict=NULL;
	cfg->dict_hint=215007;
	cfg->use_bin=0;
	cfg->use_maxseg=1;
	cfg->use_sp=0;
	cfg->use_sp2=0;
	cfg->en_dict = 0;
	cfg->use_lower = 0;
	cfg->use_upper = 0;
	cfg->pool_out = 1;
	return 0;
}

int wtk_tts_segwrd_cfg_clean(wtk_tts_segwrd_cfg_t *cfg)
{
	wtk_segmenter_cfg_clean(&(cfg->seg));
	if(cfg->dict)
	{
		if(cfg->pool_out == 0)
			wtk_strpool_delete(cfg->dict->pool);
		wtk_kdict_delete(cfg->dict);
	}
	return 0;
}

int wtk_tts_segwrd_cfg_bytes(wtk_tts_segwrd_cfg_t *cfg)
{
	return cfg->dict?wtk_kdict_bytes(cfg->dict):0;
}

int wtk_tts_segwrd_cfg_update_local(wtk_tts_segwrd_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_str(lc,cfg,dict_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,dict_hint,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_bin,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_maxseg,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_sp,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_sp2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,en_dict,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_upper,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_lower,v);
	lc=wtk_local_cfg_find_lc_s(main,"seg");
	if(lc)
	{
		ret=wtk_segmenter_cfg_update_local(&(cfg->seg),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return 0;
}


wtk_tts_wrd_pron_t* wtk_tts_wrd_pron_parse(wtk_strpool_t *pool,wtk_heap_t *heap,char *v,int v_bytes)
{
#define MAX_PHN 50
	typedef enum
	{
		WTK_TTS_SEGWRD_INIT,
		WTK_TTS_SEGWRD_WORD,
	}wtk_syndict_state_t;
	wtk_tts_syl_t vx[MAX_PHN];
	char *s,*e,c;
	wtk_syndict_state_t state;
	wtk_string_t k;
	int ki=0;
	int tone;
	wtk_tts_wrd_pron_t *pron,*p1,*p2;
//	static int xki=0;
//
//	++xki;
//	wtk_debug("v[%d]=[%.*s]\n",xki,v_bytes,v);
	wtk_string_set(&(k),0,0);
	s=v;
	e=s+v_bytes;
	state=WTK_TTS_SEGWRD_INIT;
	p1=NULL;
	while(s<e)
	{
		c=*s;
		switch(state)
		{
		case WTK_TTS_SEGWRD_INIT:
			if(isalpha(c))
			{
				k.data=s;
				k.len=0;
				state=WTK_TTS_SEGWRD_WORD;
				if(s!=e-1)
				{
					break;
				}
			}
			break;
		case WTK_TTS_SEGWRD_WORD:
			if(c=='-' || s==e-1 ||c=='|')
			{
				if(c=='-' || c=='|')
				{
					k.len=s-k.data;
				}else
				{
					k.len=s-k.data+1;
				}
				c=k.data[k.len-1];
				if(isdigit(c))
				{
					tone=c-'0';
					--k.len;
				}else
				{
					tone=0;
				}
				//wtk_debug("[%.*s]=%d\n",k.len,k.data,tone);
				if(pool)
				{
					vx[ki].v=wtk_strpool_find(pool,k.data,k.len,1);
				}else
				{
					vx[ki].v=wtk_heap_dup_string(heap,k.data,k.len);
				}
				//wtk_debug("[%.*s]=%d\n",k.len,k.data,tone);
				vx[ki].tone=tone;
				++ki;
				if(*s=='|' || s==e-1)
				{
					pron=(wtk_tts_wrd_pron_t*)wtk_heap_malloc(heap,sizeof(wtk_tts_wrd_pron_t));
					pron->nsyl=ki;
					//wtk_debug("ki=%d [%.*s]\n",ki,v_bytes,v);
					pron->syls=(wtk_tts_syl_t*)wtk_heap_malloc(heap,sizeof(wtk_tts_syl_t)*ki);
					pron->npron=1;
					pron->next=NULL;
					memcpy(pron->syls,vx,sizeof(wtk_tts_syl_t)*ki);
					if(p1)
					{
						++p1->npron;
						p2=p1;
						while(p2->next)
						{
							p2=p2->next;
						}
						p2->next=pron;
					}else
					{
						p1=pron;
					}
					ki=0;
				}
				state=WTK_TTS_SEGWRD_INIT;
			}
			break;
		}
		++s;
	}
//	if(xki==10)
//	{
//		exit(0);
//	}
	return p1;
}
//认为用空格隔开的 EH2 K S K L AH0 M EY1 SH AH0 N P OY2 N T
wtk_tts_wrd_pron_t* wtk_tts_wrd_pron_parse2(wtk_strpool_t *pool,wtk_heap_t *heap,char *v,int v_bytes)
{
#define MAX_PHN 50
	typedef enum
	{
		WTK_TTS_SEGWRD_INIT,
		WTK_TTS_SEGWRD_WORD,
	}wtk_syndict_state_t;
	wtk_tts_syl_t vx[MAX_PHN]={{NULL,},};
	char *s,*e,c;
	wtk_syndict_state_t state;
	wtk_string_t k;
	int ki=0;
	wtk_tts_wrd_pron_t *pron,*p1,*p2;
	// static int xki=0;

	// ++xki;
	// wtk_debug("v[%d]=[%.*s]\n",xki,v_bytes,v);
	wtk_string_set(&(k),0,0);
	s=v;
	e=s+v_bytes;
	state=WTK_TTS_SEGWRD_INIT;
	p1=NULL;
	while(s<e)
	{
		c=*s;
		switch(state)
		{
		case WTK_TTS_SEGWRD_INIT:
			if(isalpha(c)||c==','||c=='.'||c==':'||c==';'||c=='!'||c=='?'||c=='_'||c=='~'||c=='*')
			{
				k.data=s;
				k.len=0;
				state=WTK_TTS_SEGWRD_WORD;
				if(s!=e-1)
				{
					break;
				}else{
					continue;	//最后一个音只有一个字符的情况,重新循环一次
				}
			}
			break;
		case WTK_TTS_SEGWRD_WORD:
			if(c==' '|| s==e-1)
			{
				if(c==' ')
				{
					k.len=s-k.data;
				}else
				{
					k.len=s-k.data+1;
				}
				// c=k.data[k.len-1];
				// wtk_debug("[%.*s]\n",k.len,k.data);
				if(pool)
				{
					vx[ki].v=wtk_strpool_find(pool,k.data,k.len,1);
				}else
				{
					vx[ki].v=wtk_heap_dup_string(heap,k.data,k.len);
				}
				//wtk_debug("[%.*s]=%d\n",k.len,k.data,tone);
				vx[ki].tone=0;
				++ki;
				if(s==e-1)
				{
					pron=(wtk_tts_wrd_pron_t*)wtk_heap_malloc(heap,sizeof(wtk_tts_wrd_pron_t));
					pron->nsyl=ki;
					//wtk_debug("ki=%d [%.*s]\n",ki,v_bytes,v);
					pron->syls=(wtk_tts_syl_t*)wtk_heap_malloc(heap,sizeof(wtk_tts_syl_t)*ki);
					pron->npron=1;
					pron->next=NULL;
					memcpy(pron->syls,vx,sizeof(wtk_tts_syl_t)*ki);
					if(p1)
					{
						++p1->npron;
						p2=p1;
						while(p2->next)
						{
							p2=p2->next;
						}
						p2->next=pron;
					}else
					{
						p1=pron;
					}
					ki=0;
				}
				state=WTK_TTS_SEGWRD_INIT;
			}
			break;
		}
		++s;
	}
//	if(xki==10)
//	{
//		exit(0);
//	}
	return p1;
}



wtk_tts_wrd_pron_t* wtk_tts_segwrd_cfg_parse_pron(wtk_strbuf_t *buf,wtk_kdict_t *dict,wtk_string_t *nm,char *v,int v_bytes)
{
	wtk_tts_wrd_pron_t *pron,*p1;

	pron=wtk_tts_wrd_pron_parse(dict->pool,dict->hash->heap,v,v_bytes);
	p1=wtk_kdict_get(dict,nm->data,nm->len);
	if(p1)
	{
		p1->npron+=pron->npron;
		//++p1->npron;
		while(p1->next)
		{
			p1=p1->next;
		}
		p1->next=pron;
	}else
	{
		wtk_str_hash_add(dict->hash,nm->data,nm->len,pron);
	}
	return pron;
}

wtk_tts_wrd_pron_t* wtk_tts_segwrd_cfg_parse_pron2(wtk_strbuf_t *buf,wtk_kdict_t *dict,wtk_string_t *nm,char *v,int v_bytes)
{
	wtk_tts_wrd_pron_t *pron,*p1;

	pron=wtk_tts_wrd_pron_parse2(dict->pool,dict->hash->heap,v,v_bytes);
	p1=wtk_kdict_get(dict,nm->data,nm->len);
	if(p1)
	{
		p1->npron+=pron->npron;
		//++p1->npron;
		while(p1->next)
		{
			p1=p1->next;
		}
		p1->next=pron;
	}else
	{
		wtk_str_hash_add(dict->hash,nm->data,nm->len,pron);
	}
	return pron;
}

int wtk_tts_segwrd_cfg_update3(wtk_tts_segwrd_cfg_t *cfg,wtk_strpool_t *pool)
{
	int ret=-1;

	//wtk_debug("%s\n",cfg->dict_fn);
	if(cfg->dict_fn && !cfg->use_bin)
	{
		wtk_strbuf_t *buf;

		buf=wtk_strbuf_new(256,1);
		if(cfg->en_dict){
			cfg->dict=wtk_kdict_new(pool,cfg->dict_hint,buf,(wtk_kdict_parse_value_f)wtk_tts_segwrd_cfg_parse_pron2);
		}else{
			cfg->dict=wtk_kdict_new(pool,cfg->dict_hint,buf,(wtk_kdict_parse_value_f)wtk_tts_segwrd_cfg_parse_pron);
		}
		ret=wtk_kdict_load_file(cfg->dict,cfg->dict_fn);
		if(ret!=0){goto end;}
		wtk_strbuf_delete(buf);
	}
	if(!cfg->use_maxseg)
	{
		ret=wtk_segmenter_cfg_update(&(cfg->seg));
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_tts_segwrd_cfg_update(wtk_tts_segwrd_cfg_t *cfg)
{
	wtk_strpool_t *pool;

	pool=wtk_strpool_new(1507);
	cfg->pool_out = 0;
	return wtk_tts_segwrd_cfg_update3(cfg,pool);
}

int wtk_tts_segwrd_cfg_update2(wtk_tts_segwrd_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool)
{
	int ret=-1;

	//wtk_debug("%s\n",cfg->dict_fn);
	if(cfg->dict_fn && !cfg->use_bin)
	{
		wtk_strbuf_t *buf;

		buf=wtk_strbuf_new(256,1);
		if(cfg->en_dict){
			cfg->dict=wtk_kdict_new(pool,cfg->dict_hint,buf,(wtk_kdict_parse_value_f)wtk_tts_segwrd_cfg_parse_pron2);
		}else{
			cfg->dict=wtk_kdict_new(pool,cfg->dict_hint,buf,(wtk_kdict_parse_value_f)wtk_tts_segwrd_cfg_parse_pron);
		}
		ret=wtk_source_loader_load(sl,cfg->dict,(wtk_source_load_handler_t)wtk_kdict_load,cfg->dict_fn);
		if(ret!=0){goto end;}
		wtk_strbuf_delete(buf);
	}
	if(!cfg->use_maxseg)
	{
		ret=wtk_segmenter_cfg_update2(&(cfg->seg),sl);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}
