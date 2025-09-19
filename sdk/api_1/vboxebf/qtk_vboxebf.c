#include "qtk_vboxebf.h"
//#define USE_32MSTO8MS
#define FEED_STEP (20 * 32 * 2)
void qtk_vboxebf_on_data(qtk_vboxebf_t *vb, short *data, int len);
void qtk_vboxebf_on_ssl(qtk_vboxebf_t *vb, wtk_ssl2_extp_t *nextp, int nbaset);
void qtk_vboxebf_on_ssl_new(qtk_vboxebf_t *vb, float ts, float te, wtk_ssl2_extp_t *nbest_extp, int nbest);
void qtk_vboxebf_on_eng(qtk_vboxebf_t *vb, float energy, float snr);
int qtk_vboxebf_ssl_delay_entry(qtk_vboxebf_t *m, wtk_thread_t *t);
void qtk_mask_bf_net_on(qtk_vboxebf_t *vb, short *data, int len);
long rrlen=0;
long cvlen=0;
// FILE *skipf;

qtk_vboxebf_t *qtk_vboxebf_new(qtk_vboxebf_cfg_t *cfg)
{
	qtk_vboxebf_t *vb;
	int i, ret;

	// skipf = fopen("./5c_test.pcm", "wb+");

	vb = (qtk_vboxebf_t *)wtk_calloc(1, sizeof(*vb));
	vb->cfg = cfg;
	vb->ths=NULL;
	vb->notify=NULL;
	vb->eths=NULL;
	vb->enotify=NULL;
	vb->vebf3=NULL;
	vb->vebf4=NULL;
	vb->vebf6=NULL;
	vb->mask_bf_net=NULL;
	vb->buf=NULL;
	vb->nbest_theta = NULL;
	vb->nbest_phi = NULL;
	vb->concount = NULL;
	vb->zdata = NULL;
	vb->cache_buf = NULL;
	vb->out_buf = NULL;
	vb->micwav=NULL;
	vb->echowav=NULL;
	vb->use_line = -1;

	if(cfg->use_vboxebf6){
		if(vb->cfg->vebf6_cfg){
			vb->vebf6 = wtk_vboxebf6_new(vb->cfg->vebf6_cfg);
			if(!vb->vebf6){
				ret = -1;
				goto end;
			}
		}
		wtk_vboxebf6_set_notify(vb->vebf6, vb, (wtk_vboxebf6_notify_f)qtk_vboxebf_on_data);
		wtk_vboxebf6_set_ssl_notify(vb->vebf6, vb, (wtk_vboxebf6_notify_ssl_f)qtk_vboxebf_on_ssl_new);
		wtk_vboxebf6_set_eng_notify(vb->vebf6, vb, (wtk_vboxebf6_notify_eng_f)qtk_vboxebf_on_eng);
		vb->channel = vb->cfg->vebf6_cfg->channel;
		if(vb->cfg->vebf6_cfg->use_maskssl2){
			vb->use_line = vb->cfg->vebf6_cfg->maskssl2.use_line;
		}
		if(vb->cfg->vebf6_cfg->use_maskssl){
			vb->use_line = vb->cfg->vebf6_cfg->maskssl.use_line;
		}
		vb->winslen = vb->cfg->vebf6_cfg->wins*vb->channel;
	}else if(cfg->use_mask_bf_net){
		if(vb->cfg->mask_bf_net_cfg){
			vb->mask_bf_net = wtk_mask_bf_net_new(vb->cfg->mask_bf_net_cfg);
			if(!vb->mask_bf_net){
				ret = -1;
				goto end;
			}
		}
		wtk_mask_bf_net_set_notify(vb->mask_bf_net, vb,(wtk_mask_bf_net_notify_f)qtk_mask_bf_net_on);
		vb->channel = vb->cfg->mask_bf_net_cfg->channel;
		vb->winslen = vb->cfg->mask_bf_net_cfg->wins*vb->channel;
	}
	else if(cfg->use_vboxebf4){
		if(vb->cfg->vebf4_cfg){
			vb->vebf4 = wtk_vboxebf4_new(vb->cfg->vebf4_cfg);
			if(!vb->vebf4){
				ret = -1;
				goto end;
			}	
		}
		// wtk_debug("========================>>>>>>>>>>>>>>>>>theta step=%d\n",vb->vebf4->cfg->ssl2.theta_step);
		wtk_vboxebf4_set_notify(vb->vebf4, vb, (wtk_vboxebf4_notify_f)qtk_vboxebf_on_data);
		wtk_vboxebf4_set_ssl_notify(vb->vebf4, vb, (wtk_vboxebf4_notify_ssl_f)qtk_vboxebf_on_ssl_new);
		wtk_vboxebf4_set_eng_notify(vb->vebf4, vb, (wtk_vboxebf4_notify_eng_f)qtk_vboxebf_on_eng);
		vb->channel = vb->cfg->vebf4_cfg->channel;
		if(vb->cfg->vebf4_cfg->use_maskssl2){
			vb->use_line = vb->cfg->vebf4_cfg->maskssl2.use_line;
		}
		if(vb->cfg->vebf4_cfg->use_maskssl){
			vb->use_line = vb->cfg->vebf4_cfg->maskssl.use_line;
		}
		vb->winslen = vb->cfg->vebf4_cfg->wins*vb->channel;
		// wtk_debug("channel=%d\n",vb->channel);
		if(cfg->use_manual==0 && (vb->cfg->vebf4_cfg->mchannel >= vb->cfg->vebf4_cfg->nmicchannel)){
			int nskip=vb->cfg->vebf4_cfg->mchannel - vb->cfg->vebf4_cfg->nmicchannel;
			int k;
			cfg->nskip = nskip;
			if(nskip == 2){
#if 0 //def USE_ZHIWEI
				cfg->skip_channels[0] = 2;
				cfg->skip_channels[1] = 4;
				if(vb->cfg->vebf4_cfg->schannel >vb->cfg->vebf4_cfg->nspchannel)
				{
					cfg->skip_channels[2] = 7;
					cfg->nskip++;
				}
#else
#ifdef USE_JINGDONGFANG
				cfg->skip_channels[0] = 0;
				cfg->skip_channels[1] = 1;
				if(vb->cfg->vebf4_cfg->schannel >vb->cfg->vebf4_cfg->nspchannel){
					cfg->skip_channels[2] = 7;
					cfg->nskip++;
				}
#else
				cfg->skip_channels[0] = 0;
				cfg->skip_channels[1] = 5;
				if(vb->cfg->vebf4_cfg->schannel >vb->cfg->vebf4_cfg->nspchannel){
					cfg->skip_channels[2] = 7;
					cfg->nskip++;
				}
#endif
#endif
			}else if(nskip == 3){
				cfg->skip_channels[0] = 0;
				cfg->skip_channels[1] = 1;
				cfg->skip_channels[2] = 7;
				if(vb->cfg->vebf4_cfg->schannel >vb->cfg->vebf4_cfg->nspchannel){
					cfg->skip_channels[3] = 9;
					cfg->nskip++;
				}
			}else if(nskip == 4){
				cfg->skip_channels[0] = 0;
				cfg->skip_channels[1] = 1;
				cfg->skip_channels[2] = 6;
				cfg->skip_channels[3] = 7;
				if(vb->cfg->vebf4_cfg->schannel >vb->cfg->vebf4_cfg->nspchannel){
					cfg->skip_channels[4] = 9;
					cfg->nskip++;
				}
			}else{
				for(k=0;k<nskip;++k)
				{
					cfg->skip_channels[k] = vb->cfg->vebf4_cfg->nmicchannel+k;
				}
#ifdef USE_KANGJIA
				for(k=0;k<(vb->cfg->vebf4_cfg->schannel - vb->cfg->vebf4_cfg->nspchannel);++k)
				{
					cfg->skip_channels[k] = vb->cfg->vebf4_cfg->nmicchannel+k;
					cfg->nskip++;
				}
#else
				if(vb->cfg->vebf4_cfg->schannel >vb->cfg->vebf4_cfg->nspchannel){
					cfg->skip_channels[k] = vb->cfg->vebf4_cfg->nmicchannel+k;
					cfg->nskip++;
				}
#endif
			}
		}
	}else if(cfg->use_vboxebf3){
		if(vb->cfg->vebf3_cfg){
			vb->vebf3 = wtk_vboxebf3_new(vb->cfg->vebf3_cfg);
			if(!vb->vebf3){
				ret = -1;
				goto end;
			}
		}
		wtk_vboxebf3_set_notify(vb->vebf3, vb, (wtk_vboxebf3_notify_f)qtk_vboxebf_on_data);
		wtk_vboxebf3_set_ssl_notify(vb->vebf3, vb, (wtk_vboxebf3_notify_ssl_f)qtk_vboxebf_on_ssl_new);
		wtk_vboxebf3_set_eng_notify(vb->vebf3, vb, (wtk_vboxebf3_notify_eng_f)qtk_vboxebf_on_eng);
		vb->channel = vb->cfg->vebf3_cfg->channel;
		if(vb->cfg->vebf3_cfg->use_maskssl2){
			vb->use_line = vb->cfg->vebf3_cfg->maskssl2.use_line;
		}
		if(vb->cfg->vebf3_cfg->use_maskssl){
			vb->use_line = vb->cfg->vebf3_cfg->maskssl.use_line;
		}
		vb->winslen = vb->cfg->vebf3_cfg->wins*vb->channel;
		if(cfg->use_manual==0 && (vb->cfg->vebf3_cfg->mchannel >= vb->cfg->vebf3_cfg->nmicchannel)){
			int nskip=vb->cfg->vebf3_cfg->mchannel - vb->cfg->vebf3_cfg->nmicchannel;
			int k;
			cfg->nskip = nskip;
			if(nskip == 2){
				cfg->skip_channels[0] = 0;
				cfg->skip_channels[1] = 5;
				if(vb->cfg->vebf3_cfg->schannel >vb->cfg->vebf3_cfg->nspchannel){
					cfg->skip_channels[2] = 7;
					cfg->nskip++;
				}
			}else if(nskip == 3){
				cfg->skip_channels[0] = 0;
				cfg->skip_channels[1] = 1;
				cfg->skip_channels[2] = 7;
				if(vb->cfg->vebf3_cfg->schannel >vb->cfg->vebf3_cfg->nspchannel){
					cfg->skip_channels[3] = 9;
					cfg->nskip++;
				}
			}else if(nskip == 4){
				cfg->skip_channels[0] = 0;
				cfg->skip_channels[1] = 1;
				cfg->skip_channels[2] = 6;
				cfg->skip_channels[3] = 7;
				if(vb->cfg->vebf3_cfg->schannel >vb->cfg->vebf3_cfg->nspchannel){
					cfg->skip_channels[4] = 9;
					cfg->nskip++;
				}
			}else{
				for(k=0;k<nskip;++k)
				{
					cfg->skip_channels[k] = vb->cfg->vebf3_cfg->nmicchannel+k;
				}
				if(vb->cfg->vebf3_cfg->schannel >vb->cfg->vebf3_cfg->nspchannel){
					cfg->skip_channels[k] = vb->cfg->vebf3_cfg->nmicchannel+k;
					cfg->nskip++;
				}
			}
		}
	}else if(vb->cfg->use_ahs){
#ifdef USE_AHS
		if(vb->cfg->ahs_cfg){
			vb->ahs = qtk_ahs_new(vb->cfg->ahs_cfg);
			if(!vb->ahs){
				ret = -1;
				goto end;
			}
			qtk_ahs_set_notify(vb->ahs, vb, (qtk_ahs_notify_f)qtk_vboxebf_on_data);
		}
		vb->channel = 1;
		vb->winslen = vb->cfg->ahs_cfg->window_sz*vb->channel;
#endif
	}else{
		wtk_debug("use vboxebf error!\n");
	}

	// wtk_debug("=============>>>>>>>channel=%d/%d max_extp=%d use_ssl_delay=%d use_log_wav=%d use_manual=%d\n",vb->channel,vb->cfg->nskip,vb->cfg->max_extp,vb->cfg->use_ssl_delay,vb->cfg->use_log_wav,vb->cfg->use_manual);
	if(vb->cfg->max_extp > 0){
		vb->nbest_theta = (int *)wtk_malloc(sizeof(int) *vb->cfg->max_extp);
		vb->nbest_phi = (int *)wtk_malloc(sizeof(int) *vb->cfg->max_extp);
		vb->concount = (int *)wtk_malloc(sizeof(int) *vb->cfg->max_extp);

		memset(vb->nbest_theta, 0, vb->cfg->max_extp * sizeof(int));
		memset(vb->nbest_phi, 0, vb->cfg->max_extp * sizeof(int));
		memset(vb->concount, 0, vb->cfg->max_extp * sizeof(int));
	}

	vb->zdata = (char *)wtk_malloc(vb->winslen*sizeof(char));
	memset(vb->zdata, 0, vb->winslen);

	vb->buf = (short **)wtk_malloc(sizeof(short *) * vb->channel);
	for(i=0; i < vb->channel; i++){
		vb->buf[i] = (short *)wtk_malloc(FEED_STEP);
	}
    vb->cache_buf = wtk_strbuf_new(3200, 1.0);
	vb->out_buf = wtk_strbuf_new(1024, 1.0);
	vb->sample_bytes_in = wtk_strbuf_new(73728, 1.0);
	vb->sample_bytes_out = wtk_strbuf_new(6144, 1.0);

	if(vb->cfg->use_ssl_delay == 1){
		wtk_thread_init(&vb->ssl_delay_t, (thread_route_handler)qtk_vboxebf_ssl_delay_entry, vb);
		wtk_thread_start(&vb->ssl_delay_t);
	}

	if(vb->cfg->use_log_wav){
		char bufname[100]={0};
		snprintf(bufname, 100, "%.*s/mic.wav",vb->cfg->cache_fn.len,vb->cfg->cache_fn.data);
		vb->micwav = wtk_wavfile_new(48000);
		wtk_debug("mic:===>[%s]\n",bufname);
		ret=wtk_wavfile_open(vb->micwav, bufname);
		if(ret!=0){
			wtk_wavfile_delete(vb->micwav);
			vb->micwav = NULL;
			wtk_debug("=====================>>>>>mic wav open faild\n");
		}else{
			vb->micwav->max_pend=0;
			wtk_wavfile_set_channel(vb->micwav, vb->channel);
		}
		memset(bufname, 0, 100);
		snprintf(bufname, 100, "%.*s/echo.wav",vb->cfg->cache_fn.len,vb->cfg->cache_fn.data);
		vb->echowav = wtk_wavfile_new(48000);
		wtk_debug("echo:===>[%s]\n",bufname);
		ret = wtk_wavfile_open(vb->echowav, bufname);
		if(ret!=0){
			wtk_wavfile_delete(vb->echowav);
			vb->echowav = NULL;
			wtk_debug("=====================>>>>>echo wav open faild\n");
		}else{
			vb->echowav->max_pend=0;
		}
	}

	ret = 0;
end:
	if(ret != 0){
		qtk_vboxebf_delete(vb);
		vb = NULL;		
	}	
	return vb;
}
int qtk_vboxebf_delete(qtk_vboxebf_t *vb)
{
	int i;

	if(vb->cfg->use_ssl_delay){
		wtk_thread_join(&vb->ssl_delay_t);
	}
	if(vb->cfg->use_vboxebf6){
		if(vb->vebf6){
			wtk_vboxebf6_delete(vb->vebf6);
		}
	}else if(vb->cfg->mask_bf_net_cfg){
		 if(vb->mask_bf_net){
			wtk_mask_bf_net_delete(vb->mask_bf_net);
		}
	}
	else if(vb->cfg->use_vboxebf4){
		if(vb->vebf4){
			wtk_vboxebf4_delete(vb->vebf4);
		}
	}else if(vb->cfg->use_vboxebf3){
		if(vb->vebf3){
			wtk_vboxebf3_delete(vb->vebf3);
		}
	}else if(vb->cfg->use_ahs){
#ifdef USE_AHS
		if(vb->ahs){
			qtk_ahs_delete(vb->ahs);
		}
#endif
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
	if(vb->out_buf){
		wtk_strbuf_delete(vb->out_buf);
	}
	if(vb->sample_bytes_in){
		wtk_strbuf_delete(vb->sample_bytes_in);
	}
	if(vb->sample_bytes_out){
		wtk_strbuf_delete(vb->sample_bytes_out);
	}
	if(vb->micwav){
		wtk_wavfile_delete(vb->micwav);
	}
	if(vb->echowav){
		wtk_wavfile_delete(vb->echowav);
	}
	wtk_free(vb->zdata);
	wtk_free(vb->nbest_theta);
	wtk_free(vb->nbest_phi);
	wtk_free(vb->concount);
	wtk_free(vb);
	return 0;
}
int qtk_vboxebf_start(qtk_vboxebf_t *vb)
{
	if(vb->cfg->use_vboxebf6){
		if(vb->cfg->mic_volume != -1){
			wtk_vboxebf6_set_micvolume(vb->vebf6, vb->cfg->mic_volume);
		}
		if(vb->cfg->agc_enable != -1){
			wtk_vboxebf6_set_agcenable(vb->vebf6, vb->cfg->agc_enable);
		}
		if(vb->cfg->echo_enable != -1){
			wtk_vboxebf6_set_echoenable(vb->vebf6, vb->cfg->echo_enable);
		}
		if(vb->cfg->denoise_enable != -1){
			wtk_vboxebf6_set_denoiseenable(vb->vebf6, vb->cfg->denoise_enable);
		}
		if(vb->cfg->suppress != -1){
			wtk_vboxebf6_set_denoisesuppress(vb->vebf6, vb->cfg->suppress);
		}
		wtk_vboxebf6_start(vb->vebf6);
	}else if(vb->cfg->use_vboxebf4){
		if(vb->cfg->mic_volume != -1){
			wtk_vboxebf4_set_micvolume(vb->vebf4, vb->cfg->mic_volume);
		}
		if(vb->cfg->agc_enable != -1){
			wtk_vboxebf4_set_agcenable(vb->vebf4, vb->cfg->agc_enable);
		}
		if(vb->cfg->echo_enable != -1){
			wtk_vboxebf4_set_echoenable(vb->vebf4, vb->cfg->echo_enable);
		}
		if(vb->cfg->denoise_enable != -1){
			wtk_vboxebf4_set_denoiseenable(vb->vebf4, vb->cfg->denoise_enable);
		}
		// if(vb->cfg->suppress != -1)
		// {
		// 	wtk_vboxebf4_set_denoisesuppress(vb->vebf4, vb->cfg->suppress);
		// }
		wtk_vboxebf4_start(vb->vebf4);
#ifdef USE_3308
#if (defined  USE_AM32) || (defined  USE_802A) || (defined  USE_AM60)
		if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0){
			FILE *fn;
			fn = fopen("/oem/qdreamer/qsound/uart.cfg", "r");
#else
		if (access("/oem/uart.cfg", F_OK) == 0){
			FILE *fn;
			fn = fopen("/oem/uart.cfg", "r");
#endif
			char buf[512];
			char *pv;
			int ret, val;
			ret = fread(buf, sizeof(buf), 1, fn);
			pv = strstr(buf, "AGC=");
			vb->cfg->AGC = atoi(pv + 4);
			pv = strstr(buf, "AEC=");
			vb->cfg->AEC = atoi(pv + 4);
			pv = strstr(buf, "ANS=");
			vb->cfg->ANS = atoi(pv + 4);
			pv = strstr(buf, "mic_shift2=");
			wtk_vboxebf4_set_micvolume(vb->vebf4,(float)atof(pv + 11));
			wtk_vboxebf4_set_agcenable(vb->vebf4, vb->cfg->AGC);
			wtk_vboxebf4_set_echoenable(vb->vebf4, vb->cfg->AEC);
			wtk_vboxebf4_set_denoiseenable(vb->vebf4, vb->cfg->ANS);
			fclose(fn);
			fn = NULL;
		}
#endif

	}else if(vb->cfg->use_vboxebf3){
		if(vb->cfg->mic_volume != -1){
			wtk_vboxebf3_set_micvolume(vb->vebf3, vb->cfg->mic_volume);
		}
		if(vb->cfg->agc_enable != -1){
			wtk_vboxebf3_set_agcenable(vb->vebf3, vb->cfg->agc_enable);
		}
		if(vb->cfg->echo_enable != -1){
			wtk_vboxebf3_set_echoenable(vb->vebf3, vb->cfg->echo_enable);
		}
		if(vb->cfg->denoise_enable != -1){
			wtk_vboxebf3_set_denoiseenable(vb->vebf3, vb->cfg->denoise_enable);
		}
		if(vb->cfg->suppress != -1){
			wtk_vboxebf3_set_denoisesuppress(vb->vebf3, vb->cfg->suppress);
		}
		if(vb->cfg->ssl_enable != -1){
			wtk_vboxebf3_set_sslenable(vb->vebf3, vb->cfg->ssl_enable);
		}
		wtk_vboxebf3_start(vb->vebf3);
	}else if(vb->cfg->use_mask_bf_net){
		wtk_debug("----------------------------------\n");
		if(vb->cfg->mic_volume != -1){
			wtk_mask_bf_net_set_micscale(vb->mask_bf_net, vb->cfg->mic_volume);
		}
		wtk_debug("----------------------------------\n");
		if(vb->cfg->agc_enable != -1){
			wtk_mask_bf_net_set_agcenable(vb->mask_bf_net, vb->cfg->agc_enable);
		}
		wtk_debug("----------------------------------\n");
		if(vb->cfg->echo_enable != -1){
			wtk_mask_bf_net_set_echoenable(vb->mask_bf_net, vb->cfg->echo_enable);
		}
		wtk_debug("----------------------------------\n");
		if(vb->cfg->denoise_enable != -1){
			wtk_mask_bf_net_set_denoiseenable(vb->mask_bf_net, vb->cfg->denoise_enable);
		}
		wtk_debug("----------------------------------\n");
		if(vb->cfg->suppress != -1){
			wtk_mask_bf_net_set_denoisesuppress(vb->mask_bf_net, vb->cfg->suppress);
		}
		wtk_debug("----------------------------------\n");
		if(vb->cfg->ssl_enable != -1){
			wtk_vboxebf3_set_sslenable(vb->mask_bf_net, vb->cfg->ssl_enable);
		}
		wtk_debug("----------------------------------\n");
		wtk_mask_bf_net_start(vb->mask_bf_net);
	}else if(vb->cfg->use_ahs){
#ifdef USE_AHS
		if(vb->ahs){
			qtk_ahs_start(vb->ahs);
		}
#endif
	}

	vb->is_start=1;
	return 0;
}
int qtk_vboxebf_reset(qtk_vboxebf_t *vb)
{
	if(vb->cfg->use_vboxebf6){
		wtk_vboxebf6_reset(vb->vebf6);
	}else if(vb->cfg->use_vboxebf4){
		wtk_vboxebf4_reset(vb->vebf4);
	}else if(vb->cfg->use_vboxebf3){
		wtk_vboxebf3_reset(vb->vebf3);
	}else if(vb->cfg->use_mask_bf_net){
		wtk_mask_bf_net_reset(vb->mask_bf_net);
	}
	else if(vb->cfg->use_ahs){
#ifdef USE_AHS
		if(vb->ahs){
			qtk_ahs_reset(vb->ahs);
		}
#endif
	}

	if(vb->out_buf){
		wtk_strbuf_reset(vb->out_buf);
	}
	if(vb->cache_buf){
		wtk_strbuf_reset(vb->cache_buf);
	}
	if(vb->sample_bytes_in){
		wtk_strbuf_reset(vb->sample_bytes_in);
	}
	if(vb->sample_bytes_out){
		wtk_strbuf_reset(vb->sample_bytes_out);
	}
	return 0;
}

int qtk_vboxebf_ssl_delay_entry(qtk_vboxebf_t *m, wtk_thread_t *t)
{
	if(m){
		qtk_vboxebf_ssl_delay_new(m);
		if(m->cfg->ssl_enable != -1){
			wtk_vboxebf3_set_sslenable(m->vebf3, m->cfg->ssl_enable);
		}
	}
}

void qtk_vboxebf_ssl_delay_new(qtk_vboxebf_t *vb)
{
	if(vb->cfg->use_vboxebf3){
		wtk_vboxebf3_ssl_delay_new(vb->vebf3);
	}else if(vb->cfg->use_vboxebf4){
		wtk_vboxebf4_ssl_delay_new(vb->vebf4);
	}else if(vb->cfg->use_vboxebf6){
		wtk_vboxebf6_ssl_delay_new(vb->vebf6);
	}
}

void qtk_vboxebf_set_agcenable(qtk_vboxebf_t *vb, int enable)
{
	printf("set agc enable ==> %d\n",enable);
	vb->cfg->agc_enable = enable;
	if(vb->cfg->use_vboxebf3){
		wtk_vboxebf3_set_agcenable(vb->vebf3, vb->cfg->agc_enable);
	}else if(vb->cfg->use_vboxebf6){
		wtk_vboxebf6_set_agcenable(vb->vebf6, vb->cfg->agc_enable);
	}
}

void qtk_vboxebf_set_echoenable(qtk_vboxebf_t *vb, int enable)
{
	printf("set echo enable ==> %d\n",enable);
	vb->cfg->echo_enable=enable;
	if(vb->cfg->use_vboxebf3){
		wtk_vboxebf3_set_echoenable(vb->vebf3, vb->cfg->echo_enable);
	}else if(vb->cfg->use_vboxebf6){
		wtk_vboxebf6_set_echoenable(vb->vebf6, vb->cfg->echo_enable);
	}
}

void qtk_vboxebf_set_denoiseenable(qtk_vboxebf_t *vb, int enable)
{
	printf("set denoise enable ==> %d\n",enable);
	vb->cfg->denoise_enable=enable;
	if(vb->cfg->use_vboxebf3){
		wtk_vboxebf3_set_denoiseenable(vb->vebf3, vb->cfg->denoise_enable);
	}else if(vb->cfg->use_vboxebf6){
		wtk_vboxebf6_set_denoiseenable(vb->vebf6, vb->cfg->denoise_enable);
	}
}

void qtk_vboxebf_set_ssl_enable(qtk_vboxebf_t *vb, int enable)
{
	printf("set ssl enable ==> %d\n",enable);
	if(vb->cfg->use_vboxebf3){
		wtk_debug("set ssl enable for vboxebf3\n");
		wtk_vboxebf3_set_sslenable(vb->vebf3, enable);
		vb->cfg->ssl_enable = enable;
	}
}

static int qtk_vboxebf_on_feed3(qtk_vboxebf_t *vb, char *data, int bytes, int is_end)
{
	if(bytes > 0){
		// double tm,tm1,tm2,tm3;
		// tm = time_get_ms();
		if(vb->is_start && bytes < vb->winslen){
			vb->is_start=0;
			qtk_vboxebf_on_feed3(vb, vb->zdata, vb->winslen-bytes, 0);
		}

		if(vb->cfg->mic_shift != 1.0f || vb->cfg->spk_shift != 1.0f){
			qtk_data_change_vol2(data, bytes, vb->cfg->mic_shift, vb->cfg->spk_shift,vb->channel+vb->cfg->nskip-vb->cfg->spk_channel, vb->cfg->spk_channel);
		}

		if(vb->cfg->use_log_wav && vb->micwav){
			wtk_wavfile_write(vb->micwav, data, bytes);
		}

		int nx=(bytes>>1)/vb->channel;
		if(vb->cfg->use_vboxebf6){
			wtk_vboxebf6_feed(vb->vebf6, (short *)data, nx, 0);
		}else if(vb->cfg->use_vboxebf4){
			wtk_vboxebf4_feed(vb->vebf4, (short *)data, nx, 0);
		}else if(vb->cfg->use_vboxebf3){
			wtk_vboxebf3_feed(vb->vebf3, (short *)data, nx, 0);
		}else if(vb->cfg->use_ahs){
#ifdef USE_AHS
			qtk_ahs_feed(vb->ahs, (short *)data, nx, 0);
#endif
		}
		// tm3=time_get_ms();
		// tm3=tm3-tm2;
		// if(tm3 >= nx/16.0)
		// {
		// 	wtk_debug("===================>>>tm1=%f tm2=%f tm3=%f rtime=%f\n",tm1,tm2,tm3,nx/16.0);
		// }
	}
	if(is_end){
		if(vb->cfg->use_vboxebf6){
			wtk_vboxebf6_feed(vb->vebf6, NULL, 0, 1);
		}else if(vb->cfg->use_vboxebf4){
			wtk_vboxebf4_feed(vb->vebf4, NULL, 0, 1);
		}else if(vb->cfg->use_vboxebf3){
			wtk_vboxebf3_feed(vb->vebf3, NULL, 0, 1);
		}else if(vb->cfg->use_ahs){
#ifdef USE_AHS
			qtk_ahs_feed(vb->ahs, NULL, 0, 1);
#endif
		}
	}
	return 0;
}

static int qtk_vboxebf_on_feed2(qtk_vboxebf_t *vb, char *data, int bytes, int is_end)
{
	if(bytes > 0){
		// double tm,tm1,tm2,tm3;
		// tm = time_get_ms();
		if(vb->is_start && bytes < vb->winslen){
			vb->is_start=0;
			qtk_vboxebf_on_feed2(vb, vb->zdata, vb->winslen-bytes, 0);
		}

		if(vb->cfg->mic_shift != 1.0f || vb->cfg->spk_shift != 1.0f){
			// wtk_debug("==============================>>>>>>>>>>>>%f %f\n",vb->cfg->mic_shift,vb->cfg->spk_shift);
			qtk_data_change_vol2(data, bytes, vb->cfg->mic_shift, vb->cfg->spk_shift,vb->channel+vb->cfg->nskip-vb->cfg->spk_channel, vb->cfg->spk_channel);
		}
		// tm1=time_get_ms();
		// tm = tm1-tm;
		if(vb->cfg->use_log_wav && vb->micwav){
			wtk_wavfile_write(vb->micwav, data, bytes);
		}
		// tm2=time_get_ms();
		// tm1=tm2-tm1;
		// fwrite(data, bytes, 1, skipf);
		// fflush(skipf);
	#if 0
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
			wtk_vboxebf2_feed(vb->vebf, vb->buf, len, 0);
		}
	#endif
		int nx=(bytes>>1)/vb->channel;
		if(vb->cfg->use_vboxebf6){
			wtk_vboxebf6_feed(vb->vebf6, (short *)data, nx, 0);
		}else if(vb->cfg->use_vboxebf4){
			wtk_vboxebf4_feed(vb->vebf4, (short *)data, nx, 0);
		}else if(vb->cfg->use_vboxebf3){
			wtk_vboxebf3_feed(vb->vebf3, (short *)data, nx, 0);
		}else if(vb->cfg->use_mask_bf_net){
			wtk_mask_bf_net_feed(vb->mask_bf_net, (short *)data, nx, 0);
		}else if(vb->cfg->use_ahs){
			// double tm;
			// tm=time_get_ms();
#ifdef USE_AHS
			qtk_ahs_feed(vb->ahs, (short *)data, nx, 0);
#endif
			// tm=time_get_ms() - tm;
			// if(tm > 100)
			// {
				// wtk_debug("=====================>>>>>>>>>>nx=%d tm=%f\n",nx,tm);
			// }
		}
		// tm3=time_get_ms();
		// tm3=tm3-tm2;
		// if(tm3 >= nx/16.0)
		// {
		// 	wtk_debug("===================>>>tm1=%f tm2=%f tm3=%f rtime=%f\n",tm1,tm2,tm3,nx/16.0);
		// }
	}
	if(is_end){
		if(vb->cfg->use_vboxebf6){
			wtk_vboxebf6_feed(vb->vebf6, NULL, 0, 1);
		}else if(vb->cfg->use_vboxebf4){
			wtk_vboxebf4_feed(vb->vebf4, NULL, 0, 1);
		}else if(vb->cfg->use_vboxebf3){
			wtk_vboxebf3_feed(vb->vebf3, NULL, 0, 1);
		}else if(vb->cfg->use_mask_bf_net){
			wtk_mask_bf_net_feed(vb->mask_bf_net, NULL, 0, 1);
		}else if(vb->cfg->use_ahs){
#ifdef USE_AHS
			qtk_ahs_feed(vb->ahs, NULL, 0, 1);
#endif
		}
	}
	if(vb->cfg->use_cache_mode == 1){
		int nx=bytes/vb->channel;
		if(vb->out_buf->pos >= nx){
			if(vb->notify){
				vb->notify(vb->ths, vb->out_buf->data, nx);
			}
			qtk_var_t var;
			if(vb->enotify){
				//wtk_debug("========================>>>>>>>>>>>>>>>>>%d\n",len);
				var.type = QTK_SPEECH_DATA_PCM;
				var.v.str.data = vb->out_buf->data;
				var.v.str.len = nx;
				vb->enotify(vb->eths, &var);
			}
			wtk_strbuf_pop(vb->out_buf, NULL, nx);
		}
	}else{
	#ifdef USE_32MSTO8MS
		if(vb->out_buf->pos >= 32*8){
			if(vb->notify){
				vb->notify(vb->ths, vb->out_buf->data, 8*32);
			}
			qtk_var_t var;
			if(vb->enotify){
				//wtk_debug("========================>>>>>>>>>>>>>>>>>%d\n",len);
				var.type = QTK_SPEECH_DATA_PCM;
				var.v.str.data = vb->out_buf->data;
				var.v.str.len = 8*32;
				vb->enotify(vb->eths, &var);
			}
			wtk_strbuf_pop(vb->out_buf, NULL, 8*32);
		}
	#endif
	}
	return 0;
}

