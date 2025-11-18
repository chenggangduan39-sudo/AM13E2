#include "wtk/bfio/aec/cmask_aec/wtk_cmask_aec2.h"
#include "qtk/math/qtk_vector.h"
#define ASSIGN_SHAPE(shape, B, C, H, W)                                        \
    (shape)[0] = B;                                                            \
    (shape)[1] = C;                                                            \
    (shape)[2] = H;                                                            \
    (shape)[3] = W;
#define TENSOR_NELEM(shape) ((shape)[0] * (shape)[1] * (shape)[2] * (shape)[3])

typedef struct
{
    char in[50];
    char out[50];
}in_out_name;

void wtk_cmask_aec2_on_ssl2(wtk_cmask_aec2_t *cmask_aec2, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te);
void wtk_cmask_aec2_on_ibf_ssl(wtk_cmask_aec2_t *cmask_aec2, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te);

void wtk_cmask_aec2_ncnn_new(wtk_cmask_aec2_t *cmask_aec2,
                            wtk_cmask_aec2_cfg_t *cfg) {
    int i, j, k;
    int pos = 0;
    in_out_name in_out[12] = {
        {"in2", "out4"},
        {"in3", "out5"},
        {"in4", "out6"},
        {"in5", "out7"},
        {"in6", "out8"},
        {"in7", "out9"},
        {"in8", "out10"},
        {"in9", "out11"},
        {"in10", "out12"},
        {"in11", "out13"},
        {"in12", "out14"},
        {"in13", "out15"}
    };
    cmask_aec2->rt = qtk_nnrt_new(&cfg->rt);
    cmask_aec2->heap = wtk_heap_new(4096);

    cmask_aec2->num_in = qtk_nnrt_get_num_in(cmask_aec2->rt);
    cmask_aec2->num_out = qtk_nnrt_get_num_out(cmask_aec2->rt);
    cmask_aec2->num_cache = cmask_aec2->num_in - cfg->num_in;
    // wtk_debug("num_in: %d, num_out: %d, num_cache: %d\n", cmask_aec2->num_in, cmask_aec2->num_out, cmask_aec2->num_cache);

    cmask_aec2->cache_value = wtk_calloc(sizeof(void *), cmask_aec2->num_cache);
    cmask_aec2->in_index = (int *)wtk_malloc(sizeof(int) * cmask_aec2->num_in);
    cmask_aec2->out_index = (int *)wtk_malloc(sizeof(int) * cmask_aec2->num_out);
    cmask_aec2->in_name =
        (char **)wtk_malloc(sizeof(char *) * cmask_aec2->num_in);
    cmask_aec2->out_name =
        (char **)wtk_malloc(sizeof(char *) * cmask_aec2->num_out);

    for (i = 0; i < cmask_aec2->num_in; ++i) {
        cmask_aec2->in_name[i] = qtk_nnrt_net_get_input_name(cmask_aec2->rt, i);
        cmask_aec2->in_index[i] = -1;
        // printf("in %d %s\n", i, cmask_aec2->in_name[i]);
    }
    for (i = 0; i < cmask_aec2->num_out; ++i) {
        cmask_aec2->out_name[i] = qtk_nnrt_net_get_output_name(cmask_aec2->rt, i);
        cmask_aec2->out_index[i] = -1;
        // printf("out %d %s\n", i, cmask_aec2->out_name[i]);
    }

    for (i = 0; i < cmask_aec2->num_in; ++i) {
        for (j = 0; j < 12; ++j) {
            if(strcmp(cmask_aec2->in_name[i],in_out[j].in)==0){
                for(k=0;k<cmask_aec2->num_out;++k){
                    if(strcmp(cmask_aec2->out_name[k],in_out[j].out)==0){
                        // wtk_debug("%d %d %d %s %s\n", i, k, pos, in_out[j].in, in_out[j].out);
                        cmask_aec2->in_index[i] = pos;
                        cmask_aec2->out_index[k] = pos;
                        pos++;
                        break;
                    }
                }
                break;
            }
        }
    }

    ASSIGN_SHAPE(cmask_aec2->shape[0], 1, 2, cfg->num_frame, 257);
    ASSIGN_SHAPE(cmask_aec2->shape[1], 1, 2, cfg->num_frame, 257);

    ASSIGN_SHAPE(cmask_aec2->shape[2], 1, 2, 2, 257);
    ASSIGN_SHAPE(cmask_aec2->shape[3], 1, 2, 2, 257);
    ASSIGN_SHAPE(cmask_aec2->shape[4], 1, 16, 4, 5);
    ASSIGN_SHAPE(cmask_aec2->shape[5], 1, 16, 4, 33);
    ASSIGN_SHAPE(cmask_aec2->shape[6], 1, 16, 4, 33);
    ASSIGN_SHAPE(cmask_aec2->shape[7], 1, 16, 2, 17);
    ASSIGN_SHAPE(cmask_aec2->shape[8], 1, 16, 4, 17);
    ASSIGN_SHAPE(cmask_aec2->shape[9], 1, 16, 10, 17);
    ASSIGN_SHAPE(cmask_aec2->shape[10], 1, 1, 272, 0);
    ASSIGN_SHAPE(cmask_aec2->shape[11], 1, 16, 10, 17);
    ASSIGN_SHAPE(cmask_aec2->shape[12], 1, 16, 4, 17);
    ASSIGN_SHAPE(cmask_aec2->shape[13], 1, 16, 2, 17);
}

void wtk_cmask_aec2_ncnn_delete(wtk_cmask_aec2_t *cmask_aec2) {
    int i;
    for (i = 0; i < cmask_aec2->num_cache; i++) {
        if (cmask_aec2->cache_value[i]) {
            qtk_nnrt_value_release(cmask_aec2->rt, cmask_aec2->cache_value[i]);
        }
    }
    // for (i = 0; i < cmask_aec2->num_in; ++i) {
    //     wtk_free(cmask_aec2->in_name[i]);
    // }
    // for (i = 0; i < cmask_aec2->num_out; ++i) {
    //     wtk_free(cmask_aec2->out_name[i]);
    // }
    wtk_free(cmask_aec2->in_name);
    wtk_free(cmask_aec2->out_name);

    wtk_free(cmask_aec2->cache_value);
    wtk_free(cmask_aec2->in_index);
    wtk_free(cmask_aec2->out_index);
    qtk_nnrt_delete(cmask_aec2->rt);
    wtk_heap_delete(cmask_aec2->heap);
}

void wtk_cmask_aec2_dereb_new(wtk_cmask_aec2_t *cmask_aec2,
                            wtk_cmask_aec2_cfg_t *cfg) {
    int i, j, k;
    int pos = 0;
    in_out_name in_out[1] = {
        {"in1", "out1"},
    };
    cmask_aec2->dereb_rt = qtk_nnrt_new(&cfg->dereb_rt);
    cmask_aec2->dereb_heap = wtk_heap_new(4096);

    cmask_aec2->dereb_num_in = qtk_nnrt_get_num_in(cmask_aec2->dereb_rt);
    cmask_aec2->dereb_num_out = qtk_nnrt_get_num_out(cmask_aec2->dereb_rt);
    cmask_aec2->dereb_num_cache = cmask_aec2->dereb_num_in - cfg->dereb_num_in;
    // wtk_debug("dereb_num_in: %d, dereb_num_out: %d, dereb_num_cache: %d\n", cmask_aec2->dereb_num_in, cmask_aec2->dereb_num_out, cmask_aec2->dereb_num_cache);

    cmask_aec2->dereb_cache_value = wtk_calloc(sizeof(void *), cmask_aec2->dereb_num_cache);
    cmask_aec2->dereb_in_index = (int *)wtk_malloc(sizeof(int) * cmask_aec2->dereb_num_in);
    cmask_aec2->dereb_out_index = (int *)wtk_malloc(sizeof(int) * cmask_aec2->dereb_num_out);
    cmask_aec2->dereb_in_name =
        (char **)wtk_malloc(sizeof(char *) * cmask_aec2->dereb_num_in);
    cmask_aec2->dereb_out_name =
        (char **)wtk_malloc(sizeof(char *) * cmask_aec2->dereb_num_out);

    for (i = 0; i < cmask_aec2->dereb_num_in; ++i) {
        cmask_aec2->dereb_in_name[i] = qtk_nnrt_net_get_input_name(cmask_aec2->dereb_rt, i);
        cmask_aec2->dereb_in_index[i] = -1;
        // printf("in %d %s\n", i, cmask_aec2->in_name[i]);
    }
    for (i = 0; i < cmask_aec2->dereb_num_out; ++i) {
        cmask_aec2->dereb_out_name[i] = qtk_nnrt_net_get_output_name(cmask_aec2->dereb_rt, i);
        cmask_aec2->dereb_out_index[i] = -1;
        // printf("out %d %s\n", i, cmask_aec2->out_name[i]);
    }

    for (i = 0; i < cmask_aec2->dereb_num_in; ++i) {
        for (j = 0; j < 1; ++j) {
            if(strcmp(cmask_aec2->dereb_in_name[i],in_out[j].in)==0){
                for(k=0;k<cmask_aec2->dereb_num_out;++k){
                    if(strcmp(cmask_aec2->dereb_out_name[k],in_out[j].out)==0){
                        // wtk_debug("%d %d %d %s %s\n", i, k, pos, in_out[j].in, in_out[j].out);
                        cmask_aec2->dereb_in_index[i] = pos;
                        cmask_aec2->dereb_out_index[k] = pos;
                        pos++;
                        break;
                    }
                }
                break;
            }
        }
    }
}

void wtk_cmask_aec2_dereb_delete(wtk_cmask_aec2_t *cmask_aec2) {
    int i;
    for (i = 0; i < cmask_aec2->dereb_num_cache; i++) {
        if (cmask_aec2->dereb_cache_value[i]) {
            qtk_nnrt_value_release(cmask_aec2->dereb_rt, cmask_aec2->dereb_cache_value[i]);
        }
    }
    // for (i = 0; i < cmask_aec2->num_in; ++i) {
    //     wtk_free(cmask_aec2->in_name[i]);
    // }
    // for (i = 0; i < cmask_aec2->num_out; ++i) {
    //     wtk_free(cmask_aec2->out_name[i]);
    // }
    wtk_free(cmask_aec2->dereb_in_name);
    wtk_free(cmask_aec2->dereb_out_name);

    wtk_free(cmask_aec2->dereb_cache_value);
    wtk_free(cmask_aec2->dereb_in_index);
    wtk_free(cmask_aec2->dereb_out_index);
    qtk_nnrt_delete(cmask_aec2->dereb_rt);
    wtk_heap_delete(cmask_aec2->dereb_heap);
}

wtk_cmask_aec2_t *wtk_cmask_aec2_new(wtk_cmask_aec2_cfg_t *cfg) {
    wtk_cmask_aec2_t *cmask_aec2;
    int i;
    int fsize;

    cmask_aec2 = (wtk_cmask_aec2_t *)wtk_malloc(sizeof(wtk_cmask_aec2_t));
    cmask_aec2->cfg = cfg;
    cmask_aec2->ths = NULL;
    cmask_aec2->notify = NULL;
	cmask_aec2->ssl_ths=NULL;
    cmask_aec2->notify_ssl=NULL;
    cmask_aec2->mic = wtk_strbufs_new(cmask_aec2->cfg->nmicchannel);
    cmask_aec2->sp = wtk_strbufs_new(cmask_aec2->cfg->nspchannel);
    cmask_aec2->cmic=NULL;
    cmask_aec2->change_eng=NULL;
    cmask_aec2->change_cnt=NULL;
    cmask_aec2->change_pre=NULL;
    cmask_aec2->change_error=NULL;
    cmask_aec2->change_eng2=NULL;
    if(cfg->use_change_mic || cfg->use_change_mic2){
        cmask_aec2->cmic = wtk_strbufs_new(cmask_aec2->cfg->ncchannel);
        cmask_aec2->change_eng = (float *)wtk_malloc(sizeof(float) * cmask_aec2->cfg->ncchannel);
        cmask_aec2->change_cnt = (int *)wtk_malloc(sizeof(int) * cmask_aec2->cfg->ncchannel);
        cmask_aec2->change_pre = (float *)wtk_malloc(sizeof(float) * cmask_aec2->cfg->ncchannel);
        cmask_aec2->change_error = (float *)wtk_malloc(sizeof(float) * cfg->ncmicchannel);
        cmask_aec2->change_eng2 = (float *)wtk_malloc(sizeof(float) * cmask_aec2->cfg->ncmicchannel);
    }

    cmask_aec2->nbin = cfg->wins / 2 + 1;
    cmask_aec2->analysis_window = wtk_malloc(sizeof(float) * cfg->wins);  /// 2);
    cmask_aec2->synthesis_window = wtk_malloc(sizeof(float) * cfg->wins); /// 2);
    cmask_aec2->analysis_mem =
        wtk_float_new_p2(cfg->nmicchannel, cmask_aec2->nbin - 1);
    cmask_aec2->analysis_mem_sp =
        wtk_float_new_p2(cfg->nspchannel, cmask_aec2->nbin - 1);
    cmask_aec2->synthesis_mem =
        wtk_malloc(sizeof(float) * (cmask_aec2->nbin - 1));
    cmask_aec2->rfft = wtk_drft_new2(cfg->wins);
    cmask_aec2->rfft_in = (float *)wtk_malloc(sizeof(float) * (cfg->wins));

    cmask_aec2->fft = wtk_complex_new_p2(cfg->nmicchannel, cmask_aec2->nbin);
    cmask_aec2->fft_sp = wtk_complex_new_p2(max(1, cfg->nspchannel), cmask_aec2->nbin);
    cmask_aec2->f_fft = wtk_malloc(sizeof(wtk_complex_t) * cmask_aec2->nbin);

    cmask_aec2->ibf_fft=NULL;
    cmask_aec2->ibf_mic=NULL;
    if(cfg->use_ibf){
        cmask_aec2->ibf_fft=wtk_complex_new_p2(cfg->nmicchannel,cmask_aec2->nbin);
        cmask_aec2->ibf_mic=(int*)wtk_malloc(sizeof(int)*cfg->nmicchannel);
    }
    cmask_aec2->ibf=NULL;
    cmask_aec2->icovm=NULL;
    cmask_aec2->ibf_ssl=NULL;
    cmask_aec2->mic_diff=NULL;
    cmask_aec2->ibf_idx=NULL;
    if(cfg->use_ibf_ssl){
        cmask_aec2->ibf=wtk_bf_new(&(cfg->ibf),cfg->wins);
        cmask_aec2->icovm=wtk_covm_new(&(cfg->icovm),cmask_aec2->nbin,cfg->nibfchannel);
        cmask_aec2->mic_diff=(float *)wtk_malloc(sizeof(float)*cfg->nmicchannel);
        cmask_aec2->ibf_idx=(int*)wtk_malloc(sizeof(int)*cfg->nmicchannel);
        if(cfg->bf_theta==-1){
            cmask_aec2->ibf_ssl=wtk_maskssl2_new(&(cfg->ibf_ssl));
            wtk_maskssl2_set_notify(cmask_aec2->ibf_ssl, cmask_aec2, (wtk_maskssl2_notify_f)wtk_cmask_aec2_on_ibf_ssl);
        }
    }

    cmask_aec2->ovec=NULL;
    if(cmask_aec2->cfg->use_ds){
        cmask_aec2->ovec=wtk_complex_new_p2(cmask_aec2->nbin, cfg->nmicchannel);
    }

    cmask_aec2->erls = NULL;
    cmask_aec2->erls3 = NULL;
    cmask_aec2->enlms = NULL;
    if (cfg->use_nlms) {
        cmask_aec2->enlms = wtk_malloc(sizeof(wtk_nlms_t) * (cmask_aec2->nbin));
        for (i = 0; i < cmask_aec2->nbin; ++i) {
            wtk_nlms_init(cmask_aec2->enlms + i, &(cfg->echo_nlms));
        }
    } else if (cfg->use_rls) {
        cmask_aec2->erls = wtk_malloc(sizeof(wtk_rls_t) * (cmask_aec2->nbin));
        for (i = 0; i < cmask_aec2->nbin; ++i) {
            wtk_rls_init(cmask_aec2->erls + i, &(cfg->echo_rls));
        }
    } else if (cfg->use_rls3) {
        cmask_aec2->erls3 = wtk_malloc(sizeof(wtk_rls3_t));
        wtk_rls3_init(cmask_aec2->erls3, &(cfg->echo_rls3), cmask_aec2->nbin);
    }
    cmask_aec2->covm=NULL;
    cmask_aec2->echo_covm=NULL;
    cmask_aec2->bf=NULL;
    if(cfg->use_bf){
        cmask_aec2->covm=wtk_covm_new(&(cfg->covm),cmask_aec2->nbin,cfg->nbfchannel);
        cmask_aec2->echo_covm=wtk_covm_new(&(cfg->echo_covm),cmask_aec2->nbin,cfg->nbfchannel);
        cmask_aec2->bf=wtk_bf_new(&(cfg->bf),cfg->wins);
    }
    cmask_aec2->fftx =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cmask_aec2->nbin);
    cmask_aec2->ffty =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cmask_aec2->nbin);

	cmask_aec2->qmmse=NULL;
	if(cfg->use_qmmse)
	{
		cmask_aec2->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}
    cmask_aec2->qmmse2=NULL;
    if(cfg->use_qmmse2)
	{
		cmask_aec2->qmmse2=wtk_qmmse_new(&(cfg->qmmse2));
	}

    cmask_aec2->gc = NULL;
	if(cfg->use_gc){
		cmask_aec2->gc=qtk_gain_controller_new(&(cfg->gc));
		qtk_gain_controller_set_mode(cmask_aec2->gc,0);
		cmask_aec2->gc->kalman.Z_k = cfg->gc_gain;
	}

    cmask_aec2->eq = NULL;
    if (cfg->use_eq) {
        cmask_aec2->eq = wtk_equalizer_new(&(cfg->eq));
    }

    cmask_aec2->out = wtk_malloc(sizeof(float) * (cmask_aec2->nbin - 1));

