#include "wtk_wfst_kv_cfg.h"

int wtk_wfst_kv_cfg_init(wtk_wfst_kv_cfg_t *cfg)
{
	wtk_nglm_cfg_init(&(cfg->nglm));
	wtk_lm_dict_cfg_init(&(cfg->dict));
	wtk_rnn_dec_cfg_init(&(cfg->rnn));
	cfg->nslot=7003;
	cfg->use_rnn=0;
	cfg->oov_pen=0;
	cfg->use_ngram=1;
	cfg->rnn_scale=0.5;
	cfg->ngram_scale=1-cfg->rnn_scale;
	return 0;
}

int wtk_wfst_kv_cfg_clean(wtk_wfst_kv_cfg_t *cfg)
{
	if(cfg->use_rnn)
	{
		wtk_rnn_dec_cfg_clean(&(cfg->rnn));
	}
	wtk_nglm_cfg_clean(&(cfg->nglm));
	wtk_lm_dict_cfg_clean(&(cfg->dict));
	return 0;
}

int wtk_wfst_kv_cfg_update_local(wtk_wfst_kv_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_i(lc,cfg,nslot,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_rnn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ngram,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,oov_pen,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,rnn_scale,v);
	cfg->ngram_scale=1-cfg->rnn_scale;
	wtk_local_cfg_update_cfg_f(lc,cfg,ngram_scale,v);
	lc=wtk_local_cfg_find_lc_s(main,"nglm");
	if(lc)
	{
		ret=wtk_nglm_cfg_update_local(&(cfg->nglm),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"dict");
	if(lc)
	{
		ret=wtk_lm_dict_cfg_update_local(&(cfg->dict),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"rnn");
	if(lc)
	{
		ret=wtk_rnn_dec_cfg_update_local(&(cfg->rnn),lc);
		if(ret!=0){goto end;}
	}
end:
	return 0;
}


int wtk_wfst_kv_cfg_update2(wtk_wfst_kv_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	if(cfg->use_ngram)
	{
		ret=wtk_nglm_cfg_update(&(cfg->nglm));
		if(ret!=0){goto end;}
	}
	ret=wtk_lm_dict_cfg_update2(&(cfg->dict),sl);
	if(ret!=0){goto end;}
	if(cfg->use_rnn)
	{
		ret=wtk_rnn_dec_cfg_update2(&(cfg->rnn),sl);
		if(ret!=0){goto end;}
	}
end:
	return ret;
}

void wtk_wfst_kv_cfg_set_rbin(wtk_wfst_kv_cfg_t *cfg,wtk_rbin2_t *rbin)
{
	cfg->nglm.rbin=rbin;
	cfg->rnn.rbin=rbin;
}

int wtk_wfst_kv_cfg_update(wtk_wfst_kv_cfg_t *cfg)
{
	int ret;

	if(cfg->use_ngram)
	{
		ret=wtk_nglm_cfg_update(&(cfg->nglm));
		if(ret!=0){goto end;}
	}
	ret=wtk_lm_dict_cfg_update(&(cfg->dict));
	if(ret!=0){goto end;}
	if(cfg->use_rnn)
	{
		ret=wtk_rnn_dec_cfg_update(&(cfg->rnn));
		if(ret!=0){goto end;}
	}
end:
	return ret;
}
