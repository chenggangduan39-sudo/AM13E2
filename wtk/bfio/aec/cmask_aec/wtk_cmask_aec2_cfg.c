#include "wtk/bfio/aec/cmask_aec/wtk_cmask_aec2_cfg.h"


void wtk_cmask_aec2_cfg_get_mic_theta(wtk_cmask_aec2_cfg_t *cfg)
{
	int i;
	float x, y;
	float angle_rad;
	float angle_deg;

	cfg->mic_theta=(float *)wtk_malloc(sizeof(float)*cfg->nmicchannel);
	for(i=0;i<cfg->nmicchannel;++i){
		x = cfg->bf.mic_pos[i][0];
		y = cfg->bf.mic_pos[i][1];
		angle_rad = atan2(y, x);
		angle_deg = angle_rad * 180.0 / M_PI;
		if(angle_deg < 0){
			angle_deg += 360.0;
		}
		cfg->mic_theta[i] = angle_deg;
		// printf("mic %d mic_idx %d theta: %f\n", i, cfg->mic_channel[i], angle_deg);
	}
}

int wtk_cmask_aec2_cfg_init(wtk_cmask_aec2_cfg_t *cfg) {

	cfg->channel=0;
	cfg->nmicchannel=0;
	cfg->mic_channel=NULL;
	cfg->nspchannel=0;
	cfg->sp_channel=NULL;
	cfg->change_mic=NULL;
	cfg->ncmicchannel=0;
	cfg->change_ref=NULL;
	cfg->ncrefchannel=0;
	cfg->ncchannel=0;
	cfg->nbfchannel=0;
	cfg->nibfchannel=0;
	cfg->mic_theta=NULL;
	cfg->eq_fn=NULL;
	cfg->eq_gain=NULL;
	cfg->eq_len=0;

    cfg->wins=1024;
	cfg->sv=334;

	cfg->use_rls=1;
	wtk_rls_cfg_init(&(cfg->echo_rls));
	cfg->use_rls3=0;
	wtk_rls3_cfg_init(&(cfg->echo_rls3));
	cfg->use_nlms=0;
	wtk_nlms_cfg_init(&(cfg->echo_nlms));

	wtk_qmmse_cfg_init(&(cfg->qmmse));
	wtk_qmmse_cfg_init(&(cfg->qmmse2));
	qtk_ahs_gain_controller_cfg_init(&(cfg->gc));

	cfg->use_maskssl2=0;
	wtk_maskssl2_cfg_init(&(cfg->maskssl2));
	cfg->use_ibf_ssl=0;
	wtk_maskssl2_cfg_init(&(cfg->ibf_ssl));

	cfg->spenr_thresh=100;
	cfg->spenr_cnt=10;

	cfg->micenr_thresh=300;
	cfg->micenr_cnt=10;
	cfg->micenr_thresh2=5000;
	cfg->micenr_cnt2=80;
	cfg->micenr_thresh3=2000;
	cfg->micenr_cnt3=40;
	cfg->micenr_scale=1.0;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->rate=16000;

	wtk_equalizer_cfg_init(&(cfg->eq));
	cfg->use_eq=0;

	cfg->clip_s=0;
	cfg->clip_e=8000;

	cfg->use_qmmse=1;
	cfg->use_qmmse2=0;

	cfg->use_cnon=0;
	cfg->sym=1e-2;

#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_init(&(cfg->onnx));
#endif
	cfg->num_frame = 1;
	cfg->entropy_thresh=-1;
	cfg->entropy_sp_thresh=-1;
	cfg->entropy_in_cnt=2;
	cfg->entropy_cnt=20;
	cfg->use_ccm=0;
	cfg->use_entropy=0;
	cfg->raw_alpha1=1.0;
	cfg->raw_alpha2=1.0;
	cfg->raw_alpha3=1.0;
	cfg->raw_alpha4=1.0;
	cfg->pow_scale=0.25;
	cfg->alpha1 = 1.0;
	cfg->alpha2 = 1.0;
	cfg->alpha3 = 1.0;
	cfg->alpha4 = 1.0;
	cfg->scale1=1.0;
	cfg->scale2=1.0;
	cfg->scale3=1.0;
	cfg->scale4=1.0;
	cfg->init_low_freq=0;
	cfg->init_high_freq=0;

	cfg->de_clip_s = 0;
	cfg->de_clip_e = 8000;
	cfg->de_thresh = 1e4;
	cfg->de_alpha = 1.0;

	cfg->eng_scale = 1.0;
	cfg->eng1_thresh = 10.0;
	cfg->eng1_thresh2 = 10.0;
	cfg->eng2_thresh = 2000;
	cfg->eng2_thresh2 = 2000;
	cfg->eng_freq = 200;
	cfg->eng_cnt = 80;

	cfg->sp_max_smooth_gain=-1;
	cfg->sp_min_smooth_gain=-1;
	cfg->mic_min_smooth_gain=-1;
	cfg->mic_max_smooth_gain=-1;

	wtk_covm_cfg_init(&(cfg->covm));
	wtk_covm_cfg_init(&(cfg->echo_covm));
	wtk_covm_cfg_init(&(cfg->icovm));
	wtk_bf_cfg_init(&(cfg->bf));
	wtk_bf_cfg_init(&(cfg->ibf));
	cfg->bfmu=1;
	cfg->bfmu2=1;
	cfg->echo_bfmu=1;
	cfg->echo_bfmu2=1;
	cfg->bf_clip_s=0;
	cfg->bf_clip_e=8000;
	cfg->use_bf=0;
	cfg->use_echocovm=1;

    qtk_nnrt_cfg_init(&cfg->rt);
	cfg->num_in = 2;
	cfg->num_out = 4;

	cfg->use_onnx = 1;
	cfg->use_ncnn = 0;

	qtk_nnrt_cfg_init(&cfg->dereb_rt);
	cfg->dereb_num_in = 1;
	cfg->dereb_num_out = 1;
	cfg->dereb_delay = 5;
	cfg->dereb_taps = 10;
	cfg->dereb_alpha = 0.99;

	cfg->use_dereb = 0;

	cfg->wpe_thresh = 5000;
	cfg->wpe_cnt = 10;
	cfg->use_wpe = 0;
	cfg->wpe_alpha = 0.85;
	cfg->wpe_power = 2.0;

	cfg->use_change_mic=0;
	cfg->change_thresh=1.0;
	cfg->change_cnt=10;
	cfg->change_alpha=0.9;
	cfg->change_alpha2=0.999;
	cfg->use_freq_preemph=0;
	cfg->pre_pow_ratio=2.0;
	cfg->pre_mul_ratio=2.0;
	cfg->pre_clip_s=4000;
	cfg->pre_clip_e=8000;
	cfg->change_delay=100;

	cfg->entropy_ratio=4.0;
	cfg->entropy_min_scale=0.05;

	cfg->max_bs_out=32000;
	cfg->use_bs_win = 0;
	cfg->use_change_mic2=0;
	cfg->use_echo_bf=1;
	cfg->use_entropy_scale=0;
	cfg->use_ibf=0;
	cfg->sp_ibf_delay=3000;

	cfg->ibf_theta=90;
	cfg->ibf_thresh=0.15;
	cfg->init_ibf_thresh=0.25;
	cfg->gc_gain = 50000.0;
	cfg->gc_min_thresh = 0.2;
	cfg->gc_cnt = 10;
	cfg->out_agc_level = -1;

	cfg->use_mask_bf=0;
	cfg->use_in_eq=0;
	cfg->use_out_eq=0;
	cfg->use_scale_qmmse=0;
	cfg->use_scale_qmmse2=0;

	cfg->bf_theta=-1;
	cfg->use_ds=0;
	cfg->ds_w_alpha=0;
	cfg->use_freq_atten=0;
	cfg->gain_alpha=0.1f;
	cfg->gain_alpha2=0.6f;
	cfg->gain_beta=5.0f;

	cfg->max_mask=1.0;
	cfg->mask_peak=0;

	cfg->t_mic_in_scale=1;
	cfg->t_sp_in_scale=1;
    return 0;
}

