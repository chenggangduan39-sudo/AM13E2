#include "wtk_wpd.h" 

wtk_wpd_t* wtk_wpd_new(wtk_wpd_cfg_t *cfg, int nbin)
{
    wtk_wpd_t *wpd;

    wpd=(wtk_wpd_t *)wtk_malloc(sizeof(wtk_wpd_t));
    wpd->cfg=cfg;
    wpd->nbin=nbin;
    wpd->channel=cfg->nmic;
    wpd->nl=cfg->L*wpd->channel;

	wpd->norig=wtk_complex_new_p2(wpd->nbin, wpd->nl);
    wpd->norigs=wtk_complex_new_p2(wpd->nbin, wpd->nl);
    wpd->masks=wtk_float_new_p2(wpd->nbin, wpd->nl);
    wpd->maskn=wtk_float_new_p2(wpd->nbin, wpd->nl);

    wpd->covm=wtk_covm2_new(&(cfg->covm2), nbin, wpd->nl);

    wpd->tmp_cov=(wtk_complex_t *)wtk_calloc(wpd->nl*wpd->nl,sizeof(wtk_complex_t));
	wpd->tmp=(wtk_dcomplex_t *)wtk_calloc(wpd->nl*wpd->nl*2,sizeof(wtk_dcomplex_t));

    wpd->bfw=(wtk_complex_t*)wtk_malloc(wpd->nl*sizeof(wtk_complex_t));

    wtk_wpd_reset(wpd);

    return wpd;
}

void wtk_wpd_reset(wtk_wpd_t *wpd)
{
    wpd->nframe=0;
 
	wtk_complex_zero_p2(wpd->norig, wpd->nbin, wpd->nl);
	wtk_complex_zero_p2(wpd->norigs, wpd->nbin, wpd->nl);
    wtk_float_zero_p2(wpd->masks, wpd->nbin, wpd->nl);
	wtk_float_zero_p2(wpd->maskn, wpd->nbin, wpd->nl);
    memset(wpd->bfw, 0, sizeof(wtk_complex_t)*wpd->nl);
    wtk_covm2_reset(wpd->covm);
}

void wtk_wpd_start(wtk_wpd_t *wpd, int theta, int phi)
{

}

void wtk_wpd_delete(wtk_wpd_t *wpd)
{
	wtk_complex_delete_p2(wpd->norig, wpd->nbin);
	wtk_complex_delete_p2(wpd->norigs, wpd->nbin);
    wtk_float_delete_p2(wpd->masks, wpd->nbin);
	wtk_float_delete_p2(wpd->maskn, wpd->nbin);
    wtk_covm2_delete(wpd->covm);
    wtk_free(wpd->tmp);
    wtk_free(wpd->tmp_cov);
    wtk_free(wpd->bfw);
    wtk_free(wpd);
}

void wtk_wpd_feed_k(wtk_wpd_t *wpd,wtk_complex_t *fft, wtk_complex_t *ffts, float *mask, wtk_complex_t *outfft, int k)
{
    int channel=wpd->channel;
    int i, ret;
	float w[64];
    float sigma=wpd->cfg->sigma;
	wtk_complex_t *norig, *norigtmp;
    wtk_complex_t *norigs;
    float *masks, *maskn;
    float fa,fb,ff;
    wtk_complex_t *bfw=wpd->bfw;
    int L=wpd->cfg->L;
    int nl=wpd->nl;
    // float p=wpd->cfg->p;
    wtk_covm2_t *covm=wpd->covm;
    wtk_dcomplex_t *tmp=wpd->tmp;
    wtk_complex_t *tmp_cov=wpd->tmp_cov,  *tmp_cov1;

    norig=wpd->norig[k];
    norigs=wpd->norigs[k];
    masks=wpd->masks[k];
    maskn=wpd->maskn[k];

    w[0]=0;
    for(i=0;i<channel;++i)
    {
        w[0]+=ffts[i].a*ffts[i].a+ffts[i].b*ffts[i].b;
    } 
    w[0]=1/(w[0]+sigma);
    for(i=1; i<channel; ++i)
    {
        w[i]=w[0];
    }

    memmove(norigs+channel, norigs, sizeof(wtk_complex_t)*channel*(L-1));
    memcpy(norigs, ffts, sizeof(wtk_complex_t)*channel);
    memmove(masks+channel, masks, sizeof(float)*channel*(L-1));
    memcpy(masks, mask, sizeof(float)*channel);

    memmove(norig+channel, norig, sizeof(wtk_complex_t)*channel*(L-1));
    memcpy(norig, fft, sizeof(wtk_complex_t)*channel);
    memmove(maskn+channel, maskn, sizeof(float)*channel*(L-1));
    memcpy(maskn, w, sizeof(float)*channel);

    wtk_covm2_feed_fft(covm, norigs, k, masks, 0);
    wtk_covm2_feed_fft(covm, norig, k, maskn, 1);

    // if(k==100)
    // {
    //     wtk_complex_print2(covm->scov[k], nl, nl);
    //     wtk_complex_print2(covm->ncov[k], nl, nl);
    //     printf("==========\n");
    // }

	ret=wtk_complex_guass_elimination_p2(covm->ncov[k], covm->scov[k], tmp, nl, tmp_cov);
    if(ret==0)
    {
        // tmp_cov1=tmp_cov;
        // fa=0;
        // fb=0;
        // for(i=0; i<channel; ++i, tmp_cov1+=channel+1)
        // {
        //     fa+=tmp_cov1->a;
        //     fb+=tmp_cov1->b;
        // }
        // ff=1.0/(fa*fa+fb*fb);
        // fa*=ff;
        // fb*=ff;

        // bfw=wpd->bfw;
        // for(i=0; i<nl; ++i, ++bfw, ++norigtmp, tmp_cov+=channel)
        // {
        //     bfw->a=tmp_cov->a*fa+tmp_cov->b*fb;
        //     bfw->b=-tmp_cov->a*fb+tmp_cov->b*fa;
        // }
        tmp_cov1=tmp_cov;
        fa=0;
        fb=0;
        for(i=0; i<channel; ++i, tmp_cov1+=channel+1)
        {
            fa+=sqrt(tmp_cov1->a*tmp_cov1->a+tmp_cov1->b*tmp_cov1->b);
        }
        ff=1.0/(fa*fa+fb*fb);
        fa*=ff;
        fb*=ff;

        bfw=wpd->bfw;
        for(i=0; i<nl; ++i, ++bfw, ++norigtmp, tmp_cov+=channel)
        {
            bfw->a=tmp_cov->a*fa+tmp_cov->b*fb;
            bfw->b=-tmp_cov->a*fb+tmp_cov->b*fa;
        }


    }

    norigtmp=norig;
    fa=fb=0;
    bfw=wpd->bfw;
    for(i=0; i<nl; ++i, ++bfw, ++norigtmp)
    {
        fa+=bfw->a*norigtmp->a+bfw->b*norigtmp->b;
        fb+=bfw->a*norigtmp->b-bfw->b*norigtmp->a;
	}
    outfft->a=fa;
    outfft->b=fb;
}