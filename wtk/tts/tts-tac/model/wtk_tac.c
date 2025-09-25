#include "wtk_tac.h"

/*
wtk_nn_conv1d()
if (is_after) activation()
wtk_nn_batch_norm( gamma, beta, moving_mean, moving_variance)
if (is_before) activation()
if (is_training) drop_out()
 */
void wtk_tac_conv1d(wtk_matf_t *kernel, int kernel_size, wtk_vecf_t *bias, wtk_vecf_t *gamma, wtk_vecf_t *beta, wtk_vecf_t *moving_mean, wtk_vecf_t *moving_variance, float epsilon, void activation(float*, int), int is_after, wtk_matf_t *in, wtk_matf_t *dst)
{
    int is_before = !is_after
      , dst_len = dst->row*dst->col;
    wtk_nn_conv1d2( kernel, kernel_size, enum_conv_same, in, dst);

    if (bias)
    { wtk_matf_vecf_add( dst, bias); }
    if (is_after && activation)
    { activation(dst->p, dst_len); }
    wtk_nn_batch_norm(dst, gamma, beta, moving_mean, moving_variance, epsilon);
    if (is_before && activation)
    { activation(dst->p, dst_len); }
}
//主要是不进行填充
void wtk_tac_conv1d2(wtk_matf_t *kernel, int kernel_size, wtk_vecf_t *bias, wtk_vecf_t *gamma, wtk_vecf_t *beta, wtk_vecf_t *moving_mean, wtk_vecf_t *moving_variance, float epsilon, void activation(float*, int), int is_after, wtk_matf_t *in, wtk_matf_t *dst)
{
    int is_before = !is_after
      , dst_len = dst->row*dst->col;
    wtk_nn_conv1d2( kernel, kernel_size, enum_conv_npod, in, dst);

    if (bias)
    { wtk_matf_vecf_add( dst, bias); }
    if (is_after && activation)
    { activation(dst->p, dst_len); }
    wtk_nn_batch_norm(dst, gamma, beta, moving_mean, moving_variance, epsilon);
    if (is_before && activation)
    { activation(dst->p, dst_len); }
}
//将卷积核和batch_norm结合在一起 节省 conv  运算
void wtk_tac_conv1d3(wtk_matf_t *kernel, int kernel_size, wtk_vecf_t *bias, void activation(float*, int), wtk_matf_t *in, wtk_matf_t *dst)
{
    int dst_len = dst->row*dst->col;
    wtk_nn_conv1d2( kernel, kernel_size, enum_conv_same, in, dst);

    if (bias)
    { wtk_matf_vecf_add( dst, bias); }
    if(activation)
    { activation(dst->p, dst_len); }
}