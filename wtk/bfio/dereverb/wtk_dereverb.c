#include "wtk_dereverb.h"

void wtk_dereverb_on_stft(wtk_dereverb_t *dereverb,wtk_stft2_msg_t *msg,int pos,int is_end);
void wtk_dereverb_on_stft2(wtk_dereverb_t *dereverb,wtk_stft2_msg_t *lsty_msg,int pos,int is_end);
void wtk_dereverb_on_admm_fft(wtk_dereverb_t *dereverb,wtk_stft2_msg_t *msg,wtk_stft2_msg_t *err_msg,wtk_stft2_msg_t *lsty_msg,int pos,int is_end);

wtk_dereverb_t* wtk_dereverb_new(wtk_dereverb_cfg_t *cfg)
{
	wtk_dereverb_t *dereverb;

	dereverb=(wtk_dereverb_t*)wtk_malloc(sizeof(wtk_dereverb_t));
	dereverb->cfg=cfg;
	dereverb->notify=NULL;
	dereverb->ths=NULL;

	dereverb->stft=wtk_stft2_new(&(cfg->stft));
	wtk_stft2_set_notify(dereverb->stft,dereverb,(wtk_stft2_notify_f)wtk_dereverb_on_stft);

	dereverb->stft2=NULL;
	wtk_queue_init(&(dereverb->stft2_q));
	if(cfg->use_exlsty)
	{
		dereverb->stft2=wtk_stft2_new(&(cfg->stft));
		wtk_stft2_set_notify(dereverb->stft2,dereverb,(wtk_stft2_notify_f)wtk_dereverb_on_stft2);
	}

    dereverb->pad=(float *)wtk_malloc(sizeof(float)*dereverb->stft->cfg->win);
    dereverb->fout=(float *)wtk_malloc(sizeof(float)*dereverb->stft->cfg->win);

	dereverb->admm=NULL;
	if(cfg->use_admm)
	{
		dereverb->admm=wtk_admm_new2(&(cfg->admm), dereverb->stft);
		wtk_admm_set_notify2(dereverb->admm,dereverb,(wtk_admm_notify_f2)wtk_dereverb_on_admm_fft);
	}

	dereverb->covm=NULL;
	if(!cfg->use_fixtheta)
	{
    	dereverb->covm=wtk_covm_new(&(cfg->covm), dereverb->stft->nbin, cfg->stft.channel);
	}
	dereverb->bf=wtk_bf_new(&(cfg->bf), dereverb->stft->cfg->win);

	wtk_dereverb_reset(dereverb);

	return dereverb;
}

void wtk_dereverb_delete(wtk_dereverb_t *dereverb)
{
	wtk_stft2_delete(dereverb->stft);
	if(dereverb->stft2)
	{
		wtk_stft2_delete(dereverb->stft2);
	}
	if(dereverb->admm)
	{
		wtk_admm_delete2(dereverb->admm);
	}
	if(dereverb->covm)
	{
		wtk_covm_delete(dereverb->covm);
	}
	wtk_bf_delete(dereverb->bf);

    wtk_free(dereverb->fout);
    wtk_free(dereverb->pad);

	wtk_free(dereverb);
}

void wtk_dereverb_start(wtk_dereverb_t *dereverb)
{
    wtk_complex_t **ncov, *a, *a2, *b, *b2;
	int channel=dereverb->stft->cfg->channel;
    int nbin=dereverb->stft->nbin;
    int i, k, j;
	wtk_complex_t **ovec;

	wtk_bf_update_ovec(dereverb->bf,dereverb->cfg->theta,0);
    ovec=wtk_complex_new_p2(nbin,  channel);
	ncov=wtk_complex_new_p2(nbin,  channel*channel);

	wtk_bf_update_ovec4(dereverb->cfg->theta2, 0, channel, nbin, dereverb->bf->cfg->rate, dereverb->cfg->bf.speed, dereverb->cfg->bf.mic_pos, ovec);

	for(k=0; k<nbin; ++k)
	{
		a=ncov[k];
		a2=a;
		b=ovec[k];
		for(i=0; i<channel; ++i, ++b)
		{
			b2=ovec[k]+i;
			a+=i;
			for(j=i; j<channel; ++j, ++a,++b2)
			{
				a->a=b->a*b2->a+b->b*b2->b;
				if(j==i)
				{
					a->a+=1;
					a->b=0;
				}else
				{
					a->b=-b->a*b2->b+b->b*b2->a;
					a2[j*channel+i].a=a->a;
					a2[j*channel+i].b=-a->b;
				}
			}
		}
		wtk_bf_update_ncov(dereverb->bf, ncov, k);
		wtk_bf_update_mvdr_w(dereverb->bf, 0, k);
	}

	wtk_complex_delete_p2(ovec,nbin);
	wtk_complex_delete_p2(ncov,nbin);
}

