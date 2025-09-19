#include "wtk_tac_lpcnet.h"

#define LOG256 5.5451774444795623
#define NUM_SUBBAND 4 /* 一次出4个点 */
#define NB_BANDS 18
#define LPC_ORDER 8
// #define FRAME_SIZE 64
#define FRAME_SIZE 50
#define OVERLAP_SIZE 160
#define WINDOW_SIZE (FRAME_SIZE+OVERLAP_SIZE)
#define FREQ_SIZE (WINDOW_SIZE/2)
#define FRAME_SIZE_5MS (2)
#define OVERLAP_SIZE_5MS (2)
#define TRAINING_OFFSET_5MS (1)
#define WINDOW_SIZE_5MS (FRAME_SIZE_5MS + OVERLAP_SIZE_5MS)

void wtk_tac_lpcnet_generate_parallel2_stream(wtk_tac_cfg_syn_lpcnet_t *cfg,wtk_matf_t *feat_fold,wtk_matf_t *cfeat,int is_end);
void wtk_tac_lpcnet_generate_stream_reset(wtk_tac_cfg_syn_lpcnet_t *cfg);

#ifdef USE_DEBUG
static struct timeval start, finish, start_local, finish_local;
static double  time_sigmoid=0, time_tanh=0, time_gru=0, time_gru2=0, time_lpc=0,time_md=0, time_other=0;//, time_linear=0, time_upsample=0,time_copy=0,time_matf_add=0, time_tmp=0, time_dense=0;
#endif

static double calc_time(struct timeval finish, struct timeval start)
{
    return (1000000*(finish.tv_sec-start.tv_sec) + finish.tv_usec-start.tv_usec) /1000000.0;
}
#ifdef USE_DEBUG
#define def_calc_time() gettimeofday(&start, NULL);
#define def_calc_time_end(total) gettimeofday(&finish, NULL);total+=calc_time(finish, start);
#define def_calc_time_local() gettimeofday(&start_local, NULL);
#define def_calc_time_end_local(total) gettimeofday(&finish_local, NULL);total+=calc_time(finish_local, start_local);
#else
#define def_calc_time() 
#define def_calc_time_end(total) 
#define def_calc_time_local() 
#define def_calc_time_end_local(total) 
#endif

static inline float log2_approx(float x)
{
   int integer;
   float frac;
   union {
      float f;
      int i;
   } in;
   in.f = x;
   integer = (in.i>>23)-127;
   in.i -= integer<<23;
   frac = in.f - 1.5f;
   frac = -0.41445418f + frac*(0.95909232f
          + frac*(-0.33951290f + frac*0.16541097f));
   return 1+integer+frac;
}

#define log_approx(x) (0.69315f*log2_approx(x))

// static int lpcnet_init=0;
// static float dct_table[NB_BANDS*NB_BANDS];
// static const float compensation[] = {
//     0.8f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 0.666667f, 0.5f, 0.5f, 0.5f, 0.333333f, 0.25f, 0.25f, 0.2f, 0.166667f, 0.173913f
// };
// static const short int eband5ms[] = {
// /*0  200 400 600 800  1k 1.2 1.4 1.6  2k 2.4 2.8 3.2  4k 4.8 5.6 6.8  8k*/
//   0,  1,  2,  3,  4,  5,  6,  7,  8, 10, 12, 14, 16, 20, 24, 28, 34, 40
// };

void wtk_tac_lpcnet_embed_pitch_load( wtk_matf_t *embed_pitch, wtk_matf_t *period, wtk_matf_t *pitch)
{
    int col=embed_pitch->col
      , len=pitch->row
      , i;
    float 
        *p=period->p,
        *pout=pitch->p;
    size_t stlen=sizeof(float)*col;

    for (i=0; i<len; ++i, pout+=col)
    {
        memcpy(pout, wtk_matf_at(embed_pitch, (int)p[i], 0), stlen);
    }
}

wtk_matf_t* wtk_tac_lpcnet_encoder(wtk_tac_cfg_syn_lpcnet_t *cfg,wtk_matf_t *feat)
{
    wtk_heap_t *heap = wtk_heap_new(1024);
    int kernel_size = cfg->lpcnet_feat_conv_kernel_size;
    wtk_matf_t 
        *cat_feat=feat,
        *feat_out1=wtk_matf_heap_new( heap, feat->row, cfg->feat_conv_bias[0]->len),
        *feat_out2=wtk_matf_heap_new( heap, feat_out1->row, cfg->feat_conv_bias[1]->len),
        *fd1=wtk_matf_heap_new( heap, feat_out2->row, cfg->feat_dense_bias[0]->len),
        *fd2=wtk_matf_heap_new( heap, fd1->row,  cfg->feat_dense_bias[1]->len),
        *cfeat;

    wtk_nn_conv1d2(cfg->feat_conv_kernel[0], kernel_size, enum_conv_same, cat_feat, feat_out1);
    wtk_matf_vecf_add(feat_out1,cfg->feat_conv_bias[0]);
    wtk_nn_tanh(feat_out1->p, feat_out1->row*feat_out1->col);

    wtk_nn_conv1d2( cfg->feat_conv_kernel[1], kernel_size, enum_conv_same, feat_out1, feat_out2);
    wtk_matf_vecf_add(feat_out2, cfg->feat_conv_bias[1]);
    wtk_nn_tanh(feat_out2->p, feat_out2->row*feat_out2->col);

    wtk_matf_add2( feat_out2, cat_feat);
    wtk_nn_layer_dense( feat_out2, cfg->feat_dense_kernel[0],cfg->feat_dense_bias[0], wtk_nn_tanh, fd1);
    wtk_nn_layer_dense( fd1, cfg->feat_dense_kernel[1], cfg->feat_dense_bias[1], wtk_nn_tanh, fd2);

    cfeat = wtk_matf_new(fd2->row,fd2->col);
    memcpy(cfeat->p,fd2->p,fd2->row*fd2->col*sizeof(float));
    wtk_heap_delete(heap);

    return cfeat;
}

/* 
求取降采样之后的频谱
spec为原始频谱,[nfft,nframe]
 */
static void downsample_spectrum( float *spec, int channel, float *spec_downsampled)
{
    int i,j,
        nfft=1024,
        nqst=nfft/8,
        start=nqst*channel;
    float 
        *spec_component = spec + start;
    memset(spec_downsampled, 0, sizeof(float)*2*nqst);
    
    if (channel == 0 || channel == 2)
    {
        memcpy( spec_downsampled+1, spec_component, sizeof(float)*nqst);
        for (i=0, j=2*nqst-1; i<nqst-1; ++i, --j)
        {
            spec_downsampled[j]=spec_component[i];
        }
        // wtk_mer_file_write_float( "output/spec_downsampled.c.txt", 0, spec_downsampled, 2*nqst);
        // wtk_mer_file_write_float( "output/spec_component.c.txt", 0, spec_component, nqst);
        // wtk_exit(0);
    } else 
    {/* 1,3 */
        memcpy( spec_downsampled+nqst, spec_component, sizeof(float)*nqst);
        for (i=1, j=nqst-1; i<nqst; ++i, --j)
        {
            spec_downsampled[j]=spec_component[i];
        }
    }
    for (i=0; i<2*nqst; ++i)
    {
        spec_downsampled[i]=fabsf(spec_downsampled[i]);
    }
    wtk_float_set_bound( 1E-10, 1E10, spec_downsampled, 2*nqst);
    // wtk_mer_file_write_float( "output/spec_downsampled.c.txt", 0, spec_downsampled, 2*nqst);
}

static void downsample_spectrum2( double *spec, int channel, double *spec_downsampled)
{
    int i,j,
        nfft=1024,
        nqst=nfft/8,
        start=nqst*channel;
    double 
        *spec_component = spec + start;
    memset(spec_downsampled, 0, sizeof(double)*2*nqst);
    
    if (channel == 0 || channel == 2)
    {
        memcpy( spec_downsampled+1, spec_component, sizeof(double)*nqst);
        for (i=0, j=2*nqst-1; i<nqst-1; ++i, --j)
        {
            spec_downsampled[j]=spec_component[i];
        }
    } else {/* 1,3 */
        memcpy( spec_downsampled+nqst, spec_component, sizeof(double)*nqst);
        for (i=1, j=nqst-1; i<nqst; ++i, --j)
        {
            spec_downsampled[j]=spec_component[i];
        }
    }
    for (i=0; i<2*nqst; ++i)
    {
        spec_downsampled[i]=fabs(spec_downsampled[i]);
    }
    wtk_double_set_bound( 1E-10, 1E10, spec_downsampled, 2*nqst);
}

/* 
利用自相关函数计算LPC
输出量 g 为第 p 阶线性预测模型增益（1*p）；ap 为从第 1 阶到 p 阶线性预测系数（p*p）；
输出量 ep 为第 1 到第 p 阶线性预测最小均方误差（1*p）；输出量 r0 是进行 L-D 算法的该数据的能量（数值）
r 为自相关系数，p 点为线性预测的阶数
 */
void levinson_durbin( float *r, int p, float *out)
{
    int i, j;
    float
        k[LPC_ORDER+1]={0.0};
    float
        e[LPC_ORDER+1]={0.0f},
        a[LPC_ORDER+1][LPC_ORDER+1]={{0.0f,},},
        tmp=0.0f;

    assert(LPC_ORDER == p);
    e[0]=r[0];
    for (i=1; i<p+1; ++i)
    {
        tmp=0.0f;
        for (j=1; j<i; ++j)
        {
            tmp+= a[i-1][j]*r[i-j];
        }
        k[i]=(r[i] - tmp)/(e[i-1]);
        a[i][i]=k[i];
        if (i>1)
        {
            for (j=1; j<i; ++j)
            {
                a[i][j] = a[i-1][j] - k[i] * a[i-1][i-j];
            }
        }
        e[i] = (1.0f-k[i]*k[i])*e[i-1];
    }
    for (i=0; i<LPC_ORDER; ++i)
    {
        out[i]=-a[p][i+1];
    }
}

void levinson_durbin_double( double *r, int p, double *out)
{
    int i, j;
    double
        k[LPC_ORDER+1]={0.0};
    double
        e[LPC_ORDER+1]={0.0},
        a[LPC_ORDER+1][LPC_ORDER+1]={{0.0,},},
        tmp=0.0;

    assert(LPC_ORDER == p);
    e[0]=r[0];
    for (i=1; i<p+1; ++i)
    {
        tmp=0.0;
        for (j=1; j<i; ++j)
        {
            tmp+= a[i-1][j]*r[i-j];
        }
        k[i]=(r[i] - tmp)/e[i-1];
        a[i][i]=k[i];
        if (i>1)
        {
            for (j=1; j<i; ++j)
            {
                a[i][j] = a[i-1][j] - k[i] * a[i-1][i-j];
            }
        }
        e[i] = (1.0-k[i]*k[i])*e[i-1];
    }
    for (i=0; i<LPC_ORDER; ++i)
    {
        out[i]=-a[p][i+1];
    }
}


static double lag_window[LPC_ORDER+1]={
    1.0, 0.9998766375547586, 0.9995066415212829, 0.9988902856937028, 0.9980280260203829, 0.9969205000418225, 
    0.9955685261050763, 0.9939731023560482, 0.9921354055113971
};

