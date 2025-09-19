#include "wtk_fnnvad.h"
//#define DEUBG_LOG
#ifdef DEUBG_LOG
#include "wtk/os/wtk_log.h"
#endif

wtk_vframe_t* wtk_fnnvad_new_vframe(wtk_fnnvad_t *v)
{
	wtk_fextra_cfg_t *cfg=&(v->cfg->parm);

	return wtk_vframe_new(cfg->frame_size,cfg->frame_step);
}

wtk_vframe_t* wtk_fnnvad_pop_vframe(wtk_fnnvad_t *v)
{
	wtk_vframe_t *f;

	f=wtk_hoard_pop(&(v->frame_hoard));
	wtk_frame_reset(f);
	f->index=++v->n_frame_index;
	return f;
}

void wtk_fnnvad_push_vframe(wtk_fnnvad_t *v,wtk_vframe_t *f)
{
	wtk_hoard_push(&(v->frame_hoard),f);
}


int wtk_fnnvad_bytes(wtk_fnnvad_t *v)
{
	int bytes;

	bytes=sizeof(wtk_fnnvad_t);
	//wtk_debug("len=%d\n",v->param_output_q.length);
	bytes+=wtk_fextra_bytes(v->parm);
	bytes+=v->cfg->parm.frame_size*v->cfg->cache*sizeof(short);
	bytes+=(v->frame_hoard.use_length+v->frame_hoard.cur_free)*(sizeof(wtk_vframe_t)+v->cfg->parm.frame_size*sizeof(float)+
			v->cfg->parm.frame_step*sizeof(short));
	bytes+=wtk_bit_heap_bytes(v->feat_heap);
	if(v->feat_robin)
	{
		bytes+=wtk_robin_bytes(v->feat_robin);
	}
	bytes+=wtk_fnnvad_post_bytes(v->post);
	return bytes;
}



wtk_fnnvad_t* wtk_fnnvad_new(wtk_fnnvad_cfg_t *cfg,void *raise_ths,wtk_vframe_raise_f raise)
{
	wtk_fnnvad_t *v;
	int cache;

	v=(wtk_fnnvad_t*)wtk_malloc(sizeof(*v));
	v->cfg=cfg;
	v->raise=raise;
	v->raise_ths=raise_ths;
	wtk_queue_init(&(v->param_output_q));
	v->parm=wtk_fextra_new(&(cfg->parm));
	wtk_fextra_set_output_queue(v->parm,&(v->param_output_q));

	cache=cfg->cache;
	v->frame_buffer=wtk_short_buffer_new(cfg->parm.frame_size*cache);
	wtk_hoard_init(&v->frame_hoard,offsetof(wtk_vframe_t,hoard_n),
			cache,(wtk_new_handler_t)wtk_fnnvad_new_vframe,
			(wtk_delete_handler_t)wtk_vframe_delete,v);
	v->feat_heap=wtk_bit_heap_new2(sizeof(wtk_fnnvad_feat_t));
	if(cfg->win>0)
	{
		v->feat_robin=wtk_robin_new(cfg->win*2+1);
	}else
	{
		v->feat_robin=NULL;
	}
	v->post=wtk_fnnvad_post_new(v);
	v->post->speech_engine_thresh=cfg->speech_energe_thresh;
	v->echo_speech_thresh=cfg->echo_speech_thresh;
	wtk_fnnvad_reset(v);
	return v;
}

void wtk_fnnvad_delete(wtk_fnnvad_t *v)
{
	wtk_fnnvad_post_delete(v->post);
	if(v->feat_robin)
	{
		wtk_robin_delete(v->feat_robin);
	}
	wtk_bit_heap_delete(v->feat_heap);
	wtk_short_buffer_delete(v->frame_buffer);
	wtk_hoard_clean(&(v->frame_hoard));
	wtk_fextra_delete(v->parm);
	wtk_free(v);
}

