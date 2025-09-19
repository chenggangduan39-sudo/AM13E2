// #include "third/mkl/include/mkl_spblas.h"
#include "wtk_tac_wavrnn.h"

static struct timeval start, finish;// start_local, finish_local;
static double time_copy=0, time_gru=0, time_gru2=0, time_linear=0, time_upsample=0, time_matf_add=0;

#define def_calc_time() gettimeofday(&start, NULL);
#define def_calc_time_end(total) gettimeofday(&finish, NULL);total+=calc_time(finish, start);
#define def_calc_time_local() gettimeofday(&start_local, NULL);
#define def_calc_time_end_local(total) gettimeofday(&finish_local, NULL);total+=calc_time(finish_local, start_local);

static double calc_time(struct timeval finish, struct timeval start)
{
    return (1000000*(finish.tv_sec-start.tv_sec) + finish.tv_usec-start.tv_usec) /1000000.0;
}

void wtk_tac_wavrnn_pad( int pad, wtk_matf_t *mel, wtk_matf_t *dst)
{/* zero 填充
side == 'both' 未实现其它
 */
    int num_mel=mel->col
      , nrow=mel->row
      , t=nrow+2*pad;
    
    assert(dst->row == t);

    memcpy( dst->p+pad*num_mel, mel->p, sizeof(float)*num_mel*nrow);
}

void wtk_tac_wavrnn_res_block(wtk_matf_t **kernel, int kernel_size, wtk_vecf_t **gamma,  wtk_vecf_t **beta, wtk_vecf_t **mean, wtk_vecf_t **variance, int num_layers, wtk_matf_t *in, wtk_matf_t *out)
{/* 
层维度不变, 所以代码有简化
 */
    wtk_heap_t *heap=wtk_heap_new(1024);
    int i
      , j
      , is_after=0
      , num_mel=in->row
      , dim=gamma[0]->len;
    wtk_matf_t 
        *in_out=wtk_matf_heap_new( heap, num_mel, dim),
        *out_tmp=wtk_matf_heap_new( heap, out->row, out->col),
        *out_tmp2=wtk_matf_heap_new( heap, out->row, out->col),
        *tmp;

    for (i=0; i<num_layers; ++i)
    {
        j=i*2;
        wtk_tac_conv1d( kernel[j], kernel_size, NULL, gamma[j], beta[j], mean[j], variance[j], 1e-5, wtk_nn_relu, is_after, in, in_out);
        wtk_tac_conv1d( kernel[j+1], kernel_size, NULL, gamma[j+1], beta[j+1], mean[j+1], variance[j+1], 1e-5, NULL, is_after, in_out, out_tmp);
        wtk_matf_add2(out_tmp, in);
        in=out_tmp;
        tmp=out_tmp2;
        out_tmp2=out_tmp;
        out_tmp=tmp;
    }
    wtk_matf_cpy( out_tmp2, out);
    wtk_heap_delete( heap);
}

void wtk_tac_wavrnn_res_net( int num_res_block, int kernel_res_in_size, int kernel_res_out_size, wtk_matf_t *kernel_res_in, wtk_matf_t *kernel_res_out, wtk_vecf_t *bias_res_out, wtk_vecf_t *gamma_res,  wtk_vecf_t *beta_res, wtk_vecf_t *mean_res, wtk_vecf_t *var_res, wtk_matf_t **kernel_bk, int kernel_block_size, wtk_vecf_t **gamma_bk,  wtk_vecf_t **beta_bk, wtk_vecf_t **mean_bk, wtk_vecf_t **var_bk, wtk_matf_t *in, wtk_matf_t *out)
{/* 
conv1d
batch_norm
relu
layers (num_res_block层残差网络) x变为 batch_size*compute_dims*(mel_win_len-kernel_size+1)
conv1d
 */
    wtk_heap_t *heap=wtk_heap_new(1024);
    int is_after=0
      , ncol_in=var_res->len
      , num_mel=in->row;
    
    wtk_matf_t 
        *in_out=wtk_matf_heap_new( heap, num_mel, ncol_in),
        *block_out=wtk_matf_heap_new( heap, num_mel, ncol_in);

    wtk_tac_conv1d( kernel_res_in, kernel_res_in_size, NULL, gamma_res, beta_res, mean_res, var_res, 1e-5, wtk_nn_relu, is_after, in, in_out);

    wtk_tac_wavrnn_res_block( kernel_bk, kernel_block_size, gamma_bk, beta_bk, mean_bk, var_bk, num_res_block, in_out, block_out);

    wtk_nn_conv1d2( kernel_res_out, kernel_res_out_size, enum_conv_same, block_out, out);
    wtk_matf_vecf_add( out, bias_res_out);

    // wtk_mer_matf_write_file( block_out, "output/block_out.c.txt", 0);
    // wtk_mer_matf_write_file( out, "output/conv_out.c.txt", 0);
    // wtk_exit(0);
    wtk_heap_delete(heap);
}

void wtk_tac_wavrnn_stretch2d(wtk_matf_t *aux, int x_scale, wtk_matf_t *dst)
{/* 
Stretch2d的目的是mel残差网MelResNet的输出的粒度由帧提升到数据点（由于mel谱每一帧对应256个数据点，相当于将最后一个维度拓展256倍）
第二个功能：将输入的四维张量的最后一维拓展x_scale倍
 */
    int i, j
      , num_mel=aux->row
      , ncol=aux->col;
    float *dp=dst->p, *ap=aux->p;
    size_t col_stlen=sizeof(float)*ncol;

    for (i=0; i<num_mel; ++i, ap+=ncol)
    {
        for (j=0; j<x_scale; ++j, dp+=ncol)
        {
            memcpy(dp, ap, col_stlen);
        }
    }
}

void wtk_tac_nn_conv_copy( wtk_matf_t *src, wtk_matf_t *dst)
{
    // printf("%d %d ----> %d %d \n", src->row, src->col, dst->row, dst->col);
    wtk_matf_init_transpose(src, dst);
}

void wtk_tac_nn_conv2d( wtk_matf_t *kernel, int k_size, wtk_matf_t *in, wtk_matf_t *dst)
{/* 仅针对此次卷积运算, */
    wtk_heap_t *heap=wtk_heap_new(1024);
    int nr=in->row
      , nc=in->col
      , i
      , s=(k_size-1)/2
      , e=(k_size-1)/2
      , end=e;
    wtk_matf_t 
        *mf=wtk_matf_heap_new( heap, nr*nc, k_size),
        a_mf,
        b_mf;
    size_t k_stlen=sizeof(float)*(k_size)*nc;
    float 
        *cache=wtk_heap_zalloc( heap, k_stlen),
        *inp;

    wtk_matf_reshape( dst, -1, 1);
    a_mf.row=b_mf.col=k_size;
    a_mf.col=b_mf.row=nc;
    a_mf.p=cache;
    inp=in->p;
    for (i=0; i<nr; ++i)
    {
        if (s>0)
        {
            memcpy(cache+s*nc, in->p, sizeof(float)*(k_size-s)*nc);
            s--;
        } else if (i+k_size-end>nr&& e>0) {
            memset(cache, 0, k_stlen);
            memcpy(cache, inp, sizeof(float)*(nr-i)*nc);
            inp+=nc;
            e--;
        } else {
            // printf("++ \n");
            memcpy(cache, inp, k_stlen);
            inp+=nc;
        }
        b_mf.p=wtk_matf_at(mf, i*nc, 0);
        // printf("%d ar: %d ac: %d br: %d bc: %d %p %p %d\n", i, a_mf.row, a_mf.col, b_mf.row, b_mf.col, a_mf.p, b_mf.p, i*nc);
        wtk_tac_nn_conv_copy(&a_mf, &b_mf);
    }
    wtk_mer_blas_sgemm(mf, kernel, NULL, dst);
    wtk_matf_reshape( dst, -1, nc);
    wtk_heap_delete(heap);
}

