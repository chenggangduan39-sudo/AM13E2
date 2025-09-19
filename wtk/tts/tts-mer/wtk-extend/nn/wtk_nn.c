#include "wtk_nn.h"

/* 
数据均化函数，是现在的数据排布符合某种分布形式
shape [batch, height, width, channel]
gamma*((x-mean)/ sqrt(variance + epsilon)) + beta

epsilon: 为防止除0而加的一个很小的数

center为True时，添加位移因子beta到该BN层，否则不添加。添加beta是对BN层的变换加入位移操作。注意，beta一般设定为可训练参数，
    即trainable=True。

scale为True是，添加缩放因子gamma到该BN层，否则不添加。添加gamma是对BN层的变化加入缩放操作。注意，gamma一般设定为可训练参数，
    即trainable=True。

training表示模型当前的模式，如果为True，则模型在训练模式，否则为推理模式。要非常注意这个模式的设定，这个参数默认值为False。
    如果在训练时采用了默认值False，则滑动均值moving_mean和滑动方差moving_variance都不会根据当前batch的数据更新，这就意味着在推理模
    式下，均值和方差都是其初始值，因为这两个值并没有在训练迭代过程中滑动更新。
 */
void wtk_nn_batch_norm(wtk_matf_t *in, wtk_vecf_t *gamma, wtk_vecf_t *beta, wtk_vecf_t *mean, wtk_vecf_t *variance, float epsilon)
{
    epsilon= (epsilon==0?1e-5:epsilon);
    int size = in->row
      , channel = in->col
      , i
      , j;
    float 
        *x = in->p,
        *v = malloc(sizeof(float)*channel),
        *g = gamma->p,
        *m = mean->p,
        *b = beta->p;
    wtk_float_set(v, epsilon, channel);
    wtk_float_add(v, variance->p, channel);

    for (i=0; i<channel; ++i)
    {
        v[i] = sqrtf(v[i]);
    }
    for (j=0; j<size; ++j, x+=channel)
    {
        for (i=0; i<channel; ++i)
        {
            x[i] = ((x[i]-m[i])/ v[i])*g[i] + b[i];
        }
    }
    wtk_free(v);
}
/**
 * 数据均化函数
 * math:  y = \frac{x - \mathrm{E}[x]}{ \sqrt{\mathrm{Var}[x] + \epsilon}} * \gamma + \beta
 * 其中的  \mathrm{E}[x] 和 \mathrm{Var}[x] 需要在本次计算
 * pythorh 实现的时候是在最后的维度上实现的
 * [x] x 为最后维度 [1,24,31] 算[31]维度上的均值和方差
 * [x,y] 算[24,31]上的均值和方差
 * 本函数是算[x,y,z] 全部值x*y*z上的均值和方差
 */
void wtk_nn_layer_norm(wtk_matf_t *input,wtk_vecf_t *gamma,wtk_vecf_t *beta,float epsilon)
{
    float E = 0.0f,V = 0.0f,t = 0.0f;
    int N = 0,i = 0,j = 0;
    float *p = 0;

    N = input->col*input->row;
    p = input->p;
    for(i = 0; i < N; ++i){
        E += p[i];
    }
    E = E / N;
    p = input->p;
    for(i = 0; i < N;++i){
        t = (p[i] - E);
        V += t*t;
    }
    V = V / N;
    V = sqrtf(V+epsilon);
    p = input->p;
    for(i = 0; i < input->row; ++i,p+=input->col){
        for(j = 0; j < input->col;++j){
            p[j] = ((p[j] - E)/V)*gamma->p[j] + beta->p[j];
        }
    }
    return;
}

void wtk_nn_layer_norm_1dim(wtk_matf_t *input,wtk_vecf_t *gamma,wtk_vecf_t *beta,float epsilon)
{
    float Ep = 0.0f,Vp = 0.0f,t = 0.0f;
    int i = 0,j = 0;
    float *p = 0;
    wtk_vecf_t *E,*Var;

    E = wtk_vecf_new(input->row);
    Var = wtk_vecf_new(input->row);
    // N = input->col*input->row;
    p = input->p;
    for(i = 0; i < input->row; ++i){
        for(Ep = 0.0f,j =0;j < input->col;++j){
            Ep += p[i*input->col+j];
        }
        E->p[i] = Ep/input->col;
    }
    for(i = 0; i < input->row;++i){
        for(Vp = 0,j = 0;j<input->col;++j){
            t = p[i*input->col+j]-E->p[i];
            Vp += t*t;
        }
        Vp = Vp/input->col;
        Var->p[i] = sqrtf(Vp+epsilon);
    }
    for(i = 0; i < input->row; ++i,p+=input->col){
        for(j = 0; j < input->col;++j){
            p[j] = ((p[j] - E->p[i])/Var->p[i])*gamma->p[j] + beta->p[j];
        }
    }
    wtk_vecf_delete(E);
    wtk_vecf_delete(Var);
    return;
}

void wtk_nn_layer_dense( wtk_matf_t *input, wtk_matf_t *kernel, wtk_vecf_t *bias, void activation(float*, int), wtk_matf_t *output)
{ /* out = activation(in*kernel + bias) */
    wtk_mer_blas_sgemm2(input, kernel, bias, output);
    if (activation) 
    { activation(output->p, output->row*output->col); }
}