void wtk_fnnvad_reset(wtk_fnnvad_t *v)
{
	//wtk_debug("===================>  reset\n");
	v->last_f0=0;
	v->last_fe=0;
	v->speech_energe_thresh=0;
	v->speech_energe_thresh_lf=0;
	v->speech_energe_thresh_set_frame=-1;
	wtk_fnnvad_post_reset(v->post);
	if(v->feat_robin)
	{
		wtk_robin_reset(v->feat_robin);
	}
	wtk_bit_heap_reset(v->feat_heap);
	wtk_short_buffer_reset(v->frame_buffer);
	wtk_fextra_reset(v->parm);
	wtk_queue_init(&(v->param_output_q));
	wtk_queue_init(&(v->frame_q));
	v->n_frame_index=0;
}

wtk_fnnvad_feat_t* wtk_fnnvad_new_feat(wtk_fnnvad_t *v,wtk_vframe_t *frame)
{
	wtk_fnnvad_feat_t *f;

	f=(wtk_fnnvad_feat_t*)wtk_bit_heap_malloc(v->feat_heap);
	f->frame=frame;
	f->is_sil=0;
	f->ref=0;
	return f;
}

void wtk_fnnvad_push_feat(wtk_fnnvad_t *v,wtk_fnnvad_feat_t *f)
{
	if(f->ref==0)
	{
		wtk_bit_heap_free(v->feat_heap,f);
	}
}

void wtk_fnnvad_raise_feat(wtk_fnnvad_t *v,wtk_fnnvad_feat_t *f,int is_sil)
{
	//wtk_debug("v[%d]=%s\n",f->frame->index,is_sil?"sil":"speech");
	if(is_sil)
	{
		f->frame->state=wtk_vframe_sil;
	}else
	{
		f->frame->state=wtk_vframe_speech;
	}
	v->raise(v->raise_ths,f->frame);
	wtk_fnnvad_push_feat(v,f);
}

void wtk_fnnvad_flush_feature(wtk_fnnvad_t *v,int is_end)
{
	wtk_robin_t *r=v->feat_robin;
	wtk_fnnvad_feat_t *feat;
	int i,pad;
	float value;

	if(r->used<=v->cfg->win){return;}
	value=0;
	for(i=0;i<r->used;++i)
	{
		feat=(wtk_fnnvad_feat_t*)wtk_robin_at(r,i);
		//wtk_debug("v[%d/%d]=%d\n",i,feat->frame->index,feat->is_sil);
		value+=feat->is_sil;
	}
	value/=r->nslot;
	if(r->used==r->nslot)
	{
		feat=(wtk_fnnvad_feat_t*)wtk_robin_at(r,v->cfg->win);
	}else
	{
		pad=r->nslot-r->used;
		if(is_end)
		{
			feat=(wtk_fnnvad_feat_t*)wtk_robin_at(r,v->cfg->win);
			//wtk_debug("v[%d]=%p v=%f n=%d/%d\n",feat->frame->index,feat,value,r->nslot,r->used);
		}else
		{
			feat=(wtk_fnnvad_feat_t*)wtk_robin_at(r,v->cfg->win-pad);
		}
	}
	if(value>0.5)
	{
		feat->frame->state=wtk_vframe_sil;
	}else
	{
		feat->frame->state=wtk_vframe_speech;
	}
	//feat->frame->speechlike=value;
	//wtk_debug("v[%d]=%d v=%f\n",feat->frame->index,r->used,value);
	//wtk_fnnvad_raise_frame(v,feat->frame);
	wtk_fnnvad_post_feed(v->post,feat);
	if(r->nslot==r->used || is_end)
	{
		feat=(wtk_fnnvad_feat_t*)wtk_robin_pop(r);
		--feat->ref;
		wtk_fnnvad_push_feat(v,feat);
	}
	return;
}

void wtk_fnnvad_feed_feature(wtk_fnnvad_t *v,wtk_fnnvad_feat_t *feat)
{
	wtk_robin_t *r=v->feat_robin;

	//wtk_debug("v[%d]=%d\n",feat->frame->index,feat->is_sil);
	if(r)
	{
		++feat->ref;
		wtk_robin_push(r,feat);
		//wtk_debug("used=%d/%d\n",r->used,v->cfg->win);
		if(r->used<=v->cfg->win){return;}
		wtk_fnnvad_flush_feature(v,0);
	}else
	{
		if(feat->is_sil)
		{
			feat->frame->state=wtk_vframe_sil;
		}else
		{
			feat->frame->state=wtk_vframe_speech;
		}
		wtk_fnnvad_post_feed(v->post,feat);
	}
}