int wtk_cmask_aec2_cfg_clean(wtk_cmask_aec2_cfg_t *cfg) {
	if(cfg->mic_channel)
	{
		wtk_free(cfg->mic_channel);
	}
	if(cfg->sp_channel)
	{
		wtk_free(cfg->sp_channel);
	}
	if(cfg->change_mic)
	{
		wtk_free(cfg->change_mic);
	}
	if(cfg->change_ref){
		wtk_free(cfg->change_ref);
	}
	if(cfg->mic_theta){
		wtk_free(cfg->mic_theta);
	}
	if(cfg->eq_gain)
	{
		wtk_free(cfg->eq_gain);
	}
	wtk_rls_cfg_clean(&(cfg->echo_rls));
	wtk_rls3_cfg_clean(&(cfg->echo_rls3));
	wtk_covm_cfg_clean(&(cfg->covm));
	wtk_covm_cfg_clean(&(cfg->echo_covm));
	wtk_covm_cfg_clean(&(cfg->icovm));
    wtk_bf_cfg_clean(&cfg->bf);
    wtk_bf_cfg_clean(&cfg->ibf);
	wtk_qmmse_cfg_clean(&(cfg->qmmse));
	wtk_qmmse_cfg_clean(&(cfg->qmmse2));
	qtk_ahs_gain_controller_cfg_clean(&(cfg->gc));
	wtk_equalizer_cfg_clean(&(cfg->eq));
    wtk_free(cfg->chns);
#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_clean(&(cfg->onnx));
#endif
    qtk_nnrt_cfg_clean(&cfg->rt);
    qtk_nnrt_cfg_clean(&cfg->dereb_rt);
	wtk_maskssl2_cfg_clean(&(cfg->maskssl2));
	wtk_maskssl2_cfg_clean(&(cfg->ibf_ssl));

    return 0;
}

