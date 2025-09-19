#include "qtk_decoder_wrapper.h"
int qtk_decoder_wrapper_run_rec(qtk_decoder_route_t* route,wtk_thread_t *t);
void qtk_decoder_wrapper_notify_feature(qtk_decoder_wrapper_t *wrapper,wtk_feat_t *f);
void qtk_decoder_wrapper_flush_reced_feature(qtk_decoder_wrapper_t *warpper);
void qtk_decoder_wrapper_mt_start(qtk_decoder_route_t* route);
void qtk_decoder_wrapper_mt_feed_end(qtk_decoder_route_t* route,int wait_eof);
int qtk_decoder_wrapper_wait_end(qtk_decoder_route_t* route,int timeout);
void qtk_decoder_wrapper_stop_rec(qtk_decoder_route_t* route);
void qtk_decoder_wrapper_send_back_feature(qtk_decoder_wrapper_t *warpper,wtk_feat_t *f);
int qtk_decoder_wrapper_feed2(qtk_decoder_wrapper_t* wrapper,char *data,int bytes,int is_end);
int qtk_decoder_wrapper_feed3(qtk_decoder_wrapper_t* wrapper,short *data,int bytes,int is_end);

qtk_decoder_evt_t* qtk_decoder_wrapper_new_evt(qtk_decoder_wrapper_t* wrapper)
{
	qtk_decoder_evt_t *evt;

	evt=(qtk_decoder_evt_t*)wtk_malloc(sizeof(qtk_decoder_evt_t));
	return evt;
}

int qtk_decoder_wrapper_evt_delete(qtk_decoder_evt_t *evt)
{
	wtk_free(evt);
	return 0;
}

qtk_decoder_evt_t* qtk_decoder_wrapper_pop_evt(qtk_decoder_route_t* route)
{
	qtk_decoder_evt_t *evt;

	evt=(qtk_decoder_evt_t*)wtk_lockhoard_pop(&(route->evt_hoard));
	evt->type=QTK_DECODER_EVT_FEED;
	evt->f=NULL;
	return evt;
}

void qtk_decoder_wrapper_push_evt(qtk_decoder_route_t* route,qtk_decoder_evt_t *evt)
{	
	wtk_lockhoard_push(&(route->evt_hoard),evt);
}

qtk_decoder_route_t* qtk_decoder_wrapper_route_new(qtk_kwfstdec_cfg_t* dec_cfg,char* name,int use_lite)
{
	qtk_decoder_route_t* route;

	route=(qtk_decoder_route_t*)wtk_malloc(sizeof(qtk_decoder_route_t));
	route->res_buf=wtk_strbuf_new(1024,1);
	route->hint_buf=wtk_strbuf_new(1024,1);
	route->run=1;
	wtk_thread_init(&(route->thread),(thread_route_handler)qtk_decoder_wrapper_run_rec,route);
	wtk_thread_set_name(&(route->thread),name);
	wtk_blockqueue_init(&(route->rec_input_q));
	wtk_sem_init(&(route->rec_wait_sem),0);
	wtk_sem_init(&(route->rec_start_wait_sem),0);
	wtk_thread_start(&(route->thread));
	wtk_lockhoard_init(&(route->evt_hoard),offsetof(qtk_decoder_evt_t,hoard_n),1024,(wtk_new_handler_t)qtk_decoder_wrapper_new_evt,
						(wtk_delete_handler_t)qtk_decoder_wrapper_evt_delete,route);
	if(use_lite == 1)
	{
		route->dec_lite=qtk_kwfstdec_lite_new(dec_cfg);
		route->dec=NULL;
	}
	else
	{
		route->dec=qtk_kwfstdec_new(dec_cfg);
		route->dec_lite=NULL;
	}
	route->time_rec = 0.0;
	route->time_start = 0.0;

	return route;
}

qtk_decoder_route_t* qtk_decoder_wrapper_route_new2(qtk_kwfstdec_cfg_t* dec_cfg,char* name, int use_outnet,int use_lite)
{
	qtk_decoder_route_t* route;

	route=(qtk_decoder_route_t*)wtk_malloc(sizeof(qtk_decoder_route_t));
	route->res_buf=wtk_strbuf_new(1024,1);
	route->hint_buf=wtk_strbuf_new(1024,1);
	route->run=1;
	wtk_thread_init(&(route->thread),(thread_route_handler)qtk_decoder_wrapper_run_rec,route);
	wtk_thread_set_name(&(route->thread),name);
	wtk_blockqueue_init(&(route->rec_input_q));
	wtk_sem_init(&(route->rec_wait_sem),0);
	wtk_sem_init(&(route->rec_start_wait_sem),0);
	wtk_thread_start(&(route->thread));
	wtk_lockhoard_init(&(route->evt_hoard),offsetof(qtk_decoder_evt_t,hoard_n),1024,(wtk_new_handler_t)qtk_decoder_wrapper_new_evt,
						(wtk_delete_handler_t)qtk_decoder_wrapper_evt_delete,route);
	if(use_lite == 1)
	{
		route->dec_lite=qtk_kwfstdec_lite_new2(dec_cfg,use_outnet);
		route->dec=NULL;
	}
	else
	{
		route->dec=qtk_kwfstdec_new2(dec_cfg, use_outnet);
		route->dec_lite=NULL;
	}
	route->time_rec = 0.0;
	route->time_start = 0.0;

	return route;
}

void qtk_decoder_wrapper_route_reset(qtk_decoder_route_t* route)
{
	wtk_strbuf_reset(route->res_buf);
	wtk_strbuf_reset(route->hint_buf);
	if(route->dec)
		qtk_kwfstdec_reset(route->dec);
	else
		qtk_kwfstdec_lite_reset(route->dec_lite);
	route->time_rec = 0.0;
	route->time_start = 0.0;
}

void qtk_decoder_wrapper_route_delete(qtk_decoder_route_t* route)
{
	qtk_decoder_wrapper_stop_rec(route);
	wtk_strbuf_delete(route->res_buf);
	wtk_strbuf_delete(route->hint_buf);
	//wtk_blockqueue_clean(&(wrapper->feature_bak_q));
	wtk_sem_clean(&(route->rec_wait_sem));
	wtk_sem_clean(&(route->rec_start_wait_sem));
	//wtk_debug("clean thread\n");
	wtk_thread_clean(&(route->thread));
	wtk_blockqueue_clean(&(route->rec_input_q));
	wtk_lockhoard_clean(&(route->evt_hoard));
	if(route->dec)
		qtk_kwfstdec_delete(route->dec);
	else
		qtk_kwfstdec_lite_delete(route->dec_lite);
	wtk_free(route);
}

