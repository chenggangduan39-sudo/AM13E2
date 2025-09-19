#include "qtk_rir_estimate.h"

qtk_rir_estimate_conv1d_t* qtk_rir_estimate_conv1d_new(int l1 ,int l2, int b)
{
    qtk_rir_estimate_conv1d_t* conv = (qtk_rir_estimate_conv1d_t*)wtk_malloc(sizeof(qtk_rir_estimate_conv1d_t));

    conv->l1 = l1;
    conv->l2 = l2;
    conv->b = b;
    conv->cache = (float*)wtk_calloc((l2 - 1 + l1)*b,sizeof(float));
    conv->weight = (float*)wtk_malloc(sizeof(float)*l2*b);
    conv->out = (float*)wtk_calloc(l1,sizeof(float));
    return conv;
}

qtk_rir_estimate_conv1d_t* qtk_rir_estimate_conv1d_new2(qtk_rir_estimate_t *rir_est,int l1 ,char *fn, int b)
{
    qtk_rir_estimate_conv1d_t* conv = (qtk_rir_estimate_conv1d_t*)wtk_malloc(sizeof(qtk_rir_estimate_conv1d_t));
    FILE *file = fopen(fn,"rb");

    fread(&rir_est->recommend_delay,sizeof(int),1,file);
    fread(&conv->l2,sizeof(int),1,file);
    conv->l1 = l1;
    conv->b = b;
    conv->cache = (float*)wtk_calloc((conv->l2 - 1 + l1)*b,sizeof(float));
    conv->weight = (float*)wtk_malloc(sizeof(float)*conv->l2*b);
    fread(conv->weight,sizeof(float),conv->l2*b,file);
    fclose(file);
    conv->out = (float*)wtk_calloc(l1,sizeof(float));
    return conv;
}

void qtk_rir_estimate_conv1d_delete(qtk_rir_estimate_conv1d_t* conv)
{
    wtk_free(conv->cache);
    wtk_free(conv->weight);
    wtk_free(conv->out);
    wtk_free(conv);
}

int qtk_rir_estimate_ref_write(qtk_rir_estimate_t* rir_est, char *fn){
    FILE *file = fopen(fn,"wb");
    int ret = -1;

    if(!file){
        goto end;
    }
    ret = fwrite(&rir_est->recommend_delay,sizeof(int),1,file);
    if(ret != 1){
        ret=-1;
        goto end;
    }
    ret = fwrite(&rir_est->conv_sweep->l2,sizeof(int),1,file);
    if(ret != 1){
        ret=-1;
        goto end;
    }

    ret = fwrite(rir_est->conv_sweep->weight,sizeof(float),rir_est->conv_sweep->l2,file);
    if(ret != 1){
        ret = -1;
        goto end;
    }
    fclose(file);
end:
    return ret;
}

int qtk_rir_estimate_ref_load(qtk_rir_estimate_t* rir_est, char *fn){
    if(rir_est->conv_sweep){
        qtk_rir_estimate_conv1d_delete(rir_est->conv_sweep);
    }
    rir_est->conv_sweep = qtk_rir_estimate_conv1d_new2(rir_est,rir_est->hop_size, fn, 1);
    return 0;
}

void qtk_rir_estimate_conv1d_calc(qtk_rir_estimate_t* rir_est, float *input){
    qtk_rir_estimate_conv1d_t* conv = rir_est->conv_sweep;
    int i,j;
    float *p = conv->out;
    float *weight,*in;
    memcpy(conv->cache + conv->l2 - 1,input,sizeof(float)*conv->l1);
    for(i = 0; i < conv->l1; i++){
        weight = conv->weight;
        in = conv->cache + i;
        for(j = 0; j < conv->l2; j++){
            *p += *in * (*weight);
            in++;
            weight++;
        }
        p++;
    }

    for(i = 0; i < 128; i++){
        printf("%f %f %f\n",conv->out[i] - input[i], conv->out[i], input[i]);
    }

    memmove(conv->cache,conv->cache + conv->l1,sizeof(float)*(conv->l2 - 1));
    memcpy(input,conv->out,sizeof(float)*conv->l1);
    memset(conv->out,0,sizeof(float)*conv->l1);
}

