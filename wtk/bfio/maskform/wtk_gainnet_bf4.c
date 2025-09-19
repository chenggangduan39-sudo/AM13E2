#include "wtk_gainnet_bf4.h"
#define gainnet_bf4_tobank(n)   (13.1f*atan(.00074f*(n))+2.24f*atan((n)*(n)*1.85e-8f)+1e-4f*(n))

void wtk_gainnet_bf4_aec_on_gainnet(wtk_gainnet_bf4_aec_t *gaec, float *gain, int len, int is_end);
void wtk_gainnet_bf4_aec_on_gainnet2(wtk_gainnet_bf4_aec_t *gaec, float *gain, int len, int is_end);
void wtk_gainnet_bf4_on_qenvelope(wtk_gainnet_bf4_t *gainnet_bf4,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end);
void wtk_gainnet_bf4_on_ssl2(wtk_gainnet_bf4_t *gainnet_bf4, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te);

void wtk_gainnet_bf4_xfilterbank_init(int *eband, int bands,int rate,int len)
{
	float df;
	float max_mel, mel_interval;
	int i;
	int id;
	float curr_freq;
	float mel;

	df =rate*1.0/(2*(len-1));
	max_mel = gainnet_bf4_tobank(rate/2);
	mel_interval =max_mel/(bands-1);
	for(i=0; i<bands; ++i)
   {
	   eband[i]=-1;
   }
   for (i=0;i<len;++i)
   {
		curr_freq = i*df;
		// printf("%f\n",curr_freq);
		mel = gainnet_bf4_tobank(curr_freq);
		if (mel > max_mel)
		{
			break;
		}
		id = (int)(floor(mel/mel_interval));
		if(eband[id]==-1)
		{
			eband[id]=i;
		}
				// printf("%d %d %f \n",id,eband[id],curr_freq);
   }
   eband[bands-1]=len-1;
//    for(i=0; i<bands; ++i)
//    {
// 	   printf("%d ",eband[i]);
//    }
//    printf("\n");
}

void wtk_gainnet_bf4_aec_init(wtk_gainnet_bf4_aec_t *gaec, wtk_gainnet_bf4_cfg_t *cfg)
{
	gaec->cfg=cfg;
	gaec->nbin=cfg->wins/2+1;

	gaec->eband=wtk_malloc(sizeof(int)*cfg->nb_bands);
	wtk_gainnet_bf4_xfilterbank_init(gaec->eband, cfg->nb_bands, cfg->rate, gaec->nbin);

	gaec->Ex=wtk_malloc(sizeof(float)*cfg->nb_bands);

	gaec->dct_table=wtk_malloc(sizeof(float)*cfg->nb_bands*cfg->nb_bands);

	gaec->cepstral_mem=gaec->cepstral_mem_sp=NULL;
	if(cfg->use_ceps)
	{
		gaec->cepstral_mem=wtk_float_new_p2(cfg->ceps_mem, cfg->nb_bands);
		gaec->cepstral_mem_sp=wtk_float_new_p2(cfg->ceps_mem, cfg->nb_bands);
	}

	gaec->lastg=wtk_malloc(sizeof(float)*cfg->nb_bands);
	gaec->g=wtk_malloc(sizeof(float)*cfg->nb_bands);
	gaec->gf=wtk_malloc(sizeof(float)*gaec->nbin);
	gaec->g2=wtk_malloc(sizeof(float)*cfg->nb_bands);
	gaec->gf2=wtk_malloc(sizeof(float)*gaec->nbin);


	gaec->Ly=wtk_malloc(sizeof(float)*cfg->nb_bands);
	gaec->features=wtk_malloc(sizeof(float)*cfg->nb_features);

	gaec->gainnet=wtk_gainnet5_new(cfg->gainnet);
	wtk_gainnet5_set_notify(gaec->gainnet, gaec, (wtk_gainnet5_notify_f)wtk_gainnet_bf4_aec_on_gainnet);
	wtk_gainnet5_set_notify2(gaec->gainnet, gaec, (wtk_gainnet5_notify_f2)wtk_gainnet_bf4_aec_on_gainnet2);

	gaec->qmmse=NULL;
	if(cfg->use_preqmmse)
	{
		gaec->qmmse=wtk_qmmse_new(&(cfg->preqmmse));
	}
}

void wtk_gainnet_bf4_aec_clean(wtk_gainnet_bf4_aec_t *gaec)
{
	wtk_free(gaec->eband);
	wtk_free(gaec->Ex);

	wtk_free(gaec->dct_table);

	if(gaec->cepstral_mem)
	{
		wtk_float_delete_p2(gaec->cepstral_mem, gaec->cfg->ceps_mem);
		wtk_float_delete_p2(gaec->cepstral_mem_sp, gaec->cfg->ceps_mem);
	}

	wtk_free(gaec->lastg);
	wtk_free(gaec->g);
	wtk_free(gaec->gf);
	wtk_free(gaec->g2);
	wtk_free(gaec->gf2);

	wtk_free(gaec->Ly);
	wtk_free(gaec->features);

	if(gaec->qmmse)
	{
		wtk_qmmse_delete(gaec->qmmse);
	}

	if(gaec->gainnet)
	{
		wtk_gainnet5_delete(gaec->gainnet);
	}
}

void wtk_gainnet_bf4_aec_reset(wtk_gainnet_bf4_aec_t *gaec)
{
	int i,j;
	int nb_bands=gaec->cfg->nb_bands;

	for (i=0;i<nb_bands;++i) 
	{
		for (j=0;j<nb_bands;++j)
		{
			gaec->dct_table[i*nb_bands + j] = cos((i+.5)*j*PI/nb_bands);
			if (j==0)
			{
			gaec->dct_table[i*nb_bands + j] *= sqrt(.5);
			}
		}
	}

	memset(gaec->Ex, 0, sizeof(float)*(gaec->cfg->nb_bands));

	gaec->memid=0;
	gaec->memid_sp=0;
	if(gaec->cepstral_mem)
	{
		wtk_float_zero_p2(gaec->cepstral_mem, gaec->cfg->ceps_mem, gaec->cfg->nb_bands);
		wtk_float_zero_p2(gaec->cepstral_mem_sp, gaec->cfg->ceps_mem, gaec->cfg->nb_bands);
	}

	memset(gaec->lastg, 0, sizeof(float)*gaec->cfg->nb_bands);
	memset(gaec->g, 0, sizeof(float)*gaec->cfg->nb_bands);
	memset(gaec->gf, 0, sizeof(float)*gaec->nbin);
	memset(gaec->g2, 0, sizeof(float)*gaec->cfg->nb_bands);
	memset(gaec->gf2, 0, sizeof(float)*gaec->nbin);
	gaec->gf2_m=0;

	memset(gaec->Ly, 0, sizeof(float)*gaec->cfg->nb_bands);
	memset(gaec->features, 0, sizeof(float)*gaec->cfg->nb_features);

	if(gaec->gainnet)
	{
		wtk_gainnet5_reset(gaec->gainnet);
	}

	if(gaec->qmmse)
	{
		wtk_qmmse_reset(gaec->qmmse);
	}

	gaec->silence=1;
}

wtk_gainnet_bf4_fft_msg_t* wtk_gainnet_bf4_fft_new(wtk_gainnet_bf4_t *gainnet_bf4)
{
	wtk_gainnet_bf4_fft_msg_t *msg;
    int nbin=gainnet_bf4->nbin;
    int nmicchannel=gainnet_bf4->cfg->nmicchannel;

	msg=(wtk_gainnet_bf4_fft_msg_t*)wtk_malloc(sizeof(wtk_gainnet_bf4_fft_msg_t));
	msg->fft=wtk_complex_new_p2(nbin, nmicchannel);
    // msg->ffts=wtk_complex_new_p2(nbin, nmicchannel);
    // msg->ffty=wtk_complex_new_p2(nbin, nmicchannel);
    msg->cohv=(float *)wtk_malloc(nbin*sizeof(float));
    memset(msg->cohv,0,sizeof(float)*nbin);

	return msg;
}

void wtk_gainnet_bf4_fft_delete(wtk_gainnet_bf4_t *gainnet_bf4, wtk_gainnet_bf4_fft_msg_t *msg)
{
	wtk_complex_delete_p2(msg->fft,gainnet_bf4->nbin);
    // wtk_complex_delete_p2(msg->ffts,gainnet_bf4->nbin);
	// wtk_complex_delete_p2(msg->ffty,gainnet_bf4->nbin);
    wtk_free(msg->cohv);
	wtk_free(msg);
}

void wtk_gainnet_bf4_fft_cpy(wtk_gainnet_bf4_t *gainnet_bf4, wtk_gainnet_bf4_fft_msg_t *imsg, wtk_gainnet_bf4_fft_msg_t *omsg)
{
    wtk_complex_cpy_p2(omsg->fft, imsg->fft, gainnet_bf4->nbin, gainnet_bf4->cfg->nmicchannel);
    // wtk_complex_cpy_p2(omsg->ffts, imsg->ffts, gainnet_bf4->nbin, gainnet_bf4->cfg->nmicchannel);
    // wtk_complex_cpy_p2(omsg->ffty, imsg->ffty, gainnet_bf4->nbin, gainnet_bf4->cfg->nmicchannel);
    memcpy(omsg->cohv, imsg->cohv, sizeof(float)*gainnet_bf4->nbin);
}

wtk_gainnet_bf4_fft_msg_t* wtk_gainnet_bf4_fft_pop_msg(wtk_gainnet_bf4_t *gainnet_bf4)
{
	return  (wtk_gainnet_bf4_fft_msg_t*)wtk_hoard_pop(&(gainnet_bf4->msg_hoard));
}

