#include "qtk_evboxebf.h" 
#include "sdk/engine/comm/qtk_engine_hdr.h"
#include "sdk/codec/qtk_audio_conversion.h"

void qtk_evboxebf_on_reample(qtk_evboxebf_t *e, char *data, int len);
void qtk_evboxebf_on_data(qtk_evboxebf_t *e,qtk_var_t *var);
int qtk_evboxebf_on_start(qtk_evboxebf_t *e);
int qtk_evboxebf_on_feed(qtk_evboxebf_t *e,char *data,int len);
int qtk_evboxebf_on_end(qtk_evboxebf_t *e);
void qtk_evboxebf_on_reset(qtk_evboxebf_t *e);
void qtk_evboxebf_on_set_notify(qtk_evboxebf_t *e,void *notify_ths,qtk_engine_notify_f notify_f);
int qtk_evboxebf_on_set(qtk_evboxebf_t *e,char *data,int bytes);

void qtk_evboxebf_init(qtk_evboxebf_t *e)
{
	qtk_engine_param_init(&e->param);

	e->session = NULL;

	e->cfg = NULL;

	e->notify     = NULL;
	e->notify_ths = NULL;

	e->thread   = NULL;
	e->callback = NULL;

}

qtk_evboxebf_t* qtk_evboxebf_new(qtk_session_t *session,wtk_local_cfg_t *params)
{
	qtk_evboxebf_t *e;
	int buf_size = 0;
	int ret;

	e=(qtk_evboxebf_t*)wtk_calloc(1, sizeof(qtk_evboxebf_t));
	qtk_evboxebf_init(e);
	e->session = session;

	qtk_engine_param_set_session(&e->param, e->session);
	ret = qtk_engine_param_feed(&e->param, params);
	if(ret != 0) {
		wtk_log_warn0(e->session->log,"params als failed.");
		goto end;
	}

	if(e->param.use_bin) {
		e->cfg = qtk_vboxebf_cfg_new_bin(e->param.cfg);
	} else {
		e->cfg = qtk_vboxebf_cfg_new(e->param.cfg);
	}
	if(!e->cfg) {
		wtk_log_warn0(e->session->log,"cfg new failed.");
		_qtk_error(e->session,_QTK_CFG_NEW_FAILED);
		ret = -1;
		goto end;
	}
	if(e->param.use_logwav && e->param.log_wav_path->len>0){
		
        e->wav=wtk_wavfile_new(16000);
        e->wav->max_pend=0;
        ret=wtk_wavfile_open(e->wav,e->param.log_wav_path->data);
		if(ret==-1){goto end;}
	}
	
	if(e->param.use_cfg == 0)
	{
		e->cfg->energy_sum = e->param.energy_sum;
		if(e->cfg->use_vboxebf3)
		{
			if(e->param.use_maskssl != -1)
			{
				e->cfg->vebf3_cfg->use_maskssl = e->param.use_maskssl;
				wtk_debug("maskssl=%d\n",e->cfg->vebf3_cfg->use_maskssl);
			}
			if(e->param.use_maskssl2 != -1)
			{
				e->cfg->vebf3_cfg->use_maskssl2 = e->param.use_maskssl2;
				wtk_debug("maskssl2=%d\n",e->cfg->vebf3_cfg->use_maskssl2);
			}

			if(e->cfg->vebf3_cfg->use_maskssl)
			{
				if(e->param.online_tms != -1)
				{
					e->cfg->vebf3_cfg->maskssl.online_tms = e->param.online_tms;
					e->cfg->vebf3_cfg->maskssl.online_frame = floor(e->param.online_tms*1.0/1000*e->cfg->vebf3_cfg->rate/(e->cfg->vebf3_cfg->wins/2));
				}
				if(e->param.specsum_fs != -1)
				{
					e->cfg->vebf3_cfg->maskssl.specsum_ns=floor(e->param.specsum_fs/(e->cfg->vebf3_cfg->maskssl.rate*1.0/e->cfg->vebf3_cfg->maskssl.wins));
					e->cfg->vebf3_cfg->maskssl.specsum_ns=max(1, e->cfg->vebf3_cfg->maskssl.specsum_ns);
				}
				if(e->param.specsum_fe != -1)
				{
					e->cfg->vebf3_cfg->maskssl.specsum_ne=floor(e->param.specsum_fe/(e->cfg->vebf3_cfg->maskssl.rate*1.0/e->cfg->vebf3_cfg->maskssl.wins));
					e->cfg->vebf3_cfg->maskssl.specsum_ne=min(e->cfg->vebf3_cfg->maskssl.wins/2-1, e->cfg->vebf3_cfg->maskssl.specsum_ne);
				}
				
				if(e->param.theta_step != -1)
				{
					e->cfg->vebf3_cfg->maskssl.theta_step = e->param.theta_step;
				}
			}
			if(e->cfg->vebf3_cfg->use_maskssl2)
			{
				if(e->param.online_tms != -1)
				{
					e->cfg->vebf3_cfg->maskssl2.online_frame = floor(e->param.online_tms*1.0/1000*e->cfg->vebf3_cfg->rate/(e->cfg->vebf3_cfg->wins/2));
				}
				if(e->param.specsum_fs != -1)
				{
					e->cfg->vebf3_cfg->maskssl2.specsum_ns=floor(e->param.specsum_fs/(e->cfg->vebf3_cfg->maskssl2.rate*1.0/e->cfg->vebf3_cfg->maskssl2.wins));
					e->cfg->vebf3_cfg->maskssl2.specsum_ns=max(1, e->cfg->vebf3_cfg->maskssl2.specsum_ns);
				}
				if(e->param.specsum_fe != -1)
				{
					e->cfg->vebf3_cfg->maskssl2.specsum_ne=floor(e->param.specsum_fe/(e->cfg->vebf3_cfg->maskssl2.rate*1.0/e->cfg->vebf3_cfg->maskssl2.wins));
					e->cfg->vebf3_cfg->maskssl2.specsum_ne=min(e->cfg->vebf3_cfg->maskssl2.wins/2-1, e->cfg->vebf3_cfg->maskssl2.specsum_ne);
				}
				
				if(e->param.theta_step != -1)
				{
					e->cfg->vebf3_cfg->maskssl2.theta_step = e->param.theta_step;
				}
				if(e->param.online_frame_step != -1)
				{
					e->cfg->vebf3_cfg->maskssl2.online_frame_step = e->param.online_frame_step;
				}
			}
			if(e->cfg->vebf3_cfg->use_qmmse)
			{
				if(e->param.noise_suppress != 100000)
				{
					e->cfg->vebf3_cfg->qmmse.noise_suppress=e->param.noise_suppress;
				}
				if(e->param.echo_suppress != 100000)
				{
					e->cfg->vebf3_cfg->qmmse.echo_suppress = e->param.echo_suppress;
				}
				if(e->param.echo_suppress_active != 100000)
				{
					e->cfg->vebf3_cfg->qmmse.echo_suppress_active = e->param.echo_suppress_active;
				}
			}
			if(e->param.gbias != -1)
			{
				e->cfg->vebf3_cfg->gbias = e->param.gbias;
			}
			if(e->param.agca != -1)
			{
				e->cfg->vebf3_cfg->agc_a = e->param.agca;
			}
			if(e->param.use_fftsbf != -1)
			{
				e->cfg->vebf3_cfg->use_fftsbf = e->param.use_fftsbf;
			}
			if(e->param.bfmu != -1)
			{
				e->cfg->vebf3_cfg->bfmu = e->param.bfmu;
			}
			if(e->param.echo_bfmu != -1)
			{
				e->cfg->vebf3_cfg->echo_bfmu = e->param.echo_bfmu;
			}
			if(e->param.spenr_thresh != -100000)
			{
				e->cfg->vebf3_cfg->spenr_thresh = e->param.spenr_thresh;
			}
			if(e->param.use_cnon != -1)
			{
				e->cfg->vebf3_cfg->use_cnon=e->param.use_cnon;
			}
			if(e->param.sym != -100000)
			{
				e->cfg->vebf3_cfg->sym=e->param.sym;
			}
			if(e->param.use_erlssingle != -1)
			{
				e->cfg->vebf3_cfg->use_erlssingle = e->param.use_erlssingle;
			}
		}else if(e->cfg->use_vboxebf4)
		{
			if(e->param.use_ssl != -1)
			{
				e->cfg->vebf4_cfg->use_ssl = e->param.use_ssl;
				wtk_debug("ssl=%d\n",e->cfg->vebf4_cfg->use_ssl);
			}
			if(e->param.use_maskssl != -1)
			{
				e->cfg->vebf4_cfg->use_maskssl = e->param.use_maskssl;
				wtk_debug("maskssl=%d\n",e->cfg->vebf4_cfg->use_maskssl);
			}
			if(e->param.use_maskssl2 != -1)
			{
				e->cfg->vebf4_cfg->use_maskssl2 = e->param.use_maskssl2;
				wtk_debug("maskssl2=%d\n",e->cfg->vebf4_cfg->use_maskssl2);
			}
			
			if(e->cfg->vebf4_cfg->use_ssl)
			{
				if(e->param.online_tms != -1)
				{
					e->cfg->vebf4_cfg->ssl2.online_tms = e->param.online_tms;
					e->cfg->vebf4_cfg->ssl2.online_frame = floor(e->param.online_tms*1.0/1000*e->cfg->vebf4_cfg->rate/(e->cfg->vebf4_cfg->wins/2));
				}

				if(e->param.specsum_fs != -1)
				{
					e->cfg->vebf4_cfg->ssl2.specsum_ns=floor(e->param.specsum_fs/(e->cfg->vebf4_cfg->ssl2.rate*1.0/e->cfg->vebf4_cfg->ssl2.wins));
					e->cfg->vebf4_cfg->ssl2.specsum_ns=max(1, e->cfg->vebf4_cfg->ssl2.specsum_ns);
				}
				if(e->param.specsum_fe != -1)
				{
					e->cfg->vebf4_cfg->ssl2.specsum_ne=floor(e->param.specsum_fe/(e->cfg->vebf4_cfg->ssl2.rate*1.0/e->cfg->vebf4_cfg->ssl2.wins));
					e->cfg->vebf4_cfg->ssl2.specsum_ne=min(e->cfg->vebf4_cfg->ssl2.wins/2-1, e->cfg->vebf4_cfg->ssl2.specsum_ne);
				}
				if(e->param.lf != -1)
				{
					e->cfg->vebf4_cfg->ssl2.lf = e->param.lf;
				}
				if(e->param.theta_step != -1)
				{
					e->cfg->vebf4_cfg->ssl2.theta_step = e->param.theta_step;
				}
			}

			if(e->cfg->vebf4_cfg->use_maskssl)
			{
				if(e->param.online_tms != -1)
				{
					e->cfg->vebf4_cfg->maskssl.online_tms = e->param.online_tms;
					e->cfg->vebf4_cfg->maskssl.online_frame = floor(e->param.online_tms*1.0/1000*e->cfg->vebf4_cfg->rate/(e->cfg->vebf4_cfg->wins/2));
				}
				if(e->param.specsum_fs != -1)
				{
					e->cfg->vebf4_cfg->maskssl.specsum_ns=floor(e->param.specsum_fs/(e->cfg->vebf4_cfg->maskssl.rate*1.0/e->cfg->vebf4_cfg->maskssl.wins));
					e->cfg->vebf4_cfg->maskssl.specsum_ns=max(1, e->cfg->vebf4_cfg->maskssl.specsum_ns);
				}
				if(e->param.specsum_fe != -1)
				{
					e->cfg->vebf4_cfg->maskssl.specsum_ne=floor(e->param.specsum_fe/(e->cfg->vebf4_cfg->maskssl.rate*1.0/e->cfg->vebf4_cfg->maskssl.wins));
					e->cfg->vebf4_cfg->maskssl.specsum_ne=min(e->cfg->vebf4_cfg->maskssl.wins/2-1, e->cfg->vebf4_cfg->maskssl.specsum_ne);
				}
				
				if(e->param.theta_step != -1)
				{
					e->cfg->vebf4_cfg->maskssl.theta_step = e->param.theta_step;
				}
			}
			if(e->cfg->vebf4_cfg->use_maskssl2)
			{
				if(e->param.online_tms != -1)
				{
					e->cfg->vebf4_cfg->maskssl2.online_frame = floor(e->param.online_tms*1.0/1000*e->cfg->vebf4_cfg->rate/(e->cfg->vebf4_cfg->wins/2));
				}
				if(e->param.specsum_fs != -1)
				{
					e->cfg->vebf4_cfg->maskssl2.specsum_ns=floor(e->param.specsum_fs/(e->cfg->vebf4_cfg->maskssl2.rate*1.0/e->cfg->vebf4_cfg->maskssl2.wins));
					e->cfg->vebf4_cfg->maskssl2.specsum_ns=max(1, e->cfg->vebf4_cfg->maskssl2.specsum_ns);
				}
				if(e->param.specsum_fe != -1)
				{
					e->cfg->vebf4_cfg->maskssl2.specsum_ne=floor(e->param.specsum_fe/(e->cfg->vebf4_cfg->maskssl2.rate*1.0/e->cfg->vebf4_cfg->maskssl2.wins));
					e->cfg->vebf4_cfg->maskssl2.specsum_ne=min(e->cfg->vebf4_cfg->maskssl2.wins/2-1, e->cfg->vebf4_cfg->maskssl2.specsum_ne);
				}
				
				if(e->param.theta_step != -1)
				{
					e->cfg->vebf4_cfg->maskssl2.theta_step = e->param.theta_step;
				}
				if(e->param.online_frame_step != -1)
				{
					e->cfg->vebf4_cfg->maskssl2.online_frame_step = e->param.online_frame_step;
				}
			}
			if(e->cfg->vebf4_cfg->use_qmmse)
			{
				if(e->param.noise_suppress != 100000)
				{
					e->cfg->vebf4_cfg->qmmse.noise_suppress=e->param.noise_suppress;
				}
				if(e->param.echo_suppress != 100000)
				{
					e->cfg->vebf4_cfg->qmmse.echo_suppress = e->param.echo_suppress;
				}
				if(e->param.echo_suppress_active != 100000)
				{
					e->cfg->vebf4_cfg->qmmse.echo_suppress_active = e->param.echo_suppress_active;
				}
			}
			if(e->param.gbias != -1)
			{
				e->cfg->vebf4_cfg->gbias = e->param.gbias;
			}
			if(e->param.agca != -1)
			{
				e->cfg->vebf4_cfg->agc_a = e->param.agca;
			}
			if(e->param.use_fftsbf != -1)
			{
				e->cfg->vebf4_cfg->use_fftsbf = e->param.use_fftsbf;
			}
			if(e->param.bfmu != -1)
			{
				e->cfg->vebf4_cfg->bfmu = e->param.bfmu;
			}
			if(e->param.echo_bfmu != -1)
			{
				e->cfg->vebf4_cfg->echo_bfmu = e->param.echo_bfmu;
			}
			if(e->param.spenr_thresh != -100000)
			{
				e->cfg->vebf4_cfg->spenr_thresh = e->param.spenr_thresh;
			}
			if(e->param.use_cnon != -1)
			{
				e->cfg->vebf4_cfg->use_cnon=e->param.use_cnon;
			}
			if(e->param.sym != -100000)
			{
				e->cfg->vebf4_cfg->sym=e->param.sym;
			}
		}
		e->cfg->mic_shift = e->param.mic_shift;
		e->cfg->spk_shift = e->param.spk_shift;
		e->cfg->echo_shift = e->param.echo_shift;
		e->cfg->spk_channel = e->param.spk_channel;
	}

	e->eqform = qtk_vboxebf_new(e->cfg);
	if(!e->eqform){
		wtk_log_err0(e->session->log, "eqform new failed.");
		ret = -1;
		goto end;
	}
	qtk_vboxebf_set_notify2(e->eqform, e, (qtk_engine_notify_f)qtk_evboxebf_on_data);

	if(e->param.use_resample)
	{
		if(e->param.resample_out_rate == -1)
		{
			e->param.resample_out_rate = 48000;
		}

		e->oresample = NULL;
		e->oresample= speex_resampler_init(1 , 16000, e->param.resample_out_rate, SPEEX_RESAMPLER_QUALITY_DESKTOP, NULL);
	}

	e->outlen=16*32*2*6;
	e->outresample=(char *)wtk_malloc(e->outlen);

	e->outbuf = wtk_strbuf_new(10240, 1.0);
	if(e->param.use_thread) {
		buf_size = 640;

		e->callback = qtk_engine_callback_new();
		e->callback->start_f      = (qtk_engine_thread_start_f)      qtk_evboxebf_on_start;
		e->callback->data_f       = (qtk_engine_thread_data_f)       qtk_evboxebf_on_feed;
		e->callback->end_f        = (qtk_engine_thread_end_f)        qtk_evboxebf_on_end;
		e->callback->reset_f      = (qtk_engine_thread_reset_f)      qtk_evboxebf_on_reset;
		e->callback->set_notify_f = (qtk_engine_thread_set_notify_f) qtk_evboxebf_on_set_notify;
		e->callback->set_f        = (qtk_engine_thread_set_f)        qtk_evboxebf_on_set;
		e->callback->ths          = e;

		e->thread = qtk_engine_thread_new(
				e->callback,
				e->session->log,
				"eeqform",
				buf_size,
				20,
				0,
				e->param.syn
				);
	}

	ret = 0;
end:
	wtk_log_log(e->session->log,"ret = %d",ret);
	if(ret != 0) {
		qtk_evboxebf_delete(e);
		e = NULL;
	}
	return e;
}

