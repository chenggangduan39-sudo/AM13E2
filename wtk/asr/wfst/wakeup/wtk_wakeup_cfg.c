#include "wtk_wakeup_cfg.h" 

void wtk_wakeup_node_cfg_update_local(wtk_wakeup_node_t* cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	//wtk_local_cfg_print(lc);
	wtk_local_cfg_update_cfg_i(lc,cfg,idx,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_frame,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_frame2,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_frame,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_frame2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,cur_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pth_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pth_thresh2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_df,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,end_detect_win,v);
	cfg->pth_pow_thresh=pow(cfg->pth_thresh,cfg->depth+1);
	//wtk_debug("%f/%d/%f\n",cfg->pth_thresh,cfg->depth+1,cfg->pth_pow_thresh);
}


int wtk_wakeup_post_cfg_init(wtk_wakeup_post_cfg_t *cfg)
{
	cfg->min_raise=5;
	cfg->smooth_left=10;
	cfg->smooth_right=5;
	cfg->network=NULL;
	cfg->n_network=0;
	cfg->min_frame=30;
	cfg->left_win=0;
	cfg->right_win=0;
	cfg->avg_thresh=0;
	cfg->min_tri_frame=20;
	cfg->tri_win=2;
	cfg->scale_s=0.3;
	cfg->scale_e=0.8;
	cfg->min_s=0.2;
	cfg->min_e=0.1;
	cfg->max_end_win=0;
	cfg->pass_thresh=0.0;
	cfg->bg_thresh=0.0;
	cfg->sil_thresh=0.0;
	cfg->use_bg_check=0;
	cfg->min_win=20;
	cfg->use_half_check=0;
	cfg->debug=0;
	cfg->speech_sum_scale=1.0;
	cfg->min_hint_thresh=0.1;
	cfg->min_hint_cnt=3;
	return 0;
}

int wtk_wakeup_post_cfg_clean(wtk_wakeup_post_cfg_t *cfg)
{
	//wtk_debug("clean=%p/%p\n",cfg,cfg->network);
	if(cfg->network)
	{
		wtk_free(cfg->network);
	}
	return 0;
}

int wtk_wakeup_post_cfg_update_network(wtk_wakeup_post_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_queue_node_t *qn;
	wtk_cfg_item_t *item;
	wtk_wakeup_node_t *node;

	cfg->n_network=0;
	cfg->network=(wtk_wakeup_node_t*)wtk_malloc(sizeof(wtk_wakeup_node_t)*lc->cfg->queue.length);
	//wtk_debug("new network=%p/%p\n",cfg,cfg->network);
	for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
	{
		item=data_offset2(qn,wtk_cfg_item_t,n);
		if(item->type!=WTK_CFG_LC){continue;}
		node=cfg->network+cfg->n_network;
		node->depth=cfg->n_network;
		node->cur_thresh=0.15;
		node->pth_thresh=0.3;
		node->max_frame=30;
		node->max_frame2=0;
		node->min_frame=2;
		node->min_frame2=10;
		node->pth_thresh2=0;
		node->max_df=-0.9;
		node->end_detect_win=0;
		wtk_wakeup_node_cfg_update_local(node,item->value.cfg);
		//wtk_debug("%d\n",node->depth);
		++cfg->n_network;
	}
	return 0;
}

int wtk_wakeup_post_cfg_update_local(wtk_wakeup_post_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;

	lc=main;
	wtk_local_cfg_update_cfg_i(lc,cfg,min_win,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,left_win,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,right_win,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_frame,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_tri_frame,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_raise,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,smooth_left,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,smooth_right,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,tri_win,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,scale_s,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,scale_e,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_s,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_e,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,avg_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_hint_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_hint_cnt,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,speech_sum_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pass_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,bg_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sil_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_end_win,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_bg_check,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_half_check,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);
	lc=wtk_local_cfg_find_lc_s(main,"network");
	if(lc)
	{
		wtk_wakeup_post_cfg_update_network(cfg,lc);
	}
	return 0;
}

int wtk_wakeup_post_cfg_update(wtk_wakeup_post_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return 0;
}


int wtk_wakeup_cfg_init(wtk_wakeup_cfg_t *cfg)
{
	//wtk_string_set(&(cfg->word),"you",sizeof("you"));
	wtk_fextra_cfg_init(&(cfg->parm));
	wtk_wakeup_post_cfg_init(&(cfg->post));
	cfg->inst_cache=100;
	cfg->pth_cache=100;
	cfg->debug=0;
	cfg->rbin=0;
	cfg->use_win=1;
	cfg->max_win=120;
	cfg->print_v=0;
	cfg->log_wav=0;
	return 0;
}

