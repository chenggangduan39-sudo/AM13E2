#include <ctype.h>
#include "wtk_wfstdec.h"
#include "wtk/asr/wfst/rec/wtk_wfstr_prune.h"

void wtk_wfstdec_notify_feature(wtk_wfstdec_t *d,wtk_feat_t *f);
int wtk_wfstdec_run_rec(wtk_wfstdec_t *d,wtk_thread_t *t);
void wtk_wfstdec_mt_feed_end(wtk_wfstdec_t *d,int wait_eof);
void wtk_wfstdec_stop_rec(wtk_wfstdec_t *d);
void wtk_wfstdec_send_back_feature(wtk_wfstdec_t *d,wtk_feat_t *f);
void wtk_wfstdec_flush_reced_feature(wtk_wfstdec_t *d);
void wtk_wfstdec_feed_feature(wtk_wfstdec_t *d,wtk_feat_t *f);
void wtk_wfstdec_feed_end(wtk_wfstdec_t *d);
void wtk_wfstdec_mt_start(wtk_wfstdec_t *d);
void wtk_fst_dnnc_feed_dnn_feat(wtk_wfstdec_t* d,int cmd,int row,int col,float *data);

wtk_wfstevt_t* wtk_wfstdec_new_evt(wtk_wfstdec_t *d)
{
	wtk_wfstevt_t *evt;

	evt=(wtk_wfstevt_t*)wtk_malloc(sizeof(wtk_wfstevt_t));
	return evt;
}

int wtk_fst_evt_delete(wtk_wfstevt_t *evt)
{
	wtk_free(evt);
	return 0;
}

wtk_wfstevt_t* wtk_wfstdec_pop_evt(wtk_wfstdec_t *d)
{
	wtk_wfstevt_t *evt;

	evt=(wtk_wfstevt_t*)wtk_lockhoard_pop(&(d->evt_hoard));
	evt->type=WTK_WFSTEVT_FEAT;
	evt->f=NULL;
	return evt;
}

void wtk_wfstdec_push_evt(wtk_wfstdec_t *d,wtk_wfstevt_t *evt)
{
	wtk_lockhoard_push(&(d->evt_hoard),evt);
}

wtk_wfstdec_t* wtk_wfstdec_new(wtk_wfstdec_cfg_t *cfg)
{
	wtk_wfstdec_t *d;

	d=(wtk_wfstdec_t*)wtk_malloc(sizeof(*d));
	d->cfg=cfg;
	d->parm=wtk_fextra_new(&(cfg->extra));
	d->usrec=NULL;
	d->notify=NULL;
	d->notify_ths=NULL;
	d->hint_buf=wtk_strbuf_new(256,1);
	//wtk_debug("active=%d use_usrec=%d\n",cfg->usrec_is_active,cfg->use_usrec);
	if(cfg->usrec_is_active)
	{
		d->usrec=wtk_usrec_new(&(cfg->usrec));
		d->res_net=wtk_fst_net_new(&(cfg->net));
		d->custom_net=NULL;
	}else if(cfg->use_ebnf)
	{
		d->res_net=wtk_fst_net_new(&(cfg->net));
		d->custom_net=cfg->usr_net;
	}else
	{
		if(cfg->usr_net)
		{
			d->res_net=cfg->usr_net;
		}else
		{
			d->res_net=wtk_fst_net_new(&(cfg->net));
		}
		d->custom_net=NULL;
	}
	if(cfg->use_ebnfdec2)
	{
		d->ebnfdec2=wtk_ebnfdec2_new(&(cfg->ebnfdec2));
	}else
	{
		d->ebnfdec2=NULL;
	}
	if(cfg->use_xbnf)
	{
		d->xbnf=wtk_xbnf_rec_new(&(cfg->xbnf));
	}else
	{
		d->xbnf=NULL;
	}
	d->rec=wtk_wfstr_new(&(cfg->rec),&(cfg->net),cfg->hmmset.hmmset);
	if(cfg->extra.use_dnn && cfg->extra.dnn.use_lazy_out && cfg->extra.dnn.use_blas==0)
	{
		wtk_wfstr_set_dnn_handler(d->rec,d->parm->dnn->v.flat,(wtk_wfstr_dnn_get_value_f)wtk_flat_get_dnn_value);
	}
	if(cfg->use_vad)
	{
		d->vad=wtk_vad_new(&(cfg->vad),&(d->vad_q));
	}else
	{
		d->vad=0;
	}
	wtk_wfstenv_cfg_init(&(d->env),d->cfg);
	if(cfg->use_rescore)
	{
		d->rescore=wtk_rescore_new(&(cfg->rescore),&(d->env));
		d->rec->lat_rescore=d->rescore;
	}else
	{
		d->rescore=0;
	}
	d->output=wtk_wfstdec_output_new(&(cfg->output),d);
	d->input_net=wtk_fst_net2_new(&(cfg->net));
	d->env_parser=wtk_cfg_file_new();
	wtk_queue_init(&(d->parm_q));
	wtk_queue_init(&(d->vad_q));
	wtk_lockhoard_init(&(d->evt_hoard),offsetof(wtk_wfstevt_t,hoard_n),1024,(wtk_new_handler_t)wtk_wfstdec_new_evt,
			(wtk_delete_handler_t)wtk_fst_evt_delete,d);
	//wtk_debug("use_mt=%d\n",cfg->use_mt);
#ifdef USE_DNNC
	d->dnnc=NULL;
	if(cfg->use_dnnc)
	{
		d->dnnc=wtk_dnnc_new(&(cfg->dnnc));
		wtk_dnnc_set_raise(d->dnnc,d,(wtk_dnnc_raise_f)wtk_fst_dnnc_feed_dnn_feat);
	}else
	{
		d->dnnc=NULL;
	}
#endif
	if(cfg->use_dnnc)
	{
		wtk_fextra_set_output_queue(d->parm,&(d->parm_q));
		wtk_sem_init(&(d->rec_wait_sem),0);
	}else if(cfg->use_mt)
	{
		d->run=1;
		wtk_thread_init(&(d->thread),(thread_route_handler)wtk_wfstdec_run_rec,d);
		wtk_thread_set_name(&(d->thread),"rec_decoder");
		wtk_fextra_set_notify(d->parm,(wtk_fextra_feature_notify_f)wtk_wfstdec_notify_feature,d);
		wtk_blockqueue_init(&(d->rec_input_q));
		wtk_blockqueue_init(&(d->feature_bak_q));
		wtk_sem_init(&(d->rec_wait_sem),0);
		wtk_sem_init(&(d->rec_start_wait_sem),0);
		wtk_thread_start(&(d->thread));
	}else
	{
		wtk_fextra_set_output_queue(d->parm,&(d->parm_q));
	}
	//wtk_debug("rec=%p net=%p\n",d->rec,d->rec->net);
	//d->state=WTK_FST_DEC_INIT;
	d->start=0;
	wtk_wfstdec_reset(d);
	return d;
}

