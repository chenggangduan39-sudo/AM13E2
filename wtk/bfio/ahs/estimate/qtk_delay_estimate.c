#include "qtk_delay_estimate.h"

qtk_delay_estimate_t *qtk_delay_estimate_new(qtk_rir_estimate_cfg_t *cfg){
    qtk_delay_estimate_t *d_est = (qtk_delay_estimate_t*)wtk_malloc(sizeof(qtk_delay_estimate_t));

    d_est->cfg = cfg;
    d_est->gcc_est = NULL;
    d_est->rir_est = NULL;
    d_est->wn_est = NULL;
    d_est->rir_est2 = NULL;
    d_est->rir = NULL;
    d_est->rir_len = 0;

    switch (cfg->est_type)
    {
    case WTK_RIR_ESTIMATE:
        d_est->rir_est = qtk_rir_estimate_new(cfg,cfg->hop_size,cfg->rate);
        break;
    case WTK_WHITE_NOISE_ESTIMATE:
        d_est->wn_est = qtk_white_noise_estimate_new(cfg);
        break;
    case WTK_GCC_ESTIMATE:
        d_est->gcc_est = qtk_gcc_estimate_new(cfg->rate);
        break;
    case WTK_RIR_ESTIMATE2:
        d_est->rir_est2 = qtk_rir_estimate2_new(&cfg->sweep2);
        break;
    default:
        break;
    }
    d_est->recommend_delay = 0;
    return d_est;
}

void qtk_delay_estimate_delete(qtk_delay_estimate_t *d_est){
    if(d_est->rir_est){
       qtk_rir_estimate_delete(d_est->rir_est);
    }
    if(d_est->wn_est){
       qtk_white_noise_estimate_delete(d_est->wn_est);
    }
    if(d_est->gcc_est){
       qtk_gcc_estimate_delete(d_est->gcc_est);
    }
    if (d_est->rir_est2) {
        qtk_rir_estimate2_delete(d_est->rir_est2);
    }
    wtk_free(d_est);
}

void qtk_delay_estimate_reset(qtk_delay_estimate_t *d_est){
    d_est->recommend_delay = 0;
    if(d_est->rir_est){
       qtk_rir_estimate_reset(d_est->rir_est);
    }
    if(d_est->wn_est){
       qtk_white_noise_estimate_reset(d_est->wn_est);
    }
    if(d_est->gcc_est){
       qtk_gcc_estimate_reset(d_est->gcc_est);
    }
    if (d_est->rir_est2) {
        qtk_rir_estimate2_reset(d_est->rir_est2);
    }
}

int qtk_delay_estimate_feed_float(qtk_delay_estimate_t *d_est, float *data, int len, int is_end){
    switch (d_est->cfg->est_type)
    {
    case WTK_RIR_ESTIMATE:
        //qtk_rir_estimate_feed_float(d_est->rir_est,data,len,is_end);
        break;
    case WTK_WHITE_NOISE_ESTIMATE:
        qtk_white_noise_estimate_feed_float(d_est->wn_est,data,len,is_end);
        break;
    case WTK_GCC_ESTIMATE:
        qtk_gcc_estimate_feed_float(d_est->gcc_est,data,len,is_end);
        break;
    case WTK_RIR_ESTIMATE2:
        qtk_rir_estimate2_feed_float(d_est->rir_est2, data, len, is_end);
        break;
    default:
        break;
    }

    return 0;
}

int qtk_delay_estimate(qtk_delay_estimate_t *d_est){
    switch (d_est->cfg->est_type)
    {
    case WTK_RIR_ESTIMATE:
        qtk_estimate_rir(d_est->rir_est);
        d_est->recommend_delay = d_est->rir_est->recommend_delay;
        break;
    case WTK_WHITE_NOISE_ESTIMATE:
        qtk_white_noise_estimate(d_est->wn_est);
        d_est->recommend_delay = d_est->wn_est->recommend_delay;
        d_est->rir = d_est->wn_est->res;
        break;
    case WTK_GCC_ESTIMATE:
        qtk_gcc_estimate(d_est->gcc_est);
        d_est->recommend_delay = d_est->gcc_est->recommend_delay;
        //d_est->rir = d_est->gcc_est->res;
        break;
    case WTK_RIR_ESTIMATE2:
        qtk_estimate2_rir(d_est->rir_est2);
        d_est->recommend_delay =
            d_est->rir_est2->recommend_delay[0] -
            d_est->rir_est2->recommend_delay
                [1]; // mic信号的直达声索引减去扫频信号的直达声索引，等于延时
        d_est->rir = d_est->rir_est2->res;
        d_est->rir_len = d_est->rir_est2->res_len;
    default:
        break;
    }
    FILE *fp = fopen("rir.txt", "w");
    for (int i = 0; i < d_est->rir_len; i++) {
        fprintf(fp, "%f\n", d_est->rir[i]);
    }
    printf("rir_len:%d\n", d_est->rir_len);
    fclose(fp);
    return d_est->recommend_delay;
}

int qtk_delay_estimate_feed(qtk_delay_estimate_t *d_est, short *data, int len, int is_end){

    switch (d_est->cfg->est_type)
    {
    case WTK_RIR_ESTIMATE:
        qtk_rir_estimate_feed2(d_est->rir_est,data,len,is_end);
        break;
    case WTK_WHITE_NOISE_ESTIMATE:
        qtk_white_noise_estimate_feed(d_est->wn_est,data,len,is_end);
        break;
    case WTK_GCC_ESTIMATE:
        qtk_gcc_estimate_feed(d_est->gcc_est,data,len,is_end);
        break;
    case WTK_RIR_ESTIMATE2:
        qtk_rir_estimate2_feed(d_est->rir_est2, data, len, is_end);
        break;
    default:
        break;
    }

    return 0;
}

int qtk_delay_estimate_playdata(qtk_delay_estimate_t *d_est, short **data)
{
    if(d_est->cfg->est_type == WTK_RIR_ESTIMATE2)
    {
        qtk_estimate2_code_generate(d_est->rir_est2);
        *data = d_est->rir_est2->pv_code;
        return d_est->rir_est2->cfg->code_len;
    }

    return -1;
}
