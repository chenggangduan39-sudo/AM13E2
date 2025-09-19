#include "wtk_aspec.h" 

wtk_aspec_t* wtk_aspec_new(wtk_aspec_cfg_t *cfg, int nbin, int ang_num)
{
    wtk_aspec_t *aspec;

    aspec=(wtk_aspec_t *)wtk_malloc(sizeof(wtk_aspec_t));
    aspec->cfg=cfg;
    aspec->nbin=nbin;

    aspec->gccspec=NULL;
    aspec->ml=NULL;
    aspec->gccspec2=NULL;
    aspec->ngccspec2=NULL;
    aspec->dnmspec=NULL;
    aspec->dnmspec2=NULL;
    aspec->mvdrspec=NULL;
    aspec->mvdrspec2=NULL;
    aspec->mvdrwspec=NULL;
    aspec->mvdrwspec2=NULL;
    aspec->dsspec=NULL;
    aspec->dsspec2=NULL;
    aspec->dswspec=NULL;
    aspec->dswspec2=NULL;
    aspec->musicspec2=NULL;
    aspec->zdsspec=NULL;
    aspec->zdswspec=NULL;
    
    aspec->ths=NULL;
    aspec->flush_spec_f=NULL;
    aspec->flush_spec_f2=NULL;
    if(cfg->use_gccspec)
    {
        aspec->gccspec=wtk_gccspec_new(nbin,cfg->channel,ang_num);
        aspec->ths=aspec->gccspec;
        if(cfg->use_fftnbinfirst)
        {
            aspec->flush_spec_f=cfg->use_quick?((wtk_aspec_flush_spec_k_f)wtk_gccspec_flush_spec_k4):
                                                                                            ((wtk_aspec_flush_spec_k_f)wtk_gccspec_flush_spec_k3);
            aspec->flush_spec_f2=cfg->use_quick?((wtk_aspec_flush_spec_k_f2)wtk_gccspec_flush_spec_k4_2):
                                                                                            ((wtk_aspec_flush_spec_k_f2)wtk_gccspec_flush_spec_k3_2);
        }else
        {
            aspec->flush_spec_f=cfg->use_quick?((wtk_aspec_flush_spec_k_f)wtk_gccspec_flush_spec_k2):
                                                                                            ((wtk_aspec_flush_spec_k_f)wtk_gccspec_flush_spec_k);
            aspec->flush_spec_f2=cfg->use_quick?((wtk_aspec_flush_spec_k_f2)wtk_gccspec_flush_spec_k2_2):
                                                                                            ((wtk_aspec_flush_spec_k_f2)wtk_gccspec_flush_spec_k_2);
        }
    }else if(cfg->use_ml)
    {
        aspec->ml=wtk_ml_new(nbin,cfg->channel,ang_num);
        aspec->ths=aspec->ml;
        aspec->flush_spec_f=cfg->use_fftnbinfirst?((wtk_aspec_flush_spec_k_f)wtk_ml_flush_spec_k2):
                                                                                                    ((wtk_aspec_flush_spec_k_f)wtk_ml_flush_spec_k);
    }else if(cfg->use_gccspec2)
    {
        aspec->gccspec2=wtk_gccspec2_new(nbin,cfg->channel,aspec->cfg->pairs_n,ang_num);
        aspec->ths=aspec->gccspec2;
        aspec->flush_spec_f=cfg->use_fftnbinfirst?((wtk_aspec_flush_spec_k_f)wtk_gccspec2_flush_spec_k2):
                                                                                                    ((wtk_aspec_flush_spec_k_f)wtk_gccspec2_flush_spec_k);
    }else if(cfg->use_ngccspec2)
    {
        aspec->ngccspec2=wtk_ngccspec2_new(nbin,cfg->channel,aspec->cfg->pairs_n,ang_num);
        aspec->ths=aspec->ngccspec2;
        aspec->flush_spec_f=cfg->use_fftnbinfirst?((wtk_aspec_flush_spec_k_f)wtk_ngccspec2_flush_spec_k2):
                                                                                                    ((wtk_aspec_flush_spec_k_f)wtk_ngccspec2_flush_spec_k);
    }else if(cfg->use_dnmspec)
    {
        aspec->dnmspec=wtk_dnmspec_new(nbin,cfg->channel,ang_num);
        aspec->ths=aspec->dnmspec;
        aspec->flush_spec_f=(wtk_aspec_flush_spec_k_f)wtk_dnmspec_flush_spec_k;
    }else if(cfg->use_dnmspec2)
    {
        aspec->dnmspec2=wtk_dnmspec2_new(nbin,cfg->channel,aspec->cfg->pairs_n,ang_num);
        aspec->ths=aspec->dnmspec2;
        aspec->flush_spec_f=(wtk_aspec_flush_spec_k_f)wtk_dnmspec2_flush_spec_k;
    }else if(cfg->use_mvdrspec)
    {
        aspec->mvdrspec=wtk_mvdrspec_new(nbin,cfg->channel,ang_num);
        aspec->ths=aspec->mvdrspec;
        aspec->flush_spec_f=(wtk_aspec_flush_spec_k_f)wtk_mvdrspec_flush_spec_k;
    }else if(cfg->use_mvdrspec2)
    {
        aspec->mvdrspec2=wtk_mvdrspec2_new(nbin,cfg->channel,aspec->cfg->pairs_n,ang_num);
        aspec->ths=aspec->mvdrspec2;
        aspec->flush_spec_f=(wtk_aspec_flush_spec_k_f)wtk_mvdrspec2_flush_spec_k;
    }else if(cfg->use_mvdrwspec)
    {
        aspec->mvdrwspec=wtk_mvdrwspec_new(nbin,cfg->channel,ang_num);
        aspec->ths=aspec->mvdrwspec;
        aspec->flush_spec_f=(wtk_aspec_flush_spec_k_f)wtk_mvdrwspec_flush_spec_k;
    }else if(cfg->use_mvdrwspec2)
    {
        aspec->mvdrwspec2=wtk_mvdrwspec2_new(nbin,cfg->channel,aspec->cfg->pairs_n,ang_num);
        aspec->ths=aspec->mvdrwspec2;
        aspec->flush_spec_f=(wtk_aspec_flush_spec_k_f)wtk_mvdrwspec2_flush_spec_k;
    }else if(cfg->use_dsspec)
    {
        aspec->dsspec=wtk_dsspec_new(nbin,cfg->channel,ang_num);
        aspec->ths=aspec->dsspec;
        aspec->flush_spec_f=(wtk_aspec_flush_spec_k_f)wtk_dsspec_flush_spec_k;
    }else if(cfg->use_dsspec2)
    {
        aspec->dsspec2=wtk_dsspec2_new(nbin,cfg->channel,aspec->cfg->pairs_n,ang_num);
        aspec->ths=aspec->dsspec2;
        aspec->flush_spec_f=(wtk_aspec_flush_spec_k_f)wtk_dsspec2_flush_spec_k;
    }else if(cfg->use_dswspec)
    {
        aspec->dswspec=wtk_dswspec_new(nbin,cfg->channel,ang_num);
        aspec->ths=aspec->dswspec;
        aspec->flush_spec_f=(wtk_aspec_flush_spec_k_f)wtk_dswspec_flush_spec_k;
        aspec->flush_spec_f2=(wtk_aspec_flush_spec_k_f2)wtk_dswspec_flush_spec_k2;
    }else if(cfg->use_dswspec2)
    {
        aspec->dswspec2=wtk_dswspec2_new(nbin,cfg->channel,aspec->cfg->pairs_n,ang_num);
        aspec->ths=aspec->dswspec2;
        aspec->flush_spec_f=(wtk_aspec_flush_spec_k_f)wtk_dswspec2_flush_spec_k;
    }else if(cfg->use_musicspec2)
    {
        aspec->musicspec2=wtk_musicspec2_new(nbin,cfg->channel,aspec->cfg->pairs_n,ang_num);
        aspec->ths=aspec->musicspec2;
        aspec->flush_spec_f=(wtk_aspec_flush_spec_k_f)wtk_musicspec2_flush_spec_k;
    }else if(cfg->use_zdsspec)
    {
        aspec->zdsspec=wtk_zdsspec_new(nbin,cfg->channel,ang_num);
        aspec->ths=aspec->zdsspec;
        aspec->flush_spec_f=(wtk_aspec_flush_spec_k_f)wtk_zdsspec_flush_spec_k;
    }else if(cfg->use_zdswspec)
    {
        aspec->zdswspec=wtk_zdswspec_new(nbin,cfg->channel,ang_num);
        aspec->ths=aspec->zdswspec;
        aspec->flush_spec_f=(wtk_aspec_flush_spec_k_f)wtk_zdswspec_flush_spec_k;
    }

    wtk_aspec_reset(aspec);

    return aspec;
}

