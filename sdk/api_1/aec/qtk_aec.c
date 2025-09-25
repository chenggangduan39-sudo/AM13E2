#include "qtk_aec.h"
#include "wtk/bfio/ssl/wtk_ssl2.h"

#define FEED_STEP (32 * 16 * 2)
void qtk_aec_on_aec(qtk_aec_t *qform, short **data, int len, int is_end);
void qtk_aec_on_cmask_aec(qtk_aec_t *qform, short *data, int len);
void qtk_aec_on_cmask_aec_ssl(qtk_aec_t *qform, float ts, float te, wtk_ssl2_extp_t *nbest_extp, int nbest);
void qtk_aec_data_change_vol(char *data, int bytes, float shift);

qtk_aec_t *qtk_aec_new(qtk_aec_cfg_t *cfg)
{
	qtk_aec_t *qform;
	int i, ret;

	qform = (qtk_aec_t *)wtk_calloc(1, sizeof(*qform));
	qform->aec = NULL;
	qform->caec = NULL;
	qform->caec2 = NULL;
	qform->buf = NULL;

	qform->cfg = cfg;
	if(cfg->use_cmask_aec){
		if(qform->cfg->caec_cfg){
			qform->caec = wtk_cmask_aec_new(qform->cfg->caec_cfg);
			if(!qform->caec){
				ret = -1;
				goto end;
			}	
		}
		wtk_cmask_aec_set_notify(qform->caec, qform, (wtk_cmask_aec_notify_f)qtk_aec_on_cmask_aec);
		wtk_cmask_aec_set_ssl_notify(qform->caec, qform, (wtk_cmask_aec_notify_ssl_f)qtk_aec_on_cmask_aec_ssl);
		qform->channel = qform->caec->cfg->channel;
		qform->winslen = qform->cfg->caec_cfg->wins*qform->channel;
	}else if(cfg->use_cmask_aec2){
		if(qform->cfg->caec2_cfg){
			qform->caec2 = wtk_cmask_aec2_new(qform->cfg->caec2_cfg);
			if(!qform->caec2){
				ret = -1;
				goto end;
			}	
		}
		wtk_cmask_aec2_set_notify(qform->caec2, qform, (wtk_cmask_aec_notify_f)qtk_aec_on_cmask_aec);
		wtk_cmask_aec2_set_ssl_notify(qform->caec2, qform, (wtk_cmask_aec_notify_ssl_f)qtk_aec_on_cmask_aec_ssl);
		qform->channel = qform->caec2->cfg->channel;
		qform->winslen = qform->cfg->caec2_cfg->wins*qform->channel;
	}else{
		if(qform->cfg->aec_cfg){
			qform->aec = wtk_aec_new(qform->cfg->aec_cfg);
			if(!qform->aec){
				ret = -1;
				goto end;
			}	
		}
		wtk_aec_set_notify(qform->aec, qform, (wtk_aec_notify_f)qtk_aec_on_aec);
		qform->channel = qform->aec->cfg->stft.channel;
		qform->winslen = qform->cfg->aec_cfg->stft.win*qform->channel;
	}
	wtk_debug("channel= %d\n", qform->channel);
	qform->buf = (short **)wtk_malloc(sizeof(short *) * qform->channel);
	for(i=0; i < qform->channel; i++){
		qform->buf[i] = (short *)wtk_malloc(FEED_STEP);
	}
	qform->out_buf = wtk_strbuf_new(1024, 1.0);
	qform->zdata = (char *)wtk_malloc(qform->winslen*sizeof(char));
	memset(qform->zdata, 0, qform->winslen);

	if(qform->cfg->use_log_wav){
		char bufname[100]={0};
		snprintf(bufname, 100, "%.*s/mic.wav",qform->cfg->cache_fn.len,qform->cfg->cache_fn.data);
		qform->micwav = wtk_wavfile_new(16000);
		wtk_debug("mic:===>[%s]\n",bufname);
		ret=wtk_wavfile_open(qform->micwav, bufname);
		if(ret!=0){
			wtk_wavfile_delete(qform->micwav);
			qform->micwav = NULL;
			wtk_debug("=====================>>>>>mic wav open faild\n");
		}else{
			qform->micwav->max_pend=0;
			wtk_wavfile_set_channel(qform->micwav, qform->channel);
		}
		memset(bufname, 0, 100);
		snprintf(bufname, 100, "%.*s/echo.wav",qform->cfg->cache_fn.len,qform->cfg->cache_fn.data);
		qform->echowav = wtk_wavfile_new(16000);
		wtk_debug("echo:===>[%s]\n",bufname);
		ret = wtk_wavfile_open(qform->echowav, bufname);
		if(ret!=0){
			wtk_wavfile_delete(qform->echowav);
			qform->echowav = NULL;
			wtk_debug("=====================>>>>>echo wav open faild\n");
		}else{
			qform->echowav->max_pend=0;
		}
	}

	ret = 0;
end:
	if(ret != 0){
		qtk_aec_delete(qform);
		qform = NULL;		
	}	
	return qform;
}
int qtk_aec_delete(qtk_aec_t *qform)
{
	int i;

	if(qform->cfg->use_cmask_aec){
		if(qform->caec){
			wtk_cmask_aec_delete(qform->caec);
		}
	}else if(qform->cfg->use_cmask_aec2){
		if(qform->caec2){
			wtk_cmask_aec2_delete(qform->caec2);
		}
	}else{
		if(qform->aec){
			wtk_aec_delete(qform->aec);
		}
	}

	if(qform->micwav){
		wtk_wavfile_delete(qform->micwav);
	}
	if(qform->echowav){
		wtk_wavfile_delete(qform->echowav);
	}

	if(qform->buf){
		for(i = 0; i < qform->channel; i++){
			wtk_free(qform->buf[i]);
		}
		wtk_free(qform->buf);
	}

	if(qform->zdata){
		wtk_free(qform->zdata);
	}
	if(qform->out_buf){
		wtk_strbuf_delete(qform->out_buf);
	}
	wtk_free(qform);
	return 0;
}
int qtk_aec_start(qtk_aec_t *qform)
{
	if(qform->cfg->use_cmask_aec){
		if(qform->caec){
			wtk_cmask_aec_start(qform->caec);
		}
	}else if(qform->cfg->use_cmask_aec2){
		if(qform->caec2){
			wtk_cmask_aec2_start(qform->caec2);
		}
	}
	qform->is_start = 1;
	return 0;
}
int qtk_aec_reset(qtk_aec_t *qform)
{
	if(qform->cfg->use_cmask_aec){
		if(qform->caec){
			wtk_cmask_aec_reset(qform->caec);
		}
	}else if(qform->cfg->use_cmask_aec2){
		if(qform->caec2){
			wtk_cmask_aec2_reset(qform->caec2);
		}
	}else{
		if(qform->aec){
			wtk_aec_reset(qform->aec);
		}
	}
	if(qform->out_buf){
		wtk_strbuf_reset(qform->out_buf);
	}
	return 0;
}

