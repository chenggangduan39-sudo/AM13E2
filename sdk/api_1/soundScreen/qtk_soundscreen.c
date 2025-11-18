#include "qtk_soundscreen.h"

void qtk_soundscreen_on_qform_cb0(qtk_soundscreen_t *sc, short *data, int len, int is_end);
void qtk_soundscreen_on_qform_cb1(qtk_soundscreen_t *sc, short *data, int len, int is_end);
void qtk_soundscreen_on_qform_cb2(qtk_soundscreen_t *sc, short *data, int len, int is_end);
void qtk_soundscreen_on_qform_cb3(qtk_soundscreen_t *sc, short *data, int len, int is_end);
void qtk_soundscreen_on_qform_cb4(qtk_soundscreen_t *sc, short *data, int len, int is_end);

void qtk_soundscreen_on_beamnet_cb0(qtk_soundscreen_t *sc, short *data, int len);
void qtk_soundscreen_on_beamnet_cb1(qtk_soundscreen_t *sc, short *data, int len);
void qtk_soundscreen_on_beamnet_cb2(qtk_soundscreen_t *sc, short *data, int len);
void qtk_soundscreen_on_beamnet_cb3(qtk_soundscreen_t *sc, short *data, int len);
void qtk_soundscreen_on_beamnet_cb4(qtk_soundscreen_t *sc, short *data, int len);
void qtk_soundscreen_on_qform_center(qtk_soundscreen_t *sc, short **data, int len, int is_end);
void qtk_soundscreen_on_qform_data(qtk_soundscreen_t *sc, short *data, int len, int is_end);
void qtk_soundscreen_on_aec_data(qtk_soundscreen_t *sc, short **data, int len, int is_end);
void qtk_soundscreen_on_vboxebf_data(qtk_soundscreen_t *sc, short *data, int len);
void qtk_soundscreen_on_cmask_bfse_data(qtk_soundscreen_t *sc, short *data, int len);

// double slen=0.0;
// double clen=0.0;