void wtk_aspec_delete(wtk_aspec_t *aspec)
{
    if(aspec->gccspec)
    {
        wtk_gccspec_delete(aspec->gccspec);
    }
    if(aspec->gccspec2)
    {
        wtk_gccspec2_delete(aspec->gccspec2);
    }
    if(aspec->ngccspec2)
    {
        wtk_ngccspec2_delete(aspec->ngccspec2);
    }
    if(aspec->dnmspec)
    {
        wtk_dnmspec_delete(aspec->dnmspec);
    }
    if(aspec->dnmspec2)
    {
        wtk_dnmspec2_delete(aspec->dnmspec2);
    }
    if(aspec->mvdrspec)
    {
        wtk_mvdrspec_delete(aspec->mvdrspec);
    }
    if(aspec->mvdrspec2)
    {
        wtk_mvdrspec2_delete(aspec->mvdrspec2);
    }
    if(aspec->mvdrwspec)
    {
        wtk_mvdrwspec_delete(aspec->mvdrwspec);
    }
    if(aspec->mvdrwspec2)
    {
        wtk_mvdrwspec2_delete(aspec->mvdrwspec2);
    }
    if(aspec->dsspec)
    {
        wtk_dsspec_delete(aspec->dsspec);
    }
    if(aspec->dsspec2)
    {
        wtk_dsspec2_delete(aspec->dsspec2);
    }
    if(aspec->dswspec)
    {
        wtk_dswspec_delete(aspec->dswspec);
    }
    if(aspec->dswspec2)
    {
        wtk_dswspec2_delete(aspec->dswspec2);
    }
    if(aspec->musicspec2)
    {
        wtk_musicspec2_delete(aspec->musicspec2);
    }
    if(aspec->zdsspec)
    {
        wtk_zdsspec_delete(aspec->zdsspec);
    }
    if(aspec->zdswspec)
    {
        wtk_zdswspec_delete(aspec->zdswspec);
    }
    wtk_free(aspec);
}