qtk_decoder_wrapper_t* qtk_decoder_wrapper_new(qtk_decoder_wrapper_cfg_t* cfg)
{
	qtk_decoder_wrapper_t *wrapper;

	wrapper=(qtk_decoder_wrapper_t*)wtk_malloc(sizeof(*wrapper));
	wrapper->cfg=cfg;
	wrapper->env_parser=wtk_cfg_file_new();
	if(cfg->use_kxparm)
	{
		wrapper->parm=NULL;
		wrapper->kxparm=wtk_kxparm_new(&(cfg->parm));
	}else
	{
		wrapper->parm=wtk_fextra_new(&(cfg->extra));
		wrapper->kxparm=NULL;
	}
	wrapper->vf = NULL;
	//wrapper->dec=qtk_kwfstdec_new(&(cfg->kwfstdec));
	wtk_queue_init(&(wrapper->parm_q));
	//wrapper->res_buf=wtk_strbuf_new(1024,1);
	wrapper->res_buf=NULL;
	wrapper->hint_buf=NULL;
	wrapper->json_buf=wtk_strbuf_new(1024,1);
	wrapper->json=wtk_json_new();
	wrapper->time_stop=0;
	wrapper->wav_bytes=0;
	wrapper->rec_wav_bytes=0;
	wrapper->index=0;
	wrapper->data_left=0;
	wrapper->chnlike=NULL;
	wrapper->dec = NULL;
	wrapper->dec_lite = NULL;
    wtk_queue_init(&(wrapper->vad_q));
	if(cfg->use_xbnf)
	{
		wrapper->xbnf=wtk_xbnf_rec_new(&(cfg->xbnf));
	}else
	{
		wrapper->xbnf=NULL;
	}

    if(cfg->use_vad)
    {
        wrapper->vad=wtk_vad_new(&(cfg->vad),&(wrapper->vad_q));
        wrapper->last_vframe_state=wtk_vframe_sil;
		wrapper->vad_buf=wtk_strbuf_new(1024,1);
		wrapper->whole_buf=wtk_strbuf_new(1024,1);
    }else
    {
        wrapper->vad=0;
		wrapper->vad_buf=NULL;
		wrapper->whole_buf=NULL;
    }
	if(cfg->use_xvprint)
	{
		wrapper->xvprint=wtk_xvprint_new(&(cfg->xvprint));
		wrapper->person_buf=wtk_strbuf_new(1024,1);
	}else
	{
		wrapper->xvprint=0;
		wrapper->person_buf=NULL;
	}
	if(cfg->use_lex)
	{
		wrapper->lex=wtk_lex_new(&(cfg->lex));
		wtk_lex_compile(wrapper->lex,cfg->lex_fn);
	}else
	{
		wrapper->lex=0;
	}
	if(cfg->use_ebnfdec2)
	{
		wrapper->ebnfdec2=wtk_ebnfdec2_new(&(cfg->ebnfdec2));
	}else
	{
		wrapper->ebnfdec2=NULL;
	}
	if(wrapper->parm)
	{
		if(cfg->use_mt)
		{
	//		wrapper->run=1;
	//		wtk_thread_init(&(wrapper->thread),(thread_route_handler)qtk_decoder_wrapper_run_rec,wrapper);//TODO run thread
	//		wtk_thread_set_name(&(wrapper->thread),"kaldi_rec");
			if(cfg->extra.use_nnet3)
			{
				qtk_nnet3_set_notify(wrapper->parm->nnet3,(qtk_nnet3_feature_notify_f)qtk_decoder_wrapper_feed_nnet3_feature,wrapper);//TODO notify feature
			}else
			{
				wtk_fextra_set_notify(wrapper->parm,(wtk_fextra_feature_notify_f)qtk_decoder_wrapper_notify_feature,wrapper);//TODO notify feature
			}

			wrapper->asr=(qtk_decoder_route_t**)wtk_malloc(sizeof(qtk_decoder_route_t*)*2);
			wrapper->asr[0]=qtk_decoder_wrapper_route_new(&(wrapper->cfg->kwfstdec),"normal",wrapper->cfg->use_lite);
			if(wrapper->cfg->use_lite)
				wrapper->asr[0]->dec_lite->trans_model=cfg->extra.nnet3.t_mdl->trans_model;
			else
			{
				wrapper->asr[0]->dec->trans_model=cfg->extra.nnet3.t_mdl->trans_model;
				wrapper->chnlike=wtk_chnlike_new(&(wrapper->cfg->chnlike),wrapper->cfg->rbin);
				wrapper->asr[0]->dec->chnlike=wrapper->chnlike;
			}
			if(wrapper->cfg->asr_route==2)
			{
				wrapper->asr[1]=qtk_decoder_wrapper_route_new(&(wrapper->cfg->kwfstdec2),"little",wrapper->cfg->use_lite);
				if(wrapper->cfg->use_lite)
					wrapper->asr[1]->dec_lite->trans_model=cfg->extra.nnet3.t_mdl->trans_model;
				else
					wrapper->asr[1]->dec->trans_model=cfg->extra.nnet3.t_mdl->trans_model;
			}
			wtk_blockqueue_init(&(wrapper->feature_bak_q));
		}else if(cfg->extra.use_nnet3)
		{
			wrapper->res_buf=wtk_strbuf_new(1024,1);
			wrapper->hint_buf=wtk_strbuf_new(1024,1);
			if(wrapper->cfg->use_lite)
				wrapper->dec_lite=qtk_kwfstdec_lite_new(&(cfg->kwfstdec));
			else
			{
				if (cfg->kwfstdec.use_eval)
					wrapper->dec=qtk_kwfstdec_new3(&(cfg->kwfstdec), 0);
				else
					wrapper->dec=qtk_kwfstdec_new(&(cfg->kwfstdec));
			}
			qtk_nnet3_set_notify(wrapper->parm->nnet3,(qtk_nnet3_feature_notify_f)qtk_decoder_wrapper_feed_nnet3_feature,wrapper);//TODO notify feature
			//wrapper->dec->cfg->trans_model=*wrapper->cfg->extra.nnet3.t_mdl;
			//wtk_debug("fffa:%p %p\n",wrapper->dec->cfg->trans_model,wrapper->dec->cfg->trans_model.trans_model);
			//wrapper->dec->trans_model= wrapper->dec->cfg->trans_model.trans_model;
			wrapper->dec->trans_model=cfg->extra.nnet3.t_mdl->trans_model;
		}else
		{
			wrapper->res_buf=wtk_strbuf_new(1024,1);
			wrapper->hint_buf=wtk_strbuf_new(1024,1);
			if(wrapper->cfg->use_lite)
				wrapper->dec_lite=qtk_kwfstdec_lite_new(&(cfg->kwfstdec));
			else
				wrapper->dec=qtk_kwfstdec_new(&(cfg->kwfstdec));
			wtk_fextra_set_output_queue(wrapper->parm,&(wrapper->parm_q));
		}
	}else
	{
		if(cfg->use_mt)
		{
			qtk_nnet3_set_notify(wrapper->kxparm->nnet3,(qtk_nnet3_feature_notify_f)qtk_decoder_wrapper_feed_nnet3_feature,wrapper);//TODO notify feature
			wrapper->asr=(qtk_decoder_route_t**)wtk_malloc(sizeof(qtk_decoder_route_t*)*2);
			wrapper->asr[0]=qtk_decoder_wrapper_route_new(&(wrapper->cfg->kwfstdec),"normal",wrapper->cfg->use_lite);
			if(wrapper->cfg->use_lite)
				wrapper->asr[0]->dec_lite->trans_model=cfg->parm.nnet3.t_mdl->trans_model;
			else
			{
				wrapper->chnlike=wtk_chnlike_new(&(wrapper->cfg->chnlike),wrapper->cfg->rbin);
				wrapper->asr[0]->dec->chnlike=wrapper->chnlike;
				wrapper->asr[0]->dec->trans_model=cfg->parm.nnet3.t_mdl->trans_model;
			}
			if(wrapper->cfg->asr_route==2)
			{
				wrapper->asr[1]=qtk_decoder_wrapper_route_new(&(wrapper->cfg->kwfstdec2),"little",wrapper->cfg->use_lite);
				if(wrapper->cfg->use_lite)
					wrapper->asr[1]->dec_lite->trans_model=cfg->parm.nnet3.t_mdl->trans_model;
				else
					wrapper->asr[1]->dec->trans_model=cfg->parm.nnet3.t_mdl->trans_model;
			}
			wtk_blockqueue_init(&(wrapper->feature_bak_q));
		}
	}

	return wrapper;
}

