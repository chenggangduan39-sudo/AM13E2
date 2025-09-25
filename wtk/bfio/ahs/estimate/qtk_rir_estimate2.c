#include "qtk_rir_estimate2.h"

float *qtk_estimate2_code_generate(qtk_rir_estimate2_t *rir_es);

qtk_rir_estimate2_conv1d_t* qtk_rir_estimate2_conv1d_new(int l1 ,int l2, int b)
{
    qtk_rir_estimate2_conv1d_t* conv = (qtk_rir_estimate2_conv1d_t*)wtk_malloc(sizeof(qtk_rir_estimate2_conv1d_t));

    conv->l1 = l1;
    conv->l2 = l2;
    conv->b = b;
    conv->cache = (float*)wtk_calloc((l2 - 1 + l1)*b,sizeof(float));
    conv->weight = (float*)wtk_malloc(sizeof(float)*l2*b);
    conv->out = (float*)wtk_calloc(l1,sizeof(float));
    return conv;
}

void qtk_rir_estimate2_conv1d_delete(qtk_rir_estimate2_conv1d_t* conv)
{
    wtk_free(conv->cache);
    wtk_free(conv->weight);
    wtk_free(conv->out);
    wtk_free(conv);
}

qtk_rir_estimate2_t *qtk_rir_estimate2_new(qtk_rir_estimate2_cfg_t *cfg)
{
    qtk_rir_estimate2_t *rir_es;
    
    rir_es = (qtk_rir_estimate2_t *)wtk_malloc(sizeof(qtk_rir_estimate2_t));
    rir_es->cfg = cfg;

    rir_es->hop_size = cfg->hop_size;
    rir_es->rate = cfg->rate;
    rir_es->log_sweep = wtk_strbufs_new(cfg->channel);
    rir_es->st = -1;
    cfg->st = cfg->st * cfg->rate;
    rir_es->conv_sweep = qtk_rir_estimate2_conv1d_new(rir_es->hop_size,rir_es->rate * cfg->rt60, 1);
    rir_es->recommend_delay = (int*)wtk_malloc(sizeof(int)*cfg->channel);

    rir_es->code=(float *)wtk_malloc(sizeof(float)*cfg->code_len);
    rir_es->r_code=(float *)wtk_malloc(sizeof(float)*cfg->code_len);
    rir_es->pv_code=(short *)wtk_malloc(sizeof(short)*cfg->code_len);
    rir_es->res = NULL;
    rir_es->window=NULL;
    if(cfg->use_hanning){
        rir_es->window=wtk_math_create_hanning_window2(cfg->win_len);
    }

    qtk_rir_estimate2_reset(rir_es);
    return rir_es;
}
void qtk_rir_estimate2_delete(qtk_rir_estimate2_t *rir_es)
{
    wtk_strbufs_delete(rir_es->log_sweep, rir_es->cfg->channel);
    qtk_rir_estimate2_conv1d_delete(rir_es->conv_sweep);
    wtk_free(rir_es->recommend_delay);
    if(rir_es->code){
        wtk_free(rir_es->code);
    }
    if(rir_es->r_code){
        wtk_free(rir_es->r_code);
    }
    wtk_free(rir_es->pv_code);
    if(rir_es->window){
        wtk_free(rir_es->window);
    }
    if (rir_es->res) {
        wtk_free(rir_es->res);
    }

    wtk_free(rir_es);
}
void qtk_rir_estimate2_start(qtk_rir_estimate2_t *rir_es)
{
    qtk_estimate2_code_generate(rir_es);
}
void qtk_rir_estimate2_reset(qtk_rir_estimate2_t *rir_es)
{
    wtk_strbufs_reset(rir_es->log_sweep, rir_es->cfg->channel);
    rir_es->st = -1;
    int code_len=rir_es->cfg->code_len;

    wtk_free(rir_es->res);
    rir_es->res = NULL;
    memset(rir_es->recommend_delay, 0, sizeof(int)*rir_es->cfg->channel);
    memset(rir_es->code, 0, sizeof(float)*code_len);
    memset(rir_es->r_code, 0, sizeof(float)*code_len);
    memset(rir_es->pv_code, 0, sizeof(short)*code_len);
}

static int *topk(float *f, int n, int k) {
    float *p = f;
    float *probs = wtk_calloc(n, sizeof(float));
    int i, j, *indexes = wtk_calloc(n, sizeof(int));
    int cnt = 0;
    for (i = 0; i < n; i++, p++) {
        for (j = 0; j < k; j++) {
            if (fabs(*p) > *(probs + j)) {
                cnt = k - 1 - j;
                if (cnt >= 0) {
                    memmove(probs + j + 1, probs + j, sizeof(float) * cnt);
                    memmove(indexes + j + 1, indexes + j, sizeof(int) * cnt);
                }
                *(probs + j) = fabs(*p);
                *(indexes + j) = i;
                break;
            }
        }
    }
    wtk_free(probs);
    return indexes;
}