static int qtk_aec_on_feed2(qtk_aec_t *qform, char *data, int bytes, int is_end)
{
	int i, j;
	short *pv = NULL;
	int len;

	if(bytes > 0){
		pv = (short*)data;
		len = bytes /(qform->channel * sizeof(short));

		if(qform->cfg->mic_shift != 1.0f || qform->cfg->spk_shift != 1.0f){
			// wtk_debug("==============================>>>>>>>>>>>>%f %f\n",qform->cfg->mic_shift,qform->cfg->spk_shift);
			qtk_data_change_vol2(data, bytes, qform->cfg->mic_shift, qform->cfg->spk_shift,qform->channel-qform->cfg->spk_channel, qform->cfg->spk_channel);
		}

		if(qform->is_start && bytes < qform->winslen*qform->channel){
			qform->is_start=0;
			qtk_aec_on_feed2(qform, qform->zdata, qform->winslen*qform->channel-bytes, 0);
		}

		if(qform->cfg->use_log_wav && qform->micwav){
			wtk_wavfile_write(qform->micwav, data, bytes);
		}

		if(qform->cfg->use_cmask_aec && qform->caec){
			wtk_cmask_aec_feed(qform->caec, pv, len, 0);
		}else if(qform->cfg->use_cmask_aec2 && qform->caec2){
			wtk_cmask_aec2_feed(qform->caec2, pv, len, 0);
		}else{
			for(i = 0; i < len; ++i){
				for(j = 0; j < qform->channel; ++j){
					qform->buf[j][i] = pv[i * qform->channel + j];
				}
			}
			wtk_aec_feed(qform->aec, qform->buf, len, 0);
		}
	}
	if(is_end){
		if(qform->cfg->use_cmask_aec && qform->caec){
			wtk_cmask_aec_feed(qform->caec, NULL, 0, 1);
		}else if(qform->cfg->use_cmask_aec2 && qform->caec2){
			wtk_cmask_aec2_feed(qform->caec2, NULL, 0, 1);
		}else{
			wtk_aec_feed(qform->aec, NULL, 0, 1);
		}
	}
	if(qform->cfg->use_cache_mode == 1){
		int nx=bytes/qform->channel;
		if(qform->out_buf->pos >= nx){
			qtk_var_t var;
			if(qform->notify){
				short *out[1]={0};
				out[1]=qform->out_buf->data;
				qform->notify(qform->ths, out, nx>>1, 0);
			}
		
			if(qform->enotify){
				//wtk_debug("========================>>>>>>>>>>>>>>>>>%d\n",len);
				var.type = QTK_SPEECH_DATA_PCM;
				var.v.str.data = qform->out_buf->data;
				var.v.str.len = nx;
				qform->enotify(qform->eths, &var);
			}
			wtk_strbuf_pop(qform->out_buf, NULL, nx);
		}
	}else{
	#ifdef USE_32MSTO8MS
		if(qform->out_buf->pos >= 32*8){
			qtk_var_t var;
			if(qform->notify){
				short *out[1]={0};
				out[1]=qform->out_buf->data;
				qform->notify(qform->ths, out, 8*16, 0);
			}
		
			if(qform->enotify){
				//wtk_debug("========================>>>>>>>>>>>>>>>>>%d\n",len);
				var.type = QTK_SPEECH_DATA_PCM;
				var.v.str.data = qform->out_buf->data;
				var.v.str.len = 8*32;
				qform->enotify(qform->eths, &var);
			}
			wtk_strbuf_pop(qform->out_buf, NULL, 8*32);
		}
	#endif
	}
	return 0;
}

