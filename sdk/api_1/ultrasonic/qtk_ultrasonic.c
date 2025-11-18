#include "qtk_ultrasonic.h"
int qtk_ultrasonic_on_track(qtk_ult_track_post_t *post, int nframe, int nobj, qtk_ult_track_result_t *res);
#define FEED_STEP (32 * 32 * 2)

qtk_ultrasonic_t *qtk_ultrasonic_new(qtk_ultrasonic_cfg_t *cfg)
{
	qtk_ultrasonic_t *vb;
	int i, ret;

	vb = (qtk_ultrasonic_t *)wtk_calloc(1, sizeof(*vb));
	vb->cfg = cfg;
	vb->ths=NULL;
	vb->notify=NULL;
	vb->eths=NULL;
	vb->enotify=NULL;
	vb->ultevm=NULL;
	vb->ult_track = NULL;
	vb->buf=NULL;
	vb->iwav=NULL;
	vb->owav=NULL;

	if(vb->cfg->use_ultevm)
	{
		if(vb->cfg->ultevm_cfg){
			vb->ultevm = qtk_ultevm2_new(vb->cfg->ultevm_cfg);
			if(!vb->ultevm){
				ret = -1;
				goto end;
			}
			vb->channel = 1;
		}
	}
	if(vb->cfg->use_ult_track)
	{
		qtk_ult_track_post_cfg_init(&vb->post_cfg);
		vb->post = qtk_ult_track_post_new(&vb->post_cfg);

		if(vb->cfg->ult_track_cfg){
			vb->ult_track = qtk_ult_track_new(vb->cfg->ult_track_cfg);
			if(!vb->ult_track){
				ret = -1;
				goto end;
			}
			vb->channel = vb->cfg->ult_track_cfg->channel;
			qtk_ult_track_set_notifier(vb->ult_track, vb->post,(qtk_ult_track_notifier_t)qtk_ultrasonic_on_track);
		}
	}
	wtk_debug("channel=%d\n",vb->channel);

	vb->buf = (short **)wtk_malloc(sizeof(short *) * vb->channel);
	for(i=0; i < vb->channel; i++){
		vb->buf[i] = (short *)wtk_malloc(FEED_STEP);
	}
	vb->feed_buf = wtk_strbuf_new(1024, 1.0);

	if(vb->cfg->use_log_wav)
	{
		vb->iwav=wtk_wavfile_new(16000);
		wtk_wavfile_open(vb->iwav,vb->cfg->input_fn);
		wtk_wavfile_set_channel2(vb->iwav, vb->channel, 2);
	
		vb->owav=wtk_wavfile_new(16000);
		wtk_wavfile_open(vb->owav,vb->cfg->out_fn);
		wtk_wavfile_set_channel2(vb->owav, 1, 2);
	}

	ret = 0;
end:
	if(ret != 0){
		qtk_ultrasonic_delete(vb);
		vb = NULL;		
	}	
	return vb;
}
int qtk_ultrasonic_delete(qtk_ultrasonic_t *vb)
{
	int i;
	
	if(vb->cfg->use_ultevm)
	{
		if(vb->ultevm)
		{
			qtk_ultevm2_delete(vb->ultevm);
		}
	}
	if(vb->cfg->use_ult_track)
	{
		if(vb->ult_track)
		{
			qtk_ult_track_delete(vb->ult_track);
		}
		if(vb->post)
		{
			qtk_ult_track_post_delete(vb->post);
		}
		qtk_ult_track_post_cfg_clean(&vb->post_cfg);
	}
	if(vb->buf){
		for(i = 0; i < vb->channel; i++){
			wtk_free(vb->buf[i]);
		}
		wtk_free(vb->buf);
	}

	if(vb->feed_buf)
	{
		wtk_strbuf_delete(vb->feed_buf);
	}
	if(vb->iwav)
	{
		wtk_wavfile_delete(vb->iwav);
	}
	if(vb->owav)
	{
		wtk_wavfile_delete(vb->owav);
	}

	wtk_free(vb);
	return 0;
}
int qtk_ultrasonic_start(qtk_ultrasonic_t *vb)
{
	return 0;
}
int qtk_ultrasonic_reset(qtk_ultrasonic_t *vb)
{
	return 0;
}

