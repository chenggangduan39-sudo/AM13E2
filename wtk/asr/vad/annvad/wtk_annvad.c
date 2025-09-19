#include "wtk_annvad.h"
wtk_vframe_t* wtk_annvad_new_vframe(wtk_annvad_t *v);
wtk_annfeat_t* wtk_annvad_new_annfeat(wtk_annvad_t *v);

void wtk_annvad_print_robin(wtk_robin_t *r)
{
	wtk_annfeat_t *f;
	int i;

	printf("============== robin ================\n");
	for(i=0;i<r->used;++i)
	{
		f=(wtk_annfeat_t*)wtk_robin_at(r,i);
		printf("v[%d]=%d\n",i,f->index);
	}
}


wtk_annfeat_t* wtk_annfeat_new(int vsize)
{
	wtk_annfeat_t *f;

	f=(wtk_annfeat_t*)wtk_malloc(sizeof(*f));//+vsize*sizeof(float));
	f->v=(float*)wtk_malloc(vsize*sizeof(float));
	//f->v=(float*)(f+sizeof(*f));
	return f;
}

int wtk_annfeat_delete(wtk_annfeat_t *f)
{
	wtk_free(f->v);
	wtk_free(f);
	return 0;
}


wtk_annvad_t* wtk_annvad_new(wtk_annvad_cfg_t *cfg,void *raise_hook,wtk_vframe_raise_f raise)
{
	wtk_annvad_t *v;
	int cache=cfg->cache;
	int win;

	win=cfg->left_win+cfg->right_win+1;
	v=(wtk_annvad_t*)wtk_malloc(sizeof(*v));
	v->cfg=cfg;
	v->raise_hook=raise_hook;
	v->raise=raise;
	v->debug_ths=0;
	v->debug_f=0;
	v->parm=wtk_fextra_new(&(cfg->parm));
	wtk_queue_init(&(v->param_output_q));
	wtk_fextra_set_output_queue(v->parm,&(v->param_output_q));
	v->feature_robin=wtk_robin_new(win);
	v->sil_robin=wtk_robin_new(cfg->speechtrap);
	v->speech_robin=wtk_robin_new(cfg->siltrap);
	v->v_array=(wtk_annfeat_t**)wtk_calloc(win,sizeof(wtk_annfeat_t*));
	v->merge_feature=wtk_matrix_new(1,cfg->wbcfg.hide_rows);
	v->wbop=wtk_ann_wbop_new(&(cfg->res->wb));
	wtk_hoard_init(&v->frame_hoard,offsetof(wtk_vframe_t,hoard_n),cache,(wtk_new_handler_t)wtk_annvad_new_vframe,(wtk_delete_handler_t)wtk_vframe_delete,v);
	wtk_hoard_init(&(v->annfeat_hoard),offsetof(wtk_annfeat_t,hoard_n),cache,
			(wtk_new_handler_t)wtk_annvad_new_annfeat,(wtk_delete_handler_t)wtk_annfeat_delete,v);
	v->frame_buffer=wtk_short_buffer_new(cfg->parm.frame_size*cache);
	wtk_queue_init(&(v->frame_q));
	wtk_annvad_reset(v);
	return v;
}

void wtk_annvad_delete(wtk_annvad_t *v)
{
	wtk_short_buffer_delete(v->frame_buffer);
	wtk_hoard_clean(&(v->frame_hoard));
	wtk_hoard_clean(&(v->annfeat_hoard));
	wtk_ann_wbop_delete(v->wbop);
	wtk_matrix_delete(v->merge_feature);
	wtk_free(v->v_array);
	wtk_robin_delete(v->feature_robin);
	wtk_robin_delete(v->sil_robin);
	wtk_robin_delete(v->speech_robin);
	wtk_fextra_delete(v->parm);
	wtk_free(v);
}

wtk_vframe_t* wtk_annvad_new_vframe(wtk_annvad_t *v)
{
	wtk_fextra_cfg_t *cfg=&(v->cfg->parm);

	return wtk_vframe_new(cfg->frame_size,cfg->frame_step);
}

