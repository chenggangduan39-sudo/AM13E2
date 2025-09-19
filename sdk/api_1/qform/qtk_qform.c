#include "qtk_qform.h"

#define FEED_STEP (32 * 16 * 2)
void qtk_qform_on_qform9(qtk_qform_t *qform, short *data, int len);

qtk_qform_t *qtk_qform_new(qtk_qform_cfg_t *cfg)
{
	qtk_qform_t *qform;
	int i, ret;

	qform = (qtk_qform_t *)wtk_calloc(1, sizeof(*qform));
	qform->cfg = cfg;
	if(qform->cfg->qform_cfg){
		qform->qform = wtk_qform9_new(qform->cfg->qform_cfg);
		if(!qform->qform){
			ret = -1;
			goto end;
		}	
	}
	wtk_qform9_set_notify(qform->qform, qform, (wtk_qform9_notify_f)qtk_qform_on_qform9);
	qform->channel = qform->cfg->qform_cfg->stft2.channel;
	qform->buf = (short **)wtk_malloc(sizeof(short *) * qform->channel);
	for(i=0; i < qform->channel; i++){
		qform->buf[i] = (short *)wtk_malloc(FEED_STEP);
	}
    qform->cache_buf = wtk_strbuf_new(3200, 1.0);
	ret = 0;
end:
	if(ret != 0){
		qtk_qform_delete(qform);
		qform = NULL;		
	}	
	return qform;
}
int qtk_qform_delete(qtk_qform_t *qform)
{
	int i;

	if(qform->qform){
		wtk_qform9_delete(qform->qform);
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
int qtk_qform_start(qtk_qform_t *qform)
{
	printf("fix_theta = %f, phi = %f\n", qform->cfg->fix_theta, qform->cfg->phi);
	wtk_qform9_start(qform->qform, qform->cfg->fix_theta, qform->cfg->phi);
	return 0;
}
int qtk_qform_reset(qtk_qform_t *qform)
{
	wtk_qform9_reset(qform->qform);
	return 0;
}

static int qtk_qform_on_feed(qtk_qform_t *qform, char *data, int bytes, int is_end)
{
	int i, j;
	short *pv = NULL;
	int len;
	if(bytes > 0){
		pv = (short*)data;
		len = bytes /(qform->channel * sizeof(short));
		for(i = 0; i < len; ++i){
			for(j = 0; j < qform->channel; ++j){
				qform->buf[j][i] = pv[i * qform->channel + j];
			}
		}
		wtk_qform9_feed(qform->qform, qform->buf, len, 0);
	}
	if(is_end){
		wtk_qform9_feed(qform->qform, NULL, 0, 1);
	}
	return 0;
}




int qtk_qform_feed(qtk_qform_t *qform, char *data, int len, int is_end)
{
	int pos = 0;
	int step = 0;
	int flen;
	
	step = FEED_STEP * qform->channel;
	while(pos < len){
		flen = min(step, len - pos);
		qtk_qform_on_feed(qform, data + pos, flen, 0);
		pos += flen;
	} 
	if(is_end){
		qtk_qform_on_feed(qform, NULL, 0, 1);
	}
	return 0;
}
void qtk_qform_set_notify(qtk_qform_t *qform, void *ths, qtk_qform_notify_f notify)
{
	qform->ths = ths;
	qform->notify = notify;
}

void qtk_qform_set_notify2(qtk_qform_t *qform, void *ths, qtk_engine_notify_f notify)
{
	qform->eths = ths;
	qform->enotify = notify;
}

//int cache_count;
#define CACHE_STEP  (8)
void qtk_qform_on_qform9(qtk_qform_t *qform, short *data, int len)
{
	qtk_var_t var;

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
