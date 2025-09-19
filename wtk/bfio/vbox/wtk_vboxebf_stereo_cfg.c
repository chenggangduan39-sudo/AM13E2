#include "wtk_vboxebf_stereo_cfg.h"

int wtk_vboxebf_stereo_cfg_init(wtk_vboxebf_stereo_cfg_t *cfg)
{
	cfg->channel=0;
	cfg->nmicchannel=0;
	cfg->mic_channel=NULL;
	cfg->nspchannel=0;
	cfg->sp_channel=NULL;

    cfg->wins=1024;

	wtk_bankfeat_cfg_init(&(cfg->bankfeat));

	cfg->aecmdl_fn=NULL;
	cfg->gainnet2=NULL;
    cfg->use_rbin_res=0;

	wtk_rls_cfg_init(&(cfg->echo_rls));

	wtk_qmmse_cfg_init(&(cfg->qmmse));

	cfg->spenr_thresh=100;
	cfg->spenr_cnt=10;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->rate=16000;

	wtk_equalizer_cfg_init(&(cfg->eq));
	cfg->use_eq=0;

	cfg->agc_a=0.69;
	cfg->agc_b=6.9;
	cfg->eagc_a=0.69;
	cfg->eagc_b=6.9;
	cfg->g2_min=0.05;
	cfg->g2_max=0.95;
	cfg->agcaddg=1.0;

	cfg->gbias=0;

	cfg->g_minthresh=1e-6;

	cfg->featm_lm=1;
	cfg->featsp_lm=1;

	cfg->clip_s=0;
	cfg->clip_e=8000;

	cfg->ralpha=0;
	cfg->ralpha2=0;
	cfg->echo_ralpha=0;
	cfg->echo_ralpha2=0;

	cfg->use_qmmse=1;

    cfg->agcmdl_fn=NULL;
    cfg->agc_gainnet=NULL;

	cfg->otheta=NULL;
	cfg->noutchannel=0;
	cfg->nomicchannel=0;
	cfg->omic_pos=NULL;
	cfg->omic_channel=NULL;
	cfg->speed=340;
	cfg->eye=0.1;

	return 0;
}

int wtk_vboxebf_stereo_cfg_clean(wtk_vboxebf_stereo_cfg_t *cfg)
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
	if(cfg->omic_pos)
	{
		for(i=0;i<cfg->nomicchannel;++i)
		{
			wtk_free(cfg->omic_pos[i]);
		}
		for(i=0;i<cfg->noutchannel;++i)
		{
			wtk_free(cfg->omic_channel[i]);
			wtk_free(cfg->otheta[i]);
		}
		wtk_free(cfg->omic_pos);
		wtk_free(cfg->omic_channel);
		wtk_free(cfg->otheta);
		wtk_free(cfg->orls_channel);
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
	if(cfg->agc_gainnet)
	{
		if(cfg->use_rbin_res)
		{
			wtk_gainnet_cfg_delete_bin3(cfg->agc_gainnet);
		}else
		{
			wtk_gainnet_cfg_delete_bin2(cfg->agc_gainnet);
		}
	}

	wtk_bankfeat_cfg_clean(&(cfg->bankfeat));
	wtk_rls_cfg_clean(&(cfg->echo_rls));
	wtk_qmmse_cfg_clean(&(cfg->qmmse));
	wtk_equalizer_cfg_clean(&(cfg->eq));

	return 0;
}

int wtk_vboxebf_stereo_cfg_update_local(wtk_vboxebf_stereo_cfg_t *cfg,wtk_local_cfg_t *m)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	wtk_queue_node_t *qn;
	wtk_cfg_item_t *item;
	int ret;
	wtk_array_t *a;
	int i,j;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);

	wtk_local_cfg_update_cfg_str(lc,cfg,aecmdl_fn,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,spenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,spenr_cnt,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,micenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,micenr_cnt,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_eq,v);

	wtk_local_cfg_update_cfg_str(lc,cfg,agcmdl_fn,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agc_a,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agc_b,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,eagc_a,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,eagc_b,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,g2_min,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,g2_max,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agcaddg,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,gbias,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,g_minthresh,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,clip_e,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,ralpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,ralpha2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,echo_ralpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,echo_ralpha2,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,featsp_lm,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,featm_lm,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,speed,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,eye,v);
	
	a=wtk_local_cfg_find_array_s(m,"mic_channel");
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
	a=wtk_local_cfg_find_array_s(m,"sp_channel");
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
	lc=wtk_local_cfg_find_lc_s(m,"bankfeat");
	if(lc)
	{
		ret=wtk_bankfeat_cfg_update_local(&(cfg->bankfeat),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"echo_rls");
	if(lc)
	{
		ret=wtk_rls_cfg_update_local(&(cfg->echo_rls),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
		cfg->qmmse.step=cfg->wins/2;
        if(ret!=0){goto end;}
    }

	lc=wtk_local_cfg_find_lc_s(m,"eq");
	if(lc)
	{
		ret=wtk_equalizer_cfg_update_local(&(cfg->eq),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"omic");
	if(lc)
	{
		cfg->omic_pos=(float**)wtk_malloc(sizeof(float*)*lc->cfg->queue.length);
		cfg->nomicchannel=0;
		for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			if(item->type!=WTK_CFG_ARRAY || item->value.array->nslot!=3){continue;}
			cfg->omic_pos[cfg->nomicchannel]=(float*)wtk_malloc(sizeof(float)*3);
			for(i=0;i<3;++i)
			{
				v=((wtk_string_t**)item->value.array->slot)[i];
				cfg->omic_pos[cfg->nomicchannel][i]=wtk_str_atof(v->data,v->len);
			}
			++cfg->nomicchannel;
		}
	}
	lc=wtk_local_cfg_find_lc_s(m,"omic_channel");
	if(lc)
	{
		cfg->omic_channel=(int**)wtk_malloc(sizeof(int*)*lc->cfg->queue.length);
		cfg->noutchannel=0;
		for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			if(item->type!=WTK_CFG_ARRAY || item->value.array->nslot!=cfg->nomicchannel){continue;}
			cfg->omic_channel[cfg->noutchannel]=(int*)wtk_malloc(sizeof(int)*cfg->nomicchannel);
			for(i=0;i<cfg->nomicchannel;++i)
			{
				v=((wtk_string_t**)item->value.array->slot)[i];
				cfg->omic_channel[cfg->noutchannel][i]=wtk_str_atoi(v->data,v->len);
			}
			++cfg->noutchannel;
		}
	}
	lc=wtk_local_cfg_find_lc_s(m,"otheta");
	if(lc)
	{
		cfg->otheta=(float**)wtk_malloc(sizeof(float*)*lc->cfg->queue.length);
		j=0;
		for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			if(item->type!=WTK_CFG_ARRAY || item->value.array->nslot!=cfg->nomicchannel){continue;}
			cfg->otheta[j]=(float*)wtk_malloc(sizeof(float)*cfg->nomicchannel);
			for(i=0;i<cfg->nomicchannel;++i)
			{
				v=((wtk_string_t**)item->value.array->slot)[i];
				cfg->otheta[j][i]=wtk_str_atof(v->data,v->len);
			}
			++j;
		}
	}
	a=wtk_local_cfg_find_array_s(m,"orls_channel");
	if(a)
	{
		cfg->orls_channel=(int*)wtk_malloc(sizeof(int)*a->nslot);
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->orls_channel[i]=wtk_str_atoi(v->data,v->len);
		}
	}

	ret=0;