void wtk_aspec_reset(wtk_aspec_t *aspec)
{
    aspec->need_inv_cov=0;
    aspec->need_cov=0;
    aspec->need_cov_travg=0;
    if(aspec->dnmspec || aspec->dnmspec2 ||  aspec->mvdrwspec ||aspec->mvdrwspec2 || aspec->mvdrspec 
                || aspec->mvdrspec2 || aspec->dsspec || aspec->dsspec2 || aspec->dswspec || aspec->dswspec2 
                || aspec->musicspec2 || aspec->zdsspec || aspec->zdswspec)
    {
        aspec->need_cov=1;
        if(aspec->mvdrspec || aspec->mvdrwspec)
        {
            aspec->need_inv_cov=1;
        }
        if(aspec->mvdrspec || aspec->mvdrwspec || aspec->dsspec || aspec->dswspec || aspec->zdsspec || aspec->zdswspec)
        {
            aspec->need_cov_travg=1;
        }
    }

    aspec->start_ang_num=0;

    if(aspec->gccspec)
    {
        wtk_gccspec_reset(aspec->gccspec);
    }
    if(aspec->ml)
    {
        wtk_ml_reset(aspec->ml);
    }
    if(aspec->gccspec2)
    {
        wtk_gccspec2_reset(aspec->gccspec2);
    }
    if(aspec->ngccspec2)
    {
        wtk_ngccspec2_reset(aspec->ngccspec2);
    }
    if(aspec->dnmspec)
    {
        wtk_dnmspec_reset(aspec->dnmspec);
    }
    if(aspec->dnmspec2)
    {
        wtk_dnmspec2_reset(aspec->dnmspec2);
    }
    if(aspec->mvdrspec)
    {
        wtk_mvdrspec_reset(aspec->mvdrspec);
    }
    if(aspec->mvdrspec2)
    {
        wtk_mvdrspec2_reset(aspec->mvdrspec2);
    }
    if(aspec->mvdrwspec)
    {
        wtk_mvdrwspec_reset(aspec->mvdrwspec);
    }
    if(aspec->mvdrwspec2)
    {
        wtk_mvdrwspec2_reset(aspec->mvdrwspec2);
    }
    if(aspec->dsspec)
    {
        wtk_dsspec_reset(aspec->dsspec);
    }
    if(aspec->dsspec2)
    {
        wtk_dsspec2_reset(aspec->dsspec2);
    }
    if(aspec->dswspec)
    {
        wtk_dswspec_reset(aspec->dswspec);
    }
    if(aspec->dswspec2)
    {
        wtk_dswspec2_reset(aspec->dswspec2);
    }
    if(aspec->musicspec2)
    {
        wtk_musicspec2_reset(aspec->musicspec2);
    }
    if(aspec->zdsspec)
    {
        wtk_zdsspec_reset(aspec->zdsspec);
    }
    if(aspec->zdswspec)
    {
        wtk_zdswspec_reset(aspec->zdswspec);
    }
}

