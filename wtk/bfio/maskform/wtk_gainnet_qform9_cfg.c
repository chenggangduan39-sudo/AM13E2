#include "wtk_gainnet_qform9_cfg.h" 

int wtk_gainnet_qform9_cfg_init(wtk_gainnet_qform9_cfg_t *cfg)
{
	wtk_stft2_cfg_init(&(cfg->stft2));
    wtk_covm_cfg_init(&(cfg->covm));
    wtk_bf_cfg_init(&(cfg->bf));
    wtk_qenvelope_cfg_init(&(cfg->qenvl));
    wtk_qenvelope_cfg_init(&(cfg->qenvl2));
    wtk_qenvelope_cfg_init(&(cfg->qenvl3));
    wtk_qmmse_cfg_init(&(cfg->qmmse));
	wtk_qmmse_cfg_init(&(cfg->mdl_qmmse));
	wtk_aspec_cfg_init(&(cfg->aspec));

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

    cfg->use_cline1=0;
    cfg->use_cline2=0;
    cfg->mic_class[0]=cfg->mic_class[1]=NULL;
    cfg->theta_center_class1=0;
    cfg->theta_center_class2=0;
    cfg->theta_center_class3=0;
    cfg->theta_range_class=20;
    cfg->theta_range_class1=20;
    cfg->theta_range_class2=20;

    cfg->cohv_thresh=-1;

    cfg->use_qenvelope=0;

    cfg->use_sqenvelope=1;
    cfg->use_t_r_qenvelope=0;
    cfg->use_simple_qenvelope=0;
    cfg->use_noise_qenvelope=0;
    cfg->use_two_channel=0;

    cfg->t_r_qenvl = NULL;
    cfg->t_r_number = 0;
    cfg->qenvel_alpha = 0.8;

    cfg->mdl_fn=NULL;
	cfg->gainnet2=NULL;
    cfg->use_rbin_res=0;
	wtk_bankfeat_cfg_init(&(cfg->bankfeat));

	cfg->ralpha=0;
	cfg->gbias=0;
	cfg->clip_s=0;
	cfg->clip_e=8000;
	cfg->featm_lm=1;

    return 0;
}

int wtk_gainnet_qform9_cfg_clean(wtk_gainnet_qform9_cfg_t *cfg)
{
    int i;
    wtk_stft2_cfg_clean(&(cfg->stft2));
    wtk_covm_cfg_clean(&(cfg->covm));
    wtk_bf_cfg_clean(&(cfg->bf));
    wtk_qenvelope_cfg_clean(&(cfg->qenvl));
    wtk_qenvelope_cfg_clean(&(cfg->qenvl2));
    wtk_qenvelope_cfg_clean(&(cfg->qenvl3));
    wtk_qmmse_cfg_clean(&(cfg->qmmse));
	wtk_qmmse_cfg_clean(&(cfg->mdl_qmmse));
    wtk_aspec_cfg_clean(&(cfg->aspec));
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
	if(cfg->gainnet2)
	{
		if(cfg->use_rbin_res)
		{
			wtk_gainnet2_cfg_delete_bin3(cfg->gainnet2);
		}else
		{
			wtk_gainnet2_cfg_delete_bin2(cfg->gainnet2);
		}
	}
	wtk_bankfeat_cfg_clean(&(cfg->bankfeat));
	return 0;
}