static float get_max(float *st, int cnt) {
    float max = 0.0;
    float *p = st;
    int i;
    for (i = 0; i < cnt; i++) {
        if (*p > max) {
            max = *p;
        }
        p++;
    }

    return max;
}

int qtk_estimate2_rir(qtk_rir_estimate2_t *rir_es)
{
    int i,j,n = rir_es->cfg->code_len + rir_es->log_sweep[0]->pos/sizeof(float) - 1,m,t,s,idx;
    rir_es->res_len = n;
    float *a;
    float *b = rir_es->r_code;
    int c;
    float *res = (float*)wtk_calloc(n,sizeof(float));
    float max;
    int fs = rir_es->rate;
    qtk_rir_estimate2_cfg_t *cfg = rir_es->cfg;
    int channel=rir_es->cfg->channel;
    //scipy.signal.convolve deconvlution to get the rir
    for(c=0;c<channel;++c){
        a = (float*)rir_es->log_sweep[c]->data;
        memset(res, 0, sizeof(float)*n);
        max = 0.0;
        idx = 0;
        t = 0;
        for(i = 0; i < n; i++){
            if(i < rir_es->cfg->code_len){
                m = i + 1;
                j = 0;
                s = i;
            }else{
                m = rir_es->cfg->code_len;
                t++;
                j = t;
                s = rir_es->cfg->code_len - 1;
            }
            for(j; j < m; j++,s--){
                res[i] += a[j] * b[s];
            }

            if(fabs(res[i]) > max){
                idx = i;
                max = fabs(res[i]);
            }
        }

        if (c == 0) {
            rir_es->res = (float *)wtk_malloc(sizeof(float) * n);
            // FILE *fp = fopen("rir_sweep2_ch1.txt","w+");
            for (i = 0; i < n; i++) {
                rir_es->res[i] = res[i];
                // fprintf(fp, "%f ", res[i]/max);
            }
            // fclose(fp);
        }

        if (c == 1) {
            // FILE *fp = fopen("rir_sweep2_ch1.txt","w+");
            // FILE *fp1 = fopen("rir_sweep2_ch2.txt","w+");
            int j = idx;
            for (i = 0; j < n && i < n; i++) { //保证i、j不越界
                // fprintf(fp1, "%f ", res[i]/max);
                rir_es->res[i] =
                    rir_es->res[j] /
                    max; //除以扫频信号direct_sound_idx处的值，对rir进行归一化
                ++j;
                // rir_es->res[i] /= max;
                // fprintf(fp, "%f ", rir_es->res[i]);
            }
            // rir_es->res_len = n - idx;
            rir_es->res_len =
                n - idx; //从扫频信号的direct_sound_idx处开始截取rir
            // fclose(fp);
            // fclose(fp1);
        }
        // print_float(res,n);
        // wtk_debug("%d %f\n",idx,max);
        // strip away all harmonic distortion
        // int st = idx - (rir_es->cfg->lookahead * fs/1000.0);
        // for(i=max(idx-rir_es->cfg->rir_lookahead_len, 0);i<idx;++i){
        //     if(res[i] > max * rir_es->cfg->rir_lookahead_thresh){
        //         idx = i;
        //         break;
        //     }
        // }

        i = rir_es->cfg->code_len * 0.01;
        int *idxs = topk(res, rir_es->cfg->code_len, i);
        float thresh = fabs(*(res + *(idxs + i - 1)));
        int a = *(idxs + i);
        int b = rir_es->cfg->code_len - 1;
        i = (((a) < (b)) ? (a) : (b));

        float *p = res;
        for (i = 0; i < *(idxs); i++) {
            if (fabs(*p) < thresh) {
                *p = 0.0;
            } else {
                *p = fabs(*p);
            }
            p++;
        }

        float *st;
        int cnt;
        p = res;
        for (i = 0; i < *(idxs) + 1; i++) {
            if (*p > 1e-3) {
                if ((i - 16) > 0) {
                    st = p - 16;
                    cnt = 16;
                } else {
                    st = p;
                    cnt = i + 1;
                }
                if ((i + 16) > rir_es->cfg->code_len) {
                    cnt += *(idxs)-1 - i;
                } else {
                    cnt += 16;
                }
                if (*p < get_max(st, cnt)) {
                    *p = 0.0;
                }
                if (*p > max) {
                    max = *p;
                    idx = i;
                }
            }
            p++;
        }

        float thresh2 = 0.4 * max;
        int flag = 1;
        p = res;
        for (i = 0; i < *(idxs) + 1; i++) {
            if (*p > thresh2) {
                if (flag) {
                    idx = i;
                    flag = 0;
                }
            } else {
                *p = 0.0;
            }
            p++;
        }

        rir_es->recommend_delay[c] = idx;
        //int ed = (int)(idx + cfg->rir_duration * fs);
        int length = cfg->rt60 * fs;//ed - st;
        //wtk_debug("%d %d\n",st,length);
        int st1 = idx - (rir_es->cfg->lookahead * fs / 1000.0);
        memmove(res, res + st1, length * sizeof(float));

        //normalize rir max_val = np.max(np.abs(signal.convolve(log_sweep, inv_log_sweep)))
        for(i = 0; i < length; i++){
            res[i] /= cfg->max_val;
        }

        memcpy(rir_es->conv_sweep->weight,res,sizeof(float)*length);
    }
    wtk_free(res);

    return 0;
}

