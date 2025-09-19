#include "wtk/bfio/ahs/qtk_ahs.h"

int qtk_ahs_run_nn(qtk_ahs_t* a,wtk_thread_t *t);
void wtk_ahs_on_gainnet(qtk_ahs_t *a, float *out, int len, int is_end);
qtk_ahs_evt_t* qtk_ahs_new_evt(qtk_ahs_evt_t* wrapper)
{
    qtk_ahs_evt_t *evt;

    evt=(qtk_ahs_evt_t*)wtk_malloc(sizeof(qtk_ahs_evt_t));
    return evt;
}

int qtk_ahs_evt_delete(qtk_ahs_evt_t *evt)
{
    wtk_free(evt);
    return 0;
}

qtk_ahs_evt_t* qtk_ahs_pop_evt(qtk_ahs_t* a)
{
    qtk_ahs_evt_t *evt;

    evt=(qtk_ahs_evt_t*)wtk_lockhoard_pop(&(a->evt_hoard));
    return evt;
}

void qtk_ahs_push_evt(qtk_ahs_t* a,qtk_ahs_evt_t *evt)
{
    wtk_lockhoard_push(&(a->evt_hoard),evt);
}

static float _get_vadprob(qtk_ahs_t *a, int t){
    int offset = a->out_offset % 40;
    //wtk_debug("%d\n",offset);
    int i;
    float sum = 0.0;
    for(i = 0; i < 8; i++){
        sum += a->vad_probs[offset + i];
        //wtk_debug("%d %f\n",offset + i,a->vad_probs[offset + i]);
    }

    a->out_offset += 8;

    return sum/8.0;
}

static int file_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}

// 从文件中读取浮点数数据到动态数组中
static int readFloatsFromFile(qtk_ahs_t *a, const char *filename, wtk_strbuf_t *array,  qtk_ahs_gain_controller_t *gc) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("无法打开文件");
        return -1;
    }
 
    float value;
    if(fscanf(file, "%f", &value) == 1){
        gc->gain_cali = value;//read gain_cali
    }
    printf("=======================>gain_cali = %f\n", gc->gain_cali);
    while (fscanf(file, "%f", &value) == 1) {
        wtk_strbuf_push_float(a->gc->galis,&value,1);
    }
 
    // 检查fscanf的返回值是否为EOF，以确保不是由于格式错误而退出循环
    if (feof(file)) {
        // 成功读取到文件末尾
    } else {
        // 发生错误，可能是格式不匹配
        fprintf(stderr, "读取文件时发生格式错误。\n");
        fclose(file);
        return -1;
    }
 
    fclose(file);
    return 0;
}

/*
update gain,Update the gain for automatic gain adjustment.
*/
static void update_gain(qtk_ahs_t *a, const char *filename){
    FILE *fp = fopen(filename, "r");
    if(fscanf(fp, "%f", &a->mic_shift) != 1){
        wtk_debug("The error occurred when updating mic_shift!");
    }
    if(fscanf(fp, "%f", &a->echo_shift) != 1){
        wtk_debug("The error occurred when updating echo_shift!");
    }
    fclose(fp);
}

qtk_ahs_t *qtk_ahs_new(qtk_ahs_cfg_t *cfg) {
    qtk_ahs_t *a = wtk_malloc(sizeof(qtk_ahs_t));
    a->cfg = cfg;
    a->mic = wtk_strbuf_new(1024, 1);
    a->sp = wtk_strbuf_new(1024, 1);
    a->dcache = NULL;
    a->F = wtk_malloc(sizeof(float) * cfg->window_sz);
    a->frame_rt = wtk_malloc(sizeof(float) * cfg->window_sz);
    a->frame_yt = wtk_malloc(sizeof(float) * cfg->window_sz);
    a->frame_yt_tmp = wtk_malloc(sizeof(float) * cfg->window_sz);
    a->frame_rt_tmp = wtk_malloc(sizeof(float) * cfg->window_sz);
    a->fft = wtk_rfft_new(cfg->window_sz / 2);
    a->drft = wtk_drft_new2(cfg->window_sz);
    a->rt_pad_tmp = wtk_malloc(sizeof(float) * (cfg->pad_sz + 1));
    a->yt_pad_tmp = wtk_malloc(sizeof(float) * (cfg->pad_sz + 1));
    a->x = wtk_malloc(sizeof(float) * cfg->chunk_sz * cfg->feat_dim);
    a->y = wtk_malloc(sizeof(float) * cfg->chunk_sz * cfg->feat_dim);
    a->mix_spec = wtk_malloc(sizeof(float) * cfg->chunk_sz * cfg->nbin * 2);
    a->est_freq = wtk_malloc(sizeof(wtk_complex_t) * cfg->chunk_sz * cfg->nbin);
    a->out = wtk_malloc(sizeof(float) * cfg->chunk_sz * cfg->window_sz);
    a->pos = 0;
    a->chunk_pos = 0;
    a->start = 0;
    a->start2 = 0;
    a->cache_c0 = NULL;
    a->cache_h0 = NULL;
    a->cache[0] = NULL;
    a->analysis_mem = wtk_calloc(sizeof(float), cfg->nbin - 1);
    a->analysis_mem_sp = wtk_calloc(sizeof(float), cfg->nbin - 1);
    a->synthesis_mem = wtk_calloc(sizeof(float), cfg->nbin - 1);
    a->synthesis_mem2 = wtk_calloc(sizeof(float), cfg->nbin - 1);
    a->notify = NULL;
    a->upval = NULL;
    a->qmmse = NULL;

    a->nnrt = NULL;
    a->kalman = NULL;
    a->kalman2 = NULL;
    a->kalman2_sp = NULL;
    a->kalman_nnrt = NULL;
    a->kalman_nnrt2 = NULL;
    a->frame_kalman_tmp = NULL;
    if(cfg->type == 3){
        a->kalman = qtk_kalman_new(&cfg->km, 1, 129);
        if(a->cfg->kalman_type == 2){
            a->kalman2 = qtk_kalman_new(&cfg->km2, 1, 129);
        }
        a->km_x = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*(cfg->window_sz / 2 + 1)*cfg->km.kalman_order);
        a->km_y = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*(cfg->window_sz / 2 + 1));
        if(cfg->km.kalman_order > 1){
            a->frame_kalman_tmp = wtk_malloc(sizeof(float) * cfg->window_sz);
        }
        if(a->cfg->use_nnrt){
            a->kalman_nnrt = qtk_crnnoise_new(&(cfg->nnrt));
        }else if(a->cfg->use_lstm){
            a->nnrt = qtk_nnrt_new(&cfg->nnrt);
            a->km_z = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*(cfg->window_sz / 2 + 1));
        }else if(a->cfg->use_nnrt2){
            a->kalman_nnrt2 = qtk_crnnoise2_new(&(cfg->nnrt));
        }

        if(cfg->use_gainnet){
            a->erb = qtk_ahs_erb_new(&(cfg->erb));
            a->gainnet = wtk_bbonenet_new(&(cfg->gainnet));
            a->erb_mic = wtk_malloc(sizeof(float) * (cfg->window_sz / 2 + 1) * 4);
            a->gainnet_indim = a->erb->n * 4;
            a->gainnet_in = wtk_malloc(sizeof(float) * a->gainnet_indim);
            a->gainnet_out = wtk_malloc(sizeof(float) * (cfg->window_sz / 2 + 1) * 2);
            wtk_bbonenet_set_notify(a->gainnet,a,(wtk_bbonenet_notify_f)wtk_ahs_on_gainnet);
        }

    }else{
        a->nnrt = qtk_nnrt_new(&cfg->nnrt);
    }

    a->wiener = NULL;
    if(cfg->use_mask){
        a->wiener = qtk_wiener_new(&(cfg->covm),&(cfg->bf),cfg->use_fftsbf,cfg->nbin,cfg->window_sz,cfg->wiener_thresh);
        a->wiener->scale = cfg->scale;
        if(cfg->use_clip){
            a->wiener->clip_e = cfg->clip_e;
            a->wiener->clip_s = cfg->clip_s;
        }else{
            a->wiener->clip_e = (8000*1.0*cfg->window_sz)/cfg->rate;
            a->wiener->clip_s = 0;
        }
    }

    if(cfg->use_out_qmmse)
    {
        a->qmmse=wtk_qmmse_new(&(cfg->qmmse));
    }
    a->qmmse2=NULL;
    a->fftx=NULL;
    if(cfg->use_in_qmmse)
    {
        a->qmmse2=wtk_qmmse_new(&(cfg->qmmse2));
        a->fftx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->nbin);
    }
    if(cfg->use_loop)
    {
        a->dcache = wtk_strbuf_new(1024,1);
    }

	a->eq = NULL;
	if(cfg->use_eq)
	{
		a->eq = wtk_equalizer_new(&(cfg->eq));
	}
    a->eq_gain = NULL;
    if(cfg->use_eq2){
        a->eq_gain = (float *)wtk_malloc(sizeof(float)*cfg->nbin);
        FILE *fp = fopen(cfg->eq_gain_fn,"r");
        if (!fp) {
            // Handle error (e.g., print message, return error code)
            fprintf(stderr, "Error opening eq_gain.txt\n");
            exit(0);
        }
        for (int i = 0; i < cfg->nbin; ++i) {
            // Check return value of fscanf and use correct parameter order
            if (fscanf(fp, "%f\n", &a->eq_gain[i]) != 1) {  // Add & and check return value
                // Handle read error (e.g., insufficient values in file)
                wtk_debug("Error reading value at index %d\n", i);
                break;
            }
        }
        fclose(fp);  // Always close the file
    }

    a->gc = NULL;
    a->mic_shift=1.0;
    a->echo_shift=1.0;
    if(a->cfg->use_sp_check){
        a->gc = qtk_gain_controller_new(&cfg->gain_controller);
        if(file_exists("ahs_gain")){//If the gain file is found, update the gain.
            update_gain(a, "ahs_gain");
        }
        if(file_exists("galis")) {//check if file exists
            readFloatsFromFile(a, "galis", a->gc->galis, a->gc);//if file exists, read it into wtk_strbuf_t
            qtk_gain_controller_set_mode(a->gc,0);//set mode to 0 to use galis
            qtk_gain_controller_update_calibration(a->gc,a->mic_shift * a->echo_shift);
        } else {
            FILE *fp = fopen("galis","a");//Write to the file in append mode
            fprintf(fp,"%.10f\n", a->mic_shift * a->echo_shift);
            wtk_debug("gain_cali:%.10f\n", a->mic_shift * a->echo_shift);
            fclose(fp);
            qtk_gain_controller_set_mode(a->gc,1);//set mode to 1 to measure galis
            // printf("文件 %s 不存在或没有权限访问。\n", filename);
        }
    }

    a->pool = NULL;
    if(cfg->use_mt){
        a->run = 1;
        wtk_thread_init(&(a->thread),(thread_route_handler)qtk_ahs_run_nn,a);
        wtk_thread_set_name(&(a->thread),"ahs_nn");
        wtk_blockqueue_init(&(a->nn_input_q));
        wtk_thread_start(&(a->thread));
        wtk_sem_init(&(a->end_wait_sem),0);
        wtk_lockhoard_init(&(a->evt_hoard),offsetof(qtk_ahs_evt_t,hoard_n),1024,(wtk_new_handler_t)qtk_ahs_new_evt,
                            (wtk_delete_handler_t)qtk_ahs_evt_delete,a);
        wtk_queue_init(&(a->mt_q));
        a->pool = wtk_vpool2_new(sizeof(qtk_ahs_mt_value_t), 10);
    }

    a->conv = NULL;
    a->wav_out = wtk_malloc(cfg->hop_sz * sizeof(short));
    a->sp_in = wtk_malloc(cfg->hop_sz * sizeof(short));

    a->conv2 = NULL;

    a->freq_shift = NULL;
    if(cfg->use_freq_shift){
        a->freq_shift = qtk_freq_shift_new(&cfg->freq_shift);
    }
    a->vad = NULL;
    if(cfg->use_nnvad){
        a->vad = wtk_kvad_new(&(cfg->kvad));
        memset(a->vad_probs,0,sizeof(float)*40);
        a->in_offset = 20;
        a->out_offset = 0;
    }
    a->a_vad = a->t_vad = 0;
    a->vad_idx = 0;

    a->delay = max(floor(cfg->n_delay * cfg->rate * 1.0 / 1000), cfg->window_sz);
    if(a->kalman2){
        int n = a->delay + (cfg->km.L + 1) * cfg->hop_sz;
        a->kalman2_sp = wtk_strbuf_new(n *sizeof(float),1);
        a->st_idx = -a->delay - (cfg->km.L + 1) * cfg->hop_sz;
        a->stop_idx = -a->delay - (cfg->km.L - 1) * cfg->hop_sz;
    }
    a->loop_delay = max(floor(cfg->loop_delay * cfg->rate * 1.0 / 1000), cfg->window_sz);
    qtk_ahs_reset(a);
    return a;
}

