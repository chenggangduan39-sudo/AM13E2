#include "wtk_cosynthesis_cfg.h"

void wtk_cosynthesis_init_floor(wtk_cosynthesis_cfg_t *cfg)
{
	char *mgc_floor="-5.824605, 0.5462366, 2.1852572, -2.4437742, 1.2996849,"
			"-2.112048, -2.2533784, -2.8681285, -3.1652477, -1.2112676,"
			"-3.5159976, -0.68293804, -1.6754858";
	char *lf0_floor="-1E10";
	wtk_array_t *b;
	wtk_string_t **v;
	int k;
	float j;
	wtk_heap_t *heap;

	heap = wtk_heap_new(128);

	b=wtk_str_to_array(heap, mgc_floor, strlen(mgc_floor), ',');
	v=(wtk_string_t**)b->slot;
	cfg->mgc_floor = wtk_array_new_h(cfg->heap, b->nslot, sizeof(float));
	for(k=0;k<b->nslot;++k)
	{
		j=atof(v[k]->data);
		*((float*)wtk_array_push(cfg->mgc_floor))=j;
	}
	b=wtk_str_to_array(heap, lf0_floor, strlen(lf0_floor), ',');
	v=(wtk_string_t**)b->slot;
    cfg->lf0_floor = wtk_array_new_h(cfg->heap, b->nslot, sizeof(float));
	for(k=0;k<b->nslot;++k)
	{
		j=atof(v[k]->data);
		*((float*)wtk_array_push(cfg->lf0_floor))=j;
	}
	wtk_heap_delete(heap);
}

int wtk_cosynthesis_cfg_init(wtk_cosynthesis_cfg_t *cfg)
{
	cfg->rbin=NULL;
    cfg->pool=NULL;
    cfg->n_context_presele=30;
    cfg->n_kld_presele=10;
    cfg->user_lexicon = NULL;
    cfg->crf_model_fn = NULL;
    cfg->max_loss = 100.0f;
    cfg->inset_thd = 0.0001f;
    cfg->heap = wtk_heap_new(128);
    cfg->use_auth_fn=NULL;
    cfg->use_auth=1;
    wtk_wsola_cfg_init(&cfg->wsola_cfg);
    wtk_cosynthesis_lexicon_cfg_init(&cfg->lex_cfg);
    wtk_cosynthesis_backend_cfg_init(&cfg->be_cfg);
    wtk_trietree_cfg_init(&cfg->trie_cfg);
    wtk_cosynthesis_phrase_cfg_init(&cfg->phr_cfg);

    wtk_cosynthesis_init_floor(cfg);

    return 0;
}