void wtk_tac_wavrnn_upsample( wtk_tac_cfg_syn_wavrnn_t *cfg, int num_res_block, int kernel_res_in_size, int kernel_res_out_size, int kernel_block_size, int total_scale, int upsample_scales[], int scale_len, wtk_matf_t **up_layer_kernel, wtk_matf_t *mel, wtk_matf_t *stretch_aux, wtk_matf_t *mel_up)
{
    wtk_heap_t *heap=wtk_heap_new(1024);
    int i
      , num_mel=mel->col
      , pad=2
      , k_size;
    wtk_matf_t 
        *aux=wtk_matf_heap_new( heap, mel->row, cfg->bias_res_out->len),
        *mel_pad=wtk_matf_heap_new( heap, mel->row+pad*2, num_mel),
        *up_in,
        *up_sh,
        *up_out=NULL;

    wtk_tac_wavrnn_res_net( num_res_block, kernel_res_in_size, kernel_res_out_size, cfg->kernel_res_in, cfg->kernel_res_out, cfg->bias_res_out, cfg->gamma_res, cfg->beta_res,cfg->mean_res, cfg->var_res, cfg->kernel_bk, kernel_block_size, cfg->gamma_bk, cfg->beta_bk, cfg->mean_bk, cfg->var_bk, mel, aux);

    gettimeofday(&start, NULL);
    wtk_tac_wavrnn_stretch2d(aux, total_scale, stretch_aux);
    gettimeofday(&finish, NULL);
    time_copy+=calc_time(finish, start);
    

    wtk_tac_wavrnn_pad( pad, mel, mel_pad);
    up_in=mel_pad;

    for (i=0; i<scale_len; ++i)
    {
        k_size=upsample_scales[i]*2 + 1;
        up_sh=wtk_matf_heap_new( heap, up_in->row*upsample_scales[i], num_mel);
        up_out=wtk_matf_heap_new( heap, up_sh->row, num_mel);
        wtk_tac_wavrnn_stretch2d(up_in, upsample_scales[i], up_sh);
        wtk_tac_nn_conv2d( up_layer_kernel[i], k_size, up_sh, up_out);
        up_in=up_out;
        // wtk_mer_matf_write_file( up_sh, "output/up_sh.c.txt", 0);
        // wtk_mer_matf_write_file( up_layer_kernel[i], "output/up_layer_kernel.c.txt", 0);
        // wtk_mer_matf_write_file( up_out, "output/up_layer.0.c.txt", 0);
        // wtk_exit(1);
    }

    memcpy(mel_up->p, wtk_matf_at(up_out, pad*total_scale, 0), sizeof(float)*mel_up->row*mel_up->col);
    wtk_heap_delete(heap);
}

/* 
将输入的一条数据按照长度为target分装成若干条，overlap表示每条数据之间的重叠部分
这儿主要功能是：对最后一条数据末尾的补零。
fold: 折叠
python 结果 shape  [num_fold, target+2*overflap, num_features]
 */
wtk_matf_t* wtk_tac_fold_with_overlap( int target, int overlap, wtk_matf_t *x, int *padding)
{
    int num_fea=x->col
      , nr=x->row
      , num_fold=(nr-overlap)/(target+overlap)
    //   , extended_len=num_fold*(overlap+target)+overlap 
     , extended_len=num_fold*(overlap+target)
      , remaining=nr-extended_len // 需要补足的长度
      , nr_dst=target+2*overlap
      , nc_dst
      , offset_x
      , i
      , j
      , cur;
    wtk_matf_t *dst;
    float *dstp, *xp;
    size_t stlen=sizeof(float)*num_fea;

    if (remaining !=0)
    {
        if (padding!=NULL)
        { *padding= target + 2*overlap - remaining; }
        num_fold++;
    } else {
        if (padding!=NULL) { *padding=0; }
    }
    
    dst=wtk_matf_new( nr_dst, num_fold*num_fea);
    wtk_matf_zero(dst);
    nc_dst=dst->col;

    for (j=0; j<num_fold; ++j)
    {
        dstp=dst->p+j*num_fea;
        offset_x=j*(target+overlap);
        xp=wtk_matf_at(x, offset_x, 0);
        for (i=0, cur=offset_x; cur < nr && i<nr_dst; ++i, cur++, dstp+=nc_dst, xp+=num_fea)
        {
            memcpy(dstp, xp, stlen);
        }
    }
    // wtk_exit(1);
    return dst;
}

static float corr_calc(float *pattern, float *match, int len)
{
    int i;
    float 
        sum_a=0,
        sum_p=0,
        sum_m=0;
    for (i=0; i<len; ++i)
    {
        sum_a+=pattern[i]*match[i];
        sum_p+=pattern[i]*pattern[i];
        sum_m+=match[i]*match[i];
    }
    return sum_a/ (sqrtf(sum_p)*sqrtf(sum_m));
}

static float multiband_corr_calc(float *pattern, float *match, int len, int step, int subband)
{
    int i, j;
    float 
        sum_a=0,
        sum_p=0,
        sum_m=0;

    for (j=0; j<subband; ++j, pattern+=step, match+=step)
    {
        for (i=0; i<len; ++i)
        {
            sum_a+=pattern[i]*match[i];
            sum_p+=pattern[i]*pattern[i];
            sum_m+=match[i]*match[i];
        }
    }
    return sum_a/ (sqrtf(sum_p)*sqrtf(sum_m));
}

