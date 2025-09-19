#include "wtk_bf_cfg.h" 

int wtk_bf_cfg_init(wtk_bf_cfg_t *cfg)
{
	cfg->rate=16000;
	cfg->nmic=0;
	cfg->mic_pos=NULL;
	cfg->speed=340;
	cfg->sd_eye=0.03;
	cfg->eye=0;
	cfg->eig_iter_eps=1e-6;
	cfg->eig_iter_max_iter=10;

	cfg->use_post=0;
	cfg->post_gramma=0.8;

	cfg->mu=1.0;
	cfg->qrank=2;

	cfg->use_ncov_eig=0;
	cfg->use_scov_evd=0;
	cfg->use_eig_ovec=0;

	cfg->use_r1_mwf=0;
	cfg->use_sdw_mwf=0;
	cfg->use_mvdr=0;
	cfg->use_gev=0;
	cfg->use_vs=0;

	cfg->use_norm=0;
	cfg->use_ovec_norm=1;

	return 0;
}

int wtk_bf_cfg_clean(wtk_bf_cfg_t *cfg)
{
	int i;

	if(cfg->mic_pos)
	{
		for(i=0;i<cfg->real_nmic;++i)
		{
			wtk_free(cfg->mic_pos[i]);
		}
		wtk_free(cfg->mic_pos);
	}
	return 0;
}

int wtk_bf_cfg_update_local(wtk_bf_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;

	lc=main;
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ncov_eig,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_scov_evd,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_eig_ovec,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_sdw_mwf,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_r1_mwf,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_mvdr,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_gev,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_vs,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,mu,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,qrank,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sd_eye,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,eye,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,speed,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,eig_iter_eps,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,eig_iter_max_iter,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_post,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_norm,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ovec_norm,v);
	
	wtk_local_cfg_update_cfg_f(lc,cfg,post_gramma,v);
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
		cfg->real_nmic=cfg->nmic;
	}
	return 0;
}

int wtk_bf_cfg_update(wtk_bf_cfg_t *cfg)
{
	return 0;
}


void wtk_bf_cfg_set_channel(wtk_bf_cfg_t *cfg, int channel)
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

	cfg->mic_pos=(float**)wtk_malloc(sizeof(float*)*channel);
	for(i=0;i<channel;++i)
	{
		cfg->mic_pos[i]=(float*)wtk_calloc(3,sizeof(float));
	}
	cfg->nmic=channel;
	cfg->real_nmic=channel;
}