#define FEED_STEP (32 * 16 * 2)
qtk_soundscreen_t *qtk_soundscreen_new(qtk_soundscreen_cfg_t *cfg)
{
	qtk_soundscreen_t *sc;
	int ret;
	int i;

	sc = (qtk_soundscreen_t *)wtk_calloc(1, sizeof(*sc));
	sc->cfg = cfg;
	sc->aec = NULL;
	sc->vboxebf3 = NULL;
	sc->buf = NULL;
	sc->bufs = NULL;
	sc->channel = 4;
	sc->data = NULL;
	sc->vboxdata = NULL;
	sc->enotify = NULL;
	sc->eths = NULL;
	sc->input = NULL;
	sc->notify = NULL;
	sc->qform = NULL;
	sc->beamnet2 = NULL;
	sc->beamnet3 = NULL;
	sc->beamnet4 = NULL;
	sc->cmask_bfse = NULL;
	sc->ths = NULL;
	
	if(sc->cfg->n_theta > 5){
		wtk_debug("Supports up to 5 sectors\n");
		ret = -1;
		goto end;
	}
	
	if(sc->cfg->use_aec){
		sc->aec = wtk_aec_new(sc->cfg->aec_cfg);
		if(!sc->aec){
			wtk_debug("aec new failed.\n");
			ret = -1;
			goto end;
		}
		wtk_aec_set_notify(sc->aec, sc, (wtk_aec_notify_f)qtk_soundscreen_on_aec_data);
		sc->channel = sc->cfg->aec_cfg->stft.channel;
	}else if(sc->cfg->use_vboxebf){
		sc->vboxebf3 = wtk_vboxebf3_new(sc->cfg->vboxebf3_cfg);
		wtk_vboxebf3_set_notify(sc->vboxebf3, sc, (wtk_vboxebf3_notify_f)qtk_soundscreen_on_vboxebf_data);
		sc->channel = sc->cfg->vboxebf3_cfg->channel;
	} else{
		if(sc->cfg->use_qform9){
			sc->channel = sc->cfg->qform9_cfg->stft2.channel;
		}
		if(sc->cfg->use_beamnet2){
			sc->channel = sc->cfg->beamnet2_cfg->channel;
		}
		if(sc->cfg->use_beamnet3){
			sc->channel = sc->cfg->beamnet3_cfg->channel;
		}
		if(sc->cfg->use_beamnet4){
			sc->channel = sc->cfg->beamnet4_cfg->channel;
		}
	}
	wtk_debug("===============>>>>>>>>>>>>>n_theta=%d channel=%d\n",sc->cfg->n_theta ,sc->channel);

	if(sc->cfg->use_center){
		sc->qform = (wtk_qform9_t **)wtk_malloc(sizeof(wtk_qform9_t *) * 1);
		sc->qform[0] = wtk_qform9_new(sc->cfg->qform9_cfg);
		if(!sc->qform[0]){
			wtk_debug("qfom9 new failed.\n");
			ret = -1;
			goto end;
		}
		if(sc->cfg->qform9_cfg->use_two_channel){
			wtk_qform9_set_notify_two_channel(sc->qform[0], sc, (wtk_qform9_notify_two_channel_f)qtk_soundscreen_on_qform_center);
		}else{
			wtk_qform9_set_notify(sc->qform[0], sc, (wtk_qform9_notify_f)qtk_soundscreen_on_qform_data);
		}
	}else{
		if(sc->cfg->use_qform9){
			sc->qform = (wtk_qform9_t **)wtk_malloc(sizeof(wtk_qform9_t *) * sc->cfg->n_theta);
#ifdef USE_DESAIXIWEI
			if(sc->cfg->n_theta >= 1){
				sc->qform[0] = wtk_qform9_new(sc->cfg->qform9_cfg);
			}
			if(sc->cfg->n_theta >= 2){
				sc->qform[1] = wtk_qform9_new(sc->cfg->qform9_cfg);
			}
			if(sc->cfg->qform9_cfg2){
				if(sc->cfg->n_theta >= 3){
					sc->qform[2] = wtk_qform9_new(sc->cfg->qform9_cfg2);
				}
				if(sc->cfg->n_theta >= 4){
					sc->qform[3] = wtk_qform9_new(sc->cfg->qform9_cfg2);
				}
			}else{
				if(sc->cfg->n_theta >= 3){
					sc->qform[2] = wtk_qform9_new(sc->cfg->qform9_cfg);
				}
				if(sc->cfg->n_theta >= 4){
					sc->qform[3] = wtk_qform9_new(sc->cfg->qform9_cfg);
				}
			}
#else
			for(i = 0; i < sc->cfg->n_theta; i++){
				sc->qform[i] = wtk_qform9_new(sc->cfg->qform9_cfg);
				if(!sc->qform[i]){
					wtk_debug("qfom9 new failed.\n");
					ret = -1;
					goto end;
				}
			}
#endif
			if(sc->cfg->n_theta >= 1){
				wtk_qform9_set_notify(sc->qform[0], sc, (wtk_qform9_notify_f)qtk_soundscreen_on_qform_cb0);
			}
			if(sc->cfg->n_theta >= 2){
				wtk_qform9_set_notify(sc->qform[1], sc, (wtk_qform9_notify_f)qtk_soundscreen_on_qform_cb1);
			}
			if(sc->cfg->n_theta >= 3){
				wtk_qform9_set_notify(sc->qform[2], sc, (wtk_qform9_notify_f)qtk_soundscreen_on_qform_cb2);
			}
			if(sc->cfg->n_theta >= 4){
				wtk_qform9_set_notify(sc->qform[3], sc, (wtk_qform9_notify_f)qtk_soundscreen_on_qform_cb3);
			}
			if(sc->cfg->n_theta >= 5){
				wtk_qform9_set_notify(sc->qform[4], sc, (wtk_qform9_notify_f)qtk_soundscreen_on_qform_cb4);
			}
		}
		if(sc->cfg->use_beamnet2){
			sc->beamnet2 = (wtk_beamnet2_t **)wtk_malloc(sizeof(wtk_beamnet2_t *) * sc->cfg->n_theta);
			for(i = 0; i < sc->cfg->n_theta; i++){
				sc->beamnet2[i] = wtk_beamnet2_new(sc->cfg->beamnet2_cfg);
				if(!sc->beamnet2[i]){
					wtk_debug("beamnet2 new failed.\n");
					ret = -1;
					goto end;
				}
			}
			if(sc->cfg->n_theta >= 1){
				wtk_beamnet2_set_notify(sc->beamnet2[0], sc, (wtk_beamnet2_notify_f)qtk_soundscreen_on_beamnet_cb0);
			}
			if(sc->cfg->n_theta >= 2){
				wtk_beamnet2_set_notify(sc->beamnet2[1], sc, (wtk_beamnet2_notify_f)qtk_soundscreen_on_beamnet_cb1);
			}
			if(sc->cfg->n_theta >= 3){
				wtk_beamnet2_set_notify(sc->beamnet2[2], sc, (wtk_beamnet2_notify_f)qtk_soundscreen_on_beamnet_cb2);
			}
			if(sc->cfg->n_theta >= 4){
				wtk_beamnet2_set_notify(sc->beamnet2[3], sc, (wtk_beamnet2_notify_f)qtk_soundscreen_on_beamnet_cb3);
			}
			if(sc->cfg->n_theta >= 5){
				wtk_beamnet2_set_notify(sc->beamnet2[4], sc, (wtk_beamnet2_notify_f)qtk_soundscreen_on_beamnet_cb4);
			}
		}
		if(sc->cfg->use_beamnet3){
			sc->beamnet3 = (wtk_beamnet3_t **)wtk_malloc(sizeof(wtk_beamnet3_t *) * sc->cfg->n_theta);
			for(i = 0; i < sc->cfg->n_theta; i++){
				sc->beamnet3[i] = wtk_beamnet3_new(sc->cfg->beamnet3_cfg);
				if(!sc->beamnet3[i]){
					wtk_debug("beamnet3 new failed.\n");
					ret = -1;
					goto end;
				}
			}
			if(sc->cfg->n_theta >= 1){
				wtk_beamnet3_set_notify(sc->beamnet3[0], sc, (wtk_beamnet3_notify_f)qtk_soundscreen_on_beamnet_cb0);
			}
			if(sc->cfg->n_theta >= 2){
				wtk_beamnet3_set_notify(sc->beamnet3[1], sc, (wtk_beamnet3_notify_f)qtk_soundscreen_on_beamnet_cb1);
			}
			if(sc->cfg->n_theta >= 3){
				wtk_beamnet3_set_notify(sc->beamnet3[2], sc, (wtk_beamnet3_notify_f)qtk_soundscreen_on_beamnet_cb2);
			}
			if(sc->cfg->n_theta >= 4){
				wtk_beamnet3_set_notify(sc->beamnet3[3], sc, (wtk_beamnet3_notify_f)qtk_soundscreen_on_beamnet_cb3);
			}
			if(sc->cfg->n_theta >= 5){
				wtk_beamnet3_set_notify(sc->beamnet3[4], sc, (wtk_beamnet3_notify_f)qtk_soundscreen_on_beamnet_cb4);
			}
		}
		if(sc->cfg->use_beamnet4){
			sc->beamnet4 = (wtk_beamnet4_t **)wtk_malloc(sizeof(wtk_beamnet4_t *) * sc->cfg->n_theta);

#ifdef USE_DESAIXIWEI
			if(sc->cfg->n_theta >= 1){
				sc->beamnet4[0] = wtk_beamnet4_new(sc->cfg->beamnet4_cfg);
				if(!sc->beamnet4[0]){
					wtk_debug("beamnet4 new failed.\n");
					ret = -1;
					goto end;
				}
			}
			if(sc->cfg->n_theta >= 2){
				if(sc->cfg->beamnet4_cfg2){
					sc->beamnet4[1] = wtk_beamnet4_new(sc->cfg->beamnet4_cfg2);
				}else{
					sc->beamnet4[1] = wtk_beamnet4_new(sc->cfg->beamnet4_cfg);
				}
				if(!sc->beamnet4[1]){
					wtk_debug("beamnet4 new failed.\n");
					ret = -1;
					goto end;
				}
			}
			if(sc->cfg->n_theta >= 3){
				if(sc->cfg->beamnet4_cfg3){
					sc->beamnet4[2] = wtk_beamnet4_new(sc->cfg->beamnet4_cfg3);
				}else{
					sc->beamnet4[2] = wtk_beamnet4_new(sc->cfg->beamnet4_cfg);
				}
				if(!sc->beamnet4[2]){
					wtk_debug("beamnet4 new failed.\n");
					ret = -1;
					goto end;
				}
			}
			if(sc->cfg->n_theta >= 4){
				if(sc->cfg->beamnet4_cfg4){
					sc->beamnet4[3] = wtk_beamnet4_new(sc->cfg->beamnet4_cfg4);
				}else{
					sc->beamnet4[3] = wtk_beamnet4_new(sc->cfg->beamnet4_cfg);
				}
				if(!sc->beamnet4[3]){
					wtk_debug("beamnet4 new failed.\n");
					ret = -1;
					goto end;
				}
			}
#else
			for(i = 0; i < sc->cfg->n_theta; i++){
				sc->beamnet4[i] = wtk_beamnet4_new(sc->cfg->beamnet4_cfg);
				if(!sc->beamnet4[i]){
					wtk_debug("beamnet4 new failed.\n");
					ret = -1;
					goto end;
				}
			}
#endif
			if(sc->cfg->n_theta >= 1){
				wtk_beamnet4_set_notify(sc->beamnet4[0], sc, (wtk_beamnet4_notify_f)qtk_soundscreen_on_beamnet_cb0);
			}
			if(sc->cfg->n_theta >= 2){
				wtk_beamnet4_set_notify(sc->beamnet4[1], sc, (wtk_beamnet4_notify_f)qtk_soundscreen_on_beamnet_cb1);
			}
			if(sc->cfg->n_theta >= 3){
				wtk_beamnet4_set_notify(sc->beamnet4[2], sc, (wtk_beamnet4_notify_f)qtk_soundscreen_on_beamnet_cb2);
			}
			if(sc->cfg->n_theta >= 4){
				wtk_beamnet4_set_notify(sc->beamnet4[3], sc, (wtk_beamnet4_notify_f)qtk_soundscreen_on_beamnet_cb3);
			}
			if(sc->cfg->n_theta >= 5){
				wtk_beamnet4_set_notify(sc->beamnet4[4], sc, (wtk_beamnet4_notify_f)qtk_soundscreen_on_beamnet_cb4);
			}
		}
	}
	
	if(sc->cfg->use_cmask_bfse){
		sc->cmask_bfse = wtk_cmask_bfse_new(sc->cfg->cmask_bfse_cfg);
		if(!sc->cmask_bfse){
			wtk_debug("cmask_bfse new failed.\n");
			ret = -1;
			goto end;
		}
		wtk_cmask_bfse_set_notify(sc->cmask_bfse, sc, (wtk_cmask_bfse_notify_f)qtk_soundscreen_on_cmask_bfse_data);
	}

	sc->data = (short **)wtk_malloc(sizeof(short *) * sc->channel);
	for(i =0; i < sc->channel; i++){
		sc->data[i] = (short *)wtk_malloc(FEED_STEP);
	}
	if(sc->cfg->use_vboxebf){
		sc->vboxdata = (short **)wtk_malloc(sizeof(short *) * sc->cfg->vboxebf3_cfg->nmicchannel);
		for(i =0; i < sc->cfg->vboxebf3_cfg->nmicchannel; i++){
			sc->vboxdata[i] = (short *)wtk_malloc(FEED_STEP);
		}
	}
	if(sc->cfg->use_aec){
		sc->edata = (short *)wtk_malloc(sizeof(short) * sc->cfg->beamnet4_cfg->channel);
	}
	sc->bufs = wtk_strbufs_new(sc->cfg->n_theta);
	sc->buf = wtk_strbuf_new(FEED_STEP,1.0);
	sc->input = wtk_strbuf_new(FEED_STEP*sc->channel, 1.0);
	ret = 0;
end:
	if(ret != 0){
		qtk_soundscreen_delete(sc);
		sc = NULL;
	}
	return sc;
}

