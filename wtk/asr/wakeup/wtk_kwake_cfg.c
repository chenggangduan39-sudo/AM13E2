#include "wtk_kwake_cfg.h" 

void wtk_kwake_peak_node_init(wtk_kwake_peak_node_t *node)
{
	node->id=-1;
	node->thresh_max=0.1;
	node->thresh_min=0.05;
	node->max_count=-1;
	node->max_prob=-1;
	node->low_thresh=-1;
	node->low_cnt=0;
	node->node=NULL;
	node->nnode=0;
	node->min_ratio=-1;
}

void wtk_kwake_peak_node_clean(wtk_kwake_peak_node_t *node)
{
	if(node->node)
	{
		wtk_free(node->node);
	}
}

void wtk_kwake_peak_node_update_local(wtk_kwake_peak_node_t *node,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	wtk_array_t *a;

	wtk_local_cfg_update_cfg_i(lc,node,id,v);
	wtk_local_cfg_update_cfg_i(lc,node,max_count,v);
	wtk_local_cfg_update_cfg_f(lc,node,thresh_max,v);
	wtk_local_cfg_update_cfg_f(lc,node,thresh_min,v);
	wtk_local_cfg_update_cfg_f(lc,node,low_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,node,low_cnt,v);
	wtk_local_cfg_update_cfg_f(lc,node,max_prob,v);
	wtk_local_cfg_update_cfg_f(lc,node,min_ratio,v);

	a=wtk_local_cfg_find_array_s(lc,"node");
	if(a)
	{
		int i;
		wtk_string_t **v;

		node->node=(int*)wtk_calloc(a->nslot,sizeof(int));
		node->nnode=a->nslot;
		v=(wtk_string_t**)(a->slot);
		for(i=0;i<a->nslot;++i)
		{
			node->node[i]=wtk_str_atoi(v[i]->data,v[i]->len);
		}
	}
}

void wtk_kwake_node_init(wtk_kwake_node_t *node)
{
	node->id=-1;
	node->wrd_depth=-1;
	node->min_frame=1;
	node->max_frame=3;
	node->prob=0.1;
	node->pth_prob=-1;
	node->wrd_prob=-1;
	node->wrd_min_phn_prob=-1;
	node->max_prev_triger=-1;
	node->max_prev_triger_thresh=-1;
	node->max_next_triger=-1;
	node->max_next_triger_thresh=-1;
	node->max_prev_ratio_triger=-1;
	node->max_prev_ratio=-1;
	node->max_next_ratio_triger=-1;
	node->max_next_ratio=-1;
	node->min_mid_ratio=-1;
	node->min_mid_ratio_triger=-1;
	node->speech_thresh=-1;
	node->speech_ratio=-1;
	node->speech_win=0;
	node->speech_min_win=-1;
	node->max_other_triger=-1;
	node->max_other_triger_thresh=-1;

	node->max_other_id=-1;
	node->max_other_id_triger=-1;
	node->max_other_id_triger_thresh=-1;

	node->max_prev_node=-1;
	node->max_next_node=-1;

	node->check_edge=1;

	node->char_depth=-1;
	node->char_min_phn_prob=-1;
	node->char_prob=-1;

	node->speech_dur_thresh=-1;
	node->speech_dur_win=0;
	node->speech_dur_min=0.1;
	node->speech_dur_win2=0;

	node->speech_inter_thresh=-1;
	node->speech_inter_max_win=20;
}

void wtk_kwake_node_clean(wtk_kwake_node_t *node)
{

}