qtk_rir_estimate_t *qtk_rir_estimate_new(qtk_rir_estimate_cfg_t *cfg, int hop_size, int rate)
{
    qtk_rir_estimate_t *rir_es;
    
    rir_es = (qtk_rir_estimate_t *)wtk_malloc(sizeof(qtk_rir_estimate_t));
    rir_es->cfg = cfg;

    rir_es->hop_size = hop_size;
    rir_es->rate = rate;
    rir_es->log_sweep = wtk_strbuf_new(1024,1);
    rir_es->st = -1;
    cfg->st = cfg->st * rate;
    if(cfg->ref_fn){
        rir_es->conv_sweep = qtk_rir_estimate_conv1d_new2(rir_es,rir_es->hop_size,cfg->ref_fn, 1);
    }else{
        rir_es->conv_sweep = qtk_rir_estimate_conv1d_new(rir_es->hop_size,rir_es->rate * cfg->rt60, 1);
    }

    qtk_rir_estimate_reset(rir_es);
    return rir_es;
}
void qtk_rir_estimate_delete(qtk_rir_estimate_t *rir_es)
{
    wtk_strbuf_delete(rir_es->log_sweep);
    qtk_rir_estimate_conv1d_delete(rir_es->conv_sweep);

    wtk_free(rir_es);
}
void qtk_rir_estimate_start(qtk_rir_estimate_t *rir_es)
{

}
void qtk_rir_estimate_reset(qtk_rir_estimate_t *rir_es)
{
    wtk_strbuf_reset(rir_es->log_sweep);
    rir_es->st = -1;
}

int qtk_estimate_rir(qtk_rir_estimate_t *rir_es)
{
    int i,j,n = rir_es->cfg->nsweep + rir_es->log_sweep->pos/sizeof(float) - 1,m,t = 0,s,idx = 0;
    float *a = (float*)rir_es->log_sweep->data;
    float *b = rir_es->cfg->inv_log_sweep;
    float *res = (float*)wtk_calloc(n,sizeof(float));
    float max = 0.0;
    int fs = rir_es->rate;
    qtk_rir_estimate_cfg_t *cfg = rir_es->cfg;
    //scipy.signal.convolve deconvlution to get the rir
    for(i = 0; i < n; i++){
        if(i < rir_es->cfg->nsweep){
            m = i + 1;
            j = 0;
            s = i;
        }else{
            m = rir_es->cfg->nsweep;
            t++;
            j = t;
            s = rir_es->cfg->nsweep - 1;
        }
        for(j; j < m; j++,s--){
            res[i] += a[j] * b[s];
        }

        if(fabs(res[i]) > max){
            idx = i;
            max = fabs(res[i]);
        }
    }
    //print_float(res,n);
    //wtk_debug("%d %f\n",idx,max);
    //strip away all harmonic distortion
    int st = idx - (rir_es->cfg->lookahead * fs/1000.0);
    rir_es->recommend_delay = idx - (8 * fs);
    //int ed = (int)(idx + cfg->rir_duration * fs);
    int length = cfg->rt60 * fs;//ed - st;
    //wtk_debug("%d %d\n",st,length);
    memmove(res, res+st, length *sizeof(float));

    //normalize rir max_val = np.max(np.abs(signal.convolve(log_sweep, inv_log_sweep)))
    for(i = 0; i < length; i++){
        res[i] /= cfg->max_val;
    }

    memcpy(rir_es->conv_sweep->weight,res,sizeof(float)*length);
    wtk_free(res);

    return 0;
}

int qtk_rir_estimate_feed(qtk_rir_estimate_t *rir_es, short *data, int len, int is_end)
{
    int i;
    float fv;
    int channel = 1;
    for (i = 0; i < len; ++i) {
        if(rir_es->st > rir_es->cfg->st){
            fv = data[0] * 1.0 / 32768.0;
            wtk_strbuf_push(rir_es->log_sweep, (char *)&(fv), sizeof(float));
        }else{
            rir_es->st++;
        }
        data += channel;
    }

    if(is_end){
        return qtk_estimate_rir(rir_es);
    }
    return 0;
}

int qtk_rir_estimate_feed_float(qtk_rir_estimate_t *rir_es, float *data, int len, int is_end)
{

    int i;
    float fv;
    int channel = 1;
    for (i = 0; i < len; ++i) {
        if(rir_es->st > rir_es->cfg->st){
            fv = data[0];
            wtk_strbuf_push(rir_es->log_sweep, (char *)&(fv), sizeof(float));
        }else{
            rir_es->st++;
        }
        data += channel;
    }

    if(is_end){
        return qtk_estimate_rir(rir_es);
    }
    return 0;
}

int qtk_rir_estimate_feed2(qtk_rir_estimate_t *rir_es, short *data, int len, int is_end)
{
    int i;
    float fv;
    int channel = 2;
    for (i = 0; i < len; ++i) {
        if(rir_es->st > rir_es->cfg->st){
            fv = data[0] * 1.0 / 32768.0;
            wtk_strbuf_push(rir_es->log_sweep, (char *)&(fv), sizeof(float));
        }else{
            rir_es->st++;
        }
        data += channel;
    }
    return 0;
}