void qtk_soundscreen_delete(qtk_soundscreen_t *sc)
{
	int i;
	if(sc->cfg->use_center){
		wtk_qform9_delete(sc->qform[0]);
		wtk_free(sc->qform);
	}else{
		if(sc->cfg->use_qform9){
			if(sc->qform){
				for(i = 0; i < sc->cfg->n_theta; i++){
					if(sc->qform[i]){
						wtk_qform9_delete(sc->qform[i]);
					}
				}
				wtk_free(sc->qform);
			}
		}
		if(sc->cfg->use_beamnet2){
			if(sc->beamnet2){
				for(i = 0; i < sc->cfg->n_theta; i++){
					if(sc->beamnet2[i]){
						wtk_beamnet2_delete(sc->beamnet2[i]);
					}
				}
			}
			wtk_free(sc->beamnet2);
		}
		if(sc->cfg->use_beamnet3){
			if(sc->beamnet3){
				for(i = 0; i < sc->cfg->n_theta; i++){
					if(sc->beamnet3[i]){
						wtk_beamnet3_delete(sc->beamnet3[i]);
					}
				}
			}
			wtk_free(sc->beamnet3);
		}
		if(sc->cfg->use_beamnet4){
			if(sc->beamnet4){
				for(i = 0; i < sc->cfg->n_theta; i++){
					if(sc->beamnet4[i]){
						wtk_beamnet4_delete(sc->beamnet4[i]);
					}
				}
			}
			wtk_free(sc->beamnet4);
		}
	}
	if(sc->cfg->use_cmask_bfse){
		if(sc->cmask_bfse){
			wtk_cmask_bfse_delete(sc->cmask_bfse);
		}
	}
	if(sc->cfg->use_aec){
		if(sc->aec){
			wtk_aec_delete(sc->aec);
		}
	}
	if(sc->cfg->use_vboxebf){
		if(sc->vboxebf3){
			wtk_vboxebf3_delete(sc->vboxebf3);
		}
	}
	if(sc->bufs){
		wtk_strbufs_delete(sc->bufs, sc->cfg->n_theta);
	}	
	if(sc->buf){
		wtk_strbuf_delete(sc->buf);
	}
	if(sc->input){
		wtk_strbuf_delete(sc->input);
	}
	for(i = 0; i < sc->channel; i++){
		wtk_free(sc->data[i]);
	}
	wtk_free(sc->data);
	if(sc->cfg->use_vboxebf){
		for(i = 0; i < sc->cfg->vboxebf3_cfg->nmicchannel; i++){
			wtk_free(sc->vboxdata[i]);
		}
		wtk_free(sc->vboxdata);
	}
	if(sc->cfg->use_aec){
		wtk_free(sc->edata);
	}
	wtk_free(sc);
}