void wtk_kwake_node_update_local(wtk_kwake_node_t *node,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_b(lc,node,check_edge,v);

	wtk_local_cfg_update_cfg_i(lc,node,max_prev_node,v);
	wtk_local_cfg_update_cfg_i(lc,node,max_next_node,v);

	wtk_local_cfg_update_cfg_i(lc,node,id,v);
	//wtk_local_cfg_update_cfg_i(lc,node,id2,v);
	wtk_local_cfg_update_cfg_i(lc,node,min_frame,v);
	wtk_local_cfg_update_cfg_i(lc,node,max_frame,v);

	wtk_local_cfg_update_cfg_f(lc,node,prob,v);
	wtk_local_cfg_update_cfg_f(lc,node,pth_prob,v);

	wtk_local_cfg_update_cfg_f(lc,node,wrd_prob,v);
	wtk_local_cfg_update_cfg_f(lc,node,wrd_min_phn_prob,v);
	wtk_local_cfg_update_cfg_i(lc,node,wrd_depth,v);

	wtk_local_cfg_update_cfg_i(lc,node,char_depth,v);
	wtk_local_cfg_update_cfg_f(lc,node,char_prob,v);
	wtk_local_cfg_update_cfg_f(lc,node,char_min_phn_prob,v);

	wtk_local_cfg_update_cfg_f(lc,node,max_prev_triger,v);
	wtk_local_cfg_update_cfg_f(lc,node,max_prev_triger_thresh,v);

	wtk_local_cfg_update_cfg_f(lc,node,max_next_triger,v);
	wtk_local_cfg_update_cfg_f(lc,node,max_next_triger_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,node,max_next_ratio_triger,v);
	wtk_local_cfg_update_cfg_f(lc,node,max_next_ratio,v);
	wtk_local_cfg_update_cfg_f(lc,node,max_prev_ratio_triger,v);
	wtk_local_cfg_update_cfg_f(lc,node,max_prev_ratio,v);

	wtk_local_cfg_update_cfg_f(lc,node,min_mid_ratio,v);
	wtk_local_cfg_update_cfg_f(lc,node,min_mid_ratio_triger,v);

	wtk_local_cfg_update_cfg_i(lc,node,speech_win,v);
	wtk_local_cfg_update_cfg_i(lc,node,speech_min_win,v);
	wtk_local_cfg_update_cfg_f(lc,node,speech_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,node,speech_ratio,v);

	wtk_local_cfg_update_cfg_f(lc,node,speech_dur_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,node,speech_dur_min,v);
	wtk_local_cfg_update_cfg_i(lc,node,speech_dur_win,v);
	wtk_local_cfg_update_cfg_i(lc,node,speech_dur_win2,v);

	wtk_local_cfg_update_cfg_i(lc,node,max_other_id,v);
	wtk_local_cfg_update_cfg_f(lc,node,max_other_id_triger,v);
	wtk_local_cfg_update_cfg_f(lc,node,max_other_id_triger_thresh,v);

	wtk_local_cfg_update_cfg_f(lc,node,max_other_triger,v);
	wtk_local_cfg_update_cfg_f(lc,node,max_other_triger_thresh,v);

	wtk_local_cfg_update_cfg_f(lc,node,speech_inter_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,node,speech_inter_max_win,v);
}

void wtk_kwake_node_update(wtk_kwake_node_t *node)
{

}

void wtk_kwake_post_cfg_init(wtk_kwake_post_cfg_t * cfg)
{
	wtk_kwake_post_rf_cfg_init(&(cfg->rf_cfg));
	cfg->quick_network=NULL;
	cfg->quick_min_prob=0.1;
	cfg->quick_max_phn_prob=0.85;
	cfg->wake_s_thresh=-1;
	cfg->wake_min_win=-1;
	cfg->network=NULL;
	cfg->nnet=0;
	cfg->prob=0.2;
	cfg->max_phn_prob=0.3;
	cfg->id=NULL;
	cfg->id_prob=NULL;
	cfg->bkg_thresh=-1;
	cfg->bkg_win=0;
	cfg->sil_thresh=-1;
	cfg->sil_win=0;
	cfg->min_bkg=0.4;
	cfg->peak=NULL;
	cfg->npeak=0;
	cfg->half_scale1=-1;
	cfg->half_scale2=-1;
	cfg->fix=NULL;
	cfg->use_rf=0;
 }

int wtk_kwake_post_cfg_bytes(wtk_kwake_post_cfg_t *cfg)
{
	int bytes;

	bytes=0;
	if(cfg->network)
	{
		bytes+=cfg->nnet*sizeof(wtk_kwake_node_t);
	}
	if(cfg->id)
	{
		bytes+=cfg->nnet*sizeof(int);
		bytes+=cfg->nnet*sizeof(float);
	}
	if(cfg->peak)
	{
		bytes+=cfg->npeak*sizeof(wtk_kwake_peak_node_t);
	}
	if(cfg->use_rf)
	{
		bytes+=cfg->rf_cfg.state_cnt*sizeof(int);
	}
	return bytes;
}