void wtk_tac_wavrnn_xfade_unfold(int overlap, wtk_matf_t *x, float *wav_data)
{
    int num_fold=x->row
      , nc=x->col
      , max_idx
      , overlap_len
      , i
      , j;
    float 
        *win=wtk_nn_hanning( 2*overlap, 1),
        *fade_in=win,
        *fade_out=win+overlap,
        *prev,
        *curr,
        *pattern,
        *match,
        corr,
        max_corr,
        // *start=wav_data,
        *wav_data_end=wav_data+nc-overlap;

    prev=x->p;
    memcpy(wav_data, prev, sizeof(float)*nc);
    // wtk_float_mult(wav_data_end, fade_out, wav_data_end, overlap);
    wav_data+=nc;
    
    for (i=1; i<num_fold; ++i)
    {
        prev=wtk_matf_at(x, i-1, 0);
        curr=wtk_matf_at(x, i, 0);
        max_idx=0;
        max_corr=0;
        pattern=prev+(nc-overlap);

        for (j=0; j<overlap; ++j)
        {
            match = curr+j;
            corr=corr_calc(pattern, match, overlap);
            if (corr > max_corr)
            {
                max_idx=j;
                max_corr = corr;
            }
        }
        wtk_float_mult(wav_data_end, fade_out, wav_data_end, overlap);
        wtk_float_mult(curr+max_idx, fade_in, curr+max_idx, overlap);
        overlap_len=max_idx+overlap;
        wtk_float_add( wav_data_end, curr+max_idx, overlap);
        memcpy(wav_data_end+overlap, curr+overlap_len, sizeof(float)*(nc-overlap_len));
        // wtk_debug("[%lu, %lu]\n", wav_data_end-start, wav_data+nc-overlap_len-start);
        // wtk_exit(1);
        wav_data_end+=nc-overlap_len; 
        wav_data+=nc-overlap_len;
        // wtk_debug("max_idx: %d \n", max_idx);
    }
    // wtk_debug("wav_data_len: %lu \n", (tt-start));
    free(win);
}

void wtk_tac_wavrnn_multiband_xfade_unfold(int overlap, int subband, int padding, wtk_matf_t *x, 
                                                                                                    float *wav_data, float **subband_data, int *wav_data_len_out/* 单个wav_data长度 */)
{
    int num_fold=x->row/subband
      , nc=x->col
      , max_idx
      , overlap_len
      , i
      , j;
    float 
        *win=calloc( sizeof(*win), 2*overlap),
        *fade_in=win,
        *fade_out=win+overlap,
        *prev,
        *curr,
        *pattern,
        *match,
        corr,
        max_corr,
        *start=subband_data[0],
        **wav_data_end_arr=wtk_malloc( sizeof(void*)*subband);

    wtk_nn_hanning_numpy( win, 2*overlap);

    prev=x->p;
    //将数据填充到subband_data 填充subband个
    for (i=0; i<subband; ++i, prev+=nc)
    {
        wav_data_end_arr[i]=subband_data[i]+nc-overlap;
        memcpy(subband_data[i], prev, sizeof(float)*nc);
    }
    
    //
    for (i=1; i<num_fold; ++i)
    {
        prev=wtk_matf_at(x, (i-1)*subband, 0);
        curr=wtk_matf_at(x, i*subband, 0);
        max_idx=0;
        max_corr=0;

        pattern=prev+(nc-overlap);
        //互功率普吗??
        for (j=0; j<overlap; ++j)
        {
            match = curr+j;
            corr=multiband_corr_calc(pattern, match, overlap, nc, subband);
            if (corr > max_corr)
            {
                max_idx=j;
                max_corr = corr;
            }
        }

        for (j=0; j<subband; ++j, curr+=nc)
        {
            //乘窗
            wtk_float_mult( wav_data_end_arr[j], fade_out, wav_data_end_arr[j], overlap);

            wtk_float_mult( curr+max_idx, fade_in, curr+max_idx, overlap);
            overlap_len=max_idx+overlap;
            wtk_float_add( wav_data_end_arr[j], curr+max_idx, overlap);
            memcpy(wav_data_end_arr[j]+overlap, curr+overlap_len, sizeof(float)*(nc-overlap_len));
            wav_data_end_arr[j]+=nc-overlap_len;
        }
    }
    int 
        subband_len=wav_data_end_arr[0]-start+overlap,
        wav_data_len=subband_len-padding;

    for (i=0; i<subband; ++i, wav_data+=wav_data_len)
    {
        memcpy( wav_data, subband_data[i], sizeof(float)*(wav_data_len));
    }
    *wav_data_len_out=wav_data_len;
    free(win);
    free(wav_data_end_arr);
}

long random_seed = 5.0;
long myrandom()
{/* 自定义线性同余,做python对比测试用 */
    random_seed = (random_seed * 31 + 13) % ((1 << 15) - 1);
    return random_seed;
}

int wtk_nn_random_weight(int n, float *weight)
{/* 
权重随机
 */
    // srand(time(NULL));
    int ret = 0;
    float 
        rnd = rand()/(RAND_MAX+1.0);
    int i;
    for (i=0; i<n; ++i)
    {
        if (rnd < weight[i])
        { return i;}
        rnd -= weight[i];
    }
    
    // int ret = 0,i = 0;
    // float tmp = 0.0f;
    // for(i = 0; i < n; ++i){
    //     if(weight[i]>tmp){
    //         tmp = weight[i];
    //         ret = i;
    //     }
    // }
    return ret;
}

void wtk_nn_rnngru_batch_wavrnn(wtk_nn_rnngru_t *cell, wtk_matf_t *gin, wtk_matf_t *gout, wtk_matf_t *cout, wtk_matf_t *cout2, int batch_size, wtk_matf_t *in, wtk_matf_t *out, wtk_matf_t *h_inmf, wtk_matf_t *h_outmf)
{/* 
处理 seq_len == 1, 批量矩阵运算
 */
    wtk_nn_enum_type_t type=cell->type;
    int num_units = cell->num_units
      , incol = in->col
      , i, j;
    size_t 
        unit_stlen=sizeof(float)*num_units,
        incol_stlen=sizeof(float)*incol;
    float 
        *fi=in->p,
        *fgi=gin->p,
        *fhi=h_inmf->p,
        *fr,
        *fu,
        *fco=cout->p,
        *fco2,
        *fnew_h=h_outmf->p,
        *fout=out->p;
    
    for (i=0; i<batch_size; ++i, fgi+=gin->col, fi+=in->col, fhi+=h_inmf->col)
    {
        memcpy(fgi, fi, incol_stlen);
        memcpy(fgi+incol, fhi, unit_stlen);
    }

    wtk_nn_layer_dense( gin, cell->gate_kernel, cell->gate_bias, wtk_nn_sigmoid, gout);
    switch (type)
    {
    case wtk_nn_enum_type_tensorflow:
        // wtk_float_mult( fr, fs, r_state->p, num_units);
        // wtk_nn_layer_dense( cin, cell->candidate_kernel, cell->candidate_bias, cell->activation, cout);
        wtk_exit_debug("wavrnn gru 特殊版未实现 tensorflow 部分\n");
        break;
    case wtk_nn_enum_type_pytorch:
        wtk_mer_blas_sgemm2( in, cell->candidate_kernel, cell->candidate_bias, cout);
        wtk_mer_blas_sgemm2( h_inmf, cell->candidate_kernel_hh, cell->candidate_bias_hh, cout2);
        
        fr=gout->p;
        fco=cout->p;
        fco2=cout2->p;
        for (i=0 ; i<batch_size; ++i, fr+=gout->col, fco2+=cout2->col)
        {
            wtk_float_mult(fco2, fr, fco2, num_units);
        }
        wtk_matf_add2(cout, cout2);
        cell->activation( fco, num_units*batch_size);
        break;
    default:
        wtk_exit_debug("rnn gru 未知算法类型 [%d] \n", type);
        break;
    }
    
    fu=gout->p + num_units;
    fhi=h_inmf->p;
    fco=cout->p;

    for (j=0; j<batch_size; ++j, fout+=num_units, fnew_h+=num_units, fu+=gout->col, fco+=num_units, fhi+=num_units)
    {
        for (i=0; i<num_units; ++i)
        {
            fnew_h[i] = fu[i] * fhi[i] + (1 - fu[i]) * fco[i];
        }
        memcpy(fout, fnew_h, unit_stlen);
    }
    // wtk_mer_matf_write_file(out, "output/cout.c.txt", 0);
    // wtk_exit(1);
    // memcpy(out->p, fnew_h, sizeof(float)*num_units);
}

