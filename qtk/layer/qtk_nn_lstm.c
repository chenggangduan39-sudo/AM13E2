#include "qtk_nn_lstm.h"
#include "tts-mer/wtk-extend/wtk_heap2.h"
#include "tts-mer/wtk-extend/wtk_mat2.h"
#include "tts-mer/wtk-extend/nn/wtk_nn_activation.h"
#include "tts-mer/wtk-extend/wtk_mer_source_file.h"

#ifdef __AVX__
    #include <immintrin.h>
    #define sse256_float32_t __m256
    #define sse256_set _mm256_broadcast_ss
    #define sse256_mul _mm256_mul_ps
    #define sse256_add _mm256_add_ps
    #define sse256_loadu _mm256_loadu_ps
    #define sse256_load _mm256_load_ps
    #define sse256_storeu _mm256_storeu_ps
#endif
#if  defined(__SSE__)
    #include <xmmintrin.h>
    #define sse_float32_t __m128
    #define sse_set _mm_set1_ps
    #define sse_mul _mm_mul_ps
    #define sse_add _mm_add_ps
    #define sse_loadu _mm_loadu_ps
    #define sse_load _mm_load_ps
    #define sse_storeu _mm_storeu_ps
#endif

void qtk_lstm_matf_mul_trans(wtk_matf_t *in,wtk_matf_t *kernel, wtk_matf_t *out);

qtk_nn_lstm_t* qtk_nn_lstm_new(int input_col, int lstm_units)
{
    int len = 4*lstm_units;
    wtk_heap_t *heap = wtk_heap_new(4096);
    qtk_nn_lstm_t *cell = wtk_heap_malloc( heap, sizeof(qtk_nn_lstm_t));
    cell->heap = heap;
    cell->is_forward = 1;
    cell->lstm_units = lstm_units;
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
    qtk_nn_lstm_reset(cell);

    return cell;
}

int qtk_nn_lstm_delete(qtk_nn_lstm_t *lstm)
{
    wtk_heap_delete(lstm->heap);
    return 0;
}

int qtk_nn_lstm_reset(qtk_nn_lstm_t *lstm)
{
    wtk_vecf_zero(lstm->prev_c);
    wtk_vecf_zero(lstm->prev_h);
    return 0;
}

int qtk_nn_lstm_forward(qtk_nn_lstm_t *cell,wtk_matf_t *input,wtk_matf_t *output)
{
    int batch_size = 1;
    int channel = input->col
      , cur_dim=0
      , dim = input->row / batch_size
      , lstm_units = cell->lstm_units
      , is_forward = cell->is_forward
      , it_offset = is_forward? channel: -channel
      , ht_offset = is_forward? lstm_units: -lstm_units
      , i, t;
    float
        *it=is_forward?input->p: input->p+channel*(dim-1),
        *ht=NULL;
    // size_t hidden_stlen=sizeof(float)*lstm_units;
    wtk_matf_t 
        cell_in,
        cell_out;
    
    if (output)
    {
        ht=is_forward? output->p: output->p+lstm_units*(dim-1);
    }

    assert(dim*batch_size == input->row);

    cell_in.row = 1;
    cell_in.col = channel;
    cell_out.row = 1;
    cell_out.col = lstm_units;

    for (i=0; i<batch_size; ++i, cur_dim+=dim)
    {
        for (t=0; t<dim; ++t, it+=it_offset)
        {
            cell_in.p = it;
            cell_out.p = ht;
            qtk_nn_lstm_cell_forward(cell, &cell_in, &cell_out);
            if (ht)
            { ht+=ht_offset; }
        }
        qtk_nn_lstm_reset(cell);
    }
    return 0;
}