void wtk_gainnet_bf4_fft_push_msg(wtk_gainnet_bf4_t *gainnet_bf4,wtk_gainnet_bf4_fft_msg_t *msg)
{
	//wtk_debug("push [%ld]=%p\n",(long)(msg->hook),msg);
	wtk_hoard_push(&(gainnet_bf4->msg_hoard),msg);
}

wtk_gainnet_bf4_t* wtk_gainnet_bf4_new(wtk_gainnet_bf4_cfg_t *cfg)
{
	wtk_gainnet_bf4_t *gainnet_bf4;
	int i;

	gainnet_bf4=(wtk_gainnet_bf4_t *)wtk_malloc(sizeof(wtk_gainnet_bf4_t));
	gainnet_bf4->cfg=cfg;
	gainnet_bf4->ths=NULL;
	gainnet_bf4->notify=NULL;
	gainnet_bf4->ssl_ths=NULL;
    gainnet_bf4->notify_ssl=NULL;

	gainnet_bf4->mic=wtk_strbufs_new(gainnet_bf4->cfg->nmicchannel);
	gainnet_bf4->sp=wtk_strbufs_new(gainnet_bf4->cfg->nspchannel);

	gainnet_bf4->notch_mem=NULL;
	gainnet_bf4->memD=NULL;
	if(cfg->use_preemph)
	{
		gainnet_bf4->notch_mem=wtk_float_new_p2(cfg->nmicchannel,2);
		gainnet_bf4->memD=(float *)wtk_malloc(sizeof(float)*cfg->nmicchannel);
		gainnet_bf4->memX=0;
	}

	gainnet_bf4->nbin=cfg->wins/2+1;
	gainnet_bf4->window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	gainnet_bf4->synthesis_window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	gainnet_bf4->analysis_mem=wtk_float_new_p2(cfg->nmicchannel, gainnet_bf4->nbin-1);
	gainnet_bf4->analysis_mem_sp=wtk_float_new_p2(cfg->nspchannel, gainnet_bf4->nbin-1);
	gainnet_bf4->synthesis_mem=wtk_malloc(sizeof(float)*(gainnet_bf4->nbin-1));
	gainnet_bf4->rfft=wtk_drft_new(cfg->wins);
	gainnet_bf4->rfft_in=(float*)wtk_malloc(sizeof(float)*(cfg->wins));

	gainnet_bf4->fft=wtk_complex_new_p2(cfg->nmicchannel, gainnet_bf4->nbin);
	gainnet_bf4->fft_sp=wtk_complex_new_p2(cfg->nspchannel, gainnet_bf4->nbin);

    gainnet_bf4->erls=wtk_malloc(sizeof(wtk_rls_t)*(gainnet_bf4->nbin));
    for(i=0;i<gainnet_bf4->nbin;++i)
    {
      wtk_rls_init(gainnet_bf4->erls+i, &(cfg->echo_rls));
    }
	gainnet_bf4->fftx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*gainnet_bf4->nbin);
	gainnet_bf4->ffty=wtk_complex_new_p2(cfg->nmicchannel, gainnet_bf4->nbin);
	gainnet_bf4->ffts=wtk_complex_new_p2(gainnet_bf4->nbin, cfg->nmicchannel);
	gainnet_bf4->ffty2=wtk_complex_new_p2(gainnet_bf4->nbin, cfg->nmicchannel);
	if(cfg->use_postsingle)
	{
		gainnet_bf4->gaec=(wtk_gainnet_bf4_aec_t *)wtk_malloc(sizeof(wtk_gainnet_bf4_aec_t));
		wtk_gainnet_bf4_aec_init(gainnet_bf4->gaec, cfg);
	}else
	{
		gainnet_bf4->gaec=(wtk_gainnet_bf4_aec_t *)wtk_malloc(sizeof(wtk_gainnet_bf4_aec_t)*cfg->nmicchannel);
		for(i=0; i<cfg->nmicchannel; ++i)
		{
			wtk_gainnet_bf4_aec_init(gainnet_bf4->gaec+i, cfg);
		}
	}

	gainnet_bf4->covm=wtk_covm_new(&(cfg->covm), gainnet_bf4->nbin, cfg->nmicchannel);
	gainnet_bf4->bf=wtk_bf_new(&(cfg->bf), gainnet_bf4->cfg->wins);

    gainnet_bf4->aspec=wtk_aspec_new(&(cfg->aspec), gainnet_bf4->nbin, 3);        

    wtk_hoard_init2(&(gainnet_bf4->msg_hoard),offsetof(wtk_gainnet_bf4_fft_msg_t,hoard_n),10,
        (wtk_new_handler_t)wtk_gainnet_bf4_fft_new,
        (wtk_delete_handler2_t)wtk_gainnet_bf4_fft_delete,
        gainnet_bf4);

    wtk_queue_init(&(gainnet_bf4->fft_q));
    gainnet_bf4->cov=NULL;
    if(gainnet_bf4->aspec && gainnet_bf4->aspec->need_cov)
    {
        gainnet_bf4->cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nmicchannel*cfg->nmicchannel);
        if(cfg->lt<=0)
        {
            gainnet_bf4->wint=wtk_malloc(sizeof(float));
            gainnet_bf4->wint[0]=1;
        }else
        {
            gainnet_bf4->wint=wtk_math_create_hanning_window(2*cfg->lt+1);
        }

        if(cfg->lf<=0)
        {
            gainnet_bf4->winf=wtk_malloc(sizeof(float));
            gainnet_bf4->winf[0]=1;
        }else
        {
            gainnet_bf4->winf=wtk_math_create_hanning_window(2*cfg->lf+1);
        }
    }

    gainnet_bf4->inv_cov=NULL;
    gainnet_bf4->tmp=NULL;
    if(gainnet_bf4->aspec && gainnet_bf4->aspec->need_inv_cov)
    {
        gainnet_bf4->inv_cov=(wtk_complex_t *)wtk_malloc(cfg->nmicchannel*cfg->nmicchannel*sizeof(wtk_complex_t));
        gainnet_bf4->tmp=(wtk_dcomplex_t *)wtk_malloc(cfg->nmicchannel*cfg->nmicchannel*2*sizeof(wtk_dcomplex_t));
    }

    gainnet_bf4->qenvelope=NULL;
	gainnet_bf4->q_fring=NULL;
	if(cfg->use_simple_qenvelope){
		gainnet_bf4->q_fring = wtk_fring_new(cfg->qenvl.envelope_nf+1);
	}else if(cfg->use_qenvelope)
    {
        gainnet_bf4->qenvelope=wtk_qenvelope_new(&(cfg->qenvl));
        wtk_qenvelope_set_notify(gainnet_bf4->qenvelope, gainnet_bf4, (wtk_qenvelope_notify_f)wtk_gainnet_bf4_on_qenvelope);
    }

	gainnet_bf4->qmmse=NULL;
	if(cfg->use_qmmse)
	{
		gainnet_bf4->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}

	gainnet_bf4->maskssl2=NULL;
	if(cfg->use_maskssl2 && !cfg->use_ssl_delay)
	{
		gainnet_bf4->maskssl2=wtk_maskssl2_new(&(cfg->maskssl2));
        wtk_maskssl2_set_notify(gainnet_bf4->maskssl2, gainnet_bf4, (wtk_maskssl2_notify_f)wtk_gainnet_bf4_on_ssl2);
	}

	gainnet_bf4->out=wtk_malloc(sizeof(float)*(gainnet_bf4->nbin-1));

	wtk_gainnet_bf4_reset(gainnet_bf4);

	return gainnet_bf4;
}

