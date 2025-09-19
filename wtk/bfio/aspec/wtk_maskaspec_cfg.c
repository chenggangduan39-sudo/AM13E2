#include "wtk_maskaspec_cfg.h" 

int wtk_maskaspec_cfg_init(wtk_maskaspec_cfg_t *cfg)
{
	cfg->rate=16000;
	cfg->channel=0;
	cfg->mic_pos=NULL;
	cfg->mic_pos2=NULL;
	cfg->speed=340;

	cfg->use_maskds=1;
	cfg->use_maskzds=0;
	cfg->use_maskmvdr=0;
	cfg->use_maskgcc=0;
	cfg->th_step=5;

	cfg->use_line=0;
	cfg->ls_eye=0.5;
	cfg->use_mic2=0;

    return 0;
}

int wtk_maskaspec_cfg_clean(wtk_maskaspec_cfg_t *cfg)
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

	if(cfg->mic_pos2)
	{
		for(i=0;i<cfg->channel;++i)
		{
			wtk_free(cfg->mic_pos2[i]);
		}
		wtk_free(cfg->mic_pos2);
	}

	return 0;
}

int wtk_maskaspec_cfg_update_local(wtk_maskaspec_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
    wtk_local_cfg_t *m;

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,speed,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_maskds,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_maskzds,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_maskmvdr,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_maskgcc,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,th_step,v);
	
	wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,ls_eye,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_mic2,v);

	if(cfg->use_mic2){
		m=lc;
		lc=wtk_local_cfg_find_lc_s(m,"mic_2");
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
	}else{
		m=lc;
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

		m=lc;
		lc=wtk_local_cfg_find_lc_s(m,"mic2");
		if(lc)
		{
			wtk_queue_node_t *qn;
			wtk_cfg_item_t *item;
			int i;

			cfg->mic_pos2=(float**)wtk_malloc(sizeof(float*)*lc->cfg->queue.length);
			cfg->channel=0;
			for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
			{
				item=data_offset2(qn,wtk_cfg_item_t,n);
				if(item->type!=WTK_CFG_ARRAY || item->value.array->nslot!=3){continue;}
				cfg->mic_pos2[cfg->channel]=(float*)wtk_malloc(sizeof(float)*3);
				for(i=0;i<3;++i)
				{
					v=((wtk_string_t**)item->value.array->slot)[i];
					cfg->mic_pos2[cfg->channel][i]=wtk_str_atof(v->data,v->len);
				}
				++cfg->channel;
			}
		}
	}

    return 0;
}

int wtk_maskaspec_cfg_update(wtk_maskaspec_cfg_t *cfg)
{
	return 0;
}

int wtk_maskaspec_cfg_update2(wtk_maskaspec_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return wtk_maskaspec_cfg_update(cfg);
}