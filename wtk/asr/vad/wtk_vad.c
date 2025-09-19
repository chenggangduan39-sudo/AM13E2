#include "wtk_vad.h"
void wtk_vad_feed_vframe(wtk_vad_t *v,wtk_vframe_t *f);
void wtk_vad_feed_speech_end(wtk_vad_t *v,wtk_vframe_t *f);
void wtk_vad_feed_min_speech(wtk_vad_t *v,wtk_vframe_t *f);

int wtk_vad_bytes(wtk_vad_t *v)
{
	int bytes;

	bytes=sizeof(wtk_vad_t);
	//wtk_debug("cache=%d\n",v->cache_q.length);
	if(v->cfg->type==WTK_DNN_VAD)
	{
		bytes+=wtk_fnnvad_bytes(v->route.dnnvad);
	}
	if(v->cfg->type==WTK_K_VAD)
	{
		bytes+=wtk_kvad_bytes(v->route.kvad);
	}
	return bytes;
}

wtk_vad_t* wtk_vad_new(wtk_vad_cfg_t *cfg,wtk_queue_t *output_queue)
{
	wtk_vad_t *v;

	v=(wtk_vad_t*)wtk_malloc(sizeof(*v));
	v->cfg=cfg;
	v->state=WTK_VAD_SIL;
	switch(cfg->type)
	{
	case WTK_ANN_VAD:
		v->route.annvad=wtk_annvad_new(&(cfg->annvad),v,(wtk_vframe_raise_f)wtk_vad_feed_vframe);
		break;
	case WTK_DNN_VAD:
		v->route.dnnvad=wtk_fnnvad_new(&(cfg->dnnvad),v,(wtk_vframe_raise_f)wtk_vad_feed_vframe);
		break;
	case WTK_GMM_VAD:
		v->route.gmmvad2=wtk_gmmvad2_new(&(cfg->gmmvad2),v,(wtk_vframe_raise_f)wtk_vad_feed_vframe);
		break;
	case WTK_FE_VAD:
		v->route.fevad=wtk_fevad_new(&(cfg->fevad),v,(wtk_vframe_raise_f)wtk_vad_feed_vframe);
		break;
	case WTK_K_VAD:
#ifndef USE_KFRAME
		v->route.kvad=wtk_kvad_new2(&(cfg->kvad),&output_queue);
#else
		wtk_debug("use kframe not support vad2\n");
		exit(0);
#endif
	}
	v->left_margin=cfg->left_margin;
	v->right_margin=cfg->right_margin;
	v->output_queue=output_queue;
	wtk_queue_init(&(v->cache_q));
	v->speech_end_sil_frames=0;
	v->min_speech_count=0;
	return v;
}

void wtk_vad_delete(wtk_vad_t *v)
{
	switch(v->cfg->type)
	{
	case WTK_ANN_VAD:
		wtk_annvad_delete(v->route.annvad);
		break;
	case WTK_DNN_VAD:
		wtk_fnnvad_delete(v->route.dnnvad);
		break;
	case WTK_GMM_VAD:
		wtk_gmmvad2_delete(v->route.gmmvad2);
		break;
	case WTK_FE_VAD:
		wtk_fevad_delete(v->route.fevad);
		break;
	case WTK_K_VAD:
		wtk_kvad_delete(v->route.kvad);
	}
	wtk_free(v);
}

int wtk_vad_start(wtk_vad_t *v)
{
	int ret = 0;

	v->state=WTK_VAD_SIL;
	wtk_queue_init(&(v->cache_q));
	switch(v->cfg->type)
	{
	case WTK_K_VAD:
		wtk_kvad_start(v->route.kvad);
		break;
	default:
		break;
	}
	return ret;
}

void wtk_vad_set_margin(wtk_vad_t *v,int left,int right)
{
	v->left_margin=left;
	v->right_margin=right;
}

