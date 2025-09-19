#include "qtk_erb.h"


static float _hz2erb(float freq_hz){
    return 24.7 * log10f(0.00437 * freq_hz + 1);
}

static float _erb2hz(float erb_f){
    return (powf(10, (erb_f / 24.7)) - 1) / 0.00437;
}

static void _erb_filter_init(qtk_ahs_erb_t *erb){
    qtk_ahs_erb_cfg_t *cfg = erb->cfg;
    float low_lim = cfg->erb_subband_1 * 1.0/cfg->nfft * cfg->fs;
    int i;
    float erb_low = _hz2erb(low_lim);
    float erb_high = _hz2erb(cfg->high_lim);
    float *erb_points = wtk_malloc(sizeof(float) * (cfg->erb_subband_2));
    int *bins = wtk_malloc(sizeof(float) * (cfg->erb_subband_2));

    float step = (erb_high - erb_low) / (cfg->erb_subband_2 - 1);
    float *p = erb_points;
    *p = erb_low;
    p++;
    for(i = 1; i < cfg->erb_subband_2 - 1; i++){
        *p = *(p - 1) + step;
        p++;
    }
    *p = erb_high;
    //print_float(erb_points,cfg->erb_subband_2);
    p = erb_points;
    int *ip = bins;
    for(i = 0; i < cfg->erb_subband_2; i++){
        *ip = (int)(_erb2hz(*p)/cfg->fs * cfg->nfft + 0.5);
        p++;
        ip++;
    }
    //print_int(bins,cfg->erb_subband_2);
    int row = cfg->erb_subband_2;
    int col = cfg->nfft/2 + 1;
    erb->filters = wtk_calloc(row * col, sizeof(float));
    p = erb->filters + bins[0];
    for(i = bins[0]; i < bins[1]; i++){
        *p = (bins[1] - i + 1e-12) / (bins[1] - bins[0] + 1e-12);
        p++;
    }

    float *s;
    int n,j;
    for(i = 0; i < cfg->erb_subband_2 - 2; i++){
        s = erb->filters + (i + 1) * col + bins[i];
        n = bins[i + 1] - bins[i];
        for(j = 0; j < n; j++){
            *s = (bins[i] + j - bins[i] + 1e-12)/(bins[i + 1] - bins[i] + 1e-12);
            s++;
        }

        s = erb->filters + (i + 1) * col + bins[i + 1];
        n = bins[i + 2] - bins[i + 1];
        for(j = 0; j < n; j++){
            *s = (bins[i + 2] - (bins[i + 1] + j) + 1e-12)/(bins[i + 2] - bins[i + 1] + 1e-12);
            s++;
        }
    }
    p = erb->filters + col * (row - 1) + bins[row - 2];
    for(i = bins[row - 2],j = 0; i < bins[row - 1] + 1; i++,j++){
        *p = 1 - *(erb->filters + col * (row - 2) + bins[row - 2] + j);
        p++;
    }

    p = erb->filters;
    // for(i = 0; i < row; i++){
    //     for(j = 0; j < col; j++){0
    //         printf("%f ",*p);
    //         p++;
    //     }
    //     printf("\n");
    // }
    // exit(0);

    for(i = 0; i < erb->erb_fc_shape[0]; i++){
        memcpy(erb->erb_fc + i * erb->erb_fc_shape[1], erb->filters + i * col + cfg->erb_subband_1, sizeof(float) * erb->erb_fc_shape[1]);
    }

    //TODO trans erb to ierb
    p  = erb->ierb_fc;
    for(i = 0; i < erb->ierb_fc_shape[0]; i++){
        for(j = 0; j < erb->ierb_fc_shape[1]; j++){
            *p = *(erb->erb_fc + j * erb->erb_fc_shape[1] + i);
            p++;
        }
    }

    wtk_free(bins);
    wtk_free(erb_points);
}