int wtk_cosynthesis_cfg_update_local(wtk_cosynthesis_cfg_t *cfg,wtk_local_cfg_t *main)
{
    int ret;
    wtk_string_t *v;
    wtk_local_cfg_t *lc;

    lc=main;
    wtk_local_cfg_update_cfg_i(lc, cfg, n_context_presele, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, n_kld_presele, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_auth, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, use_auth_fn, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, max_loss, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, inset_thd, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, user_lexicon, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, crf_model_fn, v);
    lc=wtk_local_cfg_find_lc_s(main,"lexicon");
    if(lc)
    {
        ret = wtk_cosynthesis_lexicon_cfg_update_local(&cfg->lex_cfg,lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(main,"backend");
    if(lc)
    {
        ret = wtk_cosynthesis_backend_cfg_update_local(&cfg->be_cfg,lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(main,"wsola");
    if(lc)
    {
        ret=wtk_wsola_cfg_update_local(&cfg->wsola_cfg,lc);
        if(ret!=0){goto end;}
    }

    lc=wtk_local_cfg_find_lc_s(main,"mtrie");
    if(lc)
    {
        ret=wtk_trietree_cfg_update_local(&cfg->trie_cfg,lc);
        if(ret!=0){goto end;}
    }

    lc=wtk_local_cfg_find_lc_s(main,"phrase");
    if(lc)
    {
        ret=wtk_cosynthesis_phrase_cfg_update_local(&cfg->phr_cfg,lc);
        if(ret!=0){goto end;}
    }
	ret=0;
end:
    return ret;
}

int wtk_cosynthesis_cfg_clean(wtk_cosynthesis_cfg_t *cfg)
{
    wtk_cosynthesis_lexicon_cfg_clean(&cfg->lex_cfg);
    wtk_wsola_cfg_clean(&cfg->wsola_cfg);
    wtk_trietree_cfg_clean(&(cfg->trie_cfg));
    wtk_cosynthesis_backend_cfg_clean(&cfg->be_cfg);
    wtk_cosynthesis_phrase_cfg_clean(&cfg->phr_cfg);
    if(cfg->pool)
    {
        wtk_strpool_delete(cfg->pool);
    }
    if (cfg->heap)
    	wtk_heap_delete(cfg->heap);
    if (cfg->rbin && cfg->user_lexicon)
    	wtk_free(cfg->user_lexicon);

    return 0;
}

int wtk_cosynthesis_cfg_update(wtk_cosynthesis_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
    return wtk_cosynthesis_cfg_update2(cfg,&sl);
}

int wtk_cosynthesis_cfg_authset(int *use_auth,wtk_source_t *src)
{
	double time;
	int ret;

    ret = wtk_source_read_double_bin(src, &time, 1, 0);
    if (0== ret && (time > 0 || time_get_ms() > time))
    	*use_auth=1;
    else
    	*use_auth=0;

	return ret;
}

int wtk_cosynthesis_cfg_update2(wtk_cosynthesis_cfg_t *cfg,wtk_source_loader_t *sl)
{
    int ret, use_auth=0;
    wtk_string_t *pwd;
    char *np;

    if(cfg->use_auth_fn)
    {
    	ret=wtk_source_loader_load(sl,&use_auth,(wtk_source_load_handler_t)wtk_cosynthesis_cfg_authset,cfg->use_auth_fn);
    	if(ret!=0){wtk_debug("load failed:%s", cfg->use_auth_fn);goto end;}
    	cfg->use_auth = use_auth;
    }

    cfg->rbin = sl->hook;
    if (cfg->rbin)
    {
		pwd=wtk_dir_name(cfg->rbin->fn, '/');
		np = (char*)wtk_calloc(pwd->len+1+strlen(cfg->user_lexicon)+1, sizeof(char));
		memcpy(np, pwd->data, pwd->len);
		memcpy(np+pwd->len, "/", 1);
		memcpy(np+pwd->len+1, cfg->user_lexicon, strlen(cfg->user_lexicon));
		cfg->user_lexicon = np;
		wtk_free(pwd);
    }
    cfg->pool=wtk_strpool_new(1507);
    ret = wtk_wsola_cfg_update2(&cfg->wsola_cfg,sl);
    if(ret<0) goto end;
//    ret = wtk_cosynthesis_lexicon_cfg_update(&cfg->lex_cfg);
    ret = wtk_cosynthesis_lexicon_cfg_update2(&cfg->lex_cfg,sl);
    if(ret<0) goto end;
    ret = wtk_cosynthesis_backend_cfg_update2(&cfg->be_cfg,sl,cfg->pool);
    if(ret<0) goto end;

    ret = wtk_trietree_cfg_update2(&cfg->trie_cfg, sl);
    if(ret<0) goto end;

    ret = wtk_cosynthesis_phrase_cfg_update2(&cfg->phr_cfg, sl);
    if(ret<0) goto end;

end:
    return ret;
}

wtk_cosynthesis_cfg_t* wtk_cosynthesis_cfg_new_bin(char *cfg_fn,int seek_pos)
{
	wtk_mbin_cfg_t *main_cfg;
	wtk_cosynthesis_cfg_t *cfg;

	cfg=0;
	main_cfg=wtk_mbin_cfg_new_type2(seek_pos,wtk_cosynthesis_cfg,cfg_fn,"./cfg");
	if(main_cfg){
		cfg=(wtk_cosynthesis_cfg_t*)(main_cfg->cfg);
		cfg->mbin_cfg=main_cfg;
	}

	return cfg;
}

void wtk_cosynthesis_cfg_delete_bin(wtk_cosynthesis_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

int wtk_cosynthesis_cfg_delete(wtk_cosynthesis_cfg_t *cfg)
{
	if(!cfg){return 0;}
	wtk_cosynthesis_cfg_clean(cfg);
	wtk_free(cfg);
	return 0;
}

wtk_cosynthesis_cfg_t* wtk_cosynthesis_cfg_new2(char *dn,wtk_arg_t *arg)
{
	wtk_strbuf_t* buf=wtk_strbuf_new(1024,1);
	wtk_cosynthesis_cfg_t* cfg=0;
	wtk_cfg_file_t *cf=0;
	char *data=0;
	int ret=-1,len;
	int dlen;
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	if(!dn){goto end;}
	dlen=strlen(dn);
	wtk_strbuf_push(buf,dn,dlen);
	wtk_strbuf_push_s(buf,"/cfg");
	wtk_strbuf_push_c(buf,0);
	data=file_read_buf(buf->data,&len);
	if(!data){goto end;}
	cf=wtk_cfg_file_new();
	wtk_cfg_file_add_var_ks(cf,"pwd",dn,dlen);
	ret=wtk_cfg_file_feed(cf,data,len);
	if(ret!=0){goto end;}
	if(arg)
	{
		wtk_local_cfg_update_arg(cf->main,arg,1);
	}
	cfg=(wtk_cosynthesis_cfg_t*)calloc(1,sizeof(wtk_cosynthesis_cfg_t));
	wtk_cosynthesis_cfg_init(cfg);
	ret=wtk_cosynthesis_cfg_update_local(cfg,cf->main);
	if(ret!=0){goto end;}
	ret=wtk_cosynthesis_cfg_update2(cfg,&sl);
	if(ret!=0){goto end;}
	cfg->cfile=cf;
	cf=0;
end:
	if(data){free(data);}
	if(cf)
	{
		wtk_cfg_file_delete(cf);
	}
	if(ret!=0 && cfg)
	{
		wtk_cosynthesis_cfg_delete(cfg);
		cfg=0;
	}
	wtk_strbuf_delete(buf);
	return cfg;
}

wtk_cosynthesis_cfg_t* wtk_cosynthesis_cfg_new(char *dn)
{
	return wtk_cosynthesis_cfg_new2(dn,0);
}
