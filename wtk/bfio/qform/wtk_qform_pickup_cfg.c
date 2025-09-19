#include "wtk_qform_pickup_cfg.h"

int wtk_qform_pickup_cfg_init(wtk_qform_pickup_cfg_t *cfg)
{
    wtk_stft_cfg_init(&(cfg->stft));
    wtk_bf_cfg_init(&(cfg->bf));
    wtk_qmmse_cfg_init(&(cfg->qmmse));

    cfg->cohv_thresh=0.0;
    cfg->ncov_alpha=0.1;
    cfg->init_ncovnf=100;

    cfg->scov_alpha=0.1;
    cfg->init_scovnf=100;

    cfg->notch_radius=0.982;
	cfg->preemph=0.95;

    cfg->debug=1;
    cfg->theta_range=45;
    cfg->use_line=0;

    cfg->rate=16000;
    cfg->use_post=1;

    cfg->noise_block=0;
    cfg->ntheta=NULL;

    cfg->batch_tm=0;

    cfg->fs=0;
    cfg->fe=8000;

    cfg->use_cohvsum=0;

    return 0;
}


int wtk_qform_pickup_cfg_clean(wtk_qform_pickup_cfg_t *cfg)
{
    if(cfg->ntheta)
    {
        wtk_free(cfg->ntheta);
    }
    wtk_stft_cfg_clean(&(cfg->stft));
    wtk_bf_cfg_clean(&(cfg->bf));
    wtk_qmmse_cfg_clean(&(cfg->qmmse));

    return 0;
}

int wtk_qform_pickup_cfg_update_local(wtk_qform_pickup_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v;
    wtk_local_cfg_t *m;
    int ret;
    wtk_array_t *ntheta_array;
    int i;


	wtk_local_cfg_update_cfg_i(lc,cfg,fs,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,fe,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,batch_tm,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,noise_block,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,notch_radius,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,preemph,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_cohvsum,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_post,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,init_ncovnf,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,ncov_alpha,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,cohv_thresh,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,theta_range,v);

    wtk_local_cfg_update_cfg_i(lc,cfg,init_scovnf,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,scov_alpha,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
    m=lc;
	lc=wtk_local_cfg_find_lc_s(m,"stft");
	if(lc)
	{
        ret=wtk_stft_cfg_update_local(&(cfg->stft),lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(m,"bf");
	if(lc)
	{
        ret=wtk_bf_cfg_update_local(&(cfg->bf),lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
        cfg->qmmse.step=cfg->stft.win/2;
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
        if(ret!=0){goto end;}
    }

    ntheta_array=wtk_local_cfg_find_float_array_s(m,"ntheta");
    if(ntheta_array)
    {
        cfg->noise_block=ntheta_array->nslot;
        cfg->ntheta=(float *)wtk_malloc(cfg->noise_block*sizeof(float));
        for(i=0;i<cfg->noise_block;++i)
        {
            cfg->ntheta[i]=((float *)(ntheta_array->slot))[i];
        }
    }

end:
    return ret;
}

int wtk_qform_pickup_cfg_update(wtk_qform_pickup_cfg_t *cfg)
{
    int ret;
    int i, min_ntheta = -1;

    ret=wtk_stft_cfg_update(&(cfg->stft));
    if(ret!=0){goto end;}
    ret=wtk_bf_cfg_update(&(cfg->bf));
    if(ret!=0){goto end;}
    ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
    if(ret!=0){goto end;}

    cfg->notch_radius_den=cfg->notch_radius*cfg->notch_radius+0.7*(1-cfg->notch_radius)*(1-cfg->notch_radius);
    
    for(i=0;i<cfg->noise_block;++i)
    {
        if(i==0)
        {
            min_ntheta=cfg->ntheta[i];
        }else
        {
            min_ntheta=min(cfg->ntheta[i],min_ntheta);
        }
    }
    cfg->ntheta_range=min_ntheta-cfg->theta_range;

    cfg->nbatch=floor(cfg->batch_tm*(cfg->rate/1000)/cfg->stft.step);

    if(cfg->stft.channel==2)
    {
        cfg->use_line=1;
    }

    cfg->fs=cfg->fs/(cfg->rate/(cfg->stft.win));
    cfg->fe=cfg->fe/(cfg->rate/(cfg->stft.win));
    ret=0;
end:
    return ret;
}

int wtk_qform_pickup_cfg_update2(wtk_qform_pickup_cfg_t *cfg,wtk_source_loader_t *sl)
{
    return wtk_qform_pickup_cfg_update(cfg);
}



wtk_qform_pickup_cfg_t* wtk_qform_pickup_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_qform_pickup_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_qform_pickup_cfg,cfg_fn);
	cfg=(wtk_qform_pickup_cfg_t*)main_cfg->cfg;
	cfg->main_cfg=main_cfg;
	return cfg;
}

void wtk_qform_pickup_cfg_delete(wtk_qform_pickup_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_qform_pickup_cfg_t* wtk_qform_pickup_cfg_new_bin(char *bin_fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_qform_pickup_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_qform_pickup_cfg,bin_fn,"./cfg");
	cfg=(wtk_qform_pickup_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_qform_pickup_cfg_delete_bin(wtk_qform_pickup_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}


void wtk_qform_pickup_cfg_set_theta_range(wtk_qform_pickup_cfg_t *cfg,float theta_range)
{
    cfg->theta_range=theta_range;
}

void wtk_qform_pickup_cfg_set_noise_suppress(wtk_qform_pickup_cfg_t *cfg,float noise_suppress)
{
    if(fabs(noise_suppress)>0.0)
    {
        wtk_qmmse_cfg_set_noise_suppress(&(cfg->qmmse),noise_suppress);
        cfg->use_post=1;
    }
}

