#include "wtk_gainnet_denoise.h"
void wtk_gainnet_denoise_on_gainnet(wtk_gainnet_denoise_t *mdenosie2, float *gain, int len, int is_end);
void wtk_gainnet_denoise_on_gainnet2(wtk_gainnet_denoise_t *mdenosie2, float *gain, int len, int is_end);

wtk_gainnet_denoise_t *wtk_gainnet_denoise_new(wtk_gainnet_denoise_cfg_t *cfg)
{
    wtk_gainnet_denoise_t *gdenoise;

    gdenoise=(wtk_gainnet_denoise_t *)wtk_malloc(sizeof(wtk_gainnet_denoise_t));
    gdenoise->cfg=cfg;
    gdenoise->ths_tr=NULL;
    gdenoise->notify_tr=NULL;
    gdenoise->ths=NULL;
    gdenoise->notify=NULL;
    gdenoise->ths2 = NULL;
    gdenoise->notify2 = NULL;

    gdenoise->nbin=cfg->wins/2+1;

    gdenoise->input=wtk_strbuf_new(1024, 4);

    gdenoise->rfft=wtk_drft_new(cfg->wins);
    gdenoise->rfft_in=(float*)wtk_malloc(sizeof(float)*(cfg->wins));
    gdenoise->fft=(wtk_complex_t*)wtk_malloc(gdenoise->nbin*sizeof(wtk_complex_t)); 
    gdenoise->window=wtk_malloc(sizeof(float)*cfg->wins);
    gdenoise->synthesis_window=wtk_malloc(sizeof(float)*cfg->wins);
    gdenoise->analysis_mem=wtk_malloc(sizeof(float)*(gdenoise->nbin-1));
    gdenoise->synthesis_mem=wtk_malloc(sizeof(float)*(gdenoise->nbin-1));

    gdenoise->bank_mic=wtk_bankfeat_new(&(cfg->bankfeat));

    gdenoise->lastg=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
    gdenoise->g=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
    gdenoise->gf=wtk_malloc(sizeof(float)*gdenoise->nbin);

    gdenoise->g2=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
    gdenoise->gf2=wtk_malloc(sizeof(float)*gdenoise->nbin);

    gdenoise->gainnet=NULL;
    gdenoise->gainnet3=NULL;
    if(cfg->use_gainnet3)
    {
        gdenoise->gainnet3=wtk_gainnet3_new(cfg->gainnet3);
        wtk_gainnet3_set_notify(gdenoise->gainnet3, gdenoise, (wtk_gainnet3_notify_f)wtk_gainnet_denoise_on_gainnet);
        wtk_gainnet3_set_notify2(gdenoise->gainnet3, gdenoise, (wtk_gainnet3_notify_f2)wtk_gainnet_denoise_on_gainnet2);
    }else
    {
        gdenoise->gainnet=wtk_gainnet7_new(cfg->gainnet);
        wtk_gainnet7_set_notify(gdenoise->gainnet, gdenoise, (wtk_gainnet7_notify_f)wtk_gainnet_denoise_on_gainnet);
        wtk_gainnet7_set_notify2(gdenoise->gainnet, gdenoise, (wtk_gainnet7_notify_f2)wtk_gainnet_denoise_on_gainnet2);
    }

    gdenoise->out=wtk_malloc(sizeof(float)*(gdenoise->nbin-1));

    gdenoise->qmmse=NULL;
    if(cfg->use_qmmse)
    {
        gdenoise->qmmse=wtk_qmmse_new(&(cfg->qmmse));
    }

    wtk_gainnet_denoise_reset(gdenoise);

    return gdenoise;
}

