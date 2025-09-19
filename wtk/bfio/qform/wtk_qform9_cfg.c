#include "wtk_qform9_cfg.h" 

int wtk_qform9_cfg_init(wtk_qform9_cfg_t *cfg)
{
	wtk_stft2_cfg_init(&(cfg->stft2));
    wtk_covm_cfg_init(&(cfg->covm));
    wtk_bf_cfg_init(&(cfg->bf));
    wtk_qenvelope_cfg_init(&(cfg->qenvl));
    wtk_qenvelope_cfg_init(&(cfg->qenvl2));
    wtk_qenvelope_cfg_init(&(cfg->qenvl3));
    wtk_qmmse_cfg_init(&(cfg->qmmse));
	wtk_aspec_cfg_init(&(cfg->aspec));
    wtk_aspec_cfg_init(&(cfg->aspec_class1));
    wtk_aspec_cfg_init(&(cfg->aspec_class2));

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

    cfg->ntheta_range=45;
    cfg->ntheta_center=NULL;
    cfg->use_noiseblock=0;
    cfg->ntheta_num=0;
    cfg->use_noiseblock2=0;

    cfg->use_cline1=0;
    cfg->use_cline2=0;
    cfg->use_cline3=0;
    cfg->mic_class[0]=cfg->mic_class[1]=NULL;
    cfg->use_two_aspecclass=0;
    cfg->use_twojoin=0;
    cfg->theta_center_class1=0;
    cfg->theta_center_class2=0;
    cfg->theta_center_class3=0;
    cfg->theta_range_class=20;
    cfg->theta_range_class1=20;
    cfg->theta_range_class2=20;

    cfg->cohv_thresh=-1;

    cfg->use_specsum_bl=0;
    cfg->specsum_bl=0.01;

    cfg->use_qenvelope=0;

    cfg->use_nqenvelope=1;
    cfg->use_sqenvelope=1;
    cfg->use_t_r_qenvelope=0;
    cfg->use_simple_qenvelope=0;
    cfg->use_noise_qenvelope=0;
    cfg->use_two_channel=0;
    cfg->use_howl_suppression=0;

    cfg->t_r_qenvl = NULL;
    cfg->t_r_number = 0;
    cfg->qenvel_alpha = 0.8;
    cfg->use_cohv_cnt=0;

    cfg->use_noise_debug = 0;
    cfg->use_sound_debug = 0;
    cfg->noise_debug_cnt = 20;

	cfg->use_cnon=0;
	cfg->sym=1e-2;
	cfg->cnon_clip_s=0;
	cfg->cnon_clip_e=8000;

    cfg->delay_nf=0;

    cfg->ncohv_alpha=0.99;
    cfg->scohv_alpha=0.9;
    cfg->nscohv_scale=1.2;
    cfg->nscohv_scale2=2.0;
    cfg->nscohv_scale3=1.1;
	cfg->entropy_thresh=-1;
	cfg->entropy_in_cnt=2;
	cfg->entropy_cnt=20;
    cfg->cohv_init_frame=40;

    cfg->sil_in_cnt = 500;
    cfg->sil_out_cnt = 500;
    cfg->sil_max_range = 5;
    cfg->sil_noise_suppress = -75;
    cfg->use_qmmse_param = 0;

	cfg->max_out = 32700.0;
	cfg->use_bs = 0;
	cfg->use_bs_win = 0;
	cfg->out_agc_level = -1;
    return 0;
}

int wtk_qform9_cfg_clean(wtk_qform9_cfg_t *cfg)
{
    int i;
    if(cfg->ntheta_center)
    {
        wtk_free(cfg->ntheta_center);
    }
    wtk_stft2_cfg_clean(&(cfg->stft2));
    wtk_covm_cfg_clean(&(cfg->covm));
    wtk_bf_cfg_clean(&(cfg->bf));
    wtk_qenvelope_cfg_clean(&(cfg->qenvl));
    wtk_qenvelope_cfg_clean(&(cfg->qenvl2));
    wtk_qenvelope_cfg_clean(&(cfg->qenvl3));
    wtk_qmmse_cfg_clean(&(cfg->qmmse));
    wtk_aspec_cfg_clean(&(cfg->aspec));
    wtk_aspec_cfg_clean(&(cfg->aspec_class1));
    wtk_aspec_cfg_clean(&(cfg->aspec_class2));
    if(cfg->mic_class[0])
    {
        wtk_free(cfg->mic_class[0]);
        wtk_free(cfg->mic_class[1]);
    }
	if(cfg->t_r_qenvl)
	{
		for(i=0;i<cfg->t_r_number;++i)
		{
			wtk_free(cfg->t_r_qenvl[i]);
		}
		wtk_free(cfg->t_r_qenvl);
	}
	return 0;
}

