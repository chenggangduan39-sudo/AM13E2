#include "wtk/bfio/qform/beamnet/wtk_beamnet4_cfg.h"

int wtk_beamnet4_cfg_init(wtk_beamnet4_cfg_t *cfg) {

	cfg->channel=0;
	cfg->nmics=NULL;
	cfg->nmicchannel=0;
	cfg->nmicchannels=NULL;
	cfg->mic_channel=NULL;
	cfg->nbfchannel=0;
	cfg->in_channels=0;
	cfg->out_channels=NULL;
	cfg->out_channel=0;
	cfg->covar_channels=0;
	cfg->mic_pos=NULL;
	cfg->feature_len=15;
	cfg->sv=340;
	cfg->sep_feature_type=3;
	cfg->bf_feature_type=1;
	cfg->feat_scaler=1.0;
	cfg->scaler=1.0;
	cfg->cross_scaler=1.0;
	cfg->target_theta=NULL;
	cfg->target_delay=NULL;
	cfg->mic_scale=NULL;
	cfg->narray = 2;
	cfg->gf_channel = 0;

    cfg->wins=1024;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->rate=16000;

#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_init(&(cfg->seperator));
    qtk_onnxruntime_cfg_init(&(cfg->beamformer));
#endif
    wtk_qmmse_cfg_init(&(cfg->qmmse));
	qtk_ahs_gain_controller_cfg_init(&(cfg->gc));
    wtk_covm_cfg_init(&(cfg->covm));
    wtk_bf_cfg_init(&(cfg->bf));

	cfg->entropy_thresh=-1;
	cfg->entropy_in_cnt=2;
	cfg->entropy_cnt=20;
	cfg->entropy_ratio=4.0;
	cfg->entropy_min_scale=0.05;

	cfg->delay_nf=0;

	cfg->bfmu=1;
	cfg->bfmu2=1;

	cfg->clip_s=0;
	cfg->clip_e=8000;
	cfg->bf_clip_s=0;
	cfg->bf_clip_e=8000;

	cfg->bfflush_cnt=1;

	cfg->max_bs_out = 32700.0;
	cfg->use_bs_win = 0;
	cfg->gc_gain = 50000.0;
	cfg->gc_min_thresh = 0;
	cfg->gc_cnt = 10;
	cfg->out_agc_level = -1;

    cfg->use_onnx = 1;
    cfg->norm_channel = 1;
    cfg->use_mvdr = 1;
    cfg->use_qmmse = 0;
    cfg->use_bf = 0;
    cfg->use_sim_bf = 0;
    cfg->use_fixtheta = 0;
	cfg->use_gc = 0;
	cfg->mean_gf_thresh = 0;
    return 0;
}

int wtk_beamnet4_cfg_clean(wtk_beamnet4_cfg_t *cfg) {
    int i,j;
	if(cfg->mic_channel)
	{
		for(i=0;i<cfg->narray;++i){
			wtk_free(cfg->mic_channel[i]);
		}
		wtk_free(cfg->mic_channel);
    }
	if(cfg->mic_pos)
	{
		for(i=0;i<cfg->narray;++i){
			for(j=0;j<cfg->nmicchannels[i];++j){
				wtk_free(cfg->mic_pos[i][j]);
			}
			wtk_free(cfg->mic_pos[i]);
		}
		wtk_free(cfg->mic_pos);
    }
	if(cfg->nmics){
		wtk_free(cfg->nmics);
	}
	if(cfg->nmicchannels){
		wtk_free(cfg->nmicchannels);
	}
	if(cfg->out_channels){
		wtk_free(cfg->out_channels);
	}
	if(cfg->target_theta){
		wtk_free(cfg->target_theta);
	}
	if(cfg->target_delay){
		wtk_free(cfg->target_delay);
	}
	if(cfg->mic_scale){
		wtk_free(cfg->mic_scale);
	}
#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_clean(&(cfg->seperator));
    qtk_onnxruntime_cfg_clean(&(cfg->beamformer));
#endif
    wtk_qmmse_cfg_clean(&(cfg->qmmse));
	qtk_ahs_gain_controller_cfg_clean(&(cfg->gc));
    wtk_bf_cfg_clean(&(cfg->bf));
    wtk_covm_cfg_clean(&(cfg->covm));

    return 0;
}