void qtk_ahs_set_ref_compensation(qtk_ahs_t *a,float *rir, int len, float rt60){
    a->conv = qtk_linear_conv_new2(rir,len,a->cfg->hop_sz,rt60);
}

void qtk_ahs_delete(qtk_ahs_t *a) {
    int i;
    if (a->cache_h0) {
        qtk_nnrt_value_release(a->nnrt, a->cache_h0);
        qtk_nnrt_value_release(a->nnrt, a->cache_c0);
    }
    if(a->cache[0]) {
        int cnt = 12;
        if(a->cfg->type == 7){
            cnt = 8;
        }
        if(a->cfg->use_lstm){
            cnt = 4;
        }
        for(i = 0; i < cnt; i++){
            qtk_nnrt_value_release(a->nnrt, a->cache[i]);
        }
    }
    if(a->kalman){
        qtk_kalman_delete(a->kalman);
        if(a->kalman2){
            qtk_kalman_delete(a->kalman2);
            if(a->kalman2_sp){
                wtk_strbuf_delete(a->kalman2_sp);
            }
        }
        if(a->kalman_nnrt){
            qtk_crnnoise_delete(a->kalman_nnrt);
        }
        if(a->kalman_nnrt2){
            qtk_crnnoise2_delete(a->kalman_nnrt2);
        }
        if(a->cfg->use_lstm){
            wtk_free(a->km_z);
        }
        if(a->frame_kalman_tmp){
            wtk_free(a->frame_kalman_tmp);
        }
        wtk_free(a->km_x);
        wtk_free(a->km_y);

        if(a->cfg->use_gainnet){
            wtk_free(a->erb_mic);
            wtk_free(a->gainnet_in);
            wtk_free(a->gainnet_out);
            qtk_ahs_erb_delete(a->erb);
            wtk_bbonenet_delete(a->gainnet);
        }
    }
    if(a->wiener){
        qtk_wiener_delete(a->wiener);
    }
    if(a->nnrt){
        qtk_nnrt_delete(a->nnrt);
    }
    if(a->conv){
        qtk_linear_conv_delete(a->conv);
    }
    if(a->conv2){
        qtk_linear_conv_delete(a->conv2);
    }
    wtk_strbuf_delete(a->mic);
    wtk_strbuf_delete(a->sp);
    wtk_rfft_delete(a->fft);
    wtk_drft_delete2(a->drft);
    wtk_free(a->frame_rt);
    wtk_free(a->frame_yt);
    wtk_free(a->frame_yt_tmp);
    wtk_free(a->frame_rt_tmp);
    wtk_free(a->yt_pad_tmp);
    wtk_free(a->rt_pad_tmp);
    wtk_free(a->x);
    wtk_free(a->y);
    wtk_free(a->F);
    wtk_free(a->est_freq);
    wtk_free(a->mix_spec);
    wtk_free(a->analysis_mem);
    wtk_free(a->analysis_mem_sp);
    wtk_free(a->synthesis_mem);
    wtk_free(a->synthesis_mem2);
    wtk_free(a->out);
    wtk_free(a->sp_in);
    wtk_free(a->wav_out);
    if(a->qmmse)
    {
        wtk_qmmse_delete(a->qmmse);
    }
    if(a->qmmse2)
    {
        wtk_qmmse_delete(a->qmmse2);
    }
    if(a->fftx)
    {
        wtk_free(a->fftx);
    }
    if(a->dcache)
    {
        wtk_strbuf_delete(a->dcache);
    }
    if(a->gc){
        qtk_gain_controller_delete(a->gc);
    }
    if(a->eq)
    {
        wtk_equalizer_delete(a->eq);
    }
    if(a->eq_gain){
        wtk_free(a->eq_gain);
    }

    if(a->cfg->use_mt){
        wtk_thread_clean(&(a->thread));
        wtk_blockqueue_clean(&(a->nn_input_q));
        wtk_lockhoard_clean(&(a->evt_hoard));
        wtk_vpool2_delete(a->pool);
    }
    if(a->freq_shift){
        qtk_freq_shift_delete(a->freq_shift);
    }
    if(a->vad){
        wtk_kvad_delete(a->vad);
    }  
    wtk_free(a);
}

void qtk_ahs_reset(qtk_ahs_t *a) {
    if(a->qmmse)
    {
        wtk_qmmse_reset(a->qmmse);
    }
    if(a->qmmse2)
    {
        wtk_qmmse_reset(a->qmmse2);
    }
    if(a->fftx)
    {
        memset(a->fftx,0,sizeof(wtk_complex_t)*a->cfg->nbin);
    }
    if(a->kalman){
        qtk_kalman_reset(a->kalman);
        if(a->kalman2){
            qtk_kalman_reset(a->kalman2);
        }
        if(a->kalman_nnrt){
            qtk_crnnoise_reset(a->kalman_nnrt);
        }
        if(a->cfg->use_gainnet){
            qtk_ahs_erb_reset(a->erb);
            wtk_bbonenet_reset(a->gainnet);
        }
    }
    if(a->wiener){
        qtk_wiener_reset(a->wiener);
    }
    if(a->gc){
        qtk_gain_controller_reset(a->gc);
    }
}

void qtk_ahs_start(qtk_ahs_t *a) {}
//int oid = 0;