void wtk_kwake_post_cfg_clean(wtk_kwake_post_cfg_t * cfg)
{
	int i;

	if(cfg->network)
	{
		for(i=0;i<cfg->nnet;++i)
		{
			if(cfg->network[i].fix)
			{
				wtk_free(cfg->network[i].fix);
			}
		}
		wtk_free(cfg->network);
	}
	if(cfg->quick_network)
	{
		for(i=0;i<cfg->nnet;++i)
		{
			if(cfg->quick_network[i].fix)
			{
				wtk_free(cfg->quick_network[i].fix);
			}
		}
		wtk_free(cfg->quick_network);
	}
	if(cfg->peak)
	{
		for(i=0;i<cfg->npeak;++i)
		{
			if(cfg->peak[i].node)
			{
				wtk_free(cfg->peak[i].node);
			}
		}
		wtk_free(cfg->peak);
	}
	if(cfg->id)
	{
		wtk_free(cfg->id);
		wtk_free(cfg->id_prob);
	}
	if(cfg->fix)
	{
		if(cfg->fix->id_prob)
		{
			wtk_free(cfg->fix->id_prob);
		}
		wtk_free(cfg->fix);
	}
	if(cfg->use_rf)
	{
		wtk_kwake_post_rf_cfg_clean(&(cfg->rf_cfg));
	}
}

void wtk_kwake_post_cfg_update(wtk_kwake_post_cfg_t *cfg)
{

}

void wtk_kwake_post_cfg_update_local(wtk_kwake_post_cfg_t * cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;

	lc=main;
	//wtk_local_cfg_print(lc);

	wtk_local_cfg_update_cfg_f(lc,cfg,prob,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_phn_prob,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,bkg_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_bkg,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bkg_win,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,wake_s_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,wake_min_win,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,half_scale1,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,half_scale2,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,sil_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,sil_win,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,quick_min_prob,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,quick_max_phn_prob,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_rf,v);
	if(cfg->use_rf)
	{
		lc=wtk_local_cfg_find_lc_s(main,"rf_post");
		wtk_kwake_post_rf_cfg_update_local(&(cfg->rf_cfg),lc);
	}
	//wtk_debug("lpad=%d,rpad=%d,rf_thresh=%f\n",cfg->rf_cfg.rf_win_left_pad,cfg->rf_cfg.rf_win_right_pad,cfg->rf_cfg.rf_thresh);
	lc=wtk_local_cfg_find_lc_s(main,"network");
	if(lc)
	{
		wtk_queue_node_t *qn;
		wtk_cfg_item_t *item;
		wtk_kwake_node_t *node;
		int id[16]={0};
		float prob[16]={0};
		int nid=0;
		int i,j;

		cfg->network=(wtk_kwake_node_t*)wtk_calloc(lc->cfg->queue.length,sizeof(wtk_kwake_node_t));
		cfg->nnet=0;
        //wtk_debug("=======================> %p\n",cfg->network);
		for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			//wtk_debug("type=%d\n",item->type);
			if(item->type!=WTK_CFG_LC){continue;}
			//wtk_debug("type=%d\n",item->type);
			node=cfg->network+cfg->nnet;
			wtk_kwake_node_init(node);
			wtk_kwake_node_update_local(node,item->value.cfg);
			++cfg->nnet;
			//wtk_debug("id=%d\n",node->id);
			if(id[node->id]==0)
			{
				id[node->id]=1;
				++nid;
			}
			if(node->prob>prob[node->id])
			{
				prob[node->id]=node->prob;
			}
		}
		cfg->nid=nid;
		cfg->id=(int*)wtk_calloc(cfg->nid,sizeof(int));
		cfg->id_prob=(float*)wtk_calloc(cfg->nid,sizeof(float));
		for(i=0,j=0;i<16;++i)
		{
			if(id[i]!=0)
			{
				cfg->id[j]=i;
				cfg->id_prob[j]=prob[i];
				++j;
			}
		}
		//wtk_debug("nnet=%d\n",cfg->nnet);
	}
	lc=wtk_local_cfg_find_lc_s(main,"quick_network");
	if(lc)
	{
		wtk_queue_node_t *qn;
		wtk_cfg_item_t *item;
		wtk_kwake_node_t *node;
		int i;

		cfg->quick_network=(wtk_kwake_node_t*)wtk_calloc(lc->cfg->queue.length,sizeof(wtk_kwake_node_t));
		for(qn=lc->cfg->queue.pop,i=0;qn;qn=qn->next,++i)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			//wtk_debug("type=%d\n",item->type);
			if(item->type!=WTK_CFG_LC){continue;}
			//wtk_debug("type=%d\n",item->type);
			node=cfg->quick_network+i;
			wtk_kwake_node_init(node);
			wtk_kwake_node_update_local(node,item->value.cfg);
			//wtk_debug("id=%d\n",node->id);
		}
		//wtk_debug("nnet=%d\n",cfg->nnet);
	}
	lc=wtk_local_cfg_find_lc_s(main,"peak");
	if(lc)
	{
		wtk_queue_node_t *qn;
		wtk_cfg_item_t *item;
		wtk_kwake_peak_node_t *node;

		cfg->peak=(wtk_kwake_peak_node_t*)wtk_calloc(lc->cfg->queue.length,sizeof(wtk_kwake_peak_node_t));
		cfg->npeak=0;
		for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			//wtk_debug("type=%d\n",item->type);
			if(item->type!=WTK_CFG_LC){continue;}
			//wtk_debug("type=%d\n",item->type);
			node=cfg->peak+cfg->npeak;
			wtk_kwake_peak_node_init(node);
			wtk_kwake_peak_node_update_local(node,item->value.cfg);
			++cfg->npeak;
		}
	}
	wtk_kwake_post_cfg_update(cfg);
}

