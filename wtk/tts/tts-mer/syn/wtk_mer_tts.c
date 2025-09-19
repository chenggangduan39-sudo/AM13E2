#include "wtk_mer_tts.h"
#define INF_FLOAT -1.0e+10

wtk_mer_tts_t* wtk_mer_tts_new(char *cfg_fn, int is_rbin, int seek_pos )
{
    int size=sizeof(short)*512*1024;
    int fs = 16000;
    wtk_mer_tts_t *tts = malloc(sizeof(wtk_mer_tts_t));
    tts->cfg = wtk_mer_cfg_new(cfg_fn, is_rbin, seek_pos);
    tts->parser = wtk_tts_parser_new(&(tts->cfg->parser), is_rbin?tts->cfg->bin_cfg->rbin:NULL);
    tts->syn_tts = wtk_syn_new(&(tts->cfg->syn_tts_cfg));
    tts->wav = wtk_mer_wav_stream_new(size, fs, 5.0);
    tts->wparam = wtk_mer_wav_param_new(1024, fs);
    // tts->tpool = wtk_thread_tpool_create(5);
    return tts;
}
void wtk_mer_tts_reset(wtk_mer_tts_t *tts)
{
    wtk_tts_parser_reset(tts->parser);
    wtk_syn_reset(tts->syn_tts);
}
void wtk_mer_tts_delete(wtk_mer_tts_t *tts)
{
    wtk_mer_wav_stream_delete(tts->wav);
    wtk_mer_cfg_delete(tts->cfg);
    wtk_tts_parser_delete(tts->parser);
    wtk_syn_delete(tts->syn_tts);
    wtk_mer_wav_param_delete(tts->wparam);
    // wtk_thread_tpool_destroy(tts->tpool);
    free(tts);
}

static void dur_decomposition(wtk_matf_t *dur_matf)
{/* 持续时间分解 */
    int i;
    float *p;

    p = dur_matf->p;
    for (i=0; i<dur_matf->row*dur_matf->col; ++i)
    {
        *p=(int)(*p + 0.5);
        *p= *p < 1 ? 1: *p;
        p++;
    }
}

static void dur_prediction( wtk_mer_cfg_syn_t* syn_cfg, wtk_strbuf_t ***lab_arrs, int lab_len)
{
    wtk_matf_t
        *lab_feature_matf,
        *norm_fea_matf,
        *dur_matf;
    wtk_mer_cfg_syn_dnn_t dur = syn_cfg->dur;
    int layer_size;
    
    lab_feature_matf = wtk_mer_state_align(&(syn_cfg->qes), "none", lab_arrs, lab_len);
    
    // wtk_mer_matf_write_file(lab_feature_matf, "output/lab_feature_mf.txt", 0);
    // exit(1);

    norm_fea_matf = wtk_mer_normalise_data(lab_feature_matf, dur.norm);
    // wtk_mer_matf_write_file(norm_fea_matf, "output/norm_fea_matf.txt", 0);

    layer_size = syn_cfg->layer_type->nslot;
    dur_matf = wtk_mer_dnn_model(norm_fea_matf, layer_size,
        (wtk_string_t**)syn_cfg->layer_type->slot,
        (int*)syn_cfg->layer_num->slot,
        (wtk_matf_t**)dur.w_arr->slot,
        (wtk_vecf_t**)dur.b_arr->slot,
        5, "duration");
    // wtk_mer_matf_write_file(dur_matf, "output/dur_cmp.txt", 0);
    // exit(1);

    wtk_mer_mean_variance_norm(dur_matf, dur.mvn);
    // wtk_mer_matf_write_file(dur_matf, "output/dur_norm_cmp.txt", 0);

    dur_decomposition(dur_matf);
    // wtk_mer_matf_write_file(dur_matf, "output/dur.txt", 0);
    // wtk_exit(1);

    wtk_mer_dur_to_lab( dur_matf, lab_arrs, lab_len);

    wtk_matf_delete( lab_feature_matf);
    wtk_matf_delete( norm_fea_matf);
    wtk_matf_delete( dur_matf);
}