static void nn_post_(qtk_ahs_t *a, float *real, float *imag) {
    int i, j;
    int n = a->cfg->chunk_sz * a->cfg->nbin;
    float *fv1;
    short *pv;
    int pv1;
    short pv2;
    float pre_scale = a->cfg->pre_scale;
    //wtk_debug("========output %d==========\n",++oid);
    if(a->eq_gain && a->cfg->is_eq2_kalmanout){
        for(int i = 0; i < n; i++){
            a->est_freq[i].a = real[i]*a->eq_gain[i];
            a->est_freq[i].b = imag[i]*a->eq_gain[i];
        }
    }else{
        for (i = 0; i < n; i++) {
            a->est_freq[i].a = real[i];// * a->cfg->scale;
            a->est_freq[i].b = imag[i];// * a->cfg->scale;
            //wtk_debug("%d %f %f\n",i,real[i],imag[i]);
        }
    }
    if(a->cfg->use_sp_check){
        for (i = 0; i < n; i++) {
            a->est_freq[i].a *= a->echo_shift;
            a->est_freq[i].b *= a->echo_shift;
        }
    }

    if(a->cfg->use_clip){
        int clip_s = a->cfg->clip_s;
        int clip_e = a->cfg->clip_e;
        wtk_complex_t *freq;
        for(j = 0; j < a->cfg->chunk_sz; j++){
            freq = a->est_freq + j * a->cfg->nbin;
            for(i = 0; i < clip_s; ++i){
                freq[i].a = freq[i].b = 0;
            }
            for(i = clip_e; i < n; ++i){
                freq[i].a = freq[i].b = 0;
            }
        }
    }

    //wtk_debug("%d %d\n",a->cfg->chunk_sz,a->cfg->hop_sz);
    for (i = 0; i < a->cfg->chunk_sz; i++) {
        if(a->qmmse){
            wtk_qmmse_denoise(a->qmmse, a->est_freq + i * a->cfg->nbin);
        }

        if(a->cfg->use_sp_check){
            qtk_gain_controller_run(a->gc, a->est_freq + i * a->cfg->nbin, n, a->out + i * a->cfg->window_sz,_get_vadprob(a,a->a_vad));
        }
        wtk_drft_frame_synthesis3(a->drft, a->frame_rt_tmp, a->synthesis_mem,
                                    a->est_freq + i * a->cfg->nbin,
                                    a->out + i * a->cfg->window_sz,
                                    a->cfg->window_sz, a->cfg->synthesis_window);
        fv1 = a->out + i * a->cfg->window_sz;

        if(a->freq_shift){
            qtk_freq_shift_feed(a->freq_shift, fv1);
        }

        pv = (short *)fv1;
        for (j = 0; j < a->cfg->hop_sz; ++j) {
            //wtk_debug("%d %f\n",j,fv1[j]);
            pv[j] = floorf(fv1[j] * 32768.0f + 0.5);//floorf(fv1[j] + 0.5);
            pv1 = floorf(fv1[j] * 32768.0f * pre_scale + 0.5);
            if (pv1 > 32767) {
                pv1 = 32767;
            } else if (pv1 < -32768) {
                pv1 = -32768;
            }
            pv2 = pv1;
            wtk_strbuf_push(a->sp, (char *)&pv2, sizeof(short));
            if(a->dcache) {
                wtk_strbuf_push(a->dcache, (char *)&pv2, sizeof(short));
            }
        }

        if(a->eq)
        {
            wtk_equalizer_feed(a->eq, pv, a->cfg->hop_sz);
        }

        if (a->notify && !a->cfg->use_mask) {
            a->notify(a->upval, pv, a->cfg->hop_sz);
        }
    }
    // for (i = 0; i < a->cfg->chunk_sz; i++) {
    //     wtk_drft_frame_synthesis(a->drft, a->frame_rt_tmp, a->synthesis_mem,
    //                              a->est_freq + i * a->cfg->nbin,
    //                              a->out + i * a->cfg->window_sz,
    //                              a->cfg->window_sz, a->cfg->synthesis_window);
    // }
    // if (a->notify) {
    //     a->notify(a->upval, a->out, a->cfg->window_sz * a->cfg->chunk_sz);
    // }

}

static void nn_post_mask_(qtk_ahs_t *a, float *real, float *imag, float *mask_real, float *mask_img) {
    int i, j;
    int n = a->cfg->chunk_sz * a->cfg->nbin;
    float *fv1;
    short *pv;
    int pv1;
    short pv2;
    float pre_scale = a->cfg->pre_scale;
    wtk_complex_t *fftx = a->wiener->fftx;
    //wtk_debug("========output %d==========\n",++oid);
    qtk_wiener_feed(a->wiener,real,imag,mask_real,mask_img);

    for (i = 0; i < n; i++) {
        a->est_freq[i].a = fftx[i].a ;//* a->cfg->scale;
        a->est_freq[i].b = fftx[i].b ;//* a->cfg->scale;
        //wtk_debug("%d %f %f\n",i,fftx[i].a,fftx[i].b);
    }

    //wtk_debug("%d %d\n",a->cfg->chunk_sz,a->cfg->hop_sz);
    for (i = 0; i < a->cfg->chunk_sz; i++) {
        if(a->qmmse){
            wtk_qmmse_denoise(a->qmmse, a->est_freq + i * a->cfg->nbin);
        }

        if(a->cfg->use_sp_check){
            //qtk_gain_controller_run(a->gc, a->est_freq + i * a->cfg->nbin, n, a->out + i * a->cfg->window_sz);
        }else{
            wtk_drft_frame_synthesis3(a->drft, a->frame_rt_tmp, a->synthesis_mem2,
                                    a->est_freq + i * a->cfg->nbin,
                                    a->out + i * a->cfg->window_sz,
                                    a->cfg->window_sz, a->cfg->synthesis_window);
        }
        fv1 = a->out + i * a->cfg->window_sz;

        if(a->freq_shift){
            qtk_freq_shift_feed(a->freq_shift, fv1);
        }

        pv = (short *)fv1;
        for (j = 0; j < a->cfg->hop_sz; ++j) {
            //wtk_debug("%d %f\n",j,fv1[j]);
            pv[j] = floorf(fv1[j] + 0.5);//floorf(fv1[j] + 0.5);
            pv1 = floorf(fv1[j] * pre_scale + 0.5);
            if (pv1 > 32767) {
                pv1 = 32767;
            } else if (pv1 < -32768) {
                pv1 = -32768;
            }
            pv2 = pv1;
            if(a->cfg->use_mask_sp){
                wtk_strbuf_push(a->sp, (char *)&pv2, sizeof(short));
                if(a->dcache) {
                    wtk_strbuf_push(a->dcache, (char *)&pv2, sizeof(short));
                }
            }
        }

        if(a->eq)
        {
            wtk_equalizer_feed(a->eq, pv, a->cfg->hop_sz);
        }

        if (a->notify) {
            a->notify(a->upval, pv, a->cfg->hop_sz);
        }
    }
}

static void nn_post2_(qtk_ahs_t *a, wtk_complex_t *out) {
    int i, j;
    int n = a->cfg->chunk_sz * a->cfg->nbin;
    float *fv1;
    short *pv;
    int pv1;
    short pv2;
    float pre_scale = a->cfg->pre_scale;
    //wtk_debug("========output %d==========\n",++oid);
    if(a->eq_gain && a->cfg->is_eq2_kalmanout){
        for(int i = 0; i < n; i++){
            out[i].a *= a->eq_gain[i];
            out[i].b *= a->eq_gain[i];
        }
    }

    if(a->cfg->use_sp_check){
        for (i = 0; i < n; i++) {
            a->est_freq[i].a = out[i].a * a->echo_shift;
            a->est_freq[i].b = out[i].b * a->echo_shift;
        }
    }else{
        for (i = 0; i < n; i++) {
            a->est_freq[i].a = out[i].a;
            a->est_freq[i].b = out[i].b;
        }
    }

    if(a->cfg->use_clip){
        int clip_s = a->cfg->clip_s;
        int clip_e = a->cfg->clip_e;
        wtk_complex_t *freq;
        for(j = 0; j < a->cfg->chunk_sz; j++){
            freq = a->est_freq + j * a->cfg->nbin;
            for(i = 0; i < clip_s; ++i){
                freq[i].a = freq[i].b = 0;
            }
            for(i = clip_e; i < n; ++i){
                freq[i].a = freq[i].b = 0;
            }
        }
    }

    for (i = 0; i < a->cfg->chunk_sz; i++) {
        if(a->qmmse){
            wtk_qmmse_denoise(a->qmmse, a->est_freq + i * a->cfg->nbin);
        }
        //wtk_debug("%f\n",_get_vadprob(a,a->a_vad));
        if(a->cfg->use_sp_check){
            qtk_gain_controller_run(a->gc, a->est_freq + i * a->cfg->nbin, n, a->out + i * a->cfg->window_sz,_get_vadprob(a,a->a_vad));
        }
        wtk_drft_frame_synthesis3(a->drft, a->frame_rt_tmp, a->synthesis_mem,
                                    a->est_freq + i * a->cfg->nbin,
                                    a->out + i * a->cfg->window_sz,
                                    a->cfg->window_sz, a->cfg->synthesis_window);
        fv1 = a->out + i * a->cfg->window_sz;
        if(a->freq_shift){
            qtk_freq_shift_feed(a->freq_shift, fv1);
        }

        pv = a->wav_out;
        for (j = 0; j < a->cfg->hop_sz; ++j) {
            pv[j] = floorf(fv1[j] * 32768.0f + 0.5);
        }

        if(a->conv){
            qtk_linear_conv_conv1d_calc(a->conv,fv1);
        }

        if(!a->start2){
            memset(pv,0,a->cfg->hop_sz*sizeof(short));
            a->start2 = 1;
        }else{
            for (j = 0; j < a->cfg->hop_sz; ++j) {
                pv1 = floorf(fv1[j] * 32768.0f * pre_scale + 0.5);
                if (pv1 > 32767) {
                    pv1 = 32767;
                } else if (pv1 < -32768) {
                    pv1 = -32768;
                }
                pv2 = pv1;
                wtk_strbuf_push(a->sp, (char *)&pv2, sizeof(short));
                if(a->dcache) {
                    wtk_strbuf_push(a->dcache, (char *)&pv2, sizeof(short));
                }
            }

            if(a->eq)
            {
                wtk_equalizer_feed(a->eq, pv, a->cfg->hop_sz);
            }
        }

        if (a->notify) {
            a->a_vad += 8;
            a->notify(a->upval, pv, a->cfg->hop_sz);
        }
    }
}

