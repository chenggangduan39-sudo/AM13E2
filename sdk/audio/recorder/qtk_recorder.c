#include "qtk_recorder.h"

void qtk_recorder_set_mic_gain(qtk_recorder_t *rcd);
void qtk_recorder_set_cb_gain(qtk_recorder_t *rcd);

qtk_recorder_t* qtk_recorder_new(qtk_recorder_cfg_t *cfg,qtk_session_t *session,void * notify_ths,qtk_recorder_notify_f notify_func)
{
	qtk_recorder_t *r;

	r=(qtk_recorder_t*)wtk_malloc(sizeof(qtk_recorder_t));
	r->cfg = cfg;
	if(session)
	{
		r->log = session->log;
	}

	r->err_notify_func = NULL;
	r->err_notify_ths = NULL;

	r->sample_rate      = r->cfg->sample_rate;
	r->channel          = r->cfg->channel;
	r->bytes_per_sample = r->cfg->bytes_per_sample;

	if(r->cfg->use_gain_set){
        qtk_recorder_set_mic_gain(r);
        qtk_recorder_set_cb_gain(r);
    }
	qtk_recorder_module_init(&r->rcder_module);
	r->buf = wtk_strbuf_new(r->cfg->buf_size,1);
	memset(r->buf->data,0,r->buf->length);

	return r;
}

void qtk_recorder_delete(qtk_recorder_t *r)
{
	wtk_strbuf_delete(r->buf);
	wtk_free(r);
}

void qtk_recorder_set_err_notify(qtk_recorder_t *r,void *err_notify_ths,qtk_recorder_notify_f err_notify_func)
{
	r->err_notify_func = err_notify_func;
	r->err_notify_ths = err_notify_ths;
}

void qtk_recorder_set_fmt(qtk_recorder_t *r,int sample_rate,int channel,int bytes_per_sample)
{
	r->sample_rate = sample_rate>0 ? sample_rate : r->sample_rate;
	r->channel = channel>0 ? channel : r->channel;
	r->bytes_per_sample = bytes_per_sample>0 ? bytes_per_sample : r->bytes_per_sample;

	wtk_log_log(r->log,"rate/channel/bytes_per_sample %d/%d/%d  %d/%d/%d",
			sample_rate,channel,bytes_per_sample,
			r->sample_rate,r->channel,r->bytes_per_sample
			);
}

wtk_strbuf_t* qtk_recorder_read(qtk_recorder_t *r)
{
	r->buf->pos = r->rcder_module.read_func(r->rcder_module.handler,
			r->rcder_module.ths,
			r->buf->data,
			r->buf->length
			);
	if(r->buf->pos <= 0) {
		++r->read_fail_times;
		if(r->err_notify_func && r->read_fail_times > r->cfg->max_read_fail_times) {
			r->err_notify_func(r->err_notify_ths,1);
		}
		wtk_log_warn(r->log,"read bytes %d",r->buf->pos);
		return r->buf;
	} else {
		r->read_fail_times = 0;
	}

#ifndef LOG_ORI_AUDIO
	if(r->cfg->skip_channels && r->cfg->use_log_ori_audio) {
		char *pv,*pv1;
		int i,j,k,len,s;
		int pos,pos1;
		int b;
        int bytes_per_sample = r->bytes_per_sample;

		pv = pv1 = r->buf->data;
		pos = pos1 = 0;
		len = r->buf->pos / (bytes_per_sample * r->cfg->channel);
		for(i=0;i < len; ++i){
			for(j=0;j < r->cfg->channel; ++j) {
				b = 0;
				for(k=0;k<r->cfg->nskip;++k) {
					if(j == r->cfg->skip_channels[k]) {
						b = 1;
					}
				}
				if(b) {
					pos1+=bytes_per_sample;
				} else {
                    for(s=0;s<bytes_per_sample;++s)
					    pv[pos++] = pv1[pos1++];
				}
			}
		}
		r->buf->pos = pos;
	}
#endif

	return r->buf;
}

void qtk_recorder_set_callback(qtk_recorder_t *r,
		void *handler,
		qtk_recorder_start_func start_func,
		qtk_recorder_stop_func  stop_func,
		qtk_recorder_read_func  read_func,
		qtk_recorder_clean_func clean_func
		)
{
	wtk_log_log(r->log,"set callback handler %p start_func %p stop_func %p read_func %p clean_func %p",
			handler,start_func,stop_func,read_func,clean_func
			);

	qtk_recorder_module_set_callback(&r->rcder_module,
			handler,
			start_func,
			stop_func,
			read_func,
			clean_func
			);
}