#ifdef ONNX_DEC
    cmask_aec2->onnx = NULL;
    cmask_aec2->cache = NULL;
    if(cfg->use_onnx){
        cmask_aec2->onnx = qtk_onnxruntime_new(&(cfg->onnx));
        cmask_aec2->cache = wtk_calloc(sizeof(OrtValue *), cmask_aec2->onnx->num_in - cfg->onnx.outer_in_num);
        if (cmask_aec2->onnx->num_in - cfg->onnx.outer_in_num != cmask_aec2->onnx->num_out - cfg->onnx.outer_out_num) {
            wtk_debug("err inner_item\n");
            exit(0);
        }
    }
#endif

    cmask_aec2->errr = (float *)wtk_malloc(sizeof(float) * cmask_aec2->nbin * 2 *
                                          cfg->num_frame);
    cmask_aec2->ee = (float *)wtk_malloc(sizeof(float) * cmask_aec2->nbin * 2 *
                                        cfg->num_frame);

    cmask_aec2->c_onnx_len=cmask_aec2->nbin*cfg->num_frame;
    cmask_aec2->c_onnx_out = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * cmask_aec2->c_onnx_len);
    cmask_aec2->c_onnx_err = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * cmask_aec2->c_onnx_len);
    cmask_aec2->c_onnx_raw = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * cmask_aec2->c_onnx_len);
    cmask_aec2->c_onnx_echo = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * cmask_aec2->c_onnx_len);
    cmask_aec2->leak = (float *)wtk_malloc(sizeof(float) * cmask_aec2->nbin);
    cmask_aec2->gf=NULL;
    cmask_aec2->c_onnx_gf=NULL;
    if(cfg->use_qmmse2)
	{
        cmask_aec2->gf=(float *)wtk_malloc(sizeof(float)*cmask_aec2->nbin);
        cmask_aec2->c_onnx_gf=(wtk_complex_t *)wtk_malloc(
            sizeof(wtk_complex_t) * cmask_aec2->c_onnx_len);
	}

    cmask_aec2->x_phase=NULL;
    if(cfg->use_ccm){
        cmask_aec2->x_phase=(float *)wtk_malloc(sizeof(float)*cmask_aec2->nbin*cfg->num_frame);
    }

    fsize = cfg->wins / 2;
    cmask_aec2->audio_frame = wtk_malloc(sizeof(short) * (cfg->nmicchannel + cfg->nspchannel) * fsize);
    cmask_aec2->pos = 0;
    cmask_aec2->power_k = wtk_malloc(sizeof(float) * cmask_aec2->nbin);

    if(cfg->use_ncnn){
        wtk_cmask_aec2_ncnn_new(cmask_aec2, cfg);
    }

    cmask_aec2->dereb_x_mag=NULL;
    cmask_aec2->dereb_x_phase=NULL;
    cmask_aec2->Y_tilde=NULL;
    cmask_aec2->numerator=NULL;
    cmask_aec2->denominator=NULL;
    cmask_aec2->K=NULL;
    cmask_aec2->inv_R_WPE=NULL;
    cmask_aec2->G_WPE=NULL;
    cmask_aec2->dereb_tmp=NULL;
    if(cfg->use_dereb){
        wtk_cmask_aec2_dereb_new(cmask_aec2, cfg);
        cmask_aec2->dereb_x_mag = (float *)wtk_malloc(sizeof(float)*cmask_aec2->nbin);
        cmask_aec2->dereb_x_phase = (float *)wtk_malloc(sizeof(float)*cmask_aec2->nbin);
        cmask_aec2->Y_tilde = wtk_strbuf_new(1024, 1);
        cmask_aec2->numerator = wtk_float_new_p2(cmask_aec2->nbin, cfg->dereb_taps);
        cmask_aec2->denominator = (float *)wtk_malloc(sizeof(float)*cmask_aec2->nbin);
        cmask_aec2->K = wtk_float_new_p2(cmask_aec2->nbin, cfg->dereb_taps);
        cmask_aec2->inv_R_WPE = wtk_float_new_p3(cmask_aec2->nbin, cfg->dereb_taps, cfg->dereb_taps);
        cmask_aec2->G_WPE = wtk_float_new_p2(cmask_aec2->nbin, cfg->dereb_taps);
        cmask_aec2->dereb_tmp = (float *)wtk_malloc(sizeof(float)*cmask_aec2->nbin*cfg->dereb_taps*cfg->dereb_taps*2);
    }

    cmask_aec2->entropy_E=(float *)wtk_malloc(sizeof(float)*cmask_aec2->nbin);
    cmask_aec2->entropy_Eb=(float *)wtk_malloc(sizeof(float)*cfg->wins);
	cmask_aec2->maskssl2=NULL;
    cmask_aec2->mask=NULL;
    cmask_aec2->raw_fft=NULL;
    if(cfg->use_maskssl2)
	{
		cmask_aec2->maskssl2=wtk_maskssl2_new(&(cfg->maskssl2));
        wtk_maskssl2_set_notify(cmask_aec2->maskssl2, cmask_aec2, (wtk_maskssl2_notify_f)wtk_cmask_aec2_on_ssl2);
	}
    if(cfg->use_maskssl2 || cfg->use_ibf_ssl || cfg->use_gc || cfg->use_mask_bf || cfg->use_qmmse || cfg->use_qmmse2){
        cmask_aec2->mask=(float *)wtk_malloc(sizeof(float)*cmask_aec2->nbin);
        cmask_aec2->raw_fft = wtk_complex_new_p3(cfg->num_frame, cfg->nmicchannel, cmask_aec2->nbin);
    }

	cmask_aec2->bs_win=NULL;
	if(cfg->use_bs_win)
	{
		cmask_aec2->bs_win=wtk_math_create_hanning_window2(cfg->wins/2);
	}

    cmask_aec2->hist_energy=NULL;
    cmask_aec2->gains=NULL;
    cmask_aec2->energy_buf=NULL;
    cmask_aec2->prev_gains=NULL;
    cmask_aec2->aux_hist=NULL;
    if(cfg->use_freq_atten){
        cmask_aec2->hist_energy=(float *)wtk_malloc(sizeof(float)*cmask_aec2->nbin);
        cmask_aec2->gains=(float *)wtk_malloc(sizeof(float)*cmask_aec2->nbin);
        cmask_aec2->energy_buf=(float *)wtk_malloc(sizeof(float)*cmask_aec2->nbin);
        cmask_aec2->prev_gains=(float *)wtk_malloc(sizeof(float)*cmask_aec2->nbin);
        cmask_aec2->aux_hist=(float *)wtk_malloc(sizeof(float)*cmask_aec2->nbin);
    }

    wtk_cmask_aec2_reset(cmask_aec2);

    return cmask_aec2;
}
void wtk_cmask_aec2_delete(wtk_cmask_aec2_t *cmask_aec2) {
    int i;

    wtk_strbufs_delete(cmask_aec2->mic, cmask_aec2->cfg->nmicchannel);
    wtk_strbufs_delete(cmask_aec2->sp, cmask_aec2->cfg->nspchannel);
    if(cmask_aec2->cmic){
        wtk_strbufs_delete(cmask_aec2->cmic, cmask_aec2->cfg->ncchannel);
        wtk_free(cmask_aec2->change_eng);
        wtk_free(cmask_aec2->change_cnt);
        wtk_free(cmask_aec2->change_pre);
        wtk_free(cmask_aec2->change_error);
        wtk_free(cmask_aec2->change_eng2);
    }
    wtk_free(cmask_aec2->audio_frame);

    wtk_free(cmask_aec2->analysis_window);
    wtk_free(cmask_aec2->synthesis_window);
    wtk_float_delete_p2(cmask_aec2->analysis_mem, cmask_aec2->cfg->nmicchannel);
    wtk_float_delete_p2(cmask_aec2->analysis_mem_sp, cmask_aec2->cfg->nspchannel);
    wtk_free(cmask_aec2->synthesis_mem);
    wtk_free(cmask_aec2->rfft_in);
    wtk_drft_delete2(cmask_aec2->rfft);
    wtk_complex_delete_p2(cmask_aec2->fft, cmask_aec2->cfg->nmicchannel);
    wtk_complex_delete_p2(cmask_aec2->fft_sp, max(1, cmask_aec2->cfg->nspchannel));
    wtk_free(cmask_aec2->f_fft);
    if(cmask_aec2->ibf_fft){
        wtk_complex_delete_p2(cmask_aec2->ibf_fft, cmask_aec2->cfg->nmicchannel);
    }
    if(cmask_aec2->ibf){
        wtk_bf_delete(cmask_aec2->ibf);
    }
    if(cmask_aec2->icovm){
        wtk_covm_delete(cmask_aec2->icovm);
    }
    if(cmask_aec2->ibf_ssl){
        wtk_maskssl2_delete(cmask_aec2->ibf_ssl);
    }
    if(cmask_aec2->ibf_mic){
        wtk_free(cmask_aec2->ibf_mic);
    }
    if(cmask_aec2->mic_diff)
    {
        wtk_free(cmask_aec2->mic_diff);
    }
    if(cmask_aec2->ibf_idx)
    {
        wtk_free(cmask_aec2->ibf_idx);
    }

    if(cmask_aec2->ovec){
        wtk_complex_delete_p2(cmask_aec2->ovec, cmask_aec2->nbin);
    }

    if (cmask_aec2->erls) {
        for (i = 0; i < cmask_aec2->nbin; ++i) {
            wtk_rls_clean(cmask_aec2->erls + i);
        }
        wtk_free(cmask_aec2->erls);
    }
    if (cmask_aec2->erls3) {
        wtk_rls3_clean(cmask_aec2->erls3);
        wtk_free(cmask_aec2->erls3);
    }
    if (cmask_aec2->enlms) {
        for (i = 0; i < cmask_aec2->nbin; ++i) {
            wtk_nlms_clean(cmask_aec2->enlms + i);
        }
        wtk_free(cmask_aec2->enlms);
    }
    if(cmask_aec2->covm)
    {
        wtk_covm_delete(cmask_aec2->covm);
    }
    if(cmask_aec2->echo_covm)
    {
        wtk_covm_delete(cmask_aec2->echo_covm);
    }
    if(cmask_aec2->bf)
    {
        wtk_bf_delete(cmask_aec2->bf);
    }

	if(cmask_aec2->qmmse)
	{
		wtk_qmmse_delete(cmask_aec2->qmmse);
	}
    if(cmask_aec2->qmmse2)
	{
		wtk_qmmse_delete(cmask_aec2->qmmse2);
        wtk_free(cmask_aec2->gf);
        wtk_free(cmask_aec2->c_onnx_gf);
	}
	if(cmask_aec2->gc){
		qtk_gain_controller_delete(cmask_aec2->gc);
	}

    if (cmask_aec2->eq) {
        wtk_equalizer_delete(cmask_aec2->eq);
    }
	if(cmask_aec2->maskssl2)
    {
        wtk_maskssl2_delete(cmask_aec2->maskssl2);
    }
    if(cmask_aec2->mask){
        wtk_free(cmask_aec2->mask);
    }
    if(cmask_aec2->raw_fft){
        wtk_complex_delete_p3(cmask_aec2->raw_fft, cmask_aec2->cfg->num_frame, cmask_aec2->cfg->nmicchannel);
    }

    wtk_free(cmask_aec2->fftx);
    wtk_free(cmask_aec2->ffty);

    wtk_free(cmask_aec2->out);
#ifdef ONNX_DEC
    if(cmask_aec2->cfg->use_onnx){
        {
            int n = cmask_aec2->onnx->num_in - cmask_aec2->onnx->cfg->outer_in_num;
            if (cmask_aec2->cache[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    cmask_aec2->onnx->api->ReleaseValue(cmask_aec2->cache[i]);
                }
            }
        }
        if (cmask_aec2->onnx) {
            qtk_onnxruntime_delete(cmask_aec2->onnx);
        }
        wtk_free(cmask_aec2->cache);
    }
#endif
    wtk_free(cmask_aec2->errr);
    wtk_free(cmask_aec2->ee);
    wtk_free(cmask_aec2->c_onnx_out);
    wtk_free(cmask_aec2->c_onnx_err);
    wtk_free(cmask_aec2->c_onnx_raw);
    wtk_free(cmask_aec2->c_onnx_echo);
    wtk_free(cmask_aec2->leak);
    if(cmask_aec2->x_phase)
    {
        wtk_free(cmask_aec2->x_phase);
    }
    if(cmask_aec2->cfg->use_ncnn){
        wtk_cmask_aec2_ncnn_delete(cmask_aec2);
    }
    if(cmask_aec2->cfg->use_dereb){
        wtk_cmask_aec2_dereb_delete(cmask_aec2);
        wtk_free(cmask_aec2->dereb_x_mag);
        wtk_free(cmask_aec2->dereb_x_phase);
        wtk_strbuf_delete(cmask_aec2->Y_tilde);
        wtk_float_delete_p2(cmask_aec2->numerator, cmask_aec2->nbin);
        wtk_free(cmask_aec2->denominator);
        wtk_float_delete_p2(cmask_aec2->K, cmask_aec2->nbin);
        wtk_float_delete_p3(cmask_aec2->inv_R_WPE, cmask_aec2->nbin, cmask_aec2->cfg->dereb_taps);
        wtk_float_delete_p2(cmask_aec2->G_WPE, cmask_aec2->nbin);
        wtk_free(cmask_aec2->dereb_tmp);
    }
    wtk_free(cmask_aec2->power_k);
    wtk_free(cmask_aec2->entropy_E);
    wtk_free(cmask_aec2->entropy_Eb);

	if(cmask_aec2->bs_win)
	{
		wtk_free(cmask_aec2->bs_win);
	}
    if(cmask_aec2->hist_energy)
    {
        wtk_free(cmask_aec2->hist_energy);
    }
    if(cmask_aec2->gains)
    {
        wtk_free(cmask_aec2->gains);
    }
    if(cmask_aec2->energy_buf)
    {
        wtk_free(cmask_aec2->energy_buf);
    }
    if(cmask_aec2->prev_gains)
    {
        wtk_free(cmask_aec2->prev_gains);
    }
    if(cmask_aec2->aux_hist)
    {
        wtk_free(cmask_aec2->aux_hist);
    }

    wtk_free(cmask_aec2);
}