int wtk_vad_reset(wtk_vad_t *v)
{
	int ret=0;

	v->left_margin=v->cfg->left_margin;
	v->right_margin=v->cfg->right_margin;
	v->speech_end_sil_frames=0;
	v->min_speech_count=0;
	switch(v->cfg->type)
	{
	case WTK_GMM_VAD:
		if(v->route.gmmvad2->frame_hoard.use_length>0)
		{
			wtk_vad_feed(v,0,0,1);
		}
		wtk_gmmvad2_flush_frame_queue(v->route.gmmvad2,v->output_queue);
		wtk_gmmvad2_reset(v->route.gmmvad2);
		ret=0;
		break;
	case WTK_ANN_VAD:
		if(v->route.annvad->frame_hoard.use_length>0)
		{
			wtk_vad_feed(v,0,0,1);
		}
		wtk_annvad_flush_frame_queue(v->route.annvad,v->output_queue);
		wtk_annvad_reset(v->route.annvad);
		ret=0;
		break;
	case WTK_DNN_VAD:
		if(v->route.dnnvad->frame_hoard.use_length>0)
		{
			wtk_vad_feed(v,0,0,1);
		}
		wtk_fnnvad_flush_frame_queue(v->route.dnnvad,v->output_queue);
		wtk_fnnvad_reset(v->route.dnnvad);
		ret=0;
		break;
	case WTK_FE_VAD:
		if(v->route.fevad->frame_hoard.use_length>0)
		{
			wtk_vad_feed(v,0,0,1);
		}
		wtk_fevad_flush_frame_queue(v->route.fevad,v->output_queue);
		wtk_fevad_reset(v->route.fevad);
		break;
	case WTK_K_VAD:
		if(v->route.kvad->frame_hoard.use_length>0)
		{
			wtk_vad_feed(v,0,0,1);
		}
		wtk_kvad_flush_frame(v->route.kvad);
		wtk_kvad_reset(v->route.kvad);
		break;
	}
	v->state=WTK_VAD_SIL;
	return ret;
}

int wtk_vad_restart(wtk_vad_t *v)
{
	int ret;

	ret=wtk_vad_reset(v);
	if(ret!=0){goto end;}
	ret=wtk_vad_start(v);
end:
	return ret;
}

void wtk_vad_raise_frame(wtk_vad_t *v,wtk_vframe_t *f)
{
	//wtk_debug("v[%f,%d]=%s\n",f->index*f->frame_step*1.0/8000,f->index,f->state==wtk_vframe_sil?"sil":(f->state==wtk_vframe_speech_end?"speech_end":"speech"));
#ifdef DEBUG_X
	wtk_debug("v[%d]=%s %d/%d f=%p/%p\n",f->index,f->state==wtk_vframe_sil?"sil":"speech",v->cfg->left_margin,
			v->cfg->right_margin,f,&(f->q_n));
#endif
	wtk_queue_push(v->output_queue,&(f->q_n));
}

void wtk_vad_flush_queue(wtk_vad_t *v,int left_slot)
{
	wtk_queue_t *q=&(v->cache_q);
	wtk_queue_node_t *n;
	wtk_vframe_t *f;

	while(q->length>left_slot)
	{
		n=wtk_queue_pop(q);
		if(!n){break;}
		f=data_offset(n,wtk_vframe_t,q_n);
		//wtk_debug("v[%d]=%d\n",f->index,f->state);
		wtk_vad_raise_frame(v,f);
	}
}

void wtk_vad_set_cache_state(wtk_vad_t *v,wtk_vframe_state_t state)
{
	wtk_queue_node_t *n;
	wtk_queue_t *q=&(v->cache_q);
	wtk_vframe_t *f;

	for(n=q->pop;n;n=n->next)
	{
		f=data_offset(n,wtk_vframe_t,q_n);
		f->state=state;
	}
}

