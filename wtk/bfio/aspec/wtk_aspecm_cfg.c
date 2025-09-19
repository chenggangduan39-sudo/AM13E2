#include "wtk_aspecm_cfg.h"

int wtk_aspecm_cfg_init(wtk_aspecm_cfg_t *cfg) {

	cfg->rate=16000;
	cfg->nmic=0;
	cfg->mic_pos=NULL;
    cfg->nbin=257;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->use_qmmse=0;
	wtk_qmmse_cfg_init(&(cfg->qmmse));
	wtk_aspec_cfg_init(&(cfg->aspec));

	cfg->theta=90;
	cfg->theta_range=15;
    cfg->specsum_fs=0;
    cfg->specsum_fe=8000;

	cfg->q_nf=5;
	cfg->right_nf=14;
	cfg->min_speccrest=500;
	cfg->envelope_thresh=200;
	cfg->right_min_thresh=10;
	cfg->q_alpha=0.8;
	cfg->use_sqenvelope=0;
	cfg->use_line=0;
    return 0;
}

int wtk_aspecm_cfg_clean(wtk_aspecm_cfg_t *cfg) {
	int i;
	if(cfg->mic_pos)
	{
		for(i=0;i<cfg->nmic;++i)
		{
			wtk_free(cfg->mic_pos[i]);
		}
		wtk_free(cfg->mic_pos);
	}
	wtk_qmmse_cfg_clean(&(cfg->qmmse));
    wtk_aspec_cfg_clean(&(cfg->aspec));

    return 0;
}

int wtk_aspecm_cfg_update(wtk_aspecm_cfg_t *cfg) {
	int ret;

	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}
    cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/(cfg->nbin-1)*2));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/(cfg->nbin-1)*2));
	cfg->specsum_ne=min(cfg->nbin-2, cfg->specsum_ne);
	ret=0;
end:
	return ret;
}

int wtk_aspecm_cfg_update2(wtk_aspecm_cfg_t *cfg, wtk_source_loader_t *sl) {
	int ret;

	ret=wtk_qmmse_cfg_update2(&(cfg->qmmse), sl->hook);
	if(ret!=0){goto end;}
	ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}
    cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/(cfg->nbin-1)*2));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/(cfg->nbin-1)*2));
	cfg->specsum_ne=min(cfg->nbin-2, cfg->specsum_ne);
	ret=0;
end:
	return ret;
}

int wtk_aspecm_cfg_update_local(wtk_aspecm_cfg_t *cfg, wtk_local_cfg_t *m) {
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nbin,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,theta,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,theta_range,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_fs,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_fe,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,q_nf,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,right_nf,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_speccrest,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,envelope_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,right_min_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,q_alpha,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_sqenvelope,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);

	lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
		cfg->qmmse.step=cfg->nbin-1;
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(m,"aspec");
	if(lc)
	{
        ret=wtk_aspec_cfg_update_local(&(cfg->aspec),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"mic");
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
				// wtk_debug("v[%d][%d]=%f\n",cfg->nmic,i,cfg->mic_pos[cfg->nmic][i]);
			}
			++cfg->nmic;
		}
	}
    ret = 0;
end:
    return ret;
}

wtk_aspecm_cfg_t* wtk_aspecm_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_aspecm_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_aspecm_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_aspecm_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_aspecm_cfg_delete(wtk_aspecm_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_aspecm_cfg_t* wtk_aspecm_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_aspecm_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_aspecm_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_aspecm_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_aspecm_cfg_delete_bin(wtk_aspecm_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