void wtk_gainnet_denoise_delete(wtk_gainnet_denoise_t *gdenoise)
{
    wtk_strbuf_delete(gdenoise->input);

    wtk_free(gdenoise->rfft_in);
    wtk_free(gdenoise->fft);
    wtk_free(gdenoise->window);
    wtk_free(gdenoise->synthesis_window);
    wtk_drft_delete(gdenoise->rfft);
    wtk_free(gdenoise->analysis_mem);
    wtk_free(gdenoise->synthesis_mem);

    wtk_bankfeat_delete(gdenoise->bank_mic);

    wtk_free(gdenoise->lastg);
    wtk_free(gdenoise->g);
    wtk_free(gdenoise->gf);
    wtk_free(gdenoise->g2);
    wtk_free(gdenoise->gf2);

    wtk_free(gdenoise->out);

    if(gdenoise->qmmse)
    {
        wtk_qmmse_delete(gdenoise->qmmse);
    }
    if(gdenoise->gainnet3)
    {
        wtk_gainnet3_delete(gdenoise->gainnet3);
    }else
    {
        wtk_gainnet7_delete(gdenoise->gainnet);
    }
    wtk_free(gdenoise);
}

void wtk_gainnet_denoise_reset(wtk_gainnet_denoise_t *gdenoise)
{
    int i;
    int wins=gdenoise->cfg->wins;

    for (i=0;i<gdenoise->cfg->wins;++i)
    {
        gdenoise->window[i] = sin((0.5+i)*PI/(gdenoise->cfg->wins));
    }
    wtk_drft_init_synthesis_window(gdenoise->synthesis_window, gdenoise->window, wins);

    wtk_strbuf_reset(gdenoise->input);

    memset(gdenoise->fft, 0, sizeof(wtk_complex_t)*(gdenoise->nbin));
    memset(gdenoise->analysis_mem, 0, sizeof(float)*(gdenoise->nbin-1));
    memset(gdenoise->synthesis_mem, 0, sizeof(float)*(gdenoise->nbin-1));

    wtk_bankfeat_reset(gdenoise->bank_mic);

    memset(gdenoise->lastg, 0, sizeof(float)*gdenoise->cfg->bankfeat.nb_bands);
    memset(gdenoise->g, 0, sizeof(float)*gdenoise->cfg->bankfeat.nb_bands);
    memset(gdenoise->gf, 0, sizeof(float)*gdenoise->nbin);
    memset(gdenoise->g2, 0, sizeof(float)*gdenoise->cfg->bankfeat.nb_bands);
    memset(gdenoise->gf2, 0, sizeof(float)*gdenoise->nbin);

    memset(gdenoise->out, 0, sizeof(float)*(gdenoise->nbin-1));

    if(gdenoise->qmmse)
    {
        wtk_qmmse_reset(gdenoise->qmmse);
    }
    if(gdenoise->gainnet3)
    {
        wtk_gainnet3_reset(gdenoise->gainnet3);
    }else
    {
        wtk_gainnet7_reset(gdenoise->gainnet);
    }
}

void wtk_gainnet_denoise_set_notify(wtk_gainnet_denoise_t *gdenoise,void *ths,wtk_gainnet_denoise_notify_f notify)
{
    gdenoise->ths=ths;
    gdenoise->notify=notify;
}

void wtk_gainnet_denoise_set_notify_tr(wtk_gainnet_denoise_t *gdenoise,void *ths,wtk_gainnet_denoise_notify_trfeat_f notify)
{
    gdenoise->notify_tr=notify;
    gdenoise->ths_tr=ths;
}

void wtk_gainnet_denoise_on_gainnet(wtk_gainnet_denoise_t *mdenosie2, float *gain, int len, int is_end)
{
    memcpy(mdenosie2->g, gain, sizeof(float)*len);
}

void wtk_gainnet_denoise_on_gainnet2(wtk_gainnet_denoise_t *mdenosie2, float *gain, int len, int is_end)
{
    memcpy(mdenosie2->g2, gain, sizeof(float) * len);
}

