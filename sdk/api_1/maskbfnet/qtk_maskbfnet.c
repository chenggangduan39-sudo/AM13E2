#include "qtk_maskbfnet.h"
#include "wtk/bfio/ssl/wtk_ssl2.h"

#define FEED_STEP (32 * 16 * 2)
void qtk_maskbfnet_on_maskbfnet(qtk_maskbfnet_t *qform, short *data, int len);
void qtk_maskbfnet_data_change_vol(char *data, int bytes, float shift);

qtk_maskbfnet_t *qtk_maskbfnet_new(qtk_maskbfnet_cfg_t *cfg)
{
	qtk_maskbfnet_t *qform;
	int i, ret;

	qform = (qtk_maskbfnet_t *)wtk_calloc(1, sizeof(*qform));
	qform->maskbfnet = NULL;
	qform->buf = NULL;

	qform->cfg = cfg;
	if(qform->cfg->maskbfnet_cfg){
		qform->maskbfnet = wtk_mask_bf_net_new(qform->cfg->maskbfnet_cfg);
		if(!qform->maskbfnet){
			ret = -1;
			goto end;
		}
	}
	wtk_mask_bf_net_set_notify(qform->maskbfnet, qform, (wtk_mask_bf_net_notify_f)qtk_maskbfnet_on_maskbfnet);
	qform->channel = qform->maskbfnet->cfg->channel;
	qform->winslen = qform->cfg->maskbfnet_cfg->wins*qform->channel*qform->cfg->maskbfnet_cfg->num_frame;
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
		qtk_maskbfnet_delete(qform);
		qform = NULL;		
	}	
	return qform;
}
int qtk_maskbfnet_delete(qtk_maskbfnet_t *qform)
{
	int i;

	if(qform->maskbfnet){
		wtk_mask_bf_net_delete(qform->maskbfnet);
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
int qtk_maskbfnet_start(qtk_maskbfnet_t *qform)
{
	if(qform->maskbfnet){
		wtk_mask_bf_net_start(qform->maskbfnet);
	}
	qform->is_start = 1;
	return 0;
}
int qtk_maskbfnet_reset(qtk_maskbfnet_t *qform)
{
	if(qform->maskbfnet){
		wtk_mask_bf_net_reset(qform->maskbfnet);
	}
	if(qform->out_buf){
		wtk_strbuf_reset(qform->out_buf);
	}
	return 0;
}

static int qtk_maskbfnet_on_feed3(qtk_maskbfnet_t *qform, char *data, int bytes, int is_end)
{
	int i, j;
	short *pv = NULL;
	int len;

	if(bytes > 0){
		pv = (short*)data;
		len = bytes /(qform->channel * sizeof(short));

		if(qform->cfg->mic_shift != 1.0f){
			// wtk_debug("==============================>>>>>>>>>>>>%f %f\n",qform->cfg->mic_shift,qform->cfg->spk_shift);
			qtk_data_change_vol(data, bytes, qform->cfg->mic_shift);
		}

		if(qform->is_start && bytes < qform->winslen){
			qform->is_start=0;
			qtk_maskbfnet_on_feed3(qform, qform->zdata, qform->winslen - bytes, 0);
		}

		if(qform->cfg->use_log_wav && qform->micwav){
			wtk_wavfile_write(qform->micwav, data, bytes);
		}

		if(qform->maskbfnet){
			wtk_mask_bf_net_feed(qform->maskbfnet, pv, len, 0);
		}
	}
	if(is_end){
		if(qform->maskbfnet){
			wtk_mask_bf_net_feed(qform->maskbfnet, NULL, 0, 1);
		}
	}
	return 0;
}

static int qtk_maskbfnet_on_feed2(qtk_maskbfnet_t *qform, char *data, int bytes, int is_end)
{
	int i, j;
	short *pv = NULL;
	int len;

	if(bytes > 0){
		pv = (short*)data;
		len = bytes /(qform->channel * sizeof(short));

		if(qform->cfg->mic_shift != 1.0f){
			// wtk_debug("==============================>>>>>>>>>>>>%f %f\n",qform->cfg->mic_shift,qform->cfg->spk_shift);
			qtk_data_change_vol(data, bytes, qform->cfg->mic_shift);
		}

		if(qform->is_start && bytes < qform->winslen){
			qform->is_start=0;
			qtk_maskbfnet_on_feed2(qform, qform->zdata, qform->winslen - bytes, 0);
		}

		if(qform->cfg->use_log_wav && qform->micwav){
			wtk_wavfile_write(qform->micwav, data, bytes);
		}

		if(qform->maskbfnet){
			wtk_mask_bf_net_feed(qform->maskbfnet, pv, len, 0);
		}
	}
	if(is_end){
		if(qform->maskbfnet){
			wtk_mask_bf_net_feed(qform->maskbfnet, NULL, 0, 1);
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

static int qtk_maskbfnet_on_feed(qtk_maskbfnet_t *qform, char *data, int bytes, int is_end)
{
	int i, j;
	short *pv = NULL;
	int len;

	if(bytes > 0){
		pv = (short*)data;
		len = bytes /(qform->channel * sizeof(short));
		
		if(qform->cfg->mic_shift != 1.0f){
			// wtk_debug("==============================>>>>>>>>>>>>%f %f\n",vb->cfg->mic_shift,vb->cfg->spk_shift);
			qtk_data_change_vol(data, bytes, qform->cfg->mic_shift);
		}
		if(qform->cfg->use_log_wav && qform->micwav){
			wtk_wavfile_write(qform->micwav, data, bytes);
		}

		if(qform->maskbfnet){
			//wtk_debug("====>>>>>bytes=%d len=%d\n",bytes,len);
			wtk_mask_bf_net_feed(qform->maskbfnet, pv, len, 0);
		}
	}
	if(is_end){
		if(qform->maskbfnet){
			wtk_mask_bf_net_feed(qform->maskbfnet, NULL, 0, 1);
		}
	}
	return 0;
}

int qtk_maskbfnet_feed(qtk_maskbfnet_t *qform, char *data, int len, int is_end)
{
#if 0
	int pos = 0;
	int step = 0;
	int flen;
	
	if(qform->cfg->mic_shift != 1.0f)
	{
		qtk_maskbfnet_data_change_vol(data, len, qform->cfg->mic_shift);
	}

	step = FEED_STEP * qform->channel;
	while(pos < len){
		flen = min(step, len - pos);
		qtk_maskbfnet_on_feed(qform, data + pos, flen, 0);
		pos += flen;
	} 
	if(is_end){
		qtk_maskbfnet_on_feed(qform, NULL, 0, 1);
	}
#else
	if(qform->cfg->use_cache_mode == 1){
		qtk_maskbfnet_on_feed2(qform, data, len, is_end);
	}else{
	#ifdef USE_32MSTO8MS
		qtk_maskbfnet_on_feed2(qform, data, len, is_end);
	#else
		qtk_maskbfnet_on_feed(qform, data, len, is_end);
	#endif
	}
#endif
	return 0;
}

int qtk_maskbfnet_feed2(qtk_maskbfnet_t *qform, char *input, int in_bytes, char *output, int *out_bytes, int is_end)
{
	qtk_maskbfnet_on_feed3(qform, input, in_bytes, is_end);

	if(qform->cfg->use_cache_mode == 1){
		int nx=in_bytes/qform->channel;
		if(qform->out_buf->pos >= nx){
			if(output != NULL){
				memcpy(output, qform->out_buf->data, nx);
			}
			if(out_bytes != NULL){
				*out_bytes = nx;
			}
			wtk_strbuf_pop(qform->out_buf, NULL, nx);
		}
	}
	return 0;
}

void qtk_maskbfnet_set_notify(qtk_maskbfnet_t *qform, void *ths, qtk_maskbfnet_notify_f notify)
{
	qform->ths = ths;
	qform->notify = notify;
}

void qtk_maskbfnet_set_notify2(qtk_maskbfnet_t *qform, void *ths, qtk_engine_notify_f notify)
{
	qform->eths = ths;
	qform->enotify = notify;
}

void qtk_maskbfnet_on_maskbfnet(qtk_maskbfnet_t *qform, short *data, int len)
{
	qtk_var_t var;

	if(qform->cfg->echo_shift != 1.0f){
		qtk_data_change_vol((char *)data, len<<1, qform->cfg->echo_shift);
	}

	if(qform->cfg->use_log_wav && qform->echowav){
		wtk_wavfile_write(qform->echowav, (char *)data, len<<1);
	}
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
				qtk_maskbfnet_data_change_vol((char *)data, len<<1, qform->cfg->echo_shift);
			}
			var.type=QTK_SPEECH_DATA_PCM;
			var.v.str.data=(char *)data;
			var.v.str.len=len<<1;
			qform->enotify(qform->eths, &var);
		}
	}
}

void qtk_maskbfnet_data_change_vol(char *data, int bytes, float shift)
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