void wtk_tac_wavrnn_generate_paralle_big( 
    wtk_matf_t *linear_kernel, wtk_vecf_t *linear_bias, wtk_nn_rnngru_t **gru_arr, wtk_matf_t **fc_kernel, wtk_vecf_t **fc_bias,
    wtk_matf_t *seq_in, wtk_matf_t *seq_out, wtk_matf_t *gru_gin, wtk_matf_t *gru_gin2, wtk_matf_t *gru_gout, wtk_matf_t *gru_cout, wtk_matf_t *gru_cout2, wtk_matf_t  *gru_out, wtk_matf_t *h1_mf, wtk_matf_t *gru2_in, wtk_matf_t *gru2_out, wtk_matf_t *h2_mf,

    wtk_matf_t *x_h1, wtk_matf_t *a3_in, wtk_matf_t *a3_out, wtk_matf_t *a4_in, wtk_matf_t *a4_out, wtk_matf_t *logits,

    int num_fold, int aux_dim, float *p_mel, int num_mel, float *p_x, float *p_aux, int num_aux, size_t aux_stlen, float *p_out, float *sample)
{
    int j, tmp_len, random_wi;
    float 
        *p_seq,
        *p_h1,
        *p_in,
        *p_tmp;

    p_seq=seq_in->p;
    for (j=0; j<num_fold; ++j, p_seq+=seq_in->col)
    {
        memcpy(p_seq, p_x+j, sizeof(float));
        memcpy(p_seq+1, p_mel+j*num_mel, sizeof(float)*num_mel);
        memcpy(p_seq+1+num_mel, p_aux+j*num_aux, sizeof(float)*aux_dim);
    }
    // wtk_mer_matf_write_file(seq_in, "output/seq.c.txt", 0);
   
    gettimeofday(&start, NULL);
    wtk_nn_layer_dense( seq_in, linear_kernel, linear_bias, NULL, seq_out);
    gettimeofday(&finish, NULL);
    time_linear+=calc_time(finish, start);
    // wtk_mer_matf_write_file(seq_out, "output/seq_out.c.txt", 0);
    // wtk_exit(1);
    // wtk_mer_matf_write_file(h1_mf, "output/h1_mf.c.txt", 0);
    
    gettimeofday(&start, NULL);
    // wtk_nn_rnngru_batch(gru, num_fold, seq_out, gru_out, h1_mf, h1_mf);
    wtk_nn_rnngru_batch_wavrnn(gru_arr[0], gru_gin, gru_gout, gru_cout, gru_cout2, num_fold, seq_out, gru_out, h1_mf, h1_mf);
    gettimeofday(&finish, NULL);
    time_gru+=calc_time(finish, start);
    // wtk_mer_matf_write_file(gru_out, "output/gru_out.c.txt", 0);
    // wtk_exit(1);

    gettimeofday(&start, NULL);
    p_h1=x_h1->p;
    p_in=gru2_in->p;
    p_tmp=gru_out->p;
    tmp_len=gru_out->col;
    for (j=0; j<num_fold; ++j, p_in+=gru2_in->col, p_tmp+=tmp_len, p_h1+=tmp_len)
    {
        memcpy(p_in, p_tmp, sizeof(float)*tmp_len);
        wtk_float_add(p_in, wtk_matf_at( seq_out, j, 0), seq_out->col);
        memcpy(p_h1, p_in, sizeof(float)*tmp_len);
        memcpy(p_in+tmp_len, p_aux+aux_dim+j*num_aux, aux_stlen);
    }
    // wtk_nn_rnngru_batch(gru2, num_fold, gru2_in, gru2_out, h2_mf, h2_mf);
    wtk_nn_rnngru_batch_wavrnn(gru_arr[1], gru_gin2, gru_gout, gru_cout, gru_cout2, num_fold, gru2_in, gru2_out, h2_mf, h2_mf);
    gettimeofday(&finish, NULL);
    time_gru2+=calc_time(finish, start);
    // wtk_mer_matf_write_file(gru2_in, "output/gru2_in.0.c.txt", 0);
    // wtk_mer_matf_write_file(gru2_out, "output/gru2_out.0.c.txt", 0);
    // wtk_exit(1);
    gettimeofday(&start, NULL);
    p_in=a3_in->p;
    p_tmp=x_h1->p;
    tmp_len=x_h1->col;
    for (j=0; j<num_fold; ++j, p_in+=a3_in->col, p_tmp+=tmp_len)
    {
        memcpy(p_in, p_tmp, sizeof(float)*tmp_len);
        wtk_float_add(p_in, wtk_matf_at( gru2_out, j, 0), gru2_out->col);
        memcpy(p_in+tmp_len, p_aux+aux_dim*2+j*num_aux, aux_stlen);
    }
    wtk_nn_layer_dense( a3_in, fc_kernel[0], fc_bias[0], wtk_nn_relu, a3_out);
    gettimeofday(&finish, NULL);
    time_linear+=calc_time(finish, start);

    gettimeofday(&start, NULL);
    p_in=a4_in->p;
    p_tmp=a3_out->p;
    tmp_len=a3_out->col;
    for (j=0; j<num_fold; ++j, p_in+=a4_in->col, p_tmp+=tmp_len)
    {
        memcpy(p_in, p_tmp, sizeof(float)*tmp_len);
        memcpy(p_in+tmp_len, p_aux+aux_dim*3+j*num_aux, aux_stlen);
    }
    wtk_nn_layer_dense( a4_in, fc_kernel[1], fc_bias[1], wtk_nn_relu, a4_out);
    wtk_nn_layer_dense( a4_out, fc_kernel[2], fc_bias[2], NULL, logits);
    gettimeofday(&finish, NULL);
    time_linear+=calc_time(finish, start);

    // wtk_mer_matf_write_file(logits, "output/logits.c.txt", 0);
    // wtk_exit(1);
    
    p_tmp=logits->p;
    tmp_len=logits->col;
    for (j=0; j<num_fold; ++j, p_tmp+=tmp_len)
    {
        wtk_nn_softmax(p_tmp, tmp_len);
        random_wi = wtk_nn_random_weight( tmp_len, p_tmp);
        //    random_wi=512;
        p_x[j]=p_out[j]=sample[random_wi];
        // printf("%d ", random_wi);
    }

    // printf("\n");
    // printf("%f %f %f ", p_x[0], p_x[1], p_x[2]);
    // printf("\n");
    // wtk_mer_matf_write_file(logits, "output/logits.c.txt", 0);
}

