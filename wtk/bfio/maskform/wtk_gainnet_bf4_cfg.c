#include "wtk_gainnet_bf4_cfg.h"

int wtk_gainnet_bf4_cfg_init(wtk_gainnet_bf4_cfg_t *cfg)
{
	cfg->channel=0;
	cfg->nmicchannel=0;
	cfg->mic_channel=NULL;
	cfg->nspchannel=0;
	cfg->sp_channel=NULL;

    cfg->wins=320;

    cfg->ceps_mem=8;
    cfg->nb_delta_ceps=6;

    cfg->nb_bands=0;  

    cfg->mdl_fn=NULL;
    cfg->gainnet=NULL;
    cfg->use_rbin_res=0;

	wtk_aspec_cfg_init(&(cfg->aspec));
	wtk_covm_cfg_init(&(cfg->covm));
	wtk_bf_cfg_init(&(cfg->bf));
	wtk_rls_cfg_init(&(cfg->echo_rls));
	cfg->theta = 180;
	cfg->phi=0;

	wtk_qmmse_cfg_init(&(cfg->preqmmse));
	cfg->use_preqmmse=0;

	cfg->use_preemph=1;

	cfg->spenr_thresh=100;
	cfg->spenr_cnt=10;

	cfg->micenr_thresh=300;
	cfg->micenr_cnt=10;

	cfg->use_miccnon=0;
	cfg->use_spcnon=0;

	cfg->micnenr=0.5;
	cfg->spnenr=0.5;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->use_postsingle=0;

	cfg->use_ceps=1;

	cfg->fft_scale=1;

	cfg->rate=16000;

	cfg->agc_a=0.69;
	cfg->agc_b=6.9;

	cfg->pframe_fs=200;
	cfg->pframe_fe=8000;
	cfg->pframe_alpha=0.2;
	cfg->pframe_thresh=0.1;

	cfg->lf=6;
    cfg->lt=1;
    cfg->theta_range=45;
	
	cfg->use_line=0;

	wtk_qmmse_cfg_init(&(cfg->qmmse));
	cfg->use_qmmse=0;

    cfg->use_qenvelope=0;
    cfg->use_sqenvelope=1;
    cfg->use_t_r_qenvelope=0;
    cfg->use_simple_qenvelope=0;
    wtk_qenvelope_cfg_init(&(cfg->qenvl));
	cfg->specsum_fs=0;
	cfg->specsum_fe=8000;

	cfg->use_fftsbf=0;

	cfg->use_maskssl2=0;
	wtk_maskssl2_cfg_init(&(cfg->maskssl2));
	cfg->use_ssl_delay=0;

    cfg->t_r_qenvl = NULL;
    cfg->t_r_number = 0;
    cfg->qenvel_alpha = 0.8;

	return 0;
}

