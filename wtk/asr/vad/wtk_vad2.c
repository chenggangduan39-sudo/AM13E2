#include "wtk_vad2.h" 


int wtk_vad2_bytes(wtk_vad2_t *vad)
{
	int bytes;

	bytes=sizeof(wtk_vad2_t);
	//wtk_debug("len=%d\n",vad->q.length);
	bytes+=wtk_vad_bytes(vad->vad);
	bytes+=wtk_strbuf_bytes(vad->buf);
	return bytes;
}

wtk_vad2_t* wtk_vad2_new(wtk_vad_cfg_t *cfg)
{
	wtk_vad2_t *vad;

	vad=(wtk_vad2_t*)wtk_malloc(sizeof(wtk_vad2_t));
	wtk_queue_init(&(vad->q));
	vad->vad=wtk_vad_new(cfg,&(vad->q));
	vad->buf=wtk_strbuf_new(1024,1);
	vad->sil=1;
	vad->notify=NULL;
	vad->ths=NULL;
	vad->use_vad_start=0;
	vad->max_cache=5*60*16000*2;
	vad->float_ths=NULL;
	vad->float_notify=NULL;
	wtk_vad2_reset(vad);
	return vad;
}

void wtk_vad2_delete(wtk_vad2_t *vad)
{
	wtk_vad_delete(vad->vad);
	wtk_strbuf_delete(vad->buf);
	wtk_free(vad);
}

void wtk_vad2_start(wtk_vad2_t *vad)
{
	wtk_vad_start(vad->vad);
}

void wtk_vad2_reset_route(wtk_vad2_t *vad)
{
	vad->sil=1;
	vad->vad_pos=0;
	vad->nspeech=0;
	wtk_vad_reset(vad->vad);
	wtk_strbuf_reset(vad->buf);
}

void wtk_vad2_reset(wtk_vad2_t *vad)
{
	vad->sil=1;
	vad->lst_sil=0;
	vad->lst_speech=0;
	vad->lst_speech2=0;
	vad->counter=0;
	vad->vad_pos=0;
	vad->nspeech=0;
	vad->output=0;
	wtk_vad_reset(vad->vad);
	wtk_strbuf_reset(vad->buf);
}

void wtk_vad2_set_notify(wtk_vad2_t *vad,void *ths,wtk_vad2_notify_f notify)
{
	vad->ths=ths;
	vad->notify=notify;
}

void wtk_vad2_set_float_notify(wtk_vad2_t *vad,void *ths,wtk_vad2_float_notify_f notify)
{
	vad->float_ths=ths;
	vad->float_notify=notify;
}