int wtk_cmask_aec2_cfg_update(wtk_cmask_aec2_cfg_t *cfg) {
	int ret;

	if(cfg->use_nlms)
	{
		cfg->echo_nlms.channel=1;
		cfg->echo_nlms.N=cfg->nspchannel;
		ret=wtk_nlms_cfg_update(&(cfg->echo_nlms));
		if(ret!=0){goto end;}
	}else if(cfg->use_rls)
	{
		cfg->echo_rls.channel=1;
		cfg->echo_rls.N=cfg->nspchannel;
		ret=wtk_rls_cfg_update(&(cfg->echo_rls));
		if(ret!=0){goto end;}
	}else if(cfg->use_rls3)
	{
		cfg->echo_rls3.channel=1;
		cfg->echo_rls3.N=cfg->nspchannel;
		ret=wtk_rls3_cfg_update(&(cfg->echo_rls3));
		if(ret!=0){goto end;}
	}
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse2));
	if(ret!=0){goto end;}
	ret=qtk_ahs_gain_controller_cfg_update(&(cfg->gc));
	if(ret!=0){goto end;}
	ret=wtk_equalizer_cfg_update(&(cfg->eq));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->covm));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->echo_covm));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->icovm));
	if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->ibf));
	if(ret!=0){goto end;}
	if(cfg->use_maskssl2)
	{
		ret=wtk_maskssl2_cfg_update(&(cfg->maskssl2));
		if(ret!=0){goto end;}
	}
	if(cfg->use_ibf_ssl){
		ret=wtk_maskssl2_cfg_update(&(cfg->ibf_ssl));
		if(ret!=0){goto end;}
	}
	if(cfg->channel<cfg->nmicchannel+cfg->nspchannel)
	{
		cfg->channel=cfg->nmicchannel+cfg->nspchannel;
	}
	if(cfg->nbfchannel==0){
		cfg->nbfchannel=cfg->nmicchannel;
	}
	if(cfg->nibfchannel==0){
		cfg->nibfchannel=cfg->nmicchannel;
	}
	cfg->bf.nmic = cfg->nbfchannel;
	cfg->ibf.nmic = cfg->nibfchannel;
	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;
	cfg->bf_clip_s=(cfg->bf_clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->bf_clip_e=(cfg->bf_clip_e*1.0*cfg->wins)/cfg->rate;
	cfg->de_clip_s = (cfg->de_clip_s * 1.0 * cfg->wins) / cfg->rate;
	cfg->de_clip_e = (cfg->de_clip_e * 1.0 * cfg->wins) / cfg->rate;
	cfg->pre_clip_s = (cfg->pre_clip_s * 1.0 * cfg->wins) / cfg->rate;
	cfg->pre_clip_e = (cfg->pre_clip_e * 1.0 * cfg->wins) / cfg->rate;

    cfg->chns = wtk_malloc((cfg->nmicchannel+cfg->nspchannel) * sizeof(int));
    memcpy(cfg->chns, cfg->mic_channel, sizeof(int) * cfg->nmicchannel);
    memcpy(cfg->chns + cfg->nmicchannel, cfg->sp_channel, sizeof(int) * cfg->nspchannel);
	cfg->ncchannel = cfg->ncmicchannel + cfg->ncrefchannel;
	wtk_cmask_aec2_cfg_get_mic_theta(cfg);

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
		ret = qtk_onnxruntime_cfg_update(&(cfg->onnx));
		if (ret != 0) {
			wtk_debug("update onnx failed\n");
			goto end;
		}
	}