static int qtk_vboxebf_on_feed(qtk_vboxebf_t *vb, char *data, int bytes, int is_end)
{
	if(bytes > 0){
		// double tm,tm1,tm2,tm3;
		// tm = time_get_ms();
		if(vb->cfg->mic_shift != 1.0f || vb->cfg->spk_shift != 1.0f){
			// wtk_debug("==============================>>>>>>>>>>>>%f %f\n",vb->cfg->mic_shift,vb->cfg->spk_shift);
			qtk_data_change_vol2(data, bytes, vb->cfg->mic_shift, vb->cfg->spk_shift,vb->channel+vb->cfg->nskip-vb->cfg->spk_channel, vb->cfg->spk_channel);
		}
		// tm1=time_get_ms();
		// tm = tm1-tm;
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
		if(vb->cfg->use_log_wav && vb->micwav){
			wtk_wavfile_write(vb->micwav, data, bytes);
		}
		// tm2=time_get_ms();
		// tm1=tm2-tm1;
		// fwrite(data, bytes, 1, skipf);
		// fflush(skipf);
	#if 0
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
			wtk_vboxebf2_feed(vb->vebf, vb->buf, len, 0);
		}
	#endif
		int nx=(bytes>>1)/vb->channel;
		if(vb->cfg->use_vboxebf6){
			wtk_vboxebf6_feed(vb->vebf6, (short *)data, nx, 0);
		}else if(vb->cfg->use_vboxebf4){
			wtk_vboxebf4_feed(vb->vebf4, (short *)data, nx, 0);
		}else if(vb->cfg->use_vboxebf3){
			wtk_vboxebf3_feed(vb->vebf3, (short *)data, nx, 0);
		}else if(vb->cfg->use_mask_bf_net){
			wtk_mask_bf_net_feed(vb->mask_bf_net, (short *)data, nx, 0);
		}
		else if(vb->cfg->use_ahs){
			// double tm;
			// tm=time_get_ms();
#ifdef USE_AHS
			qtk_ahs_feed(vb->ahs, (short *)data, nx, 0);
#endif
			// tm=time_get_ms() - tm;
			// if(tm > 100)
			// {
				// wtk_debug("=====================>>>>>>>>>>nx=%d tm=%f\n",nx,tm);
			// }
		}
		// tm3=time_get_ms();
		// tm3=tm3-tm2;
		// if(tm3 >= nx/16.0)
		// {
		// 	wtk_debug("===================>>>tm1=%f tm2=%f tm3=%f rtime=%f\n",tm1,tm2,tm3,nx/16.0);
		// }
	}
	if(is_end){
		if(vb->cfg->use_vboxebf6){
			wtk_vboxebf6_feed(vb->vebf6, NULL, 0, 1);
		}else if(vb->cfg->use_vboxebf4){
			wtk_vboxebf4_feed(vb->vebf4, NULL, 0, 1);
		}else if(vb->cfg->use_vboxebf3){
			wtk_vboxebf3_feed(vb->vebf3, NULL, 0, 1);
		}else if(vb->cfg->use_mask_bf_net){
			wtk_mask_bf_net_feed(vb->mask_bf_net, NULL, 0, 1);
		}
		else if(vb->cfg->use_ahs){
#ifdef USE_AHS
			qtk_ahs_feed(vb->ahs, NULL, 0, 1);
#endif
		}
	}
	return 0;
}