void wtk_aspec_start(wtk_aspec_t *aspec, float  theta, float phi, int ang_idx)
{
    if(aspec->gccspec)
    {
        wtk_gccspec_start(aspec->gccspec, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, ang_idx);
    }else if(aspec->ml)
    {
        wtk_ml_start(aspec->ml, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, ang_idx);
    }else if(aspec->gccspec2)
    {
        wtk_gccspec2_start(aspec->gccspec2, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, aspec->cfg->pairs_mic, ang_idx);
    }else if(aspec->ngccspec2)
    {
        wtk_ngccspec2_start(aspec->ngccspec2, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, aspec->cfg->pairs_mic, ang_idx);
    }else if(aspec->dnmspec)
    {
        wtk_dnmspec_start(aspec->dnmspec, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, ang_idx);
    }else if(aspec->dnmspec2)
    {
        wtk_dnmspec2_start(aspec->dnmspec2, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, aspec->cfg->pairs_mic, ang_idx);
    }else if(aspec->mvdrspec)
    {
        wtk_mvdrspec_start(aspec->mvdrspec, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, ang_idx);
    }else if(aspec->mvdrspec2)
    {
        wtk_mvdrspec2_start(aspec->mvdrspec2, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, aspec->cfg->pairs_mic, ang_idx);
    }else if(aspec->mvdrwspec)
    {
        wtk_mvdrwspec_start(aspec->mvdrwspec, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, ang_idx);
    }else if(aspec->mvdrwspec2)
    {
        wtk_mvdrwspec2_start(aspec->mvdrwspec2, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, aspec->cfg->pairs_mic, ang_idx);
    }else if(aspec->dsspec)
    {
        wtk_dsspec_start(aspec->dsspec, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, ang_idx);
    }else if(aspec->dsspec2)
    {
        wtk_dsspec2_start(aspec->dsspec2, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, aspec->cfg->pairs_mic, ang_idx);
    }else if(aspec->dswspec)
    {
        wtk_dswspec_start(aspec->dswspec, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, ang_idx);
    }else if(aspec->dswspec2)
    {
        wtk_dswspec2_start(aspec->dswspec2, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, aspec->cfg->pairs_mic, ang_idx);
    }else if(aspec->musicspec2)
    {
        wtk_musicspec2_start(aspec->musicspec2, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, aspec->cfg->pairs_mic, ang_idx);
    }else if(aspec->zdsspec)
    {
        wtk_zdsspec_start(aspec->zdsspec, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, ang_idx, aspec->cfg->use_line, aspec->cfg->ls_eye);
    }else if(aspec->zdswspec)
    {
        wtk_zdswspec_start(aspec->zdswspec, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, theta, phi, ang_idx, aspec->cfg->use_line, aspec->cfg->ls_eye);
    }
}

