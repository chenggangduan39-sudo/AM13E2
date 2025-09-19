#include "qtk_wakeup_decoder.h"
int qtk_wakeup_decoder_run_rec(qtk_wakeup_decoder_route_t* route,wtk_thread_t *t);
void qtk_wakeup_decoder_notify_feature(qtk_wakeup_decoder_t *decoder,wtk_feat_t *f);
void qtk_wakeup_decoder_flush_reced_feature(qtk_wakeup_decoder_t *decoder);
void qtk_wakeup_decoder_mt_start(qtk_wakeup_decoder_route_t* route);
void qtk_wakeup_decoder_mt_feed_end(qtk_wakeup_decoder_route_t* route,int wait_eof);
int qtk_wakeup_decoder_wait_end(qtk_wakeup_decoder_route_t* route,int timeout);
void qtk_wakeup_decoder_stop_rec(qtk_wakeup_decoder_route_t* route);
void qtk_wakeup_decoder_send_back_feature(qtk_wakeup_decoder_t *decoder,wtk_feat_t *f);
int qtk_wakeup_decoder_feed2(qtk_wakeup_decoder_t* decoder,char *data,int bytes,int is_end);


void qtk_wakeup_decoder_get_wake_time(qtk_wakeup_decoder_t *decoder,float *fs,float *fe)
{
	float dur=decoder->parm->cfg->frame_dur;
	int beg_idx;
	int end_idx;
	if(decoder->cfg->use_mt==0)
	{
		beg_idx = decoder->dec->wake_beg_idx;
		end_idx = decoder->dec->wake_end_idx;
	}else
	{
		beg_idx = decoder->asr[0]->dec->wake_beg_idx;
		end_idx = decoder->asr[0]->dec->wake_end_idx;
	}
	*fs = beg_idx *dur*decoder->parm->nnet3->frames_per_chunk;
	*fe = end_idx *dur*decoder->parm->nnet3->frames_per_chunk;
	//wtk_debug("====%f,beg=%d,end=%d\n",dur,beg_idx,end_idx);
}



qtk_wakeup_decoder_evt_t* qtk_wakeup_decoder_new_evt(qtk_wakeup_decoder_t* decoder)
{
	qtk_wakeup_decoder_evt_t *evt;

	evt=(qtk_wakeup_decoder_evt_t*)wtk_malloc(sizeof(qtk_wakeup_decoder_evt_t));
	return evt;
}

int qtk_wakeup_decoder_evt_delete(qtk_wakeup_decoder_evt_t *evt)
{
	wtk_free(evt);
	return 0;
}

qtk_wakeup_decoder_evt_t* qtk_wakeup_decoder_pop_evt(qtk_wakeup_decoder_route_t* route)
{
	qtk_wakeup_decoder_evt_t *evt;

	evt=(qtk_wakeup_decoder_evt_t*)wtk_lockhoard_pop(&(route->evt_hoard));
	evt->type=QTK_WAKEUP_DECODER_EVT_FEED;
	evt->f=NULL;
	return evt;
}

void qtk_wakeup_decoder_push_evt(qtk_wakeup_decoder_route_t* route,qtk_wakeup_decoder_evt_t *evt)
{	
	wtk_lockhoard_push(&(route->evt_hoard),evt);
}

qtk_wakeup_decoder_route_t* qtk_wakeup_decoder_route_new(qtk_kwdec_cfg_t* dec_cfg,char* name)
{
	qtk_wakeup_decoder_route_t* route;

	route=(qtk_wakeup_decoder_route_t*)wtk_malloc(sizeof(qtk_wakeup_decoder_route_t));
	route->run=1;
	wtk_thread_init(&(route->thread),(thread_route_handler)qtk_wakeup_decoder_run_rec,route);
	wtk_thread_set_name(&(route->thread),name);
	wtk_blockqueue_init(&(route->rec_input_q));
	wtk_sem_init(&(route->rec_wait_sem),0);
	wtk_sem_init(&(route->rec_start_wait_sem),0);
	wtk_thread_start(&(route->thread));
	wtk_lockhoard_init(&(route->evt_hoard),offsetof(qtk_wakeup_decoder_evt_t,hoard_n),1024,(wtk_new_handler_t)qtk_wakeup_decoder_new_evt,
						(wtk_delete_handler_t)qtk_wakeup_decoder_evt_delete,route);
	route->dec=qtk_kwdec_new(dec_cfg);
        wtk_lock_init(&route->mut);
        return route;
}