int qtk_recorder_start(qtk_recorder_t *r)
{

	int ret;
	r->read_fail_times = 0;
	wtk_log_log(r->log,"rate %d channel %d bytes_per_sample %d buf_time %d",
			r->sample_rate,r->channel,r->bytes_per_sample,r->cfg->buf_time
			);
	wtk_debug("rate %d channel %d bytes_per_sample %d buf_time %d\n",
		r->sample_rate,r->channel,r->bytes_per_sample,r->cfg->buf_time
		);

#if (defined __ANDROID__) || (defined USE_XDW)
	{
		int card;
		int pdev[2];

		if(r->cfg->use_for_bfio) {
			// card = qtk_asound_get_card();
			// if (card >= 0)
			{
				// pdev[0] = card;
				pdev[0] = atoi(r->cfg->snd_name);
				pdev[1] = r->cfg->device_number;
				wtk_log_log(r->log,"card %d device %d",pdev[0],pdev[1]);
				r->rcder_module.ths = r->rcder_module.start_func(r->rcder_module.handler,
						(char*)&pdev,
						r->sample_rate,
						r->channel,
						r->bytes_per_sample,
						r->cfg->buf_time
						);
				ret = r->rcder_module.ths?0:-1;
				goto end;
			}
		} else {
			pdev[0] = atoi(r->cfg->snd_name);
			pdev[1] = r->cfg->device_number;
			wtk_log_log(r->log,"card %d device %d",pdev[0],pdev[1]);
			r->rcder_module.ths = r->rcder_module.start_func(r->rcder_module.handler,
					(char*)&pdev,
					r->sample_rate,
					r->channel,
					r->bytes_per_sample,
					r->cfg->buf_time
					);
			ret = r->rcder_module.ths?0:-1;
			goto end;
		}
	}


#else
	{
		char name[32];
		int card;

		if(r->cfg->use_for_bfio) {
			// card = qtk_asound_get_card();
			card =0;
			if(card >= 0) {
#ifdef MTK8516
				snprintf(name,32,"hw:%d,1",card);
#else
				// snprintf(name,32,"plughw:%d,0",card);
				snprintf(name,32,"hw:%d,0",0);
#endif
				// wtk_log_log(r->log,"card %s",name);
				r->rcder_module.ths = r->rcder_module.start_func(r->rcder_module.handler,
						r->cfg->snd_name,
						r->sample_rate,
						r->channel,
						r->bytes_per_sample,
						r->cfg->buf_time
						);
				ret = r->rcder_module.ths?0:-1;
				goto end;
			}
		} else {
			if(r->cfg->snd_name) {
				wtk_log_log(r->log,"card %s",r->cfg->snd_name);
				r->rcder_module.ths = r->rcder_module.start_func(r->rcder_module.handler,
						r->cfg->snd_name,
						r->sample_rate,
						r->channel,
						r->bytes_per_sample,
						r->cfg->buf_time
						);
				ret = r->rcder_module.ths?0:-1;
				goto end;
			}
		}
	}
#endif
	ret = -1;
end:
	if(ret != 0) {
		if(r->err_notify_func) {
			r->err_notify_func(r->err_notify_ths,1);
		}
	}
	return ret;
}

void qtk_recorder_stop(qtk_recorder_t *r)
{
	if(!r->rcder_module.stop_func) {
		return;
	}

	wtk_log_log0(r->log,"stop");
	r->rcder_module.stop_func(r->rcder_module.handler,r->rcder_module.ths);
	r->rcder_module.ths = 0;
}

void qtk_recorder_clean(qtk_recorder_t *r)
{
	if(r->rcder_module.clean_func)
	{
		r->rcder_module.clean_func(r->rcder_module.handler,r->rcder_module.ths);
	}
}

int qtk_recorder_isErr(qtk_recorder_t *r)
{
	return r->read_fail_times > r->cfg->max_read_fail_times;
}

void qtk_recorder_set_mic_gain(qtk_recorder_t *rcd)
{
    char gain[4];
    char params[256];
    printf("set mic gain: %d\n", rcd->cfg->mic_gain);
    snprintf(gain, 4, "%2x", rcd->cfg->mic_gain);
    snprintf(params, 256, "echo 1,0x30,0x%s%s%s%s > /sys/class/dmic/dmic_reg_debug", gain,gain,gain,gain);
    printf("%s\n", params);
	int ret;
    ret = system(params);
	if(ret < 0)
	{
		return;
	}
    snprintf(params, 256, "echo 1,0x34,0x%s%s%s%s > /sys/class/dmic/dmic_reg_debug", gain,gain,gain,gain);
    printf("%s\n", params);
    ret = system(params);
	if(ret < 0)
	{
		return;
	}
}

void qtk_recorder_set_cb_gain(qtk_recorder_t *rcd)
{
    printf("set cb gain: %d\n", rcd->cfg->cb_gain);
#ifdef USE_R328
	char params[256];
    snprintf(params, 256, "amixer cset numid=4 %d", rcd->cfg->cb_gain);
    printf("%s\n", params);
    system(params);

    snprintf(params, 256, "amixer cset numid=5 %d", rcd->cfg->cb_gain);
    printf("%s\n", params);
    system(params);
#endif

#ifdef USE_R311
	char params[256];
    snprintf(params, 256, "echo 1A7%02x > /sys/bus/platform/devices/twi0/i2c-0/0-0036/ac107_debug/ac107", rcd->cfg->cb_gain);
    printf("%s\n", params);
    system(params);

    snprintf(params, 256, "echo 1A2%02x > /sys/bus/platform/devices/twi0/i2c-0/0-0036/ac107_debug/ac107", rcd->cfg->cb_gain);
    printf("%s\n", params);
    system(params);
#endif
}