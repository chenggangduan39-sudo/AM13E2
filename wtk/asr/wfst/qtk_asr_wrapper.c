#include "qtk_asr_wrapper.h"

qtk_asr_wrapper_t* qtk_asr_wrapper_new(qtk_asr_wrapper_cfg_t* cfg)
{
	qtk_asr_wrapper_t *wrapper;

	wrapper=(qtk_asr_wrapper_t*)wtk_malloc(sizeof(*wrapper));
	wrapper->cfg=cfg;

	wrapper->chnlike = NULL;
	wrapper->mode = 0;
	wrapper->idle_time = 10;
	wrapper->idle_bytes = 0;
	wrapper->notify_asr = 0;
	wrapper->notify_asr_ths = 0;
	wrapper->notify_wakeup = 0;
	wrapper->notify_wakeup_ths = 0;
	wrapper->is_end = 1;
	wrapper->start_frame = 0;
	wrapper->reset_bytes = 0;
	wrapper->wav_cache = NULL;
	wrapper->waked = 0;
	wrapper->asr = 0;
	wrapper->asr_start = 0;
    wtk_queue_init(&(wrapper->vad_q));
	wrapper->keywords2 = wtk_strbuf_new(1024,1);
	wrapper->heap = 0;
    if(cfg->use_vad)
    {
        wrapper->vad = wtk_vad_new(&(cfg->vad),&(wrapper->vad_q));
		wrapper->frame_bytes = sizeof(short)*wrapper->vad->cfg->dnnvad.parm.frame_step;
        wrapper->last_vframe_state = wtk_vframe_sil;
    }else
    {
        wrapper->vad=0;
    }

    if(cfg->use_wake)
    {
        wrapper->wakeup=qtk_k2_dec_new(&(cfg->kwake));
		if(cfg->wakeup_buf){
			qtk_k2_dec_set_hotwords(wrapper->wakeup,cfg->wakeup_buf->data,cfg->wakeup_buf->pos);
		}
		wrapper->wav_cache = wtk_strbuf_new(1024,1);
    }else
    {
        wrapper->wakeup=0;
	}

	if(cfg->use_lex)
	{
		wrapper->lex=wtk_lex_new(&(cfg->lex));
		wtk_lex_compile(wrapper->lex,cfg->lex_fn);
	}else
	{
		wrapper->lex=0;
	}

	switch(cfg->type)
	{
	case QTK_K2:
		qtk_asr_wrapper_set_sourcer(wrapper,cast(void* (*)(void*),qtk_k2_dec_new),
				cast(void (*)(void *),qtk_k2_dec_reset),
				cast(void (*)(void *),qtk_k2_dec_delete),
				cast(int (*)(void *),qtk_k2_dec_start),
				cast(int (*)(void *,char *, int),qtk_k2_dec_start2),
				cast(int (*)(void *, char *, int, int),qtk_k2_dec_feed),
				cast(void (*)(void *, wtk_string_t *),qtk_k2_dec_get_result),
				cast(void (*)(void *, wtk_string_t *),qtk_k2_dec_get_hint_result),
				cast(int (*)(void *, char *, int),qtk_k2_dec_set_hotwords));
		wrapper->dec = wrapper->sourcer.qtk_asr_new(&(cfg->k2));
		break;
	case QTK_KALDI:
		qtk_asr_wrapper_set_sourcer(wrapper,cast(void* (*)(void*),qtk_decoder_wrapper_new),
				cast(void (*)(void *),qtk_decoder_wrapper_reset),
				cast(void (*)(void *),qtk_decoder_wrapper_delete),
				cast(int (*)(void *),qtk_decoder_wrapper_start),
				cast(int (*)(void *,char *, int),qtk_decoder_wrapper_start2),
				cast(int (*)(void *, char *, int, int),qtk_decoder_wrapper_feed),
				cast(void (*)(void *, wtk_string_t *),qtk_decoder_wrapper_get_result),
				cast(void (*)(void *, wtk_string_t *),qtk_decoder_wrapper_get_hint_result),
				NULL);
		wrapper->dec = wrapper->sourcer.qtk_asr_new(&(cfg->kaldi));
		break;
	case QTK_WENET:
		qtk_asr_wrapper_set_sourcer(wrapper,cast(void* (*)(void*),qtk_wenet_wrapper_new),
				cast(void (*)(void *),qtk_wenet_wrapper_reset),
				cast(void (*)(void *),qtk_wenet_wrapper_delete),
				cast(int (*)(void *),qtk_wenet_wrapper_start),
				cast(int (*)(void *,char *, int),qtk_wenet_wrapper_start2),
				cast(int (*)(void *, char *, int, int),qtk_wenet_wrapper_feed),
				cast(void (*)(void *, wtk_string_t *),qtk_wenet_wrapper_get_result),
				cast(void (*)(void *, wtk_string_t *),qtk_wenet_wrapper_get_hint_result),
				cast(int (*)(void *, char *, int),qtk_wenet_wrapper_set_context_str));
		wrapper->dec = wrapper->sourcer.qtk_asr_new(&(cfg->wenet));
		break;
	default:
		break;
	}

	if(wrapper->cfg->asr_buf){
		qtk_asr_wrapper_set_context(wrapper,cfg->asr_buf->data,cfg->asr_buf->pos);
	}
	if(wrapper->cfg->use_hc_asr){
		wrapper->heap = wtk_heap_new(4096);
	}

	return wrapper;
}