static int qtk_ultrasonic_on_feed(qtk_ultrasonic_t *vb, char *data, int bytes, int is_end)
{
	static int tb=0;
	if(bytes > 0)
	{
		if(vb->cfg->mic_shift != 1.0f)
		{
			qtk_data_change_vol(data, bytes, vb->cfg->mic_shift);
		}
		wtk_strbuf_reset(vb->feed_buf);
		wtk_strbuf_push(vb->feed_buf, data, bytes);
		short *pv,*pv1;
		int i,j,k,len;
		if(vb->cfg->nskip > 0){
			int pos,pos1;
			int b;
			int channel=vb->channel+vb->cfg->nskip;

			pv = pv1 = (short*)(vb->feed_buf->data);
			pos = pos1 = 0;
			len = vb->feed_buf->pos / (2 * channel);
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
			vb->feed_buf->pos = pos << 1;
		}
		if(vb->cfg->use_log_wav)
		{
			if(vb->iwav)
			{
				tb+=vb->feed_buf->pos;
				//wtk_debug("==========================>>>>>>%d/%d/%d/%f\n",bytes,vb->feed_buf->pos,tb,tb/(3.0*32));
				wtk_wavfile_write(vb->iwav, vb->feed_buf->data, vb->feed_buf->pos);
			}			
		}

		if(vb->cfg->use_ultevm)
		{
			qtk_ultevm2_feed(vb->ultevm, (short *)(vb->feed_buf->data), (vb->feed_buf->pos)/(vb->channel*sizeof(short)));
			if(vb->enotify)
			{
				qtk_var_t var;
				var.type = QTK_ULTEVM_TYPE;
				var.v.i = vb->ultevm->active;
				vb->enotify(vb->eths, &var);
			}
			// wtk_debug("====>active = %d\n",vb->ultevm->active);
		}
		if(vb->cfg->use_ult_track)
		{
			if(bytes > 0){
				pv = (short*)(vb->feed_buf->data);
				len = vb->feed_buf->pos /(vb->channel * sizeof(short));
				for(i = 0; i < len; ++i){
					for(j = 0; j < vb->channel; ++j){
						vb->buf[j][i] = pv[i * vb->channel + j];
					}
				}
			}
			qtk_ult_track_feed(vb->ult_track, vb->buf, len);
		}
	}
	if(is_end){
		if(vb->cfg->use_ultevm)
		{
			qtk_ultevm2_feed_end(vb->ultevm);
		}
		if(vb->cfg->use_ult_track)
		{
			qtk_ult_track_feed_end(vb->ult_track);
		}
	}
	return 0;
}

int qtk_ultrasonic_feed(qtk_ultrasonic_t *vb, char *data, int len, int is_end)
{
#if 1
	int pos = 0;
	int step = 0;
	int flen;

	step = FEED_STEP * (vb->channel+vb->cfg->nskip);
	while(pos < len){
		flen = min(step, len-pos);
		qtk_ultrasonic_on_feed(vb, data + pos, flen, 0);
		pos +=flen;
	}
	if(is_end){
		qtk_ultrasonic_on_feed(vb, NULL, 0, 1);
	}
#else
	qtk_ultrasonic_on_feed(vb, data, len, is_end);
#endif
	return 0;
}
void qtk_ultrasonic_set_notify(qtk_ultrasonic_t *vb, void *ths, qtk_ultrasonic_notify_f notify)
{
	vb->ths = ths;
	vb->notify = notify;
}

void qtk_ultrasonic_set_notify2(qtk_ultrasonic_t *vb, void *ths, qtk_engine_notify_f notify)
{
	vb->eths=ths;
	vb->enotify=notify;
}

int qtk_ultrasonic_get_signal(qtk_ultrasonic_t *vb, short **data)
{
	if(vb->cfg->use_ultevm)
	{
		return qtk_ultevm2_get_signal(vb->ultevm, data);
	}
	return 0;
}

int qtk_ultrasonic_on_track(qtk_ult_track_post_t *post, int nframe, int nobj, qtk_ult_track_result_t *res)
{
	int i;
    for (i = 0; i < nobj; i++) {
        printf("%d:%f,%f,%f\n", nframe, res[i].r, res[i].theta, res[i].y_theta);
    }
    if (nobj == 1) {
        qtk_ult_track_post_feed(post, res);
    }
}