/* 梅尔谱提取lpc特征
lpc.shape=[num_fold,NUM_SUBBAND,LPC_ORDER]
 */
static void lpcnet_multiband_mel_to_lpc( wtk_matf_t *spec_filter_real, wtk_matf_t *spec_filter_imag, wtk_matf_t *mel_inv_basis, float *pmel, int num_mel, float *lpc)
{
    int 
        nfft=1024,
        nqst=nfft/8,
        nfreq=mel_inv_basis->row,
        i, j;
    wtk_heap_t *heap=wtk_heap_new(1024);
    wtk_matf_t
        mel,
        *linear_spec=wtk_matf_heap_new( heap, mel_inv_basis->row, 1);
    wtk_nn_complex_t
        cpx_tmp1,
        cpx_tmp2,
        cpx_tmp3;
    wtk_rfft_t *rf=wtk_rfft_new( nqst);
    float
        *plinear_spec_imag=wtk_heap_zalloc( heap, sizeof(float)*nfreq),
        *pspec_real=wtk_heap_zalloc( heap, sizeof(float)*nfreq),
        *pspec_imag=wtk_heap_zalloc( heap, sizeof(float)*nfreq),
        *pspec_abs=wtk_heap_zalloc( heap, sizeof(float)*nfreq),
        *fft_cache=wtk_heap_zalloc( heap, sizeof(float)*2*nqst*2),
        *spec_downsampled=wtk_heap_zalloc( heap, sizeof(float)*2*nqst*2),
        *fft_y=spec_downsampled + nqst*2,
        *ac=wtk_heap_zalloc( heap, sizeof(float)*(LPC_ORDER+1));

    for (i=0; i<num_mel; ++i)
    {
        pmel[i]= (pmel[i]+4.0f) *(120.0f)/(8.0f) - 120.0f;
        pmel[i]= pow(10.0f, (pmel[i]+20.0f)*0.05f);
    }

    mel.p=pmel;
    mel.row=1;
    mel.col=num_mel;
    wtk_mer_blas_sgemm( mel_inv_basis, &mel, NULL, linear_spec);
    wtk_nn_relu( linear_spec->p, linear_spec->row);

    for (i=0; i<NUM_SUBBAND; ++i, lpc+=LPC_ORDER)
    {
        memset(plinear_spec_imag, 0, sizeof(float)*nfreq);
        cpx_tmp1.len=cpx_tmp2.len=cpx_tmp3.len=nfreq;
        cpx_tmp1.real=linear_spec->p;
        cpx_tmp1.imag=plinear_spec_imag;
        cpx_tmp2.real=wtk_matf_at( spec_filter_real, i, 0);
        cpx_tmp2.imag=wtk_matf_at( spec_filter_imag, i, 0);
        cpx_tmp3.real=pspec_real;
        cpx_tmp3.imag=pspec_imag;
        wtk_nn_complex_mul( &cpx_tmp1, &cpx_tmp2, &cpx_tmp3);
        wtk_nn_complex_abs( &cpx_tmp3, pspec_abs);
        downsample_spectrum( pspec_abs, i, spec_downsampled);
        memset( fft_y, 0, sizeof(float)*nqst*2);
        for (j=0; j<nqst*2; ++j)
        {
            spec_downsampled[j]=spec_downsampled[j]*spec_downsampled[j];
        }
        wtk_nn_rfft_ifft( rf, fft_cache, spec_downsampled, fft_y);
        
        for (j=0; j<LPC_ORDER+1; ++j)
        {
            ac[j]=spec_downsampled[j]*lag_window[j];
        }
        levinson_durbin( ac, LPC_ORDER, lpc);
    }
    wtk_heap_delete( heap);
    wtk_rfft_delete(rf);
}

#include "wtk/_inv_mel_basis.h"
void lpcnet_multiband_mel_to_lpc2( wtk_tac_cfg_syn_lpcnet_t *cfg, float *pmel, int num_mel, double *lpc)
{
    int 
        nfft=1024,
        nqst=nfft/8,
        nfreq=cfg->stft->nfreq,//cfg->mel_inv_basis->row,
        i, j;
    wtk_matf_t *spec_filter_real = cfg->pqmf_analysis_filter_real;
    wtk_matf_t *spec_filter_imag = cfg->pqmf_analysis_filter_imag;
    // wtk_matf_t *mel_inv_basis;
    wtk_heap_t *heap=wtk_heap_new(1024);
    wtk_matf_t mel;
    wtk_nn_dcomplex_t  cpx_tmp1,cpx_tmp3;
    wtk_nn_complex_t  cpx_tmp2;
    wtk_rfft_t *rf=wtk_rfft_new( nqst);
    float
        *fft_cache=wtk_heap_zalloc( heap, sizeof(float)*2*nqst*2),
        *fft_y=wtk_heap_zalloc( heap, sizeof(float)*2*nqst),
        *spec_downsampled_tmp = wtk_heap_zalloc( heap, sizeof(float)*2*nqst);

    wtk_matdf_t *linear_spec=wtk_matdf_heap_new( heap, nfreq, 1);
    wtk_matdf_t inv_bales;

    double *plinear_spec_imag=wtk_heap_zalloc( heap, sizeof(double)*nfreq);
    double *pspec_real=wtk_heap_zalloc( heap, sizeof(double)*nfreq);
    double *pspec_imag=wtk_heap_zalloc( heap, sizeof(double)*nfreq);
    double *pspec_abs=wtk_heap_zalloc( heap, sizeof(double)*nfreq);
    double *spec_downsampled=wtk_heap_zalloc( heap, sizeof(double)*2*nqst);
    double *ac=wtk_heap_zalloc( heap, sizeof(double)*(LPC_ORDER+1));

    for (i=0; i<num_mel; ++i)
    {
        pmel[i]= (pmel[i]+4.0f) *(120.0f)/(2*4.f) - 120.0f;
        pmel[i]= powf(10.0f, (pmel[i]+20.0f)*0.05f);
    }

    mel.p=pmel;
    mel.row=1;
    mel.col=num_mel;

    inv_bales.p = _inv_mel_basis;
    inv_bales.row = nfreq;//cfg->mel_inv_basis->row;
    inv_bales.col = cfg->feat_dim;//cfg->mel_inv_basis->col;

    wtk_mer_blas_sgemm3( &inv_bales, &mel, NULL, linear_spec);
    wtk_nn_relu_d( linear_spec->p, linear_spec->row);


    for (i=0; i<cfg->num_subband; ++i, lpc+=LPC_ORDER)
    {
        memset(plinear_spec_imag, 0, sizeof(double)*nfreq);
        cpx_tmp1.len=cpx_tmp2.len=cpx_tmp3.len=nfreq;
        cpx_tmp1.real=linear_spec->p;
        cpx_tmp1.imag=plinear_spec_imag;
        cpx_tmp2.real=wtk_matf_at( spec_filter_real, i, 0);
        cpx_tmp2.imag=wtk_matf_at( spec_filter_imag, i, 0);
        cpx_tmp3.real=pspec_real;
        cpx_tmp3.imag=pspec_imag;
        wtk_nn_complex_mul2( &cpx_tmp1, &cpx_tmp2, &cpx_tmp3);
        wtk_nn_dcomplex_abs( &cpx_tmp3, pspec_abs);
        downsample_spectrum2( pspec_abs, i, spec_downsampled);
        
        memset( fft_y, 0, sizeof(float)*nqst*2);
        for (j=0; j<nqst*2; ++j)
        {
            spec_downsampled_tmp[j]=spec_downsampled[j]*spec_downsampled[j];
        }
        wtk_nn_rfft_ifft( rf, fft_cache, spec_downsampled_tmp, fft_y);
        for (j=0; j<LPC_ORDER+1; ++j)
        {
            ac[j]=spec_downsampled_tmp[j]*lag_window[j];
        }
        levinson_durbin_double( ac, LPC_ORDER, lpc);
    }
    wtk_heap_delete( heap);
    wtk_rfft_delete(rf);
}

static inline short lin2ulaw(double x)
{
    double u = 0.0;
    double scale = (255.0/32768.0);
    int s = 0;

    if(x > 1e-10){
        s = 1;
    }else if(x < -1e-10){
        s = -1;
    }else{
        s = 0;
    }
    x = fabs(x);
    u = (s*((255.0/2.0)*log(1+scale*x)/LOG256));
    // u = (s*((255.0f/2.0f)*log_approx(1+scale*x)/log(256.0)));
    u = 128.0 + floor(u+.5);
    if (u < 0.f) u = 0.;
    if (u > 255.) u = 255.;
    return (short)u;
}

static inline float ulaw2lin(int u)
{
    int s;
    double scale_1 = 32768./255.;
    double t = u-128;
    double tmp = 0.0;

    if(t > 1e-10f){
        s = 1;
    }else if(t < -1e-10f){
        s = -1;
    }else{
        s = 0;
    }
   // tmp = fabs(u/128.0 -1.);
    // tmp = s*(exp(fabs(u/128.0 -1.)*log1p(255))-1.0)*scale_1;
    tmp = s*(exp(fabs(u/128.0 -1.)*log1p(255))-1.0)*scale_1;
    return tmp;
}

// typedef struct
// {/* 
// 用于 gru batch 计算时缓存结果
//  */
//     wtk_matf_t *ih_out_mf;
//     wtk_matf_t *hh_out_mf;
// } wtk_nn_rnngru_batch_cache_t;

wtk_nn_rnngru_batch_cache_t *wtk_nn_rnngru_batch_cache_heap_new( wtk_heap_t *heap, int batch_size, int num_units)
{
    wtk_nn_rnngru_batch_cache_t *cache=wtk_heap_malloc( heap, sizeof(*cache));
    cache->ih_out_mf=wtk_matf_heap_new( heap, batch_size, 3*num_units);
    cache->hh_out_mf=wtk_matf_heap_new( heap, batch_size, 3*num_units);
    return cache;
}
//稀疏矩阵gru运算
void wtk_nn_lpcnet_gru_compute( wtk_nn_rnngru_t *gru, wtk_nn_rnngru_batch_cache_t *cache, wtk_matf_t *hin_mf, wtk_matf_t *hout_mf, wtk_matf_t *in, wtk_matf_t *out)
{
    wtk_nn_enum_type_t type=gru->type;
    wtk_matf_t 
        *ih_out_mf=cache->ih_out_mf,
        *hh_out_mf=cache->hh_out_mf;
    int batch_size=ih_out_mf->row
      , dim=gru->num_units
      , dim2=2*dim
      , dim3=ih_out_mf->col
    //   , incol=in->col
      , i
      , j;
    float 
        *pih=ih_out_mf->p,
        *phh=hh_out_mf->p,
        *phin=hin_mf->p,
        *phout=hout_mf->p,
        *pout=out->p,
        *r,
        *u,
        *c;

    // wtk_matf_t in2, *in3=&in2;
    switch (type)
    {
    case wtk_nn_enum_type_tensorflow:
        wtk_exit_debug("未实现的分支\n");
        break;
    case wtk_nn_enum_type_pytorch:
        // def_calc_time_local();
        wtk_nn_matf_sparse_sgemm( gru->weight_hh_smf, hin_mf, hh_out_mf);
        wtk_matf_vecf_add( hh_out_mf, gru->bias_hh);
        // def_calc_time_end_local(time_dense);
        // wtk_debug("time_dense: %lf \n", time_dense);
        // wtk_exit(0);
        break;
    default:
        wtk_exit_debug("未实现的分支\n");
        break;
    }

    for (i=0; i<batch_size; ++i, pih+=dim3, phh+=dim3, phin+=dim, phout+=dim, pout+=dim)
    {
        r=pih;
        u=pih+dim;
        c=pih+dim2;
        wtk_float_add(pih, phh, dim2);
        def_calc_time_local();
        wtk_nn_sigmoid(pih, dim2);
        def_calc_time_end_local( time_sigmoid);
        // wtk_exit_debug("sigmoid: %lf \n", time_sigmoid);
        wtk_float_mult_add( r, phh+dim2, c, dim);
        def_calc_time_local();
        gru->activation( c, dim);
        def_calc_time_end_local( time_tanh);
        for (j=0; j<dim; ++j)
        {
            // printf("%f ", phin[j]);
            phout[j] = (1-u[j])*c[j] + u[j]*phin[j];
        }
        memcpy(pout, phout, sizeof(float)*dim);
    }
}