void wtk_vad_feed_sil(wtk_vad_t *v,wtk_vframe_t *f)
{
	wtk_queue_t *q=&(v->cache_q);

	if(f->state==wtk_vframe_sil)
	{
		//if vframe is silence, process left margin;
		wtk_queue_push(q,&(f->q_n));
		wtk_vad_flush_queue(v,v->left_margin);
	}else
	{
		if(v->cfg->min_speech>0)
		{
			v->state=WTK_VAD_MIN_SPEECH;
			v->min_speech_count=0;
			wtk_vad_feed_min_speech(v,f);
		}else
		{
			v->state=WTK_VAD_SPEECH;
			wtk_vad_feed_vframe(v,f);
		}
	}
}

void wtk_vad_feed_min_speech(wtk_vad_t *v,wtk_vframe_t *f)
{
	wtk_queue_t *q=&(v->cache_q);

	wtk_queue_push(q,&(f->q_n));
	if(f->state==wtk_vframe_sil)
	{
		//if it is silence, goto sil state and leave left margin;
		wtk_vad_set_cache_state(v,wtk_vframe_sil);
		wtk_vad_flush_queue(v,v->left_margin);
		v->state=WTK_VAD_SIL;
	}else
	{
		++v->min_speech_count;
		//wtk_debug("min_speech=%d/%d\n",v->min_speech_count,v->cfg->min_speech);
		//if(q->length>=(v->cfg->left_margin+v->cfg->min_speech))
		if(v->min_speech_count>=v->cfg->min_speech)
		{
			wtk_vad_set_cache_state(v,wtk_vframe_speech);
			wtk_vad_flush_queue(v,0);
			v->state=WTK_VAD_SPEECH;
		}
	}
}

void wtk_vad_feed_speech(wtk_vad_t *v,wtk_vframe_t *f)
{
	if(f->state==wtk_vframe_sil)
	{
		v->state=WTK_VAD_SPEECH_END;
		v->speech_end_sil_frames=0;
		wtk_vad_feed_speech_end(v,f);
	}else
	{
		//if there is some silence, this will not be happen,but logical will be;
		if(v->cache_q.length>0)
		{
			wtk_vad_set_cache_state(v,wtk_vframe_speech);
			wtk_vad_flush_queue(v,0);
		}
		wtk_vad_raise_frame(v,f);
	}
}

void wtk_vad_flush_speech_end(wtk_vad_t *v)
{
	wtk_queue_t *q=&(v->cache_q);
	//wtk_vframe_t *f;

	if(q->length<=0){return;}
	//wtk_debug("len=%d,margin=%d.\n",q->length,v->cfg->right_margin);
	//if right margin satisfied, flush speech and goto sil;
	wtk_vad_set_cache_state(v,wtk_vframe_speech);
	//the last frame is speech end;
	//f=data_offset(q->push,wtk_vframe_t,q_n);
	//f->state=wtk_vframe_speech_end;
	//f->state=wtk_vframe_speech;
	wtk_vad_flush_queue(v,0);
	//wtk_debug("len=%d,margin=%d.\n",q->length,v->cfg->right_margin);
}

void wtk_vad_feed_speech_end(wtk_vad_t *v,wtk_vframe_t *f)
{
	wtk_queue_t *q=&(v->cache_q);

	if(f->state==wtk_vframe_sil)
	{
		if(v->cfg->use_dnn && v->route.dnnvad->post->force_sil)
		{
			if(q->length>0)
			{
				wtk_vad_flush_speech_end(v);
			}
			v->state=WTK_VAD_SIL;
			wtk_vad_raise_frame(v,f);
		}else
		{
			if(v->right_margin<=0)
			{
				wtk_vad_raise_frame(v,f);
				v->state=WTK_VAD_SIL;
			}else
			{
				++v->speech_end_sil_frames;
				//wtk_debug("####################### speech end=%d\n",v->speech_end_sil_frames);
				if(q->length>0)
				{
					wtk_vad_flush_speech_end(v);
				}
				//wtk_queue_push(q,&(f->q_n));
				//wtk_debug("v=%p end=%d right=%d\n",v,v->speech_end_sil_frames,v->right_margin);
				if(v->speech_end_sil_frames>=v->right_margin)
				{
					f->state=wtk_vframe_speech_end;
					//f->state=wtk_vframe_speech;
					wtk_vad_raise_frame(v,f);
					v->state=WTK_VAD_SIL;
				}else
				{
					f->state=wtk_vframe_speech;
					wtk_vad_raise_frame(v,f);
				}
			}
		}
	}else
	{
		if(q->length>0)
		{
			wtk_vad_flush_speech_end(v);
		}
		v->state=WTK_VAD_SPEECH;
		wtk_vad_feed_min_speech(v,f);
	}
}

