#include "qtk_rir_estimate_cfg.h"
#include "qtk_rir_estimate2_cfg.h"

int qtk_rir_estimate_cfg_init(qtk_rir_estimate_cfg_t *cfg)
{
	cfg->distance = 0.0;
	cfg->max_val = 429.020265;
	cfg->rt60 = 0.6;
	cfg->lsweep_fn = NULL;
	cfg->ref_fn = NULL;
	cfg->inv_log_sweep = NULL;
	cfg->rir_duration = 10;
	cfg->lookahead = 50.0;
	cfg->st = 1.0;//s
	cfg->kind = 0;
	cfg->rate = 16000;

        qtk_rir_estimate2_cfg_init(&cfg->sweep2);
        return 0;
}
int qtk_rir_estimate_cfg_clean(qtk_rir_estimate_cfg_t *cfg)
{
	if(cfg->inv_log_sweep){
		wtk_free(cfg->inv_log_sweep);
	}
        qtk_rir_estimate2_cfg_clean(&cfg->sweep2);
        return 0;
}
int qtk_rir_estimate_cfg_update_local(qtk_rir_estimate_cfg_t *cfg, wtk_local_cfg_t *m)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret = 0;

	lc=m;
	wtk_local_cfg_update_cfg_f(lc,cfg,distance,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_val,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,rt60,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,lookahead,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,st,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,lsweep_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,ref_fn,v);
        wtk_local_cfg_update_cfg_f(lc, cfg, rir_duration, v);
        wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,hop_size,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,kind,v);

        lc = wtk_local_cfg_find_lc_s(m, "sweep2");
        if (lc) {
            qtk_rir_estimate2_cfg_update_local(&cfg->sweep2, lc);
        }

        if(strcmp(cfg->kind,"sweep") == 0){
		cfg->est_type = WTK_RIR_ESTIMATE;
	}else if(strcmp(cfg->kind,"white") == 0){
		cfg->est_type = WTK_WHITE_NOISE_ESTIMATE;
	}else if(strcmp(cfg->kind,"gcc") == 0){
		cfg->est_type = WTK_GCC_ESTIMATE;
        } else if (strcmp(cfg->kind, "sweep2") == 0) {
            cfg->est_type = WTK_RIR_ESTIMATE2;
        } else {
            wtk_debug("unknown kind:%s\n", cfg->kind);
            ret = -1;
        }
    return ret;
}

int qtk_rir_estimate_load_vec(qtk_rir_estimate_cfg_t *cfg, wtk_source_t *src){
	int ret,len;
	float *val;

	ret = wtk_source_read_int_little(src,&len,1,1);
	val = (float*)wtk_malloc(sizeof(float)*len);
	ret = wtk_source_read_float_little(src, val, len, 1);
	//print_float(val,len);
	cfg->inv_log_sweep = val;
	cfg->nsweep = len;
	return ret;
}

int qtk_rir_estimate_cfg_update(qtk_rir_estimate_cfg_t *cfg)
{
	int ret;
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;

    ret = qtk_rir_estimate_cfg_update2(cfg,&sl);
    if(ret != 0){
        goto end;
    }
    if (cfg->est_type == WTK_RIR_ESTIMATE2) {
        ret = qtk_rir_estimate2_cfg_update(&cfg->sweep2);
    }

end:
	return ret;
}
int qtk_rir_estimate_cfg_update2(qtk_rir_estimate_cfg_t *cfg, wtk_source_loader_t *sl)
{
	int ret = 0;

	if(cfg->lsweep_fn){
		ret = wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)qtk_rir_estimate_load_vec,cfg->lsweep_fn);
	}
	return ret;
}

qtk_rir_estimate_cfg_t* qtk_rir_estimate_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_rir_estimate_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(qtk_rir_estimate_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(qtk_rir_estimate_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void qtk_rir_estimate_cfg_delete(qtk_rir_estimate_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_rir_estimate_cfg_t* qtk_rir_estimate_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	qtk_rir_estimate_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(qtk_rir_estimate_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(qtk_rir_estimate_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void qtk_rir_estimate_cfg_delete_bin(qtk_rir_estimate_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