void wtk_cmask_aec2_start(wtk_cmask_aec2_t *cmask_aec2) {
    if(cmask_aec2->bf){
        wtk_bf_update_ovec(cmask_aec2->bf,90,0);
        wtk_bf_init_w(cmask_aec2->bf);
    }
    if(cmask_aec2->ibf){
        wtk_bf_update_ovec(cmask_aec2->ibf,90,0);
        wtk_bf_init_w(cmask_aec2->ibf);
    }
    if(cmask_aec2->ovec){
        if(cmask_aec2->cfg->bf_theta==-1){
            wtk_debug("error bf_theta==-1");
            exit(1);
        }
        wtk_bf_update_ovec4(cmask_aec2->cfg->bf_theta, 0, cmask_aec2->cfg->nmicchannel, \
            cmask_aec2->nbin, cmask_aec2->cfg->rate, cmask_aec2->cfg->sv, cmask_aec2->cfg->bf.mic_pos, cmask_aec2->ovec);
        if(cmask_aec2->cfg->ds_w_alpha!=0){
            int i, k;
            int nbin=cmask_aec2->nbin;
            int nmicchannel=cmask_aec2->cfg->nmicchannel;
            float alpha = cmask_aec2->cfg->ds_w_alpha * 1.0/nmicchannel;

            for(k=0;k<nbin;++k){
                for(i=0;i<nmicchannel;++i){
                    cmask_aec2->ovec[k][i].a = cmask_aec2->ovec[k][i].a * (1.0-cmask_aec2->cfg->ds_w_alpha) + alpha;
                }
            }
        }
    }
    if(cmask_aec2->cfg->use_ibf_ssl && cmask_aec2->cfg->bf_theta!=-1){
        int i, j;
        float tmp;
        int nmicchannel = cmask_aec2->cfg->nmicchannel;
        float *mic_diff = cmask_aec2->mic_diff;
        int *ibf_idx = cmask_aec2->ibf_idx;
        for(i=0;i<nmicchannel;++i){
            mic_diff[i] = fabs(cmask_aec2->cfg->bf_theta-cmask_aec2->cfg->mic_theta[i]);
            if(mic_diff[i] > 180){
                mic_diff[i] = 360 - mic_diff[i];
            }
        }
        for (i = 0; i < nmicchannel; i++) {
            ibf_idx[i] = i;
        }
        for (i = 0; i < nmicchannel - 1; ++i) {
            for (j = 0; j < nmicchannel - i - 1; ++j) {
                if (mic_diff[ibf_idx[j]] > mic_diff[ibf_idx[j + 1]]) {
                    tmp = ibf_idx[j];
                    ibf_idx[j] = ibf_idx[j + 1];
                    ibf_idx[j + 1] = tmp;
                }
            }
        }
        for(i=0;i<nmicchannel;++i){
            cmask_aec2->ibf_mic[i] = ibf_idx[i];
        }
    }
}
void wtk_cmask_aec2_reset(wtk_cmask_aec2_t *cmask_aec2) {
    int wins = cmask_aec2->cfg->wins;
    int i, nbin = cmask_aec2->nbin;

    wtk_strbufs_reset(cmask_aec2->mic, cmask_aec2->cfg->nmicchannel);
    wtk_strbufs_reset(cmask_aec2->sp, cmask_aec2->cfg->nspchannel);
    if(cmask_aec2->cmic){
        wtk_strbufs_reset(cmask_aec2->cmic, cmask_aec2->cfg->ncchannel);
        for(i=0;i<cmask_aec2->cfg->ncchannel;++i){
            cmask_aec2->change_eng[i] = -1.0;
        }
        memset(cmask_aec2->change_cnt, 0, sizeof(int)*cmask_aec2->cfg->ncchannel);
        memset(cmask_aec2->change_pre, 0, sizeof(float)*cmask_aec2->cfg->ncchannel);
        memset(cmask_aec2->change_error, 0, sizeof(float)*cmask_aec2->cfg->ncmicchannel);
        for(i=0;i<cmask_aec2->cfg->ncmicchannel;++i){
            cmask_aec2->change_eng2[i] = -1.0;
        }
    }

    for (i = 0; i < wins; ++i) {
        cmask_aec2->analysis_window[i] = sin((0.5 + i) * PI / (wins));
    }
    wtk_drft_init_synthesis_window(cmask_aec2->synthesis_window,
                                   cmask_aec2->analysis_window, wins);

    wtk_float_zero_p2(cmask_aec2->analysis_mem, cmask_aec2->cfg->nmicchannel,
                      (cmask_aec2->nbin - 1));
    wtk_float_zero_p2(cmask_aec2->analysis_mem_sp, cmask_aec2->cfg->nspchannel,
                      (cmask_aec2->nbin - 1));
    memset(cmask_aec2->synthesis_mem, 0, sizeof(float) * (cmask_aec2->nbin - 1));

    wtk_complex_zero_p2(cmask_aec2->fft, cmask_aec2->cfg->nmicchannel, cmask_aec2->nbin);
    wtk_complex_zero_p2(cmask_aec2->fft_sp, max(1, cmask_aec2->cfg->nspchannel), cmask_aec2->nbin);
    memset(cmask_aec2->f_fft, 0, sizeof(wtk_complex_t) * cmask_aec2->nbin);
    if(cmask_aec2->ibf_fft){
        wtk_complex_zero_p2(cmask_aec2->ibf_fft, cmask_aec2->cfg->nmicchannel, cmask_aec2->nbin);
    }
    if(cmask_aec2->ibf){
        wtk_bf_reset(cmask_aec2->ibf);
    }
    if(cmask_aec2->icovm){
        wtk_covm_reset(cmask_aec2->icovm);
    }
    if(cmask_aec2->ibf_ssl){
        wtk_maskssl2_reset(cmask_aec2->ibf_ssl);
    }
    if(cmask_aec2->ibf_mic){
        for(i=0;i<cmask_aec2->cfg->nmicchannel;++i){
            cmask_aec2->ibf_mic[i] = i;
        }
    }
    if(cmask_aec2->mic_diff)
    {
        memset(cmask_aec2->mic_diff, 0, sizeof(float)*cmask_aec2->cfg->nmicchannel);
    }
    if(cmask_aec2->ibf_idx)
    {
        memset(cmask_aec2->ibf_idx, 0, sizeof(int)*cmask_aec2->cfg->nmicchannel);
    }

    if(cmask_aec2->ovec){
        wtk_complex_zero_p2(cmask_aec2->ovec, cmask_aec2->nbin, cmask_aec2->cfg->nmicchannel);
    }

    if (cmask_aec2->erls) {
        for (i = 0; i < nbin; ++i) {
            wtk_rls_reset(cmask_aec2->erls + i);
        }
    }
    if (cmask_aec2->erls3) {
        wtk_rls3_reset(cmask_aec2->erls3, cmask_aec2->nbin);
    }
    if (cmask_aec2->enlms) {
        for (i = 0; i < nbin; ++i) {
            wtk_nlms_reset(cmask_aec2->enlms + i);
        }
    }
    if(cmask_aec2->covm)
    {
        wtk_covm_reset(cmask_aec2->covm);
    }
    if(cmask_aec2->echo_covm) 
    {
        wtk_covm_reset(cmask_aec2->echo_covm);
    }
    if(cmask_aec2->bf)
    {
        wtk_bf_reset(cmask_aec2->bf);
    }

	if(cmask_aec2->qmmse)
	{
		wtk_qmmse_reset(cmask_aec2->qmmse);
	}
    if(cmask_aec2->qmmse2)
	{
		wtk_qmmse_reset(cmask_aec2->qmmse2);
        memset(cmask_aec2->gf, 0, sizeof(float)*cmask_aec2->nbin);
        memset(cmask_aec2->c_onnx_gf, 0, sizeof(float)*cmask_aec2->c_onnx_len);
	}
    if(cmask_aec2->gc){
        qtk_gain_controller_reset(cmask_aec2->gc);
    }
	if(cmask_aec2->maskssl2)
    {
        wtk_maskssl2_reset(cmask_aec2->maskssl2);
    }
    if(cmask_aec2->mask){
        memset(cmask_aec2->mask, 0, sizeof(float)*nbin);
    }
    if(cmask_aec2->raw_fft){
        wtk_complex_zero_p3(cmask_aec2->raw_fft, cmask_aec2->cfg->num_frame, cmask_aec2->cfg->nmicchannel, cmask_aec2->nbin);
    }

    memset(cmask_aec2->ffty, 0, sizeof(wtk_complex_t) * (cmask_aec2->nbin));
    memset(cmask_aec2->fftx, 0, sizeof(wtk_complex_t) * (cmask_aec2->nbin));
    memset(cmask_aec2->entropy_E, 0, sizeof(float) * nbin);
    memset(cmask_aec2->entropy_Eb, 0, sizeof(float) * wins);

    cmask_aec2->sp_silcnt = 0;
    cmask_aec2->sp_sil = 1;

    cmask_aec2->mic_silcnt = 0;
    cmask_aec2->mic_sil = 1;
    cmask_aec2->mic_silcnt2 = 0;
    cmask_aec2->mic_sil2 = 1;
    cmask_aec2->wpe_silcnt = 0;
    cmask_aec2->wpe_sil = 1;

    cmask_aec2->entropy_in_cnt = 0;
    cmask_aec2->entropy_silcnt = 0;
    cmask_aec2->entropy_sil = 1;
    cmask_aec2->entropy_sp_in_cnt = 0;
    cmask_aec2->entropy_sp_silcnt = 0;
    cmask_aec2->entropy_sp_sil = 1;
    for(i=0;i<nbin;++i){
        cmask_aec2->power_k[i] = (1.0-powf(i*1.0/nbin, cmask_aec2->cfg->pow_scale));
    }

    cmask_aec2->bs_scale = 1.0;
    cmask_aec2->bs_last_scale = 1.0;
	cmask_aec2->bs_real_scale=1.0;
    cmask_aec2->bs_max_cnt = 0;
#ifdef ONNX_DEC
    if(cmask_aec2->cfg->use_onnx){
        qtk_onnxruntime_reset(cmask_aec2->onnx);
        {
            int n = cmask_aec2->onnx->num_in - cmask_aec2->onnx->cfg->outer_in_num;
            if (cmask_aec2->cache[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    cmask_aec2->onnx->api->ReleaseValue(cmask_aec2->cache[i]);
                }
                memset(cmask_aec2->cache, 0, sizeof(OrtValue *) * n);
            }
        }
    }
#endif
    memset(cmask_aec2->errr, 0,
           sizeof(float) * cmask_aec2->nbin * 2 * cmask_aec2->cfg->num_frame);
    memset(cmask_aec2->ee, 0,
           sizeof(float) * cmask_aec2->nbin * 2 * cmask_aec2->cfg->num_frame);
    memset(cmask_aec2->c_onnx_out, 0,
           sizeof(wtk_complex_t) * cmask_aec2->c_onnx_len);
    memset(cmask_aec2->c_onnx_err, 0,
           sizeof(wtk_complex_t) * cmask_aec2->c_onnx_len);
    memset(cmask_aec2->c_onnx_raw, 0,
           sizeof(wtk_complex_t) * cmask_aec2->c_onnx_len);
    memset(cmask_aec2->c_onnx_echo, 0,
            sizeof(wtk_complex_t) * cmask_aec2->c_onnx_len);
    memset(cmask_aec2->ee, 0, sizeof(float) * cmask_aec2->nbin);

    if(cmask_aec2->x_phase)
    {
        memset(cmask_aec2->x_phase, 0, sizeof(float)*cmask_aec2->nbin*cmask_aec2->cfg->num_frame);
    }
    if(cmask_aec2->cfg->use_dereb){
        memset(cmask_aec2->dereb_x_mag, 0, sizeof(float)*cmask_aec2->nbin);
        memset(cmask_aec2->dereb_x_phase, 0, sizeof(float)*cmask_aec2->nbin);
        wtk_strbuf_reset(cmask_aec2->Y_tilde);
        wtk_float_zero_p2(cmask_aec2->numerator, cmask_aec2->nbin, cmask_aec2->cfg->dereb_taps);
        memset(cmask_aec2->denominator, 0, sizeof(float) * cmask_aec2->nbin);
        wtk_float_zero_p2(cmask_aec2->K, cmask_aec2->nbin, cmask_aec2->cfg->dereb_taps);
        for(i=0;i<nbin;++i){
            for(int j=0;j<cmask_aec2->cfg->dereb_taps;++j){
                for(int k=0;k<cmask_aec2->cfg->dereb_taps;++k){
                    if(j==k){
                        cmask_aec2->inv_R_WPE[i][j][k] = 1.0;
                    }else{
                        cmask_aec2->inv_R_WPE[i][j][k] = 0.0;
                    }
                }
            }
        }
        wtk_float_zero_p2(cmask_aec2->G_WPE, cmask_aec2->nbin, cmask_aec2->cfg->dereb_taps);
        memset(cmask_aec2->dereb_tmp, 0, sizeof(float) * cmask_aec2->nbin * cmask_aec2->cfg->dereb_taps * cmask_aec2->cfg->dereb_taps*2);
    }
    if(cmask_aec2->hist_energy)
    {
        memset(cmask_aec2->hist_energy, 0, sizeof(float)*cmask_aec2->nbin);
    }
    if(cmask_aec2->gains){
        for(i=0;i<nbin;++i){
            cmask_aec2->gains[i] = 1.0;
        }
    }
    if(cmask_aec2->energy_buf)
    {
        memset(cmask_aec2->energy_buf, 0, sizeof(float)*cmask_aec2->nbin);
    }
    if(cmask_aec2->prev_gains)
    {
        for(i=0;i<nbin;++i){
            cmask_aec2->prev_gains[i] = 1.0;
        }
    }
    if(cmask_aec2->aux_hist)
    {
        memset(cmask_aec2->aux_hist, 0, sizeof(float)*cmask_aec2->nbin);
    }
    cmask_aec2->feed_frame = 0;
    cmask_aec2->frame_pos = 0;
    cmask_aec2->nframe = 0;
    cmask_aec2->eng_cnt = 0;
    cmask_aec2->sp_eng_cnt = 0;
    cmask_aec2->eng_times = 0;
    cmask_aec2->sp_eng_times = 0;
    cmask_aec2->dereb_nframe = 0;
    cmask_aec2->change_delay = 0;
    cmask_aec2->change_idx = min(cmask_aec2->cfg->mic_channel[0], cmask_aec2->cfg->nmicchannel-1);
    cmask_aec2->need_ibf = 0;
    cmask_aec2->theta = 90;
    cmask_aec2->gc_cnt = 0;
}
void wtk_cmask_aec2_set_notify(wtk_cmask_aec2_t *cmask_aec2, void *ths,
                              wtk_cmask_aec2_notify_f notify) {
    cmask_aec2->notify = notify;
    cmask_aec2->ths = ths;
}
void wtk_cmask_aec2_set_ssl_notify(wtk_cmask_aec2_t *cmask_aec2,void *ths,wtk_cmask_aec2_notify_ssl_f notify)
{
	cmask_aec2->notify_ssl=notify;
	cmask_aec2->ssl_ths=ths;
}
void wtk_cmask_aec2_on_ibf_ssl(wtk_cmask_aec2_t *cmask_aec2, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te)
{
    int debug=0;
    int state=0;
    if(cmask_aec2->cfg->use_ibf){
        if(nbest>0){
            float *mic_diff=cmask_aec2->mic_diff;
            int *ibf_idx=cmask_aec2->ibf_idx;
            int i, j;
            int tmp;
            int nmicchannel = cmask_aec2->cfg->nmicchannel;
            float ibf_theta = cmask_aec2->cfg->ibf_theta;
            float ibf_thresh = cmask_aec2->cfg->ibf_thresh;
            float init_ibf_thresh = cmask_aec2->cfg->init_ibf_thresh;

            // printf("%d\n", nbest_extp[0].theta);
            // printf("%f\n", nbest_extp[0].nspecsum);
            if(nbest_extp[0].nspecsum > ibf_thresh){
                cmask_aec2->theta=nbest_extp[0].theta;
            }
            if(debug){
                state=0;
            }
            for(i=0;i<nbest;++i){
                if(abs(nbest_extp[i].theta - (int)ibf_theta) < 30 && nbest_extp[i].nspecsum > init_ibf_thresh){
                    if(debug){
                        printf("%f\n", nbest_extp[i].nspecsum);
                        state=1;
                    }
                    // printf("%d\n", nbest_extp[i].theta);
                    cmask_aec2->theta=ibf_theta;
                    break;
                }
            }
            if(debug){
                if(state==0){
                    printf("0\n");
                }
            }

            for(i=0;i<nmicchannel;++i){
                mic_diff[i] = fabs(cmask_aec2->theta-cmask_aec2->cfg->mic_theta[i]);
                if(mic_diff[i] > 180){
                    mic_diff[i] = 360 - mic_diff[i];
                }
            }
            // 初始化 b 数组，存储初始索引
            for (i = 0; i < nmicchannel; i++) {
                ibf_idx[i] = i;
            }
            // 使用冒泡排序对索引数组 b 按照 a 数组对应元素值进行排序
            for (i = 0; i < nmicchannel - 1; ++i) {
                for (j = 0; j < nmicchannel - i - 1; ++j) {
                    if (mic_diff[ibf_idx[j]] > mic_diff[ibf_idx[j + 1]]) {
                        tmp = ibf_idx[j];
                        ibf_idx[j] = ibf_idx[j + 1];
                        ibf_idx[j + 1] = tmp;
                    }
                }
            }

            // printf("%f ", cmask_aec2->theta);
            // for(i=0;i<nmicchannel;++i){
            //     printf("%f %f %d ", cmask_aec2->cfg->mic_theta[i], mic_diff[i], ibf_idx[i]);
            // }
            for(i=0;i<nmicchannel;++i){
                cmask_aec2->ibf_mic[i] = ibf_idx[i];
                // printf("%d ", cmask_aec2->ibf_mic[i]);
                // for(j=0;j<nmicchannel;++j){
                //     if(cmask_aec2->cfg->mic_channel[j] == ibf_idx[i]){
                //         cmask_aec2->ibf_mic[i] = cmask_aec2->cfg->mic_channel[j];
                //     }
                // }
            }
            // printf("\n");
        }else{
            if(debug){
                printf("0\n");
            }
        }
    }
}

void wtk_cmask_aec2_on_ssl2(wtk_cmask_aec2_t *cmask_aec2, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te)
{
    if(cmask_aec2->notify_ssl)
    {
        cmask_aec2->notify_ssl(cmask_aec2->ssl_ths, ts, te, nbest_extp, nbest);
    }
}

static float wtk_cmask_aec2_sp_energy(short *p, int n) {
    float f, f2;
    int i;

    f = 0;
    for (i = 0; i < n; ++i) {
        f += p[i];
    }
    f /= n;

    f2 = 0;
    for (i = 0; i < n; ++i) {
        f2 += (p[i] - f) * (p[i] - f);
    }
    f2 /= n;

    return f2;
}

static float wtk_cmask_aec2_fft_energy(wtk_complex_t *fftx, int nbin) {
    return qtk_vector_cpx_mag_squared_sum(fftx + 1, nbin - 2);
}

void wtk_cmask_aec2_ccm(wtk_cmask_aec2_t *cmask_aec2, float *x_mag_unfold, float *x_phase, float *m_mag, float *m_real, float *m_imag)
{
    int nbin = cmask_aec2->nbin;
    int i, n;
    float mag;
    float mag_mask;
    float phase_mask;
    float tmp, tmp1;
    float scale = 64.0;
    wtk_complex_t *c_onnx_out = cmask_aec2->c_onnx_out;
    int num_frame = cmask_aec2->cfg->num_frame;

    for(n=0;n<num_frame;++n){
        mag = 0;
        for(i=0;i<nbin;++i)
        {
            // mag = x_mag_unfold[i+n*nbin]*m_mag[i+n*nbin]+x_mag_unfold[i+nbin+n*nbin]*m_mag[i+nbin+n*nbin]+x_mag_unfold[i+nbin*2+n*nbin]*m_mag[i+nbin*2+n*nbin];
            // mag = x_mag_unfold[i+3*n*nbin]*m_mag[i+3*n*nbin]+x_mag_unfold[i+(3*n+1)*nbin]*m_mag[i+(3*n+1)*nbin]+x_mag_unfold[i+(3*n+2)*nbin]*m_mag[i+(3*n+2)*nbin];
            mag = x_mag_unfold[i+n*nbin]*m_mag[i+n*nbin]+x_mag_unfold[i+(num_frame+n)*nbin]*m_mag[i+(num_frame+n)*nbin]+x_mag_unfold[i+(num_frame*2+n)*nbin]*m_mag[i+(num_frame*2+n)*nbin];
            // mag = x_mag_unfold[i*3+3*n*nbin]*m_mag[i*3+3*n*nbin]+x_mag_unfold[i*3+3*n*nbin+1]*m_mag[i*3+3*n*nbin+1]+x_mag_unfold[i*3+3*n*nbin+2]*m_mag[i*3+3*n*nbin+2];
            mag_mask = sqrtf(m_real[i+n*nbin]*m_real[i+n*nbin] + m_imag[i+n*nbin]*m_imag[i+n*nbin]);
            phase_mask = atan2f(m_imag[i+n*nbin], m_real[i+n*nbin]);
            // real = mag * tanhf(mag_mask) * cosf(x_phase[i]+phase_mask);
            // imag = mag * tanhf(mag_mask) * sinf(x_phase[i]+phase_mask);
            // fftx[i].a = real*scale;
            // fftx[i].b = imag*scale;

            tmp = x_phase[i+n*nbin]+phase_mask;
            tmp1 = mag * tanhf(mag_mask)*scale;
            c_onnx_out[i+n*nbin].a = tmp1 * cosf(tmp);
            c_onnx_out[i+n*nbin].b = tmp1 * sinf(tmp);
        }
    }
}

