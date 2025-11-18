#include "wtk_gainnet_ssl_cfg.h"

int wtk_gainnet_ssl_cfg_init(wtk_gainnet_ssl_cfg_t *cfg)
{
	cfg->channel=0;
	cfg->nmicchannel=0;
	cfg->mic_channel=NULL;
	cfg->nspchannel=0;
	cfg->sp_channel=NULL;

    cfg->wins=1024;

	wtk_bankfeat_cfg_init(&(cfg->bankfeat));

    cfg->mdl_fn=NULL;
    cfg->gainnet7=NULL;
	cfg->aecmdl_fn=NULL;
	cfg->gainnet2=NULL;
    cfg->use_rbin_res=0;

	cfg->theta = 180;
	cfg->phi=0;

	wtk_rls_cfg_init(&(cfg->echo_rls));

	wtk_qmmse_cfg_init(&(cfg->qmmse));

	cfg->spenr_thresh=100;
	cfg->spenr_cnt=10;

	cfg->micenr_thresh=300;
	cfg->micenr_cnt=10;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->use_erlssingle=1;
	cfg->use_firstds=0;

	cfg->rate=16000;

	cfg->use_maskssl=0;
	wtk_maskssl_cfg_init(&(cfg->maskssl));
	cfg->use_maskssl2=0;
	wtk_maskssl2_cfg_init(&(cfg->maskssl2));
	
	cfg->pframe_fs=200;
	cfg->pframe_fe=8000;
	cfg->pframe_alpha=1;

	cfg->ralpha=0;

	cfg->use_qmmse=1;

	return 0;
}

int wtk_gainnet_ssl_cfg_clean(wtk_gainnet_ssl_cfg_t *cfg)
{
	if(cfg->mic_channel)
	{
		wtk_free(cfg->mic_channel);
	}
	if(cfg->sp_channel)
	{
		wtk_free(cfg->sp_channel);
	}
	if(cfg->gainnet7)
	{
		if(cfg->use_rbin_res)
		{
			wtk_gainnet7_cfg_delete_bin3(cfg->gainnet7);
		}else
		{
			wtk_gainnet7_cfg_delete_bin2(cfg->gainnet7);
		}
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
	wtk_rls_cfg_clean(&(cfg->echo_rls));
	wtk_qmmse_cfg_clean(&(cfg->qmmse));
	wtk_maskssl_cfg_clean(&(cfg->maskssl));
	wtk_maskssl2_cfg_clean(&(cfg->maskssl2));

	return 0;
}

int wtk_gainnet_ssl_cfg_update_local(wtk_gainnet_ssl_cfg_t *cfg,wtk_local_cfg_t *m)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;
	wtk_array_t *a;
	int i;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);

    wtk_local_cfg_update_cfg_str(lc,cfg,mdl_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,aecmdl_fn,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,theta,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,phi,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,spenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,spenr_cnt,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,micenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,micenr_cnt,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_erlssingle,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_firstds,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_maskssl,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_maskssl2,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,pframe_fs,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pframe_fe,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pframe_alpha,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,ralpha,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);

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

	lc=wtk_local_cfg_find_lc_s(m,"maskssl");
	if(lc)
	{
		ret=wtk_maskssl_cfg_update_local(&(cfg->maskssl),lc);
		cfg->maskssl.wins=cfg->wins;
		if(ret!=0){goto end;}
	}	
	lc=wtk_local_cfg_find_lc_s(m,"maskssl2");
	if(lc)
	{
		ret=wtk_maskssl2_cfg_update_local(&(cfg->maskssl2),lc);
		cfg->maskssl2.wins=cfg->wins;
		if(ret!=0){goto end;}
	}	

	ret=0;
end:
	return ret;
}

int wtk_gainnet_ssl_cfg_update(wtk_gainnet_ssl_cfg_t *cfg)
{
	int ret;

	if(cfg->mdl_fn)
	{
		cfg->gainnet7=wtk_gainnet7_cfg_new_bin2(cfg->mdl_fn);
		if(!cfg->gainnet7){ret=-1;goto end;}
	}
	if(cfg->aecmdl_fn)
	{
		cfg->gainnet2=wtk_gainnet2_cfg_new_bin2(cfg->aecmdl_fn);
		if(!cfg->gainnet2){ret=-1;goto end;}
	}
	cfg->pframe_fs=max(1, cfg->pframe_fs/(cfg->rate*1.0/cfg->wins));
	cfg->pframe_fe=min(cfg->wins/2-2, cfg->pframe_fe/(cfg->rate*1.0/cfg->wins));

	ret=wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if(ret!=0){goto end;}

	if(cfg->use_erlssingle)
	{
		cfg->echo_rls.channel=1;
	}else
	{
		cfg->echo_rls.channel=cfg->nmicchannel;
		cfg->use_firstds=0;
	}
	cfg->echo_rls.N=cfg->nspchannel;
	ret=wtk_rls_cfg_update(&(cfg->echo_rls));
	if(ret!=0){goto end;}
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	if(cfg->use_maskssl)
	{
		ret=wtk_maskssl_cfg_update(&(cfg->maskssl));
		if(ret!=0){goto end;}
	}
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

int wtk_gainnet_ssl_cfg_update2(wtk_gainnet_ssl_cfg_t *cfg,wtk_source_loader_t *sl)
{
	wtk_rbin2_item_t *item;
	wtk_rbin2_t *rbin=(wtk_rbin2_t*)(sl->hook);
	int ret;

	cfg->use_rbin_res=1;
	if(cfg->mdl_fn)
	{
		item=wtk_rbin2_get(rbin,cfg->mdl_fn,strlen(cfg->mdl_fn));
		if(!item){ret=-1;goto end;}
		cfg->gainnet7=wtk_gainnet7_cfg_new_bin3(rbin->fn,item->pos);
		if(!cfg->gainnet7){ret=-1;goto end;}
	}
	if(cfg->aecmdl_fn)
	{
		item=wtk_rbin2_get(rbin,cfg->aecmdl_fn,strlen(cfg->aecmdl_fn));
		if(!item){ret=-1;goto end;}
		cfg->gainnet2=wtk_gainnet2_cfg_new_bin3(rbin->fn,item->pos);
		if(!cfg->gainnet2){ret=-1;goto end;}
	}
	cfg->pframe_fs=max(1, cfg->pframe_fs/(cfg->rate*1.0/cfg->wins));
	cfg->pframe_fe=min(cfg->wins/2-2, cfg->pframe_fe/(cfg->rate*1.0/cfg->wins));
	
	ret=wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if(ret!=0){goto end;}

	if(cfg->use_erlssingle)
	{
		cfg->echo_rls.channel=1;
	}else
	{
		cfg->echo_rls.channel=cfg->nmicchannel;
		cfg->use_firstds=0;
	}
	cfg->echo_rls.N=cfg->nspchannel;
	ret=wtk_rls_cfg_update(&(cfg->echo_rls));	
	if(ret!=0){goto end;}
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	if(cfg->use_maskssl)
	{
		ret=wtk_maskssl_cfg_update2(&(cfg->maskssl),sl);
		if(ret!=0){goto end;}
	}
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

wtk_gainnet_ssl_cfg_t* wtk_gainnet_ssl_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_gainnet_ssl_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_gainnet_ssl_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_gainnet_ssl_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_gainnet_ssl_cfg_delete(wtk_gainnet_ssl_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_gainnet_ssl_cfg_t* wtk_gainnet_ssl_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_gainnet_ssl_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_gainnet_ssl_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_gainnet_ssl_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_gainnet_ssl_cfg_delete_bin(wtk_gainnet_ssl_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

