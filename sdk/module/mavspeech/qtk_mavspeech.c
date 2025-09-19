#include "qtk_mavspeech.h" 
#include "sdk/engine/comm/qtk_engine_hdr.h"

int qtk_mavspeech_on_start(qtk_mavspeech_t *e);
int qtk_mavspeech_on_feed(qtk_mavspeech_t *e,char *data,int len);
int qtk_mavspeech_on_end(qtk_mavspeech_t *e);
void qtk_mavspeech_on_reset(qtk_mavspeech_t *e);
void qtk_mavspeech_on_set_notify(qtk_mavspeech_t *e,void *notify_ths,qtk_engine_notify_f notify_f);
int qtk_mavspeech_on_set(qtk_mavspeech_t *e,char *data,int bytes);

void qtk_mavspeech_init(qtk_mavspeech_t *e)
{
	qtk_engine_param_init(&e->param);

	e->session = NULL;

	e->cfg = NULL;

	e->notify     = NULL;
	e->notify_ths = NULL;

	e->thread   = NULL;
	e->callback = NULL;

}

static int on_sep_(qtk_mavspeech_t *ctx, int id, uint32_t frame_idx,
                   qtk_avspeech_separator_result_t *result) {
    switch (result->type)
    {
    case QTK_AVSPEECH_SEPARATOR_AVATOR:
        wtk_debug("id=%d frame_idx=%d width=%d height=%d\n", id, frame_idx, result->avator.width, result->avator.height);
        break;
    case QTK_AVSPEECH_SEPARATOR_AUDIO:
        // fwrite(result->audio.wav, sizeof(short), result->audio.len, ctx->f);
        break;
    case QTK_AVSPEECH_SEPARATOR_FACE_ROI:
        wtk_debug("id=%d frame_idx=%d x1=%f y1=%f x2=%f y2=%f\n", id, frame_idx, result->face_roi.x1, result->face_roi.y1, result->face_roi.x2, result->face_roi.y2);
        break;
    case QTK_AVSPEECH_SEPARATOR_NOPERSON:
        wtk_debug("id=%d frame_idx=%d\n",id,frame_idx);
        break;    
    default:
        break;
    }

    return 0;
}

qtk_mavspeech_t* qtk_mavspeech_new(qtk_session_t *session,wtk_local_cfg_t *params)
{
	qtk_mavspeech_t *e;
	int buf_size = 0;
	int ret;

	e=(qtk_mavspeech_t*)wtk_calloc(1, sizeof(qtk_mavspeech_t));
	qtk_mavspeech_init(e);
	e->session = session;

	qtk_engine_param_set_session(&e->param, e->session);
	ret = qtk_engine_param_feed(&e->param, params);
	if(ret != 0) {
		wtk_log_warn0(e->session->log,"params als failed.");
		goto end;
	}

	qtk_nnrt_set_global_thread_pool(4, NULL);

	e->main_cfg = wtk_main_cfg_new_type(qtk_avspeech_separator_cfg, e->param.cfg);
	e->cfg = (qtk_avspeech_separator_cfg_t *)(e->main_cfg->cfg);
    e->sep = qtk_avspeech_separator_new(
        (qtk_avspeech_separator_cfg_t *)e->main_cfg->cfg, e,
        (qtk_avspeech_separator_notifier_t)on_sep_);
	e->channel = 1;
	if(e->cfg->use_qform9){
		e->channel = e->cfg->qform9.stft2.channel;
	}

	if(e->param.use_logwav && e->param.log_wav_path->len>0){
        e->wav=wtk_wavfile_new(16000);
        e->wav->max_pend=0;
        ret=wtk_wavfile_open(e->wav,e->param.log_wav_path->data);
		if(ret==-1){goto end;}
	}

	ret = 0;
end:
	wtk_log_log(e->session->log,"ret = %d",ret);
	if(ret != 0) {
		qtk_mavspeech_delete(e);
		e = NULL;
	}
	return e;
}