int wtk_beamnet4_cfg_update(wtk_beamnet4_cfg_t *cfg) {
    int ret;
	int i;

	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=qtk_ahs_gain_controller_cfg_update(&(cfg->gc));
	if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->covm));
	if(ret!=0){goto end;}
	cfg->nmicchannel=0;
	for(i=0;i<cfg->narray;++i){
		cfg->nmicchannel+=cfg->nmicchannels[i];
	}
	if(cfg->channel<cfg->nmicchannel)
	{
		cfg->channel=cfg->nmicchannel;
    }
	if(cfg->nbfchannel==0){
		cfg->nbfchannel=cfg->nmicchannel;
    }
    cfg->bf.nmic = cfg->nbfchannel;
	cfg->out_channel = 0;
	for(i=0;i<cfg->narray;++i){
		cfg->out_channels[i] = cfg->nmicchannels[i] * (cfg->nmicchannels[i] - 1) / 2;
		cfg->out_channel += cfg->out_channels[i];
	}
    cfg->covar_channels = cfg->nmicchannel * (cfg->nmicchannel - 1) / 2;
	if(cfg->sep_feature_type==0){
		cfg->in_channels = cfg->covar_channels + 1 + cfg->narray;
	}else if(cfg->sep_feature_type==1 || cfg->sep_feature_type==2){
		cfg->in_channels = cfg->covar_channels * 2 + 2 + cfg->narray;
	}else if(cfg->sep_feature_type==3 || cfg->sep_feature_type==4){
		cfg->in_channels = cfg->covar_channels * 2 + 3 * cfg->narray;
	}else if(cfg->sep_feature_type==5){
		cfg->in_channels = cfg->covar_channels * 2 + 2 + cfg->narray;
	}
    cfg->entropy_cnt = cfg->entropy_cnt + cfg->delay_nf;
	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;
	cfg->bf_clip_s=(cfg->bf_clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->bf_clip_e=(cfg->bf_clip_e*1.0*cfg->wins)/cfg->rate;

	if(cfg->out_agc_level==1){
		cfg->max_bs_out=pow(10, 75.0/20.0);
	}else if(cfg->out_agc_level==2){
		cfg->max_bs_out=pow(10, 78.0/20.0);
	}else if(cfg->out_agc_level==3){
		cfg->max_bs_out=pow(10, 81.0/20.0);
	}else if(cfg->out_agc_level==4){
		cfg->max_bs_out=pow(10, 84.0/20.0);
	}else if(cfg->out_agc_level==5){
		cfg->max_bs_out=pow(10, 87.0/20.0);
	}else if(cfg->out_agc_level>=6){
		cfg->max_bs_out=pow(10, 90.0/20.0);
	}
#ifdef ONNX_DEC
	if(cfg->use_onnx){
        ret = qtk_onnxruntime_cfg_update(&(cfg->seperator));
        if (ret != 0) {
            wtk_debug("update onnx failed\n");
            goto end;
        }
        ret = qtk_onnxruntime_cfg_update(&(cfg->beamformer));
        if (ret != 0) {
            wtk_debug("update onnx failed\n");
            goto end;
        }
    }
#endif
	ret=0;
end:
    return ret;
}

int wtk_beamnet4_cfg_update2(wtk_beamnet4_cfg_t *cfg, wtk_source_loader_t *sl) {
    int ret;
	int i;

	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=qtk_ahs_gain_controller_cfg_update(&(cfg->gc));
	if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->covm));
	if(ret!=0){goto end;}
	cfg->nmicchannel=0;
	for(i=0;i<cfg->narray;++i){
		cfg->nmicchannel+=cfg->nmicchannels[i];
	}
	if(cfg->channel<cfg->nmicchannel)
	{
		cfg->channel=cfg->nmicchannel;
    }
	if(cfg->nbfchannel==0){
		cfg->nbfchannel=cfg->nmicchannel;
    }
    cfg->bf.nmic = cfg->nbfchannel;
	cfg->out_channel = 0;
	for(i=0;i<cfg->narray;++i){
		cfg->out_channels[i] = cfg->nmicchannels[i] * (cfg->nmicchannels[i] - 1) / 2;
		cfg->out_channel += cfg->out_channels[i];
	}
    cfg->covar_channels = cfg->nmicchannel * (cfg->nmicchannel - 1) / 2;
	if(cfg->sep_feature_type==0){
		cfg->in_channels = cfg->covar_channels + 1 + cfg->narray;
	}else if(cfg->sep_feature_type==1 || cfg->sep_feature_type==2){
		cfg->in_channels = cfg->covar_channels * 2 + 2 + cfg->narray;
	}else if(cfg->sep_feature_type==3 || cfg->sep_feature_type==4){
		cfg->in_channels = cfg->covar_channels * 2 + 3 * cfg->narray;
	}else if(cfg->sep_feature_type==5){
		cfg->in_channels = cfg->covar_channels * 2 + 2 + cfg->narray;
	}
    cfg->entropy_cnt = cfg->entropy_cnt + cfg->delay_nf;
	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;
	cfg->bf_clip_s=(cfg->bf_clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->bf_clip_e=(cfg->bf_clip_e*1.0*cfg->wins)/cfg->rate;

	if(cfg->out_agc_level==1){
		cfg->max_bs_out=pow(10, 75.0/20.0);
	}else if(cfg->out_agc_level==2){
		cfg->max_bs_out=pow(10, 78.0/20.0);
	}else if(cfg->out_agc_level==3){
		cfg->max_bs_out=pow(10, 81.0/20.0);
	}else if(cfg->out_agc_level==4){
		cfg->max_bs_out=pow(10, 84.0/20.0);
	}else if(cfg->out_agc_level==5){
		cfg->max_bs_out=pow(10, 87.0/20.0);
	}else if(cfg->out_agc_level>=6){
		cfg->max_bs_out=pow(10, 90.0/20.0);
	}
#ifdef ONNX_DEC
	if(cfg->use_onnx){
        ret = qtk_onnxruntime_cfg_update2(&(cfg->seperator), sl->hook);
        if (ret != 0) {
            wtk_debug("update onnx failed\n");
            goto end;
        }
        ret = qtk_onnxruntime_cfg_update2(&(cfg->beamformer), sl->hook);
        if (ret != 0) {
            wtk_debug("update onnx failed\n");
            goto end;
        }
    }
#endif
	ret=0;
end:
    return ret;
}