void wtk_aspec_start2(wtk_aspec_t *aspec, float x, float y, float z, int ang_idx)
{
    if(aspec->gccspec)
    {
        wtk_gccspec_start2(aspec->gccspec, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, x, y, z, ang_idx);
    }else if(aspec->ml)
    {
        wtk_ml_start2(aspec->ml, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, x, y, z, ang_idx);
    }else if(aspec->gccspec2)
    {
        wtk_gccspec2_start2(aspec->gccspec2, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate,  x, y, z, aspec->cfg->pairs_mic, ang_idx);
    }else if(aspec->ngccspec2)
    {
        wtk_ngccspec2_start2(aspec->ngccspec2, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, x, y, z, aspec->cfg->pairs_mic, ang_idx);
    }else if(aspec->dnmspec)
    {
        wtk_dnmspec_start2(aspec->dnmspec, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, x, y, z, ang_idx);
    }else if(aspec->dnmspec2)
    {
        wtk_dnmspec2_start2(aspec->dnmspec2, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, x, y, z, aspec->cfg->pairs_mic, ang_idx);
    }else if(aspec->mvdrspec)
    {
        wtk_mvdrspec_start2(aspec->mvdrspec, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, x, y, z, ang_idx);
    }else if(aspec->mvdrspec2)
    {
        wtk_mvdrspec2_start2(aspec->mvdrspec2, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, x, y, z, aspec->cfg->pairs_mic, ang_idx);
    }else if(aspec->mvdrwspec)
    {
        wtk_mvdrwspec_start2(aspec->mvdrwspec, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, x, y, z, ang_idx);
    }else if(aspec->mvdrwspec2)
    {
        wtk_mvdrwspec2_start2(aspec->mvdrwspec2, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, x, y, z, aspec->cfg->pairs_mic, ang_idx);
    }else if(aspec->dsspec)
    {
        wtk_dsspec_start2(aspec->dsspec, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, x, y, z, ang_idx);
    }else if(aspec->dsspec2)
    {
        wtk_dsspec2_start2(aspec->dsspec2, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, x, y, z, aspec->cfg->pairs_mic, ang_idx);
    }else if(aspec->dswspec)
    {
        wtk_dswspec_start2(aspec->dswspec, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, x, y, z, ang_idx);
    }else if(aspec->dswspec2)
    {
        wtk_dswspec2_start2(aspec->dswspec2, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, x, y, z, aspec->cfg->pairs_mic, ang_idx);
    }else if(aspec->musicspec2)
    {
        wtk_musicspec2_start2(aspec->musicspec2, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, x, y, z, aspec->cfg->pairs_mic, ang_idx);
    }else if(aspec->zdsspec)
    {
        wtk_zdsspec_start2(aspec->zdsspec, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, x, y, z, ang_idx, aspec->cfg->use_line, aspec->cfg->ls_eye);
    }else if(aspec->zdswspec)
    {
        wtk_zdswspec_start2(aspec->zdswspec, aspec->cfg->mic_pos, aspec->cfg->speed, aspec->cfg->rate, x, y, z, ang_idx, aspec->cfg->use_line, aspec->cfg->ls_eye);
    }
}

float wtk_aspec_flush_spec_k(wtk_aspec_t *aspec, wtk_complex_t **fft, float fftabs2, float cov_travg, wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx)
{
    float spec=0;
    if(aspec->ths)
    {
        spec=aspec->flush_spec_f(aspec->ths, aspec->cfg->pairs_mic, fft, fftabs2, cov_travg, cov, inv_cov, k, ang_idx);
    }else
    {
        wtk_debug("error: no spec_f\n");
    }
    
    return spec;
}

void wtk_aspec_flush_spec_k2(wtk_aspec_t *aspec, wtk_complex_t **fft, float fftabs2, float cov_travg, wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx, float *spec)
{
    if(aspec->ths)
    {
        aspec->flush_spec_f2(aspec->ths, aspec->cfg->pairs_mic, fft, fftabs2, cov_travg, cov, inv_cov, k, ang_idx, spec);
    }else
    {
        wtk_debug("error: no spec_f\n");
    }
}