void wtk_tac_wavrnn_generate_paralle_small( 
    wtk_matf_t *linear_kernel, wtk_vecf_t *linear_bias, wtk_nn_rnngru_t **gru_arr, wtk_matf_t **fc_kernel, wtk_vecf_t **fc_bias,
    wtk_matf_t *seq_in, wtk_matf_t *seq_out, wtk_matf_t *gru_gin,  wtk_matf_t *gru_gout, wtk_matf_t *gru_cout, wtk_matf_t *gru_cout2, wtk_matf_t  *gru_out, wtk_matf_t *h1_mf,

    wtk_matf_t *a2_in, wtk_matf_t *a2_out, wtk_matf_t *logits, 

    int num_fold, int aux_dim, float *p_mel, int num_mel, float *p_x, float *p_aux, int num_aux, size_t aux_stlen, float *p_out, float *sample)
{
    int j, tmp_len, random_wi;
    float 
        *p_seq,
        *p_h1,
        *p_in,
        *p_tmp;
    
    p_seq=seq_in->p;
    for (j=0; j<num_fold; ++j, p_seq+=seq_in->col)
    {
        memcpy(p_seq, p_x+j, sizeof(float));
        memcpy(p_seq+1, p_mel+j*num_mel, sizeof(float)*num_mel);
        memcpy(p_seq+1+num_mel, p_aux+j*num_aux, sizeof(float)*(aux_dim-1));
    }
    // wtk_debug("num_aux: %d \n", num_aux);
    // wtk_mer_file_write_float( "output/aux.c.txt", 0, p_aux, num_fold*num_aux);
    // wtk_mer_matf_write_file(seq_in, "output/seq_in.c.txt", 0);
    // wtk_exit(0);

    gettimeofday(&start, NULL);
    wtk_nn_layer_dense( seq_in, linear_kernel, linear_bias, NULL, seq_out);
    gettimeofday(&finish, NULL);
    time_linear+=calc_time(finish, start);
    // wtk_mer_matf_write_file(seq_in, "output/seq_in.c.txt", 0);
    // wtk_mer_matf_write_file(seq_out, "output/seq_out.c.txt", 0);
    // wtk_exit(1);

    gettimeofday(&start, NULL);
    // wtk_nn_rnngru_batch(gru_arr[0], num_fold, seq_out, gru_out, h1_mf, h1_mf);
    wtk_nn_rnngru_batch_wavrnn(gru_arr[0], gru_gin, gru_gout, gru_cout, gru_cout2, num_fold, seq_out, gru_out, h1_mf, h1_mf);
    gettimeofday(&finish, NULL);
    time_gru+=calc_time(finish, start);
    // wtk_exit_debug("gru dur: %lf \n", calc_time(finish, start));
    // wtk_mer_matf_write_file(gru_out, "output/gru_out.c.txt", 0);
    // static global_i=-1;
    // global_i++;
    // if (global_i==5000) 
    // { 
    //     wtk_debug("global_i: %d \n", global_i);
    //     // wtk_mer_matf_write_file( h1_mf, "output/h1_mf.c.txt", 0); wtk_exit(0);
    //     wtk_mer_matf_write_file( gru_out, "output/gru_out.c.txt", 0); wtk_exit(0);
    // }
    // wtk_exit(1);

    p_in=a2_in->p;
    p_tmp=gru_out->p;
    tmp_len=gru_out->col;
    p_h1=h1_mf->p;
    for (j=0; j<num_fold; ++j, p_in+=a2_in->col, p_tmp+=tmp_len, p_h1+=tmp_len)
    {
        memcpy(p_in, p_h1, sizeof(float)*tmp_len);
        wtk_float_add(p_in, wtk_matf_at( seq_out, j, 0), seq_out->col);
        memcpy(p_in+tmp_len, p_aux+aux_dim+j*num_aux, aux_stlen);
    }
    gettimeofday(&start, NULL);
    wtk_nn_layer_dense( a2_in, fc_kernel[0], fc_bias[0], wtk_nn_relu, a2_out);
    gettimeofday(&finish, NULL);
    time_linear+=calc_time(finish, start);

    gettimeofday(&start, NULL);
    wtk_nn_layer_dense( a2_out, fc_kernel[1], fc_bias[1], NULL, logits);
    gettimeofday(&finish, NULL);
    time_linear+=calc_time(finish, start);

    // wtk_mer_matf_write_file(seq_in, "output/seq_in.c.txt", 0);
    // wtk_mer_matf_write_file(logits, "output/logits.c.txt", 0);
    
    p_tmp=logits->p;
    tmp_len=logits->col;
    for (j=0; j<num_fold; ++j, p_tmp+=tmp_len)
    {
        wtk_nn_softmax(p_tmp, tmp_len);
        random_wi = wtk_nn_random_weight( tmp_len, p_tmp);
        // random_wi = 120;
        // printf("%d ", random_wi);
        p_x[j]=p_out[j]=sample[random_wi];
    }
    // printf(" \n");
    // wtk_mer_matf_write_file(logits, "output/logits.c.txt", 0);
    // wtk_exit(0);
}