int wtk_qform9_cfg_update_local(wtk_qform9_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
    wtk_local_cfg_t *m;
    int ret;
    wtk_array_t *ntheta_array;
    int i;

    wtk_local_cfg_update_cfg_b(lc,cfg,use_qenvelope,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_nqenvelope,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_sqenvelope,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_t_r_qenvelope,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_simple_qenvelope,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_noise_qenvelope,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,qenvel_alpha,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_cohv_cnt,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_specsum_bl,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_bl,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_cline1,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_cline2,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_cline3,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_two_aspecclass,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_twojoin,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_noiseblock,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,ntheta_range,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_noiseblock2,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_fs,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_fe,v);

    wtk_local_cfg_update_cfg_i(lc,cfg,lt,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,lf,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,cohv_thresh,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,theta_range,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_post,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_preemph,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_two_channel,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_howl_suppression,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,theta_center_class1,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,theta_center_class2,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,theta_center_class3,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,theta_range_class,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,theta_range_class1,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,theta_range_class2,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_noise_debug,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,noise_debug_cnt,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_sound_debug,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,use_cnon,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sym,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,cnon_clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,cnon_clip_e,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,delay_nf,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,ncohv_alpha,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,scohv_alpha,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,nscohv_scale,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,nscohv_scale2,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,nscohv_scale3,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,entropy_thresh,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,entropy_in_cnt,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,entropy_cnt,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,cohv_init_frame,v);

    wtk_local_cfg_update_cfg_i(lc,cfg,sil_in_cnt,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,sil_out_cnt,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,sil_max_range,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,sil_noise_suppress,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse_param,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,max_out,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_bs,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_bs_win,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,out_agc_level,v);

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
    if(cfg->use_two_channel || cfg->use_noise_qenvelope){
        lc=wtk_local_cfg_find_lc_s(m,"qenvelope2");
        if(lc)
        {
            ret=wtk_qenvelope_cfg_update_local(&(cfg->qenvl2),lc);
            if(ret!=0){goto end;}
        }
    }
    if(cfg->use_noise_qenvelope){
        lc=wtk_local_cfg_find_lc_s(m,"qenvelope3");
        if(lc)
        {
            ret=wtk_qenvelope_cfg_update_local(&(cfg->qenvl3),lc);
            if(ret!=0){goto end;}
        }
    }
    if(cfg->use_t_r_qenvelope){
        lc=wtk_local_cfg_find_lc_s(m,"theta_range_qenvelope");
        if(lc)
        {
            wtk_queue_node_t *qn;
            wtk_cfg_item_t *item;
            int i;
            int type=6;

            cfg->t_r_qenvl=(float**)wtk_malloc(sizeof(float*)*lc->cfg->queue.length);
            cfg->t_r_number=0;
            for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
            {
                item=data_offset2(qn,wtk_cfg_item_t,n);
                if(item->type!=WTK_CFG_ARRAY || item->value.array->nslot!=type){continue;}
                cfg->t_r_qenvl[cfg->t_r_number]=(float*)wtk_malloc(sizeof(float)*type);
                for(i=0;i<type;++i)
                {
                    v=((wtk_string_t**)item->value.array->slot)[i];
                    cfg->t_r_qenvl[cfg->t_r_number][i]=wtk_str_atof(v->data,v->len);
                    // wtk_debug("v[%d][%d]=%f\n",cfg->t_r_number,i,cfg->t_r_qenvl[cfg->t_r_number][i]);
                }
                ++cfg->t_r_number;
            }
        }
    }
    lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
        cfg->qmmse.step=cfg->stft2.win/2;
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
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

int wtk_qform9_cfg_update(wtk_qform9_cfg_t *cfg)
{
    int ret;

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
    ret=wtk_qenvelope_cfg_update(&(cfg->qenvl2));
    if(ret!=0){goto end;}
    ret=wtk_qenvelope_cfg_update(&(cfg->qenvl3));
    if(ret!=0){goto end;}
    ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
    if(ret!=0){goto end;}
    ret=wtk_aspec_cfg_update(&(cfg->aspec_class1));
    if(ret!=0){goto end;}
    ret=wtk_aspec_cfg_update(&(cfg->aspec_class2));
    if(ret!=0){goto end;}
    cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ne=min(cfg->stft2.win/2-1, cfg->specsum_ne);
	cfg->cnon_clip_s=(cfg->cnon_clip_s*1.0*cfg->stft2.win)/cfg->rate;
	cfg->cnon_clip_e=(cfg->cnon_clip_e*1.0*cfg->stft2.win)/cfg->rate;
    // cfg->specsum_ns+=cfg->lf;
    // cfg->specsum_ne-=cfg->lf;
	if(cfg->out_agc_level==1){
		cfg->max_out=pow(10, 75.0/20.0);
	}else if(cfg->out_agc_level==2){
		cfg->max_out=pow(10, 78.0/20.0);
	}else if(cfg->out_agc_level==3){
		cfg->max_out=pow(10, 81.0/20.0);
	}else if(cfg->out_agc_level==4){
		cfg->max_out=pow(10, 84.0/20.0);
	}else if(cfg->out_agc_level==5){
		cfg->max_out=pow(10, 87.0/20.0);
	}else if(cfg->out_agc_level>=6){
		cfg->max_out=pow(10, 90.0/20.0);
	}

    ret=0;
end:
    return ret;
}

int wtk_qform9_cfg_update2(wtk_qform9_cfg_t *cfg,wtk_source_loader_t *sl)
{
    int ret;

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
    ret=wtk_qenvelope_cfg_update(&(cfg->qenvl2));
    if(ret!=0){goto end;}
    ret=wtk_qenvelope_cfg_update(&(cfg->qenvl3));
    if(ret!=0){goto end;}
    ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
    if(ret!=0){goto end;}
    ret=wtk_aspec_cfg_update(&(cfg->aspec_class1));
    if(ret!=0){goto end;}
    ret=wtk_aspec_cfg_update(&(cfg->aspec_class2));
    if(ret!=0){goto end;}
    cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ne=min(cfg->stft2.win/2-1, cfg->specsum_ne);
	cfg->cnon_clip_s=(cfg->cnon_clip_s*1.0*cfg->stft2.win)/cfg->rate;
	cfg->cnon_clip_e=(cfg->cnon_clip_e*1.0*cfg->stft2.win)/cfg->rate;
    // cfg->specsum_ns+=cfg->lf;
    // cfg->specsum_ne-=cfg->lf;
	if(cfg->out_agc_level==1){
		cfg->max_out=pow(10, 75.0/20.0);
	}else if(cfg->out_agc_level==2){
		cfg->max_out=pow(10, 78.0/20.0);
	}else if(cfg->out_agc_level==3){
		cfg->max_out=pow(10, 81.0/20.0);
	}else if(cfg->out_agc_level==4){
		cfg->max_out=pow(10, 84.0/20.0);
	}else if(cfg->out_agc_level==5){
		cfg->max_out=pow(10, 87.0/20.0);
	}else if(cfg->out_agc_level>=6){
		cfg->max_out=pow(10, 90.0/20.0);
	}

    ret=0;
end:
    return ret;
}

wtk_qform9_cfg_t* wtk_qform9_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_qform9_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_qform9_cfg,cfg_fn);
	cfg=(wtk_qform9_cfg_t*)main_cfg->cfg;
	cfg->main_cfg=main_cfg;
	return cfg;
}

void wtk_qform9_cfg_delete(wtk_qform9_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_qform9_cfg_t* wtk_qform9_cfg_new_bin(char *bin_fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_qform9_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_qform9_cfg,bin_fn,"./cfg");
	cfg=(wtk_qform9_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_qform9_cfg_delete_bin(wtk_qform9_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}


void wtk_qform9_cfg_set_theta_range(wtk_qform9_cfg_t *cfg,float theta_range)
{
    cfg->theta_range=theta_range;
}

void wtk_qform9_cfg_set_noise_suppress(wtk_qform9_cfg_t *cfg,float noise_suppress)
{
    if(fabs(noise_suppress)>0.0)
    {
        wtk_qmmse_cfg_set_noise_suppress(&(cfg->qmmse),noise_suppress);
        cfg->use_post=1;
    }
}