int qtk_vboxebf_feed(qtk_vboxebf_t *vb, char *data, int len, int is_end)
{
	// rrlen+=(len/(240));
#if 0
	int pos = 0;
	int step = 0;
	int flen;
	int channelx2=2*(vb->channel+vb->cfg->nskip);
	
	wtk_strbuf_push(vb->cache_buf, data, len);
	len=vb->cache_buf->pos/channelx2*channelx2;

	step = FEED_STEP * (vb->channel+vb->cfg->nskip);
	while(pos < len){
		// if(pos+step > len)
		// {
		// 	break;
		// }
		flen = min(step, len-pos);
		qtk_vboxebf_on_feed(vb, vb->cache_buf->data + pos, flen, 0);
		// pos += step;
		pos +=flen;
	}
	wtk_strbuf_pop(vb->cache_buf, NULL, pos);
	if(is_end){
		qtk_vboxebf_on_feed(vb, NULL, 0, 1);
	}
#else
#ifdef USE_RESAMPLE_BYTES
	wtk_strbuf_reset(vb->sample_bytes_in);
	int pos=0,i;
	int32_t tsample;
	short rsample=0;
	while(pos < len){
		for(i=0;i<vb->channel;++i)
		{
			tsample = ((int32_t *)(data+pos))[0];
			rsample = (short)(tsample >> 16);
			// wtk_debug("=============>>>>>>>>>>>>>pos=%d .tsample=%d rsample=%d\n",pos ,tsample, rsample);
			wtk_strbuf_push(vb->sample_bytes_in, (char *)(&rsample), 2);
			pos+=4;
		}
	}
	data = vb->sample_bytes_in->data;
	len = vb->sample_bytes_in->pos;
#endif
	if(vb->cfg->use_cache_mode == 1){
		qtk_vboxebf_on_feed2(vb, data, len, is_end);
		wtk_debug("--------------------->>>>>>>>>>>>>>>>>>..\n");
	}else{
	#ifdef USE_32MSTO8MS
		qtk_vboxebf_on_feed2(vb, data, len, is_end);
	#else
		qtk_vboxebf_on_feed(vb, data, len, is_end);
	#endif
	}
#endif
	return 0;
}

