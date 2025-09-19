#include "qtk_kwdec_cfg.h"

int qtk_kwdec_cfg_init(qtk_kwdec_cfg_t *cfg)
{
	wtk_fst_net_cfg_init(&(cfg->net));
	//qtk_trans_model_cfg_init(&(cfg->trans_model));
	//wtk_prune_cfg_init(&(cfg->prune));
	cfg->h=wtk_heap_new(512);
	cfg->size=1000;
	cfg->beam=18.0;
	cfg->beam_delta=0.5;
	cfg->lattice_beam=10.0;
	cfg->max_active=7000;
	cfg->min_active=200;
	cfg->prune_interval=25;
	cfg->prune_scale=0.1;
	cfg->lm_scale=1.0;
	cfg->use_prune=0;
    cfg->add_softmax=0;
    cfg->min_kws=3;
    cfg->wdec_post_win_size=50;
    cfg->use_wdec=0;
    cfg->ac_scale=1.7;
    cfg->pdf_conf = -30;
    cfg->sil_frame = 20;
	cfg->default_key_id =123;
	cfg->words=NULL;
	cfg->n_networds=0;
	cfg->set=NULL;
	cfg->use_words_set=0;
	cfg->reset_time=500;
	return 0;
}
int qtk_kwdec_cfg_clean(qtk_kwdec_cfg_t *cfg)
{
	wtk_fst_net_cfg_clean(&(cfg->net));
	if(cfg->use_words_set)
	{
		if(cfg->words)
		{
			int i;
			wtk_string_t **s;
			s=(wtk_string_t**)(cfg->words->slot);
			for(i=0;i<cfg->words->nslot;++i)
			{
				wtk_string_delete(s[i]);
			}
			//wtk_array_dispose(cfg->words);
		}
	}
	if(cfg->set)
	{
		wtk_free(cfg->set);
	}
	wtk_heap_delete(cfg->h);
	return 0;
}

int qtk_kwdec_word_cfg_update_local(qtk_kwdec_words_set_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	wtk_local_cfg_update_cfg_str(lc,cfg,word,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,key_id,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pdf_conf,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_kws,v);
	return 0;
}

int qtk_kwdec_post_cfg_update_words_set(qtk_kwdec_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_queue_node_t *qn;
	wtk_cfg_item_t *item;
	qtk_kwdec_words_set_t *word;

	cfg->n_networds=0;
	cfg->set=(qtk_kwdec_words_set_t*)wtk_malloc(sizeof(qtk_kwdec_words_set_t)*lc->cfg->queue.length);
	for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
	{
		item=data_offset2(qn,wtk_cfg_item_t,n);
		if(item->type!=WTK_CFG_LC){continue;}
		word=cfg->set+cfg->n_networds;
		word->key_id=-1;
		word->pdf_conf=0;
		word->min_kws = 5;
		word->word=NULL;
		qtk_kwdec_word_cfg_update_local(word,item->value.cfg);
		++cfg->n_networds;
	}
	return 0;
}

int qtk_kwdec_cfg_update_local(qtk_kwdec_cfg_t *cfg,wtk_local_cfg_t *main)
{
	int ret;
	wtk_local_cfg_t *lc;
	wtk_string_t *v;

	lc=main;
	ret=0;

	wtk_local_cfg_update_cfg_i(lc,cfg,size,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,use_prune,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,beam,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_active,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_active,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,prune_interval,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,lattice_beam,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,beam_delta,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,prune_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,lm_scale,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,add_softmax,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_wdec,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,min_kws,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,ac_scale,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,wdec_post_win_size,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,pdf_conf,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,sil_frame,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,default_key_id,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_words_set,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,reset_time,v);

	lc=wtk_local_cfg_find_lc_s(main,"net");
	if(lc)
	{
		ret=wtk_fst_net_cfg_update_local(&(cfg->net),lc);
		if(ret!=0){goto end;}
	}
	cfg->words=wtk_local_cfg_find_array_s(lc,"words");
	// wtk_debug("%d\n",cfg->words->nslot);
	lc=wtk_local_cfg_find_lc_s(main,"networds_set");
	if(lc)
	{
		ret = qtk_kwdec_post_cfg_update_words_set(cfg,lc);
	}
	ret=0;

	
	end:
	return ret;
}
int qtk_kwdec_cfg_update(qtk_kwdec_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;
/*	ret=qtk_trans_model_cfg_update(&(cfg->trans_model),sl);
	if(ret!=0)
	{
		wtk_debug("update trans_model failed\n");
		goto end;
	}*/

	ret=wtk_fst_net_cfg_update3(&(cfg->net),NULL,sl);
	if(ret!=0)
	{
		wtk_debug("update net failed\n");
		goto end;
	}

/*	if(cfg->use_prune)
	{
		ret=wtk_prune_cfg_update(&(cfg->prune));
		if(ret!=0){goto end;}
	}*/

	end:
	return ret;
}
void qtk_kwdec_cfg_print(qtk_kwdec_cfg_t *cfg)
{

}

void qtk_kwdec_cfg_words_set(qtk_kwdec_cfg_t *cfg, char **words,int n)
{
	//wtk_heap_t *h;
	wtk_array_t *a;
	int i;
	a=wtk_array_new_h(cfg->h,256,sizeof(wtk_string_t*));
	wtk_string_t *s;
	for(i=0;i<n;++i)
	{
		s=wtk_string_new(64);
		s->data=words[i];
		s->len=strlen(words[i]);
		wtk_array_push2(a,&s);
		//wtk_array_print_string(a);
	}
	cfg->words =a;
	//wtk_wdec_net_cfg_print_words(cfg);

}