void wtk_cmask_aec2_ccm2(wtk_cmask_aec2_t *cmask_aec2, float *x_mag_unfold_0, float *x_mag_unfold_1, float *x_mag_unfold_2, float *x_phase, float *m_mag_0, float *m_mag_1, float *m_mag_2, float *m_real, float *m_imag)
{
    int nbin = cmask_aec2->nbin;
    int i, n;
    float mag;
    float mag_mask;
    float phase_mask;
    float tmp, tmp1;
    float scale = 64.0;
    wtk_complex_t *c_onnx_out = cmask_aec2->c_onnx_out;
    int num_frame = cmask_aec2->cfg->num_frame;

    for(n=0;n<num_frame;++n){
        mag = 0;
        for(i=0;i<nbin;++i)
        {
            mag = x_mag_unfold_0[i+n*nbin]*m_mag_0[i+n*nbin]+x_mag_unfold_1[i+n*nbin]*m_mag_1[i+n*nbin]+x_mag_unfold_2[i+n*nbin]*m_mag_2[i+n*nbin];
            mag_mask = sqrtf(m_real[i+n*nbin]*m_real[i+n*nbin] + m_imag[i+n*nbin]*m_imag[i+n*nbin]);
            phase_mask = atan2f(m_imag[i+n*nbin], m_real[i+n*nbin]);
            // real = mag * tanhf(mag_mask) * cosf(x_phase[i]+phase_mask);
            // imag = mag * tanhf(mag_mask) * sinf(x_phase[i]+phase_mask);
            // fftx[i].a = real*scale;
            // fftx[i].b = imag*scale;

            tmp = x_phase[i+n*nbin]+phase_mask;
            tmp1 = mag * tanhf(mag_mask)*scale;
            c_onnx_out[i+n*nbin].a = tmp1 * cosf(tmp);
            c_onnx_out[i+n*nbin].b = tmp1 * sinf(tmp);
        }
    }
}

float wtk_cmask_aec2_entropy(wtk_cmask_aec2_t *cmask_aec2, wtk_complex_t *fftx)
{
    int rate = cmask_aec2->cfg->rate;
    int wins = cmask_aec2->cfg->wins;
    int i;
    int fx1 = (250*1.0*wins)/rate;
    int fx2 = (3500*1.0*wins)/rate;
    int km = floor(wins*1.0/8);
    float K = 0.5;
    float *E=cmask_aec2->entropy_E;
    float P1;
    float *Eb=cmask_aec2->entropy_Eb;
    float sum;
    float prob;
    float Hb;

    memset(E, 0, sizeof(float) * cmask_aec2->nbin);
    memset(Eb, 0, sizeof(float) * wins);
    qtk_vector_cpx_mag_squared(fftx + fx1, E + fx1, fx2 - fx1);
    sum = 1e-10;
    for(i=fx1;i<fx2;++i)
    {
        sum += E[i];
    }
    for(i=fx1;i<fx2;++i)
    {
        P1 = E[i]/sum;
        if(P1>=0.9){
            E[i] = 0;
        }
    }
    sum = 0;
    for(i=0;i<km;++i)
    {
        Eb[i] = K;
        Eb[i] += E[i*4]+E[i*4+1]+E[i*4+2]+E[i*4+3];
        sum += Eb[i];
    }
    Hb = 0;
    for(i=0;i<wins;++i)
    {
        prob = Eb[i]/sum;
        Hb += -prob*logf(prob+1e-10);
    }
    // printf("%f\n", Hb);

    return Hb;
}

void wtk_cmask_aec2_edra_feed_onnx(wtk_cmask_aec2_t *cmask_aec2, wtk_complex_t *fft,
                             wtk_complex_t *err, wtk_complex_t *fft_sp,
                             wtk_complex_t *ffty) {
#ifdef ONNX_DEC
    int i, j;
    int nbin = cmask_aec2->nbin;
    float *errr = cmask_aec2->errr;
    float *ee = cmask_aec2->ee;
    wtk_complex_t *c_onnx_out = cmask_aec2->c_onnx_out;
    wtk_complex_t *c_onnx_err = cmask_aec2->c_onnx_err;
    wtk_complex_t *c_onnx_raw = cmask_aec2->c_onnx_raw;
    wtk_complex_t *c_onnx_echo = cmask_aec2->c_onnx_echo;
    float scale = 64.0;
    float scale_1 = 1.0/64.0;
    float *out=NULL;
    float *x_mag_unfold=NULL;
    float *m_mag=NULL;
    float *m_real=NULL;
    float *m_imag=NULL;
    float *x_phase=cmask_aec2->x_phase;
    int num_frame = cmask_aec2->cfg->num_frame;
    int pos = cmask_aec2->frame_pos;

    cmask_aec2->feed_frame++;
    if(x_phase){
        for (i = 0; i < nbin; ++i) {
            x_phase[pos + i] = atan2f(err[i].b, err[i].a);
        }
    }

    for (i = 0; i < nbin; ++i) {
        c_onnx_err[pos + i].a = ffty[i].a;
        c_onnx_err[pos + i].b = ffty[i].b;
        c_onnx_raw[pos + i].a = fft[i].a;
        c_onnx_raw[pos + i].b = fft[i].b;
        c_onnx_echo[pos + i].a = fft_sp[i].a;
        c_onnx_echo[pos + i].b = fft_sp[i].b;
    }
    qtk_vector_scale((float *)err, (float *)err, nbin * 2, scale_1);
    qtk_vector_scale((float *)fft_sp, (float *)fft_sp, nbin * 2, scale_1);
    for (i = 0; i < nbin; ++i) {
        errr[pos + i] = err[i].a;
        errr[pos + i + nbin * num_frame] = err[i].b;
        ee[pos + i] = fft_sp[i].a;
        ee[pos + i + nbin * num_frame] = fft_sp[i].b;
    }
    cmask_aec2->frame_pos += nbin;
    if (cmask_aec2->feed_frame >= num_frame) {
        const OrtApi *api = cmask_aec2->onnx->api;
        OrtMemoryInfo *meminfo = cmask_aec2->onnx->meminfo;
        qtk_onnxruntime_t *onnx = cmask_aec2->onnx;
        OrtStatus *status;
        int num_in = onnx->num_in;
        int outer_in_num = onnx->cfg->outer_in_num;
        int outer_out_num = onnx->cfg->outer_out_num;
        qtk_onnx_item_t *item;
        void *onnx_out;
        // int64_t size = 0, *out_shape;

        for (i = 0; i < outer_in_num; ++i) {
            item = onnx->in_items + i;
            if (i == 0) {
                memcpy(item->val, errr, item->bytes * item->in_dim);
            } else if (i == 1) {
                memcpy(item->val, ee, item->bytes * item->in_dim);
            }
        }

        // printf("num_in:\n");
        for (i = 0; i < num_in; ++i) {
            item = onnx->in_items + i;
            status = api->CreateTensorWithDataAsOrtValue(
                meminfo, item->val, item->bytes * item->in_dim, item->shape,
                item->shape_len, item->type, onnx->in + i);
            // printf("%d\n", i);
            // for(j=0;j<item->shape_len;++j){
            // 	printf("%d %ld\n", j, item->shape[j]);
            // }
            // printf("%ld\n", item->bytes*item->in_dim/sizeof(float));
        }

        status = api->Run(onnx->session, NULL,
                          cast(const char *const *, onnx->name_in),
                          cast(const OrtValue *const *, onnx->in), onnx->num_in,
                          cast(const char *const *, onnx->name_out),
                          onnx->num_out, onnx->out);

        // wtk_debug("run success\n");
        // out_shape = qtk_onnxruntime_get_outshape(onnx, 0, &size);
        if(cmask_aec2->cfg->use_ccm){
            x_mag_unfold = qtk_onnxruntime_getout(onnx, 0);
            m_mag = qtk_onnxruntime_getout(onnx, 1);
            m_real = qtk_onnxruntime_getout(onnx, 2);
            m_imag = qtk_onnxruntime_getout(onnx, 3);
            wtk_cmask_aec2_ccm(cmask_aec2, x_mag_unfold, x_phase, m_mag, m_real, m_imag);
        }else{
            out = qtk_onnxruntime_getout(onnx, 0);
            for (i = 0; i < nbin * num_frame; ++i) {
                c_onnx_out[i].a = out[i] * scale;
                c_onnx_out[i].b = out[i + nbin * num_frame] * scale;
            }
        }

        for (i = outer_in_num, j = outer_out_num; i < num_in; ++i, ++j) {
            item = onnx->in_items + i;
            onnx_out = qtk_onnxruntime_getout(onnx, j);
            memcpy(item->val, onnx_out, item->bytes * item->in_dim);
        }
        qtk_onnxruntime_reset(onnx);
        // printf("num_out:\n");
        // for(i=0;i<num_out;++i){
        // 	qtk_onnx_item_t *item = onnx->num_out + i;
        // 	printf("%d\n", i);
        // 	for(j=0;j<item->shape_len;++j){
        // 		printf("%d %ld\n", j, item->shape[j]);
        // 	}
        // }
        (void)status;
        // exit(0);
        cmask_aec2->feed_frame = 0;
        cmask_aec2->frame_pos = 0;
    }
#endif
}

