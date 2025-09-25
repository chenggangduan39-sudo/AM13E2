#include "wtk_mer_dnn.h"

//static void matf2fix(wtk_matf_t *in, int is_tofix)
//{
//    #define FIX_TYPE int
//    #define FIX (1<<10)
//    #define FTOI(x) ((FIX_TYPE)((x)*FIX))
//    #define ITOF(x) ((float)((x)*1.0/FIX))
//
//    int alen=in->row*in->col
//      , i;
//
//    if (is_tofix)
//    {
//        float *a = in->p;
//        int *pf1 = (int*)a;
//        for (i=0; i<alen-4; i+=4)
//        {
//            // printf("%f ", a[i]);
//            pf1[i] = FTOI(a[i]);
//            pf1[i+1] = FTOI(a[i+1]);
//            pf1[i+2] = FTOI(a[i+2]);
//            pf1[i+3] = FTOI(a[i+3]);
//            // printf("%d ", pf1[i]);
//        }
//        for (; i<alen; ++i)
//        {
//            pf1[i] = FTOI(a[i]);
//        }
//    } else
//    {/* tofloat */
//        int *a = (int*)in->p;
//        float *pf1 = in->p;
//        for (i=0; i<alen-4; i+=4)
//        {
//            pf1[i] = ITOF(a[i]);
//            pf1[i+1] = ITOF(a[i+1]);
//            pf1[i+2] = ITOF(a[i+2]);
//            pf1[i+3] = ITOF(a[i+3]);
//            // printf("%f ", pf1[i] );
//        }
//        for (; i<alen; ++i)
//        {
//            pf1[i] = ITOF(a[i]);
//        }
//    }
//
//}

wtk_matf_t* wtk_mer_dnn_model(wtk_matf_t *in_x, int layer_size, wtk_string_t **layer_type, int *layer_num, wtk_matf_t **w_arr, wtk_vecf_t **b_arr, const int n_final, char *mname)
{
    /* 
         ：param n_in：输入要素的维数
         ：type in：整数
         ：param layer_type_size：每个隐藏层的图层大小
         ：type layer_type_size：整数列表
         ：param n_out：输出要素的维数
         ：type n_out：Integrer
         ：param layer_type_type：每个隐藏层的激活类型，例如TANH，LSTM，GRU，BLSTM
         ：param L1_reg：L1调节权重
         ：param L2_reg：L2调节权重
         ：param output_type：输出图层的激活类型，默认为'LINEAR'，线性回归。
         ：param dropout_rate：丢失的概率，0到1之间的浮点数。
    输入 in_x
    in xcol out 512
    in 512 out 512
    in 512 out 512
    in 512 out 512
    in 512 out n_final
    dst = xrow * n_final

    self.output = T.dot(self.x, self.W) + self.b
    self.output = T.tanh(self.output)
    self.params = [self.W, self.b]

    ((tanh(((tanh(((tanh(((tanh(((x \dot W) + b)) \dot W) + b)) \dot W) + b)) \dot W) + b)) \dot W) + b)
     */
    // int layer_size = 5
    //   , layer_n = 512
    //   , layer_num[] = {layer_n, layer_n, layer_n, layer_n, layer_n}
    //   , i;
    // char *layer_type[] = {"TANH", "TANH", "TANH", "TANH", "LINEAR"};
    // char w_fn[50], b_fn[50];
    // int n_in
    //   , n_out;
    int layer_n = layer_num[0]
      , i;
    wtk_heap_t *heap = wtk_heap_new(4096);
    wtk_matf_t 
        *layer_in,
        *layer_out,
        *layer_tmp;

    // matf2fix(in_x, 1);
    // matf2fix(in_x, 0);
    layer_in = wtk_matf_heap_new(heap, in_x->row, layer_n);
    layer_out = wtk_matf_heap_new(heap, in_x->row, layer_n);

    int last_i = layer_size-1;
    for (i=0; i<layer_size; ++i)
    {
        if (i==last_i)
        { 
            layer_out = wtk_matf_heap_new(heap, in_x->row, n_final);
        }
        // wtk_debug("      [%d] dnn 计算 (%d, %d) * (%d,%d) \n", i, i==0?in_x->row:layer_in->row, i==0?in_x->col:layer_in->col, w_arr[i]->row, w_arr[i]->col);

        wtk_nn_layer_dense(i==0? in_x: layer_in, w_arr[i], b_arr[i], i==last_i?NULL: wtk_float_tanh, layer_out);
        // matf2fix(layer_out, 0);
        // wtk_mer_matf_write_file( layer_out, "output/matf.txt", 0);
        // wtk_exit(1);
        
        if (i==last_i) {break;}
        layer_tmp = layer_in;
        layer_in = layer_out;
        layer_out = layer_tmp;
        // wtk_matf_cpy(layer_out, layer_in);
    }
    // matf2fix(layer_out, 0);
    // wtk_mer_matf_write_file( layer_out, "output/matf.txt", 0);

    wtk_matf_t* ret_matf = wtk_matf_row_slice(layer_out, 0, layer_out->row);

    wtk_heap_delete(heap);
    return ret_matf;
}