int qtk_soundscreen_start(qtk_soundscreen_t *sc)
{
	if(sc->cfg->use_aec){
	}
	if(sc->cfg->use_vboxebf){
		wtk_vboxebf3_start(sc->vboxebf3);
	}
	if(sc->cfg->use_center){
		wtk_qform9_start(sc->qform[0], 0.0, 0.0);
	}else{
		int i;
		if(sc->cfg->use_qform9){
			for(i = 0; i < sc->cfg->n_theta; i++){
				wtk_debug("%d\n", sc->cfg->thetas[i]);
				wtk_qform9_start(sc->qform[i], sc->cfg->thetas[i], 0);
			}
		}
		if(sc->cfg->use_beamnet2){
			for(i = 0; i < sc->cfg->n_theta; i++){
				wtk_beamnet2_start(sc->beamnet2[i], sc->cfg->thetas[i], 0);
			}
		}
		if(sc->cfg->use_beamnet3){
			for(i = 0; i < sc->cfg->n_theta; i++){
				wtk_beamnet3_start(sc->beamnet3[i], sc->cfg->thetas[i], 0);
			}
		}
		if(sc->cfg->use_beamnet4){
			for(i = 0; i < sc->cfg->n_theta; i++){
				wtk_beamnet4_start(sc->beamnet4[i]);
			}
		}
	}
	if(sc->cfg->use_cmask_bfse){
		wtk_cmask_bfse_start(sc->cmask_bfse);
	}
	wtk_strbuf_reset(sc->input);
	return 0;
}

