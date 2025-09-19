#include "wtk_gainnet_bf7_cfg.h"

int wtk_gainnet_bf7_cfg_init(wtk_gainnet_bf7_cfg_t *cfg)
{
	cfg->channel=0;
	cfg->mic_pos=NULL;
    cfg->wins=320;
	cfg->speed=340;

    cfg->mdl_fn=NULL;
    cfg->masknet=NULL;
    cfg->use_rbin_res=0;

	cfg->use_preemph=1;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->rate=16000;
	
	cfg->use_line=0;

	wtk_covm_cfg_init(&(cfg->covm));
	wtk_bf_cfg_init(&(cfg->bf));

	cfg->use_bf=0;

	return 0;
}

int wtk_gainnet_bf7_cfg_clean(wtk_gainnet_bf7_cfg_t *cfg)
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
	
	if(cfg->masknet)
	{
		if(cfg->use_rbin_res)
		{
			wtk_masknet_cfg_delete_bin3(cfg->masknet);
		}else
		{
			wtk_masknet_cfg_delete_bin2(cfg->masknet);
		}
	}
	wtk_bf_cfg_clean(&(cfg->bf));
	wtk_covm_cfg_clean(&(cfg->covm));

	return 0;
}

int wtk_gainnet_bf7_cfg_update_local(wtk_gainnet_bf7_cfg_t *cfg,wtk_local_cfg_t *m)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,mdl_fn,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_preemph,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,speed,v);
	
	wtk_local_cfg_update_cfg_b(lc,cfg,use_bf,v);

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

	lc=wtk_local_cfg_find_lc_s(m,"bf");
	if(lc)
	{
		ret=wtk_bf_cfg_update_local(&(cfg->bf),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"covm");
	if(lc)
	{
        ret=wtk_covm_cfg_update_local(&(cfg->covm),lc);
        if(ret!=0){goto end;}
    }

	ret=0;
end:
	return ret;
}

int wtk_gainnet_bf7_cfg_update(wtk_gainnet_bf7_cfg_t *cfg)
{
	int ret;

	cfg->features_len=(cfg->wins/2-6)*(cfg->channel+1);
	if(cfg->mdl_fn)
	{
		cfg->masknet=wtk_masknet_cfg_new_bin2(cfg->mdl_fn);
		if(!cfg->masknet){ret=-1;goto end;}
	}

	ret=wtk_covm_cfg_update(&(cfg->covm));
    if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}

	ret=0;
end:
	return ret;
}

int wtk_gainnet_bf7_cfg_update2(wtk_gainnet_bf7_cfg_t *cfg,wtk_source_loader_t *sl)
{
	wtk_rbin2_item_t *item;
	wtk_rbin2_t *rbin=(wtk_rbin2_t*)(sl->hook);
	int ret;

	cfg->features_len=(cfg->wins/2-6)*(cfg->channel+1);
	cfg->use_rbin_res=1;
	if(cfg->mdl_fn)
	{
		item=wtk_rbin2_get(rbin,cfg->mdl_fn,strlen(cfg->mdl_fn));
		if(!item){ret=-1;goto end;}
		cfg->masknet=wtk_masknet_cfg_new_bin3(rbin->fn,item->pos);
		if(!cfg->masknet){ret=-1;goto end;}
	}

	ret=wtk_covm_cfg_update(&(cfg->covm));
    if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}

	ret=0;
end:
	return ret;
}

wtk_gainnet_bf7_cfg_t* wtk_gainnet_bf7_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_gainnet_bf7_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_gainnet_bf7_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_gainnet_bf7_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_gainnet_bf7_cfg_delete(wtk_gainnet_bf7_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_gainnet_bf7_cfg_t* wtk_gainnet_bf7_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_gainnet_bf7_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_gainnet_bf7_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_gainnet_bf7_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_gainnet_bf7_cfg_delete_bin(wtk_gainnet_bf7_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

