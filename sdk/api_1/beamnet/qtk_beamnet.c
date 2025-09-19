#include "qtk_beamnet.h"

#define FEED_STEP (32 * 16 * 2)
void qtk_beamnet_on_qform9(qtk_beamnet_t *qform, short *data, int len);

qtk_beamnet_t *qtk_beamnet_new(qtk_beamnet_cfg_t *cfg)
{
	qtk_beamnet_t *qform;
	int i, ret;

	qform = (qtk_beamnet_t *)wtk_calloc(1, sizeof(*qform));
	qform->cfg = cfg;
	if(qform->cfg->use_beamnet)
	{
		if(qform->cfg->qform_cfg){
			qform->qform = wtk_beamnet_new(qform->cfg->qform_cfg);
			if(!qform->qform){
				ret = -1;
				goto end;
			}	
		}
		wtk_beamnet_set_notify(qform->qform, qform, (wtk_beamnet_notify_f)qtk_beamnet_on_qform9);
		qform->channel = qform->cfg->qform_cfg->channel;
	}
	if(qform->cfg->use_beamnet2)
	{
		if(qform->cfg->qform2_cfg){
			qform->qform2 = wtk_beamnet2_new(qform->cfg->qform2_cfg);
			if(!qform->qform2){
				ret = -1;
				goto end;
			}
		}
		wtk_beamnet2_set_notify(qform->qform2, qform, (wtk_beamnet2_notify_f)qtk_beamnet_on_qform9);
		qform->channel = qform->cfg->qform2_cfg->channel;
	}
	if(qform->cfg->use_beamnet3)
	{
		if(qform->cfg->qform3_cfg){
			qform->qform3 = wtk_beamnet3_new(qform->cfg->qform3_cfg);
			if(!qform->qform3){
				ret = -1;
				goto end;
			}
		}
		wtk_beamnet3_set_notify(qform->qform3, qform, (wtk_beamnet3_notify_f)qtk_beamnet_on_qform9);
		qform->channel = qform->cfg->qform3_cfg->channel;
	}
	qform->buf = (short **)wtk_malloc(sizeof(short *) * qform->channel);
	for(i=0; i < qform->channel; i++){
		qform->buf[i] = (short *)wtk_malloc(FEED_STEP);
	}
    qform->cache_buf = wtk_strbuf_new(3200, 1.0);
	ret = 0;
end:
	if(ret != 0){
		qtk_beamnet_delete(qform);
		qform = NULL;		
	}	
	return qform;
}
int qtk_beamnet_delete(qtk_beamnet_t *qform)
{
	int i;

	if(qform->cfg->use_beamnet)
	{
		if(qform->qform){
			wtk_beamnet_delete(qform->qform);
		}
	}
	if(qform->cfg->use_beamnet2)
	{
		if(qform->qform2){
			wtk_beamnet2_delete(qform->qform2);
		}
	}
	if(qform->cfg->use_beamnet3)
	{
		if(qform->qform3){
			wtk_beamnet3_delete(qform->qform3);
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
int qtk_beamnet_start(qtk_beamnet_t *qform)
{
	printf("fix_theta = %f, phi = %f\n", qform->cfg->fix_theta, qform->cfg->phi);
	if(qform->cfg->use_beamnet)
	{
		wtk_beamnet_start(qform->qform, qform->cfg->fix_theta, qform->cfg->phi);
	}
	if(qform->cfg->use_beamnet2)
	{
		wtk_beamnet2_start(qform->qform2, qform->cfg->fix_theta, qform->cfg->phi);
	}
	if(qform->cfg->use_beamnet3)
	{
		wtk_beamnet3_start(qform->qform3, qform->cfg->fix_theta, qform->cfg->phi);
	}
	return 0;
}
int qtk_beamnet_reset(qtk_beamnet_t *qform)
{
	if(qform->cfg->use_beamnet)
	{
		wtk_beamnet_reset(qform->qform);
	}
	if(qform->cfg->use_beamnet2)
	{
		wtk_beamnet2_reset(qform->qform2);
	}
	if(qform->cfg->use_beamnet3)
	{
		wtk_beamnet3_reset(qform->qform3);
	}
	return 0;
}

static int qtk_beamnet_on_feed(qtk_beamnet_t *qform, char *data, int bytes, int is_end)
{
	int i, j;
	short *pv = NULL;
	int len;
	if(bytes > 0){
		pv = (short*)data;
		len = bytes /(qform->channel * sizeof(short));
		// for(i = 0; i < len; ++i){
		// 	for(j = 0; j < qform->channel; ++j){
		// 		qform->buf[j][i] = pv[i * qform->channel + j];
		// 	}
		// }
		if(qform->cfg->use_beamnet)
		{
			wtk_beamnet_feed(qform->qform, pv, len, 0);
		}
		if(qform->cfg->use_beamnet2)
		{
			wtk_beamnet2_feed(qform->qform2, pv, len, 0);
		}
		if(qform->cfg->use_beamnet3)
		{
			wtk_beamnet3_feed(qform->qform3, pv, len, 0);
		}
	}
	if(is_end){
		if(qform->cfg->use_beamnet)
		{
			wtk_beamnet_feed(qform->qform, NULL, 0, 1);
		}
		if(qform->cfg->use_beamnet2)
		{
			wtk_beamnet2_feed(qform->qform2, NULL, 0, 1);
		}
		if(qform->cfg->use_beamnet3)
		{
			wtk_beamnet3_feed(qform->qform3, NULL, 0, 1);
		}
	}
	return 0;
}




int qtk_beamnet_feed(qtk_beamnet_t *qform, char *data, int len, int is_end)
{
	int pos = 0;
	int step = 0;
	int flen;
	
	step = FEED_STEP * qform->channel;
	while(pos < len){
		flen = min(step, len - pos);
		qtk_beamnet_on_feed(qform, data + pos, flen, 0);
		pos += flen;
	} 
	if(is_end){
		qtk_beamnet_on_feed(qform, NULL, 0, 1);
	}
	return 0;
}
void qtk_beamnet_set_notify(qtk_beamnet_t *qform, void *ths, qtk_beamnet_notify_f notify)
{
	qform->ths = ths;
	qform->notify = notify;
}

void qtk_beamnet_set_notify2(qtk_beamnet_t *qform, void *ths, qtk_engine_notify_f notify)
{
	qform->eths = ths;
	qform->enotify = notify;
}

//int cache_count;
#define CACHE_STEP  (8)
void qtk_beamnet_on_qform9(qtk_beamnet_t *qform, short *data, int len)
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