void wtk_vad_feed_vframe(wtk_vad_t *v,wtk_vframe_t *f)
{
	f->raw_state=f->state;
	//wtk_debug("v[%d]=%s state=%d left=%d right=%d state=%d\n",f->index,f->state==wtk_vframe_sil?"sil":"speech",v->state,v->left_margin,v->right_margin,v->state);
	switch(v->state)
	{
	case WTK_VAD_SIL:
		wtk_vad_feed_sil(v,f);
		break;
	case WTK_VAD_MIN_SPEECH:
		wtk_vad_feed_min_speech(v,f);
		break;
	case WTK_VAD_SPEECH:
		wtk_vad_feed_speech(v,f);
		break;
	case WTK_VAD_SPEECH_END:
		wtk_vad_feed_speech_end(v,f);
		break;
	}
}

void wtk_vad_flush_cache(wtk_vad_t *v)
{
	switch(v->state)
	{
	case WTK_VAD_SIL:
		wtk_vad_flush_queue(v,0);
		break;
	case WTK_VAD_MIN_SPEECH:
		wtk_vad_set_cache_state(v,wtk_vframe_sil);
		wtk_vad_flush_queue(v,0);
		break;
	case WTK_VAD_SPEECH:
		wtk_vad_set_cache_state(v,wtk_vframe_speech);
		wtk_vad_flush_queue(v,0);
		break;
	case WTK_VAD_SPEECH_END:
		wtk_vad_flush_speech_end(v);
		break;
	}
	return;
}

int wtk_vad_feed_float(wtk_vad_t *v,float *data,int len,int is_end)
{
	int ret=0;

	switch(v->cfg->type)
	{
	case WTK_GMM_VAD:
	case WTK_ANN_VAD:
		wtk_debug("not found\n");
		ret=0;
		break;
	case WTK_DNN_VAD:
		ret=wtk_fnnvad_feed_float(v->route.dnnvad,data,len,is_end);
		break;
	case  WTK_FE_VAD:
		wtk_fevad_feed_float(v->route.fevad,data,len,is_end);
		ret=0;
		break;
	default:
		break;
	}
	if(ret!=0){goto end;}
	if(is_end)
	{
		wtk_vad_flush_cache(v);
	}
end:
	return ret;
}

int wtk_vad_feed(wtk_vad_t *v,char *data,int bytes,int is_end)
{
	int ret=0;

	switch(v->cfg->type)
	{
	case WTK_GMM_VAD:
		wtk_gmmvad2_feed(v->route.gmmvad2,data,bytes,is_end);
		ret=0;
		break;
	case WTK_ANN_VAD:
		ret=wtk_annvad_feed(v->route.annvad,data,bytes,is_end);
		break;
	case WTK_DNN_VAD:
		ret=wtk_fnnvad_feed(v->route.dnnvad,data,bytes,is_end);
		break;
	case WTK_FE_VAD:
		wtk_fevad_feed(v->route.fevad,data,bytes,is_end);
		ret=0;
		break;
	case WTK_K_VAD:
		wtk_kvad_feed(v->route.kvad,(short*)data,bytes/2,is_end);
		break;
	}
	if(ret!=0){goto end;}
	if(is_end)
	{
		wtk_vad_flush_cache(v);
	}
end:
	return ret;
}

