#include "wtk_qform2.h"

void wtk_qform2_on_stft2(wtk_qform2_t *qform2,wtk_stft2_msg_t *msg,int pos,int is_end);
void wtk_qform2_on_admm(wtk_qform2_t *qform2,wtk_stft2_msg_t *msg,wtk_stft2_msg_t *err_msg,wtk_stft2_msg_t *lsty_msg,int pos,int is_end);

wtk_qform2_t* wtk_qform2_new(wtk_qform2_cfg_t *cfg)
{
    wtk_qform2_t *qform2;
    int i;


    qform2=(wtk_qform2_t *)wtk_malloc(sizeof(wtk_qform2_t));
    qform2->cfg=cfg;
    qform2->ths=NULL;
    qform2->notify=NULL;

    qform2->stft2=wtk_stft2_new(&(cfg->stft2));
    wtk_stft2_set_notify(qform2->stft2,qform2,(wtk_stft2_notify_f)wtk_qform2_on_stft2);

    qform2->nbin=qform2->stft2->nbin;
    qform2->channel=qform2->stft2->cfg->channel;

    qform2->input=NULL;
    if(cfg->use_preemph)
    {
        qform2->input=wtk_strbufs_new(qform2->channel);
    }
    qform2->admm=NULL;
    if(cfg->use_admm2)
    {
        qform2->admm=wtk_admm2_new2(&(cfg->admm),qform2->stft2);
        wtk_admm2_set_notify2(qform2->admm, qform2, (wtk_admm2_notify_f2)wtk_qform2_on_admm);
    }
    // qform2->bf=wtk_bf_new(&(cfg->bf), qform2->stft2->cfg->win);

    qform2->ovec=wtk_complex_new_p3(qform2->channel, qform2->nbin, qform2->channel);

    qform2->rls3=NULL;
    qform2->nlms=NULL;
    qform2->xld_2=NULL;
    if(!cfg->use_nlms)
    {
        qform2->rls3 = (wtk_rls3_t *)wtk_malloc(sizeof(wtk_rls3_t));
        wtk_rls3_init(qform2->rls3, &(qform2->cfg->rls3), qform2->nbin);
        qform2->xld_2=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform2->nbin*(qform2->channel-1));
    }else
    {
        qform2->nlms=(wtk_nlms_t *)wtk_malloc(sizeof(wtk_nlms_t)*qform2->nbin);
        for(i=0; i<qform2->nbin; ++i)
        {
            wtk_nlms_init(qform2->nlms+i, &(qform2->cfg->nlms));
        }
    }
    qform2->xld=wtk_complex_new_p2(qform2->nbin, qform2->channel-1);

    qform2->fft=wtk_complex_new_p2(qform2->channel, qform2->nbin);

    qform2->pad=(float *)wtk_malloc(sizeof(float)*qform2->stft2->cfg->win);
    qform2->fout=(float *)wtk_malloc(sizeof(float)*qform2->stft2->cfg->win);

    qform2->Yf=(float *)wtk_malloc(sizeof(float)*qform2->nbin);
    qform2->out=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform2->nbin);
    
    qform2->Se=NULL;
    qform2->Sd=NULL;
    qform2->Sed=NULL;
    qform2->qmmse=NULL;
    if(cfg->use_post)
    {
        qform2->Se=(float *)wtk_malloc(sizeof(float)*qform2->nbin);
        qform2->Sd=(float *)wtk_malloc(sizeof(float)*qform2->nbin);
        qform2->Sed=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform2->nbin);
        qform2->qmmse=wtk_qmmse_new(&(cfg->qmmse));
    }
    
    wtk_qform2_reset(qform2);

    return qform2;
}