/* 
kernel.shape  [kernel_size * input->col, filter]
kernel 需要转置
kernel_size*input->col == kernel->col
input.shape [row,dim]   实际上是个二维数组而不是类是kernel的三维转二维数组
kernel_size : 卷积核的大小，卷积核其实应该是一个二维的，这里只需要指定一维，是因为卷积核的第二维与输入的词向量维度是一致的，
                           因为对于句子而言，卷积的移动方向只能是沿着词的方向，即只能在列维度移动 
*/
void wtk_nn_conv1d(wtk_matf_t *kernel, int kernel_size, enum_conv padding, wtk_matf_t *input, wtk_matf_t *dst)
{
    // strides[] = {1,1,1,1}
    int k_row = kernel->row
      , k_col = kernel->col
      , i_row = input->row
      , i_col = input->col
      , filter = k_row
      , j, s, e, en, h;
    size_t 
        blas_in_stlen = sizeof(float)*k_col,
        in_stlen = sizeof(float)*i_row*i_col;
    float 
        *in,
        *dstp = dst->p;
    wtk_matf_t 
        blas_in,
        blas_out;
    
    blas_in.col = k_col;
    blas_in.row = blas_out.row = 1;
    blas_out.col = filter;

    if (kernel_size*i_col != k_col)
    {
        wtk_debug("卷积核形状错误 (%d, %d) 请参考注释!\n", k_row, k_col);
        wtk_exit(1);
    }

    s=0;
    h=kernel_size-1;
    en=i_row-h;
    switch (padding)
    {
        case enum_conv_npod:
            break;
        case enum_conv_same:
            if (dst->row!=input->row || dst->col!=filter)
            {wtk_debug("卷积dst大小异常: dst_row: %d != input->row: %d || dst_col: %d != filter:%d \n", dst->row, i_row, dst->col, filter);wtk_exit(1);}
            float *in_cache = malloc(blas_in_stlen);
            int zero_len;
            size_t other_stlen;
            memset(in_cache, 0, blas_in_stlen);
            in=input->p;
            s=h/2;
            dstp = dst->p;
            blas_in.p = in_cache;
            /* 头 */
            for (j=s; j>0; --j, dstp+=filter)
            {
                zero_len = j*i_col;
                other_stlen = blas_in_stlen-zero_len*sizeof(float);
                if (in_stlen < other_stlen)
                {
                    memset(in_cache+zero_len, 0, other_stlen);
                    memcpy(in_cache+zero_len, in, in_stlen);
                } else {
                    memcpy(in_cache+zero_len, in, blas_in_stlen-zero_len*sizeof(float));
                }
                blas_out.p = dstp;
                wtk_mer_blas_sgemm2(&blas_in, kernel, NULL, &blas_out);
                if (s-j==i_row-1)
                {// 提前填充满了. 当i_row 远小于kernel_size/2
                    goto end;
                }
            }
            e=h-s;
            in = wtk_matf_at(input, en+e-1, 0);
            dstp = wtk_matf_at(dst, i_row-1, 0);
            blas_in.p = in_cache;
            memset(in_cache, 0, blas_in_stlen);
            /* 尾, 结果从后往前填充*/
            for (j=e; j>0; --j, in-=i_col, dstp-=filter)
            {
                zero_len = j*i_col;
                blas_out.p = dstp;
                memcpy(in_cache, in, blas_in_stlen-zero_len*sizeof(float));
                wtk_mer_blas_sgemm2(&blas_in, kernel, NULL, &blas_out);
            }
            end:
                free(in_cache);
            break;
        default:
            wtk_debug("未实现的卷积填充方式: enum %d\n", padding);
            wtk_exit(1);
            break;
    }
    in = input->p;
    dstp = wtk_matf_at(dst, s, 0);
    for (j=0; j<en; ++j, in+=i_col, dstp += filter)
    {
        blas_in.p = in;
        blas_out.p = dstp;
        wtk_mer_blas_sgemm2(&blas_in, kernel, NULL, &blas_out);
    }
}
/* 
将input填充成大矩阵进行运算 
*/
void wtk_nn_conv1d2(wtk_matf_t *kernel, int kernel_size, enum_conv padding, wtk_matf_t *input, wtk_matf_t *dst)
{
    wtk_heap_t *heap=wtk_heap_new(4096);
    int k_row=kernel->row
      , k_col=kernel->col
      , i_row=input->row
      , i_col=input->col
      , out_row=dst->row
      , zero_len
      , i
      , s
      , e
      , h;
      //,en;
    wtk_matf_t
        *blas_in=wtk_matf_heap_new( heap, out_row, k_col);
    float *inp, *blas_inp;
    size_t col_stlen=sizeof(float)*k_col;
    int pod_data = 0;
    int in_data = i_row * i_col;

    // wtk_debug("%d %d %d\n",kernel_size,k_row,k_col);
    if (kernel_size*i_col != k_col)
    {
        wtk_debug("卷积核形状错误 (%d, %d) 请参考注释!\n", k_row, k_col);
        wtk_exit(1);
    }

    s=0;
    e=0;
    h=kernel_size-1;
    // en=i_row-h;
    switch (padding)
    {
        case enum_conv_npod:
            break;
        case enum_conv_same:
            s=h/2;
            e=h-s;
            break;
        default:
            wtk_debug("未实现的卷积填充方式 vaild \n");
            wtk_exit(1);
            break;
    }

    blas_inp=blas_in->p;
    inp=input->p;
    // wtk_debug("%d %d \n",k_col,i_col);

    for (i=0; i<out_row; ++i, blas_inp+=k_col)
    {
        if (s>i)
        {// 头
            zero_len=(s-i)*i_col;
            pod_data = k_col-zero_len;
            if(pod_data<in_data){
                memcpy(blas_inp+zero_len, inp, sizeof(float)*(k_col-zero_len));
            }else{
                memcpy(blas_inp+zero_len, inp, sizeof(float)*(in_data));    //sys
            }
        } else if (i+e>=out_row)
        {// 尾
            zero_len=(i+e-out_row+1)*i_col;
            // wtk_debug("i: %d kernel_size: %d i_row:  %d\n", i, kernel_size, i_row);
            memcpy(blas_inp, inp, col_stlen-sizeof(float)*zero_len);
            inp+=i_col;
        } else {
            zero_len=0;
            if (i_row+s-i<kernel_size)
            { wtk_debug(" 未考虑到的cnn情况, input_row < kernel_size \n"); wtk_exit(1);}
            memcpy(blas_inp, inp, col_stlen);
            inp+=i_col;
        }
        // wtk_debug("zero_len: %d i: %d \n", zero_len, i);
    }
    wtk_mer_blas_sgemm2(blas_in, kernel, NULL, dst);
    wtk_heap_delete( heap);
}