static void forward_lstm(qtk_ahs_t *a) {
    float *fake_cache = NULL, *fake_cache1 = NULL;
    qtk_nnrt_value_t cache_h0, cache_c0, x, mix_spec;
    qtk_nnrt_value_t output_real, output_imag;
    int64_t x_shape[3] = {1, a->cfg->chunk_sz, a->cfg->feat_dim};
    int64_t mix_spec_shape[3] = {1, a->cfg->chunk_sz, 2 * a->cfg->nbin};
    int c0_idx = a->cfg->c0_idx;
    int h0_idx = a->cfg->h0_idx;
    if (a->cache_h0) {
        cache_h0 = a->cache_h0;
        cache_c0 = a->cache_c0;
    } else {
        int64_t cache_shape[3];
        qtk_nnrt_get_input_shape(a->nnrt, 1, cache_shape, 3);
        fake_cache = wtk_calloc(
            cache_shape[0] * cache_shape[1] * cache_shape[2], sizeof(float));
        cache_h0 = qtk_nnrt_value_create_external(
            a->nnrt, QTK_NNRT_VALUE_ELEM_F32, cache_shape, 3, fake_cache);
        //wtk_free(fake_cache);
        qtk_nnrt_get_input_shape(a->nnrt, 2, cache_shape, 3);
        fake_cache1 = wtk_calloc(
            cache_shape[0] * cache_shape[1] * cache_shape[2], sizeof(float));
        cache_c0 = qtk_nnrt_value_create_external(
            a->nnrt, QTK_NNRT_VALUE_ELEM_F32, cache_shape, 3, fake_cache1);
    }
    x = qtk_nnrt_value_create_external(a->nnrt, QTK_NNRT_VALUE_ELEM_F32,
                                       x_shape, 3, a->x);
    mix_spec = qtk_nnrt_value_create_external(a->nnrt, QTK_NNRT_VALUE_ELEM_F32,
                                              mix_spec_shape, 3, a->mix_spec);

    qtk_nnrt_feed(a->nnrt, x, 0);
    qtk_nnrt_feed(a->nnrt, cache_h0, 1);
    qtk_nnrt_feed(a->nnrt, cache_c0, 2);
    qtk_nnrt_feed(a->nnrt, mix_spec, 3);

    qtk_nnrt_run(a->nnrt);

    qtk_nnrt_get_output(a->nnrt, &output_real, 0);
    qtk_nnrt_get_output(a->nnrt, &output_imag, 1);
    qtk_nnrt_get_output(a->nnrt, &a->cache_h0, h0_idx);
    qtk_nnrt_get_output(a->nnrt, &a->cache_c0, c0_idx);

    if(a->cfg->use_mask){
        qtk_nnrt_value_t mask_real, mask_imag;
        qtk_nnrt_get_output(a->nnrt, &mask_real, 2);
        qtk_nnrt_get_output(a->nnrt, &mask_imag, 3);
        nn_post_mask_(a, qtk_nnrt_value_get_data(a->nnrt, output_real),
                qtk_nnrt_value_get_data(a->nnrt, output_imag),
                qtk_nnrt_value_get_data(a->nnrt, mask_real),
                qtk_nnrt_value_get_data(a->nnrt, mask_imag));
        qtk_nnrt_value_release(a->nnrt, mask_real);
        qtk_nnrt_value_release(a->nnrt, mask_imag);
        if(!a->cfg->use_mask_sp){
            nn_post_(a, qtk_nnrt_value_get_data(a->nnrt, output_real),
                qtk_nnrt_value_get_data(a->nnrt, output_imag));
        }
    }else{
        nn_post_(a, qtk_nnrt_value_get_data(a->nnrt, output_real),
                qtk_nnrt_value_get_data(a->nnrt, output_imag));
    }


    qtk_nnrt_value_release(a->nnrt, x);
    qtk_nnrt_value_release(a->nnrt, cache_h0);
    qtk_nnrt_value_release(a->nnrt, cache_c0);
    qtk_nnrt_value_release(a->nnrt, mix_spec);
    qtk_nnrt_value_release(a->nnrt, output_real);
    qtk_nnrt_value_release(a->nnrt, output_imag);
    if (fake_cache) {
        wtk_free(fake_cache);
        wtk_free(fake_cache1);
    }
}

static void forward_lstm2(qtk_ahs_t *a) {
    float *fake_cache = NULL, *fake_cache1 = NULL;
    qtk_nnrt_value_t cache_h0, cache_c0, x, mix_spec;
    qtk_nnrt_value_t output_real, output_imag;
    int64_t x_shape[3] = {1, a->cfg->chunk_sz, a->cfg->feat_dim};
    int64_t mix_spec_shape[3] = {1, a->cfg->chunk_sz, 2 * a->cfg->nbin};
    if (a->cache_h0) {
        cache_h0 = a->cache_h0;
        cache_c0 = a->cache_c0;
    } else {
        int64_t cache_shape[3];
        qtk_nnrt_get_input_shape(a->nnrt, 1, cache_shape, 3);
        fake_cache = wtk_calloc(
            cache_shape[0] * cache_shape[1] * cache_shape[2], sizeof(float));
        cache_h0 = qtk_nnrt_value_create_external(
            a->nnrt, QTK_NNRT_VALUE_ELEM_F32, cache_shape, 3, fake_cache);
        //wtk_free(fake_cache);
        qtk_nnrt_get_input_shape(a->nnrt, 2, cache_shape, 3);
        fake_cache1 = wtk_calloc(
            cache_shape[0] * cache_shape[1] * cache_shape[2], sizeof(float));
        cache_c0 = qtk_nnrt_value_create_external(
            a->nnrt, QTK_NNRT_VALUE_ELEM_F32, cache_shape, 3, fake_cache1);
    }
    x = qtk_nnrt_value_create_external(a->nnrt, QTK_NNRT_VALUE_ELEM_F32,
                                       x_shape, 3, a->x);
    mix_spec = qtk_nnrt_value_create_external(a->nnrt, QTK_NNRT_VALUE_ELEM_F32,
                                              mix_spec_shape, 3, a->mix_spec);

    qtk_nnrt_feed(a->nnrt, x, 0);
    qtk_nnrt_feed(a->nnrt, cache_h0, 1);
    qtk_nnrt_feed(a->nnrt, cache_c0, 2);
    qtk_nnrt_feed(a->nnrt, mix_spec, 3);

    qtk_nnrt_run(a->nnrt);

    qtk_nnrt_get_output(a->nnrt, &output_real, 0);
    qtk_nnrt_get_output(a->nnrt, &output_imag, 1);
    qtk_nnrt_get_output(a->nnrt, &a->cache_h0, 4);
    qtk_nnrt_get_output(a->nnrt, &a->cache_c0, 5);

    nn_post_(a, qtk_nnrt_value_get_data(a->nnrt, output_real),
             qtk_nnrt_value_get_data(a->nnrt, output_imag));

    qtk_nnrt_value_release(a->nnrt, x);
    qtk_nnrt_value_release(a->nnrt, cache_h0);
    qtk_nnrt_value_release(a->nnrt, cache_c0);
    qtk_nnrt_value_release(a->nnrt, mix_spec);
    qtk_nnrt_value_release(a->nnrt, output_real);
    qtk_nnrt_value_release(a->nnrt, output_imag);
    if (fake_cache) {
        wtk_free(fake_cache);
        wtk_free(fake_cache1);
    }
}

static void forward_crnn_lstm(qtk_ahs_t *a) {
    float *fake_cache[4];
    fake_cache[0] = NULL;
    int i;
    qtk_nnrt_value_t cache[4], x, y, mix_spec;
    qtk_nnrt_value_t output_real, output_imag;
    int64_t x_shape[4] = {a->cfg->chunk_sz, 3, 1, a->cfg->nbin};
    int64_t y_shape[4] = {a->cfg->chunk_sz, 1, 1, a->cfg->nbin};
    int64_t mix_spec_shape[4] = {a->cfg->chunk_sz, 2, 1, a->cfg->nbin};
    if (a->cache[0]) {
        for(i = 0; i < 4; i++){
            cache[i] = a->cache[i];
        }
    } else {
        int64_t cache_shape[4];
        for(i = 0; i < 4; i++){
            cache[i] = a->cache[i];
            if(i > 0 && i < 3){
                qtk_nnrt_get_input_shape(a->nnrt, i + 2, cache_shape, 3);
                fake_cache[i] = wtk_calloc(
                    cache_shape[0] * cache_shape[1] * cache_shape[2], sizeof(float));
                cache[i] = qtk_nnrt_value_create_external(
                    a->nnrt, QTK_NNRT_VALUE_ELEM_F32, cache_shape, 3, fake_cache[i]);
            }else{
                qtk_nnrt_get_input_shape(a->nnrt, i + 2, cache_shape, 4);
                fake_cache[i] = wtk_calloc(
                    cache_shape[0] * cache_shape[1] * cache_shape[2]* cache_shape[3], sizeof(float));
                cache[i] = qtk_nnrt_value_create_external(
                    a->nnrt, QTK_NNRT_VALUE_ELEM_F32, cache_shape, 4, fake_cache[i]);
            }
        }
    }
    x = qtk_nnrt_value_create_external(a->nnrt, QTK_NNRT_VALUE_ELEM_F32,
                                       x_shape, 4, a->x);
    y = qtk_nnrt_value_create_external(a->nnrt, QTK_NNRT_VALUE_ELEM_F32,
                                       y_shape, 4, a->y);
    mix_spec = qtk_nnrt_value_create_external(a->nnrt, QTK_NNRT_VALUE_ELEM_F32,
                                              mix_spec_shape, 4, a->mix_spec);

    qtk_nnrt_feed(a->nnrt, x, 0);
    qtk_nnrt_feed(a->nnrt, y, 1);
    for(i = 0; i < 4; i++) {
        qtk_nnrt_feed(a->nnrt, cache[i], i + 2);
    }
    qtk_nnrt_feed(a->nnrt, mix_spec, i + 2);

    qtk_nnrt_run(a->nnrt);

    qtk_nnrt_get_output(a->nnrt, &output_real, 0);
    qtk_nnrt_get_output(a->nnrt, &output_imag, 1);
    for(i = 0; i < 4; i++) {
        qtk_nnrt_get_output(a->nnrt, &a->cache[i], i + 3);
    }

    if(a->cfg->use_mask){
        qtk_nnrt_value_t mask;
        qtk_nnrt_get_output(a->nnrt, &mask, 2);
        nn_post_mask_(a, qtk_nnrt_value_get_data(a->nnrt, output_real),
                qtk_nnrt_value_get_data(a->nnrt, output_imag),
                qtk_nnrt_value_get_data(a->nnrt, mask),
                NULL);
        qtk_nnrt_value_release(a->nnrt, mask);
        if(!a->cfg->use_mask_sp){
            nn_post_(a, qtk_nnrt_value_get_data(a->nnrt, output_real),
                qtk_nnrt_value_get_data(a->nnrt, output_imag));
        }
    }else{
        if(a->cfg->use_mt){
            qtk_ahs_mt_value_t *val = (qtk_ahs_mt_value_t*)wtk_vpool2_pop(a->pool);
            memcpy(val->a,qtk_nnrt_value_get_data(a->nnrt, output_real),sizeof(float)*a->cfg->nbin);
            memcpy(val->b,qtk_nnrt_value_get_data(a->nnrt, output_imag),sizeof(float)*a->cfg->nbin);
            wtk_queue_push(&(a->mt_q),&(val->q_n));
        }else{
            nn_post_(a, qtk_nnrt_value_get_data(a->nnrt, output_real),
                    qtk_nnrt_value_get_data(a->nnrt, output_imag));
        }

    }

    qtk_nnrt_value_release(a->nnrt, x);
    qtk_nnrt_value_release(a->nnrt, y);
    for(i = 0; i < 4; i++) {
        qtk_nnrt_value_release(a->nnrt, cache[i]);
    }
    qtk_nnrt_value_release(a->nnrt, mix_spec);
    qtk_nnrt_value_release(a->nnrt, output_real);
    qtk_nnrt_value_release(a->nnrt, output_imag);
    if (fake_cache[0]) {
        for(i = 0; i < 4; i++) {
            wtk_free(fake_cache[i]);
        }
    }
}