int qtk_mavspeech_delete(qtk_mavspeech_t *e)
{
    qtk_avspeech_separator_delete(e->sep);
    wtk_main_cfg_delete(e->main_cfg);
	qtk_engine_param_clean(&e->param);
	wtk_free(e);
	return 0;
}


int qtk_mavspeech_on_start(qtk_mavspeech_t *e)
{
	return 0;
}

int qtk_mavspeech_on_feed(qtk_mavspeech_t *e,char *data,int bytes)
{
	int ret;
	int len=bytes/(sizeof(short)*e->channel);
	ret = qtk_avspeech_separator_feed_audio(e->sep, data, len);
	return ret;
}

int qtk_mavspeech_on_end(qtk_mavspeech_t *e)
{
	return 0;
}

void qtk_mavspeech_on_reset(qtk_mavspeech_t *e)
{

}

void qtk_mavspeech_on_set_notify(qtk_mavspeech_t *e,void *notify_ths,qtk_engine_notify_f notify_f)
{
	e->notify_ths = notify_ths;
	e->notify     = notify_f;
}

int qtk_mavspeech_on_set(qtk_mavspeech_t *e,char *data,int bytes)
{
	wtk_cfg_file_t *cfile = NULL;
	wtk_cfg_item_t *item;
	wtk_queue_node_t *qn;
	int ret;

	wtk_log_log(e->session->log,"set param = %.*s\n",bytes,data);
	cfile = wtk_cfg_file_new();
	if(!cfile) {
		return -1;
	}

	ret = wtk_cfg_file_feed(cfile,data,bytes);
	if(ret != 0) {
		goto end;
	}

	// for(qn=cfile->main->cfg->queue.pop;qn;qn=qn->next) {
	// 	item = data_offset2(qn,wtk_cfg_item_t,n);
	// 	if(wtk_string_cmp2(item->key,&qtk_engine_set_str[16]) == 0) {
	// 		qtk_agc_set_agcenable(e->eqform->vebf3, atoi(item->value.str->data));
	// 	} else if (wtk_string_cmp2(item->key,&qtk_engine_set_str[17]) == 0) {
	// 		qtk_agc_set_echoenable(e->eqform->vebf3, atoi(item->value.str->data));
	// 	} else if (wtk_string_cmp2(item->key,&qtk_engine_set_str[18]) == 0) {
	// 		qtk_agc_set_denoiseenable(e->eqform->vebf3, atoi(item->value.str->data));
	// 	}
	// }
	
end:
	if(cfile) {
		wtk_cfg_file_delete(cfile);
	}
	return 0;
}


int qtk_mavspeech_start(qtk_mavspeech_t *e)
{
	int ret;

	qtk_mavspeech_on_start(e);
	ret = 0;
	return ret;
}

int qtk_mavspeech_feed_audio(qtk_mavspeech_t *e,char *data,int bytes,int is_end)
{
	if(bytes > 0) {
		qtk_mavspeech_on_feed(e,data,bytes);
	}
	if(is_end) {
		qtk_mavspeech_on_end(e);
	}
	return 0;
}

int qtk_mavspeech_feed_image(qtk_mavspeech_t *e, char *data)
{
	qtk_avspeech_separator_feed_image(e->sep, data);
	return 0;
}

int qtk_mavspeech_reset(qtk_mavspeech_t *e)
{
	qtk_mavspeech_on_reset(e);
	return 0;
}

int qtk_mavspeech_cancel(qtk_mavspeech_t *e)
{
	return 0;
}

void qtk_mavspeech_set_notify(qtk_mavspeech_t *e,void *ths,qtk_engine_notify_f notify_f)
{
	qtk_mavspeech_on_set_notify(e,ths,notify_f);
}

int qtk_mavspeech_set(qtk_mavspeech_t *e,char *data,int bytes)
{
	int ret = 0;

	ret = qtk_mavspeech_on_set(e,data,bytes);
	return ret;
}