void wtk_tac_wavrnn_generate_paralle_multiband( 
    wtk_matf_t *linear_kernel, wtk_vecf_t *linear_bias, wtk_nn_rnngru_t **gru_arr, wtk_matf_t **fc_kernel, wtk_vecf_t **fc_bias, int fc_band_num, wtk_matf_t **fc_band_kernel, wtk_vecf_t **fc_band_bias, 
    wtk_matf_t *seq_in, wtk_matf_t *seq_out, wtk_matf_t *gru_gin,  wtk_matf_t *gru_gout, wtk_matf_t *gru_cout, wtk_matf_t *gru_cout2, wtk_matf_t  *gru_out, wtk_matf_t *h1_mf,

    wtk_matf_t *a2_in, wtk_matf_t *a2_out, wtk_matf_t **fc_band_out_arr, 

    int num_fold, int aux_dim, float *p_mel, int num_mel, float *p_x, float *p_aux, int num_aux, size_t aux_stlen, float *p_out, float *sample)
{
    int i,j, tmp_len, random_wi;
    float 
        *p_seq,
        *p_in,
        *p_tmp;
    
    p_seq=seq_in->p;
    for (j=0; j<num_fold; ++j, p_seq+=seq_in->col)
    {
        memcpy(p_seq, p_x+j*4, sizeof(float)*4);
        memcpy(p_seq+4, p_mel+j*num_mel, sizeof(float)*num_mel);
        memcpy(p_seq+4+num_mel, p_aux+j*num_aux, sizeof(float)*(aux_dim-1));
    }
    // wtk_debug("num_aux: %d aux_dim: %d\n", num_aux, aux_dim);
    // wtk_mer_file_write_float( "output/aux.c.txt", 0, p_aux, num_fold*num_aux);
    // wtk_mer_matf_write_file(seq_in, "output/seq.c.txt", 0);
    // wtk_exit(0);

    gettimeofday(&start, NULL);
    wtk_nn_layer_dense( seq_in, linear_kernel, linear_bias, NULL, seq_out);
    gettimeofday(&finish, NULL);
    time_linear+=calc_time(finish, start);
    // wtk_mer_matf_write_file(seq_in, "output/seq_in.c.txt", 0);
    // wtk_mer_matf_write_file(seq_out, "output/seq_out.c.txt", 0);
    // wtk_exit(1);

    gettimeofday(&start, NULL);
    wtk_nn_rnngru_batch_wavrnn(gru_arr[0], gru_gin, gru_gout, gru_cout, gru_cout2, num_fold, seq_out, gru_out, h1_mf, h1_mf);
    gettimeofday(&finish, NULL);
    time_gru+=calc_time(finish, start);

    p_in=a2_in->p;
    p_tmp=gru_out->p;
    tmp_len=gru_out->col;
    for (j=0; j<num_fold; ++j, p_in+=a2_in->col, p_tmp+=tmp_len)
    {
        memcpy(p_in, p_tmp, sizeof(float)*tmp_len);
        wtk_float_add(p_in, wtk_matf_at( seq_out, j, 0), seq_out->col);
        memcpy(p_in+tmp_len, p_aux+aux_dim+j*num_aux, aux_stlen);
    }
    gettimeofday(&start, NULL);
    wtk_nn_layer_dense( a2_in, fc_kernel[0], fc_bias[0], wtk_nn_relu, a2_out);
    gettimeofday(&finish, NULL);
    time_linear+=calc_time(finish, start);

    gettimeofday(&start, NULL);
    for (i=0; i<fc_band_num; ++i)
    {
        wtk_nn_layer_dense( a2_out, fc_band_kernel[i], fc_band_bias[i], NULL, fc_band_out_arr[i]);
    }
    gettimeofday(&finish, NULL);
    time_linear+=calc_time(finish, start);

    // wtk_debug("fc_band_num: %d \n", fc_band_num);
    for (j=0; j<num_fold; ++j)
    {
        for (i=0; i<fc_band_num; ++i)
        {
            p_tmp=wtk_matf_at( fc_band_out_arr[i], j, 0);
            tmp_len=fc_band_out_arr[i]->col;
            wtk_nn_softmax(p_tmp, tmp_len);
            random_wi = wtk_nn_random_weight( tmp_len, p_tmp);
            // random_wi = 120 + i;
            // random_wi = 128;
            p_x[j*fc_band_num+i]=p_out[j*fc_band_num+i]=sample[random_wi];
        }
    }
    // wtk_mer_matf_write_file( fc_band_out_arr[1], "output/fc_band.c.txt", 0);
    // wtk_exit(0);
}

void wtk_tac_wavrnn_generate_paralle( int is_modle_small, int is_model_multiband, int aux_dim, int gru_layers, int fc_size, wtk_matf_t *linear_kernel, wtk_vecf_t *linear_bias, wtk_nn_rnngru_t **gru_arr, wtk_matf_t **fc_kernel, wtk_vecf_t **fc_bias, int fc_band_num, wtk_matf_t **fc_band_kernel, wtk_vecf_t **fc_band_bias, int num_mel, wtk_matf_t *mel, wtk_matf_t *aux, wtk_matf_t *output_trans)
{
    wtk_heap_t *heap=wtk_heap_new(1024);
    int num_fold=mel->col/num_mel
      , num_aux=aux->col/num_fold
      , seq_len=mel->row
    //   , tmp_len
      , gru_dim=gru_arr[0]->num_units
    //   , random_wi
      , n_class=is_model_multiband? fc_band_bias[0]->len:fc_bias[fc_size-1]->len
      , i;
    //   , j;
    
    wtk_matf_t 
        *x=wtk_matf_heap_new( heap, num_fold, is_model_multiband?4:1),
        *seq_in=wtk_matf_heap_new( heap, num_fold, linear_kernel->col),
        *seq_out=wtk_matf_heap_new( heap, num_fold, linear_bias->len),
        *gru_out=wtk_matf_heap_new( heap, num_fold, gru_dim),
        *h1_mf=wtk_matf_heap_new( heap, num_fold, gru_dim),
        *h2_mf=wtk_matf_heap_new( heap, num_fold, gru_dim),
        *output=wtk_matf_heap_new( heap, output_trans->col, output_trans->row),
        // *gru2_in=NULL,
        // *gru2_out=NULL,
        // *gru_gin2=NULL,
        // *x_h1=NULL,
        *a2_in=NULL,
        *a2_out=NULL,
        // *a3_in=NULL,
        // *a3_out=NULL,
        // *a4_in=NULL,
        // *a4_out=NULL,
        *logits=NULL;
    if (is_modle_small)
    {
        a2_in=wtk_matf_heap_new( heap, num_fold, fc_kernel[0]->col);
        a2_out=wtk_matf_heap_new( heap, num_fold, fc_bias[0]->len);
        logits=wtk_matf_heap_new( heap, num_fold, fc_bias[fc_size-1]->len);
    }
    else {
        // gru2_in=wtk_matf_heap_new( heap, num_fold, gru_dim+aux_dim);
        // gru_gin2=wtk_matf_heap_new( heap, num_fold, gru2_in->col + gru_dim),
        // gru2_out=wtk_matf_heap_new( heap, num_fold, gru_dim),
        // x_h1=wtk_matf_heap_new( heap, num_fold, gru_out->col),
        // a3_in=wtk_matf_heap_new( heap, num_fold, gru_dim+aux_dim),
        // a3_out=wtk_matf_heap_new( heap, num_fold, fc_bias[0]->len);
        // a4_in=wtk_matf_heap_new( heap, num_fold, a3_out->col+aux_dim),
        // a4_out=wtk_matf_heap_new( heap, num_fold, fc_bias[1]->len),
        logits=wtk_matf_heap_new( heap, num_fold, fc_bias[2]->len);
    }
    
    wtk_matf_t
        *gru_gin=wtk_matf_heap_new( heap, num_fold, linear_bias->len + gru_dim),
        *gru_gout=wtk_matf_heap_new( heap, num_fold, gru_dim*2),
        *gru_cout=wtk_matf_heap_new( heap, num_fold, gru_dim),
        *gru_cout2=wtk_matf_heap_new( heap, num_fold, gru_dim),
        **fc_band_out_arr=wtk_heap_malloc( heap, sizeof(void*)*fc_band_num);
    size_t
        aux_stlen=sizeof(float)*aux_dim;
    float 
        *p_mel,
        *p_aux,
        // *p_h1,
        *p_x=x->p,
        // *p_in,
        // *p_tmp,
        *p_out,
        *sample=wtk_heap_malloc(heap, sizeof(*sample)*n_class);

    // gru_arr[0]->candidate_kernel_smatf=wtk_mer_matf_sparse_new( gru_arr[0]->candidate_kernel);
    // gru_arr[0]->candidate_kernel_hh_smatf=wtk_mer_matf_sparse_new( gru_arr[0]->candidate_kernel_hh);
    // gru_arr[0]->gate_kernel_smatf=wtk_mer_matf_sparse_new( gru_arr[0]->gate_kernel);

    for (i=0; i<fc_band_num; ++i)
    {
        fc_band_out_arr[i]=wtk_matf_heap_new( heap, num_fold, fc_band_bias[i]->len);
    }
    
    for (i=0; i<n_class; ++i)
    {
        float mu=n_class-1;
        float tmp=(i/mu)*2-1;
        if (is_model_multiband)
        {
            sample[i]= (tmp>0?1:x==0?0:-1) * (expf(fabs(tmp) * 5.54517744447) - 1 )/mu;
        } else 
        {   sample[i]=2.0*i/(n_class-1.0)-1.0; }
    }
    wtk_debug("num_fold: %d \n", num_fold);
    for (i=0; i<gru_layers; ++i)
    {
        wtk_nn_rnngru_reset(gru_arr[i]);
    }
    
    wtk_matf_zero(h1_mf);
    wtk_matf_zero(h2_mf);

    p_mel=mel->p;
    p_aux=aux->p;
    p_out=output->p;
    for (i=0; i<seq_len; ++i, p_mel+=mel->col, p_aux+=aux->col, p_out+=output->col)
    {
        // wtk_tac_wavrnn_generate_paralle_big( linear_kernel, linear_bias, gru_arr, fc_kernel, fc_bias, seq_in, seq_out, gru_gin, gru_gin2, gru_gout, gru_cout, gru_cout2, gru_out, h1_mf, gru2_in, gru2_out, h2_mf, x_h1, a3_in, a3_out, a4_in, a4_out, logits, num_fold, aux_dim, p_mel, num_mel, p_x, p_aux, num_aux, aux_stlen, p_out, sample);

        if (is_model_multiband) {
            wtk_tac_wavrnn_generate_paralle_multiband( linear_kernel, linear_bias, gru_arr, fc_kernel, fc_bias, fc_band_num, fc_band_kernel, fc_band_bias, seq_in, seq_out, gru_gin, gru_gout, gru_cout, gru_cout2, gru_out, h1_mf, a2_in, a2_out, fc_band_out_arr, num_fold, aux_dim, p_mel, num_mel, p_x, p_aux, num_aux, aux_stlen, p_out, sample);
        } else {
            wtk_tac_wavrnn_generate_paralle_small( linear_kernel, linear_bias, gru_arr, fc_kernel, fc_bias, seq_in, seq_out, gru_gin, gru_gout, gru_cout, gru_cout2, gru_out, h1_mf, a2_in, a2_out, logits, num_fold, aux_dim, p_mel, num_mel, p_x, p_aux, num_aux, aux_stlen, p_out, sample);
        }

        // if (i%300==0)
        // { wtk_debug("rate: %d/%d\n", i, seq_len);}
        // if (i==1)
        // {
        //     wtk_mer_matf_write_file( seq_in, "output/seq_in.c.txt", 0);
        //     wtk_mer_matf_write_file( logits, "output/logits.c.txt", 0);
        //     // wtk_mer_matf_write_file(fc_band_out_arr[0], "output/fc_band.c.txt", 0);
        //     wtk_exit(0);
        // }
    }
    wtk_matf_init_transpose(output, output_trans);
    wtk_heap_delete(heap);
}

