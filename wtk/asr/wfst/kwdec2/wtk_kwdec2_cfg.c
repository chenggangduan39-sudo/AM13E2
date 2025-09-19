#include "wtk_kwdec2_cfg.h"

int wtk_kwdec2_cfg_init(wtk_kwdec2_cfg_t *cfg)
{
	wtk_fst_net_cfg_init(&(cfg->net));
    wtk_kxparm_cfg_init(&(cfg->parm));
    wtk_egram_cfg_init(&(cfg->egram));
    wtk_kwdec2_trans_model_cfg_init(&(cfg->mdl));
    cfg->mdl_fn=NULL;
	cfg->h=wtk_heap_new(512);
	cfg->size=1000;
	cfg->beam=18.0;
	cfg->beam_delta=0.5;
	cfg->max_active=7000;
	cfg->min_active=200;
	cfg->use_prune=0;
    cfg->add_softmax=0;
    cfg->min_kws=3;
    cfg->wdec_post_win_size=50;
    cfg->ac_scale=1.7;
    cfg->pdf_conf = -30;
    cfg->sil_frame = 20;
	cfg->default_key_id =123;
	cfg->words=NULL;
	cfg->n_networds=0;
	cfg->set=NULL;
	cfg->use_words_set=0;
	cfg->use_fixpoint=0;
	cfg->use_egram = 0;
	cfg->reset_time = 200;
	cfg->shift=0;
	cfg->ebnf_dump = 0;
	cfg->plus_conf = 0.0;
	return 0;
}

void wtk_kwdec2_cfg_clean_words_set(wtk_kwdec2_cfg_t *cfg) {
    int i;
    wtk_kwdec2_words_set_t *s;
    if (cfg->use_words_set) {
        if (cfg->words) {
            int i;
            wtk_string_t **s;
            s = (wtk_string_t **)(cfg->words->slot);
            for (i = 0; i < cfg->words->nslot; ++i) {
                wtk_string_delete(s[i]);
            }
            // wtk_array_dispose(cfg->words);
        }
    }
    if (cfg->set) {
        if (cfg->ebnf_dump) {
            for (i = 0; i < cfg->n_networds; i++) {
                s = cfg->set + i;
                if (s->word) {
                    wtk_free(s->word);
                    s->word = NULL;
                }
            }
        }
        wtk_free(cfg->set);
    }
}

int wtk_kwdec2_cfg_clean(wtk_kwdec2_cfg_t *cfg) {
    wtk_fst_net_cfg_clean(&(cfg->net));
    wtk_kxparm_cfg_clean(&(cfg->parm));
    wtk_egram_cfg_clean(&(cfg->egram));
    wtk_kwdec2_trans_model_cfg_clean(&(cfg->mdl), cfg->mdl.use_chain);
    wtk_kwdec2_cfg_clean_words_set(cfg);
    wtk_heap_delete(cfg->h);
    return 0;
}

int wtk_kwdec2_cfg_bytes(wtk_kwdec2_cfg_t *cfg)
{
	int bytes=0;

	return bytes;
}

int wtk_kwdec2_word_cfg_update_local(wtk_kwdec2_words_set_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	wtk_local_cfg_update_cfg_str(lc,cfg,word,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,key_id,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pdf_conf,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_kws,v);
	return 0;
}

static float kwdec2_pdf_conf1[] = {-13,-20,-27.5,-33,-35};
static float kwdec2_pdf_conf2[] = {-16,-24,-29.5,-35,-38};

static int wtk_kwdec2_cfg_get_pdf_conf(int len){
	if(len < 7){
		return 0;
	}
	if(len < 10){
		return 1;
	}
	if(len < 13){
		return 2;
	}
	if(len < 16){
		return 3;
	}
	return 4;
}

int wtk_kwdec2_cfg_set_words_set(wtk_kwdec2_cfg_t *cfg, char *data, int len) {
    cfg->words = wtk_str_to_array(cfg->h, data, len, '|');
    int cnt = cfg->words->nslot;
    wtk_kwdec2_words_set_t *word;
    wtk_string_t **strs;
    strs = (wtk_string_t **)cfg->words->slot;

    int i, id = 123, conf_idx = 0;

    cfg->n_networds = 0;
    cfg->set = (wtk_kwdec2_words_set_t *)wtk_malloc(
        sizeof(wtk_kwdec2_words_set_t) * cnt);
    for (i = 0; i < cnt; i++) {
        word = cfg->set + cfg->n_networds;
		word->key_id = id;
		conf_idx = wtk_kwdec2_cfg_get_pdf_conf(strs[i]->len);
		word->pdf_conf = kwdec2_pdf_conf1[conf_idx];
		word->pdf_conf2 = kwdec2_pdf_conf2[conf_idx];
		word->min_kws = cfg->min_kws;
        word->word = (char *)wtk_malloc(strs[i]->len);
        memcpy(word->word, strs[i]->data, strs[i]->len * sizeof(char));
        id++;
        ++cfg->n_networds;
    }

    return 0;
}