void wtk_gainnet_denoise_feed(wtk_gainnet_denoise_t *gdenoise,short *data,int len,int is_end)
{
    int i, length, wins=gdenoise->cfg->wins;
    int fsize=wins/2;
    wtk_strbuf_t *input=gdenoise->input;
    int nbin=gdenoise->nbin;
    wtk_complex_t *fft=gdenoise->fft;
    wtk_drft_t *rfft=gdenoise->rfft;
	float *rfft_in=gdenoise->rfft_in;
    float *analysis_mem=gdenoise->analysis_mem,	*synthesis_mem=gdenoise->synthesis_mem;
	float *window=gdenoise->window, *synthesis_window=gdenoise->synthesis_window;
    wtk_gainnet7_t *gainnet=gdenoise->gainnet;
    wtk_gainnet3_t *gainnet3=gdenoise->gainnet3;
    wtk_bankfeat_t *bank_mic=gdenoise->bank_mic;
	int nb_bands=bank_mic->cfg->nb_bands;
	int nb_features=bank_mic->cfg->nb_features;
    wtk_qmmse_t *qmmse=gdenoise->qmmse;
    float agc_a = gdenoise->cfg->agc_a;
    float agc_b = gdenoise->cfg->agc_b;
    float *g=gdenoise->g, *gf=gdenoise->gf, *lastg=gdenoise->lastg, *g2=gdenoise->g2, *gf2=gdenoise->gf2;
    float gbias=gdenoise->cfg->gbias, gftmp;
    float ralpha=gdenoise->cfg->ralpha;
    float *qmmse_gain;
    short *pv;
    float fv, *fp1, *out=gdenoise->out;

    for(i=0;i<len;++i)
	{
        fv=data[i];
        wtk_strbuf_push(input, (char *)&(fv),sizeof(float));
	}
	length=input->pos/sizeof(float);
	while(length>=fsize)
    {
        fp1=(float *)input->data;
        wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem, fft, fp1, wins, window);
        wtk_bankfeat_flush_frame_features(bank_mic, fft);	
        if(qmmse)
        {
            wtk_qmmse_flush_denoise_mask(qmmse, fft);
        }
        if (!bank_mic->silence)
        {
            if(gainnet)
            {
                wtk_gainnet7_feed(gainnet, bank_mic->features, nb_features, 0);   
            }else
            {
                wtk_gainnet3_feed(gainnet3, bank_mic->features, nb_features, 0);   
            }
            for (i=0;i<nb_bands;++i)
            {
                g[i] = max(g[i], ralpha*lastg[i]);
                lastg[i] = g[i];
            }
            wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf, g);
            if((gainnet && gainnet->cfg->use_agc) || (gainnet3 && gainnet3->cfg->use_agc))
            {
                for (i = 0; i < nb_bands; ++i) {
                    g2[i] = max(0.001f, g2[i]);
                    g2[i] = min(0.999f, g2[i]);
                    g2[i] = -1 / agc_a * (logf(1 / g2[i] - 1) - agc_b);
                    if (g2[i] < 0) {
                        g2[i] = g[i];
                    };
                }
                wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf2, g2);
            }
            if (qmmse && ((gainnet && gainnet->cfg->use_agc) ||
                          (gainnet3 && gainnet3->cfg->use_agc))) {
                qmmse_gain=qmmse->gain;
                for (i=1; i<nbin-1; ++i)
                {
                    if (gf[i] >= gf2[i]) {
                        gf[i] = gf2[i];
                        if (gf[i] > qmmse_gain[i]) {
                            gf[i] = qmmse_gain[i];
                        }
                        gf2[i] = 1;
                    } else {
                        gf2[i] = gf2[i] / (gf[i] + 1e-6);
                        if (gf[i] > qmmse_gain[i]) {
                            gf2[i] *= qmmse_gain[i] / gf[i];
                            gf[i] = qmmse_gain[i];
                        }
                        gf2[i] = max(1, gf2[i]);
                    }
                }
            } else if (qmmse) {
                qmmse_gain = qmmse->gain;
                for (i = 1; i < nbin - 1; ++i) {
                    if (gf[i] > qmmse_gain[i]) {
                        gf[i] = qmmse_gain[i];
                    }
                }
            } else if ((gainnet && gainnet->cfg->use_agc) ||
                       (gainnet3 && gainnet3->cfg->use_agc)) {
                for (i = 1; i < nbin - 1; ++i) {
                    if (gf[i] >= gf2[i]) {
                        gf[i] = gf2[i];
                        gf2[i] = 1;
                    } else {
                        gf2[i] = gf2[i] / (gf[i] + 1e-6);
                        gf2[i] = max(1, gf2[i]);
                    }
                }
            }
            if(gbias>0)
            {
                for (i=1; i<nbin-1; ++i)
                {
                    gf[i]=min(gf[i]+gbias,1);
                }
            }
            if((gainnet && gainnet->cfg->use_agc) || (gainnet3 && gainnet3->cfg->use_agc))
            {
    			gftmp=wtk_float_abs_mean(gf2+1, nbin-2);
                for (i=1;i<nbin-1;i++)
                {
                    gf[i]*=gftmp;
                }
            }
            fft[0].a=fft[0].b=fft[nbin-1].a=fft[nbin-1].b=0;
            for (i=1;i<nbin-1;i++)
            {
                fft[i].a *= gf[i];
                fft[i].b *= gf[i];
            }
        }
        wtk_strbuf_pop(input, NULL, fsize*sizeof(float));
        length=input->pos/sizeof(float);

	    wtk_drft_frame_synthesis(rfft, rfft_in, synthesis_mem, fft, out, wins, synthesis_window);

        pv=(short *)out;
        for(i=0;i<fsize;++i)
        {
            pv[i]=floor(0.5+out[i]);
        }
        if(gdenoise->notify)
        {
            gdenoise->notify(gdenoise->ths, pv, fsize, 0);
        }
    }
    if(is_end)
    {
        pv=(short *)input->data;
        len=input->pos/sizeof(short);
        if(gdenoise->notify)
        {
            gdenoise->notify(gdenoise->ths, pv, len, 1);
        }
    }
}