int qtk_asr_wrapper_start(qtk_asr_wrapper_t* wrapper)
{
	if(wrapper->cfg->use_vad)
	{
        return wtk_vad_start(wrapper->vad);
    }

	return wrapper->sourcer.qtk_asr_start(wrapper->dec);
}

int qtk_asr_wrapper_start2(qtk_asr_wrapper_t* wrapper,char *data,int bytes)
{	
    if(wrapper->cfg->use_vad)
    {   
        wtk_vad_start(wrapper->vad);
    }

    return wrapper->sourcer.qtk_asr_start2(wrapper->dec,data,bytes);
}

static int _qasr_prepare(qtk_asr_wrapper_t *wrapper,char *data,int bytes){
	int ret = 0;
	switch (wrapper->mode)
	{
	case 0://asr + wakeup
		qtk_k2_dec_start(wrapper->wakeup);
		ret = qtk_k2_dec_feed(wrapper->wakeup,data,bytes,0);
		if(wrapper->asr == 1){
			wrapper->sourcer.qtk_asr_start(wrapper->dec);
			wrapper->asr_start = 1;
			ret = wrapper->sourcer.qtk_asr_feed(wrapper->dec,data,bytes,0);
		}

		break;
	case 1://asr
		wrapper->sourcer.qtk_asr_start(wrapper->dec);
		ret = wrapper->sourcer.qtk_asr_feed(wrapper->dec,data,bytes,0);
		break;
	case 2://wakeup
		qtk_k2_dec_start(wrapper->wakeup);
		ret = qtk_k2_dec_feed(wrapper->wakeup,data,bytes,0);
		break;
	default:
		break;
	}
	return ret;
}

static void _qasr_notify_result(qtk_asr_wrapper_t *wrapper,char *data,int bytes){
	if(wrapper->notify_asr){
		wtk_array_t *a = wtk_str_to_array(wrapper->heap, data, bytes, ' ');
		wtk_string_t **strs = (wtk_string_t**)a->slot;
		int i;
		for(i = 0; i < a->nslot; i++){
			wrapper->notify_asr(wrapper->notify_asr_ths,strs[i]->data,strs[i]->len);
		}
	}
}

