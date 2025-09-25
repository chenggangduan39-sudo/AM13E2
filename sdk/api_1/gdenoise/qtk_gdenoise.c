#include "qtk_gdenoise.h"

#define FEED_STEP (20 * 32 * 2)
void qtk_gdenoise_on_data(qtk_gdenoise_t *vb, short *data, int len, int is_end);
void qtk_gdenoise_on_aec(qtk_gdenoise_t *gd, short **data, int len, int is_end);
void qtk_gdenoise_on_gainnet_aec3(qtk_gdenoise_t *gd, short **data, short **lasty, int len, int is_end);

qtk_gdenoise_t *qtk_gdenoise_new(qtk_gdenoise_cfg_t *cfg)
{
	qtk_gdenoise_t *vb;
	int i, ret;

	vb = (qtk_gdenoise_t *)wtk_calloc(1, sizeof(*vb));
	vb->cfg = cfg;
	vb->ths=NULL;
	vb->notify=NULL;
	vb->eths=NULL;
	vb->enotify=NULL;
	vb->aec=NULL;
	vb->gdenoise=NULL;
	vb->buf=NULL;
	vb->channel = 1;

	if(cfg->use_aec)
	{
		if(vb->cfg->aec_cfg){
			vb->aec = wtk_aec_new(vb->cfg->aec_cfg);
			if(!vb->aec){
				ret = -1;
				goto end;
			}	
		}
		wtk_aec_set_notify(vb->aec, vb, (wtk_aec_notify_f)qtk_gdenoise_on_aec);
		vb->channel = vb->cfg->aec_cfg->stft.channel;
	}
	// wtk_debug("channel=%d\n",vb->channel);

	if(vb->cfg->gdenoise_cfg){
		vb->gdenoise = wtk_gainnet_denoise_new(vb->cfg->gdenoise_cfg);
		if(!vb->gdenoise){
			ret = -1;
			goto end;
		}	
	}
	wtk_gainnet_denoise_set_notify(vb->gdenoise, vb, (wtk_gainnet_denoise_notify_f)qtk_gdenoise_on_data);

	vb->buf = (short **)wtk_malloc(sizeof(short *) * vb->channel);
	for(i=0; i < vb->channel; i++){
		vb->buf[i] = (short *)wtk_malloc(FEED_STEP);
	}
    vb->cache_buf = wtk_strbuf_new(3200, 1.0);
	vb->out_buf = wtk_strbuf_new(1024, 1.0);
	ret = 0;
end:
	if(ret != 0){
		qtk_gdenoise_delete(vb);
		vb = NULL;		
	}	
	return vb;
}
int qtk_gdenoise_delete(qtk_gdenoise_t *vb)
{
	int i;
	
	if(vb->cfg->use_aec)
	{
		if(vb->aec)
		{
			wtk_aec_delete(vb->aec);
		}
	}
	if(vb->gdenoise)
	{
		wtk_gainnet_denoise_delete(vb->gdenoise);
	}
	if(vb->buf){
		for(i = 0; i < vb->channel; i++){
			wtk_free(vb->buf[i]);
		}
		wtk_free(vb->buf);
	}
    if(vb->cache_buf){
        wtk_strbuf_delete(vb->cache_buf);
    }
	if(vb->out_buf)
	{
		wtk_strbuf_delete(vb->out_buf);
	}
	wtk_free(vb);
	return 0;
}
int qtk_gdenoise_start(qtk_gdenoise_t *vb)
{
	return 0;
}
int qtk_gdenoise_reset(qtk_gdenoise_t *vb)
{
	if(vb->cfg->use_aec)
	{
		wtk_aec_reset(vb->aec);
	}
		
	wtk_gainnet_denoise_reset(vb->gdenoise);

	if(vb->out_buf)
	{
		wtk_strbuf_reset(vb->out_buf);
	}
	if(vb->cache_buf)
	{
		wtk_strbuf_reset(vb->cache_buf);
	}
	return 0;
}