void wtk_gainnet_bf4_delete(wtk_gainnet_bf4_t *gainnet_bf4)
{
	int i;
	int nmicchannel=gainnet_bf4->cfg->nmicchannel;

	wtk_strbufs_delete(gainnet_bf4->mic,gainnet_bf4->cfg->nmicchannel);
	wtk_strbufs_delete(gainnet_bf4->sp,gainnet_bf4->cfg->nspchannel);
	if(gainnet_bf4->notch_mem)
	{
		wtk_float_delete_p2(gainnet_bf4->notch_mem, gainnet_bf4->cfg->nmicchannel);
		wtk_free(gainnet_bf4->memD);
	}
	wtk_free(gainnet_bf4->window);
	wtk_free(gainnet_bf4->synthesis_window);
	wtk_float_delete_p2(gainnet_bf4->analysis_mem, gainnet_bf4->cfg->nmicchannel);
	wtk_float_delete_p2(gainnet_bf4->analysis_mem_sp, gainnet_bf4->cfg->nspchannel);
	wtk_free(gainnet_bf4->synthesis_mem);
	wtk_free(gainnet_bf4->rfft_in);
	wtk_drft_delete(gainnet_bf4->rfft);
	wtk_complex_delete_p2(gainnet_bf4->fft, gainnet_bf4->cfg->nmicchannel);
	wtk_complex_delete_p2(gainnet_bf4->fft_sp, gainnet_bf4->cfg->nspchannel);

	if(gainnet_bf4->covm)
	{
		wtk_covm_delete(gainnet_bf4->covm);
	}
	wtk_bf_delete(gainnet_bf4->bf);

	for(i=0;i<gainnet_bf4->nbin;++i)
	{
		wtk_rls_clean(gainnet_bf4->erls+i);
	}
	wtk_free(gainnet_bf4->erls);

	wtk_free(gainnet_bf4->fftx);
	wtk_complex_delete_p2(gainnet_bf4->ffty, gainnet_bf4->cfg->nmicchannel);
	wtk_complex_delete_p2(gainnet_bf4->ffts, gainnet_bf4->nbin);
	wtk_complex_delete_p2(gainnet_bf4->ffty2, gainnet_bf4->nbin);
	
	if(gainnet_bf4->cfg->use_postsingle)
	{
		wtk_gainnet_bf4_aec_clean(gainnet_bf4->gaec);
		wtk_free(gainnet_bf4->gaec);
	}else
	{
		for(i=0; i<nmicchannel; ++i)
		{
			wtk_gainnet_bf4_aec_clean(gainnet_bf4->gaec+i);
		}
		wtk_free(gainnet_bf4->gaec);
	}

	wtk_hoard_clean(&(gainnet_bf4->msg_hoard));
  	if(gainnet_bf4->cov)
    {
        wtk_free(gainnet_bf4->cov);
        wtk_free(gainnet_bf4->wint);
        wtk_free(gainnet_bf4->winf);
    }
    if(gainnet_bf4->inv_cov)
    {
        wtk_free(gainnet_bf4->inv_cov);
    }
    if(gainnet_bf4->tmp)
    {
        wtk_free(gainnet_bf4->tmp);
    }
    if(gainnet_bf4->aspec)
    {
        wtk_aspec_delete(gainnet_bf4->aspec);
    }
    if(gainnet_bf4->q_fring){
        wtk_fring_delete(gainnet_bf4->q_fring);
    }
    if(gainnet_bf4->qenvelope)
    {
        wtk_qenvelope_delete(gainnet_bf4->qenvelope);
    }
	if(gainnet_bf4->qmmse)
	{
		wtk_qmmse_delete(gainnet_bf4->qmmse);
	}
	if(gainnet_bf4->maskssl2)
    {
        wtk_maskssl2_delete(gainnet_bf4->maskssl2);
    }
	wtk_free(gainnet_bf4->out);

	wtk_free(gainnet_bf4);
}



void wtk_gainnet_bf4_start_aspec1(wtk_aspec_t *aspec, float theta, float phi, float theta_range)
{
    float theta2, theta3;

    theta2=theta+2*theta_range;
    if(theta2>=360)
    {
        theta2-=360;
    }
    theta3=theta-2*theta_range;
    if(theta3<0)
    {
        theta3+=360;
    }
    aspec->start_ang_num=3;
    wtk_aspec_start(aspec, theta, 0, 0);
    wtk_aspec_start(aspec, theta2, 0, 1);
    wtk_aspec_start(aspec, theta3, 0, 2);   
}

void wtk_gainnet_bf4_start_aspec2(wtk_aspec_t *aspec, float theta, float phi, float theta_range)
{
    float theta2, theta3;

    if(theta==0 || theta==180)
    {
        theta2=theta+2*theta_range;
        if(theta2>180)
        {
            theta2=360-theta2;
        }

        aspec->start_ang_num=2;
        wtk_aspec_start(aspec, theta, 0, 0);
        wtk_aspec_start(aspec, theta2, 0, 1);
    }else
    {
        theta2=theta+2*theta_range;
        if(theta2>180)
        {
            if(theta+theta_range>=180)
            {
                theta2=-1;
            }else
            {
                theta2=180;   
            }
        }
        theta3=theta-2*theta_range;
        if(theta3<0)
        {
            if(theta-theta_range<=0)
            {
                theta3=-1;
            }else
            {
                theta3=0;
            }
        }
        if(theta2==-1 || theta3==-1)
        {
            aspec->start_ang_num=2;
            wtk_aspec_start(aspec, theta, 0, 0);
            wtk_aspec_start(aspec, theta2==-1?theta3:theta2, 0, 1);
        }else
        {
            aspec->start_ang_num=3;
            wtk_aspec_start(aspec, theta, 0, 0);
            wtk_aspec_start(aspec, theta2, 0, 1);
            wtk_aspec_start(aspec, theta3, 0, 2);   
        }
    }
}



void wtk_gainnet_bf4_start(wtk_gainnet_bf4_t *gainnet_bf4)
{
	wtk_bf_update_ovec(gainnet_bf4->bf,gainnet_bf4->cfg->theta,gainnet_bf4->cfg->phi);
	wtk_bf_init_w(gainnet_bf4->bf);

    if(gainnet_bf4->cfg->use_line)
    {
        wtk_gainnet_bf4_start_aspec2(gainnet_bf4->aspec, gainnet_bf4->cfg->theta, gainnet_bf4->cfg->phi, gainnet_bf4->cfg->theta_range);
    }else
    {
        wtk_gainnet_bf4_start_aspec1(gainnet_bf4->aspec, gainnet_bf4->cfg->theta, gainnet_bf4->cfg->phi, gainnet_bf4->cfg->theta_range);
    }

    if(gainnet_bf4->cfg->use_t_r_qenvelope){
        float **t_r_qenvl=gainnet_bf4->cfg->t_r_qenvl;
        int idx=0;
        float min_dis=360;
        float dis;
		int i;
		for(i=0;i<gainnet_bf4->cfg->t_r_number;++i){
			if(t_r_qenvl[i][0] == -1){
				dis = fabs(gainnet_bf4->cfg->theta_range-t_r_qenvl[i][1]);
				if(dis < min_dis){
					min_dis = dis;
					idx = i;
				}
			}else{
				if(gainnet_bf4->cfg->theta == t_r_qenvl[i][0]){
					dis = fabs(gainnet_bf4->cfg->theta_range-t_r_qenvl[i][1]);
					if(dis < min_dis){
						min_dis = dis;
						idx = i;
					}
				}
			}
		}
		gainnet_bf4->cfg->qenvl.envelope_thresh = t_r_qenvl[idx][3];
		gainnet_bf4->cfg->qenvl.min_speccrest = t_r_qenvl[idx][4];
		gainnet_bf4->cfg->qenvl.right_min_thresh = t_r_qenvl[idx][5];
	}

}

void wtk_gainnet_bf4_reset(wtk_gainnet_bf4_t *gainnet_bf4)
{
	int wins=gainnet_bf4->cfg->wins;
	int frame_size=gainnet_bf4->cfg->wins/2;
	int i,nbin=gainnet_bf4->nbin;
	int nmicchannel=gainnet_bf4->cfg->nmicchannel;
	int shift, nshift, j, n;
	wtk_queue_node_t *qn;
    wtk_gainnet_bf4_fft_msg_t *msg;

	wtk_strbufs_reset(gainnet_bf4->mic,gainnet_bf4->cfg->nmicchannel);
	wtk_strbufs_reset(gainnet_bf4->sp,gainnet_bf4->cfg->nspchannel);
	if(gainnet_bf4->notch_mem)
	{
		for(i=0;i<gainnet_bf4->cfg->nmicchannel;++i)
		{
			memset(gainnet_bf4->notch_mem[i],0,2*sizeof(float));
		}
		memset(gainnet_bf4->memD,0,gainnet_bf4->cfg->nmicchannel*sizeof(float));
		gainnet_bf4->memX=0;
	}
	for (i=0;i<wins;++i)
	{
		gainnet_bf4->window[i] = sin((0.5+i)*PI/(wins));//sin(.5*PI*sin(.5*PI*(i+.5)/frame_size) * sin(.5*PI*(i+.5)/frame_size));
		gainnet_bf4->synthesis_window[i]=0;
	}
	shift=wins-frame_size;
	nshift=wins / shift;
	for(i=0;i<shift;++i)
	{
		for(j=0;j<nshift+1;++j)
		{
			n = i+j*shift;
			if(n < wins)
			{
				gainnet_bf4->synthesis_window[i] += gainnet_bf4->window[n]*gainnet_bf4->window[n];
			}
		}
	}
	for(i=1;i<nshift;++i)
	{
		for(j=0;j<shift;++j)
		{
			gainnet_bf4->synthesis_window[i*shift+j] = gainnet_bf4->synthesis_window[j];
		}
	}
	for(i=0;i<wins;++i)
	{
		gainnet_bf4->synthesis_window[i]=gainnet_bf4->window[i]/gainnet_bf4->synthesis_window[i];
	}


	wtk_float_zero_p2(gainnet_bf4->analysis_mem, gainnet_bf4->cfg->nmicchannel, (gainnet_bf4->nbin-1));
	wtk_float_zero_p2(gainnet_bf4->analysis_mem_sp, gainnet_bf4->cfg->nspchannel, (gainnet_bf4->nbin-1));
	memset(gainnet_bf4->synthesis_mem, 0, sizeof(float)*(gainnet_bf4->nbin-1));

	wtk_complex_zero_p2(gainnet_bf4->fft, gainnet_bf4->cfg->nmicchannel, gainnet_bf4->nbin);
	wtk_complex_zero_p2(gainnet_bf4->fft_sp, gainnet_bf4->cfg->nspchannel, gainnet_bf4->nbin);


    while(gainnet_bf4->fft_q.length>0)
    {
        qn=wtk_queue_pop(&(gainnet_bf4->fft_q));
        if(!qn){break;}
        msg=(wtk_gainnet_bf4_fft_msg_t *)data_offset(qn,wtk_gainnet_bf4_fft_msg_t,q_n);
        wtk_gainnet_bf4_fft_push_msg(gainnet_bf4,msg);
    }

    if(gainnet_bf4->covm)
	{
		wtk_covm_reset(gainnet_bf4->covm);
	}
	wtk_bf_reset(gainnet_bf4->bf);

	if(gainnet_bf4->aspec)
    {
        wtk_aspec_reset(gainnet_bf4->aspec);
    }

    if(gainnet_bf4->q_fring){
        wtk_fring_reset(gainnet_bf4->q_fring);
        gainnet_bf4->q_spec=0;
        gainnet_bf4->right_nf=0;
    }
	gainnet_bf4->nframe=0;
    if(gainnet_bf4->qenvelope)
    {
        wtk_qenvelope_reset(gainnet_bf4->qenvelope);
    }

    if(gainnet_bf4->qmmse)
    {
        wtk_qmmse_reset(gainnet_bf4->qmmse);
    }

	for(i=0;i<nbin;++i)
    {
		wtk_rls_reset(gainnet_bf4->erls+i);
	}
	wtk_complex_zero_p2(gainnet_bf4->ffty, gainnet_bf4->cfg->nmicchannel, gainnet_bf4->nbin);

	wtk_complex_zero_p2(gainnet_bf4->ffts, gainnet_bf4->nbin, gainnet_bf4->cfg->nmicchannel);
	wtk_complex_zero_p2(gainnet_bf4->ffty2, gainnet_bf4->nbin, gainnet_bf4->cfg->nmicchannel);

	memset(gainnet_bf4->fftx, 0, sizeof(wtk_complex_t)*(gainnet_bf4->nbin));

	if(gainnet_bf4->cfg->use_postsingle)
	{
		wtk_gainnet_bf4_aec_reset(gainnet_bf4->gaec);
	}else
	{
		for(i=0; i<nmicchannel; ++i)
		{
			wtk_gainnet_bf4_aec_reset(gainnet_bf4->gaec+i);
		}
	}
	gainnet_bf4->ssl_enable=0;
	if(gainnet_bf4->maskssl2)
    {
        wtk_maskssl2_reset(gainnet_bf4->maskssl2);
		gainnet_bf4->ssl_enable=1;
    }
	gainnet_bf4->sp_silcnt=0;
	gainnet_bf4->sp_sil=1;

	gainnet_bf4->mic_silcnt=0;
	gainnet_bf4->mic_sil=1;

	gainnet_bf4->pframe=0;
}