int wtk_kwake_post_cfg_min_end(wtk_kwake_post_cfg_t *post,int depth)
{
	int i,v;

	v=0;
	for(i=depth+1;i<post->nnet;++i)
	{
		v+=post->network[i].min_frame;
	}
	return v;
}

void wtk_kwake_feature_cfg_init(wtk_kwake_feature_cfg_t *cfg)
{
	cfg->nodes=NULL;
	cfg->nnode=0;
}

void wtk_kwake_feature_cfg_clean(wtk_kwake_feature_cfg_t *cfg)
{
	int i;

	if(cfg->nodes)
	{
		for(i=0;i<cfg->nnode;++i)
		{
			if(cfg->nodes[i].in)
			{
				wtk_free(cfg->nodes[i].in);
			}
		}
		wtk_free(cfg->nodes);
	}
}

void wtk_kwake_feat_node_init(wtk_kwake_feat_node_t *node)
{
	node->in=NULL;
	node->nin=-1;
	node->out=-1;
}

void wtk_kwake_feat_node_update_local(wtk_kwake_feat_node_t *node,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	wtk_array_t *a;
	int i;

	wtk_local_cfg_update_cfg_i(lc,node,out,v);
	a=wtk_local_cfg_find_array_s(lc,"in");
	if(a)
	{
		wtk_string_t **strs;

		strs=(wtk_string_t**)(a->slot);
		node->in=(int*)wtk_calloc(a->nslot,sizeof(int));
		node->nin=a->nslot;
		for(i=0;i<node->nin;++i)
		{
			node->in[i]=wtk_str_atoi(strs[i]->data,strs[i]->len);
		}
	}
}

int wtk_kwake_cfg_init(wtk_kwake_cfg_t *cfg)
{
	wtk_kxparm_cfg_init(&(cfg->parm));
	wtk_kwake_post_cfg_init(&(cfg->post));
	wtk_kwake_post_cfg_init(&(cfg->triger));
	wtk_kwake_post_cfg_init(&(cfg->echo));
	wtk_kwake_post_cfg_init(&(cfg->triger_quiet));

	wtk_kwake_post_cfg_init(&(cfg->echo_quick));
	wtk_kwake_post_cfg_init(&(cfg->quick));
	wtk_kwake_feature_cfg_init(&(cfg->feature));
	cfg->use_fixpoint =0;
	cfg->sil_idx=0;
	cfg->bkg_idx=1;
	cfg->smooth_left=5;
	cfg->smooth_right=2;
	cfg->max_win=200;
	cfg->min_win=10;
	cfg->debug=0;
	cfg->step=5;
	cfg->check_step=1;
	cfg->use_triger=0;
	cfg->use_full_triger=0;
	cfg->triger_time=30000;
	cfg->triger_len=0;
	cfg->use_echo=0;
	cfg->use_full_echo=0;
	cfg->use_quick=0;
	cfg->use_feature=0;
	cfg->quick_max_phn_prob=0.7;
	cfg->use_full_quick=0;
	cfg->quick_prob=0;

	cfg->use_triger_quiet=0;
	cfg->use_echo_quick=0;

	cfg->quiet_time=1000;
	cfg->normal_time=1000;
	cfg->quiet_max_prob=0.05;
	cfg->normal_min_prob=0.5;

	return 0;
}

