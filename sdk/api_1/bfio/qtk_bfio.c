#include "qtk_bfio.h"

#define FEED_STEP (32 * 16 * 2)
void qtk_bfio_on_bfio(qtk_bfio_t *bfio, wtk_bfio_cmd_t cmd, short *data, int len);
void qtk_bfio5_on_bfio(qtk_bfio_t *bfio, wtk_bfio5_cmd_t cmd, short *data, int len);

qtk_bfio_t *qtk_bfio_new(qtk_bfio_cfg_t *cfg)
{
	qtk_bfio_t *qform;
	int i, ret;

	qform = (qtk_bfio_t *)wtk_calloc(1, sizeof(*qform));
	qform->cfg = cfg;
	if(qform->cfg->use_bfio){
		if(qform->cfg->qform_cfg){
			qform->qform = wtk_bfio_new(qform->cfg->qform_cfg);
			if(!qform->qform){
				ret = -1;
				goto end;
			}
		}
		wtk_bfio_set_notify(qform->qform, qform, (wtk_bfio_notify_f)qtk_bfio_on_bfio);
		qform->channel = qform->cfg->qform_cfg->stft2.channel + qform->cfg->qform_cfg->sp_stft2.channel;
	}else if(qform->cfg->use_bfio5){
		if(qform->cfg->bfio5_cfg){
			qform->bfio5 = wtk_bfio5_new(qform->cfg->bfio5_cfg);
			if(!qform->bfio5){
				ret = -1;
				goto end;
			}
		}
		wtk_bfio5_set_notify(qform->bfio5, qform, (wtk_bfio5_notify_f)qtk_bfio5_on_bfio);
		if(qform->cfg->bfio5_cfg->in_channel > 0){
			qform->channel = qform->cfg->bfio5_cfg->in_channel;
		}else{
			qform->channel = qform->cfg->bfio5_cfg->stft2_2.channel + qform->cfg->bfio5_cfg->stft2.channel + qform->cfg->bfio5_cfg->sp_stft2.channel;
		}
	}
	//wtk_debug("channel=%d stft2channel=%d spstft2channel=%d\n",qform->channel,qform->cfg->qform_cfg->stft2.channel,qform->cfg->qform_cfg->sp_stft2.channel);
	qform->buf = (short **)wtk_malloc(sizeof(short *) * qform->channel);
	for(i=0; i < qform->channel; i++){
		qform->buf[i] = (short *)wtk_malloc(FEED_STEP);
	}
    qform->cache_buf = wtk_strbuf_new(3200, 1.0);
	ret = 0;
end:
	if(ret != 0){
		qtk_bfio_delete(qform);
		qform = NULL;		
	}	
	return qform;
}
int qtk_bfio_delete(qtk_bfio_t *qform)
{
	int i;

	if(qform->cfg->use_bfio){
		if(qform->qform){
			wtk_bfio_delete(qform->qform);
		}
	}else if(qform->cfg->use_bfio5){
		if(qform->bfio5){
			wtk_bfio5_delete(qform->bfio5);
		}
	}
	if(qform->buf){
		for(i = 0; i < qform->channel; i++){
			wtk_free(qform->buf[i]);
		}
		wtk_free(qform->buf);
	}
    if(qform->cache_buf){
        wtk_strbuf_delete(qform->cache_buf);
    }
	wtk_free(qform);
	return 0;
}
int qtk_bfio_start(qtk_bfio_t *qform)
{
	if(qform->cfg->use_bfio){
		wtk_bfio_start(qform->qform);
		if(qform->cfg->wake_word.len > 0){
			wtk_bfio_set_wake_words(qform->qform, qform->cfg->wake_word.data, qform->cfg->wake_word.len);
		}
	}else if(qform->cfg->use_bfio5){
		wtk_bfio5_start(qform->bfio5);
		if(qform->cfg->wake_word.len > 0){
			wtk_bfio5_set_wake_words(qform->bfio5, qform->cfg->wake_word.data, qform->cfg->wake_word.len);
		}
	}
	return 0;
}
int qtk_bfio_reset(qtk_bfio_t *qform)
{
	if(qform->cfg->use_bfio){
		wtk_bfio_reset(qform->qform);
	}else if(qform->cfg->use_bfio5){
		wtk_bfio5_reset(qform->bfio5);
	}
	return 0;
}