static int _qasr_feed(qtk_asr_wrapper_t *wrapper,char *data,int bytes, int st){
	int ret = 0;
	float fs,fe;
	wtk_string_t v;
	wrapper->is_end = 0;
	switch (wrapper->mode)
	{
	case 0://asr + wakeup
		/* wakeup */
		ret = qtk_k2_dec_feed(wrapper->wakeup,data,bytes,0);
		if(ret == 1){
			wrapper->waked = 1;
			wrapper->idle_bytes = 0;
			qtk_k2_dec_get_wake_time(wrapper->wakeup,&fs,&fe);
			if(wrapper->notify_wakeup){
				fs += st * 0.02;
				fe += st * 0.02;
				wrapper->notify_wakeup(wrapper->notify_wakeup_ths,fs,fe);
			}
			wrapper->asr = 1;
			qtk_k2_dec_enter_norm_state(wrapper->dec);
			if(wrapper->asr_start){
				ret = wrapper->sourcer.qtk_asr_feed(wrapper->dec,0,0,1);
				wrapper->sourcer.qtk_asr_get_result(wrapper->dec,&v);
				_qasr_notify_result(wrapper,v.data,v.len);
				wrapper->sourcer.qtk_asr_reset(wrapper->dec);
			}
			wrapper->sourcer.qtk_asr_start(wrapper->dec);
		}
		if(wrapper->waked == 1){
			wrapper->reset_bytes += bytes;
			if(wrapper->reset_bytes >= 6400){
				wrapper->waked = 0;
				wrapper->reset_bytes = 0;
				qtk_k2_dec_reset(wrapper->wakeup);
			}
		}

		if(wrapper->asr == 1){
			ret = wrapper->sourcer.qtk_asr_feed(wrapper->dec,data,bytes,0);
		}

		break;
	case 1://asr
		ret = wrapper->sourcer.qtk_asr_feed(wrapper->dec,data,bytes,0);
		break;
	case 2://wakeup
		ret = qtk_k2_dec_feed(wrapper->wakeup,data,bytes,0);
		if(ret == 1){
			wrapper->waked = 1;
			qtk_k2_dec_get_wake_time(wrapper->wakeup,&fs,&fe);
			if(wrapper->notify_wakeup){
				fs += st * 0.02;
				fe += st * 0.02;
				wrapper->notify_wakeup(wrapper->notify_wakeup_ths,fs,fe);
			}
		}
		if(wrapper->waked == 1){
			wrapper->reset_bytes += bytes;
			if(wrapper->reset_bytes >= 6400){
				wrapper->waked = 0;
				wrapper->reset_bytes = 0;
				qtk_k2_dec_reset(wrapper->wakeup);
			}
			//wtk_debug("%d\n",wrapper->reset_bytes);
		}
		break;
	default:
		break;
	}
	return ret;
}

static int _qasr_feed_end(qtk_asr_wrapper_t *wrapper, int st){
	int ret = 0;
	wtk_string_t v;
	float fs,fe;
	wrapper->is_end = 1;
	qtk_k2_dec_t * dec = (qtk_k2_dec_t*)wrapper->dec;
	switch (wrapper->mode)
	{
	case 0://asr + wakeup
		if(wrapper->asr == 1){
			wrapper->asr_start = 0;
			ret = wrapper->sourcer.qtk_asr_feed(wrapper->dec,0,0,1);
			wrapper->sourcer.qtk_asr_get_result(wrapper->dec,&v);
			if(wrapper->notify_asr){// dec->res_buf->pos > 0 TODO
				if(dec->res_buf->pos > 0){
					wrapper->idle_bytes = 0;
					qtk_k2_dec_enter_norm_state(wrapper->dec);
				}
				_qasr_notify_result(wrapper,v.data,v.len);
			}
			wrapper->sourcer.qtk_asr_reset(wrapper->dec);
		}

		ret = qtk_k2_dec_feed(wrapper->wakeup,0,0,1);
		if(ret == 1){
			wrapper->asr = 1;
			qtk_k2_dec_enter_norm_state(wrapper->dec);
			wrapper->idle_bytes = 0;
			qtk_k2_dec_get_wake_time(wrapper->wakeup,&fs,&fe);
			if(wrapper->notify_wakeup){
				fs += st * 0.02;
				fe += st * 0.02;
				wrapper->notify_wakeup(wrapper->notify_wakeup_ths,fs,fe);
			}
		}
		wrapper->waked = 0;
		wrapper->reset_bytes = 0;
		qtk_k2_dec_reset(wrapper->wakeup);

		break;
	case 1://asr
		ret = wrapper->sourcer.qtk_asr_feed(wrapper->dec,0,0,1);
		wrapper->sourcer.qtk_asr_get_result(wrapper->dec,&v);
		_qasr_notify_result(wrapper,v.data,v.len);
		wrapper->sourcer.qtk_asr_reset(wrapper->dec);
		break;
	case 2://wakeup
		ret = qtk_k2_dec_feed(wrapper->wakeup,0,0,1);
		if(ret == 1){
			qtk_k2_dec_get_wake_time(wrapper->wakeup,&fs,&fe);
			if(wrapper->notify_wakeup){
				fs += st * 0.02;
				fe += st * 0.02;
				wrapper->notify_wakeup(wrapper->notify_wakeup_ths,fs,fe);
			}
		}
		wrapper->waked = 0;
		wrapper->reset_bytes = 0;
		qtk_k2_dec_reset(wrapper->wakeup);
		break;
	default:
		break;
	}
	return ret;
}