wtk_fst_net_t* wtk_wfstdec_get_net(wtk_wfstdec_t *d)
{
	return d->custom_net?d->custom_net:d->res_net;
}

void wtk_wfstdec_reset_net(wtk_wfstdec_t *d)
{
	wtk_fst_net_t *net;

	if(d->cfg->use_ebnf)
	{
		//wtk_debug("usr_rbin=%d/%d\n",d->res_net->use_rbin,d->custom_net->use_rbin);
		//wtk_debug("usr_bin=%d/%d\n",d->res_net->cfg->use_bin,d->custom_net->cfg->use_bin);
		if(d->res_net)
		{
			wtk_fst_net_reset(d->res_net);
		}
		if(d->custom_net)
		{
			wtk_fst_net_reset(d->custom_net);
		}
	}else
	{
		net=wtk_wfstdec_get_net(d);
		//wtk_debug("use_rbin=%d use_bin=%d\n",net->use_rbin,net->cfg->use_bin);
		//wtk_debug("reset net=%p %p/%p\n",net,d->custom_net,d->res_net);
		wtk_fst_net_reset(net);
	}
}

void wtk_wfstdec_delete(wtk_wfstdec_t *d)
{
	wtk_strbuf_delete(d->hint_buf);
#ifdef USE_DNNC
	if(d->dnnc)
	{
		wtk_dnnc_delete(d->dnnc);
	}
#endif
	if(d->ebnfdec2)
	{
		wtk_ebnfdec2_delete(d->ebnfdec2);
	}
	if(d->xbnf)
	{
		wtk_xbnf_rec_delete(d->xbnf);
	}
	wtk_wfstdec_output_delete(d->output);
	wtk_fst_net2_delete(d->input_net);
	if(d->cfg->use_dnnc)
	{
		wtk_sem_clean(&(d->rec_wait_sem));
	}else if(d->cfg->use_mt)
	{
		//wtk_debug("len=%d/%d\n",d->feature_bak_q.length,d->rec_input_q.length);
		wtk_wfstdec_stop_rec(d);
		wtk_blockqueue_clean(&(d->feature_bak_q));
		wtk_sem_clean(&(d->rec_wait_sem));
		wtk_sem_clean(&(d->rec_start_wait_sem));
		//wtk_debug("clean thread\n");
		wtk_thread_clean(&(d->thread));
		wtk_blockqueue_clean(&(d->rec_input_q));
	}
	wtk_lockhoard_clean(&(d->evt_hoard));
	if(d->vad)
	{
		wtk_vad_delete(d->vad);
	}
	if(d->rescore)
	{
		wtk_rescore_delete(d->rescore);
	}
	wtk_cfg_file_delete(d->env_parser);
	if(d->usrec)
	{
		wtk_usrec_delete(d->usrec);
	}
	if(d->cfg->use_ebnf)
	{
		wtk_fst_net_delete(d->res_net);
	}else
	{
		if(!d->cfg->usr_net)
		{
			wtk_fst_net_delete(d->res_net);
		}
	}
	wtk_wfstr_delete(d->rec);
	wtk_fextra_delete(d->parm);
	wtk_free(d);
}

