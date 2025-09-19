#include "wtk_resample2.h"

// 计算零阶修正Bessel函数 I0(x)
static float modified_bessel0(float x) {
    float sum = 1.0f; // 第0项为1
    float term = 1.0f;
    float x_squared = x * x;
    int i = 1;

    // 使用幂级数展开公式计算 I0(x)
    while (term > 1e-6f) { // 设置一个阈值来决定何时停止迭代
        term *= x_squared / (4 * i * i);
        sum += term;
        i++;
    }

    return sum;
}

/* Kaiser window calculation */
static float* kaiser(int M, float beta) {
    float* window = (float*)wtk_malloc(M * sizeof(float));
    memset(window, 0, M * sizeof(float));
    float alpha = (M - 1) / 2.0;
    float denominator = modified_bessel0(beta); // 需要实现贝塞尔函数

    for (int n = 0; n < M; n++) {
        float t = (n - alpha) / alpha;
        window[n] = modified_bessel0(beta * sqrt(1 - t*t)) / denominator;
    }
    return window;
}


/* Prototype filter design */
static float* design_prototype_filter(int taps, float cutoff_ratio, float beta) {
    if(taps %2 != 0){
        wtk_debug("taps must be even\n");
        exit(0);
    }
    if(cutoff_ratio <= 0 || cutoff_ratio >= 1){
        wtk_debug("cutoff_ratio must be in (0, 1)\n");
        exit(0);
    }
    int len = taps + 1;
    float* h = (float*)wtk_malloc(len * sizeof(float));
    memset(h, 0, len * sizeof(float));
    
    // 生成初始滤波器
    float omega_c = PI * cutoff_ratio;
    for (int i = 0; i < len; i++) {
        float t = i - 0.5 * taps;
        h[i] = (t == 0) ? cutoff_ratio : 
               sin(omega_c * t) / (PI * t);
    }
    // print_float(h, len);
    
    //应用Kaiser窗
    float* w = kaiser(len, beta);
    // print_float(w, len);
    for (int i = 0; i < len; i++) {
        h[i] *= w[i];
    }
    wtk_free(w);
    return h;
}

float *hstack(float *a, int len, int zero_len){
    // print_float(a, len);
    float *b = (float*)wtk_malloc((len+zero_len) * sizeof(float));
    memset(b, 0, (len+zero_len) * sizeof(float));
    memcpy(b, a, len * sizeof(float));
    // print_float(b, len+zero_len);
    return b;
}

void Linear_Conv(wtk_resample2_t *r, float *weight, int weight_len){
    // int N = frm_size + weight_len + 1;//+1是因为要保证为2的幂次方
    // int N = r->cfg->window_sz;
    int window_sz = r->cfg->window_sz;
    int nbin = window_sz/2+1;
    wtk_complex_t cpx;
    float *weight_hstack = hstack(weight, weight_len, window_sz-weight_len);
    // print_float(weight_hstack, window_sz);
    float *F = (float*)wtk_malloc(window_sz * sizeof(float));
    memset(F, 0, window_sz * sizeof(float));
    wtk_rfft_process_fft(r->fft, F, weight_hstack);
    for (int i = 0; i < nbin; i++) {
        cpx = wtk_rfft_get_value(F, window_sz, i);
        r->weight[i].a = cpx.a;
        r->weight[i].b = cpx.b;
        // printf("idx %d %f %f\n",i, cpx.a, cpx.b);
    }   

    wtk_free(F);
    wtk_free(weight_hstack);
}