void wtk_tac_wavrnn_pqmf( wtk_matf_t *spec_filter_real, wtk_matf_t *spec_filter_imag, 
                                    wtk_nn_stft_t *stft, wtk_matf_t *in, int subbnad, float *out, int *out_len)
{
    wtk_heap_t *heap=wtk_heap_new(1024);
    int i
      , j
      , nfft=stft->nfft
      , hop_len=stft->hop_len
      , col_norm=in->col*subbnad
      , y_len=col_norm+nfft
      , n_frame=((y_len-nfft)/hop_len + 1)
      , istft_out_len=(hop_len*(n_frame - 1) + nfft);
    wtk_matf_t
        *normed=wtk_matf_heap_new( heap, subbnad, col_norm); // 插值采样
    float 
        *pin=in->p,
        *pnorm=normed->p,
        *y_pad=wtk_heap_malloc( heap, sizeof(float)*y_len),
        *istft_out=wtk_heap_malloc( heap, sizeof(float)*istft_out_len),
        *win_hann_sumsquare=wtk_nn_istft_win_sumsquare_heap_new( heap, stft, n_frame);

    int nfreq=stft->nfreq;
    wtk_nn_complex_t 
        *cpx_stft_out=wtk_nn_complex_new( nfreq*n_frame),
        *cpx_istft_in=wtk_nn_complex_new( nfreq*n_frame),
        cpx_tmp1,
        cpx_tmp2,
        cpx_tmp3;

    for (i=0; i<in->row*in->col; ++i, pin++, pnorm+=4)
    {
        pnorm[0]=4*pin[0];
    }
    pnorm=normed->p;
    memset(out, 0, sizeof(float)*(istft_out_len-nfft));
    for (i=0; i<subbnad; ++i, pnorm+=col_norm)
    {
        wtk_nn_pad_float( wtk_nn_pad_type_reflect, pnorm, normed->col, y_pad, y_len, nfft/2);
        wtk_nn_stft( stft, y_pad, y_len, cpx_stft_out);

        cpx_tmp1.len=cpx_tmp2.len=cpx_tmp3.len=nfreq;
        cpx_tmp1.real=cpx_stft_out->real;
        cpx_tmp1.imag=cpx_stft_out->imag;
        cpx_tmp2.real=wtk_matf_at( spec_filter_real, i, 0);
        cpx_tmp2.imag=wtk_matf_at( spec_filter_imag, i, 0);
        cpx_tmp3.real=cpx_istft_in->real;
        cpx_tmp3.imag=cpx_istft_in->imag;
        for (j=0; j<n_frame; ++j)
        {
            cpx_tmp1.real[0]=0;
            cpx_tmp1.imag[0]=0;
            wtk_nn_complex_mul( &cpx_tmp1, &cpx_tmp2, &cpx_tmp3);
            cpx_tmp1.real+=nfreq;
            cpx_tmp1.imag+=nfreq;
            cpx_tmp3.real+=nfreq;
            cpx_tmp3.imag+=nfreq;
        }
        // wtk_matf_t mf;
        // mf.p=cpx_istft_in->real;
        // mf.row=n_frame;
        // mf.col=nfreq;
        // wtk_mer_matf_write_file( &mf, "output/cpx_in.c.txt", 0);
        wtk_nn_istft( stft, n_frame, win_hann_sumsquare, cpx_istft_in, istft_out);
        // wtk_mer_file_write_float("output/istft_out.c.txt", 0, istft_out+nfft/2, istft_out_len-nfft);
        // if (i==1){exit(0);}
        /* out值是累加的, 切记 out一定要置0 */
        wtk_float_add( out, istft_out+nfft/2, istft_out_len-nfft);
    }
    // if (out_len) { out_len[0]=istft_out_len-nfft + 10*hop_len;}
    if (out_len) { out_len[0]=istft_out_len-nfft;}
    wtk_nn_complex_delete(cpx_stft_out);
    wtk_nn_complex_delete(cpx_istft_in);
    wtk_heap_delete( heap);
}

