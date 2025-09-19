#include "wtk_pem_afc_b.h"

static float wtk_pem_afc_b_c_abs(wtk_complex_t fft)
{
    float f;
    f = sqrtf(fft.a * fft.a + fft.b * fft.b);
    return f;
}

static float wtk_pem_afc_b_c_angle(wtk_complex_t fft)
{
    float f;
    f = atan2f(fft.b, fft.a);
    return f;
}
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


void wtk_pem_afc_b_ncnn_new(wtk_pem_afc_b_t *pem_afc_b,
                            wtk_pem_afc_b_cfg_t *cfg) {
    int i, j, k;
    int pos = 0;
    in_out_name in_out[12] = {
        {"in6", "out2"},
        {"in7", "out3"},
        {"in8", "out4"},
        {"in9", "out5"},
        {"in10", "out6"},
        {"in11", "out7"},
        {"in12", "out8"},
        {"in13", "out9"},
        {"in14", "out10"},
        {"in15", "out11"},
        {"in16", "out12"},
        {"in17", "out13"}
    };
    pem_afc_b->rt = qtk_nnrt_new(&cfg->rt);
    pem_afc_b->heap = wtk_heap_new(4096);

    pem_afc_b->num_in = qtk_nnrt_get_num_in(pem_afc_b->rt);
    pem_afc_b->num_out = qtk_nnrt_get_num_out(pem_afc_b->rt);
    pem_afc_b->num_cache = pem_afc_b->num_in - cfg->num_in;
    // wtk_debug("num_in: %d, num_out: %d, num_cache: %d\n", pem_afc_b->num_in, pem_afc_b->num_out, pem_afc_b->num_cache);

    pem_afc_b->cache_value = wtk_calloc(sizeof(void *), pem_afc_b->num_cache);
    pem_afc_b->in_index = (int *)wtk_malloc(sizeof(int) * pem_afc_b->num_in);
    pem_afc_b->out_index = (int *)wtk_malloc(sizeof(int) * pem_afc_b->num_out);
    pem_afc_b->in_name =
        (char **)wtk_malloc(sizeof(char *) * pem_afc_b->num_in);
    pem_afc_b->out_name =
        (char **)wtk_malloc(sizeof(char *) * pem_afc_b->num_out);

    for (i = 0; i < pem_afc_b->num_in; ++i) {
        pem_afc_b->in_name[i] = qtk_nnrt_net_get_input_name(pem_afc_b->rt, i);
        pem_afc_b->in_index[i] = -1;
        // printf("in %d %s\n", i, pem_afc_b->in_name[i]);
    }
    for (i = 0; i < pem_afc_b->num_out; ++i) {
        pem_afc_b->out_name[i] = qtk_nnrt_net_get_output_name(pem_afc_b->rt, i);
        pem_afc_b->out_index[i] = -1;
        // printf("out %d %s\n", i, pem_afc_b->out_name[i]);
    }

    for (i = 0; i < pem_afc_b->num_in; ++i) {
        for (j = 0; j < 12; ++j) {
            if(strcmp(pem_afc_b->in_name[i],in_out[j].in)==0){
                for(k=0;k<pem_afc_b->num_out;++k){
                    if(strcmp(pem_afc_b->out_name[k],in_out[j].out)==0){
                        // wtk_debug("%d %d %d %s %s\n", i, k, pos, in_out[j].in, in_out[j].out);
                        pem_afc_b->in_index[i] = pos;
                        pem_afc_b->out_index[k] = pos;
                        pos++;
                        break;
                    }
                }
                break;
            }
        }
    }
}

void wtk_pem_afc_b_ncnn_delete(wtk_pem_afc_b_t *pem_afc_b) {
    int i;
    for (i = 0; i < pem_afc_b->num_cache; i++) {
        if (pem_afc_b->cache_value[i]) {
            qtk_nnrt_value_release(pem_afc_b->rt, pem_afc_b->cache_value[i]);
        }
    }
    // for (i = 0; i < pem_afc_b->num_in; ++i) {
    //     wtk_free(pem_afc_b->in_name[i]);
    // }
    // for (i = 0; i < pem_afc_b->num_out; ++i) {
    //     wtk_free(pem_afc_b->out_name[i]);
    // }
    wtk_free(pem_afc_b->in_name);
    wtk_free(pem_afc_b->out_name);

    wtk_free(pem_afc_b->cache_value);
    wtk_free(pem_afc_b->in_index);
    wtk_free(pem_afc_b->out_index);
    qtk_nnrt_delete(pem_afc_b->rt);
    wtk_heap_delete(pem_afc_b->heap);
}