void wtk_gainnet_bf4_set_notify(wtk_gainnet_bf4_t *gainnet_bf4,void *ths,wtk_gainnet_bf4_notify_f notify)
{
	gainnet_bf4->notify=notify;
	gainnet_bf4->ths=ths;
}

void wtk_gainnet_bf4_set_ssl_notify(wtk_gainnet_bf4_t *gainnet_bf4,void *ths,wtk_gainnet_bf4_notify_ssl_f notify)
{
	gainnet_bf4->notify_ssl=notify;
	gainnet_bf4->ssl_ths=ths;
}

static void compute_band_energy(float *bandE, int *eband, int nb_bands, wtk_complex_t *fft) 
{
    int i;
    int j;
    int band_size;
    float tmp;
    float frac;

    memset(bandE, 0, sizeof(float)*nb_bands);
    for (i=0;i<nb_bands-1;++i)
    {
        band_size = eband[i+1]-eband[i];
        for (j=0;j<band_size;j++) 
        {
            frac = (float)j/band_size;
            tmp = fft[eband[i] + j].a* fft[eband[i] + j].a+ fft[eband[i] + j].b* fft[eband[i] + j].b;
            bandE[i] += (1-frac)*tmp;
            bandE[i+1] += frac*tmp;
        }
    }
    bandE[0] *= 2;
    bandE[nb_bands-1] *= 2;
}

static void interp_band_gain(int *eband, int nb_bands, int nbin, float *g, const float *bandE)
{
  int i,j;
  int band_size;
  float frac;

  memset(g, 0, nbin*sizeof(float));
  for (i=0;i<nb_bands-1;++i)
  {
    band_size = eband[i+1]-eband[i];
    for (j=0;j<band_size;j++)
    {
      frac = (float)j/band_size;
      g[eband[i] + j] = (1-frac)*bandE[i] + frac*bandE[i+1];
    }
  }
}

static void dct(float *dct_table, int nb_bands, float *out, const float *in)
{
  int i,j;
  float sum;

  for (i=0;i<nb_bands;++i)
  {
    sum = 0;
    for (j=0;j<nb_bands;++j)
    {
      sum += in[j] * dct_table[j*nb_bands + i];
    }
    out[i] = sum*sqrt(2./nb_bands);
  }
}

static void inverse_transform(wtk_gainnet_bf4_t *gainnet_bf4, wtk_complex_t *fft, float *out)
{
	wtk_drft_ifft2(gainnet_bf4->rfft,fft,out);
}

static void forward_transform(wtk_gainnet_bf4_t *gainnet_bf4, wtk_complex_t *fft, float *in)
{
	wtk_drft_fft2(gainnet_bf4->rfft,in,fft);
}

static void frame_analysis(wtk_gainnet_bf4_t *gainnet_bf4, float *rfft_in, float *analysis_mem, wtk_complex_t *fft, const float *in)
{
  int i;
  int wins=gainnet_bf4->cfg->wins;
  int fsize=wins/2;

  memmove(rfft_in, analysis_mem, fsize*sizeof(float));
  for(i=0;i<fsize;++i)
  {
    rfft_in[i+fsize]=in[i];
  }
  memcpy(analysis_mem, in, fsize*sizeof(float));
	for (i=0;i<wins;++i)
	{
		rfft_in[i] *= gainnet_bf4->window[i];
		//rfft_in[wins - 1 - i] *= gainnet_bf4->window[i];
	}
  forward_transform(gainnet_bf4, fft, rfft_in);
}

static void frame_synthesis(wtk_gainnet_bf4_t *gainnet_bf4, float *out, wtk_complex_t *fft)
{
  float *rfft_in=gainnet_bf4->rfft_in;
  int i;
  int wins=gainnet_bf4->cfg->wins;
  int fsize=wins/2;
  float *synthesis_mem=gainnet_bf4->synthesis_mem;

  inverse_transform(gainnet_bf4, fft, rfft_in);
  for (i=0;i<wins;++i)
  {
		rfft_in[i] *= gainnet_bf4->synthesis_window[i];
    // rfft_in[i] *= gainnet_bf4->window[i];
    // rfft_in[wins - 1 - i] *= gainnet_bf4->window[i];
  }
  for (i=0;i<fsize;i++) out[i] = rfft_in[i] + synthesis_mem[i];
  memcpy(synthesis_mem, &rfft_in[fsize], fsize*sizeof(float));
}

static int compute_frame_gaec_features(wtk_gainnet_bf4_aec_t *gaec, float *features, wtk_complex_t *fftx)
{
	float *Ex=gaec->Ex;
	int *eband=gaec->eband;
	int i,j,k;
	float E = 0;
	float *ceps_0, *ceps_1, *ceps_2;
	float spec_variability = 0;
	float *Ly=gaec->Ly;
	int nb_bands=gaec->cfg->nb_bands;
	int nb_delta_ceps=gaec->cfg->nb_delta_ceps;
	int ceps_mem=gaec->cfg->ceps_mem;
	float follow, logMax;
	float mindist,dist,tmp;

	compute_band_energy(Ex,eband, nb_bands, fftx);
	logMax = -2;
	follow = -2;
	for (i=0;i<nb_bands;i++)
	{
		Ly[i] = log10f(1e-2f+Ex[i]);
		Ly[i] = max(logMax-7, max(follow-1.5, Ly[i]));
		logMax = max(logMax, Ly[i]);
		follow = max(follow-1.5, Ly[i]);
		E += Ex[i];
	}
	dct(gaec->dct_table, nb_bands, features, Ly);
	
	if(gaec->cepstral_mem)
	{
		features[0] -= 12;
		features[1] -= 4;
		ceps_0 = gaec->cepstral_mem[gaec->memid];
		ceps_1 = (gaec->memid < 1) ? gaec->cepstral_mem[ceps_mem+gaec->memid-1] : gaec->cepstral_mem[gaec->memid-1];
		ceps_2 = (gaec->memid < 2) ? gaec->cepstral_mem[ceps_mem+gaec->memid-2] : gaec->cepstral_mem[gaec->memid-2];
		for (i=0;i<nb_bands;i++)
		{
			ceps_0[i] = features[i];
		}
		gaec->memid++;
		for (i=0;i<nb_delta_ceps;i++)
		{
			features[i] = ceps_0[i] + ceps_1[i] + ceps_2[i];
			features[nb_bands+i] = ceps_0[i] - ceps_2[i];
			features[nb_bands+nb_delta_ceps+i] =  ceps_0[i] - 2*ceps_1[i] + ceps_2[i];
		}
		/* Spectral variability features. */
		if (gaec->memid == ceps_mem)
		{
			gaec->memid = 0;
		}
		for (i=0;i<ceps_mem;++i)
		{
			mindist = 1e15f;
			for (j=0;j<ceps_mem;++j)
			{
					dist=0;
				for (k=0;k<nb_bands;++k)
				{
					tmp = gaec->cepstral_mem[i][k] - gaec->cepstral_mem[j][k];
					dist += tmp*tmp;
				}
				if (j!=i)
				{
					mindist = min(mindist, dist);
				}
			}
			spec_variability += mindist;
		}
		features[nb_bands+2*nb_delta_ceps] = spec_variability/ceps_mem-2.1;
	}

	return E < 0.1;
}