int qtk_evboxebf_delete(qtk_evboxebf_t *e)
{
	if(e->thread) {
		qtk_engine_thread_delete(e->thread,1);
	}
	if(e->callback) {
		qtk_engine_callback_delete(e->callback);
	}
	if(e->eqform)
	{
		qtk_vboxebf_delete(e->eqform);
		e->eqform = NULL;
	}
	if(e->cfg)
	{
		if(e->param.use_bin)
		{
			qtk_vboxebf_cfg_delete_bin(e->cfg);
		}else{
			qtk_vboxebf_cfg_delete(e->cfg);
		}
		e->cfg = NULL;
	}
	if(e->param.use_resample && e->oresample)
	{
		speex_resampler_destroy(e->oresample);
	}
	if(e->outresample)
	{
		wtk_free(e->outresample);
	}
	if(e->outbuf)
	{
		wtk_strbuf_delete(e->outbuf);
	}
	qtk_engine_param_clean(&e->param);

	wtk_free(e);
	return 0;
}


int qtk_evboxebf_on_start(qtk_evboxebf_t *e)
{
	wtk_strbuf_reset(e->outbuf);
	return qtk_vboxebf_start(e->eqform);
}

int qtk_evboxebf_on_feed(qtk_evboxebf_t *e,char *data,int len)
{
	// double tm = time_get_ms();
	int ret;
	// wtk_debug("==================>>>>>>>>>>>>>feed start len=%d\n",len);
	ret = qtk_vboxebf_feed(e->eqform, data, len, 0);
	// wtk_debug("=============>>>>>>>>>>feed end time=%f\n",time_get_ms() - tm);
	return ret;
}