void upsamper_weight(wtk_resample2_t *r)
{
    wtk_resample2_cfg_t *cfg = r->cfg;
    int taps = cfg->taps;
    int window_sz = cfg->window_sz;
    int weight_len = taps+1;
    int rate = cfg->SR < cfg->new_SR ? cfg->upsamp_rate : cfg->downsamp_rate;
    int frm_size = cfg->frm_size;
    int factor = cfg->is_upsample ? cfg->upsamp_rate : 1;
    int h_synthesis_len = window_sz - frm_size;

    float *h_proto = design_prototype_filter(cfg->taps, cfg->cutoff_ratio, cfg->beta);
    // print_float(h_proto, weight_len);
    float *h_synthesis = (float*)wtk_malloc(h_synthesis_len * sizeof(float));
    memset(h_synthesis, 0, h_synthesis_len * sizeof(float));
 
    for (int i = 0; i < weight_len; i++) {
        float phase = PI / (2*rate) * (i - ((float)taps-1)/2.0) - PI/4;
        h_synthesis[i] = 2 * h_proto[i] * cos(phase);
    }

    for(int i=0; i<h_synthesis_len/2; i++){
        float tmp = h_synthesis[i];
        h_synthesis[i] = h_synthesis[h_synthesis_len-i-1]*factor;
        h_synthesis[h_synthesis_len-i-1] = tmp*factor;
    }

    wtk_free(h_proto);
    // print_float(h_synthesis, h_synthesis_len);
    Linear_Conv(r, h_synthesis, h_synthesis_len);

    wtk_free(h_synthesis);
}

void downsamper_weight(wtk_resample2_t *r)
{
    wtk_resample2_cfg_t *cfg = r->cfg;
    int taps = cfg->taps;
    int window_sz = cfg->window_sz;
    int weight_len = taps+1;
    int rate = cfg->SR < cfg->new_SR ? cfg->upsamp_rate : cfg->downsamp_rate;
    int frm_size = cfg->frm_size;
    int factor = cfg->is_upsample ? cfg->upsamp_rate : 1;
    int h_analysis_len = window_sz - frm_size;

    float *h_proto = design_prototype_filter(cfg->taps, cfg->cutoff_ratio, cfg->beta);
    // print_float(h_proto, weight_len);
    float *h_analysis = (float*)wtk_malloc(h_analysis_len * sizeof(float));
    memset(h_analysis, 0, h_analysis_len * sizeof(float));

    for (int i = 0; i < weight_len; i++) {
        float phase = PI / (2*rate) * (i - ((float)taps-1)/2.0) + PI/4;
        h_analysis[i] = 2 * h_proto[i] * cos(phase);
    }


    for(int i=0; i<h_analysis_len/2; i++){
        float tmp = h_analysis[i];
        h_analysis[i] = h_analysis[h_analysis_len-i-1]*factor;
        h_analysis[h_analysis_len-i-1] = tmp*factor;
    }

    wtk_free(h_proto);
    // print_float(h_analysis, h_analysis_len);
    Linear_Conv(r, h_analysis, h_analysis_len);

    wtk_free(h_analysis);
}


void generate_weight(wtk_resample2_t *r){
    if(r->cfg->SR < r->cfg->new_SR){
        upsamper_weight(r);
    }else{
        downsamper_weight(r);
    }
}

wtk_resample2_t* wtk_resample2_new(wtk_resample2_cfg_t *cfg)
{
    int weight_length = cfg->weight_length;

    wtk_resample2_t* r = (wtk_resample2_t*)wtk_malloc(sizeof(wtk_resample2_t));
    memset(r, 0, sizeof(wtk_resample2_t));
    r->cfg = cfg;
    int nbin = cfg->window_sz/2+1;
    // r->drft = drft_hann_new(nbin, cfg->window_sz, 1);
    r->fft = wtk_rfft_new(cfg->window_sz/2);
    r->ifft = wtk_rfft_new(cfg->window_sz/2);
    r->weight = (wtk_complex_t *)wtk_malloc(nbin * sizeof(wtk_complex_t));
    memset(r->weight, 0, nbin * sizeof(wtk_complex_t));
    r->buf = wtk_strbuf_new(1024,1);
    generate_weight(r);
    
    // FILE *fp = fopen("weight.txt", "w");
    // for(int i=0; i<weight_length; i++){
    //     fprintf(fp, "%f\n", r->weight[i].a);
    // }
    // fclose(fp);
    // exit(0);

    r->cache = (float *)wtk_malloc(weight_length * sizeof(float));
    memset(r->cache, 0, weight_length * sizeof(float));

    r->F = (float *)wtk_malloc(cfg->window_sz * sizeof(float));
    memset(r->F, 0, cfg->window_sz * sizeof(float));

    r->input = (float *)wtk_malloc(cfg->frm_size * sizeof(float));
    memset(r->input, 0, cfg->frm_size * sizeof(float));

    if(cfg->SR < cfg->new_SR){
        r->frame = (float *)wtk_malloc(cfg->window_sz * sizeof(float));
        r->output = (float *)wtk_malloc(cfg->frm_size * cfg->upsamp_rate * sizeof(float));
        memset(r->frame, 0, cfg->window_sz * sizeof(float));
        memset(r->output, 0, cfg->frm_size * cfg->upsamp_rate * sizeof(float));
    }else{
        r->frame = (float *)wtk_malloc(cfg->window_sz * sizeof(float));
        r->output = (float *)wtk_malloc(cfg->window_sz * sizeof(float));
        memset(r->frame, 0, cfg->window_sz * sizeof(float));
        memset(r->output, 0, cfg->window_sz * sizeof(float));
    }
    r->freq = (wtk_complex_t *)wtk_malloc(nbin * sizeof(wtk_complex_t));
    memset(r->freq, 0, nbin * sizeof(wtk_complex_t));
    return r;
}