/* 
conv1d strip 1 pad  量化运算
量化为了保证精度只在运算中使用量化定点运算，一般量化运算只进行单层
*/
// void wtk_nn_conv1d2_matq8(wtk_matq8_t *kernel, int kernel_size, enum_conv padding, wtk_matf_t *input, wtk_matf_t *dst)
// {
//     wtk_heap_t *heap=wtk_heap_new(4096);
//     int k_row=kernel->row
//       , k_col=kernel->col
//       , i_row=input->row
//       , i_col=input->col
//       , out_row=dst->row
//       , zero_len
//       , i, s, e, h;

//     wtk_matq8_t *blas_in=wtk_matq8_heap_new( heap, out_row, k_col);
//     char *inp, *blas_inp;
//     size_t col_stlen=sizeof(char)*k_col;
//     int pod_data = 0;
//     int in_data = i_row * i_col;

//     // wtk_debug("%d %d %d\n",kernel_size,k_row,k_col);
//     if (kernel_size*i_col != k_col)
//     {
//         wtk_exit_debug("卷积核形状错误 (%d, %d) 请参考注释!\n", k_row, k_col);
//     }

//     s=0;
//     e=0;
//     h=kernel_size-1;
//     // en=i_row-h;
//     switch (padding)
//     {
//         case enum_conv_npod:
//             break;
//         case enum_conv_same:
//             s=h/2;
//             e=h-s;
//             break;
//         default:
//             wtk_exit_debug("未实现的卷积填充方式 vaild \n");
//             break;
//     }

//     wtk_matq8_t *inq8 = wtk_matq8_heap_new(heap,i_row,i_col);
//     wtk_mat_f32toi8_transfer(input,inq8);

//     blas_inp=blas_in->p;
//     inp=inq8->p;

//     for (i=0; i<out_row; ++i, blas_inp+=k_col)
//     {
//         if (s>i)
//         {// 头
//             zero_len=(s-i)*i_col;
//             pod_data = k_col-zero_len;
//             if(pod_data<in_data){
//                 memcpy(blas_inp+zero_len, inp, sizeof(char)*(k_col-zero_len));
//             }else{
//                 memcpy(blas_inp+zero_len, inp, sizeof(char)*(in_data));    //sys
//             }
//         } else if (i+e>=out_row)
//         {// 尾
//             zero_len=(i+e-out_row+1)*i_col;
//             // wtk_debug("i: %d kernel_size: %d i_row:  %d\n", i, kernel_size, i_row);
//             memcpy(blas_inp, inp, col_stlen-sizeof(float)*zero_len);
//             inp+=i_col;
//         } else {
//             zero_len=0;
//             if (i_row+s-i<kernel_size)
//             { wtk_exit_debug(" 未考虑到的cnn情况, input_row < kernel_size \n"); }
//             memcpy(blas_inp, inp, col_stlen);
//             inp+=i_col;
//         }
//         // wtk_debug("zero_len: %d i: %d \n", zero_len, i);
//     }
//     wtk_mer_blas_sgemm2(blas_in, kernel, NULL, dst);
//     wtk_heap_delete( heap);
// }