static int _qasr_feed_vad(qtk_asr_wrapper_t* wrapper,char *data,int bytes,int is_end){
	wtk_vad_t *vad = wrapper->vad;
	wtk_vframe_t *f;
	wtk_queue_node_t *qn;

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
				if(wrapper->mode == 1){
					qtk_asr_wrapper_set_vadindex(wrapper,f->index);
				}else{
					qtk_k2_dec_set_vadindex(wrapper->wakeup,f->index);
				}
				_qasr_prepare(wrapper,(char*)f->wav_data,wrapper->frame_bytes);
				wrapper->last_vframe_state = wtk_vframe_speech;
			}
			break;
		case wtk_vframe_speech:
			if(f->state != wtk_vframe_speech){
				wrapper->last_vframe_state = wtk_vframe_sil;
				_qasr_feed_end(wrapper,wrapper->start_frame);
			}else{
				_qasr_feed(wrapper,(char*)f->wav_data,wrapper->frame_bytes,wrapper->start_frame);
			}
			break;
		case wtk_vframe_speech_end:
			break;
		default:
			break;
		}
		wtk_vad_push_vframe(vad,f);
	}

	if(wrapper->asr){
		wrapper->idle_bytes += bytes;
		if(wrapper->idle_bytes >= wrapper->idle_time * 32000 && wrapper->asr_start == 0){
			wrapper->asr = 0;
		}
		if(wrapper->idle_bytes >= 60 * 32000 * 5){
			qtk_k2_dec_enter_idle_state(wrapper->dec);
		}
	}

	if(is_end == 1 && wrapper->is_end == 0){
		_qasr_feed_end(wrapper,wrapper->start_frame);
	}

	return 0;
}

int qtk_asr_wrapper_feed(qtk_asr_wrapper_t* wrapper,char *data,int bytes,int is_end)
{
	int ret = 0;

	if(wrapper->cfg->use_vad){
		ret = _qasr_feed_vad(wrapper,data,bytes,is_end);
	}else{
		ret = wrapper->sourcer.qtk_asr_feed(wrapper->dec,data,bytes,is_end);
	}

	return ret;
}

void qtk_asr_wrapper_reset(qtk_asr_wrapper_t* wrapper)
{	
	wtk_queue_init(&(wrapper->vad_q));
	wrapper->last_vframe_state = wtk_vframe_sil;
	wrapper->is_end = 1;
	wrapper->start_frame = 0;
	wrapper->reset_bytes = 0;
	wrapper->waked = 0;
	wrapper->asr = 0;
	wrapper->asr_start = 0;
	wrapper->idle_bytes = 0;
    if(wrapper->vad)
    {
        wtk_vad_reset(wrapper->vad);
    }
	if(wrapper->wakeup)
	{
		qtk_k2_dec_reset(wrapper->wakeup);
	}
	if(wrapper->chnlike!=NULL)
	{
		wtk_chnlike_reset(wrapper->chnlike);
	}
	if(wrapper->heap){
		wtk_heap_reset(wrapper->heap);
	}
	wrapper->sourcer.qtk_asr_reset(wrapper->dec);
}