static void compute_frame_gaec_features2(wtk_gainnet_bf4_aec_t *gaec, float *features, wtk_complex_t *fftx, wtk_complex_t *fftsp)
{
	float *Ex_sp=gaec->Ex;
	int *eband=gaec->eband;
	int i,j,k;
	float E = 0;
	float *ceps_0, *ceps_1, *ceps_2;
	float spec_variability = 0;
	float *Ly=gaec->Ly;
	int nb_bands=gaec->cfg->nb_bands;
	int nb_delta_ceps=gaec->cfg->nb_delta_ceps;
	int ceps_mem=gaec->cfg->ceps_mem;
	float follow, logMax;
	float mindist,dist,tmp;
	int nbin=gaec->nbin;
	wtk_complex_t *fftytmp, sed, *fftxtmp;
	float ef,yf;
	float leak;
	float Yf[1024];

	fftytmp=fftsp;
	fftxtmp=fftx;
	for(i=0;i<nbin;++i,++fftxtmp,++fftytmp)
	{
		ef=fftxtmp->a*fftxtmp->a+fftxtmp->b*fftxtmp->b;
		yf=fftytmp->a*fftytmp->a+fftytmp->b*fftytmp->b;
		sed.a=fftytmp->a*fftxtmp->a+fftytmp->b*fftxtmp->b;
		sed.b=-fftytmp->a*fftxtmp->b+fftytmp->b*fftxtmp->a;

		leak=(sed.a*sed.a+sed.b*sed.b)/(max(ef,yf)*yf+1e-9);
		Yf[i]=leak*yf;

		leak=sqrtf(leak);
		fftytmp->a*=leak;
		fftytmp->b*=leak;
	}
	if(gaec->qmmse)
	{
		wtk_qmmse_flush_mask(gaec->qmmse, fftx, Yf);
	}
	compute_band_energy(Ex_sp,eband, nb_bands, fftsp);
	logMax = -2;
	follow = -2;
	for (i=0;i<nb_bands;i++)
	{
		Ly[i] = log10f(1e-2f+Ex_sp[i]);
		Ly[i] = max(logMax-7, max(follow-1.5, Ly[i]));
		logMax = max(logMax, Ly[i]);
		follow = max(follow-1.5, Ly[i]);
		E += Ex_sp[i];
	}
	dct(gaec->dct_table, nb_bands, features, Ly);
	if(gaec->cepstral_mem_sp)
	{
		features[0] -= 12;
		features[1] -= 4;

		ceps_0 = gaec->cepstral_mem_sp[gaec->memid_sp];
		ceps_1 = (gaec->memid_sp < 1) ? gaec->cepstral_mem_sp[ceps_mem+gaec->memid_sp-1] : gaec->cepstral_mem_sp[gaec->memid_sp-1];
		ceps_2 = (gaec->memid_sp < 2) ? gaec->cepstral_mem_sp[ceps_mem+gaec->memid_sp-2] : gaec->cepstral_mem_sp[gaec->memid_sp-2];
		for (i=0;i<nb_bands;i++)
		{
			ceps_0[i] = features[i];
		}
		gaec->memid_sp++;
		for (i=0;i<nb_delta_ceps;i++)
		{
			features[i] = ceps_0[i] + ceps_1[i] + ceps_2[i];
			features[nb_bands+i] = ceps_0[i] - ceps_2[i];
			features[nb_bands+nb_delta_ceps+i] =  ceps_0[i] - 2*ceps_1[i] + ceps_2[i];
		}
		/* Spectral variability features. */
		if (gaec->memid_sp == ceps_mem)
		{
			gaec->memid_sp = 0;
		}
		for (i=0;i<ceps_mem;++i)
		{
			mindist = 1e15f;
			for (j=0;j<ceps_mem;++j)
			{
					dist=0;
				for (k=0;k<nb_bands;++k)
				{
					tmp = gaec->cepstral_mem_sp[i][k] - gaec->cepstral_mem_sp[j][k];
					dist += tmp*tmp;
				}
				if (j!=i)
				{
					mindist = min(mindist, dist);
				}
			}
			spec_variability += mindist;
		}
		features[nb_bands+2*nb_delta_ceps] = spec_variability/ceps_mem-2.1;
	}
}


static float wtk_gainnet_bf4_sp_energy(float *p,int n)
{
	float f;
	int i;

	f=0;
	for(i=0;i<n;++i)
	{
		f+=p[i]*p[i];
	}
	f/=n;

	return f;
}

// static float wtk_gainnet_bf4_fft_energy(wtk_complex_t *fftx,int nbin)
// {
// 	float f;
// 	int i;

// 	f=0;
// 	for(i=1; i<nbin-1; ++i)
// 	{
// 		f+=fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b;
// 	}

// 	return f;
// }

void wtk_gainnet_bf4_aec_on_gainnet(wtk_gainnet_bf4_aec_t *gaec, float *gain, int len, int is_end)
{
	memcpy(gaec->g, gain, sizeof(float)*gaec->cfg->nb_bands);
}


void wtk_gainnet_bf4_aec_on_gainnet2(wtk_gainnet_bf4_aec_t *gaec, float *gain, int len, int is_end)
{
	int i;
	int nb_bands=gaec->cfg->nb_bands;
	float *g2=gaec->g2;
	float agc_a=gaec->cfg->agc_a;
	float agc_b=gaec->cfg->agc_b;

	for(i=0; i<nb_bands; ++i)
	{
		g2[i]=-1/agc_a*(logf(1/gain[i]-1)-agc_b);
	}
}

void wtk_gainnet_bf4_aec_feed(wtk_gainnet_bf4_aec_t *gaec, wtk_complex_t *fftx, wtk_complex_t *ffty, wtk_complex_t **ffts, wtk_complex_t **ffty2, int nch, int sp_sil)
{
	int i;
	int nb_bands=gaec->cfg->nb_bands;
	int nbin=gaec->nbin;
	float *g=gaec->g, *gf=gaec->gf, *lastg=gaec->lastg, *g2=gaec->g2, *gf2=gaec->gf2;
	int *eband=gaec->eband;
	wtk_gainnet5_t *gainnet=gaec->gainnet;
	int nb_features=gaec->cfg->nb_features;
	int nb_features_x=gaec->cfg->nb_features_x;
	static float fx=2.0f*PI/RAND_MAX;
	float p;
	float *qmmse_gain;
	// time_t t;
	// srand((unsigned) time(&t));
	// float nn_alpha=gaec->cfg->nn_alpha;
	
 	if(gaec->cfg->use_miccnon || gaec->cfg->use_spcnon)
    {
      for(i=1;i<nbin-1;++i)
      {
        p=rand()*fx;
        if(gaec->cfg->use_miccnon)
        {
          fftx[i].a+=cosf(p)*gaec->cfg->micnenr;
          fftx[i].b+=sinf(p)*gaec->cfg->micnenr;
        }  
        if(gaec->cfg->use_spcnon)
        {
          ffty[i].a+=cosf(p)*gaec->cfg->spnenr;
          ffty[i].b+=sinf(p)*gaec->cfg->spnenr;
        }
      }
    }

    gaec->silence=compute_frame_gaec_features(gaec, gaec->features, fftx);
    compute_frame_gaec_features2(gaec, gaec->features+nb_features_x, fftx, ffty);
	if(0)
	{
		for (i=1; i<nbin-1; ++i)
		{
			gf[i]=0;
		}
	}else
	{
		if(gainnet)
		{
			wtk_gainnet5_feed(gainnet, gaec->features, nb_features_x, gaec->features+nb_features_x, nb_features-nb_features_x, 0);   
		}
		for (i=0;i<nb_bands;++i)
		{
			if(sp_sil==1)
			{
				g[i] = max(g[i], 0.6f*lastg[i]);
			}
			lastg[i] = g[i];
		}
		interp_band_gain(eband, nb_bands, nbin, gf, g);
		if(gainnet && gainnet->cfg->use_agc)
		{
			interp_band_gain(eband, nb_bands, nbin, gf2, g2);
		}
		if(gaec->qmmse)
		{
			qmmse_gain=gaec->qmmse->gain;
			for (i=1; i<nbin-1; ++i)
			{
				if(gf[i]>qmmse_gain[i])
				{
					if(gainnet && gainnet->cfg->use_agc)
					{
						gf2[i]*=qmmse_gain[i]/gf[i];
					}
					gf[i]=qmmse_gain[i];
				}
			}
		}
	}

	for (i=1; i<nbin-1; ++i)
	{
		ffts[i][nch].a *= gf[i];
		ffts[i][nch].b *= gf[i];

		ffty2[i][nch].a=fftx[i].a-ffts[i][nch].a;
		ffty2[i][nch].b=fftx[i].b-ffts[i][nch].b;
	}

	if(gainnet && gainnet->cfg->use_agc)
	{
		gaec->gf2_m=wtk_float_abs_mean(gf2+1, nbin-2);
		for (i=1;i<nbin-1;++i)
		{
			ffts[i][nch].a *= gaec->gf2_m;
			ffts[i][nch].b *= gaec->gf2_m;
		}
	}
}

