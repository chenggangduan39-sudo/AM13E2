#include "wtk_nn_sparse.h"
#include "wtk/tts-mer/wtk-extend/wtk_mat.h"
#include "wtk/tts-mer/wtk-extend/wtk_heap.h"
#include "wtk/tts-mer/wtk-extend/wtk_sse.h"

/* 通过原始矩阵生成稀疏矩阵
w_t 已转置
因为矩阵稀疏度不确定, 所以内存申请按完整矩阵来
 */
wtk_nn_matf_sparse_t *wtk_nn_matf_sparse_heap_new( wtk_heap_t *heap, wtk_matf_t *w_t)
{
    wtk_matf_t *w=wtk_matf_heap_new( heap, w_t->col, w_t->row);
    wtk_matf_init_transpose( w_t, w);
    wtk_nn_matf_sparse_t *smf=wtk_heap_malloc( heap, sizeof(*smf));
    int row=w->row
      , col=w->col
      , *idx=wtk_heap_malloc( heap, sizeof(int)*row*(col/16)+row)
      , *pi=idx
      , *pid=idx
      , count
      , size16=0
      , step=16
      , ci // 纵坐标
      , i
      , j;
    size_t step_stlen=sizeof(float)*step;
    float
        *weight=wtk_heap_malloc( heap, sizeof(float)*row*col),
        *weight_diag=wtk_heap_malloc( heap, sizeof(float)*col);
    float
        *ps,
        *pcur,
        *pw,
        *pdiag=weight_diag;

    assert( col !=0 && col %16 == 0);

    pw=w->p;
    for (i=0; i<col/row; ++i)
    {
        ci=i*row;
        for (j=0; j<row; ++j)
        {
            pdiag[0]=pw[j*col + ci+j];
            pw[j*col + ci+j]=0;
            pdiag++;
        }
    }

    ps=w->p;
    pw=weight;
    for (i=0; i<col/16; ++i, ps+=step)
    {
        pcur=ps;
        count=0;
        pi=pid;
        pid+=1;
        for (j=0; j<row; ++j, pcur+=col)
        {
            if (pcur[0] == 0 && pcur[1] == 0)
            { continue; }
            memcpy(pw, pcur, step_stlen);
            pw+=step;
            count++;
            pid[0]=j;
            pid+=1;
        }
        pi[0]=count;
        size16+=count;
    }

    smf->weight=weight;
    smf->weight_diag=weight_diag;
    smf->idx=idx;
    smf->cols=col;
    smf->size16=size16;
    // print_int(idx, 500);
    return smf;
}

#ifdef __AVX__
static void sparse_sgemv_accum16(float *out, const float *w, int rows, const int *idx, const float *x)
{
   int i, j;
   float *y;
   for (i=0;i<rows;i+=16)
   {
        int cols;
        cols = *idx++;
        y = &out[i];
        sse256_float32_t y0=sse256_loadu(y);
        sse256_float32_t y1=sse256_loadu(y+8);
        for (j=0;j<cols;j++)
        {
            
            float xj;
            xj = x[*idx++];
            sse256_float32_t vxj=sse256_set(&xj);
            sse256_float32_t w0=sse256_loadu(w);
            sse256_float32_t w1=sse256_loadu(w+8);
            y0=sse256_add(y0, sse256_mul(w0, vxj));
            y1=sse256_add(y1, sse256_mul(w1, vxj));
            w += 16;
        }
        sse256_storeu(y, y0);
        sse256_storeu(y+8, y1);
   }
}
#elif defined(__SSE__)
static void sparse_sgemv_accum16(float *out, const float *w, int rows, const int *idx, const float *x)
{
   int i, j;
   float *y;
   for (i=0;i<rows;i+=16)
   {
        int cols;
        cols = *idx++;
        y = &out[i];
        sse_float32_t y0=sse_loadu(y);
        sse_float32_t y1=sse_loadu(y+4);
        sse_float32_t y2=sse_loadu(y+8);
        sse_float32_t y3=sse_loadu(y+12);
        for (j=0;j<cols;j++)
        {
            
            float xj;
            xj = x[*idx++];
            sse_float32_t vxj=sse_set(xj);
            sse_float32_t w0=sse_loadu(w);
            sse_float32_t w1=sse_loadu(w+4);
            sse_float32_t w2=sse_loadu(w+8);
            sse_float32_t w3=sse_loadu(w+12);
            y0=sse_add(y0, sse_mul(w0, vxj));
            y1=sse_add(y1, sse_mul(w1, vxj));
            y2=sse_add(y2, sse_mul(w2, vxj));
            y3=sse_add(y3, sse_mul(w3, vxj));
            w += 16;
        }
        sse_storeu(y, y0);
        sse_storeu(y+4, y1);
        sse_storeu(y+8, y2);
        sse_storeu(y+12, y3);
   }
}
#else
static void sparse_sgemv_accum16(float *out, const float *w, int rows, const int *idx, const float *x)
{
   int i, j;
   float *y;
   for (i=0;i<rows;i+=16)
   {
      int cols;
      cols = *idx++;
      y = &out[i];
      for (j=0;j<cols;j++)
      {
        float xj;
        xj = x[*idx++];
        y[0] += w[0]*xj;
        y[1] += w[1]*xj;
        y[2] += w[2]*xj;
        y[3] += w[3]*xj;
        y[4] += w[4]*xj;
        y[5] += w[5]*xj;
        y[6] += w[6]*xj;
        y[7] += w[7]*xj;
        y[8] += w[8]*xj;
        y[9] += w[9]*xj;
        y[10] += w[10]*xj;
        y[11] += w[11]*xj;
        y[12] += w[12]*xj;
        y[13] += w[13]*xj;
        y[14] += w[14]*xj;
        y[15] += w[15]*xj;
        w += 16;
      }
   }
}
#endif

void wtk_nn_matf_sparse_sgemm( wtk_nn_matf_sparse_t *kernel, wtk_matf_t *in, wtk_matf_t *out)
{/* 稀疏矩阵乘法,效果非常好,基本没有稀疏块指针跳转损耗,相当于同等有效数据量下矩阵乘法 */
    int i
      , j
      , col_in=in->col
      , col_out=out->col;
    float 
        *pin=in->p,
        *pout=out->p;

    assert( col_in < col_out || col_out%col_in != 0);

    for (i=0; i<in->row; ++i, pout+=col_out, pin+=col_in)
    {
        for (j=0; j<col_out/col_in; ++j)
        {
            wtk_float_mult( kernel->weight_diag + j*col_in, pin, pout + j*col_in, col_in);
        }
        
        sparse_sgemv_accum16(pout, kernel->weight, kernel->cols, kernel->idx, pin);
    }
}