int wtk_kwdec2_post_cfg_update_words_set(wtk_kwdec2_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_queue_node_t *qn;
	wtk_cfg_item_t *item;
	wtk_kwdec2_words_set_t *word;

	cfg->n_networds=0;
	cfg->set=(wtk_kwdec2_words_set_t*)wtk_malloc(sizeof(wtk_kwdec2_words_set_t)*lc->cfg->queue.length);
	for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
	{
		item=data_offset2(qn,wtk_cfg_item_t,n);
		if(item->type!=WTK_CFG_LC){continue;}
		word=cfg->set+cfg->n_networds;
		word->key_id=-1;
		word->pdf_conf=0;
		word->min_kws = 5;
		word->word=NULL;
		wtk_kwdec2_word_cfg_update_local(word,item->value.cfg);
		++cfg->n_networds;
	}
	return 0;
}

int wtk_kwdec2_cfg_update_local(wtk_kwdec2_cfg_t *cfg,wtk_local_cfg_t *main)
{
	int ret;
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	// wtk_array_t *a;
	// int i;

	lc=main;
	ret=0;

	wtk_local_cfg_update_cfg_i(lc,cfg,size,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,use_prune,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,beam,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_active,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_active,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,beam_delta,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,add_softmax,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,min_kws,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,ac_scale,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,wdec_post_win_size,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,pdf_conf,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,plus_conf,v);	
    wtk_local_cfg_update_cfg_i(lc,cfg,sil_frame,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,default_key_id,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_words_set,v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_egram, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, reset_time, v);
	// wtk_local_cfg_update_cfg_str(lc,cfg,mdl_fn,v);

	lc = wtk_local_cfg_find_lc_s(main, "parm");
	if(lc)
	{
		wtk_kxparm_cfg_update_local(&(cfg->parm),lc);
	}

	lc=wtk_local_cfg_find_lc_s(main,"net");
	if(lc)
	{
		ret=wtk_fst_net_cfg_update_local(&(cfg->net),lc);
		if(ret!=0){goto end;}
	}

	if (cfg->use_egram) {
		lc = wtk_local_cfg_find_lc_s(main, "egram");
		if (lc) {
			ret = wtk_egram_cfg_update_local(&(cfg->egram), lc);
			if (ret != 0) {
				goto end;
			}
		}
	}

    lc=wtk_local_cfg_find_lc_s(main,"mdl");
    if(lc)
    {
        ret=wtk_kwdec2_trans_model_cfg_update_local(&(cfg->mdl),lc);
    }
	cfg->words=wtk_local_cfg_find_array_s(lc,"words");
	//wtk_debug("%d\n",cfg->words->nslot);
	lc=wtk_local_cfg_find_lc_s(main,"networds_set");
	if(lc)
	{
		ret = wtk_kwdec2_post_cfg_update_words_set(cfg,lc);
	}
	ret=0;

	end:
	return ret;
}

int wtk_kwdec2_cfg_update(wtk_kwdec2_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_kwdec2_cfg_update2(cfg,&(sl));
}

int wtk_kwdec2_cfg_update2(wtk_kwdec2_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;
    ret=wtk_kwdec2_trans_model_cfg_update(&(cfg->mdl),sl);
	if(ret!=0)
	{
		wtk_debug("update trans_model failed\n");
		goto end;
	}

	ret=wtk_fst_net_cfg_update3(&(cfg->net),NULL,sl);
	if(ret!=0)
	{
		wtk_debug("update net failed\n");
		goto end;
	}
	ret=wtk_kxparm_cfg_update2(&(cfg->parm),sl);
	if(ret!=0)
	{
		wtk_debug("update parm failed\n");
		goto end;
	}

	if (cfg->use_egram) {
		ret = wtk_egram_cfg_update2(&(cfg->egram), sl);
		if (ret != 0) {
			wtk_debug("update egram failed\n");
			goto end;
		}
	}

	cfg->use_fixpoint = cfg->parm.knn.use_fixpoint;
	cfg->rbin = sl->hook;
	end:
	return ret;
}