wtk_vframe_t* wtk_annvad_pop_vframe(wtk_annvad_t *v)
{
	wtk_vframe_t *f;

	f=wtk_hoard_pop(&(v->frame_hoard));
	wtk_frame_reset(f);
	f->index=++v->n_frame_index;
	return f;
}

void wtk_annvad_push_vframe(wtk_annvad_t *v,wtk_vframe_t *f)
{
	wtk_hoard_push(&(v->frame_hoard),f);
}

wtk_annfeat_t* wtk_annvad_new_annfeat(wtk_annvad_t *v)
{
	return wtk_annfeat_new(v->parm->cfg->feature_cols);
}

wtk_annfeat_t* wtk_annvad_pop_annfeat(wtk_annvad_t *v)
{
	wtk_annfeat_t *f;

	f=wtk_hoard_pop(&(v->annfeat_hoard));
	f->ref=0;
	return f;
}

void wtk_annvad_push_annfeat(wtk_annvad_t *v,wtk_annfeat_t *f)
{
	if(f->ref==0)
	{
		wtk_hoard_push(&(v->annfeat_hoard),f);
	}
}

void wtk_annvad_flush_frame_queue(wtk_annvad_t *v,wtk_queue_t *q)
{
	wtk_queue_node_t *n;
	wtk_vframe_t *f;

	while(1)
	{
		n=wtk_queue_pop(q);
		if(!n){break;}
		f=data_offset(n,wtk_vframe_t,q_n);
		wtk_annvad_push_vframe(v,f);
	}
}

void wtk_annvad_reset(wtk_annvad_t *v)
{
	wtk_annvad_flush_frame_queue(v,&(v->frame_q));
	wtk_robin_reset(v->feature_robin);
	wtk_robin_reset(v->sil_robin);
	wtk_robin_reset(v->speech_robin);
	wtk_short_buffer_reset(v->frame_buffer);
	wtk_fextra_reset(v->parm);
	v->n_frame_index=0;
	v->state=WTK_ANNVAD_SIL;
}

wtk_annfeat_t* wtk_annvad_normalize_feature(wtk_annvad_t *v,wtk_feat_t *f)
{
	wtk_vector_t *rv;
	wtk_vector_t *mean;
	wtk_vector_t *var;
	int i,n;
	wtk_annfeat_t *annfeat;

	annfeat=wtk_annvad_pop_annfeat(v);
	annfeat->index=f->index;
	rv=f->rv;
	n=wtk_vector_size(rv);
	mean=v->cfg->res->parm_mean;
	var=v->cfg->res->parm_var;
	for(i=1;i<=n;++i)
	{
		annfeat->v[i-1]=(rv[i]-mean[i])*var[i];
	}
	return annfeat;
}

int wtk_annvad_raise_vframe(wtk_annvad_t *v,int index,int is_sil)
{
	wtk_queue_node_t *n;
	wtk_vframe_t *f;
	int ret=-1;

	n=v->frame_q.pop;
	if(!n){goto end;}
	f=data_offset(n,wtk_vframe_t,q_n);
	if(f->index!=index)
	{
		wtk_debug("error: index(%d,%d) not equal.\n",f->index,index);
		goto end;
	}
	wtk_queue_pop(&(v->frame_q));
	f->state=is_sil?wtk_vframe_sil:wtk_vframe_speech;
	//wtk_debug("v[%d]=%s\n",f->index,is_sil?"sil":"speech");
	v->raise(v->raise_hook,f);
	ret=0;
end:
	return ret;
}

int wtk_annvad_flush_feature_robin(wtk_annvad_t *v,wtk_robin_t *r,int is_sil)
{
	wtk_annfeat_t *f;
	int ret=0;

	while(r->used>0)
	{
		f=wtk_robin_pop(r);
		if(!f){break;}
		--f->ref;
		ret=wtk_annvad_raise_vframe(v,f->index,is_sil);
		wtk_annvad_push_annfeat(v,f);
		if(ret!=0)
		{
			goto end;
		}
	}
end:
	return ret;
}