void wtk_vad_push_vframe(wtk_vad_t *v,wtk_vframe_t *f)
{
	switch(v->cfg->type)
	{
	case WTK_GMM_VAD:
		wtk_gmmvad2_push_vframe(v->route.gmmvad2,f);
		break;
	case WTK_ANN_VAD:
		wtk_annvad_push_vframe(v->route.annvad,f);
		break;
	case WTK_DNN_VAD:
		wtk_fnnvad_push_vframe(v->route.dnnvad,f);
		break;
	case WTK_FE_VAD:
		wtk_fevad_push_vframe(v->route.fevad,f);
		break;
	case WTK_K_VAD:
#ifdef USE_KFRAME
		break;
#else
		wtk_kvad_push_frame(v->route.kvad,f);
#endif
		break;
	default:
		break;
	}

}

void wtk_vad_set_speech_thresh(wtk_vad_t *v,float f)
{
	switch(v->cfg->type)
	{
	case WTK_DNN_VAD:
		wtk_fnnvad_set_speech_thresh(v->route.dnnvad,f);
		break;
	default:
		break;
	}
}

void wtk_vad_set_speech_thresh2(wtk_vad_t *v,float hf,float lf)
{
	switch(v->cfg->type)
	{
	case WTK_DNN_VAD:
		wtk_fnnvad_set_speech_thresh2(v->route.dnnvad,hf,lf);
		break;
	default:
		break;
	}
}

void wtk_vad_get_left_data(wtk_vad_t *v,wtk_string_t *data)
{
	wtk_short_buffer_t *buf;

	switch(v->cfg->type)
	{
	case WTK_GMM_VAD:
		buf=v->route.gmmvad2->frame_buffer;
		break;
	case WTK_ANN_VAD:
		buf=v->route.annvad->frame_buffer;
		break;
	case WTK_DNN_VAD:
		buf=v->route.dnnvad->frame_buffer;
		break;
	default:
		wtk_string_set(data,0,0);
		goto end;
		break;
	}
	data->data=(char*)buf->start;
	data->len=(buf->cur-buf->start)*2;
end:
	return;
}

void wtk_vad_flush_queue_audio(wtk_vad_t *v,wtk_queue_t *q,void *ths,wtk_vad_flush_data_f flush)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *f;

	for(qn=q->pop;qn;qn=qn->next)
	{
		f=data_offset2(qn,wtk_vframe_t,q_n);
		//wtk_debug("feed frame=%d\n",f->index);
		flush(ths,(char*)f->wav_data,f->frame_step*2);
	}
}

void wtk_vad_flush_cached_audio(wtk_vad_t *v,void *ths,wtk_vad_flush_data_f flush)
{
	wtk_robin_t *rb;
	wtk_vframe_t *f;
	wtk_fnnvad_feat_t *df;
	int i;
	wtk_string_t p;

	//wtk_debug("output=%d cache=%d\n",v->output_queue->length,v->cache_q.length);
	if(v->output_queue)
	{
		wtk_vad_flush_queue_audio(v,v->output_queue,ths,flush);
	}
	if(v->cache_q.length>0)
	{
		wtk_vad_flush_queue_audio(v,&(v->cache_q),ths,flush);
	}
	switch(v->cfg->type)
	{
	case WTK_DNN_VAD:
		rb=v->route.dnnvad->feat_robin;
		for(i=v->route.dnnvad->cfg->win;i<rb->used;++i)
		{
			df=wtk_robin_at(rb,i);
			f=df->frame;
			//wtk_debug("feed frame=%d/%d\n",i,f->index);
			flush(ths,(char*)f->wav_data,f->frame_step*2);
		}
		wtk_vad_flush_queue_audio(v,&(v->route.dnnvad->frame_q),ths,flush);
		break;
	default:
		break;
	}
	wtk_vad_get_left_data(v,&p);
	if(p.len>0)
	{
		flush(ths,p.data,p.len);
	}
}

