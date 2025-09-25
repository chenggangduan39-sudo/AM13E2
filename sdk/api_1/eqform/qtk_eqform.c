#include "qtk_eqform.h"
#include "sdk/codec/qtk_audio_conversion.h"

#define FEED_STEP (32 * 16 * 2)
void qtk_eqform_on_qform(qtk_eqform_t *qform, short *data, int len);

qtk_eqform_t *qtk_eqform_new(qtk_eqform_cfg_t *cfg)
{
	qtk_eqform_t *qform;
	int i, ret;

	qform = (qtk_eqform_t *)wtk_calloc(1, sizeof(*qform));
	qform->cfg = cfg;
	if(qform->cfg->eqform_cfg){
		qform->eqform = wtk_eqform_new(qform->cfg->eqform_cfg);
		if(!qform->eqform){
			ret = -1;
			goto end;
		}	
	}
	wtk_eqform_set_notify(qform->eqform, qform, (wtk_eqform_notify_f)qtk_eqform_on_qform);
	qform->channel = qform->cfg->eqform_cfg->aec.stft.channel;
#ifdef USE_802A
	qform->inchannel = qform->channel+1;
#else
	qform->inchannel = qform->channel;
#endif
	qform->buf = (short **)wtk_malloc(sizeof(short *) * qform->channel);
	for(i=0; i < qform->channel; i++){
		qform->buf[i] = (short *)wtk_malloc(FEED_STEP);
	}
    qform->cache_buf = wtk_strbuf_new(3200, 1.0);
	ret = 0;
end:
	if(ret != 0){
		qtk_eqform_delete(qform);
		qform = NULL;		
	}	
	return qform;
}
int qtk_eqform_delete(qtk_eqform_t *qform)
{
	int i;

	if(qform->eqform){
		wtk_eqform_delete(qform->eqform);
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
int qtk_eqform_start(qtk_eqform_t *qform)
{
	printf("fix_theta = %f, phi = %f\n", qform->cfg->fix_theta, qform->cfg->phi);
	wtk_eqform_start(qform->eqform, qform->cfg->fix_theta, qform->cfg->phi);
	return 0;
}
int qtk_eqform_reset(qtk_eqform_t *qform)
{
	wtk_eqform_reset(qform->eqform);
	return 0;
}

static int qtk_eqform_on_feed(qtk_eqform_t *qform, char *data, int bytes, int is_end)
{
	int i, j;
	short *pv = NULL;
	int len;

	if(qform->cfg->mic_shift != 1.0)
	{
		qtk_data_change_vol(data, bytes, qform->cfg->mic_shift);
	}

	if(bytes > 0){
		pv = (short*)data;
		len = bytes /(qform->inchannel * sizeof(short));
		for(i = 0; i < len; ++i){
			for(j = 0; j < qform->channel; ++j){
				qform->buf[j][i] = pv[i * qform->inchannel + j];
			}
		}
		wtk_eqform_feed(qform->eqform, qform->buf, len, 0);
	}
	if(is_end){
		wtk_eqform_feed(qform->eqform, NULL, 0, 1);
	}
	return 0;
}

int qtk_eqform_feed(qtk_eqform_t *qform, char *data, int len, int is_end)
{
	int pos = 0;
	int step = 0;
	int flen;
	
	step = FEED_STEP * qform->inchannel;
	while(pos < len){
		flen = min(step, len - pos);
		qtk_eqform_on_feed(qform, data + pos, flen, 0);
		pos += flen;
	} 
	if(is_end){
		qtk_eqform_on_feed(qform, NULL, 0, 1);
	}
	return 0;
}
void qtk_eqform_set_notify(qtk_eqform_t *qform, void *ths, qtk_eqform_notify_f notify)
{
	qform->ths = ths;
	qform->notify = notify;
}

void qtk_eqform_set_notify2(qtk_eqform_t *qform, void *ths, qtk_engine_notify_f notify)
{
	qform->eths = ths;
	qform->enotify = notify;
}

//int cache_count;
#define CACHE_STEP  (8)
void qtk_eqform_on_qform(qtk_eqform_t *qform, short *data, int len)
{
	qtk_var_t var;

	if(qform->cfg->echo_shit != 1.0)
	{
		qtk_data_change_vol((char *)data, len <<1, qform->cfg->echo_shit);
	}

	if(qform->enotify)
	{
		var.type = QTK_SPEECH_DATA_PCM;
		var.v.str.data = (char *)data;
		var.v.str.len = len << 1;
		qform->enotify(qform->eths, &var);
	}
#if 1
	if(qform->notify){
		qform->notify(qform->ths, (char *)data, len<<1);
	}
#else
	cache_count++;
	wtk_strbuf_push(qform->cache_buf, (char *)data, len * 2);
	if(cache_count < CACHE_STEP){
		return ;
	}
	// printf("cache_count = %d  pos = %d\n", cache_count, qform->cache->pos);
    if(qform->notify){
		qform->notify(qform->ths, qform->cache_buf->data, qform->cache_buf->pos);
        wtk_strbuf_reset(qform->cache_buf);
		cache_count = 0;
	}
#endif


}
