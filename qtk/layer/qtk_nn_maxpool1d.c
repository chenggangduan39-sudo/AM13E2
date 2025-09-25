#include "qtk_nn_maxpool1d.h"

qtk_nn_maxpool1d_t* qtk_nn_maxpool1d_new(int kernel_size,int stride,int padding)
{
    qtk_nn_maxpool1d_t *pool = NULL;
    pool = wtk_malloc(sizeof(*pool));
    pool->kernel_size = kernel_size;
    pool->stride = stride;
    pool->padding = padding;
    return pool;
}

#define MAX_VAL(v,n) ({int _ui = 0;\
                                                float _max = (v)[0];\
                                                for(_ui = 1; _ui < n; ++_ui)\
                                                {if((v)[_ui] > _max) _max = (v)[_ui];}\
                                                _max;\
                                                })

#include <float.h>
//填充使用负无穷 使用 -1e10 代替
int qtk_nn_maxpool1d_forward(qtk_nn_maxpool1d_t *pool,wtk_matf_t *in,wtk_matf_t *out)
{
    int padding = pool->padding;
    int stride = pool->stride;
    int kernel_size = pool->kernel_size;
    int i_row = in->row;
    int i_col = in->col;
    int i = 0,k = 0;
    int padd = (i_row+padding*2-kernel_size)/stride+1;
    wtk_matf_t *pad = wtk_matf_new(i_row+padding*2,i_col);
    memcpy(pad->p+padding*i_col,in->p,in->col*in->row*sizeof(float));

    for(i = 0; i < pad->col; ++i){
        pad->p[i] = -1e10;
        pad->p[pad->col*(pad->row-1)+i] = -1e10;
    }

    wtk_matf_t *tmp = wtk_matf_transpose(pad);
    float *op = out->p;
    float *tp = tmp->p;
    int t_col = tmp->col;

    for(k = 0;k < i_col; ++k){
        for(i = 0; i < padd; ++i){  //默认stride为1
            // *(op+i*i_col+k) = MAX_VAL(tp+k*t_col+i,kernel_size);
            *(op+i*i_col+k) = wtk_float_max(tp+k*t_col+i,kernel_size);
        }
    }
    // wtk_matf_print(out);
    wtk_matf_delete(tmp);
    wtk_matf_delete(pad);
    return 0;
}

int qtk_nn_maxpool1d_delete(qtk_nn_maxpool1d_t *pool)
{
    wtk_free(pool);
    return 0;
}

int qtk_nn_maxpool1d_out_row(qtk_nn_maxpool1d_t *pool,int in_row)
{
    return (in_row+pool->padding*2-pool->kernel_size)/pool->stride+1;
}