/*
分组卷积 在pytroch中是将 groups>1 做初始化
控制输入和输出之间的连接， group=1，输出是所有的输入的卷积；
group=2，此时相当于有并排的两个卷积层，每个卷积层计算输入通道的一半，
并且产生的输出是输出通道的一半，随后将这两个输出连接起来。
不进行pad建议在外做好填充
kernel 将kernel的个数做为维数
*/
int wtk_nn_conv1d_group(wtk_matf_t *kernel, int kernel_size,int groups, wtk_matf_t *input, wtk_matf_t *dst)
{
    int i = 0;
    if(input->col/groups != kernel->col/kernel_size){
        wtk_debug("conv1d group dim error\n");
        wtk_exit(1);
    }
    if(kernel->row%groups != 0){
        wtk_debug("kernel groupe size error\n");
        wtk_exit(1);
    }
    //暂时没有想到办法做成大矩阵相乘  将输入分为小的矩阵相乘
    wtk_matf_t *t_input = wtk_matf_new(input->col,input->row);
    wtk_matf_init_transpose(input,t_input);
    int kernel_gn = kernel->row/groups; //每个小kernel的个数  
    int t_in_gr = t_input->row/groups;    //input的每组的大小
    wtk_matf_t *s_input = wtk_matf_new(input->row,t_in_gr);
    wtk_matf_t t_s_input;
    t_s_input.col = t_input->col;
    t_s_input.row = t_in_gr;
    wtk_matf_t s_kernel; 
    s_kernel.col = kernel->col;
    s_kernel.row = kernel_gn;
    wtk_matf_t *s_dst = wtk_matf_new(dst->row,dst->col/groups);
    wtk_matf_t *t_dst = wtk_matf_new(dst->col,dst->row);
    float *t_dst_p = t_dst->p;
    for(i = 0; i < groups; ++i){
        t_s_input.p = t_input->p+t_input->col*t_in_gr*i;
        wtk_matf_init_transpose(&t_s_input,s_input);
        s_kernel.p = kernel->p+kernel->col*kernel_gn*i;
        wtk_nn_conv1d2(&s_kernel,kernel_size,enum_conv_npod,s_input,s_dst);
        memcpy(t_dst_p,s_dst->p,sizeof(float)*s_dst->row*s_dst->col);
        t_dst_p += s_dst->row*s_dst->col;
    }
    wtk_matf_init_transpose(t_dst,dst);
    wtk_matf_delete(t_dst);
    wtk_matf_delete(s_dst);
    wtk_matf_delete(s_input);
    wtk_matf_delete(t_input);
    return 0;
}

/*
0填充
默认in为转置好的矩阵
                                              0
实际上将in矩阵变为了 in 矩阵
                                              0
*/
int wtk_nn_constant_pad1d(wtk_matf_t *in,wtk_matf_t *out,int left,int right)
{
    int row_new = 0,col = 0,irow = 0;
    row_new = in->row+left+right;
    float *inp = NULL,*op = NULL;
    int len = 0;

    if(row_new != out->row || in->col != out->col){
        wtk_debug("pad dim is error\n");
        wtk_exit(1);
    }
    col = in->col;
    irow = in->row;
    inp = in->p;
    op = out->p;
    if(left > 0){
        memset(op,0,sizeof(float)*col*left);
        op += left*col;
    }
    if(left < 0){
        inp += col*abs(left);
        irow += left;
    }
    if(right < 0){
        len = (irow+right)*col;
    }else{
        len = irow * col;
    }
    memcpy(op,inp,sizeof(float)*len);
    op += len;
    if(right > 0){
        memset(op,0,sizeof(float)*col*right);
    }

    return 0;
}

inline float wk_nn_arr_max_inline(float **p, int len, int i)
{/* 二维数组纵向取最大值 */
    if (len > 2)
    {
        return max(p[len-1][i], wk_nn_arr_max_inline(p+1, len-1, i));
    }
    return max(p[1][i], p[0][i]);
}
void wtk_nn_max_pool1d(int pool_size, int stride, enum_conv padding, wtk_matf_t *in, wtk_matf_t *dst)
{
    float 
        **p = malloc(sizeof(float*)*pool_size),
        *inp,
        *dstp;
    int irow = in->row
      , icol = in->col
      , i, j, k, s, e, en, h;
    
    s = 0;
    h = pool_size - 1;
    en = irow - h;
    switch (padding)
    {
        case enum_conv_same:
            s = h / 2;
            inp = in->p;
            dstp = dst->p;
            for (i=s; i>0; --i, dstp+=icol)
            {/* 头部填充规则 */
                if (pool_size-s > 1)
                {
                    for (j=0; j<pool_size-s; ++j)
                    {
                        p[j] = inp + icol*j;
                    }
                    for (k=0; k<icol; ++k)
                    {
                        dstp[k] = wk_nn_arr_max_inline(p, pool_size-s, k);
                    }
                } else 
                {
                    memcpy(dstp, inp, sizeof(float)*icol);
                }
            }

            /* 尾部填充规则 */
            e = h - s;
            // wtk_debug("max_pool %d %d\n", e, en);
            inp = wtk_matf_at(in, en+e-1, 0);
            dstp = wtk_matf_at(dst, irow-1, 0);
            for (i=e; i>0; --i, dstp-=icol, inp-=icol)
            {
                if (pool_size-e > 1)
                {
                    for (j=0; j<pool_size-e; ++j)
                    {
                        p[j] = inp + icol*j;
                    }
                    for (k=0; k<icol; ++k)
                    {
                        dstp[k] = wk_nn_arr_max_inline(p, pool_size-e, k);
                    }
                } else 
                {
                    memcpy(dstp, inp, sizeof(float)*icol);
                }
            }
            break;
        case enum_conv_valid:
            wtk_debug("暂未实现的填充方式: valid\n");
            wtk_exit(1);
        default:
            wtk_debug("暂未实现的填充方式\n");
            wtk_exit(1);
            break;
    }
    inp = in->p;
    dstp = wtk_matf_at(dst, s, 0);
    for (i=0; i<en; ++i, inp+=icol, dstp+=icol)
    {
        for (j=0; j<pool_size; ++j)
        {
            p[j] = inp + icol*j;
        }
        for (k=0; k<icol; ++k)
        {
            dstp[k] = wk_nn_arr_max_inline(p, pool_size, k);
        }
    }
    wtk_free(p);
}