static int qtk_bfio_on_feed(qtk_bfio_t *qform, char *data, int bytes, int is_end)
{
	int i, j;
	short *pv = NULL;
	int len;
	// double tm=0.0;
	if(bytes > 0){
		pv = (short*)data;
		len = bytes /(qform->channel * sizeof(short));
		for(i = 0; i < len; ++i){
			for(j = 0; j < qform->channel; ++j){
				qform->buf[j][i] = pv[i * qform->channel + j];
			}
		}
		// tm = time_get_ms();
		if(qform->cfg->use_bfio){
			wtk_bfio_feed(qform->qform, qform->buf, len, 0);
		}else if(qform->cfg->use_bfio5){
			wtk_bfio5_feed(qform->bfio5, qform->buf, len, 0);
		}
		// tm = time_get_ms() - tm;
		// if(tm > len/16.0)
		// {
		// 	wtk_debug("=================>>>>>>>>bfio feed time:%f  bytes=%d\n", tm, bytes);
		// }
	}
	if(is_end){
		if(qform->cfg->use_bfio){
			wtk_bfio_feed(qform->qform, NULL, 0, 1);
		}else if(qform->cfg->use_bfio5){
			wtk_bfio5_feed(qform->bfio5, NULL, 0, 1);
		}
	}
	return 0;
}

int qtk_bfio_feed(qtk_bfio_t *qform, char *data, int len, int is_end)
{
	int pos = 0;
	int step = 0;
	int flen;
	
	step = FEED_STEP * qform->channel;
	while(pos < len){
		flen = min(step, len - pos);
		qtk_bfio_on_feed(qform, data + pos, flen, 0);
		pos += flen;
	} 
	if(is_end){
		qtk_bfio_on_feed(qform, NULL, 0, 1);
	}
	return 0;
}
void qtk_bfio_set_notify(qtk_bfio_t *qform, void *ths, qtk_bfio_notify_f notify)
{
	qform->ths = ths;
	qform->notify = notify;
}

void qtk_bfio_set_notify2(qtk_bfio_t *qform, void *ths, qtk_engine_notify_f notify)
{
	qform->eths = ths;
	qform->enotify = notify;
}

void qtk_bfio5_on_bfio(qtk_bfio_t *bfio, wtk_bfio5_cmd_t cmd, short *data, int len){
    qtk_var_t var;
    int b = 0;

    switch (cmd) {
    case WTK_BFIO5_VAD_START:
        // wtk_debug("=================> vad start\n");
        var.type = QTK_SPEECH_START;
        b = 1;
        break;
    case WTK_BFIO5_VAD_DATA:
		if(bfio->cancel_len > 0){
			if(len > bfio->cancel_len){
				len = len - bfio->cancel_len;
				bfio->cancel_len = 0;
			}else{
				bfio->cancel_len = bfio->cancel_len - len;
				len = 0;
			}
		}
		// if (bfio->enotify) {
		// 	qtk_var_t var2;
		// 	var2.type = QTK_AUDIO_RANGE_IDX;
		// 	var2.v.i = 1;
		// 	bfio->enotify(bfio->eths, &var2);
		// }
		if(len > 0){
			if(bfio->cfg->echo_shift != 1.0){
				qtk_data_change_vol((char *)data, len << 1, bfio->cfg->echo_shift);
			}
			var.type = QTK_SPEECH_DATA_PCM;
			var.v.str.data = (char *)data;
			var.v.str.len = len << 1;
			b = 1;
		}
        break;
    case WTK_BFIO5_VAD_END:
        // wtk_debug("=================> vad end\n");
        var.type = QTK_SPEECH_END;
        b = 1;
        break;
    case WTK_BFIO5_WAKE:
        // wtk_debug("=================> wake\n");
        var.type = QTK_AEC_WAKE;
        b = 1;
        break;
    case WTK_BFIO5_VAD_CANCEL:
        // wtk_debug("=================> vad cancel len=%d\n",len);
        var.type = QTK_AEC_CANCEL_DATA;
        var.v.i = len << 1;
		bfio->cancel_len = len;
        b = 1;
        break;
    case WTK_BFIO5_WAKE_SSL:
        var.type = QTK_AEC_DIRECTION;
		var.v.ii.nbest = 0;
		var.v.ii.nspecsum=0.0;
        var.v.ii.theta = bfio->bfio5->wake_theta;
        var.v.ii.phi = 0;
        b = 1;
        break;
    case WTK_BFIO5_SPEECH_END:
        // wtk_debug("=================> WTK_BFIO5_SPEECH_END\n");
        break;
    case WTK_BFIO5_SIL_END:
        // wtk_debug("=================> WTK_BFIO5_SIL_END\n");
        break;
    default:
        break;
    }

    if (b == 1 && bfio->enotify) {
        bfio->enotify(bfio->eths, &var);
    }
}