int qtk_soundscreen_reset(qtk_soundscreen_t *sc)
{
	if(sc->cfg->use_aec){
		wtk_aec_reset(sc->aec);
	}
	if(sc->cfg->use_vboxebf){
		wtk_vboxebf3_reset(sc->vboxebf3);
	}
	if(sc->cfg->use_center){
		wtk_qform9_reset(sc->qform[0]);
	}else{
		int i;
		if(sc->cfg->use_qform9){
			for(i = 0; i < sc->cfg->n_theta; i++){
				wtk_qform9_reset(sc->qform[i]);
			}
		}
		if(sc->cfg->use_beamnet2){
			for(i = 0; i < sc->cfg->n_theta; i++){
				wtk_beamnet2_reset(sc->beamnet2[i]);
			}
		}
		if(sc->cfg->use_beamnet3){
			for(i = 0; i < sc->cfg->n_theta; i++){
				wtk_beamnet3_reset(sc->beamnet3[i]);
			}
		}
		if(sc->cfg->use_beamnet4){
			for(i = 0; i < sc->cfg->n_theta; i++){
				wtk_beamnet4_reset(sc->beamnet4[i]);
			}
		}
	}
	if(sc->cfg->use_cmask_bfse){
		wtk_cmask_bfse_reset(sc->cmask_bfse);
	}
	wtk_strbuf_reset(sc->input);
	return 0;
	
}

void qtk_soundscreen_set_notify(qtk_soundscreen_t *sc, void *ths, qtk_soundscreen_notify_f notify)
{
	sc->ths = ths;
	sc->notify = notify;
}

void qtk_soundscreen_set_notify2(qtk_soundscreen_t *sc, void *eths, qtk_engine_notify_f enotify)
{
	sc->eths = eths;
	sc->enotify = enotify;
}

int qtk_soundscreen_set_denoiseenable(qtk_soundscreen_t *sc, int enable)
{
	wtk_debug("denoise_enable=%d\n",enable);
	if(sc->cfg->use_beamnet3){
		int i;
		for(i = 0; i < sc->cfg->n_theta; i++){
			wtk_beamnet3_set_denoiseenable(sc->beamnet3[i], enable);
		}
	}
}

int qtk_soundscreen_set_noise_suppress(qtk_soundscreen_t *sc, float noise_suppress)
{
	wtk_debug("noise_suppress=%f\n",noise_suppress);
	sc->cfg->noise_suppress = noise_suppress;
	if(sc->cfg->use_beamnet3){
		int i;
		for(i = 0; i < sc->cfg->n_theta; i++){
			wtk_beamnet3_set_noise_suppress(sc->beamnet3[i], noise_suppress);
		}
	}
}

int qtk_soundscreen_set_out_scale(qtk_soundscreen_t *sc, float scale)
{
	wtk_debug("out_scale=%f\n",scale);
	if(sc->cfg->use_beamnet3){
		int i;
		for(i = 0; i < sc->cfg->n_theta; i++){
			wtk_beamnet3_set_out_scale(sc->beamnet3[i], scale);
		}
	}
}

int qtk_soundscreen_set_agcenable(qtk_soundscreen_t *sc, int enable)
{
	wtk_debug("agc_enable=%d\n",enable);
	if(sc->cfg->use_beamnet3){
		int i;
		for(i = 0; i < sc->cfg->n_theta; i++){
			wtk_beamnet3_set_agcenable(sc->beamnet3[i], enable);
		}
	}
}