int qtk_nn_lstm_cell_forward(qtk_nn_lstm_t *lstm,wtk_matf_t *input,wtk_matf_t *output)
{
    int channel = input->col
      , lstm_units = lstm->lstm_units
      , k;
    float 
        *it = input->p,
        *ht = output->p,
        *prev_c = lstm->prev_c->p,
        *new_c = lstm->new_c->p,
        *new_h = lstm->new_h->p,
        *i = lstm->i->p,
        *j = lstm->j->p,
        *f = lstm->f->p,
        *o = lstm->o->p;

    wtk_matf_t 
        *kernel = lstm->kernel, // is transposed
        *lstm_in = lstm->lstm_in,
        *lstm_out = lstm->lstm_out;
    wtk_vecf_t 
        *bias = lstm->bias;

    memcpy(lstm_in->p, it, sizeof(float)*channel);
    // wtk_mer_blas_sgemm2(lstm_in, kernel, bias, lstm_out);
    wtk_matf_vecf_cpy(lstm_out,bias);
    qtk_lstm_matf_mul_trans(lstm_in,kernel,lstm_out);
    //sys
    wtk_nn_sigmoid(i,lstm_units);
    wtk_nn_sigmoid(f,lstm_units*2); //因为f和o是同一块内存 同时算

    for (k=0; k<lstm_units; ++k)
    {
        //sys
        new_c[k] = f[k] * prev_c[k] + i[k] * tanhf(j[k]);
        new_h[k] = o[k] * tanhf(new_c[k]);

        if (ht)
        { ht[k] = new_h[k]; }
    }
    wtk_vecf_cpy(lstm->prev_c, lstm->new_c);
    wtk_vecf_cpy(lstm->prev_h, lstm->new_h);
    return 0;
}

int qtk_nn_lstm_loadfile(qtk_nn_lstm_t *lstm,char *kernel_fn,char *bias_fn)
{
    wtk_source_loader_t sl;
    wtk_source_t source;
    
    wtk_source_loader_init_file(&sl);
    wtk_mer_source_loader_load_matf(&sl,&source,kernel_fn,lstm->kernel);
    wtk_mer_source_loader_load_vecf(&sl,&source,bias_fn,lstm->bias);
    return 0;
}

#ifdef __SSE__
void qtk_lstm_matf_mul_trans(wtk_matf_t *in,wtk_matf_t *kernel, wtk_matf_t *out)
{
    float t1;
    float *pf1,*pf2,*pf3,*kp;
    int i,j,k,n;
    int row=kernel->row;
    int col=kernel->col;
    int irow = in->row;
    int x=16;
    
	n=(col >> 4)<<4;
	pf1=in->p;
	pf3=out->p;
    kp = kernel->p;
	for(i=0;i<irow;++i)
	{
		pf2=kp;
		for(j=0;j<row;++j)
		{
            t1=0;
            sse_float32_t sum=_mm_setzero_ps();
			for(k=0; k<n; k+=x,pf2+=x)
			{
                sse_float32_t a1 = sse_loadu(pf1+k);
                sse_float32_t a2 = sse_loadu(pf1+k + 4);
                sse_float32_t a3 = sse_loadu(pf1+k + 8);
                sse_float32_t a4 = sse_loadu(pf1+k + 12);
                sse_float32_t b1 = sse_loadu(pf2);
                sse_float32_t b2 = sse_loadu(pf2+4);
                sse_float32_t b3 = sse_loadu(pf2+8);
                sse_float32_t b4 = sse_loadu(pf2+12);
                sse_float32_t c = sse_add(
                sse_add( sse_mul(a1, b1), sse_mul(a2, b2) ),
                sse_add( sse_mul(a3, b3), sse_mul(a4, b4) )
                );
                sum=sse_add(sum, c);
			}
            for (;k<col;++k)
            {
                t1+=pf1[k]*(*(pf2++));
            }
            pf3[j]+=t1+sum[0]+sum[1]+sum[2]+sum[3];
		}
        pf3+=row;
		pf1+=col;
	}
}
#else
/*
计算矩阵乘法 dot()
方法选择: A*B
                    原来是aij*bji 然后对B进行转置 变成 aij*bij 加快运算速度
要求B矩阵是转置好的
*/
void qtk_lstm_matf_mul_trans(wtk_matf_t *in,wtk_matf_t *kernel, wtk_matf_t *out)
{
    float t1, t2, t3, t4;
    float *pf1,*pf2,*pf3,*kp;
    int i,j,k,n;
    int row=kernel->row;
    int col=kernel->col;
    int irow = in->row;

	n=col-4; 
	pf1=in->p;
	pf3=out->p;
    kp = kernel->p;
	for(i=0;i<irow;++i)
	{
		pf2=kp;
		for(j=0;j<row;++j)
		{
			t1=t2=t3=t4=0;
			for(k=0; k<n; k+=4,pf2+=4)
			{
                t1 += pf1[k]*pf2[0];
                t2 += pf1[k+1]*pf2[1];
                t3 += pf1[k+2]*pf2[2];
                t4 += pf1[k+3]*pf2[3];
			}
            for (;k<col;++k)
            {
                t1+=pf1[k]*(*(pf2++));
            }
            pf3[j]+=t1+t2+t3+t4;
		}
        pf3+=row;
		pf1+=col;
	}
}
#endif
