#include "wtk_qform11_cfg.h" 

int wtk_qform11_cfg_init(wtk_qform11_cfg_t *cfg)
{
	wtk_stft2_cfg_init(&(cfg->stft2));
    wtk_covm_cfg_init(&(cfg->covm));
    wtk_bf_cfg_init(&(cfg->bf));
    wtk_qenvelope_cfg_init(&(cfg->qenvl));
    wtk_qmmse_cfg_init(&(cfg->qmmse));
	wtk_aspec_cfg_init(&(cfg->aspec));
	cfg->nmulchannel=0;
	cfg->mul_theta_range=NULL;
    cfg->aspec_theta=NULL;
    cfg->naspec=0;
    cfg->tstep=5;
    cfg->out_len=512;

    cfg->debug=0;
    cfg->theta_range=45;

    cfg->rate=16000;

    cfg->use_post=1;

    cfg->use_preemph=1;

    cfg->lf=6;
    cfg->lt=1;

    cfg->specsum_fs=0;
    cfg->specsum_fe=8000;

    cfg->use_line=0;

    cfg->cohv_thresh=-1;

    cfg->use_qenvelope=0;

    cfg->use_sqenvelope=1;

    return 0;
}

int wtk_qform11_cfg_clean(wtk_qform11_cfg_t *cfg)
{
    wtk_stft2_cfg_clean(&(cfg->stft2));
    wtk_covm_cfg_clean(&(cfg->covm));
    wtk_bf_cfg_clean(&(cfg->bf));
    wtk_qenvelope_cfg_clean(&(cfg->qenvl));
    wtk_qmmse_cfg_clean(&(cfg->qmmse));
    wtk_aspec_cfg_clean(&(cfg->aspec));
	int i;

	if(cfg->mul_theta_range)
	{
		for(i=0;i<cfg->nmulchannel;++i)
		{
			wtk_free(cfg->mul_theta_range[i]);
		}
		wtk_free(cfg->mul_theta_range);
	}
    if(cfg->aspec_theta){
        wtk_free(cfg->aspec_theta);
    }
	return 0;
}

