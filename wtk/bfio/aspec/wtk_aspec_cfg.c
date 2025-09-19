#include "wtk_aspec_cfg.h" 

int wtk_aspec_cfg_init(wtk_aspec_cfg_t *cfg)
{
	cfg->rate=16000;
	cfg->channel=0;
	cfg->mic_pos=NULL;
	cfg->speed=340;

    cfg->pairs_mic=NULL;
    cfg->pairs_n=0;

	cfg->use_gccspec=1;
	cfg->use_ml=0;
	cfg->use_gccspec2=0;
	cfg->use_ngccspec2=0;
	cfg->use_dnmspec=0;
	cfg->use_dnmspec2=0;
	cfg->use_mvdrspec=0;
	cfg->use_mvdrspec2=0;
	cfg->use_mvdrwspec=0;
	cfg->use_mvdrwspec2=0;
	cfg->use_dsspec=0;
	cfg->use_dsspec2=0;
	cfg->use_dswspec=0;
	cfg->use_dswspec2=0;
	cfg->use_musicspec2=0;
	cfg->use_zdsspec=0;
	cfg->use_zdswspec=0;

	cfg->use_quick=0;
	cfg->use_fftnbinfirst=0;

	cfg->use_line=0;

	cfg->ls_eye=0.5;
    return 0;
}

int wtk_aspec_cfg_clean(wtk_aspec_cfg_t *cfg)
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
	if(cfg->pairs_mic)
	{
		for(i=0;i<cfg->pairs_n;++i)
		{
			wtk_free(cfg->pairs_mic[i]);
		}
		wtk_free(cfg->pairs_mic);
	}

	return 0;
}

int wtk_aspec_cfg_update_local(wtk_aspec_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
    wtk_local_cfg_t *m;

	wtk_local_cfg_update_cfg_b(lc,cfg,use_quick,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fftnbinfirst,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_gccspec,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ml,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_gccspec2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ngccspec2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dnmspec,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dnmspec2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_mvdrspec,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_mvdrspec2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_mvdrwspec,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_mvdrwspec2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dsspec,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dsspec2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dswspec,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dswspec2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_musicspec2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_zdsspec,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_zdswspec,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,speed,v);
	
	wtk_local_cfg_update_cfg_f(lc,cfg,ls_eye,v);

    m=lc;
    lc=wtk_local_cfg_find_lc_s(m,"pairs_mic");
	if(lc)
	{
		wtk_queue_node_t *qn;
		wtk_cfg_item_t *item;
		int i;

		cfg->pairs_mic=(float**)wtk_malloc(sizeof(float*)*lc->cfg->queue.length);
		cfg->pairs_n=0;
		for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			if(item->type!=WTK_CFG_ARRAY || item->value.array->nslot!=2){continue;}
			cfg->pairs_mic[cfg->pairs_n]=(float*)wtk_malloc(sizeof(float)*2);
			for(i=0;i<2;++i)
			{
				v=((wtk_string_t**)item->value.array->slot)[i];
				cfg->pairs_mic[cfg->pairs_n][i]=wtk_str_atof(v->data,v->len);
			}
			++cfg->pairs_n;
		}
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

    return 0;
}

int wtk_aspec_cfg_update(wtk_aspec_cfg_t *cfg)
{
	return 0;
}

int wtk_aspec_cfg_update2(wtk_aspec_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return wtk_aspec_cfg_update(cfg);
}