void wtk_vad2_feed_normal(wtk_vad2_t *vad,char *data,int len,int is_end)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *vf=NULL;
	wtk_queue_t *q=vad->vad->output_queue;
	//int frame_step=wtk_vad_get_frame_step(vad->vad);
	int nsil,nspeech;
	int n,idx;
	wtk_strbuf_t *buf=vad->buf;

	wtk_strbuf_push(buf,data,len);
	wtk_vad_feed(vad->vad,data,len,is_end);
	nsil=nspeech=0;
	while(1)
	{
		qn=wtk_queue_pop(q);
		if(!qn){break;}
		vf=data_offset2(qn,wtk_vframe_t,q_n);
		vad->counter+=vf->frame_step;
		//wtk_debug("v[%d]=%s/%s %ld/%d\n",vf->index,vf->state==wtk_vframe_sil?"sil":"speech",vf->raw_state==wtk_vframe_sil?"sil":"speech",vad->counter,vf->frame_step);
		if(vad->sil)
		{
			if(vf->state!=wtk_vframe_sil)
			{
				if(nsil>0)
				{
					vad->notify(vad->ths,WTK_VAD2_DATA,(short*)(buf->data),nsil);
					wtk_strbuf_pop(buf,NULL,nsil<<1);
				}
				vad->lst_speech=vad->counter-vf->frame_step;
				idx=wtk_vad_get_output_left_sil_margin(vad->vad);
				n=idx+(vf->raw_state==wtk_vframe_sil);
				vad->lst_speech2=vad->lst_speech+n*vf->frame_step;
				vad->notify(vad->ths,WTK_VAD2_START,NULL,0);
				vad->sil=0;
				nsil=0;
				nspeech=vf->frame_step;
			}else
			{
				nsil+=vf->frame_step;
			}
		}else
		{
//			if(vf->state!=wtk_vframe_speech)
			if(vf->state==wtk_vframe_sil)
			{
				if(vf->state==wtk_vframe_sil)
				{
					vad->lst_sil=vad->counter-vf->frame_step;
					nsil=vf->frame_step;
				}else
				{
					nspeech+=vf->frame_step;
					nsil=0;
					vad->lst_sil=vad->counter;
				}
				if(nspeech>0)
				{
					vad->notify(vad->ths,WTK_VAD2_DATA,(short*)(buf->data),nspeech);
					wtk_strbuf_pop(buf,NULL,nspeech<<1);
					nspeech=0;
				}
				vad->sil=1;
				vad->notify(vad->ths,WTK_VAD2_END,NULL,0);
			}else
			{
				nspeech+=vf->frame_step;
			}
		}
		wtk_vad_push_vframe(vad->vad,vf);
	}
	if(nsil>0)
	{
		vad->notify(vad->ths,WTK_VAD2_DATA,(short*)(buf->data),nsil);
		wtk_strbuf_pop(buf,NULL,nsil<<1);
	}
	if(nspeech>0)
	{
		vad->notify(vad->ths,WTK_VAD2_DATA,(short*)(buf->data),nspeech);
		wtk_strbuf_pop(buf,NULL,nspeech<<1);
	}
	if(is_end)
	{
		//wtk_debug("buf=%d\n",buf->pos);
		if(buf->pos>0)
		{
			vad->notify(vad->ths,WTK_VAD2_DATA,(short*)(buf->data),buf->pos/2);
			vad->counter+=buf->pos/2;
			wtk_strbuf_reset(buf);
		}
		if(vad->sil==0)
		{
			vad->lst_sil=vad->counter;
			vad->notify(vad->ths,WTK_VAD2_END,NULL,0);
			vad->sil=1;
		}
	}
}

void wtk_vad2_feed_quick(wtk_vad2_t *vad,char *data,int len,int is_end)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *vf=NULL;
	wtk_queue_t *q=vad->vad->output_queue;
	//int frame_step=wtk_vad_get_frame_step(vad->vad);
	int nsil;
	int n,idx;
	wtk_strbuf_t *buf=vad->buf;