void qtk_decoder_wrapper_flush_vad_left_data(qtk_decoder_wrapper_t *d)
{
    wtk_string_t v;

    wtk_vad_get_left_data(d->vad,&(v));
    if(v.len>0)
    {
		d->rec_wav_bytes+=v.len;
        qtk_decoder_wrapper_feed2(d,v.data,v.len,0);
    }
}

int qtk_decoder_wrapper_start3(qtk_decoder_wrapper_t* wrapper);

int qtk_decoder_wrapper_feed_vad(qtk_decoder_wrapper_t *d,char *data,int bytes,int is_end)
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
        d->last_vframe_state = f->state;
        //wtk_debug("v[%d]=%d/%d\n",f->index,f->state,f->raw_state);
        if(f->state!=wtk_vframe_sil)
        {   
            cnt=f->frame_step<<1;
            d->rec_wav_bytes+=cnt;
            qtk_decoder_wrapper_feed2(d,(char*)f->wav_data,cnt,0);
        }
		if(f->state == wtk_vframe_speech_end)
		{
        	qtk_decoder_wrapper_feed2(d,0,0,1);
			wtk_strbuf_push(d->vad_buf,d->asr[0]->res_buf->data,d->asr[0]->res_buf->pos);
			qtk_decoder_wrapper_reset(d);
			qtk_decoder_wrapper_start3(d);
		}
        //wtk_debug("use_vad_restart=%d\n",d->env.use_vad_restart);
        wtk_vad_push_vframe(d->vad,f);
    }
    if(is_end)
    {
        //wtk_debug("get last end=%d/%d\n",d->last_vframe_state,wtk_vframe_sil);
        if(d->last_vframe_state!=wtk_vframe_sil)
        {   
            qtk_decoder_wrapper_flush_vad_left_data(d);
        }
        qtk_decoder_wrapper_feed2(d,0,0,1);
    }
end:
    return ret;
}

void qtk_decoder_wrapper_nnet3_delete(qtk_decoder_wrapper_t *d)
{
	if(d->parm)
	{
		qtk_nnet3_del(d->parm->nnet3);
	}else
	{
		qtk_nnet3_del(d->kxparm->nnet3);
	}
}

int qtk_decoder_wrapper_feed_kvad(qtk_decoder_wrapper_t *d,char *data,int bytes,int is_end)
{
    wtk_queue_t *q;
    wtk_queue_node_t *n;
    wtk_vframe_t *f;
	wtk_string_t v;//for lex
	wtk_string_t *v2;//for xvprint
    int ret;
    int cnt;
	wtk_string_t v3;
    ret=wtk_vad_feed(d->vad,data,bytes,is_end);
    if(ret!=0){goto end;}
    q=d->vad->output_queue;
    while(q->length>0)
    {
        n=wtk_queue_pop(q);
        if(!n){break;}
        f=data_offset(n,wtk_vframe_t,q_n);
        //wtk_debug("v[%d]=%d/%d\n",f->index,f->state,f->raw_state);

		if(d->last_vframe_state == 1)
		{
			if(f->state == 0 )
			{
        		if(d->parm)
					qtk_decoder_wrapper_feed2(d,0,0,1);
				else
					qtk_decoder_wrapper_feed3(d,0,0,1);
				d->data_left=0;
				if(d->lex)
				{
					v=wtk_lex_process(d->lex,d->asr[0]->res_buf->data,d->asr[0]->res_buf->pos);
					wtk_strbuf_push(d->vad_buf,v.data,v.len);
					wtk_lex_reset(d->lex);	
					wtk_strbuf_push_c(d->vad_buf,',');
				}else
				{
					wtk_strbuf_push(d->vad_buf,d->asr[0]->res_buf->data,d->asr[0]->res_buf->pos);
					wtk_strbuf_push_c(d->vad_buf,',');
				}
				if(d->xvprint)
				{
					wtk_xvprint_feed(d->xvprint,0,0,1);
				}
				qtk_decoder_wrapper_reset(d);
				qtk_decoder_wrapper_start3(d);
				if(d->xvprint)
				{
					wtk_vecf_t *feat;
				    feat=wtk_xvprint_compute_feat(d->xvprint);
				    v2=wtk_xvprint_feat_likelihood(d->xvprint,feat);
					if(v2!=NULL)
					{
						wtk_strbuf_push(d->person_buf,v2->data,v2->len);
						wtk_strbuf_push_c(d->person_buf,',');
					}else
					{
						wtk_strbuf_push_string(d->person_buf,"NULL");
						wtk_strbuf_push_c(d->person_buf,',');
					}
					wtk_xvprint_start(d->xvprint);
					qtk_decoder_wrapper_get_result_with_name(d,&v3);
				}
			}
		}

		if(f->state == 1)
		{
			d->data_left=1;
            cnt=f->frame_step<<1;
            d->rec_wav_bytes+=cnt;
			if(d->parm)
            	qtk_decoder_wrapper_feed2(d,(char*)f->wav_data,cnt,0);
			else
            	qtk_decoder_wrapper_feed3(d,(short*)f->wav_data,cnt/2,0);
			if(d->xvprint)
			{
				wtk_xvprint_feed(d->xvprint,(short*)f->wav_data,(cnt)>>1,0);
			}
		}
        //wtk_debug("use_vad_restart=%d\n",d->env.use_vad_restart);
        d->last_vframe_state=f->state;
        wtk_vad_push_vframe(d->vad,f);
    }
    if(is_end)
    {
        //wtk_debug("get last end=%d/%d\n",d->last_vframe_state,wtk_vframe_sil);
        if(d->last_vframe_state!=wtk_vframe_sil)
        {   
            qtk_decoder_wrapper_flush_vad_left_data(d);
        }
		if(d->data_left == 1 )
		{
			if(d->parm)
        		qtk_decoder_wrapper_feed2(d,0,0,1);
			else
				qtk_decoder_wrapper_feed3(d,0,0,1);
			if(d->lex)
			{
				v=wtk_lex_process(d->lex,d->asr[0]->res_buf->data,d->asr[0]->res_buf->pos);
				wtk_strbuf_push(d->vad_buf,v.data,v.len);
				wtk_lex_reset(d->lex);	
				wtk_strbuf_push_c(d->vad_buf,'.');
			}else
			{
				wtk_strbuf_push(d->vad_buf,d->asr[0]->res_buf->data,d->asr[0]->res_buf->pos);
				wtk_strbuf_push_c(d->vad_buf,'.');
			}
		}else
		{
			qtk_decoder_wrapper_nnet3_delete(d);
		}
		if(d->xvprint)
		{
			wtk_xvprint_feed(d->xvprint,0,0,1);
			wtk_vecf_t *feat;
		    feat=wtk_xvprint_compute_feat(d->xvprint);
		    v2=wtk_xvprint_feat_likelihood(d->xvprint,feat);
			if(v2!=NULL)
			{
				wtk_strbuf_push(d->person_buf,v2->data,v2->len);
				wtk_strbuf_push_c(d->person_buf,'.');
			}else
			{
				wtk_strbuf_push_string(d->person_buf,"NULL");
				wtk_strbuf_push_c(d->person_buf,'.');
			}
		}
    }
end:
    return ret;
}

int qtk_decoder_wrapper_start(qtk_decoder_wrapper_t* wrapper)
{
	return qtk_decoder_wrapper_start2(wrapper,0,0);
}

