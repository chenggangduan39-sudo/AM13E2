#include "wtk_maskaspec.h" 

wtk_maskaspec_t* wtk_maskaspec_new(wtk_maskaspec_cfg_t *cfg, int nbin, int ang_num)
{
    wtk_maskaspec_t *maskaspec;

    maskaspec=(wtk_maskaspec_t *)wtk_malloc(sizeof(wtk_maskaspec_t));
    maskaspec->cfg=cfg;
    maskaspec->nbin=nbin;

    maskaspec->maskmvdrspec=NULL;
    maskaspec->maskdsspec=NULL;
    maskaspec->maskzdsspec=NULL;
     maskaspec->maskgccspec=NULL;
    if(cfg->use_maskmvdr)
    {
        maskaspec->maskmvdrspec=wtk_maskmvdrspec_new(nbin,cfg->channel,ang_num);
    }else if(cfg->use_maskds)
    {
        maskaspec->maskdsspec=wtk_maskdsspec_new(nbin,cfg->channel,ang_num);
    }else if(cfg->use_maskzds)
    {
        // maskaspec->maskzdsspec=wtk_maskzdsspec_new(nbin,cfg->channel,ang_num);
        maskaspec->maskzdsspec=wtk_maskzdsspec_new2(nbin,cfg->channel,ang_num,cfg->mic_pos,cfg->speed,cfg->rate,cfg->use_line);
    }else if(cfg->use_maskgcc)
    {
        maskaspec->maskgccspec=wtk_maskgccspec_new(nbin,cfg->channel,ang_num);
    }

    wtk_maskaspec_reset(maskaspec);

    return maskaspec;
}

void wtk_maskaspec_delete(wtk_maskaspec_t *maskaspec)
{
    if(maskaspec->maskmvdrspec)
    {
        wtk_maskmvdrspec_delete(maskaspec->maskmvdrspec);
    }else if(maskaspec->maskdsspec)
    {
        wtk_maskdsspec_delete(maskaspec->maskdsspec);
    }else if(maskaspec->maskzdsspec)
    {
        // wtk_maskzdsspec_delete(maskaspec->maskzdsspec);
        wtk_maskzdsspec_delete2(maskaspec->maskzdsspec);
    }else if(maskaspec->maskgccspec)
    {
        wtk_maskgccspec_delete(maskaspec->maskgccspec);
    }

    wtk_free(maskaspec);
}

void wtk_maskaspec_reset(wtk_maskaspec_t *maskaspec)
{
    maskaspec->need_cov=0;
    maskaspec->need_inv_ncov=0;
    if(maskaspec->maskmvdrspec)
    {
        maskaspec->need_inv_ncov=1;
    }
    if(maskaspec->maskdsspec || maskaspec->maskmvdrspec || maskaspec->maskzdsspec)
    {
        maskaspec->need_cov=1;
    }

    maskaspec->start_ang_num=0;

    if(maskaspec->maskmvdrspec)
    {
        wtk_maskmvdrspec_reset(maskaspec->maskmvdrspec);
    }else if(maskaspec->maskdsspec)
    {
        wtk_maskdsspec_reset(maskaspec->maskdsspec);
    }else if(maskaspec->maskzdsspec)
    {
        // wtk_maskzdsspec_reset(maskaspec->maskzdsspec);
        wtk_maskzdsspec_reset2(maskaspec->maskzdsspec);
    }else if(maskaspec->maskgccspec)
    {
        wtk_maskgccspec_reset(maskaspec->maskgccspec);
    }
}

void wtk_maskaspec_start(wtk_maskaspec_t *maskaspec, float  theta, float phi, int ang_idx)
{
    if(maskaspec->maskmvdrspec)
    {
        wtk_maskmvdrspec_start(maskaspec->maskmvdrspec, maskaspec->cfg->mic_pos, maskaspec->cfg->speed, maskaspec->cfg->rate, theta, phi, ang_idx);
    }else if(maskaspec->maskdsspec)
    {
        wtk_maskdsspec_start(maskaspec->maskdsspec, maskaspec->cfg->mic_pos, maskaspec->cfg->speed, maskaspec->cfg->rate, theta, phi, ang_idx);   
    }else if(maskaspec->maskzdsspec)
    {
        // wtk_maskzdsspec_start(maskaspec->maskzdsspec, maskaspec->cfg->mic_pos, maskaspec->cfg->speed, maskaspec->cfg->rate, theta, phi, ang_idx, maskaspec->cfg->use_line, maskaspec->cfg->ls_eye, maskaspec->cfg->th_step);   
        wtk_maskzdsspec_start2(maskaspec->maskzdsspec, maskaspec->cfg->mic_pos, maskaspec->cfg->speed, maskaspec->cfg->rate, theta, phi, ang_idx, maskaspec->cfg->use_line, maskaspec->cfg->ls_eye, maskaspec->cfg->th_step);   
    }else if(maskaspec->maskgccspec)
    {
        wtk_maskgccspec_start(maskaspec->maskgccspec, maskaspec->cfg->mic_pos, maskaspec->cfg->speed, maskaspec->cfg->rate, theta, phi, ang_idx);   
    }
}