void wtk_dereverb_reset(wtk_dereverb_t *dereverb)
{
	wtk_queue_node_t *qn;
	wtk_stft2_msg_t *msg;

	dereverb->end_pos=0;
	wtk_stft2_reset(dereverb->stft);
	if(dereverb->stft2)
	{
		while(dereverb->stft2_q.length>0)
		{
			qn=wtk_queue_pop(&(dereverb->stft2_q));
			if(!qn){break;}
			msg=(wtk_stft2_msg_t *)data_offset(qn,wtk_stft2_msg_t,q_n);
			wtk_stft2_push_msg(dereverb->stft2,msg);
		}
		wtk_stft2_reset(dereverb->stft2);
	}
	if(dereverb->admm)
	{
		wtk_admm_reset2(dereverb->admm);
	}
	if(dereverb->covm)
	{
		wtk_covm_reset(dereverb->covm);
	}
	wtk_bf_reset(dereverb->bf);

    memset(dereverb->fout, 0, sizeof(float)*dereverb->stft->cfg->win);
    memset(dereverb->pad, 0, sizeof(float)*dereverb->stft->cfg->win);
}

void wtk_dereverb_set_notify(wtk_dereverb_t *dereverb,void *ths,wtk_dereverb_notify_f notify)
{
	dereverb->ths=ths;
	dereverb->notify=notify;
}

void wtk_dereverb_notify_data(wtk_dereverb_t *dereverb,float *data,int len,int is_end)
{
    short *pv=(short *)data;
    int i;
  
	for(i=0; i<len; ++i)
	{
		if(fabs(data[i])<1.0)
		{
			pv[i]=data[i]*32767;
		}else
		{
			if(data[i]>0)
			{
				pv[i]=32767;
			}else
			{
				pv[i]=-32767;
			}
		}
	}
    dereverb->notify(dereverb->ths,pv,len,is_end);
}

