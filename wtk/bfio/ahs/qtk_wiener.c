#include "qtk_wiener.h"

qtk_ahs_wiener_t *qtk_wiener_new(wtk_covm_cfg_t* covm, wtk_bf_cfg_t* bf, int use_fftsbf, int nbin, int win_sz, float thresh){
    qtk_ahs_wiener_t * wiener = (qtk_ahs_wiener_t*)wtk_malloc(sizeof(qtk_ahs_wiener_t));

    wiener->covm = wtk_covm_new(covm, nbin, 1);
    wiener->bf = wtk_bf_new(bf, win_sz);
    wiener->nbin = nbin;
    wiener->use_fftsbf = use_fftsbf;
    wiener->fftx = (wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t)*nbin);
    wiener->gf = (float*)wtk_malloc(sizeof(float)*nbin);
    wiener->scale = 1.0;
    wiener->thresh = thresh;
    wtk_bf_init_w(wiener->bf);
    return wiener;
}

void qtk_wiener_delete(qtk_ahs_wiener_t *wiener){
    wtk_free(wiener->fftx);
    wtk_free(wiener->gf);
    wtk_covm_delete(wiener->covm);
    wtk_bf_delete(wiener->bf);
    wtk_free(wiener);
}

void qtk_wiener_reset(qtk_ahs_wiener_t *wiener){
    wtk_covm_reset(wiener->covm);
    wtk_bf_reset(wiener->bf);
    wtk_bf_init_w(wiener->bf);
}

void qtk_wiener_feed(qtk_ahs_wiener_t *wiener,float *real, float *imag, float *mask_real, float *mask_imag){
    wtk_complex_t *fftx = wiener->fftx;
    int k,nbin = wiener->nbin,b;
    wtk_bf_t *bf = wiener->bf;
    wtk_covm_t *covm = wiener->covm;
    //wtk_gainnet_howls_hsra_t *hsra=gainnet_howls->hsra;//TODO mask
    float gf,*p,scale = 32768.0,thresh = wiener->thresh;
    wtk_complex_t fft2[2];
    wtk_complex_t ffts[2];
    wtk_complex_t ffty[2];
    int clip_s = wiener->clip_s;
    int clip_e = wiener->clip_e;
    //float agcaddg=gainnet_howls->cfg->agc_gainnet ?1.0:gainnet_howls->cfg->agcaddg;
    p = wiener->gf;
    for(k = 0; k < wiener->nbin; k++){
        if(mask_imag){
            gf = sqrt(mask_real[k] * mask_real[k] + mask_imag[k] * mask_imag[k]) * scale / thresh;
        }else{
            gf = mask_real[k] * scale / thresh;
        }
        *p = gf > 1 ? 1:gf;
        if(gf < 0){
            *p = 0.0;
        }
        p++;
    }

    fftx[0].a=fftx[0].b=0;
    fftx[nbin-1].a=fftx[nbin-1].b=0;

    for(k=clip_s+1; k<clip_e; ++k)
    {
        gf=wiener->gf[k];

        ffts[0].a=real[k]*gf* scale;
        ffts[0].b=imag[k]*gf* scale;

        ffty[0].a=real[k]*(1-gf)* scale;
        ffty[0].b=imag[k]*(1-gf)* scale;

        if(wiener->use_fftsbf)
        {
            fft2[0] = ffts[0];
        }else
        {
            fft2[0].a = real[k]* scale;
            fft2[0].b = imag[k]* scale;
        }

        b=0;
        b=wtk_covm_feed_fft3(covm, ffty, k, 1);
        if(b==1)
        {
            wtk_bf_update_ncov(bf, covm->ncov, k);
        }
        if(covm->scov)
        {
            b=wtk_covm_feed_fft3(covm, ffts, k, 0);
            if(b==1)
            {
                wtk_bf_update_scov(bf, covm->scov, k);
            }
        }
        if(b==1)
        {
            wtk_bf_update_w(bf, k);
        }
        wtk_bf_output_fft_k(bf, fft2, fftx+k, k);
        // fftx[k]=fft[0][k];
        //fftx[k].a*=agcaddg;
        //fftx[k].b*=agcaddg;
    }

    for(k=0; k<=clip_s; ++k)
    {
        fftx[k].a=fftx[k].b=0;
    }
    for(k=clip_e; k<nbin; ++k)
    {
        fftx[k].a=fftx[k].b=0;
    }
}