wtk_nn_lstm_t* wtk_nn_lstm_new(int is_forward, int input_col, int lstm_units, float forget_bias, float activation(float), float zoneout[])
{
    int len = 4*lstm_units;
    wtk_heap_t *heap = wtk_heap_new(4096);
    wtk_nn_lstm_t *cell = wtk_heap_malloc( heap, sizeof(wtk_nn_lstm_t));
    cell->heap = heap;
    cell->is_forward = is_forward;
    cell->forget_bias = forget_bias;
    cell->lstm_units = lstm_units;
    cell->is_zoneout = zoneout != NULL;
    if (cell->is_zoneout) { cell->zoneout_cell=zoneout[0],  cell->zoneout_output=zoneout[1]; }
    cell->kernel = wtk_matf_heap_new( heap, len, input_col + lstm_units);
    cell->bias = wtk_vecf_heap_new( heap, len);
    cell->lstm_in = wtk_matf_heap_new( heap, 1, input_col + lstm_units);
    cell->lstm_out = wtk_matf_heap_new( heap, 1, len);
    cell->prev_c = wtk_vecf_heap_new( heap, lstm_units);
    cell->prev_h = wtk_heap_malloc( heap, sizeof(wtk_vecf_t));
    cell->prev_h->p = cell->lstm_in->p + input_col;
    cell->prev_h->len = lstm_units;
    cell->new_c = wtk_vecf_heap_new( heap, lstm_units);
    cell->new_h = wtk_vecf_heap_new( heap, lstm_units);
    cell->i = wtk_heap_malloc( heap, sizeof(wtk_vecf_t));
    cell->j = wtk_heap_malloc( heap, sizeof(wtk_vecf_t));
    cell->f = wtk_heap_malloc( heap, sizeof(wtk_vecf_t));
    cell->o = wtk_heap_malloc( heap, sizeof(wtk_vecf_t));
    cell->i->len = cell->j->len = cell->f->len = cell->o->len = lstm_units;
    float *p = cell->lstm_out->p;
    cell->i->p = p;
    cell->j->p = p + lstm_units;
    cell->f->p = p + 2*lstm_units;
    cell->o->p = p + 3*lstm_units;
    cell->activation = activation==NULL? tanhf:activation;
    wtk_nn_lstm_reset(cell);

    return cell;
}
void wtk_nn_lstm_reset(wtk_nn_lstm_t *cell)
{
    wtk_vecf_zero(cell->prev_c);
    wtk_vecf_zero(cell->prev_h);
}
void wtk_nn_lstm_delete(wtk_nn_lstm_t *cell)
{
    wtk_heap_delete(cell->heap);
}
void wtk_nn_lstm(wtk_nn_lstm_t *cell, wtk_matf_t *input, wtk_matf_t *output)
{/* 
默认 bath_size = 1;
bath_size > 1 时, 每个batch都需要重置 prev_c, prev_h 为zero
shape [bath_size, seq_len, input_size]
lstm_matrix = np.dot( np.concatenate([input, prev_h]), fw_kernel) + fw_bias
// i = input_gate, j = new_input, f = forget_gate, o = output_gate
i, j, f, o = np.split(lstm_matrix, 4)
c = (sigmoid(f + _forget_bias) * prev_c + sigmoid(i) *np.tanh(j))
h = sigmoid(o) * np.tanh(c)

if (is_zoneout)
{
    c = (1 - zoneout_cell) * c + zoneout_cell * prev_c
    m = (1 - zoneout_output) * m + zoneout_output * prev_m
}
 */
    wtk_nn_lstm_batch(cell, 1, input, output, NULL, NULL, NULL, NULL);
    // int channel = input->col
    //   , dim = input->row
    //   , lstm_units = cell->lstm_units
    //   , is_forward = cell->is_forward
    //   , it_offset = is_forward? channel: -channel
    //   , ht_offset = is_forward? lstm_units: -lstm_units
    //   , t;
    // float 
    //     *it = is_forward? input->p: input->p+channel*(dim-1),
    //     *ht = is_forward? output->p: output->p+lstm_units*(dim-1);
    // wtk_matf_t 
    //     cell_in,
    //     cell_out;

    // cell_in.row = 1;
    // cell_in.col = channel;
    // cell_out.row = 1;
    // cell_out.col = lstm_units;

    // for (t=0; t<dim; ++t, it+=it_offset, ht+=ht_offset)
    // {
    //     cell_in.p = it;
    //     cell_out.p = ht;
    //     wtk_nn_lstm_cell(cell, &cell_in, &cell_out);
    // }
}
void wtk_nn_lstm_batch(wtk_nn_lstm_t *cell, int batch_size, wtk_matf_t *input, wtk_matf_t *output, wtk_matf_t *c_inmf, wtk_matf_t *h_inmf, wtk_matf_t *c_outmf, wtk_matf_t *h_outmf)
{
    wtk_nn_lstm_batch2(cell, batch_size, NULL, input, output, c_inmf, h_inmf, c_outmf, h_outmf);
}
void wtk_nn_lstm_batch2(wtk_nn_lstm_t *cell, int batch_size, int *dim_arr, wtk_matf_t *input, wtk_matf_t *output, wtk_matf_t *c_inmf, wtk_matf_t *h_inmf, wtk_matf_t *c_outmf, wtk_matf_t *h_outmf)
{/* 
bath_size > 1 时, 每个batch都需要重置 prev_c, prev_h 为zero
sum(dim_arr) == input->row
shape [bath_size, seq_len, input_size]
lstm_matrix = np.dot( np.concatenate([input, prev_h]), fw_kernel) + fw_bias
// i = input_gate, j = new_input, f = forget_gate, o = output_gate
i, j, f, o = np.split(lstm_matrix, 4)
c = (sigmoid(f + _forget_bias) * prev_c + sigmoid(i) *np.tanh(j))
h = sigmoid(o) * np.tanh(c)

if (is_zoneout)
{
    c = (1 - zoneout_cell) * c + zoneout_cell * prev_c
    m = (1 - zoneout_output) * m + zoneout_output * prev_m
}
 */
    //input (23,512)
    int channel = input->col
      , cur_dim=0
      , dim = input->row / batch_size
      , lstm_units = cell->lstm_units
      , is_forward = cell->is_forward
      , it_offset = is_forward? channel: -channel
      , ht_offset = is_forward? lstm_units: -lstm_units
      , i, t;
    float
        // *p, 
        *it=is_forward?input->p: input->p+channel*(dim-1),
        *ht=NULL;
    size_t hidden_stlen=sizeof(float)*lstm_units;
    wtk_matf_t 
        cell_in,
        cell_out;
    if (output)
    {
        ht=is_forward? output->p: output->p+lstm_units*(dim-1);
    }

    if (!dim_arr)
    { assert(dim*batch_size == input->row); }

    cell_in.row = 1;
    cell_in.col = channel;
    cell_out.row = 1;
    cell_out.col = lstm_units;

    for (i=0; i<batch_size; ++i, cur_dim+=dim)
    {
        if (c_inmf)
        { memcpy( cell->prev_c->p, wtk_matf_at(c_inmf, i, 0), hidden_stlen); }
        if (h_inmf)
        { memcpy( cell->prev_h->p, wtk_matf_at(h_inmf, i, 0), hidden_stlen); }

        if (dim_arr)
        {
            dim=dim_arr[i];
            if (!is_forward)
            {
                it= input->p+channel*(cur_dim+dim-1);
                ht= output? output->p+lstm_units*(cur_dim+dim-1): NULL;
            }
        }
        for (t=0; t<dim; ++t, it+=it_offset)
        {
            cell_in.p = it;
            cell_out.p = ht;
            wtk_nn_lstm_cell(cell, &cell_in, &cell_out);
            if (ht)
            { ht+=ht_offset; }
        }

        if (c_outmf)
        { memcpy( wtk_matf_at(c_outmf, i, 0), cell->new_c->p, hidden_stlen); }
        if (h_outmf)
        { memcpy( wtk_matf_at(h_outmf, i, 0), cell->new_h->p, hidden_stlen); }
        wtk_nn_lstm_reset(cell);
    }
}