static void forward_werk(qtk_ahs_t *a) {
    float *fake_cache[12];
    fake_cache[0] = NULL;
    int i;
    qtk_nnrt_value_t cache[12], x, y, mix_spec;
    qtk_nnrt_value_t output_real, output_imag;
    int64_t x_shape[4] = {a->cfg->chunk_sz, 2, 1, a->cfg->nbin};
    int64_t y_shape[4] = {a->cfg->chunk_sz, 2, 1, a->cfg->nbin};
    int64_t mix_spec_shape[4] = {a->cfg->chunk_sz, 2, 1, a->cfg->nbin};
    if (a->cache[0]) {
        for(i = 0; i < 12; i++){
            cache[i] = a->cache[i];
        }
    } else {
        int64_t cache_shape[4];
        for(i = 0; i < 12; i++){
            cache[i] = a->cache[i];
            if(i == 8){
                qtk_nnrt_get_input_shape(a->nnrt, i + 2, cache_shape, 3);
                fake_cache[i] = wtk_calloc(
                    cache_shape[0] * cache_shape[1] * cache_shape[2], sizeof(float));
                cache[i] = qtk_nnrt_value_create_external(
                    a->nnrt, QTK_NNRT_VALUE_ELEM_F32, cache_shape, 3, fake_cache[i]);
            }else{
                qtk_nnrt_get_input_shape(a->nnrt, i + 2, cache_shape, 4);
                fake_cache[i] = wtk_calloc(
                    cache_shape[0] * cache_shape[1] * cache_shape[2]* cache_shape[3], sizeof(float));
                cache[i] = qtk_nnrt_value_create_external(
                    a->nnrt, QTK_NNRT_VALUE_ELEM_F32, cache_shape, 4, fake_cache[i]);
            }
        }
    }
    x = qtk_nnrt_value_create_external(a->nnrt, QTK_NNRT_VALUE_ELEM_F32,
                                       x_shape, 4, a->x);
    y = qtk_nnrt_value_create_external(a->nnrt, QTK_NNRT_VALUE_ELEM_F32,
                                       y_shape, 4, a->y);
    mix_spec = qtk_nnrt_value_create_external(a->nnrt, QTK_NNRT_VALUE_ELEM_F32,
                                              mix_spec_shape, 4, a->mix_spec);

    qtk_nnrt_feed(a->nnrt, y, 0);
    qtk_nnrt_feed(a->nnrt, x, 1);
    for(i = 0; i < 12; i++) {
        qtk_nnrt_feed(a->nnrt, cache[i], i + 2);
    }
    qtk_nnrt_feed(a->nnrt, mix_spec, i + 2);

    qtk_nnrt_run(a->nnrt);

    qtk_nnrt_get_output(a->nnrt, &output_real, 0);
    qtk_nnrt_get_output(a->nnrt, &output_imag, 1);
    for(i = 0; i < 12; i++) {
        qtk_nnrt_get_output(a->nnrt, &a->cache[i], i + 2);
    }

    nn_post_(a, qtk_nnrt_value_get_data(a->nnrt, output_real),
             qtk_nnrt_value_get_data(a->nnrt, output_imag));

    qtk_nnrt_value_release(a->nnrt, x);
    qtk_nnrt_value_release(a->nnrt, y);
    for(i = 0; i < 12; i++) {
        qtk_nnrt_value_release(a->nnrt, cache[i]);
    }
    qtk_nnrt_value_release(a->nnrt, mix_spec);
    qtk_nnrt_value_release(a->nnrt, output_real);
    qtk_nnrt_value_release(a->nnrt, output_imag);
    if (fake_cache[0]) {
        for(i = 0; i < 12; i++) {
            wtk_free(fake_cache[i]);
        }
    }
}

int qtk_ahs_run_nn(qtk_ahs_t* a,wtk_thread_t *t)
{
    qtk_ahs_evt_t *evt;
    wtk_blockqueue_t *q = &(a->nn_input_q);
    wtk_queue_node_t *qn;
    int timeout = -1;
    int b;
    int nbin = a->cfg->window_sz / 2 + 1;

	while(a->run){
        qn=wtk_blockqueue_pop(q,timeout,&b);
        if(!qn){
            continue;
        }
		evt = data_offset(qn,qtk_ahs_evt_t,q_n);
        qtk_ahs_mt_value_t *val = (qtk_ahs_mt_value_t*)wtk_vpool2_pop(a->pool);
        //wtk_debug("%d\n",evt->index);
        switch (evt->type)
        {
        case QTK_AHS_EVT_FEED:
            //qtk_crnnoise2_run(a->kalman_nnrt2, evt->x, a->est_freq);
            //nn_post2_(a,a->est_freq);

            if(a->kalman_nnrt){
                qtk_crnnoise_run(a->kalman_nnrt, evt->x, a->est_freq);
                memcpy(val->x, a->est_freq, sizeof(wtk_complex_t)*nbin);
                wtk_queue_push(&(a->mt_q),&(val->q_n));
                //nn_post2_(a,a->est_freq);
            }else if (a->nnrt)
            {
                int i;
                int nbin = a->cfg->window_sz / 2 + 1;
                float *dst = a->x;
                float *y_re_ptr = dst;
                float *y_im_ptr = dst + nbin;
                float *Y_ptr = dst + 2 * nbin;
                float *dst2 = a->y;
                float *R_ptr = dst2;
                wtk_complex_t cpx;
                wtk_complex_t *fftx = a->fftx;

                //wtk_rfft_process_fft(a->fft, a->F, a->frame_yt_tmp);
                if(a->qmmse2){
                    for (i = 0; i < nbin; i++) {
                        cpx = evt->x[i];//wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
                        fftx[i].a = cpx.a;
                        fftx[i].b = cpx.b;
                    }
                    wtk_qmmse_denoise(a->qmmse2, fftx);
                    for (i = 0; i < nbin; i++) {
                        y_re_ptr[i] = fftx[i].a;
                        y_im_ptr[i] = fftx[i].b;
                        Y_ptr[i] = sqrtf(fftx[i].a * fftx[i].a + fftx[i].b * fftx[i].b);
                    }
                }else{
                    for (i = 0; i < nbin; i++) {
                        cpx = evt->x[i];//wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
                        //y_re_ptr[i] = cpx[i].a;
                        //y_im_ptr[i] = cpx[i].b;
                        y_re_ptr[i] = cpx.a;
                        y_im_ptr[i] = cpx.b;
                        //wtk_debug("%f %f\n",y_re_ptr[i],y_im_ptr[i]);
                        Y_ptr[i] = sqrtf(cpx.a * cpx.a + cpx.b * cpx.b);
                    }
                }
                //wtk_rfft_process_fft(a->fft, a->F, a->frame_rt_tmp);
                for (i = 0; i < nbin; i++) {
                    cpx = evt->z[i];//wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
                    R_ptr[i] = sqrtf(cpx.a * cpx.a + cpx.b * cpx.b);
                }
                memcpy(a->mix_spec, y_re_ptr,
                    sizeof(float) * 2 * a->cfg->nbin);
                forward_crnn_lstm(a);
            }else if(a->kalman_nnrt2){
                qtk_crnnoise2_run(a->kalman_nnrt2, evt->x, a->est_freq);
                memcpy(val->x, a->est_freq, sizeof(wtk_complex_t)*nbin);
                wtk_queue_push(&(a->mt_q),&(val->q_n));
                //nn_post2_(a,a->est_freq);
            }else{
                memcpy(val->x, evt->x, sizeof(wtk_complex_t)*nbin);
                wtk_queue_push(&(a->mt_q),&(val->q_n));
                //nn_post2_(a,evt->x);
            }
            break;
        case QTK_AHS_EVT_END:
            wtk_sem_release(&(a->end_wait_sem),1);
            break;
        default:
            break;
        }
        qtk_ahs_push_evt(a,evt);
    }
    return 0;
}

void qtk_ahs_feed_nn_thread(qtk_ahs_t *a, wtk_complex_t *in){
    qtk_ahs_evt_t *evt;
    evt = qtk_ahs_pop_evt(a);
    //evt->x = in;
    memcpy(evt->x, in, sizeof(wtk_complex_t)*129);
    if(a->nnrt){
        memcpy(evt->z, a->km_z, sizeof(wtk_complex_t)*129);
    }
    evt->type = QTK_AHS_EVT_FEED;
    wtk_blockqueue_push(&(a->nn_input_q),&(evt->q_n));
}

