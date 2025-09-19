#include "qtk_tts_polyphone_cfg.h"
#include <ctype.h>

int qtk_tts_polyphone_cfg_init(qtk_tts_polyphone_cfg_t *cfg)
{
    memset(cfg,0,sizeof(*cfg));
    cfg->is_en = 1;
    qtk_tts_dispoly_cfg_init(&cfg->disply);
    return 0;
}

int qtk_tts_polyphone_cfg_clean(qtk_tts_polyphone_cfg_t *cfg)
{
    if(cfg->pron_dict){
        wtk_kdict_delete(cfg->pron_dict);
    }
    if(cfg->ply_dict){
        wtk_kdict_delete(cfg->ply_dict);
    }
    if(cfg->unstressed_dict){
        wtk_kdict_delete(cfg->unstressed_dict);
    }
    if(cfg->char_dict){
        wtk_kdict_delete(cfg->char_dict);
    }
    if(cfg->pp_dict){
        wtk_kdict_delete(cfg->pp_dict);
    }
    if(cfg->poly_id_dict){
        wtk_kdict_delete(cfg->poly_id_dict);
    }
    if(cfg->use_disply){
        qtk_tts_dispoly_cfg_clean(&cfg->disply);
    }
    return 0;
}

int qtk_tts_polyphone_cfg_update_local(qtk_tts_polyphone_cfg_t *cfg,wtk_local_cfg_t *main_cfg)
{
    wtk_local_cfg_t *lc = main_cfg;
    wtk_string_t *v = NULL;

    wtk_local_cfg_update_cfg_b(main_cfg,cfg,is_en,v);
    wtk_local_cfg_update_cfg_b(main_cfg,cfg,use_disply,v);
    wtk_local_cfg_update_cfg_str(main_cfg,cfg,pron_fn,v);
    wtk_local_cfg_update_cfg_str(main_cfg,cfg,ply_fn,v);
    wtk_local_cfg_update_cfg_str(main_cfg,cfg,unstressed_fn,v);
    wtk_local_cfg_update_cfg_str(main_cfg,cfg,char_fn,v);
    wtk_local_cfg_update_cfg_str(main_cfg,cfg,pp_fn,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,pron_hint,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,ply_hint,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,unstressed_hint,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,char_hint,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,pp_hint,v);
    lc = wtk_local_cfg_find_lc_s(main_cfg,"disply_model");
    if(lc && cfg->use_disply){
        qtk_tts_dispoly_cfg_update_local(&cfg->disply,lc);
    }
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,poly_id_hint,v);
    wtk_local_cfg_update_cfg_str(main_cfg,cfg,poly_id_fn,v);
    return 0;
}


//移了个函数过来
extern wtk_tts_wrd_pron_t* wtk_tts_wrd_pron_parse(wtk_strpool_t *pool,wtk_heap_t *heap,char *v,int v_bytes);