void wtk_vad_peek_unuse_data(wtk_vad_t *v,wtk_strbuf_t *buf)
{
	wtk_queue_t *q=v->output_queue;
	wtk_queue_node_t *qn;
	wtk_vframe_t *frame;
	wtk_string_t data;

	while(q->length>0)
	{
		qn=wtk_queue_pop(q);
		if(!qn){break;}
		frame=data_offset2(qn,wtk_vframe_t,q_n);
		//wtk_debug("[%s]\n",frame->state==wtk_vframe_sil?"sil":"speech");
		wtk_strbuf_push(buf,(char*)(frame->wav_data),frame->frame_step<<1);
		wtk_vad_push_vframe(v,frame);
	}
	wtk_vad_get_left_data(v,&(data));
	if(data.len>0)
	{
		wtk_strbuf_push(buf,data.data,data.len);
	}
}

void wtk_vad_flush(wtk_vad_t *v)
{
	switch(v->cfg->type)
	{
	case WTK_GMM_VAD:
	case WTK_ANN_VAD:
		break;
	case WTK_DNN_VAD:
		wtk_fnnvad_flush(v->route.dnnvad);
		break;
	default:
		break;
	}
}

void wtk_vad_test_file(wtk_vad_t *v,char *wav_fn)
{
	//wtk_queue_node_t *n;
	//wtk_vframe_t *f;
	char *data;
	int x;

	printf("fn=%s\n",wav_fn);
	data=file_read_buf(wav_fn,&x);
	printf("data=%p x=%d\n",data,x);
	wtk_vad_start(v);
	printf("data=%p x=%d\n",data,x);
	if(1)
	{
		char *s,*e;
		int step=512;
		int n;
		double t,f;
		int  len=x-44;

		t=time_get_ms();
		s=data+44;e=s+len;
		//step=len;
		while(s<e)
		{
			n=min(e-s,step);
			//wtk_debug("n=%d\n",n);
			wtk_vad_feed(v,s,n,0);
			s+=n;
		}
		wtk_vad_feed(v,0,0,1);
		//wtk_vad2_feed(vad,WTK_PARM_END,data,len);
		t=time_get_ms()-t;
		//wtk_debug("time=%f wav=%d\n",t,vad->output_queue->length*20);
		f=t/(v->output_queue->length*20);
		printf("time=%f rate=%f\n",t,f);
	}else
	{
		wtk_vad_feed(v,data+44,x-44,1);
	}
	printf("data=%p x=%d\n",data,x);
	wtk_vad_queue_print_mlf(v->output_queue,v->cfg->dnnvad.parm.frame_dur,stdout);
	printf("data=%p x=%d\n",data,x);
	/*
	while(1)
	{
		n=wtk_queue_pop(v->output_queue);
		if(!n){break;}
		f=data_offset(n,wtk_vframe_t,q_n);
		wtk_vad_push_vframe(v,f);
	}*/
	wtk_vad_reset(v);
	wtk_free(data);
}


int wtk_vad_get_frame_step(wtk_vad_t *vad)
{
	if (vad->cfg->use_ann)
	{
		return vad->route.annvad->cfg->parm.frame_step;
	}
	else if(vad->cfg->use_dnn)
	{
		return vad->route.dnnvad->cfg->parm.frame_step;
	}else if(vad->cfg->use_gmm2)
	{
		return vad->route.gmmvad2->vad->cfg->frame_size;
	}else if(vad->cfg->use_fe)
	{
		return vad->route.fevad->stft->cfg->win;
	}else if(vad->cfg->use_k)
	{
		return vad->route.kvad->cfg->parm.parm.frame_step;
	}else
	{
		return 0;
	}
}