void wtk_gainnet_bf4_feed_aec(wtk_gainnet_bf4_t *gainnet_bf4, wtk_complex_t **fft, wtk_complex_t **fft_sp)
{
	int nmicchannel=gainnet_bf4->cfg->nmicchannel;
	int nspchannel=gainnet_bf4->cfg->nspchannel;
	int i,j,k;
	int nbin=gainnet_bf4->nbin;
	wtk_rls_t *erls=gainnet_bf4->erls, *erlstmp;
	wtk_gainnet_bf4_aec_t *gaec=gainnet_bf4->gaec;
	wtk_complex_t fft2[64];
	wtk_complex_t fftsp2[10];
	wtk_complex_t **ffty=gainnet_bf4->ffty;
	wtk_complex_t **ffty2=gainnet_bf4->ffty2;
	wtk_complex_t **ffts=gainnet_bf4->ffts;
	float pframe_alpha=gainnet_bf4->cfg->pframe_alpha;
	int pfs=gainnet_bf4->cfg->pframe_fs;
	int fe_len=gainnet_bf4->cfg->pframe_fe-gainnet_bf4->cfg->pframe_fs;
	float *gf=gaec->gf;

	wtk_complex_zero_p2(gainnet_bf4->ffty, gainnet_bf4->cfg->nmicchannel, gainnet_bf4->nbin);

	erlstmp=erls;
	for(k=0; k<nbin; ++k, ++erlstmp)
	{
		for(i=0; i<nmicchannel; ++i)
		{
			fft2[i]=fft[i][k];
		}
		for(j=0; j<nspchannel; ++j)
		{
			fftsp2[j]=fft_sp[j][k];
		}
		wtk_rls_feed3(erlstmp, fft2, fftsp2, gainnet_bf4->sp_sil==0);
		if(gainnet_bf4->sp_sil==0)
		{
			for(i=0; i<nmicchannel; ++i)
			{
				fft[i][k]=erlstmp->out[i];
				ffts[k][i]=erlstmp->out[i];
				ffty[i][k]=erlstmp->lsty[i];
			}
		}else
		{
			for(i=0; i<nmicchannel; ++i)
			{
				ffts[k][i]=fft[i][k];
				ffty[i][k].a=ffty[i][k].b=0;
			}
		}

	}
	if(gainnet_bf4->cfg->use_postsingle)
	{
		wtk_gainnet_bf4_aec_feed(gaec, fft[0], ffty[0], ffts, ffty2, 0, gainnet_bf4->sp_sil);
		for (k=1; k<nbin-1; ++k)
		{
			for(i=1; i<nmicchannel; ++i)
			{
				ffts[k][i].a *= gf[k];
				ffts[k][i].b *= gf[k];

				ffty2[k][i].a=fft[i][k].a-ffts[k][i].a;
				ffty2[k][i].b=fft[i][k].b-ffts[k][i].b;

				if(gaec->gainnet && gaec->gainnet->cfg->use_agc)
				{
					ffts[k][i].a *= gaec->gf2_m;
					ffts[k][i].b *= gaec->gf2_m;
				}
			}
		}

	}else
	{
		for(i=0; i<nmicchannel; ++i, ++gaec)
		{
			wtk_gainnet_bf4_aec_feed(gaec, fft[i], ffty[i], ffts, ffty2, i, gainnet_bf4->sp_sil);
		}
	}
	gainnet_bf4->pframe=(1-pframe_alpha)*gainnet_bf4->pframe+pframe_alpha*wtk_float_abs_mean(gainnet_bf4->gaec->gf+pfs, fe_len);
}

void wtk_gainnet_bf4_on_ssl2(wtk_gainnet_bf4_t *gainnet_bf4, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te)
{
	// printf("[%d %d] ==> %d %d %f\n", ts,te, nbest_extp[0].theta, nbest_extp[0].phi, nbest_extp[0].nspecsum);
    if(gainnet_bf4->notify_ssl)
    {
        gainnet_bf4->notify_ssl(gainnet_bf4->ssl_ths, ts, te, nbest_extp, nbest);
    }
}

void wtk_gainnet_bf4_control_bs(wtk_gainnet_bf4_t *gainnet_bf4, float *out, int len)
{
	float out_max;
	static float scale=1.0;
	static float last_scale=1.0;
	static int max_cnt=0;
	int i;

	if(gainnet_bf4->mic_sil==0)
	{
		out_max=wtk_float_abs_max(out, len);
		if(out_max>30000.0)
		{
			scale=30000.0/out_max;
			if(scale<last_scale)
			{
				last_scale=scale;
			}else
			{
				scale=last_scale;
			}
			max_cnt=10;
		}
		for(i=0; i<len; ++i)
		{
			out[i]*=scale;
		}
		if(max_cnt>0)
		{
			--max_cnt;
		}
		if(max_cnt<=0 && scale<1.0)
		{
			scale*=1.1;
			last_scale=scale;
			if(scale>1.0)
			{
				scale=1.0;
				last_scale=1.0;
			}
		}
	}else
	{
		scale=1.0;
		last_scale=1.0;
		max_cnt=0;
	}
} 

void wtk_gainnet_bf4_flush(wtk_gainnet_bf4_t *gainnet_bf4,wtk_gainnet_bf4_fft_msg_t *msg)
{
    int k;
    int nbin=gainnet_bf4->bf->nbin;
    wtk_bf_t *bf=gainnet_bf4->bf;
    int i, nmicchannel=gainnet_bf4->cfg->nmicchannel;
    int b;
    wtk_covm_t *covm=gainnet_bf4->covm;
    wtk_complex_t *fftx=gainnet_bf4->fftx;
    wtk_complex_t **fft=msg->fft;
    // wtk_complex_t **ffts=msg->ffts;
    // wtk_complex_t **ffty=msg->ffty;
    float *cohv=msg->cohv;
	int wins=gainnet_bf4->cfg->wins;
	int fsize=wins/2;
	float *out=gainnet_bf4->out;
	short *pv=(short *)out;

    for(k=1; k<nbin-1; ++k)
    {
        b=0;
        if(cohv[k]<0.0)
        {
			b=wtk_covm_feed_fft2(covm, fft, k, 1);
			if(b==1)
			{
				wtk_bf_update_ncov(bf, covm->ncov, k);
			}
		}else
		{
			if(covm->scov)
			{
				b=wtk_covm_feed_fft2(covm, fft, k, 0);
				if(b==1)
				{
					wtk_bf_update_scov(bf, covm->scov, k);
				}
			}
		}
        if(covm->ncnt_sum[k]>0 && (covm->scnt_sum==NULL ||  covm->scnt_sum[k]>0) && b==1)
        {
            wtk_bf_update_w(bf, k);
        }

        if(gainnet_bf4->cfg->qenvl.debug==1)
        {
            if(cohv[k]<0)
            {
                for(i=0;i<nmicchannel;++i)
                {
                    bf->w[k][i].a=0;
                    bf->w[k][i].b=0;
                }
            }else
            {
                for(i=0;i<nmicchannel;++i)
                {
                    bf->w[k][i].a=0;
                    bf->w[k][i].b=0;
                    if(i==0)
                    {
                        bf->w[k][i].a=1;
                    }
                }
            }
        }
        wtk_bf_output_fft_k(bf, fft[k], fftx+k, k);
		// fftx[k]=fft[k][0];
    }
    if(gainnet_bf4->qmmse)
    {
        wtk_qmmse_feed_cohv(gainnet_bf4->qmmse,fftx,cohv);      
    }
	frame_synthesis(gainnet_bf4, out, fftx);
	if(gainnet_bf4->notch_mem)
	{
		gainnet_bf4->memX=wtk_preemph_asis2(out,fsize,gainnet_bf4->memX);
	}
	wtk_gainnet_bf4_control_bs(gainnet_bf4, out, fsize);
	for(i=0; i<fsize; ++i)
	{
		pv[i]=floorf(out[i]+0.5);
	}
	if(gainnet_bf4->notify)
	{
		gainnet_bf4->notify(gainnet_bf4->ths,pv,fsize);
	}
}

void wtk_gainnet_bf4_on_qenvelope(wtk_gainnet_bf4_t *gainnet_bf4,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end)
{
	wtk_gainnet_bf4_fft_msg_t *gbmsg;
    int k;
    int nbin=gainnet_bf4->nbin;

    if(msg)
    {
        gbmsg=(wtk_gainnet_bf4_fft_msg_t *)msg->hook;
        if(state==WTK_QENVELOPE_TROUGH)
        {
            for(k=1; k<nbin-1; ++k)
            {
                gbmsg->cohv[k]=-1;
            }
        }else if(state==WTK_QENVELOPE_CREST || state==WTK_QENVELOPE_FLAT)
        {
            for(k=1; k<nbin-1; ++k)
            {
                gbmsg->cohv[k]=1;
            }
        }
        wtk_gainnet_bf4_flush(gainnet_bf4, gbmsg);
		wtk_gainnet_bf4_fft_push_msg(gainnet_bf4, gbmsg);
    }
}


void wtk_gainnet_bf4_update_aspec2(wtk_gainnet_bf4_t *gainnet_bf4, wtk_aspec_t *aspec, wtk_complex_t *cov, 
                                                                                    wtk_complex_t *inv_cov, float cov_travg, int k, float *spec_k, float *cohv)
{
    int sang_num=aspec->start_ang_num;
    int n;

    for(n=0; n<sang_num; ++n)
    {
        spec_k[n]=wtk_aspec_flush_spec_k(aspec, NULL, 0, cov_travg, cov, inv_cov, k ,n);
    }

    *cohv=1;
    for(n=1; n<sang_num; ++n)
    {
        if(spec_k[0]<=spec_k[n])
        {
            *cohv=-1;
        }
    }
}