wtk_tts_wrd_pron_t* qtk_tts_polyphone_cfg_parse_pron(wtk_strbuf_t *buf,wtk_kdict_t *dict,wtk_string_t *nm,char *v,int v_bytes)
{
	wtk_tts_wrd_pron_t *pron,*p1;

	pron=wtk_tts_wrd_pron_parse(dict->pool,dict->hash->heap,v,v_bytes);
	p1=wtk_kdict_get(dict,nm->data,nm->len);
	if(p1)
	{
		p1->npron+=pron->npron;
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

qtk_tts_polyphone_char_t* qtk_tts_char_id_parse(wtk_strpool_t *pool,wtk_heap_t *heap,char *v,int v_bytes)
{
	typedef enum
	{
		WTK_TTS_SEGWRD_INIT,
		WTK_TTS_SEGWRD_WORD,
	}wtk_syndict_state_t;
	char *s,*e,c;
	wtk_syndict_state_t state;
	wtk_string_t k;
	int tone;
	qtk_tts_polyphone_char_t *char_p = NULL;
    wtk_tts_wrd_pron_t *pron = NULL;
    wtk_tts_syl_t syl;

	wtk_string_set(&(k),0,0);
	s=v;
	e=s+v_bytes;
	state=WTK_TTS_SEGWRD_INIT;
	while(s<e){
		c=*s;
		switch(state){
		case WTK_TTS_SEGWRD_INIT:
			if(!isspace((unsigned char)c))
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
			if(c=='_')
			{
				k.len=s-k.data;
                char_p =  wtk_heap_malloc(heap,sizeof(qtk_tts_polyphone_char_t));
				char_p->chart = wtk_heap_dup_string(heap,k.data,k.len);
                // wtk_debug("%.*s\n",k.len,k.data);
				state=WTK_TTS_SEGWRD_INIT;
			}
			break;
		}
		++s;
	}
    k.len = s-k.data-1;
    tone = wtk_str_atoi(s-1,1);
    syl.v = wtk_heap_dup_string(heap,k.data,k.len);
    syl.tone=tone;
    // wtk_debug("%.*s:%d\n",syl.v->len,syl.v->data,syl.tone);
	pron=(wtk_tts_wrd_pron_t*)wtk_heap_malloc(heap,sizeof(wtk_tts_wrd_pron_t));
	pron->nsyl=1;
	pron->syls=(wtk_tts_syl_t*)wtk_heap_malloc(heap,sizeof(wtk_tts_syl_t));
	pron->npron=1;
	pron->next=NULL;
	memcpy(pron->syls,&syl,sizeof(wtk_tts_syl_t));
    char_p->pron = pron;
	return char_p;
}

qtk_tts_polyphone_char_t* qtk_tts_polyphone_cfg_parse_char_id(wtk_strbuf_t *buf,wtk_kdict_t *dict,wtk_string_t *nm,char *v,int v_bytes)
{
	qtk_tts_polyphone_char_t *pron;

	pron=qtk_tts_char_id_parse(dict->pool,dict->hash->heap,v,v_bytes);
	if(pron){
        wtk_str_hash_add(dict->hash,nm->data,nm->len,pron);
    }
	return pron;
}

int qtk_tts_polyphone_cfg_update(qtk_tts_polyphone_cfg_t *cfg)
{
    if(cfg->pron_fn){
        cfg->pron_dict = wtk_kdict_new(NULL,cfg->pron_hint,NULL,(wtk_kdict_parse_value_f)qtk_tts_polyphone_cfg_parse_pron);
        wtk_kdict_load_file(cfg->pron_dict,cfg->pron_fn);
    }
    if(cfg->ply_fn){
        cfg->ply_dict = wtk_kdict_new(NULL,cfg->pron_hint,NULL,(wtk_kdict_parse_value_f)qtk_tts_polyphone_cfg_parse_pron);
        wtk_kdict_load_file(cfg->ply_dict,cfg->ply_fn);
    }
    if(cfg->unstressed_fn){
        cfg->unstressed_dict = wtk_kdict_new(NULL,cfg->pron_hint,NULL,(wtk_kdict_parse_value_f)qtk_tts_polyphone_cfg_parse_pron);
        wtk_kdict_load_file(cfg->unstressed_dict,cfg->unstressed_fn);
    }
    if(cfg->char_fn){
        cfg->char_dict = wtk_kdict_new(NULL,cfg->char_hint,NULL,NULL);
        wtk_kdict_load_file(cfg->char_dict,cfg->char_fn);
    }
    if(cfg->pp_fn){
        cfg->pp_dict = wtk_kdict_new(NULL,cfg->pp_hint,NULL,NULL);
        wtk_kdict_load_file(cfg->pp_dict,cfg->pp_fn);
    }
    if(cfg->poly_id_fn){
        cfg->poly_id_dict = wtk_kdict_new(NULL,cfg->poly_id_hint,NULL,(wtk_kdict_parse_value_f)qtk_tts_polyphone_cfg_parse_char_id);
        wtk_kdict_load_file(cfg->poly_id_dict,cfg->poly_id_fn);
    }
    if(cfg->use_disply){
        qtk_tts_dispoly_cfg_update(&cfg->disply);
    }
    return 0;
}


int qtk_tts_polyphone_cfg_update2(qtk_tts_polyphone_cfg_t *cfg, wtk_source_loader_t *sl)
{
	int ret=0;

    if(cfg->pron_fn){
        cfg->pron_dict = wtk_kdict_new(NULL,cfg->pron_hint,NULL,(wtk_kdict_parse_value_f)qtk_tts_polyphone_cfg_parse_pron);
		ret=wtk_source_loader_load(sl,cfg->pron_dict,(wtk_source_load_handler_t)wtk_kdict_load,cfg->pron_fn);
		if(ret!=0){goto end;}
    }
    if(cfg->ply_fn){
        cfg->ply_dict = wtk_kdict_new(NULL,cfg->pron_hint,NULL,(wtk_kdict_parse_value_f)qtk_tts_polyphone_cfg_parse_pron);
		ret=wtk_source_loader_load(sl,cfg->ply_dict,(wtk_source_load_handler_t)wtk_kdict_load,cfg->ply_fn);
		if(ret!=0){goto end;}
    }
    if(cfg->unstressed_fn){
        cfg->unstressed_dict = wtk_kdict_new(NULL,cfg->pron_hint,NULL,(wtk_kdict_parse_value_f)qtk_tts_polyphone_cfg_parse_pron);
		ret=wtk_source_loader_load(sl,cfg->unstressed_dict,(wtk_source_load_handler_t)wtk_kdict_load,cfg->unstressed_fn);
		if(ret!=0){goto end;}
    }
    if(cfg->char_fn){
        cfg->char_dict = wtk_kdict_new(NULL,cfg->char_hint,NULL,NULL);
		ret=wtk_source_loader_load(sl,cfg->char_dict,(wtk_source_load_handler_t)wtk_kdict_load,cfg->char_fn);
		if(ret!=0){goto end;}
    }
    if(cfg->pp_fn){
        cfg->pp_dict = wtk_kdict_new(NULL,cfg->pp_hint,NULL,NULL);
		ret=wtk_source_loader_load(sl,cfg->pp_dict,(wtk_source_load_handler_t)wtk_kdict_load,cfg->pp_fn);
		if(ret!=0){goto end;}
    }
    if(cfg->poly_id_fn){
        cfg->poly_id_dict = wtk_kdict_new(NULL,cfg->poly_id_hint,NULL,(wtk_kdict_parse_value_f)qtk_tts_polyphone_cfg_parse_char_id);
		ret=wtk_source_loader_load(sl,cfg->poly_id_dict,(wtk_source_load_handler_t)wtk_kdict_load,cfg->poly_id_fn);
		if(ret!=0){goto end;}
    }
    if(cfg->use_disply){
        qtk_tts_dispoly_cfg_update(&cfg->disply);
    }

end:
    return ret;
}