void wtk_wfstdec_reset_rec(wtk_wfstdec_t *d)
{
	if(d->rescore)
	{
		wtk_rescore_reset(d->rescore);
	}
	wtk_wfstdec_reset_net(d);
	wtk_wfstr_reset(d->rec);
	if(d->usrec && d->env.use_usrec)
	{
		wtk_usrec_reset(d->usrec);
	}
}

void wtk_wfstdec_reset(wtk_wfstdec_t *d)
{
	d->rec_notify_end=0;
	d->last_vframe_state=wtk_vframe_sil;
	//wtk_debug("d->state=%d\n",d->state);
	d->is_end=0;
	d->start_sil=1;
	if(d->cfg->use_mt)
	{
		if(d->start)
		{
			wtk_wfstdec_mt_feed_end(d,1);
			wtk_wfstdec_flush_reced_feature(d);
		}
	}
	wtk_wfstdec_output_reset(d->output);
	//wtk_heap_reset(d->heap);
	d->time_rec=0;
	d->delay_frame=-1;
	d->time_stop=0;
	d->wav_bytes=0;
	d->rec_wav_bytes=0;

	//wtk_debug("[%d/%d]\n",d->parm->feature_hoard.use_length,d->parm->feature_hoard.cur_free);
	if(d->vad)
	{
		wtk_vad_reset(d->vad);
	}
	if(d->rec->use_rescore)
	{
		if(d->rescore)
		{
			wtk_rescore_clean_hist(d->rescore);
		}
	}
	wtk_wfstdec_reset_rec(d);
	if(d->ebnfdec2 && d->env.use_ebnfdec)
	{
		wtk_ebnfdec2_reset(d->ebnfdec2);
	}
	if(d->xbnf)
	{
		wtk_xbnf_rec_reset(d->xbnf);
	}
	wtk_wfstenv_cfg_init(&(d->env),d->cfg);
	wtk_cfg_file_reset(d->env_parser);
	wtk_fextra_reset(d->parm);
	wtk_queue_init(&(d->parm_q));
	wtk_queue_init(&(d->vad_q));
	wtk_fst_net2_reset(d->input_net);
}

int wtk_wfstdec_start(wtk_wfstdec_t *d)
{
	return wtk_wfstdec_start2(d,0,0);
}


int wtk_wfstdec_start_rec(wtk_wfstdec_t *d)
{
	wtk_fst_net_t *net;
	int ret;

	//wtk_debug("usrrec=%p use_ebnf=%d\n",d->usrec,d->cfg->use_ebnf);
	if(d->usrec)
	{
		if(d->env.use_usrec)
		{
			ret=wtk_usrec_start(d->usrec);
			if(ret!=0){goto end;}
		}
		net=wtk_wfstdec_get_net(d);
		ret=wtk_wfstr_start(d->rec,net);
		if(ret!=0){goto end;}
		ret=0;
	}else if(d->cfg->use_ebnf)
	{
		ret=wtk_wfstr_start2(d->rec,d->res_net,d->custom_net);
		if(ret!=0){goto end;}
		ret=0;
	}else
	{
		net=wtk_wfstdec_get_net(d);
		ret=wtk_wfstr_start(d->rec,net);
		if(ret!=0){goto end;}
		ret=0;
	}
end:
	return ret;
}


