#include "wtk_wfstr_cfg.h"

int wtk_wfstrec_cfg_init(wtk_wfstrec_cfg_t *cfg)
{
	wtk_fst_net3_cfg_init(&(cfg->lat_net));
	wtk_wfst_dnn_cfg_init(&(cfg->dnn));
	wtk_prune_cfg_init(&(cfg->prune));
	cfg->tok_cache=100000;
	cfg->tok_reset_cache=1000;
	cfg->inst_cache=100000;
	cfg->pth_cache=100000;
	cfg->align_cache=100000;

	cfg->phn_end_beam=0;
	cfg->phn_start_beam=0;
	cfg->emit_beam=0;
	cfg->word_beam=0;
	cfg->min_log_exp = -log(-LZERO);
	cfg->use_single_best=1;
	cfg->use_dnn=0;
	cfg->use_prune=0;
	cfg->max_emit_hyps=0;
	cfg->use_transpose_hmm=1;
	cfg->ac_lookahead_alpha=0;
//	cfg->ntok=0;
//	cfg->ntok_beam=0;
	cfg->frame_dur=0.02;
	//cfg->use_merge_full_cmp=0;
	cfg->use_forceout=1;
	cfg->use_lat=0;
	cfg->lat_beam=0;
	cfg->inst_use_heap=1;
	cfg->use_eps_pth=0;

	cfg->dump_lat=0;

	cfg->state=0;
	cfg->model=0;
	cfg->sil_trans_scale=0;
	//cfg->ac_scale=1;
	//cfg->ac_trans_scale=1;

	//--------- init hlda ---------
	cfg->hlda_fn=0;
	cfg->hlda_matrix=0;
	cfg->use_hlda_bin=0;

	cfg->use_in_wrd_beam=0;

	cfg->snt_end_frame=25;

	cfg->use_max_like_path=0;
	cfg->max_final_tok_pad_like=0;
	cfg->add_wrd_sep=0;

	cfg->min_sil_end_frame=6;
	cfg->min_sil_frame=100;
	cfg->max_sil_wrd=15;
	cfg->min_sil_thresh=0.5;
	cfg->use_end_hint=0;
	cfg->lmscale2=0.03;

	cfg->ac_scale=1.0;
	cfg->trans_scale=0.0;
	cfg->custom_ac_scale=1.0;
	cfg->use_spre=1;

	cfg->use_phn_prune=0;
	wtk_prune_cfg_init(&(cfg->phn_prune));
	cfg->phn_prune_thresh=0;

	cfg->use_wrd_prune=0;
	wtk_prune_cfg_init(&(cfg->wrd_prune));
	cfg->wrd_prune_thresh=0;

	cfg->use_expand_prune=0;
	cfg->expand_prune_thresh=0;
	wtk_prune_cfg_init(&(cfg->expand_prune));
	return 0;
}

int wtk_wfstrec_cfg_clean(wtk_wfstrec_cfg_t *cfg)
{
	/*
	if(cfg->hlda_fix)
	{
		wtk_fixc_delete(cfg->hlda_fix);
	}*/
	if(cfg->hlda_matrix)
	{
		wtk_matrix_delete(cfg->hlda_matrix);
	}
	wtk_fst_net3_cfg_clean(&(cfg->lat_net));
	wtk_wfst_dnn_cfg_clean(&(cfg->dnn));
	return 0;
}

int wtk_wfstrec_cfg_bytes(wtk_wfstrec_cfg_t *cfg)
{
	int bytes=0;

	bytes+=wtk_wfst_dnn_cfg_bytes(&(cfg->dnn));
	return bytes;
}