wtk_pem_afc_b_t *wtk_pem_afc_b_new(wtk_pem_afc_b_cfg_t *cfg)
{
    wtk_pem_afc_b_t *pem_afc_b;
    
    pem_afc_b = (wtk_pem_afc_b_t *)wtk_malloc(sizeof(wtk_pem_afc_b_t));
    pem_afc_b->cfg = cfg;
    pem_afc_b->ths = NULL;
    pem_afc_b->notify = NULL;

    pem_afc_b->mic = wtk_strbuf_new(1024, 1);
    pem_afc_b->sp = wtk_strbuf_new(1024, 1);


    pem_afc_b->f1_L1 = cfg->hop_size;
    pem_afc_b->f1_L2 = cfg->N_afc;
    pem_afc_b->f2_L1 = cfg->N_afc;
    pem_afc_b->f2_L2 = cfg->N_afc;
    pem_afc_b->m_L1 = cfg->hop_size;
    pem_afc_b->m_L2 = cfg->N_ar;
    pem_afc_b->u_L1 = cfg->hop_size;
    pem_afc_b->u_L2 = cfg->N_ar;

    pem_afc_b->nbin = cfg->wins/2+1;
    pem_afc_b->fft1_len = pem_afc_b->f1_L1+pem_afc_b->f1_L2;
    pem_afc_b->fft2_len = pem_afc_b->f2_L1+pem_afc_b->f2_L2;
    pem_afc_b->fft3_len = pem_afc_b->m_L1+pem_afc_b->m_L2;
    pem_afc_b->fft4_len = cfg->hop_size*2;
    pem_afc_b->fft5_len = cfg->hop_size*2;
    pem_afc_b->pffft1 = pffft_new_setup(pem_afc_b->fft1_len, PFFFT_REAL);
    pem_afc_b->pffft2 = pffft_new_setup(pem_afc_b->fft2_len, PFFFT_REAL);
    // pem_afc_b->pffft3 = pffft_new_setup(pem_afc_b->fft3_len, PFFFT_REAL);
    pem_afc_b->drft3 = wtk_drft_new(pem_afc_b->fft3_len);
    pem_afc_b->pffft4 = pffft_new_setup(pem_afc_b->fft4_len, PFFFT_REAL);
    pem_afc_b->pffft5 = pffft_new_setup(pem_afc_b->fft5_len, PFFFT_REAL);
    pem_afc_b->analysis_window = (float *)wtk_malloc(pem_afc_b->fft5_len*sizeof(float));
    pem_afc_b->synthesis_window = (float *)wtk_malloc(pem_afc_b->fft5_len*sizeof(float));
    pem_afc_b->analysis_mem = (float *)wtk_malloc(pem_afc_b->fft5_len/2*sizeof(float));
    pem_afc_b->synthesis_mem = (float *)wtk_malloc(pem_afc_b->fft5_len/2*sizeof(float));

    pem_afc_b->ar_a = (float *)wtk_malloc(cfg->N_ar*cfg->N_ar*sizeof(float));
    pem_afc_b->ar_R = (float *)wtk_malloc(cfg->hop_size*sizeof(float));
    pem_afc_b->ar_rou = (float *)wtk_malloc(cfg->N_ar*sizeof(float));
    pem_afc_b->ar_w = (float *)wtk_malloc(cfg->N_ar*sizeof(float));
    pem_afc_b->afc_w = (float *)wtk_malloc(cfg->N_afc*sizeof(float));

    pem_afc_b->filtering_conv1_w_len = pem_afc_b->f1_L1+pem_afc_b->f1_L2+2;
    pem_afc_b->filtering_conv2_w_len = pem_afc_b->f2_L1+pem_afc_b->f2_L2+2;
    pem_afc_b->whiten_m_conv_w_len = pem_afc_b->m_L1+pem_afc_b->m_L2+2;
    pem_afc_b->whiten_u_conv_w_len = pem_afc_b->u_L1+pem_afc_b->u_L2+2;
    pem_afc_b->filtering_conv1_w = (float *)wtk_malloc(pem_afc_b->filtering_conv1_w_len*sizeof(float));
    pem_afc_b->filtering_conv2_w = (float *)wtk_malloc(pem_afc_b->filtering_conv2_w_len*sizeof(float));
    pem_afc_b->whiten_m_conv_w = (float *)wtk_malloc(pem_afc_b->whiten_m_conv_w_len*sizeof(float));
    pem_afc_b->whiten_u_conv_w = (float *)wtk_malloc(pem_afc_b->whiten_u_conv_w_len*sizeof(float));

    pem_afc_b->filtering_conv1_cache = (float *)wtk_malloc(cfg->N_afc*sizeof(float));
    pem_afc_b->filtering_conv2_cache = (float *)wtk_malloc(cfg->N_afc*sizeof(float));
    pem_afc_b->whiten_m_conv_cache = (float *)wtk_malloc(cfg->N_ar*sizeof(float));
    pem_afc_b->whiten_u_conv_cache = (float *)wtk_malloc(cfg->N_ar*sizeof(float));

    pem_afc_b->T_buffer_size = cfg->N_afc;
    pem_afc_b->u_buffer_size = cfg->N_afc*2;
    pem_afc_b->m_buffer_size = cfg->N_afc;
    pem_afc_b->T_buffer_len = pem_afc_b->T_buffer_size+cfg->hop_size;
    pem_afc_b->u_buffer_len = pem_afc_b->u_buffer_size+cfg->hop_size;
    pem_afc_b->m_buffer_len = pem_afc_b->m_buffer_size+cfg->hop_size;
    pem_afc_b->Truncation_indicator_buffer = (float *)wtk_malloc(pem_afc_b->T_buffer_len*sizeof(float));
    pem_afc_b->u_whiten_buffer = (float *)wtk_malloc(pem_afc_b->u_buffer_len*sizeof(float));
    pem_afc_b->m_whiten_buffer = (float *)wtk_malloc(pem_afc_b->m_buffer_len*sizeof(float));

    pem_afc_b->prev_m = (float *)wtk_malloc(cfg->hop_size*sizeof(float));
    pem_afc_b->prev_u = (float *)wtk_malloc(cfg->hop_size*sizeof(float));

    pem_afc_b->power = (float *)wtk_malloc((cfg->N_afc+1)*sizeof(float));

    pem_afc_b->error = (float *)wtk_malloc(cfg->hop_size*sizeof(float));
    pem_afc_b->error_soft_clipped = (float *)wtk_malloc(cfg->hop_size*sizeof(float));
    pem_afc_b->Truncation_indicator = (float *)wtk_malloc(cfg->hop_size*sizeof(float));

    pem_afc_b->m_whiten = (float *)wtk_malloc(cfg->hop_size*sizeof(float));
    pem_afc_b->u_whiten = (float *)wtk_malloc(cfg->hop_size*sizeof(float));

    pem_afc_b->Yk = (float *)wtk_malloc(cfg->N_afc*sizeof(float));
    pem_afc_b->Xk = (float *)wtk_malloc((cfg->N_afc+1)*2*sizeof(float));

    pem_afc_b->out = (float *)wtk_malloc(cfg->hop_size*sizeof(float));

    pem_afc_b->fft_tmp = (float *)wtk_malloc((cfg->N_afc*2+cfg->hop_size*2+cfg->N_ar*2)*sizeof(float));
    pem_afc_b->ifft_tmp = (float *)wtk_malloc((cfg->N_afc*2+cfg->hop_size*2+cfg->N_ar*2)*sizeof(float));
    pem_afc_b->tmp = (float *)wtk_malloc((cfg->N_afc*2+cfg->hop_size*2+cfg->N_ar*2)*sizeof(float));

	pem_afc_b->eq=NULL;
	if(cfg->use_eq)
	{
		pem_afc_b->eq=wtk_equalizer_new(&(cfg->eq));
	}

    pem_afc_b->pem=NULL;
    pem_afc_b->drft6=NULL;
    pem_afc_b->analysis_mem_mic=NULL;
    pem_afc_b->analysis_mem_sp=NULL;
    pem_afc_b->analysis_mem_pem=NULL;
    pem_afc_b->synthesis_mem2=NULL;
    pem_afc_b->mic_fft=NULL;
    pem_afc_b->sp_fft=NULL;
    pem_afc_b->pem_fft=NULL;
    pem_afc_b->fftx=NULL;

    pem_afc_b->mag_Y=NULL;
    pem_afc_b->pha_Y=NULL;
    pem_afc_b->mag_R1=NULL;
    pem_afc_b->pha_R1=NULL;
    pem_afc_b->mag_R2=NULL;
    pem_afc_b->pha_R2=NULL;
    if(cfg->use_onnx || cfg->use_ncnn){
        pem_afc_b->pem = wtk_strbuf_new(1024, 1);
        pem_afc_b->drft6 = wtk_drft_new2(cfg->wins);
        pem_afc_b->analysis_mem_mic = (float *)wtk_malloc(cfg->hop_size*sizeof(float));
        pem_afc_b->analysis_mem_sp = (float *)wtk_malloc(cfg->hop_size*sizeof(float));
        pem_afc_b->analysis_mem_pem = (float *)wtk_malloc(cfg->hop_size*sizeof(float));
        pem_afc_b->synthesis_mem2 = (float *)wtk_malloc(cfg->hop_size*sizeof(float));
        
        pem_afc_b->mic_fft = (wtk_complex_t *)wtk_malloc(pem_afc_b->nbin*sizeof(wtk_complex_t));
        pem_afc_b->sp_fft = (wtk_complex_t *)wtk_malloc(pem_afc_b->nbin*sizeof(wtk_complex_t));
        pem_afc_b->pem_fft = (wtk_complex_t *)wtk_malloc(pem_afc_b->nbin*sizeof(wtk_complex_t));
        pem_afc_b->fftx = (wtk_complex_t *)wtk_malloc(pem_afc_b->nbin*sizeof(wtk_complex_t));

        pem_afc_b->mag_Y = (float *)wtk_malloc(pem_afc_b->nbin*sizeof(float));
        pem_afc_b->pha_Y = (float *)wtk_malloc(pem_afc_b->nbin*sizeof(float));
        pem_afc_b->mag_R1 = (float *)wtk_malloc(pem_afc_b->nbin*sizeof(float));
        pem_afc_b->pha_R1 = (float *)wtk_malloc(pem_afc_b->nbin*sizeof(float));
        pem_afc_b->mag_R2 = (float *)wtk_malloc(pem_afc_b->nbin*sizeof(float));
        pem_afc_b->pha_R2 = (float *)wtk_malloc(pem_afc_b->nbin*sizeof(float));
    }
    if(cfg->use_onnx){
#ifdef ONNX_DEC
        pem_afc_b->onnx = qtk_onnxruntime_new(&(cfg->onnx));
        pem_afc_b->cache = wtk_calloc(sizeof(OrtValue *), pem_afc_b->onnx->num_in - cfg->onnx.outer_in_num);
        if (pem_afc_b->onnx->num_in - cfg->onnx.outer_in_num != pem_afc_b->onnx->num_out - cfg->onnx.outer_out_num) {
            wtk_debug("err inner_item\n");
            exit(0);
        }
#endif
    }else if(cfg->use_ncnn){
        wtk_pem_afc_b_ncnn_new(pem_afc_b, cfg);
    }

    pem_afc_b->sweep_ok = 0;
    pem_afc_b->recommend_delay = 0;
    pem_afc_b->r_est = 0;
    if(cfg->use_refcompensation){
        pem_afc_b->r_est = qtk_rir_estimate_new(&cfg->r_est,cfg->hop_size,cfg->rate);
        if(cfg->r_est.ref_fn != NULL){
            pem_afc_b->sweep_ok = 1;
        }
    }
    wtk_pem_afc_b_reset(pem_afc_b);
    return pem_afc_b;
}
void wtk_pem_afc_b_delete(wtk_pem_afc_b_t *pem_afc_b)
{
    wtk_strbuf_delete(pem_afc_b->mic);
    wtk_strbuf_delete(pem_afc_b->sp);

    pffft_destroy_setup(pem_afc_b->pffft1);
    pffft_destroy_setup(pem_afc_b->pffft2);
    wtk_drft_delete(pem_afc_b->drft3);
    // pffft_destroy_setup(pem_afc_b->pffft3);
    pffft_destroy_setup(pem_afc_b->pffft4);
    pffft_destroy_setup(pem_afc_b->pffft5);

    wtk_free(pem_afc_b->analysis_window);
    wtk_free(pem_afc_b->synthesis_window);
    wtk_free(pem_afc_b->analysis_mem);
    wtk_free(pem_afc_b->synthesis_mem);

    wtk_free(pem_afc_b->ar_a);
    wtk_free(pem_afc_b->ar_R);
    wtk_free(pem_afc_b->ar_rou);
    wtk_free(pem_afc_b->ar_w);

    wtk_free(pem_afc_b->afc_w);

    wtk_free(pem_afc_b->filtering_conv1_w);
    wtk_free(pem_afc_b->filtering_conv2_w);
    wtk_free(pem_afc_b->whiten_m_conv_w);
    wtk_free(pem_afc_b->whiten_u_conv_w);

    wtk_free(pem_afc_b->filtering_conv1_cache);
    wtk_free(pem_afc_b->filtering_conv2_cache);
    wtk_free(pem_afc_b->whiten_m_conv_cache);
    wtk_free(pem_afc_b->whiten_u_conv_cache);

    wtk_free(pem_afc_b->Truncation_indicator_buffer);
    wtk_free(pem_afc_b->u_whiten_buffer);
    wtk_free(pem_afc_b->m_whiten_buffer);

    wtk_free(pem_afc_b->prev_m);
    wtk_free(pem_afc_b->prev_u);

    wtk_free(pem_afc_b->power);

    wtk_free(pem_afc_b->error);
    wtk_free(pem_afc_b->error_soft_clipped);
    wtk_free(pem_afc_b->Truncation_indicator);

    wtk_free(pem_afc_b->m_whiten);
    wtk_free(pem_afc_b->u_whiten);

    wtk_free(pem_afc_b->Yk);
    wtk_free(pem_afc_b->Xk);

    wtk_free(pem_afc_b->out);

    wtk_free(pem_afc_b->fft_tmp);
    wtk_free(pem_afc_b->ifft_tmp);
    wtk_free(pem_afc_b->tmp);

	if(pem_afc_b->eq)
	{
		wtk_equalizer_delete(pem_afc_b->eq);
	}

    if(pem_afc_b->pem){
        wtk_strbuf_delete(pem_afc_b->pem);
        wtk_drft_delete2(pem_afc_b->drft6);
        wtk_free(pem_afc_b->analysis_mem_mic);
        wtk_free(pem_afc_b->analysis_mem_sp);
        wtk_free(pem_afc_b->analysis_mem_pem);
        wtk_free(pem_afc_b->synthesis_mem2);
        wtk_free(pem_afc_b->mic_fft);
        wtk_free(pem_afc_b->sp_fft);
        wtk_free(pem_afc_b->pem_fft);
        wtk_free(pem_afc_b->fftx);
        wtk_free(pem_afc_b->mag_Y);
        wtk_free(pem_afc_b->pha_Y);
        wtk_free(pem_afc_b->mag_R1);
        wtk_free(pem_afc_b->pha_R1);
        wtk_free(pem_afc_b->mag_R2);
        wtk_free(pem_afc_b->pha_R2);
    }

    if(pem_afc_b->cfg->use_onnx){
#ifdef ONNX_DEC
        {
            int n = pem_afc_b->onnx->num_in - pem_afc_b->onnx->cfg->outer_in_num;
            if (pem_afc_b->cache[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    pem_afc_b->onnx->api->ReleaseValue(pem_afc_b->cache[i]);
                }
            }
        }
        if (pem_afc_b->onnx) {
            qtk_onnxruntime_delete(pem_afc_b->onnx);
        }
        wtk_free(pem_afc_b->cache);
#endif
    }else if(pem_afc_b->cfg->use_ncnn){
        wtk_pem_afc_b_ncnn_delete(pem_afc_b);
    }

    if(pem_afc_b->r_est){
        qtk_rir_estimate_delete(pem_afc_b->r_est);
    }
    wtk_free(pem_afc_b);
}
void wtk_pem_afc_b_start(wtk_pem_afc_b_t *pem_afc_b)
{

}
void wtk_pem_afc_b_reset(wtk_pem_afc_b_t *pem_afc_b)
{
    wtk_strbuf_reset(pem_afc_b->mic);
    wtk_strbuf_reset(pem_afc_b->sp);
    wtk_strbuf_push(pem_afc_b->sp, NULL, sizeof(float)*pem_afc_b->cfg->Nframe_DAC_delay);

    for (int i=0;i<pem_afc_b->fft5_len;++i)
	{
		pem_afc_b->analysis_window[i] = sin((0.5+i)*PI/(pem_afc_b->fft5_len));
	}
	wtk_drft_init_synthesis_window(pem_afc_b->synthesis_window, pem_afc_b->analysis_window, pem_afc_b->fft5_len);
    memset(pem_afc_b->analysis_mem, 0, pem_afc_b->fft5_len/2*sizeof(float));
    memset(pem_afc_b->synthesis_mem, 0, pem_afc_b->fft5_len/2*sizeof(float));

    memset(pem_afc_b->ar_a, 0, pem_afc_b->cfg->N_ar*pem_afc_b->cfg->N_ar*sizeof(float));
    memset(pem_afc_b->ar_R, 0, pem_afc_b->cfg->hop_size*sizeof(float));
    memset(pem_afc_b->ar_rou, 0, pem_afc_b->cfg->N_ar*sizeof(float));
    memset(pem_afc_b->ar_w, 0, pem_afc_b->cfg->N_ar*sizeof(float));

    memset(pem_afc_b->afc_w, 0, pem_afc_b->cfg->N_afc*sizeof(float));

    memset(pem_afc_b->filtering_conv1_w, 0, pem_afc_b->filtering_conv1_w_len*sizeof(float));
    memset(pem_afc_b->filtering_conv2_w, 0, pem_afc_b->filtering_conv2_w_len*sizeof(float));
    memset(pem_afc_b->whiten_m_conv_w, 0, pem_afc_b->whiten_m_conv_w_len*sizeof(float));
    memset(pem_afc_b->whiten_u_conv_w, 0, pem_afc_b->whiten_u_conv_w_len*sizeof(float));

    memset(pem_afc_b->filtering_conv1_cache, 0, pem_afc_b->cfg->N_afc*sizeof(float));
    memset(pem_afc_b->filtering_conv2_cache, 0, pem_afc_b->cfg->N_afc*sizeof(float));
    memset(pem_afc_b->whiten_m_conv_cache, 0, pem_afc_b->cfg->N_ar*sizeof(float));
    memset(pem_afc_b->whiten_u_conv_cache, 0, pem_afc_b->cfg->N_ar*sizeof(float));

    for(int i=0;i<pem_afc_b->T_buffer_len;++i){
        pem_afc_b->Truncation_indicator_buffer[i] = 1.0;
    }
    memset(pem_afc_b->u_whiten_buffer, 0, pem_afc_b->u_buffer_len*sizeof(float));
    memset(pem_afc_b->m_whiten_buffer, 0, pem_afc_b->m_buffer_len*sizeof(float));

    memset(pem_afc_b->prev_m, 0, pem_afc_b->cfg->hop_size*sizeof(float));
    memset(pem_afc_b->prev_u, 0, pem_afc_b->cfg->hop_size*sizeof(float));

    for(int i=0;i<pem_afc_b->cfg->N_afc+1;++i){
        pem_afc_b->power[i] = pem_afc_b->cfg->delta;
    }

    memset(pem_afc_b->error, 0, pem_afc_b->cfg->hop_size*sizeof(float));
    memset(pem_afc_b->error_soft_clipped, 0, pem_afc_b->cfg->hop_size*sizeof(float));
    memset(pem_afc_b->Truncation_indicator, 0, pem_afc_b->cfg->hop_size*sizeof(float));

    memset(pem_afc_b->m_whiten, 0, pem_afc_b->cfg->hop_size*sizeof(float));
    memset(pem_afc_b->u_whiten, 0, pem_afc_b->cfg->hop_size*sizeof(float));

    memset(pem_afc_b->Yk, 0, pem_afc_b->cfg->N_afc*sizeof(float));
    memset(pem_afc_b->Xk, 0, (pem_afc_b->cfg->N_afc+1)*2*sizeof(float));

    memset(pem_afc_b->out, 0, pem_afc_b->cfg->hop_size*sizeof(float));

    memset(pem_afc_b->fft_tmp, 0, (pem_afc_b->cfg->N_afc*2+pem_afc_b->cfg->hop_size*2+pem_afc_b->cfg->N_ar*2)*sizeof(float));
    memset(pem_afc_b->ifft_tmp, 0, (pem_afc_b->cfg->N_afc*2+pem_afc_b->cfg->hop_size*2+pem_afc_b->cfg->N_ar*2)*sizeof(float));
    memset(pem_afc_b->tmp, 0, (pem_afc_b->cfg->N_afc*2+pem_afc_b->cfg->hop_size*2+pem_afc_b->cfg->N_ar*2)*sizeof(float));

    if(pem_afc_b->pem){
        wtk_strbuf_reset(pem_afc_b->pem);
        memset(pem_afc_b->analysis_mem_mic, 0, pem_afc_b->cfg->hop_size*sizeof(float));
        memset(pem_afc_b->analysis_mem_sp, 0, pem_afc_b->cfg->hop_size*sizeof(float));
        memset(pem_afc_b->analysis_mem_pem, 0, pem_afc_b->cfg->hop_size*sizeof(float));
        memset(pem_afc_b->synthesis_mem2, 0, pem_afc_b->cfg->hop_size*sizeof(float));
        memset(pem_afc_b->mic_fft, 0, pem_afc_b->nbin*sizeof(wtk_complex_t));
        memset(pem_afc_b->sp_fft, 0, pem_afc_b->nbin*sizeof(wtk_complex_t));
        memset(pem_afc_b->pem_fft, 0, pem_afc_b->nbin*sizeof(wtk_complex_t));
        memset(pem_afc_b->fftx, 0, pem_afc_b->nbin*sizeof(float));
        memset(pem_afc_b->mag_Y, 0, pem_afc_b->nbin*sizeof(float));
        memset(pem_afc_b->pha_Y, 0, pem_afc_b->nbin*sizeof(float));
        memset(pem_afc_b->mag_R1, 0, pem_afc_b->nbin*sizeof(float));
        memset(pem_afc_b->pha_R1, 0, pem_afc_b->nbin*sizeof(float));
        memset(pem_afc_b->mag_R2, 0, pem_afc_b->nbin*sizeof(float));
        memset(pem_afc_b->pha_R2, 0, pem_afc_b->nbin*sizeof(float));
    }

    if(pem_afc_b->cfg->use_onnx){
#ifdef ONNX_DEC
        qtk_onnxruntime_reset(pem_afc_b->onnx);
        {
            int n = pem_afc_b->onnx->num_in - pem_afc_b->onnx->cfg->outer_in_num;
            if (pem_afc_b->cache[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    pem_afc_b->onnx->api->ReleaseValue(pem_afc_b->cache[i]);
                }
                memset(pem_afc_b->cache, 0, sizeof(OrtValue *) * n);
            }
        }
#endif
    }
    pem_afc_b->nframe = 0;
    pem_afc_b->ncnn_nframe = 0;
}
void wtk_pem_afc_b_set_notify(wtk_pem_afc_b_t *pem_afc_b,void *ths,wtk_pem_afc_b_notify_f notify)
{
    pem_afc_b->ths = ths;
    pem_afc_b->notify = notify;
}