void qtk_wakeup_decoder_route_reset(qtk_wakeup_decoder_route_t* route)
{
        wtk_lock_lock(&route->mut);
        qtk_kwdec_reset(route->dec);
        wtk_lock_unlock(&route->mut);
}

void qtk_wakeup_decoder_route_delete(qtk_wakeup_decoder_route_t* route)
{
	qtk_wakeup_decoder_stop_rec(route);
	//wtk_blockqueue_clean(&(wrapper->feature_bak_q));
	wtk_sem_clean(&(route->rec_wait_sem));
	wtk_sem_clean(&(route->rec_start_wait_sem));
	//wtk_debug("clean thread\n");
	wtk_thread_clean(&(route->thread));
	wtk_blockqueue_clean(&(route->rec_input_q));
	wtk_lockhoard_clean(&(route->evt_hoard));
	qtk_kwdec_delete(route->dec);
        wtk_lock_clean(&route->mut);
        wtk_free(route);
}

qtk_wakeup_decoder_t* qtk_wakeup_decoder_new(qtk_wakeup_decoder_cfg_t* cfg)
{

	qtk_wakeup_decoder_t *decoder;

	decoder=(qtk_wakeup_decoder_t*)wtk_malloc(sizeof(*decoder));
	decoder->cfg=cfg;
	decoder->env_parser=wtk_cfg_file_new();
	decoder->parm=wtk_fextra_new(&(cfg->extra));
	//wrapper->dec=qtk_kwfstdec_new(&(cfg->kwfstdec));
	wtk_queue_init(&(decoder->parm_q));
	decoder->time_stop=0;
	decoder->wav_bytes=0;
	decoder->rec_wav_bytes=0;
	decoder->index=0;
    wtk_queue_init(&(decoder->vad_q));
    if(cfg->use_vad)
    {
        	decoder->vad=wtk_vad_new(&(cfg->vad),&(decoder->vad_q));
        	decoder->last_vframe_state=wtk_vframe_sil;
    }else
    {
        	decoder->vad=0;
    }
	if(cfg->use_mt)
	{
//		wrapper->run=1;
//		wtk_thread_init(&(wrapper->thread),(thread_route_handler)qtk_decoder_wrapper_run_rec,wrapper);//TODO run thread
//		wtk_thread_set_name(&(wrapper->thread),"kaldi_rec");
		if(cfg->extra.use_nnet3)
		{
			qtk_nnet3_set_notify(decoder->parm->nnet3,(qtk_nnet3_feature_notify_f)qtk_wakeup_decoder_feed_nnet3_feature,decoder);//TODO notify feature
			//wrapper->dec->trans_model=cfg->extra.nnet3.t_mdl->trans_model;
		}else
		{
			wtk_fextra_set_notify(decoder->parm,(wtk_fextra_feature_notify_f)qtk_wakeup_decoder_notify_feature,decoder);//TODO notify feature
		}

		decoder->asr=(qtk_wakeup_decoder_route_t**)wtk_malloc(sizeof(qtk_wakeup_decoder_route_t*)*2);
		decoder->asr[0]=qtk_wakeup_decoder_route_new(&(decoder->cfg->kwdec),"normal");
		decoder->asr[0]->dec->trans_model=cfg->extra.nnet3.wt_mdl->trans_model;
		if(decoder->cfg->asr_route==2)
		{
			decoder->asr[1]=qtk_wakeup_decoder_route_new(&(decoder->cfg->kwdec2),"little");
			decoder->asr[1]->dec->trans_model=cfg->extra.nnet3.wt_mdl->trans_model;
		}
		wtk_blockqueue_init(&(decoder->feature_bak_q));
	}else if(cfg->extra.use_nnet3)
	{
		decoder->dec=qtk_kwdec_new(&(cfg->kwdec));
		qtk_nnet3_set_notify(decoder->parm->nnet3,(qtk_nnet3_feature_notify_f)qtk_wakeup_decoder_feed_nnet3_feature,decoder);//TODO notify feature
		decoder->dec->trans_model=cfg->extra.nnet3.wt_mdl->trans_model;
		//wrapper->dec->cfg->trans_model=*wrapper->cfg->extra.nnet3.t_mdl;
		//wtk_debug("fffa:%p %p\n",wrapper->dec->cfg->trans_model,wrapper->dec->cfg->trans_model.trans_model);
		//wrapper->dec->trans_model= wrapper->dec->cfg->trans_model.trans_model;
		//wrapper->dec->trans_model=cfg->extra.nnet3.t_mdl->trans_model;
	}else
	{
		decoder->dec=qtk_kwdec_new(&(cfg->kwdec));
		wtk_fextra_set_output_queue(decoder->parm,&(decoder->parm_q));
		decoder->dec->trans_model=cfg->extra.nnet3.wt_mdl->trans_model;
	}

	return decoder;
}