void wtk_tac_wavrnn( wtk_tac_hparams_t *hp, wtk_tac_cfg_syn_wavrnn_t *cfg, wtk_matf_t *mel, wtk_mer_wav_stream_t *wav)
{/* 
从目前的并行加速方案来看，这个版本的并行只是将待合成的序列拆分成了若干个子块单独合成，
最后将这些子块进行平滑拼接，生成最终的完整 长音频。在音频的拼接点的位置依旧有不平滑的跳边现象。
这也是wavernn目前面临的最大窘境：后一时刻的音频需要前一时刻音频的输出，如果将音频序列进行简单粗暴的拆分，
就会造成拼接位置的不连续。
 */
    struct timeval start, finish;
    gettimeofday(&start, NULL);

    wtk_array_t *upsample_scale=cfg->upsample_scale;
    int num_mel=mel->col
      , nrow=mel->row
      , hop_size=hp->wavrnn_hop_size
      , wav_len=(nrow-1)*hop_size
      , total_scale=1
      , *upsample_scale_iarr=upsample_scale->slot
      , scale_len=upsample_scale->nslot
    //   , timesteps_each_batch=5000
      , timesteps_each_batch=hp->wavrnn_is_model_multiband?2750:5000
      , overlap=100
      , num_padding
      , i;
    for (i=0; i<scale_len; ++i)
    { total_scale*=upsample_scale_iarr[i]; }
    wtk_heap_t *heap=wtk_heap_new(4096);
    wtk_matf_t 
        *aux=wtk_matf_heap_new( heap, nrow*total_scale, hp->wavrnn_res_out_dim),
        *mel_up=wtk_matf_heap_new( heap, nrow*total_scale, num_mel),
        *mel_fold,
        *aux_fold,
        *output;
    float *wav_data;

    gettimeofday(&start, NULL);
    wtk_tac_wavrnn_upsample( cfg, hp->wavrnn_num_res_block, hp->wavrnn_res_in_kernel_size, hp->wavrnn_res_out_kernel_size, hp->wavrnn_kernel_block_size, total_scale, upsample_scale_iarr, scale_len, cfg->up_layer_kernel, mel, aux, mel_up);
    gettimeofday(&finish, NULL);
    time_upsample+=calc_time(finish, start);

    gettimeofday(&start, NULL);
    mel_fold=wtk_tac_fold_with_overlap( timesteps_each_batch, overlap, mel_up, &num_padding);
    aux_fold=wtk_tac_fold_with_overlap( timesteps_each_batch, overlap, aux, NULL);
    gettimeofday(&finish, NULL);
    time_copy+=calc_time(finish, start);

    if (hp->wavrnn_is_model_multiband)
    {
        output=wtk_matf_heap_new( heap, (mel_fold->col/num_mel)*4, timesteps_each_batch+overlap*2);
    } else {
        output=wtk_matf_heap_new( heap, mel_fold->col/num_mel, timesteps_each_batch+overlap*2);
    }

    wtk_tac_wavrnn_generate_paralle( hp->wavrnn_is_model_small, hp->wavrnn_is_model_multiband, hp->wavrnn_aux_dim, hp->wavrnn_gru_layers, hp->wavrnn_num_fc, cfg->linear_kernel, cfg->linear_bias, cfg->gru, cfg->fc_kernel, cfg->fc_bias, hp->wavrnn_fc_band_num, cfg->fc_band_kernel, cfg->fc_band_bias, num_mel, mel_fold, aux_fold, output);

    // wtk_mer_matf_write_file( output, "output/output.c.mfbin", 1);
    // wtk_mer_matf_write_file( output, "output/output.c.txt", 0);
    // wtk_mer_matf_read_file2( output, "output/output.c.mfbin", 1);
    // wtk_mer_matf_read_file2( output, "output/output.mfbin", 1);
    // wtk_exit(1);

    wav_data=wtk_heap_zalloc( heap, sizeof(float)*(output->row*output->col+10*hop_size));
    wtk_debug("wav_data_len: %d wav_len: %d\n", output->row*output->col, wav_len);
    if (hp->wavrnn_is_model_multiband)
    {
        float **subband_data;
        int pqmf_col;
        wtk_matf_t 
            output_unfold;
            // *output_unfold=wtk_matf_heap_new( heap, 4, output->row*output->col);
        output_unfold.p=wav_data;
        output_unfold.row=4;
        subband_data=wtk_heap_malloc( heap, sizeof(void*)*4);
        for (i=0; i<4; ++i)
        {
            subband_data[i]=wtk_heap_zalloc( heap, sizeof(float)*(output->row*output->col/4));
        }

        wtk_tac_wavrnn_multiband_xfade_unfold( overlap, 4, num_padding, output, output_unfold.p, subband_data, &pqmf_col);
        output_unfold.col=pqmf_col;
        // wtk_mer_matf_shape_print( &output_unfold);
        // wtk_mer_matf_write_file( output_unfold, "output/unfold.c.txt", 0);
        // wtk_mer_matf_write_file( output_unfold, "output/output_unfold.mfbin", 1);
        // wtk_exit(0);
        /* output_unfold->p 和 wav_data是同一块内存,注意初始化为0的问题 */
        wtk_tac_wavrnn_pqmf( cfg->pqmf_spec_filter_real, cfg->pqmf_spec_filter_imag, cfg->stft, &output_unfold, 4, wav_data, NULL);
        // wtk_mer_file_write_float( "output/wave_data.c.txt", 0, wav_data, 20736);
        // wtk_mer_file_write_float( "output/wave_data.c.mfbin", 1, wav_data, 20736);
        // wtk_exit(0);
    } else {
        wtk_tac_wavrnn_xfade_unfold( overlap, output, wav_data);
    }

    int fade_in_frame=5;
    float step = 1.0/(fade_in_frame*hop_size);
    // 对末尾的若干帧做fade_out操作
    // 对起始的若干帧做fadein
    memset(wav_data+wav_len, 0, sizeof(float)*10*hop_size);
    for (i=0; i<fade_in_frame*hop_size; ++i)
    {
        wav_data[i]*=(i*step);
        wav_data[wav_len-i-1]*=(i*step);
    }

    // wtk_debug("sample_rate: %d \n", wav->hdr.fmt_sample_rate);
    wav_len += 10*hop_size;
    wtk_mer_wav_stream_write_float(wav, wav_data, wav_len);
    free(mel_fold);
    free(aux_fold);
    wtk_heap_delete( heap);

    // wtk_debug("wav_len: %d \n", wav_len);

    gettimeofday(&finish, NULL);
    double time_wavrnn=calc_time(finish, start);
    printf( "wavrnn: %lf seconds\n", time_wavrnn);
    printf( "data copy: %lf seconds weight: %.2lf \n", time_copy, (time_copy/time_wavrnn)*100);
    printf( "gru: %lf seconds weight: %.2lf \n", time_gru, (time_gru/time_wavrnn)*100);
    printf( "gru2: %lf seconds weight: %.2lf \n", time_gru2, (time_gru2/time_wavrnn)*100);
    printf( "linear: %lf seconds weight: %.2lf \n", time_linear, (time_linear/time_wavrnn)*100);
    printf( "upsample: %lf seconds weight: %.2lf \n", time_upsample, (time_upsample/time_wavrnn)*100);
    printf( "matf_add: %lf seconds weight: %.2lf \n", time_matf_add, (time_matf_add/time_wavrnn)*100);
}