static int qtk_soundscreen_on_feed(qtk_soundscreen_t *sc, char *data, int bytes, int is_end)
{
	int i, j ,k;
	short *pv = NULL;
	int len;
	// double tm;

	// slen+=(bytes/(32.0*sc->channel));
	// tm = time_get_ms();
	if(bytes > 0){
		pv = (short*)data;
		len = bytes /(sc->channel * sizeof(short));
		for(i = 0; i < len; ++i){
			for(j = 0; j < sc->channel; ++j){
				sc->data[j][i] = pv[i * sc->channel + j];
			}
		}
		// wtk_debug("==========>>>>>>>>>>>>>len=%d bytes=%d channel=%d\n", len, bytes, sc->channel);
		if(sc->cfg->use_aec){
			wtk_aec_feed(sc->aec, sc->data, len, 0);
		}else if(sc->cfg->use_vboxebf){
			wtk_vboxebf3_feed_mul(sc->vboxebf3, (short *)data, len, 0);
		}else{
			if(sc->cfg->use_center){
				wtk_qform9_feed(sc->qform[0], sc->data, len, 0);
			}else{
				if(sc->cfg->use_qform9){
					for(k = 0; k < sc->cfg->n_theta; k++){
						wtk_qform9_feed(sc->qform[k], sc->data, len, 0);
					}
				}
				if(sc->cfg->use_beamnet2){
					for(k = 0; k < sc->cfg->n_theta; k++){
						wtk_beamnet2_feed(sc->beamnet2[k], (short *)data, len, 0);
					}
				}
				if(sc->cfg->use_beamnet3){
					for(k = 0; k < sc->cfg->n_theta; k++){
						wtk_beamnet3_feed(sc->beamnet3[k], (short *)data, len, 0);
					}
				}
				if(sc->cfg->use_beamnet4){
					for(k = 0; k < sc->cfg->n_theta; k++){
						wtk_beamnet4_feed(sc->beamnet4[k], (short *)data, len, 0);
					}
				}
			}
			if(sc->cfg->use_cmask_bfse){
				wtk_cmask_bfse_feed(sc->cmask_bfse, pv, len, 0);
			}
		}
	}
	// tm = time_get_ms() - tm;
	
	// if(tm > bytes/(32.0*sc->channel))
	// {
	// 	wtk_debug("==================>>>>>>>>>>>tm=%f\n",tm);
	// }
	if(is_end){
		if(sc->cfg->use_aec){
			wtk_aec_feed(sc->aec, NULL, 0, 1);
		}else if(sc->cfg->use_vboxebf){
			wtk_vboxebf3_feed_mul(sc->vboxebf3, NULL, 0, 1);
		}else{
			if(sc->cfg->use_center){
				wtk_qform9_feed(sc->qform[0], NULL, 0, 1);
			}else{
				if(sc->cfg->use_qform9){
					for(k = 0; k < sc->cfg->n_theta; k++){
						wtk_qform9_feed(sc->qform[k], NULL, 0, 1);
					}
				}
				if(sc->cfg->use_beamnet2){
					for(k = 0; k < sc->cfg->n_theta; k++){
						wtk_beamnet2_feed(sc->beamnet2[k], NULL, 0, 1);
					}
				}
				if(sc->cfg->use_beamnet3){
					for(k = 0; k < sc->cfg->n_theta; k++){
						wtk_beamnet3_feed(sc->beamnet3[k], NULL, 0, 1);
					}
				}
				if(sc->cfg->use_beamnet4){
					for(k = 0; k < sc->cfg->n_theta; k++){
						wtk_beamnet4_feed(sc->beamnet4[k], NULL, 0, 1);
					}
				}
			}
			if(sc->cfg->use_cmask_bfse){
				wtk_cmask_bfse_feed(sc->cmask_bfse, NULL, 0, 1);
			}
		}
	}
	return 0;
}

int qtk_soundscreen_feed(qtk_soundscreen_t *sc, char *data, int len, int is_end)
{
	int pos = 0;
	int step = 0;
	int flen;
	
	wtk_strbuf_push(sc->input, data, len);
	if(sc->input->pos >= (FEED_STEP *sc->channel)){
		qtk_soundscreen_on_feed(sc, sc->input->data, FEED_STEP*sc->channel, 0);
		wtk_strbuf_pop(sc->input, NULL, FEED_STEP*sc->channel);
	}
	// step = FEED_STEP * sc->channel;
	// while(pos < len){
	// 	flen = min(step, len - pos);
	// 	qtk_soundscreen_on_feed(sc, data + pos, flen, 0);
	// 	pos += flen;
	// }
	if(is_end){
		qtk_soundscreen_on_feed(sc, NULL, 0, 1);
	}
	return 0;
}

int qtk_soundscreen_feed2(qtk_soundscreen_t *sc, short **data, int len, int is_end)
{
	if(sc->cfg->use_center){
		wtk_qform9_feed(sc->qform[0], data, len, is_end);
	}else{
		int k;
		for(k = 0;k < sc->cfg->n_theta; ++k)
		{
			wtk_qform9_feed(sc->qform[k], data, len, is_end);
		}
	}
	return 0;
}

void qtk_soundscreen_on_notify(qtk_soundscreen_t *sc, int len)
{
	// wtk_debug("===============>>>>>>>>>>>>len=%d\n",len);
	qtk_var_t var;
	int i,k=0;
	while(k < len){
		for(i = 0; i < sc->cfg->n_theta; i++){
			wtk_strbuf_push(sc->buf, sc->bufs[i]->data+k, 2);
		}
		k+=2;
	}
	// clen+=(sc->buf->pos/64.0);
	// wtk_debug("===================>>>>>>>>>>>>slen=%f clen=%f == %f\n",slen,clen,slen-clen);
	// wtk_debug("===============>>>>>>>>>>>>buf->pos=%d\n",sc->buf->pos);
	if(sc->notify){
		sc->notify(sc->ths, sc->buf->data, sc->buf->pos);
	}

	var.type = QTK_SPEECH_DATA_PCM;
	var.v.str.data = sc->buf->data;
	var.v.str.len = sc->buf->pos;
	if(sc->enotify){
		sc->enotify(sc->eths, &var);
	}

	wtk_strbufs_pop(sc->bufs, sc->cfg->n_theta, len);
	wtk_strbuf_reset(sc->buf);
}