int wtk_annvad_raise_feature(wtk_annvad_t *v,wtk_annfeat_t *f,int is_sil)
{
	int ret=0;

	switch(v->state)
	{
	case WTK_ANNVAD_SIL:
		if(is_sil)
		{
			if(v->sil_robin->used>0)
			{
				ret=wtk_annvad_flush_feature_robin(v,v->sil_robin,1);
				if(ret!=0){goto end;}
			}
			ret=wtk_annvad_raise_vframe(v,f->index,1);
		}else
		{
			++f->ref;
			wtk_robin_push(v->sil_robin,f);
			if(v->sil_robin->used>=v->cfg->speechtrap)
			{
				ret=wtk_annvad_flush_feature_robin(v,v->sil_robin,0);
				v->state=WTK_ANNVAD_SPEECH;
			}
		}
		break;
	case WTK_ANNVAD_SPEECH:
		if(is_sil)
		{
			++f->ref;
			wtk_robin_push(v->speech_robin,f);
			if(v->speech_robin->used>=v->cfg->siltrap)
			{
				ret=wtk_annvad_flush_feature_robin(v,v->speech_robin,1);
				v->state=WTK_ANNVAD_SIL;
			}
		}else
		{
			if(v->speech_robin->used>0)
			{
				ret=wtk_annvad_flush_feature_robin(v,v->speech_robin,0);
				if(ret!=0){goto end;}
			}
			ret=wtk_annvad_raise_vframe(v,f->index,0);
		}
		break;
	}
	//exit(0);
end:
	return ret;
}

int wtk_annvad_raise_feature_end(wtk_annvad_t *v)
{
	int ret=0;

	switch(v->state)
	{
	case WTK_ANNVAD_SIL:
		if(v->sil_robin->used>0)
		{
			ret=wtk_annvad_flush_feature_robin(v,v->sil_robin,1);
		}
		break;
	case WTK_ANNVAD_SPEECH:
		if(v->speech_robin->used>0)
		{
			ret=wtk_annvad_flush_feature_robin(v,v->speech_robin,0);
		}
		break;
	}
	return ret;
}

int wtk_annvad_process_ann(wtk_annvad_t *v,wtk_annfeat_t **va)
{
	wtk_robin_t *r=v->feature_robin;
	int fi=v->parm->cfg->static_feature_cols,fl=fi*2;
	float *fv;
	int i,j,k;
	float *vi;
	wtk_annfeat_t *f;
	int is_sil;
	int ret;
	int nf;

	fv=v->merge_feature[1];
	k=0;
	nf=r->nslot;
	//wtk_debug("fi=%d/%d/%d\n",nf,fi,v->cfg->wbcfg.hide_rows);
	for(i=0;i<nf;++i)
	{
		vi=va[i]->v;
		for(j=0;j<fi;++j)
		{
			fv[++k]=vi[j];
		}
	}
	for(i=0;i<nf;++i)
	{
		vi=va[i]->v;
		//wtk_debug("v[%d]=%d,ref=%d\n",i,va[i]->index,va[i]->ref);
		for(j=fi;j<fl;++j)
		{
			fv[++k]=vi[j];
			//printf("\t\t[%d]=%f\n",j,vi[j]);
		}
	}
	//wtk_matrix_print(v->merge_feature);
	wtk_ann_wbop_process(v->wbop,v->merge_feature);
	is_sil=v->wbop->out_matrix[1][1]>=v->wbop->out_matrix[1][2];
	f=va[v->cfg->left_win];
	//wtk_debug("index=%d,f=%p\n",f->index,f);
	//wtk_debug("v[%d]=%f,%f\n",f->index,v->wbop->out_matrix[1][1],v->wbop->out_matrix[1][2]);
	//exit(0);
	if(v->debug_f)
	{
		v->debug_f(v->debug_ths,f->index,v->wbop->out_matrix[1][1],v->wbop->out_matrix[1][2]);
	}
	ret=wtk_annvad_raise_feature(v,f,is_sil);
	return ret;
}

void wtk_annvad_pop_feature_robin(wtk_annvad_t *v)
{
	wtk_annfeat_t *f;

	f=(wtk_annfeat_t*)wtk_robin_pop(v->feature_robin);
	--f->ref;
	wtk_annvad_push_annfeat(v,f);
}