void wtk_qform2_delete(wtk_qform2_t *qform2)
{
    int nbin=qform2->nbin;
    int i;

    wtk_complex_delete_p3(qform2->ovec, qform2->channel,qform2->nbin);

    if(qform2->input)
    {
        wtk_strbufs_delete(qform2->input,qform2->channel);
    }
    wtk_stft2_delete(qform2->stft2);

    if(qform2->admm)
    {
        wtk_admm2_delete2(qform2->admm);
    }

    if(qform2->rls3)
    {
        wtk_rls3_clean(qform2->rls3);
        wtk_free(qform2->rls3);
        wtk_free(qform2->xld_2);
    }else if(qform2->nlms)
    {
        for(i=0; i<nbin; ++i)
        {
            wtk_nlms_clean(qform2->nlms+i);
        }
        wtk_free(qform2->nlms);
    }
    wtk_complex_delete_p2(qform2->xld, qform2->nbin);

    wtk_complex_delete_p2(qform2->fft, qform2->channel);

    wtk_free(qform2->Yf);
    wtk_free(qform2->out);
    if(qform2->qmmse)
    {
        wtk_free(qform2->Se);
        wtk_free(qform2->Sd);
        wtk_free(qform2->Sed);
        wtk_qmmse_delete(qform2->qmmse);
    }

    wtk_free(qform2->fout);
    wtk_free(qform2->pad);
    wtk_free(qform2);
}

void wtk_qform2_reset(wtk_qform2_t *qform2)
{
    int i;
    int channel=qform2->channel;
    int nbin=qform2->nbin;

    // wtk_bf_reset(qform2->bf);

    for(i=0;i<channel;++i)
    {
        memset(qform2->notch_mem[i],0,2*sizeof(float));
    }
	memset(qform2->memD,0,channel*sizeof(float));
    qform2->memX=0;

    qform2->theta=qform2->phi=0;
    wtk_stft2_reset(qform2->stft2);

    if(qform2->rls3)
    {
        wtk_rls3_reset(qform2->rls3, nbin);
        memset(qform2->xld_2, 0, sizeof(wtk_complex_t)*nbin*(channel-1));
    }else if(qform2->nlms)
    {
        for(i=0; i<nbin; ++i)
        {
            wtk_nlms_reset(qform2->nlms+i);
        }
    }
    wtk_complex_zero_p2(qform2->xld, qform2->nbin, qform2->channel-1);

    if(qform2->admm)
    {
        wtk_admm2_reset2(qform2->admm);
    }

    qform2->nframe=0;
    qform2->end_pos=0;

    if(qform2->qmmse)
    {
        memset(qform2->Se, 0, sizeof(float)*qform2->nbin);
        memset(qform2->Sd, 0, sizeof(float)*qform2->nbin);
        memset(qform2->Sed, 0, sizeof(wtk_complex_t)*qform2->nbin);

        wtk_qmmse_reset(qform2->qmmse);
    }

    memset(qform2->fout, 0, sizeof(float)*qform2->stft2->cfg->win);
    memset(qform2->pad, 0, sizeof(float)*qform2->stft2->cfg->win);
}

void wtk_qform2_set_notify(wtk_qform2_t *qform2,void *ths,wtk_qform2_notify_f notify)
{
    qform2->ths=ths;
    qform2->notify=notify;
}

void wtk_qform2_notify_data(wtk_qform2_t *qform2,float *data,int len,int is_end)
{
    short *pv=(short *)data;
    int i;

    if(qform2->input)
    {
        qform2->memX=wtk_preemph_asis2(data,len,qform2->memX);
    }
    for(i=0;i<len;++i)
    {
        if(fabs(data[i])<32000.0)
        {
            pv[i]=data[i];
        }else
        {
            if(data[i]>0)
            {
                pv[i]=32000;
            }else
            {
                pv[i]=-32000;
            }
        }
    }
    qform2->notify(qform2->ths,pv,len,is_end);
}