int qtk_evboxebf_on_end(qtk_evboxebf_t *e)
{
	return qtk_vboxebf_feed(e->eqform, NULL, 0, 1);
}

void qtk_evboxebf_on_reset(qtk_evboxebf_t *e)
{
	qtk_vboxebf_reset(e->eqform);
}

void qtk_evboxebf_on_set_notify(qtk_evboxebf_t *e,void *notify_ths,qtk_engine_notify_f notify_f)
{
	e->notify_ths = notify_ths;
	e->notify     = notify_f;
}

int qtk_evboxebf_on_set(qtk_evboxebf_t *e,char *data,int bytes)
{
	wtk_cfg_file_t *cfile = NULL;
	wtk_cfg_item_t *item;
	wtk_queue_node_t *qn;
	int ret;

	wtk_log_log(e->session->log,"set param = %.*s\n",bytes,data);
	cfile = wtk_cfg_file_new();
	if(!cfile) {
		return -1;
	}

	ret = wtk_cfg_file_feed(cfile,data,bytes);
	if(ret != 0) {
		goto end;
	}

	for(qn=cfile->main->cfg->queue.pop;qn;qn=qn->next) {
		item = data_offset2(qn,wtk_cfg_item_t,n);
		if(wtk_string_cmp2(item->key,&qtk_engine_set_str[16]) == 0) {
			qtk_vboxebf_set_agcenable(e->eqform, atoi(item->value.str->data));
		} else if (wtk_string_cmp2(item->key,&qtk_engine_set_str[17]) == 0) {
			qtk_vboxebf_set_echoenable(e->eqform, atoi(item->value.str->data));
		} else if (wtk_string_cmp2(item->key,&qtk_engine_set_str[18]) == 0) {
			qtk_vboxebf_set_denoiseenable(e->eqform, atoi(item->value.str->data));
		} else if (wtk_string_cmp2(item->key,&qtk_engine_set_str[32]) == 0) {
			qtk_vboxebf_set_ssl_enable(e->eqform, atoi(item->value.str->data));
		} else if (wtk_string_cmp2(item->key,&qtk_engine_set_str[33]) == 0) {
			e->eqform->cfg->noise_suppress = atof(item->value.str->data);
			qtk_vboxebf_cfg_set_agcanc(e->eqform->cfg);
		} else if (wtk_string_cmp2(item->key,&qtk_engine_set_str[34]) == 0) {
			e->eqform->cfg->agc_level = atof(item->value.str->data);
			qtk_vboxebf_cfg_set_agcanc(e->eqform->cfg);
		}
	}
	
end:
	if(cfile) {
		wtk_cfg_file_delete(cfile);
	}
	return 0;
}