void wtk_ahs_on_gainnet(qtk_ahs_t *a, float *out, int len, int is_end){
    //nn_post2_(a,a->est_freq);
    int nbin = a->cfg->window_sz / 2 + 1 ,i;
    float *real,*imag;
    wtk_complex_t *cpx = a->kalman->s_hat;

    qtk_ahs_erb_bs(a->erb, out, a->gainnet_out, 2);
    real = a->gainnet_out;
    imag = a->gainnet_out + nbin;
    for(i = 0; i < nbin; i++){
        a->est_freq[i].a = cpx->a * *real - cpx->b * *imag;
        a->est_freq[i].b = cpx->a * *imag + cpx->b * *real;
        cpx++;
        real++;
        imag++;
    }
    nn_post2_(a,a->est_freq);
}

static void forward_kalman(qtk_ahs_t *a) {
    qtk_kalman_update(a->kalman, a->km_y, a->cfg->window_sz/2 + 1, 1, a->km_x);
    wtk_complex_t* output = a->kalman->s_hat;
    if(a->kalman2){
        if(a->stop_idx > 0){
            if(a->st_idx < 0){
                wtk_strbuf_push_float_front(a->kalman2_sp,NULL,-a->st_idx);
            }else{
                a->st_idx += a->cfg->hop_sz;
            }
            float *window = a->cfg->window;
            float *p = (float*)a->kalman2_sp->data;
            int i,j;
            int nbin = a->cfg->window_sz / 2 + 1;
            wtk_complex_t cpx,*km_x = a->km_x;
            for (i = 0; i < a->cfg->window_sz; i++) {
                a->frame_rt_tmp[i] = window[i] * p[i];
            }
            wtk_rfft_process_fft(a->fft, a->F, a->frame_rt_tmp);
            for (i = 0; i < nbin; i++) {
                cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
                km_x->a = cpx.a;
                km_x->b = cpx.b;
                km_x++;
            }
            int order = a->cfg->km.kalman_order;
            if(order > 1){
                float *p = a->frame_kalman_tmp;
                float *p2 = a->frame_rt_tmp;
                for(j = 1; j < order; j++){
                    for(i = 0; i < a->cfg->window_sz; i++){
                        *p = pow(*p2,j + 1);
                        p++;
                        p2++;
                    }
                    wtk_rfft_process_fft(a->fft, a->F, a->frame_kalman_tmp);
                    for (i = 0; i < nbin; i++) {
                        cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
                        km_x->a = cpx.a;
                        km_x->b = cpx.b;
                        km_x++;
                    }
                }
            }
            wtk_strbuf_pop(a->kalman2_sp,NULL,sizeof(float)*a->cfg->hop_sz);
        }else{
            a->stop_idx += a->cfg->hop_sz;
            a->st_idx += a->cfg->hop_sz;
            wtk_complex_zero(a->km_x,a->cfg->window_sz/2 + 1);
        }

        qtk_kalman_update(a->kalman2, a->kalman->s_hat, a->cfg->window_sz/2 + 1, 1, a->km_x);
        output = a->kalman2->s_hat;
    }

    if(a->cfg->use_mt){
        qtk_ahs_feed_nn_thread(a, output);
        return;
    }

    if(a->kalman_nnrt){
        qtk_crnnoise_run(a->kalman_nnrt, output, a->est_freq);
        nn_post2_(a,a->est_freq);
    }else if (a->nnrt)
    {
        int i;
        int nbin = a->cfg->window_sz / 2 + 1;
        float *dst = a->x;
        float *y_re_ptr = dst;
        float *y_im_ptr = dst + nbin;
        float *Y_ptr = dst + 2 * nbin;
        float *dst2 = a->y;
        float *R_ptr = dst2;
        wtk_complex_t cpx;
        wtk_complex_t *fftx = a->fftx;

        //wtk_rfft_process_fft(a->fft, a->F, a->frame_yt_tmp);
        if(a->qmmse2){
            for (i = 0; i < nbin; i++) {
                cpx = output[i];//wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
                fftx[i].a = cpx.a;
                fftx[i].b = cpx.b;
            }
            wtk_qmmse_denoise(a->qmmse2, fftx);
            for (i = 0; i < nbin; i++) {
                y_re_ptr[i] = fftx[i].a;
                y_im_ptr[i] = fftx[i].b;
                Y_ptr[i] = sqrtf(fftx[i].a * fftx[i].a + fftx[i].b * fftx[i].b);
            }
        }else{
            for (i = 0; i < nbin; i++) {
                cpx = output[i];//wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
                //y_re_ptr[i] = cpx[i].a;
                //y_im_ptr[i] = cpx[i].b;
                y_re_ptr[i] = cpx.a;
                y_im_ptr[i] = cpx.b;
                //wtk_debug("%f %f\n",y_re_ptr[i],y_im_ptr[i]);
                Y_ptr[i] = sqrtf(cpx.a * cpx.a + cpx.b * cpx.b);
            }
        }
        //wtk_rfft_process_fft(a->fft, a->F, a->frame_rt_tmp);
        for (i = 0; i < nbin; i++) {
            cpx = a->km_z[i];//wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
            R_ptr[i] = sqrtf(cpx.a * cpx.a + cpx.b * cpx.b);
        }
        memcpy(a->mix_spec, y_re_ptr,
            sizeof(float) * 2 * a->cfg->nbin);
        forward_crnn_lstm(a);

    }else if(a->kalman_nnrt2){
        qtk_crnnoise2_run(a->kalman_nnrt2, output, a->est_freq);
        nn_post2_(a,a->est_freq);
    }else if(a->cfg->use_gainnet){
        int nbin = a->cfg->window_sz / 2 + 1,i;
        float *dst = a->erb_mic;
        float *y_re_ptr = dst;
        float *y_im_ptr = dst + nbin;
        float *Y_ptr = dst + 2 * nbin;
        float *dst2 = dst + 3 * nbin;

        for(i = 0; i < nbin; i++){
            y_re_ptr[i] = output[i].a;
            y_im_ptr[i] = output[i].b;
            Y_ptr[i] = sqrtf(output[i].a * output[i].a + output[i].b * output[i].b);
            dst2[i] = sqrtf(a->km_x[i].a * a->km_x[i].a + a->km_x[i].b * a->km_x[i].b);
        }
        qtk_ahs_erb_bm(a->erb, a->erb_mic, a->gainnet_in, 4);
        wtk_bbonenet_feed(a->gainnet, a->gainnet_in, a->gainnet_indim, 1, 0);
    }else {
        nn_post2_(a,output);
    }
}

static void compute_feature_lstm(qtk_ahs_t *a) {
    int i;
    int nbin = a->cfg->window_sz / 2 + 1;
    float *dst = a->x + a->cfg->feat_dim * a->chunk_pos;
    float *Y_ptr = dst;
    float *R_ptr = dst + nbin;
    float *y_re_ptr = dst + 2 * nbin;
    float *y_im_ptr = dst + 3 * nbin;
    wtk_complex_t cpx;
    wtk_complex_t *fftx = a->fftx;

    //wtk_drft_frame_analysis22(a->drft, a->F, analysis_mem, cpx, pv,
    //                          a->cfg->window_sz, window);
    wtk_rfft_process_fft(a->fft, a->F, a->frame_yt_tmp);
    if(a->qmmse2){
        for (i = 0; i < nbin; i++) {
            cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
            fftx[i].a = cpx.a;
            fftx[i].b = cpx.b;
        }
        wtk_qmmse_denoise(a->qmmse2, fftx);
        for (i = 0; i < nbin; i++) {
            y_re_ptr[i] = fftx[i].a;
            y_im_ptr[i] = fftx[i].b;
            Y_ptr[i] = sqrtf(fftx[i].a * fftx[i].a + fftx[i].b * fftx[i].b);
        }
    }else{
        for (i = 0; i < nbin; i++) {
            cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
            //y_re_ptr[i] = cpx[i].a;
            //y_im_ptr[i] = cpx[i].b;
            y_re_ptr[i] = cpx.a;
            y_im_ptr[i] = cpx.b;
            //wtk_debug("%f %f\n",y_re_ptr[i],y_im_ptr[i]);
            Y_ptr[i] = sqrtf(cpx.a * cpx.a + cpx.b * cpx.b);
        }
    }
    //wtk_drft_frame_analysis22(a->drft, a->F, analysis_mem_sp, cpx, pv_sp,
    //                          a->cfg->window_sz, window);
    wtk_rfft_process_fft(a->fft, a->F, a->frame_rt_tmp);
    for (i = 0; i < nbin; i++) {
        cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
        R_ptr[i] = sqrtf(cpx.a * cpx.a + cpx.b * cpx.b);
    }
    memcpy(a->mix_spec + a->cfg->nbin * 2 * a->chunk_pos, y_re_ptr,
           sizeof(float) * 2 * a->cfg->nbin);
}

