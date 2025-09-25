#include "wtk_wpd_cfg.h" 

int wtk_wpd_cfg_init(wtk_wpd_cfg_t *cfg)
{
	cfg->nmic=0;
	cfg->mic_pos=NULL;
	cfg->speed=340;

	cfg->L=4;

	cfg->rate=16000;

	cfg->sigma=1e-10;
    cfg->p=0.75;

	wtk_covm2_cfg_init(&(cfg->covm2));
	
	return 0;
}

int wtk_wpd_cfg_clean(wtk_wpd_cfg_t *cfg)
{
	int i;

	if(cfg->mic_pos)
	{
		for(i=0;i<cfg->nmic;++i)
		{
			wtk_free(cfg->mic_pos[i]);
		}
		wtk_free(cfg->mic_pos);
	}
	wtk_covm2_cfg_clean(&(cfg->covm2));

	return 0;
}

int wtk_wpd_cfg_update_local(wtk_wpd_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_i(lc,cfg,L,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,speed,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,sigma,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,p,v);

	lc=wtk_local_cfg_find_lc_s(main,"mic");
	if(lc)
	{
		wtk_queue_node_t *qn;
		wtk_cfg_item_t *item;
		int i;

		cfg->mic_pos=(float**)wtk_malloc(sizeof(float*)*lc->cfg->queue.length);
		cfg->nmic=0;
		for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			if(item->type!=WTK_CFG_ARRAY || item->value.array->nslot!=3){continue;}
			cfg->mic_pos[cfg->nmic]=(float*)wtk_malloc(sizeof(float)*3);
			for(i=0;i<3;++i)
			{
				v=((wtk_string_t**)item->value.array->slot)[i];
				cfg->mic_pos[cfg->nmic][i]=wtk_str_atof(v->data,v->len);
				//wtk_debug("v[%d][%d]=%f\n",cfg->nmic,i,cfg->mic_pos[cfg->nmic][i]);
			}
			++cfg->nmic;
		}
	}

	lc=wtk_local_cfg_find_lc_s(main,"covm2");
	if(lc)
	{
		ret=wtk_covm2_cfg_update_local(&(cfg->covm2),lc);
		if(ret!=0){goto end;}
	}

	ret=0;
end:
	return ret;
}

int wtk_wpd_cfg_update(wtk_wpd_cfg_t *cfg)
{
	int ret;
	
	ret=wtk_covm2_cfg_update(&(cfg->covm2));
	if(ret!=0){goto end;}
	
	ret=0;
end:
	return ret;
}