end:
	return ret;
}

int wtk_vboxebf_stereo_cfg_update(wtk_vboxebf_stereo_cfg_t *cfg)
{
	int ret;

	if(cfg->aecmdl_fn)
	{
		cfg->gainnet2=wtk_gainnet2_cfg_new_bin2(cfg->aecmdl_fn);
		if(!cfg->gainnet2){ret=-1;goto end;}
	}
	if(cfg->agcmdl_fn)
	{
		cfg->agc_gainnet=wtk_gainnet_cfg_new_bin2(cfg->agcmdl_fn);
		if(!cfg->agc_gainnet){ret=-1;goto end;}
	}
	ret=wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if(ret!=0){goto end;}
	
	cfg->echo_rls.channel=cfg->noutchannel;
	
	cfg->echo_rls.N=cfg->nspchannel;
	ret=wtk_rls_cfg_update(&(cfg->echo_rls));
	if(ret!=0){goto end;}
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=wtk_equalizer_cfg_update(&(cfg->eq));
	if(ret!=0){goto end;}
	if(cfg->channel<cfg->nmicchannel+cfg->nspchannel)
	{
		cfg->channel=cfg->nmicchannel+cfg->nspchannel;
	}
	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;

	ret=0;
end:
	return ret;
}

int wtk_vboxebf_stereo_cfg_update2(wtk_vboxebf_stereo_cfg_t *cfg,wtk_source_loader_t *sl)
{
	wtk_rbin2_item_t *item;
	wtk_rbin2_t *rbin=(wtk_rbin2_t*)(sl->hook);
	int ret;

	cfg->use_rbin_res=1;
	if(cfg->aecmdl_fn)
	{
		item=wtk_rbin2_get(rbin,cfg->aecmdl_fn,strlen(cfg->aecmdl_fn));
		if(!item){ret=-1;goto end;}
		cfg->gainnet2=wtk_gainnet2_cfg_new_bin3(rbin->fn,item->pos);
		if(!cfg->gainnet2){ret=-1;goto end;}
	}
	if(cfg->agcmdl_fn)
	{
		item=wtk_rbin2_get(rbin,cfg->agcmdl_fn,strlen(cfg->agcmdl_fn));
		if(!item){ret=-1;goto end;}
		cfg->agc_gainnet=wtk_gainnet_cfg_new_bin3(rbin->fn,item->pos);
		if(!cfg->agc_gainnet){ret=-1;goto end;}
	}
	
	ret=wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if(ret!=0){goto end;}
	cfg->echo_rls.channel=cfg->noutchannel;

	cfg->echo_rls.N=cfg->nspchannel;
	ret=wtk_rls_cfg_update(&(cfg->echo_rls));	
	if(ret!=0){goto end;}
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=wtk_equalizer_cfg_update(&(cfg->eq));
	if(ret!=0){goto end;}

	if(cfg->channel<cfg->nmicchannel+cfg->nspchannel)
	{
		cfg->channel=cfg->nmicchannel+cfg->nspchannel;
	}
	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;

	ret=0;
end:
	return ret;
}

wtk_vboxebf_stereo_cfg_t* wtk_vboxebf_stereo_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_vboxebf_stereo_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_vboxebf_stereo_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_vboxebf_stereo_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_vboxebf_stereo_cfg_delete(wtk_vboxebf_stereo_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_vboxebf_stereo_cfg_t* wtk_vboxebf_stereo_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_vboxebf_stereo_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_vboxebf_stereo_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_vboxebf_stereo_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_vboxebf_stereo_cfg_delete_bin(wtk_vboxebf_stereo_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