void _tanh(float *f, int len)
{
    float *p;
	float *e;
	float inv_expx,expx;
	p=f;e=p+len;
	while(p<e)
	{
		if(*p>0.0)
		{
			inv_expx=expf(-(*p));
			*p=-1.0+2.0/(1.0+inv_expx*inv_expx);
		}else
		{
			expx=expf(*p);
			*p=1.0-2.0/(1.0+expx*expx);
		}
		++p;
	}
}

void wtk_pem_afc_b_levinson(wtk_pem_afc_b_t *pem_afc_b)
{
    float delta = pem_afc_b->cfg->delta;
    float *ar_w = pem_afc_b->ar_w;
    int N_ar=pem_afc_b->cfg->N_ar;
    float *R=pem_afc_b->ar_R;
    float *a=pem_afc_b->ar_a;
    float *rou=pem_afc_b->ar_rou;
    float sum;
    int i, j;

    for(i=0;i<N_ar;++i){
        for(j=0;j<N_ar;++j){
            a[i*N_ar+j] = 1;
        }
        rou[i] = 0;
    }
    rou[0] = R[0];
    a[N_ar+1] = -R[1] / rou[0];
    rou[1] = rou[0] * (1.0 - a[N_ar+1] * a[N_ar+1]);

    for(i=2;i<N_ar;++i){
        sum = 0;
        for(j=1;j<i;++j){
            sum += a[(i-1)*N_ar+j] * R[i-j];
        }
        a[i*N_ar+i] = -(R[i]+sum)/(rou[i-1]+delta);
        for(j=1;j<i;++j){
            a[i*N_ar+j] = a[(i-1)*N_ar+j] + a[i*N_ar+i] * a[(i - 1)*N_ar+i - j];
        }
        rou[i] = rou[i - 1] * (1.0 - a[i*N_ar+i] * a[i*N_ar+i]);
    }
    // print_float2(a, N_ar*N_ar);

    for(i=0;i<N_ar;++i){
        ar_w[i] = a[(N_ar-1)*N_ar+i];
    }

}

void wtk_pem_afc_b_reset_weight(wtk_pem_afc_b_t *pem_afc_b, PFFFT_Setup *pffft, wtk_drft_t *drft, int fft_len, float *data, int len, float *out, int out_len)
{
    float *fft_tmp = pem_afc_b->fft_tmp;
    float *ifft_tmp = pem_afc_b->ifft_tmp;
    float *tmp = pem_afc_b->tmp;

    memset(fft_tmp, 0, (out_len-2)*sizeof(float));
    if(data){
        memcpy(fft_tmp, data, len*sizeof(float));
    }
    if(pffft){
        pffft_transform_ordered(pffft, fft_tmp, ifft_tmp, tmp, PFFFT_FORWARD);
        ifft_tmp[fft_len] = ifft_tmp[1];
        ifft_tmp[1] = 0;
        // ifft_tmp[fft_len+1] = 0;  ////
        memcpy(out, ifft_tmp, out_len*sizeof(float));
    }else if(drft){
        wtk_drft_fft(drft, fft_tmp);
        out[0] = fft_tmp[0];
        out[1] = 0;
        out[out_len-2] = fft_tmp[fft_len-1];
        out[out_len-1] = 0;
        memcpy(out+2, fft_tmp+1, (out_len-4)*sizeof(float));
    }
}

