#include "wtk_gainnet_bf5_cfg.h"

int wtk_gainnet_bf5_cfg_init(wtk_gainnet_bf5_cfg_t *cfg)
{
	cfg->channel=0;
	cfg->mic_pos=NULL;

    cfg->wins=320;

    cfg->ceps_mem=8;
    cfg->nb_delta_ceps=6;

    cfg->nb_bands=0;  

    cfg->mdl_fn=NULL;
    cfg->gainnet=NULL;

    cfg->use_rbin_res=0;

	wtk_qmmse_cfg_init(&(cfg->qmmse));

	cfg->use_qmmse=0;

	cfg->use_preemph=1;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->use_ceps=1;

	cfg->rate=16000;

	cfg->agc_a=0.69;
	cfg->agc_b=6.9;
	
	cfg->specsum_fs=0;
	cfg->specsum_fe=8000;

	cfg->use_line=0;

	cfg->lds_eye=1e-6;

	cfg->speed=340;

	return 0;
}

int wtk_gainnet_bf5_cfg_clean(wtk_gainnet_bf5_cfg_t *cfg)
{
	int i;

	if(cfg->mic_pos)
	{
		for(i=0;i<cfg->channel;++i)
		{
			wtk_free(cfg->mic_pos[i]);
		}
		wtk_free(cfg->mic_pos);
	}
	
	if(cfg->gainnet)
	{
		if(cfg->use_rbin_res)
		{
			wtk_gainnet_cfg_delete_bin3(cfg->gainnet);
		}else
		{
			wtk_gainnet_cfg_delete_bin2(cfg->gainnet);
		}
	}
	wtk_qmmse_cfg_clean(&(cfg->qmmse));

	return 0;
}

int wtk_gainnet_bf5_cfg_update_local(wtk_gainnet_bf5_cfg_t *cfg,wtk_local_cfg_t *m)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,ceps_mem,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nb_delta_ceps,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nb_bands,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,mdl_fn,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_preemph,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_ceps,v);
	
	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,agc_a,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agc_b,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,specsum_fs,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,specsum_fe,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,lds_eye,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,speed,v);

	lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
		cfg->qmmse.step=cfg->wins/2;
        if(ret!=0){goto end;}
    }

	lc=wtk_local_cfg_find_lc_s(m,"mic");
	if(lc)
	{
		wtk_queue_node_t *qn;
		wtk_cfg_item_t *item;
		int i;

		cfg->mic_pos=(float**)wtk_malloc(sizeof(float*)*lc->cfg->queue.length);
		cfg->channel=0;
		for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			if(item->type!=WTK_CFG_ARRAY || item->value.array->nslot!=3){continue;}
			cfg->mic_pos[cfg->channel]=(float*)wtk_malloc(sizeof(float)*3);
			for(i=0;i<3;++i)
			{
				v=((wtk_string_t**)item->value.array->slot)[i];
				cfg->mic_pos[cfg->channel][i]=wtk_str_atof(v->data,v->len);
			}
			++cfg->channel;
		}
	}
	ret=0;
end:
	return ret;
}

int wtk_gainnet_bf5_cfg_update(wtk_gainnet_bf5_cfg_t *cfg)
{
	int ret;

	if(cfg->use_ceps)
	{
		cfg->nb_features_x=cfg->nb_bands+2*cfg->nb_delta_ceps+1;
	}else
	{
		cfg->nb_features_x=cfg->nb_bands;
	}
	cfg->nb_features=cfg->nb_features_x*2;//+cfg->nb_bands;

	if(cfg->mdl_fn)
	{
		cfg->gainnet=wtk_gainnet_cfg_new_bin2(cfg->mdl_fn);
		if(!cfg->gainnet){ret=-1;goto end;}
	}

	cfg->specsum_fs=max(1, cfg->specsum_fs/(cfg->rate*1.0/cfg->wins));
	cfg->specsum_fe=min(cfg->wins/2-2, cfg->specsum_fe/(cfg->rate*1.0/cfg->wins));

	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}

	ret=0;
end:
	return ret;
}

int wtk_gainnet_bf5_cfg_update2(wtk_gainnet_bf5_cfg_t *cfg,wtk_source_loader_t *sl)
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
	cfg->nb_features=cfg->nb_features_x*2;//+cfg->nb_bands;

	cfg->use_rbin_res=1;
	if(cfg->mdl_fn)
	{
		item=wtk_rbin2_get(rbin,cfg->mdl_fn,strlen(cfg->mdl_fn));
		if(!item){ret=-1;goto end;}
		cfg->gainnet=wtk_gainnet_cfg_new_bin3(rbin->fn,item->pos);
		if(!cfg->gainnet){ret=-1;goto end;}
	}

	cfg->specsum_fs=max(1, cfg->specsum_fs/(cfg->rate*1.0/cfg->wins));
	cfg->specsum_fe=min(cfg->wins/2-2, cfg->specsum_fe/(cfg->rate*1.0/cfg->wins));

	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}

	ret=0;
end:
	return ret;
}

wtk_gainnet_bf5_cfg_t* wtk_gainnet_bf5_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_gainnet_bf5_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_gainnet_bf5_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_gainnet_bf5_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_gainnet_bf5_cfg_delete(wtk_gainnet_bf5_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_gainnet_bf5_cfg_t* wtk_gainnet_bf5_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_gainnet_bf5_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_gainnet_bf5_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_gainnet_bf5_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_gainnet_bf5_cfg_delete_bin(wtk_gainnet_bf5_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