void qtk_wakeup_decoder_flush_vad_left_data(qtk_wakeup_decoder_t *d)
{
    wtk_string_t v;

    wtk_vad_get_left_data(d->vad,&(v));
    if(v.len>0)
    {
		d->rec_wav_bytes+=v.len;
        qtk_wakeup_decoder_feed2(d,v.data,v.len,0);
    }
}

int qtk_wakeup_decoder_feed_vad(qtk_wakeup_decoder_t *d,char *data,int bytes,int is_end)
{
    wtk_queue_t *q;
    wtk_queue_node_t *n;
    wtk_vframe_t *f;
    int ret;
    int cnt;

    ret=wtk_vad_feed(d->vad,data,bytes,is_end);
    if(ret!=0){goto end;}
    q=d->vad->output_queue;
    while(q->length>0)
    {
        n=wtk_queue_pop(q);
        if(!n){break;}
        f=data_offset(n,wtk_vframe_t,q_n);
        d->last_vframe_state=f->state;
        //wtk_debug("v[%d]=%d\n",f->index,f->state);
        //wtk_debug("v[%d]=%d/%d\n",f->index,f->state,f->raw_state);
        if(f->state!=wtk_vframe_sil)
        {   
            cnt=f->frame_step<<1;
            d->rec_wav_bytes+=cnt;
            qtk_wakeup_decoder_feed2(d,(char*)f->wav_data,cnt,0);
        }
        //wtk_debug("use_vad_restart=%d\n",d->env.use_vad_restart);
        wtk_vad_push_vframe(d->vad,f);
    }
    if(is_end)
    {
        //wtk_debug("get last end=%d/%d\n",d->last_vframe_state,wtk_vframe_sil);
        if(d->last_vframe_state!=wtk_vframe_sil)
        {   
            qtk_wakeup_decoder_flush_vad_left_data(d);
       }
            qtk_wakeup_decoder_feed2(d,0,0,1);
    }
end:
    return ret;
}

int qtk_wakeup_decoder_start(qtk_wakeup_decoder_t* decoder)
{
	return qtk_wakeup_decoder_start2(decoder,0,0);
}

int qtk_wakeup_decoder_start2(qtk_wakeup_decoder_t* decoder,char *data,int bytes)
{	
	int ret;
	wtk_wfstenv_cfg_init2(&(decoder->env));
	if(bytes>0)
	{
		ret=wtk_cfg_file_feed(decoder->env_parser,data,bytes);
		if(ret!=0){goto end;}
	}

    if(decoder->cfg->use_vad && decoder->env.use_vad)
    {   
        wtk_vad_start(decoder->vad);
    }

	if(decoder->cfg->use_mt)
	{
		//int i;
		//for(i=0;i<wrapper->cfg->asr_route;i++)
		//{
			qtk_wakeup_decoder_mt_start(decoder->asr[0]);
		//}
		if(decoder->cfg->asr_route==2 && decoder->env.use_dec2)
		{
			qtk_wakeup_decoder_mt_start(decoder->asr[1]);
		}
	}else
	{
		qtk_kwdec_start(decoder->dec);
	}
	ret=0;
	end:
		return ret;
}

void qtk_wakeup_decoder_feed_feature(qtk_wakeup_decoder_t* decoder,wtk_feat_t *f)
{
	qtk_kwdec_feed(decoder->dec,f);
	//qtk_decoder_wrapper_send_back_feature(wrapper,f);
}