static int qtk_gdenoise_on_feed(qtk_gdenoise_t *vb, char *data, int bytes, int is_end)
{
	if(bytes > 0)
	{
		if(vb->cfg->mic_shift != 1.0f)
		{
			qtk_data_change_vol(data, bytes, vb->cfg->mic_shift);
		}
		if(vb->cfg->nskip > 0){
			short *pv,*pv1;
			int i,j,k,len;
			int pos,pos1;
			int b;
			int channel=vb->channel+vb->cfg->nskip;

			pv = pv1 = (short*)data;
			pos = pos1 = 0;
			len = bytes / (2 * channel);
			for(i=0;i < len; ++i){
				for(j=0;j < channel; ++j) {
					b = 0;
					for(k=0;k<vb->cfg->nskip;++k) {
						if(j == vb->cfg->skip_channels[k]) {
							b = 1;
						}
					}
					if(b) {
						++pos1;
					} else {
						pv[pos++] = pv1[pos1++];
					}
				}
			}
			bytes = pos << 1;
		}

		int i, j;
		short *pv = NULL;
		int len;
		if(bytes > 0){
			pv = (short*)data;
			len = bytes /(vb->channel * sizeof(short));
			for(i = 0; i < len; ++i){
				for(j = 0; j < vb->channel; ++j){
					vb->buf[j][i] = pv[i * vb->channel + j];
				}
			}

			if(vb->cfg->use_aec)
			{
				wtk_aec_feed(vb->aec, vb->buf, len, 0);
			}else{
				wtk_gainnet_denoise_feed(vb->gdenoise, (short *)data, bytes>>1, 0);
			}
		}

	}
	if(is_end){
		if(vb->cfg->use_aec)
		{
			wtk_aec_feed(vb->aec, NULL, 0, 1);
		}else{
			wtk_gainnet_denoise_feed(vb->gdenoise, NULL, 0, 1);
		}
	}
	return 0;
}

int qtk_gdenoise_feed(qtk_gdenoise_t *vb, char *data, int len, int is_end)
{
#if 0
	int pos = 0;
	int step = 0;
	int flen;

	step = FEED_STEP * (vb->channel+vb->cfg->nskip);
	while(pos < len){
		flen = min(step, len-pos);
		qtk_gdenoise_on_feed(vb, data + pos, flen, 0);
		pos +=flen;
	}
	if(is_end){
		qtk_gdenoise_on_feed(vb, NULL, 0, 1);
	}
#else
	qtk_gdenoise_on_feed(vb, data, len, is_end);
#endif
	return 0;
}
void qtk_gdenoise_set_notify(qtk_gdenoise_t *vb, void *ths, qtk_gdenoise_notify_f notify)
{
	vb->ths = ths;
	vb->notify = notify;
}

void qtk_gdenoise_set_notify2(qtk_gdenoise_t *vb, void *ths, qtk_engine_notify_f notify)
{
	vb->eths=ths;
	vb->enotify=notify;
}

void qtk_gdenoise_on_aec(qtk_gdenoise_t *gd, short **data, int len, int is_end)
{
	if(len > 0)
	{
		wtk_gainnet_denoise_feed(gd->gdenoise, data[0], len, 0);
	}
	if(is_end)
	{
		wtk_gainnet_denoise_feed(gd->gdenoise, NULL, 0, 1);
	}
}

void qtk_gdenoise_on_gainnet_aec3(qtk_gdenoise_t *gd, short **data, short **lasty, int len, int is_end)
{
	if(len > 0)
	{
		wtk_gainnet_denoise_feed(gd->gdenoise, data[0], len, 0);
	}
	if(is_end)
	{
		wtk_gainnet_denoise_feed(gd->gdenoise, NULL, 0, 1);
	}
}


//int cache_count;
//#define CACHE_STEP  (8)
void qtk_gdenoise_on_data(qtk_gdenoise_t *vb, short *data, int len, int is_end)
{
	qtk_var_t var;
	wtk_strbuf_push(vb->out_buf, (char *)data, len<<1);

#if 1
	while(vb->out_buf->pos > 320)
	{
		if(vb->cfg->echo_shift != 1.0f)
		{
			qtk_data_change_vol((char *)data, len<<1, vb->cfg->echo_shift);
		}
		if(vb->notify){
			vb->notify(vb->ths, vb->out_buf->data, 320);
		}
		if(vb->enotify)
		{
			var.type = QTK_SPEECH_DATA_PCM;
			var.v.str.data = vb->out_buf->data;
			var.v.str.len = 320;
			vb->enotify(vb->eths, &var);
		}
		wtk_strbuf_pop(vb->out_buf, NULL, 320);
	}
#else
	cache_count++;
	wtk_strbuf_push(vb->cache_buf, (char *)data, len * 2);
	if(cache_count < CACHE_STEP){
		return ;
	}
	// printf("cache_count = %d  pos = %d\n", cache_count, vb->cache->pos);
    if(vb->notify){
		vb->notify(vb->ths, vb->cache_buf->data, vb->cache_buf->pos);
        wtk_strbuf_reset(vb->cache_buf);
		cache_count = 0;
	}
#endif


}