static wtk_mer_bandmat_t** build_win_mats(wtk_heap_t *heap, float *(*wins)[3], int wins_len, int frame_num)
{
    wtk_mer_bandmat_t **wins_bm = wtk_heap_malloc(heap, sizeof(wtk_mer_bandmat_t*)*wins_len);
    int i
      , j;
    float *l
        , *u
        , *win_coff;
    for (i=0; i<wins_len; ++i)
    {
        l = wins[i][0];
        u = wins[i][1];
        wins_bm[i] = wtk_mer_bandmat_heap_new(heap, *l, *u, frame_num, 1);
        win_coff = wins[i][2];

        for (j=0; j<floor(*l + *u + 1); j++)
        {
            float *p = wtk_matf_row(wins_bm[i]->matf, j);
            wtk_float_set(p, win_coff[j], frame_num);
        }
    }
    return wins_bm;
}

static wtk_mer_bandmat_t* build_poe(wtk_vecf_t *b, wtk_matf_t *b_mf, wtk_matf_t *tau_mf, wtk_mer_bandmat_t **wins, int win_len)
{
    wtk_mer_bandmat_t *prec;
    int i
      , j
      , frame_num = b_mf->row
      , sdw = 0;
    float *tp = wtk_malloc(sizeof(float)*frame_num)
        , *dip = wtk_malloc(sizeof(float)*frame_num);
    wtk_vecf_t tmp_vf;
    tmp_vf.p = tp;
    tmp_vf.len = frame_num;
    wtk_vecf_t diag;
    diag.p = dip;
    diag.len = frame_num;
    for (i=0; i<win_len; i++)
    {   
        sdw = max( sdw, wins[i]->l + wins[i]->u );
    }
    prec = wtk_mer_bandmat_new(sdw, sdw, frame_num, 0);

    for (i=0; i<win_len; i++)
    {/*
        matf = wins[i][2]
        tmp_vf = b_mf[: 1]
      */
        for (j=0; j<frame_num; j++)
        {
            tmp_vf.p[j] = wtk_matf_row(b_mf, j)[i];
            diag.p[j] = wtk_matf_row(tau_mf, j)[i];
        }
        /* 这里python代码明确表达的是 bm.T, 当 l == u 时，仅改变is_transposed即可
        参考函数 wtk_mer_bandmat_T
         */
        wtk_mer_bandmat_dot_mv_plus_equals(wins[i], !wins[i]->is_transposed, &tmp_vf, b);
        wtk_mer_bandmat_dot_mm_plus_equals(wins[i], !wins[i]->is_transposed, wins[i], wins[i]->is_transposed, prec, prec->is_transposed, &diag);
    }

    wtk_free(tp);
    wtk_free(dip);
    return prec;
}