wtk_gainnet_denoise_t *
wtk_gainnet_denoise_new2(wtk_gainnet_denoise_cfg_t *cfg) {
    wtk_gainnet_denoise_t *gdenoise;

    gdenoise =
        (wtk_gainnet_denoise_t *)wtk_malloc(sizeof(wtk_gainnet_denoise_t));
    gdenoise->cfg = cfg;
    gdenoise->ths_tr = NULL;
    gdenoise->notify_tr = NULL;
    gdenoise->ths = NULL;
    gdenoise->notify = NULL;
    gdenoise->ths2 = NULL;
    gdenoise->notify2 = NULL;

    gdenoise->nbin = cfg->wins / 2 + 1;

    gdenoise->input = NULL;
    gdenoise->rfft = NULL;
    gdenoise->rfft_in = NULL;
    gdenoise->window = NULL;
    gdenoise->synthesis_window = NULL;
    gdenoise->analysis_mem = NULL;
    gdenoise->synthesis_mem = NULL;
    gdenoise->out = NULL;

    gdenoise->fft =
        (wtk_complex_t *)wtk_malloc(gdenoise->nbin * sizeof(wtk_complex_t));
    gdenoise->bank_mic = wtk_bankfeat_new(&(cfg->bankfeat));

    gdenoise->lastg = wtk_malloc(sizeof(float) * cfg->bankfeat.nb_bands);
    gdenoise->g = wtk_malloc(sizeof(float) * cfg->bankfeat.nb_bands);
    gdenoise->gf = wtk_malloc(sizeof(float) * gdenoise->nbin);

    gdenoise->g2 = wtk_malloc(sizeof(float) * cfg->bankfeat.nb_bands);
    gdenoise->gf2 = wtk_malloc(sizeof(float) * gdenoise->nbin);

    gdenoise->gainnet = NULL;
    gdenoise->gainnet3 = NULL;
    if (cfg->use_gainnet3) {
        gdenoise->gainnet3 = wtk_gainnet3_new(cfg->gainnet3);
        wtk_gainnet3_set_notify(
            gdenoise->gainnet3, gdenoise,
            (wtk_gainnet3_notify_f)wtk_gainnet_denoise_on_gainnet);
        wtk_gainnet3_set_notify2(
            gdenoise->gainnet3, gdenoise,
            (wtk_gainnet3_notify_f2)wtk_gainnet_denoise_on_gainnet2);
    } else {
        gdenoise->gainnet = wtk_gainnet7_new(cfg->gainnet);
        wtk_gainnet7_set_notify(
            gdenoise->gainnet, gdenoise,
            (wtk_gainnet7_notify_f)wtk_gainnet_denoise_on_gainnet);
        wtk_gainnet7_set_notify2(
            gdenoise->gainnet, gdenoise,
            (wtk_gainnet7_notify_f2)wtk_gainnet_denoise_on_gainnet2);
    }

    gdenoise->qmmse = NULL;
    if (cfg->use_qmmse) {
        gdenoise->qmmse = wtk_qmmse_new(&(cfg->qmmse));
    }

    gdenoise->pad = (float *)wtk_malloc(cfg->wins * sizeof(float));

    wtk_gainnet_denoise_reset2(gdenoise);

    return gdenoise;
}