void wtk_pem_afc_b_conv_call(wtk_pem_afc_b_t *pem_afc_b, PFFFT_Setup *pffft, wtk_drft_t *drft, int fft_len, float *data, int len, float *cache, int cache_len, float *weight, int w_len, float *out, int out_len)
{
    float *fft_tmp = pem_afc_b->fft_tmp;
    float *ifft_tmp = pem_afc_b->ifft_tmp;
    float *tmp = pem_afc_b->tmp;
    int i;

    memcpy(fft_tmp, cache, cache_len*sizeof(float));
    memcpy(fft_tmp+cache_len, data, len*sizeof(float));
    memcpy(cache, fft_tmp+len, cache_len*sizeof(float));
    if(pffft){
        pffft_transform_ordered(pffft, fft_tmp, ifft_tmp, tmp, PFFFT_FORWARD);
        ifft_tmp[fft_len] = ifft_tmp[1];
        ifft_tmp[1] = 0;
        // ifft_tmp[fft_len+1] = 0;  ////
        // print_float2(ifft_tmp, w_len);
        for(i=0;i<fft_len/2+1;++i){
            fft_tmp[2*i] = ifft_tmp[2*i] * weight[2*i] - ifft_tmp[2*i+1] * weight[2*i+1];
            fft_tmp[2*i+1] = ifft_tmp[2*i] * weight[2*i+1] + ifft_tmp[2*i+1] * weight[2*i];
        }
        // print_float2(fft_tmp, w_len);
        fft_tmp[1] = fft_tmp[fft_len];
        pffft_transform_ordered(pffft, fft_tmp, ifft_tmp, tmp, PFFFT_BACKWARD);
        for(i=0;i<out_len;++i){
            out[i] = ifft_tmp[i+cache_len] * 1.0/fft_len;
        }
        // print_float2(out, out_len);
    }else if(drft){
        wtk_drft_fft(drft, fft_tmp);
        ifft_tmp[0] = fft_tmp[0];
        ifft_tmp[1] = 0;
        ifft_tmp[fft_len] = fft_tmp[fft_len-1];
        ifft_tmp[fft_len+1] = 0;
        memcpy(ifft_tmp+2, fft_tmp+1, (fft_len-2)*sizeof(float));
        for(i=0;i<fft_len/2+1;++i){
            fft_tmp[2*i] = ifft_tmp[2*i] * weight[2*i] - ifft_tmp[2*i+1] * weight[2*i+1];
            fft_tmp[2*i+1] = ifft_tmp[2*i] * weight[2*i+1] + ifft_tmp[2*i+1] * weight[2*i];
        }
        // print_float2(ifft_tmp, w_len);
        // print_float2(weight, w_len);
        // print_float2(fft_tmp, fft_len+2);
        ifft_tmp[0] = fft_tmp[0];
        ifft_tmp[fft_len-1] = fft_tmp[fft_len];
        memcpy(ifft_tmp+1, fft_tmp+2, (fft_len-2)*sizeof(float));
        // print_float2(ifft_tmp, fft_len+2);
        wtk_drft_ifft(drft, ifft_tmp);
        for(i=0;i<out_len;++i){
            out[i] = ifft_tmp[i+cache_len] * 1.0/fft_len;
        }
        // print_float2(out, out_len);
    }
}

void wtk_pem_afc_b_buffer_stack_in(float *buffer, float *data, int len, int buffer_size)
{
    memcpy(buffer, buffer+len, buffer_size*sizeof(float));
    memcpy(buffer+buffer_size, data, len*sizeof(float));
}

float *wtk_pem_afc_b_get_buffer(float *buffer, int buffer_len, int buffer_size)
{
    return buffer+buffer_len-buffer_size;
}

void wtk_pem_afc_b_ccm(wtk_pem_afc_b_t *pem_afc_b, float *mag, float *pha, wtk_complex_t *fftx)
{
    int nbin = pem_afc_b->nbin;
    float compress_factor = pem_afc_b->cfg->compress_factor;
    int i;

    // for(i=0;i<nbin;++i){
    //     printf("%f %f\n", mag[i], pha[i]);
    // }
    // exit(0);

    for(i=0;i<nbin;++i){
        mag[i] = powf(mag[i], 1.0/compress_factor);
        fftx[i].a = mag[i] * cosf(pha[i]);
        fftx[i].b = mag[i] * sinf(pha[i]);
    }
}

