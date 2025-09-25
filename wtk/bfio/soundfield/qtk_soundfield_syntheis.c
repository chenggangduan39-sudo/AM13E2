#include "qtk_soundfield_syntheis.h"
#include "wtk/core/math/wtk_math.h"
static void complex_dump(wtk_complex_t *c, int len){
    // int i;
    // for(i = 0; i < len; i++){
    //     printf("%f %f\n", c[i].a, c[i].b);
    // }
    int i;
    for(i = 0; i < len; i++){
            printf("%.6g %.6gj\n", c[i].a, c[i].b);
    }
    printf("==============\n");
}


float* caculate_lambda2_vector_optimized(float lambda2, int n_bin, int sample_rate) {
    float nyquist = sample_rate / 2.0;
    float* lambda2_vector = (float*)wtk_malloc(n_bin * sizeof(float));
    for (int i = 0; i < n_bin; i++) {
        float f = i * nyquist / (n_bin - 1);
        float weight;
        if (f < 31) weight = 1.0;
        else if (f < 63) weight = 1.0;
        else if (f < 125) weight = 1.0;
        else if (f < 250) weight = 1.0;
        else if (f < 500) weight = 1.0;
        else if (f < 1000) weight = 1.0;
        else if (f < 2000) weight = 1.0;
        else weight = 1.0;
        lambda2_vector[i] = lambda2 * weight;
    }
    return lambda2_vector;
}

void calculate_distance_frequency_weights(qtk_soundfield_syntheis_t* sos,int n_bin, int N, float sample_rate) {
    sos->weights = (float**)malloc(n_bin * sizeof(float*));
    float **weights = sos->weights;
    for (int i = 0; i < n_bin; i++) {
        weights[i] = (float*)malloc(N * sizeof(float));
    }

    float **positions = sos->cfg->positions;
    float *source_position= sos->cfg->source_position;
    float *reference_position= sos->cfg->reference_position;

    float* dist_to_source = (float*)malloc(N * sizeof(float)); ;
    for (int j = 0; j < N; j++) {
        float sum = 0.0;
        for (int k = 0; k < 3; k++) {
            sum += (positions[j][k] - source_position[k]) * (positions[j][k] - source_position[k]);
        }
        dist_to_source[j] = sqrt(sum);
    }

    float dist_ref_to_source = 0.0;
    for (int k = 0; k < 3; k++) {
        dist_ref_to_source += (reference_position[k] - source_position[k]) * (reference_position[k] - source_position[k]);
    }
    dist_ref_to_source = sqrt(dist_ref_to_source);

    float *delta_d = (float*)malloc(N * sizeof(float)); 
    for (int j = 0; j < N; j++) {
        delta_d[j] = dist_to_source[j] - dist_ref_to_source;
    }

    float *frequencies = (float*)malloc(n_bin * sizeof(float)); 
    for (int i = 0; i < n_bin; i++) {
        frequencies[i] = (i < n_bin/2) ? (i * sample_rate / n_bin) : (-(n_bin - i) * sample_rate / n_bin);
    }

    for (int i = 0; i < n_bin; i++) {
        float freq = fabs(frequencies[i]);
        for (int j = 0; j < N; j++) { 
            float position_weight;
            if (delta_d[j] < 0) {
                position_weight = 0.5;
            } else {
                position_weight = 1.0 + 1.0 * delta_d[j];
            }

            float freq_weight;
            if (freq < 31) freq_weight = 1e-2;
            else if (freq < 63) freq_weight = 1e-2;
            else if (freq < 125) freq_weight = 1e-2;
            else if (freq < 250) freq_weight = 1e-1;
            else if (freq < 500) freq_weight = 5e-1;
            else if (freq < 1000) freq_weight = 1e+0;
            else if (freq < 2000) freq_weight = 1e+0;
            else freq_weight = 1e+0;

            weights[i][j] = position_weight * freq_weight;
        }
    }
    wtk_free(dist_to_source);
    wtk_free(delta_d);
}


sfs_linear_conv_t* conv_new(int L1, int L2, int n) {
    sfs_linear_conv_t* ctx = (sfs_linear_conv_t*)wtk_malloc(sizeof(sfs_linear_conv_t));
    int i;

    ctx->n = n;
    ctx->L1 = L1;
    ctx->L2 = L2;
    ctx->L = L1 + L2;
    ctx->cache = (float**)wtk_malloc(sizeof(float*)*n);
    ctx->cache_x = (float**)wtk_malloc(sizeof(float*)*n);
    ctx->fft_n = ctx->L/2 + 1;
    ctx->fft_buf = (wtk_complex_t*)wtk_calloc(ctx->L/2 + 1, sizeof(wtk_complex_t));
    ctx->drft = wtk_drft_new(ctx->L);
    ctx->weight = (float**)wtk_malloc(n * sizeof(float*));
    ctx->WEIGHT = wtk_complex_new_p2_2(ctx->n, ctx->fft_n);
    ctx->output_buf = (float**)wtk_malloc(n * sizeof(float*));
    ctx->output = (float**)wtk_malloc(n * sizeof(float*));
    for(i = 0; i < n; i++){
        ctx->cache[i] = (float*)wtk_calloc(ctx->L2, sizeof(float));
        ctx->cache_x[i] = (float*)wtk_calloc(ctx->L, sizeof(float));
        ctx->weight[i] = (float*)wtk_calloc(ctx->L, sizeof(float));
        ctx->output_buf[i] = (float*)wtk_calloc(L1 + L2, sizeof(float));
        ctx->output[i] = ctx->output_buf[i] + L2;
    }
    return ctx;
}