void wtk_nn_rnngru_batch_lpcnet(wtk_nn_rnngru_t *cell, wtk_matf_t *gin, wtk_matf_t *gout, 
    wtk_matf_t *cout, wtk_matf_t *cout2, int batch_size, wtk_matf_t *in, wtk_matf_t *out, 
    wtk_matf_t *h_inmf, wtk_matf_t *h_outmf)
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
    // wtk_nn_layer_dense( gin, cell->gate_kernel, cell->gate_bias, wtk_nn_sigmoid, gout);
    wtk_mer_unblas_sgemm_notrans( gin, cell->gate_kernel, cell->gate_bias, gout);
    wtk_nn_sigmoid(gout->p, gout->row*gout->col);
    switch (type)
    {
    case wtk_nn_enum_type_tensorflow:
        // wtk_float_mult( fr, fs, r_state->p, num_units);
        // wtk_nn_layer_dense( cin, cell->candidate_kernel, cell->candidate_bias, cell->activation, cout);
        wtk_exit_debug("wavrnn gru 特殊版未实现 tensorflow 部分\n");
        break;
    case wtk_nn_enum_type_pytorch:
        // wtk_mer_blas_sgemm2( in, cell->candidate_kernel, cell->candidate_bias, cout);
        // wtk_mer_blas_sgemm2( h_inmf, cell->candidate_kernel_hh, cell->candidate_bias_hh, cout2);

        wtk_mer_unblas_sgemm_notrans( in, cell->candidate_kernel, cell->candidate_bias, cout);
        wtk_mer_unblas_sgemm_notrans( h_inmf, cell->candidate_kernel_hh, cell->candidate_bias_hh, cout2);
        
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

void wtk_tac_lpcnet_decoder( 
    wtk_nn_rnngru_t **gru_arr, wtk_nn_rnngru_batch_cache_t *gru_cache, wtk_matf_t *gru_a_ih_out, wtk_matf_t **gru_gin, 
    wtk_matf_t **gru_gout, wtk_matf_t **gru_cout, wtk_matf_t **gru_cout_hh, wtk_matf_t **gru_in, wtk_matf_t  **gru_out, wtk_matf_t **h_mf,
    int num_md, wtk_matf_t ***md_kernel, wtk_vecf_t ***md_bias, wtk_vecf_t ***md_factor, wtk_matf_t ***md_out,
    wtk_matf_t **embed_sig_arr, wtk_matf_t **embed_exc_arr, wtk_matf_t *fexc, wtk_mati_t *iexc, wtk_matf_t *cfeat)
{
    int batch_size=fexc->row
      , fcol=fexc->col
      , sig_size=embed_sig_arr[0]->col
      , i, j;
    wtk_matf_t *gin=gru_in[0];
    float 
        *pfexc=fexc->p,
        *pin,
        *pih,
        *pout;

    int *piexc=iexc->p;

    wtk_matf_cpy( gru_a_ih_out, gru_cache->ih_out_mf);
    pih=gru_cache->ih_out_mf->p;
    for (i=0; i<batch_size; ++i, pih+=sig_size)
    {
        for (j=0; j<NUM_SUBBAND; ++j)
        {
            wtk_float_add( pih, wtk_matf_at(embed_sig_arr[j], (int)pfexc[i*fcol+j], 0), sig_size);
            wtk_float_add( pih, wtk_matf_at(embed_sig_arr[NUM_SUBBAND+j], (int)pfexc[i*fcol+NUM_SUBBAND+j], 0), sig_size);
            wtk_float_add( pih, wtk_matf_at(embed_exc_arr[j], (int)piexc[i*iexc->col + j], 0), sig_size);
        }
    }


    def_calc_time();
    wtk_nn_lpcnet_gru_compute( gru_arr[0], gru_cache, h_mf[0], h_mf[0], gin, gru_out[0]);
    def_calc_time_end(time_gru);

    pin=gru_in[1]->p;
    pout=gru_out[0]->p;
    for (i=0; i<batch_size; ++i, pin+=gru_in[1]->col, pout+=gru_out[0]->col)
    {
        memcpy(pin, pout, gru_out[0]->col*sizeof(float));
        memcpy(pin+gru_out[0]->col, wtk_matf_at(cfeat, i, 0), cfeat->col*sizeof(float));
    }

    def_calc_time();
    wtk_nn_rnngru_batch_lpcnet( gru_arr[1], gru_gin[1], gru_gout[1], gru_cout[1], gru_cout_hh[1], batch_size, gru_in[1], gru_out[1], h_mf[1], h_mf[1]);
    def_calc_time_end(time_gru2);

    wtk_heap_t *heap=wtk_heap_new( 1024);
    wtk_matf_t  *gru_out4=wtk_matf_heap_new(  heap,  batch_size, gru_out[1]->col/4);
    def_calc_time();
    for (i=0; i< NUM_SUBBAND; ++i)
    {
        wtk_matf_slice2(  gru_out[1],  gru_out4, 0, batch_size,  i*4, 4);
        for (j=0; j<num_md; ++j)
        {
            // wtk_nn_layer_dense( gru_out4, md_kernel[i][j], md_bias[i][j], wtk_nn_tanh, md_out[i][j]);
            // wtk_mer_unblas_sgemm_notrans( gru_out[1], md_kernel[i][j], md_bias[i][j], md_out[i][j]);
            wtk_mer_unblas_sgemm_notrans(  gru_out4, md_kernel[i][j],  md_bias[i][j],  md_out[i][j]);
            wtk_nn_tanh( md_out[i][j]->p, md_out[i][j]->row*md_out[i][j]->col);
            wtk_matf_vecf_multi( md_out[i][j], md_factor[i][j]);
        }
        wtk_matf_add2(md_out[i][0], md_out[i][1]);
        // wtk_mer_matf_write_file( md_out[j][0], "output/md1.c.txt", 0);
    }

    // p_tmp=md_out[0]->p;
    // for (i=0; i<batch_size; ++i, p_tmp+=md_out[0]->col)
    // {
    //     wtk_nn_softmax(p_tmp, md_out[0]->col);
    // }
    def_calc_time_end(time_md);
    wtk_heap_delete( heap);
}

/* 对输出分步进行平滑 */
static void smooth_multinomial_distribution(int len, float *p)
{
    int n;
    float sum_tmp=0;

    // sum_tmp=wtk_float_sum( p, len);
    // sum_tmp=1.f/(1E-18+sum_tmp);
    double *pd = wtk_malloc(sizeof(double)*len);
    for (n=0;n <len; ++n)
    {
        // p[n] *=sum_tmp;
        pd[n] = max(p[n]-0.002, 0.0);
    }
    sum_tmp=1e-8+wtk_double_sum( pd, len);
    // sum_tmp=1.f/(1E-8+sum_tmp);
    for (n=0;n <len; ++n)
    {
        // p[n] *=sum_tmp;
        p[n] =pd[n]/sum_tmp;
    }

    wtk_free(pd);
    // sum_tmp=wtk_float_sum( p, len);
    // sum_tmp=1.f/(1E-18+sum_tmp);
    // for (n=0;n <len; ++n)
    // {
    //     p[n] *=sum_tmp;
    //     p[n] = max(p[n]-0.002, 0);
    // }
    // sum_tmp=wtk_float_sum( p, len);
    // sum_tmp=1.f/(1E-8+sum_tmp);
    // for (n=0;n <len; ++n)
    // {
    //     p[n] *=sum_tmp;
    // }
}

/* 反预加重，一个低通滤波 */
static void de_emphasis( float *p, int len)
{
    float factor=0.85f, mem=0.0f;
    int i;
    for (i=0; i<len; ++i)
    {
        mem=factor*mem + p[i];
        p[i] = mem;
    }
}

static void de_emphasis_stream( float *p, int len, float *mem_p)
{
    float factor=0.85f, mem=*mem_p;
    int i;
    for (i=0; i<len; ++i)
    {
        mem=factor*mem + p[i];
        p[i] = mem;
    }
    *mem_p = mem;
}

void wtk_tac_lpcnet_generate_parallel( wtk_mer_wav_stream_t *wav, wtk_matf_t **embed_sig_arr, wtk_matf_t **embed_exc_arr, wtk_nn_rnngru_t **gru_arr, wtk_matf_t ***md_kernel, wtk_vecf_t ***md_bias, wtk_vecf_t ***md_factor, wtk_matf_t *mel_inv_basis, wtk_matf_t *analysis_filter_real, wtk_matf_t *analysis_filter_imag, wtk_matf_t *syn_filter_real, wtk_matf_t *syn_filter_imag, wtk_nn_stft_t *stft, wtk_matf_t *feat_fold, int num_extend, wtk_matf_t *cfeat)
{
    struct timeval global_start, global_finish;
    gettimeofday(&global_start, NULL);
    int fr
      , i
      , j
      , k
      , n
      , pmd_out_len
      , pitch_idx=36
      , global_step
      , skip=LPC_ORDER+1
      , num_md=2
      , num_fea=80
      , num_fold=feat_fold->col/num_fea
      , num_frame=feat_fold->row
      , pred_len=num_fold*NUM_SUBBAND;
    wtk_heap_t *heap=wtk_heap_new( 1024);
    wtk_matf_t
        *lpc=wtk_matf_heap_new( heap, num_fold, LPC_ORDER*NUM_SUBBAND),
        *pcm=wtk_matf_heap_new( heap, num_fold, FRAME_SIZE*num_frame*NUM_SUBBAND),
        *fexc=wtk_matf_heap_new( heap, num_fold, 2*NUM_SUBBAND),
        // *output=wtk_matf_heap_new( heap, num_fold, FRAME_SIZE*num_frame*NUM_SUBBAND),
        *output=wtk_matf_heap_new( heap, num_fold*NUM_SUBBAND, FRAME_SIZE*num_frame),
        cfeat_frame;

    wtk_mati_t  *iexc=wtk_mati_heap_new( heap, num_fold, 1*NUM_SUBBAND); //ssy
    
    float 
        *pfeat=feat_fold->p,
        *pcfeat,
        *pred=wtk_heap_malloc( heap, sizeof(float)*pred_len),
        *plpc,
        *ppcm,
        *pfexc,
        *pmd_out,
        // sum_tmp,
        *pin,
        *pout;

    int *piexc;

    int gru_dim=gru_arr[0]->num_units
      , gru_dim2=gru_arr[1]->num_units;
    wtk_matf_t
        *gru_a_ih_out=wtk_matf_heap_new( heap, num_fold, gru_dim*3),
        **h_mf=wtk_heap_malloc( heap, sizeof(void*)*2),
        **gru_in=wtk_heap_malloc( heap, sizeof(void*)*2),
        **gru_gin=wtk_heap_malloc( heap, sizeof(void*)*2),
        **gru_gout=wtk_heap_malloc( heap, sizeof(void*)*2),
        **gru_cout=wtk_heap_malloc( heap, sizeof(void*)*2),
        **gru_cout_hh=wtk_heap_malloc( heap, sizeof(void*)*2),
        **gru_out=wtk_heap_malloc( heap, sizeof(void*)*2),
        ***md_out=wtk_heap_malloc( heap, sizeof(void**)*NUM_SUBBAND);
    wtk_nn_rnngru_batch_cache_t *gru_cache=wtk_nn_rnngru_batch_cache_heap_new( heap, num_fold, gru_dim);
    gru_arr[0]->weight_hh_smf=wtk_nn_matf_sparse_heap_new( heap, gru_arr[0]->weight_hh);
    // wtk_mer_matf_write_file(gru_arr[0]->weight_hh, "output/gru_a_ih.c.txt", 0);

    h_mf[0]=wtk_matf_heap_new( heap, num_fold, gru_dim);
    h_mf[1]=wtk_matf_heap_new( heap, num_fold, gru_dim2);
    gru_in[0]=wtk_matf_heap_new( heap, num_fold, 128+128+64);
    gru_in[1]=wtk_matf_heap_new( heap, num_fold, gru_dim + 128);
    gru_gin[0]=wtk_matf_heap_new( heap, num_fold, 512 + gru_dim);
    gru_gin[1]=wtk_matf_heap_new( heap, num_fold, gru_in[1]->col + gru_dim2);
    gru_gout[0]=wtk_matf_heap_new( heap, num_fold, gru_dim*2);
    gru_gout[1]=wtk_matf_heap_new( heap, num_fold, gru_dim2*2);
    gru_cout[0]=wtk_matf_heap_new( heap, num_fold, gru_dim);
    gru_cout[1]=wtk_matf_heap_new( heap, num_fold, gru_dim2);
    gru_cout_hh[0]=wtk_matf_heap_new( heap, num_fold, gru_dim);
    gru_cout_hh[1]=wtk_matf_heap_new( heap, num_fold, gru_dim2);
    gru_out[0]=wtk_matf_heap_new( heap, num_fold, gru_dim);
    gru_out[1]=wtk_matf_heap_new( heap, num_fold, gru_dim2);

    assert( num_md == 2);

    for (j=0; j<NUM_SUBBAND; ++j)
    {
        md_out[j]=wtk_heap_malloc( heap, sizeof(void*)*num_md);
        for (i=0; i<num_md; ++i)
        {
            wtk_nn_rnngru_reset(gru_arr[i]);
            md_out[j][i]=wtk_matf_heap_new( heap, num_fold, 256);
            wtk_matf_zero(h_mf[i]);
        }
    }

    cfeat_frame.row=num_fold;
    cfeat_frame.col=cfeat->col/num_fold;

    pfexc=fexc->p;
    piexc=iexc->p;
    for (i=0; i<num_fold; ++i, pfexc+=fexc->col, piexc+=iexc->col)
    {
        pfexc[0] = 128;
        pfexc[1] = 128;
        pfexc[2] = 128;
        pfexc[3] = 128;
        piexc[0] = 128;
        piexc[1] = 128;
        piexc[2] = 128;
        piexc[3] = 128;
    }

    wtk_matf_t
        lpc_trans,
        lpc_tmp;
    lpc_tmp.p=wtk_heap_malloc( heap, sizeof(float)*LPC_ORDER*NUM_SUBBAND);
    lpc_trans.row=LPC_ORDER;
    lpc_trans.col=NUM_SUBBAND;
    lpc_tmp.row=NUM_SUBBAND;
    lpc_tmp.col=LPC_ORDER;

    for (fr=0; fr<num_frame; ++fr)
    {
        plpc=lpc->p;
        for (i=0; i<num_fold; ++i, plpc+=lpc->col, pfeat+=num_fea)
        {
            def_calc_time();
            lpcnet_multiband_mel_to_lpc( analysis_filter_real, analysis_filter_imag, mel_inv_basis, pfeat, num_fea, plpc);
            memcpy( lpc_tmp.p, plpc, sizeof(float)*NUM_SUBBAND*LPC_ORDER);
            lpc_trans.p=plpc;
            wtk_matf_init_transpose( &lpc_tmp, &lpc_trans);
            def_calc_time_end(time_lpc);
        }

        wtk_matf_zero(gru_in[0]);
        pin=gru_in[0]->p;
        pcfeat=wtk_matf_at(cfeat, fr, 0);
        for (i=0; i<num_fold; ++i, pin+=gru_in[0]->col, pcfeat+=cfeat_frame.col)
        {
            memcpy( pin+128, pcfeat, cfeat_frame.col*sizeof(float));
        }
        wtk_mer_unblas_sgemm_notrans( gru_in[0], gru_arr[0]->weight_ih, gru_arr[0]->bias_ih, gru_a_ih_out);

        for (i=skip; i<FRAME_SIZE; ++i)
        {
            global_step=fr*FRAME_SIZE + i;
            plpc=wtk_matf_at( lpc, 0, 0);
            ppcm=wtk_matf_at( pcm, 0, (global_step-1)*NUM_SUBBAND);
            memset(pred, 0, sizeof(float)*pred_len);

            for (n=0, pfexc=fexc->p; n<num_fold; ++n, ppcm+=pcm->col, plpc+=lpc->col, pfexc+=fexc->col)
            {
                int m;
                for (m=0, j=0; m<NUM_SUBBAND; ++m, j-=LPC_ORDER)
                {
                    for (k=0; k<LPC_ORDER; ++k)
                    {
                        /* 这里很关键的信息. 顺着 subband是正序, 整体是逆序 */
                        pred[n*NUM_SUBBAND+k%NUM_SUBBAND]+=plpc[m*LPC_ORDER + k]*ppcm[j + k - (k>3?LPC_ORDER:0)];
                    }
                }
                for (m=0, j=0; m<NUM_SUBBAND; ++m)
                {
                    pred[n*NUM_SUBBAND + m]=-pred[n*NUM_SUBBAND + m];
                    pfexc[1*NUM_SUBBAND+m]=lin2ulaw(pred[n*NUM_SUBBAND+m]);
                }
            }

            cfeat_frame.p=wtk_matf_at(cfeat, fr, 0);
            wtk_tac_lpcnet_decoder( gru_arr, gru_cache, gru_a_ih_out, gru_gin, gru_gout, gru_cout, gru_cout_hh, gru_in, 
              gru_out, h_mf, num_md, md_kernel, md_bias, md_factor, md_out, embed_sig_arr, embed_exc_arr, fexc, iexc, &cfeat_frame);


            pfeat=wtk_matf_at( feat_fold, fr, 0);
            pout=output->p;
            for (k=0; k<num_fold; ++k, pfeat+=num_fea, pmd_out+=pmd_out_len, pout+=output->col*NUM_SUBBAND)
            {
                float po = max(0, 1.5*(pfeat[pitch_idx+1] - 0.5 ));

                def_calc_time();
                for (j=0; j<NUM_SUBBAND; ++j)
                {
                    pmd_out=wtk_matf_at( md_out[j][0], k, 0);
                    pmd_out_len=md_out[j][0]->col;
                    /* softmax hack */
                    for (n=0;n<pmd_out_len;n++)
                    {
                        pmd_out[n] = pmd_out[n] * (1.f+po);
                    }
                    wtk_nn_softmax(pmd_out, pmd_out_len);
                    /* softmax hack end */

                    smooth_multinomial_distribution( pmd_out_len, pmd_out);
                    iexc->p[k*NUM_SUBBAND+j]=wtk_nn_random_weight( pmd_out_len, pmd_out);
                    ppcm=wtk_matf_at(pcm, k, global_step*NUM_SUBBAND);
                    ppcm[j]=pred[k*NUM_SUBBAND + j] + ulaw2lin( iexc->p[k*NUM_SUBBAND+j]);
                    wtk_matf_at(fexc, k, 0)[j] = lin2ulaw( ppcm[j]);
                    /* 注意此处是转置输出 */
                    pout[global_step + j*output->col] = ppcm[j];
                }
                def_calc_time_end(time_other);
            }
        }
        skip=0;
        // wtk_exit(1);
    }


    int wav_len=output->row*output->col;
    int overlap=1;
    float *wav_data=wtk_heap_zalloc( heap, sizeof(float)*(output->row*output->col+10*stft->hop_len));
    // wtk_tac_wavrnn_xfade_unfold( overlap*FRAME_SIZE, output, wav_data);

    float **subband_data;
    int pqmf_col;
    wtk_matf_t 
        output_unfold;
        // *output_unfold=wtk_matf_heap_new( heap, 4, output->row*output->col);
    output_unfold.p=wav_data;
    output_unfold.row=NUM_SUBBAND;
    subband_data=wtk_heap_malloc( heap, sizeof(void*)*NUM_SUBBAND);
    for (i=0; i<NUM_SUBBAND; ++i)
    {
        subband_data[i]=wtk_heap_zalloc( heap, sizeof(float)*(output->row*output->col/NUM_SUBBAND));
    }

    wtk_tac_wavrnn_multiband_xfade_unfold( overlap*FRAME_SIZE, NUM_SUBBAND, num_extend*FRAME_SIZE, output, output_unfold.p, subband_data, &pqmf_col);
    output_unfold.col=pqmf_col;
 
    wtk_mer_matf_shape_print(&output_unfold);
    wtk_tac_wavrnn_pqmf( syn_filter_real, syn_filter_imag, stft, &output_unfold, NUM_SUBBAND, wav_data, &wav_len);
    de_emphasis( wav_data, wav_len);

    wtk_mer_wav_stream_write_float( wav, wav_data, wav_len);
    wtk_heap_delete( heap);

    gettimeofday(&global_finish, NULL);
    double time_lpcnet=calc_time(global_finish, global_start);
#ifdef USE_DEBUG
    printf( "lpc特征提取: %lf seconds weight: %.2lf \n", time_lpc, (time_lpc/time_lpcnet)*100);
    printf( "other: %lf seconds weight: %.2lf \n", time_other, (time_other/time_lpcnet)*100);
    printf( "gru: %lf seconds weight: %.2lf \n", time_gru, (time_gru/time_lpcnet)*100);
    printf( "sigmoid: %lf seconds weight: %.2lf \n", time_sigmoid, (time_sigmoid/time_lpcnet)*100);
    printf( "tanh: %lf seconds weight: %.2lf \n", time_tanh, (time_tanh/time_lpcnet)*100);
    printf( "gru2: %lf seconds weight: %.2lf \n", time_gru2, (time_gru2/time_lpcnet)*100);
    printf( "md: %lf seconds weight: %.2lf \n", time_md, (time_md/time_lpcnet)*100);
    printf( " lpcnet time: %lfs, rate: %lf \n", time_lpcnet, time_lpcnet/(wav->len/(wav->hdr.fmt_sample_rate*sizeof(short)*1.0)));
    printf("已统计性能占比: %f \n", 
        ((time_lpc/time_lpcnet)+
        (time_other/time_lpcnet)+
        (time_gru/time_lpcnet)+
        (time_sigmoid/time_lpcnet)+
        (time_tanh/time_lpcnet)+
        (time_gru2/time_lpcnet)+
        (time_md/time_lpcnet))*100);
#endif
    printf( " lpcnet time: %lfs \n", time_lpcnet);
}

void wtk_tac_lpcnet_generate_parallel2(wtk_tac_cfg_syn_lpcnet_t *cfg,wtk_matf_t *feat_fold,wtk_matf_t *cfeat,wtk_mer_wav_stream_t *wav, int is_end)
{
    struct timeval global_start, global_finish;
    gettimeofday(&global_start, NULL);
    int fr
      , i
      , j
      , k
      , n
      , pmd_out_len
      , global_step
      , skip=LPC_ORDER+1
      , num_md=2
      , num_fea=80
      , num_fold=feat_fold->col/num_fea
      , num_frame=feat_fold->row
      ,num_subband=cfg->num_subband
      , pred_len=num_fold*num_subband;
    wtk_heap_t *heap=wtk_heap_new( 1024);
    wtk_matf_t
        *fexc=wtk_matf_heap_new( heap, num_fold, 2*num_subband),
        *output=wtk_matf_heap_new( heap, num_fold*num_subband, FRAME_SIZE*num_frame),
        cfeat_frame;

    wtk_mati_t *iexc=wtk_mati_heap_new( heap, num_fold, 1*num_subband);
    wtk_matdf_t *pcm=wtk_matdf_heap_new( heap, num_fold, FRAME_SIZE*num_frame*num_subband);
    memset(pcm->p,0,sizeof(double)*pcm->row*pcm->col);
    wtk_matdf_t *lpc=wtk_matdf_heap_new( heap, num_fold, LPC_ORDER*num_subband);
    memset(lpc->p,0,sizeof(double)*lpc->row*lpc->col);

    float 
        *pfeat=feat_fold->p,
        *pcfeat,
        *pfexc,
        *pmd_out,
        *pin,
        *pout;
    
    int *piexc;
    double *ppcm;
    double *plpc;
    
    double *pred=wtk_heap_malloc( heap, sizeof(double)*pred_len);

    int gru_dim=cfg->gru_arr[0]->num_units
      , gru_dim2=cfg->gru_arr[1]->num_units;
    wtk_matf_t
        *gru_a_ih_out=wtk_matf_heap_new( heap, num_fold, gru_dim*3),
        **h_mf=wtk_heap_malloc( heap, sizeof(void*)*2),
        **gru_in=wtk_heap_malloc( heap, sizeof(void*)*2),
        **gru_gin=wtk_heap_malloc( heap, sizeof(void*)*2),
        **gru_gout=wtk_heap_malloc( heap, sizeof(void*)*2),
        **gru_cout=wtk_heap_malloc( heap, sizeof(void*)*2),
        **gru_cout_hh=wtk_heap_malloc( heap, sizeof(void*)*2),
        **gru_out=wtk_heap_malloc( heap, sizeof(void*)*2),
        ***md_out=wtk_heap_malloc( heap, sizeof(void**)*num_subband);
    wtk_nn_rnngru_batch_cache_t *gru_cache=wtk_nn_rnngru_batch_cache_heap_new( heap, num_fold, gru_dim);
    cfg->gru_arr[0]->weight_hh_smf=wtk_nn_matf_sparse_heap_new( heap, cfg->gru_arr[0]->weight_hh);

    h_mf[0]=wtk_matf_heap_new( heap, num_fold, gru_dim);
    h_mf[1]=wtk_matf_heap_new( heap, num_fold, gru_dim2);
    gru_in[0]=wtk_matf_heap_new( heap, num_fold, 128+128+64);
    gru_in[1]=wtk_matf_heap_new( heap, num_fold, gru_dim + 128);
    gru_gin[0]=wtk_matf_heap_new( heap, num_fold, 512 + gru_dim);
    gru_gin[1]=wtk_matf_heap_new( heap, num_fold, gru_in[1]->col + gru_dim2);
    gru_gout[0]=wtk_matf_heap_new( heap, num_fold, gru_dim*2);
    gru_gout[1]=wtk_matf_heap_new( heap, num_fold, gru_dim2*2);
    gru_cout[0]=wtk_matf_heap_new( heap, num_fold, gru_dim);
    gru_cout[1]=wtk_matf_heap_new( heap, num_fold, gru_dim2);
    gru_cout_hh[0]=wtk_matf_heap_new( heap, num_fold, gru_dim);
    gru_cout_hh[1]=wtk_matf_heap_new( heap, num_fold, gru_dim2);
    gru_out[0]=wtk_matf_heap_new( heap, num_fold, gru_dim);
    gru_out[1]=wtk_matf_heap_new( heap, num_fold, gru_dim2);

    assert( num_md == 2);

    for (j=0; j<num_subband; ++j)
    {
        md_out[j]=wtk_heap_malloc( heap, sizeof(void*)*num_md);
        for (i=0; i<num_md; ++i)
        {
            md_out[j][i]=wtk_matf_heap_new( heap, num_fold, 256);
        }
    }

    for(i = 0; i < cfg->lpcnet_num_gru;++i){
        wtk_nn_rnngru_reset(cfg->gru_arr[i]);
        wtk_matf_zero(h_mf[i]);
    }

    cfeat_frame.row=num_fold;
    cfeat_frame.col=cfeat->col/num_fold;

    pfexc=fexc->p;
    piexc=iexc->p;
    for (i=0; i<num_fold; ++i, pfexc+=fexc->col, piexc+=iexc->col)
    {
        pfexc[0] = 128.0f;
        pfexc[1] = 128.0f;
        pfexc[2] = 128.0f;
        pfexc[3] = 128.0f;
        piexc[0] = 128;
        piexc[1] = 128;
        piexc[2] = 128;
        piexc[3] = 128;
    }

    wtk_matdf_t
        lpc_trans,
        lpc_tmp;
    lpc_tmp.p=wtk_heap_malloc( heap, sizeof(double)*LPC_ORDER*num_subband);
    lpc_trans.row=LPC_ORDER;
    lpc_trans.col=num_subband;
    lpc_tmp.row=num_subband;
    lpc_tmp.col=LPC_ORDER;

    for (fr=0; fr<num_frame; ++fr)
    {
        plpc=lpc->p;
        for (i=0; i<num_fold; ++i, plpc+=lpc->col, pfeat+=num_fea)
        {
            def_calc_time();
            lpcnet_multiband_mel_to_lpc2(cfg, pfeat, num_fea, plpc);
            memcpy( lpc_tmp.p, plpc, sizeof(double)*num_subband*LPC_ORDER);
            lpc_trans.p=plpc;
            wtk_matdf_init_transpose( &lpc_tmp, &lpc_trans);
            def_calc_time_end(time_lpc);
        }
        
        wtk_matf_zero(gru_in[0]);
        pin=gru_in[0]->p;
        pcfeat=wtk_matf_at(cfeat, fr, 0);
        for (i=0; i<num_fold; ++i, pin+=gru_in[0]->col, pcfeat+=cfeat_frame.col)
        {
            memcpy( pin+128, pcfeat, cfeat_frame.col*sizeof(float));
        }
        wtk_mer_unblas_sgemm_notrans( gru_in[0], cfg->gru_arr[0]->weight_ih, cfg->gru_arr[0]->bias_ih, gru_a_ih_out);

        for (i=skip; i<FRAME_SIZE; ++i)
        {
            global_step=fr*FRAME_SIZE + i;
            plpc=wtk_matf_at( lpc, 0, 0);
            memset(pred, 0, sizeof(double)*pred_len);
            ppcm=wtk_matf_at( pcm, 0, (global_step-LPC_ORDER)*num_subband); 

            for (n=0, pfexc=fexc->p; n<num_fold; ++n, ppcm+=pcm->col, plpc+=lpc->col, pfexc+=fexc->col)
            {
                int m;
                double tmp = 0.0;
                for (m=0; m<num_subband; ++m)
                {
                    tmp = 0.0;
                    for (k=0; k<LPC_ORDER; ++k)
                    {
                        /* 这里很关键的信息. 顺着 subband是正序, 整体是逆序 */
                        tmp+=plpc[k*num_subband + m]*ppcm[(LPC_ORDER-k-1)*num_subband+m];
                    }
                    pred[m]  = tmp;
                }
                for (m=0, j=0; m<num_subband; ++m)
                {
                    pred[n*num_subband + m]=-pred[n*num_subband + m];
                    pfexc[1*num_subband+m]=lin2ulaw(pred[n*num_subband+m]);
                }
            }

            cfeat_frame.p=wtk_matf_at(cfeat, fr, 0);
            wtk_tac_lpcnet_decoder( cfg->gru_arr, gru_cache, gru_a_ih_out, gru_gin, gru_gout, gru_cout, gru_cout_hh, gru_in, 
              gru_out, h_mf, num_md, cfg->md_kernel, cfg->md_bias, cfg->md_factor, md_out, cfg->embed_sig_arr, cfg->embed_exc_arr, 
              fexc, iexc, &cfeat_frame);

            pfeat=wtk_matf_at( feat_fold, fr, 0);
            pout=output->p;
            for (k=0; k<num_fold; ++k, pfeat+=num_fea, pmd_out+=pmd_out_len, pout+=output->col*num_subband)
            {
                // float po = max(0.0f, 1.5f*(pfeat[pitch_idx+1] - 0.5f ));

                def_calc_time();
                for (j=0; j<num_subband; ++j)
                {
                    pmd_out=wtk_matf_at( md_out[j][0], k, 0);
                    pmd_out_len=md_out[j][0]->col;
                    /* softmax hack */
                    // for (n=0;n<pmd_out_len;n++)
                    // {
                    //     pmd_out[n] = pmd_out[n] * (1.f+po);
                    // }
                    wtk_nn_softmax(pmd_out, pmd_out_len);
                    /* softmax hack end */
                    smooth_multinomial_distribution( pmd_out_len, pmd_out);

                    iexc->p[k*num_subband+j]=wtk_nn_random_weight( pmd_out_len, pmd_out);
                    ppcm=wtk_matf_at(pcm, k, global_step*num_subband);
                    ppcm[j]=pred[k*num_subband + j] + ulaw2lin( iexc->p[k*num_subband+j]);
                    wtk_matf_at(fexc, k, 0)[j] = lin2ulaw( ppcm[j]);
                    /* 注意此处是转置输出 */
                    pout[global_step + j*output->col] = ppcm[j];
                }
                def_calc_time_end(time_other);
            }
        }
        skip=0;
    }

    int wav_len=output->row*output->col;
    float *wav_data=wtk_heap_zalloc( heap, sizeof(float)*(output->row*output->col));

    float **subband_data;
    subband_data=wtk_heap_malloc( heap, sizeof(void*)*num_subband);
    for (i=0; i<num_subband; ++i)
    {
        subband_data[i]=wtk_heap_zalloc( heap, sizeof(float)*(output->row*output->col/num_subband));
    }

    wtk_tac_wavrnn_pqmf( cfg->pqmf_syn_filter_real, cfg->pqmf_syn_filter_imag, cfg->stft, output, num_subband, wav_data, &wav_len);
    de_emphasis( wav_data, wav_len);

    if(cfg->notify){
        cfg->notify(cfg->user_data,wav_data,wav_len,is_end);
    }else{
        wtk_mer_wav_stream_write_float( wav, wav_data, wav_len);
    }
    wtk_heap_delete( heap);
    gettimeofday(&global_finish, NULL);
    double time_lpcnet=calc_time(global_finish, global_start);
#ifdef USE_DEBUG
    printf( "lpc特征提取: %lf seconds weight: %.2lf \n", time_lpc, (time_lpc/time_lpcnet)*100);
    printf( "other: %lf seconds weight: %.2lf \n", time_other, (time_other/time_lpcnet)*100);
    printf( "gru: %lf seconds weight: %.2lf \n", time_gru, (time_gru/time_lpcnet)*100);
    printf( "sigmoid: %lf seconds weight: %.2lf \n", time_sigmoid, (time_sigmoid/time_lpcnet)*100);
    printf( "tanh: %lf seconds weight: %.2lf \n", time_tanh, (time_tanh/time_lpcnet)*100);
    printf( "gru2: %lf seconds weight: %.2lf \n", time_gru2, (time_gru2/time_lpcnet)*100);
    printf( "md: %lf seconds weight: %.2lf \n", time_md, (time_md/time_lpcnet)*100);
    // printf( " lpcnet time: %lfs, rate: %lf \n", time_lpcnet, time_lpcnet/(wav->len/(wav->hdr.fmt_sample_rate*1.0)));
    printf("已统计性能占比: %f \n", 
        ((time_lpc/time_lpcnet)+
        (time_other/time_lpcnet)+
        (time_gru/time_lpcnet)+
        (time_sigmoid/time_lpcnet)+
        (time_tanh/time_lpcnet)+
        (time_gru2/time_lpcnet)+
        (time_md/time_lpcnet))*100);
#endif
    printf( " lpcnet time: %lfs \n", time_lpcnet);
}

void wtk_tac_lpcnet_process(wtk_tac_cfg_syn_lpcnet_t *cfg, wtk_matf_t *mel, wtk_mer_wav_stream_t *wav,int is_end)
{
    wtk_debug("------------> lpcnet_start \n");
    // int fold_len=cfg->fold_len, overlap=cfg->overlap, num_extend=0;
    wtk_matf_t *features_fold = NULL,*cfeat = NULL,*cfeat2 = NULL;

    cfeat2 = wtk_tac_lpcnet_encoder(cfg,mel);

    srand(1024);    //会有随机数  固定随机数
    // if(0){
    //     cfeat=wtk_tac_fold_with_overlap( fold_len, overlap, cfeat2, NULL);
    //     features_fold=wtk_tac_fold_with_overlap( fold_len, overlap, mel, &num_extend);
    //     wtk_tac_lpcnet_generate_parallel( wav, cfg->embed_sig_arr, cfg->embed_exc_arr, cfg->gru_arr, cfg->md_kernel, 
    //         cfg->md_bias, cfg->md_factor, cfg->mel_inv_basis, cfg->pqmf_analysis_filter_real, cfg->pqmf_analysis_filter_imag,
    //         cfg->pqmf_syn_filter_real, cfg->pqmf_syn_filter_imag, cfg->stft, features_fold, num_extend, cfeat);
    // }else{
        wtk_tac_lpcnet_generate_parallel2(cfg,mel,cfeat2,wav,is_end);
    // }
    if(cfeat) wtk_matf_delete(cfeat);
    if(cfeat2) wtk_matf_delete(cfeat2);
    if(features_fold) wtk_matf_delete( features_fold);
}

void wtk_tac_lpcnet_set_notify(wtk_tac_cfg_syn_lpcnet_t *cfg, wtk_lpcnet_notify_f notify, void *user_data)
{
    cfg->notify = notify;
    cfg->user_data = user_data;
    return;
}

int wtk_tac_lpcnet_matf_pzero(wtk_strbuf_t *buf,int size,int n)
{
    char *zero = wtk_calloc(n,sizeof(float)*size);
    wtk_strbuf_push(buf,zero,n*size*sizeof(float));
    wtk_free(zero);
    return 0;
}
//实际上问题主要是conv的问题
wtk_matf_t* wtk_tac_lpcnet_encoder_stream(wtk_tac_cfg_syn_lpcnet_t *cfg,wtk_matf_t *feat,int is_end)
{
    wtk_heap_t *heap = wtk_heap_new(1024);
    int kernel_size = cfg->lpcnet_feat_conv_kernel_size;
    wtk_matf_t cat_feat,
        *feat_out1 = NULL,
        *feat_out2 = NULL,
        *fd1 = NULL,
        *fd2 = NULL,
        *cfeat = NULL;
    int use_flag = 0;

    wtk_strbuf_push(cfg->conv_bufs[0],(char*)feat->p,feat->col*feat->row*sizeof(float));
    if(is_end){
        wtk_tac_lpcnet_matf_pzero(cfg->conv_bufs[0],feat->col,(kernel_size-1)/2);
    }
    if(cfg->conv_bufs[0]->pos >= cfg->feat_conv_kernel[0]->col*sizeof(float)){
        cat_feat.p = (float*)cfg->conv_bufs[0]->data;
        cat_feat.col = feat->col;
        cat_feat.row = cfg->conv_bufs[0]->pos/(sizeof(float)*feat->col);
        feat_out1 = wtk_matf_heap_new( heap, cat_feat.row-(kernel_size-1), cfg->feat_conv_bias[0]->len);
        wtk_nn_conv1d2(cfg->feat_conv_kernel[0], kernel_size, enum_conv_npod, &cat_feat, feat_out1);
        wtk_matf_vecf_add(feat_out1,cfg->feat_conv_bias[0]);
        wtk_nn_tanh(feat_out1->p, feat_out1->row*feat_out1->col);
        use_flag = 1;
    }else{
        goto end;
    }
    if(use_flag && is_end != 1){
        wtk_strbuf_pop(cfg->conv_bufs[0],NULL,feat->col*sizeof(float));
    }

    wtk_strbuf_push(cfg->conv_bufs[1],(char*)feat_out1->p,feat_out1->row*feat_out1->col*sizeof(float));
    if(is_end){
        wtk_tac_lpcnet_matf_pzero(cfg->conv_bufs[1],feat_out1->col,(kernel_size-1)/2);
    }

    if(cfg->conv_bufs[1]->pos >= cfg->feat_conv_kernel[1]->col*sizeof(float)){
        cat_feat.p = (float*)cfg->conv_bufs[1]->data;
        cat_feat.col = feat_out1->col;
        cat_feat.row = cfg->conv_bufs[1]->pos/(sizeof(float)*feat_out1->col);
        // printf("%d\n",cat_feat.row);
        feat_out2 = wtk_matf_heap_new( heap, cat_feat.row-(kernel_size-1), cfg->feat_conv_bias[1]->len);
        wtk_nn_conv1d2( cfg->feat_conv_kernel[1], kernel_size, enum_conv_npod, &cat_feat, feat_out2);
        wtk_matf_vecf_add(feat_out2, cfg->feat_conv_bias[1]);
        wtk_nn_tanh(feat_out2->p, feat_out2->row*feat_out2->col);
        use_flag = 1;
    }else{
        goto end;
    }
    if(use_flag && is_end != 1){
        wtk_strbuf_pop(cfg->conv_bufs[1],NULL,feat_out1->col*sizeof(float));
    }

    fd1 = wtk_matf_heap_new( heap, feat_out2->row, cfg->feat_dense_bias[0]->len);
    fd2 = wtk_matf_heap_new( heap, fd1->row,  cfg->feat_dense_bias[1]->len);

    cat_feat.p = (float*)cfg->mels_buf->data;
    cat_feat.col = feat_out2->col;
    cat_feat.row = feat_out2->row;
    wtk_matf_add2( feat_out2, &cat_feat);

    wtk_nn_layer_dense( feat_out2, cfg->feat_dense_kernel[0],cfg->feat_dense_bias[0], wtk_nn_tanh, fd1);
    wtk_nn_layer_dense( fd1, cfg->feat_dense_kernel[1], cfg->feat_dense_bias[1], wtk_nn_tanh, fd2);
    cfeat = wtk_matf_new(fd2->row,fd2->col);
    memcpy(cfeat->p,fd2->p,fd2->row*fd2->col*sizeof(float));
end:
    wtk_heap_delete(heap);
    return cfeat;
}

//一般mel是一帧一帧的进的
void wtk_tac_lpcnet_process_stream(wtk_tac_cfg_syn_lpcnet_t *cfg, wtk_matf_t *mel,int is_end)
{
    wtk_matf_t *cfeat2 = NULL;
    wtk_matf_t feat_fold;
    
    if(cfg->have_reset){
        wtk_tac_lpcnet_matf_pzero(cfg->conv_bufs[0],mel->col,(cfg->lpcnet_feat_conv_kernel_size-1)/2);
        wtk_tac_lpcnet_matf_pzero(cfg->conv_bufs[1],cfg->feat_conv_kernel[0]->row,(cfg->lpcnet_feat_conv_kernel_size-1)/2);
        wtk_tac_lpcnet_generate_stream_reset(cfg);
        srand(1024);
        cfg->have_reset = 0;
        cfg->pqmf_pad_flag = 1;
        cfg->pqmf_out_n = 0;
        cfg->mem = 0;
        wtk_strbufs_reset(cfg->stream_wav_buf,cfg->num_subband);
    }
    wtk_strbuf_push(cfg->mels_buf,(char*)mel->p,sizeof(float)*mel->row*mel->col);
    cfeat2 = wtk_tac_lpcnet_encoder_stream(cfg,mel,is_end);
    if(cfeat2 == NULL) goto end;
    
    feat_fold.p = (float*)cfg->mels_buf->data;
    feat_fold.row = cfeat2->row;
    feat_fold.col = mel->col;
    wtk_tac_lpcnet_generate_parallel2_stream(cfg,&feat_fold,cfeat2,is_end);
    wtk_strbuf_pop(cfg->mels_buf,NULL,sizeof(float)*feat_fold.row*feat_fold.col);

    if(is_end){
        wtk_strbufs_reset(cfg->conv_bufs,cfg->lpcnet_num_feat_conv);
        wtk_strbuf_reset(cfg->mels_buf);
        cfg->have_reset = 1;
    }
end:
    if(cfeat2) wtk_matf_delete(cfeat2);
    return;
}

void wtk_tac_lpcnet_generate_stream_reset(wtk_tac_cfg_syn_lpcnet_t *cfg)
{
    wtk_heap_reset(cfg->stream_heap);
    int i,j;
    int num_md=cfg->lpcnet_num_md
      ,num_fold=1
      ,num_subband=cfg->num_subband;
    wtk_heap_t *heap = cfg->stream_heap;

    cfg->stream_fexc=wtk_matf_heap_new( heap, num_fold, 2*num_subband);
    cfg->stream_iexc=wtk_mati_heap_new( heap, num_fold, 1*num_subband);

    int gru_dim=cfg->gru_arr[0]->num_units, 
        gru_dim2=cfg->gru_arr[1]->num_units;
    
    cfg->stream_gru_a_ih_out =wtk_matf_heap_new( heap, num_fold, gru_dim*3),
    cfg->stream_h_mf=wtk_heap_malloc( heap, sizeof(void*)*2),
    cfg->stream_gru_in=wtk_heap_malloc( heap, sizeof(void*)*2),
    cfg->stream_gru_gin=wtk_heap_malloc( heap, sizeof(void*)*2),
    cfg->stream_gru_gout=wtk_heap_malloc( heap, sizeof(void*)*2),
    cfg->stream_gru_cout=wtk_heap_malloc( heap, sizeof(void*)*2),
    cfg->stream_gru_cout_hh=wtk_heap_malloc( heap, sizeof(void*)*2),
    cfg->stream_gru_out=wtk_heap_malloc( heap, sizeof(void*)*2),
    cfg->stream_md_out=wtk_heap_malloc( heap, sizeof(void**)*num_subband);
    cfg->stream_gru_cache=wtk_nn_rnngru_batch_cache_heap_new( heap, num_fold, gru_dim);
    cfg->gru_arr[0]->weight_hh_smf=wtk_nn_matf_sparse_heap_new( heap, cfg->gru_arr[0]->weight_hh);

    cfg->stream_h_mf[0]=wtk_matf_heap_new( heap, num_fold, gru_dim);
    cfg->stream_h_mf[1]=wtk_matf_heap_new( heap, num_fold, gru_dim2);
    cfg->stream_gru_in[0]=wtk_matf_heap_new( heap, num_fold, 128+128+64);
    cfg->stream_gru_in[1]=wtk_matf_heap_new( heap, num_fold, gru_dim + 128);
    cfg->stream_gru_gin[0]=wtk_matf_heap_new( heap, num_fold, 512 + gru_dim);
    cfg->stream_gru_gin[1]=wtk_matf_heap_new( heap, num_fold, cfg->stream_gru_in[1]->col + gru_dim2);
    cfg->stream_gru_gout[0]=wtk_matf_heap_new( heap, num_fold, gru_dim*2);
    cfg->stream_gru_gout[1]=wtk_matf_heap_new( heap, num_fold, gru_dim2*2);
    cfg->stream_gru_cout[0]=wtk_matf_heap_new( heap, num_fold, gru_dim);
    cfg->stream_gru_cout[1]=wtk_matf_heap_new( heap, num_fold, gru_dim2);
    cfg->stream_gru_cout_hh[0]=wtk_matf_heap_new( heap, num_fold, gru_dim);
    cfg->stream_gru_cout_hh[1]=wtk_matf_heap_new( heap, num_fold, gru_dim2);
    cfg->stream_gru_out[0]=wtk_matf_heap_new( heap, num_fold, gru_dim);
    cfg->stream_gru_out[1]=wtk_matf_heap_new( heap, num_fold, gru_dim2);

    for(i = 0; i < cfg->lpcnet_num_gru;++i){
        wtk_nn_rnngru_reset(cfg->gru_arr[i]);
        wtk_matf_zero(cfg->stream_h_mf[i]);
    }

    for (j=0; j<num_subband; ++j)
    {
        cfg->stream_md_out[j]=wtk_heap_malloc( heap, sizeof(void*)*num_md);
        for (i=0; i<num_md; ++i)
        {
            cfg->stream_md_out[j][i]=wtk_matf_heap_new( heap, num_fold, 256);
        }
    }
    cfg->stream_skip = LPC_ORDER+1;
    cfg->pre_pcm = wtk_matdf_heap_new(heap, 1, FRAME_SIZE*num_subband);
    wtk_zero(cfg->pre_pcm->p,sizeof(double)* FRAME_SIZE*num_subband);

    float *pfexc=cfg->stream_fexc->p;
    int *piexc=cfg->stream_iexc->p;
    for (i=0; i<num_fold; ++i, pfexc+=cfg->stream_fexc->col, piexc+=cfg->stream_iexc->col)
    {
        pfexc[0] = 128.0f;
        pfexc[1] = 128.0f;
        pfexc[2] = 128.0f;
        pfexc[3] = 128.0f;
        piexc[0] = 128;
        piexc[1] = 128;
        piexc[2] = 128;
        piexc[3] = 128;
    }

    return;
}

int wtk_tac_wavrnn_pqmf_stream_step1(wtk_tac_cfg_syn_lpcnet_t *cfg,wtk_matf_t *in, int is_end)
{
    int i = 0,j = 0;
    int nfft2 = cfg->stft->nfft/2;
    float *norm = NULL;
    int subbnad = in->row;
    int norm_len  = in->col * subbnad;
    float *pin = NULL;
    float *fp = NULL;
    int data_len = 0;

    norm = wtk_malloc(sizeof(float)*norm_len);
    wtk_memset(norm,0,sizeof(float)*norm_len);
    pin = in->p;
    for(i = 0;i < subbnad; ++i){
        for(j = 0; j < in->col; ++j){
            norm[j*subbnad] = 4*pin[j];
        }
        pin += in->col;
        wtk_strbuf_push(cfg->stream_wav_buf[i],(char*)norm,sizeof(float)*norm_len);
    }
    wtk_free(norm);
    if(cfg->stream_wav_buf[0]->pos/sizeof(float) >= nfft2 && cfg->pqmf_pad_flag == 1){
        //进行镜像扩充 pad
        norm = wtk_malloc(sizeof(float)*nfft2);
        for(i = 0; i < subbnad; ++i){
            fp = (float*)cfg->stream_wav_buf[i]->data;
            for(j = 0; j < nfft2; ++j){
                norm[j] = fp[nfft2-j];
            }
            wtk_strbuf_push_front(cfg->stream_wav_buf[i],(char*)norm,sizeof(float)*nfft2);
        }
        wtk_free(norm);
        cfg->pqmf_pad_flag = 0;
    }else if(cfg->pqmf_pad_flag == 1 && is_end){   //在没有到达三帧数据时 并且is_end 的情况
        norm = wtk_malloc(sizeof(float)*nfft2);
        for(i = 0; i < subbnad; ++i){
            fp = (float*)cfg->stream_wav_buf[i]->data;
            data_len = cfg->stream_wav_buf[i]->pos/sizeof(float);
            for(j = 1;j < data_len; ++j){
                norm[nfft2-j-1] = fp[j];
            }
            wtk_strbuf_push_front(cfg->stream_wav_buf[i],(char*)norm,sizeof(float)*nfft2);
        }
        wtk_free(norm);
        cfg->pqmf_pad_flag = 0;
    }
    if(is_end){
        norm = wtk_malloc(sizeof(float)*nfft2);
        for(i = 0; i < subbnad; ++i){
            fp = (float*)cfg->stream_wav_buf[i]->data;
            data_len = cfg->stream_wav_buf[i]->pos/sizeof(float);
            for(j = 0; j < nfft2; ++j){
                norm[j] = fp[data_len-j-2];
            }
            wtk_strbuf_push(cfg->stream_wav_buf[i],(char*)norm,sizeof(float)*nfft2);
        }
        wtk_free(norm);
    }
    return 0;
}

float* wtk_tac_wavrnn_pqmf_stream_step2(wtk_tac_cfg_syn_lpcnet_t *cfg,int *out_len,int is_end)
{
    wtk_heap_t *heap = wtk_heap_new(1024);
    int nfft = cfg->stft->nfft;
    int hop_len = cfg->stft->hop_len;
    wtk_matf_t *spec_filter_real = cfg->pqmf_syn_filter_real;
    wtk_matf_t *spec_filter_imag = cfg->pqmf_syn_filter_imag;
    wtk_nn_stft_t *stft = cfg->stft;
    int subbnad = cfg->num_subband;
    float *data = NULL;
    int data_len = cfg->stream_wav_buf[0]->pos/(sizeof(float));
    int n_frame = (data_len-nfft)/hop_len+1;
    float *win_hann_sumsquare = wtk_nn_istft_win_sumsquare_heap_new(heap,stft,n_frame);
    int i = 0,j = 0;
    int nfreq = stft->nfreq;
    int istft_out_len=(hop_len*(n_frame - 1) + nfft);
    float *istft_out = wtk_heap_malloc( heap, sizeof(float)*istft_out_len);
    // printf("%d %d %d %d\n",istft_out_len,n_frame,data_len,hop_len);

    wtk_nn_complex_t *cpx_stft_out = wtk_nn_complex_new(nfft*n_frame);
    wtk_nn_complex_t *cpx_istft_in = wtk_nn_complex_new(nfft*n_frame);
    wtk_nn_complex_t cpx_tmp1,cpx_tmp2,cpx_tmp3;

    float *out = wtk_heap_zalloc(cfg->stream_heap,sizeof(float)*n_frame*hop_len);

    for(i = 0; i < subbnad; ++i){
        data = (float*)cfg->stream_wav_buf[i]->data;
        data_len = cfg->stream_wav_buf[i]->pos/sizeof(float);
        wtk_nn_stft(stft,data,data_len,cpx_stft_out);

        cpx_tmp1.len=cpx_tmp2.len=cpx_tmp3.len=nfreq;
        cpx_tmp1.real=cpx_stft_out->real;
        cpx_tmp1.imag=cpx_stft_out->imag;
        cpx_tmp2.real=wtk_matf_at( spec_filter_real, i, 0);
        cpx_tmp2.imag=wtk_matf_at( spec_filter_imag, i, 0);
        cpx_tmp3.real=cpx_istft_in->real;
        cpx_tmp3.imag=cpx_istft_in->imag;
        for(j = 0;j<n_frame;++j){
            cpx_tmp1.real[0]=0;
            cpx_tmp1.imag[0]=0;
            wtk_nn_complex_mul( &cpx_tmp1, &cpx_tmp2, &cpx_tmp3);
            cpx_tmp1.real+=nfreq;
            cpx_tmp1.imag+=nfreq;
            cpx_tmp3.real+=nfreq;
            cpx_tmp3.imag+=nfreq;   
        }
        wtk_nn_istft( stft, n_frame, win_hann_sumsquare, cpx_istft_in, istft_out);
        wtk_float_add(out, istft_out+nfft/2, hop_len*n_frame);
    }

    if (out_len) {out_len[0]=hop_len*n_frame;}
    wtk_nn_complex_delete(cpx_stft_out);
    wtk_nn_complex_delete(cpx_istft_in);
    wtk_heap_delete(heap);

    return out;
}

float* wtk_tac_wavrnn_pqmf_stream_step3(wtk_tac_cfg_syn_lpcnet_t *cfg, float *in, int *out_len, int is_end)
{
    wtk_heap_t *heap = cfg->stream_heap;
    int hop_len = cfg->hop_len;
    float *ret_out = NULL;
    int n_f = 0;
    int skip_len = 0;
    int pop = 0;
    int num_sbuband = cfg->num_subband;
    // int nfft2 = cfg->stft->nfft/2;
    // int nfft = cfg->stft->nfft;

    if(is_end){
        if(cfg->pqmf_out_n == 0){
            skip_len = 0;
            n_f = (*out_len)/hop_len-1;
        }else if(cfg->pqmf_out_n == 1){
            skip_len = hop_len;
            n_f = (*out_len)/hop_len - 2;
        }else if(cfg->pqmf_out_n > 1){
            skip_len = hop_len*2;
            n_f = (*out_len)/hop_len - 3;
        }
        goto end;
    }

    if(cfg->pqmf_out_n == 0){
        skip_len = 0;
        n_f = 1;
    }else if(cfg->pqmf_out_n == 1){
        skip_len = hop_len;
        n_f = 1;
    }else if(cfg->pqmf_out_n > 1){
        skip_len = hop_len*2;
        n_f = 1;
        pop = 1;
    }

end:
    if(n_f > 0){
        *out_len = hop_len * n_f;
        ret_out = (float*)wtk_heap_malloc(heap,n_f*hop_len*sizeof(float));
        wtk_memcpy(ret_out,in+skip_len,sizeof(float)*n_f*hop_len);
        if(pop){
            wtk_strbufs_pop(cfg->stream_wav_buf,num_sbuband,hop_len*sizeof(float));
        }
        cfg->pqmf_out_n += n_f;
    }else{
        *out_len = 0;
    }
    return ret_out;
}

float* wtk_tac_wavrnn_pqmf_stream(wtk_tac_cfg_syn_lpcnet_t *cfg,wtk_matf_t *in, int *out_len,int is_end)
{
    int nfft = cfg->stft->nfft;
    // int n = cfg->num_subband;
    float *out_wav = NULL;
    //存储数据进行格式化
    wtk_tac_wavrnn_pqmf_stream_step1(cfg,in,is_end);
    //理论上向后延4帧
    if(cfg->stream_wav_buf[0]->pos/sizeof(float) > (nfft+4*cfg->stft->hop_len) || is_end){
        out_wav = wtk_tac_wavrnn_pqmf_stream_step2(cfg,out_len,is_end);
        //对数据进行裁剪和选择
        out_wav = wtk_tac_wavrnn_pqmf_stream_step3(cfg,out_wav,out_len,is_end);
    }else{
        *out_len = 0;
    }
    return out_wav;
}

void wtk_tac_lpcnet_generate_parallel2_stream(wtk_tac_cfg_syn_lpcnet_t *cfg,wtk_matf_t *feat_fold,wtk_matf_t *cfeat, int is_end)
{
    int fr, i, j, k, n
      , pmd_out_len
      , global_step
      , skip=cfg->stream_skip
      , num_md=cfg->lpcnet_num_md
      , num_fea=80
      , num_fold=feat_fold->col/num_fea
      , num_frame=feat_fold->row
      ,num_subband=cfg->num_subband
      , pred_len=num_fold*num_subband;
    wtk_heap_t *heap=wtk_heap_new( 1024);
    wtk_matf_t
        *fexc=cfg->stream_fexc,
        *output=wtk_matf_heap_new( heap, num_fold*num_subband, FRAME_SIZE*num_frame),
        cfeat_frame;

    wtk_mati_t *iexc=cfg->stream_iexc;
    wtk_matdf_t *pcm=wtk_matdf_heap_new( heap, num_fold, FRAME_SIZE*(num_frame+1)*num_subband); //多申请一帧的数据 储存上一帧的数据
    memset(pcm->p,0,sizeof(double)*pcm->row*pcm->col);
    wtk_matdf_t *lpc=wtk_matdf_heap_new( heap, num_fold, LPC_ORDER*num_subband);
    memset(lpc->p,0,sizeof(double)*lpc->row*lpc->col);

    float *pfeat=feat_fold->p,*pcfeat,*pfexc,*pmd_out,*pin,*pout;
    double *ppcm,*plpc;
    double *pred=wtk_heap_malloc( heap, sizeof(double)*pred_len);

    wtk_matf_t
        *gru_a_ih_out=cfg->stream_gru_a_ih_out,
        **h_mf=cfg->stream_h_mf,
        **gru_in=cfg->stream_gru_in,
        **gru_gin=cfg->stream_gru_gin,
        **gru_gout=cfg->stream_gru_gout,
        **gru_cout=cfg->stream_gru_cout,
        **gru_cout_hh=cfg->stream_gru_cout_hh,
        **gru_out=cfg->stream_gru_out,
        ***md_out=cfg->stream_md_out;
    wtk_nn_rnngru_batch_cache_t *gru_cache=cfg->stream_gru_cache;

    assert( num_md == 2);

    cfeat_frame.row=num_fold;
    cfeat_frame.col=cfeat->col/num_fold;

    wtk_matdf_t
        lpc_trans,
        lpc_tmp;
    lpc_tmp.p=wtk_heap_malloc( heap, sizeof(double)*LPC_ORDER*num_subband);
    lpc_trans.row=LPC_ORDER;
    lpc_trans.col=num_subband;
    lpc_tmp.row=num_subband;
    lpc_tmp.col=LPC_ORDER;

    //在流式算法里面  pcm 需要把上一帧的数据copy进来
    wtk_memcpy(pcm->p,cfg->pre_pcm->p,
                sizeof(double)*FRAME_SIZE*num_subband);
    
    for (fr=0; fr<num_frame; ++fr)
    {
        plpc=lpc->p;
        for (i=0; i<num_fold; ++i, plpc+=lpc->col, pfeat+=num_fea)
        {
            def_calc_time();
            lpcnet_multiband_mel_to_lpc2(cfg, pfeat, num_fea, plpc);
            memcpy( lpc_tmp.p, plpc, sizeof(double)*num_subband*LPC_ORDER);
            lpc_trans.p=plpc;
            wtk_matdf_init_transpose( &lpc_tmp, &lpc_trans);
            def_calc_time_end(time_lpc);
        }
        
        wtk_matf_zero(gru_in[0]);
        pin=gru_in[0]->p;
        pcfeat=wtk_matf_at(cfeat, fr, 0);
        for (i=0; i<num_fold; ++i, pin+=gru_in[0]->col, pcfeat+=cfeat_frame.col)
        {
            memcpy( pin+128, pcfeat, cfeat_frame.col*sizeof(float));
        }
        wtk_mer_unblas_sgemm_notrans( gru_in[0], cfg->gru_arr[0]->weight_ih, cfg->gru_arr[0]->bias_ih, gru_a_ih_out);

        for (i=skip; i<FRAME_SIZE; ++i)
        {
            global_step=fr*FRAME_SIZE + i;
            plpc=wtk_matf_at( lpc, 0, 0);
            memset(pred, 0, sizeof(double)*pred_len);
            ppcm=wtk_matf_at(pcm, 0, FRAME_SIZE*num_subband+(global_step-LPC_ORDER)*num_subband); 
            
            for (n=0, pfexc=fexc->p; n<num_fold; ++n, ppcm+=pcm->col, plpc+=lpc->col, pfexc+=fexc->col)
            {
                int m;
                double tmp = 0.0;
                for (m=0; m<num_subband; ++m)
                {
                    tmp = 0.0;
                    for (k=0; k<LPC_ORDER; ++k)
                    {
                        /* 这里很关键的信息. 顺着 subband是正序, 整体是逆序 */
                        tmp+=plpc[k*num_subband + m]*ppcm[(LPC_ORDER-k-1)*num_subband+m];
                    }
                    pred[m]  = tmp;
                }
                for (m=0, j=0; m<num_subband; ++m)
                {
                    pred[n*num_subband + m]=-pred[n*num_subband + m];
                    pfexc[1*num_subband+m]=lin2ulaw(pred[n*num_subband+m]);
                }
            }

            cfeat_frame.p=wtk_matf_at(cfeat, fr, 0);
            wtk_tac_lpcnet_decoder( cfg->gru_arr, gru_cache, gru_a_ih_out, gru_gin, gru_gout, gru_cout, gru_cout_hh, gru_in, 
              gru_out, h_mf, num_md, cfg->md_kernel, cfg->md_bias, cfg->md_factor, md_out, cfg->embed_sig_arr, cfg->embed_exc_arr, 
              fexc, iexc, &cfeat_frame);

            pfeat=wtk_matf_at( feat_fold, fr, 0);
            pout=output->p;
            for (k=0; k<num_fold; ++k, pfeat+=num_fea, pmd_out+=pmd_out_len, pout+=output->col*num_subband)
            {
                def_calc_time();
                for (j=0; j<num_subband; ++j)
                {
                    pmd_out=wtk_matf_at( md_out[j][0], k, 0);
                    pmd_out_len=md_out[j][0]->col;

                    wtk_nn_softmax(pmd_out, pmd_out_len);
                    /* softmax hack end */
                    smooth_multinomial_distribution( pmd_out_len, pmd_out);

                    iexc->p[k*num_subband+j]=wtk_nn_random_weight( pmd_out_len, pmd_out);
                    ppcm=wtk_matf_at(pcm, k,FRAME_SIZE*num_subband+global_step*num_subband);
                    ppcm[j]=pred[k*num_subband + j] + ulaw2lin( iexc->p[k*num_subband+j]);
                    wtk_matf_at(fexc, k, 0)[j] = lin2ulaw( ppcm[j]);
                    /* 注意此处是转置输出 */
                    pout[global_step + j*output->col] = ppcm[j];
                }
                def_calc_time_end(time_other);
            }
        }
        skip=0;
    }
    cfg->stream_skip = skip;
    //保存pcm进行下一帧
    cfg->pre_pcm = wtk_matdf_heap_new(cfg->stream_heap,1,FRAME_SIZE*num_subband);
    wtk_memcpy(cfg->pre_pcm->p,pcm->p+FRAME_SIZE*num_subband*num_frame,sizeof(double)*FRAME_SIZE*num_subband);

    int wav_len=output->row*output->col;
    float **subband_data;
    subband_data=wtk_heap_malloc( heap, sizeof(void*)*num_subband);
    for (i=0; i<num_subband; ++i)
    {
        subband_data[i]=wtk_heap_zalloc( heap, sizeof(float)*(output->row*output->col/num_subband));
    }
    float *wav_data = NULL;
    wav_len = 0;
    wav_data = wtk_tac_wavrnn_pqmf_stream(cfg,output,&wav_len,is_end);
    if(wav_data && wav_len > 0){
        de_emphasis_stream( wav_data, wav_len,&cfg->mem);
    }
    if(cfg->notify && wav_len){
        cfg->notify(cfg->user_data,wav_data,wav_len,is_end);
    }
    wtk_heap_delete( heap);
    
    return;
}