void wtk_cmask_aec2_edra_feed_ncnn(wtk_cmask_aec2_t *cmask_aec2, wtk_complex_t *fft,
                             wtk_complex_t *err, wtk_complex_t *fft_sp,
                             wtk_complex_t *ffty){
    int num_in = cmask_aec2->num_in;
    int num_in2 = cmask_aec2->cfg->num_in;
    int num_out = cmask_aec2->num_out;
    char **in_name = cmask_aec2->in_name;
    char **out_name = cmask_aec2->out_name;
    int *in_index = cmask_aec2->in_index;
    int *out_index = cmask_aec2->out_index;
    int i;
    int shape_len;
    int nbin = cmask_aec2->nbin;
    int num_frame = cmask_aec2->cfg->num_frame;
    int pos = cmask_aec2->frame_pos;
    float *errr = cmask_aec2->errr;
    float *ee = cmask_aec2->ee;
    float scale = 64.0;
    float scale_1 = 1.0/64.0;
    float *x_phase=cmask_aec2->x_phase;
    float *x_mag_unfold_0=NULL;
    float *x_mag_unfold_1=NULL;
    float *x_mag_unfold_2=NULL;
    float *m_mag_0=NULL;
    float *m_mag_1=NULL;
    float *m_mag_2=NULL;
    float *m_real=NULL;
    float *m_imag=NULL;
    wtk_complex_t *c_onnx_out = cmask_aec2->c_onnx_out;
    wtk_complex_t *c_onnx_err = cmask_aec2->c_onnx_err;
    wtk_complex_t *c_onnx_raw = cmask_aec2->c_onnx_raw;
    wtk_complex_t *c_onnx_echo = cmask_aec2->c_onnx_echo;

    cmask_aec2->feed_frame++;
    if(x_phase){
        for (i = 0; i < nbin; ++i) {
            x_phase[pos + i] = atan2f(err[i].b, err[i].a);
        }
    }
    for (i = 0; i < nbin; ++i) {
        c_onnx_err[pos + i].a = ffty[i].a;
        c_onnx_err[pos + i].b = ffty[i].b;
        c_onnx_raw[pos + i].a = fft[i].a;
        c_onnx_raw[pos + i].b = fft[i].b;
        c_onnx_echo[pos + i].a = fft_sp[i].a;
        c_onnx_echo[pos + i].b = fft_sp[i].b;
    }
    qtk_vector_scale((float *)err, (float *)err, nbin * 2, scale_1);
    qtk_vector_scale((float *)fft_sp, (float *)fft_sp, nbin * 2, scale_1);
    for (i = 0; i < nbin; ++i) {
        errr[pos + i] = err[i].a;
        errr[pos + i + nbin * num_frame] = err[i].b;
        ee[pos + i] = fft_sp[i].a;
        ee[pos + i + nbin * num_frame] = fft_sp[i].b;
    }
    cmask_aec2->frame_pos += nbin;

    // for (i = 0; i < nbin; ++i) {
    //     errr[i] = err[i].a;
    //     errr[i + nbin] = err[i].b;
    //     ee[i] = fft_sp[i].a;
    //     ee[i + nbin] = fft_sp[i].b;
    // }

    if (cmask_aec2->feed_frame >= num_frame) {
        i = 0;
        cmask_aec2->input_val[i] = qtk_nnrt_value_create_external(
            cmask_aec2->rt, QTK_NNRT_VALUE_ELEM_F32, cmask_aec2->shape[i], 4, errr);
        qtk_nnrt_feed_s(cmask_aec2->rt, cmask_aec2->input_val[i], in_name[i]);

        i = 1;
        cmask_aec2->input_val[i] = qtk_nnrt_value_create_external(
            cmask_aec2->rt, QTK_NNRT_VALUE_ELEM_F32, cmask_aec2->shape[i], 4, ee);
        qtk_nnrt_feed_s(cmask_aec2->rt, cmask_aec2->input_val[i], in_name[i]);

        for (i = num_in2; i < num_in; ++i) {
            if (cmask_aec2->nframe == 0) {
                int64_t nelem;
                float *input_data;
                shape_len = cmask_aec2->shape[i][3] == 0 ? 3:4;
                if(shape_len==3){
                    nelem = cmask_aec2->shape[i][0]*cmask_aec2->shape[i][1]*cmask_aec2->shape[i][2];
                }else{
                    nelem = TENSOR_NELEM(cmask_aec2->shape[i]);
                }
                input_data =
                    wtk_heap_zalloc(cmask_aec2->heap, sizeof(float) * nelem);
                cmask_aec2->input_val[i] = qtk_nnrt_value_create_external(
                    cmask_aec2->rt, QTK_NNRT_VALUE_ELEM_F32, cmask_aec2->shape[i], shape_len,
                    input_data);
                qtk_nnrt_feed_s(cmask_aec2->rt, cmask_aec2->input_val[i], in_name[i]);
            } else {
                cmask_aec2->input_val[i] = cmask_aec2->cache_value[in_index[i]];
                qtk_nnrt_feed_s(cmask_aec2->rt, cmask_aec2->input_val[i], in_name[i]);
            }
        }
    
        for (i = 0; i < num_out; ++i) {
            if (out_index[i] != -1) {
                qtk_nnrt_get_output_s(cmask_aec2->rt,
                                    cmask_aec2->cache_value + out_index[i],
                                    out_name[i]);
            }else{
                qtk_nnrt_get_output_s(cmask_aec2->rt, cmask_aec2->output_val+i, out_name[i]);
                if(cmask_aec2->cfg->use_ccm){
                    if(strcmp(out_name[i], "out0")==0){
                        x_mag_unfold_0 = qtk_nnrt_value_get_channel_data(cmask_aec2->rt, cmask_aec2->output_val[i], 0);
                        x_mag_unfold_1 = qtk_nnrt_value_get_channel_data(cmask_aec2->rt, cmask_aec2->output_val[i], 1);
                        x_mag_unfold_2 = qtk_nnrt_value_get_channel_data(cmask_aec2->rt, cmask_aec2->output_val[i], 2);
                    }else if(strcmp(out_name[i], "out1")==0){
                        m_mag_0 = qtk_nnrt_value_get_channel_data(cmask_aec2->rt, cmask_aec2->output_val[i], 0);
                        m_mag_1 = qtk_nnrt_value_get_channel_data(cmask_aec2->rt, cmask_aec2->output_val[i], 1);
                        m_mag_2 = qtk_nnrt_value_get_channel_data(cmask_aec2->rt, cmask_aec2->output_val[i], 2);
                    }else if(strcmp(out_name[i], "out2")==0){
                        m_real = qtk_nnrt_value_get_data(cmask_aec2->rt, cmask_aec2->output_val[i]);
                    }else if(strcmp(out_name[i], "out3")==0){
                        m_imag = qtk_nnrt_value_get_data(cmask_aec2->rt, cmask_aec2->output_val[i]);
                    }
                }else{
                    if(strcmp(out_name[i], "out0")==0){
                        x_mag_unfold_0 = qtk_nnrt_value_get_data(cmask_aec2->rt, cmask_aec2->output_val[i]);
                    }
                }
            }
        }

        if(cmask_aec2->cfg->use_ccm){
            wtk_cmask_aec2_ccm2(cmask_aec2, x_mag_unfold_0, x_mag_unfold_1, x_mag_unfold_2, x_phase, m_mag_0, m_mag_1, m_mag_2, m_real, m_imag);
        }else{
            for (i = 0; i < nbin * num_frame; ++i) {
                c_onnx_out[i].a = x_mag_unfold_0[i] * scale;
                c_onnx_out[i].b = x_mag_unfold_0[i + nbin * num_frame] * scale;
            }
        }
        for (i = 0; i < num_in; i++) {
            qtk_nnrt_value_release(cmask_aec2->rt, cmask_aec2->input_val[i]);
        }
        for(i=0;i<num_out;++i){
            if(out_index[i]==-1){
                qtk_nnrt_value_release(cmask_aec2->rt, cmask_aec2->output_val[i]);
            }
        }
        qtk_nnrt_reset(cmask_aec2->rt);
        if(cmask_aec2->nframe==0){
            ++cmask_aec2->nframe;
        }
        cmask_aec2->feed_frame = 0;
        cmask_aec2->frame_pos = 0;
    }
}
void wtk_cmask_aec2_feed_edra(wtk_cmask_aec2_t *cmask_aec2, wtk_complex_t *fft,
                             wtk_complex_t *fft_sp) {
    int nspchannel = cmask_aec2->cfg->nspchannel;
    int k;
    int nbin = cmask_aec2->nbin;
    wtk_rls_t *erls = cmask_aec2->erls, *erlstmp;
    wtk_rls3_t *erls3 = cmask_aec2->erls3;
    wtk_nlms_t *enlms = cmask_aec2->enlms, *enlmstmp;
    wtk_complex_t *fftx = cmask_aec2->fftx;
    wtk_complex_t *ffttmp=cmask_aec2->ffttmp;
    wtk_complex_t *fftsp2=cmask_aec2->fftsp2;
    wtk_complex_t *ffty = cmask_aec2->ffty;
    int num_frame = cmask_aec2->cfg->num_frame;
    int pos = cmask_aec2->frame_pos;

    if(nspchannel==0){
        memcpy(fftx, fft, nbin * sizeof(wtk_complex_t));
        memset(ffty, 0, nbin * sizeof(wtk_complex_t));
    }else if (erls) {
        erlstmp = erls;
        for (k = 0; k < nbin; ++k, ++erlstmp) {
            ffttmp[0] = fft[k];
            fftsp2[0] = fft_sp[k];
            wtk_rls_feed3(erlstmp, ffttmp, fftsp2,
                          cmask_aec2->sp_sil == 0);
            if (cmask_aec2->sp_sil == 0) {
                ffty[k] = erlstmp->lsty[0];
                fftx[k] = erlstmp->out[0];
            } else {
                fftx[k] = ffttmp[0];
                ffty[k].a = ffty[k].b = 0;
            }
        }
    }else if (erls3) {
        wtk_rls3_feed3(erls3, fft, fft_sp, cmask_aec2->sp_sil==0, nbin);
        if (cmask_aec2->sp_sil == 0) {
            memcpy(ffty, erls3->lsty, nbin * sizeof(wtk_complex_t));
            memcpy(fftx, erls3->out, nbin * sizeof(wtk_complex_t));
        } else {
            memcpy(fftx, fft, nbin * sizeof(wtk_complex_t));
            memset(ffty, 0, nbin * sizeof(wtk_complex_t));
        }
    }else if (enlms) {
        enlmstmp = enlms;
        for (k = 0; k < nbin; ++k, ++enlmstmp) {
            ffttmp[0] = fft[k];
            fftsp2[0] = fft_sp[k];
            wtk_nlms_feed3(enlmstmp, ffttmp, fftsp2,
                           cmask_aec2->sp_sil == 0);
            if (cmask_aec2->sp_sil == 0) {
                ffty[k] = enlmstmp->lsty[0];
                fftx[k] = enlmstmp->out[0];
            } else {
                fftx[k] = ffttmp[0];
                ffty[k].a = ffty[k].b = 0;
            }
        }
    } else {
        for (k = 0; k < nbin; ++k) {
            fftx[k] = fft[k];
            ffty[k] = fft_sp[k];
        }
    }

	if(cmask_aec2->qmmse2)
	{
        float *gf=cmask_aec2->gf;
        // wtk_qmmse_t *qmmse2=cmask_aec2->qmmse2;
        wtk_complex_t *c_onnx_gf=cmask_aec2->c_onnx_gf;
        wtk_complex_t *fftytmp, sed, *fftxtmp;
        float ef,yf;
        float leak;
        memcpy(c_onnx_gf+pos, fftx, nbin * sizeof(wtk_complex_t));
		fftxtmp=c_onnx_gf+pos;
		fftytmp=ffty;
        if(cmask_aec2->cfg->use_scale_qmmse2){
            for(k=0;k<nbin;++k){
                fftxtmp[k].a *= 1.0/cmask_aec2->cfg->wins;
                fftxtmp[k].b *= 1.0/cmask_aec2->cfg->wins;
                fftytmp[k].a *= 1.0/cmask_aec2->cfg->wins;
                fftytmp[k].b *= 1.0/cmask_aec2->cfg->wins;
            }
        }
		for(k=0;k<nbin;++k,++fftxtmp,++fftytmp)
		{
			ef=fftxtmp->a*fftxtmp->a+fftxtmp->b*fftxtmp->b;
			yf=fftytmp->a*fftytmp->a+fftytmp->b*fftytmp->b;
			sed.a=fftytmp->a*fftxtmp->a+fftytmp->b*fftxtmp->b;
			sed.b=-fftytmp->a*fftxtmp->b+fftytmp->b*fftxtmp->a;
			leak=(sed.a*sed.a+sed.b*sed.b)/(max(ef,yf)*yf+1e-9);
			leak=sqrtf(leak);
			// fftytmp->a*=leak;
			// fftytmp->b*=leak;
			leak=(sed.a*sed.a+sed.b*sed.b)/(ef*yf+1e-9);
			gf[k]=leak*yf;
		}
		fftxtmp=c_onnx_gf+pos;
		fftytmp=ffty;
		// wtk_qmmse_flush_mask(qmmse2, fftxtmp, gf);
        if(cmask_aec2->cfg->use_scale_qmmse2){
            for(k=0;k<nbin;++k){
                fftxtmp[k].a *= 1.0*cmask_aec2->cfg->wins;
                fftxtmp[k].b *= 1.0*cmask_aec2->cfg->wins;
                fftytmp[k].a *= 1.0*cmask_aec2->cfg->wins;
                fftytmp[k].b *= 1.0*cmask_aec2->cfg->wins;
                fftxtmp[k].a = fftytmp[k].a;
                fftxtmp[k].b = fftytmp[k].b;
            }
        }
	}

    if(cmask_aec2->cfg->use_onnx){
        wtk_cmask_aec2_edra_feed_onnx(cmask_aec2, fft, fftx, fft_sp, ffty);
    }else if(cmask_aec2->cfg->use_ncnn){
        wtk_cmask_aec2_edra_feed_ncnn(cmask_aec2, fft, fftx, fft_sp, ffty);
    }else{
        cmask_aec2->feed_frame++;
        memcpy(cmask_aec2->c_onnx_out+pos, fftx, nbin * sizeof(wtk_complex_t));
        memcpy(cmask_aec2->c_onnx_err+pos, ffty, nbin * sizeof(wtk_complex_t));
        memcpy(cmask_aec2->c_onnx_raw+pos, fft, nbin * sizeof(wtk_complex_t));
        memcpy(cmask_aec2->c_onnx_echo+pos, fft_sp, nbin * sizeof(wtk_complex_t));
        cmask_aec2->frame_pos += nbin;
        if (cmask_aec2->feed_frame >= num_frame) {
            cmask_aec2->feed_frame = 0;
            cmask_aec2->frame_pos = 0;
        }
    }
}

void wtk_cmask_aec2_feed_cnon(wtk_cmask_aec2_t *cmask_aec2, wtk_complex_t *fft) {
    int nbin = cmask_aec2->nbin;
    float sym = cmask_aec2->cfg->sym;
    static float fx = 2.0f * PI / RAND_MAX;
    float f, f2;
    int i;

    for (i = 1; i < nbin - 1; ++i) {
        f = rand() * fx;
        f2 = 1.f;
        if (f2 > 0) {
            // f2=sqrtf(f2);
            fft[i].a += sym * cosf(f) * f2;
            fft[i].b += sym * sinf(f) * f2;
        }
    }
}

void wtk_cmask_aec2_control_bs(wtk_cmask_aec2_t *cmask_aec2, float *out, int len) {
	float *bs_win=cmask_aec2->bs_win;
    float out_max;
    int i;

    if (cmask_aec2->mic_sil == 0) {
        out_max = wtk_float_abs_max(out, len);
        if (out_max > cmask_aec2->cfg->max_bs_out) {
            cmask_aec2->bs_scale = cmask_aec2->cfg->max_bs_out / out_max;
            if (cmask_aec2->bs_scale < cmask_aec2->bs_last_scale) {
                cmask_aec2->bs_last_scale = cmask_aec2->bs_scale;
            } else {
                cmask_aec2->bs_scale = cmask_aec2->bs_last_scale;
            }
            cmask_aec2->bs_max_cnt = 5;
        }
		if(bs_win){
			for(i=0; i<len/2; ++i)
			{
				out[i]*=cmask_aec2->bs_scale * bs_win[i] + cmask_aec2->bs_real_scale * (1.0-bs_win[i]);
			}
			for(i=len/2; i<len; ++i){
				out[i]*=cmask_aec2->bs_scale;
			}
			cmask_aec2->bs_real_scale = cmask_aec2->bs_scale;
		}else{
			for(i=0; i<len; ++i){
				out[i]*=cmask_aec2->bs_scale;
			}
		}
        if (cmask_aec2->bs_max_cnt > 0) {
            --cmask_aec2->bs_max_cnt;
        }
        if (cmask_aec2->bs_max_cnt <= 0 && cmask_aec2->bs_scale < 1.0) {
            cmask_aec2->bs_scale *= 1.1f;
            cmask_aec2->bs_last_scale = cmask_aec2->bs_scale;
            if (cmask_aec2->bs_scale > 1.0) {
                cmask_aec2->bs_scale = 1.0;
                cmask_aec2->bs_last_scale = 1.0;
            }
        }
    } else {
        cmask_aec2->bs_scale = 1.0;
        cmask_aec2->bs_last_scale = 1.0;
        cmask_aec2->bs_max_cnt = 0;
    }
}

void wtk_cmask_aec2_feed_bf(wtk_cmask_aec2_t *cmask_aec2, wtk_complex_t *fft, wtk_complex_t *fft_sp)
{
	int k;
	wtk_bf_t *bf=cmask_aec2->bf;
	wtk_covm_t *covm;
	int b;
	wtk_complex_t fft2;
	wtk_complex_t ffts;
	wtk_complex_t ffty;
	int clip_s=cmask_aec2->cfg->clip_s;
	int clip_e=cmask_aec2->cfg->clip_e;
	int bf_clip_s=cmask_aec2->cfg->bf_clip_s;
	int bf_clip_e=cmask_aec2->cfg->bf_clip_e;

    for(k=clip_s+1; k<clip_e; ++k)
    {
        if(cmask_aec2->sp_sil==0)
        {
            if(k>=bf_clip_s && k<bf_clip_e){
                bf->cfg->mu=cmask_aec2->cfg->echo_bfmu;
            }else{
                bf->cfg->mu=cmask_aec2->cfg->echo_bfmu2;
            }
            if(cmask_aec2->cfg->use_echocovm){
                covm = cmask_aec2->echo_covm;
            }else{
                covm = cmask_aec2->covm;
            }
        }else
        {
            if(k>=bf_clip_s && k<bf_clip_e){
                bf->cfg->mu=cmask_aec2->cfg->bfmu;
            }else{
                bf->cfg->mu=cmask_aec2->cfg->bfmu2;
            }
            covm = cmask_aec2->covm;
        }
        ffts.a=fft[k].a;
        ffts.b=fft[k].b;

        if(cmask_aec2->cfg->use_echo_bf){
            ffty.a=fft_sp[k].a;
            ffty.b=fft_sp[k].b;
        }else{
            ffty.a=0;
            ffty.b=0;
        }
        fft2.a=ffts.a;
        fft2.b=ffts.b;

        b=0;
        b=wtk_covm_feed_fft3(covm, &ffty, k, 1);
        if(b==1)
        {
            wtk_bf_update_ncov(bf, covm->ncov, k);
        }
        if(covm->scov)
        {
            b=wtk_covm_feed_fft3(covm, &ffts, k, 0);
            if(b==1)
            {
                wtk_bf_update_scov(bf, covm->scov, k);
            }
        }
        if(b==1)
        {
            wtk_bf_update_w(bf, k);
        }
        wtk_bf_output_fft_k(bf, &fft2, fft+k, k);
	}
}

void wtk_cmask_aec2_feed_wpe(wtk_cmask_aec2_t *cmask_aec2, float *amp, float *mask, float *x_phase, wtk_complex_t *fftx)
{
    int taps=cmask_aec2->cfg->dereb_taps;
    int delay=cmask_aec2->cfg->dereb_delay;
    float alpha=cmask_aec2->cfg->dereb_alpha;
    float wpe_alpha=cmask_aec2->cfg->wpe_alpha;
    float wpe_alpha_1=1.0-wpe_alpha;

    wtk_strbuf_t *Y_tilde=cmask_aec2->Y_tilde;
    float **numerator=cmask_aec2->numerator;
    float *denominator=cmask_aec2->denominator;
    float **K=cmask_aec2->K;
    float ***inv_R_WPE=cmask_aec2->inv_R_WPE;
    float **G_WPE=cmask_aec2->G_WPE;
    float *tmp=cmask_aec2->dereb_tmp;
    float *Y;
    int i, j, k;
    int nbin = cmask_aec2->nbin;
    float *tmp1, *tmp2;
    float X_inter;
    float out;
    if(Y_tilde->pos==0){
        wtk_strbuf_push(Y_tilde, NULL, nbin*(delay+1+taps)*sizeof(float));
    }
    wtk_strbuf_push_float(Y_tilde, amp, nbin);
    Y = (float *)Y_tilde->data;

    for(i=0;i<nbin;++i){
        memset(numerator[i], 0, sizeof(float)*taps);
        for(j=0;j<taps;++j){
            for(k=0;k<taps;++k){
                numerator[i][j] += inv_R_WPE[i][j][k] * Y[i+k*nbin];
            }
            numerator[i][j] *= (1.0-alpha);
        }
        if(cmask_aec2->cfg->wpe_power==1.0){
            denominator[i] = alpha * mask[i];
        }else if(cmask_aec2->cfg->wpe_power==2.0){
            denominator[i] = alpha * mask[i] * mask[i];
        }else{
            denominator[i] = alpha * powf(mask[i], cmask_aec2->cfg->wpe_power);
        }
        for(j=0;j<taps;++j){
            denominator[i] += Y[i+j*nbin] * numerator[i][j];
        }
        for(j=0;j<taps;++j){
            K[i][j] = numerator[i][j] / (denominator[i] + 1e-3);
        }

        tmp1 = tmp;
        memset(tmp1, 0, sizeof(float)*taps);
        for(j=0;j<taps;++j){
            for(k=0;k<taps;++k){
                tmp1[j] += Y[i+k*nbin] * inv_R_WPE[i][j][k];
            }
        }
        tmp2 = tmp1 + taps;
        memset(tmp2, 0, sizeof(float)*taps*taps);
        for(j=0;j<taps;++j){
            for(k=0;k<taps;++k){
                tmp2[j+k*taps] = K[i][j] * tmp1[k];
            }
        }
        for(j=0;j<taps;++j){
            for(k=0;k<taps;++k){
                inv_R_WPE[i][j][k] -= tmp2[j+k*taps];
                inv_R_WPE[i][j][k] *= 1.0/alpha;
            }
        }
        X_inter = amp[i];
        for(j=0;j<taps;++j){
            X_inter -= G_WPE[i][j] * Y[i+j*nbin];
        }
        for(j=0;j<taps;++j){
            G_WPE[i][j] += K[i][j] * X_inter;
        }
        out = amp[i];
        for(j=0;j<taps;++j){
            out -= G_WPE[i][j] * Y[i+j*nbin];
        }
        fftx[i].a = fftx[i].a * wpe_alpha_1 + out * cosf(x_phase[i]) * wpe_alpha;
        fftx[i].b = fftx[i].b * wpe_alpha_1 + out * sinf(x_phase[i]) * wpe_alpha;
    }

    wtk_strbuf_pop(Y_tilde, NULL, nbin*sizeof(float));
}