int wtk_resample2_start(wtk_resample2_t *r)
{
    return 0;
}

int wtk_resample2_delete(wtk_resample2_t *r)
{
    if(r->fft){
        wtk_rfft_delete(r->fft);
    }
    if(r->ifft){
        wtk_rfft_delete(r->ifft);
    }
    if(r->weight){
        wtk_free(r->weight);
    }
    if(r->buf){
        wtk_strbuf_delete(r->buf);
    }
    if(r->cache){
        wtk_free(r->cache);
    }
    if(r->F){
        wtk_free(r->F);
    }
    if(r->input){
        wtk_free(r->input);
    }
    if(r->frame){
        wtk_free(r->frame);
    }
    if(r->output){
        wtk_free(r->output);
    }
    if(r->freq){
        wtk_free(r->freq);
    }

    wtk_free(r);
    return 0;
}

void wtk_resample2_set_notify(wtk_resample2_t *r,void *ths,wtk_resample2_notify_f notify)
{
    r->ths = ths;
    r->notify = notify;
}

void idrft_frame(wtk_drft_t* rfft, wtk_complex_t *fft, float *rfft_in, int wins){
	wtk_drft_ifft22(rfft, fft, rfft_in);
	for (int i=0;i<wins;++i)
	{
		rfft_in[i] /= wins;
	}
}


/*
data:输入数据
out:输出数据
len:输入数据长度,window_sz
*/
static void conv(wtk_resample2_t *r, float *data, float *out, int len)
{
    wtk_complex_t cpx;
    int window_sz = r->cfg->window_sz;
    int nbin = window_sz/2+1;
    float A, B, C;
    wtk_complex_t p1, p2;
    // float *in = r->frame;
    int chunk_size = r->cfg->frm_size;
    int weight_length = r->cfg->weight_length;

    wtk_rfft_process_fft(r->fft, r->F, data);
    for (int i = 0; i < nbin; i++) {
        cpx = wtk_rfft_get_value(r->F, window_sz, i);
        r->freq[i].a = cpx.a;
        r->freq[i].b = cpx.b;
    }

    // print_float(r->F, window_sz);

    // 时域卷积等价于频域乘
    for(int i=0; i<nbin; i++){
        p1 = r->freq[i];
        p2 = r->weight[i];
        A = (p1.a + p1.b) * p2.a;
        B = (p2.a + p2.b) * p1.b;
        C = (p1.b - p1.a) * p2.b;
        r->freq[i].a = A - B;
        r->freq[i].b = B - C;
        // printf("idx %d %f %f\n",i, r->weight[i].a, r->weight[i].b);
    }
    // exit(0);

    for(int i=0; i<nbin; i++){
        wtk_rfft_set_value(r->F, window_sz, i, r->freq[i]);
    }
    wtk_rfft_process_ifft(r->fft, r->F, data);

    // print_float(data, window_sz);

    // idrft_frame(r->drft->rfft, r->freq, r->drft->rfft_in, window_sz);


    float *conv_out=data;
    if(chunk_size <= weight_length){
        for(int i=0; i<chunk_size; i++){
            out[i] = conv_out[i] + r->cache[i];
        }
        //update cache
        //将缓存中的前N-chunk_size个元素更新为缓存中chunk_size之后的元素
        memmove(r->cache, r->cache + chunk_size, (weight_length - chunk_size) * sizeof(float));
        //将缓存的最后chunk_size个元素清零
        memset(r->cache + (weight_length - chunk_size), 0, chunk_size * sizeof(float));
        //将当前帧卷积结果的后chunk_size个元素添加到缓存中。
        for (int i = 0; i < weight_length; i++) {
            r->cache[i] += conv_out[chunk_size + i];
        }
        // print_float(r->cache, weight_length);
    }else{
        memcpy(out, conv_out, chunk_size * sizeof(float));

        for(int i=0; i<weight_length; i++){
            out[i] += r->cache[i];
        }
        memcpy(r->cache, conv_out + chunk_size, weight_length * sizeof(float));
    }
}