int wtk_kwake_cfg_bytes(wtk_kwake_cfg_t *cfg)
{
	int bytes;

	bytes=sizeof(wtk_kwake_cfg_t);
	bytes+=wtk_kxparm_cfg_bytes(&(cfg->parm));
	bytes+=wtk_kwake_post_cfg_bytes(&(cfg->triger));
	bytes+=wtk_kwake_post_cfg_bytes(&(cfg->post));
	bytes+=wtk_kwake_post_cfg_bytes(&(cfg->echo));
	bytes+=wtk_kwake_post_cfg_bytes(&(cfg->quick));
	return bytes;
}

int wtk_kwake_cfg_clean(wtk_kwake_cfg_t *cfg)
{
	if(cfg->rbin)
	{
		wtk_rbin2_delete(cfg->rbin);
	}
	if(cfg->cfile)
	{
		wtk_cfg_file_delete(cfg->cfile);
	}
	wtk_kxparm_cfg_clean(&(cfg->parm));

	wtk_kwake_post_cfg_clean(&(cfg->triger_quiet));
	wtk_kwake_post_cfg_clean(&(cfg->echo_quick));

	wtk_kwake_post_cfg_clean(&(cfg->post));
	wtk_kwake_post_cfg_clean(&(cfg->triger));
	wtk_kwake_post_cfg_clean(&(cfg->echo));
	wtk_kwake_post_cfg_clean(&(cfg->quick));
	wtk_kwake_feature_cfg_clean(&(cfg->feature));
	return 0;
}

int wtk_kwake_cfg_update_local(wtk_kwake_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;

	lc=main;
	wtk_local_cfg_update_cfg_b(lc,cfg,use_triger_quiet,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_full_quick,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_echo_quick,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_echo,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_feature,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_quick,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_full_echo,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_triger,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_full_triger,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,triger_time,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bkg_idx,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,sil_idx,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,smooth_left,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,smooth_right,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_win,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_win,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,step,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,check_step,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,quick_max_phn_prob,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,quick_prob,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,quiet_time,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,normal_time,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,quiet_max_prob,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,normal_min_prob,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,use_fixpoint,v);
	lc=wtk_local_cfg_find_lc_s(main,"parm");
	if(lc)
	{
		wtk_kxparm_cfg_update_local(&(cfg->parm),lc);
	}
	lc=wtk_local_cfg_find_lc_s(main,"post");
	if(lc)
	{
		wtk_kwake_post_cfg_update_local(&(cfg->post),lc);
	}
	if(cfg->use_triger)
	{
		lc=wtk_local_cfg_find_lc_s(main,"triger");
		if(lc)
		{
			wtk_kwake_post_cfg_update_local(&(cfg->triger),lc);
		}
		if(cfg->use_triger_quiet)
		{
			lc=wtk_local_cfg_find_lc_s(main,"triger_quiet");
			if(lc)
			{
				wtk_kwake_post_cfg_update_local(&(cfg->triger_quiet),lc);
			}
		}
	}
	if(cfg->use_echo)
	{
		lc=wtk_local_cfg_find_lc_s(main,"echo");
		if(lc)
		{
			wtk_kwake_post_cfg_update_local(&(cfg->echo),lc);
		}
	}
	if(cfg->use_quick)
	{
		lc=wtk_local_cfg_find_lc_s(main,"quick");
		if(lc)
		{
			wtk_kwake_post_cfg_update_local(&(cfg->quick),lc);
		}
	}
	if(cfg->use_echo_quick)
	{
		lc=wtk_local_cfg_find_lc_s(main,"echo_quick");
		if(lc)
		{
			wtk_kwake_post_cfg_update_local(&(cfg->echo_quick),lc);
		}
	}
	if(cfg->use_feature)
	{
		lc=wtk_local_cfg_find_lc_s(main,"feature");
		if(lc)
		{
			wtk_queue_node_t *qn;
			wtk_cfg_item_t *item;
			wtk_kwake_feat_node_t *node;

			cfg->feature.nodes=(wtk_kwake_feat_node_t*)wtk_calloc(lc->cfg->queue.length,sizeof(wtk_kwake_feat_node_t));
			cfg->feature.nnode=0;
			for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
			{
				item=data_offset2(qn,wtk_cfg_item_t,n);
				//wtk_debug("type=%d\n",item->type);
				if(item->type!=WTK_CFG_LC){continue;}
				//wtk_debug("type=%d\n",item->type);
				node=cfg->feature.nodes+cfg->feature.nnode;
				wtk_kwake_feat_node_init(node);
				wtk_kwake_feat_node_update_local(node,item->value.cfg);
				++cfg->feature.nnode;
			}
		}
	}
	return 0;
}