//	wtk_debug("len=%d time=%f len=%f cur=%f\n",len,ki*1.0/32000,vad->vad->route.dnnvad->parm->n_frame_index*vad->vad->route.dnnvad->parm->cfg->frame_dur,
//			vad->counter*1.0/16000);
//	{
//		float df;
//
//		df=vad->vad->route.dnnvad->parm->n_frame_index*vad->vad->route.dnnvad->parm->cfg->frame_dur-vad->counter*1.0/16000;
//		if(df>0.7)
//		{
//			wtk_debug("frame=%d/%d dur=%f\n",vad->vad->route.dnnvad->parm->n_frame_index,ni,vad->vad->route.dnnvad->parm->cfg->frame_dur);
//			wtk_vad_print_pend(vad->vad);
//			exit(0);
//		}
//	}
	if(vad->sil==0)
	{
		if(len>0)
		{
			n=len/2;
			vad->notify(vad->ths,WTK_VAD2_DATA,(short*)(data),n);
			vad->vad_pos+=n;
			//wtk_debug("pos=%d\n",vad->vad_pos);
		}
	}
	wtk_strbuf_push(buf,data,len);
	wtk_vad_feed(vad->vad,data,len,is_end);
	nsil=0;
	while(1)
	{
		qn=wtk_queue_pop(q);
		if(!qn){break;}
		vf=data_offset2(qn,wtk_vframe_t,q_n);
		//wtk_debug("v[%d]=%s state=%d/%d [%d,%d]\n",vf->index,vf->state==wtk_vframe_sil?"sil":"speech",vf->state,vf->raw_state,vad->vad->left_margin,vad->vad->right_margin);
		//wtk_debug("v[%d]=%s/%s at %f\n",vf->index,vf->state==wtk_vframe_sil?"sil":"speech",vf->raw_state==wtk_vframe_sil?"sil":"speech",vad->counter*1.0/16000);
		vad->counter+=vf->frame_step;
		if(vad->sil)
		{
			if(vf->state!=wtk_vframe_sil)
			{
				if(nsil>0)
				{
					vad->notify(vad->ths,WTK_VAD2_DATA,(short*)(buf->data),nsil);
					wtk_strbuf_pop(buf,NULL,nsil<<1);
					wtk_strbuf_control_cache(buf,vad->max_cache);
				}
				vad->lst_speech=vad->counter-vf->frame_step;
				idx=wtk_vad_get_output_left_sil_margin(vad->vad);
				n=idx+(vf->raw_state==wtk_vframe_sil);
				vad->lst_speech2=vad->lst_speech+n*vf->frame_step;
				vad->notify(vad->ths,WTK_VAD2_START,NULL,0);
				vad->sil=0;

				n=buf->pos/2;
				vad->notify(vad->ths,WTK_VAD2_DATA,(short*)(buf->data),n);
				vad->vad_pos=n;
				//wtk_debug("pos=%d\n",vad->vad_pos);

				nsil=0;
				vad->nspeech=vf->frame_step;
			}else
			{
				nsil+=vf->frame_step;
			}
		}else
		{
			//if(vf->state!=wtk_vframe_speech)
			if(vf->state==wtk_vframe_sil)
			{
				if(vf->state==wtk_vframe_sil)
				{
					vad->lst_sil=vad->counter-vf->frame_step;
					nsil=vf->frame_step;
				}else
				{
					vad->nspeech+=vf->frame_step;
					nsil=0;
					vad->lst_sil=vad->counter;
				}
				n=vad->nspeech;
				idx=vad->vad_pos-n;
				if(idx>0)
				{
					vad->notify(vad->ths,WTK_VAD2_CANCEL,((short*)buf->data)+n,idx);
				}
				wtk_strbuf_pop(buf,NULL,n<<1);
				wtk_strbuf_control_cache(buf,vad->max_cache);

				vad->sil=1;
				vad->notify(vad->ths,WTK_VAD2_END,NULL,0);
				vad->nspeech=0;
				vad->vad_pos=0;
			}else
			{
				vad->nspeech+=vf->frame_step;
			}
		}
		wtk_vad_push_vframe(vad->vad,vf);
	}
	if(nsil>0)
	{
		vad->notify(vad->ths,WTK_VAD2_DATA,(short*)(buf->data),nsil);
		wtk_strbuf_pop(buf,NULL,nsil<<1);
		wtk_strbuf_control_cache(buf,vad->max_cache);
	}
	if(is_end)
	{
		if(buf->pos>0)
		{
			if(vad->sil)
			{
				vad->notify(vad->ths,WTK_VAD2_DATA,(short*)(buf->data),buf->pos/2);
				vad->counter+=buf->pos/2;
			}else
			{
				if(vad->nspeech>0)
				{
					vad->counter+=buf->pos/2-vad->nspeech;
				}
			}
			wtk_strbuf_reset(buf);
		}
		vad->vad_pos=0;
		if(vad->sil==0)
		{
			vad->lst_sil=vad->counter;
			vad->notify(vad->ths,WTK_VAD2_END,NULL,0);
			vad->sil=1;
		}
	}
}

void wtk_vad2_feed(wtk_vad2_t *vad,char *data,int len,int is_end)
{
#ifdef USE_LOG
	static wtk_wavfile_t *log=NULL;

	//wtk_debug("len=%d end=%d skip=%d\n",len,is_end,wakeup->wakeup->skip_post);
	//if(wakeup->wakeup->skip_post==0)
	{
		if(!log)
		{
			log=wtk_wavfile_new(16000);
			wtk_wavfile_open2(log,"vad");
			log->max_pend=0;
		}
		wtk_wavfile_write(log,data,len);
		if(is_end)
		{
			wtk_debug("============ close ============\n");
			wtk_wavfile_close(log);
			wtk_wavfile_delete(log);
			log=NULL;
		}
	}
#endif
	if(vad->use_vad_start)
	{
		wtk_vad2_feed_quick(vad,data,len,is_end);
	}else
	{
		wtk_vad2_feed_normal(vad,data,len,is_end);
	}
}