void qtk_asr_wrapper_delete(qtk_asr_wrapper_t* wrapper)
{
	if(wrapper->heap){
		wtk_heap_delete(wrapper->heap);
	}
    if(wrapper->vad)
    {
        wtk_vad_delete(wrapper->vad);
    }
	if(wrapper->wakeup)
	{
		qtk_k2_dec_delete(wrapper->wakeup);
		if(wrapper->wav_cache){
			wtk_strbuf_delete(wrapper->wav_cache);
		}
	}
	if(wrapper->lex)
	{
		wtk_lex_delete(wrapper->lex);
	}

	if(wrapper->chnlike)
	{
		wtk_chnlike_delete(wrapper->chnlike);
	}
	wrapper->sourcer.qtk_asr_delete(wrapper->dec);
	wtk_strbuf_delete(wrapper->keywords2);
	wtk_free(wrapper);
}

void qtk_asr_wrapper_get_result(qtk_asr_wrapper_t *wrapper,wtk_string_t *v)
{
	wrapper->sourcer.qtk_asr_get_result(wrapper->dec,v);
}

void qtk_asr_wrapper_get_hint_result(qtk_asr_wrapper_t *wrapper,wtk_string_t *v)
{
	wrapper->sourcer.qtk_asr_get_hint_result(wrapper->dec,v);
}

void qtk_asr_wrapper_get_vad_result(qtk_asr_wrapper_t *wrapper,wtk_string_t *v)
{

}

int qtk_asr_wrapper_set_context(qtk_asr_wrapper_t *wrapper,char *data,int bytes){
	if(wrapper->sourcer.qtk_asr_set_context){
		return wrapper->sourcer.qtk_asr_set_context(wrapper->dec,data,bytes);
	}
	return -1;
}

int qtk_asr_wrapper_set_context_asr(qtk_asr_wrapper_t *wrapper,char *data,int bytes){
	if(wrapper->cfg->type == QTK_K2){
		return qtk_k2_dec_set_keywords_asr(wrapper->dec,data,bytes);
	}
	return -1;
}

int qtk_asr_wrapper_set_context_wakeup(qtk_asr_wrapper_t *wrapper,char *data,int bytes){
	if(wrapper->cfg->type == QTK_K2){
		return qtk_k2_dec_set_keywords_wakeup(wrapper->dec,data,bytes);
	}
	return -1;
}

void qtk_asr_wrapper_set_sourcer(qtk_asr_wrapper_t *wrapper,
		void* (*qtk_asr_new)(void *),
		void (*qtk_asr_reset)(void *),
		void (*qtk_asr_delete)(void *),
		int (*qtk_asr_start)(void *),
		int (*qtk_asr_start2)(void *, char *data, int bytes),
		int (*qtk_asr_feed)(void *, char *data, int bytes, int is_end),
		void (*qtk_asr_get_result)(void *,wtk_string_t *v),
		void (*qtk_asr_get_hint_result)(void *,wtk_string_t *v),
		int (*qtk_asr_set_context)(void *,char *data, int bytes)
		)
{
	wrapper->sourcer.qtk_asr_new = qtk_asr_new;
	wrapper->sourcer.qtk_asr_reset = qtk_asr_reset;
	wrapper->sourcer.qtk_asr_delete = qtk_asr_delete;
	wrapper->sourcer.qtk_asr_start = qtk_asr_start;
	wrapper->sourcer.qtk_asr_start2 = qtk_asr_start2;
	wrapper->sourcer.qtk_asr_feed = qtk_asr_feed;
	wrapper->sourcer.qtk_asr_get_result = qtk_asr_get_result;
	wrapper->sourcer.qtk_asr_get_hint_result = qtk_asr_get_hint_result;
	wrapper->sourcer.qtk_asr_set_context = qtk_asr_set_context;
}