int wtk_gainnet_bf4_cfg_clean(wtk_gainnet_bf4_cfg_t *cfg)
{
	int i;
	if(cfg->mic_channel)
	{
		wtk_free(cfg->mic_channel);
	}
	if(cfg->sp_channel)
	{
		wtk_free(cfg->sp_channel);
	}
	if(cfg->gainnet)
	{
		if(cfg->use_rbin_res)
		{
			wtk_gainnet5_cfg_delete_bin3(cfg->gainnet);
		}else
		{
			wtk_gainnet5_cfg_delete_bin2(cfg->gainnet);
		}
	}
	wtk_qmmse_cfg_clean(&(cfg->preqmmse));
	wtk_rls_cfg_clean(&(cfg->echo_rls));
	wtk_bf_cfg_clean(&(cfg->bf));
	wtk_covm_cfg_clean(&(cfg->covm));
	wtk_qmmse_cfg_clean(&(cfg->qmmse));
    wtk_aspec_cfg_clean(&(cfg->aspec));
	wtk_qenvelope_cfg_clean(&(cfg->qenvl));
	wtk_maskssl2_cfg_clean(&(cfg->maskssl2));

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

int wtk_gainnet_bf4_cfg_update_local(wtk_gainnet_bf4_cfg_t *cfg,wtk_local_cfg_t *m)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;
	wtk_array_t *a;
	int i;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,ceps_mem,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nb_delta_ceps,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nb_bands,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,mdl_fn,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,theta,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,phi,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_preemph,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,spenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,spenr_cnt,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_miccnon,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_spcnon,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,micenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,micenr_cnt,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_postsingle,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,micnenr,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,spnenr,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_ceps,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,fft_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agc_a,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agc_b,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,pframe_fs,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pframe_fe,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pframe_alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pframe_thresh,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_fftsbf,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,lt,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,lf,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,theta_range,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_preqmmse,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_qenvelope,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_sqenvelope,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_t_r_qenvelope,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_simple_qenvelope,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,qenvel_alpha,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,specsum_fs,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,specsum_fe,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_maskssl2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ssl_delay,v);

	a=wtk_local_cfg_find_array_s(lc,"mic_channel");
	if(a)
	{
		cfg->mic_channel=(int*)wtk_malloc(sizeof(int)*a->nslot);
		cfg->nmicchannel=a->nslot;
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->mic_channel[i]=wtk_str_atoi(v->data,v->len);
		}
	}
	a=wtk_local_cfg_find_array_s(lc,"sp_channel");
	if(a)
	{
		cfg->sp_channel=(int*)wtk_malloc(sizeof(int)*a->nslot);
		cfg->nspchannel=a->nslot;
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->sp_channel[i]=wtk_str_atoi(v->data,v->len);
		}
	}

	lc=wtk_local_cfg_find_lc_s(m,"echo_rls");
	if(lc)
	{
		wtk_rls_cfg_update_local(&(cfg->echo_rls),lc);
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
		wtk_bf_cfg_update_local(&(cfg->bf),lc);
	}
	lc=wtk_local_cfg_find_lc_s(m,"covm");
	if(lc)
	{
        ret=wtk_covm_cfg_update_local(&(cfg->covm),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"preqmmse");
	if(lc)
	{
        ret=wtk_qmmse_cfg_update_local(&(cfg->preqmmse),lc);
		cfg->preqmmse.step=cfg->wins/2;
        if(ret!=0){goto end;}
    }

	lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
		cfg->qmmse.step=cfg->wins/2;
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"qenvelope");
	if(lc)
	{
        ret=wtk_qenvelope_cfg_update_local(&(cfg->qenvl),lc);
        if(ret!=0){goto end;}
    }
	if(cfg->use_maskssl2){
		lc=wtk_local_cfg_find_lc_s(m,"maskssl2");
		if(lc)
		{
			ret=wtk_maskssl2_cfg_update_local(&(cfg->maskssl2),lc);
			cfg->maskssl2.wins=cfg->wins;
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
	ret=0;
end:
	return ret;
}

int wtk_gainnet_bf4_cfg_update(wtk_gainnet_bf4_cfg_t *cfg)
{
	int ret;

	if(cfg->use_ceps)
	{
		cfg->nb_features_x=cfg->nb_bands+2*cfg->nb_delta_ceps+1;
	}else
	{
		cfg->nb_features_x=cfg->nb_bands;
	}
	cfg->nb_features=cfg->nb_features_x*2;

	if(cfg->mdl_fn)
	{
		cfg->gainnet=wtk_gainnet5_cfg_new_bin2(cfg->mdl_fn);
		if(!cfg->gainnet){ret=-1;goto end;}
	}

	cfg->pframe_fs=max(1, cfg->pframe_fs/(cfg->rate*1.0/cfg->wins));
	cfg->pframe_fe=min(cfg->wins/2-2, cfg->pframe_fe/(cfg->rate*1.0/cfg->wins));

	cfg->specsum_fs=max(1, cfg->specsum_fs/(cfg->rate*1.0/cfg->wins));
	cfg->specsum_fe=min(cfg->wins/2-2, cfg->specsum_fe/(cfg->rate*1.0/cfg->wins));

	cfg->echo_rls.channel=cfg->nmicchannel;
	cfg->echo_rls.N=cfg->nspchannel;
	ret=wtk_rls_cfg_update(&(cfg->echo_rls));
	if(ret!=0){goto end;}
	ret=wtk_qmmse_cfg_update(&(cfg->preqmmse));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->covm));
    if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}
	ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
    ret=wtk_qenvelope_cfg_update(&(cfg->qenvl));
    if(ret!=0){goto end;}
	if(cfg->use_maskssl2)
	{
		ret=wtk_maskssl2_cfg_update(&(cfg->maskssl2));
		if(ret!=0){goto end;}
	}

	if(cfg->channel<cfg->nmicchannel+cfg->nspchannel)
	{
		cfg->channel=cfg->nmicchannel+cfg->nspchannel;
	}

	ret=0;