int qtk_decoder_wrapper_start2(qtk_decoder_wrapper_t* wrapper,char *data,int bytes)
{	
	int ret;
	wtk_wfstenv_cfg_init2(&(wrapper->env));
	if(bytes>0)
	{
		ret=wtk_cfg_file_feed(wrapper->env_parser,data,bytes);
		if(ret!=0){goto end;}
		if(wrapper->ebnfdec2)
		{
			ret=wtk_wfstenv_cfg_update_local2(&(wrapper->env),wrapper->env_parser->main,1);
		}else
		{
			ret=wtk_wfstenv_cfg_update_local2(&(wrapper->env),wrapper->env_parser->main,0);
		}
		if(ret!=0){goto end;}
	}

    if(wrapper->cfg->use_vad && wrapper->env.use_vad)
    {   
        wtk_vad_start(wrapper->vad);
		wtk_strbuf_reset(wrapper->vad_buf);
		wtk_strbuf_reset(wrapper->whole_buf);
    }

	if(wrapper->cfg->use_xvprint)
	{
		wtk_xvprint_start(wrapper->xvprint);
	}

	if(wrapper->ebnfdec2 && wrapper->env.use_ebnfdec)
	{
		//wtk_debug("ebnf2 start\n");
		wtk_ebnfdec2_start(wrapper->ebnfdec2);
	}
	if(wrapper->cfg->use_mt)
	{
		//int i;
		//for(i=0;i<wrapper->cfg->asr_route;i++)
		//{
			qtk_decoder_wrapper_mt_start(wrapper->asr[0]);
		//}
		if(wrapper->cfg->asr_route==2 && wrapper->env.use_dec2)
		{
			qtk_decoder_wrapper_mt_start(wrapper->asr[1]);
		}
	}else
	{
		if(wrapper->dec)
		{
			if (wrapper->dec->cfg->use_eval)
				qtk_kwfstdec_start3(wrapper->dec);
			else
				qtk_kwfstdec_start(wrapper->dec);
		}
		else
			qtk_kwfstdec_lite_start(wrapper->dec_lite);
	}
	ret=0;
	end:
		return ret;
}

int qtk_decoder_wrapper_start3(qtk_decoder_wrapper_t* wrapper)
{	
    if(wrapper->cfg->use_vad && wrapper->env.use_vad)
    {   
        wtk_vad_start(wrapper->vad);
    }

	if(wrapper->ebnfdec2 && wrapper->env.use_ebnfdec)
	{
		//wtk_debug("ebnf2 start\n");
		wtk_ebnfdec2_start(wrapper->ebnfdec2);
	}
	if(wrapper->cfg->use_mt)
	{
		//int i;
		//for(i=0;i<wrapper->cfg->asr_route;i++)
		//{
			qtk_decoder_wrapper_mt_start(wrapper->asr[0]);
		//}
		if(wrapper->cfg->asr_route==2 && wrapper->env.use_dec2)
		{
			qtk_decoder_wrapper_mt_start(wrapper->asr[1]);
		}
	}else
	{
        if(wrapper->dec)
            qtk_kwfstdec_start(wrapper->dec);
        else
            qtk_kwfstdec_lite_start(wrapper->dec_lite);
	}
	return 0;
}

void qtk_wrapper_feed_feature(qtk_decoder_wrapper_t* wrapper,wtk_feat_t *f)
{
	qtk_kwfstdec_feed(wrapper->dec,f);
	//qtk_decoder_wrapper_send_back_feature(wrapper,f);
}

void qtk_decoder_wrapper_nnet3_feed_route(qtk_decoder_route_t* route,float *f,int index)
{
	qtk_decoder_evt_t *evt;

	evt=qtk_decoder_wrapper_pop_evt(route);
				//evt->f=f;
	evt->index=index;
	evt->out=f;
	wtk_blockqueue_push(&(route->rec_input_q),&(evt->q_n));
}

extern wtk_transcription_t* qtk_kwfstdec_get_trans2(qtk_kwfstdec_t* dec, void* data, int state, float frame_scale);
extern wtk_transcription_t* qtk_kwfstdec_get_trans3(qtk_kwfstdec_t* dec, void* data, int state, float frame_scale, int skip);

//int static ssss=0;
void qtk_decoder_wrapper_feed_nnet3_feature(qtk_decoder_wrapper_t *wrapper,qtk_blas_matrix_t *f,int end, int plus)
{
	if(f && wrapper->cfg->extra.nnet3.prior)
	{
//		ssss++;
//		if(ssss==23)
//		{
//			qtk_blas_matrix_print(f);
//			exit(0);
//		}
		//wtk_debug("11111\n");
		//qtk_blas_matrix_print(f);
		int i,j;
		float *data,*p;
		for(i=0;i<f->row;i++)
		{
			if(wrapper->parm)
			{
				p=wrapper->cfg->extra.nnet3.prior->m;
			}else
			{
				p=wrapper->cfg->parm.nnet3.prior->m;
			}
			data=f->m+i*f->col;
			for(j=0;j<f->col;j++)
			{
				data[j]+=p[j];
			}
		}
	}

	if(wrapper->cfg->use_mt)
	{
		int j;
		if(f)
		{
		//	for(i=0;i<wrapper->cfg->asr_route;i++)
		//	{
			for(j=0;j<f->row;j++)
			{
				qtk_decoder_wrapper_nnet3_feed_route(wrapper->asr[0],f->m+f->col*j,wrapper->index);
				if(wrapper->cfg->asr_route==2 && wrapper->env.use_dec2)
		       	{
					qtk_decoder_wrapper_nnet3_feed_route(wrapper->asr[1],f->m+f->col*j,wrapper->index);
				}
				wrapper->index++;
			}
		//	}
		//	wrapper->index++;
		}
		if(end)
		{
			//for(i=0;i<wrapper->cfg->asr_route;i++)
			//{
				//qtk_decoder_wrapper_nnet3_feed_route(wrapper->asr[i]);
				qtk_decoder_wrapper_mt_feed_end(wrapper->asr[0],1);
				if(wrapper->cfg->asr_route==2 && wrapper->env.use_dec2)
				{
					qtk_decoder_wrapper_mt_feed_end(wrapper->asr[1],1);
				}
			//}
		}
	}else
	{
		if(f)
		{
	        if(wrapper->dec)
	        {
	        	if (wrapper->dec->cfg->use_eval)
	        		qtk_kwfstdec_feed3(wrapper->dec,f->m,wrapper->index);
	        	else
	        		qtk_kwfstdec_feed2(wrapper->dec,f->m,wrapper->index);
	        }
        	else
            	qtk_kwfstdec_lite_feed2(wrapper->dec_lite,f->m,wrapper->index);

			wrapper->index++;
		}
		if(end)
		{
	        if(wrapper->dec)
	        {
	        	if (wrapper->dec->cfg->use_eval)
	        	{
	        		//qtk_kwfstdec_extra_compute_final_cost(wrapper->dec);
	        		qtk_kwfstdec_get_result3(wrapper->dec,wrapper->res_buf);
#ifdef DEBUG
	        		wtk_transcription_print(qtk_kwfstdec_get_trans3(wrapper->dec, NULL, 1, wrapper->cfg->extra.frame_dur*1000*10000, wrapper->cfg->extra.nnet3.frame_subsample_factor*wrapper->dec->cfg->frame_skip));
#endif
	        	}
	        	else
	        		qtk_kwfstdec_get_result(wrapper->dec,wrapper->res_buf);
	        }
    	    else
        	    qtk_kwfstdec_lite_get_result(wrapper->dec_lite,wrapper->res_buf);

			//qtk_kwfstdec_get_fa(wrapper->dec,wrapper->res_buf);
		}
	}
}

