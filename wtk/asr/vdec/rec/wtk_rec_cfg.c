#include "wtk_rec_cfg.h"

int wtk_rec_cfg_init(wtk_rec_cfg_t* cfg)
{
	memset(cfg,0,sizeof(*cfg));
	wtk_prune_cfg_init(&(cfg->prune));
	wtk_wfst_dnn_cfg_init(&(cfg->dnn));
	cfg->use_dnn=0;
	cfg->ntok=0;
	cfg->nbest=1;
	//cfg->word_thresh=cfg->gen_thresh=cfg->n_thresh=LSMALL;
	if(cfg->ntok==0)
	{
		cfg->n_beam=0.0;
		//cfg->n_beam=150;
	}else
	{
		cfg->n_beam=10000000000.f;
	}
	cfg->gen_beam=-LZERO;
	cfg->word_beam=-LZERO;
	cfg->lmscale=1.0;
	cfg->pscale=1.0;
	cfg->wordpen=0.0;
	cfg->path_coll_thresh=1024;
	cfg->align_coll_thresh=1024;
	cfg->state=0;
	cfg->model=1;
	cfg->bit_heap_min=4096;
	cfg->bit_heap_max=40960;
	cfg->bit_heap_growf=0.5;
	//--------- init hlda ---------
	cfg->hlda_fn=0;
	cfg->hlda_matrix=0;
	cfg->use_prune=0;
	return 0;
}

int wtk_rec_cfg_clean(wtk_rec_cfg_t* cfg)
{
	wtk_prune_cfg_clean(&(cfg->prune));
	if(cfg->hlda_matrix)
	{
		wtk_matrix_delete(cfg->hlda_matrix);
	}
	wtk_wfst_dnn_cfg_clean(&(cfg->dnn));
	return 0;
}

int wtk_rec_cfg_update_local(wtk_rec_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_b(lc,cfg,use_prune,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,path_coll_thresh,v);
	//wtk_debug("%d\n",cfg->path_coll_thresh);
	wtk_local_cfg_update_cfg_i(lc,cfg,align_coll_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bit_heap_min,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bit_heap_max,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,bit_heap_growf,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,ntok,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nbest,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,lmscale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pscale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,wordpen,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,n_beam,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,gen_beam,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,word_beam,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,state,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,model,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,hlda_fn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dnn,v);
	lc=wtk_local_cfg_find_lc_s(main,"dnn");
	if(lc)
	{
		ret=wtk_wfst_dnn_cfg_update_local(&(cfg->dnn),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"prune");
	if(lc)
	{
		ret=wtk_prune_cfg_update_local(&(cfg->prune),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_rec_cfg_update(wtk_rec_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_rec_cfg_update2(cfg,&(sl));
}

int wtk_rec_cfg_update2(wtk_rec_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret=0;

	if(cfg->ntok>1)
	{
		cfg->n_beam=cfg->gen_beam;
	}
	if(cfg->nbest<=0)
	{
		cfg->nbest=1;
	}
	if(cfg->hlda_fn)
	{
		ret=wtk_source_loader_load(sl,&(cfg->hlda_matrix),(wtk_source_load_handler_t)wtk_hlda_read,cfg->hlda_fn);
		//ret=wtk_source_load_file(&(cfg->hlda_mat),(wtk_source_load_handler_t)wtk_load_hlda,cfg->hlda_fn);
		if(ret!=0){goto end;}
	}
	if(cfg->use_dnn)
	{
		ret=wtk_wfst_dnn_cfg_update(&(cfg->dnn),sl);
		if(ret!=0){goto end;}
	}
	wtk_prune_cfg_update(&(cfg->prune));
end:
	return ret;
}