end:
	return ret;
}

int wtk_gainnet_bf4_cfg_update2(wtk_gainnet_bf4_cfg_t *cfg,wtk_source_loader_t *sl)
{
	wtk_rbin2_item_t *item;
	wtk_rbin2_t *rbin=(wtk_rbin2_t*)(sl->hook);
	int ret;

	if(cfg->use_ceps)
	{
		cfg->nb_features_x=cfg->nb_bands+2*cfg->nb_delta_ceps+1;
	}else
	{
		cfg->nb_features_x=cfg->nb_bands;
	}
	cfg->nb_features=cfg->nb_features_x*2;


	cfg->use_rbin_res=1;
	if(cfg->mdl_fn)
	{
		item=wtk_rbin2_get(rbin,cfg->mdl_fn,strlen(cfg->mdl_fn));
		if(!item){ret=-1;goto end;}
		cfg->gainnet=wtk_gainnet5_cfg_new_bin3(rbin->fn,item->pos);
		if(!cfg->gainnet){ret=-1;goto end;}
	}

	cfg->pframe_fs=max(1, cfg->pframe_fs/(cfg->rate*1.0/cfg->wins));
	cfg->pframe_fe=min(cfg->wins/2-2, cfg->pframe_fe/(cfg->rate*1.0/cfg->wins));

	cfg->specsum_fs=max(1, cfg->specsum_fs/(cfg->rate*1.0/cfg->wins));
	cfg->specsum_fe=min(cfg->wins/2-2, cfg->specsum_fe/(cfg->rate*1.0/cfg->wins));

	cfg->echo_rls.channel=cfg->nmicchannel;
	cfg->echo_rls.N=cfg->nspchannel;
	ret=wtk_rls_cfg_update(&(cfg->echo_rls));
	if(ret!=0){goto end;}
	ret=wtk_qmmse_cfg_update(&(cfg->preqmmse));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->covm));
    if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}
    ret=wtk_qenvelope_cfg_update(&(cfg->qenvl));
    if(ret!=0){goto end;}
	if(cfg->use_maskssl2)
	{
		ret=wtk_maskssl2_cfg_update2(&(cfg->maskssl2),sl);
		if(ret!=0){goto end;}
	}

	if(cfg->channel<cfg->nmicchannel+cfg->nspchannel)
	{
		cfg->channel=cfg->nmicchannel+cfg->nspchannel;
	}
	ret=0;
end:
	return ret;
}

wtk_gainnet_bf4_cfg_t* wtk_gainnet_bf4_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_gainnet_bf4_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_gainnet_bf4_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_gainnet_bf4_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_gainnet_bf4_cfg_delete(wtk_gainnet_bf4_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_gainnet_bf4_cfg_t* wtk_gainnet_bf4_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_gainnet_bf4_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_gainnet_bf4_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_gainnet_bf4_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_gainnet_bf4_cfg_delete_bin(wtk_gainnet_bf4_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

