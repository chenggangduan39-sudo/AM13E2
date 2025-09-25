#include "wtk_rls3_cfg.h" 

int wtk_rls3_cfg_init(wtk_rls3_cfg_t *cfg)
{
	cfg->L=4;
	cfg->N=1;
    cfg->nl=1;
    cfg->channel=1;

	cfg->lemma=0.99;
    cfg->sigma=1e-10;
    cfg->p=0.75;
    cfg->w_alpha=0.8;

    cfg->use_wx=0;

	cfg->Td=50;
	cfg->nd=0;
	cfg->T60=500;
	cfg->delta=0;
	cfg->rate=16000;
	cfg->step=512;
	cfg->iters=3;

	cfg->px=1e3;

    cfg->use_admm=0;
	cfg->use_zvar=0;

	cfg->Q_eye=0;

	cfg->Q_eye_alpha=-1;  // 0.0001
	cfg->min_lemma=1.0;  // 0.98
	cfg->max_lemma=1.0;  // 0.999

	return 0;
}

int wtk_rls3_cfg_clean(wtk_rls3_cfg_t *cfg)
{
	return 0;
}

int wtk_rls3_cfg_update_local(wtk_rls3_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;

	lc=main;

	wtk_local_cfg_update_cfg_i(lc,cfg,L,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,N,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,lemma,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,w_alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sigma,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,p,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_wx,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,Td,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,T60,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,step,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,iters,v);
	
	wtk_local_cfg_update_cfg_f(lc,cfg,px,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,Q_eye,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,Q_eye_alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_lemma,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_lemma,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_admm,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_zvar,v);

	return 0;
}

int wtk_rls3_cfg_update(wtk_rls3_cfg_t *cfg)
{
	cfg->nl=cfg->L*cfg->N;

	cfg->nd=cfg->Td*1.0*cfg->rate/1000/cfg->step+0.5;
	cfg->delta=3*log(10)/(cfg->T60/cfg->Td);

	return 0;
}