int qtk_decoder_wrapper_feed2(qtk_decoder_wrapper_t* wrapper,char *data,int bytes,int is_end)
{
	int ret;
	if(is_end)
	{
		wrapper->time_stop=time_get_ms();
   	}
	if(wrapper->cfg->use_mt)
	{
		qtk_decoder_wrapper_flush_reced_feature(wrapper);//TODO ???
	}
	//wrapper->wav_bytes+=bytes;
	ret=wtk_fextra_feed2(wrapper->parm,data,bytes,is_end);

	if(wrapper->ebnfdec2 && wrapper->env.use_ebnfdec)
	{
		ret=wtk_ebnfdec2_feed(wrapper->ebnfdec2,data,bytes,is_end);
		//if(ret!=0){goto end;}
	}
/*	if(!wrapper->cfg->use_mt)
	{
		q=wrapper->parm->output_queue;
		while(q->length>0)
		{
			n=wtk_queue_pop(q);
			if(!n){break;}
			f=data_offset(n,wtk_feat_t,queue_n);
			qtk_kwfstdec_feed(wrapper->dec,f);
			wtk_feat_push_back(f);
		}

		if(is_end)
		{
			qtk_kwfstdec_get_result(wrapper->dec,wrapper->res_buf);
			//wtk_wfstdec_feed_end(d);// GET PATH
		}
	}else{
		if(is_end)
		{
			qtk_decoder_wrapper_mt_feed_end(wrapper,1);
		}//else
	//	{
	//		qtk_decoder_wrapper_flush_reced_feature(wrapper);
	//	}
		wrapper->start=0;
	}*/
	return ret;
}

int qtk_decoder_wrapper_feed3(qtk_decoder_wrapper_t* wrapper,short *data,int bytes,int is_end)
{
	int ret=0;
	if(is_end)
	{
		wrapper->time_stop=time_get_ms();
   	}
//	if(wrapper->cfg->use_mt)
//	{
//		qtk_decoder_wrapper_flush_reced_feature(wrapper);//TODO ???
//	}
	//wrapper->wav_bytes+=bytes;
	//ret=wtk_fextra_feed2(wrapper->parm,data,bytes,is_end);
	wtk_kxparm_feed(wrapper->kxparm,data,bytes,is_end);
	if(wrapper->ebnfdec2 && wrapper->env.use_ebnfdec)
	{
		ret=wtk_ebnfdec2_feed(wrapper->ebnfdec2,(char*)data,2*bytes,is_end);
		//if(ret!=0){goto end;}
	}
	return ret;
}

int qtk_decoder_wrapper_feed(qtk_decoder_wrapper_t* wrapper,char *data,int bytes,int is_end)
{
	int ret = 0;
	wrapper->wav_bytes+=bytes;
	//wtk_debug("%p %d\n",wrapper->vad,wrapper->env.use_vad);
    if(wrapper->vad && wrapper->env.use_vad)
    {
		if(wrapper->vad->cfg->use_k == 1)
		{
			ret = qtk_decoder_wrapper_feed_kvad(wrapper,data,bytes,is_end);
		}else
		{
        	ret = qtk_decoder_wrapper_feed_vad(wrapper,data,bytes,is_end);
		}
    }else if(wrapper->kxparm)
    {
        qtk_decoder_wrapper_feed3(wrapper,(short*)data,bytes/2,is_end);
    }else
    {
        ret = qtk_decoder_wrapper_feed2(wrapper,data,bytes,is_end);
    }
	return ret;
}

int qtk_decoder_wrapper_feedk(qtk_decoder_wrapper_t* wrapper,short *data,int bytes,int is_end)
{
	int ret = 0;
	wrapper->wav_bytes+=bytes*2;

    ret = qtk_decoder_wrapper_feed3(wrapper,data,bytes,is_end);

	return ret;
}

void qtk_decoder_wrapper_feed_test_feat(qtk_decoder_wrapper_t* wrapper,int is_end)
{
	int i;
	wtk_debug("%d\n",wrapper->cfg->f_cnt);
//	wtk_matrix_print(wrapper->cfg->m);
	for(i=1;i<=wrapper->cfg->f_cnt;i++)
	{
		//qtk_kwfstdec_feed(wrapper->dec,i,wrapper->cfg->m[i]);
	}
	//qtk_kwfstdec_feed(wrapper->dec,f->index,f->rv);

	qtk_kwfstdec_get_result(wrapper->dec,wrapper->res_buf);

}

void qtk_decoder_wrapper_feed_test_feat2(qtk_decoder_wrapper_t* wrapper,wtk_matrix_t *m,int cnt)
{
	int i;
	wtk_debug("%d\n",cnt);
//	wtk_matrix_print(wrapper->cfg->m);
	for(i=1;i<=cnt;i++)
	{
		qtk_kwfstdec_feed2(wrapper->dec,m[i],i);
	}
	//qtk_kwfstdec_feed(wrapper->dec,f->index,f->rv);

	qtk_kwfstdec_get_result(wrapper->dec,wrapper->res_buf);
	wtk_free(m);
}

void qtk_decoder_wrapper_feed_test_feat3(qtk_decoder_wrapper_t* wrapper,wtk_matrix_t *m,int cnt)
{
	int i;
	wtk_feat_t *f;
	wtk_debug("%d\n",cnt);
//	wtk_matrix_print(wrapper->cfg->m);
	for(i=1;i<=cnt;i++)
	{
		//qtk_kwfstdec_feed2(wrapper->dec,m[i],i);
		f=wtk_fextra_pop_feature(wrapper->parm);
		wtk_vector_cpy(m[i], f->v);
	    //f->v = (wtk_vector_t*)m[i];
		qtk_nnet3_run(wrapper->parm->nnet3,f,0);
	}
	qtk_nnet3_run(wrapper->parm->nnet3,NULL,1);
	//qtk_kwfstdec_feed(wrapper->dec,f->index,f->rv);
	//qtk_kwfstdec_get_result(wrapper->dec,wrapper->res_buf);
}

void qtk_decoder_wrapper_reset(qtk_decoder_wrapper_t* wrapper)
{	
	wtk_cfg_file_reset(wrapper->env_parser);
    wtk_queue_init(&(wrapper->vad_q));
    wrapper->last_vframe_state=wtk_vframe_sil;
	wrapper->data_left=0;
	if(wrapper->ebnfdec2 && wrapper->env.use_ebnfdec)
	{
		wtk_ebnfdec2_reset(wrapper->ebnfdec2);
	}
	if(wrapper->cfg->use_mt)
	{
		//if(wrapper->start)
		//{
			//qtk_decoder_wrapper_mt_feed_end(wrapper,1);
		//	qtk_decoder_wrapper_flush_reced_feature(wrapper);
		//}
		//for(i=0;i<wrapper->cfg->asr_route;i++)
		//{
			qtk_decoder_wrapper_route_reset(wrapper->asr[0]);
			if(wrapper->cfg->asr_route==2 && wrapper->env.use_dec2)
				qtk_decoder_wrapper_route_reset(wrapper->asr[1]);
		//}
	}else
	{
		wtk_strbuf_reset(wrapper->res_buf);
		wtk_strbuf_reset(wrapper->hint_buf);
        if(wrapper->dec){
        	if (wrapper->dec->cfg->use_eval)
        		qtk_kwfstdec_reset3(wrapper->dec);
        	else
        		qtk_kwfstdec_reset(wrapper->dec);
        }
        else
            qtk_kwfstdec_lite_reset(wrapper->dec_lite);

	}
    if(wrapper->vad && wrapper->env.use_vad)
    {
        wtk_vad_reset(wrapper->vad);
//		wtk_strbuf_reset(wrapper->vad_buf);
//		wtk_strbuf_reset(wrapper->whole_buf);
    }
	if(wrapper->xvprint)
	{
//		wtk_strbuf_reset(wrapper->person_buf);
		wtk_xvprint_reset(wrapper->xvprint);
	}
	wtk_wfstenv_cfg_init2(&(wrapper->env));
	wrapper->time_stop=0;
	wrapper->wav_bytes=0;
    wrapper->rec_wav_bytes=0;
	wrapper->index=0;
//	wrapper->sil=1;
	if(wrapper->kxparm)
	{
		wtk_kxparm_reset(wrapper->kxparm);
	}else
	{
		wtk_fextra_reset(wrapper->parm);
	}
	if(wrapper->chnlike!=NULL)
	{
		wtk_chnlike_reset(wrapper->chnlike);
	}
	if(wrapper->xbnf)
	{
		wtk_xbnf_rec_reset(wrapper->xbnf);
	}
	wtk_queue_init(&(wrapper->parm_q));
	wtk_json_reset(wrapper->json);
	wtk_strbuf_reset(wrapper->json_buf);
}