void qtk_soundscreen_on_qform_data(qtk_soundscreen_t *sc, short *data, int len, int is_end)
{
	qtk_var_t var;

	if(sc->notify){
		sc->notify(sc->ths, (char *)data, len<<1);
	}

	var.type = QTK_SPEECH_DATA_PCM;
	var.v.str.data = (char *)data;
	var.v.str.len = len<<1;
	if(sc->enotify){
		sc->enotify(sc->eths, &var);
	}
}

void qtk_soundscreen_on_cmask_bfse_data(qtk_soundscreen_t *sc, short *data, int len)
{
	qtk_var_t var;

	if(sc->notify){
		sc->notify(sc->ths, (char *)data, len<<1);
	}

	var.type = QTK_SPEECH_DATA_PCM;
	var.v.str.data = (char *)data;
	var.v.str.len = len<<1;
	if(sc->enotify){
		sc->enotify(sc->eths, &var);
	}
}

void qtk_soundscreen_on_aec_data(qtk_soundscreen_t *sc, short **data, int len, int is_end)
{
	if(sc->cfg->use_center){
		wtk_qform9_feed(sc->qform[0], data, len, is_end);
	}else{
		if(sc->cfg->use_qform9){
	#ifdef USE_DESAIXIWEI
			if(sc->cfg->qform9_cfg2){
				short *pv[512];
				pv[0]=data[2];
				pv[1]=data[3];
				pv[2]=data[4];
				pv[3]=data[5];
				wtk_qform9_feed(sc->qform[0], pv, len, is_end);
				wtk_qform9_feed(sc->qform[1], pv, len, is_end);
				wtk_qform9_feed(sc->qform[2], data, len, is_end);
				wtk_qform9_feed(sc->qform[3], data, len, is_end);
			}else{
				int k;
				for(k = 0;k < sc->cfg->n_theta; ++k)
				{
					wtk_qform9_feed(sc->qform[k], data, len, is_end);
				}
			}
	#else
			int k;
			for(k = 0;k < sc->cfg->n_theta; ++k)
			{
				wtk_qform9_feed(sc->qform[k], data, len, is_end);
			}
	#endif
		}
		if(sc->cfg->use_beamnet4){
			int k,i,j,h;
			i=0;
			j=0;
			memset(sc->edata, 0, sizeof(short)*sc->cfg->beamnet4_cfg->channel);
			while(i < len){
				for(h=0;h<sc->cfg->beamnet4_cfg->channel;++h){
					memcpy(sc->edata+j, data[h]+i, sizeof(short));
					j++;
				}
				i++;
			}
			for(k = 0;k < sc->cfg->n_theta; ++k)
			{
				wtk_beamnet4_feed(sc->beamnet4[k], sc->edata, len*sc->cfg->beamnet4_cfg->channel, is_end);
			}
		}
	}
}

void qtk_soundscreen_on_vboxebf_data(qtk_soundscreen_t *sc, short *data, int len)
{

	int i, j ,k;
	short *pv = NULL;
	int bytes;
	pv = data;
	bytes = len /(sc->cfg->vboxebf3_cfg->nmicchannel);
	for(i = 0; i < bytes; ++i){
		for(j = 0; j < sc->cfg->vboxebf3_cfg->nmicchannel; ++j){
			sc->vboxdata[j][i] = pv[i * sc->cfg->vboxebf3_cfg->nmicchannel + j];
		}
	}

	if(sc->cfg->use_center){
		wtk_qform9_feed(sc->qform[0], sc->vboxdata, bytes, 0);
	}else{
		if(sc->cfg->use_qform9){
	#ifdef USE_DESAIXIWEI
			if(sc->cfg->qform9_cfg2){
				short *pv[512];
				pv[0]=sc->vboxdata[2];
				pv[1]=sc->vboxdata[3];
				pv[2]=sc->vboxdata[4];
				pv[3]=sc->vboxdata[5];
				wtk_qform9_feed(sc->qform[0], pv, bytes, 0);
				wtk_qform9_feed(sc->qform[1], pv, bytes, 0);
				wtk_qform9_feed(sc->qform[2], sc->vboxdata, bytes, 0);
				wtk_qform9_feed(sc->qform[3], sc->vboxdata, bytes, 0);
			}else{
				int k;
				for(k = 0;k < sc->cfg->n_theta; ++k)
				{
					wtk_qform9_feed(sc->qform[k], sc->vboxdata, bytes, 0);
				}
			}
	#else
			int k;
			for(k = 0;k < sc->cfg->n_theta; ++k)
			{
				wtk_qform9_feed(sc->qform[k], sc->vboxdata, bytes, 0);
			}
	#endif
		}
		if(sc->cfg->use_beamnet4){
			int k;
			for(k = 0;k < sc->cfg->n_theta; ++k)
			{
				wtk_beamnet4_feed(sc->beamnet4[k], data, len/sc->cfg->beamnet4_cfg->channel, 0);
			}
		}
	}
}