void wtk_gainnet_denoise_delete2(wtk_gainnet_denoise_t *gdenoise) {
    wtk_free(gdenoise->fft);

    wtk_bankfeat_delete(gdenoise->bank_mic);

    wtk_free(gdenoise->lastg);
    wtk_free(gdenoise->g);
    wtk_free(gdenoise->gf);
    wtk_free(gdenoise->g2);
    wtk_free(gdenoise->gf2);

    wtk_free(gdenoise->pad);

    if (gdenoise->qmmse) {
        wtk_qmmse_delete(gdenoise->qmmse);
    }
    if (gdenoise->gainnet3) {
        wtk_gainnet3_delete(gdenoise->gainnet3);
    } else {
        wtk_gainnet7_delete(gdenoise->gainnet);
    }
    wtk_free(gdenoise);
}

void wtk_gainnet_denoise_reset2(wtk_gainnet_denoise_t *gdenoise) {
    memset(gdenoise->fft, 0, sizeof(wtk_complex_t) * (gdenoise->nbin));

    wtk_bankfeat_reset(gdenoise->bank_mic);

    memset(gdenoise->lastg, 0,
           sizeof(float) * gdenoise->cfg->bankfeat.nb_bands);
    memset(gdenoise->g, 0, sizeof(float) * gdenoise->cfg->bankfeat.nb_bands);
    memset(gdenoise->gf, 0, sizeof(float) * gdenoise->nbin);
    memset(gdenoise->g2, 0, sizeof(float) * gdenoise->cfg->bankfeat.nb_bands);
    memset(gdenoise->gf2, 0, sizeof(float) * gdenoise->nbin);

    memset(gdenoise->pad, 0, sizeof(float) * gdenoise->cfg->wins);

    if (gdenoise->qmmse) {
        wtk_qmmse_reset(gdenoise->qmmse);
    }
    if (gdenoise->gainnet3) {
        wtk_gainnet3_reset(gdenoise->gainnet3);
    } else {
        wtk_gainnet7_reset(gdenoise->gainnet);
    }
}

void wtk_gainnet_denoise_set_notify2(wtk_gainnet_denoise_t *gdenoise, void *ths,
                                     wtk_gainnet_denoise_notify_f2 notify) {
    gdenoise->ths2 = ths;
    gdenoise->notify2 = notify;
}

