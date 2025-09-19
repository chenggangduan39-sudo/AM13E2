#include "wtk_fst_net_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
void wtk_fst_net_nwlp_delete(wtk_fst_net_nwlp_t *nwlp);

int wtk_fst_net_nwlp_load2(wtk_fst_net_nwlp_t *lp,wtk_source_t *src)
{
	int ret;
	unsigned int idx;
	float prob;
	int is_bin=1;

	src->swap=0;
	while(1)
	{
		ret=wtk_source_read_int(src,(int*)&idx,1,is_bin);
		if(ret!=0){ret=0;goto end;}
		ret=wtk_source_read_float(src,&(prob),1,is_bin);
		if(ret!=0){goto end;}
		//wtk_debug("v[%d]=%f\n",idx,prob);
		lp->probs[idx]=prob;
	}
	ret=0;
end:
	return ret;
}

int wtk_fst_net_nwlp_load3(wtk_fst_net_nwlp_t *lp,char *fn)
{
	FILE *f;
	int ret;
	unsigned int idx;
	float prob;

	f=fopen(fn,"rb");
	if(!f){ret=-1;goto end;}
	while(1)
	{
		ret=fread(&(idx),sizeof(int),1,f);
		if(ret!=1){ret=0;goto end;}
		ret=fread(&(prob),sizeof(float),1,f);
		if(ret!=1){ret=-1;goto end;}
		lp->probs[idx]=prob;
	}
	ret=0;
end:
	if(f)
	{
		fclose(f);
	}
	return ret;
}

int wtk_fst_net_nwlp_load(wtk_fst_net_nwlp_t *lp,char *fn,float scale)
{
	FILE *f;
	int ret;
	unsigned int idx;
	float prob;

	f=fopen(fn,"rb");
	if(!f){ret=-1;goto end;}
	for(idx=0;idx<lp->n;++idx)
	{
		ret=fread(&(prob),sizeof(float),1,f);
		if(ret!=1){ret=-1;goto end;}
		lp->probs[idx]=prob*scale;
	}
	ret=0;
end:
	if(f)
	{
		fclose(f);
	}
	return ret;
}

wtk_fst_net_nwlp_t* wtk_fst_net_nwlp_new(char *fn,unsigned int n,float scale)
{
	wtk_fst_net_nwlp_t *nwlp;
	int ret;

	nwlp=(wtk_fst_net_nwlp_t*)wtk_malloc(sizeof(wtk_fst_net_nwlp_t));
	//nwlp->n=wtk_file_lines(fn);
	nwlp->n=n;//63793267;
	nwlp->probs=(float*)wtk_malloc(sizeof(float)*nwlp->n);
	//ret=wtk_source_load_file(nwlp,(wtk_source_load_handler_t)wtk_fst_net_nwlp_load,fn);
	ret=wtk_fst_net_nwlp_load(nwlp,fn,scale);
	if(ret!=0)
	{
		wtk_fst_net_nwlp_delete(nwlp);
		nwlp=NULL;
	}
	return nwlp;
}

int wtk_fst_net_nwlp_bytes(wtk_fst_net_nwlp_t *n)
{
	int bytes;

	bytes=sizeof(*n);
	bytes+=n->n*sizeof(float);
	return bytes;
}

void wtk_fst_net_nwlp_delete(wtk_fst_net_nwlp_t *nwlp)
{
	if(nwlp->probs)
	{
		wtk_free(nwlp->probs);
	}
	wtk_free(nwlp);
}


int wtk_fst_net_cfg_init(wtk_fst_net_cfg_t *cfg)
{
	cfg->symbol_in_fn=0;
	cfg->symbol_out_fn=0;
	cfg->lm_look_ahead_fn=0;
	cfg->n_state=-1;
	cfg->n_final_state=-1;
	cfg->n_trans=-1;
	cfg->lmscale=1.0;
	cfg->wordpen=0.0;
	cfg->eps_id=0;
	cfg->snt_end_id=1;
	cfg->snt_start_id=2;

	wtk_fst_binet_cfg_init(&(cfg->bin));
	cfg->trans_cache=50000;
	cfg->state_cache=50000;
	cfg->trans_reset_cache=50000;
	cfg->state_reset_cache=50000;

	cfg->use_bin=1;

	cfg->sym_in=0;
	cfg->sym_out=0;
	cfg->hash_scale=7;
	cfg->min_hash_size=-1;//10000001;
	cfg->use_shash=0;
	cfg->array_nslot2=256;

	cfg->label=NULL;
	cfg->nwlp=NULL;
	cfg->use_cheap=0;
	cfg->lm_look_scale=0.3;
	cfg->symin_use_hash=0;

	cfg->use_in_bin=0;
	cfg->use_out_bin=0;
	cfg->use_rbin=0;

	cfg->fsm_fn=NULL;
	cfg->load_all=0;
	cfg->reset_max_bytes=-1;
	cfg->use_dynamic_reset=0;
	return 0;
}

int wtk_fst_net_cfg_clean(wtk_fst_net_cfg_t *cfg)
{
	if(cfg->nwlp)
	{
		wtk_fst_net_nwlp_delete(cfg->nwlp);
	}
	if(cfg->label)
	{
		wtk_label_delete(cfg->label);
	}
	if(cfg->sym_in)
	{
		wtk_fst_insym_delete(cfg->sym_in);
	}
	if(cfg->sym_out)
	{
		//wtk_debug("delete sym_out=%p\n",cfg->sym_out);
		wtk_fst_sym_delete(cfg->sym_out);
	}
	wtk_fst_binet_cfg_clean(&(cfg->bin));
	return 0;
}