void wtk_vad2_feed_normal_float(wtk_vad2_t *vad,float *data,int len,int is_end)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *vf=NULL;
	wtk_queue_t *q=vad->vad->output_queue;
	int nsil,nspeech;
	int n,idx;
	wtk_strbuf_t *buf=vad->buf;

	wtk_strbuf_push(buf,(char *)data,len<<2);
	wtk_vad_feed_float(vad->vad,data,len,is_end);
	nsil=nspeech=0;
	while(1)
	{
		qn=wtk_queue_pop(q);
		if(!qn){break;}
		vf=data_offset2(qn,wtk_vframe_t,q_n);
		vad->counter+=vf->frame_step;
		//wtk_debug("v[%d]=%s/%s %ld/%d\n",vf->index,vf->state==wtk_vframe_sil?"sil":"speech",vf->raw_state==wtk_vframe_sil?"sil":"speech",vad->counter,vf->frame_step);
		if(vad->sil)
		{
			if(vf->state!=wtk_vframe_sil)
			{
				if(nsil>0)
				{
					vad->float_notify(vad->float_ths,WTK_VAD2_DATA,(float*)(buf->data),nsil);
					wtk_strbuf_pop(buf,NULL,nsil<<2);
				}
				vad->lst_speech=vad->counter-vf->frame_step;
				idx=wtk_vad_get_output_left_sil_margin(vad->vad);
				n=idx+(vf->raw_state==wtk_vframe_sil);
				vad->lst_speech2=vad->lst_speech+n*vf->frame_step;
				vad->float_notify(vad->float_ths,WTK_VAD2_START,NULL,0);
				vad->sil=0;
				nsil=0;
				nspeech=vf->frame_step;
			}else
			{
				nsil+=vf->frame_step;
			}
		}else
		{
			//if(vf->state!=wtk_vframe_speech)
			if(vf->state==wtk_vframe_sil)
			{
				if(vf->state==wtk_vframe_sil)
				{
					vad->lst_sil=vad->counter-vf->frame_step;
					nsil=vf->frame_step;
				}else
				{
					nspeech+=vf->frame_step;
					nsil=0;
					vad->lst_sil=vad->counter;
				}
				if(nspeech>0)
				{
					vad->float_notify(vad->float_ths,WTK_VAD2_DATA,(float*)(buf->data),nspeech);
					wtk_strbuf_pop(buf,NULL,nspeech<<2);
					nspeech=0;
				}
				vad->sil=1;
				vad->float_notify(vad->float_ths,WTK_VAD2_END,NULL,0);
			}else
			{
				nspeech+=vf->frame_step;
			}
		}
		wtk_vad_push_vframe(vad->vad,vf);
	}
	if(nsil>0)
	{
		vad->float_notify(vad->float_ths,WTK_VAD2_DATA,(float*)(buf->data),nsil);
		wtk_strbuf_pop(buf,NULL,nsil<<2);
	}
	if(nspeech>0)
	{
		vad->float_notify(vad->float_ths,WTK_VAD2_DATA,(float*)(buf->data),nspeech);
		wtk_strbuf_pop(buf,NULL,nspeech<<2);
	}
	if(is_end)
	{
		//wtk_debug("buf=%d\n",buf->pos);
		if(buf->pos>0)
		{
			vad->float_notify(vad->float_ths,WTK_VAD2_DATA,(float*)(buf->data),buf->pos/4);
			vad->counter+=buf->pos/4;
			wtk_strbuf_reset(buf);
		}
		if(vad->sil==0)
		{
			vad->lst_sil=vad->counter;
			vad->float_notify(vad->float_ths,WTK_VAD2_END,NULL,0);
			vad->sil=1;
		}
	}
}


