#include "qtk_ssl.h"

#define FEED_STEP (32 * 32 * 2)
void qtk_ssl_on_data(qtk_ssl_t *vb, wtk_ssl_extp_t *nbest_extp);
void qtk_ssl_on_gainnet_ssl(qtk_ssl_t *vb, float ts, float te, wtk_ssl2_extp_t *nbest_extp, int nbest);

qtk_ssl_t *qtk_ssl_new(qtk_ssl_cfg_t *cfg)
{
	qtk_ssl_t *vb;
	int i, ret;

	vb = (qtk_ssl_t *)wtk_calloc(1, sizeof(*vb));
	vb->cfg = cfg;
	vb->ths=NULL;
	vb->notify=NULL;
	vb->eths=NULL;
	vb->enotify=NULL;
	vb->ssl=NULL;
	vb->gainnet_ssl=NULL;
	vb->buf=NULL;
	vb->cache_buf = NULL;
	vb->out_buf = NULL;
	vb->extp = NULL;
	vb->channel = 1;


	if(vb->cfg->use_ssl)
	{
		if(vb->cfg->ssl_cfg){
			vb->ssl = wtk_ssl_new(vb->cfg->ssl_cfg);
			if(!vb->ssl){
				ret = -1;
				goto end;
			}
			vb->channel = vb->cfg->ssl_cfg->stft2.channel;
		}
		wtk_debug("channel=%d\n",vb->channel);
		wtk_ssl_set_notify(vb->ssl, vb, (wtk_ssl_notify_f)qtk_ssl_on_data);
		vb->extp = (qtk_ssl_extp_t *)wtk_malloc(vb->cfg->ssl_cfg->max_extp * sizeof(qtk_ssl_extp_t));
	}
	if(vb->cfg->use_gainnet_ssl)
	{
		if(vb->cfg->gainnet_ssl_cfg)
		{
			vb->gainnet_ssl = wtk_gainnet_ssl_new(vb->cfg->gainnet_ssl_cfg);
			if(!vb->gainnet_ssl){
				ret = -1;
				goto end;
			}
			vb->channel = vb->cfg->gainnet_ssl_cfg->channel;
			wtk_gainnet_ssl_set_ssl_notify(vb->gainnet_ssl, vb, (wtk_gainnet_ssl_notify_ssl_f)qtk_ssl_on_gainnet_ssl);
		}
	}

	vb->buf = (short **)wtk_malloc(sizeof(short *) * vb->channel);
	for(i=0; i < vb->channel; i++){
		vb->buf[i] = (short *)wtk_malloc(FEED_STEP);
	}
    vb->cache_buf = wtk_strbuf_new(3200, 1.0);
	vb->out_buf = wtk_strbuf_new(1024, 1.0);

	ret = 0;
end:
	if(ret != 0){
		qtk_ssl_delete(vb);
		vb = NULL;		
	}	
	return vb;
}
int qtk_ssl_delete(qtk_ssl_t *vb)
{
	int i;
	
	if(vb->cfg->use_ssl)
	{
		if(vb->ssl)
		{
			wtk_ssl_delete(vb->ssl);
		}
	}
	if(vb->cfg->use_gainnet_ssl)
	{
		if(vb->gainnet_ssl)
		{
			wtk_gainnet_ssl_delete(vb->gainnet_ssl);
		}
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
	if(vb->extp)
	{
		wtk_free(vb->extp);
	}
	wtk_free(vb);
	return 0;
}
int qtk_ssl_start(qtk_ssl_t *vb)
{
	if(vb->cfg->use_gainnet_ssl)
	{
		if(vb->cfg->use_echoenable != -1)
		{
			wtk_gainnet_ssl_set_echoenable(vb->gainnet_ssl, vb->cfg->use_echoenable);
		}
		wtk_gainnet_ssl_start(vb->gainnet_ssl);
	}
	return 0;
}
int qtk_ssl_reset(qtk_ssl_t *vb)
{
	if(vb->cfg->use_ssl)
	{
		wtk_ssl_reset(vb->ssl);
	}
	if(vb->cfg->use_gainnet_ssl)
	{
		wtk_gainnet_ssl_reset(vb->gainnet_ssl);
	}

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

static int qtk_ssl_on_feed(qtk_ssl_t *vb, char *data, int bytes, int is_end)
{
	if(bytes > 0)
	{
		if(vb->cfg->mic_shift != 1.0f)
		{
			qtk_data_change_vol(data, bytes, vb->cfg->mic_shift);
		}

		int i, j;
		short *pv = NULL;
		int len;
		if(bytes > 0){
			if(vb->cfg->nskip > 0){
				short *pv1;
				int k;
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

			pv = (short*)data;
			len = bytes /(vb->channel * sizeof(short));
			for(i = 0; i < len; ++i){
				for(j = 0; j < vb->channel; ++j){
					vb->buf[j][i] = pv[i * vb->channel + j];
				}
			}

			if(vb->cfg->use_ssl)
			{
				wtk_ssl_feed_count(vb->ssl, vb->buf, len, 0);
			}
			if(vb->cfg->use_gainnet_ssl)
			{
				len=bytes/(vb->channel*2);
				wtk_gainnet_ssl_feed(vb->gainnet_ssl, (short *)data, len, 0);
			}
		}

	}
	if(is_end){
		if(vb->cfg->use_ssl)
		{
			wtk_ssl_feed_count(vb->ssl, NULL, 0, 1);
		}
		if(vb->cfg->use_gainnet_ssl)
		{
			wtk_gainnet_ssl_feed(vb->gainnet_ssl, NULL, 0, 1);
		}
	}
	return 0;
}

int qtk_ssl_feed(qtk_ssl_t *vb, char *data, int len, int is_end)
{
	int pos = 0;
	int step = 0;
	int flen;

	step = FEED_STEP * (vb->channel+vb->cfg->nskip);
	while(pos < len){
		flen = min(step, len-pos);
		qtk_ssl_on_feed(vb, data + pos, flen, 0);
		pos +=flen;
	}
	if(is_end){
		qtk_ssl_on_feed(vb, NULL, 0, 1);
	}

	return 0;
}
void qtk_ssl_set_notify(qtk_ssl_t *vb, void *ths, qtk_ssl_notify_f notify)
{
	vb->ths = ths;
	vb->notify = notify;
}

void qtk_ssl_set_notify2(qtk_ssl_t *vb, void *ths, qtk_engine_notify_f notify)
{
	vb->eths=ths;
	vb->enotify=notify;
}

void qtk_ssl_on_gainnet_ssl(qtk_ssl_t *vb, float ts, float te, wtk_ssl2_extp_t *nbest_extp, int nbest)
{
	qtk_var_t var;
	int i;

	// printf("nbest=%d/%f theta=%d phi=%d\n",nbest, nbest_extp[0].nspecsum, nbest_extp[0].theta, nbest_extp[0].phi);
	if(nbest > 0)
	{
		for(i=0; i<nbest; ++i)
		{
			if(nbest_extp[i].nspecsum > vb->cfg->energy_sum)
			{
				// printf("nbest=%d/%d theta=%d phi=%d\n",nbest, i, nbest_extp[i].theta, nbest_extp[i].phi);
				if (vb->enotify)
				{
					var.type = QTK_AEC_DIRECTION;
					var.v.ii.nbest = i;
					var.v.ii.theta = nbest_extp[i].theta;
					var.v.ii.phi = nbest_extp[i].phi;
					var.v.ii.nspecsum = nbest_extp[i].nspecsum;
					vb->enotify(vb->eths, &(var));
				}
			}else{
				// wtk_debug("================>>>>>>>-1\n");
				if (vb->enotify)
				{
					var.type = QTK_AEC_DIRECTION;
					var.v.ii.nbest = i;
					var.v.ii.theta = -1;
					var.v.ii.phi = -1;
					var.v.ii.nspecsum = 0.0;
					vb->enotify(vb->eths, &(var));
				}
			}
		}
	}else{
		if (vb->enotify)
		{
			var.type = QTK_AEC_DIRECTION;
			var.v.ii.nbest = 0;
			var.v.ii.theta = -1;
			var.v.ii.phi = -1;
			var.v.ii.nspecsum = 0;
			vb->enotify(vb->eths, &(var));
		}
	}
}

void qtk_ssl_on_data(qtk_ssl_t *vb, wtk_ssl_extp_t *nbest_extp)
{
	qtk_var_t var;

	if(vb->enotify)
	{
		var.type = QTK_AEC_DIRECTION;
		var.v.ii.theta=nbest_extp[0].theta;
		var.v.ii.phi = nbest_extp[0].phi;
		var.v.ii.nspecsum = nbest_extp[0].nspecsum;
		vb->enotify(vb->eths, &var);
	}
	if(vb->notify)
	{
		vb->extp[0].theta = nbest_extp[0].theta;
		vb->extp[0].phi = nbest_extp[0].phi;
		vb->extp[0].nspecsum = nbest_extp[0].nspecsum;
		vb->notify(vb->ths, vb->extp);
	}

}