int qtk_vboxebf_feed2(qtk_vboxebf_t *vb, char *intput, int in_bytes, char *output, int *out_bytes, int is_end)
{
#ifdef USE_RESAMPLE_BYTES
	wtk_strbuf_reset(vb->sample_bytes_in);
	int pos=0,i;
	int32_t tsample;
	short rsample=0;
	while(pos < in_bytes){
		for(i=0;i<vb->channel;++i)
		{
			tsample = ((int32_t *)(intput+pos))[0];
			rsample = (short)(tsample >> 16);
			// wtk_debug("=============>>>>>>>>>>>>>pos=%d .tsample=%d rsample=%d\n",pos ,tsample, rsample);
			wtk_strbuf_push(vb->sample_bytes_in, (char *)(&rsample), 2);
			pos+=4;
		}
	}
	intput = vb->sample_bytes_in->data;
	in_bytes = vb->sample_bytes_in->pos;
#endif
	qtk_vboxebf_on_feed3(vb, intput, in_bytes, is_end);
	int nx=in_bytes/vb->channel;
	if(vb->out_buf->pos >= nx){
		if(output != NULL){
			memcpy(output, vb->out_buf->data, nx);
		}
		if(out_bytes != NULL){
			*out_bytes = nx;
		}
		wtk_strbuf_pop(vb->out_buf, NULL, nx);
	}
	return 0;
}