float wtk_vad_get_frame_dur(wtk_vad_t *vad)
{
	if (vad->cfg->use_ann)
	{
		return vad->route.annvad->cfg->parm.frame_dur;
	}
	else if(vad->cfg->use_dnn)
	{
		return vad->route.dnnvad->cfg->parm.frame_dur;
	}else if(vad->cfg->use_gmm2)
	{
		return 0;
	}else if(vad->cfg->use_fe)
	{
		return 0;
	}else if(vad->cfg->use_k)
	{
		return vad->route.kvad->cfg->parm.parm.frame_step_ms;
	}else
	{
		return 0;
	}
}

void wtk_vad_print_mlf(wtk_vad_t *vad)
{
	float dur;

	if(vad->cfg->use_dnn)
	{
		dur=vad->cfg->dnnvad.parm.frame_dur;
	}else if(vad->cfg->use_gmm2)
	{
		dur=vad->cfg->gmmvad2.frame_size*1.0/16000;
	}else if(vad->cfg->use_fe)
	{
		dur=vad->cfg->fevad.stft.win*1.0/16000;
	}
	else
	{
		dur=0;
	}
	wtk_vad_queue_print_mlf(vad->output_queue,dur,stdout);
}

void wtk_vad_queue_print_p(wtk_queue_t *q,float frame_dur,FILE *s)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *vf;
	int sil=1;
	float last=0;

	//fprintf(s,"0");
	for(qn=q->pop;qn;qn=qn->next)
	{
		vf=data_offset2(qn,wtk_vframe_t,q_n);
		last=vf->index*frame_dur;
		if(sil)
		{
			if(vf->state!=wtk_vframe_sil)
			{
				//wtk_debug("goto speech at %f\n",vf->index*frame_dur);
				fprintf(s,"%f",vf->index*frame_dur);
				sil=0;
			}
		}else
		{
			if(vf->state==wtk_vframe_sil)
			{
				fprintf(s,"-%f speech\n",vf->index*frame_dur);
				//wtk_debug("goto sil at %f\n",vf->index*frame_dur);
				sil=1;
			}
		}
	}
	if(sil==0)
	{
		fprintf(s,"-%f",last);
	}
	fprintf(s," %f",last);
	//fprintf(s,"\n");
	//exit(0);
}

/**
 * @param frame_dur seconds
 */
void wtk_vad_queue_print_mlf(wtk_queue_t *q,float frame_dur,FILE *s)
{
	wtk_queue_node_t *n;
	wtk_vframe_t *f;
	//int dur=wtk_float_round(vad->parm->cfg->frame_dur*1E7);
	//int dur=wtk_float_round(frame_dur*1E2);
	float dur=frame_dur*1E7;
#define PAD_0 "00000"
	int state=wtk_vframe_sil;
	int i=0;
	int last_index=0;

	if(q->length>0)
	{
		fprintf(s,"0 ");
	}
	for(n=q->pop;n;n=n->next)
	{
		f=data_offset(n,wtk_vframe_t,q_n);
		//wtk_debug("v[%d]=%s state=%d/%d\n",f->index,f->state==wtk_vframe_sil?"sil":(f->state==wtk_vframe_speech_end?"speech_end":"speech"),state,f->state);
		switch(state)
		{
		case wtk_vframe_sil:
			if(f->state!=wtk_vframe_sil)
			{
				if(f->index>1)
				{
					fprintf(s,"%.0f sil\n%.0f ",(f->index-1)*dur,(f->index-1)*dur);
				}
				last_index=f->index;
				++i;
				state=wtk_vframe_speech;
			}
			break;
		case wtk_vframe_speech:
			if(f->state==wtk_vframe_speech_end)
			{
				last_index=f->index+1;
				++i;
				fprintf(s,"%.0f speech\n%.0f ",(f->index)*dur,(f->index)*dur);
			}else if(f->state==wtk_vframe_sil)
			{
				if(f->index>last_index)
				{
					++i;
					fprintf(s,"%.0f speech\n%.0f ",(f->index-1)*dur,(f->index-1)*dur);
				}
				last_index=f->index;
				state=wtk_vframe_sil;
			}
			break;
		case wtk_vframe_speech_end:
			break;
		}
		if(i>=3)
		{
			//exit(0);
		}
		if(!n->next)
		{
			fprintf(s,"%.0f %s\n",f->index*dur,f->state==wtk_vframe_sil?"sil":"speech");
		}
	}
	fprintf(s,".\n");
}