void qtk_wakeup_decoder_nnet3_feed_route(qtk_wakeup_decoder_route_t* route,float *f,int index)
{
	qtk_wakeup_decoder_evt_t *evt;

	evt=qtk_wakeup_decoder_pop_evt(route);
				//evt->f=f;
	evt->index=index;
	evt->out=f;
	wtk_blockqueue_push(&(route->rec_input_q),&(evt->q_n));
}

void qtk_wakeup_decoder_feed_nnet3_feature(qtk_wakeup_decoder_t *decoder,qtk_blas_matrix_t *f,int end)
{
	if(decoder->cfg->use_mt)
	{
		int j;
		if(f)
		{
			if(decoder->asr[0]->dec->found==1) return;
			decoder->asr[0]->dec->out_col = f->col;
			for(j=0;j<f->row;j++)
			{
				qtk_wakeup_decoder_nnet3_feed_route(decoder->asr[0],f->m+f->col*j,decoder->index);
				if(decoder->cfg->asr_route==2 && decoder->env.use_dec2)
		       		{
					qtk_wakeup_decoder_nnet3_feed_route(decoder->asr[1],f->m+f->col*j,decoder->index);
				}
				decoder->index++;
			}
		}
		if(end)
		{
			//for(i=0;i<wrapper->cfg->asr_route;i++)
			//{
				//qtk_decoder_wrapper_nnet3_feed_route(wrapper->asr[i]);
				qtk_wakeup_decoder_mt_feed_end(decoder->asr[0],1);
				if(decoder->cfg->asr_route==2 && decoder->env.use_dec2)
				{
					qtk_wakeup_decoder_mt_feed_end(decoder->asr[1],1);
				}
			//}
		}
	}else
	{
		if(f)
		{
			if(decoder->dec->found==1) return;
			decoder->dec->out_col = f->col;
			qtk_kwdec_feed2(decoder->dec,f->m,decoder->index);
			decoder->index++;
		}
		if(end)
		{
		//	qtk_kwdec_get_result(decoder->dec,decoder->res_buf);

		}
	}
}

int qtk_wakeup_decoder_feed2(qtk_wakeup_decoder_t* decoder,char *data,int bytes,int is_end)
{
	int ret;
	if(is_end)
	{
		decoder->time_stop=time_get_ms();
   	}
	if(decoder->cfg->use_mt)
	{
		qtk_wakeup_decoder_flush_reced_feature(decoder);//TODO ???
	}
	//wrapper->wav_bytes+=bytes;
	ret=wtk_fextra_feed2(decoder->parm,data,bytes,is_end);

	return ret;
}

int qtk_wakeup_decoder_feed(qtk_wakeup_decoder_t* decoder,char *data,int bytes,int is_end)
{
	int ret = 0;
	decoder->wav_bytes+=bytes;
    if(decoder->vad && decoder->env.use_vad)
    {
        ret = qtk_wakeup_decoder_feed_vad(decoder,data,bytes,is_end);
    }else
    {
        ret = qtk_wakeup_decoder_feed2(decoder,data,bytes,is_end);
    }
	return ret;
}
/*
void qtk_wakeup_decoder_feed_test_feat(qtk_wakeup_decoder_t* decoder,int is_end)
{
	int i;
	wtk_debug("%d\n",decoder->cfg->f_cnt);
//	wtk_matrix_print(wrapper->cfg->m);
	for(i=1;i<=decoder->cfg->f_cnt;i++)
	{
		//qtk_kwfstdec_feed(wrapper->dec,i,wrapper->cfg->m[i]);
	}
	//qtk_kwfstdec_feed(wrapper->dec,f->index,f->rv);

	qtk_kwdec_get_result(decoder->dec,decoder->res_buf);

}

void qtk_wakeup_decoder_feed_test_feat2(qtk_wakeup_decoder_t* decoder,wtk_matrix_t *m,int cnt)
{
	int i;
	wtk_debug("%d\n",cnt);
//	wtk_matrix_print(decoder->cfg->m);
	for(i=1;i<=cnt;i++)
	{
		qtk_kwdec_feed2(decoder->dec,m[i],i);
	}
	//qtk_kwdec_feed(decoder->dec,f->index,f->rv);

	qtk_kwdec_get_result(decoder->dec,decoder->res_buf);
	wtk_free(m);
}
*/
void qtk_wakeup_decoder_reset(qtk_wakeup_decoder_t* decoder)
{	
	wtk_cfg_file_reset(decoder->env_parser);
    wtk_queue_init(&(decoder->vad_q));
    decoder->last_vframe_state=wtk_vframe_sil;
	if(decoder->cfg->use_mt)
	{
		//if(wrapper->start)
		//{
			//qtk_decoder_wrapper_mt_feed_end(wrapper,1);
		//	qtk_decoder_wrapper_flush_reced_feature(wrapper);
		//}
		//for(i=0;i<wrapper->cfg->asr_route;i++)
		//{
			qtk_wakeup_decoder_route_reset(decoder->asr[0]);
			if(decoder->cfg->asr_route==2 && decoder->env.use_dec2)
			qtk_wakeup_decoder_route_reset(decoder->asr[1]);
		//}
	}else
	{
		qtk_kwdec_reset(decoder->dec);
	}
    if(decoder->vad && decoder->env.use_vad)
    {
        wtk_vad_reset(decoder->vad);
    }
	wtk_wfstenv_cfg_init2(&(decoder->env));
	decoder->time_stop=0;
	decoder->wav_bytes=0;
    	decoder->rec_wav_bytes=0;
	decoder->index=0;
	wtk_fextra_reset(decoder->parm);
	wtk_queue_init(&(decoder->parm_q));
}

