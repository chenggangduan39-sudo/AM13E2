#include "qtk_wakeup_wrapper.h"

void qtk_wakeup_on_vad2(qtk_wakeup_wrapper_t *wrapper,wtk_vad2_cmd_t cmd,short *data,int len);

qtk_wakeup_wrapper_t* qtk_wakeup_wrapper_new(qtk_wakeup_wrapper_cfg_t* cfg){
	qtk_wakeup_wrapper_t *wrapper;

	wrapper = (qtk_wakeup_wrapper_t*)wtk_malloc(sizeof(qtk_wakeup_wrapper_t));
	wrapper->cfg=cfg;

	wrapper->idle_time = 10;
	wrapper->idle_bytes = 0;
	wrapper->notify_wakeup = 0;
	wrapper->notify_wakeup_ths = 0;
	wrapper->is_end = 1;
	wrapper->start_frame = 0;
	wrapper->reset_bytes = 0;
	wrapper->waked = 0;
    wtk_queue_init(&(wrapper->vad_q));

	wrapper->vad = 0;
	wrapper->vad2 = 0;

	if(cfg->use_vad2){
		wrapper->vad2 = wtk_vad2_new(&(cfg->vad));
		wtk_vad2_set_notify(wrapper->vad2, wrapper, (wtk_vad2_notify_f)qtk_wakeup_on_vad2);
	}else if(cfg->use_vad){
		wrapper->vad = wtk_vad_new(&(cfg->vad),&(wrapper->vad_q));
		wrapper->frame_bytes = sizeof(short)*wrapper->vad->cfg->dnnvad.parm.frame_step;
		wrapper->last_vframe_state = wtk_vframe_sil;
	}

	switch(cfg->type){
	case QTK_K2:
		qtk_wakeup_wrapper_set_sourcer(wrapper,cast(void* (*)(void*),qtk_k2_dec_new),
				cast(void (*)(void *),qtk_k2_dec_reset),
				cast(void (*)(void *),qtk_k2_dec_delete),
				cast(int (*)(void *),qtk_k2_dec_start),
				cast(int (*)(void *, char *, int, int),qtk_k2_dec_feed),
				cast(int (*)(void *, char *, int),qtk_k2_dec_set_keywords),
				cast(float (*)(void *),qtk_k2_dec_get_conf),
				cast(void (*)(void *,float *fs,float *fe),qtk_k2_dec_get_wake_time));
		wrapper->dec = wrapper->sourcer.qtk_wakeup_new(&(cfg->kwake));
		break;
	case QTK_KWDEC:
		qtk_wakeup_wrapper_set_sourcer(wrapper,cast(void* (*)(void*),wtk_kwdec2_new),
				cast(void (*)(void *),wtk_kwdec2_reset),
				cast(void (*)(void *),wtk_kwdec2_delete),
				cast(int (*)(void *),wtk_kwdec2_start),
				cast(int (*)(void *, char *, int, int),wtk_kwdec2_feed),
				cast(int (*)(void *, char *, int),wtk_kwdec2_set_context),
				cast(float (*)(void *),wtk_kwdec2_get_conf),
				cast(void (*)(void *,float *fs,float *fe),wtk_kwdec2_get_wake_time));
		wrapper->dec = wrapper->sourcer.qtk_wakeup_new(&(cfg->kwdec2));
		break;
	case QTK_IMG:
		qtk_wakeup_wrapper_set_sourcer(wrapper,cast(void* (*)(void*),qtk_img_rec_new),
				cast(void (*)(void *),qtk_img_rec_reset),
				cast(void (*)(void *),qtk_img_rec_delete),
				cast(int (*)(void *),qtk_img_rec_start),
				cast(int (*)(void *, char *, int, int),qtk_img_rec_feed),
				NULL,
				cast(float (*)(void *),qtk_img_rec_get_conf),
				cast(void (*)(void *,float *fs,float *fe),qtk_img_rec_get_time));
				wrapper->dec = wrapper->sourcer.qtk_wakeup_new(&(cfg->img));
		break;
	default:
		break;
	}

	if(wrapper->cfg->wakeup_buf){
		//qtk_asr_wrapper_set_context(wrapper,cfg->wakeup_buf->data,cfg->wakeup_buf->pos);
	}

	return wrapper;
}