void wtk_gainnet_denoise_feed2(wtk_gainnet_denoise_t *gdenoise,
                               wtk_complex_t *infft, int is_end) {
    int i;
    int nbin = gdenoise->nbin;
    wtk_complex_t *fft = gdenoise->fft;
    wtk_gainnet7_t *gainnet = gdenoise->gainnet;
    wtk_gainnet3_t *gainnet3 = gdenoise->gainnet3;
    wtk_bankfeat_t *bank_mic = gdenoise->bank_mic;
    int nb_bands = bank_mic->cfg->nb_bands;
    int nb_features = bank_mic->cfg->nb_features;
    wtk_qmmse_t *qmmse = gdenoise->qmmse;
    float agc_a = gdenoise->cfg->agc_a;
    float agc_b = gdenoise->cfg->agc_b;
    float *g = gdenoise->g, *gf = gdenoise->gf, *lastg = gdenoise->lastg,
          *g2 = gdenoise->g2, *gf2 = gdenoise->gf2;
    float gbias = gdenoise->cfg->gbias, gftmp;
    float ralpha = gdenoise->cfg->ralpha;
    float *qmmse_gain;

    if (!infft) {
        if (gdenoise->notify2) {
            gdenoise->notify2(gdenoise->ths2, NULL, 1);
        }
    }
    memcpy(fft, infft, sizeof(wtk_complex_t) * nbin);
    wtk_bankfeat_flush_frame_features(bank_mic, fft);
    if (qmmse) {
        wtk_qmmse_flush_denoise_mask(qmmse, fft);
    }
    if (!bank_mic->silence) {
        if (gainnet) {
            wtk_gainnet7_feed(gainnet, bank_mic->features, nb_features, 0);
        } else {
            wtk_gainnet3_feed(gainnet3, bank_mic->features, nb_features, 0);
        }
        for (i = 0; i < nb_bands; ++i) {
            g[i] = max(g[i], ralpha * lastg[i]);
            lastg[i] = g[i];
        }
        wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf, g);
        if ((gainnet && gainnet->cfg->use_agc) ||
            (gainnet3 && gainnet3->cfg->use_agc)) {
            for (i = 0; i < nb_bands; ++i) {
                g2[i] = max(0.001f, g2[i]);
                g2[i] = min(0.999f, g2[i]);
                g2[i] = -1 / agc_a * (logf(1 / g2[i] - 1) - agc_b);
                if (g2[i] < 0) {
                    g2[i] = g[i];
                };
            }
            wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf2, g2);
        }
        if (qmmse && ((gainnet && gainnet->cfg->use_agc) ||
                      (gainnet3 && gainnet3->cfg->use_agc))) {
            qmmse_gain = qmmse->gain;
            for (i = 1; i < nbin - 1; ++i) {
                if (gf[i] >= gf2[i]) {
                    gf[i] = gf2[i];
                    if (gf[i] > qmmse_gain[i]) {
                        gf[i] = qmmse_gain[i];
                    }
                    gf2[i] = 1;
                } else {
                    gf2[i] = gf2[i] / (gf[i] + 1e-6);
                    if (gf[i] > qmmse_gain[i]) {
                        gf2[i] *= qmmse_gain[i] / gf[i];
                        gf[i] = qmmse_gain[i];
                    }
                    gf2[i] = max(1, gf2[i]);
                }
            }
        } else if (qmmse) {
            qmmse_gain = qmmse->gain;
            for (i = 1; i < nbin - 1; ++i) {
                if (gf[i] > qmmse_gain[i]) {
                    gf[i] = qmmse_gain[i];
                }
            }
        } else if ((gainnet && gainnet->cfg->use_agc) ||
                   (gainnet3 && gainnet3->cfg->use_agc)) {
            for (i = 1; i < nbin - 1; ++i) {
                if (gf[i] >= gf2[i]) {
                    gf[i] = gf2[i];
                    gf2[i] = 1;
                } else {
                    gf2[i] = gf2[i] / (gf[i] + 1e-6);
                    gf2[i] = max(1, gf2[i]);
                }
            }
        }
        if (gbias > 0) {
            for (i = 1; i < nbin - 1; ++i) {
                gf[i] = min(gf[i] + gbias, 1);
            }
        }
        if ((gainnet && gainnet->cfg->use_agc) ||
            (gainnet3 && gainnet3->cfg->use_agc)) {
            gftmp = wtk_float_abs_mean(gf2 + 1, nbin - 2);
            for (i = 1; i < nbin - 1; i++) {
                gf[i] *= gftmp;
            }
        }
        fft[0].a = fft[0].b = fft[nbin - 1].a = fft[nbin - 1].b = 0;
        for (i = 1; i < nbin - 1; i++) {
            fft[i].a *= gf[i];
            fft[i].b *= gf[i];
        }
    }
    if (gdenoise->notify2) {
        gdenoise->notify2(gdenoise->ths2, fft, is_end);
    }
}