void wtk_kwake_node_update_fix(wtk_kwake_node_t *node,int shift)
{
	wtk_kwake_node_fixprob_t *fix;
	//int shift=12;

	fix=node->fix=(wtk_kwake_node_fixprob_t*)wtk_malloc(sizeof(wtk_kwake_node_fixprob_t));
	fix->prob=FLOAT2FIX_ANY(node->prob,shift);
	fix->pth_prob=FLOAT2FIX_ANY(node->pth_prob,shift);
	fix->wrd_prob=FLOAT2FIX_ANY(node->wrd_prob,shift);
	fix->wrd_min_phn_prob=FLOAT2FIX_ANY(node->wrd_min_phn_prob,shift);
	fix->char_min_phn_prob=FLOAT2FIX_ANY(node->char_min_phn_prob,shift);
	fix->char_prob=FLOAT2FIX_ANY(node->char_prob,shift);
	fix->max_prev_triger=FLOAT2FIX_ANY(node->max_prev_triger,shift);
	fix->max_prev_triger_thresh=FLOAT2FIX_ANY(node->max_prev_triger_thresh,shift);
	fix->max_prev_ratio_triger=FLOAT2FIX_ANY(node->max_prev_ratio_triger,shift);
	fix->max_prev_ratio=FLOAT2FIX_ANY(node->max_prev_ratio,shift);
	fix->max_next_triger=FLOAT2FIX_ANY(node->max_next_triger,shift);
	fix->max_next_triger_thresh=FLOAT2FIX_ANY(node->max_next_triger_thresh,shift);
	fix->max_next_ratio_triger=FLOAT2FIX_ANY(node->max_next_ratio_triger,shift);
	fix->max_next_ratio=FLOAT2FIX_ANY(node->max_next_ratio,shift);

	fix->min_mid_ratio_triger=FLOAT2FIX_ANY(node->min_mid_ratio_triger,shift);
	fix->min_mid_ratio=FLOAT2FIX_ANY(node->min_mid_ratio,shift);
	fix->max_other_triger=FLOAT2FIX_ANY(node->max_other_triger,shift);
	fix->max_other_triger_thresh=FLOAT2FIX_ANY(node->max_other_triger_thresh,shift);
	fix->max_other_id_triger=FLOAT2FIX_ANY(node->max_other_id_triger,shift);
	fix->max_other_id_triger_thresh=FLOAT2FIX_ANY(node->max_other_id_triger_thresh,shift);
	fix->speech_thresh=FLOAT2FIX_ANY(node->speech_thresh,shift);
	fix->speech_ratio=FLOAT2FIX_ANY(node->speech_ratio,shift);
	fix->speech_dur_thresh=FLOAT2FIX_ANY(node->speech_dur_thresh,shift);
	fix->speech_dur_min=FLOAT2FIX_ANY(node->speech_dur_min,shift);
}

void wtk_kwake_post_cfg_update_fix(wtk_kwake_post_cfg_t *post,int shift)
{
	wtk_kwake_post_cfg_fixpob_t *fix;
	int i;

	fix=post->fix=(wtk_kwake_post_cfg_fixpob_t*)wtk_malloc(sizeof(wtk_kwake_post_cfg_fixpob_t));
	fix->wake_s_thresh=FLOAT2FIX_ANY(post->wake_s_thresh,shift);
	fix->prob=FLOAT2FIX_ANY(pow(post->prob,post->nnet),shift);
	fix->max_phn_prob=FLOAT2FIX_ANY(post->max_phn_prob,shift);
	fix->sil_thresh=FLOAT2FIX_ANY(post->sil_thresh,shift);
	fix->bkg_thresh=FLOAT2FIX_ANY(post->bkg_thresh,shift);
	fix->min_bkg=FLOAT2FIX_ANY(post->min_bkg,shift);
	fix->half_scale1=FLOAT2FIX_ANY(post->half_scale1,shift);
	fix->half_scale2=FLOAT2FIX_ANY(post->half_scale2,shift);
	fix->id_prob=(int*)wtk_calloc(post->nid,sizeof(int));
	for(i=0;i<post->nid;++i)
	{
		fix->id_prob[i]=FLOAT2FIX_ANY(post->id_prob[i],shift);
	}
	for(i=0;i<post->nnet;++i)
	{
		wtk_kwake_node_update_fix(post->network+i,shift);
	}
}