int wtk_wfstdec_start2(wtk_wfstdec_t *d,char *data,int bytes)
{
	int ret;

	wtk_strbuf_reset(d->hint_buf);
	d->start=1;
	d->time_start=time_get_ms();
	wtk_wfstenv_cfg_init(&(d->env),d->cfg);
	if(bytes>0)
	{
		ret=wtk_cfg_file_feed(d->env_parser,data,bytes);
		if(ret!=0){goto end;}
		ret=wtk_wfstenv_cfg_update_local(&(d->env),d->env_parser->main);
		if(ret!=0){goto end;}
	}
	//wtk_debug("rescore=%p [%.*s]\n",d->usr_rescore,d->env.userid.len,d->env.use_link)
	if(d->cfg->use_vad)
	{
		wtk_vad_start(d->vad);
	}
	if(d->ebnfdec2 && d->env.use_ebnfdec)
	{
		ret=wtk_ebnfdec2_start(d->ebnfdec2);
		if(ret!=0){goto end;}
	}
	wtk_wfstdec_output_start(d->output);
	if(d->cfg->use_dnnc)
	{
		ret=wtk_wfstdec_start_rec(d);
		if(ret!=0){goto end;}
	}else if(d->cfg->use_mt)
	{
		wtk_wfstdec_mt_start(d);
	}else
	{
		ret=wtk_wfstdec_start_rec(d);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}


void wtk_wfstdec_finish_rescore(wtk_wfstdec_t *d)
{
	wtk_string_t v;
	wtk_string_t *sep=&(d->env.sep);
	int ret;

	if(d->rec->cfg->use_lat)
	{
		ret=wtk_wfstr_create_lat_fst(d->rec,d->input_net);
	}else
	{
		ret=-1;
	}
	//wtk_debug("N=%d L=%d\n",d->input_net->state_id,d->input_net->trans_id);
	//wtk_fst_net2_print_lat(d->input_net,stdout);
	//exit(0);
	if(ret==0)
	{
		ret=wtk_rescore_process(d->rescore,d->input_net);
		wtk_rescore_get_result(d->rescore,&v,sep->data,sep->len);
		wtk_wfstdec_output_set_result(d->output,v.data,v.len);
	}else
	{
		wtk_wfstr_finish2(d->rec,d->rec->buf,sep->data,sep->len);
		wtk_wfstdec_output_set_result(d->output,d->rec->buf->data,d->rec->buf->pos);
	}
}

void wtk_wfstdec_finish_lat(wtk_wfstdec_t *d)
{
	wtk_string_t *sep=&(d->env.sep);

	wtk_wfstr_finish2(d->rec,d->rec->buf,sep->data,sep->len);
	wtk_wfstdec_output_set_result(d->output,d->rec->buf->data,d->rec->buf->pos);
}

int wtk_wfstdec_finish_rec(wtk_wfstdec_t *d)
{
	wtk_string_t *sep=&(d->env.sep);
	wtk_wfst_path_t *pth;
	int b=0;

	//wtk_log_log0(glb_log,"================> rec feed feature end");
	if(d->cfg->debug)
	{
		wtk_debug("rec frame=%d\n",d->rec->frame);
	}
	//wtk_fst_rec_print_final(d->rec);
	wtk_wfstr_touch_end(d->rec);
	if(d->usrec)
	{
		if(d->env.use_usrec)
		{
			wtk_usrec_finish(d->usrec);
		}
	}else if(d->cfg->use_ebnf)
	{
		pth=wtk_wfstr_get_path(d->rec);
		if(pth && pth->trans->to_state->custom)
		{
			//wtk_debug("ac=%f lm=%f\n",pth->ac_like,pth->lm_like);
			b=1;
		}
	}
	if(d->rec->use_rescore && b==0)// d->rec->cfg->use_lat)
	{
		if(d->rescore)
		{
			wtk_wfstdec_finish_rescore(d);
		}else
		{
			wtk_wfstdec_finish_lat(d);
		}
	}else
	{
		wtk_wfstr_finish2(d->rec,d->rec->buf,sep->data,sep->len);
		wtk_wfstdec_output_set_result(d->output,d->rec->buf->data,d->rec->buf->pos);
	}
	//wtk_log_log0(glb_log,"================> rec rescoring end");
	return 0;
}

void wtk_wfstdec_restart_rec(wtk_wfstdec_t *d)
{
//#define DEBUG_TX
#ifdef DEBUG_TX
	double t;

	t=time_get_ms();
#endif
	if(d->rescore)
	{
		wtk_rescore_reset(d->rescore);
	}
	//wtk_fst_net_reset_hook(d->net);
	wtk_wfstdec_reset_net(d);
	wtk_wfstr_reset(d->rec);
	wtk_wfstdec_start_rec(d);
#ifdef DEBUG_TX
	t=time_get_ms()-t;
	wtk_debug("time=%f\n",t);
#endif
}

void wtk_fst_dnnc_feed_dnn_feat(wtk_wfstdec_t *d,int cmd,int row,int col,float *f)
{
	wtk_wfstr_t *rec=d->rec;
	int i;
	double t;

	t=time_get_ms();
	if(cmd==1)
	{
		for(i=0;i<row;++i)
		{
			//print_float(f,10);
			//++rec->cache_frame;
			wtk_wfstr_feed(rec,f-1);
			f+=col;
		}
	}else
	{
		wtk_wfstdec_finish_rec(d);
		wtk_sem_release(&(d->rec_wait_sem),1);
	}
	d->time_rec+=time_get_ms()-t;
}

void wtk_wfstdec_feed_dnnc(wtk_wfstdec_t *d,int is_end)
{
#ifdef USE_DNNC
	wtk_queue_t *q;
	wtk_queue_node_t *n;
	wtk_feat_t *feat;
	float *f;
	int col;
	wtk_dnnc_msg_t *msg;
	int i;

	q=d->parm->output_queue;
	if(d->delay_frame>=0)
	{
		d->delay_frame+=q->length;
	}
	if(q->length>0)
	{
		col=sizeof(float)*d->parm->cfg->feature_cols;
		msg=(wtk_dnnc_msg_t*)wtk_malloc(sizeof(wtk_dnnc_msg_t));
		i=col*q->length;
		msg->data=wtk_string_new(i);
		msg->is_end=is_end;
		f=(float*)msg->data->data;
		if(d->dnnc->cfg->debug)
		{
			wtk_debug("feed feature q=%d col=%d\n",q->length,d->parm->cfg->feature_cols);
		}
		while(q->length>0)
		{
			n=wtk_queue_pop(q);
			if(!n){break;}
			feat=data_offset(n,wtk_feat_t,queue_n);
			memcpy(f,feat->rv+1,col);
			//print_float(f2,10);
			///exit(0);
			f+=d->parm->cfg->feature_cols;
			if(feat->app_hook)
			{
				wtk_feat_push_back((wtk_feat_t*)feat->app_hook);
			}
			wtk_feat_push_back(feat);
		}
		wtk_pipequeue_push(&(d->dnnc->pipeq),&(msg->q_n));
	}else
	{
		if(is_end)
		{
			wtk_dnnc_feed(d->dnnc,NULL,0,1);
		}
	}
#endif
}

void wtk_wfstdec_wait_dnnc_end(wtk_wfstdec_t *d)
{
	wtk_sem_acquire(&(d->rec_wait_sem),-1);
}

void wtk_wfstdec_feed_rec(wtk_wfstdec_t *d,int is_end)
{
	wtk_queue_t *q;
	wtk_queue_node_t *n;
	wtk_feat_t *f;

	q=d->parm->output_queue;
	if(d->delay_frame>=0)
	{
		d->delay_frame+=q->length;
	}
	while(q->length>0)
	{
		n=wtk_queue_pop(q);
		if(!n){break;}
		f=data_offset(n,wtk_feat_t,queue_n);
		wtk_wfstdec_feed_feature(d,f);
	}
	if(is_end)
	{
		wtk_wfstdec_feed_end(d);
	}
}

int wtk_wfstdec_feed_parm2(wtk_wfstdec_t *d,char *data,int bytes,int is_end)
{
	int ret;

	ret=wtk_fextra_feed2(d->parm,data,bytes,is_end);
	if(ret!=0){goto end;}
	//wtk_debug("dec=%d dec2=%p\n",d->env.use_ebnfdec2,d->ebnfdec2);
	if(d->env.use_ebnfdec && d->ebnfdec2)
	{
		ret=wtk_ebnfdec2_feed(d->ebnfdec2,data,bytes,is_end);
		if(ret!=0){goto end;}
	}
end:
	return ret;
}

int wtk_wfstdec_feed_parm(wtk_wfstdec_t *d,char *data,int bytes,int is_end)
{
	int ret;

	if(d->cfg->sil_start_data && d->rec_wav_bytes==0)
	{
		wtk_wfstdec_feed_parm2(d,d->cfg->sil_start_data->data,d->cfg->sil_start_data->len,0);
	}
	d->rec_wav_bytes+=bytes;
	//tx=time_get_ms();
	if(is_end)
	{
		if(d->cfg->sil_end_data)
		{
			ret=wtk_wfstdec_feed_parm2(d,data,bytes,0);
			if(ret!=0){goto end;}
			ret=wtk_wfstdec_feed_parm2(d,d->cfg->sil_end_data->data,d->cfg->sil_end_data->len,1);
		}else
		{
			ret=wtk_wfstdec_feed_parm2(d,data,bytes,1);
			if(ret!=0){goto end;}
		}
	}else
	{
		ret=wtk_wfstdec_feed_parm2(d,data,bytes,is_end);
	}
	if(ret!=0){goto end;}
	if(d->cfg->flush_parm)
	{
		wtk_fextra_flush_layer(d->parm,0);
	}
	//printf("parm: %f\n",time_get_ms()-tx);
	//wtk_debug("use_mt=%d\n",d->cfg->use_mt);
	if(d->cfg->use_dnnc)
	{
		wtk_wfstdec_feed_dnnc(d,is_end);
	}else if(!d->cfg->use_mt)
	{
		wtk_wfstdec_feed_rec(d,is_end);
	}
	//printf("dec: %f\n",time_get_ms()-tx);
end:
	return ret;
}

void wtk_wfstdec_flush_vad_left_data(wtk_wfstdec_t *d)
{
	wtk_string_t v;

	wtk_vad_get_left_data(d->vad,&(v));
	if(v.len>0)
	{
		d->rec_wav_bytes+=v.len;
		wtk_wfstdec_feed_parm2(d,v.data,v.len,0);
	}
}

void wtk_wfstdec_flush_vad_wav(wtk_wfstdec_t *d)
{
	wtk_queue_t *q;
	wtk_queue_node_t *n;
	wtk_vframe_t *f;
	int ret;

	wtk_vad_feed(d->vad,0,0,1);
	q=d->vad->output_queue;
	while(q->length>0)
	{
		n=wtk_queue_pop(q);
		if(!n){break;}
		f=data_offset(n,wtk_vframe_t,q_n);
		d->rec_wav_bytes+=f->frame_step<<1;
		ret=wtk_wfstdec_feed_parm2(d,(char*)f->wav_data,f->frame_step<<1,0);
		if(ret!=0){goto end;}
		wtk_vad_push_vframe(d->vad,f);
	}
	wtk_wfstdec_flush_vad_left_data(d);
end:
	return;
}

int wtk_wfstdec_feed_vad(wtk_wfstdec_t *d,char *data,int bytes,int is_end)
{
	wtk_queue_t *q;
	wtk_queue_node_t *n;
	wtk_vframe_t *f;
	int ret;
	int cnt;

	/*
	if(d->wav_bytes==bytes && d->cfg->sil_start_data)
	{
		wtk_vad_feed(d->vad2,WTK_fextra_APPEND,d->cfg->sil_start_data->data,d->cfg->sil_start_data->len);
	}*/
	ret=wtk_vad_feed(d->vad,data,bytes,is_end);
	if(ret!=0){goto end;}
	if(d->cfg->flush_parm && !is_end)
	{
		wtk_vad_flush(d->vad);
	}
	q=d->vad->output_queue;
	if(q->length>0 && d->cfg->sil_start_data && d->start_sil==1)
	{
		wtk_wfstdec_feed_parm2(d,d->cfg->sil_start_data->data,d->cfg->sil_start_data->len,0);
		d->start_sil=0;
	}
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
			ret=wtk_wfstdec_feed_parm2(d,(char*)f->wav_data,cnt,0);
			if(ret!=0){goto end;}
		}
		//wtk_debug("use_vad_restart=%d\n",d->env.use_vad_restart);
		wtk_vad_push_vframe(d->vad,f);
	}
	if(is_end)
	{
		//wtk_debug("get last end=%d/%d\n",d->last_vframe_state,wtk_vframe_sil);
		if(d->last_vframe_state!=wtk_vframe_sil)
		{
			wtk_wfstdec_flush_vad_left_data(d);
		}
		if(d->cfg->sil_end_data)
		{
			ret=wtk_wfstdec_feed_parm2(d,d->cfg->sil_end_data->data,d->cfg->sil_end_data->len,1);
		}else
		{
			ret=wtk_wfstdec_feed_parm2(d,0,0,1);
		}
		if(ret!=0){goto end;}
	}
	if(!is_end && (d->cfg->flush_parm))
	{
		wtk_fextra_flush_layer(d->parm,0);
	}
	if(d->cfg->use_dnnc)
	{
		wtk_wfstdec_feed_dnnc(d,is_end);
	}else if(!d->cfg->use_mt)
	{
		wtk_wfstdec_feed_rec(d,is_end);
	}
