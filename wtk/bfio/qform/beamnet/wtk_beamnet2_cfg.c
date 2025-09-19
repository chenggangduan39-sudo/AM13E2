#include "wtk/bfio/qform/beamnet/wtk_beamnet2_cfg.h"

int wtk_beamnet2_cfg_init(wtk_beamnet2_cfg_t *cfg) {

	cfg->channel=0;
	cfg->nmicchannel=0;
	cfg->mic_channel=NULL;
	cfg->nspchannel=0;
	cfg->sp_channel=NULL;
	cfg->nbfchannel=0;
	cfg->nmic=0;
	cfg->mic_pos=NULL;
	cfg->out_channels=0;
	cfg->feature_len=15;
	cfg->sv=340;
	cfg->feature_type=1;
	cfg->gf_channel = 0;
	cfg->mic_scale=NULL;

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

int wtk_beamnet2_cfg_clean(wtk_beamnet2_cfg_t *cfg) {
	int i;
	if(cfg->mic_channel)
	{
		wtk_free(cfg->mic_channel);
	}
	if(cfg->sp_channel)
	{
		wtk_free(cfg->sp_channel);
	}
	if(cfg->mic_pos)
	{
		for(i=0;i<cfg->nmic;++i)
		{
			wtk_free(cfg->mic_pos[i]);
		}
		wtk_free(cfg->mic_pos);
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

int wtk_beamnet2_cfg_update(wtk_beamnet2_cfg_t *cfg) {
	int ret;

	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=qtk_ahs_gain_controller_cfg_update(&(cfg->gc));
	if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->covm));
	if(ret!=0){goto end;}
	if(cfg->channel<cfg->nmicchannel+cfg->nspchannel)
	{
		cfg->channel=cfg->nmicchannel+cfg->nspchannel;
	}
	if(cfg->nbfchannel==0){
		cfg->nbfchannel=cfg->nmicchannel;
		}
	cfg->bf.nmic = cfg->nbfchannel;
	cfg->out_channels = cfg->nmicchannel * (cfg->nmicchannel - 1) / 2;
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
	if(!cfg->mic_scale){
		cfg->mic_scale=(float*)wtk_malloc(cfg->nmic*sizeof(float));
		for(int i=0;i<cfg->nmic;++i){
			cfg->mic_scale[i]=1.0;
		}
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

int wtk_beamnet2_cfg_update2(wtk_beamnet2_cfg_t *cfg, wtk_source_loader_t *sl) {
	int ret;

	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=qtk_ahs_gain_controller_cfg_update(&(cfg->gc));
	if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->covm));
	if(ret!=0){goto end;}
	if(cfg->channel<cfg->nmicchannel+cfg->nspchannel)
	{
		cfg->channel=cfg->nmicchannel+cfg->nspchannel;
	}
	if(cfg->nbfchannel==0){
		cfg->nbfchannel=cfg->nmicchannel;
		}
	cfg->bf.nmic = cfg->nbfchannel;
	cfg->out_channels = cfg->nmicchannel * (cfg->nmicchannel - 1) / 2;
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
	if(!cfg->mic_scale){
		cfg->mic_scale=(float*)wtk_malloc(cfg->nmic*sizeof(float));
		for(int i=0;i<cfg->nmic;++i){
			cfg->mic_scale[i]=1.0;
		}
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

int wtk_beamnet2_cfg_update_local(wtk_beamnet2_cfg_t *cfg, wtk_local_cfg_t *m) {
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;
	wtk_array_t *a;
	int i;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,feature_len,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sv,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,feature_type,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nbfchannel,v);
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

	a=wtk_local_cfg_find_array_s(lc,"mic_channel");
	if(a)
	{
		cfg->mic_channel=(int*)wtk_malloc(sizeof(int)*a->nslot);
		cfg->nmicchannel=a->nslot;
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->mic_channel[i]=wtk_str_atoi(v->data,v->len);
		}
	}

	a=wtk_local_cfg_find_array_s(lc,"sp_channel");
	if(a)
	{
		cfg->sp_channel=(int*)wtk_malloc(sizeof(int)*a->nslot);
		cfg->nspchannel=a->nslot;
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->sp_channel[i]=wtk_str_atoi(v->data,v->len);
		}
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
		if(cfg->nmic!=cfg->nmicchannel)
		{
			wtk_debug("error: nmic=%d!=nmicchannel=%d\n",cfg->nmic,cfg->nmicchannel);
			exit(1);
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

wtk_beamnet2_cfg_t* wtk_beamnet2_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_beamnet2_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_beamnet2_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_beamnet2_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_beamnet2_cfg_delete(wtk_beamnet2_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_beamnet2_cfg_t* wtk_beamnet2_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_beamnet2_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_beamnet2_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_beamnet2_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_beamnet2_cfg_delete_bin(wtk_beamnet2_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