/* 
input.shape = [1, ?]
output.shape = [1, lstm_units]

lstm_matrix = np.dot( np.concatenate([input, prev_h]), fw_kernel) + fw_bias
// i = input_gate, j = new_input, f = forget_gate, o = output_gate
i, j, f, o = np.split(lstm_matrix, 4)
pytorch 下 结构顺序是 i, f, g, o
c = (sigmoid(f + _forget_bias) * prev_c + sigmoid(i) *np.tanh(j))
h = sigmoid(o) * np.tanh(c)

if (is_zoneout)
{
    c = (1 - zoneout_cell) * c + zoneout_cell * prev_c
    m = (1 - zoneout_output) * m + zoneout_output * prev_m
}
*/  
void wtk_nn_lstm_cell(wtk_nn_lstm_t *cell, wtk_matf_t *input, wtk_matf_t *output)
{
    float (*activation)(float); 
    activation = cell->activation;
    int channel = input->col
      , lstm_units = cell->lstm_units
    //   , is_zoneout = cell->is_zoneout
      , k;
    float 
        *it = input->p,
        *ht = output->p,
        // forget_bias = cell->forget_bias,
        *prev_c = cell->prev_c->p,
        // *prev_h = cell->prev_h->p,
        *new_c = cell->new_c->p,
        *new_h = cell->new_h->p,
        *i = cell->i->p,
        *j = cell->j->p,
        *f = cell->f->p,
        *o = cell->o->p;
        // zoneout_cell = cell->zoneout_cell,
        // zoneout_output = cell->zoneout_output,
        // zone_c = 1 - zoneout_cell,
        // zone_o = 1 - zoneout_output;
    wtk_matf_t 
        *kernel = cell->kernel, // is transposed
        *lstm_in = cell->lstm_in,
        *lstm_out = cell->lstm_out;
    wtk_vecf_t 
        *bias = cell->bias;

    memcpy(lstm_in->p, it, sizeof(float)*channel);
    wtk_mer_blas_sgemm2(lstm_in, kernel, bias, lstm_out);
    //sys
    wtk_nn_sigmoid(i,lstm_units);
    wtk_nn_sigmoid(f,lstm_units*2); //因为f和o是同一块内存 同时算
    // wtk_nn_sigmoid(o,lstm_units);
    for (k=0; k<lstm_units; ++k)
    {
        // new_c[k] = wtk_nn_sigmoid_inline(f[k]+forget_bias) * prev_c[k] + wtk_nn_sigmoid_inline(i[k]) * activation(j[k]);
        // new_h[k] = wtk_nn_sigmoid_inline(o[k]) * activation(new_c[k]);
        //sys
        new_c[k] = f[k] * prev_c[k] + i[k] * activation(j[k]);
        new_h[k] = o[k] * activation(new_c[k]);

        if (ht)
        { ht[k] = new_h[k]; }
        // printf("%f ", ht[k]);
        
        //is use in train
        // if (is_zoneout)
        // {
        //     new_c[k] = zone_c*new_c[k] + zoneout_cell*prev_c[k];
        //     new_h[k] = zone_o*new_h[k] + zoneout_output*prev_h[k];
        //     // printf("%f ", new_h[k]);
        // }
    }
    wtk_vecf_cpy(cell->prev_c, cell->new_c);
    wtk_vecf_cpy(cell->prev_h, cell->new_h);
}
void wtk_nn_lstm_multi_cell(wtk_nn_lstm_t **cell, int layer_len, wtk_matf_t *cell_in, wtk_matf_t *cell_out)
{
    int i;

    for (i=0; i<layer_len; ++i)
    {
        wtk_mer_matf_shape_print(cell_in);
        wtk_nn_lstm_cell(cell[i], cell_in, cell_out);
        cell_in = cell_out;
    }

}