void qtk_wakeup_decoder_delete(qtk_wakeup_decoder_t* decoder)
{
	wtk_cfg_file_delete(decoder->env_parser);
	if(decoder->cfg->use_mt)
	{
		int i;
		for(i=0;i<decoder->cfg->asr_route;i++)
		{
			qtk_wakeup_decoder_route_delete(decoder->asr[i]);
		}
		wtk_blockqueue_clean(&(decoder->feature_bak_q));
		wtk_free(decoder->asr);
	}else
	{
		qtk_kwdec_delete(decoder->dec);
	}
    if(decoder->vad)
    {
        wtk_vad_delete(decoder->vad);
    }
	wtk_fextra_delete(decoder->parm);
	wtk_free(decoder);
}

//int qtk_decoder_wrapper_send_back_reced_feature(qtk_decoder_wrapper_t *warpper,wtk_feat_t *f)
//{
//	//wtk_debug("get feature %d use=%d free=%d\n",f->index,d->parm->feature_hoard.use_length,d->parm->feature_hoard.cur_free);
//	wtk_blockqueue_push(&(warpper->feature_bak_q),&(f->queue_n));
//	return 0;
//}

void qtk_wakeup_decoder_flush_reced_feature(qtk_wakeup_decoder_t *decoder)
{
	wtk_blockqueue_t *q=&(decoder->feature_bak_q);
	wtk_queue_node_t *qn;
	wtk_feat_t *f,*fp;
	int dec;

	dec=decoder->parm->cfg->use_dnn&&decoder->parm->cfg->dnn.use_mlat;
	while(q->length>0)
	{
		qn=wtk_blockqueue_pop(q,0,NULL);
		if(!qn)
		{
			break;
		}
		f=data_offset(qn,wtk_feat_t,queue_n);
		if(dec)
		{
			--f->used;
		}
		if(f->app_hook)
		{
			fp=(wtk_feat_t*)f->app_hook;
			//wtk_debug("%p=%d:%d\n",fp,fp->index,fp->used);
			wtk_feat_push_back(fp);
		}
		//wtk_debug("%p=%d:%d\n",f,f->index,f->used);
		wtk_feat_push_back(f);
	}
}

void qtk_wakeup_decoder_send_back_feature(qtk_wakeup_decoder_t *decoder,wtk_feat_t *f)
{
	wtk_feat_t *f2;

	//wtk_debug("%p=%d:%d\n",f,f->index,f->used);
	if(decoder->cfg->use_mt)
	{
		//wtk_debug("%p=%d:%d\n",f,f->index,f->used);
		/*
		if(f->app_hook)
		{
			f2=(wtk_feat_t*)f->app_hook;
			wtk_wfstdec_send_back_reced_feature(d,f2);
		}*/
		//qtk_decoder_wrapper_send_back_reced_feature(warpper,f);
	}else
	{
		if(f->app_hook)
		{
			f2=(wtk_feat_t*)f->app_hook;
			wtk_feat_push_back(f2);
		}
		wtk_feat_push_back(f);
	}
}