void qtk_decoder_wrapper_delete(qtk_decoder_wrapper_t* wrapper)
{
	wtk_cfg_file_delete(wrapper->env_parser);
	if(wrapper->ebnfdec2)
	{
		wtk_ebnfdec2_delete(wrapper->ebnfdec2);
	}
	if(wrapper->cfg->use_mt)
	{
		int i;
		for(i=0;i<wrapper->cfg->asr_route;i++)
		{
			qtk_decoder_wrapper_route_delete(wrapper->asr[i]);
		}
		wtk_blockqueue_clean(&(wrapper->feature_bak_q));
		wtk_free(wrapper->asr);
	}else
	{
		wtk_strbuf_delete(wrapper->res_buf);
		wtk_strbuf_delete(wrapper->hint_buf);
        if(wrapper->dec)
        {
        	if (wrapper->dec->cfg->use_eval)
        		qtk_kwfstdec_delete3(wrapper->dec);
        	else
        		qtk_kwfstdec_delete(wrapper->dec);
        }
        else
            qtk_kwfstdec_lite_delete(wrapper->dec_lite);

	}
    if(wrapper->vad)
    {
        wtk_vad_delete(wrapper->vad);
		wtk_strbuf_delete(wrapper->vad_buf);
		wtk_strbuf_delete(wrapper->whole_buf);
    }
	if(wrapper->xvprint)
	{
		wtk_strbuf_delete(wrapper->person_buf);
		wtk_xvprint_delete(wrapper->xvprint);
	}
	if(wrapper->lex)
	{
		wtk_lex_delete(wrapper->lex);
	}

	wtk_json_delete(wrapper->json);
	wtk_strbuf_delete(wrapper->json_buf);
	if(wrapper->parm)
	{
		wtk_fextra_delete(wrapper->parm);
	}
	if(wrapper->kxparm)
	{
		wtk_kxparm_delete(wrapper->kxparm);
	}
	if(wrapper->chnlike)
	{
		wtk_chnlike_delete(wrapper->chnlike);
	}
    if(wrapper->vf)
    {
    	wtk_vecf_delete(wrapper->vf);
    }
	if(wrapper->xbnf)
	{
		wtk_xbnf_rec_delete(wrapper->xbnf);
	}

	wtk_free(wrapper);
}

//int qtk_decoder_wrapper_send_back_reced_feature(qtk_decoder_wrapper_t *warpper,wtk_feat_t *f)
//{
//	//wtk_debug("get feature %d use=%d free=%d\n",f->index,d->parm->feature_hoard.use_length,d->parm->feature_hoard.cur_free);
//	wtk_blockqueue_push(&(warpper->feature_bak_q),&(f->queue_n));
//	return 0;
//}

