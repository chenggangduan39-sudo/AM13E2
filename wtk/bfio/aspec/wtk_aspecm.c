#include "wtk_aspecm.h"

wtk_aspecm_t *wtk_aspecm_new(wtk_aspecm_cfg_t *cfg, int nbin, int channel) {
    wtk_aspecm_t *aspecm;

    aspecm = (wtk_aspecm_t *)wtk_malloc(sizeof(wtk_aspecm_t));
    aspecm->cfg = cfg;
    if(cfg->nbin != nbin || cfg->nmic != channel){
        wtk_debug("err cfg->nbin:%d cfg->nmic:%d nbin:%d channel:%d\n",cfg->nbin,cfg->nmic,nbin,channel);
        exit(0);
    }

	aspecm->qmmse=NULL;
	if(cfg->use_qmmse)
	{
        aspecm->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}

    aspecm->aspec=wtk_aspec_new(&(cfg->aspec), nbin, 3);
    aspecm->q_fring=wtk_fring_new(cfg->q_nf);
    aspecm->cohv=(float *)wtk_malloc(sizeof(float) * nbin);
    aspecm->cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel*channel);
    aspecm->inv_cov=NULL;
    if(aspecm->aspec->need_inv_cov){
        aspecm->inv_cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel*channel);
        aspecm->tmp=(wtk_dcomplex_t *)wtk_malloc(sizeof(wtk_dcomplex_t)*channel*channel*2);
    }
    aspecm->gcc_fft=NULL;
    if(aspecm->aspec->gccspec || aspecm->aspec->gccspec2){
        aspecm->gcc_fft = wtk_complex_new_p2(nbin, channel);
    }

    wtk_aspecm_reset(aspecm);

    return aspecm;
}

void wtk_aspecm_reset(wtk_aspecm_t *aspecm) {
    int nbin = aspecm->cfg->nbin;
    int channel = aspecm->cfg->nmic;

    if(aspecm->qmmse){
        wtk_qmmse_reset(aspecm->qmmse);
    }
    wtk_aspec_reset(aspecm->aspec);
    wtk_fring_reset(aspecm->q_fring);
    memset(aspecm->cohv, 0, sizeof(float)*nbin);
    memset(aspecm->cov, 0, sizeof(wtk_complex_t)*channel*channel);
    if(aspecm->inv_cov){
        memset(aspecm->inv_cov, 0, sizeof(wtk_complex_t)*channel*channel);
        memset(aspecm->tmp, 0, sizeof(wtk_dcomplex_t)*channel*channel*2);
    }
    if(aspecm->gcc_fft){
        wtk_complex_zero_p2(aspecm->gcc_fft, nbin, channel);
    }
    aspecm->right_nf = 0;
    aspecm->q_spec = 0;
}

void wtk_aspecm_delete(wtk_aspecm_t *aspecm) {
    int nbin = aspecm->cfg->nbin;
    if(aspecm->qmmse){
        wtk_qmmse_delete(aspecm->qmmse);
    }
    wtk_aspec_delete(aspecm->aspec);
    wtk_fring_delete(aspecm->q_fring);
    wtk_free(aspecm->cohv);
    wtk_free(aspecm->cov);
    if(aspecm->inv_cov){
        wtk_free(aspecm->inv_cov);
        wtk_free(aspecm->tmp);
    }
    if(aspecm->gcc_fft){
        wtk_complex_delete_p2(aspecm->gcc_fft, nbin);
    }
    wtk_free(aspecm);
}