void wtk_gainnet_denoise_feed_train(wtk_gainnet_denoise_t *gdenoise,short **data,int len)
{
    short *sdata, *mdata;
    int wins=gdenoise->cfg->wins;
    int fsize=wins/2;
    int i;
    float E, vad;
    int vad_cnt=0;
    float xn[1024];
    float x[1024];
    float Es[512];
    float g[512];
    wtk_drft_t *rfft=gdenoise->rfft;
    float *rfft_in=gdenoise->rfft_in;
    wtk_complex_t *fft=gdenoise->fft;
    wtk_complex_t ffts[1024];
    wtk_bankfeat_t *bank_mic=gdenoise->bank_mic;
	int nb_bands=bank_mic->cfg->nb_bands;
	int nb_features=bank_mic->cfg->nb_features;
    float *Ex=bank_mic->Ex;
    float *analysis_mem=gdenoise->analysis_mem;
    float *window=gdenoise->window;
    float analysis_mem_tr[1024]={0};
    int pos;

    pos=0;
    while((len-pos)>=fsize)
    {
        mdata=data[0]+pos;
        sdata=data[3]+pos;
        for(i=0;i<fsize;++i)
        {
            xn[i]=mdata[i];
            x[i]=sdata[i];
        }

        E=0;
        for(i=0;i<fsize;++i)
        {
            E+=x[i]*x[i];
        }

        if (E > 1e9f)
        {
            vad_cnt=0;
        }else if (E > 1e8f)
        {
            vad_cnt -= 5;
        }else if (E > 1e7f)
        {
            vad_cnt++;
        }else
        {
            vad_cnt+=2;
        }
        if (vad_cnt < 0) vad_cnt = 0;
        if (vad_cnt > 15) vad_cnt = 15;

        if (vad_cnt >= 10) vad = 0;
        else if (vad_cnt > 0) vad = 0.5f;
        else vad = 1.f;

        // printf("[%f %f]=%f  %f\n",pos/16.0, (pos+fsize)/16.0,vad,E);
        wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem_tr, ffts, x, wins, window);
        wtk_bankfeat_compute_band_energy(bank_mic, Es, ffts);

        wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem, fft, xn, wins, window);
        wtk_bankfeat_flush_frame_features(bank_mic, fft);	
        for (i=0;i<nb_bands;i++)
        {
            if(bank_mic->silence || Ex[i] < 5e-2 )
            {
                g[i]=0;
            }else
            {
                g[i] = sqrt((Es[i])/(Ex[i]));
            }
            if (g[i] > 1) g[i] = 1;
        }

        //    for (i=0;i<nb_bands;i++)
        //    {
        //      g[i]=1/(1+expf(-1*(0.138*g[i]-6.9)));
        //    }

        if(gdenoise->notify_tr)
        {
            gdenoise->notify_tr(gdenoise->ths_tr, bank_mic->features, nb_features, g, nb_bands, vad);
        }
        pos+=fsize;
    }
    //  printf("%f %f %f\n",g_min,g_max,g_mean/(nb_bands*cnt));
}