int qtk_rir_estimate2_feed(qtk_rir_estimate2_t *rir_es, short *data, int len, int is_end)
{
    int i, j;
    float fv;
    int channel = rir_es->cfg->channel;

    if(rir_es->cfg->st-rir_es->st >= len){
        rir_es->st += len;
    }else if(rir_es->cfg->st-rir_es->st >= 0){
        for(i=0;i<rir_es->cfg->st-rir_es->st; ++i){
            data += channel;
        }
        for(i=rir_es->cfg->st-rir_es->st; i<len; ++i){
            for(j=0;j<channel;++j){
                fv = data[j] * 1.0 / 32768.0;
                wtk_strbuf_push(rir_es->log_sweep[j], (char *)&(fv), sizeof(float));
            }
            data += channel;
        }
        rir_es->st=rir_es->cfg->st;
    }

    if(is_end){
        return qtk_estimate2_rir(rir_es);
    }
    return 0;
}

int qtk_rir_estimate2_feed_float(qtk_rir_estimate2_t *rir_es, float *data, int len, int is_end)
{
    int i, j;
    float fv;
    int channel = rir_es->cfg->channel;

    if(rir_es->cfg->st-rir_es->st >= len){
        rir_es->st += len;
    }else if(rir_es->cfg->st-rir_es->st >= 0){
        for(i=0;i<rir_es->cfg->st-rir_es->st; ++i){
            data += channel;
        }
        for(i=rir_es->cfg->st-rir_es->st; i<len; ++i){
            for(j=0;j<channel; ++j){
                fv = data[j];
                wtk_strbuf_push(rir_es->log_sweep[j], (char *)&(fv), sizeof(float));
            }
            data += channel;
        }
        rir_es->st=rir_es->cfg->st;
    }
    if(is_end){
        return qtk_estimate2_rir(rir_es);
    }
    return 0;
}

float *qtk_estimate2_code_generate(qtk_rir_estimate2_t *rir_es){
    float max_freq=rir_es->cfg->max_freq;
    float min_freq=rir_es->cfg->min_freq;
    float code_tms=rir_es->cfg->code_tms;
    float energy_shift=rir_es->cfg->energy_shift;
    int code_len=rir_es->cfg->code_len;
    int i,j;
    int half_win=rir_es->cfg->win_len>>1;
    float *code=rir_es->code;
    float *r_code=rir_es->r_code;
    short *pv_code=rir_es->pv_code;
    float *window=rir_es->window;
    float k=(max_freq - min_freq) / code_tms;
    double t_step=1.0/(rir_es->cfg->rate * 1.0);
    double t;

    for(i=0;i<code_len;++i){
        t = t_step * i;
        code[i] = cos(2*PI*(min_freq*t + 0.5 * k * (t * t)));
    }
    for(i=0;i<code_len;++i){
        code[i] *= energy_shift;
    }

    if(rir_es->window){
        for(i=0;i<half_win;++i){
            code[i] = code[i] * window[i];
        }
        for(i=code_len-half_win,j=half_win;i<code_len;++i,++j){
            code[i] = code[i] * window[j];
        }
    }
    for(i=0;i<code_len;++i){
        r_code[i] = code[code_len-i-1];
    }
    for(i=0;i<code_len;++i){
        if(code[i]>0){
            pv_code[i] = code[i] * 32767.0;
        }else{
            pv_code[i] = code[i] * 32768.0;
        }
    }
    // print_float(r_code,code_len);

    return code;
}