int wtk_vad_get_output_left_sil_margin(wtk_vad_t *vad)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *vf;
	int i;

	i=0;
	for(qn=vad->output_queue->pop;qn;qn=qn->next)
	{
		vf=data_offset2(qn,wtk_vframe_t,q_n);
		//wtk_debug("v[%d]=%s state=%d/%d\n",vf->index,vf->state==wtk_vframe_sil?"sil":"speech",vf->state,vf->raw_state);
		if(vf->raw_state==wtk_vframe_sil)
		{
			++i;
		}else
		{
			break;
		}
	}
	//wtk_debug("i=%d\n",i);
	return i;
}

void wtk_vad_print_pend(wtk_vad_t *vad)
{
	wtk_debug("cache=%d\n",vad->cache_q.length);
	wtk_fnnvad_print_pend(vad->route.dnnvad);
}

void wtk_vad_test_file2(char *cfg_fn,char *fn)
{
	wtk_vad_cfg_t *cfg;
	wtk_vad_t *vad;
	wtk_queue_t queue;
	wtk_main_cfg_t *main_cfg;
	int i;

	printf("cfg_fn=%s fn=%s\n",cfg_fn,fn);
	main_cfg=wtk_main_cfg_new(sizeof(wtk_vad_cfg_t),
			(wtk_main_cfg_init_f)wtk_vad_cfg_init,
			(wtk_main_cfg_clean_f)wtk_vad_cfg_clean,
			(wtk_main_cfg_update_local_f)wtk_vad_cfg_update_local,
			(wtk_main_cfg_update_f)wtk_vad_cfg_update,cfg_fn);
	cfg=(wtk_vad_cfg_t*)main_cfg->cfg;
	printf("cfg=%p\n",cfg);
	wtk_queue_init(&queue);
	vad=wtk_vad_new(cfg,&queue);
	printf("vad=%p\n",vad);
	for(i=0;i<10;++i)
	{
		wtk_vad_test_file(vad,fn);
	}
	wtk_vad_delete(vad);
	wtk_main_cfg_delete(main_cfg);
}

void wtk_vad_conf_set(wtk_vad_t *v,float power,float speech,float sil, int left, int right)
{	
	switch(v->cfg->type)
	{
	case WTK_GMM_VAD:
	//	wtk_gmmvad2_set_conf(v->route.gmmvad2);
		break;
	case WTK_DNN_VAD:
//		wtk_fnnvad_set_conf(v->route.dnnvad);
		break;
	case WTK_FE_VAD:
	//	wtk_fevad_set_conf(v->route.fevad,f);
		break;
	case WTK_K_VAD:
		//if(v->cfg->prob_proc)
		{
			wtk_kvad_set_prob(v->route.kvad,power,sil,speech);
		}
		//if(v->cfg->margin_proc)
		{
			wtk_kvad_set_margin(v->route.kvad,left,right);
		}
	default:
		break;
	}	
}

void wtk_vad_conf_print(wtk_vad_t *v)
{	
	switch(v->cfg->type)
	{
	case WTK_GMM_VAD:
	//	wtk_gmmvad2_set_conf(v->route.gmmvad2);
		break;
	case WTK_DNN_VAD:
//		wtk_fnnvad_print_conf(v->route.dnnvad);
		break;
	case WTK_FE_VAD:
	//	wtk_fevad_set_conf(v->route.fevad,f);
		break;
	case WTK_K_VAD:
		//if(v->cfg->prob_proc)
		{
			wtk_kvad_print_prob(v->route.kvad);
		}
		//if(v->cfg->margin_proc)
		{
			wtk_kvad_print_margin(v->route.kvad);
		}
	default:
		break;
	}	
}