int wtk_annvad_flush_feature(wtk_annvad_t *v,int state)
{
	wtk_robin_t *r=v->feature_robin;
	wtk_annfeat_t **va=v->v_array;
	wtk_annfeat_t *f;
	int is_end;
	int pad,i,j;
	int ret;

	if(r->used<=v->cfg->left_win){return 0;}
	is_end=state==1;
	i=0;
	pad=r->nslot-r->used;
	if(pad>0 && !is_end)
	{
		//expanded with the first feature;
		f=(wtk_annfeat_t*)wtk_robin_at(r,0);
		for(;i<pad;)
		{
			va[i++]=f;
			//wtk_debug("v[%d]=0,%d\n",i,f->index);
		}
	}
	for(j=0;j<r->used;++j)
	{
		va[i++]=((wtk_annfeat_t*)wtk_robin_at(r,j));
		//wtk_debug("v[%d]=%d,%d\n",i,j,va[i-1]->index);
	}
	if(i<r->nslot)
	{
		//expand with the last feature;
		f=(wtk_annfeat_t*)wtk_robin_at(r,r->used-1);
		for(;i<r->nslot;)
		{
			va[i++]=f;
			//wtk_debug("v[%d]=%d,%d\n",i,r->used-1,f->index);
		}
	}
	ret=wtk_annvad_process_ann(v,va);
	if(r->nslot==r->used || is_end)
	{
		f=(wtk_annfeat_t*)wtk_robin_pop(r);
		--f->ref;
		wtk_annvad_push_annfeat(v,f);
	}
	return ret;
}

int wtk_annvad_feed_feature(wtk_annvad_t *v,wtk_feat_t *f)
{
	wtk_robin_t *robin=v->feature_robin;
	wtk_annfeat_t *annfeat;
	int ret=0;

	//wtk_feature_print(f);
	annfeat=wtk_annvad_normalize_feature(v,f);
	wtk_feat_push_back(f);
	++annfeat->ref;
	//wtk_debug("================= %d ================\n",f->index);
	//print_float(annfeat->v,v->parm->feature_cols);
	wtk_robin_push(v->feature_robin,annfeat);
	if(robin->used<=v->cfg->left_win){goto end;}
	ret=wtk_annvad_flush_feature(v,0);
end:
	return ret;
}

void wtk_annvad_flush_short_feature_idx(wtk_annvad_t *v,int idx)
{
	wtk_robin_t *r=v->feature_robin;
	wtk_annfeat_t **va=v->v_array;
	wtk_annfeat_t *f;
	int i,j;
	int pad_hdr;

	i=0;
	if(r->used<=v->cfg->left_win)
	{
		pad_hdr=v->cfg->left_win-idx;
	}else
	{
		pad_hdr=v->cfg->left_win-idx+1;
	}
	if(pad_hdr>0)
	{
		//expanded with the first feature;
		f=(wtk_annfeat_t*)wtk_robin_at(r,0);
		for(;i<pad_hdr;)
		{
			va[i++]=f;
			//wtk_debug("v[%d]=0,%d\n",i,f->index);
		}
		j=0;
	}else
	{
		j=-pad_hdr;
	}
	//wtk_debug("pad_hdr=%d,idx=%d,used=%d\n",pad_hdr,idx,r->used);
	for(;j<r->used;++j)
	{
		va[i++]=((wtk_annfeat_t*)wtk_robin_at(r,j));
		//wtk_debug("v[%d]=%d,%d\n",i,j,va[i-1]->index);
	}
	if(i<r->nslot)
	{
		//expand with the last feature;
		f=(wtk_annfeat_t*)wtk_robin_at(r,r->used-1);
		for(;i<r->nslot;)
		{
			va[i++]=f;
			//wtk_debug("v[%d]=%d,%d\n",i,r->used-1,f->index);
		}
	}
	//wtk_debug("end i=%d\n",i);
	//wtk_debug("%d,index=%d\n",idx,va[r->nslot/2]->index);
	wtk_annvad_process_ann(v,va);
}

void wtk_annvad_flush_short_feature(wtk_annvad_t *v)
{
	wtk_robin_t *r=v->feature_robin;
	int i,n;

	n=r->used;
	for(i=0;i<n;++i)
	{
		wtk_annvad_flush_short_feature_idx(v,i);
	}
	//wtk_robin_reset(r);
}