static void mlpg_algo_generation( wtk_matf_t *fea_mf, wtk_matf_t *cov_mf, int static_dim, wtk_matf_t *gen_mf)
{/* mlpg_fast
    cov => covariance
 */
    int frame_num = fea_mf->row
      , i
      , j
      , wins_len = 3;
    float win1[] = {0, 0}
      , win2[] = {1, 1}
      , win3[] = {1, 1};
    float w1[] = {1.0}
        , w2[] = {-0.5, 0.0, 0.5}
        , w3[] = {1.0, -2.0, 1.0};
    float *wins[3][3] = {
          { win1, win1+1, w1},
          { win2, win2+1, w2},
          { win3, win3+1, w3},
       };
    wtk_heap_t *heap = wtk_heap_new(4096);
    wtk_matf_t 
        *mu_mf = wtk_matf_heap_new( heap, frame_num, 3),
        *var_mf = wtk_matf_heap_new( heap, frame_num, 3),
        *b_mf = wtk_matf_heap_new( heap, frame_num, 3),
        *tau_mf = wtk_matf_heap_new( heap, frame_num, 3);
    wtk_vecf_t *b_vf;
    float *mp
        , *vp
        , *cp
        , *fp;
    wtk_mer_bandmat_t 
        *prec,
        **wins_bm;
    wtk_matf_zero(mu_mf);
    wtk_matf_zero(var_mf);

    wins_bm = build_win_mats(heap, wins, wins_len, frame_num);
    for (i=0; i<static_dim; ++i)
    {
        b_vf = wtk_vecf_heap_new( heap, frame_num);
        for (j=0; j<frame_num; j++)
        {
            mp = wtk_matf_row(mu_mf, j);
            vp = wtk_matf_row(var_mf, j);
            cp = wtk_matf_row(cov_mf, j);
            fp = wtk_matf_row(fea_mf, j);

            vp[0] = cp[i];
            vp[1] = cp[static_dim+i];
            vp[2] = cp[static_dim*2+i];

            mp[0] = fp[i];
            mp[1] = fp[static_dim+i];
            mp[2] = fp[static_dim*2+i];
        }
        float *p = wtk_matf_row(var_mf, frame_num-1);
        p[1] = p[2] = var_mf->p[1] = var_mf->p[2] = 1000000.0;

        for (j=0; j<mu_mf->row*mu_mf->col; ++j)
        {
            b_mf->p[j] = mu_mf->p[j] / var_mf->p[j];
            tau_mf->p[j] = 1.0 / var_mf->p[j];
        }
        prec = build_poe(b_vf, b_mf, tau_mf, wins_bm, 3);
        wtk_mer_bandmat_solveh(prec, b_vf);
        for (j=0; j<frame_num; j++)
        {
            gen_mf->p[ j*gen_mf->col + i ] = b_vf->p[j];
        }
        wtk_mer_bandmat_delete(prec);
    }
    // wtk_mer_matf_write_file(gen_mf, "dnn_data/gen.txt", 0);
    wtk_heap_delete(heap);
}

typedef enum
{
    enum_mgc,
    enum_vuv,
    enum_lf0,
    enum_bap
} fea_name_enum_t;

static int dim_switch_find(int enum_name)
{
    int r;
    switch (enum_name)
    {/* 180, 1, 3, 3 */
        case enum_mgc:
            r=180;
            break;
        case enum_vuv:
            r=1;
            break;
        case enum_lf0:
            r=3;
            break;
        case enum_bap:
            r=3;
            break;
        default:
            wtk_debug("找不到匹配值\n");
            wtk_exit(1);
            break;
    }
    return r;
}

static int stream_switch_find(int enum_name)
{
    int r;
    switch (enum_name)
    {/* 180, 1, 3, 3 */
        case enum_mgc:
            r=0;
            break;
        case enum_vuv:
            r=180;
            break;
        case enum_lf0:
            r=181;
            break;
        case enum_bap:
            r=184;
            break;
        default:
            wtk_debug("找不到匹配值\n");
            wtk_exit(1);
            break;
    }
    return r;
}

static wtk_vecf_t* covar_switch_find(wtk_mer_covar_t *covar, char *fea)
{
    wtk_vecf_t *v;
    if (strncmp(fea, "mgc", 3)==0) {
        v=covar->mgc;
    } else if (strncmp(fea, "bap", 3)==0) {
        v=covar->bap;
    } else if (strncmp(fea, "lf0", 3)==0) {
        v=covar->lf0;
    } else if (strncmp(fea, "vuv", 3)==0) {
        v=covar->vuv;
    } else {
        wtk_debug("未找到匹配值\n");
        wtk_exit(1);
    }
    return v;
}