int wtk_qform11_cfg_update_local(wtk_qform11_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
    wtk_local_cfg_t *m;
    int ret;

    wtk_local_cfg_update_cfg_b(lc,cfg,use_qenvelope,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_sqenvelope,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_fs,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_fe,v);

    wtk_local_cfg_update_cfg_i(lc,cfg,lt,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,lf,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,cohv_thresh,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,theta_range,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,tstep,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,out_len,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_post,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_preemph,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);


    m=lc;
	lc=wtk_local_cfg_find_lc_s(m,"stft2");
	if(lc)
	{
        ret=wtk_stft2_cfg_update_local(&(cfg->stft2),lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(m,"aspec");
	if(lc)
	{
        ret=wtk_aspec_cfg_update_local(&(cfg->aspec),lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(m,"covm");
	if(lc)
	{
        ret=wtk_covm_cfg_update_local(&(cfg->covm),lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(m,"bf");
	if(lc)
	{
        ret=wtk_bf_cfg_update_local(&(cfg->bf),lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(m,"qenvelope");
	if(lc)
	{
        ret=wtk_qenvelope_cfg_update_local(&(cfg->qenvl),lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
        cfg->qmmse.step=cfg->stft2.win/2;
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"mul_theta_range");
	if(lc)
	{
		wtk_queue_node_t *qn;
		wtk_cfg_item_t *item;
		int i;

		cfg->mul_theta_range=(float**)wtk_malloc(sizeof(float*)*lc->cfg->queue.length);
		cfg->nmulchannel=0;
		for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			if(item->type!=WTK_CFG_ARRAY || item->value.array->nslot!=2){continue;}
			cfg->mul_theta_range[cfg->nmulchannel]=(float*)wtk_malloc(sizeof(float)*6);
			for(i=0;i<2;++i)
			{
				v=((wtk_string_t**)item->value.array->slot)[i];
				cfg->mul_theta_range[cfg->nmulchannel][i]=wtk_str_atof(v->data,v->len);
				//wtk_debug("v[%d][%d]=%f\n",cfg->nmic,i,cfg->mic_pos[cfg->nmic][i]);
			}
			++cfg->nmulchannel;
		}
	}

	ret=0;
end:
    return ret;
}

int wtk_qform11_cfg_update(wtk_qform11_cfg_t *cfg)
{
    int ret;
    int i,j;
    float theta=0;
    float range;
    float max_theta;
    float theta1,theta2;

    ret=wtk_stft2_cfg_update(&(cfg->stft2));
    if(ret!=0){goto end;}
	ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}
    ret=wtk_covm_cfg_update(&(cfg->covm));
    if(ret!=0){goto end;}
    ret=wtk_bf_cfg_update(&(cfg->bf));
    if(ret!=0){goto end;}
    ret=wtk_qenvelope_cfg_update(&(cfg->qenvl));
    if(ret!=0){goto end;}
    ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
    if(ret!=0){goto end;}
    cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ne=min(cfg->stft2.win/2-1, cfg->specsum_ne);
    // cfg->specsum_ns+=cfg->lf;
    // cfg->specsum_ne-=cfg->lf;
    if(cfg->nmulchannel==0){
        cfg->nmulchannel=1;
    }
    cfg->naspec=cfg->use_line?floor(180/cfg->tstep)+1:floor(360/cfg->tstep);
    cfg->aspec_theta=(float *)wtk_malloc(sizeof(float)*cfg->naspec);
    for(i=0;i<cfg->naspec;++i){
        cfg->aspec_theta[i] = theta;
        theta += cfg->tstep;
    }
    max_theta=cfg->use_line?180:359;
    for(i=0;i<cfg->nmulchannel;++i){
        theta = cfg->mul_theta_range[i][0];
        range = cfg->mul_theta_range[i][1];
        theta1 = max(0, theta-range-cfg->tstep);
        theta2 = min(max_theta, theta+range+cfg->tstep);
        cfg->mul_theta_range[i][2]=theta1;
        cfg->mul_theta_range[i][3]=theta2;
        for(j=0;j<cfg->naspec;++j){
            if(cfg->aspec_theta[j]>theta1){
                cfg->mul_theta_range[i][4]=j+1;
                break;
            }
        }
        for(j=cfg->naspec-1;j>=0;--j){
            if(cfg->aspec_theta[j]<theta2){
                cfg->mul_theta_range[i][5]=j-1;
                break;
            }
        }
    }

    ret=0;
end:
    return ret;
}

int wtk_qform11_cfg_update2(wtk_qform11_cfg_t *cfg,wtk_source_loader_t *sl)
{
    int ret;
    ret = wtk_qform11_cfg_update(cfg);
    if(ret!=0){goto end;}

    ret=0;
end:
    return ret;
}

wtk_qform11_cfg_t* wtk_qform11_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_qform11_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_qform11_cfg,cfg_fn);
	cfg=(wtk_qform11_cfg_t*)main_cfg->cfg;
	cfg->main_cfg=main_cfg;
	return cfg;
}

void wtk_qform11_cfg_delete(wtk_qform11_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_qform11_cfg_t* wtk_qform11_cfg_new_bin(char *bin_fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_qform11_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_qform11_cfg,bin_fn,"./cfg");
	cfg=(wtk_qform11_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_qform11_cfg_delete_bin(wtk_qform11_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}


void wtk_qform11_cfg_set_theta_range(wtk_qform11_cfg_t *cfg,float theta_range)
{
    cfg->theta_range=theta_range;
}

void wtk_qform11_cfg_set_mul_theta_range(wtk_qform11_cfg_t *cfg,float theta_range, int idx)
{
    if(idx<cfg->nmulchannel){
        cfg->mul_theta_range[idx][1] = theta_range;
    }
}

void wtk_qform11_cfg_set_noise_suppress(wtk_qform11_cfg_t *cfg,float noise_suppress)
{
    if(fabs(noise_suppress)>0.0)
    {
        wtk_qmmse_cfg_set_noise_suppress(&(cfg->qmmse),noise_suppress);
        cfg->use_post=1;
    }
}
