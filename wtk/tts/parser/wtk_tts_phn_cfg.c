#include <ctype.h>
#include "wtk_tts_phn_cfg.h" 

int wtk_tts_phn_cfg_init(wtk_tts_phn_cfg_t *cfg)
{
	cfg->dict_fn=NULL;
	cfg->dict=NULL;
	cfg->dict_hint=1101;
	return 0;
}

int wtk_tts_phn_cfg_clean(wtk_tts_phn_cfg_t *cfg)
{
	if(cfg->dict)
	{
		wtk_kdict_delete(cfg->dict);
	}
	return 0;
}

int wtk_tts_phn_cfg_bytes(wtk_tts_phn_cfg_t *cfg)
{
	return wtk_kdict_bytes(cfg->dict);
}

int wtk_tts_phn_cfg_update_local(wtk_tts_phn_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,dict_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,dict_hint,v);
	return 0;
}

wtk_tts_sylphn_t* wtk_tts_phn_cfg_parse_phn(wtk_strbuf_t *buf,wtk_kdict_t *dict,wtk_string_t *nm,char *v,int v_bytes)
{
typedef enum
{
	WTK_TTS_PHN_INIT,
	WTK_TTS_PHN_WRD,
}wtk_tts_phn_state_t;
	wtk_tts_sylphn_t *phn;
	wtk_string_t *vs[32];
	int ki=0;
	char *s,*e,c;
	wtk_tts_phn_state_t state;
	wtk_string_t k={0,0};
//	static int xi=0;
//
//	++xi;
//	wtk_debug("v[%d]: %.*s=%.*s\n",xi,nm->len,nm->data,v_bytes,v);

	s=v;e=s+v_bytes;
	state=WTK_TTS_PHN_INIT;
	while(s<e)
	{
		c=*s;
		switch(state)
		{
		case WTK_TTS_PHN_INIT:
			if(!isspace(c))
			{
				k.data=s;
				k.len=0;
				state=WTK_TTS_PHN_WRD;
				if(s!=(e-1))
				{
					break;
				}
			}
			break;
		case WTK_TTS_PHN_WRD:
			if(isspace(c))
			{
				k.len=s-k.data;
			}else if(s==e-1)
			{
				k.len=s-k.data+1;
			}
			if(k.len>0)
			{
				//wtk_debug("[%.*s]\n",k.len,k.data);
				vs[ki++]=wtk_strpool_find(dict->pool,k.data,k.len,1);
				state=WTK_TTS_PHN_INIT;
			}
			break;
		}
		++s;
	}
	if(state==WTK_TTS_PHN_WRD)
	{
		k.len=e-k.data;
		//wtk_debug("[%.*s]\n",k.len,k.data);
		vs[ki++]=wtk_strpool_find(dict->pool,k.data,k.len,1);
	}
	phn=(wtk_tts_sylphn_t*)wtk_heap_malloc(dict->hash->heap,sizeof(wtk_tts_sylphn_t));
	phn->nphn=ki;
	phn->phns=(wtk_string_t**)wtk_heap_malloc(dict->hash->heap,sizeof(wtk_string_t*)*ki);
	memcpy(phn->phns,vs,sizeof(wtk_string_t*)*ki);
	wtk_str_hash_add(dict->hash,nm->data,nm->len,phn);
//	if(xi==275)
//	{
//		wtk_debug("nphn=%d\n",phn->nphn);
//		exit(0);
//	}
	return phn;
}

int wtk_tts_phn_cfg_update(wtk_tts_phn_cfg_t *cfg,wtk_strpool_t *pool)
{
	wtk_source_loader_t sl;

	wtk_source_loader_init_file(&(sl));
	return wtk_tts_phn_cfg_update2(cfg,&(sl),pool);
}


int wtk_tts_phn_cfg_update2(wtk_tts_phn_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool)
{
	int ret=-1;
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(256,1);
	//wtk_debug("%s\n",cfg->dict_fn);
	if(cfg->dict_fn)
	{
		cfg->dict=wtk_kdict_new(pool,cfg->dict_hint,buf,(wtk_kdict_parse_value_f)wtk_tts_phn_cfg_parse_phn);
		ret=wtk_source_loader_load(sl,cfg->dict,(wtk_source_load_handler_t)wtk_kdict_load,cfg->dict_fn);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}