void qtk_soundscreen_on_qform_center(qtk_soundscreen_t *sc, short **data, int len, int is_end)
{
	// wtk_debug("===============>>>>>>>>>>>>len=%d\n",len);
	if(len > 0){
		wtk_strbuf_push(sc->bufs[0], (char *)(data[0]), len<<1);
		wtk_strbuf_push(sc->bufs[1], (char *)(data[1]), len<<1);
		qtk_soundscreen_on_notify(sc, len<<1);
	}
}

void qtk_soundscreen_on_qform_cb0(qtk_soundscreen_t *sc, short *data, int len, int is_end)
{
	// wtk_debug("===============>>>>>>>>>>>>len=%d\n",len);
	// clen+=(len/16.0);
	// wtk_debug("===================>>>>>>>>>>>>slen=%f clen=%f == %f\n",slen,clen,slen-clen);
	wtk_strbuf_push(sc->bufs[0], (char *)data, len << 1);	
	if(sc->cfg->n_theta == 1){
		qtk_soundscreen_on_notify(sc, len<<1);
	}
}

void qtk_soundscreen_on_qform_cb1(qtk_soundscreen_t *sc, short *data, int len, int is_end)
{
	// wtk_debug("===============>>>>>>>>>>>>len=%d\n",len);
	wtk_strbuf_push(sc->bufs[1], (char *)data, len << 1);
	if(sc->cfg->n_theta == 2){
		qtk_soundscreen_on_notify(sc, len<<1);
	}
}

void qtk_soundscreen_on_qform_cb2(qtk_soundscreen_t *sc, short *data, int len, int is_end)
{
	// wtk_debug("===============>>>>>>>>>>>>len=%d\n",len);
	wtk_strbuf_push(sc->bufs[2], (char *)data, len << 1);	
	if(sc->cfg->n_theta == 3){
		qtk_soundscreen_on_notify(sc, len<<1);
	}
}

void qtk_soundscreen_on_qform_cb3(qtk_soundscreen_t *sc, short *data, int len, int is_end)
{
	// wtk_debug("===============>>>>>>>>>>>>len=%d\n",len);
	wtk_strbuf_push(sc->bufs[3], (char *)data, len << 1);	
	if(sc->cfg->n_theta == 4){
		qtk_soundscreen_on_notify(sc, len<<1);
	}
}


void qtk_soundscreen_on_qform_cb4(qtk_soundscreen_t *sc, short *data, int len, int is_end)
{
	// wtk_debug("===============>>>>>>>>>>>>len=%d\n",len);
	wtk_strbuf_push(sc->bufs[4], (char *)data, len << 1);
	if(sc->cfg->n_theta == 5){
		qtk_soundscreen_on_notify(sc, len<<1);
	}
}

void qtk_soundscreen_on_beamnet_cb0(qtk_soundscreen_t *sc, short *data, int len)
{
	// clen+=(len/16.0);
	// wtk_debug("===================>>>>>>>>>>>>slen=%f clen=%f == %f\n",slen,clen,slen-clen);
	wtk_strbuf_push(sc->bufs[0], (char *)data, len << 1);	
	if(sc->cfg->n_theta == 1){
		qtk_soundscreen_on_notify(sc, len<<1);
	}
}

void qtk_soundscreen_on_beamnet_cb1(qtk_soundscreen_t *sc, short *data, int len)
{
	wtk_strbuf_push(sc->bufs[1], (char *)data, len << 1);	
	if(sc->cfg->n_theta == 2){
		qtk_soundscreen_on_notify(sc, len<<1);
	}
}

void qtk_soundscreen_on_beamnet_cb2(qtk_soundscreen_t *sc, short *data, int len)
{
	wtk_strbuf_push(sc->bufs[2], (char *)data, len << 1);	
	if(sc->cfg->n_theta == 3){
		qtk_soundscreen_on_notify(sc, len<<1);
	}
}

void qtk_soundscreen_on_beamnet_cb3(qtk_soundscreen_t *sc, short *data, int len)
{
	wtk_strbuf_push(sc->bufs[3], (char *)data, len << 1);	
	if(sc->cfg->n_theta == 4){
		qtk_soundscreen_on_notify(sc, len<<1);
	}
}


void qtk_soundscreen_on_beamnet_cb4(qtk_soundscreen_t *sc, short *data, int len)
{
	wtk_strbuf_push(sc->bufs[4], (char *)data, len << 1);
	if(sc->cfg->n_theta == 5){
		qtk_soundscreen_on_notify(sc, len<<1);
	}
}