void wtk_aspecm_start_aspec1(wtk_aspecm_t *aspecm) {
    float theta2, theta3;
    float theta = aspecm->cfg->theta;
    float theta_range = aspecm->cfg->theta_range;
    wtk_aspec_t *aspec = aspecm->aspec;

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

void wtk_aspecm_start_aspec2(wtk_aspecm_t *aspecm) {
    float theta2, theta3;
    float theta = aspecm->cfg->theta;
    float theta_range = aspecm->cfg->theta_range;
    wtk_aspec_t *aspec = aspecm->aspec;

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

void wtk_aspecm_start(wtk_aspecm_t *aspecm) {
    if(aspecm->cfg->use_line){
        wtk_aspecm_start_aspec1(aspecm);
    }else{
        wtk_aspecm_start_aspec2(aspecm);
    }
}

void wtk_aspecm_feed_aspec(wtk_aspecm_t *aspecm, wtk_complex_t **fft, wtk_complex_t *fftx){
    int channel = aspecm->cfg->nmic;
    int specsum_ns=aspecm->cfg->specsum_ns;
    int specsum_ne=aspecm->cfg->specsum_ne;
    int nbin = aspecm->cfg->nbin;
    int i, j, k;
    int ret;
    wtk_complex_t *cov = aspecm->cov;
    wtk_complex_t *inv_cov = aspecm->inv_cov;
    wtk_complex_t **gcc_fft = aspecm->gcc_fft;
    wtk_dcomplex_t *tmp = aspecm->tmp;
    float *cohv = aspecm->cohv;
    float cov_travg;
    float spec_k[3]={0};
    float fftabs2;
    float specsum=0;
    float max_spec;
    float q_alpha = aspecm->cfg->q_alpha;
    float q_alpha_1 = 1.0 - q_alpha;

    if(aspecm->aspec->gccspec || aspecm->aspec->gccspec2){
        for(i=0;i<channel;++i){
            for(k=0;k<nbin;++k){
                gcc_fft[k][i].a = fft[i][k].a;
                gcc_fft[k][i].b = fft[i][k].b;
            }
        }
    }

    for(k=0;k<nbin;++k){
        memset(cov, 0, sizeof(wtk_complex_t)*channel*channel);
        for(i=0;i<channel;++i){
            for(j=i;j<channel;++j){
                if(i!=j){
                    cov[i*channel+j].a = fft[i][k].a * fft[j][k].a + fft[i][k].b * fft[j][k].b;
                    cov[i*channel+j].b = -fft[i][k].a * fft[j][k].b + fft[i][k].b * fft[j][k].a;
                    cov[j*channel+i].a = cov[i*channel+j].a;
                    cov[j*channel+i].b = -cov[i*channel+j].b;
                }else{
                    cov[i*channel+j].a = fft[i][k].a * fft[j][k].a + fft[i][k].b * fft[j][k].b;
                    cov[i*channel+j].b = 0;
                }
            }
        }
        if(inv_cov){
            ret=wtk_complex_invx4(cov, tmp, channel, inv_cov, 1);
            if(ret!=0)
            {
                j=0;
                for(i=0;i<channel;++i)
                {
                    cov[j].a+=0.01;
                    j+=channel+1;
                }
                wtk_complex_invx4(cov,tmp,channel,inv_cov,1);
            }
        }
        cov_travg = 0;
        if(aspecm->aspec->need_cov_travg){
            for(i=0;i<channel;++i){
                cov_travg += cov[i*channel+i].a;
            }
            cov_travg /= channel;
        }
        if(aspecm->aspec->gccspec || aspecm->aspec->gccspec2){
            fftabs2=0;
            for(i=0; i<channel; ++i)
            {
                fftabs2+=gcc_fft[k][i].a*gcc_fft[k][i].a+gcc_fft[k][i].b*gcc_fft[k][i].b;
            }
        }
        for(i=0;i<3;++i){
            spec_k[i] = wtk_aspec_flush_spec_k(aspecm->aspec, gcc_fft, fftabs2, cov_travg, cov, inv_cov, k, i);
        }
        if(spec_k[0] > spec_k[1] && spec_k[0] > spec_k[2]){
            cohv[k] = 1;
            if(k>=specsum_ns && k<=specsum_ne){
                specsum += spec_k[0] * 2 - spec_k[1] - spec_k[2];
            }
        }else{
            cohv[k] = 0;
        }
    }
    // printf("%f\n", specsum);
    wtk_fring_push2(aspecm->q_fring, specsum);
    if(aspecm->q_fring->used == aspecm->q_fring->nslot - 1){
        max_spec = wtk_fring_max(aspecm->q_fring);
        if(max_spec < aspecm->q_spec){
            max_spec = q_alpha * aspecm->q_spec + q_alpha_1 * max_spec;
        }
        aspecm->q_spec = max_spec;
        if(max_spec > aspecm->cfg->min_speccrest){
            aspecm->right_nf = aspecm->cfg->right_nf;
        }else if(max_spec > aspecm->cfg->envelope_thresh){

        }else if(max_spec > aspecm->cfg->right_min_thresh){
            --aspecm->right_nf;
        }else{
            aspecm->right_nf = 0;
        }
    }
    if(aspecm->right_nf <= 0){
        for(k=0;k<nbin;++k){
            cohv[k] = 0;
        }
    }else{
        if(aspecm->cfg->use_sqenvelope){
            for(k=1; k<nbin-1; ++k)
            {
                cohv[k] = 1;
            }
        }
    }
    if(aspecm->cfg->use_qmmse && fftx){
        if(aspecm->right_nf <= 0){
            for(i=0;i<nbin;++i){
                fftx[i].a = 0;
                fftx[i].b = 0;
            }
        // }else{
        //     for(i=0;i<nbin;++i){
        //         fftx[i].a *= aspecm->cohv[i];
        //         fftx[i].b *= aspecm->cohv[i];
        //     }
        }
        wtk_qmmse_feed_cohv(aspecm->qmmse, fftx, aspecm->cohv);
    }
}