void qtk_decoder_wrapper_flush_reced_feature(qtk_decoder_wrapper_t *warpper)
{
	wtk_blockqueue_t *q=&(warpper->feature_bak_q);
	wtk_queue_node_t *qn;
	wtk_feat_t *f,*fp;
	int dec;

	dec=warpper->parm->cfg->use_dnn&&warpper->parm->cfg->dnn.use_mlat;
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

void qtk_decoder_wrapper_send_back_feature(qtk_decoder_wrapper_t *warpper,wtk_feat_t *f)
{
	wtk_feat_t *f2;

	//wtk_debug("%p=%d:%d\n",f,f->index,f->used);
	if(warpper->cfg->use_mt)
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

void qtk_decoder_wrapper_mt_start(qtk_decoder_route_t *route)
{
	qtk_decoder_evt_t *evt;

	//d->state=WTK_FST_DEC_WAV_START;
	//wtk_blockqueue_wake(&(d->rec_input_q));
	evt=qtk_decoder_wrapper_pop_evt(route);
	evt->type=QTK_DECODER_EVT_START;
	//wtk_debug("push evt=%d\n",evt->type);
	wtk_blockqueue_push(&(route->rec_input_q),&(evt->q_n));
	wtk_sem_acquire(&(route->rec_start_wait_sem),-1);
}

void qtk_decoder_wrapper_mt_feed_end(qtk_decoder_route_t* route,int wait_eof)
{
	qtk_decoder_evt_t *evt;

	//d->state=WTK_FST_DEC_WAV_END;
	//wtk_blockqueue_wake(&(d->rec_input_q));
	evt=qtk_decoder_wrapper_pop_evt(route);
	evt->type=QTK_DECODER_EVT_END;
	wtk_blockqueue_push(&(route->rec_input_q),&(evt->q_n));
	if(wait_eof)
	{
		qtk_decoder_wrapper_wait_end(route,-1);
	}
}

int qtk_decoder_wrapper_wait_end(qtk_decoder_route_t* route,int timeout)
{
	int ret;

	ret=wtk_sem_acquire(&(route->rec_wait_sem),timeout);
	if(ret==0)
	{
		//qtk_decoder_wrapper_flush_reced_feature(route);TODO
	}
	return ret;
}

void qtk_decoder_wrapper_stop_rec(qtk_decoder_route_t* route)
{
	//d->state=WTK_FST_DEC_DIE;
	route->run=0;
	wtk_blockqueue_wake(&(route->rec_input_q));
	wtk_thread_join(&(route->thread));
}

void qtk_decoder_wrapper_notify_feature(qtk_decoder_wrapper_t *wrapper,wtk_feat_t *f)
{
//	int i;

//	for(i=0;i<wrapper->cfg->asr_route;i++)
//	{
//		qtk_decoder_wrapper_nnet3_feed_route(wrapper->asr[i],f->dnn_v,0);
//	}
}

int qtk_decoder_wrapper_run_rec(qtk_decoder_route_t* route,wtk_thread_t *t)
{
	qtk_decoder_evt_t *evt;
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
		evt=data_offset(qn,	qtk_decoder_evt_t,q_n);
		//wtk_debug("rec=%d pend=%d\n",d->rec->frame,q->length);

		switch(evt->type)
		{
		case QTK_DECODER_EVT_START:
			//time=time_get_cpu();
			route->time_start = time_get_ms();
	        if(route->dec)
    	        qtk_kwfstdec_start(route->dec);
    	    else
        	    qtk_kwfstdec_lite_start(route->dec_lite);

			//wtk_fst_rec_start(d->rec,d->net);
			//wtk_debug("next\n");
			wtk_sem_release(&(route->rec_start_wait_sem),1);
			break;
		case QTK_DECODER_EVT_FEED:
			//if(evt->v.f->index%2==1)
//			if(wrapper->cfg->extra.use_nnet3)
//			{	
        	if(route->dec)
				qtk_kwfstdec_feed2(route->dec,evt->out,evt->index);
       	 	else
				qtk_kwfstdec_lite_feed2(route->dec_lite,evt->out,evt->index);
	
			//	qtk_blas_matrix_delete(blas);
			//	blas=NULL;
//				wtk_free(evt->out);
//
//			}else
//			{
//				//wtk_debug("input=%d\n",q->length);
//				qtk_wrapper_feed_feature(route,evt->f);
//			}

			break;
		case QTK_DECODER_EVT_END:
	        if(route->dec)
				qtk_kwfstdec_get_result(route->dec,route->res_buf);
        	else
				qtk_kwfstdec_lite_get_result(route->dec_lite,route->res_buf);
			//qtk_kwfstdec_get_fa(route->dec,route->res_buf);
			//wrapper->time_rec=time_get_cpu()-time;
			//wtk_debug("%.*s\n",route->res_buf->pos,route->res_buf->data);
			wtk_sem_release(&(route->rec_wait_sem),1);
			break;
		}
		qtk_decoder_wrapper_push_evt(route,evt);
	}
	return 0;
}

void qtk_decoder_wrapper_attach_ebnfdec(qtk_decoder_wrapper_t *wrapper,wtk_json_t *json,wtk_json_item_t *item,wtk_strbuf_t* inbuf,float conf)
{
	//wtk_json_t *json=output->json;
	wtk_wfstr_t *rec;
	float f;
	wtk_json_item_t *ti;
	wtk_strbuf_t *buf;

	ti=wtk_json_new_object(json);
	if(inbuf==NULL)
	{
		rec=wrapper->ebnfdec2->dec->rec;
		buf=wtk_strbuf_new(256,1);
		f=wtk_wfstr_get_conf(rec);
		wtk_json_obj_add_ref_number_s(json,ti,"conf",f);
		wtk_wfstr_finish2(rec,buf," ",1);
	}else
	{
		buf=inbuf;
		wtk_json_obj_add_ref_number_s(json,ti,"conf",conf);
	}
	wtk_json_obj_add_str2_s(json,ti,"rec",buf->data,buf->pos);
	wtk_json_obj_add_item2_s(json,item,"usrec",ti);
	if(inbuf==NULL)
	{
		wtk_strbuf_delete(buf);
	}
}

void qtk_decoder_wrapper_get_result(qtk_decoder_wrapper_t *wrapper,wtk_string_t *v)
{
	wtk_json_item_t *item,*item2,*obj=0;
	wtk_json_t *json=wrapper->json;
	wtk_strbuf_t *buf=wrapper->json_buf;
	double t,f;
	//float strlike=0.0;
	//float best_final,conf;

	item=wtk_json_new_object(json);
	item2=wtk_json_new_object(json);
	wtk_json_obj_add_ref_str_s(json,item,"version",&(wrapper->cfg->version.ver));
	wtk_json_obj_add_ref_str_s(json,item,"res",&(wrapper->cfg->res));
	if(wrapper->res_buf)
	{
		wtk_json_obj_add_str2_s(json,item,"rec",wrapper->res_buf->data,wrapper->res_buf->pos);
		wtk_json_obj_add_ref_number_s(json,item,"conf",wrapper->dec->conf);
	}else
	{
		//wtk_json_obj_add_str2_s(json,item,"rec",wrapper->asr[0]->res_buf->data,wrapper->asr[0]->res_buf->pos);
		if(wrapper->cfg->asr_route==2 && wrapper->env.use_dec2)
		{
			//conf=wrapper->asr[1]->dec->conf;
			//best_final=wrapper->asr[1]->dec->best_final_cost;
			//wtk_json_obj_add_str2_s(json,item,"rec2",wrapper->asr[1]->res_buf->data,wrapper->asr[1]->res_buf->pos);
			//wtk_json_obj_add_ref_number_s(json,item,"conf2",wrapper->asr[1]->dec->conf);
			//strlike=wtk_chnlike_like(wrapper->chnlike,wrapper->asr[0]->res_buf->data,wrapper->asr[0]->res_buf->pos,
			//	wrapper->asr[1]->res_buf->data,wrapper->asr[1]->res_buf->pos,NULL);
			if(wrapper->cfg->use_lite)
			{
				wtk_json_obj_add_str2_s(json,item,"rec",wrapper->asr[0]->res_buf->data,wrapper->asr[0]->res_buf->pos);
				wtk_json_obj_add_ref_number_s(json,item,"conf",wrapper->asr[0]->dec_lite->conf);
			}else
			{
				wtk_json_obj_add_str2_s(json,item,"rec",wrapper->asr[0]->res_buf->data,wrapper->asr[0]->res_buf->pos);
				wtk_json_obj_add_ref_number_s(json,item,"conf",wrapper->asr[0]->dec->conf);
			}
			qtk_decoder_wrapper_attach_ebnfdec(wrapper,json,item,wrapper->asr[1]->res_buf,wrapper->asr[1]->dec_lite->conf);
		}else
		{
			wtk_string_t v2;
			if(wrapper->vad_buf && wrapper->vad_buf->pos !=0)
			{
				wtk_json_obj_add_str2_s(json,item,"rec",wrapper->vad_buf->data,wrapper->vad_buf->pos);
				wtk_strbuf_reset(wrapper->vad_buf);
			}else{
				if(wrapper->lex)
				{
					v2=wtk_lex_process(wrapper->lex,wrapper->asr[0]->res_buf->data,wrapper->asr[0]->res_buf->pos);
					wtk_json_obj_add_str2_s(json,item,"rec",v2.data,v2.len);
					wtk_lex_reset(wrapper->lex);
				}else
				{
					wtk_json_obj_add_str2_s(json,item,"rec",wrapper->asr[0]->res_buf->data,wrapper->asr[0]->res_buf->pos);
				}
				if(wrapper->xbnf)
				{
					wtk_xbnf_rec_process(wrapper->xbnf,wrapper->asr[0]->res_buf->data,wrapper->asr[0]->res_buf->pos);
		     		obj=wtk_xbnf_rec_get_json(wrapper->xbnf,json);
        			if(item)
        			{
            			wtk_json_obj_add_item2_s(json,item,"sem",obj);
        			}
				}
			}
			if(wrapper->asr[0]->dec)
				wtk_json_obj_add_ref_number_s(json,item,"conf",wrapper->asr[0]->dec->conf);
			else
				wtk_json_obj_add_ref_number_s(json,item,"conf",wrapper->asr[0]->dec_lite->conf);
		}
		t=time_get_ms();
		f=t-wrapper->asr[0]->time_start;
		wtk_json_obj_add_ref_number_s(json,item2,"ses",(int)f);
	}
	t=1000.0/(2*wtk_fextra_cfg_get_sample_rate(&(wrapper->cfg->extra)));
	f=wrapper->wav_bytes*t;
	wtk_json_obj_add_ref_number_s(json,item2,"wav",(int)f);
	if(wrapper->vad && wrapper->env.use_vad)
	{
		f=wrapper->rec_wav_bytes*t;
	}
	wtk_json_obj_add_ref_number_s(json,item2,"vad",(int)f);
	wtk_json_obj_add_ref_number_s(json,item2,"sys",0);

	t=time_get_ms();
	f=t-wrapper->time_stop;
	wtk_json_obj_add_ref_number_s(json,item2,"dly",(int)f);
	wtk_json_obj_add_ref_number_s(json,item2,"dfm",0);
	wtk_json_obj_add_item2_s(json,item,"time",item2);
	if(wrapper->ebnfdec2 && wrapper->env.use_ebnfdec)
	{
		qtk_decoder_wrapper_attach_ebnfdec(wrapper,json,item,NULL,0.0);
	}	

	wtk_json_item_print(item,buf);
	wtk_string_set(v,buf->data,buf->pos);
}


int qtk_decoder_wrapper_get_result2(qtk_decoder_wrapper_t *wrapper,wtk_string_t *v)
{
	wtk_string_set(v,wrapper->res_buf->data,wrapper->res_buf->pos);
	//wtk_debug("%f %.*s\n",wrapper->dec_lite->conf,wrapper->res_buf->pos,wrapper->res_buf->data);
	if(wrapper->res_buf->pos>0 && wrapper->dec_lite->conf > 1.15)
	{
		return 1;
	}
	return 0;
}

void qtk_decoder_wrapper_get_hint_result(qtk_decoder_wrapper_t *wrapper,wtk_string_t *v)
{
	wtk_strbuf_t *tmp_rec;
	wtk_strbuf_t *hint=wrapper->asr[0]->hint_buf;
	wtk_string_t v2;

	tmp_rec=wtk_strbuf_new(256,1);
	wtk_strbuf_reset(hint);
	wtk_strbuf_push_s(hint,"{\"hint\":\"");
	if(wrapper->asr[0]->dec)
		qtk_kwfstdec_get_hint_result(wrapper->asr[0]->dec,tmp_rec);
	else
		qtk_kwfstdec_lite_get_result(wrapper->asr[0]->dec_lite,tmp_rec);
	if(wrapper->vad_buf && wrapper->vad_buf->pos!=0)
	{
		wtk_strbuf_push(hint,wrapper->vad_buf->data,wrapper->vad_buf->pos);
	}
	if(wrapper->lex)
	{
    	v2=wtk_lex_process(wrapper->lex,tmp_rec->data,tmp_rec->pos);
		wtk_strbuf_push(hint,v2.data,v2.len);
		wtk_lex_reset(wrapper->lex);	
	}else
	{
		wtk_strbuf_push(hint,tmp_rec->data,tmp_rec->pos);
	}
	wtk_strbuf_push_s(hint,"\"}");
	wtk_string_set(v,hint->data,hint->pos);

	wtk_strbuf_delete(tmp_rec);
}

void qtk_decoder_wrapper_get_vad_result(qtk_decoder_wrapper_t *wrapper,wtk_string_t *v)
{
	wtk_strbuf_t *tmp_rec;
	wtk_strbuf_t *hint=wrapper->vad_buf;

	tmp_rec=wtk_strbuf_new(256,1);
	wtk_strbuf_push_s(tmp_rec,"{\"vad\":\"");
	wtk_strbuf_push(tmp_rec,hint->data,hint->pos);
	wtk_strbuf_push_s(tmp_rec,"\"}");
	wtk_string_set(v,tmp_rec->data,tmp_rec->pos);

	wtk_strbuf_reset(hint);
	wtk_strbuf_delete(tmp_rec);
}

void qtk_decoder_wrapper_get_result_with_name(qtk_decoder_wrapper_t *wrapper,wtk_string_t *v)
{
	wtk_strbuf_t *tmp_rec;
	wtk_strbuf_t *hint=wrapper->vad_buf;
	wtk_strbuf_t *hint2=wrapper->person_buf;

	tmp_rec=wtk_strbuf_new(256,1);
	wtk_strbuf_push_s(tmp_rec,"{\"asr\":\"");
	wtk_strbuf_push(tmp_rec,hint->data,hint->pos);

	//printf(" %.*s\n",hint->pos,hint->data);
	//printf(" %.*s\n",hint2->pos,hint2->data);
	if(wrapper->xvprint)
	{
		wtk_strbuf_push_s(tmp_rec,"\"");
		wtk_strbuf_push_s(tmp_rec,"\"name\":\"");
		wtk_strbuf_push(tmp_rec,hint2->data,hint2->pos);
	}

	wtk_strbuf_push_s(tmp_rec,"\"}");
	wtk_string_set(v,tmp_rec->data,tmp_rec->pos);
    printf(" %.*s\n",v->len,v->data);

	//wtk_strbuf_reset(hint);
	wtk_strbuf_delete(tmp_rec);
}

int qtk_decoder_wrapper_read_xvector_vec(qtk_decoder_wrapper_t *wrapper,wtk_source_t *src)
{
    wtk_strbuf_t *buf;
    int ret;
    wtk_vecf_t *vf;
    buf=wtk_strbuf_new(1024,1.0f);

    vf=wtk_knn_read_vector(src,buf);
#ifdef USE_NEON
    wtk_vecf_t *m;
    m=wtk_neon_math_mat_transf_8float(vf);
    wtk_vecf_delete(vf);
    vf=m;
#endif
    if(!vf)
    {
        wtk_debug("read xvector vec failed\n");
		ret=-1;goto end;
    }

    ret=0;
    wrapper->kxparm->nnet3->ivector_dim = vf->len;
    wrapper->kxparm->nnet3->ivector = vf->p;
    if(wrapper->vf)
    {
    	wtk_vecf_delete(wrapper->vf);
    }
    wrapper->vf = vf;
end:
    wtk_strbuf_delete(buf);
    return ret;
}

int qtk_decoder_wrapper_set_xvector(qtk_decoder_wrapper_t *wrapper, char *fn)
{
	int ret=0;
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;

	if(wrapper->kxparm && wrapper->cfg->parm.nnet3.use_xvector)
	{
		if(fn)
		{
			ret=wtk_source_loader_load(&sl,wrapper,(wtk_source_load_handler_t)qtk_decoder_wrapper_read_xvector_vec,fn);
		}
	}else
	{
		ret = -1;
	}
	return ret;
}

void qtk_decoder_wrapper_set_xbnf(qtk_decoder_wrapper_t *wrapper,char* buf,int len)
{
	if(wrapper->xbnf)
	{
		wtk_xbnf_reset(wrapper->cfg->xbnf.xb);
		wtk_xbnf_compile(wrapper->cfg->xbnf.xb,buf,len);
	}
}

float qtk_decoder_wrapper_get_conf(qtk_decoder_wrapper_t *wrapper)
{
	float conf = 0.0;
	if(wrapper->dec){
		conf = wrapper->dec->conf;
	}else if(wrapper->dec_lite){
		conf = wrapper->dec_lite->conf;
	}else if(wrapper->asr[0]->dec){
		conf = wrapper->asr[0]->dec->conf;
	}else {
		conf = wrapper->asr[0]->dec_lite->conf;
	}
	return conf;
}


int qtk_decoder_wrapper_get_time(qtk_decoder_wrapper_t * wrapper,float *fs,float *fe)
{
	double t,f;

	if (wrapper->cfg->parm.use_htk) {
		t = 1.0 / (2 * wtk_fextra_cfg_get_sample_rate(
							&(wrapper->cfg->parm.htk)));
	} else {
		t = 1.0 / (2 * wrapper->cfg->parm.parm.rate);
	}

	f=wrapper->wav_bytes*t;

	if(f - 0.2 > 0.2)
	{
		*fe = f - 0.2;
		*fs = 0.2;
	}else
	{
		*fs = 0.0;
		*fe = f;
	}

	return 1;
}

float qtk_decoder_wrapper_set_vadindex(qtk_decoder_wrapper_t * wrapper, int index){
	if(wrapper->dec){
		return qtk_kwfstdec_set_vadindex(wrapper->dec,index);
	}else if (wrapper->asr[0]){
		return qtk_kwfstdec_set_vadindex(wrapper->asr[0]->dec,index);
	}
	return 0.0;
}