end:
	return ret;
}

int wtk_wfstdec_feed(wtk_wfstdec_t *d,char *data,int bytes,int is_end)
{
	return wtk_wfstdec_feed2(d,data,bytes,1,is_end);
}

int wtk_wfstdec_feed2(wtk_wfstdec_t *d,char *data,int bytes,int wait_eof,int is_end)
{
	int ret;

	//wtk_debug("feadd %d\n",bytes);
	if(is_end)
	{
		d->time_stop=time_get_ms();
		d->delay_frame=0;
		d->is_end=1;
		//wtk_log_log0(glb_log,"rec feed data end");
	}
	d->wav_bytes+=bytes;
	if(d->cfg->use_mt)
	{
		wtk_wfstdec_flush_reced_feature(d);
	}
	if(d->vad && d->env.use_vad)
	{
		//wtk_debug("feed vad\n");
		ret=wtk_wfstdec_feed_vad(d,data,bytes,is_end);
	}else
	{
		ret=wtk_wfstdec_feed_parm(d,data,bytes,is_end);
	}
	if(d->cfg->use_dnnc)
	{
		if(is_end && wait_eof)
		{
			wtk_wfstdec_wait_dnnc_end(d);
		}
	}else if(d->cfg->use_mt)
	{
		if(is_end)
		{
			wtk_wfstdec_mt_feed_end(d,wait_eof);
		}else
		{
			wtk_wfstdec_flush_reced_feature(d);
		}
		d->start=0;
	}
	//exit(0);
	return ret;
}