#endif
#ifdef QTK_NNRT_NCNN
	if(cfg->use_ncnn){
		ret = qtk_nnrt_cfg_update(&cfg->rt);
		if (ret != 0) {
			wtk_debug("update nnrt failed\n");
			goto end;
		}
	}
	if(cfg->use_dereb){
		ret = qtk_nnrt_cfg_update(&cfg->dereb_rt);
		if (ret != 0) {
			wtk_debug("update nnrt failed\n");
			goto end;
		}
	}
#endif
	ret=0;
end:
	return ret;
}

int wtk_cmask_aec2_cfg_update2(wtk_cmask_aec2_cfg_t *cfg, wtk_source_loader_t *sl) {
	int ret;

	if(cfg->use_nlms)
	{
		cfg->echo_nlms.channel=1;
		cfg->echo_nlms.N=cfg->nspchannel;
		ret=wtk_nlms_cfg_update(&(cfg->echo_nlms));
		if(ret!=0){goto end;}
	}else if(cfg->use_rls)
	{
		cfg->echo_rls.channel=1;
		cfg->echo_rls.N=cfg->nspchannel;
		ret=wtk_rls_cfg_update(&(cfg->echo_rls));
		if(ret!=0){goto end;}
	}else if(cfg->use_rls3)
	{
		cfg->echo_rls3.channel=1;
		cfg->echo_rls3.N=cfg->nspchannel;
		ret=wtk_rls3_cfg_update(&(cfg->echo_rls3));
		if(ret!=0){goto end;}
	}
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse2));
	if(ret!=0){goto end;}
	ret=qtk_ahs_gain_controller_cfg_update(&(cfg->gc));
	if(ret!=0){goto end;}
	ret=wtk_equalizer_cfg_update(&(cfg->eq));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->covm));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->echo_covm));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->icovm));
	if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->ibf));
	if(ret!=0){goto end;}
	if(cfg->use_maskssl2)
	{
		ret=wtk_maskssl2_cfg_update2(&(cfg->maskssl2),sl);
		if(ret!=0){goto end;}
	}
	if(cfg->use_ibf_ssl){
		ret=wtk_maskssl2_cfg_update2(&(cfg->ibf_ssl),sl);
		if(ret!=0){goto end;}
	}

	if(cfg->channel<cfg->nmicchannel+cfg->nspchannel)
	{
		cfg->channel=cfg->nmicchannel+cfg->nspchannel;
	}
	if(cfg->nbfchannel==0){
		cfg->nbfchannel=cfg->nmicchannel;
	}
	if(cfg->nibfchannel==0){
		cfg->nibfchannel=cfg->nmicchannel;
	}
	cfg->bf.nmic = cfg->nbfchannel;
	cfg->ibf.nmic = cfg->nibfchannel;
	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;
	cfg->bf_clip_s=(cfg->bf_clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->bf_clip_e=(cfg->bf_clip_e*1.0*cfg->wins)/cfg->rate;
	cfg->de_clip_s = (cfg->de_clip_s * 1.0 * cfg->wins) / cfg->rate;
	cfg->de_clip_e = (cfg->de_clip_e * 1.0 * cfg->wins) / cfg->rate;
	cfg->pre_clip_s = (cfg->pre_clip_s * 1.0 * cfg->wins) / cfg->rate;
	cfg->pre_clip_e = (cfg->pre_clip_e * 1.0 * cfg->wins) / cfg->rate;

    cfg->chns = wtk_malloc((cfg->nmicchannel+cfg->nspchannel) * sizeof(int));
    memcpy(cfg->chns, cfg->mic_channel, sizeof(int) * cfg->nmicchannel);
    memcpy(cfg->chns + cfg->nmicchannel, cfg->sp_channel, sizeof(int) * cfg->nspchannel);
	cfg->ncchannel = cfg->ncmicchannel + cfg->ncrefchannel;
	wtk_cmask_aec2_cfg_get_mic_theta(cfg);

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
		ret = qtk_onnxruntime_cfg_update2(&(cfg->onnx), sl->hook);
		if (ret != 0) {
			wtk_debug("update onnx failed\n");
			goto end;
		}
	}