void wtk_dereverb_on_admm_fft(wtk_dereverb_t *dereverb,wtk_stft2_msg_t *msg,wtk_stft2_msg_t *err_msg,wtk_stft2_msg_t *lsty_msg,int pos,int is_end)
{
	int i,k,b;
	int nbin=dereverb->stft->nbin;
	int channel=dereverb->bf->channel;
	wtk_covm_t *covm=dereverb->covm;
	wtk_complex_t *bf_out;
	int len;
	wtk_queue_node_t *qn;
	wtk_stft2_msg_t *lsty_msg2;
	float fft_scale=dereverb->cfg->fft_scale;

#ifdef DEBUG_WAV
	int k;
	short *pv;
	int i;
	static wtk_wavfile_t *mic_log=NULL;
	wtk_complex_t in[1024];

	if(msg)
	{
		for(i=0;i<dereverb->stft->nbin;++i)
		{
			in[i]=msg->fft[i][0];
		}
		k=wtk_stft2_output_ifft(dereverb->admm->stft,in,dereverb->fout, dereverb->pad, dereverb->end_pos, is_end);
		pv=(short *)(dereverb->fout);
		for(i=0;i<k;++i)
		{
			pv[i]=32700*dereverb->fout[i];
		}
		if(!mic_log)
		{
			mic_log=wtk_wavfile_new(16000);
			wtk_wavfile_open2(mic_log,"admm_out");
		}
		if(k>0)
		{
			wtk_wavfile_write(mic_log,(char *)pv,k<<1);
		}
	}
	if(is_end && mic_log)
	{
		wtk_wavfile_close(mic_log);
		wtk_wavfile_delete(mic_log);
		mic_log=NULL;
	}

#endif
	if(!msg){return;}

	if(dereverb->stft2)
	{
		qn=wtk_queue_pop(&(dereverb->stft2_q));
		if(!qn){return;}
		lsty_msg2=(wtk_stft2_msg_t *)data_offset(qn,wtk_stft2_msg_t,q_n);

		if(dereverb->cfg->use_fixtheta)
		{
			bf_out=wtk_bf_output_fft2_msg(dereverb->bf, err_msg);
			len=wtk_stft2_output_ifft(dereverb->stft, bf_out, dereverb->fout, dereverb->pad, dereverb->end_pos, is_end);
			if(dereverb->notify)
			{
				wtk_dereverb_notify_data(dereverb, dereverb->fout,len,is_end);
			}
		}else
		{
			for(k=1; k<nbin-1; ++k)
			{
				for(i=0; i<channel; ++i)
				{
					lsty_msg->fft[k][i].a=(lsty_msg->fft[k][i].a+lsty_msg2->fft[k][i].a)*fft_scale;
					lsty_msg->fft[k][i].b=(lsty_msg->fft[k][i].b+lsty_msg2->fft[k][i].b)*fft_scale;

					err_msg->fft[k][i].a*=fft_scale;
					err_msg->fft[k][i].b*=fft_scale;

					msg->fft[k][i].a*=fft_scale;
					msg->fft[k][i].b*=fft_scale;
				}
				b=0;
				b=wtk_covm_feed_fft2(covm, lsty_msg->fft, k, 1);
				if(b==1)
				{
					wtk_bf_update_ncov(dereverb->bf, covm->ncov, k);
				}
				if(covm->scov)
				{
					b=wtk_covm_feed_fft2(covm, err_msg->fft, k, 0);
					if(b==1)
					{
						wtk_bf_update_scov(dereverb->bf, covm->scov, k);
					}
				}
				if(b==1)
				{
					wtk_bf_update_w(dereverb->bf, k);
				}
			}
			bf_out=wtk_bf_output_fft2_msg(dereverb->bf, msg);
			len=wtk_stft2_output_ifft(dereverb->stft, bf_out, dereverb->fout, dereverb->pad, dereverb->end_pos, is_end);
			if(dereverb->notify)
			{
				wtk_dereverb_notify_data(dereverb, dereverb->fout,len,is_end);
			}
		}
		wtk_stft2_push_msg(dereverb->stft, msg);
		wtk_stft2_push_msg(dereverb->stft, err_msg);
		wtk_stft2_push_msg(dereverb->stft, lsty_msg);
		wtk_stft2_push_msg(dereverb->stft2, lsty_msg2);
	}else
	{
		if(dereverb->cfg->use_fixtheta)
		{
			bf_out=wtk_bf_output_fft2_msg(dereverb->bf, err_msg);
			len=wtk_stft2_output_ifft(dereverb->stft, bf_out, dereverb->fout, dereverb->pad, dereverb->end_pos, is_end);
			if(dereverb->notify)
			{
				wtk_dereverb_notify_data(dereverb, dereverb->fout,len,is_end);
			}
		}else
		{
			for(k=1; k<nbin-1; ++k)
			{
				for(i=0; i<channel; ++i)
				{
					lsty_msg->fft[k][i].a*=fft_scale;
					lsty_msg->fft[k][i].b*=fft_scale;

					err_msg->fft[k][i].a*=fft_scale;
					err_msg->fft[k][i].b*=fft_scale;

					msg->fft[k][i].a*=fft_scale;
					msg->fft[k][i].b*=fft_scale;
				}


				b=0;
				b=wtk_covm_feed_fft2(covm, lsty_msg->fft, k, 1);
				if(b==1)
				{
					wtk_bf_update_ncov(dereverb->bf, covm->ncov, k);
				}
				if(covm->scov)
				{
					b=wtk_covm_feed_fft2(covm, err_msg->fft, k, 0);
					if(b==1)
					{
						wtk_bf_update_scov(dereverb->bf, covm->scov, k);
					}
				}
				if(b==1)
				{
					wtk_bf_update_w(dereverb->bf, k);
				}
			}
			bf_out=wtk_bf_output_fft2_msg(dereverb->bf, err_msg);
			len=wtk_stft2_output_ifft(dereverb->stft, bf_out, dereverb->fout, dereverb->pad, dereverb->end_pos, is_end);
			if(dereverb->notify)
			{
				wtk_dereverb_notify_data(dereverb, dereverb->fout,len,is_end);
			}
		}
		wtk_stft2_push_msg(dereverb->stft, msg);
		wtk_stft2_push_msg(dereverb->stft, err_msg);
		wtk_stft2_push_msg(dereverb->stft, lsty_msg);
	}
}