int wtk_wakeup_cfg_clean(wtk_wakeup_cfg_t *cfg)
{
	wtk_wakeup_post_cfg_clean(&(cfg->post));
	if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
	if(cfg->cfile)
	{
		wtk_cfg_file_delete(cfg->cfile);
	}
	wtk_fextra_cfg_clean(&(cfg->parm));
	return 0;
}

int wtk_wakeup_cfg_delete_bin(wtk_wakeup_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->cfg.bin_cfg);
	return 0;
}

int wtk_wakeup_cfg_delete_bin2(wtk_wakeup_cfg_t *cfg)
{
	wtk_wakeup_cfg_clean(cfg);
	wtk_free(cfg);
	return 0;
}



int wtk_wakeup_cfg_update_local(wtk_wakeup_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;

	lc=main;
	wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,inst_cache,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pth_cache,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_win,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,use_win,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,word,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,log_wav,v);
	//wtk_debug("========%d=======\n",cfg->wsmooth);
	//wtk_debug("========%s=======\n",cfg->word.data);
	lc=wtk_local_cfg_find_lc_s(main,"post");
	if(lc)
	{
		wtk_wakeup_post_cfg_update_local(&(cfg->post),lc);
	}else
	{
		wtk_wakeup_post_cfg_update_local(&(cfg->post),main);
	}
	lc=wtk_local_cfg_find_lc_s(main,"parm");
	if(lc)
	{
		wtk_fextra_cfg_update_local(&(cfg->parm),lc);
	}
	return 0;
}

int wtk_wakeup_cfg_update(wtk_wakeup_cfg_t *cfg)
{
	wtk_source_loader_t sl;
	int ret;

	wtk_source_loader_init_file(&(sl));
	ret=wtk_fextra_cfg_update(&(cfg->parm));
	if(ret!=0){goto end;}
	ret=wtk_wakeup_post_cfg_update(&(cfg->post),&sl);
	if(ret!=0){goto end;}
end:
	return ret;
}


int wtk_wakeup_cfg_update2(wtk_wakeup_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	ret=wtk_fextra_cfg_update2(&(cfg->parm),sl);
	if(ret!=0){goto end;}
end:
	return ret;
}

wtk_wakeup_cfg_t* wtk_wakeup_cfg_new_bin3(char *bin_fn,unsigned int seek_pos)
{
	wtk_wakeup_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./wakeup.cfg";
	int ret;

	cfg=(wtk_wakeup_cfg_t*)wtk_malloc(sizeof(wtk_wakeup_cfg_t));
	//wtk_debug("new cfg=%p\n",cfg);
	wtk_wakeup_cfg_init(cfg);
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
	ret=wtk_wakeup_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=wtk_wakeup_cfg_update2(cfg,&sl);
	if(ret!=0){goto end;}
end:
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		wtk_wakeup_cfg_delete_bin2(cfg);
		cfg=NULL;
	}
	return cfg;
}

wtk_wakeup_cfg_t* wtk_wakeup_cfg_new_bin(char *bin_fn,char *cfg_fn)
{
	wtk_mbin_cfg_t *cfg;
	wtk_wakeup_cfg_t *wc;

	//wtk_debug("%s/%s\n",bin_fn,cfg_fn);
	cfg=wtk_mbin_cfg_new_type(wtk_wakeup_cfg,bin_fn,cfg_fn);
	wc=(wtk_wakeup_cfg_t*)(cfg->cfg);
	wc->cfg.bin_cfg=cfg;

	return wc;
}

wtk_wakeup_cfg_t* wtk_wakeup_cfg_new_bin2(char *bin_fn)
{
	return wtk_wakeup_cfg_new_bin(bin_fn,"./wakeup.cfg");
}

wtk_wakeup_cfg_t* wtk_wakeup_cfg_new(char *fn)
{
	wtk_main_cfg_t *cfg;
	wtk_wakeup_cfg_t *tcfg;

	cfg=wtk_main_cfg_new_type(wtk_wakeup_cfg,fn);
	if(cfg)
	{
		tcfg=(wtk_wakeup_cfg_t*)(cfg->cfg);
		tcfg->cfg.cfg=cfg;
		return tcfg;
	}else
	{
		return NULL;
	}
}

void wtk_wakeup_cfg_delete(wtk_wakeup_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->cfg.cfg);
}
