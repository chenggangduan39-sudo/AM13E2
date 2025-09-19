#include "qtk_gainnetbf.h"

#define FEED_STEP (32 * 32 * 2)
void qtk_gainnetbf_on_data(qtk_gainnetbf_t *vb, short *data, int len, int is_end);
void qtk_gainnetbf3_on_data(qtk_gainnetbf_t *vb, short *data, int len, int is_end);
void qtk_gainnetbf4_on_data(qtk_gainnetbf_t *vb, short *data, int len);
void qtk_gainnetbf6_on_data(qtk_gainnetbf_t *vb, short *data, int len);
void qtk_gainnetbf_mix_speech_on_data(qtk_gainnetbf_t * vb, short *data, int len);
void qtk_gainnetbf_on_aec(qtk_gainnetbf_t *gd, short **data, int len, int is_end);
void qtk_gainnetbf_on_gainnet_aec(qtk_gainnetbf_t *gd, short **data, short **lasty, int len, int is_end);
void qtk_gainnetbf_on_ssl(qtk_gainnetbf_t *vb,wtk_ssl2_extp_t *nbest_extp, int nbest);
void qtk_gainnetbf4_on_ssl(qtk_gainnetbf_t *vb, float ts, float te, wtk_ssl2_extp_t *nbest_extp, int nbest);
void qtk_rtjoin2_on(qtk_gainnetbf_t *vb, short *data, int len);
qtk_gainnetbf_t *qtk_gainnetbf_new(qtk_gainnetbf_cfg_t *cfg)
{
	qtk_gainnetbf_t *vb;
	int i, ret;

	vb = (qtk_gainnetbf_t *)wtk_calloc(1, sizeof(*vb));
	vb->cfg = cfg;
	vb->ths=NULL;
	vb->notify=NULL;
	vb->eths=NULL;
	vb->enotify=NULL;
	vb->gainnet_bf=NULL;
	vb->gainnet_bf3=NULL;
	vb->gainnet_bf6=NULL;
	vb->rtjoin2 = NULL;
	vb->buf=NULL;
	vb->energy = 0;
	vb->zdata = NULL;
	vb->energy_cnt =0;
	vb->iwav=NULL;
	vb->owav=NULL;
	wtk_debug("------------------------------>>>>>>>>>>>>>>>>\n");
	if(vb->cfg->use_gainnet_bf)
	{
		if(vb->cfg->gainnet_bf_cfg){
			vb->gainnet_bf = wtk_gainnet_bf_new(vb->cfg->gainnet_bf_cfg);
			if(!vb->gainnet_bf){
				ret = -1;
				goto end;
			}	
			wtk_gainnet_bf_set_notify(vb->gainnet_bf, vb, (wtk_gainnet_bf_notify_f)qtk_gainnetbf_on_data);
			wtk_gainnet_bf_set_ssl_notify(vb->gainnet_bf, vb, (wtk_gainnet_bf_notify_ssl_f)qtk_gainnetbf_on_ssl);
		}
		vb->channel = vb->cfg->gainnet_bf_cfg->channel;
	}else if(vb->cfg->use_gainnet_bf3)
	{
		if(vb->cfg->gainnet_bf3_cfg){
			vb->gainnet_bf3 = wtk_gainnet_bf3_new(vb->cfg->gainnet_bf3_cfg);
			if(!vb->gainnet_bf3){
				ret = -1;
				goto end;
			}
			wtk_gainnet_bf3_set_notify(vb->gainnet_bf3, vb, (wtk_gainnet_bf3_notify_f)qtk_gainnetbf3_on_data);
			wtk_gainnet_bf3_set_ssl_notify(vb->gainnet_bf3, vb, (wtk_gainnet_bf3_notify_ssl_f)qtk_gainnetbf_on_ssl);
		}
		vb->channel = vb->cfg->gainnet_bf3_cfg->channel;
		// wtk_debug("====================>>>channel=%d/%d\n",vb->cfg->gainnet_bf3_cfg->channel,vb->channel);
	}else if(vb->cfg->use_gainnet_bf4)
	{
		if(vb->cfg->gainnet_bf4_cfg){
			if(vb->cfg->use_ssl_delay != -1)
			{
				vb->cfg->gainnet_bf4_cfg->use_ssl_delay = vb->cfg->use_ssl_delay;
			}
			vb->gainnet_bf4 = wtk_gainnet_bf4_new(vb->cfg->gainnet_bf4_cfg);
			if(!vb->gainnet_bf4){
				ret = -1;
				goto end;
			}	
			wtk_gainnet_bf4_set_notify(vb->gainnet_bf4, vb, (wtk_gainnet_bf4_notify_f)qtk_gainnetbf4_on_data);
			wtk_gainnet_bf4_set_ssl_notify(vb->gainnet_bf4, vb, (wtk_gainnet_bf4_notify_ssl_f)qtk_gainnetbf4_on_ssl);
		}
		vb->channel = vb->cfg->gainnet_bf4_cfg->channel;
		// wtk_debug("====================>>>channel=%d/%d\n",vb->cfg->gainnet_bf4_cfg->channel,vb->channel);
	}else if(vb->cfg->use_gainnet_bf6)
	{
#if 0
		if(vb->cfg->gainnet_bf6_cfg){
			vb->cfg->gainnet_bf6_cfg->theta_range=vb->cfg->theta_range;
			vb->gainnet_bf6 = wtk_gainnet_bf6_new(vb->cfg->gainnet_bf6_cfg);
			if(!vb->gainnet_bf6){
				ret = -1;
				goto end;
			}	
			wtk_gainnet_bf6_set_notify(vb->gainnet_bf6, vb, (wtk_gainnet_bf6_notify_f)qtk_gainnetbf6_on_data);
		}
		vb->channel = vb->cfg->gainnet_bf6_cfg->gaec.nmicchannel+vb->cfg->gainnet_bf6_cfg->gaec.nspchannel;
#endif
	}else if(vb->cfg->use_rtjoin2)
	{
		wtk_debug("----------------------------------\n");
		if(vb->cfg->rtjoin2_cfg){
			vb->rtjoin2 = wtk_rtjoin2_new(vb->cfg->rtjoin2_cfg);
			if(!vb->rtjoin2){
				ret = -1;
				goto end;
			}
			// wtk_debug("---------------------------------->%p\n",vb->rtjoin2);
			wtk_rtjoin2_set_notify(vb->rtjoin2, vb, (wtk_rtjoin2_notify_f)qtk_rtjoin2_on);
		wtk_debug("----------------------------------\n");
		}
		// vb->channel = vb->cfg->gainnet_bf4_cfg->channel;
		vb->channel = vb->cfg->rtjoin2_cfg->channel;
		
		// wtk_debug("====================>>>channel=%d/%d\n",vb->cfg->gainnet_bf4_cfg->channel,vb->channel);
	}
	else if(vb->cfg->use_mix_speech)
	{

		if(vb->cfg->mix_speech_cfg)
		{
#ifndef USE_KSD_EBNF
			float scale=1.0;
			if(vb->cfg->mix_speech_cfg->mic_scale)
			{
				scale = vb->cfg->mix_speech_cfg->mic_scale[0];
			}
			wtk_mix_speech_cfg_set_channel(vb->cfg->mix_speech_cfg, vb->cfg->join_channel, scale);
#endif
			vb->mix_speech = wtk_mix_speech_new(vb->cfg->mix_speech_cfg);
			wtk_mix_speech_set_notify(vb->mix_speech, vb, (wtk_mix_speech_notify_f)qtk_gainnetbf_mix_speech_on_data);
		}
		vb->channel = vb->cfg->mix_speech_cfg->channel;
	}
	wtk_debug("channel=%d\n",vb->channel);

	vb->buf = (short **)wtk_malloc(sizeof(short *) * vb->channel);
	for(i=0; i < vb->channel; i++){
		vb->buf[i] = (short *)wtk_malloc(FEED_STEP);
	}
    vb->cache_buf = wtk_strbuf_new(3200, 1.0);
	vb->out_buf = wtk_strbuf_new(1024, 1.0);
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

	vb->zdata = (char *)wtk_malloc(sizeof(char)*vb->cfg->energy_thr_time*32);
	vb->nbest_theta = (int *)wtk_malloc(sizeof(int) *vb->cfg->max_extp);
	vb->nbest_phi = (int *)wtk_malloc(sizeof(int) *vb->cfg->max_extp);
	vb->concount = (int *)wtk_malloc(sizeof(int)*vb->cfg->max_extp);

	memset(vb->zdata, 0, vb->cfg->energy_thr_time*32*sizeof(char));
	memset(vb->nbest_theta, 0, vb->cfg->max_extp * sizeof(int));
	memset(vb->nbest_phi, 0, vb->cfg->max_extp * sizeof(int));
	memset(vb->concount, 0, vb->cfg->max_extp * sizeof(int));

	ret = 0;
end:
	if(ret != 0){
		qtk_gainnetbf_delete(vb);
		vb = NULL;		
	}
	wtk_debug("------------------------------>>>>>>>>>>>>>>>>\n");

	return vb;
}
int qtk_gainnetbf_delete(qtk_gainnetbf_t *vb)
{
	int i;
	
	if(vb->cfg->use_gainnet_bf)
	{
		if(vb->gainnet_bf)
		{
			wtk_gainnet_bf_delete(vb->gainnet_bf);
		}
	}else if(vb->cfg->use_rtjoin2)
	{
		if(vb->rtjoin2)
		{
			wtk_rtjoin2_delete(vb->rtjoin2);
		}
	}
	else if(vb->cfg->use_gainnet_bf3)
	{
		if(vb->gainnet_bf3)
		{
			wtk_gainnet_bf3_delete(vb->gainnet_bf3);
		}
	}else if(vb->cfg->use_gainnet_bf4)
	{
		if(vb->gainnet_bf4)
		{
			wtk_gainnet_bf4_delete(vb->gainnet_bf4);
		}
	}else if(vb->cfg->use_gainnet_bf6)
	{
		if(vb->gainnet_bf6)
		{
			wtk_gainnet_bf6_delete(vb->gainnet_bf6);
		}
	}else if(vb->cfg->use_mix_speech)
	{
		if(vb->mix_speech)
		{
			wtk_mix_speech_delete(vb->mix_speech);
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
	wtk_free(vb->zdata);
	wtk_free(vb->nbest_theta);
	wtk_free(vb->nbest_phi);
	wtk_free(vb->concount);
	wtk_free(vb);
	return 0;
}
int qtk_gainnetbf_start(qtk_gainnetbf_t *vb)
{
	if(vb->cfg->use_gainnet_bf4)
	{
		if(vb->cfg->theta >= 0)
		{
			vb->gainnet_bf4->cfg->theta=vb->cfg->theta;
		}
		if(vb->cfg->phi >= 0)
		{
			vb->gainnet_bf4->cfg->phi = vb->cfg->phi;
		}
		if(vb->cfg->theta_range >= 0)
		{
			vb->gainnet_bf4->cfg->theta_range = vb->cfg->theta_range;
		}
		wtk_gainnet_bf4_start(vb->gainnet_bf4);
	}else if(vb->cfg->use_rtjoin2)
	{
		wtk_debug("==================>>>>>>>>>>>>>>%p\n",vb->rtjoin2);
		wtk_rtjoin2_start(vb->rtjoin2);
	}
	else if(vb->cfg->use_gainnet_bf6)
	{
		vb->cfg->gainnet_bf6_cfg->theta_range = vb->cfg->theta_range;
		wtk_gainnet_bf6_start(vb->gainnet_bf6, vb->cfg->theta, vb->cfg->phi);
	}else if(vb->cfg->use_mix_speech)
	{
		wtk_mix_speech_start(vb->mix_speech);
	}
	return 0;
}
int qtk_gainnetbf_reset(qtk_gainnetbf_t *vb)
{
	if(vb->cfg->use_gainnet_bf)
	{
		wtk_gainnet_bf_reset(vb->gainnet_bf);
	}else if(vb->cfg->use_rtjoin2)
	{
		wtk_rtjoin2_reset(vb->rtjoin2);
	}
	else if(vb->cfg->use_gainnet_bf3)
	{
		wtk_gainnet_bf3_reset(vb->gainnet_bf3);
	}else if(vb->cfg->use_gainnet_bf4)
	{
		wtk_gainnet_bf4_reset(vb->gainnet_bf4);
	}else if(vb->cfg->use_gainnet_bf6)
	{
		wtk_gainnet_bf6_reset(vb->gainnet_bf6);
	}else if(vb->cfg->use_mix_speech)
	{
		wtk_mix_speech_reset(vb->mix_speech);
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

int qtk_gainnetbf_set_shift(qtk_gainnetbf_t *vb, float mic_shift, float echo_shift)
{
	vb->cfg->mic_shift=mic_shift;
	vb->cfg->echo_shift=echo_shift;
	return 0;
}

static int qtk_gainnetbf_on_feed(qtk_gainnetbf_t *vb, char *data, int bytes, int is_end)
{
	// static int tb=0;
	if(bytes > 0)
	{
		// double tm;
		// tm = time_get_ms();
		if(vb->cfg->mic_shift != 1.0f)
		{
			qtk_data_change_vol(data, bytes, vb->cfg->mic_shift);
		}
		wtk_strbuf_reset(vb->feed_buf);
		wtk_strbuf_push(vb->feed_buf, data, bytes);
		if(vb->cfg->nskip > 0){
			short *pv,*pv1;
			int i,j,k,len;
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
				// tb+=vb->feed_buf->pos;
				//wtk_debug("==========================>>>>>>%d/%d/%d/%f\n",bytes,vb->feed_buf->pos,tb,tb/(3.0*32));
				wtk_wavfile_write(vb->iwav, vb->feed_buf->data, vb->feed_buf->pos);
			}			
		}

		if(vb->cfg->use_min_sil)
		{
			int i=0;
			while(i<vb->feed_buf->pos)
			{
				wtk_strbuf_push(vb->cache_buf, vb->feed_buf->data+i, 2);
				i+=(2*vb->channel);
			}
		}

		if(vb->cfg->use_gainnet_bf3)
		{
			wtk_gainnet_bf3_feed(vb->gainnet_bf3, (short *)(vb->feed_buf->data), (vb->feed_buf->pos)/(vb->channel*sizeof(short)), 0);
		}else if(vb->cfg->use_gainnet_bf4)
		{
			wtk_gainnet_bf4_feed(vb->gainnet_bf4, (short *)(vb->feed_buf->data), (vb->feed_buf->pos)/(vb->channel*sizeof(short)), 0);
		}else if(vb->cfg->use_rtjoin2)
		{
			// wtk_debug("---------------------------------->%p len=%d\n",vb->rtjoin2,vb->feed_buf->pos);
			wtk_rtjoin2_feed(vb->rtjoin2, (short *)(vb->feed_buf->data), (vb->feed_buf->pos)/(vb->channel*sizeof(short)), 0);
		}
		else if(vb->cfg->use_mix_speech)
		{
			wtk_mix_speech_feed(vb->mix_speech, (short *)(vb->feed_buf->data), (vb->feed_buf->pos)/(vb->channel*sizeof(short)), 0);
		}else{
			int i, j;
			short *pv = NULL;
			int len;
			pv = (short*)(vb->feed_buf->data);
			len = vb->feed_buf->pos /(vb->channel * sizeof(short));
			for(i = 0; i < len; ++i){
				for(j = 0; j < vb->channel; ++j){
					vb->buf[j][i] = pv[i * vb->channel + j];
				}
			}
			if(vb->cfg->use_gainnet_bf)
			{
				wtk_gainnet_bf_feed(vb->gainnet_bf, vb->buf, len, 0);
			}else if(vb->cfg->use_gainnet_bf6)
			{
				wtk_gainnet_bf6_feed(vb->gainnet_bf6, vb->buf, len, 0);
			}
		}
		// tm = time_get_ms() - tm;
		// if(tm > 32.0)
		// {
		// 	wtk_debug("==============================>>>>>>>>>>>>>>>>tm=%f bytes=%d pos=%d\n",tm,bytes,vb->feed_buf->pos);
		// }
	}
	if(is_end){
		if(vb->cfg->use_gainnet_bf)
		{
			wtk_gainnet_bf_feed(vb->gainnet_bf, NULL, 0, 1);
		}else if(vb->cfg->use_gainnet_bf3)
		{
			wtk_gainnet_bf3_feed(vb->gainnet_bf3, NULL, 0, 1);
		}else if(vb->cfg->use_gainnet_bf4)
		{
			wtk_gainnet_bf4_feed(vb->gainnet_bf4, NULL, 0, 1);
		}else if(vb->cfg->use_rtjoin2)
		{
			wtk_rtjoin2_feed(vb->rtjoin2, NULL, 0, 1);
		}
		else if(vb->cfg->use_gainnet_bf6)
		{
			wtk_gainnet_bf6_feed(vb->gainnet_bf6, NULL, 0, 1);
		}else if(vb->cfg->use_mix_speech)
		{
			wtk_mix_speech_feed(vb->mix_speech, NULL, 0, 1);
		}
	}
	return 0;
}

int qtk_gainnetbf_feed(qtk_gainnetbf_t *vb, char *data, int len, int is_end)
{
	if(vb->cfg->use_gainnet_bf4)
	{
		if(vb->cfg->gainnet_bf4_cfg->use_ssl_delay > 0)
		{
			wtk_gainnet_bf4_ssl_delay_new(vb->gainnet_bf4);
		}
	}
	// wtk_debug("----------------------------------\n");

#if 0
	int pos = 0;
	int step = 0;
	int flen;

	step = FEED_STEP * (vb->channel+vb->cfg->nskip);
	while(pos < len){
		flen = min(step, len-pos);
		qtk_gainnetbf_on_feed(vb, data + pos, flen, 0);
		pos +=flen;
	}
	if(is_end){
		qtk_gainnetbf_on_feed(vb, NULL, 0, 1);
	}
#else

	qtk_gainnetbf_on_feed(vb, data, len, is_end);
#endif
	return 0;
}
void qtk_gainnetbf_set_notify(qtk_gainnetbf_t *vb, void *ths, qtk_gainnetbf_notify_f notify)
{
	wtk_debug("-------------------------------------\n");
	vb->ths = ths;
	vb->notify = notify;
}

void qtk_gainnetbf_set_notify2(qtk_gainnetbf_t *vb, void *ths, qtk_engine_notify_f notify)
{
	vb->eths=ths;
	vb->enotify=notify;
}

void qtk_gainnetbf_on_aec(qtk_gainnetbf_t *gd, short **data, int len, int is_end)
{
	if(len > 0)
	{
		wtk_gainnet_bf_feed(gd->gainnet_bf, data, len, 0);
	}
	if(is_end)
	{
		wtk_gainnet_bf_feed(gd->gainnet_bf, NULL, 0, 1);
	}
}

void qtk_gainnetbf_on_gainnet_aec(qtk_gainnetbf_t *gd, short **data, short **lasty, int len, int is_end)
{
	if(len > 0)
	{
		wtk_gainnet_bf_feed(gd->gainnet_bf, data, len, 0);
	}
	if(is_end)
	{
		wtk_gainnet_bf_feed(gd->gainnet_bf, NULL, 0, 1);
	}
}

void qtk_gainnetbf4_on_ssl(qtk_gainnetbf_t *vb, float ts, float te, wtk_ssl2_extp_t *nbest_extp, int nbest)
{
	qtk_var_t var;
	int i;
	static int first=1;
	int errnum[10]={0};

	//wtk_debug("================================>>>>>>%d %d %f\n",nbest,nbest_extp[0].theta,nbest_extp[0].nspecsum);

	for(i=0; i<nbest; ++i)
	{
		if(nbest_extp[i].nspecsum > vb->cfg->energy_sum)
		{
			if((nbest_extp[i].theta == 180 || nbest_extp[i].theta == 0) || (nbest_extp[i].nspecsum < vb->cfg->zero_sum))
			{
				vb->concount[i]=0;
				errnum[i]=1;
			}
		}else{
			vb->concount[i] = 0;
			errnum[i] = 1;
		}
	}
	if(first)
	{
		for(i=0; i<nbest; ++i)
		{
			if(errnum[i]){continue;}
			vb->nbest_theta[i]=nbest_extp[i].theta;
			vb->nbest_phi[i]=nbest_extp[i].phi;
			if (vb->enotify)
			{
				var.type = QTK_AEC_DIRECTION;
				var.v.ii.nbest = i;
				var.v.ii.theta = nbest_extp[i].theta;
				var.v.ii.phi = nbest_extp[i].phi;
				// printf("%d %d %d %d\n",nbest, i, nbest_extp[i].theta, nbest_extp[i].phi);
				vb->enotify(vb->eths, &(var));
			}
		}
		first = 0;
	}else{
		for(i=0; i<nbest; ++i)
		{
			if(errnum[i]){continue;}
			int err;
			err=abs(nbest_extp[i].theta - vb->nbest_theta[i]);

			if(err > vb->cfg->theta_range)
			{
				vb->concount[i]++;
			}else{
				vb->concount[i] = 0;
			}

			if(vb->concount[i] >= vb->cfg->continue_count)
			{
				vb->nbest_theta[i] = nbest_extp[i].theta;
				vb->concount[i] = 0;

			}				
			if (vb->enotify)
			{
				var.type = QTK_AEC_DIRECTION;
				var.v.ii.nbest = i;
				var.v.ii.theta = vb->nbest_theta[i];
				var.v.ii.phi = nbest_extp[i].phi;
				// printf("%d %d %d %d\n",nbest, i, nbest_extp[i].theta, nbest_extp[i].phi);
				vb->enotify(vb->eths, &(var));
			}
		}
	}
}

void qtk_gainnetbf_on_ssl(qtk_gainnetbf_t *vb, wtk_ssl2_extp_t *nbest_extp, int nbest)
{
	qtk_var_t var;
	int i;
	static int first=1;
	int errnum[10]={0};

	//wtk_debug("================================>>>>>>%d %d %f\n",nbest,nbest_extp[0].theta,nbest_extp[0].nspecsum);

	for(i=0; i<nbest; ++i)
	{
		if(nbest_extp[i].nspecsum > vb->cfg->energy_sum)
		{
			if((nbest_extp[i].theta == 180 || nbest_extp[i].theta == 0) || (nbest_extp[i].nspecsum < vb->cfg->zero_sum))
			{
				vb->concount[i]=0;
				errnum[i]=1;
			}
		}else{
			vb->concount[i] = 0;
			errnum[i] = 1;
		}
	}
	if(first)
	{
		for(i=0; i<nbest; ++i)
		{
			if(errnum[i]){continue;}
			vb->nbest_theta[i]=nbest_extp[i].theta;
			vb->nbest_phi[i]=nbest_extp[i].phi;
			if (vb->enotify)
			{
				var.type = QTK_AEC_DIRECTION;
				var.v.ii.nbest = i;
				var.v.ii.theta = nbest_extp[i].theta;
				var.v.ii.phi = nbest_extp[i].phi;
				// printf("%d %d %d %d\n",nbest, i, nbest_extp[i].theta, nbest_extp[i].phi);
				vb->enotify(vb->eths, &(var));
			}
		}
		first = 0;
	}else{
		for(i=0; i<nbest; ++i)
		{
			if(errnum[i]){continue;}
			int err;
			err=abs(nbest_extp[i].theta - vb->nbest_theta[i]);

			if(err > vb->cfg->theta_range)
			{
				vb->concount[i]++;
			}else{
				vb->concount[i] = 0;
			}

			if(vb->concount[i] >= vb->cfg->continue_count)
			{
				vb->nbest_theta[i] = nbest_extp[i].theta;
				vb->concount[i] = 0;

			}				
			if (vb->enotify)
			{
				var.type = QTK_AEC_DIRECTION;
				var.v.ii.nbest = i;
				var.v.ii.theta = vb->nbest_theta[i];
				var.v.ii.phi = nbest_extp[i].phi;
				// printf("%d %d %d %d\n",nbest, i, nbest_extp[i].theta, nbest_extp[i].phi);
				vb->enotify(vb->eths, &(var));
			}
		}
	}
}
void qtk_rtjoin2_on(qtk_gainnetbf_t *vb, short *data, int len)
{
	qtk_var_t var;

	if(vb->cfg->use_log_wav)
	{
		if(vb->owav)
		{
			wtk_wavfile_write(vb->owav, (char *)data, len<<1);
		}			
	}

	if(vb->notify){
		vb->notify(vb->ths, (char *)data, len<<1);
	}

	if(vb->enotify)
	{
		var.type = QTK_SPEECH_DATA_PCM;
		var.v.str.data = (char *)data;
		var.v.str.len = len<<1;
		vb->enotify(vb->eths, &var);
	}

}

void qtk_gainnetbf4_on_data(qtk_gainnetbf_t *vb, short *data, int len)
{
	qtk_var_t var;
	// static double gtm=0.0;

	// wtk_debug("=======================>>>>>>>>>>>>>>>>>tm=%f\n",time_get_ms() - gtm);
	// gtm = time_get_ms();

	if(vb->cfg->use_log_wav)
	{
		if(vb->owav)
		{
			wtk_wavfile_write(vb->owav, (char *)data, len<<1);
		}			
	}
	if(vb->cfg->use_cachebuf)
	{
		wtk_strbuf_push(vb->out_buf, (char *)data, len<<1);
		if(vb->out_buf->pos >= vb->cfg->cache_len)
		{
			if(vb->notify){
				vb->notify(vb->ths, vb->out_buf->data, vb->cfg->cache_len);
			}
			if(vb->enotify)
			{
				var.type = QTK_SPEECH_DATA_PCM;
				var.v.str.data = vb->out_buf->data;
				var.v.str.len = vb->cfg->cache_len;
				vb->enotify(vb->eths, &var);
			}	
			wtk_strbuf_pop(vb->out_buf, NULL, vb->cfg->cache_len);
		}
	}else{
		if(vb->notify){
			vb->notify(vb->ths, (char *)data, len<<1);
		}
		if(vb->enotify)
		{
			var.type = QTK_SPEECH_DATA_PCM;
			var.v.str.data = (char *)data;
			var.v.str.len = len<<1;
			vb->enotify(vb->eths, &var);
		}
	}
}

void qtk_gainnetbf_mix_speech_on_data(qtk_gainnetbf_t * vb, short *data, int len)
{
	qtk_var_t var;
	// static double gtm=0.0;

	// wtk_debug("=======================>>>>>>>>>>>>>>>>>tm=%f\n",time_get_ms() - gtm);
	// gtm = time_get_ms();

	if(vb->cfg->use_log_wav)
	{
		if(vb->owav)
		{
			wtk_wavfile_write(vb->owav, (char *)data, len<<1);
		}			
	}
	if(vb->cfg->use_cachebuf)
	{
		wtk_strbuf_push(vb->out_buf, (char *)data, len<<1);
		if(vb->out_buf->pos >= vb->cfg->cache_len)
		{
			if(vb->notify){
				vb->notify(vb->ths, vb->out_buf->data, vb->cfg->cache_len);
			}
			if(vb->enotify)
			{
				var.type = QTK_SPEECH_DATA_PCM;
				var.v.str.data = vb->out_buf->data;
				var.v.str.len = vb->cfg->cache_len;
				vb->enotify(vb->eths, &var);
			}	
			wtk_strbuf_pop(vb->out_buf, NULL, vb->cfg->cache_len);
		}
	}else{
		if(vb->notify){
			vb->notify(vb->ths, (char *)data, len<<1);
		}
		if(vb->enotify)
		{
			var.type = QTK_SPEECH_DATA_PCM;
			var.v.str.data = (char *)data;
			var.v.str.len = len<<1;
			vb->enotify(vb->eths, &var);
		}
	}
}

void qtk_gainnetbf3_on_data(qtk_gainnetbf_t *vb, short *data, int len, int is_end)
{
	qtk_var_t var;
	float sum;


	if(vb->cfg->use_min_sil)
	{
		int nx=0;
		int step=vb->cfg->energy_thr_time*32;

		while(step < (vb->cache_buf->pos-nx))
		{
			sum = wtk_short_energy((short *)(vb->cache_buf->data+nx), step<<1);
			if(vb->cfg->use_energy_debug)
			{
				printf("========>>energy=[%f] / energy_threshold=[%f]\n",sum,vb->cfg->energy_thr);
			}
			if(sum < vb->cfg->energy_thr)
			{
				vb->energy_cnt++;
				if(vb->energy_cnt > vb->cfg->energy_thr_count)
				{
					wtk_strbuf_push(vb->out_buf, vb->zdata, step);
				}else{
					wtk_strbuf_push(vb->out_buf, vb->cache_buf->data + nx, step);
				}
			}else{
				vb->energy_cnt = 0;
				wtk_strbuf_push(vb->out_buf, vb->cache_buf->data+nx, step);
			}
			nx+=step;
		}

		if(vb->out_buf->pos > 0)
		{
			if(vb->cfg->use_log_wav)
			{
				if(vb->owav)
				{
					wtk_wavfile_write(vb->owav, vb->out_buf->data, vb->out_buf->pos);
				}			
			}

			if(vb->notify){
				vb->notify(vb->ths, vb->out_buf->data, vb->out_buf->pos);
			}
			if(vb->enotify)
			{
				var.type = QTK_SPEECH_DATA_PCM;
				var.v.str.data = vb->out_buf->data;
				var.v.str.len = vb->out_buf->pos;
				vb->enotify(vb->eths, &var);
			}
			wtk_strbuf_reset(vb->out_buf);
		}
		if(vb->cache_buf->pos >= (vb->cfg->left_count*vb->cfg->energy_thr_time*32))
		{
			wtk_strbuf_pop(vb->cache_buf, NULL, vb->cfg->energy_thr_time*32);
		}
	}else
	{
		if(vb->cfg->use_log_wav)
		{
			if(vb->owav)
			{
				wtk_wavfile_write(vb->owav, (char *)data, len<<1);
			}			
		}
		if(vb->notify){
			vb->notify(vb->ths, (char *)data, len<<1);
		}
		if(vb->enotify)
		{
			var.type = QTK_SPEECH_DATA_PCM;
			var.v.str.data = (char *)data;
			var.v.str.len = len<<1;
			vb->enotify(vb->eths, &var);
		}
	}
}

//int cache_count;
#define CACHE_STEP  (8)
void qtk_gainnetbf_on_data(qtk_gainnetbf_t *vb, short *data, int len, int is_end)
{
	qtk_var_t var;

	if(vb->cfg->use_log_wav)
	{
		if(vb->owav)
		{
			wtk_wavfile_write(vb->owav, (char *)data, len<<1);
		}			
	}

	if(vb->notify){
		vb->notify(vb->ths, (char *)data, len<<1);
	}
	if(vb->enotify)
	{
		var.type = QTK_SPEECH_DATA_PCM;
		var.v.str.data = (char *)data;
		var.v.str.len = len<<1;
		vb->enotify(vb->eths, &var);
	}
#if 0
	wtk_strbuf_push(vb->out_buf, (char *)data, len<<1);
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
// #else
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

void qtk_gainnetbf6_on_data(qtk_gainnetbf_t *vb, short *data, int len)
{
	qtk_var_t var;

	if(vb->cfg->use_log_wav)
	{
		if(vb->owav)
		{
			wtk_wavfile_write(vb->owav, (char *)data, len<<1);
		}			
	}

	if(vb->notify){
		vb->notify(vb->ths, (char *)data, len<<1);
	}
	if(vb->enotify)
	{
		var.type = QTK_SPEECH_DATA_PCM;
		var.v.str.data = (char *)data;
		var.v.str.len = len<<1;
		vb->enotify(vb->eths, &var);
	}
}