void wtk_vad2_feed_quick_float(wtk_vad2_t *vad,float *data,int len,int is_end)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *vf=NULL;
	wtk_queue_t *q=vad->vad->output_queue;
	//int frame_step=wtk_vad_get_frame_step(vad->vad);
	int nsil;
	int n,idx;
	wtk_strbuf_t *buf=vad->buf;

	//wtk_debug("+++++++++++++ len=%d end=%d\n",len,is_end);
	vad->counter+=len;
	if(vad->sil==0)
	{
		if(len>0)
		{
			vad->output+=len;
			vad->float_notify(vad->float_ths,WTK_VAD2_DATA,data,len);
			vad->vad_pos+=len;
			//wtk_debug("pos=%d\n",vad->vad_pos);
		}
	}
	wtk_strbuf_push(buf,(char*)data,len*4);
	wtk_vad_feed_float(vad->vad,data,len,is_end);
	nsil=0;
	while(1)
	{
		qn=wtk_queue_pop(q);
		if(!qn){break;}
		vf=data_offset2(qn,wtk_vframe_t,q_n);
		//wtk_debug("v[%d]=%s state=%d/%d [%d,%d]\n",vf->index,vf->state==wtk_vframe_sil?"sil":"speech",vf->state,vf->raw_state,vad->vad->left_margin,vad->vad->right_margin);
		//wtk_debug("v[%d]=%s/%s\n",vf->index,vf->state==wtk_vframe_sil?"sil":"speech",vf->raw_state==wtk_vframe_sil?"sil":"speech");
		if(vad->sil)
		{
			if(vf->state!=wtk_vframe_sil)
			{
				if(nsil>0)
				{
					vad->output+=nsil;
					vad->float_notify(vad->float_ths,WTK_VAD2_DATA,(float*)(buf->data),nsil);
					wtk_strbuf_pop(buf,NULL,nsil*4);
					wtk_strbuf_control_cache(buf,vad->max_cache);
				}
				vad->lst_speech=vad->counter-vf->frame_step;
				idx=wtk_vad_get_output_left_sil_margin(vad->vad);
				n=idx+(vf->raw_state==wtk_vframe_sil);
				vad->lst_speech2=vad->lst_speech+n*vf->frame_step;
				vad->float_notify(vad->float_ths,WTK_VAD2_START,NULL,0);
				vad->sil=0;

				n=buf->pos/4;
				vad->output+=n;
				vad->float_notify(vad->float_ths,WTK_VAD2_DATA,(float*)(buf->data),n);
				vad->vad_pos=n;
				//wtk_debug("pos=%d\n",vad->vad_pos);

				nsil=0;
				vad->nspeech=vf->frame_step;
			}else
			{
				nsil+=vf->frame_step;
			}
		}else
		{
			if(vf->state!=wtk_vframe_speech)
			{
				if(vf->state==wtk_vframe_sil)
				{
					vad->lst_sil=vad->counter-vf->frame_step;
					nsil=vf->frame_step;
				}else
				{
					vad->nspeech+=vf->frame_step;
					nsil=0;
					vad->lst_sil=vad->counter;
				}
				n=vad->nspeech;
				idx=vad->vad_pos-n;
				//wtk_debug("%d/%d\n",vad->vad_pos,n);
				if(idx>0)
				{
					vad->output-=idx;
					vad->float_notify(vad->float_ths,WTK_VAD2_CANCEL,NULL,idx);
				}
				wtk_strbuf_pop(buf,NULL,n*4);
				wtk_strbuf_control_cache(buf,vad->max_cache);

				vad->sil=1;
				vad->float_notify(vad->float_ths,WTK_VAD2_END,NULL,0);
				vad->nspeech=0;
				vad->vad_pos=0;
			}else
			{
				vad->nspeech+=vf->frame_step;
			}
		}
		wtk_vad_push_vframe(vad->vad,vf);
	}
	//wtk_debug("pos=%d\n",buf->pos);
	if(nsil>0)
	{
		vad->output+=nsil;
		//wtk_debug("nsil=%d pos=%d\n",nsil,buf->pos/4);
		//print_float((float*)(buf->data),10);
		vad->float_notify(vad->float_ths,WTK_VAD2_DATA,(float*)(buf->data),nsil);
		wtk_strbuf_pop(buf,NULL,nsil*4);
		wtk_strbuf_control_cache(buf,vad->max_cache);
	}
	if(is_end)
	{
		if(buf->pos>0)
		{
			if(vad->sil)
			{
				vad->output+=buf->pos/4;
				vad->float_notify(vad->float_ths,WTK_VAD2_DATA,(float*)(buf->data),buf->pos/4);
			}
			wtk_strbuf_reset(buf);
		}
		vad->vad_pos=0;
		if(vad->sil==0)
		{
			vad->lst_sil=vad->counter;
			vad->float_notify(vad->float_ths,WTK_VAD2_END,NULL,0);
			vad->sil=1;
		}
	}
}

void wtk_vad2_feed_float(wtk_vad2_t *vad,float *data,int len,int is_end)
{

	if(vad->use_vad_start)
	{
		wtk_vad2_feed_quick_float(vad,data,len,is_end);
	}else
	{
		wtk_vad2_feed_normal_float(vad,data,len,is_end);
	}
}
