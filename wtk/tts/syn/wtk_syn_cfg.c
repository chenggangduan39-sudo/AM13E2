#include "wtk_syn_cfg.h" 

int wtk_syn_win_cfg_init(wtk_syn_win_cfg_t *cfg)
{
	wtk_syn_dwin_cfg_init(&(cfg->lf0_cfg));
	wtk_syn_dwin_cfg_init(&(cfg->mcp_cfg));
	wtk_syn_dwin_cfg_init(&(cfg->bap_cfg));
	cfg->lf0=NULL;
	cfg->mcp=NULL;
	cfg->bap=NULL;
	return 0;
}

int wtk_syn_win_cfg_clean(wtk_syn_win_cfg_t *cfg)
{
	wtk_syn_dwin_cfg_clean(&(cfg->lf0_cfg));
	wtk_syn_dwin_cfg_clean(&(cfg->mcp_cfg));
	wtk_syn_dwin_cfg_clean(&(cfg->bap_cfg));
	if(cfg->lf0)
	{
		wtk_syn_dwin_delete(cfg->lf0);
	}
	if(cfg->mcp)
	{
		wtk_syn_dwin_delete(cfg->mcp);
	}
	if(cfg->bap)
	{
		wtk_syn_dwin_delete(cfg->bap);
	}
	return 0;
}



int wtk_syn_win_cfg_update_local(wtk_syn_win_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	int ret;

	lc=wtk_local_cfg_find_lc_s(main,"lf0");
	if(lc)
	{
		ret=wtk_syn_dwin_cfg_update_local(&(cfg->lf0_cfg),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"mcp");
	if(lc)
	{
		ret=wtk_syn_dwin_cfg_update_local(&(cfg->mcp_cfg),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"bap");
	if(lc)
	{
		ret=wtk_syn_dwin_cfg_update_local(&(cfg->bap_cfg),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_syn_win_cfg_update(wtk_syn_win_cfg_t *cfg,wtk_strpool_t *pool)
{
	wtk_source_loader_t sl;

	wtk_source_loader_init_file(&(sl));
	return wtk_syn_win_cfg_update2(cfg,&(sl),pool);
}

int wtk_syn_win_cfg_update2(wtk_syn_win_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool)
{
	int ret;

	ret=wtk_syn_dwin_cfg_update(&(cfg->lf0_cfg));
	if(ret!=0){goto end;}
	ret=wtk_syn_dwin_cfg_update(&(cfg->mcp_cfg));
	if(ret!=0){goto end;}
	ret=wtk_syn_dwin_cfg_update(&(cfg->bap_cfg));
	if(ret!=0){goto end;}
	cfg->lf0=wtk_syn_dwin_new(&(cfg->lf0_cfg),sl,pool);
	cfg->mcp=wtk_syn_dwin_new(&(cfg->mcp_cfg),sl,pool);
	cfg->bap=wtk_syn_dwin_new(&(cfg->bap_cfg),sl,pool);
	ret=0;
end:
	return ret;
}



int wtk_syn_cfg_init(wtk_syn_cfg_t *cfg)
{
	wtk_syn_dtree_cfg_init(&(cfg->tree_cfg));
	wtk_syn_hmm_cfg_init(&(cfg->hmm_cfg));
	wtk_syn_win_cfg_init(&(cfg->win));
	cfg->tree=NULL;
	cfg->hmm=NULL;
	cfg->frame=20;
	cfg->use_stream=1;
	cfg->stream_pad_sil_dur=2;
	cfg->stream_min_dur=40;
	cfg->steam_min_left_dur=20;
	cfg->f0_norm_win=0;
	return 0;
}

int wtk_syn_cfg_clean(wtk_syn_cfg_t *cfg)
{
	wtk_syn_dtree_cfg_clean(&(cfg->tree_cfg));
	wtk_syn_hmm_cfg_clean(&(cfg->hmm_cfg));
	wtk_syn_win_cfg_clean(&(cfg->win));
	if(cfg->tree)
	{
		wtk_syn_dtree_delete(cfg->tree);
	}
	if(cfg->hmm)
	{
		wtk_syn_hmm_delete(cfg->hmm);
	}
	return 0;
}

int wtk_syn_cfg_bytes(wtk_syn_cfg_t *cfg)
{
	int bytes;

	bytes=sizeof(wtk_syn_cfg_t);
	return bytes;
}

int wtk_syn_cfg_update_local(wtk_syn_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret=-1;

	lc=main;
	wtk_local_cfg_update_cfg_i(lc,cfg,frame,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,stream_min_dur,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,stream_pad_sil_dur,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,steam_min_left_dur,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,f0_norm_win,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_stream,v);
	lc=wtk_local_cfg_find_lc_s(main,"tree");
	if(lc)
	{
		ret=wtk_syn_dtree_cfg_update_local(&(cfg->tree_cfg),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"hmm");
	if(lc)
	{
		ret=wtk_syn_hmm_cfg_update_local(&(cfg->hmm_cfg),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"win");
	if(lc)
	{
		ret=wtk_syn_win_cfg_update_local(&(cfg->win),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_syn_cfg_update(wtk_syn_cfg_t *cfg)
{
	return wtk_syn_cfg_update2(cfg,NULL);
}


int wtk_syn_cfg_update2(wtk_syn_cfg_t *cfg,wtk_strpool_t *pool)
{
	wtk_source_loader_t sl;

	wtk_source_loader_init_file(&(sl));
	return  wtk_syn_cfg_update3(cfg,&sl,pool);
}

int wtk_syn_cfg_update3(wtk_syn_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool)
{
	int ret;

	if(!pool)
	{
		pool=wtk_strpool_new(15007);
	}
	ret=wtk_syn_dtree_cfg_update2(&(cfg->tree_cfg),sl);
	if(ret!=0){goto end;}
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	ret=wtk_syn_hmm_cfg_update2(&(cfg->hmm_cfg),sl);
	if(ret!=0){goto end;}
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	ret=wtk_syn_win_cfg_update2(&(cfg->win),sl,pool);
	if(ret!=0){goto end;}
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	ret=-1;
	cfg->tree=wtk_syn_dtree_new(&(cfg->tree_cfg),sl,pool);
	if(!cfg->tree){goto end;}
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	cfg->hmm=wtk_syn_hmm_new(&(cfg->hmm_cfg),sl,pool,cfg->tree);
	if(!cfg->hmm){goto end;}
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	//exit(0);
	ret=0;
end:
	return ret;
}