float qtk_asr_wrapper_set_vadindex(qtk_asr_wrapper_t * wrapper, int index){
	if(wrapper->cfg->type == QTK_K2){
		return qtk_k2_dec_set_vadindex(wrapper->dec,index);
	}
	return 0.0;
}

void qtk_asr_wrapper_get_wake_time(qtk_asr_wrapper_t *wrapper,float *fs,float *fe){
	if(wrapper->cfg->type == QTK_K2){
		qtk_k2_dec_get_wake_time(wrapper->dec,fs,fe);
	}
}

float qtk_asr_wrapper_get_conf(qtk_asr_wrapper_t *wrapper){
	if(wrapper->cfg->type == QTK_K2){
		return qtk_k2_dec_get_conf(wrapper->dec);
	}
	return -10.0;
}

void qtk_asr_wrapper_set_asr_notify(qtk_asr_wrapper_t *wrapper,void *upval, qtk_asr_wrapper_asr_notify_t notify){
	wrapper->notify_asr = notify;
	wrapper->notify_asr_ths = upval;
}

void qtk_asr_wrapper_set_wakeup_notify(qtk_asr_wrapper_t *wrapper,void *upval, qtk_asr_wrapper_wakeup_notify_t notify){
	wrapper->notify_wakeup = notify;
	wrapper->notify_wakeup_ths = upval;
}

void qtk_asr_wrapper_set_idle_time(qtk_asr_wrapper_t *wrapper, long unsigned int val){
	wrapper->idle_time = val;
}

void qtk_asr_wrapper_set_mode(qtk_asr_wrapper_t *wrapper, int mode){
	wrapper->mode = mode;
}

int qtk_asr_wrapper_set_contacts(qtk_asr_wrapper_t *wrapper, char *data, int len){
	wtk_strbuf_t *tmp_buf = wtk_strbuf_new(1024,1);
	char *s = data,*e = data + len;
	int i,cnt = 0;
	wtk_array_t *ab = wrapper->cfg->contact_b,*ae = wrapper->cfg->contact_e;
	if(!ab || !ae || !wrapper->cfg->place_b || !wrapper->cfg->place_e){
		return -1;
	}
	wtk_strbuf_reset(wrapper->keywords2);
	while(s < e){
        if(*s == '\n' || *s == EOF){
			if(tmp_buf->pos == 0){
				ab = wrapper->cfg->place_b;
				ae = wrapper->cfg->place_e;
			}else{
				cnt += 1;
				if(cnt >= 50){
					wtk_strbuf_delete(tmp_buf);
					return -1;
				}
				wtk_string_t **sb = (wtk_string_t**)ab->slot;
				wtk_string_t **se = (wtk_string_t**)ae->slot;
				for(i = 0; i < ab->nslot; i++){
					wtk_strbuf_push(wrapper->keywords2,sb[i]->data,sb[i]->len);
					//wtk_debug("%.*s\n",sb[i]->len,sb[i]->data);
					wtk_strbuf_push(wrapper->keywords2,tmp_buf->data,tmp_buf->pos);
					if(se[i]->len > 0){
						wtk_strbuf_push(wrapper->keywords2,se[i]->data,se[i]->len);
					}
					wtk_strbuf_push(wrapper->keywords2,"\n",1);
				}
			}
			wtk_strbuf_reset(tmp_buf);
		}else{
			wtk_strbuf_push(tmp_buf,s,1);
		}
		s++;
	}
	//wtk_debug("%.*s",wrapper->keywords2->pos,wrapper->keywords2->data);
	wtk_strbuf_delete(tmp_buf);
	return qtk_k2_dec_set_contacts(wrapper->dec,wrapper->keywords2->data,wrapper->keywords2->pos);
}

int qtk_asr_wrapper_set_text(qtk_asr_wrapper_t *wrapper, char *data, int len){
	return qtk_k2_dec_set_contacts(wrapper->dec,data,len);
}