int wtk_beamnet4_cfg_update_local(wtk_beamnet4_cfg_t *cfg, wtk_local_cfg_t *m) {
    wtk_string_t *v;
    wtk_local_cfg_t *lc;
    int ret;
    wtk_array_t *a;
    int i;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,feature_len,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sv,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,sep_feature_type,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bf_feature_type,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,feat_scaler,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,scaler,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,cross_scaler,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nbfchannel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,narray,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,gf_channel,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_onnx,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,norm_channel,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_mvdr,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_bf,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_sim_bf,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fixtheta,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,entropy_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,entropy_in_cnt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,entropy_cnt,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,entropy_min_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,entropy_ratio,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,delay_nf,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,bfmu,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,bfmu2,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,clip_e,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bf_clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bf_clip_e,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,bfflush_cnt,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,max_bs_out,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_bs_win,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,gc_gain,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,gc_min_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,gc_cnt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,out_agc_level,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_gc,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,mean_gf_thresh,v);

	cfg->nmicchannels=(int *)wtk_malloc(sizeof(int)*cfg->narray);
	cfg->out_channels=(int *)wtk_malloc(sizeof(int)*cfg->narray);
	cfg->mic_channel=(int**)wtk_malloc(sizeof(int*)*cfg->narray);
	cfg->mic_pos=(float ***)wtk_malloc(sizeof(float**)*cfg->narray);
	cfg->nmics=(int*)wtk_malloc(sizeof(int)*cfg->narray);

	a=wtk_local_cfg_find_array_s(lc,"mic1_channel");
	if(a)
	{
		cfg->mic_channel[0]=(int*)wtk_malloc(sizeof(int)*a->nslot);
		cfg->nmicchannels[0]=a->nslot;
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->mic_channel[0][i]=wtk_str_atoi(v->data,v->len);
        }
    }
	a=wtk_local_cfg_find_array_s(lc,"mic2_channel");
	if(a)
	{
		cfg->mic_channel[1]=(int*)wtk_malloc(sizeof(int)*a->nslot);
		cfg->nmicchannels[1]=a->nslot;
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->mic_channel[1][i]=wtk_str_atoi(v->data,v->len);
        }
    }

	lc=wtk_local_cfg_find_lc_s(m,"mic1");
	if(lc)
	{
        wtk_queue_node_t *qn;
        wtk_cfg_item_t *item;
        int i;

		cfg->mic_pos[0]=(float**)wtk_malloc(sizeof(float*)*lc->cfg->queue.length);
		cfg->nmics[0]=0;
		for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			if(item->type!=WTK_CFG_ARRAY || item->value.array->nslot!=3){continue;}
			cfg->mic_pos[0][cfg->nmics[0]]=(float*)wtk_malloc(sizeof(float)*3);
			for(i=0;i<3;++i)
			{
				v=((wtk_string_t**)item->value.array->slot)[i];
				cfg->mic_pos[0][cfg->nmics[0]][i]=wtk_str_atof(v->data,v->len);
                // wtk_debug("v[%d][%d]=%f\n",cfg->nmics[0],i,cfg->mic_pos[0][cfg->nmics[0]][i]);
            }
            ++cfg->nmics[0];
        }
		if(cfg->nmics[0]!=cfg->nmicchannels[0])
		{
			wtk_debug("error: nmics[0]=%d!=nmicchannels[0]=%d\n",cfg->nmics[0],cfg->nmicchannels[0]);
            exit(1);
        }
    }

	lc=wtk_local_cfg_find_lc_s(m,"mic2");
	if(lc)
	{
        wtk_queue_node_t *qn;
        wtk_cfg_item_t *item;
        int i;

		cfg->mic_pos[1]=(float**)wtk_malloc(sizeof(float*)*lc->cfg->queue.length);
		cfg->nmics[1]=0;
		for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			if(item->type!=WTK_CFG_ARRAY || item->value.array->nslot!=3){continue;}
			cfg->mic_pos[1][cfg->nmics[1]]=(float*)wtk_malloc(sizeof(float)*3);
			for(i=0;i<3;++i)
			{
				v=((wtk_string_t**)item->value.array->slot)[i];
				cfg->mic_pos[1][cfg->nmics[1]][i]=wtk_str_atof(v->data,v->len);
                // wtk_debug("v[%d][%d]=%f\n",cfg->nmics[1],i,cfg->mic_pos[1][cfg->nmics[1]][i]);
            }
            ++cfg->nmics[1];
        }
		if(cfg->nmics[1]!=cfg->nmicchannels[1])
		{
			wtk_debug("error: nmics[1]=%d!=nmicchannels[1]=%d\n",cfg->nmics[1],cfg->nmicchannels[1]);
            exit(1);
        }
    }

	a=wtk_local_cfg_find_array_s(lc,"target_theta");
	if(a)
	{
		cfg->target_theta=(float*)wtk_malloc(sizeof(float)*a->nslot);
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->target_theta[i]=wtk_str_atof(v->data,v->len);
        }
		if(a->nslot!=cfg->narray){
			wtk_debug("error: target_theta.nslot=%d!=narray=%d\n",a->nslot,cfg->narray);
		}
    }

	a=wtk_local_cfg_find_array_s(lc,"target_delay");
	if(a)
	{
		cfg->target_delay=(float*)wtk_malloc(sizeof(float)*a->nslot);
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->target_delay[i]=wtk_str_atof(v->data,v->len);
        }
    }

	a=wtk_local_cfg_find_array_s(lc,"mic_scale");
	if(a)
	{
		cfg->mic_scale=(float*)wtk_malloc(sizeof(float)*a->nslot);
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->mic_scale[i]=wtk_str_atof(v->data,v->len);
        }
    }

	lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
		cfg->qmmse.step=cfg->wins/2;
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"gc");
	if(lc)
	{
        ret=qtk_ahs_gain_controller_cfg_update_local(&(cfg->gc),lc);
		if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"bf");
	if(lc)
	{
		ret=wtk_bf_cfg_update_local(&(cfg->bf),lc);
		if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"covm");
	if(lc)
	{
        ret=wtk_covm_cfg_update_local(&(cfg->covm),lc);
        if(ret!=0){goto end;}
    }