void wtk_cmask_aec2_feed_dereb(wtk_cmask_aec2_t *cmask_aec2, wtk_complex_t *fftx)
{
    int num_in = cmask_aec2->dereb_num_in;
    int num_in2 = cmask_aec2->cfg->dereb_num_in;
    int num_out = cmask_aec2->dereb_num_out;
    qtk_nnrt_value_t input_val[2];
    qtk_nnrt_value_t output_val[2];
    int64_t shape[2][5];
    char **in_name = cmask_aec2->dereb_in_name;
    char **out_name = cmask_aec2->dereb_out_name;
    int *in_index = cmask_aec2->dereb_in_index;
    int *out_index = cmask_aec2->dereb_out_index;
    float *x_mag=cmask_aec2->dereb_x_mag;  
    float *x_phase=cmask_aec2->dereb_x_phase;
    int i;
    int shape_len;
    int nbin = cmask_aec2->nbin;
    float scale = 64.0;
    float scale_1 = 1.0/64.0;
    float *clean_mag=NULL;

    for(i=0;i<nbin;++i){
        x_mag[i] = sqrtf(fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b);
        x_phase[i] = atan2f(fftx[i].b, fftx[i].a);
    }
    for (i = 0; i < nbin; ++i) {
        x_mag[i] *= scale_1;
    }
    ASSIGN_SHAPE(shape[0], 1, 1, 257, 0);
    ASSIGN_SHAPE(shape[1], 1, 1, 257, 0);

    i = 0;
    input_val[i] = qtk_nnrt_value_create_external(
        cmask_aec2->dereb_rt, QTK_NNRT_VALUE_ELEM_F32, shape[i], 3, x_mag);
    qtk_nnrt_feed_s(cmask_aec2->dereb_rt, input_val[i], in_name[i]);

    for (i = num_in2; i < num_in; ++i) {
        if (cmask_aec2->dereb_nframe == 0) {
            int64_t nelem;
            float *input_data;
            shape_len = shape[i][2] == 0 ? 2 : shape[i][3] == 0 ? 3:4;
            if(shape_len==2){
                nelem = shape[i][0]*shape[i][1];
            }else if(shape_len==3){
                nelem = shape[i][0]*shape[i][1]*shape[i][2];
            }else{
                nelem = TENSOR_NELEM(shape[i]);
            }
            input_data =
                wtk_heap_zalloc(cmask_aec2->dereb_heap, sizeof(float) * nelem);
            input_val[i] = qtk_nnrt_value_create_external(
                cmask_aec2->dereb_rt, QTK_NNRT_VALUE_ELEM_F32, shape[i], shape_len,
                input_data);
            qtk_nnrt_feed_s(cmask_aec2->dereb_rt, input_val[i], in_name[i]);
        } else {
            input_val[i] = cmask_aec2->dereb_cache_value[in_index[i]];
            qtk_nnrt_feed_s(cmask_aec2->dereb_rt, input_val[i], in_name[i]);
        }
    }

    for (i = 0; i < num_out; ++i) {
        if (out_index[i] != -1) {
            qtk_nnrt_get_output_s(cmask_aec2->dereb_rt,
                                cmask_aec2->dereb_cache_value + out_index[i],
                                out_name[i]);
        }else{
            qtk_nnrt_get_output_s(cmask_aec2->dereb_rt, output_val+i, out_name[i]);
            if(strcmp(out_name[i], "out0")==0){
                clean_mag = qtk_nnrt_value_get_data(cmask_aec2->dereb_rt, output_val[i]);
            }
        }
    }
    for(i=0;i<nbin;++i){
        x_mag[i] *= scale;
        clean_mag[i] *= scale;
    }
    static int state=0;
    if(cmask_aec2->cfg->use_wpe && cmask_aec2->wpe_sil==0){
        if(state==1){
            for(i=0;i<nbin;++i){
                for(int j=0;j<cmask_aec2->cfg->dereb_taps;++j){
                    for(int k=0;k<cmask_aec2->cfg->dereb_taps;++k){
                        if(j==k){
                            cmask_aec2->inv_R_WPE[i][j][k] = 1.0;
                        }else{
                            cmask_aec2->inv_R_WPE[i][j][k] = 0.0;
                        }
                    }
                }
            }
            wtk_float_zero_p2(cmask_aec2->G_WPE, cmask_aec2->nbin, cmask_aec2->cfg->dereb_taps);
        }
        wtk_cmask_aec2_feed_wpe(cmask_aec2, x_mag, clean_mag, x_phase, fftx);
    }else{
        for(i=0;i<nbin;++i){
            fftx[i].a = clean_mag[i] * cosf(x_phase[i]);
            fftx[i].b = clean_mag[i] * sinf(x_phase[i]);
        }
        state=1;
    }

    for (i = 0; i < num_in; i++) {
        qtk_nnrt_value_release(cmask_aec2->dereb_rt, input_val[i]);
    }
    for(i=0;i<num_out;++i){
        if(out_index[i]==-1){
            qtk_nnrt_value_release(cmask_aec2->dereb_rt, output_val[i]);
        }
    }
    qtk_nnrt_reset(cmask_aec2->dereb_rt);
    if(cmask_aec2->dereb_nframe==0){
        ++cmask_aec2->dereb_nframe;
    }
}

void wtk_cmask_aec2_feed_gain(wtk_cmask_aec2_t* cmask_aec2, wtk_complex_t* fft)
{
    int de_clip_s = cmask_aec2->cfg->de_clip_s;
    int de_clip_e = cmask_aec2->cfg->de_clip_e;
    float gain_alpha = cmask_aec2->cfg->gain_alpha;
    float gain_alpha2 = cmask_aec2->cfg->gain_alpha2;
    float gain_beta = cmask_aec2->cfg->gain_beta;
    const int TRANS_WIDTH = 8; // 建议值8

    if(cmask_aec2->mic_sil) return;

    // 1. 双通道能量追踪
    for(int k=0; k<de_clip_e; ++k) {
        float mag = sqrtf(fft[k].a*fft[k].a + fft[k].b*fft[k].b);
        float curr_energy = mag * mag;
        
        // 主通道：快速跟踪强信号
        cmask_aec2->energy_buf[k] = 0.7f * cmask_aec2->energy_buf[k] + 0.3f * curr_energy;
        cmask_aec2->hist_energy[k] = gain_alpha * cmask_aec2->energy_buf[k] 
                                  + (1-gain_alpha) * cmask_aec2->hist_energy[k];
        
        // 辅助通道：慢速跟踪基底
        cmask_aec2->aux_hist[k] = (1-gain_alpha2) * cmask_aec2->aux_hist[k] + gain_alpha2 * curr_energy;
    }

    // 2. 自适应增益计算
    for(int k=0; k<de_clip_e; ++k) {
        float hist_energy = fmaxf(cmask_aec2->hist_energy[k], cmask_aec2->aux_hist[k]);
        float delta = fmaxf(cmask_aec2->energy_buf[k] - hist_energy, 0.0f);
        
        // 低频保护区（0-2000Hz）
        if(k < de_clip_s - TRANS_WIDTH) {
            cmask_aec2->gains[k] = 1.0f;
        }
        // 过渡带处理
        else if(k < de_clip_s + TRANS_WIDTH) {
            float blend = (float)(k - (de_clip_s - TRANS_WIDTH)) / (2 * TRANS_WIDTH);
            float denom = hist_energy + gain_beta * delta;
            float raw_gain = sqrtf(hist_energy / (denom + 1e-10f));
            cmask_aec2->gains[k] = (1-blend) + blend * raw_gain;
        }
        // 高频处理区
        else {
            float denom = hist_energy + gain_beta * delta;
            cmask_aec2->gains[k] = sqrtf(hist_energy / (denom + 1e-10f));
            
            // 弱信号保护
            if(cmask_aec2->energy_buf[k] < cmask_aec2->aux_hist[k]) {
                cmask_aec2->gains[k] = 1.0f;
            }
        }
    }

    // 3. 时频联合平滑
    const int HALF_WIN = 2;
    static const float GAUSS_WEIGHTS[] = {0.18f, 0.64f, 1.0f, 0.64f, 0.18f};
    for(int k=de_clip_s - TRANS_WIDTH; k<de_clip_e; ++k) {
        float sum = 0.0f, weight_sum = 0.0f;
        
        // 频域高斯平滑
        for(int d=-HALF_WIN; d<=HALF_WIN; ++d) {
            int idx = k + d;
            if(idx >=0 && idx < de_clip_e) {
                float w = GAUSS_WEIGHTS[d + HALF_WIN];
                sum += w * cmask_aec2->gains[idx];
                weight_sum += w;
            }
        }
        
        // 时域递归平滑
        if(weight_sum > 0.1f) {
            float smoothed = sum / weight_sum;
            cmask_aec2->gains[k] = 0.8f * smoothed + 0.2f * cmask_aec2->prev_gains[k];
            cmask_aec2->prev_gains[k] = cmask_aec2->gains[k];
        }
    }

    // 4. 应用增益
    for(int k=0; k<de_clip_e; ++k) {
        fft[k].a *= cmask_aec2->gains[k];
        fft[k].b *= cmask_aec2->gains[k];
    }
}