static void compute_feature_kalman(qtk_ahs_t *a) {
    int i,j;
    int nbin = a->cfg->window_sz / 2 + 1;
    wtk_complex_t *fftx = a->fftx;
    wtk_complex_t *km_x = a->km_x;
    wtk_complex_t *km_y = a->km_y;
    wtk_complex_t cpx;
    //torch.stack(mic.real,mic.imag) dim = 1

    wtk_rfft_process_fft(a->fft, a->F, a->frame_yt_tmp);
    if(a->qmmse2){
        for (i = 0; i < nbin; i++) {
            cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
            fftx[i].a = cpx.a;
            fftx[i].b = cpx.b;
        }
        wtk_qmmse_denoise(a->qmmse2, fftx);
        for (i = 0; i < nbin; i++) {
            km_y[i].a = fftx[i].a;
            km_y[i].b = fftx[i].b;
        }
    }else{
        for (i = 0; i < nbin; i++) {
            cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
            km_y[i].a = cpx.a;
            km_y[i].b = cpx.b;
        }
    }

    if(a->eq_gain && !a->cfg->is_eq2_kalmanout){
        for(int i = 0; i < nbin; i++){
            km_y[i].a *= a->eq_gain[i];
            km_y[i].b *= a->eq_gain[i];
        }
    }

    wtk_rfft_process_fft(a->fft, a->F, a->frame_rt_tmp);
    for (i = 0; i < nbin; i++) {
        cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
        km_x->a = cpx.a;
        km_x->b = cpx.b;
        km_x++;
    }

    int order = a->cfg->km.kalman_order;
    if(order > 1){
        float *p = a->frame_kalman_tmp;
        float *p2 = a->frame_rt_tmp;
        for(j = 1; j < order; j++){
            for(i = 0; i < a->cfg->window_sz; i++){
                *p = pow(*p2,j + 1);
                p++;
                p2++;
            }
            wtk_rfft_process_fft(a->fft, a->F, a->frame_kalman_tmp);
            for (i = 0; i < nbin; i++) {
                cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
                km_x->a = cpx.a;
                km_x->b = cpx.b;
                km_x++;
            }
        }
    }

    if(a->nnrt){
        wtk_complex_t *km_z = a->km_z;
        for(i = 0; i < a->cfg->window_sz; i++){
            a->frame_rt_tmp[i] = a->frame_rt_tmp[i]/a->cfg->pre_scale;
        }
        wtk_rfft_process_fft(a->fft, a->F, a->frame_rt_tmp);
        for (i = 0; i < nbin; i++) {
            cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
            km_z[i].a = cpx.a;
            km_z[i].b = cpx.b;
        }
    }

}

static void compute_feature_lstm2(qtk_ahs_t *a) {
    int i;
    int nbin = a->cfg->window_sz / 2 + 1;
    float *dst = a->x + a->cfg->feat_dim * a->chunk_pos;
    float *Y_ptr = dst;
    float *R_ptr = dst + nbin;
    float *y_re_ptr = dst + 2 * nbin;
    float *y_im_ptr = dst + 3 * nbin;
    float *r_re_ptr = dst + 4 * nbin;
    float *r_im_ptr = dst + 5 * nbin;
    wtk_complex_t cpx;
    wtk_complex_t *fftx = a->fftx;

    //wtk_drft_frame_analysis22(a->drft, a->F, analysis_mem, cpx, pv,
    //                          a->cfg->window_sz, window);
    wtk_rfft_process_fft(a->fft, a->F, a->frame_yt_tmp);
    if(a->qmmse2){
        for (i = 0; i < nbin; i++) {
            cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
            fftx[i].a = cpx.a;
            fftx[i].b = cpx.b;
        }
        wtk_qmmse_denoise(a->qmmse2, fftx);
        for (i = 0; i < nbin; i++) {
            y_re_ptr[i] = fftx[i].a;
            y_im_ptr[i] = fftx[i].b;
            Y_ptr[i] = sqrtf(fftx[i].a * fftx[i].a + fftx[i].b * fftx[i].b);
        }
    }else{
        for (i = 0; i < nbin; i++) {
            cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
            //y_re_ptr[i] = cpx[i].a;
            //y_im_ptr[i] = cpx[i].b;
            y_re_ptr[i] = cpx.a;
            y_im_ptr[i] = cpx.b;
            //wtk_debug("%f %f\n",y_re_ptr[i],y_im_ptr[i]);
            Y_ptr[i] = sqrtf(cpx.a * cpx.a + cpx.b * cpx.b);
        }
    }
    //wtk_drft_frame_analysis22(a->drft, a->F, analysis_mem_sp, cpx, pv_sp,
    //                          a->cfg->window_sz, window);
    wtk_rfft_process_fft(a->fft, a->F, a->frame_rt_tmp);
    for (i = 0; i < nbin; i++) {
        cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
        r_re_ptr[i] = cpx.a;
        r_im_ptr[i] = cpx.b;
        R_ptr[i] = sqrtf(cpx.a * cpx.a + cpx.b * cpx.b);
    }
    memcpy(a->mix_spec + a->cfg->nbin * 2 * a->chunk_pos, y_re_ptr,
           sizeof(float) * 2 * a->cfg->nbin);
}

static void compute_feature_werk(qtk_ahs_t *a) {
    int i;
    int nbin = a->cfg->window_sz / 2 + 1;
    float *mf_real = a->x + a->cfg->feat_dim * a->chunk_pos;
    float *mf_image = mf_real + nbin;
    float *rf_real = a->y + a->cfg->feat_dim * a->chunk_pos;
    float *rf_image = rf_real + nbin;
    wtk_complex_t cpx;
    wtk_complex_t *fftx = a->fftx;
    //torch.stack(mic.real,mic.imag) dim = 1
    wtk_rfft_process_fft(a->fft, a->F, a->frame_yt_tmp);
    if(a->qmmse2){
        for (i = 0; i < nbin; i++) {
            cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
            fftx[i].a = cpx.a;
            fftx[i].b = cpx.b;
        }
        wtk_qmmse_denoise(a->qmmse2, fftx);
        for (i = 0; i < nbin; i++) {
            mf_real[i] = fftx[i].a;
            mf_image[i] = fftx[i].b;
        }
    }else{
        for (i = 0; i < nbin; i++) {
            cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
            mf_real[i] = cpx.a;
            mf_image[i] = cpx.b;
        }
    }

    wtk_rfft_process_fft(a->fft, a->F, a->frame_rt_tmp);
    for (i = 0; i < nbin; i++) {
        cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
        rf_real[i] = cpx.a;
        rf_image[i] = cpx.b;
    }

    memcpy(a->mix_spec + a->cfg->nbin * 2 * a->chunk_pos, mf_real,
           sizeof(float) * 2 * a->cfg->nbin);
}

static void compute_feature_crnn_lstm(qtk_ahs_t *a) {
    int i;
    int nbin = a->cfg->window_sz / 2 + 1;
    float *dst = a->x + a->cfg->feat_dim * a->chunk_pos;
    float *y_re_ptr = dst;
    float *y_im_ptr = dst + nbin;
    float *Y_ptr = dst + 2 * nbin;
    float *dst2 = a->y + a->cfg->feat_dim * a->chunk_pos;
    float *R_ptr = dst2;
    wtk_complex_t cpx;
    wtk_complex_t *fftx = a->fftx;

    wtk_rfft_process_fft(a->fft, a->F, a->frame_yt_tmp);
    if(a->qmmse2){
        for (i = 0; i < nbin; i++) {
            cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
            fftx[i].a = cpx.a;
            fftx[i].b = cpx.b;
        }
        wtk_qmmse_denoise(a->qmmse2, fftx);
        for (i = 0; i < nbin; i++) {
            y_re_ptr[i] = fftx[i].a;
            y_im_ptr[i] = fftx[i].b;
            Y_ptr[i] = sqrtf(fftx[i].a * fftx[i].a + fftx[i].b * fftx[i].b);
        }
    }else{
        for (i = 0; i < nbin; i++) {
            cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
            //y_re_ptr[i] = cpx[i].a;
            //y_im_ptr[i] = cpx[i].b;
            y_re_ptr[i] = cpx.a;
            y_im_ptr[i] = cpx.b;
            //wtk_debug("%f %f\n",y_re_ptr[i],y_im_ptr[i]);
            Y_ptr[i] = sqrtf(cpx.a * cpx.a + cpx.b * cpx.b);
        }
    }
    wtk_rfft_process_fft(a->fft, a->F, a->frame_rt_tmp);
    for (i = 0; i < nbin; i++) {
        cpx = wtk_rfft_get_value(a->F, a->cfg->window_sz, i);
        R_ptr[i] = sqrtf(cpx.a * cpx.a + cpx.b * cpx.b);
    }
    memcpy(a->mix_spec + a->cfg->nbin * 2 * a->chunk_pos, y_re_ptr,
           sizeof(float) * 2 * a->cfg->nbin);
}

static void process_frame_(qtk_ahs_t *a) {
    int i;
    float *window = a->cfg->window;

    if(a->conv2){
        float tmp_rt[128];
        memcpy(tmp_rt,a->frame_rt + 128,sizeof(float) * 128);
        qtk_linear_conv_conv1d_calc(a->conv2,a->frame_rt + 128);
        for (i = 0; i < a->cfg->hop_sz; i++) {
            a->frame_yt[i + 128] += tmp_rt[i] * a->cfg->gain;
        }
    }

    if(a->kalman2_sp){
        wtk_strbuf_push_float(a->kalman2_sp,a->frame_rt,a->cfg->hop_sz);
    }

    for (i = 0; i < a->cfg->window_sz; i++) {
        a->frame_yt_tmp[i] = window[i] * a->frame_yt[i];
        a->frame_rt_tmp[i] = window[i] * a->frame_rt[i];
    }

    switch (a->cfg->type)
    {
    case 0:
        compute_feature_lstm(a);
        break;
    case 1:
        compute_feature_werk(a);
        break;
    case 2:
        compute_feature_lstm2(a);
        break;
    case 3:
        compute_feature_kalman(a);
        break;
    case 7:
        compute_feature_crnn_lstm(a);
    default:
        break;
    }

    a->chunk_pos++;
    if (a->chunk_pos == a->cfg->chunk_sz) {
        switch (a->cfg->type)
        {
        case 0:
            forward_lstm(a);
            break;
        case 1:
            forward_werk(a);
            break;
        case 2:
            forward_lstm2(a);
            break;
        case 3:
            forward_kalman(a);
            break;
        case 7:
            forward_crnn_lstm(a);
            break;
        default:
            break;
        }
        a->chunk_pos = 0;
    }
}