void qtk_wakeup_decoder_mt_start(qtk_wakeup_decoder_route_t *route)
{
	qtk_wakeup_decoder_evt_t *evt;

	//d->state=WTK_FST_DEC_WAV_START;
	//wtk_blockqueue_wake(&(d->rec_input_q));
	evt=qtk_wakeup_decoder_pop_evt(route);
	evt->type=QTK_WAKEUP_DECODER_EVT_START;
	//wtk_debug("push evt=%d\n",evt->type);
	wtk_blockqueue_push(&(route->rec_input_q),&(evt->q_n));
	wtk_sem_acquire(&(route->rec_start_wait_sem),-1);
}

void qtk_wakeup_decoder_mt_feed_end(qtk_wakeup_decoder_route_t* route,int wait_eof)
{
	qtk_wakeup_decoder_evt_t *evt;

	//d->state=WTK_FST_DEC_WAV_END;
	//wtk_blockqueue_wake(&(d->rec_input_q));
	evt=qtk_wakeup_decoder_pop_evt(route);
	evt->type=QTK_WAKEUP_DECODER_EVT_END;
	wtk_blockqueue_push(&(route->rec_input_q),&(evt->q_n));
	if(wait_eof)
	{
		qtk_wakeup_decoder_wait_end(route,-1);
	}
}

int qtk_wakeup_decoder_wait_end(qtk_wakeup_decoder_route_t* route,int timeout)
{
	int ret;

	ret=wtk_sem_acquire(&(route->rec_wait_sem),timeout);
	if(ret==0)
	{
		//qtk_decoder_wrapper_flush_reced_feature(route);TODO
	}
	return ret;
}

void qtk_wakeup_decoder_stop_rec(qtk_wakeup_decoder_route_t* route)
{
	//d->state=WTK_FST_DEC_DIE;
	route->run=0;
	wtk_blockqueue_wake(&(route->rec_input_q));
	wtk_thread_join(&(route->thread));
}

void qtk_wakeup_decoder_notify_feature(qtk_wakeup_decoder_t *decoder,wtk_feat_t *f)
{
//	int i;

//	for(i=0;i<wrapper->cfg->asr_route;i++)
//	{
//		qtk_decoder_wrapper_nnet3_feed_route(wrapper->asr[i],f->dnn_v,0);
//	}
}

int qtk_wakeup_decoder_run_rec(qtk_wakeup_decoder_route_t* route,wtk_thread_t *t)
{
	qtk_wakeup_decoder_evt_t *evt;
	wtk_blockqueue_t *q=&(route->rec_input_q);
	wtk_queue_node_t *qn;
	int timeout=-1;
	int b;

	while(route->run)
	{
		//qn=wtk_blockqueue_pop(q,-1,NULL);
		qn=wtk_blockqueue_pop(q,timeout,&b);
	if(!qn)
	{
	    //wtk_debug("found bug len=%d qn=%p b=%d pop=%p push=%p\n",q->length,qn,b,q->pop,q->push);
	    //perror(__FUNCTION__);
	    continue;
	}
		//wtk_debug("qn=%p\n",qn);
		evt=data_offset(qn,	qtk_wakeup_decoder_evt_t,q_n);
		//wtk_debug("rec=%d pend=%d\n",d->rec->frame,q->length);

		switch(evt->type)
		{
		case QTK_WAKEUP_DECODER_EVT_START:
			//time=time_get_cpu();
			qtk_kwdec_start(route->dec);
			//wtk_fst_rec_start(d->rec,d->net);
			//wtk_debug("next\n");
			wtk_sem_release(&(route->rec_start_wait_sem),1);
			break;
		case QTK_WAKEUP_DECODER_EVT_FEED:
                        wtk_lock_lock(&route->mut);
                        qtk_kwdec_feed2(route->dec,evt->out,evt->index);
                        wtk_lock_unlock(&route->mut);
                        break;
		case QTK_WAKEUP_DECODER_EVT_END:
		//	qtk_kwdec_get_result(route->dec,route->res_buf);
			//wrapper->time_rec=time_get_cpu()-time;
			wtk_sem_release(&(route->rec_wait_sem),1);
			break;
		}
		qtk_wakeup_decoder_push_evt(route,evt);
	}
	return 0;
}