void wtk_cmask_aec2_notify_data(wtk_cmask_aec2_t *cmask_aec2, wtk_complex_t *fftx, wtk_complex_t *ffty, wtk_complex_t *fft, wtk_complex_t *fft_sp, wtk_complex_t *fft_gf, int nframe)
{
    int i;
    int nbin = cmask_aec2->nbin;
    wtk_drft_t *rfft = cmask_aec2->rfft;
    float *rfft_in = cmask_aec2->rfft_in;
    float *synthesis_mem = cmask_aec2->synthesis_mem;
    float *synthesis_window = cmask_aec2->synthesis_window;
    float *out = cmask_aec2->out;
    short *pv = (short *)out;
    int wins = cmask_aec2->cfg->wins;
    int fsize = wins / 2;
    int clip_s = cmask_aec2->cfg->clip_s;
    int clip_e = cmask_aec2->cfg->clip_e;
    float entropy=0;
    float entropy_thresh = cmask_aec2->cfg->entropy_thresh;
    float entropy_sp_thresh = cmask_aec2->cfg->entropy_sp_thresh;
    int entropy_cnt = cmask_aec2->cfg->entropy_cnt;
    
    float raw_alpha1 = cmask_aec2->cfg->raw_alpha1;
    float raw_alpha2 = cmask_aec2->cfg->raw_alpha2;
    float raw_alpha3 = cmask_aec2->cfg->raw_alpha3;
    float raw_alpha4 = cmask_aec2->cfg->raw_alpha4;
    float raw_alpha;
    float raw_alpha_1;
    float *power_k = cmask_aec2->power_k;
    float raw_alphak;
    float raw_alphak_1;
    float alpha1 = cmask_aec2->cfg->alpha1;
    float alpha2 = cmask_aec2->cfg->alpha2;
    float alpha3 = cmask_aec2->cfg->alpha3;
    float alpha4 = cmask_aec2->cfg->alpha4;
    float scale1 = cmask_aec2->cfg->scale1;
    float scale2 = cmask_aec2->cfg->scale2;
    float scale3 = cmask_aec2->cfg->scale3;
    float scale4 = cmask_aec2->cfg->scale4;
    float micenr;
    float micenr_thresh = cmask_aec2->cfg->micenr_thresh;
    int micenr_cnt = cmask_aec2->cfg->micenr_cnt;
    float wpe_thresh = cmask_aec2->cfg->wpe_thresh;
    int wpe_cnt = cmask_aec2->cfg->wpe_cnt;
    float entropy_scale;
    float *mask=cmask_aec2->mask;
    wtk_complex_t **raw_fft;
    float max_mask=cmask_aec2->cfg->max_mask;
    float mask_peak=cmask_aec2->cfg->mask_peak;
    if(cmask_aec2->raw_fft){
        raw_fft = cmask_aec2->raw_fft[nframe];
    }

    if(cmask_aec2->raw_fft)
    {
        for(i=0;i<nbin;++i){
            mask[i] = sqrtf(fftx[i].a * fftx[i].a + fftx[i].b * fftx[i].b) / (sqrtf(raw_fft[0][i].a * raw_fft[0][i].a + raw_fft[0][i].b * raw_fft[0][i].b) + 1e-9);
            if(mask_peak>0){
                if(mask[i] < mask_peak){
                    mask[i] = (max_mask/(mask_peak*mask_peak) - 1/mask_peak) * mask[i] * mask[i] + mask[i];
                }else{
                    mask[i] = max_mask;
                }
            }
            mask[i] = max(0.0, min(1.0, mask[i]));
        }
    }
    if(cmask_aec2->cfg->use_qmmse2){
        // for(i=0;i<nbin;++i){
        //     fftx[i].a = fft_gf[i].a * mask[i];
        //     fftx[i].b = fft_gf[i].b * mask[i];
        // }
        if(cmask_aec2->qmmse2)
        {
            float *gf=cmask_aec2->gf;
            wtk_qmmse_t *qmmse2=cmask_aec2->qmmse2;
            wtk_complex_t *fftytmp, sed, *fftxtmp;
            float ef,yf;
            float leak;
            int k;
            fftxtmp=fftx;
            fftytmp=fft_gf;
            if(cmask_aec2->cfg->use_scale_qmmse2){
                for(k=0;k<nbin;++k){
                    fftxtmp[k].a *= 1.0/cmask_aec2->cfg->wins;
                    fftxtmp[k].b *= 1.0/cmask_aec2->cfg->wins;
                    fftytmp[k].a *= 1.0/cmask_aec2->cfg->wins;
                    fftytmp[k].b *= 1.0/cmask_aec2->cfg->wins;
                }
            }
            for(k=0;k<nbin;++k,++fftxtmp,++fftytmp)
            {
                ef=fftxtmp->a*fftxtmp->a+fftxtmp->b*fftxtmp->b;
                yf=fftytmp->a*fftytmp->a+fftytmp->b*fftytmp->b;
                sed.a=fftytmp->a*fftxtmp->a+fftytmp->b*fftxtmp->b;
                sed.b=-fftytmp->a*fftxtmp->b+fftytmp->b*fftxtmp->a;
                leak=(sed.a*sed.a+sed.b*sed.b)/(max(ef,yf)*yf+1e-9);
                leak=sqrtf(leak);
                // fftytmp->a*=leak;
                // fftytmp->b*=leak;
                leak=(sed.a*sed.a+sed.b*sed.b)/(ef*yf+1e-9);
                gf[k]=leak*yf;
            }
            fftxtmp=fftx;
            fftytmp=fft_gf;
            wtk_qmmse_flush_mask(qmmse2, fftxtmp, gf);
            if(cmask_aec2->cfg->use_scale_qmmse2){
                for(k=0;k<nbin;++k){
                    fftxtmp[k].a *= 1.0*cmask_aec2->cfg->wins;
                    fftxtmp[k].b *= 1.0*cmask_aec2->cfg->wins;
                    fftytmp[k].a *= 1.0*cmask_aec2->cfg->wins;
                    fftytmp[k].b *= 1.0*cmask_aec2->cfg->wins;
                }
            }
        }
    }

    if(entropy_thresh>0 || entropy_sp_thresh>0){
        entropy=wtk_cmask_aec2_entropy(cmask_aec2, fftx);
    }
    if(cmask_aec2->cfg->eng_scale!=1.0){
        float eng_1=0;
        float eng_2=0;
        float freq=cmask_aec2->cfg->eng_freq;
        int freq_idx=floor(nbin/(cmask_aec2->cfg->rate/2/freq));
        float eng1_thresh;
        float eng2_thresh;
        for(i=0;i<freq_idx;++i){
            eng_1+=fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b;
        }
        for(i=freq_idx;i<nbin;++i){
            eng_2+=fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b;
        }
        eng_1 = eng_1/(eng_2+1e-9);
        // printf("%f\n", eng_1);
        // printf("%f\n", eng_2);
        if(cmask_aec2->sp_sil){
            eng1_thresh=cmask_aec2->cfg->eng1_thresh;
            eng2_thresh=cmask_aec2->cfg->eng2_thresh;
            if(eng_1>eng1_thresh || eng_2 < eng2_thresh){
                --cmask_aec2->eng_cnt;
                if(cmask_aec2->eng_times==1 || cmask_aec2->eng_cnt<=0){
                    cmask_aec2->eng_cnt=0;
                    cmask_aec2->eng_times = 0;
                }
            }else{
                cmask_aec2->eng_cnt=cmask_aec2->cfg->eng_cnt;
                cmask_aec2->eng_times += 1;
            }
            if(cmask_aec2->eng_cnt <= 0){
                scale3 = cmask_aec2->cfg->eng_scale;
                scale4 = cmask_aec2->cfg->eng_scale;
            }
        }else{
            eng1_thresh=cmask_aec2->cfg->eng1_thresh2;
            eng2_thresh=cmask_aec2->cfg->eng2_thresh2;
            if(eng_1>eng1_thresh || eng_2 < eng2_thresh){
                --cmask_aec2->sp_eng_cnt;
                if(cmask_aec2->sp_eng_times==1 || cmask_aec2->sp_eng_cnt<=0){
                    cmask_aec2->sp_eng_cnt=0;
                    cmask_aec2->sp_eng_times = 0;
                }
            }else{
                cmask_aec2->sp_eng_cnt=cmask_aec2->cfg->eng_cnt;
                cmask_aec2->sp_eng_times += 1;
            }
            if(cmask_aec2->sp_eng_cnt <= 0){
                scale1 = cmask_aec2->cfg->eng_scale;
                scale2 = cmask_aec2->cfg->eng_scale;
            }
        }
    }

    // static int cnt=0;
    // cnt++;
    micenr = wtk_cmask_aec2_fft_energy(fftx, nbin);
    if (micenr > micenr_thresh) {
        // if(cmask_aec2->mic_sil==1)
        // {
        // 	printf("sp start %f %f
        // %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
        // }
        cmask_aec2->mic_sil = 0;
        cmask_aec2->mic_silcnt = micenr_cnt;
    } else if (cmask_aec2->mic_sil == 0) {
        cmask_aec2->mic_silcnt -= 1;
        if (cmask_aec2->mic_silcnt <= 0) {
            // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
            cmask_aec2->mic_sil = 1;
        }
    }
    // printf("%f\n", micenr);

    if(micenr > cmask_aec2->cfg->micenr_thresh2){
        cmask_aec2->mic_silcnt2 = cmask_aec2->cfg->micenr_cnt2;
        cmask_aec2->mic_sil2 = 0;
    }else if(cmask_aec2->mic_silcnt2>cmask_aec2->cfg->micenr_cnt3){
        --cmask_aec2->mic_silcnt2;
    }else if(cmask_aec2->mic_silcnt2>0){
        if(micenr>cmask_aec2->cfg->micenr_thresh3){
            cmask_aec2->mic_silcnt2 = cmask_aec2->cfg->micenr_cnt3;
        }else{
            --cmask_aec2->mic_silcnt2;
        }
    }else{
        cmask_aec2->mic_sil2 = 1;
    }
    if(cmask_aec2->mic_sil2==1 && cmask_aec2->cfg->micenr_scale!=1.0){
        scale1 = scale2 = scale3 = scale4 = cmask_aec2->cfg->micenr_scale;
    }

    if(entropy_thresh>0){
        if(entropy<entropy_thresh){
            ++cmask_aec2->entropy_in_cnt;
        }else{
            cmask_aec2->entropy_in_cnt = 0;
        }
        if(cmask_aec2->entropy_in_cnt>=cmask_aec2->cfg->entropy_in_cnt){
            cmask_aec2->entropy_sil = 0;
            cmask_aec2->entropy_silcnt = entropy_cnt;
        }else if(cmask_aec2->entropy_sil==0){
            cmask_aec2->entropy_silcnt -= 1;
            if(cmask_aec2->entropy_silcnt<=0){
                cmask_aec2->entropy_sil = 1;
            }
        }
    }
    if(entropy_sp_thresh>0){
        if(entropy<entropy_sp_thresh){
            ++cmask_aec2->entropy_sp_in_cnt;
        }else{
            cmask_aec2->entropy_sp_in_cnt = 0;
        }
        if(cmask_aec2->entropy_sp_in_cnt>=cmask_aec2->cfg->entropy_in_cnt){
            cmask_aec2->entropy_sp_sil = 0;
            cmask_aec2->entropy_sp_silcnt = entropy_cnt;
        }else if(cmask_aec2->entropy_sp_sil==0){
            cmask_aec2->entropy_sp_silcnt -= 1;
            if(cmask_aec2->entropy_sp_silcnt<=0){
                cmask_aec2->entropy_sp_sil = 1;
            }
        }
    }
    if(cmask_aec2->raw_fft)
    {
        if(cmask_aec2->maskssl2){
            wtk_maskssl2_feed_fft2(cmask_aec2->maskssl2, raw_fft, mask, cmask_aec2->mic_sil);
        }else if(cmask_aec2->ibf_ssl){
            wtk_maskssl2_feed_fft2(cmask_aec2->ibf_ssl, raw_fft, mask, cmask_aec2->mic_sil);
        }
    }
    if(raw_alpha1!=1 || raw_alpha2!=1 || raw_alpha3!=1 || raw_alpha4!=1){
        if(cmask_aec2->sp_sil==0){
            if(cmask_aec2->entropy_sp_sil==0){
                raw_alpha = raw_alpha1;
                raw_alpha_1 = 1.0 - raw_alpha;
            }else{
                raw_alpha = raw_alpha2;
                raw_alpha_1 = 1.0 - raw_alpha;
            }
        }else{
            if(cmask_aec2->entropy_sil==0){
                raw_alpha = raw_alpha3;
                raw_alpha_1 = 1.0 - raw_alpha;
            }else{
                raw_alpha = raw_alpha4;
                raw_alpha_1 = 1.0 - raw_alpha;
            }
        }
        if(raw_alpha!=1){
            for(i=0;i<nbin;++i){
                raw_alphak_1 = power_k[i] * raw_alpha_1;
                raw_alphak = 1.0 - raw_alphak_1;
                fftx[i].a = raw_alphak * fftx[i].a + raw_alphak_1 * fft[i].a;
                fftx[i].b = raw_alphak * fftx[i].b + raw_alphak_1 * fft[i].b;
            }
        }
    }
    // printf("%d\n", cmask_aec2->mic_sil);
    // printf("%d\n", cmask_aec2->sp_sil);
    // printf("%d\n", cmask_aec2->entropy_sil);
    // printf("%d\n", cmask_aec2->entropy_sp_sil);
    // printf("%f\n", entropy);

    if(cmask_aec2->raw_fft){
        if(cmask_aec2->qmmse2){
            for(i=0;i<nbin;++i){
                mask[i] = min(cmask_aec2->qmmse2->gain[i], mask[i]);
                fftx[i].a = raw_fft[0][i].a * mask[i];
                fftx[i].b = raw_fft[0][i].b * mask[i];
            }
        }
    }

    if(cmask_aec2->cfg->use_bf){
        if(cmask_aec2->cfg->use_mask_bf){
            for(i=0;i<nbin;++i){
                ffty[i].a = fft[i].a * (1-mask[i]);
                ffty[i].b = fft[i].b * (1-mask[i]);
            }
        }
        wtk_cmask_aec2_feed_bf(cmask_aec2, fftx, ffty);
    }
    
    float gc_mask = 1;
    if(cmask_aec2->mask){
        gc_mask = wtk_float_abs_mean(cmask_aec2->mask, nbin);
    }
    // printf("%f\n", gc_mask);
    if(gc_mask > cmask_aec2->cfg->gc_min_thresh){
        cmask_aec2->gc_cnt = cmask_aec2->cfg->gc_cnt;
    }else{
        --cmask_aec2->gc_cnt;
    }

    if(cmask_aec2->qmmse){
        if(cmask_aec2->sp_sil==0){  // 有回声
            if(cmask_aec2->entropy_sp_sil==0){  // 双讲
                cmask_aec2->qmmse->cfg->io_alpha = alpha1;
            }else{  // 单讲
                cmask_aec2->qmmse->cfg->io_alpha = alpha2;
            }
        }else{  // 无回声语音
            if(cmask_aec2->entropy_sil==0){
                cmask_aec2->qmmse->cfg->io_alpha = alpha3;
            }else{  // 纯噪声
                cmask_aec2->qmmse->cfg->io_alpha = alpha4;
            }
        }
        if(cmask_aec2->cfg->mic_max_smooth_gain==-1){
            cmask_aec2->cfg->mic_max_smooth_gain = cmask_aec2->qmmse->cfg->max_smooth_gain;
            cmask_aec2->cfg->mic_min_smooth_gain = cmask_aec2->qmmse->cfg->min_smooth_gain;
        }
        if(cmask_aec2->cfg->sp_max_smooth_gain!=-1){
            if(cmask_aec2->sp_sil==0){
                cmask_aec2->qmmse->cfg->max_smooth_gain = cmask_aec2->cfg->sp_max_smooth_gain;
                cmask_aec2->qmmse->cfg->min_smooth_gain = cmask_aec2->cfg->sp_min_smooth_gain;
            }else{
                cmask_aec2->qmmse->cfg->max_smooth_gain = cmask_aec2->cfg->mic_max_smooth_gain;
                cmask_aec2->qmmse->cfg->min_smooth_gain = cmask_aec2->cfg->mic_min_smooth_gain;
            }
        }

        if(cmask_aec2->cfg->use_scale_qmmse){
            for(i=0;i<nbin;++i){
                fftx[i].a *= 1.0/cmask_aec2->cfg->wins;
                fftx[i].b *= 1.0/cmask_aec2->cfg->wins;
                ffty[i].a *= 1.0/cmask_aec2->cfg->wins;
                ffty[i].b *= 1.0/cmask_aec2->cfg->wins;
            }
        }
        wtk_qmmse_feed_echo_denoise3(cmask_aec2->qmmse, fftx, ffty, cmask_aec2->sp_sil);
        if(cmask_aec2->cfg->use_scale_qmmse){
            for(i=0;i<nbin;++i){
                fftx[i].a *= 1.0*cmask_aec2->cfg->wins;
                fftx[i].b *= 1.0*cmask_aec2->cfg->wins;
                ffty[i].a *= 1.0*cmask_aec2->cfg->wins;
                ffty[i].b *= 1.0*cmask_aec2->cfg->wins;
            }
        }
        if(cmask_aec2->sp_sil==0){
            if(cmask_aec2->entropy_sp_sil==0){
                for(i=0;i<nbin;++i){
                    fftx[i].a *= scale1;
                    fftx[i].b *= scale1;
                }
            }else{
                for(i=0;i<nbin;++i){
                    fftx[i].a *= scale2;
                    fftx[i].b *= scale2;
                }
            }
        }else{
            if(cmask_aec2->entropy_sil==0){
                for(i=0;i<nbin;++i){
                    fftx[i].a *= scale3;
                    fftx[i].b *= scale3;
                }
            }else{
                for(i=0;i<nbin;++i){
                    fftx[i].a *= scale4;
                    fftx[i].b *= scale4;
                }
            }
        }
    }

    float de_alpha=cmask_aec2->cfg->de_alpha;
    if(de_alpha!=1.0){
        float de_eng_sum=0;
        float de_eng_mean=0;
        int de_clip_s=cmask_aec2->cfg->de_clip_s;
        int de_clip_e=cmask_aec2->cfg->de_clip_e;
        float de_thresh=cmask_aec2->cfg->de_thresh;
        float de_alpha2;
        for(i=de_clip_s;i<de_clip_e;++i){
            de_eng_sum+=fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b;
        }
        de_eng_mean = de_eng_sum/(de_clip_e-de_clip_s);
        // printf("%f\n", de_eng_mean);
        if(de_eng_mean>de_thresh){
            for(i=de_clip_s;i<de_clip_e;++i){
                de_eng_sum = fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b;
                if(de_eng_sum>de_thresh){
                    de_alpha2 = de_thresh/de_eng_sum * (1.0-de_alpha) + de_alpha;
                    fftx[i].a*=de_alpha2;
                    fftx[i].b*=de_alpha2;
                }
            }
        }
    }

    if(cmask_aec2->gc){
        // printf("%f\n", wtk_float_abs_mean(cmask_aec2->mask, nbin));
        if(cmask_aec2->gc_cnt>0){
            qtk_gain_controller_run(cmask_aec2->gc, fftx, fsize, NULL,gc_mask);
        }
    }
    if(cmask_aec2->gc_cnt<0){
        memset(fftx, 0, sizeof(wtk_complex_t)*nbin);
    }
    if(cmask_aec2->entropy_sil==1){
        memset(fftx, 0, sizeof(wtk_complex_t)*nbin);
    }

    for (i = 0; i <= clip_s; ++i) {
        fftx[i].a = fftx[i].b = cmask_aec2->cfg->init_low_freq;
    }
    for (i = clip_e; i < nbin; ++i) {
        fftx[i].a = fftx[i].b = cmask_aec2->cfg->init_high_freq;
    }

    if(micenr > wpe_thresh){
        cmask_aec2->wpe_sil = 0;
        cmask_aec2->wpe_silcnt = wpe_cnt;
    }else if(cmask_aec2->wpe_sil==0){
        cmask_aec2->wpe_silcnt -= 1;
        if(cmask_aec2->wpe_silcnt<=0){
            cmask_aec2->wpe_sil = 1;
        }
    }

    if(cmask_aec2->cfg->use_freq_atten){
        wtk_cmask_aec2_feed_gain(cmask_aec2, fftx);
    }
    if(entropy>entropy_thresh){
        if(cmask_aec2->cfg->use_entropy_scale){
            if(cmask_aec2->entropy_silcnt > 0){
                entropy_scale = powf(cmask_aec2->entropy_silcnt * 1.0/entropy_cnt, cmask_aec2->cfg->entropy_ratio)+cmask_aec2->cfg->entropy_min_scale;
            }else{
                entropy_scale = powf(1.0/entropy_cnt, cmask_aec2->cfg->entropy_ratio)+cmask_aec2->cfg->entropy_min_scale;
            }
            entropy_scale = min(entropy_scale, 1.0);
            for(i=0;i<nbin;++i){
                fftx[i].a*=entropy_scale;
                fftx[i].b*=entropy_scale;
            }
        }
    }

    if(cmask_aec2->cfg->use_dereb){
        wtk_cmask_aec2_feed_dereb(cmask_aec2, fftx);
    }

    if (cmask_aec2->cfg->use_cnon) {
        wtk_cmask_aec2_feed_cnon(cmask_aec2, fftx);
    }
    // float eq_mask[257]={1.000000, 1.000000, 1.258925, 1.584893, 1.995262, 2.511886, 2.973280, 2.644024, 2.256586, 1.873245, 1.529078, 1.314260, 1.184870, 1.103666, 1.049772, 1.015508, 1.004084, 1.027122, 1.100311, 1.233672, 1.417660, 1.618624, 1.789250, 1.876451, 1.840964, 1.694374, 1.488933, 1.272773, 1.078586, 0.927563, 0.826011, 0.767893, 0.742714, 0.740253, 0.751401, 0.768031, 0.783363, 0.792858, 0.794775, 0.789450, 0.777544, 0.758652, 0.731454, 0.695235, 0.651484, 0.604398, 0.559955, 0.524142, 0.501599, 0.495449, 0.507867, 0.540278, 0.592292, 0.659756, 0.734232, 0.805780, 0.867280, 0.916840, 0.956433, 0.987546, 1.008300, 1.015361, 1.008073, 0.990968, 0.972923, 0.963462, 0.969632, 0.995662, 1.042669, 1.105043, 1.166768, 1.205896, 1.206213, 1.165594, 1.096982, 1.021529, 0.956371, 0.908239, 0.877013, 0.861515, 0.862193, 0.880875, 0.918785, 0.973862, 1.039518, 1.106978, 1.170115, 1.228641, 1.286275, 1.343335, 1.389995, 1.409911, 1.391932, 1.338684, 1.267754, 1.204330, 1.169307, 1.174368, 1.226087, 1.330205, 1.489670, 1.695928, 1.919534, 2.111293, 2.220133, 2.221355, 2.134469, 2.009265, 1.893294, 1.812987, 1.774306, 1.770781, 1.792721, 1.835603, 1.906229, 2.026423, 2.234137, 2.482249, 2.734383, 2.963030, 3.123492, 3.162278, 3.092757, 2.941582, 2.744725, 2.531973, 2.319329, 2.155456, 2.034323, 1.924002, 1.806969, 1.681760, 1.560122, 1.459695, 1.396315, 1.381138, 1.420406, 1.509043, 1.617954, 1.696851, 1.701850, 1.618806, 1.468792, 1.297756, 1.146310, 1.029604, 0.945676, 0.889367, 0.857200, 0.847186, 0.857681, 0.886120, 0.927089, 0.970319, 1.001585, 1.008060, 0.984245, 0.934106, 0.868925, 0.801898, 0.742770, 0.696411, 0.664565, 0.647452, 0.643927, 0.650949, 0.663481, 0.675348, 0.680716, 0.675557, 0.658755, 0.632381, 0.600804, 0.569290, 0.542955, 0.526123, 0.521895, 0.531945, 0.556302, 0.592537, 0.634336, 0.671539, 0.693892, 0.696505, 0.682523, 0.661529, 0.644951, 0.641223, 0.653710, 0.681140, 0.718301, 0.756640, 0.786413, 0.800643, 0.798310, 0.784509, 0.767738, 0.756125, 0.754853, 0.765946, 0.789388, 0.823842, 0.866366, 0.911908, 0.953989, 0.987081, 1.009543, 1.025482, 1.044473, 1.079266, 1.143033, 1.247090, 1.397965, 1.593337, 1.820475, 2.060712, 2.297339, 2.522749, 2.732299, 2.900926, 3.032206, 3.121718, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278};
    if(cmask_aec2->cfg->use_out_eq && cmask_aec2->sp_sil){
        float *eq_gain = cmask_aec2->cfg->eq_gain;
        for(i=0;i<nbin;++i){
            fftx[i].a *= eq_gain[i];
            fftx[i].b *= eq_gain[i];
        }
    }
    fftx[0].a = fftx[0].b = 0;
    fftx[nbin-1].a = fftx[nbin-1].b = 0;

    wtk_drft_frame_synthesis22(rfft, rfft_in, synthesis_mem, fftx,
                                out, wins, synthesis_window);
    if (cmask_aec2->eq) {
        wtk_equalizer_feed_float(cmask_aec2->eq, out, fsize);
    }
    wtk_cmask_aec2_control_bs(cmask_aec2, out, fsize);
    for (i = 0; i < fsize; ++i) {
        pv[i] = floorf(out[i] + 0.5);
    }
    if (cmask_aec2->notify) {
        cmask_aec2->notify(cmask_aec2->ths, pv, fsize);
    }
}

