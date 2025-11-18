#include "qtk_kwfstdec_cfg.h"

int qtk_kwfstdec_cfg_init(qtk_kwfstdec_cfg_t *cfg)
{
	wtk_fst_net_cfg_init(&(cfg->net));
	//qtk_trans_model_cfg_init(&(cfg->trans_model));
	wtk_prune_cfg_init(&(cfg->prune));
	wtk_fst_net3_cfg_init(&(cfg->lat_net));
	wtk_rescore_cfg_init(&(cfg->rescore));
	cfg->trans2phn = NULL;
	cfg->size=1000;
	cfg->beam=18.0;
	cfg->laws_beam = 5.0;
	cfg->beam_delta=0.5;
	cfg->lattice_beam=10.0;
	cfg->max_active=7000;
	cfg->min_active=200;
	cfg->prune_interval=25;
	cfg->frame_skip=1;
	cfg->prune_scale=0.1;
	cfg->lm_scale=1.0;
	cfg->ac_scale=1.0;
	cfg->hot_thresh=0.8;
	cfg->hot_words=NULL;
	cfg->trans_fn=NULL;
	cfg->use_prune=0;
	cfg->use_rescore=0;
    cfg->use_lat=0;
    cfg->use_hot=0;
    cfg->add_softmax=0;
	cfg->remove_label=1;
	cfg->use_eval=0;
	cfg->use_laws_beam = 0;
	cfg->use_memctl=1;
	cfg->pool_scale=2.0;
	cfg->use_context=0;
	cfg->use_av_conf=0;

	//filler for egram rec
	cfg->norm_conf = 1.6;
	cfg->idle_conf = 1.9;
	cfg->idle_hint = 20000;
	cfg->has_filler = 0;
	cfg->use_multi_filler = 0;
	cfg->idle_filler_id = -1;
	cfg->norm_filler_id = -1;

	//trick
	return 0;
}
int qtk_kwfstdec_cfg_clean(qtk_kwfstdec_cfg_t *cfg)
{
	wtk_fst_net_cfg_clean(&(cfg->net));
	wtk_fst_net3_cfg_clean(&(cfg->lat_net));
	wtk_rescore_cfg_clean(&(cfg->rescore));
	//qtk_trans_model_cfg_clean(&(cfg->trans_model));

	if(cfg->trans2phn)
	{
		wtk_free(cfg->trans2phn);
	}

	return 0;
}
int qtk_kwfstdec_cfg_update_local(qtk_kwfstdec_cfg_t *cfg,wtk_local_cfg_t *main)
{
	int ret;
	wtk_local_cfg_t *lc;
	wtk_string_t *v;

	lc=main;
	ret=0;

	wtk_local_cfg_update_cfg_i(lc,cfg,size,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,remove_label,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,use_prune,v);
	//wtk_debug("%d\n",cfg->size);
	wtk_local_cfg_update_cfg_f(lc,cfg,beam,v);
	wtk_local_cfg_update_cfg_f(lc, cfg, laws_beam, v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_active,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_active,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,prune_interval,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,frame_skip,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,lattice_beam,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,beam_delta,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,prune_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,lm_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,ac_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,hot_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,norm_conf,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,idle_conf,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,hot_words,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,trans_fn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_rescore,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_lat,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_hot,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,add_softmax,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_eval,v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_laws_beam, v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_memctl,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pool_scale,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,use_context,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,use_av_conf,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,has_filler,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_multi_filler,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,idle_filler_id,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,norm_filler_id,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,idle_hint,v);

	lc=wtk_local_cfg_find_lc_s(main,"net");
	if(lc)
	{
		ret=wtk_fst_net_cfg_update_local(&(cfg->net),lc);
		if(ret!=0){goto end;}
	}

	if(cfg->use_lat)
	{
		lc=wtk_local_cfg_find_lc_s(main,"lat_net");
		if(lc)
		{
			ret=wtk_fst_net3_cfg_update_local(&(cfg->lat_net),lc);
			if(ret!=0){goto end;}
		}
	}

	if(cfg->use_rescore)
	{
		lc=wtk_local_cfg_find_lc_s(main,"rescore");
		if(lc)
		{
			ret=wtk_rescore_cfg_update_local(&(cfg->rescore),lc);
			if(ret!=0){goto end;}
		}
	}
//	lc=wtk_local_cfg_find_lc_s(main,"trans_model");
//	if(lc)
//	{
//		ret=qtk_trans_model_cfg_update_local(&(cfg->trans_model),lc);
//		if(ret!=0){goto end;}
//	}

	if(cfg->use_prune)
	{
		lc=wtk_local_cfg_find_lc_s(main,"prune");
		if(lc)
		{
			ret=wtk_prune_cfg_update_local(&(cfg->prune),lc);
			if(ret!=0){goto end;}
		}
	}

	end:
	return ret;
}

int qtk_kwfstdec_cfg_phone_info_load(qtk_kwfstdec_cfg_t *cfg,wtk_source_t *src)
{
	int ret = 0,ntrans,i,v;
	qtk_kwfstdec_trans2phn* trans2phn;

	ret = wtk_source_get_lines(&ntrans,src);
	wtk_source_clean_file(src);
	wtk_source_init_file(src,cfg->trans_fn);
	cfg->trans2phn = (qtk_kwfstdec_trans2phn*)wtk_malloc(ntrans*sizeof(qtk_kwfstdec_trans2phn));

	for(i = 0;i < ntrans; i++)
	{
		trans2phn = cfg->trans2phn + i;
		ret = wtk_source_read_int(src, &v, 1, 0);
		ret = wtk_source_read_int(src, &(trans2phn->phn_id), 1, 0);
		ret = wtk_source_read_int(src, &(trans2phn->state_id), 1, 0);
	}
	return ret;
}

int qtk_kwfstdec_cfg_update(qtk_kwfstdec_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;
//	ret=qtk_trans_model_cfg_update(&(cfg->trans_model),sl);
//	if(ret!=0)
//	{
//		wtk_debug("update trans_model failed\n");
//		goto end;
//	}

	ret=wtk_fst_net_cfg_update3(&(cfg->net),NULL,sl);
	if(ret!=0)
	{
		wtk_debug("update net failed\n");
		goto end;
	}

	if(cfg->use_prune)
	{
		ret=wtk_prune_cfg_update(&(cfg->prune));
		if(ret!=0){goto end;}
	}

	if(cfg->use_lat)
	{
		ret=wtk_fst_net3_cfg_update(&(cfg->lat_net));
		if(ret!=0){goto end;}
	}

	if(cfg->use_rescore)
	{
		ret=wtk_rescore_cfg_update2(&(cfg->rescore),sl);
		if(ret!=0)
		{
            wtk_debug("update rescore failed\n");
			goto end;
		}
		wtk_rescore_set_sym_out(&(cfg->rescore),cfg->net.sym_out);
	}

	if(cfg->trans_fn)// add for eval
	{
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)qtk_kwfstdec_cfg_phone_info_load,cfg->trans_fn);
	}

	end:
	return ret;
}
void qtk_kwfstdec_cfg_print(qtk_kwfstdec_cfg_t *cfg)
{

}