static void upsamper(wtk_resample2_t *r, float *data, int len, int is_end)
{
    int rate = r->cfg->SR < r->cfg->new_SR ? r->cfg->upsamp_rate : r->cfg->downsamp_rate;
    int frm_size = len;
    int window_sz = r->cfg->window_sz;
    // print_float(data, len);
    //frm_up = np.vstack([frm, np.zeros((self.upsamp_rate - 1, len(frm)))]).T.reshape(-1)
    for(int i=0; i<frm_size; i++){//用output暂存，vstack输出，类型为float；先存float，再存short。
        r->output[i*rate] = data[i];
        for(int j=1; j<rate; j++){
            r->output[i*rate+j] = 0;
        }
    }
    // print_float(r->output, len*rate);

    float *in = r->frame;//frame为windows_sz大小的数组
    for(int i=0; i<rate; ++i){
        float *output = r->output+i*frm_size;
        for(int j=0; j<frm_size; j++){
            in[j] = output[j];
        }
        for(int j=frm_size; j<window_sz; j++){
            in[j]=0;
        }//每次fft都要padding到window_sz
        // print_float(in, window_sz);
        conv(r, in, r->output+i*frm_size, frm_size);//这里往out里面写数据，不会覆盖暂存未用的float数据
    }
    short *out = (short *)r->output;
    for(int i=0; i<len*rate; i++){
        out[i] = floor(r->output[i]*32768+0.5);
    }

    if(r->notify){
        r->notify(r->ths, (char *)out, len*rate*sizeof(short));
    }
}

static void downsamper(wtk_resample2_t *r, float *data, int len, int is_end)
{
    int rate = r->cfg->SR < r->cfg->new_SR ? r->cfg->upsamp_rate : r->cfg->downsamp_rate;
    // int fsize = r->cfg->window_sz/2;
    //int weight_len = r->cfg->taps+1;
    int window_sz = r->cfg->window_sz;

    //np.hstack([chunk, np.zeros(self.weight_length)])
    for(int i=0;i<len;i++){
        r->frame[i] = data[i];
    }
    for(int i=len; i<window_sz; i++){
        r->frame[i] = 0;
    }
    // print_float(r->output, window_sz);
    conv(r, r->frame, r->output, window_sz);

    //frm_down = np.reshape(frm, (-1, self.downsamp_rate))[:, 0]
    short *out = (short *)r->output;
    for(int i=0; i<len/rate; i++){
        out[i] = floor(r->output[i*rate]*32768+0.5);
    }

    if(r->notify){
        r->notify(r->ths, (char *)out, len/rate*sizeof(short));
    }
} 

int wtk_resample2_feed(wtk_resample2_t *r,short *data,int len,int is_end)
{
    wtk_strbuf_t *buf = r->buf;
    int length;
    int frm_size = r->cfg->frm_size;
    short *yt;

    wtk_strbuf_push(buf, (char *)data, len * sizeof(short));
    length = buf->pos / sizeof(short);
    while(length >= frm_size){
        yt = (short *)buf->data;
        for(int i=0;i<frm_size;i++){
            r->input[i] = yt[i]/32768.0;
        }
        // print_float(r->input, frm_size);
        if(r->cfg->SR < r->cfg->new_SR){
            upsamper(r, r->input, frm_size, 0);
        }else{
            downsamper(r, r->input, frm_size, 0);
        }
        wtk_strbuf_pop(buf, NULL, frm_size * sizeof(short));
        length = buf->pos / sizeof(short);
    }
    return 0;
}