void wtk_pem_afc_b_feed_onnx(wtk_pem_afc_b_t *pem_afc_b, wtk_complex_t *fftx)
{
    int i, j;
    float *mag_Y = pem_afc_b->mag_Y;
    float *pha_Y = pem_afc_b->pha_Y;
    float *mag_R1 = pem_afc_b->mag_R1;
    float *pha_R1 = pem_afc_b->pha_R1;
    float *mag_R2 = pem_afc_b->mag_R2;
    float *pha_R2 = pem_afc_b->pha_R2;
    float *mag;
    float *pha;

#ifdef ONNX_DEC
    const OrtApi *api = pem_afc_b->onnx->api;
    OrtMemoryInfo *meminfo = pem_afc_b->onnx->meminfo;
    qtk_onnxruntime_t *onnx = pem_afc_b->onnx;
    OrtStatus *status;
    int num_in = onnx->num_in;
    int outer_in_num = onnx->cfg->outer_in_num;
    int outer_out_num = onnx->cfg->outer_out_num;
    qtk_onnx_item_t *item;
    // int64_t size = 0, *out_shape;

    item = onnx->in_items + 0;
    status = api->CreateTensorWithDataAsOrtValue(
        meminfo, mag_Y, item->bytes * item->in_dim, item->shape,
        item->shape_len, item->type, onnx->in + 0);

    item = onnx->in_items + 1;
    status = api->CreateTensorWithDataAsOrtValue(
        meminfo, pha_Y, item->bytes * item->in_dim, item->shape,
        item->shape_len, item->type, onnx->in + 1);

    item = onnx->in_items + 2;
    status = api->CreateTensorWithDataAsOrtValue(
        meminfo, mag_R1, item->bytes * item->in_dim, item->shape,
        item->shape_len, item->type, onnx->in + 2);

    item = onnx->in_items + 3;
    status = api->CreateTensorWithDataAsOrtValue(
        meminfo, pha_R1, item->bytes * item->in_dim, item->shape,
        item->shape_len, item->type, onnx->in + 3);

    item = onnx->in_items + 4;
    status = api->CreateTensorWithDataAsOrtValue(
        meminfo, mag_R2, item->bytes * item->in_dim, item->shape,
        item->shape_len, item->type, onnx->in + 4);

    item = onnx->in_items + 5;
    status = api->CreateTensorWithDataAsOrtValue(
        meminfo, pha_R2, item->bytes * item->in_dim, item->shape,
        item->shape_len, item->type, onnx->in + 5);

    // printf("num_in:\n");
    if (pem_afc_b->cache[0] == NULL) {
        for (i = pem_afc_b->cfg->onnx.outer_in_num; i < num_in; ++i) {
            item = onnx->in_items + i;
            status = api->CreateTensorWithDataAsOrtValue(
                    meminfo, item->val, item->bytes * item->in_dim, item->shape,
                    item->shape_len, item->type, onnx->in + i);
        }
    } else {
        for (i = pem_afc_b->cfg->onnx.outer_in_num; i < num_in; ++i) {
            onnx->in[i] = pem_afc_b->cache[i - pem_afc_b->cfg->onnx.outer_in_num];
        }
    }

    status = api->Run(onnx->session, NULL,
                        cast(const char *const *, onnx->name_in),
                        cast(const OrtValue *const *, onnx->in), onnx->num_in,
                        cast(const char *const *, onnx->name_out),
                        onnx->num_out, onnx->out);

    // wtk_debug("run success\n");
    // out_shape = qtk_onnxruntime_get_outshape(onnx, 0, &size);
    mag = qtk_onnxruntime_getout(onnx, 0);
    pha = qtk_onnxruntime_getout(onnx, 1);
    wtk_pem_afc_b_ccm(pem_afc_b, mag, pha, fftx);

    for (i = outer_in_num, j = outer_out_num; i < num_in; ++i, ++j) {
        pem_afc_b->cache[i - outer_in_num] = onnx->out[j];
        onnx->out[j] = NULL;
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
#endif
}


void wtk_pem_afc_b_feed_ncnn(wtk_pem_afc_b_t *pem_afc_b, wtk_complex_t *fftx)
{
    int num_in = pem_afc_b->num_in;
    int num_in2 = pem_afc_b->cfg->num_in;
    int num_out = pem_afc_b->num_out;
    qtk_nnrt_value_t input_val[32];
    qtk_nnrt_value_t output_val[32];
    int64_t shape[32][5];
    char **in_name = pem_afc_b->in_name;
    char **out_name = pem_afc_b->out_name;
    int *in_index = pem_afc_b->in_index;
    int *out_index = pem_afc_b->out_index;
    int i, j;
    int shape_len;
    float *mag_Y = pem_afc_b->mag_Y;
    float *pha_Y = pem_afc_b->pha_Y;
    float *mag_R1 = pem_afc_b->mag_R1;
    float *pha_R1 = pem_afc_b->pha_R1;
    float *mag_R2 = pem_afc_b->mag_R2;
    float *pha_R2 = pem_afc_b->pha_R2;
    float *mag;
    float *pha;

    // ASSIGN_SHAPE(shape[0], 1, 1, 129, 0);
    // ASSIGN_SHAPE(shape[1], 1, 1, 129, 0);
    // ASSIGN_SHAPE(shape[2], 1, 1, 129, 0);
    // ASSIGN_SHAPE(shape[3], 1, 1, 129, 0);
    // ASSIGN_SHAPE(shape[4], 1, 1, 129, 0);
    // ASSIGN_SHAPE(shape[5], 1, 1, 129, 0);

    ASSIGN_SHAPE(shape[0], 1, 129, 1, 0);
    ASSIGN_SHAPE(shape[1], 1, 129, 1, 0);
    ASSIGN_SHAPE(shape[2], 1, 129, 1, 0);
    ASSIGN_SHAPE(shape[3], 1, 129, 1, 0);
    ASSIGN_SHAPE(shape[4], 1, 129, 1, 0);
    ASSIGN_SHAPE(shape[5], 1, 129, 1, 0);

    // ASSIGN_SHAPE(shape[0], 1, 129, 1, 1);
    // ASSIGN_SHAPE(shape[1], 1, 129, 1, 1);
    // ASSIGN_SHAPE(shape[2], 1, 129, 1, 1);
    // ASSIGN_SHAPE(shape[3], 1, 129, 1, 1);
    // ASSIGN_SHAPE(shape[4], 1, 129, 1, 1);
    // ASSIGN_SHAPE(shape[5], 1, 129, 1, 1);

    // ASSIGN_SHAPE(shape[0], 1, 1, 129, 1);
    // ASSIGN_SHAPE(shape[1], 1, 1, 129, 1);
    // ASSIGN_SHAPE(shape[2], 1, 1, 129, 1);
    // ASSIGN_SHAPE(shape[3], 1, 1, 129, 1);
    // ASSIGN_SHAPE(shape[4], 1, 1, 129, 1);
    // ASSIGN_SHAPE(shape[5], 1, 1, 129, 1);

    // ASSIGN_SHAPE(shape[0], 1, 1, 1, 129);
    // ASSIGN_SHAPE(shape[1], 1, 1, 1, 129);
    // ASSIGN_SHAPE(shape[2], 1, 1, 1, 129);
    // ASSIGN_SHAPE(shape[3], 1, 1, 1, 129);
    // ASSIGN_SHAPE(shape[4], 1, 1, 1, 129);
    // ASSIGN_SHAPE(shape[5], 1, 1, 1, 129);

    ASSIGN_SHAPE(shape[6], 1, 12, 2, 129);
    ASSIGN_SHAPE(shape[7], 1, 12, 4, 129);
    ASSIGN_SHAPE(shape[8], 1, 12, 8, 129);
    ASSIGN_SHAPE(shape[9], 1, 12, 16, 129);

    ASSIGN_SHAPE(shape[10], 1, 12, 2, 65);
    ASSIGN_SHAPE(shape[11], 1, 12, 4, 65);
    ASSIGN_SHAPE(shape[12], 1, 12, 8, 65);
    ASSIGN_SHAPE(shape[13], 1, 12, 16, 65);

    ASSIGN_SHAPE(shape[14], 1, 12, 2, 65);
    ASSIGN_SHAPE(shape[15], 1, 12, 4, 65);
    ASSIGN_SHAPE(shape[16], 1, 12, 8, 65);
    ASSIGN_SHAPE(shape[17], 1, 12, 16, 65);

    for(j=0;j<pem_afc_b->nbin;++j){
        if(mag_Y[j] == 0){
            mag_Y[j] = 1e-16;
        }
    }
    for(j=0;j<pem_afc_b->nbin;++j){
        if(mag_R1[j] == 0){
            mag_R1[j] = 1e-16;
        }
    }
    for(j=0;j<pem_afc_b->nbin;++j){
        if(mag_R2[j] == 0){
            mag_R2[j] = 1e-16;
        }
    }

    i = 0;
    shape_len = shape[i][3] == 0 ? 3:4;
    input_val[i] = qtk_nnrt_value_create_external(
        pem_afc_b->rt, QTK_NNRT_VALUE_ELEM_F32, shape[i], shape_len, mag_Y);
    qtk_nnrt_feed_s(pem_afc_b->rt, input_val[i], in_name[i]);

    i = 1;
    shape_len = shape[i][3] == 0 ? 3:4;
    input_val[i] = qtk_nnrt_value_create_external(
        pem_afc_b->rt, QTK_NNRT_VALUE_ELEM_F32, shape[i], shape_len, pha_Y);
    qtk_nnrt_feed_s(pem_afc_b->rt, input_val[i], in_name[i]);

    i = 2;
    shape_len = shape[i][3] == 0 ? 3:4;
    input_val[i] = qtk_nnrt_value_create_external(
        pem_afc_b->rt, QTK_NNRT_VALUE_ELEM_F32, shape[i], shape_len, mag_R1);
    qtk_nnrt_feed_s(pem_afc_b->rt, input_val[i], in_name[i]);

    i = 3;
    shape_len = shape[i][3] == 0 ? 3:4;
    input_val[i] = qtk_nnrt_value_create_external(
        pem_afc_b->rt, QTK_NNRT_VALUE_ELEM_F32, shape[i], shape_len, pha_R1);
    qtk_nnrt_feed_s(pem_afc_b->rt, input_val[i], in_name[i]);

    i = 4;
    shape_len = shape[i][3] == 0 ? 3:4;
    input_val[i] = qtk_nnrt_value_create_external(
        pem_afc_b->rt, QTK_NNRT_VALUE_ELEM_F32, shape[i], shape_len, mag_R2);
    qtk_nnrt_feed_s(pem_afc_b->rt, input_val[i], in_name[i]);

    i = 5;
    shape_len = shape[i][3] == 0 ? 3:4;
    input_val[i] = qtk_nnrt_value_create_external(
        pem_afc_b->rt, QTK_NNRT_VALUE_ELEM_F32, shape[i], shape_len, pha_R2);
    qtk_nnrt_feed_s(pem_afc_b->rt, input_val[i], in_name[i]);

    for(i = num_in2; i < num_in; ++i){
        if(pem_afc_b->ncnn_nframe == 0){
            int64_t nelem;
            float *input_data;
            shape_len = shape[i][3] == 0 ? 3:4;
            if(shape_len==3){
                nelem = shape[i][0]*shape[i][1]*shape[i][2];
            }else{
                nelem = TENSOR_NELEM(shape[i]);
            }
            input_data =
                wtk_heap_zalloc(pem_afc_b->heap, sizeof(float) * nelem);
            input_val[i] = qtk_nnrt_value_create_external(
                pem_afc_b->rt, QTK_NNRT_VALUE_ELEM_F32, shape[i], shape_len,
                input_data);
            qtk_nnrt_feed_s(pem_afc_b->rt, input_val[i], in_name[i]);
        } else {
            input_val[i] = pem_afc_b->cache_value[in_index[i]];
            qtk_nnrt_feed_s(pem_afc_b->rt, input_val[i], in_name[i]);
        }
    }

    // qtk_nnrt_get_output_s(pem_afc_b->rt, output_val, "22");
    // qtk_nnrt_value_get_shape(pem_afc_b->rt, output_val[0], shape[32], shape_len);
    // printf("shape_len:%d\n", shape_len);
    // for(i=0;i<shape_len;++i){
    //     printf("%d %ld\n", i, shape[32][i]);
    // }
    // // mag = qtk_nnrt_value_get_channel_data(pem_afc_b->rt, output_val[0], 0);
    // mag = qtk_nnrt_value_get_data(pem_afc_b->rt, output_val[0]);
    // for(i=0;i<10;++i){
    //     printf("%f ", mag[i]);
    //     printf("%f\n", mag_R1[i]);
    // }
    // exit(0);
    for (i = 0; i < num_out; ++i) {
        if (out_index[i] != -1) {
            qtk_nnrt_get_output_s(pem_afc_b->rt,
                                  pem_afc_b->cache_value + out_index[i],
                                  out_name[i]);
        }else{
            qtk_nnrt_get_output_s(pem_afc_b->rt, output_val+i, out_name[i]);
            if(strcmp(out_name[i], "out0")==0){
                mag = qtk_nnrt_value_get_data(pem_afc_b->rt, output_val[i]);
            }else if(strcmp(out_name[i], "out1")==0){
                pha = qtk_nnrt_value_get_data(pem_afc_b->rt, output_val[i]);
            }
        }
    }
    wtk_pem_afc_b_ccm(pem_afc_b, mag, pha, fftx);
    for (i = 0; i < num_in; i++) {
        qtk_nnrt_value_release(pem_afc_b->rt, input_val[i]);
    }
    for(i=0;i<num_out;++i){
        if(out_index[i]==-1){
            qtk_nnrt_value_release(pem_afc_b->rt, output_val[i]);
        }
    }
    qtk_nnrt_reset(pem_afc_b->rt);
    ++pem_afc_b->ncnn_nframe;
}

void wtk_pem_afc_b_feed_model(wtk_pem_afc_b_t *pem_afc_b)
{
    wtk_strbuf_t *mic = pem_afc_b->mic;
    wtk_strbuf_t *sp = pem_afc_b->sp;
    wtk_strbuf_t *pem = pem_afc_b->pem;
    wtk_drft_t *drft = pem_afc_b->drft6;
    float *analysis_window = pem_afc_b->analysis_window;
    float *synthesis_window = pem_afc_b->synthesis_window;
    float *analysis_mem_mic = pem_afc_b->analysis_mem_mic;
    float *analysis_mem_sp = pem_afc_b->analysis_mem_sp;
    float *analysis_mem_pem = pem_afc_b->analysis_mem_pem;
	float *synthesis_mem2 = pem_afc_b->synthesis_mem2;

    wtk_complex_t *mic_fft = pem_afc_b->mic_fft;
    wtk_complex_t *sp_fft = pem_afc_b->sp_fft;
    wtk_complex_t *pem_fft = pem_afc_b->pem_fft;
    wtk_complex_t *fftx = pem_afc_b->fftx;
    float *mag_Y = pem_afc_b->mag_Y;
    float *pha_Y = pem_afc_b->pha_Y;
    float *mag_R1 = pem_afc_b->mag_R1;
    float *pha_R1 = pem_afc_b->pha_R1;
    float *mag_R2 = pem_afc_b->mag_R2;
    float *pha_R2 = pem_afc_b->pha_R2;
	int clip_s=pem_afc_b->cfg->clip_s;
	int clip_e=pem_afc_b->cfg->clip_e;

    float *fft_tmp = pem_afc_b->fft_tmp;
    float *out = pem_afc_b->out;
    int wins = pem_afc_b->cfg->wins;
    int fsize = pem_afc_b->cfg->hop_size;
    int nbin = pem_afc_b->nbin;
    int length;
    float *fv, *fv1, *fv2;
    int i;

    length = pem->pos/sizeof(float);

    if(length >= fsize){
        fv = (float *)mic->data;
        fv1 = (float *)sp->data;
        fv2 = (float *)pem->data;

        wtk_drft_frame_analysis22_float(drft, fft_tmp, analysis_mem_mic, mic_fft, fv, wins, analysis_window);
        wtk_drft_frame_analysis22_float(drft, fft_tmp, analysis_mem_sp, sp_fft, fv1, wins, analysis_window);
        wtk_drft_frame_analysis22_float(drft, fft_tmp, analysis_mem_pem, pem_fft, fv2, wins, analysis_window);

        for(i=0;i<nbin;++i){
            mag_Y[i] = wtk_pem_afc_b_c_abs(mic_fft[i]);
            pha_Y[i] = wtk_pem_afc_b_c_angle(mic_fft[i]);

            mag_R1[i] = wtk_pem_afc_b_c_abs(pem_fft[i]);
            pha_R1[i] = wtk_pem_afc_b_c_angle(pem_fft[i]);

            mag_R2[i] = wtk_pem_afc_b_c_abs(sp_fft[i]);
            pha_R2[i] = wtk_pem_afc_b_c_angle(sp_fft[i]);
        }
        if(pem_afc_b->cfg->use_onnx){
            wtk_pem_afc_b_feed_onnx(pem_afc_b, fftx);
        }else if(pem_afc_b->cfg->use_ncnn){
            wtk_pem_afc_b_feed_ncnn(pem_afc_b, fftx);
        }

        if(pem_afc_b->cfg->use_filter){
            for(i=0;i<clip_s;++i){
                fftx[i].a = 0;
                fftx[i].b = 0;
            }
            for(i=clip_e;i<nbin;++i){
                fftx[i].a = 0;
                fftx[i].b = 0;
            }
        }

        wtk_drft_frame_synthesis22(drft, fft_tmp, synthesis_mem2, fftx, out, wins, synthesis_window);
        wtk_strbuf_pop(pem, NULL, fsize*sizeof(float));
    }
}

void wtk_pem_afc_b_feed2(wtk_pem_afc_b_t *pem_afc_b, float *mic, float *sp, int len, int is_end)
{
    int i;
    int N_afc = pem_afc_b->cfg->N_afc;
    int N_ar = pem_afc_b->cfg->N_ar;
    int hop_size = pem_afc_b->cfg->hop_size;
    float power_alpha = pem_afc_b->cfg->power_alpha;
    float power_alpha_1 = 1.0 - power_alpha;
    float mu1 = pem_afc_b->cfg->mu1;
    float mu2 = pem_afc_b->cfg->mu2;
    int f1_w_len = pem_afc_b->filtering_conv1_w_len;
    int f2_w_len = pem_afc_b->filtering_conv2_w_len;
    int m_w_len = pem_afc_b->whiten_m_conv_w_len;
    int u_w_len = pem_afc_b->whiten_u_conv_w_len;
	int clip_s=pem_afc_b->cfg->clip_s;
	int clip_e=pem_afc_b->cfg->clip_e;
    PFFFT_Setup *pffft1 = pem_afc_b->pffft1;
    PFFFT_Setup *pffft2 = pem_afc_b->pffft2;
    wtk_drft_t *drft3 = pem_afc_b->drft3;
    // PFFFT_Setup *pffft3 = pem_afc_b->pffft3;
    PFFFT_Setup *pffft4 = pem_afc_b->pffft4;
    PFFFT_Setup *pffft5 = pem_afc_b->pffft5;
    float *error = pem_afc_b->error;
    float *error_soft_clipped = pem_afc_b->error_soft_clipped;
    float *Truncation_indicator = pem_afc_b->Truncation_indicator;
    float *m_whiten = pem_afc_b->m_whiten;
    float *u_whiten = pem_afc_b->u_whiten;
    float *Truncation_indicator_buffer = pem_afc_b->Truncation_indicator_buffer;
    float *m_whiten_buffer = pem_afc_b->m_whiten_buffer;
    float *u_whiten_buffer = pem_afc_b->u_whiten_buffer;

    float *filtering_conv1_w = pem_afc_b->filtering_conv1_w;
    float *filtering_conv2_w = pem_afc_b->filtering_conv2_w;
    float *whiten_m_conv_w = pem_afc_b->whiten_m_conv_w;
    float *whiten_u_conv_w = pem_afc_b->whiten_u_conv_w;

    float *filtering_conv1_cache = pem_afc_b->filtering_conv1_cache;
    float *filtering_conv2_cache = pem_afc_b->filtering_conv2_cache;
    float *whiten_m_conv_cache = pem_afc_b->whiten_m_conv_cache;
    float *whiten_u_conv_cache = pem_afc_b->whiten_u_conv_cache;
    float *prev_m = pem_afc_b->prev_m;
    float *prev_u = pem_afc_b->prev_u;
    float *power = pem_afc_b->power;

    float *ar_R = pem_afc_b->ar_R;
    float *ar_w = pem_afc_b->ar_w;
    float *afc_w = pem_afc_b->afc_w;

    float *Yk = pem_afc_b->Yk;
    float *Xk = pem_afc_b->Xk;

    float *fft_tmp = pem_afc_b->fft_tmp;
    float *ifft_tmp = pem_afc_b->ifft_tmp;
    float *tmp = pem_afc_b->tmp;
    float *out = pem_afc_b->out;
    short *pv;

    float Ti;
    float afc_w_mean;
    float mu;

    float *fv;

    // static int cnt=0;
    // char file_path[] = "/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/";

  
    // char result[strlen(file_path) + 11 + strlen(file_path) + 1]; // +1 for the null terminator  
    // sprintf(result, "%s%ddata1.bin", file_path, cnt);  
    // FILE *fp1 = fopen(result, "rb");
    // sprintf(result, "%s%ddata2.bin", file_path, cnt);  
    // FILE *fp2 = fopen(result, "rb");
    // sprintf(result, "%s%ddata3.bin", file_path, cnt);  
    // FILE *fp3 = fopen(result, "rb");
    // sprintf(result, "%s%ddata4.bin", file_path, cnt);  
    // FILE *fp4 = fopen(result, "rb");
    // sprintf(result, "%s%ddata5.bin", file_path, cnt);  
    // FILE *fp5 = fopen(result, "rb");
    // sprintf(result, "%s%ddata6.bin", file_path, cnt);  
    // FILE *fp6 = fopen(result, "rb");
    // sprintf(result, "%s%ddata7.bin", file_path, cnt);  
    // FILE *fp7 = fopen(result, "rb");
    // sprintf(result, "%s%ddata8.bin", file_path, cnt);  
    // FILE *fp8 = fopen(result, "rb");
    // sprintf(result, "%s%ddata9.bin", file_path, cnt);  
    // FILE *fp9 = fopen(result, "rb");
    // sprintf(result, "%s%ddata10.bin", file_path, cnt);  
    // FILE *fp10 = fopen(result, "rb");
    // sprintf(result, "%s%ddata11.bin", file_path, cnt);  
    // FILE *fp11 = fopen(result, "rb");
    // sprintf(result, "%s%ddata12.bin", file_path, cnt);  
    // FILE *fp12 = fopen(result, "rb");
    // sprintf(result, "%s%ddata13.bin", file_path, cnt);  
    // FILE *fp13 = fopen(result, "rb");
    // sprintf(result, "%s%ddata14.bin", file_path, cnt);  
    // FILE *fp14 = fopen(result, "rb");
    // sprintf(result, "%s%ddata15.bin", file_path, cnt);  
    // FILE *fp15 = fopen(result, "rb");
    // sprintf(result, "%s%ddata16.bin", file_path, cnt);  
    // FILE *fp16 = fopen(result, "rb");
    // sprintf(result, "%s%ddata17.bin", file_path, cnt);  
    // FILE *fp17 = fopen(result, "rb");
    // sprintf(result, "%s%ddata18.bin", file_path, cnt);  
    // FILE *fp18 = fopen(result, "rb");

    // // FILE *fp2 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data2.bin", "rb");
    // // FILE *fp3 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data3.bin", "rb");
    // // FILE *fp4 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data4.bin", "rb");
    // // FILE *fp5 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data5.bin", "rb");
    // // FILE *fp6 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data6.bin", "rb");
    // // FILE *fp7 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data7.bin", "rb");
    // // FILE *fp8 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data8.bin", "rb");
    // // FILE *fp9 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data9.bin", "rb");
    // // FILE *fp10 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data10.bin", "rb");
    // // FILE *fp11 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data11.bin", "rb");
    // // FILE *fp12 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data12.bin", "rb");
    // // FILE *fp13 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data13.bin", "rb");
    // // FILE *fp14 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data14.bin", "rb");
    // // FILE *fp15 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data15.bin", "rb");
    // // FILE *fp16 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data16.bin", "rb");
    // // FILE *fp17 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data17.bin", "rb");
    // // FILE *fp18 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data18.bin", "rb");

    // ++cnt;

    // if(fp1){
    //     fread(mic, sizeof(float), hop_size, fp1);
    //     fclose(fp1);
    // }
    // if(fp2){
    //     fread(sp, sizeof(float), hop_size, fp2);
    //     fclose(fp2);
    // }
    // if(fp3){
    //     fread(filtering_conv1_cache, sizeof(float), N_afc, fp3);
    //     fclose(fp3);
    // }
    // if(fp4){
    //     fread(filtering_conv2_cache, sizeof(float), N_afc, fp4);
    //     fclose(fp4);
    // }
    // if(fp5){
    //     fread(whiten_m_conv_cache, sizeof(float), N_ar, fp5);
    //     fclose(fp5);
    // }
    // if(fp6){
    //     fread(whiten_u_conv_cache, sizeof(float), N_ar, fp6);
    //     fclose(fp6);
    // }
    // if(fp7){
    //     fread(filtering_conv1_w, sizeof(float), f1_w_len, fp7);
    //     fclose(fp7);
    // }
    // if(fp8){
    //     fread(filtering_conv2_w, sizeof(float), f2_w_len, fp8);
    //     fclose(fp8);
    // }
    // if(fp9){
    //     fread(whiten_m_conv_w, sizeof(float), m_w_len, fp9);
    //     fclose(fp9);
    // }
    // if(fp10){
    //     fread(whiten_u_conv_w, sizeof(float), u_w_len, fp10);
    //     fclose(fp10);
    // }
    // if(fp11){
    //     fread(prev_m, sizeof(float), hop_size, fp11);
    //     fclose(fp11);
    // }
    // if(fp12){
    //     fread(prev_u, sizeof(float), hop_size, fp12);
    //     fclose(fp12);
    // }
    // if(fp13){
    //     fread(u_whiten_buffer, sizeof(float), pem_afc_b->u_buffer_len, fp13);
    // }
    // if(fp14){
    //     fread(m_whiten_buffer, sizeof(float), pem_afc_b->m_buffer_len, fp14);
    // }
    // if(fp15){
    //     fread(Truncation_indicator_buffer, sizeof(float), pem_afc_b->T_buffer_len, fp15);
    //     fclose(fp15);
    // }
    // if(fp16){
    //     fread(power, sizeof(float), N_afc+1, fp16);
    //     fclose(fp16);
    // }
    // if(fp17){
    //     fread(ar_w, sizeof(float), N_ar, fp17);
    //     fclose(fp17);
    // }
    // if(fp18){
    //     fread(afc_w, sizeof(float), N_afc, fp18);
    //     fclose(fp18);
    // }


    wtk_pem_afc_b_conv_call(pem_afc_b, pffft1, NULL, pem_afc_b->fft1_len, sp, hop_size, filtering_conv1_cache, N_afc, filtering_conv1_w, f1_w_len, error, hop_size);
    // print_float2(error, hop_size);
    for(i=0;i<hop_size;++i){
        error[i] = mic[i] - error[i];
        error_soft_clipped[i] = error[i] * 0.5;
    }
    _tanh(error_soft_clipped, hop_size);
    for(i=0;i<hop_size;++i){
        error_soft_clipped[i] *= 2.0;
        Truncation_indicator[i] = fabs(error[i] - error_soft_clipped[i]) >= 0.15 ? 1.0:0.0;
    }

    // FILE *fp = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/error_soft_clipped.bin", "rb");
    // if(fp){
    //     fread(error_soft_clipped, sizeof(float), hop_size, fp);
    //     fclose(fp);
    // }else{
    //     printf("error_soft_clipped.bin not found\n");
    // }
    // print_float2(error, hop_size);
    // print_float2(error_soft_clipped, hop_size);
    // print_float2(Truncation_indicator, hop_size);

    wtk_pem_afc_b_buffer_stack_in(Truncation_indicator_buffer, Truncation_indicator, hop_size, pem_afc_b->T_buffer_size);
    // print_float2(Truncation_indicator_buffer, pem_afc_b->T_buffer_len);
    // printf("Truncation_indicator_buffer\n");

    // FILE *fp5 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data5.bin", "rb");
    // FILE *fp6 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data6.bin", "rb");
    // FILE *fp7 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data7.bin", "rb");

    // if(fp5){
    //     fread(prev_m, sizeof(float), hop_size, fp5);
    //     fclose(fp5);
    // }else{
    //     printf("data5.bin not found\n");
    // }

    // if(fp6){
    //     fread(whiten_m_conv_cache, sizeof(float), N_ar, fp6);
    //     fclose(fp6);
    // }else{
    //     printf("data6.bin not found\n");
    // }

    // if(fp7){
    //     fread(whiten_m_conv_w, sizeof(float), m_w_len, fp7);
    //     fclose(fp7);
    // }else{
    //     printf("data7.bin not found\n");
    // }

    wtk_pem_afc_b_conv_call(pem_afc_b, NULL, drft3, pem_afc_b->fft3_len, prev_m, hop_size, whiten_m_conv_cache, N_ar, whiten_m_conv_w, m_w_len, m_whiten, hop_size);
    // print_float2(m_whiten, hop_size);
    // getchar();
    wtk_pem_afc_b_conv_call(pem_afc_b, NULL, drft3, pem_afc_b->fft3_len, prev_u, hop_size, whiten_u_conv_cache, N_ar, whiten_u_conv_w, u_w_len, u_whiten, hop_size);
    // print_float2(u_whiten, hop_size);
    // printf("u_whiten\n");
    // getchar();
    memcpy(prev_m, mic, hop_size*sizeof(float));
    memcpy(prev_u, sp, hop_size*sizeof(float));

    wtk_pem_afc_b_buffer_stack_in(m_whiten_buffer, m_whiten, hop_size, pem_afc_b->m_buffer_size);
    wtk_pem_afc_b_buffer_stack_in(u_whiten_buffer, u_whiten, hop_size, pem_afc_b->u_buffer_size);
    // print_float2(u_whiten_buffer, pem_afc_b->u_buffer_len);
    // printf("u_whiten_buffer\n");

    memcpy(fft_tmp, error_soft_clipped, hop_size*sizeof(float));
    memset(fft_tmp+hop_size, 0, hop_size*sizeof(float));
    // print_float2(fft_tmp, hop_size*2);
    pffft_transform_ordered(pffft4, fft_tmp, ifft_tmp, tmp, PFFFT_FORWARD);
    ifft_tmp[pem_afc_b->fft4_len] = ifft_tmp[1];
    ifft_tmp[1] = 0;
    ifft_tmp[pem_afc_b->fft4_len+1] = 0;
    // printf("X\n");
    // print_float2(ifft_tmp, pem_afc_b->fft4_len+2);
    for(i=0;i<hop_size+1;++i){
        fft_tmp[2*i] = ifft_tmp[2*i] * ifft_tmp[2*i] + ifft_tmp[2*i+1] * ifft_tmp[2*i+1];
        fft_tmp[2*i+1] = 0;
    }
    // print_float2(fft_tmp, hop_size*2+2);
    fft_tmp[1] = fft_tmp[pem_afc_b->fft4_len];
    pffft_transform_ordered(pffft4, fft_tmp, ifft_tmp, tmp, PFFFT_BACKWARD);
    for(i=0;i<pem_afc_b->fft4_len;++i){
        ifft_tmp[i] *= 1.0/pem_afc_b->fft4_len;
    }
    memcpy(ar_R, ifft_tmp, hop_size*sizeof(float));
    for(i=0;i<hop_size;++i){
        ar_R[i] *= 1.0/hop_size;
    }
    // printf("R\n");
    // print_float2(ar_R, hop_size);
    // 1e-8误差
    // FILE *fp2 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/corr.bin", "rb");
    // if(fp2){
    //     fread(ar_R, sizeof(float), N_ar, fp2);
    //     fclose(fp2);
    // }else{
    //     printf("corr.bin not found\n");
    // }

    wtk_pem_afc_b_levinson(pem_afc_b);
    // print_float2(ar_w, N_ar);

    wtk_pem_afc_b_reset_weight(pem_afc_b, NULL, drft3, pem_afc_b->fft3_len, ar_w, N_ar, whiten_m_conv_w, m_w_len);
    // print_float2(whiten_m_conv_w, m_w_len);
    wtk_pem_afc_b_reset_weight(pem_afc_b, NULL, drft3, pem_afc_b->fft3_len, ar_w, N_ar, whiten_u_conv_w, u_w_len);
    // print_float2(whiten_u_conv_w, u_w_len);

    // print_float2(filtering_conv1_w, f1_w_len);
    // printf("filtering_conv1\n");
    // print_float2(filtering_conv2_w, f2_w_len);
    // printf("filtering_conv2\n");
    // print_float2(whiten_m_conv_w, m_w_len);
    // printf("whiten_m_conv\n");
    // print_float2(whiten_u_conv_w, u_w_len);
    // printf("whiten_u_conv\n");
    // print_float2(ar_w, N_ar);
    // printf("ar_w\n");
    // print_float2(afc_w, N_afc);
    // printf("afc_w\n");
    // print_float2(error_soft_clipped, hop_size);
    // printf("error_soft_clipped\n");
    // getchar();

    if((pem_afc_b->nframe+1)%pem_afc_b->cfg->N_block==0){

        // FILE *fp3 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data1.bin", "rb");
        // FILE *fp4 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data2.bin", "rb");
        // FILE *fp5 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data3.bin", "rb");
        // if(fp3){
        //     fread(u_whiten_buffer+hop_size, sizeof(float), N_afc, fp3);
        //     fclose(fp3);
        // }else{
        //     printf("data1.bin not found\n");
        // }
        // if(fp4){
        //     fread(filtering_conv2_cache, sizeof(float), N_afc, fp4);
        //     fclose(fp4);
        // }else{
        //     printf("data2.bin not found\n");
        // }
        // if(fp5){
        //     fread(filtering_conv2_w, sizeof(float), f2_w_len, fp5);
        //     fclose(fp5);
        // }else{
        //     printf("data3.bin not found\n");
        // }
        // print_float2(u_whiten_buffer+hop_size, N_afc);
        // print_float2(filtering_conv2_cache, N_afc);
        // print_float2(filtering_conv2_w, f2_w_len);
        fv = wtk_pem_afc_b_get_buffer(u_whiten_buffer, pem_afc_b->u_buffer_len, pem_afc_b->u_buffer_size);
        // print_float2(filtering_conv2_cache, N_afc);
        // printf("filtering_conv2_cache\n");
        wtk_pem_afc_b_conv_call(pem_afc_b, pffft2, NULL, pem_afc_b->fft2_len, fv+N_afc, pem_afc_b->u_buffer_size-N_afc, filtering_conv2_cache, N_afc, filtering_conv2_w, f2_w_len, Yk, N_afc);
        // print_float2(fv+N_afc, N_afc);
        // printf("fv\n");
        // print_float2(filtering_conv2_cache, N_afc);
        // printf("filtering_conv2_cache2\n");
        // print_float2(filtering_conv2_w, f2_w_len);
        // printf("filtering_conv2_w\n");
        // print_float2(Yk, N_afc);
        // printf("Yk\n");
        // FILE *fp6 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data4.bin", "rb");
        // if(fp6){
        //     fread(u_whiten_buffer+hop_size, sizeof(float), pem_afc_b->u_buffer_size, fp6);
        //     fclose(fp6);
        // }else{
        //     printf("data4.bin not found\n");
        // }

        fv = wtk_pem_afc_b_get_buffer(u_whiten_buffer, pem_afc_b->u_buffer_len, pem_afc_b->u_buffer_size);
        memcpy(fft_tmp, fv, pem_afc_b->u_buffer_size*sizeof(float));
        pffft_transform_ordered(pffft2, fft_tmp, ifft_tmp, tmp, PFFFT_FORWARD);
        ifft_tmp[pem_afc_b->fft2_len] = ifft_tmp[1];
        ifft_tmp[1] = 0;
        ifft_tmp[pem_afc_b->fft2_len+1] = 0;
        memcpy(Xk, ifft_tmp, (N_afc+1)*2*sizeof(float));
        // print_float2(Xk, N_afc*2+2);
        // print_float2(power, N_afc+1);
        // printf("power1\n");
        for(i=0;i<N_afc+1;++i){
            power[i] = power_alpha * power[i] + power_alpha_1 * (Xk[2*i] * Xk[2*i] + Xk[2*i+1] * Xk[2*i+1]);
        }
        // print_float2(power, N_afc+1);
        fv = wtk_pem_afc_b_get_buffer(Truncation_indicator_buffer, pem_afc_b->T_buffer_len, pem_afc_b->T_buffer_size);
        Ti = wtk_float_mean(fv, pem_afc_b->T_buffer_size) > 0.5? 1.0:0.0;
        mu = Ti * mu2 + (1.0 - Ti) * mu1;
        // printf("mu=%f Ti=%f mu1=%f mu2=%f\n", mu, Ti, mu1, mu2);

        fv = wtk_pem_afc_b_get_buffer(m_whiten_buffer, pem_afc_b->m_buffer_len, pem_afc_b->m_buffer_size);
        for(i=0;i<N_afc;++i){
            Yk[i] = fv[i] - Yk[i];
        }

        // FILE *fp9 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data9.bin", "rb");
        // FILE *fp10 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data10.bin", "rb");
        // FILE *fp11 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data11.bin", "rb");

        // if(fp9){
        //     fread(Yk, sizeof(float), N_afc, fp9);
        // }else{
        //     printf("data9.bin not found\n");
        // }

        // if(fp10){
        //     fread(Xk, sizeof(float), N_afc*2+2, fp10);
        // }else{
        //     printf("data10.bin not found\n");
        // }

        // if(fp11){
        //     fread(power, sizeof(float), N_afc+1, fp11);
        // }else{
        //     printf("data11.bin not found\n");
        // }


        memset(fft_tmp, 0, N_afc*sizeof(float));
        memcpy(fft_tmp+N_afc, Yk, N_afc*sizeof(float));
        pffft_transform_ordered(pffft2, fft_tmp, ifft_tmp, tmp, PFFFT_FORWARD);
        ifft_tmp[pem_afc_b->fft2_len] = ifft_tmp[1];
        ifft_tmp[1] = 0;
        ifft_tmp[pem_afc_b->fft2_len+1] = 0;
        // print_float2(ifft_tmp, pem_afc_b->fft2_len+2);
        // printf("Ek\n");
        // print_float2(Xk, N_afc*2+2);
        // printf("Xk\n");
        // print_float2(power, N_afc+1);
        // printf("power\n");
        // wtk_debug("==================\n");
        for(i=0;i<pem_afc_b->fft2_len/2+1;++i){
            fft_tmp[2*i] = Xk[2*i] * ifft_tmp[2*i] + Xk[2*i+1] * ifft_tmp[2*i+1];
            fft_tmp[2*i+1] = Xk[2*i] * ifft_tmp[2*i+1] - Xk[2*i+1] * ifft_tmp[2*i];
            fft_tmp[2*i] *= 1.0/power[i];
            fft_tmp[2*i+1] *= 1.0/power[i];
        }
        // print_float2(fft_tmp, pem_afc_b->fft2_len+2);
        fft_tmp[1] = fft_tmp[pem_afc_b->fft2_len];
        pffft_transform_ordered(pffft2, fft_tmp, ifft_tmp, tmp, PFFFT_BACKWARD);
        for(i=0;i<pem_afc_b->fft2_len/2;++i){
            ifft_tmp[i] *= 1.0/pem_afc_b->fft2_len;
        }
        // print_float2(ifft_tmp, pem_afc_b->fft2_len/2);
        // printf("gradient\n");
        // getchar();
        // print_float2(afc_w, N_afc);
        // printf("afc_w\n");
        for(i=0;i<N_afc;++i){
            afc_w[i] += mu * ifft_tmp[i];
        }
        afc_w_mean = wtk_float_mean(afc_w, N_afc);
        // printf("afc_w_mean=%f\n", afc_w_mean);
        for(i=0;i<N_afc;++i){
            afc_w[i] -= afc_w_mean;
        }
        // FILE *fp8 = fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data8.bin", "rb");

        // if(fp8){
        //     fread(afc_w, sizeof(float), N_afc, fp8);
        //     fclose(fp8);
        // }else{
        //     printf("data8.bin not found\n");
        // }

        wtk_pem_afc_b_reset_weight(pem_afc_b, pffft1, NULL, pem_afc_b->fft1_len, afc_w, N_afc, filtering_conv1_w, f1_w_len);
        wtk_pem_afc_b_reset_weight(pem_afc_b, pffft2, NULL, pem_afc_b->fft2_len, afc_w, N_afc, filtering_conv2_w, f2_w_len);
        // print_float2(filtering_conv1_w, f1_w_len);
        // print_float2(error_soft_clipped, hop_size);
        // getchar();
        // wtk_debug("=====================%d\n", pem_afc_b->nframe);
    }
    ++pem_afc_b->nframe;
    wtk_strbuf_push(pem_afc_b->sp, (char *)error_soft_clipped, hop_size*sizeof(float));

    if(pem_afc_b->cfg->use_onnx || pem_afc_b->cfg->use_ncnn){
        wtk_strbuf_push(pem_afc_b->pem, (char *)error_soft_clipped, hop_size*sizeof(float));
        wtk_pem_afc_b_feed_model(pem_afc_b);
    }else{
        memcpy(out, error_soft_clipped, hop_size*sizeof(float));
        if(pem_afc_b->cfg->use_filter){
            memmove(fft_tmp, pem_afc_b->analysis_mem, pem_afc_b->fft5_len/2*sizeof(float));
            memcpy(fft_tmp+pem_afc_b->fft5_len/2, out, pem_afc_b->fft5_len/2*sizeof(float));
            memcpy(pem_afc_b->analysis_mem, out, pem_afc_b->fft5_len/2*sizeof(float));
            for(i=0;i<pem_afc_b->fft5_len;++i){
                fft_tmp[i] *= pem_afc_b->analysis_window[i];
            }
            pffft_transform_ordered(pffft5, fft_tmp, ifft_tmp, tmp, PFFFT_FORWARD);
            ifft_tmp[pem_afc_b->fft5_len] = ifft_tmp[1];
            ifft_tmp[1] = 0;
            ifft_tmp[pem_afc_b->fft5_len+1] = 0;

            for(i=0;i<clip_s;++i){
                ifft_tmp[2*i] = 0;
                ifft_tmp[2*i+1] = 0;
            }
            for(i=clip_e;i<pem_afc_b->fft5_len/2+1;++i){
                ifft_tmp[2*i] = 0;
                ifft_tmp[2*i+1] = 0;
            }
            ifft_tmp[1] = ifft_tmp[pem_afc_b->fft5_len];
            pffft_transform_ordered(pffft5, ifft_tmp, fft_tmp, tmp, PFFFT_BACKWARD);
            for(i=0;i<pem_afc_b->fft5_len;++i){
                fft_tmp[i] *= 1.0/pem_afc_b->fft5_len;
            }
            for(i=0;i<pem_afc_b->fft5_len;++i){
                fft_tmp[i] *= pem_afc_b->synthesis_window[i];
            }
            for(i=0;i<pem_afc_b->fft5_len/2;++i){
                out[i] = fft_tmp[i]+pem_afc_b->synthesis_mem[i];
            }
            memcpy(pem_afc_b->synthesis_mem, fft_tmp+pem_afc_b->fft5_len/2, pem_afc_b->fft5_len/2*sizeof(float));
        }
    }


    // print_float2(filtering_conv1_w, f1_w_len);
    // printf("filtering_conv1\n");
    // // print_float2(filtering_conv2_w, f2_w_len);
    // // printf("filtering_conv2\n");
    // // print_float2(whiten_m_conv_w, m_w_len);
    // // printf("whiten_m_conv\n");
    // // print_float2(whiten_u_conv_w, u_w_len);
    // // printf("whiten_u_conv\n");
    // // print_float2(ar_w, N_ar);
    // // printf("ar_w\n");
    // print_float2(afc_w, N_afc);
    // printf("afc_w\n");
    // print_float2(error_soft_clipped, hop_size);
    // printf("error_soft_clipped\n");
    // // print_float2(sp, hop_size);
    // // printf("sp\n");
    // // print_float2(filtering_conv1_cache, N_afc);
    // // printf("filtering_conv1_cache\n");
    // getchar();

    for(i=0;i<hop_size;++i){
        out[i] = out[i] * 32767.0 * pem_afc_b->cfg->out_scale;
        if(out[i]>32767.0){
            out[i]=32767.0;
        }
        if(out[i]<-32767.0){
            out[i]=-32767.0;
        }
    }

    if(pem_afc_b->eq)
    {
        wtk_equalizer_feed_float(pem_afc_b->eq, out, hop_size);
    }
    pv=(short *)out;
    for(i=0;i<hop_size;++i){
        pv[i]=floorf(out[i]+0.5);
    }
    if(pem_afc_b->notify){
        if(!pem_afc_b->cfg->use_refcompensation || (pem_afc_b->cfg->use_refcompensation && pem_afc_b->sweep_ok == 1)){
            pem_afc_b->notify(pem_afc_b->ths, pv, hop_size);
        }
    }
    // getchar();
}

void wtk_pem_afc_b_python_feed(wtk_pem_afc_b_t *pem_afc_b, float *mic, float *sp, int len, int is_end)
{
    static FILE *fp=NULL;
    static int seek=0;
    if(fp==NULL){
        fp=fopen("/home/lixiao/work/doc/algorithm/howl/fw/AHS_Adaptive_Filter/data.bin", "rb");
    }
    if(fp){
        fseek(fp, seek*sizeof(float), SEEK_SET);
        fread(mic, sizeof(float), len, fp);
        seek+=len;
        wtk_pem_afc_b_feed2(pem_afc_b, mic, sp, len, is_end);
    }
}

void wtk_pem_afc_b_feed_float(wtk_pem_afc_b_t *pem_afc_b, float *data, int len, int is_end)
{
    int i;
    int *mic_channel = pem_afc_b->cfg->mic_channel;
    int channel = pem_afc_b->cfg->channel;
    float in_scale=pem_afc_b->cfg->in_scale;
    wtk_strbuf_t *mic = pem_afc_b->mic;
    wtk_strbuf_t *sp = pem_afc_b->sp;

    float fv;
    float *fv1, *fv2;
    int length;
    int fsize=pem_afc_b->cfg->hop_size;

//    if(pem_afc_b->cfg->use_refcompensation && !pem_afc_b->sweep_ok){
//        qtk_rir_estimate_feed(pem_afc_b,data,len,is_end);
//    }

    for (i = 0; i < len; ++i) {
        fv = data[mic_channel[0]] * 1.0 *in_scale;
        if(fv>1.0){
            fv=1.0;
        }
        if(fv<-1.0){
            fv=-1.0;
        }
        wtk_strbuf_push(mic, (char *)&(fv), sizeof(float));
        data += channel;
    }
    length = mic->pos / sizeof(float);

    while (length>=fsize)
    {
        fv1 = (float *)mic->data;
        fv2 = (float *)sp->data;

        if(pem_afc_b->cfg->use_refcompensation && pem_afc_b->sweep_ok){
            qtk_rir_estimate_conv1d_calc(pem_afc_b->r_est,fv2);
        }

        if(0){
            wtk_pem_afc_b_python_feed(pem_afc_b, fv1, fv2, fsize, is_end);
        }else{
            wtk_pem_afc_b_feed2(pem_afc_b, fv1, fv2, fsize, is_end);
        }
        wtk_strbuf_pop(mic, NULL, fsize*sizeof(float));
        wtk_strbuf_pop(sp, NULL, fsize*sizeof(float));
        length = mic->pos / sizeof(float);
    }
}

void wtk_pem_afc_b_feed(wtk_pem_afc_b_t *pem_afc_b, short *data, int len, int is_end)
{
    int i;
    int *mic_channel = pem_afc_b->cfg->mic_channel;
    int channel = pem_afc_b->cfg->channel;
    float in_scale=pem_afc_b->cfg->in_scale;
    wtk_strbuf_t *mic = pem_afc_b->mic;
    wtk_strbuf_t *sp = pem_afc_b->sp;

    float fv;
    float *fv1, *fv2;
    int length;
    int fsize=pem_afc_b->cfg->hop_size;

    if(pem_afc_b->cfg->use_refcompensation && !pem_afc_b->sweep_ok){
        //wtk_pem_afc_b_feed_sweep(pem_afc_b,data,len,is_end);
        qtk_rir_estimate_feed(pem_afc_b->r_est,data,len,is_end);
        if(is_end){
            pem_afc_b->sweep_ok = 1;
        }
    }

    for (i = 0; i < len; ++i) {
        fv = data[mic_channel[0]] * 1.0 / 32768.0*in_scale;
        if(fv>1.0){
            fv=1.0;
        }
        if(fv<-1.0){
            fv=-1.0;
        }
        wtk_strbuf_push(mic, (char *)&(fv), sizeof(float));
        data += channel;
    }
    length = mic->pos / sizeof(float);

    while (length>=fsize)
    {
        fv1 = (float *)mic->data;
        fv2 = (float *)sp->data;

        if(pem_afc_b->cfg->use_refcompensation && pem_afc_b->sweep_ok){
            //wtk_pem_afc_b_conv1d_calc(pem_afc_b->conv_sweep,fv2);
            qtk_rir_estimate_conv1d_calc(pem_afc_b->r_est, fv2);
        }

        if(0){
            wtk_pem_afc_b_python_feed(pem_afc_b, fv1, fv2, fsize, is_end);
        }else{
            wtk_pem_afc_b_feed2(pem_afc_b, fv1, fv2, fsize, is_end);
        }
        wtk_strbuf_pop(mic, NULL, fsize*sizeof(float));
        wtk_strbuf_pop(sp, NULL, fsize*sizeof(float));
        length = mic->pos / sizeof(float);
    }
}