int wtk_fst_net_cfg_bytes(wtk_fst_net_cfg_t *cfg)
{
	int bytes=0;

	if(cfg->sym_in)
	{
		bytes+=wtk_fst_insym_bytes(cfg->sym_in);
	}
	if(cfg->sym_out)
	{
		bytes+=wtk_fst_sym_bytes(cfg->sym_out);
	}
	bytes+=wtk_fst_binet_cfg_bytes(&(cfg->bin));
	return bytes;
}

int wtk_fst_net_cfg_update_local(wtk_fst_net_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	//wtk_local_cfg_print(main);
	lc=main;

	wtk_local_cfg_update_cfg_f(lc,cfg,reset_max_bytes,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,use_dynamic_reset,v);

	wtk_local_cfg_update_cfg_str(lc,cfg,symbol_in_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,symbol_out_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,lm_look_ahead_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,fsm_fn,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,lm_look_scale,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,n_state,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,n_final_state,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,n_trans,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_cheap,v);
#ifdef __ANDROID__
#else
	wtk_local_cfg_update_cfg_b(lc,cfg,load_all,v);
#endif

	wtk_local_cfg_update_cfg_f(lc,cfg,lmscale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,wordpen,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,eps_id,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,snt_end_id,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,snt_start_id,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_bin,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_shash,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,trans_cache,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,state_cache,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,trans_reset_cache,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,state_reset_cache,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,hash_scale,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_hash_size,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,array_nslot2,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,symin_use_hash,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_in_bin,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_out_bin,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_rbin,v);
	if(cfg->use_bin)
	{
		lc=wtk_local_cfg_find_lc_s(main,"bin");
		if(lc)
		{
			ret=wtk_fst_binet_cfg_update_local(&(cfg->bin),lc);
			if(ret!=0){goto end;}
		}
	}
	ret=0;
end:
	if(cfg->use_dynamic_reset)
	{
		cfg->reset_max_bytes*=1024*1024;
		cfg->use_shash=0;
	}
	return ret;
}

int wtk_fst_net_cfg_update(wtk_fst_net_cfg_t *cfg)
{
	wtk_label_t *label;

	label=wtk_label_new(25007);
	cfg->label=label;
	return wtk_fst_net_cfg_update2(cfg,label);
}


int wtk_fst_net_cfg_update3(wtk_fst_net_cfg_t *cfg,wtk_label_t *label,wtk_source_loader_t *sl)
{
	int ret=-1;

	//wtk_debug("%s\n",cfg->symbol_in_fn);
	if(cfg->symbol_in_fn)
	{
		//wtk_fst_insym_t *wtk_fst_insym_new2(wtk_label_t *label,char *fn,int use_hash,wtk_source_loader_t *sl);
		cfg->sym_in=wtk_fst_insym_new3(label,cfg->symbol_in_fn,cfg->symin_use_hash,sl,cfg->use_in_bin);
		//cfg->sym_in=wtk_fst_insym_new(label,cfg->symbol_in_fn,cfg->symin_use_hash);
		if(!cfg->sym_in)
		{
			wtk_debug("load sym[%s] failed\n",cfg->symbol_in_fn);
			ret=-1;goto end;
		}
	}
	//wtk_debug("sym_in=%p\n",cfg->sym_in);
	if(cfg->symbol_out_fn)
	{
		cfg->sym_out=wtk_fst_sym_new3(label,cfg->symbol_out_fn,sl,cfg->use_out_bin);
		if(!cfg->sym_out)
		{
			wtk_debug("load sym[%s] failed\n",cfg->symbol_out_fn);
			ret=-1;goto end;
		}
	}
//	wtk_debug("aaaaaaaaaa\n");
	if(cfg->use_bin)
	{
//		wtk_debug("bbbbbbbbb\n");
		ret=wtk_fst_binet_cfg_update2(&(cfg->bin),sl);
		if(ret!=0)
		{
			wtk_debug("update bin failed\n");
			goto end;
		}
	}
	if(cfg->lm_look_ahead_fn)
	{
		cfg->nwlp=wtk_fst_net_nwlp_new(cfg->lm_look_ahead_fn,cfg->bin.ndx-1,cfg->lm_look_scale);
	}
	ret=0;
end:
	//wtk_debug("%s\n",cfg->symbol_out_fn);
	return ret;
}

int wtk_fst_net_cfg_update2(wtk_fst_net_cfg_t *cfg,wtk_label_t *label)
{
	wtk_source_loader_t file_sl;

	file_sl.hook=0;
	file_sl.vf=wtk_source_load_file_v;
	return wtk_fst_net_cfg_update3(cfg,label,&(file_sl));
}



int wtk_fst_net_cfg_is_visible_wrd(wtk_fst_net_cfg_t *cfg,unsigned int id)
{
	if(id==cfg->snt_end_id || id==cfg->eps_id ||id==cfg->snt_start_id)
	//if(id==cfg->eps_id)
	{
		return 0;
	}else
	{
		return 1;
	}
}


void wtk_fst_net_cfg_print(wtk_fst_net_cfg_t *cfg)
{
	wtk_debug("============== network =================\n");
	printf("nstate: %d\n",cfg->n_state);
	printf("ntrans: %d\n",cfg->n_trans);
	printf("final: %d\n",cfg->n_final_state);
}
