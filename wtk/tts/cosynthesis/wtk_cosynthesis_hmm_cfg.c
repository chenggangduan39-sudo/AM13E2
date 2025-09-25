#include "wtk_cosynthesis_hmm_cfg.h" 

void wtk_consynthesis_hmm_init_dynamic(wtk_cosynthesis_hmm_cfg_t *cfg)
{
    char *dynamic_spec="162.0101, 117.3258, 114.48288, 141.72328, 90.10946,"
    		"82.84914, 84.46475, 66.40762, 74.67709, 53.575264, 71.96042, 37.131134, 29.703623";
    char *dynamic_pitch= "0.09419621";
	wtk_array_t *b;
	wtk_string_t **v;
	int k;
	float j;
	wtk_heap_t *heap;

	heap = wtk_heap_new(128);

	b=wtk_str_to_array(heap, dynamic_spec, strlen(dynamic_spec), ',');
	v=(wtk_string_t**)b->slot;
	cfg->dynamic_spec = wtk_array_new_h(cfg->heap, b->nslot, sizeof(float));
	for(k=0;k<b->nslot;++k)
	{
		j=atof(v[k]->data);
		*((float*)wtk_array_push(cfg->dynamic_spec))=j;
	}
	b=wtk_str_to_array(heap, dynamic_pitch, strlen(dynamic_pitch), ',');
	v=(wtk_string_t**)b->slot;
    cfg->dynamic_pitch = wtk_array_new_h(cfg->heap, b->nslot, sizeof(float));
	for(k=0;k<b->nslot;++k)
	{
		j=atof(v[k]->data);
		*((float*)wtk_array_push(cfg->dynamic_pitch))=j;
	}
	wtk_heap_delete(heap);
}

int wtk_cosynthesis_hmm_cfg_init(wtk_cosynthesis_hmm_cfg_t *cfg)
{
	cfg->dur_fn=NULL;
	cfg->lf0_fn=NULL;
	cfg->mcp_fn=NULL;
	cfg->bap_fn=NULL;
	cfg->lf0_gv_fn=NULL;
	cfg->mcp_gv_fn=NULL;
	cfg->bap_gv_fn=NULL;
	cfg->conca_lf0_fn = NULL;
	cfg->conca_mcp_fn = NULL;
	cfg->bin_fn = NULL;

	cfg->rate=16000;
	cfg->fperiod=80;
	cfg->rho=1.0;
	cfg->alpha=0.42;
	cfg->f0_mean=0;
	cfg->f0_std=1.0;
	cfg->beta=0.0;
	cfg->uv=0.5;
	cfg->length=0;
	cfg->fftlen=512;
    cfg->sigp=8.0;
    cfg->gamma=0;
    cfg->bapcfgfn=NULL;
    cfg->use_algnst=0;
    cfg->use_algnph=0;
    cfg->use_rnd_flag=0;//1
    cfg->use_bapcfg=0;
    cfg->load_all=1;
    cfg->heap=wtk_heap_new(128);

    wtk_consynthesis_hmm_init_dynamic(cfg);

    return 0;
}

int wtk_cosynthesis_hmm_cfg_clean(wtk_cosynthesis_hmm_cfg_t *cfg)
{
	if (cfg->heap)
		wtk_heap_delete(cfg->heap);
	return 0;
}

int wtk_cosynthesis_hmm_cfg_update_local(wtk_cosynthesis_hmm_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	wtk_array_t* tmpa;

	wtk_local_cfg_update_cfg_str(lc,cfg,bin_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,dur_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,lf0_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,mcp_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,bap_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,lf0_gv_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,mcp_gv_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,bap_gv_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,conca_lf0_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,conca_mcp_fn,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,fperiod,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,fftlen,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,rho,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,f0_mean,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,f0_std,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,beta,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,uv,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,length,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sigp,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,gamma,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,bapcfgfn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_algnst,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_algnph,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_rnd_flag,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_bapcfg,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,load_all,v);

	tmpa = wtk_local_cfg_find_float_array_s(lc, "dynamic_spec");
	if (tmpa)
		cfg->dynamic_spec = tmpa;

	tmpa = wtk_local_cfg_find_float_array_s(lc, "dynamic_pitch");
	if (tmpa)
		cfg->dynamic_pitch = tmpa;

    return 0;
}

int wtk_cosynthesis_hmm_cfg_update(wtk_cosynthesis_hmm_cfg_t *cfg)
{
	return 0;
}

int wtk_cosynthesis_hmm_cfg_update2(wtk_cosynthesis_hmm_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return 0;
}