void wtk_qform2_feed_smsg2(wtk_qform2_t *qform2,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    wtk_complex_t ***ovec=qform2->ovec, **ovec2, *ovec3;
    wtk_complex_t **fft=qform2->fft, *fft1;
    int channel=qform2->channel;
    int nbin=qform2->nbin;
    int i, k, c;
    float ta,tb;
    wtk_complex_t **infft, *infft2;
    wtk_complex_t **xld=qform2->xld;
    wtk_complex_t *xld_2=qform2->xld_2;
    wtk_rls3_t *rls3=qform2->rls3;
    wtk_nlms_t *nlms=qform2->nlms;
    // int XN=qform2->cfg->XN;
    float *Se=qform2->Se;
    float *Sd=qform2->Sd;
    wtk_complex_t *Sed=qform2->Sed;
    wtk_complex_t *out=qform2->out, *E, *Y;
    float *Yf=qform2->Yf, ef, yf, leak;
    float coh_alpha=qform2->cfg->coh_alpha;
    int xld_2_pos=0;

    ++qform2->nframe;
    if(is_end)
    {
        qform2->end_pos=pos;
    }

    if(msg)
    {
        infft=msg->fft;
        for(c=0; c<channel; ++c)
        {
            ovec2=ovec[c];

            fft1=fft[c];
            for(k=0; k<nbin; ++k, ++fft1)
            {
                ovec3=ovec2[k];
                ta=tb=0;
                infft2=infft[k];
                for(i=0; i<channel; ++i, ++infft2, ++ovec3)
                {
                    ta+=ovec3->a*infft2->a + ovec3->b*infft2->b;
                    tb+=ovec3->a*infft2->b - ovec3->b*infft2->a;
                }
                fft1->a=ta;
                fft1->b=tb;

                if(c>0)
                {
                    xld[k][c-1].a=fft1->a;
                    xld[k][c-1].b=fft1->b;
                }
            }
        }

        if(qform2->rls3)
        {
            for(i=0;i<nbin;++i){
                for(c=0;c<channel-1;++c){
                    xld_2[xld_2_pos].a = xld[i][c].a;
                    xld_2[xld_2_pos].b = xld[i][c].b;
                    ++xld_2_pos;
                }
            }
            wtk_rls3_feed(rls3, fft[0], xld_2, nbin);
            for(i=0; i<nbin; ++i)
            {
                out[i]=rls3->out[i*rls3->cfg->channel];
                if(qform2->qmmse)
                {
                    E=rls3->out+i*rls3->cfg->channel;
                    Y=rls3->lsty+i*rls3->cfg->channel;
                    if(qform2->nframe<=1)
                    {
                        Se[i]+=E->a*E->a+E->b*E->b;
                        yf=Y->a*Y->a+Y->b*Y->b;
                        Sd[i]+=yf;
                        Sed[i].a+=Y->a*E->a+Y->b*E->b;
                        Sed[i].b+=-Y->a*E->b+Y->b*E->a;

                        leak=(Sed[i].a*Sed[i].a+Sed[i].b*Sed[i].b)/(max(Se[i],Sd[i])*Sd[i]+1e-9);
                        /*leak=max(0.005, leak);*/
                        Yf[i]=leak*yf;
                    }else
                    {
                        ef=E->a*E->a+E->b*E->b;
                        yf=Y->a*Y->a+Y->b*Y->b;

                        Se[i]=(1-coh_alpha)*Se[i]+coh_alpha*ef;
                        Sd[i]=(1-coh_alpha)*Sd[i]+coh_alpha*yf;
                        Sed[i].a=(1-coh_alpha)*Sed[i].a+coh_alpha*(Y->a*E->a+Y->b*E->b);
                        Sed[i].b=(1-coh_alpha)*Sed[i].b+coh_alpha*(-Y->a*E->b+Y->b*E->a);

                        leak=(Sed[i].a*Sed[i].a+Sed[i].b*Sed[i].b)/(max(Se[i],Sd[i])*Sd[i]+1e-9);
                        /*leak=max(0.005, leak);*/
                        Yf[i]=leak*yf;
                    }
                }
            }
        }else if(qform2->nlms)
        {
            for(i=0; i<nbin; ++i, ++nlms)
            {
                wtk_nlms_feed(nlms, &(fft[0][i]), xld[i]);
                out[i]=nlms->out[0];
                if(qform2->qmmse)
                {
                    Yf[i]=nlms->lsty_power[0];
                }
            }
        }

        if(qform2->qmmse)
        {
            wtk_qmmse_feed_echo_denoise(qform2->qmmse, out, Yf);
        }
    }
}