void wtk_wfstdec_print(wtk_wfstdec_t *d)
{
	int bytes=0;
	int t;

	wtk_wfstr_print(d->rec);
	t=wtk_wfstr_bytes(d->rec);
	wtk_debug("rec: %.3fMB\n",t*1.0/(1024*1024));
	bytes+=t;
	if(d->rescore)
	{
		//wtk_fst_rescore_print(d->rescore);
		t=wtk_rescore_bytes(d->rescore);
		wtk_debug("res: %.3fMB\n",t*1.0/(1024*1024));
		bytes+=t;
	}
	t=wtk_fextra_bytes(d->parm);
	wtk_debug("parm: %.3fMB\n",t*1.0/(1024*1024));
	wtk_debug("parm use:%d free=%d\n",d->parm->feature_hoard.use_length,d->parm->feature_hoard.cur_free);
	bytes+=t;
	wtk_debug("tot: %.3fMB\n",bytes*1.0/(1024*1024));
}

int wtk_wfstdec_bytes(wtk_wfstdec_t *d)
{
	int bytes=0;

	bytes+=wtk_wfstr_bytes(d->rec);
	if(d->rescore)
	{
		bytes+=wtk_rescore_bytes(d->rescore);
	}
	bytes+=wtk_fextra_bytes(d->parm);
	return bytes;
}

void wtk_wfstdec_print_mlf(wtk_wfstdec_t *d,FILE *log)
{

}

void wtk_wfstdec_write_lat(wtk_wfstdec_t *d,char *fn)
{

}


//--------------------------- Thread ------------------------------