static int post_feed_(qtk_ahs_t *a, float *yt, float *rt, int len) {
    while (len > 0) {
        int need_len = a->cfg->window_sz - a->pos;
        int consume_len = min(need_len, len);
        memcpy(a->frame_yt + a->pos, yt, sizeof(float) * consume_len);
        memcpy(a->frame_rt + a->pos, rt, sizeof(float) * consume_len);
        a->pos += consume_len;
        rt += consume_len;
        yt += consume_len;
        len -= consume_len;
        if (a->pos == a->cfg->window_sz) {
            process_frame_(a);
            memmove(a->frame_yt, a->frame_yt + a->cfg->hop_sz,
                    sizeof(float) * a->cfg->pad_sz);
            memmove(a->frame_rt, a->frame_rt + a->cfg->hop_sz,
                    sizeof(float) * a->cfg->pad_sz);
            a->pos = a->cfg->pad_sz;
        }
    }
    return 0;
}

static int post_feed_short_(qtk_ahs_t *a, short *yt, short *rt, int len) {
    int i;
    short *dc;
    if(a->cfg->use_loop){
        dc = (short*)a->dcache->data;
    }
    while (len > 0) {
        int need_len = a->cfg->window_sz - a->pos;
        int consume_len = min(need_len, len);
        for (i = 0; i < consume_len; i++) {
            a->frame_yt[a->pos + i] = yt[i] / 32768.0f;
            a->frame_rt[a->pos + i] = rt[i] / 32768.0f;
            if(a->cfg->use_loop){
                a->frame_yt[a->pos + i] += (dc[i] / 32768.0f) * a->cfg->gain * a->cfg->rir_gain;
            }
        }
        a->pos += consume_len;
        rt += consume_len;
        yt += consume_len;
        if(a->cfg->use_loop){
            dc += consume_len;
        }
        len -= consume_len;
        if (a->pos == a->cfg->window_sz) {
            process_frame_(a);
            memmove(a->frame_yt, a->frame_yt + a->cfg->hop_sz,
                    sizeof(float) * a->cfg->pad_sz);
            memmove(a->frame_rt, a->frame_rt + a->cfg->hop_sz,
                    sizeof(float) * a->cfg->pad_sz);
            a->pos = a->cfg->pad_sz;
        }
    }
    return 0;
}

int qtk_ahs_feed_float(qtk_ahs_t *a, float *yt, float *rt, int len) {
    int i;
    if (!a->start) {
        int need_len = a->cfg->pad_sz + 1 - a->pos;
        int consume_len = min(need_len, len);
        memcpy(a->rt_pad_tmp + a->pos, rt, sizeof(float) * consume_len);
        memcpy(a->yt_pad_tmp + a->pos, yt, sizeof(float) * consume_len);
        a->pos += consume_len;
        rt += consume_len;
        yt += consume_len;
        len -= consume_len;
        if (a->pos == a->cfg->pad_sz + 1) {
            for (i = 0; i < a->cfg->pad_sz; i++) {
                a->frame_rt[a->cfg->pad_sz - (i + 1)] = a->rt_pad_tmp[i + 1];
                a->frame_yt[a->cfg->pad_sz - (i + 1)] = a->yt_pad_tmp[i + 1];
            }
            a->pos = a->cfg->pad_sz;
            post_feed_(a, a->yt_pad_tmp, a->rt_pad_tmp, a->cfg->pad_sz + 1);
            post_feed_(a, yt, rt, len);
            a->start = 1;
        }
        return 0;
    }
    return post_feed_(a, yt, rt, len);
}

int qtk_ahs_feed_short(qtk_ahs_t *a, short *yt, short *rt, int len) {
    int i;
    if (!a->start) {
        int need_len = a->cfg->pad_sz + 1 - a->pos;
        int consume_len = min(need_len, len);
        for (i = 0; i < consume_len; i++) {
            a->yt_pad_tmp[a->pos + i] = yt[i] / 32768.0f;
            a->rt_pad_tmp[a->pos + i] = rt[i] / 32768.0f;
        }
        a->pos += consume_len;
        rt += consume_len;
        yt += consume_len;
        len -= consume_len;
        if (a->pos == a->cfg->pad_sz + 1) {
            for (i = 0; i < a->cfg->pad_sz; i++) {
                a->frame_rt[a->cfg->pad_sz - (i + 1)] = a->rt_pad_tmp[i + 1];
                a->frame_yt[a->cfg->pad_sz - (i + 1)] = a->yt_pad_tmp[i + 1];
            }
            a->pos = a->cfg->pad_sz;
            post_feed_(a, a->yt_pad_tmp, a->rt_pad_tmp, a->cfg->pad_sz + 1);
            post_feed_short_(a, yt, rt, len);
            a->start = 1;
        }
        return 0;
    }
    return post_feed_short_(a, yt, rt, len);
}

void qtk_ahs_feed(qtk_ahs_t *a, short *data, int len, int is_end) {
    wtk_strbuf_t *mic = a->mic;
    wtk_strbuf_t *sp = a->sp;
    short *yt, *rt;
    int fsize = a->cfg->window_sz / 2;
    int length,len_sp;
    wtk_queue_node_t *qn;
    qtk_ahs_mt_value_t *val;
    wtk_strbuf_push(mic, (char *)data, len * sizeof(short));
    if (sp->pos == 0) {
        wtk_strbuf_push(sp, NULL, a->delay * sizeof(short));
        if(a->dcache) {
            wtk_strbuf_push(a->dcache,NULL,a->loop_delay * sizeof(short));
        }
    }
    length = mic->pos / sizeof(short);

    if(a->vad){
        wtk_queue_t *q=&(a->vad->output_q);
        wtk_queue_node_t *qn;
        wtk_vframe_t *vf=NULL;

        wtk_kvad_feed(a->vad,data,len,is_end);
        while(1){
            qn=wtk_queue_pop(q);
            if(!qn){break;}
            vf=data_offset2(qn,wtk_vframe_t,q_n);
            a->t_vad += 10;
            a->vad_idx++;
            //wtk_debug("%d %d %f\n",vadidx++,vadidx*10,vf->speechlike);
            int i;
            if(a->vad_idx %2 == 0){
                if(a->in_offset % 40 == 0){
                    for(i = 0; i < 20; i++){
                        a->vad_probs[i] = vf->speechlike;
                        //wtk_debug("%d %f\n",i,a->vad_probs[i]);
                    }
                    a->in_offset += 20;
                }else if(a->in_offset % 20 == 0){
                    for(i = 0; i < 20; i++){
                        a->vad_probs[i + 20] = vf->speechlike;
                        //wtk_debug("%d %f\n",i + 20,a->vad_probs[i + 20]);
                    }
                    a->in_offset += 20;
                }
            }
            wtk_kvad_push_frame(a->vad,vf);
        }

        while (length >= fsize) {
            if(a->vad_idx < 80 || (a->vad_idx >= 80 && a->t_vad >= a->a_vad)){
                if(a->cfg->use_mt){
                    len_sp = sp->pos / sizeof(short);
                    while(len_sp < fsize){
                        len_sp = sp->pos / sizeof(short);
                        if(len_sp >= fsize){
                            break;
                        }
                        while(1){
                            qn = wtk_queue_pop(&(a->mt_q));
                            if(!qn){
                                break;
                            }
                            val = data_offset(qn, qtk_ahs_mt_value_t, q_n);
                            if(a->nnrt){
                                nn_post_(a,val->a,val->b);
                            }else{
                                nn_post2_(a,val->x);
                            }
                            wtk_vpool2_push(a->pool, val);
                        }
                        usleep(100);
                    }
                }
                //wtk_debug("%d %d\n",a->wav_idx,a->km_idx);
                yt = (short *)mic->data;
                rt = (short *)sp->data;
                qtk_ahs_feed_short(a, yt, rt, fsize);
                wtk_strbuf_pop(mic, NULL, fsize * sizeof(short));
                wtk_strbuf_pop(sp, NULL, fsize * sizeof(short));
                if(a->dcache) {
                    wtk_strbuf_pop(a->dcache, NULL, fsize * sizeof(short));
                }
                length = mic->pos / sizeof(short);
            }else{
                break;
            }
        }
    }else{
        while (length >= fsize) {
            if(a->cfg->use_mt){
                len_sp = sp->pos / sizeof(short);
                while(len_sp < fsize){
                    len_sp = sp->pos / sizeof(short);
                    if(len_sp >= fsize){
                        break;
                    }
                    while(1){
                        qn = wtk_queue_pop(&(a->mt_q));
                        if(!qn){
                            break;
                        }
                        val = data_offset(qn, qtk_ahs_mt_value_t, q_n);
                        if(a->nnrt){
                            nn_post_(a,val->a,val->b);
                        }else{
                            nn_post2_(a,val->x);
                        }
                        wtk_vpool2_push(a->pool, val);
                    }
                    usleep(100);
                }
            }
            //wtk_debug("%d %d\n",a->wav_idx,a->km_idx);
            yt = (short *)mic->data;
            rt = (short *)sp->data;
            qtk_ahs_feed_short(a, yt, rt, fsize);
            wtk_strbuf_pop(mic, NULL, fsize * sizeof(short));
            wtk_strbuf_pop(sp, NULL, fsize * sizeof(short));
            if(a->dcache) {
                wtk_strbuf_pop(a->dcache, NULL, fsize * sizeof(short));
            }
            length = mic->pos / sizeof(short);
        }
    }

    if(is_end == 1 && a->cfg->use_mt){

        while(1){
            qn = wtk_queue_pop(&(a->mt_q));
            if(!qn){
                break;
            }
            val = data_offset(qn, qtk_ahs_mt_value_t, q_n);
            if(a->nnrt){
                nn_post_(a,val->a,val->b);
            }else{
                nn_post2_(a,val->x);
            }
            wtk_vpool2_push(a->pool, val);
        }

        qtk_ahs_evt_t *evt;
        evt = qtk_ahs_pop_evt(a);
        //evt->x = NULL;
        evt->type = QTK_AHS_EVT_END;
        wtk_blockqueue_push(&(a->nn_input_q),&(evt->q_n));
        wtk_sem_acquire(&(a->end_wait_sem),-1);
    }
}

void qtk_ahs_set_notify(qtk_ahs_t *a, void *upval, qtk_ahs_notify_f notify) {
    a->upval = upval;
    a->notify = notify;
}