void wtk_qform2_on_stft2(wtk_qform2_t *qform2,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    int len;
    wtk_complex_t *out=qform2->out;
    if(qform2->admm)
    {
        wtk_admm2_feed_stftmsg(qform2->admm, msg, pos, is_end);
    }else
    {
        wtk_qform2_feed_smsg2(qform2,msg,pos,is_end);
    }
    len=wtk_stft2_output_ifft(qform2->stft2, out, qform2->fout, qform2->pad, qform2->end_pos, is_end);
    if(qform2->notify)
    {
        wtk_qform2_notify_data(qform2, qform2->fout, len, is_end);
    }
    wtk_stft2_push_msg(qform2->stft2, msg);

}

void wtk_qform2_on_admm(wtk_qform2_t *qform2,wtk_stft2_msg_t *omsg,wtk_stft2_msg_t *msg,wtk_stft2_msg_t *lmsg,int pos,int is_end)
{
    wtk_qform2_feed_smsg2(qform2,msg,pos,is_end);
}

void wtk_qform2_start(wtk_qform2_t *qform2,float theta,float phi)
{
    wtk_complex_t ***ovec=qform2->ovec;
    int channel=qform2->channel;
    int nbin=qform2->nbin;
    int i, k, c, j;
    wtk_complex_t ***ow;
    wtk_complex_t **ncov, *a, *a2, *rk, *b, *b2;
    float fa,f;
    wtk_dcomplex_t *tmp2;
    wtk_complex_t **alpha[512];
    int ret;

    ow=wtk_complex_new_p3(qform2->channel-1, qform2->nbin,  qform2->channel);
    ncov=wtk_complex_new_p2(qform2->nbin,  qform2->channel*qform2->channel);
    tmp2=(wtk_dcomplex_t *)wtk_calloc(qform2->channel*qform2->channel*2,sizeof(wtk_dcomplex_t));

    wtk_bf_update_ovec4(theta, phi, channel, nbin, qform2->cfg->rate, qform2->cfg->bf.speed, qform2->cfg->bf.mic_pos, ovec[0]);

    for(i=1; i<channel; ++i)
    {
        wtk_bf_update_ovec_orth2(ovec[0], ow[i-1], channel, nbin, i);
        alpha[i]=ow[i-1];
    }
    alpha[0]=ovec[0];
    wtk_bf_orth_ovec(alpha, channel, nbin, channel, 1);
    // wtk_complex_print(alpha[0][9], channel);
    // wtk_complex_print(alpha[1][9], channel);
    // wtk_complex_print(alpha[2][9], channel);
    // wtk_complex_print(alpha[3][9], channel);

    for(c=1; c<channel; ++c)
    {
        for(k=0; k<nbin; ++k)
        {
            a=ncov[k];
            a2=a;
            b=ovec[0][k];
            for(i=0; i<channel; ++i, ++b)
            {
                b2=ovec[0][k]+i;
                a+=i;
                for(j=i; j<channel; ++j, ++a,++b2)
                {
                    a->a=b->a*b2->a+b->b*b2->b;
                    if(j==i)
                    {
                        a->a+=1e-6;
                        a->b=0;
                    }else
                    {
                        a->b=-b->a*b2->b+b->b*b2->a;
                        a2[j*channel+i].a=a->a;
                        a2[j*channel+i].b=-a->b;
                    }
                }
            }

            a=ncov[k];
            rk=ow[c-1][k];
            ret=wtk_complex_guass_elimination_p1(a, rk, tmp2, channel, ovec[c][k]);
            if(ret!=0)
            {
                wtk_debug("inv error\n");
            }

            fa=0;
            a=ovec[c][k];
            for(i=0;i<channel;++i,++rk,++a)
            {
                fa+=rk->a*a->a+rk->b*a->b;
            }
            f=1.0/fa;
            a=ovec[c][k];
            for(i=0;i<channel;++i,++a)
            {
                a->a*=f;
                a->b*=f;
            }
        }
    }

    // wtk_bf_update_ovec(qform2->bf, theta, phi);
    // wtk_bf_init_w(qform2->bf);
    // wtk_complex_cpy_p2(ovec[0], qform2->bf->w, qform2->nbin, qform2->channel);

    wtk_complex_delete_p3(ow, qform2->channel-1, qform2->nbin);
    wtk_complex_delete_p2(ncov, qform2->nbin);
    wtk_free(tmp2);
}