int wtk_kwake_cfg_update(wtk_kwake_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_kwake_cfg_update2(cfg,&sl);
}

int wtk_kwake_cfg_update2(wtk_kwake_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;
	int win;
	int rate;
	ret=wtk_kxparm_cfg_update2(&(cfg->parm),sl);
	if(ret!=0){goto end;}
	rate=wtk_kxparm_cfg_get_rate(&(cfg->parm));
	cfg->triger_len=cfg->triger_time*rate/1000;
	//wtk_debug("quick=%f nid=%d\n",cfg->quick_prob,cfg->post.nid);
	if(cfg->use_full_triger==1)
	{
		cfg->quick_prob=pow(cfg->quick_prob,cfg->triger.nid);
	}else
	{
		cfg->quick_prob=pow(cfg->quick_prob,cfg->post.nid);
	}
	win=wtk_kxparm_cfg_get_win(&(cfg->parm));
	cfg->quiet_hist=cfg->quiet_time*rate/(1000*win);
	cfg->normal_hist=cfg->normal_time*rate/(1000*win);
	cfg->use_fixpoint=cfg->parm.knn.use_fixpoint;
	if(cfg->use_fixpoint)
	{
		int shift;

		cfg->shift=shift=cfg->parm.knn.softmax_fixl->shift;
		cfg->fix_quick_max_phn_prob=FLOAT2FIX_ANY(cfg->quick_max_phn_prob,shift);
		cfg->fix_quick_prob=FLOAT2FIX_ANY(cfg->quick_prob,shift);
		wtk_kwake_post_cfg_update_fix(&(cfg->post),shift);
		if(cfg->use_triger)
		{
			wtk_kwake_post_cfg_update_fix(&(cfg->triger),shift);
		}
	}
	ret=0;
end:
	return ret;
}


wtk_kwake_cfg_t* wtk_kwake_cfg_new_bin3(char *bin_fn,unsigned int seek_pos)
{
	wtk_kwake_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./kwake.cfg";
	int ret;

	cfg=(wtk_kwake_cfg_t*)wtk_malloc(sizeof(wtk_kwake_cfg_t));
	//wtk_debug("new cfg=%p\n",cfg);
	wtk_kwake_cfg_init(cfg);
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
	ret=wtk_kwake_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=wtk_kwake_cfg_update2(cfg,&sl);
	if(ret!=0){goto end;}
end:
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		wtk_kwake_cfg_delete_bin2(cfg);
		cfg=NULL;
	}
	return cfg;
}

wtk_kwake_cfg_t* wtk_kwake_cfg_new_bin(char *bin_fn,char *cfg_fn)
{
	wtk_mbin_cfg_t *cfg;
	wtk_kwake_cfg_t *wc;

	//wtk_debug("%s/%s\n",bin_fn,cfg_fn);
	cfg=wtk_mbin_cfg_new_type(wtk_kwake_cfg,bin_fn,cfg_fn);
	wc=(wtk_kwake_cfg_t*)(cfg->cfg);
	wc->cfg.bin_cfg=cfg;

	return wc;
}

wtk_kwake_cfg_t* wtk_kwake_cfg_new_bin2(char *bin_fn)
{
	return wtk_kwake_cfg_new_bin(bin_fn,"./kwake.cfg");
}

wtk_kwake_cfg_t* wtk_kwake_cfg_new(char *fn)
{
	wtk_main_cfg_t *cfg;
	wtk_kwake_cfg_t *tcfg;

	cfg=wtk_main_cfg_new_type(wtk_kwake_cfg,fn);
	if(cfg)
	{
		tcfg=(wtk_kwake_cfg_t*)(cfg->cfg);
		tcfg->cfg.cfg=cfg;
		return tcfg;
	}else
	{
		return NULL;
	}
}

void wtk_kwake_cfg_delete(wtk_kwake_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->cfg.cfg);
}
void wtk_kwake_cfg_delete_bin(wtk_kwake_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->cfg.bin_cfg);
}
void wtk_kwake_cfg_delete_bin2(wtk_kwake_cfg_t *cfg)
{
	wtk_kwake_cfg_clean(cfg);
	wtk_free(cfg);
}