int qtk_evboxebf_start(qtk_evboxebf_t *e)
{
	int ret;

	if(e->param.use_thread){
		qtk_engine_thread_start(e->thread);
	}else{
		qtk_evboxebf_on_start(e);
	}
	ret = 0;
	return ret;
}

int qtk_evboxebf_feed(qtk_evboxebf_t *e,char *data,int bytes,int is_end)
{
	if(e->param.use_thread)
	{
		qtk_engine_thread_feed(e->thread,data,bytes,is_end);
	}else{
		if(bytes > 0) {
			qtk_evboxebf_on_feed(e,data,bytes);
		}
		if(is_end) {
			qtk_evboxebf_on_end(e);
		}
	}
	return 0;
}

int qtk_evboxebf_feed2(qtk_evboxebf_t *e, char *input, int in_bytes, char *output, int *out_bytes, int is_end)
{
	qtk_vboxebf_feed2(e->eqform, input, in_bytes, output, out_bytes, is_end);
	return 0;
}

int qtk_evboxebf_reset(qtk_evboxebf_t *e)
{
	if(e->param.use_thread){
		qtk_engine_thread_reset(e->thread);
	}else{
		qtk_evboxebf_on_reset(e);
	}

	return 0;
}

int qtk_evboxebf_cancel(qtk_evboxebf_t *e)
{
	if(e->param.use_thread) {
		qtk_engine_thread_cancel(e->thread);
	}
	return 0;
}