static wtk_mer_wav_t* acoustic_decomposition(wtk_mer_covar_t *var_dict, wtk_matf_t *in_x, wtk_strbuf_t ***lab_arrs, int lab_len)
{/* 声音分解
确定参数顺序很重要
必须和训练时一致
*gen_wav_fea[] = { "mgc", "lf0", "bap"}
 */
    wtk_heap_t *heap = wtk_heap_new( 4096);
    wtk_heap_t *ret_heap = wtk_heap_new( 4096);
    wtk_mer_wav_t *ret_dict = wtk_mer_wav_new(ret_heap);
    wtk_matf_t 
        *var_matf,
        *var_matf2,
        *cur_matf,
        *vuv_matf,
        *gen_matf;
    wtk_vecf_t *var_vecf;
    wtk_strbuf_t **str_arr;
    char *fea_name
       , *gen_wav_fea[] = { "mgc", "lf0", "bap"}
       , *stream_start_keys = "mgc,vuv,lf0,bap";
    int gen_wav_fea_enum[] = {enum_mgc, enum_lf0, enum_bap};
    int i
      , j
      , gen_i
      , line_i
      , is_silence;

    int start_time
      , end_time
      , s_time=0
      , frame_num = in_x->row;

    for (gen_i=0; gen_i<3; ++gen_i)
    {
        int col
          , scol
          , dim_fea_val
          , fea_enum;
        fea_enum = gen_wav_fea_enum[gen_i];
        fea_name = gen_wav_fea[gen_i];
        dim_fea_val = dim_switch_find(fea_enum);
        scol = stream_switch_find(fea_enum);
        col = dim_fea_val;

        // wtk_debug("起止: %d %d\n", scol, col);
        cur_matf = wtk_matf_heap_new(heap, frame_num, col);
        wtk_matf_slice2(in_x, cur_matf, 0, frame_num, scol, col);
        
        var_vecf = covar_switch_find(var_dict, fea_name);
        var_matf2 = wtk_matf_heap_new(heap, var_vecf->len, frame_num);
        var_matf = wtk_matf_heap_new(heap, frame_num, var_vecf->len);
        
        float *p = var_matf2->p;
        for (j=0; j<var_vecf->len; ++j)
        {
            p = wtk_matf_row(var_matf2, j);
            wtk_float_set(p, var_vecf->p[j], frame_num);
        }
        wtk_matf_init_transpose(var_matf2, var_matf);
        gen_matf = wtk_matf_heap_new(ret_dict->heap, frame_num, dim_fea_val/3);
        mlpg_algo_generation( cur_matf, var_matf, dim_fea_val/3, gen_matf);

        if (strstr("lf0,F0", fea_name) != NULL)
        {
            if (strstr(stream_start_keys, "vuv") != NULL)
            {
                int vuv = stream_switch_find(fea_enum);
                vuv_matf = wtk_matf_heap_new(heap, in_x->row, 1);
                wtk_matf_slice2( in_x, vuv_matf, 0, in_x->row, vuv, 1);
                float *vp = vuv_matf->p;
                int col = vuv_matf->col
                  , n;
                for (i=0; i<frame_num; ++i)
                {
                   // { inf_float = -1.0e+10}
                    n = i*gen_matf->col;
                    if (vp[i*col] < 0.5 || gen_matf->p[n] < log(20))
                    {
                        gen_matf->p[n] = INF_FLOAT;
                    }
                }
            }
        }

        // 获取初始刻度值.主要考虑多线程刻度溢出
        str_arr = lab_arrs[0];
        s_time = str_arr[1]->pos;
        // printf("s_time: %d \n", s_time);

        for (line_i=0; line_i<lab_len; ++line_i)
        {
            str_arr = lab_arrs[line_i];

            // printf("%s \n", full_lab);
            is_silence = wtk_mer_check_sil(str_arr[3]->data);

            if (is_silence)
            {
                float *p;
                start_time = str_arr[1]->pos - s_time;
                end_time = str_arr[2]->pos - s_time;

                // printf("frame_num: %d end_time: %d start_time: %d s_time: %d \n", frame_num, end_time, start_time, s_time);
                if (end_time>frame_num)
                { wtk_debug("frame_num: %d 计算异常,小于 end_time: %d \n", frame_num, end_time);wtk_exit(1);}
                
                if ( strstr("lf0,F0,mag", fea_name) != NULL)
                {
                    for(i=start_time; i<end_time; i++)
                    {
                        p = wtk_matf_row(gen_matf, i);
                        wtk_float_set(p, INF_FLOAT, gen_matf->col);
                    }
                } else 
                {
                    for(i=start_time; i<end_time; i++)
                    {
                        p = wtk_matf_row(gen_matf, i);
                        wtk_float_set(p, 0.0, gen_matf->col);
                    }
                }
            }
        }
        wtk_mer_wav_add(ret_dict, fea_name, gen_matf);
    }
    wtk_heap_delete(heap);
    return ret_dict;
}