int wtk_gainnet_qform9_cfg_update_local(wtk_gainnet_qform9_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
    wtk_local_cfg_t *m;
    int ret;

    wtk_local_cfg_update_cfg_b(lc,cfg,use_qenvelope,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_sqenvelope,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_t_r_qenvelope,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_simple_qenvelope,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_noise_qenvelope,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,qenvel_alpha,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_cline1,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_cline2,v);

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

    wtk_local_cfg_update_cfg_f(lc,cfg,theta_center_class1,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,theta_center_class2,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,theta_center_class3,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,theta_range_class,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,theta_range_class1,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,theta_range_class2,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

    wtk_local_cfg_update_cfg_str(lc,cfg,mdl_fn,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,gbias,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,ralpha,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,clip_e,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,featm_lm,v);

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
	lc=wtk_local_cfg_find_lc_s(m,"mdl_qmmse");
	if(lc)
	{
        ret=wtk_qmmse_cfg_update_local(&(cfg->mdl_qmmse),lc);
        cfg->qmmse.step=cfg->stft2.win/2;
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
	lc=wtk_local_cfg_find_lc_s(m,"bankfeat");
	if(lc)
	{
		ret=wtk_bankfeat_cfg_update_local(&(cfg->bankfeat),lc);
		if(ret!=0){goto end;}
	}

	ret=0;
end:
    return ret;
}

int wtk_gainnet_qform9_cfg_update(wtk_gainnet_qform9_cfg_t *cfg)
{
    int ret;

	if(cfg->mdl_fn)
	{
		cfg->gainnet2=wtk_gainnet2_cfg_new_bin2(cfg->mdl_fn);
		if(!cfg->gainnet2){ret=-1;goto end;}
	}

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
	ret=wtk_qmmse_cfg_update(&(cfg->mdl_qmmse));
	if(ret!=0){goto end;}
	ret=wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if(ret!=0){goto end;}
    cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ne=min(cfg->stft2.win/2-1, cfg->specsum_ne);
    // cfg->specsum_ns+=cfg->lf;
    // cfg->specsum_ne-=cfg->lf;
	cfg->clip_s=(cfg->clip_s*1.0*cfg->stft2.win)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->stft2.win)/cfg->rate;

    ret=0;
end:
    return ret;
}

int wtk_gainnet_qform9_cfg_update2(wtk_gainnet_qform9_cfg_t *cfg,wtk_source_loader_t *sl)
{
	wtk_rbin2_item_t *item;
	wtk_rbin2_t *rbin=(wtk_rbin2_t*)(sl->hook);
    int ret;

	cfg->use_rbin_res=1;
	if(cfg->mdl_fn)
	{
		item=wtk_rbin2_get(rbin,cfg->mdl_fn,strlen(cfg->mdl_fn));
		if(!item){ret=-1;goto end;}
		cfg->gainnet2=wtk_gainnet2_cfg_new_bin3(rbin->fn,item->pos);
		if(!cfg->gainnet2){ret=-1;goto end;}
	}
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
	ret=wtk_qmmse_cfg_update(&(cfg->mdl_qmmse));
	if(ret!=0){goto end;}
	ret=wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if(ret!=0){goto end;}
    cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ne=min(cfg->stft2.win/2-1, cfg->specsum_ne);
    // cfg->specsum_ns+=cfg->lf;
    // cfg->specsum_ne-=cfg->lf;
	cfg->clip_s=(cfg->clip_s*1.0*cfg->stft2.win)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->stft2.win)/cfg->rate;

    ret=0;
end:
    return ret;
}

wtk_gainnet_qform9_cfg_t* wtk_gainnet_qform9_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_gainnet_qform9_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_gainnet_qform9_cfg,cfg_fn);
	cfg=(wtk_gainnet_qform9_cfg_t*)main_cfg->cfg;
	cfg->main_cfg=main_cfg;
	return cfg;
}

void wtk_gainnet_qform9_cfg_delete(wtk_gainnet_qform9_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_gainnet_qform9_cfg_t* wtk_gainnet_qform9_cfg_new_bin(char *bin_fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_gainnet_qform9_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_gainnet_qform9_cfg,bin_fn,"./cfg");
	cfg=(wtk_gainnet_qform9_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_gainnet_qform9_cfg_delete_bin(wtk_gainnet_qform9_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}


void wtk_gainnet_qform9_cfg_set_theta_range(wtk_gainnet_qform9_cfg_t *cfg,float theta_range)
{
    cfg->theta_range=theta_range;
}

void wtk_gainnet_qform9_cfg_set_noise_suppress(wtk_gainnet_qform9_cfg_t *cfg,float noise_suppress)
{
    if(fabs(noise_suppress)>0.0)
    {
        wtk_qmmse_cfg_set_noise_suppress(&(cfg->qmmse),noise_suppress);
        cfg->use_post=1;
    }
}