void qtk_evboxebf_set_notify(qtk_evboxebf_t *e,void *ths,qtk_engine_notify_f notify_f)
{
	if(e->param.use_thread) {
		qtk_engine_thread_set_notify(e->thread,ths,notify_f);
	} else {
		qtk_evboxebf_on_set_notify(e,ths,notify_f);
	}
}

int qtk_evboxebf_set(qtk_evboxebf_t *e,char *data,int bytes)
{
	int ret = 0;

	if(e->param.use_thread) {
		qtk_engine_thread_set(e->thread,data,bytes);
	} else {
		ret = qtk_evboxebf_on_set(e,data,bytes);
	}
	return ret;
}


void qtk_evboxebf_on_data(qtk_evboxebf_t *e,qtk_var_t *var)
{
	int i;

	if(e->param.use_resample && var->type == QTK_SPEECH_DATA_PCM)
	{
		qtk_var_t var2;
		// wtk_debug("=============>>>>>>>>>>.len=%d\n",var->v.str.len);

		memset(e->outresample, 0, var->v.str.len*3);
		int inlen;
		int outlen;
		inlen=(var->v.str.len >> 1);
		outlen=inlen*6;

		if(e->oresample)
		{
			speex_resampler_process_interleaved_int(e->oresample,
										(spx_int16_t *)(var->v.str.data), (spx_uint32_t *)(&inlen), 
										(spx_int16_t *)(e->outresample), (spx_uint32_t *)(&outlen));
			if(e->param.out_channel  == 2)
			{
				wtk_strbuf_reset(e->outbuf);
				i=0;
				while(i<(outlen<<1))
				{
					wtk_strbuf_push(e->outbuf, e->outresample + i, 2);
					wtk_strbuf_push(e->outbuf, e->outresample + i, 2);
					i+=2;
				}
			}else{
				wtk_strbuf_reset(e->outbuf);
				wtk_strbuf_push(e->outbuf, e->outresample, outlen<<1);
			}
		}

		if(e->notify){
			var2.v.str.data = e->outbuf->data;
			var2.v.str.len = e->outbuf->pos;
			var2.type = QTK_SPEECH_DATA_PCM;
			e->notify(e->notify_ths, &var2);
		}
		if(e->param.use_logwav && e->param.log_wav_path->len>0)
		{
			if(e->outbuf->pos > 0)
			{
				wtk_wavfile_write(e->wav, e->outbuf->data, e->outbuf->pos);       
			}
		}
	}else{
		if(e->param.out_channel  == 2 && var->type == QTK_SPEECH_DATA_PCM)
		{
			wtk_strbuf_reset(e->outbuf);
			i=0;
			while(i<var->v.str.len)
			{
				wtk_strbuf_push(e->outbuf, var->v.str.data + i, 2);
				wtk_strbuf_push(e->outbuf, var->v.str.data + i, 2);
				i+=2;
			}
			var->v.str.data = e->outbuf->data;
			var->v.str.len = e->outbuf->pos;
		}
		if(e->notify){
			e->notify(e->notify_ths, var);
		}
		if(e->param.use_logwav && e->param.log_wav_path->len>0)
		{
			if(var->v.str.len>0 && var->type == QTK_SPEECH_DATA_PCM)
			{
				for(i=0;i<var->v.str.len;++i)
				{
					var->v.str.data[i]*=1;
				}
				wtk_wavfile_write(e->wav,(char *)var->v.str.data,var->v.str.len);       
			}
		}
	}
}

void qtk_evboxebf_on_reample(qtk_evboxebf_t *e, char *data, int len)
{
	qtk_var_t var;
	int i;

	if(e->param.out_channel  == 2)
	{
		wtk_strbuf_reset(e->outbuf);
		i=0;
		while(i<len)
		{
			wtk_strbuf_push(e->outbuf, data + i, 2);
			wtk_strbuf_push(e->outbuf, data + i, 2);
			i+=2;
		}
		var.v.str.data = e->outbuf->data;
		var.v.str.len = e->outbuf->pos;
		
		if(e->notify){
			var.type = QTK_SPEECH_DATA_PCM;
			e->notify(e->notify_ths, &var);
		}
	}else{
		if(e->notify){
			var.type = QTK_SPEECH_DATA_PCM;
			var.v.str.data = data;
			var.v.str.len = len;
			e->notify(e->notify_ths, &var);
		}
	}

	if(e->param.use_logwav && e->param.log_wav_path->len>0)
	{
        if(len>0)
    	{
         	for(i=0;i<len;++i)
         	{
            	data[i]*=1;
         	}
        	wtk_wavfile_write(e->wav, data, len);       
      	}
	}
}