static wtk_mer_wav_t* slt_arctic( wtk_mer_cfg_syn_t *syn_cfg, wtk_strbuf_t ***lab_arrs, int lab_len)
{
    wtk_mer_cfg_syn_dnn_t act = syn_cfg->act;
    wtk_matf_t
        *lab_feature_matf,
        *norm_fea_matf,
        *act_matf;
    wtk_mer_wav_t *hash;
    int layer_size;

    lab_feature_matf = wtk_mer_state_align(&(syn_cfg->qes), "full", lab_arrs, lab_len);
    // wtk_mer_matf_write_file( lab_feature_matf, "output/matf.txt", 0);
    // wtk_exit(1);

    norm_fea_matf = wtk_mer_normalise_data(lab_feature_matf, act.norm);
    // wtk_mer_matf_write_file( norm_fea_matf, "output/mf2.txt", 0);

    layer_size = syn_cfg->layer_type->nslot;
    act_matf = wtk_mer_dnn_model(norm_fea_matf, layer_size,
        (wtk_string_t**)syn_cfg->layer_type->slot,
        (int*)syn_cfg->layer_num->slot,
        (wtk_matf_t**)act.w_arr->slot,
        (wtk_vecf_t**)act.b_arr->slot,
        187, "acoustic");
    wtk_mer_mean_variance_norm(act_matf, act.mvn);
    // wtk_mer_matf_write_file(act_matf, "output/act_mf.txt", 0);
    // wtk_exit(1);
    hash = acoustic_decomposition(&syn_cfg->covar, act_matf, lab_arrs, lab_len);
    
    wtk_matf_delete(lab_feature_matf);
    wtk_matf_delete(norm_fea_matf);
    wtk_matf_delete(act_matf);
    return hash;
}

void wtk_mer_process(char *wav_fn, wtk_strbuf_t ***lab_arrs, int lab_len, wtk_mer_tts_t *tts)
{
    struct  timeval start;
    struct  timeval finish;
    struct  timeval end;
    double duration;
    gettimeofday(&start,NULL);
    wtk_heap_t *heap = wtk_heap_new(4096);
    wtk_mer_wav_t *hash;
    size_t wav_size;

    /* 时长预测 */
    if (tts->cfg->syn.use_dur)
    { dur_prediction( &(tts->cfg->syn), lab_arrs, lab_len); }
    // wtk_exit(0);

    /* 声音分解*/
    hash = slt_arctic( &(tts->cfg->syn), lab_arrs, lab_len);

    // if (wtk_mer_thread_act_process(NULL, NULL/* 当前线程号 */,&hash)!=0) {return;}

    gettimeofday(&finish, NULL);
    
    /* 合成声音 */
    wav_size = wtk_mer_generate_wav( tts->wparam, hash, tts->wav);

    // delete hash
    wtk_mer_wav_delete(hash);

    if (wav_size)
    {
        gettimeofday(&end,NULL);
        duration = (1000000*(finish.tv_sec-start.tv_sec) + finish.tv_usec-start.tv_usec) / 1000000.0;
        wtk_debug( "\n dnn time: %lfs\n", duration );
        // f = wtk_mer_getfp("output/dnn_time2.txt", "a+");
        // fprintf(f, "%lf\n", duration);
        float ll=(tts->wav->len/(tts->wav->hdr.fmt_sample_rate*sizeof(short)*1.0));
        duration = (1000000*(end.tv_sec-start.tv_sec) + end.tv_usec-start.tv_usec) /1000000.0;
        wtk_mer_wav_stream_savefile2(tts->wav, wav_fn, duration);
        printf( " single time: %lfs, rate: %lf \n", duration, duration/(tts->wav->len/(tts->wav->hdr.fmt_sample_rate*sizeof(short)*1.0)));

        duration = (1000000*(end.tv_sec-finish.tv_sec) + end.tv_usec-finish.tv_usec) /1000000.0;
        printf( " wav synthesis time: %lfs, ll=%lfs rate: %lf \n", duration, ll, duration/ll);
    }
    wtk_heap_delete( heap);
}

