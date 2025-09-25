#include "wtk_wdec_net_cfg.h" 

int wtk_wdec_net_cfg_init(wtk_wdec_net_cfg_t *cfg)
{
	wtk_hmmset_cfg_init(&(cfg->hmmset));
	cfg->dict_fn=NULL;
	cfg->dict=NULL;
	cfg->word=NULL;
	cfg->words=NULL;
	cfg->phn=NULL;
	cfg->use_cross=1;
	cfg->label=wtk_label_new(17005);
	cfg->use_sil=1;
	cfg->h=wtk_heap_new(512);
	cfg->use_words_set=1;
	cfg->n_words = 0;
    cfg->set=NULL;
	return 0;
}

int wtk_wdec_net_cfg_clean(wtk_wdec_net_cfg_t *cfg)
{
	wtk_hmmset_cfg_clean(&(cfg->hmmset));
	if(cfg->dict)
	{
		wtk_dict_delete(cfg->dict);
	}
	wtk_label_delete(cfg->label);
	// if(cfg->use_words_set)
	// {
	// 	if(cfg->words)
	// 	{
	// 		int i;
	// 		wtk_string_t **s;
	// 		s=(wtk_string_t**)(cfg->words->slot);
	// 		for(i=0;i<cfg->words->nslot;++i)
	// 		{
	// 			wtk_string_delete(s[i]);
	// 		}
	// 		//wtk_array_dispose(cfg->words);
	// 	}
	// }
	if(cfg->set)
	{
		int i;
		wtk_wdec_words_set_t *set;
		for(i=0;i<cfg->n_words;++i)
		{
			set=cfg->set+i;
			if(set->min_wrd_conf)
			{
				wtk_free(set->min_wrd_conf);
			}
		}
		wtk_free(cfg->set);
	}
	wtk_heap_delete(cfg->h);
	return 0;
}
int wtk_wdec_word_cfg_update_local(wtk_wdec_words_set_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	wtk_array_t *a;
	int i;
	wtk_local_cfg_update_cfg_str(lc,cfg,word,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,label,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_conf,v);
	a=wtk_local_cfg_find_array_s(lc,"min_wrd_conf");
	if(a)
	{
		cfg->min_wrd_conf=(float*)wtk_calloc(a->nslot,sizeof(float));
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)(a->slot))[i];
			cfg->min_wrd_conf[i]=wtk_str_atof(v->data,v->len);
		}
	}
	return 0;
}

int wtk_wdec_post_cfg_update_words_set(wtk_wdec_net_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_queue_node_t *qn;
	wtk_cfg_item_t *item;
	wtk_wdec_words_set_t *word;

	cfg->n_words=0;
	cfg->set=(wtk_wdec_words_set_t*)wtk_malloc(sizeof(wtk_wdec_words_set_t)*lc->cfg->queue.length);
	for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
	{
		item=data_offset2(qn,wtk_cfg_item_t,n);
		if(item->type!=WTK_CFG_LC){continue;}
		word=cfg->set+cfg->n_words;
		word->label=-1;
		word->min_wrd_conf=NULL;
		word->min_conf=0;
		word->word=NULL;
		wtk_wdec_word_cfg_update_local(word,item->value.cfg);
		++cfg->n_words;
		//wtk_debug("word=%s,label=%d,wrd_conf=[%f %f %f %f],conf=%f\n",word->word,word->label,word->min_wrd_conf[0]
		//,word->min_wrd_conf[1],word->min_wrd_conf[2],word->min_wrd_conf[3],word->min_conf);
	}
	return 0;
}

int wtk_wdec_net_cfg_update_local(wtk_wdec_net_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_str(lc,cfg,dict_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,word,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_cross,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_sil,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_words_set,v);
	cfg->phn=wtk_local_cfg_find_array_s(lc,"phn");
	cfg->words=wtk_local_cfg_find_array_s(lc,"words");
	lc=wtk_local_cfg_find_lc_s(main,"hmmset");
	if(lc)
	{
		ret=wtk_hmmset_cfg_update_local(&(cfg->hmmset),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"words_set");
	if(lc)
	{
		ret = wtk_wdec_post_cfg_update_words_set(cfg,lc);
	}
	ret=0;
end:
	return ret;
}

void wtk_wdec_net_cfg_print_phn(wtk_wdec_net_cfg_t *cfg)
{
	wtk_array_t *a=cfg->phn;
	wtk_string_t **phs=(wtk_string_t**)(a->slot);
	int i;

	for(i=0;i<a->nslot;++i)
	{
		wtk_debug("v[%d]=[%.*s]\n",i,phs[i]->len,phs[i]->data);
	}
}

int wtk_wdec_net_cfg_update2(wtk_wdec_net_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	cfg->dict=wtk_dict_new2(cfg->label,0,153,15007);
	ret=wtk_source_loader_load(sl,cfg->dict,(wtk_source_load_handler_t)wtk_dict_load,cfg->dict_fn);
	if(ret!=0){goto end;}
	ret=wtk_hmmset_cfg_update2(&(cfg->hmmset),cfg->label,sl);
	if(ret!=0){goto end;}
end:
	return ret;
}

void wtk_wdec_net_cfg_print_words(wtk_wdec_net_cfg_t *cfg)
{
	wtk_array_t *a=cfg->words;
	wtk_string_t **words=(wtk_string_t**)(a->slot);
	int i;

	for(i=0;i<a->nslot;++i)
	{
		wtk_debug("v[%d]=[%.*s]\n",i,words[i]->len,words[i]->data);
	}
}

void wtk_wdec_net_cfg_set_words(wtk_wdec_net_cfg_t *cfg, char *words,int n)
{
	wtk_array_t *a;
	a=wtk_str_to_array(cfg->h, words,n, '|');
	cfg->words =a;
	//cfg->n_words = a->nslot;
}