void wtk_annvad_feed_feature_end(wtk_annvad_t *v)
{
	wtk_robin_t *r=v->feature_robin;
	wtk_annfeat_t *f;
	int i;

	//wtk_annvad_print_robin(r);
	if(r->used<=0){return;}
	if(r->used<=v->cfg->left_win)
	{
		i=0;
	}else
	{
		i=r->used-v->cfg->left_win+1;
	}
	//wtk_debug("i=%d,used=%d,nslot=%d\n",i,r->used,r->nslot);
	for(;i<=r->used;++i)
	{
		wtk_annvad_flush_short_feature_idx(v,i);
	}
	//wtk_debug("used=%d,left_win=%d\n",r->used,v->cfg->left_win);
	while(r->used>0)
	{
		f=(wtk_annfeat_t*)wtk_robin_pop(r);
		if(!f){break;}
		--f->ref;
		//wtk_debug("ref=%d\n",f->ref);
		wtk_annvad_push_annfeat(v,f);
	}
}

void wtk_annvad_save_audio(wtk_annvad_t *v,char *data,int bytes)
{
	wtk_fextra_cfg_t *cfg=&(v->cfg->parm);
	wtk_short_buffer_t *b=v->frame_buffer;
	wtk_vframe_t *f;
	char *end=data+bytes;

	while(data<end)
	{
		data+=wtk_short_buffer_push_c(b,data,end-data);
		while(wtk_short_buffer_used_samples(b)>=cfg->frame_size)
		{
			f=wtk_annvad_pop_vframe(v);
			wtk_short_buffer_peek(b,f);
			wtk_queue_push(&(v->frame_q),&(f->q_n));
			wtk_short_buffer_skip(b,cfg->frame_step,cfg->frame_size);
		}
	}
}

int wtk_annvad_feed_end(wtk_annvad_t *v)
{
	int ret;

	wtk_annvad_feed_feature_end(v);
	ret=wtk_annvad_raise_feature_end(v);
	return ret;
}

int wtk_annvad_feed(wtk_annvad_t *v,char *data,int bytes, int state)
{
	wtk_queue_t *q;
	wtk_queue_node_t *n;
	wtk_feat_t *f;
	int ret=0;

	wtk_annvad_save_audio(v,data,bytes);
	q=v->parm->output_queue;
	wtk_fextra_feed2(v->parm,data,bytes,state);
	//wtk_debug("len=%d,%d\n",q->length,v->frame_q.length);
	while(1)
	{
		n=wtk_queue_pop(q);
		if(!n){break;}
		f=data_offset(n,wtk_feat_t,queue_n);
		//wtk_debug("index=%d,used=%d,nslot=%d\n",f->index,v->feature_robin->used,v->feature_robin->nslot);
		ret=wtk_annvad_feed_feature(v,f);
		if(ret!=0){goto end;}
	}
	//wtk_debug("used=%d,nslot=%d\n",v->feature_robin->used,v->feature_robin->nslot);
	if(state==1)
	{
		ret=wtk_annvad_feed_end(v);
	}
end:
	return ret;
}

int wtk_annvad_feed2(wtk_annvad_t *v,wtk_feat_t *f)
{
	wtk_vframe_t *vf;
	int ret;

	vf=wtk_annvad_pop_vframe(v);
	wtk_queue_push(&(v->frame_q),&(vf->q_n));
	ret=wtk_annvad_feed_feature(v,f);
	return ret;
}


void wtk_annvad_print(wtk_annvad_t *v)
{
	printf("============== annvad =================\n");
	printf("parm output q: %d\n",v->param_output_q.length);
	printf("frame: use=%d,free=%d\n",v->frame_hoard.use_length,v->frame_hoard.cur_free);
	printf("annfeat: used=%d,free=%d\n",v->annfeat_hoard.use_length,v->annfeat_hoard.cur_free);
	printf("parm: use=%d,free=%d\n",v->parm->feature_hoard.use_length,v->parm->feature_hoard.cur_free);
}