static int qtk_aec_on_feed(qtk_aec_t *qform, char *data, int bytes, int is_end)
{
	int i, j;
	short *pv = NULL;
	int len;

	if(bytes > 0){
		pv = (short*)data;
		len = bytes /(qform->channel * sizeof(short));
		
		if(qform->cfg->mic_shift != 1.0f || qform->cfg->spk_shift != 1.0f){
			// wtk_debug("==============================>>>>>>>>>>>>%f %f\n",vb->cfg->mic_shift,vb->cfg->spk_shift);
			qtk_data_change_vol2(data, bytes, qform->cfg->mic_shift, qform->cfg->spk_shift,qform->channel+qform->cfg->nmic-qform->cfg->spk_channel, qform->cfg->spk_channel);
		}
		if(qform->cfg->use_log_wav && qform->micwav){
			wtk_wavfile_write(qform->micwav, data, bytes);
		}

		if(qform->cfg->use_cmask_aec && qform->caec){
			//wtk_debug("====>>>>>bytes=%d len=%d\n",bytes,len);
			wtk_cmask_aec_feed(qform->caec, pv, len, 0);
		}else if(qform->cfg->use_cmask_aec2 && qform->caec2){
			//wtk_debug("====>>>>>bytes=%d len=%d\n",bytes,len);
			wtk_cmask_aec2_feed(qform->caec2, pv, len, 0);
		}else{
			for(i = 0; i < len; ++i){
				for(j = 0; j < qform->channel; ++j){
					qform->buf[j][i] = pv[i * qform->channel + j];
				}
			}
			wtk_aec_feed(qform->aec, qform->buf, len, 0);
		}
	}
	if(is_end){
		if(qform->cfg->use_cmask_aec && qform->caec){
			wtk_cmask_aec_feed(qform->caec, NULL, 0, 1);
		}else if(qform->cfg->use_cmask_aec2 && qform->caec2){
			wtk_cmask_aec2_feed(qform->caec2, NULL, 0, 1);
		}else{
			wtk_aec_feed(qform->aec, NULL, 0, 1);
		}
	}
	return 0;
}

int qtk_aec_feed(qtk_aec_t *qform, char *data, int len, int is_end)
{
#if 0
	int pos = 0;
	int step = 0;
	int flen;
	
	if(qform->cfg->mic_shift != 1.0f)
	{
		qtk_aec_data_change_vol(data, len, qform->cfg->mic_shift);
	}

	step = FEED_STEP * qform->channel;
	while(pos < len){
		flen = min(step, len - pos);
		qtk_aec_on_feed(qform, data + pos, flen, 0);
		pos += flen;
	} 
	if(is_end){
		qtk_aec_on_feed(qform, NULL, 0, 1);
	}
#else
	if(qform->cfg->use_cache_mode == 1){
		qtk_aec_on_feed2(qform, data, len, is_end);
	}else{
	#ifdef USE_32MSTO8MS
		qtk_aec_on_feed2(qform, data, len, is_end);
	#else
		qtk_aec_on_feed(qform, data, len, is_end);
	#endif
	}
#endif
	return 0;
}
void qtk_aec_set_notify(qtk_aec_t *qform, void *ths, qtk_aec_notify_f notify)
{
	qform->ths = ths;
	qform->notify = notify;
}

void qtk_aec_set_notify2(qtk_aec_t *qform, void *ths, qtk_engine_notify_f notify)
{
	qform->eths = ths;
	qform->enotify = notify;
}