void wtk_wfstdec_feed_feature(wtk_wfstdec_t *d,wtk_feat_t *f)
{
	//wtk_feat_t *fv;
	wtk_wfstr_t *rec=d->rec;

	if(d->rec_notify_end)
	{
		wtk_wfstdec_send_back_feature(d,f);
		return;
	}
	if(d->usrec && d->env.use_usrec)
	{
		wtk_wfstr_feed2(d->usrec->rec,f);
		//wtk_wfstdec_send_back_feature(d,f);
		//return;
	}
	//wtk_debug("index=%d\n",f->index);
	//wtk_feat_print(f);
	if(f->app_hook)
	{
		//fv=(wtk_feat_t*)f->app_hook;
		//wtk_debug("feed %p idx=%d used=%d\n",fv,fv->index,fv->used);
		wtk_wfstr_feed2(rec,f);
	}else
	{
		wtk_wfstr_feed2(rec,f);
	}

	if(d->rec->end_hint)
	{
		wtk_wfstdec_feed_end(d);
		wtk_wfstdec_restart_rec(d);
	}
	wtk_wfstdec_send_back_feature(d,f);
}

void wtk_wfstdec_feed_end(wtk_wfstdec_t *d)
{
	wtk_wfstdec_finish_rec(d);
}

void wtk_wfstdec_flush_reced_feature(wtk_wfstdec_t *d)
{
	wtk_blockqueue_t *q=&(d->feature_bak_q);
	wtk_queue_node_t *qn;
	wtk_feat_t *f,*fp;
	int dec;

	dec=d->parm->cfg->use_dnn&&d->parm->cfg->dnn.use_mlat;
	//wtk_debug("==> q=%d\n",q->length);
	while(q->length>0)
	{
		qn=wtk_blockqueue_pop(q,0,NULL);
		if(!qn)
		{
			//wtk_debug("break len=%d/%d %p cnt=%d\n",q->length,d->rec_input_q.length,q->pop,cnt);
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

int wtk_wfstdec_send_back_reced_feature(wtk_wfstdec_t *d,wtk_feat_t *f)
{
	//wtk_debug("get feature %d use=%d free=%d\n",f->index,d->parm->feature_hoard.use_length,d->parm->feature_hoard.cur_free);
	wtk_blockqueue_push(&(d->feature_bak_q),&(f->queue_n));
	return 0;
}

void wtk_wfstdec_notify_feature(wtk_wfstdec_t *d,wtk_feat_t *f)
{
	wtk_wfstevt_t *evt;

	//wtk_debug("%p=%d:%d\n",f,f->index,f->used);
	//wtk_debug("notify feature[%d]\n",f->index);
	//f->send_hook=d;
	//f->send=(wtk_feat_sender_t)wtk_wfstdec_get_reced_feature;
	if(d->is_end)
	{
		++d->delay_frame;
	}
	//wtk_blockqueue_push(&(d->rec_input_q),&(f->queue_n));
	evt=wtk_wfstdec_pop_evt(d);
	evt->f=f;
	wtk_blockqueue_push(&(d->rec_input_q),&(evt->q_n));
}

void wtk_wfstdec_send_back_feature(wtk_wfstdec_t *d,wtk_feat_t *f)
{
	wtk_feat_t *f2;

	//wtk_debug("%p=%d:%d\n",f,f->index,f->used);
	if(d->cfg->use_mt)
	{
		//wtk_debug("%p=%d:%d\n",f,f->index,f->used);
		/*
		if(f->app_hook)
		{
			f2=(wtk_feat_t*)f->app_hook;
			wtk_wfstdec_send_back_reced_feature(d,f2);
		}*/
		wtk_wfstdec_send_back_reced_feature(d,f);
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

void wtk_wfstdec_mt_start(wtk_wfstdec_t *d)
{
	wtk_wfstevt_t *evt;

	//d->state=WTK_FST_DEC_WAV_START;
	//wtk_blockqueue_wake(&(d->rec_input_q));
	evt=wtk_wfstdec_pop_evt(d);
	evt->type=WTK_WFSTEVT_START;
	//wtk_debug("push evt=%d\n",evt->type);
	wtk_blockqueue_push(&(d->rec_input_q),&(evt->q_n));
	wtk_sem_acquire(&(d->rec_start_wait_sem),-1);
}


void wtk_wfstdec_mt_feed_end(wtk_wfstdec_t *d,int wait_eof)
{
	wtk_wfstevt_t *evt;

	//d->state=WTK_FST_DEC_WAV_END;
	//wtk_blockqueue_wake(&(d->rec_input_q));
	evt=wtk_wfstdec_pop_evt(d);
	evt->type=WTK_WFSTEVT_END;
	wtk_blockqueue_push(&(d->rec_input_q),&(evt->q_n));
	if(wait_eof)
	{
		wtk_wfstdec_wait_end(d,-1);
	}
}

int wtk_wfstdec_wait_end(wtk_wfstdec_t *d,int timeout)
{
	int ret;

	ret=wtk_sem_acquire(&(d->rec_wait_sem),timeout);
	if(ret==0)
	{
		wtk_wfstdec_flush_reced_feature(d);
	}
	return ret;
}

void wtk_wfstdec_update_vad_restart_result(wtk_wfstdec_t *d)
{
	wtk_wfstevt_t *evt;

	//wtk_debug("update vad\n");
	if(d->cfg->use_mt)
	{
		evt=wtk_wfstdec_pop_evt(d);
		evt->type=WTK_WFSTEVT_RESTART;
		wtk_blockqueue_push(&(d->rec_input_q),&(evt->q_n));
	}else
	{
		wtk_wfstdec_feed_rec(d,1);
		wtk_wfstdec_restart_rec(d);
	}
}

void wtk_wfstdec_stop_rec(wtk_wfstdec_t *d)
{
	//d->state=WTK_FST_DEC_DIE;
	d->run=0;
	wtk_blockqueue_wake(&(d->rec_input_q));
	wtk_thread_join(&(d->thread));
}

int wtk_wfstdec_run_rec(wtk_wfstdec_t *d,wtk_thread_t *t)
{
	wtk_wfstevt_t *evt;
	wtk_blockqueue_t *q=&(d->rec_input_q);
	wtk_queue_node_t *qn;
	double time=0;
	int timeout=-1;
	int b;

	while(d->run)
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
		evt=data_offset(qn,	wtk_wfstevt_t,q_n);
		//wtk_debug("rec=%d pend=%d\n",d->rec->frame,q->length);
		switch(evt->type)
		{
		case WTK_WFSTEVT_START:
			time=time_get_cpu();
			wtk_wfstdec_start_rec(d);
			//wtk_fst_rec_start(d->rec,d->net);
			//wtk_debug("next\n");
			wtk_sem_release(&(d->rec_start_wait_sem),1);
			break;
		case WTK_WFSTEVT_FEAT:
			//if(evt->v.f->index%2==1)
			{
				//wtk_debug("input=%d\n",q->length);
				wtk_wfstdec_feed_feature(d,evt->f);
			}
			break;
		case WTK_WFSTEVT_RESTART:
			wtk_wfstdec_feed_end(d);
			wtk_wfstdec_restart_rec(d);
			break;
		case WTK_WFSTEVT_END:
			wtk_wfstdec_feed_end(d);
			d->time_rec=time_get_cpu()-time;
			wtk_sem_release(&(d->rec_wait_sem),1);
			break;
		}
		wtk_wfstdec_push_evt(d,evt);
	}
	return 0;
}

void wtk_wfstdec_get_result(wtk_wfstdec_t *d,wtk_string_t *v)
{
	wtk_wfstdec_output_get_result(d->output,v);
}

void wtk_wfstdec_get_str_result(wtk_wfstdec_t *d,wtk_string_t *v)
{
	wtk_wfstdec_output_get_str_result(d->output,v);
}


void wtk_wfstdec_get_hint_result(wtk_wfstdec_t *d,wtk_string_t *v)
{
	wtk_strbuf_t *buf=d->rec->buf;
	wtk_strbuf_t *buf2=d->output->hint;
	wtk_strbuf_t *hint=d->hint_buf;

	if(d->env.use_hint)
	{
		wtk_wfstr_finish2(d->rec,buf,
				d->env.sep.data,d->env.sep.len);
		if(buf->pos>0)
		{
			wtk_strbuf_reset(buf2);
			wtk_strbuf_push_s(buf2,"{\"hint\":\"");
			wtk_strbuf_push(buf2,buf->data,buf->pos);
			wtk_strbuf_push_s(buf2,"\"}");
			if(hint->pos==0 || wtk_data_cmp(buf2->data,buf2->pos,hint->data,hint->pos)!=0)
			{
				wtk_strbuf_reset(hint);
				wtk_strbuf_push(hint,buf2->data,buf2->pos);
				wtk_string_set(v,hint->data,hint->pos);
			}else
			{
				wtk_string_set(v,0,0);
			}
		}else
		{
			wtk_string_set(v,0,0);
		}
	}else
	{
		wtk_string_set(v,0,0);
	}
}

void wtk_wfstdec_set_notify(wtk_wfstdec_t *d,void *ths,wtk_wfstdec_notify_f notify)
{
	//wtk_fst_rec_set_notify(d->rec,ths,notify);
	d->notify=notify;
	d->notify_ths=ths;
}


void wtk_wfstdec_set_custom_net(wtk_wfstdec_t *d,wtk_fst_net_t *net)
{
	d->custom_net=net;
}

float wtk_wfstdec_get_conf(wtk_wfstdec_t *d)
{
	return wtk_wfstr_get_conf(d->rec);
}

int wtk_wfstdec_is_forceout(wtk_wfstdec_t *d)
{
	return d->rec->best_final_tokset.path?0:1;
}

void wtk_wfstdec_print_path(wtk_wfstdec_t *d)
{
	wtk_wfst_path_t *pth;
	wtk_strbuf_t *buf;
	float conf;

	pth=wtk_wfstr_get_path(d->rec);
	if(!pth){return;}
	buf=wtk_strbuf_new(256,1);
	wtk_wfstr_finish4(d->rec,pth,buf,0,0);
	conf=pth->ac_like/pth->frame;
	wtk_debug("[%.*s] frame=%d conf=%f\n",buf->pos,buf->data,pth->frame,conf);
	wtk_strbuf_delete(buf);
}