void conv_delete(sfs_linear_conv_t *ctx){
    int i;

    for(i = 0; i < ctx->n; i++){
        wtk_free(ctx->cache[i]);
        wtk_free(ctx->cache_x[i]);
        wtk_free(ctx->output_buf[i]);
        if(ctx->weight){
            wtk_free(ctx->weight[i]);
        }
    }
    wtk_free(ctx->cache);
    wtk_free(ctx->cache_x);
    wtk_free(ctx->output_buf);
    if(ctx->weight){
        wtk_free(ctx->weight);
    }
    wtk_free(ctx->output);
    wtk_free(ctx->fft_buf);
    wtk_complex_delete_p2_2(ctx->WEIGHT);
    wtk_drft_delete(ctx->drft);
    wtk_free(ctx);
}

sfs_linear_conv_t *conv_load(wtk_strbuf_t* buf){
    sfs_linear_conv_t* ctx = (sfs_linear_conv_t*)wtk_malloc(sizeof(sfs_linear_conv_t));
    int i,j;
    int *data = (int*)buf->data;
    ctx->n = data[0];
    ctx->L1 = data[1];
    ctx->L2 = data[2];
    data = data + 3;

    ctx->L = ctx->L1 + ctx->L2;
    ctx->fft_n = ctx->L/2 + 1;
    ctx->weight = NULL;
    ctx->fft_n = ctx->L/2 + 1;

    float *weight_real = (float*)data;
    float *weight_imag = weight_real + ctx->n * (ctx->L/2 + 1);

    ctx->fft_buf = (wtk_complex_t*)wtk_calloc(ctx->L/2 + 1, sizeof(wtk_complex_t));
    ctx->drft = wtk_drft_new(ctx->L);

    ctx->cache = (float**)wtk_malloc(sizeof(float*)*ctx->n);
    ctx->cache_x = (float**)wtk_malloc(sizeof(float*)*ctx->n);
    ctx->WEIGHT = wtk_complex_new_p2_2(ctx->n, ctx->fft_n);
    ctx->output_buf = (float**)wtk_malloc(ctx->n * sizeof(float*));
    ctx->output = (float**)wtk_malloc(ctx->n * sizeof(float*));

    for(i = 0; i < ctx->n; i++){
        ctx->cache[i] = (float*)wtk_calloc(ctx->L2, sizeof(float));
        ctx->cache_x[i] = (float*)wtk_calloc(ctx->L, sizeof(float));
        for(j = 0; j < ctx->fft_n; j++,weight_real++,weight_imag++){
            ctx->WEIGHT[i][j].a = *weight_real;
            ctx->WEIGHT[i][j].b = *weight_imag;
        }
        ctx->output_buf[i] = (float*)wtk_calloc(ctx->L, sizeof(float));
        ctx->output[i] = ctx->output_buf[i] + ctx->L2;
    }
    return ctx;
}

void conv_reset_weight2(sfs_linear_conv_t* ctx, float* weight1,float *weight2) {
    int i;
    float *weight[2];
    weight[0] = weight1;
    weight[1] = weight2;
    for(i = 0; i < ctx->n; i++){
        memset(ctx->weight[i], 0, ctx->L * sizeof(float));
        memcpy(ctx->weight[i], weight[i], (ctx->L2 * sizeof(float)));//TODO
        wtk_drft_fft2_x(ctx->drft, ctx->weight[i], ctx->WEIGHT[i]);
        for(int j = 0; j < ctx->L/2 + 1; j++){
            ctx->WEIGHT[i][j].a *= ctx->L;
            ctx->WEIGHT[i][j].b *= ctx->L;
        }
    }
    //complex_dump(ctx->WEIGHT, ctx->L/2 + 1);
}