#ifdef ONNX_DEC
    lc = wtk_local_cfg_find_lc_s(m, "seperator");
    if (lc) {
        ret = qtk_onnxruntime_cfg_update_local(&(cfg->seperator), lc);
        if (ret != 0) {
            wtk_debug("update local onnx failed\n");
            goto end;
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "beamformer");
    if (lc) {
        ret = qtk_onnxruntime_cfg_update_local(&(cfg->beamformer), lc);
        if (ret != 0) {
            wtk_debug("update local onnx failed\n");
            goto end;
        }
    }
#endif
    ret = 0;
end:
    return ret;
}

wtk_beamnet4_cfg_t* wtk_beamnet4_cfg_new(char *fn)
{
    wtk_main_cfg_t *main_cfg;
    wtk_beamnet4_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_beamnet4_cfg,fn);
	if(!main_cfg)
	{
        return NULL;
    }
	cfg=(wtk_beamnet4_cfg_t*)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void wtk_beamnet4_cfg_delete(wtk_beamnet4_cfg_t *cfg)
{
    wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_beamnet4_cfg_t* wtk_beamnet4_cfg_new_bin(char *fn)
{
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_beamnet4_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_beamnet4_cfg,fn,"./beamnet4.cfg");
	if(!mbin_cfg)
	{
        return NULL;
    }
	cfg=(wtk_beamnet4_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
    return cfg;
}

void wtk_beamnet4_cfg_delete_bin(wtk_beamnet4_cfg_t *cfg)
{
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
