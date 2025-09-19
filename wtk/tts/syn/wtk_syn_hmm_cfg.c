#include "wtk_syn_hmm_cfg.h" 

int wtk_syn_hmm_cfg_init(wtk_syn_hmm_cfg_t *cfg)
{
	cfg->dur_fn=NULL;
	cfg->lf0_fn=NULL;
	cfg->mcp_fn=NULL;
	cfg->bap_fn=NULL;
	cfg->lf0_gv_fn=NULL;
	cfg->mcp_gv_fn=NULL;
	cfg->bap_gv_fn=NULL;

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
	return 0;
}

int wtk_syn_hmm_cfg_clean(wtk_syn_hmm_cfg_t *cfg)
{
	return 0;
}

int wtk_syn_hmm_cfg_update_local(wtk_syn_hmm_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,dur_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,lf0_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,mcp_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,bap_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,lf0_gv_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,mcp_gv_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,bap_gv_fn,v);

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
	return 0;
}

int wtk_syn_hmm_cfg_update(wtk_syn_hmm_cfg_t *cfg)
{
	return 0;
}

int wtk_syn_hmm_cfg_update2(wtk_syn_hmm_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return 0;
}