void wtk_qform2_feed2(wtk_qform2_t *qform2,short **data,int len,int is_end)
{   
    int i,j;
    int channel=qform2->channel;
    float fv;
    float *fp[10];
    wtk_strbuf_t **input=qform2->input;

    for(i=0;i<channel;++i)
    {
        wtk_strbuf_reset(input[i]);
        for(j=0;j<len;++j)
        {
            fv=data[i][j];
            wtk_strbuf_push(input[i],(char *)(&fv),sizeof(float));
        }
        fp[i]=(float *)(input[i]->data);
        
        wtk_preemph_dc(fp[i],qform2->notch_mem[i],len);

        qform2->memD[i]=wtk_preemph_asis(fp[i],len,qform2->memD[i]);
    }

    wtk_stft2_feed_float(qform2->stft2,fp,len,is_end);
}

void wtk_qform2_feed(wtk_qform2_t *qform2,short **data,int len,int is_end)
{
#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,qform2->bf->channel);
		wtk_wavfile_open2(mic_log,"qform2");
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
    if(qform2->input)
    {
        wtk_qform2_feed2(qform2,data,len,is_end);
    }else
    {
        wtk_stft2_feed2(qform2->stft2,data,len,is_end);
    }
}

























wtk_qform2_t* wtk_qform2_new2(wtk_qform2_cfg_t *cfg, wtk_stft2_t *stft2)
{
    wtk_qform2_t *qform2;
    int i;


    qform2=(wtk_qform2_t *)wtk_malloc(sizeof(wtk_qform2_t));
    qform2->cfg=cfg;
    qform2->ths=NULL;
    qform2->notify=NULL;

    qform2->stft2=stft2;

    qform2->nbin=qform2->stft2->nbin;
    qform2->channel=qform2->stft2->cfg->channel;

    qform2->input=NULL;

    qform2->admm=NULL;
    if(cfg->use_admm2)
    {
        qform2->admm=wtk_admm2_new2(&(cfg->admm),qform2->stft2);
        wtk_admm2_set_notify2(qform2->admm, qform2, (wtk_admm2_notify_f2)wtk_qform2_on_admm);
    }

    qform2->ovec=wtk_complex_new_p3(qform2->channel, qform2->nbin, qform2->channel);

    qform2->rls3=NULL;
    qform2->nlms=NULL;
    qform2->xld_2=NULL;
    if(!cfg->use_nlms)
    {
        qform2->rls3 = (wtk_rls3_t *)wtk_malloc(sizeof(wtk_rls3_t));
        wtk_rls3_init(qform2->rls3, &(qform2->cfg->rls3), qform2->nbin);
        qform2->xld_2=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform2->nbin*(qform2->channel-1));
    }else
    {
        qform2->nlms=(wtk_nlms_t *)wtk_malloc(sizeof(wtk_nlms_t)*qform2->nbin);
        for(i=0; i<qform2->nbin; ++i)
        {
            wtk_nlms_init(qform2->nlms+i, &(qform2->cfg->nlms));
        }
    }
    qform2->xld=wtk_complex_new_p2(qform2->nbin, qform2->channel-1);

    qform2->fft=wtk_complex_new_p2(qform2->channel, qform2->nbin);

    qform2->pad=(float *)wtk_malloc(sizeof(float)*stft2->cfg->win);
    qform2->fout=(float *)wtk_malloc(sizeof(float)*stft2->cfg->win);

    qform2->Yf=(float *)wtk_malloc(sizeof(float)*qform2->nbin);
    qform2->out=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform2->nbin);
    
    qform2->Se=NULL;
    qform2->Sd=NULL;
    qform2->Sed=NULL;
    qform2->qmmse=NULL;
    if(cfg->use_post)
    {
        qform2->Se=(float *)wtk_malloc(sizeof(float)*qform2->nbin);
        qform2->Sd=(float *)wtk_malloc(sizeof(float)*qform2->nbin);
        qform2->Sed=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform2->nbin);
        qform2->qmmse=wtk_qmmse_new(&(cfg->qmmse));
    }
    
    wtk_qform2_reset2(qform2);

    return qform2;
}