void qtk_aec_on_cmask_aec(qtk_aec_t *qform, short *data, int len)
{
	qtk_var_t var;

	if(qform->cfg->echo_shift != 1.0f){
		qtk_data_change_vol((char *)data, len<<1, qform->cfg->echo_shift);
	}

	if(qform->cfg->use_log_wav && qform->echowav){
		wtk_wavfile_write(qform->echowav, (char *)data, len<<1);
	}
#ifdef USE_LIXUN
	if(qform->cfg->use_cache_mode == 1){
		wtk_strbuf_push(qform->out_buf, (char *)data, len<<1);
	}else{
	#ifdef USE_32MSTO8MS
		wtk_strbuf_push(qform->out_buf, (char *)data, len<<1);
	#else
		wtk_strbuf_push(qform->out_buf, (char *)data, len<<1);
		if(qform->out_buf->pos >= 1024){
			if(qform->notify){
				short *out[1]={0};
				out[1]=qform->out_buf->data;
				qform->notify(qform->ths, out, 512, 0);
			}
			if(qform->enotify){
				if(qform->cfg->echo_shift != 1.0f){
					qtk_aec_data_change_vol(qform->out_buf->data, 1024, qform->cfg->echo_shift);
				}
				var.type=QTK_SPEECH_DATA_PCM;
				var.v.str.data=qform->out_buf->data;
				var.v.str.len=1024;
				qform->enotify(qform->eths, &var);
			}
			wtk_strbuf_pop(qform->out_buf, NULL, 1024);
		}
	#endif
	}
#else
	if(qform->cfg->use_cache_mode == 1){
		wtk_strbuf_push(qform->out_buf, (char *)data, len<<1);
	}else{
		if(qform->notify){
			short *out[1]={0};
			out[0]=data;
			qform->notify(qform->ths, out, len<<1, 0);
		}
		if(qform->enotify){
			if(qform->cfg->echo_shift != 1.0f){
				qtk_aec_data_change_vol((char *)data, len<<1, qform->cfg->echo_shift);
			}
			var.type=QTK_SPEECH_DATA_PCM;
			var.v.str.data=(char *)data;
			var.v.str.len=len<<1;
			qform->enotify(qform->eths, &var);
		}
	}
#endif
}

void qtk_aec_on_aec(qtk_aec_t *qform, short **data, int len, int is_end)
{
	qtk_var_t var;

	if(qform->notify){
		qform->notify(qform->ths, data, len, is_end);
	}
	if(qform->enotify){
		if(qform->cfg->echo_shift != 1.0f){
			qtk_aec_data_change_vol((char *)(data[0]), (len<<1)/(qform->channel-qform->cfg->spk_channel), qform->cfg->echo_shift);
		}
		var.type=QTK_SPEECH_DATA_PCM;
		var.v.str.data=(char *)(data[0]);
		var.v.str.len=(len<<1)/(qform->channel - qform->cfg->spk_channel);
		qform->enotify(qform->eths, &var);
	}
}


void qtk_aec_on_cmask_aec_ssl(qtk_aec_t *qform, float ts, float te, wtk_ssl2_extp_t *nbest_extp, int nbest)
{
	qtk_var_t var;
	int i;

	for(i=0; i<nbest; ++i)
	{
		// printf("nbest=%d/%f theta=%d phi=%d\n",nbest, nbest_extp[i].nspecsum, nbest_extp[i].theta, nbest_extp[i].phi);
		if(nbest_extp[i].nspecsum > qform->cfg->energy_sum){
			// printf("nbest=%d/%d theta=%d phi=%d\n",nbest, i, nbest_extp[i].theta, nbest_extp[i].phi);
			if (qform->enotify){
				var.type = QTK_AEC_DIRECTION;
				var.v.ii.nbest = i;
				var.v.ii.theta = nbest_extp[i].theta;
				var.v.ii.phi = nbest_extp[i].phi;
				var.v.ii.nspecsum = nbest_extp[i].nspecsum;
				qform->enotify(qform->eths, &(var));
			}
		}else{
			// wtk_debug("================>>>>>>>-1\n");
			if (qform->enotify){
				var.type = QTK_AEC_DIRECTION;
				var.v.ii.nbest = i;
				var.v.ii.theta = -1;
				var.v.ii.phi = -1;
				var.v.ii.nspecsum = 0.0;
				qform->enotify(qform->eths, &(var));
			}
		}
	}
}

void qtk_aec_data_change_vol(char *data, int bytes, float shift)
{
	short *ps, *pe;
	int num;

	ps = (short *)data;
	pe = (short *)(data + bytes);

	while(ps < pe){
		num = (*ps) *shift;
		if(num > 32767){
			*ps = 32767;
		}else if(num < -32768){
			*ps = -32768;
		}else{
			*ps = num;
		}
		++ps;
	}
}