qtk_ahs_erb_t *qtk_ahs_erb_new(qtk_ahs_erb_cfg_t *cfg){
    qtk_ahs_erb_t *erb = wtk_malloc(sizeof(qtk_ahs_erb_t));
    erb->cfg = cfg;
    int nfreqs = cfg->nfft / 2 + 1;

    erb->erb_fc_shape[0] = cfg->erb_subband_2;
    erb->erb_fc_shape[1] = nfreqs - cfg->erb_subband_1;
    erb->erb_fc = wtk_malloc(sizeof(float) * erb->erb_fc_shape[0] * erb->erb_fc_shape[1]);
    erb->ierb_fc_shape[0] = nfreqs - cfg->erb_subband_1;
    erb->ierb_fc_shape[1] = cfg->erb_subband_2;
    erb->ierb_fc = wtk_malloc(sizeof(float) * erb->ierb_fc_shape[0] * erb->ierb_fc_shape[1]);
    _erb_filter_init(erb);
    erb->n = cfg->erb_subband_1 + cfg->erb_subband_2;
    erb->nfreqs = nfreqs;
    return erb;
}

void qtk_ahs_erb_delete(qtk_ahs_erb_t *erb){
    wtk_free(erb->filters);
    wtk_free(erb->erb_fc);
    wtk_free(erb->ierb_fc);
    wtk_free(erb);
}
void qtk_ahs_erb_reset(qtk_ahs_erb_t *erb){

}

float *data_read(){

    FILE *fp;
    int len = 3 * 129;
    float *data;
    fp = fopen("mic.bin","rb");
    data = (float*)malloc(len*sizeof(float));
    fread(data,sizeof(float),len,fp);
    fclose(fp);
    return data;
}

float *data_read2(){

    FILE *fp;
    int len = 129;
    float *data;
    fp = fopen("ref.bin","rb");
    data = (float*)malloc(len*sizeof(float));
    fread(data,sizeof(float),len,fp);
    fclose(fp);
    return data;
}


void qtk_ahs_erb_bm(qtk_ahs_erb_t *erb, float *in, float *out, int cnt){
    // float *tmp1 = data_read();
    // float *tmp2 = data_read2();
    // memcpy(in,tmp1,sizeof(float) * 129 * 3);
    // memcpy(in + 129 * 3,tmp2,sizeof(float) * 129);

    memset(out,0,sizeof(float) * erb->n * cnt);
    int i,m,n;
    float *p,*p1,*p2;
    for(i = 0; i < cnt; i++){
        memcpy(out + i * erb->n,in + i * erb->nfreqs,sizeof(float) * erb->cfg->erb_subband_1);
        p = out + i*erb->n + erb->cfg->erb_subband_1;
        p2 = erb->erb_fc;
        for(m = 0; m < erb->erb_fc_shape[0];m++){
            p1 = in + i*erb->nfreqs + erb->cfg->erb_subband_1;
            for(n = 0; n < erb->erb_fc_shape[1]; n++){
                *p += *p1 * *p2;
                p1++;
                p2++;
            }
            p++;
        }
    }
    //print_float(out,cnt * erb->n);
}


void qtk_ahs_erb_bs(qtk_ahs_erb_t *erb, float *in, float * out, int cnt){
    int i,m,n;
    float *p,*p1,*p2;
    memset(out,0,sizeof(float) * erb->nfreqs * cnt);
    for(i = 0; i < cnt; i++){
        memcpy(out + i * erb->nfreqs,in + i * erb->n,sizeof(float) * erb->cfg->erb_subband_1);
        p = out + i*erb->nfreqs + erb->cfg->erb_subband_1;
        p2 = erb->ierb_fc;
        for(m = 0; m < erb->ierb_fc_shape[0];m++){
            p1 = in + i*erb->n + erb->cfg->erb_subband_1;
            for(n = 0; n < erb->ierb_fc_shape[1]; n++){
                *p += *p1 * *p2;
                p1++;
                p2++;
            }
            p++;
        }
    }
    //print_float(out,cnt * erb->nfreqs);
}