void wtk_fnnvad_feed_parm_feature(wtk_fnnvad_t *v,wtk_feat_t *f)
{
	wtk_vector_t *vec;
	wtk_queue_node_t *n;
	wtk_vframe_t *frame;
	wtk_fnnvad_feat_t *feat;
	int is_sil;
	//float thresh=v->echo_speech_thresh;

	if(v->speech_energe_thresh_set_frame>0 && f->index>=v->speech_energe_thresh_set_frame)
	{
		v->speech_energe_thresh_set_frame=-1;
		v->post->speech_engine_thresh=v->speech_energe_thresh;
		v->post->speech_engine_thresh_low=v->speech_energe_thresh_lf;
#ifdef DEUBG_LOG
		{
			wtk_log_log(glb_log,"v[%d]=%f\n",v->speech_energe_thresh_set_frame,v->speech_energe_thresh);
			//wtk_log_log(glb_log,"f0[%d]=%f/%f is_sil=%d %f/%f\n",f->frame->index,f->f0,f->fe,is_sil,v->vad->cfg->min_f0,v->vad->cfg->max_f0);
		}
#endif
	}
	vec=f->rv;
	if(v->cfg->parm.dnn.use_expand_vector)
	{
		is_sil=vec[1]>=v->cfg->sil_thresh;
	}else
	{
		is_sil=vec[1]>=vec[2];
	}
	//wtk_debug("v[%d]=%f/%f is_sil=%d\n",f->index,vec[1],vec[2],is_sil);
	if(v->frame_q.length==0)
	{
		frame=wtk_fnnvad_pop_vframe(v);
		memset(frame->wav_data,0,frame->frame_size<<1);
	}else
	{
		n=wtk_queue_pop(&(v->frame_q));
		frame=data_offset(n,wtk_vframe_t,q_n);
	}
	frame->speechlike=vec[2]-vec[1];
	feat=wtk_fnnvad_new_feat(v,frame);
	if(v->cfg->use_speech_end_detect)
	{
		feat->frame->energy=f->energy;
	}
	//wtk_vector_print(f->v);
	//print_float(f->v+1,13);
	//第一个特征的值组判断;
	//wtk_debug("v[%d]=%f/%f sil=%d\n",frame->index,f->rv[1],f->rv[2],is_sil);
	feat->is_sil=is_sil;
	//wtk_debug("v[%d]=%s vec=%f/%f expand=%d sil=%f\n",feat->frame->index,feat->is_sil?"sil":"speech",vec[1],vec[2],v->cfg->parm.dnn.use_expand_vector,v->cfg->sil_thresh);
	//wtk_debug("v[%d] sil=%d sil=%f speech=%f %f thresh=%f f0=%f/%f\n",f->index,is_sil,vec[1],vec[2],vec[1]-vec[2],v->cfg->sil_thresh,feat->f0,feat->fe);
	wtk_fnnvad_feed_feature(v,feat);
}

void wtk_fnnvad_save_audio(wtk_fnnvad_t *v,char *data,int bytes)
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
			f=wtk_fnnvad_pop_vframe(v);
			wtk_short_buffer_peek(b,f);
			wtk_queue_push(&(v->frame_q),&(f->q_n));
			wtk_short_buffer_skip(b,cfg->frame_step,cfg->frame_size);
		}
	}
}

void wtk_fnnvad_feed_end(wtk_fnnvad_t *v)
{
	wtk_robin_t *r=v->feat_robin;

	if(r)
	{
		while(r->used>v->cfg->win)
		{
			wtk_fnnvad_flush_feature(v,1);
		}
	}
	wtk_fnnvad_flush_end(v->post);
}