void wtk_kwdec2_cfg_words_set(wtk_kwdec2_cfg_t *cfg, char **words,int n)
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

wtk_kwdec2_cfg_t* wtk_kwdec2_cfg_new_bin4(char *bin_fn)
{
	wtk_kwdec2_cfg_t* cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./kwdec.cfg";

	cfg=(wtk_kwdec2_cfg_t*)wtk_malloc(sizeof(wtk_kwdec2_cfg_t));
	wtk_kwdec2_cfg_init(cfg);
	cfg->rbin=wtk_rbin2_new();
	wtk_rbin2_read(cfg->rbin,bin_fn);
	item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	cfg->cfile=wtk_cfg_file_new();
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	wtk_kwdec2_cfg_update_local(cfg,cfg->cfile->main);
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	wtk_kwdec2_cfg_update2(cfg,&sl);
	return cfg;
}

wtk_kwdec2_cfg_t* wtk_kwdec2_cfg_new_bin3(char *bin_fn,unsigned int seek_pos)
{
	wtk_kwdec2_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./kwdec2.cfg";
	int ret;

	cfg=(wtk_kwdec2_cfg_t*)wtk_malloc(sizeof(wtk_kwdec2_cfg_t));
	//wtk_debug("new cfg=%p\n",cfg);
	wtk_kwdec2_cfg_init(cfg);
	cfg->rbin=wtk_rbin2_new();
	ret=wtk_rbin2_read2(cfg->rbin,bin_fn,seek_pos);
	if(ret!=0){
		wtk_debug("read failed\n");
		goto end;
	}
	item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	if(!item)
	{
		wtk_debug("%s not found %s\n",cfg_fn,bin_fn);
		ret=-1;goto end;
	}
	cfg->cfile=wtk_cfg_file_new();
	//wtk_debug("f=%p\n",cfg->rbin->f);
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	ret=wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	if(ret!=0){goto end;}
	ret=wtk_kwdec2_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=wtk_kwdec2_cfg_update2(cfg,&sl);
	if(ret!=0){goto end;}
end:
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		wtk_kwdec2_cfg_delete_bin2(cfg);
		cfg=NULL;
	}
	return cfg;
}

wtk_kwdec2_cfg_t* wtk_kwdec2_cfg_new_bin(char *bin_fn,char *cfg_fn)
{
	wtk_mbin_cfg_t *cfg;
	wtk_kwdec2_cfg_t *wc;

	//wtk_debug("%s/%s\n",bin_fn,cfg_fn);
	cfg=wtk_mbin_cfg_new_type(wtk_kwdec2_cfg,bin_fn,cfg_fn);
	wc=(wtk_kwdec2_cfg_t*)(cfg->cfg);
	wc->cfg.bin_cfg=cfg;

	return wc;
}

wtk_kwdec2_cfg_t* wtk_kwdec2_cfg_new_bin2(char *bin_fn)
{
	return wtk_kwdec2_cfg_new_bin(bin_fn,"./kwdec2.cfg");
}

wtk_kwdec2_cfg_t* wtk_kwdec2_cfg_new(char *fn)
{
	wtk_main_cfg_t *cfg;
	wtk_kwdec2_cfg_t *tcfg;

	cfg=wtk_main_cfg_new_type(wtk_kwdec2_cfg,fn);
	if(cfg)
	{
		tcfg=(wtk_kwdec2_cfg_t*)(cfg->cfg);
		tcfg->cfg.main_cfg=cfg;
		return tcfg;
	}else
	{
		return NULL;
	}
}

void wtk_kwdec2_cfg_delete(wtk_main_cfg_t *main_cfg)
{
	wtk_main_cfg_delete(main_cfg);
}

void wtk_kwdec2_cfg_delete_bin(wtk_kwdec2_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->cfg.bin_cfg);
}

void wtk_kwdec2_cfg_delete_bin2(wtk_kwdec2_cfg_t *cfg)
{
	wtk_rbin2_delete(cfg->rbin);
    wtk_cfg_file_delete(cfg->cfile);
	wtk_kwdec2_cfg_clean(cfg);
	wtk_free(cfg);
}