#include "wtk_qform_pickup1_cfg.h" 

int wtk_qform_pickup1_cfg_init(wtk_qform_pickup1_cfg_t *cfg)
{
	wtk_stft2_cfg_init(&(cfg->stft2));
    wtk_bf_cfg_init(&(cfg->bf));
	wtk_aspec_cfg_init(&(cfg->aspec));
    wtk_aspec_cfg_init(&(cfg->aspec_class1));
    wtk_aspec_cfg_init(&(cfg->aspec_class2));

    cfg->notch_radius=0;
	cfg->preemph=0;

    cfg->debug=1;
    cfg->theta_range=45;

    cfg->rate=16000;


    cfg->lf=6;
    cfg->lt=1;

    cfg->ncov_alpha=0.1;
    cfg->init_ncovnf=100;

    cfg->scov_alpha=0.1;
    cfg->init_scovnf=100;

    cfg->flushcohvgap=1;
    cfg->flushcovgap=3;

	cfg->use_covhist=1;
	cfg->cov_hist=30;
    cfg->batch=10;

    cfg->use_line=0;

    cfg->ntheta_range=45;
    cfg->ntheta_center=NULL;
    cfg->use_noiseblock=0;
    cfg->ntheta_num=0;
    cfg->use_noiseblock2=0;

    cfg->cohv_thresh=-1;

    cfg->specsum_fs=0;
    cfg->specsum_fe=8000;

    cfg->use_cline1=0;
    cfg->use_cline2=0;
    cfg->mic_class[0]=cfg->mic_class[1]=NULL;
    cfg->use_two_aspecclass=0;

    return 0;
}

int wtk_qform_pickup1_cfg_clean(wtk_qform_pickup1_cfg_t *cfg)
{
    if(cfg->ntheta_center)
    {
        wtk_free(cfg->ntheta_center);
    }
    wtk_stft2_cfg_clean(&(cfg->stft2));
    wtk_bf_cfg_clean(&(cfg->bf));
    wtk_aspec_cfg_clean(&(cfg->aspec));
    wtk_aspec_cfg_clean(&(cfg->aspec_class1));
    wtk_aspec_cfg_clean(&(cfg->aspec_class2));
    if(cfg->mic_class[0])
    {
        wtk_free(cfg->mic_class[0]);
        wtk_free(cfg->mic_class[1]);
    }

	return 0;
}

int wtk_qform_pickup1_cfg_update_local(wtk_qform_pickup1_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
    wtk_local_cfg_t *m;
    int ret;
    wtk_array_t *ntheta_array;
    int i;

    wtk_local_cfg_update_cfg_b(lc,cfg,use_cline1,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_cline2,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_two_aspecclass,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,notch_radius,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,preemph,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_noiseblock,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,ntheta_range,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_noiseblock2,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_fs,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_fe,v);

    wtk_local_cfg_update_cfg_i(lc,cfg,lt,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,lf,v);

    wtk_local_cfg_update_cfg_i(lc,cfg,flushcovgap,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_covhist,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,cov_hist,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,batch,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,init_ncovnf,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,ncov_alpha,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,cohv_thresh,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,theta_range,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,flushcohvgap,v);

    wtk_local_cfg_update_cfg_i(lc,cfg,init_scovnf,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,scov_alpha,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,theta_range,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	ntheta_array=wtk_local_cfg_find_float_array_s(lc,"ntheta_center");
    if(ntheta_array)
    {
        cfg->ntheta_num=ntheta_array->nslot;
        cfg->ntheta_center=(float *)wtk_malloc(cfg->ntheta_num*sizeof(float));
        for(i=0;i<cfg->ntheta_num;++i)
        {
            cfg->ntheta_center[i]=((float *)(ntheta_array->slot))[i];
        }
    }

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
    lc=wtk_local_cfg_find_lc_s(m,"bf");
	if(lc)
	{
        ret=wtk_bf_cfg_update_local(&(cfg->bf),lc);
        if(ret!=0){goto end;}
    }
      lc=wtk_local_cfg_find_lc_s(m,"aspec_class1");
	if(lc)
	{
        ret=wtk_aspec_cfg_update_local(&(cfg->aspec_class1),lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(m,"aspec_class2");
	if(lc)
	{
        ret=wtk_aspec_cfg_update_local(&(cfg->aspec_class2),lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(m,"mic_class");
	if(lc)
	{
		wtk_queue_node_t *qn;
		wtk_cfg_item_t *item;
		int i;
        int n;

        n=0;
		for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			if(item->type!=WTK_CFG_ARRAY){continue;}
			cfg->mic_class[n]=(float*)wtk_malloc(sizeof(float)*item->value.array->nslot);
			for(i=0;i<item->value.array->nslot;++i)
			{
				v=((wtk_string_t**)item->value.array->slot)[i];
				cfg->mic_class[n][i]=wtk_str_atof(v->data,v->len);
				//wtk_debug("v[%d][%d]=%f\n",cfg->nmic,i,cfg->mic_pos[cfg->nmic][i]);
			}
            ++n;
		}
	}


	ret=0;
end:
    return ret;
}

int wtk_qform_pickup1_cfg_update(wtk_qform_pickup1_cfg_t *cfg)
{
    int ret;

    ret=wtk_stft2_cfg_update(&(cfg->stft2));
    if(ret!=0){goto end;}
	ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}
    ret=wtk_bf_cfg_update(&(cfg->bf));
    if(ret!=0){goto end;}
    ret=wtk_aspec_cfg_update(&(cfg->aspec_class1));
    if(ret!=0){goto end;}
    ret=wtk_aspec_cfg_update(&(cfg->aspec_class2));
    if(ret!=0){goto end;}

    cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ne=min(cfg->stft2.win/2-1, cfg->specsum_ne);
    cfg->notch_radius_den=cfg->notch_radius*cfg->notch_radius+0.7*(1-cfg->notch_radius)*(1-cfg->notch_radius);


    ret=0;
end:
    return ret;
}

int wtk_qform_pickup1_cfg_update2(wtk_qform_pickup1_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return wtk_qform_pickup1_cfg_update(cfg);
}

wtk_qform_pickup1_cfg_t* wtk_qform_pickup1_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_qform_pickup1_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_qform_pickup1_cfg,cfg_fn);
	cfg=(wtk_qform_pickup1_cfg_t*)main_cfg->cfg;
	cfg->main_cfg=main_cfg;
	return cfg;
}

void wtk_qform_pickup1_cfg_delete(wtk_qform_pickup1_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_qform_pickup1_cfg_t* wtk_qform_pickup1_cfg_new_bin(char *bin_fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_qform_pickup1_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_qform_pickup1_cfg,bin_fn,"./cfg");
	cfg=(wtk_qform_pickup1_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_qform_pickup1_cfg_delete_bin(wtk_qform_pickup1_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}


void wtk_qform_pickup1_cfg_set_theta_range(wtk_qform_pickup1_cfg_t *cfg,float theta_range)
{
    cfg->theta_range=theta_range;
}