void wtk_qform2_delete2(wtk_qform2_t *qform2)
{
    int nbin=qform2->nbin;
    int i;

    if(qform2->admm)
    {
        wtk_admm2_delete2(qform2->admm);
    }

    wtk_complex_delete_p3(qform2->ovec, qform2->channel,qform2->nbin);

    if(qform2->rls3)
    {
        wtk_rls3_clean(qform2->rls3);
        wtk_free(qform2->xld_2);
        wtk_free(qform2->rls3);
    }else if(qform2->nlms)
    {
        for(i=0; i<nbin; ++i)
        {
            wtk_nlms_clean(qform2->nlms+i);
        }
        wtk_free(qform2->nlms);
    }
    wtk_complex_delete_p2(qform2->xld, qform2->nbin);

    wtk_complex_delete_p2(qform2->fft, qform2->channel);

    wtk_free(qform2->Yf);
    wtk_free(qform2->out);
    if(qform2->qmmse)
    {
        wtk_free(qform2->Se);
        wtk_free(qform2->Sd);
        wtk_free(qform2->Sed);
        wtk_qmmse_delete(qform2->qmmse);
    }

    wtk_free(qform2->fout);
    wtk_free(qform2->pad);
    wtk_free(qform2);
}

void wtk_qform2_reset2(wtk_qform2_t *qform2)
{
    int i;
    int nbin=qform2->nbin;

    qform2->theta=qform2->phi=0;

    if(qform2->admm)
    {
        wtk_admm2_reset2(qform2->admm);
    }

    if(qform2->rls3)
    {
        wtk_rls3_reset(qform2->rls3, nbin);
        memset(qform2->xld_2, 0, sizeof(wtk_complex_t)*nbin*(qform2->channel-1));
    }else if(qform2->nlms)
    {
        for(i=0; i<nbin; ++i)
        {
            wtk_nlms_reset(qform2->nlms+i);
        }
    }
    wtk_complex_zero_p2(qform2->xld, qform2->nbin, qform2->channel-1);

    qform2->nframe=0;
    qform2->end_pos=0;

    if(qform2->qmmse)
    {
        memset(qform2->Se, 0, sizeof(float)*qform2->nbin);
        memset(qform2->Sd, 0, sizeof(float)*qform2->nbin);
        memset(qform2->Sed, 0, sizeof(wtk_complex_t)*qform2->nbin);

        wtk_qmmse_reset(qform2->qmmse);
    }

    memset(qform2->fout, 0, sizeof(float)*qform2->stft2->cfg->win);
    memset(qform2->pad, 0, sizeof(float)*qform2->stft2->cfg->win);
}

void wtk_qform2_feed_smsg(wtk_qform2_t *qform2,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    if(qform2->admm)
    {
        wtk_admm2_feed_stftmsg(qform2->admm, msg, pos, is_end);
    }else
    {
        wtk_qform2_feed_smsg2(qform2,msg, pos, is_end);
    }
}

void wtk_qform2_start2(wtk_qform2_t *qform2,float theta,float phi)
{
    wtk_qform2_start(qform2, theta, phi);
}