void wtk_mer_process2(wtk_mer_process_t *pt)
{
    wtk_mer_process(pt->wav_fn, pt->lab_arrs, pt->lab_len, pt->tts);
}
void wtk_mer_process_thread(char *wav_fn, wtk_strbuf_t ***lab_arrs, int lab_len, wtk_mer_tts_t *tts)
{
    int t_n = min(wtk_mer_get_cpu_core_num(),4)
    // int t_n = 2
      , t_page, i, j, ret, n;
    wtk_heap_t *heap = wtk_heap_new(4096);
    pthread_t *tids = wtk_heap_malloc(heap, sizeof(pthread_t)*t_n);
    pthread_t *tid;
    wtk_mer_process_t *pt;
    // cpu_set_t mask;  //CPU核的集合
    // cpu_set_t get;   //获取在集合中的CPU
    t_n = min((int)ceil(lab_len/25.0), t_n);
    t_page = ((int)ceil(lab_len/5.0/t_n))*5;

    // if (t_page*(t_n-1) >= lab_len || t_page*t_n < lab_len)
    // {
    //     wtk_exit_debug("任务分配存在问题, t_page计算有误, t_page*(t_n-1) >= lab_len, lab_len: %d t_page: %d t_n: %d \n", lab_len, t_page, t_n);
    // }
    // wtk_thread_tpool_t *tpool = tts->tpool;

    for (i=0; i<t_n; i++)
    {
        n = i;
        tid = wtk_heap_malloc(heap, sizeof(pthread_t));
        // pt = wtk_heap_malloc( heap, sizeof(wtk_mer_process_t));
        pt = wtk_heap_malloc( heap, sizeof(wtk_mer_process_t));

        pt->wav_fn = wav_fn;
        pt->tts = tts;
        pt->lab_arrs = lab_arrs;
        pt->lab_len = min(t_page, lab_len-n*t_page);
        pt->i = i;
        tid = &tids[i];
        lab_arrs += t_page;

        if (pt->lab_len<=0)
        { /* 任务分配完了.减线程 */
            t_n--;break;
        }

        ret=pthread_create(tid, NULL, (void*)wtk_mer_process2, pt);
        if (ret!=0){wtk_debug("线程创建失败\n");wtk_exit(1);}
        wtk_mer_thread_wav_process(tid, &n, NULL, NULL, NULL);
        // wtk_mer_thread_act_process(&tid, &pt->i, NULL);
        // wtk_thread_tpool_add_task(tpool,(void*)wtk_mer_process2, pt);

        // CPU_ZERO(&mask);    //置空
        // CPU_SET(i, &mask);   //设置亲和力值
        // if (pthread_setaffinity_np(*tid,sizeof(mask),&mask) == -1)//设置线程CPU亲和力
        // {  
        //     printf("warning: could not set CPU affinity, continuing...\n");exit(1);
        // }
    }
    wtk_debug("thread 数量: %d , 核数: %d \n", t_n, wtk_mer_get_cpu_core_num());
    // while(!(tpool->work_head == NULL && tpool->n_work==0)){
    //     /* code */
    // }
    for (j=0; j<t_n; ++j)
    {
        pthread_join(tids[j], NULL);
    }
    
    // for (j=0; j<tpool->n_total; ++j)
    // {
    //     pthread_join(tpool->tids[j], NULL);
    // }
    wtk_heap_delete(heap);
}