void qtk_bfio_on_bfio(qtk_bfio_t *bfio, wtk_bfio_cmd_t cmd, short *data, int len) {
    qtk_var_t var;
    int b = 0;

    switch (cmd) {
    case WTK_BFIO_VAD_START:
        // wtk_debug("=================> vad start\n");
        var.type = QTK_SPEECH_START;
        b = 1;
        break;
    case WTK_BFIO_VAD_DATA:
		if(bfio->cancel_len > 0){
			if(len > bfio->cancel_len){
				len = len - bfio->cancel_len;
				bfio->cancel_len = 0;
			}else{
				bfio->cancel_len = bfio->cancel_len - len;
				len = 0;
			}
		}
		// if (bfio->enotify) {
		// 	qtk_var_t var2;
		// 	var2.type = QTK_AUDIO_RANGE_IDX;
		// 	var2.v.i = 1;
		// 	bfio->enotify(bfio->eths, &var2);
		// }
		if(len > 0){
			if(bfio->cfg->echo_shift != 1.0){
				qtk_data_change_vol((char *)data, len << 1, bfio->cfg->echo_shift);
			}
			var.type = QTK_SPEECH_DATA_PCM;
			var.v.str.data = (char *)data;
			var.v.str.len = len << 1;
			b = 1;
		}
        break;
    case WTK_BFIO_VAD_END:
        // wtk_debug("=================> vad end\n");
        var.type = QTK_SPEECH_END;
        b = 1;
        break;
    case WTK_BFIO_WAKE:
        // wtk_debug("=================> wake\n");
        var.type = QTK_AEC_WAKE;
        b = 1;
        break;
    case WTK_BFIO_VAD_CANCEL:
        // wtk_debug("=================> vad cancel len=%d\n",len);
        var.type = QTK_AEC_CANCEL_DATA;
        var.v.i = len << 1;
		bfio->cancel_len = len;
        b = 1;
        break;
    case WTK_BFIO_WAKE_SSL:
        var.type = QTK_AEC_DIRECTION;
		var.v.ii.nbest = 0;
		var.v.ii.nspecsum=0.0;
        var.v.ii.theta = bfio->qform->wake_theta;
        var.v.ii.phi = 0;
        b = 1;
        break;
    case WTK_BFIO_SPEECH_END:
        // wtk_debug("=================> WTK_BFIO_SPEECH_END\n");
        break;
    case WTK_BFIO_SIL_END:
        // wtk_debug("=================> WTK_BFIO_SIL_END\n");
        break;
    default:
        break;
    }

    if (b == 1 && bfio->enotify) {
        bfio->enotify(bfio->eths, &var);
    }
}