#endif
#ifdef QTK_NNRT_NCNN
	if(cfg->use_ncnn){
		ret = qtk_nnrt_cfg_update2(&cfg->rt, sl);
		if (ret != 0) {
			wtk_debug("update nnrt failed\n");
			goto end;
		}
	}
	if(cfg->use_dereb){
		ret = qtk_nnrt_cfg_update2(&cfg->dereb_rt, sl);
		if (ret != 0) {
			wtk_debug("update nnrt failed\n");
			goto end;
		}
	}
#endif
	ret=0;
end:
	return ret;
}

int wtk_cmask_aec2_cfg_update_local(wtk_cmask_aec2_cfg_t *cfg, wtk_local_cfg_t *m) {
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;
	wtk_array_t *a;
	int i;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sv,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,spenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,spenr_cnt,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,micenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,micenr_cnt,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,micenr_thresh2,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,micenr_cnt2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,micenr_thresh3,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,micenr_cnt3,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,micenr_scale,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_nlms,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_rls,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_rls3,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_eq,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,clip_e,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse2,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nbfchannel,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,use_cnon,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sym,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,num_frame,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,entropy_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,entropy_sp_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,entropy_in_cnt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,entropy_cnt,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,raw_alpha1,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,raw_alpha2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,raw_alpha3,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,raw_alpha4,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pow_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,alpha1,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,alpha2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,alpha3,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,alpha4,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,scale1,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,scale2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,scale3,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,scale4,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,init_low_freq,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,init_high_freq,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,de_clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,de_clip_e,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,de_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,de_alpha,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,eng_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,eng1_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,eng1_thresh2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,eng2_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,eng2_thresh2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,eng_freq,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,eng_cnt,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,sp_max_smooth_gain,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sp_min_smooth_gain,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,mic_max_smooth_gain,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,mic_min_smooth_gain,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_ccm,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_entropy,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bf_clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bf_clip_e,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,bfmu,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,bfmu2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,echo_bfmu,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,echo_bfmu2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_bf,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_echocovm,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,num_in,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,num_out,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_onnx,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ncnn,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,dereb_num_in,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,dereb_num_out,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,dereb_delay,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,dereb_taps,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,dereb_alpha,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_dereb,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,wpe_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,wpe_cnt,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_wpe,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,wpe_alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,wpe_power,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_change_mic,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,change_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,change_cnt,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,change_alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,change_alpha2,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_freq_preemph,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pre_pow_ratio,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pre_mul_ratio,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pre_clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pre_clip_e,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,change_delay,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_bs_out,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,entropy_min_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,entropy_ratio,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_change_mic2,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_echo_bf,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_maskssl2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ibf_ssl,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_entropy_scale,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ibf,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nibfchannel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,sp_ibf_delay,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,ibf_theta,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,ibf_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,init_ibf_thresh,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_bs_win,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_gc,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,gc_gain,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,gc_min_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,gc_cnt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,out_agc_level,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_mask_bf,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_in_eq,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_out_eq,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_scale_qmmse,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_scale_qmmse2,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,eq_fn,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,bf_theta,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ds,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,ds_w_alpha,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_freq_atten,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,gain_alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,gain_alpha2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,gain_beta,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_mask,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,mask_peak,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,t_mic_in_scale,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,t_sp_in_scale,v);

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
	a=wtk_local_cfg_find_array_s(lc,"change_mic");
	if(a)
	{
		cfg->change_mic=(int*)wtk_malloc(sizeof(int)*a->nslot);
		cfg->ncmicchannel=a->nslot;
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->change_mic[i]=wtk_str_atoi(v->data,v->len);
		}
	}
	a=wtk_local_cfg_find_array_s(lc,"change_ref");
	if(a)
	{
		cfg->change_ref=(int*)wtk_malloc(sizeof(int)*a->nslot);
		cfg->ncrefchannel=a->nslot;
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->change_ref[i]=wtk_str_atoi(v->data,v->len);
		}
	}

	lc=wtk_local_cfg_find_lc_s(m,"echo_rls");
	if(lc)
	{
		ret=wtk_rls_cfg_update_local(&(cfg->echo_rls),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"echo_rls3");
	if(lc)
	{
		ret=wtk_rls3_cfg_update_local(&(cfg->echo_rls3),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"echo_nlms");
	if(lc)
	{
		ret=wtk_nlms_cfg_update_local(&(cfg->echo_nlms),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
		cfg->qmmse.step=cfg->wins/2;
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"qmmse2");
	if(lc)
	{
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse2),lc);
		cfg->qmmse2.step=cfg->wins/2;
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"gc");
	if(lc)
	{
        ret=qtk_ahs_gain_controller_cfg_update_local(&(cfg->gc),lc);
		if(ret!=0){goto end;}
    }

	lc=wtk_local_cfg_find_lc_s(m,"eq");
	if(lc)
	{
		ret=wtk_equalizer_cfg_update_local(&(cfg->eq),lc);
		if(ret!=0){goto end;}
	}
	if(cfg->use_in_eq || cfg->use_out_eq){
		if(cfg->eq_fn){
			cfg->eq_gain = wtk_file_read_float(cfg->eq_fn, &(cfg->eq_len));
			if(cfg->eq_len!= cfg->wins/2+1){
				wtk_debug("eq_len is not equal to wins/2+1\n");
			}
		}else{
			wtk_debug("eq_fn is not set\n");
			ret = -1;
			goto end;
		}
	}
	lc=wtk_local_cfg_find_lc_s(m,"covm");
	if(lc)
	{
		ret=wtk_covm_cfg_update_local(&(cfg->covm),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"echo_covm");
	if(lc)
	{
		ret=wtk_covm_cfg_update_local(&(cfg->echo_covm),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"icovm");
	if(lc)
	{
		ret=wtk_covm_cfg_update_local(&(cfg->icovm),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"bf");
	if(lc)
	{
		ret=wtk_bf_cfg_update_local(&(cfg->bf),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"ibf");
	if(lc)
	{
		ret=wtk_bf_cfg_update_local(&(cfg->ibf),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"maskssl2");
	if(lc)
	{
		ret=wtk_maskssl2_cfg_update_local(&(cfg->maskssl2),lc);
		cfg->maskssl2.wins=cfg->wins;
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"ibf_ssl");
	if(lc)
	{
		ret=wtk_maskssl2_cfg_update_local(&(cfg->ibf_ssl),lc);
		cfg->ibf_ssl.wins=cfg->wins;
		if(ret!=0){goto end;}
	}

#ifdef ONNX_DEC
    lc = wtk_local_cfg_find_lc_s(m, "onnx");
    if (lc) {
        ret = qtk_onnxruntime_cfg_update_local(&(cfg->onnx), lc);
        if (ret != 0) {
            wtk_debug("update local onnx failed\n");
            goto end;
        }
    }
#endif
#ifdef QTK_NNRT_NCNN
    lc = wtk_local_cfg_find_lc_s(m, "rt");
    if (lc) {
        qtk_nnrt_cfg_update_local(&cfg->rt, lc);
    }
    lc = wtk_local_cfg_find_lc_s(m, "dereb_rt");
    if (lc) {
        qtk_nnrt_cfg_update_local(&cfg->dereb_rt, lc);
    }
#endif
    ret = 0;
end:
    return ret;
}

wtk_cmask_aec2_cfg_t* wtk_cmask_aec2_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_cmask_aec2_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_cmask_aec2_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_cmask_aec2_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_cmask_aec2_cfg_delete(wtk_cmask_aec2_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_cmask_aec2_cfg_t* wtk_cmask_aec2_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_cmask_aec2_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_cmask_aec2_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_cmask_aec2_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_cmask_aec2_cfg_delete_bin(wtk_cmask_aec2_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