int qtk_wakeup_wrapper_start(qtk_wakeup_wrapper_t* wrapper){
	if(wrapper->cfg->use_vad){
		return wtk_vad_start(wrapper->vad);
	}

	if(wrapper->cfg->use_vad2){
		wtk_vad2_start(wrapper->vad2);
		return 0;
	}

	return wrapper->sourcer.qtk_wakeup_start(wrapper->dec);
}

void qtk_wakeup_on_vad2(qtk_wakeup_wrapper_t *wrapper,wtk_vad2_cmd_t cmd,short *data,int len){
	int ret = 0;
	float fs,fe;
	switch (cmd)
	{
	case WTK_VAD2_START:
		wrapper->sourcer.qtk_wakeup_start(wrapper->dec);
		break;
	case WTK_VAD2_DATA:
		ret = wrapper->sourcer.qtk_wakeup_feed(wrapper->dec,(char *)data,len<<1,0);
		if(ret == 1){
			qtk_wakeup_wrapper_get_wake_time(wrapper,&fs,&fe);
			if(wrapper->notify_wakeup){
				wrapper->notify_wakeup(wrapper->notify_wakeup_ths,fs,fe);
			}
		}
		break;
	case WTK_VAD2_END:
		ret = wrapper->sourcer.qtk_wakeup_feed(wrapper->dec,0,0,1);
		if(ret == 1){
			qtk_wakeup_wrapper_get_wake_time(wrapper,&fs,&fe);
			if(wrapper->notify_wakeup){
				wrapper->notify_wakeup(wrapper->notify_wakeup_ths,fs,fe);
			}
		}
		wrapper->sourcer.qtk_wakeup_reset(wrapper->dec);
		break;
	case WTK_VAD2_CANCEL:
		break;
	default:
		break;
	}
}

int _qwakeup_feed_vad(qtk_wakeup_wrapper_t* wrapper,char *data,int bytes,int is_end){
	wtk_vad_t *vad = wrapper->vad;
	wtk_vframe_t *f;
	wtk_queue_node_t *qn;
	int ret;
	float fs,fe;

	wtk_vad_feed(vad,data,bytes,is_end);
	while(1){
		qn = wtk_queue_pop(&(wrapper->vad_q));
		if(!qn){
			break;
		}
		f = data_offset(qn,wtk_vframe_t,q_n);

		switch (wrapper->last_vframe_state)
		{
		case wtk_vframe_sil:
			if(f->state != wtk_vframe_sil){
				wrapper->start_frame = f->index;
				//qtk_asr_wrapper_set_vadindex(dec,f->index);
				wrapper->sourcer.qtk_wakeup_start(wrapper->dec);
				wrapper->last_vframe_state = wtk_vframe_speech;
			}
			break;
		case wtk_vframe_speech:
			if(f->state != wtk_vframe_speech){
				wrapper->last_vframe_state = wtk_vframe_sil;
				ret = wrapper->sourcer.qtk_wakeup_feed(wrapper->dec,0,0,1);
			}else{
				ret = wrapper->sourcer.qtk_wakeup_feed(wrapper->dec,data,bytes,0);
			}

			if(ret == 1){
				qtk_wakeup_wrapper_get_wake_time(wrapper,&fs,&fe);
				if(wrapper->notify_wakeup){
					wrapper->notify_wakeup(wrapper->notify_wakeup_ths,fs,fe);
				}
			}
			wrapper->sourcer.qtk_wakeup_reset(wrapper->dec);
			break;
		case wtk_vframe_speech_end:
			break;
		default:
			break;
		}
		wtk_vad_push_vframe(vad,f);
	}

	if(is_end == 1 && wrapper->is_end == 0){
		ret = wrapper->sourcer.qtk_wakeup_feed(wrapper->dec,0,0,1);
		if(ret == 1){
			qtk_wakeup_wrapper_get_wake_time(wrapper,&fs,&fe);
			if(wrapper->notify_wakeup){
				wrapper->notify_wakeup(wrapper->notify_wakeup_ths,fs,fe);
			}
		}
	}

	return 0;
}

int _qwakeup_feed_vad2(qtk_wakeup_wrapper_t* wrapper,char *data,int bytes,int is_end){
	wtk_vad2_feed(wrapper->vad2, data, bytes, is_end);
	return 0;
}

int qtk_wakeup_wrapper_feed(qtk_wakeup_wrapper_t* wrapper,char *data,int bytes,int is_end){
	int ret = 0;

	if(wrapper->cfg->use_vad){
		ret = _qwakeup_feed_vad(wrapper,data,bytes,is_end);
	}else if(wrapper->cfg->use_vad2){
		ret = _qwakeup_feed_vad2(wrapper,data,bytes,is_end);
	}else{
		ret = wrapper->sourcer.qtk_wakeup_feed(wrapper->dec,data,bytes,is_end);
	}

	return ret;
}

