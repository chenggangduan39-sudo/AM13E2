#include "qtk_sv_cluster_cfg.h"

int qtk_sv_cluster_cfg_init(qtk_sv_cluster_cfg_t *cfg)
{
    memset(cfg,0,sizeof(qtk_sv_cluster_cfg_t));
    wtk_nnet3_xvector_compute_cfg_init(&cfg->xvector);
    wtk_ivector_plda_scoring_cfg_init(&cfg->plda);
    wtk_kvad_cfg_init(&cfg->kvad);
    cfg->thresh1 = 15.0;
    cfg->thresh2 = 7.0;
    cfg->mode=0;
    cfg->svprint_nn_fn = 0;
    cfg->pool_fn = 0;
    cfg->min_len = 12000;
    return 0;
}

int qtk_sv_cluster_cfg_update_local(qtk_sv_cluster_cfg_t *cfg,wtk_local_cfg_t *main_lc)
{
    wtk_string_t *v = NULL;
    wtk_local_cfg_t *lc = NULL;
    wtk_local_cfg_update_cfg_str(main_lc,cfg,svprint_nn_fn,v);
    wtk_local_cfg_update_cfg_str(main_lc,cfg,pool_fn,v);
    wtk_local_cfg_update_cfg_f(main_lc,cfg,thresh1,v);
    wtk_local_cfg_update_cfg_f(main_lc,cfg,thresh2,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,mode,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,min_len,v);

    lc = wtk_local_cfg_find_lc_s(main_lc, "xvector");
    if(lc){
        wtk_nnet3_xvector_compute_cfg_update_local(&cfg->xvector,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_lc, "vad");
    if(lc){
        wtk_kvad_cfg_update_local(&cfg->kvad,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_lc,"plda");
    if(lc){
        wtk_ivector_plda_scoring_cfg_update_local(&cfg->plda,lc);
    }
    return 0;
}

int qtk_sv_cluster_cfg_update(qtk_sv_cluster_cfg_t *cfg)
{
	cfg->sl.hook=0;
	cfg->sl.vf=wtk_source_load_file_v;
	qtk_sv_cluster_cfg_update2(cfg,&cfg->sl);
    return 0;
}

int qtk_sv_cluster_cfg_update2(qtk_sv_cluster_cfg_t *cfg,wtk_source_loader_t *sl)
{
	cfg->sl = *sl;
    wtk_nnet3_xvector_compute_cfg_update2(&cfg->xvector,sl);
    wtk_ivector_plda_scoring_cfg_update2(&cfg->plda,sl);
    wtk_kvad_cfg_update2(&cfg->kvad,sl);
    return 0;
}

int qtk_sv_cluster_cfg_clean(qtk_sv_cluster_cfg_t *cfg)
{
    wtk_nnet3_xvector_compute_cfg_clean(&cfg->xvector);
    wtk_ivector_plda_scoring_cfg_clean(&cfg->plda);
    wtk_kvad_cfg_clean(&cfg->kvad);

    return 0;
}

qtk_sv_cluster_cfg_t* qtk_sv_cluster_cfg_new_bin2(char *bin_fn,char *cfg_fn)
{
	wtk_mbin_cfg_t *cfg;
	qtk_sv_cluster_cfg_t *vc;

	//wtk_debug("%s/%s\n",bin_fn,cfg_fn);
	cfg=wtk_mbin_cfg_new_type(qtk_sv_cluster_cfg,bin_fn,cfg_fn);
	vc=(qtk_sv_cluster_cfg_t*)(cfg->cfg);
	vc->hook=cfg;
	return vc;
}

qtk_sv_cluster_cfg_t* qtk_sv_cluster_cfg_new_bin(char *bin_fn)
{
	return qtk_sv_cluster_cfg_new_bin2(bin_fn,"./cluster.cfg");
}

qtk_sv_cluster_cfg_t* qtk_sv_cluster_cfg_new_bin3(char *bin_fn,unsigned int seek_pos)
{
	qtk_sv_cluster_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	char *cfg_fn="./cluster.cfg";
	int ret;

	cfg=(qtk_sv_cluster_cfg_t*)wtk_malloc(sizeof(qtk_sv_cluster_cfg_t));
	qtk_sv_cluster_cfg_init(cfg);
	cfg->rbin=wtk_rbin2_new();
	ret=wtk_rbin2_read2(cfg->rbin,bin_fn,seek_pos);
	if(ret!=0){
		wtk_debug("read failed\n");
		goto end;
	}
	item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	if(!item)
	{
		wtk_debug("%s not found %s\n",cfg_fn,bin_fn);
		ret=-1;goto end;
	}
	cfg->cfile=wtk_cfg_file_new();
	//wtk_debug("f=%p\n",cfg->rbin->f);
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	ret=wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	if(ret!=0){goto end;}
	ret=qtk_sv_cluster_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}
	cfg->sl.hook=cfg->rbin;
	cfg->sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=qtk_sv_cluster_cfg_update2(cfg,&(cfg->sl));
	if(ret!=0){goto end;}
end:
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		qtk_sv_cluster_cfg_delete_bin2(cfg);
		cfg=NULL;
	}
	return cfg;
}

int qtk_sv_cluster_cfg_delete_bin(qtk_sv_cluster_cfg_t *cfg)
{
	wtk_mbin_cfg_delete((wtk_mbin_cfg_t*)(cfg->hook));
	return 0;
}

int qtk_sv_cluster_cfg_delete_bin2(qtk_sv_cluster_cfg_t *cfg)
{
	qtk_sv_cluster_cfg_clean(cfg);
	wtk_free(cfg);
	return 0;
}

qtk_sv_cluster_cfg_t* qtk_sv_cluster_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_sv_cluster_cfg_t *vc;

	main_cfg=wtk_main_cfg_new_type(qtk_sv_cluster_cfg,cfg_fn);
	vc=(qtk_sv_cluster_cfg_t*)(main_cfg->cfg);
	vc->hook=main_cfg;
	return vc;
}

void qtk_sv_cluster_cfg_delete(qtk_sv_cluster_cfg_t *cfg)
{
	wtk_main_cfg_delete((wtk_main_cfg_t*)(cfg->hook));
}