void qtk_vboxebf_set_notify(qtk_vboxebf_t *vb, void *ths, qtk_vboxebf_notify_f notify)
{
	vb->ths = ths;
	vb->notify = notify;
}

void qtk_vboxebf_set_notify2(qtk_vboxebf_t *vb, void *ths, qtk_engine_notify_f notify)
{
	vb->eths=ths;
	vb->enotify=notify;
}

//int cache_count;
//#define CACHE_STEP  (8)
void qtk_mask_bf_net_on(qtk_vboxebf_t *vb, short *data, int len) {
    qtk_var_t var;
    // printf("%d\n",len);
   if(vb->cfg->use_log_wav && vb->echowav){
		wtk_wavfile_write(vb->echowav, (char *)data, len<<1);
	}
	if(vb->cfg->echo_shift != 1.0f){
		qtk_data_change_vol((char *)data, len<<1, vb->cfg->echo_shift);
	}
	if(vb->cfg->use_cache_mode == 1){
		wtk_strbuf_push(vb->out_buf, (char *)data, len<<1);
	}else{
	#ifdef USE_32MSTO8MS
		wtk_strbuf_push(vb->out_buf, (char *)data, len<<1);
	#else
		if(vb->notify){
			vb->notify(vb->ths, (char *)data, len<<1);
		}
		if(vb->enotify){
			//wtk_debug("========================>>>>>>>>>>>>>>>>>%d\n",len);
			var.type = QTK_SPEECH_DATA_PCM;
			var.v.str.data = (char *)data;
			var.v.str.len = len<<1;
			vb->enotify(vb->eths, &var);
		}
	#endif
	}
}
void qtk_vboxebf_on_data(qtk_vboxebf_t *vb, short *data, int len)
{
	qtk_var_t var;

	if(vb->cfg->use_log_wav && vb->echowav){
		wtk_wavfile_write(vb->echowav, (char *)data, len<<1);
	}

#ifdef USE_RESAMPLE_BYTES
	wtk_strbuf_reset(vb->sample_bytes_out);
	int pos=0,i;
	short tsample;
	int32_t rsample=0;
	while(pos < len){
		tsample = (data+pos)[0];
		rsample = (int32_t)(tsample << 16);
		// wtk_debug("=============>>>>>>>>>>>>>pos=%d .tsample=%d rsample=%d\n",pos ,tsample, rsample);
		wtk_strbuf_push(vb->sample_bytes_out, (char *)(&rsample), 4);
		pos++;
	}
	data = (short *)(vb->sample_bytes_out->data);
	len = vb->sample_bytes_out->pos>>1;
#endif
	if(vb->cfg->echo_shift != 1.0f){
		qtk_data_change_vol((char *)data, len<<1, vb->cfg->echo_shift);
	}
	// cvlen+=(len/24);
	// if(rrlen-cvlen > 32)
	// {
	// 	wtk_debug("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv==> %d\n",rrlen - cvlen);
	// }
#if 0 //def USE_R328
	wtk_strbuf_push(vb->out_buf, (char *)data, len<<1);

#if 1
	while(vb->out_buf->pos > 320)
	{
		// if(vb->cfg->echo_shift != 1.0f)
		// {
		// 	qtk_data_change_vol((char *)data, len<<1, vb->cfg->echo_shift);
		// }
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
#else
	if(vb->cfg->use_cache_mode == 1){
		wtk_strbuf_push(vb->out_buf, (char *)data, len<<1);
	}else{
	#ifdef USE_32MSTO8MS
		wtk_strbuf_push(vb->out_buf, (char *)data, len<<1);
	#else
		if(vb->notify){
			vb->notify(vb->ths, (char *)data, len<<1);
		}
		if(vb->enotify){
			//wtk_debug("========================>>>>>>>>>>>>>>>>>%d\n",len);
			var.type = QTK_SPEECH_DATA_PCM;
			var.v.str.data = (char *)data;
			var.v.str.len = len<<1;
			vb->enotify(vb->eths, &var);
		}
	#endif
	}
#endif

}

void qtk_vboxebf_on_eng(qtk_vboxebf_t *vb, float energy, float snr)
{
	qtk_var_t var;
	
	if (vb->enotify){
		var.type = QTK_AUDIO_ENERGY;
		var.v.ff.energy = energy;
		var.v.ff.snr = snr;
		vb->enotify(vb->eths, &(var));
	}
}

void qtk_vboxebf_on_ssl_new(qtk_vboxebf_t *vb, float ts, float te, wtk_ssl2_extp_t *nbest_extp, int nbest)
{
	qtk_var_t var;
	int i;

	if(vb->cfg->use_ssl_filter == 0){
		for(i=0; i<nbest; ++i)
		{
			// printf("nbest=%d/%f theta=%d phi=%d\n",nbest, nbest_extp[i].nspecsum, nbest_extp[i].theta, nbest_extp[i].phi);
			if(nbest_extp[i].nspecsum > vb->cfg->energy_sum){
				// printf("nbest=%d/%d theta=%d phi=%d\n",nbest, i, nbest_extp[i].theta, nbest_extp[i].phi);
				if (vb->enotify){
					var.type = QTK_AEC_DIRECTION;
					var.v.ii.nbest = i;
					var.v.ii.theta = nbest_extp[i].theta;
					var.v.ii.phi = nbest_extp[i].phi;
					var.v.ii.nspecsum = nbest_extp[i].nspecsum;
					vb->enotify(vb->eths, &(var));
				}
	#ifdef USE_BM10
				if(vb->cfg->theta_fn.len > 0){
					char theta[10]={0};
					int ret;
					ret = snprintf(theta, 10, "%d", nbest_extp[i].theta);
					if(ret > 0){
						file_write_buf(vb->cfg->theta_fn.data,theta,ret);
					}
				}
	#endif
			}else{
				// wtk_debug("================>>>>>>>-1\n");
				if (vb->enotify){
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
		static int first=1;
		int errnum[10]={0};
		for(i=0; i<nbest; ++i)
		{
			if(nbest_extp[i].nspecsum > vb->cfg->energy_sum){
				if(vb->use_line){
					if((nbest_extp[i].theta == 180 || nbest_extp[i].theta == 0) && (nbest_extp[i].nspecsum < vb->cfg->zero_sum)){
						vb->concount[i]=0;
						errnum[i]=1;
					}
				}
			}else{
				vb->concount[i]=0;
				errnum[i]=1;
			}
		}
		if(first){
			for(i=0; i<nbest; ++i)
			{
				if(errnum[i]){continue;}
				vb->nbest_theta[i]=nbest_extp[i].theta;
				vb->nbest_phi[i]=nbest_extp[i].phi;
				//printf("nbest=%d/%d theta=%d phi=%d\n",nbest, i, nbest_extp[i].theta, nbest_extp[i].phi);
				if (vb->enotify){
					var.type = QTK_AEC_DIRECTION;
					var.v.ii.nbest = i;
					var.v.ii.theta = nbest_extp[i].theta;
					var.v.ii.phi = nbest_extp[i].phi;
					var.v.ii.nspecsum = nbest_extp[i].nspecsum;
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

				if(err > vb->cfg->theta_range){
					vb->concount[i]++;
				}else{
					vb->concount[i] = 0;
				}

				if(vb->concount[i] >= vb->cfg->continue_count){
					vb->nbest_theta[i] = nbest_extp[i].theta;
					vb->concount[i] = 0;

					//printf("nbest=%d/%d theta=%d phi=%d\n",nbest, i, nbest_extp[i].theta, nbest_extp[i].phi);
					if (vb->enotify){
						var.type = QTK_AEC_DIRECTION;
						var.v.ii.nbest = i;
						var.v.ii.theta = vb->nbest_theta[i];
						var.v.ii.phi = nbest_extp[i].phi;
						var.v.ii.nspecsum = nbest_extp[i].nspecsum;
						vb->enotify(vb->eths, &(var));
					}
				}
			}
		}
	}
}

void qtk_vboxebf_on_ssl(qtk_vboxebf_t *vb, wtk_ssl2_extp_t *nbest_extp, int nbest)
{
	qtk_var_t var;
	int i;

	// printf("nbest=%d/%f theta=%d phi=%d\n",nbest, nbest_extp[0].nspecsum, nbest_extp[0].theta, nbest_extp[0].phi);
	if(vb->cfg->use_ssl_filter == 0){
		static int errcnt=0;
		for(i=0; i<nbest; ++i)
		{
			if(nbest_extp[i].nspecsum > vb->cfg->energy_sum){
				if(nbest_extp[i].theta == 180 || nbest_extp[i].theta == 0){
					errcnt++;
				}else{
					errcnt=0;
				}
				if(errcnt <= vb->cfg->continue_count){
					// printf("nbest=%d/%d theta=%d phi=%d\n",nbest, i, nbest_extp[i].theta, nbest_extp[i].phi);
					if (vb->enotify){
						var.type = QTK_AEC_DIRECTION;
						var.v.ii.nbest = i;
						var.v.ii.theta = nbest_extp[i].theta;
						var.v.ii.phi = nbest_extp[i].phi;
						var.v.ii.nspecsum = nbest_extp[i].nspecsum;
						vb->enotify(vb->eths, &(var));
					}
	#ifdef USE_BM10
					if(vb->cfg->theta_fn.len > 0){
						char theta[10]={0};
						int ret;
						ret = snprintf(theta, 10, "%d", nbest_extp[i].theta);
						if(ret > 0){
							file_write_buf(vb->cfg->theta_fn.data,theta,ret);
						}
					}
	#endif
				}else{
					// wtk_debug("================>>>>>>>-1\n");
					if (vb->enotify){
						var.type = QTK_AEC_DIRECTION;
						var.v.ii.nbest = i;
						var.v.ii.theta = -1;
						var.v.ii.phi = -1;
						var.v.ii.nspecsum = 0.0;
						vb->enotify(vb->eths, &(var));
					}
				}
			}else{
				// wtk_debug("================>>>>>>>-1\n");
				if (vb->enotify){
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
		static int first=1;
		int errnum[10]={0};
		for(i=0; i<nbest; ++i)
		{
			if(nbest_extp[i].nspecsum > vb->cfg->energy_sum){
				if((nbest_extp[i].theta == 180 || nbest_extp[i].theta == 0) && (nbest_extp[i].nspecsum < vb->cfg->zero_sum)){
					vb->concount[i]=0;
					errnum[i]=1;
				}
			}else{
				vb->concount[i]=0;
				errnum[i]=1;
			}
		}
		if(first){
			for(i=0; i<nbest; ++i)
			{
				if(errnum[i]){continue;}
				vb->nbest_theta[i]=nbest_extp[i].theta;
				vb->nbest_phi[i]=nbest_extp[i].phi;
				//printf("nbest=%d/%d theta=%d phi=%d\n",nbest, i, nbest_extp[i].theta, nbest_extp[i].phi);
				if (vb->enotify){
					var.type = QTK_AEC_DIRECTION;
					var.v.ii.nbest = i;
					var.v.ii.theta = nbest_extp[i].theta;
					var.v.ii.phi = nbest_extp[i].phi;
					var.v.ii.nspecsum = nbest_extp[i].nspecsum;
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

				if(err > vb->cfg->theta_range){
					vb->concount[i]++;
				}else{
					vb->concount[i] = 0;
				}

				if(vb->concount[i] >= vb->cfg->continue_count){
					vb->nbest_theta[i] = nbest_extp[i].theta;
					vb->concount[i] = 0;
					//printf("nbest=%d/%d theta=%d phi=%d\n",nbest, i, nbest_extp[i].theta, nbest_extp[i].phi);
					if (vb->enotify){
						var.type = QTK_AEC_DIRECTION;
						var.v.ii.nbest = i;
						var.v.ii.theta = vb->nbest_theta[i];
						var.v.ii.phi = nbest_extp[i].phi;
						var.v.ii.nspecsum = nbest_extp[i].nspecsum;
						vb->enotify(vb->eths, &(var));
					}
				}
			}
		}
	}
}