void wtk_dereverb_on_stft(wtk_dereverb_t *dereverb,wtk_stft2_msg_t *msg,int pos,int is_end)
{
	wtk_complex_t *bf_out;
	int len;
	int i,k,b;
	int nbin=dereverb->stft->nbin;
	int channel=dereverb->bf->channel;
	wtk_covm_t *covm=dereverb->covm;
	wtk_queue_node_t *qn;
	wtk_stft2_msg_t *lsty_msg;
	float fft_scale=dereverb->cfg->fft_scale;

	if(is_end)
	{
		dereverb->end_pos=pos;
	}
	if(dereverb->admm)
	{
		wtk_admm_feed_stftmsg(dereverb->admm, msg, pos, is_end);
	}else
	{
		if(dereverb->stft2)
		{
			qn=wtk_queue_pop(&(dereverb->stft2_q));
			if(!qn){return;}
			lsty_msg=(wtk_stft2_msg_t *)data_offset(qn,wtk_stft2_msg_t,q_n);

			if(dereverb->cfg->use_fixtheta)
			{
				bf_out=wtk_bf_output_fft2_msg(dereverb->bf, msg);
				len=wtk_stft2_output_ifft(dereverb->stft, bf_out, dereverb->fout, dereverb->pad, dereverb->end_pos, is_end);
				if(dereverb->notify)
				{
					wtk_dereverb_notify_data(dereverb, dereverb->fout,len,is_end);
				}
			}else
			{
				for(k=1; k<nbin-1; ++k)
				{
					for(i=0; i<channel; ++i)
					{
						lsty_msg->fft[k][i].a*=fft_scale;
						lsty_msg->fft[k][i].b*=fft_scale;

						msg->fft[k][i].a*=fft_scale;
						msg->fft[k][i].b*=fft_scale;
					}
					b=0;
					b=wtk_covm_feed_fft2(covm, lsty_msg->fft, k, 1);
					if(b==1)
					{
						wtk_bf_update_ncov(dereverb->bf, covm->ncov, k);
					}
					if(covm->scov)
					{
						b=wtk_covm_feed_fft2(covm, msg->fft, k, 0);
						if(b==1)
						{
							wtk_bf_update_scov(dereverb->bf, covm->scov, k);
						}
					}
					if(b==1)
					{
						wtk_bf_update_w(dereverb->bf, k);
					}
				}
				bf_out=wtk_bf_output_fft2_msg(dereverb->bf, msg);
				len=wtk_stft2_output_ifft(dereverb->stft, bf_out, dereverb->fout, dereverb->pad, dereverb->end_pos, is_end);
				if(dereverb->notify)
				{
					wtk_dereverb_notify_data(dereverb, dereverb->fout,len,is_end);
				}
			}
			wtk_stft2_push_msg(dereverb->stft, msg);
			wtk_stft2_push_msg(dereverb->stft2, lsty_msg);
		}else
		{
			bf_out=wtk_bf_output_fft2_msg(dereverb->bf, msg);
			len=wtk_stft2_output_ifft(dereverb->stft, bf_out, dereverb->fout, dereverb->pad, dereverb->end_pos, is_end);
			if(dereverb->notify)
			{
				wtk_dereverb_notify_data(dereverb, dereverb->fout,len,is_end);
			}
			wtk_stft2_push_msg(dereverb->stft, msg);
		}
	}	
}

void wtk_dereverb_on_stft2(wtk_dereverb_t *dereverb,wtk_stft2_msg_t *lsty_msg,int pos,int is_end)
{
	if(lsty_msg)
	{
		wtk_queue_push(&(dereverb->stft2_q), &(lsty_msg->q_n));
	}
}

void wtk_dereverb_feed(wtk_dereverb_t *dereverb,short **data,int len,int is_end)
{
#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,dereverb->cfg->mimo.stft.channel);
		wtk_wavfile_open2(mic_log,"dereverb");
	}
	if(len>0)
	{
		wtk_wavfile_write_mc(mic_log,data,len);
	}
	if(is_end && mic_log)
	{
		wtk_wavfile_close(mic_log);
		wtk_wavfile_delete(mic_log);
		mic_log=NULL;
	}
#endif
	wtk_stft2_feed(dereverb->stft,data,len,is_end);
}

void wtk_dereverb_feed2(wtk_dereverb_t *dereverb,short **data,short **lsty, int len,int is_end)
{
	if(dereverb->stft2)
	{
		wtk_stft2_feed(dereverb->stft2,lsty,len,is_end);
	}
	wtk_stft2_feed(dereverb->stft,data,len,is_end);
}