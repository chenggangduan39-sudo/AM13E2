#include "wtk_egram_cfg.h"
#include "wtk/core/cfg/wtk_source.h"

int wtk_egram_cfg_init(wtk_egram_cfg_t *cfg)
{
	wtk_string_set_s(&(cfg->symout_fn),"final.out.r");
	wtk_string_set_s(&(cfg->fsm_fn),"fsm");
	wtk_string_set_s(&(cfg->ebnf_fn),"ebnf");
	wtk_e2fst_cfg_init(&(cfg->e2fst));
	wtk_xbnfnet_cfg_init(&(cfg->xbnfnet));
        cfg->cfg.main_cfg = NULL;
        cfg->dict_fn = NULL;
        cfg->phn_hash_hint=257;
	cfg->wrd_hash_hint=25007;
	cfg->lower=0;
	cfg->use_ebnf=0;
	cfg->use_wordnet=0;
	cfg->use_bin=1;
	cfg->use_kwdec=0;
	cfg->hmm_expand=0;
	cfg->dict=NULL;
	cfg->label=NULL;
	cfg->cfile=0;
	cfg->use_leak=0;
	cfg->rbin=0;
	cfg->heap =wtk_heap_new(128);
	cfg->use_apron = 0;
	cfg->apron_conn = 0;
	cfg->type = 0;
	cfg->sym_fn=0;
	cfg->mdl_fn=0;
	cfg->fst_fn=0;

	return 0;
}

int wtk_egram_cfg_clean(wtk_egram_cfg_t *cfg)
{
	wtk_xbnfnet_cfg_clean(&(cfg->xbnfnet));
	wtk_e2fst_cfg_clean(&(cfg->e2fst));
	if(cfg->dict)
	{
		wtk_dict_delete(cfg->dict);
	}
	if(cfg->label)
	{
		wtk_label_delete(cfg->label);
	}
	if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
	if(cfg->cfile)
	{
		wtk_cfg_file_delete(cfg->cfile);
	}
	if (cfg->heap)
		wtk_heap_delete(cfg->heap);
	return 0;
}

int wtk_egram_cfg_update_local(wtk_egram_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	//wtk_local_cfg_print(lc);
	wtk_local_cfg_update_cfg_str(lc,cfg,dict_fn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,lower,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ebnf,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_wordnet,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_kwdec,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_bin,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,hmm_expand,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_leak,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,symout_fn,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,fsm_fn,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,ebnf_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,fst_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,sym_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,mdl_fn,v);

	lc=wtk_local_cfg_find_lc_s(main,"e2fst");
	if(lc)
	{
		ret=wtk_e2fst_cfg_update_local(&(cfg->e2fst),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"xbnfnet");
	if(lc)
	{
		ret=wtk_xbnfnet_cfg_update_local(&(cfg->xbnfnet),lc);
		if(ret!=0){goto end;}
	}
	lc=main;
	wtk_local_cfg_update_cfg_i(lc,cfg,use_leak,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,use_apron,v);
	if (cfg->use_apron)
	{
		wtk_array_t* tmpa;
		tmpa = wtk_local_cfg_find_array_s(lc,"apron_conn_a");
		if (tmpa)
			cfg->apron_conn = tmpa;
		else
			cfg->apron_conn = wtk_str_to_array(cfg->heap, "-,_", strlen("-,_"), ',');
	}
	ret=0;
end:
	return ret;
}

int wtk_egram_cfg_update(wtk_egram_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_egram_cfg_update2(cfg,&sl);
}

int wtk_egram_cfg_update2(wtk_egram_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	//wtk_debug("bin=%d\n",cfg->use_bin);
	if(!cfg->use_bin)
	{
		cfg->label=wtk_label_new(25007);
		cfg->dict=wtk_dict_new2(cfg->label,0,cfg->phn_hash_hint,cfg->wrd_hash_hint);
		ret=wtk_source_loader_load(sl,cfg->dict,(wtk_source_load_handler_t)wtk_dict_load,cfg->dict_fn);
		//int wtk_source_loader_load(wtk_source_loader_t *l,void *data_ths,wtk_source_load_handler_t loader,char *fn);
		if(ret!=0){goto end;}
	}
	ret=wtk_e2fst_cfg_update2(&(cfg->e2fst),cfg->label,sl);
	if(ret!=0){goto end;}
	if(cfg->use_ebnf)
	{
	}else
	{
		//ret=wtk_xbnfnet_cfg_update(&(cfg->xbnfnet));
		ret=wtk_xbnfnet_cfg_update2(&(cfg->xbnfnet), sl);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_egram_cfg_delete_bin(wtk_egram_cfg_t *cfg)
{
	wtk_egram_cfg_clean(cfg);
	wtk_free(cfg);
	return 0;
}

wtk_egram_cfg_t *wtk_egram_cfg_new_bin2(char *fn) {
    wtk_mbin_cfg_t *bin_cfg;
    wtk_egram_cfg_t *cfg = NULL;

    bin_cfg = wtk_mbin_cfg_new_type(wtk_egram_cfg, fn, "./egram.cfg.r");
    if (!bin_cfg) {
        goto end;
    }
    cfg = (wtk_egram_cfg_t *)bin_cfg->cfg;
    cfg->cfg.bin_cfg = bin_cfg;

end:
    return cfg;
}

wtk_egram_cfg_t* wtk_egram_cfg_new_bin(char *bin_fn,unsigned int seek_pos)
{
	wtk_egram_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./egram.cfg.r";
	int ret;

	cfg=(wtk_egram_cfg_t*)wtk_malloc(sizeof(wtk_egram_cfg_t));
	wtk_egram_cfg_init(cfg);
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
	ret=wtk_egram_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=wtk_egram_cfg_update2(cfg,&sl);
	if(ret!=0){goto end;}
end:
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		wtk_egram_cfg_delete_bin(cfg);
		cfg=NULL;
	}
	return cfg;
}