void wtk_maskaspec_start2(wtk_maskaspec_t *maskaspec, float  theta, float phi, int ang_idx)
{
    if(maskaspec->maskmvdrspec)
    {
        wtk_maskmvdrspec_start(maskaspec->maskmvdrspec, maskaspec->cfg->mic_pos2, maskaspec->cfg->speed, maskaspec->cfg->rate, theta, phi, ang_idx);
    }else if(maskaspec->maskdsspec)
    {
        wtk_maskdsspec_start(maskaspec->maskdsspec, maskaspec->cfg->mic_pos2, maskaspec->cfg->speed, maskaspec->cfg->rate, theta, phi, ang_idx);   
    }else if(maskaspec->maskzdsspec)
    {
        // wtk_maskzdsspec_start(maskaspec->maskzdsspec, maskaspec->cfg->mic_pos, maskaspec->cfg->speed, maskaspec->cfg->rate, theta, phi, ang_idx, maskaspec->cfg->use_line, maskaspec->cfg->ls_eye, maskaspec->cfg->th_step);   
        wtk_maskzdsspec_start2(maskaspec->maskzdsspec, maskaspec->cfg->mic_pos2, maskaspec->cfg->speed, maskaspec->cfg->rate, theta, phi, ang_idx, maskaspec->cfg->use_line, maskaspec->cfg->ls_eye, maskaspec->cfg->th_step);   
    }else if(maskaspec->maskgccspec)
    {
        wtk_maskgccspec_start(maskaspec->maskgccspec, maskaspec->cfg->mic_pos2, maskaspec->cfg->speed, maskaspec->cfg->rate, theta, phi, ang_idx);   
    }
}

float wtk_maskaspec_flush_spec_k(wtk_maskaspec_t *maskaspec, wtk_complex_t *fft, wtk_complex_t *scov,wtk_complex_t *ncov, wtk_complex_t *inv_cov, float prob, int k, int ang_idx)
{
    float spec=0;

    if(maskaspec->maskmvdrspec)
    {
        spec=wtk_maskmvdrspec_flush_spec_k(maskaspec->maskmvdrspec, scov, inv_cov, prob, k,ang_idx);
    }else if(maskaspec->maskdsspec)
    {
        spec=wtk_maskdsspec_flush_spec_k(maskaspec->maskdsspec, scov, ncov, prob, k,ang_idx);
    }else if(maskaspec->maskzdsspec)
    {
        spec=wtk_maskzdsspec_flush_spec_k(maskaspec->maskzdsspec, scov, ncov, prob, k,ang_idx);
    }else if(maskaspec->maskgccspec)
    {
        spec=wtk_maskgccspec_flush_spec_k(maskaspec->maskgccspec, fft, prob, k,ang_idx);
    }

    return spec;
}

void wtk_maskaspec_flush_spec_k2(wtk_maskaspec_t *maskaspec, wtk_complex_t *fft, wtk_complex_t *scov,wtk_complex_t *ncov, wtk_complex_t *inv_cov, float prob, int k, int ang_idx, float *spec)
{
    if(maskaspec->maskmvdrspec)
    {
        wtk_maskmvdrspec_flush_spec_k2(maskaspec->maskmvdrspec, scov, inv_cov, prob, k,ang_idx, spec);
    }else if(maskaspec->maskdsspec)
    {
        wtk_maskdsspec_flush_spec_k2(maskaspec->maskdsspec, scov, ncov, prob, k,ang_idx, spec);
    }else if(maskaspec->maskzdsspec)
    {
        wtk_maskzdsspec_flush_spec_k2(maskaspec->maskzdsspec, scov, ncov, prob, k,ang_idx, spec);
    }else if(maskaspec->maskgccspec)
    {
        wtk_maskgccspec_flush_spec_k2(maskaspec->maskgccspec, fft, prob, k,ang_idx, spec);
    }
}