void qtk_wakeup_wrapper_reset(qtk_wakeup_wrapper_t* wrapper){
	wtk_queue_init(&(wrapper->vad_q));
	wrapper->last_vframe_state = wtk_vframe_sil;
	wrapper->is_end = 1;
	wrapper->start_frame = 0;
	wrapper->reset_bytes = 0;
	wrapper->waked = 0;

	if(wrapper->vad){
		wtk_vad_reset(wrapper->vad);
	}

	if(wrapper->vad2){
		wtk_vad2_reset(wrapper->vad2);
	}
	wrapper->sourcer.qtk_wakeup_reset(wrapper->dec);
}

void qtk_wakeup_wrapper_delete(qtk_wakeup_wrapper_t* wrapper){
	if(wrapper->vad){
		wtk_vad_delete(wrapper->vad);
	}
	if(wrapper->vad2){
		wtk_vad2_delete(wrapper->vad2);
	}

	wrapper->sourcer.qtk_wakeup_delete(wrapper->dec);
	wtk_free(wrapper);
}

int qtk_wakeup_wrapper_set_context(qtk_wakeup_wrapper_t *wrapper,char *data,int bytes){

	return -1;
}

int qtk_wakeup_wrapper_set_context_wakeup(qtk_wakeup_wrapper_t *wrapper,char *data,int bytes){
	//if(wrapper->cfg->type == QTK_K2){
	//	return qtk_k2_dec_set_keywords(wrapper->dec,data,bytes);
	//}
	if(wrapper->cfg->type != QTK_IMG){
		return wrapper->sourcer.qtk_wakeup_set_context(wrapper->dec,data,bytes);
	}
	return -1;
}

void qtk_wakeup_wrapper_set_sourcer(qtk_wakeup_wrapper_t *wrapper,
		void* (*qtk_wakeup_new)(void *),
		void (*qtk_wakeup_reset)(void *),
		void (*qtk_wakeup_delete)(void *),
		int (*qtk_wakeup_start)(void *),
		int (*qtk_wakeup_feed)(void *, char *data, int bytes, int is_end),
		int (*qtk_wakeup_set_context)(void *,char *data, int bytes),
		float (*qtk_wakeup_get_conf)(void *),
		void (*qtk_wakeup_get_wake_time)(void *,float *fs, float *fe)
		){
	wrapper->sourcer.qtk_wakeup_new = qtk_wakeup_new;
	wrapper->sourcer.qtk_wakeup_reset = qtk_wakeup_reset;
	wrapper->sourcer.qtk_wakeup_delete = qtk_wakeup_delete;
	wrapper->sourcer.qtk_wakeup_start = qtk_wakeup_start;
	wrapper->sourcer.qtk_wakeup_feed = qtk_wakeup_feed;
	wrapper->sourcer.qtk_wakeup_set_context = qtk_wakeup_set_context;
	wrapper->sourcer.qtk_wakeup_get_conf = qtk_wakeup_get_conf;
	wrapper->sourcer.qtk_wakeup_get_wake_time = qtk_wakeup_get_wake_time;
}

float qtk_wakeup_wrapper_set_vadindex(qtk_wakeup_wrapper_t * wrapper, int index){
	if(wrapper->cfg->type == QTK_K2){
		return qtk_k2_dec_set_vadindex(wrapper->dec,index);
	}
	return 0.0;
}

void qtk_wakeup_wrapper_get_wake_time(qtk_wakeup_wrapper_t *wrapper,float *fs,float *fe){
	wrapper->sourcer.qtk_wakeup_get_wake_time(wrapper->dec,fs,fe);
}

float qtk_wakeup_wrapper_get_conf(qtk_wakeup_wrapper_t *wrapper){
	return wrapper->sourcer.qtk_wakeup_get_conf(wrapper->dec);
}

void qtk_wakeup_wrapper_set_wakeup_notify(qtk_wakeup_wrapper_t *wrapper,void *upval, qtk_wakeup_wrapper_notify_t notify){
	wrapper->notify_wakeup = notify;
	wrapper->notify_wakeup_ths = upval;
}

void qtk_wakeup_wrapper_set_idle_time(qtk_wakeup_wrapper_t *wrapper, long unsigned int val){
	wrapper->idle_time = val;
}