void conv_process(sfs_linear_conv_t* ctx, float* input) {
    float A, B, C;
    wtk_complex_t* p1, *p2;
    int i;

    for(i = 0; i < ctx->n; i++){
        memcpy(ctx->cache_x[i], ctx->cache[i], ctx->L2 * sizeof(float));
        memcpy(ctx->cache_x[i] + ctx->L2, input, ctx->L1 * sizeof(float));
        memcpy(ctx->cache[i], ctx->cache_x[i] + ctx->L1, ctx->L2 * sizeof(float));
        wtk_drft_fft2_x(ctx->drft, ctx->cache_x[i], ctx->fft_buf);
        for(int j = 0; j < ctx->L/2 + 1; j++){
            //ctx->fft_buf[i].a *= ctx->L;
            //ctx->fft_buf[i].b *= ctx->L;
            p1 = ctx->fft_buf + j;
            p2 = ctx->WEIGHT[i] + j;
            A = (p1->a + p1->b) * p2->a;
            B = (p2->a + p2->b) * p1->b;
            C = (p1->b - p1->a) * p2->b;
            ctx->fft_buf[j].a = (A - B) * ctx->L;
            ctx->fft_buf[j].b = (B - C) * ctx->L;
        }
        //complex_dump(ctx->WEIGHT[i],20);
        //complex_dump(ctx->fft_buf,20);
        wtk_drft_ifft2_x(ctx->drft, ctx->fft_buf, ctx->output_buf[i]);
        for(int j = ctx->L2; j < ctx->L; j++){
            ctx->output_buf[i][j] /= ctx->L;
        }
    }
}

qtk_soundfield_syntheis_t *qtk_soundfield_syntheis_new(qtk_soundfield_syntheis_cfg_t *cfg){
	qtk_soundfield_syntheis_t *sos = wtk_malloc(sizeof(qtk_soundfield_syntheis_t));
    sos->cfg = cfg;
    int i;
    sos->input = wtk_strbuf_new(1024,1);
    sos->output_buf = wtk_calloc(cfg->N * cfg->hop_size, sizeof(float));
	sos->output = (short **)wtk_malloc(sizeof(short*) * cfg->N);
    for(i = 0; i < cfg->N; i++){
        sos->output[i] = (short *)wtk_malloc(sizeof(short) * cfg->hop_size);
    }

    if(cfg->lp){
        sos->spliter = conv_new(cfg->hop_size, cfg->lp->len, 2);
        conv_reset_weight2(sos->spliter,cfg->lp->p,cfg->hp->p);
    }

    if(cfg->weight_buf){
        sos->weight = conv_load(cfg->weight_buf);
    }
	return sos;
}


void qtk_soundfield_syntheis_delete(qtk_soundfield_syntheis_t *sos){
    int i;
    wtk_strbuf_delete(sos->input);
    wtk_free(sos->output_buf);
    for(i = 0; i < sos->cfg->N; i++){
        wtk_free(sos->output[i]);
    }
    conv_delete(sos->weight);
    conv_delete(sos->spliter);
    wtk_free(sos->output);
    wtk_free(sos);
}

void qtk_soundfield_syntheis_reset(qtk_soundfield_syntheis_t *sos){

}

static void process_frame_(qtk_soundfield_syntheis_t *sos){
    float *data = (float*)sos->input->data,*frame_lowpass,*frame_highpass;
    int i,j;
    if(sos->cfg->bandsplit_on){
        //print_float(data,256);
        conv_process(sos->spliter,data);
        frame_lowpass = sos->spliter->output[0];
        frame_highpass = sos->spliter->output[1];
        //print_float(frame_highpass,256);
        //wtk_debug("%d\n",sos->spliter->L1);
        conv_process(sos->weight,frame_lowpass);
        for(i = 0; i < sos->weight->n; i++){
            for(j = 0; j < sos->weight->L1; j++){
                sos->weight->output[i][j] += frame_highpass[j];
                if(sos->weight->output[i][j] > 1){
                    sos->weight->output[i][j] = 0.9999;
                }
                sos->output[i][j] = sos->weight->output[i][j] * 32768.0;
            }
        }
    }else{
        conv_process(sos->weight, data);
    }
}

void qtk_soundfield_syntheis_feed(qtk_soundfield_syntheis_t *sos, short *data, int len){

    wtk_strbuf_t *input = sos->input;
    int i;
    float fv;
    for(i = 0;i < len;++i)
    {
        fv = data[i]/32768.0;
        wtk_strbuf_push(input,(char *)(&fv),sizeof(float));
    }

    int wav_len = input->pos/sizeof(float);
    int fsize = sos->cfg->hop_size;
    while(wav_len > fsize){
        process_frame_(sos);

        if(sos->notify){
            sos->notify(sos->upval, sos->output, sos->cfg->hop_size);
        }

        wtk_strbuf_pop(input, NULL, sos->cfg->hop_size * sizeof(float));
        wav_len = input->pos/sizeof(float);
    }
}

void qtk_soundfield_syntheis_set_notify(qtk_soundfield_syntheis_t *sos, void *upval, qtk_soundfield_syntheis_notify_f notify){
    sos->upval = upval;
    sos->notify = notify;
}