int wtk_wfstrec_cfg_update_local(wtk_wfstrec_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;

	wtk_local_cfg_update_cfg_f(lc,cfg,lmscale2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_end_hint,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_spre,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_sil_end_frame,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_sil_frame,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_sil_wrd,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_sil_thresh,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,tok_cache,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,tok_reset_cache,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,inst_cache,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pth_cache,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_emit_hyps,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,phn_end_beam,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,phn_start_beam,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,emit_beam,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,word_beam,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_single_best,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dnn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_prune,v);
	if(!cfg->use_prune)
	{
		wtk_local_cfg_update_cfg_b2(lc,cfg,use_prune,use_histogram,v);
	}
	wtk_local_cfg_update_cfg_b(lc,cfg,use_transpose_hmm,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_forceout,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_lat,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_eps_pth,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,inst_use_heap,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,dump_lat,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_in_wrd_beam,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,frame_dur,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,ac_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,trans_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,custom_ac_scale,v);

//	wtk_local_cfg_update_cfg_i(lc,cfg,ntok,v);
//	wtk_local_cfg_update_cfg_f(lc,cfg,ntok_beam,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,ac_lookahead_alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,lat_beam,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sil_trans_scale,v);


	wtk_local_cfg_update_cfg_str_local(lc,cfg,hlda_fn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_hlda_bin,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,dump_lat,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,align_cache,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,state,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,model,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,snt_end_frame,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_max_like_path,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_final_tok_pad_like,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,add_wrd_sep,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,phn_prune_thresh,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_phn_prune,v);
	lc=wtk_local_cfg_find_lc_s(main,"phn_prune");
	if(lc)
	{
		ret=wtk_prune_cfg_update_local(&(cfg->phn_prune),lc);
		if(ret!=0){goto end;}
	}

	wtk_local_cfg_update_cfg_i(lc,cfg,wrd_prune_thresh,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_wrd_prune,v);
	lc=wtk_local_cfg_find_lc_s(main,"wrd_prune");
	if(lc)
	{
		ret=wtk_prune_cfg_update_local(&(cfg->wrd_prune),lc);
		if(ret!=0){goto end;}
	}

	wtk_local_cfg_update_cfg_i(lc,cfg,expand_prune_thresh,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_expand_prune,v);
	lc=wtk_local_cfg_find_lc_s(main,"expand_prune");
	if(lc)
	{
		ret=wtk_prune_cfg_update_local(&(cfg->expand_prune),lc);
		if(ret!=0){goto end;}
	}

	if(cfg->use_dnn)
	{
		lc=wtk_local_cfg_find_lc_s(main,"dnn");
		if(lc)
		{
			ret=wtk_wfst_dnn_cfg_update_local(&(cfg->dnn),lc);
			if(ret!=0){goto end;}
		}
	}
	if(cfg->use_prune)
	{
		lc=wtk_local_cfg_find_lc_s(main,"prune");
		if(!lc)
		{
			lc=wtk_local_cfg_find_lc_s(main,"histogram");
		}
		if(lc)
		{
			ret=wtk_prune_cfg_update_local(&(cfg->prune),lc);
			if(ret!=0){goto end;}
		}
	}
	if(cfg->use_lat)
	{
		lc=wtk_local_cfg_find_lc_s(main,"lat_net");
		if(lc)
		{
			ret=wtk_fst_net3_cfg_update_local(&(cfg->lat_net),lc);
			if(ret!=0){goto end;}
		}
	}
	ret=0;
end:
	return ret;
}

int wtk_wfstrec_cfg_update(wtk_wfstrec_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_wfstrec_cfg_update2(cfg,&sl);
}

int wtk_wfstrec_cfg_update2(wtk_wfstrec_cfg_t *cfg, wtk_source_loader_t *sl)
{
	int ret;

	if(cfg->hlda_fn)
	{
		if(cfg->use_hlda_bin)
		{
			ret=wtk_source_loader_load(sl,&(cfg->hlda_matrix),(wtk_source_load_handler_t)wtk_source_read_hlda_bin,cfg->hlda_fn);
		}else
		{
			ret=wtk_source_loader_load(sl,&(cfg->hlda_matrix),(wtk_source_load_handler_t)wtk_hlda_read,cfg->hlda_fn);
		}
		//ret=wtk_source_load_file(&(cfg->hlda_mat),(wtk_source_load_handler_t)wtk_load_hlda,cfg->hlda_fn);
		if(ret!=0){goto end;}
		/*
		if(cfg->use_fix)
		{
			cfg->hlda_fix=wtk_fixc_new(cfg->hlda_matrix);
		}*/
	}
	if(cfg->use_dnn)
	{
		ret=wtk_wfst_dnn_cfg_update(&(cfg->dnn),sl);
		if(ret!=0){goto end;}
	}
	if(cfg->use_prune)
	{
		ret=wtk_prune_cfg_update(&(cfg->prune));
		if(ret!=0){goto end;}
	}
	if(cfg->use_wrd_prune)
	{
		ret=wtk_prune_cfg_update(&(cfg->wrd_prune));
		if(ret!=0){goto end;}
	}
	if(cfg->use_phn_prune)
	{
		ret=wtk_prune_cfg_update(&(cfg->phn_prune));
		if(ret!=0){goto end;}
	}
	if(cfg->use_expand_prune)
	{
		ret=wtk_prune_cfg_update(&(cfg->expand_prune));
		if(ret!=0){goto end;}
	}
	if(cfg->use_lat)
	{
		ret=wtk_fst_net3_cfg_update(&(cfg->lat_net));
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

void wtk_fst_rec_scale_sil_loop(wtk_wfstrec_cfg_t *cfg,wtk_matrix_t *m)
{
	int r,c;
	int i,j;
	double t;

	r=wtk_matrix_rows(m);
	c=wtk_matrix_cols(m);
	for(i=1;i<=r;++i)
	{
		for(j=1;j<=c;++j)
		{
			if((m[i][j]>LSMALL))
			{
				if(i==j)
				{
					t=m[i][j];
					m[i][j]=(t-log(1-exp(t)))*cfg->sil_trans_scale;
				}
			}
		}
	}
	//exit(0);
}

void wtk_wfstrec_cfg_update_hmmset(wtk_wfstrec_cfg_t *cfg,wtk_hmmset_t *hmmset)
{
	wtk_hmm_t *hmm;


	if(cfg->use_dnn)
	{
		wtk_wfst_dnn_cfg_attach_hmmset(&(cfg->dnn),hmmset);
		if(cfg->sil_trans_scale!=0)
		{
			hmm=wtk_hmmset_find_hmm_s(hmmset,"sil");
			if(hmm)
			{
				wtk_fst_rec_scale_sil_loop(cfg,hmm->transP);
			}
		}
	}
	//if(cfg->use_transpose_hmm)
	{
		wtk_hmmset_transpose_trans_matrix(hmmset);//,cfg->ac_trans_scale);
	}
}