void wtk_fnnvad_process_feature(wtk_fnnvad_t *v)
{
	wtk_queue_t *q;
	wtk_queue_node_t *n;
	wtk_feat_t *f;

	q=&(v->param_output_q);
	while(1)
	{
		n=wtk_queue_pop(q);
		if(!n){break;}
		f=data_offset(n,wtk_feat_t,queue_n);
		//wtk_debug("indx=%d\n",f->index);
		if(f->app_hook)
		{
			wtk_fnnvad_feed_parm_feature(v,(wtk_feat_t*)(f->app_hook));
			wtk_feat_push_back((wtk_feat_t*)(f->app_hook));
			wtk_feat_push_back(f);
		}else
		{
			wtk_fnnvad_feed_parm_feature(v,f);
			wtk_feat_push_back(f);
		}
	}

}

int wtk_fnnvad_feed_float(wtk_fnnvad_t *v,float *data,int len,int is_end)
{
	int ret;

	ret=wtk_fextra_feed_float(v->parm,data,len,is_end);
	if(ret!=0){goto end;}
	wtk_fnnvad_process_feature(v);
	if(is_end)
	{
		wtk_fnnvad_feed_end(v);
	}
	ret=0;
end:
	return ret;
}

int wtk_fnnvad_feed(wtk_fnnvad_t *v,char *data,int bytes,int is_end)
{
	int ret;

	if(bytes>0)
	{
		wtk_fnnvad_save_audio(v,data,bytes);
	}
	ret=wtk_fextra_feed2(v->parm,data,bytes,is_end);
	if(ret!=0){goto end;}
	wtk_fnnvad_process_feature(v);
	if(is_end)
	{
		wtk_fnnvad_feed_end(v);
	}
	ret=0;
end:
	return ret;
}


void wtk_fnnvad_flush_frame_queue(wtk_fnnvad_t *v,wtk_queue_t *q)
{
	wtk_queue_node_t *n;
	wtk_vframe_t *f;

	while(1)
	{
		n=wtk_queue_pop(q);
		if(!n){break;}
		f=data_offset(n,wtk_vframe_t,q_n);
		wtk_fnnvad_push_vframe(v,f);
	}
}

void wtk_fnnvad_flush(wtk_fnnvad_t *v)
{
	wtk_fextra_flush_layer(v->parm,0);
	if(v->param_output_q.length>0)
	{
		wtk_fnnvad_process_feature(v);
	}
}

void wtk_fnnvad_set_speech_thresh2(wtk_fnnvad_t *v,float hf,float lf)
{
	v->speech_energe_thresh_set_frame=v->parm->n_frame_index+1;
	v->speech_energe_thresh=hf;
	v->speech_energe_thresh_lf=lf;
}

void wtk_fnnvad_set_speech_thresh(wtk_fnnvad_t *v,float f)
{
//	if(f>10)
//	{
//		v->post->speech_engine_thresh=f;
//	}else
	{
		v->speech_energe_thresh_set_frame=v->parm->n_frame_index+1;
		v->speech_energe_thresh=f*v->cfg->parm.frame_size;
	}
	//wtk_debug("%f/%d\n",v->speech_energe_thresh,v->speech_energe_thresh_set_frame);
	//v->post->speech_engine_thresh=f;
	//v->parm->n_frame_index;
#ifdef DEUBG_LOG
		{
			wtk_log_log(glb_log,"v[%d]=%f\n",v->speech_energe_thresh_set_frame,v->speech_energe_thresh);
			//wtk_log_log(glb_log,"f0[%d]=%f/%f is_sil=%d %f/%f\n",f->frame->index,f->f0,f->fe,is_sil,v->vad->cfg->min_f0,v->vad->cfg->max_f0);
		}
#endif
}

void wtk_fnnvad_print_pend(wtk_fnnvad_t *vad)
{
	//wtk_debug("feat=%d\n",vad->parm->);
	wtk_debug("fnn=%d/%d\n",vad->parm->dnn->robin->used,vad->parm->dnn->robin->nslot);
	wtk_debug("feat=%d/%d\n",vad->feat_robin->used,vad->feat_robin->nslot);
	wtk_debug("sil=%d \n",vad->post->sil_robin->used);
}

//----------------- test section ----------------------
void wtk_fnnvad_raise_dummy(wtk_fnnvad_t *v,wtk_vframe_t *f)
{
	wtk_debug("v[%d]=%s\n",f->index,f->state==wtk_vframe_sil?"sil":"speech");
	wtk_fnnvad_push_vframe(v,f);
}