void wtk_cmask_aec2_feed_change_mic(wtk_cmask_aec2_t * cmask_aec2, int len)
{
    int ncmicchannel=cmask_aec2->cfg->ncmicchannel;
    int ncrefchannel=cmask_aec2->cfg->ncrefchannel;
    int ncchannel=cmask_aec2->cfg->ncchannel;
    int *change_mic=cmask_aec2->cfg->change_mic;
    wtk_strbuf_t **cmic=cmask_aec2->cmic;
    int i, j;
    float alpha=cmask_aec2->cfg->change_alpha;
    float alpha_1 = 1.0 - alpha;
    float *eng = cmask_aec2->change_eng;
    int c_cnt;
    float std;
    float sum;
    short *fv;

    for(i=0;i<ncchannel;++i){
        sum=0;
        fv = (short *)cmic[i]->data;
        for(j=0;j<len;++j){
            sum+=fv[j]*fv[j];
        }
        sum*=1.0/len;
        sum = 10 * log10f(sum+1e-9);
        if(eng[i]==-1){
            eng[i]=sum;
        }else{
            eng[i] = alpha * eng[i] + alpha_1 * sum;
        }
    }
    sum=0;
    c_cnt=0;
    for(i=0;i<ncmicchannel;++i){
        if(cmask_aec2->change_error[i]==0){
            sum+=eng[i];
            ++c_cnt;
        }
    }
    if(c_cnt==0){
        sum=0;
    }else{
        sum*=1.0/c_cnt;
    }

    std=0;
    c_cnt=0;
    for(i=0;i<ncmicchannel;++i){
        if(cmask_aec2->change_error[i]==0){
            std += (eng[i]-sum)*(eng[i]-sum);
            ++c_cnt;
        }
    }
    if(sum==0 || std==0){
        for(i=0;i<ncmicchannel;++i){
            cmask_aec2->change_error[i] = 0;
        }
    }else{
        std = sqrtf(std/c_cnt);
    }
    // printf("%f\n", std);
    // printf("%f\n", sum);
    if(std>cmask_aec2->cfg->change_thresh){
        sum = 0;
        for(j=0;j<ncrefchannel;++j){
            sum += eng[j+ncmicchannel];
        }
        sum*=1.0/(ncrefchannel);
        
        std = 0;
        for(j=0;j<ncrefchannel;++j){
            std += (eng[j+ncmicchannel]-sum)*(eng[j+ncmicchannel]-sum);
        }
        std = sqrtf(std/ncrefchannel);
        if(std<cmask_aec2->cfg->change_thresh){
            for(i=0;i<ncmicchannel;++i){
                sum = eng[i];
                for(j=0;j<ncrefchannel;++j){
                    sum += eng[j+ncmicchannel];
                }
                sum*=1.0/(ncrefchannel+1);
                
                std = (eng[i]-sum)*(eng[i]-sum);
                for(j=0;j<ncrefchannel;++j){
                    std += (eng[j+ncmicchannel]-sum)*(eng[j+ncmicchannel]-sum);
                }
                std = sqrtf(std/(ncrefchannel+1));
                if(std>cmask_aec2->cfg->change_thresh){
                    cmask_aec2->change_error[i] = 1;
                }else{
                    cmask_aec2->change_idx = change_mic[i];
                }
            }
        }
    }
}

void wtk_cmask_aec2_feed_change_mic2(wtk_cmask_aec2_t * cmask_aec2, int len)
{
    int ncmicchannel=cmask_aec2->cfg->ncmicchannel;
    wtk_strbuf_t **cmic=cmask_aec2->cmic;
    int i, j;
    float alpha=cmask_aec2->cfg->change_alpha2;
    float alpha_1 = 1.0 - alpha;
    float *eng = cmask_aec2->change_eng2;
    float sum;
    short *fv;
    float max_eng=1e-10;
    int idx=0;

    for(i=0;i<ncmicchannel;++i){
        sum=0;
        fv = (short *)cmic[i]->data;
        for(j=0;j<len;++j){
            sum+=fv[j]*fv[j];
        }
        sum*=1.0/len;
        sum = 10 * log10f(sum+1e-9);
        if(eng[i]==-1){
            eng[i]=sum;
        }else{
            eng[i] = alpha * eng[i] + alpha_1 * sum;
        }
    }

    for(i=0;i<ncmicchannel;++i){
        if(eng[i]>max_eng){
            max_eng=eng[i];
            idx=i;
        }
    }
    // printf("%f\n", eng[0]);
    if(cmask_aec2->change_delay<=0){
        if(cmask_aec2->cfg->use_change_mic){
            if(cmask_aec2->change_error[idx]==0){
                cmask_aec2->change_idx = idx;
            }
        }else{
            cmask_aec2->change_idx = idx;
        }
        cmask_aec2->change_delay = cmask_aec2->cfg->change_delay;
    }else{
        cmask_aec2->change_delay--;
    }
}

void wtk_cmask_aec2_feed_ibf(wtk_cmask_aec2_t *cmask_aec2, wtk_complex_t **ibf_fft, wtk_complex_t *fft)
{
	int i, k;
	wtk_bf_t *bf=cmask_aec2->ibf;
	wtk_complex_t fft2[64];
    int nbin = cmask_aec2->nbin;
    int nibfchannel = cmask_aec2->cfg->nibfchannel;

    for(k=1; k<nbin-1; ++k)
    {
        for(i=0; i<nibfchannel; ++i)
        {
            fft2[i].a=ibf_fft[i][k].a;
            fft2[i].b=ibf_fft[i][k].b;
        }
        wtk_bf_output_fft_k(bf, fft2, fft+k, k);
	}
}

void wtk_cmask_aec2_feed(wtk_cmask_aec2_t *cmask_aec2, short *data, int len,
                        int is_end) {
    int i, j, n;
    int nbin = cmask_aec2->nbin;
    int nmicchannel = cmask_aec2->cfg->nmicchannel;
    int nspchannel = cmask_aec2->cfg->nspchannel;
    int channel = cmask_aec2->cfg->channel;
    int wins = cmask_aec2->cfg->wins;
    int fsize = wins / 2;
    float spenr;
    float spenr_thresh = cmask_aec2->cfg->spenr_thresh;
    int spenr_cnt = cmask_aec2->cfg->spenr_cnt;
    wtk_drft_t *rfft = cmask_aec2->rfft;
    float *rfft_in = cmask_aec2->rfft_in;
    wtk_complex_t **fft = cmask_aec2->fft;
    wtk_complex_t **fft_sp = cmask_aec2->fft_sp;
    wtk_complex_t *f_fft = cmask_aec2->f_fft;
    float **analysis_mem = cmask_aec2->analysis_mem,
          **analysis_mem_sp = cmask_aec2->analysis_mem_sp;
    float *analysis_window = cmask_aec2->analysis_window;
    float *out = cmask_aec2->out;
    short *pv = (short *)out;
    int ncmicchannel=cmask_aec2->cfg->ncmicchannel;
    int ncrefchannel=cmask_aec2->cfg->ncrefchannel;
    int ncchannel=cmask_aec2->cfg->ncchannel;
    wtk_strbuf_t **cmic=cmask_aec2->cmic;
    int *change_mic=cmask_aec2->cfg->change_mic;
    int *change_ref=cmask_aec2->cfg->change_ref;
    short fv;
    int t_mic_in_scale=cmask_aec2->cfg->t_mic_in_scale;
    int t_sp_in_scale=cmask_aec2->cfg->t_sp_in_scale;

    wtk_complex_t **ibf_fft=cmask_aec2->ibf_fft;
    int nibfchannel=cmask_aec2->cfg->nibfchannel;

    int tot_chn = nmicchannel + nspchannel;
    while (len > 0) {
        int nsample = len;
        int need_sample = fsize - cmask_aec2->pos;
        if (nsample >= need_sample) {
            for (i = 0; i < need_sample; i++, data += channel) {
                for (j = 0; j < nmicchannel; j++) {
                    cmask_aec2->audio_frame[j * fsize + cmask_aec2->pos + i] = data[cmask_aec2->cfg->chns[j]]*t_mic_in_scale;
                }
                for (j = nmicchannel; j < tot_chn; j++) {
                    cmask_aec2->audio_frame[j * fsize + cmask_aec2->pos + i] = data[cmask_aec2->cfg->chns[j]]*t_sp_in_scale;
                }
                if(cmic){
                    for(j=0;j<ncmicchannel;++j){
                        fv = data[change_mic[j]];
                        wtk_strbuf_push(cmic[j], (char *)&(fv), sizeof(short));
                    }
                    for(j=0;j<ncrefchannel;++j){
                        fv = data[change_ref[j]];
                        wtk_strbuf_push(cmic[j+ncmicchannel], (char *)&(fv), sizeof(short));
                    }
                }
            }

            spenr = nspchannel > 0 ? wtk_cmask_aec2_sp_energy(cmask_aec2->audio_frame + fsize * nmicchannel, fsize) : 0;
            if (spenr > spenr_thresh) {
                // if(cmask_aec2->sp_sil==1)
                // {
                // 	printf("sp start %f %f
                // %f\n", 1.0/16000*cnt*(nbin-1),spenr,spenr_thresh);
                // }
                cmask_aec2->sp_sil = 0;
                cmask_aec2->sp_silcnt = spenr_cnt;
            } else if (cmask_aec2->sp_sil == 0) {
                cmask_aec2->sp_silcnt -= 1;
                if (cmask_aec2->sp_silcnt <= 0) {
                    // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
                    cmask_aec2->sp_sil = 1;
                }
            }

            if(cmask_aec2->cfg->use_change_mic && cmask_aec2->sp_sil==1){
                wtk_cmask_aec2_feed_change_mic(cmask_aec2, cmic[0]->pos/sizeof(short));
            }
            if(cmask_aec2->cfg->use_change_mic2){
                wtk_cmask_aec2_feed_change_mic2(cmask_aec2, cmic[0]->pos/sizeof(short));
            }

            for (i = 0; i < nmicchannel; ++i) {
                wtk_drft_frame_analysis22(rfft, rfft_in, analysis_mem[i], fft[i],
                        cmask_aec2->audio_frame + i * fsize, wins,
                        analysis_window);
            }
            // float eq_mask[257]={1.000000, 1.000000, 1.258925, 1.584893, 1.995262, 2.511886, 2.973280, 2.644024, 2.256586, 1.873245, 1.529078, 1.314260, 1.184870, 1.103666, 1.049772, 1.015508, 1.004084, 1.027122, 1.100311, 1.233672, 1.417660, 1.618624, 1.789250, 1.876451, 1.840964, 1.694374, 1.488933, 1.272773, 1.078586, 0.927563, 0.826011, 0.767893, 0.742714, 0.740253, 0.751401, 0.768031, 0.783363, 0.792858, 0.794775, 0.789450, 0.777544, 0.758652, 0.731454, 0.695235, 0.651484, 0.604398, 0.559955, 0.524142, 0.501599, 0.495449, 0.507867, 0.540278, 0.592292, 0.659756, 0.734232, 0.805780, 0.867280, 0.916840, 0.956433, 0.987546, 1.008300, 1.015361, 1.008073, 0.990968, 0.972923, 0.963462, 0.969632, 0.995662, 1.042669, 1.105043, 1.166768, 1.205896, 1.206213, 1.165594, 1.096982, 1.021529, 0.956371, 0.908239, 0.877013, 0.861515, 0.862193, 0.880875, 0.918785, 0.973862, 1.039518, 1.106978, 1.170115, 1.228641, 1.286275, 1.343335, 1.389995, 1.409911, 1.391932, 1.338684, 1.267754, 1.204330, 1.169307, 1.174368, 1.226087, 1.330205, 1.489670, 1.695928, 1.919534, 2.111293, 2.220133, 2.221355, 2.134469, 2.009265, 1.893294, 1.812987, 1.774306, 1.770781, 1.792721, 1.835603, 1.906229, 2.026423, 2.234137, 2.482249, 2.734383, 2.963030, 3.123492, 3.162278, 3.092757, 2.941582, 2.744725, 2.531973, 2.319329, 2.155456, 2.034323, 1.924002, 1.806969, 1.681760, 1.560122, 1.459695, 1.396315, 1.381138, 1.420406, 1.509043, 1.617954, 1.696851, 1.701850, 1.618806, 1.468792, 1.297756, 1.146310, 1.029604, 0.945676, 0.889367, 0.857200, 0.847186, 0.857681, 0.886120, 0.927089, 0.970319, 1.001585, 1.008060, 0.984245, 0.934106, 0.868925, 0.801898, 0.742770, 0.696411, 0.664565, 0.647452, 0.643927, 0.650949, 0.663481, 0.675348, 0.680716, 0.675557, 0.658755, 0.632381, 0.600804, 0.569290, 0.542955, 0.526123, 0.521895, 0.531945, 0.556302, 0.592537, 0.634336, 0.671539, 0.693892, 0.696505, 0.682523, 0.661529, 0.644951, 0.641223, 0.653710, 0.681140, 0.718301, 0.756640, 0.786413, 0.800643, 0.798310, 0.784509, 0.767738, 0.756125, 0.754853, 0.765946, 0.789388, 0.823842, 0.866366, 0.911908, 0.953989, 0.987081, 1.009543, 1.025482, 1.044473, 1.079266, 1.143033, 1.247090, 1.397965, 1.593337, 1.820475, 2.060712, 2.297339, 2.522749, 2.732299, 2.900926, 3.032206, 3.121718, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278, 3.162278};
            if(cmask_aec2->cfg->use_in_eq && cmask_aec2->sp_sil){
                float *eq_gain = cmask_aec2->cfg->eq_gain;
                for(i=0;i<nbin;++i){
                    for(j=0;j<nmicchannel;++j){
                        fft[j][i].a *= eq_gain[i];
                        fft[j][i].b *= eq_gain[i];
                    }
                }
            }
            if(cmask_aec2->raw_fft){
                for(i=0;i<nmicchannel;++i){
                    memcpy(cmask_aec2->raw_fft[cmask_aec2->feed_frame][i], fft[i], sizeof(wtk_complex_t)*nbin);
                }
            }
            if(cmic){
                memcpy(f_fft, fft[cmask_aec2->change_idx], sizeof(wtk_complex_t)*nbin);
            }else{
                memcpy(f_fft, fft[0], sizeof(wtk_complex_t)*nbin);
            }
            for (i = nmicchannel; i < tot_chn; ++i) {
                wtk_drft_frame_analysis22(rfft, rfft_in, analysis_mem_sp[i - nmicchannel],
                        fft_sp[i - nmicchannel], cmask_aec2->audio_frame + i * fsize, wins,
                        analysis_window);
            }

            if(ibf_fft){
                // printf("%f\n", cmask_aec2->theta);
                if(cmask_aec2->sp_sil==0){
                    cmask_aec2->need_ibf=0;
                }else{
                    ++cmask_aec2->need_ibf;
                }
                if(cmask_aec2->need_ibf >= cmask_aec2->cfg->sp_ibf_delay){
                    // printf("%f ", cmask_aec2->theta);
                    for(i=0;i<nmicchannel;++i){
                        // printf("%d ", cmask_aec2->ibf_mic[i]);
                        // memcpy(ibf_fft[i], fft[ibf_mic[cmask_aec2->ibf_mic[i]]], sizeof(wtk_complex_t)*nbin);
                        memcpy(ibf_fft[i], fft[cmask_aec2->ibf_mic[i]], sizeof(wtk_complex_t)*nbin);
                    }
                    // printf("\n");
                    memset(f_fft, 0, sizeof(wtk_complex_t) * nbin);
                    for(i=0;i<nibfchannel;++i){
                        for(j=0;j<nbin;++j){
                            f_fft[j].a += ibf_fft[i][j].a;
                            f_fft[j].b += ibf_fft[i][j].b;
                        }
                    }
                    for(i=0;i<nbin;++i){
                        f_fft[i].a *= 1.0/nibfchannel;
                        f_fft[i].b *= 1.0/nibfchannel;
                    }
                    if(cmask_aec2->cfg->use_ibf_ssl || cmask_aec2->cfg->bf_theta!=-1){
                        wtk_cmask_aec2_feed_ibf(cmask_aec2, ibf_fft, f_fft);
                    }
                }
            }else if(cmask_aec2->ovec){
                wtk_complex_t **ovec=cmask_aec2->ovec;
                wtk_complex_t *ovec1;
                wtk_complex_t **infft;
                memset(f_fft, 0, sizeof(wtk_complex_t) * nbin);
                infft = cmask_aec2->raw_fft[0];
                for(int k=0;k<nbin;++k){
                    ovec1 = ovec[k];
                    for(i=0;i<nmicchannel;++i){
                        f_fft[k].a += ovec1[i].a * infft[i][k].a + ovec1[i].b * infft[i][k].b;
                        f_fft[k].b += ovec1[i].a * infft[i][k].b - ovec1[i].b * infft[i][k].a;
                    }
                }
            }
            if(cmask_aec2->cfg->use_freq_preemph){
                int pre_clip_s = cmask_aec2->cfg->pre_clip_s;
                int pre_clip_e = cmask_aec2->cfg->pre_clip_e;
                float pre_pow_ratio = cmask_aec2->cfg->pre_pow_ratio;
                float pre_mul_ratio = cmask_aec2->cfg->pre_mul_ratio;
                float alpha;
                for(i=pre_clip_s;i<pre_clip_e;++i){
                    alpha = (pre_mul_ratio - 1) * (pow(i-pre_clip_s, pre_pow_ratio))/pow((nbin-pre_clip_s), pre_pow_ratio) + 1;
                    f_fft[i].a *= alpha;
                    f_fft[i].b *= alpha;
                }
            }
            wtk_cmask_aec2_feed_edra(cmask_aec2, f_fft, fft_sp[0]);
            if(cmask_aec2->feed_frame==0){
                for (n = 0; n < cmask_aec2->cfg->num_frame; ++n) {
                    wtk_cmask_aec2_notify_data(cmask_aec2, &cmask_aec2->c_onnx_out[n * nbin], &cmask_aec2->c_onnx_err[n * nbin], \
                        &cmask_aec2->c_onnx_raw[n * nbin], &cmask_aec2->c_onnx_echo[n * nbin], &cmask_aec2->c_onnx_gf[n * nbin], n);
                }
            }
            if(cmic){
                wtk_strbufs_pop(cmic, ncchannel, need_sample*sizeof(short));
            }
            cmask_aec2->pos = 0;
            len -= need_sample;
        } else {
            for (i = 0; i < nsample; i++, data += channel) {
                for (j = 0; j < tot_chn; j++) {
                    cmask_aec2->audio_frame[j * fsize + cmask_aec2->pos + i] = data[cmask_aec2->cfg->chns[j]];
                }
            }
            cmask_aec2->pos += nsample;
            break;
        }
    }

    if (is_end && cmask_aec2->pos > 0) {
        if (cmask_aec2->notify) {
            pv = cmask_aec2->audio_frame;
            cmask_aec2->notify(cmask_aec2->ths, pv, cmask_aec2->pos);
        }
    }
}