/* ------------------------ GRU ------------------------ */
wtk_nn_rnngru_t* wtk_nn_rnngru_new(int is_forward, int incol, int num_units, void activation(float*, int))
{
    return wtk_nn_rnngru_new2( wtk_nn_enum_type_tensorflow, is_forward, incol, num_units, activation);
};
wtk_nn_rnngru_t* wtk_nn_rnngru_new2( wtk_nn_enum_type_t type, int is_forward, int incol, int num_units, void activation(float*, int))
{
    wtk_heap_t *heap = wtk_heap_new( 4096);
    wtk_nn_rnngru_t *cell=wtk_heap_malloc( heap, sizeof(*cell));
    int gate_col = 2*num_units;

    cell->heap = heap;
    cell->type=type;
    cell->num_units = num_units;
    cell->is_forward = is_forward;
    cell->gate_in = wtk_matf_heap_new( heap, 1, incol+num_units);
    cell->gate_out = wtk_matf_heap_new( heap, 1, gate_col);
    cell->gate_kernel = wtk_matf_heap_new( heap, gate_col, incol + num_units);
    cell->gate_bias = wtk_vecf_heap_new( heap, gate_col);
    cell->candidate_in = wtk_matf_heap_new( heap, 1, incol+num_units);
    cell->candidate_out = wtk_matf_heap_new( heap, 1, num_units);
    cell->candidate_out_hh=NULL;
    cell->candidate_kernel=NULL;
    cell->candidate_kernel_hh=NULL;
    cell->candidate_bias = wtk_vecf_heap_new( heap, num_units);
    cell->candidate_bias_hh=NULL;
    cell->r = wtk_heap_malloc( heap, sizeof(wtk_vecf_t));
    cell->u = wtk_heap_malloc( heap, sizeof(wtk_vecf_t));
    cell->prev_h = wtk_heap_malloc( heap, sizeof(wtk_vecf_t));
    cell->new_h = wtk_heap_malloc( heap, sizeof(wtk_vecf_t));
    cell->r_state = wtk_heap_malloc( heap, sizeof(wtk_vecf_t));
    cell->r_state->p = cell->candidate_in->p + incol;
    cell->r_state->len = num_units;
    cell->prev_h->p = cell->gate_in->p + incol;
    cell->prev_h->len = num_units;
    cell->new_h = cell->prev_h;
    cell->r->p = cell->gate_out->p;
    cell->u->p = cell->gate_out->p + num_units; // 内存复用
    cell->r->len = cell->u->len = num_units;
    cell->activation = activation==NULL? wtk_nn_tanh : activation;

    switch (type)
    {
    case wtk_nn_enum_type_tensorflow:
        cell->candidate_kernel = wtk_matf_heap_new( heap, num_units, incol + num_units);
        break;
    case wtk_nn_enum_type_pytorch:
        cell->candidate_out_hh=wtk_matf_heap_new( heap, 1, num_units);
        cell->candidate_bias_hh=wtk_vecf_heap_new( cell->heap, cell->num_units);
        cell->candidate_kernel = wtk_matf_heap_new( heap, num_units, incol);
        cell->candidate_kernel_hh = wtk_matf_heap_new( heap, num_units, num_units);

        cell->weight_ih=wtk_matf_heap_new( heap, 3*num_units, incol);
        cell->weight_hh=wtk_matf_heap_new( heap, 3*num_units, num_units);
        cell->bias_ih=wtk_vecf_heap_new( heap, 3*num_units);
        cell->bias_hh=wtk_vecf_heap_new( heap, 3*num_units);

        break;
    default:
        assert(0);
        break;
    }
    return cell;
}
void wtk_nn_rnngru(wtk_nn_rnngru_t *cell, wtk_matf_t *in, wtk_matf_t *out)
{
    int is_forward = cell->is_forward
      , dim = in->row
      , channel = in->col
      , num_units = cell->num_units
      , it_offset = is_forward? channel: -channel
      , ht_offset = is_forward? num_units: -num_units
      , i;
    float 
        *it = is_forward? in->p: in->p+channel*(dim-1),
        *ht = is_forward? out->p: out->p+num_units*(dim-1);
    wtk_matf_t cell_in, cell_out;
    cell_in.row = cell_out.row = 1;
    cell_in.col = channel;
    cell_out.col = num_units;

    // wtk_debug("r: %d c: %d \n", in->row, in->col);
    // wtk_exit(1);
    for (i=0; i<dim; ++i, it+=it_offset, ht+=ht_offset)
    {
        cell_in.p = it;
        cell_out.p = ht;
        wtk_nn_rnngru_cell(cell, &cell_in, &cell_out);
    }
}
void wtk_nn_rnngru_batch(wtk_nn_rnngru_t *cell, int batch_size, wtk_matf_t *in, wtk_matf_t *out, wtk_matf_t *h_inmf, wtk_matf_t *h_outmf)
{
    int is_forward = cell->is_forward
      , dim = in->row/batch_size
      , channel = in->col
      , num_units = cell->num_units
      , it_offset = is_forward? channel: -channel
      , ht_offset = is_forward? num_units: -num_units
      , i
      , j;
    float 
        *it = is_forward? in->p: in->p+channel*(dim-1),
        *ht = is_forward? out->p: out->p+num_units*(dim-1);
    size_t units_stlen=sizeof(float)*num_units;
    wtk_matf_t cell_in, cell_out;
    cell_in.row = cell_out.row = 1;
    cell_in.col = channel;
    cell_out.col = num_units;

    for (j=0; j<batch_size; ++j)
    {
        if (h_inmf)
        {
            memcpy(cell->prev_h->p, wtk_matf_at(h_inmf, j, 0), units_stlen);
        }

        for (i=0; i<dim; ++i, it+=it_offset, ht+=ht_offset)
        {
            cell_in.p = it;
            cell_out.p = ht;
            wtk_nn_rnngru_cell(cell, &cell_in, &cell_out);
        }

        if (h_outmf)
        {
            memcpy(wtk_matf_at(h_outmf, j, 0), cell->new_h->p, units_stlen);
        }
        wtk_nn_rnngru_reset(cell);
    }
}
void wtk_nn_rnngru_cell(wtk_nn_rnngru_t *cell, wtk_matf_t *in, wtk_matf_t *out)
{/* 
tensorflow 规则
r == reset_gate
u == update_gate
gin = concat( in, state)
gout = sigmoid( gin*gkernel + gbias)
r,u = gout.split(2)
r_state = r * state
cin = concat( in, r_state)
cout = activation( cin * ckernel + cbias)
new_state = u * state + (1-u)*cout

pytorch 规则
xw = x*w_ih + bias_ih
hw = h_prev*w_hh + bias_hh
r = sigmod( xw+hw)
u = sigmod( xw+hw)
cell_gate = tanh( xw + hw*reset_gate )
new_state = u * h_prev + (1-u)*cell_gate
 */
    int num_units = cell->num_units
      , incol=in->col
      , i;
    wtk_nn_enum_type_t type=cell->type;
    wtk_matf_t
        *gin = cell->gate_in,
        *gout = cell->gate_out,
        *cin = cell->candidate_in,
        *cout = cell->candidate_out,
        *cout2 = cell->candidate_out_hh,
        cin1,
        cin2;
    wtk_vecf_t
        *r = cell->r,
        *u = cell->u,
        *prev_h = cell->prev_h,
        *r_state = cell->r_state;
    size_t 
        // unit_stlen=sizeof(float)*num_units,
        incol_stlen=sizeof(float)*incol;
    float 
        *fi = in->p,
        *fgi = gin->p,
        *fci = cin->p,
        *fco = cout->p,
        *fr = r->p,
        *fu = u->p,
        *fs = prev_h->p,
        *fnew_h = cell->new_h->p;

    memcpy(fgi, fi, incol_stlen);
    memcpy(fci, fi, incol_stlen);
    wtk_nn_layer_dense( gin, cell->gate_kernel, cell->gate_bias, wtk_nn_sigmoid, gout);
    switch (type)
    {
    case wtk_nn_enum_type_tensorflow:
        wtk_float_mult( fr, fs, r_state->p, num_units);
        wtk_nn_layer_dense( cin, cell->candidate_kernel, cell->candidate_bias, cell->activation, cout);
        break;
    case wtk_nn_enum_type_pytorch:
        assert(cin->row == 1);
        cin1.row=cin2.row=1;
        cin1.p=cin->p;
        cin1.col=incol;
        cin2.p=prev_h->p;
        cin2.col=num_units;
        wtk_mer_blas_sgemm2( &cin1, cell->candidate_kernel, cell->candidate_bias, cout);
        // wtk_mer_matf_write_file(cout, "output/cout.nn.c.txt", 0);
        // wtk_exit(1);
        wtk_mer_blas_sgemm2( &cin2, cell->candidate_kernel_hh, cell->candidate_bias_hh, cout2);
        wtk_matf_vecf_multi( cout2, cell->r);
        wtk_matf_add2(cout, cout2);
        cell->activation( fco, num_units);
        // print_float2(cout1->p, 10);
        break;
    default:
        wtk_debug("rnn gru 未知算法类型 [%d] \n", type);
        wtk_exit(1);
        break;
    }
    
    for (i=0; i<num_units; ++i)
    {
        fnew_h[i] = fu[i] * fs[i] + (1 - fu[i]) * fco[i];
    }
    memcpy(out->p, fnew_h, sizeof(float)*num_units);
    // wtk_mer_matf_write_file(out, "output/cout.nn.c.txt", 0);
    // wtk_exit(1);
};
void wtk_nn_rnngru_reset(wtk_nn_rnngru_t *cell)
{
    int is = !cell->candidate_bias_hh || !cell->candidate_kernel_hh;
    if (cell->type == wtk_nn_enum_type_pytorch && is)
    { wtk_debug("rnngru pytorch： 缺少 candidate_bias_hh, 详情查看公式差异 \n"); wtk_exit(1);}
    wtk_vecf_zero(cell->prev_h);
}
void wtk_nn_rnngru_delete(wtk_nn_rnngru_t *cell)
{
    wtk_heap_delete(cell->heap);
}