void wtk_gainnet_bf4_flush_aspec_lt(wtk_gainnet_bf4_t *gainnet_bf4, int index)
{
    wtk_queue_t *fft_q=&(gainnet_bf4->fft_q);
    int lf=gainnet_bf4->cfg->lf;
    int lt=gainnet_bf4->cfg->lt;
    int i,j,k,k2,tt,ff;
    wtk_queue_node_t *qn;
    wtk_gainnet_bf4_fft_msg_t *smsg,*smsg_index,*gbmsg;
    int nbin=gainnet_bf4->nbin;
    int nmicchannel=gainnet_bf4->cfg->nmicchannel;
    wtk_complex_t *cov=gainnet_bf4->cov;
    wtk_complex_t **fft,*fft1,*fft2,*a,*b;
    float *wint=gainnet_bf4->wint;
    float *winf=gainnet_bf4->winf;
    float wint2,wintf,winsum;
    wtk_complex_t *inv_cov=gainnet_bf4->inv_cov;
    wtk_dcomplex_t *tmp=gainnet_bf4->tmp;
    float cov_travg;
    int ret;
    float *cohv;
    float spec_k[3]={0}, specsum;
	int specsum_fs=gainnet_bf4->cfg->specsum_fs;
    int specsum_fe=gainnet_bf4->cfg->specsum_fe;
    float max_spec;
    float qenvel_alpha = gainnet_bf4->cfg->qenvel_alpha;
    float qenvel_alpha_1 = 1.0 - qenvel_alpha;
    float min_speccrest;
    int right_nf;
    float envelope_thresh;
    float right_min_thresh;

    qn=wtk_queue_peek(fft_q, index);
    smsg_index=data_offset2(qn,wtk_gainnet_bf4_fft_msg_t,q_n);
    cohv=smsg_index->cohv;

    specsum=0;
    for(k=1;k<nbin-1;++k)
    {
        memset(cov,0,sizeof(wtk_complex_t)*nmicchannel*nmicchannel);
        winsum=0;
        for(qn=fft_q->pop,tt=2*lt+1-fft_q->length;qn;qn=qn->next,++tt)
        {
            wint2=wint[tt];
            smsg=data_offset2(qn,wtk_gainnet_bf4_fft_msg_t,q_n);
            fft=smsg->fft;
            for(k2=max(1,k-lf),ff=k2-(k-lf);k2<min(nbin-1,k+lf+1);++k2,++ff)
            {
                wintf=wint2*winf[ff];
                winsum+=wintf;

                fft1=fft2=fft[k2];
                for(i=0;i<nmicchannel;++i,++fft1)
                {
                    fft2=fft1;
                    for(j=i;j<nmicchannel;++j,++fft2)
                    {
                        a=cov+i*nmicchannel+j;
                        if(i!=j)
                        {
                            a->a+=(fft1->a*fft2->a+fft1->b*fft2->b)*wintf;
                            a->b+=(-fft1->a*fft2->b+fft1->b*fft2->a)*wintf;
                        }else
                        {
                            a->a+=(fft1->a*fft2->a+fft1->b*fft2->b)*wintf;
                            a->b+=0;
                        }
                    }
                }
            }
        }
        winsum=1.0/winsum;
        for(i=0;i<nmicchannel;++i)
        {
            for(j=i;j<nmicchannel;++j)
            {
                a=cov+i*nmicchannel+j;
                a->a*=winsum;
                a->b*=winsum;

                if(i!=j)
                {
                    b=cov+j*nmicchannel+i;
                    b->a=a->a;
                    b->b=-a->b;
                }
            }
        }
        if(inv_cov)
        {
            ret=wtk_complex_invx4(cov,tmp,nmicchannel,inv_cov,1);            
            if(ret!=0)
            {
                j=0;
                for(i=0;i<nmicchannel;++i)
                {
                    cov[j].a+=0.01;
                    j+=nmicchannel+1;
                }
                wtk_complex_invx4(cov,tmp,nmicchannel,inv_cov,1);
            }
        }

        cov_travg=0;
        if(gainnet_bf4->aspec->need_cov_travg) 
        {
            for(i=0;i<nmicchannel;++i)
            {
                cov_travg+=cov[i*nmicchannel+i].a;
            }
            cov_travg/=nmicchannel;
        }

        wtk_gainnet_bf4_update_aspec2(gainnet_bf4, gainnet_bf4->aspec, cov, inv_cov, cov_travg, k, spec_k, cohv+k);
		if(k>=specsum_fs && k<=specsum_fe  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
		{
			specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
		}

    }
	if(gainnet_bf4->cfg->use_simple_qenvelope){
		wtk_fring_push2(gainnet_bf4->q_fring, specsum);
		min_speccrest = gainnet_bf4->cfg->qenvl.min_speccrest;
		right_nf = gainnet_bf4->cfg->qenvl.right_nf;
		envelope_thresh = gainnet_bf4->cfg->qenvl.envelope_thresh;
		right_min_thresh = gainnet_bf4->cfg->qenvl.right_min_thresh;
		if(gainnet_bf4->q_fring->used == gainnet_bf4->q_fring->nslot - 1){
			max_spec = wtk_fring_max(gainnet_bf4->q_fring);
			// printf("%f\n", max_spec);
			if(max_spec < gainnet_bf4->q_spec){
				max_spec = qenvel_alpha * gainnet_bf4->q_spec + qenvel_alpha_1 * max_spec; 
			}
			// printf("%f\n", max_spec);
			gainnet_bf4->q_spec = max_spec;
			if(max_spec > min_speccrest){
				gainnet_bf4->right_nf = right_nf;
			}else if(max_spec > envelope_thresh){
				// --gainnet_bf4->right_nf;
			}else if(max_spec > right_min_thresh){
				--gainnet_bf4->right_nf;
			}else{
				gainnet_bf4->right_nf = 0;
			}
			if(gainnet_bf4->right_nf <= 0){
				for(k=1; k<nbin-1; ++k)
				{
					cohv[k]=-1;
				}
			}else{
				if(gainnet_bf4->cfg->use_sqenvelope){
					for(k=1; k<nbin-1; ++k)
					{
						cohv[k]=1;
					}
				}
			}
		}
		wtk_gainnet_bf4_flush(gainnet_bf4, smsg_index);
	}else if(gainnet_bf4->cfg->use_qenvelope)
    {
		gbmsg=wtk_gainnet_bf4_fft_pop_msg(gainnet_bf4);
		wtk_gainnet_bf4_fft_cpy(gainnet_bf4, smsg_index, gbmsg);
        wtk_qenvelope_feed(gainnet_bf4->qenvelope, specsum, (void *)gbmsg, 0);
    }else
    {
		wtk_gainnet_bf4_flush(gainnet_bf4, smsg_index);
	}
}


void wtk_gainnet_bf4_update_aspec(wtk_gainnet_bf4_t *gainnet_bf4, wtk_aspec_t *aspec, wtk_complex_t **fft, float fftabs2, int k, float *spec_k, float *cohv)
{
    int sang_num=aspec->start_ang_num;
    int n;

    for(n=0; n<sang_num; ++n)
    {
        spec_k[n]=wtk_aspec_flush_spec_k(aspec, fft, fftabs2, 0, NULL, NULL, k ,n);
    }
    *cohv=1;
    for(n=1; n<sang_num; ++n)
    {
        if(spec_k[0]<=spec_k[n])
        {
            *cohv=-1;
        }
    }
}

void wtk_gainnet_bf4_flush_aspec(wtk_gainnet_bf4_t *gainnet_bf4, wtk_gainnet_bf4_fft_msg_t *msg)
{
    int k,i;
    int nbin=gainnet_bf4->nbin;
    int nmicchannel=gainnet_bf4->cfg->nmicchannel;
    wtk_complex_t **fft, *fft2;
    float fftabs2;
    float spec_k[3]={0}, specsum;
	int specsum_fs=gainnet_bf4->cfg->specsum_fs;
    int specsum_fe=gainnet_bf4->cfg->specsum_fe;
    float *cohv=msg->cohv;
	wtk_gainnet_bf4_fft_msg_t *gbmsg;
    float max_spec;
    float qenvel_alpha = gainnet_bf4->cfg->qenvel_alpha;
    float qenvel_alpha_1 = 1.0 - qenvel_alpha;
    float min_speccrest;
    int right_nf;
    float envelope_thresh;
    float right_min_thresh;

    fft=msg->fft;
	specsum=0;
    for(k=1; k<nbin-1; ++k)
    {
        fftabs2=0;
        fft2=fft[k];
        for(i=0; i<nmicchannel; ++i,++fft2)
        {
            fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
        }

        wtk_gainnet_bf4_update_aspec(gainnet_bf4,gainnet_bf4->aspec,fft,fftabs2,k,spec_k,cohv+k);
		if(k>=specsum_fs && k<=specsum_fe  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
		{
			specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
		}
    }	
	if(gainnet_bf4->cfg->use_simple_qenvelope){
		wtk_fring_push2(gainnet_bf4->q_fring, specsum);
		min_speccrest = gainnet_bf4->cfg->qenvl.min_speccrest;
		right_nf = gainnet_bf4->cfg->qenvl.right_nf;
		envelope_thresh = gainnet_bf4->cfg->qenvl.envelope_thresh;
		right_min_thresh = gainnet_bf4->cfg->qenvl.right_min_thresh;
		if(gainnet_bf4->q_fring->used == gainnet_bf4->q_fring->nslot - 1){
			max_spec = wtk_fring_max(gainnet_bf4->q_fring);
			if(max_spec < gainnet_bf4->q_spec){
				max_spec = qenvel_alpha * gainnet_bf4->q_spec + qenvel_alpha_1 * max_spec; 
			}
			// printf("%f\n", max_spec);
			gainnet_bf4->q_spec = max_spec;
			if(max_spec > min_speccrest){
				gainnet_bf4->right_nf = right_nf;
			}else if(max_spec > envelope_thresh){
				// --gainnet_bf4->right_nf;
			}else if(max_spec > right_min_thresh){
				--gainnet_bf4->right_nf;
			}else{
				gainnet_bf4->right_nf = 0;
			}
			if(gainnet_bf4->right_nf <= 0){
				for(k=1; k<nbin-1; ++k)
				{
					cohv[k]=-1;
				}
			}else{
				if(gainnet_bf4->cfg->use_sqenvelope){
					for(k=1; k<nbin-1; ++k)
					{
						cohv[k]=1;
					}
				}
			}
		}
		wtk_gainnet_bf4_flush(gainnet_bf4, msg);
	}else if(gainnet_bf4->cfg->use_qenvelope)
    {
		gbmsg=wtk_gainnet_bf4_fft_pop_msg(gainnet_bf4);
		wtk_gainnet_bf4_fft_cpy(gainnet_bf4, msg, gbmsg);
        wtk_qenvelope_feed(gainnet_bf4->qenvelope, specsum, (void *)gbmsg, 0);
    }else
    {
		wtk_gainnet_bf4_flush(gainnet_bf4, msg);
	}
}


void wtk_gainnet_bf4_feed_bf(wtk_gainnet_bf4_t *gainnet_bf4, wtk_complex_t **fft)
{
    int lt=gainnet_bf4->cfg->lt;
    wtk_queue_t *fft_q=&(gainnet_bf4->fft_q);
    wtk_queue_node_t *qn;
    wtk_gainnet_bf4_fft_msg_t *msg;
    int nbin=gainnet_bf4->nbin;
    int nmicchannel=gainnet_bf4->cfg->nmicchannel;
    int i,k;

	++gainnet_bf4->nframe;
    msg=wtk_gainnet_bf4_fft_pop_msg(gainnet_bf4);
    for(k=0; k<nbin; ++k)
    {
        for(i=0; i<nmicchannel; ++i)
        {
            msg->fft[k][i]=fft[k][i];
        }
    }
    if(gainnet_bf4->cov)
    {   
        wtk_queue_push(fft_q,&(msg->q_n));
        if(fft_q->length>=lt+1 && fft_q->length<2*lt+1)
        {
            wtk_gainnet_bf4_flush_aspec_lt(gainnet_bf4,fft_q->length-lt-1);
        }else if(fft_q->length==2*lt+1)
        {
            wtk_gainnet_bf4_flush_aspec_lt(gainnet_bf4,fft_q->length-lt-1);
            qn=wtk_queue_pop(fft_q);
            if(!qn){return;}
            msg=(wtk_gainnet_bf4_fft_msg_t *)data_offset(qn,wtk_gainnet_bf4_fft_msg_t,q_n);
            wtk_gainnet_bf4_fft_push_msg(gainnet_bf4, msg);
        }
    }else
    {
        wtk_gainnet_bf4_flush_aspec(gainnet_bf4,msg);
        wtk_gainnet_bf4_fft_push_msg(gainnet_bf4, msg);
    }   
}

static float wtk_gainnet_bf4_fft_energy(wtk_complex_t *fftx,int nbin)
{
	float f;
	int i;

	f=0;
	for(i=1; i<nbin-1; ++i)
	{
		f+=fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b;
	}

	return f;
}

void wtk_gainnet_bf4_feed(wtk_gainnet_bf4_t *gainnet_bf4,short *data,int len,int is_end)
{
	int i,j,k;
	int nbin=gainnet_bf4->nbin;
	int nmicchannel=gainnet_bf4->cfg->nmicchannel;
	int *mic_channel=gainnet_bf4->cfg->mic_channel;
	int nspchannel=gainnet_bf4->cfg->nspchannel;
	int *sp_channel=gainnet_bf4->cfg->sp_channel;
	int channel=gainnet_bf4->cfg->channel;
	wtk_strbuf_t **mic=gainnet_bf4->mic;
	wtk_strbuf_t **sp=gainnet_bf4->sp;
	float **notch_mem=gainnet_bf4->notch_mem;
	float *memD=gainnet_bf4->memD;
	float fv, *fp1, *fp2;
	int wins=gainnet_bf4->cfg->wins;
	int fsize=wins/2;
	int length;
	// float micenr;
	// float micenr_thresh=gainnet_bf4->cfg->micenr_thresh;
	// int micenr_cnt=gainnet_bf4->cfg->micenr_cnt;
	float spenr;
	float spenr_thresh=gainnet_bf4->cfg->spenr_thresh;
	int spenr_cnt=gainnet_bf4->cfg->spenr_cnt;
	float *rfft_in=gainnet_bf4->rfft_in;
	wtk_complex_t **fft=gainnet_bf4->fft;
	wtk_complex_t **fft_sp=gainnet_bf4->fft_sp;
	float **analysis_mem=gainnet_bf4->analysis_mem, **analysis_mem_sp=gainnet_bf4->analysis_mem_sp;
	// wtk_complex_t *fftx=gainnet_bf4->fftx;
	float fft_scale=gainnet_bf4->cfg->fft_scale;
	wtk_complex_t **ffts=gainnet_bf4->ffts;
	float micenr;
	float micenr_thresh=gainnet_bf4->cfg->micenr_thresh;
	int micenr_cnt=gainnet_bf4->cfg->micenr_cnt;

	for(i=0;i<len;++i)
	{
		for(j=0; j<nmicchannel; ++j)
		{
			fv=data[mic_channel[j]];
			wtk_strbuf_push(mic[j],(char *)&(fv),sizeof(float));
		}
		for(j=0; j<nspchannel; ++j)
		{
			fv=data[sp_channel[j]];
			wtk_strbuf_push(sp[j],(char *)&(fv),sizeof(float));
		}
		data+=channel;
	}
	length=mic[0]->pos/sizeof(float);
	while(length>=fsize)
	{
		for(i=0; i<nmicchannel; ++i)
		{
			fp1=(float *)mic[i]->data;
			if(notch_mem)
			{
				wtk_preemph_dc(fp1, notch_mem[i], fsize);
				memD[i]=wtk_preemph_asis(fp1, fsize, memD[i]);
			}
			frame_analysis(gainnet_bf4, rfft_in, analysis_mem[i], fft[i], fp1);
			for(k=0;k<nbin;++k)
			{
				fft[i][k].a*=fft_scale;
				fft[i][k].b*=fft_scale;
			}
		}
		for(i=0, j=nmicchannel; i<nspchannel; ++i, ++j)
		{
			fp2=(float *)sp[i]->data;
			if(notch_mem)
			{
				wtk_preemph_dc(fp2, notch_mem[j], fsize);
				memD[j]=wtk_preemph_asis(fp2, fsize, memD[j]);
			}
			frame_analysis(gainnet_bf4, rfft_in, analysis_mem_sp[i], fft_sp[i], fp2);
		}

		if(nspchannel>0){
			spenr=wtk_gainnet_bf4_sp_energy((float *)sp[0]->data, fsize);
		}else{
			spenr=0;
		}
		// static int cnt=0;

		// cnt++;
		if(spenr>spenr_thresh)
		{
			// if(gainnet_bf4->sp_sil==1)
			// {
			// 	printf("sp start %f %f %f\n", 1.0/16000*cnt*(nbin-1),spenr,spenr_thresh);
			// }
			gainnet_bf4->sp_sil=0;
			gainnet_bf4->sp_silcnt=spenr_cnt;
		}else if(gainnet_bf4->sp_sil==0)
		{
			gainnet_bf4->sp_silcnt-=1;
			if(gainnet_bf4->sp_silcnt<=0)
			{
				// printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
				gainnet_bf4->sp_sil=1;
			}
		}
		wtk_gainnet_bf4_feed_aec(gainnet_bf4, fft, fft_sp);
		wtk_gainnet_bf4_feed_bf(gainnet_bf4, ffts);

		micenr=wtk_gainnet_bf4_fft_energy(fft[0], nbin);
		if(micenr>micenr_thresh)
		{
			// if(gainnet_bf4->mic_sil==1)
			// {
			// 	printf("sp start %f %f %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
			// }
			gainnet_bf4->mic_sil=0;
			gainnet_bf4->mic_silcnt=micenr_cnt;
		}else if(gainnet_bf4->mic_sil==0)
		{
			gainnet_bf4->mic_silcnt-=1;
			if(gainnet_bf4->mic_silcnt<=0)
			{
				// printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
				gainnet_bf4->mic_sil=1;
			}
		}

		if(gainnet_bf4->maskssl2 && gainnet_bf4->ssl_enable)
		{
			wtk_maskssl2_feed_fft2(gainnet_bf4->maskssl2, fft, gainnet_bf4->gaec->gf, gainnet_bf4->mic_sil);
		}

		wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(float));
		wtk_strbufs_pop(sp, nspchannel, fsize*sizeof(float));
		length=mic[0]->pos/sizeof(float);
	}
}

void wtk_gainnet_bf4_ssl_delay_new(wtk_gainnet_bf4_t *gainnet_bf4)
{
	if(gainnet_bf4->cfg->use_maskssl2)
	{
		gainnet_bf4->maskssl2=wtk_maskssl2_new(&(gainnet_bf4->cfg->maskssl2));
        wtk_maskssl2_set_notify(gainnet_bf4->maskssl2, gainnet_bf4, (wtk_maskssl2_notify_f)wtk_gainnet_bf4_on_ssl2);
	}

	if(gainnet_bf4->maskssl2)
    {
        wtk_maskssl2_reset(gainnet_bf4->maskssl2);
    }
	gainnet_bf4->ssl_enable = 1;
}