#include "qtk_nn_gru.h"
#include "tts-mer/wtk-extend/nn/wtk_nn_activation.h"
#include "tts-mer/wtk-extend/wtk_heap2.h"
#include "tts-mer/wtk-extend/wtk_mat2.h"
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

void qtk_gru_matf_mul_trans(wtk_matf_t *in,wtk_matf_t *kernel, wtk_matf_t *out);

// int incol, int num_units, void activation(float*, int)
qtk_nn_gru_t* qtk_nn_gru_new(int input_size,int num_units,int use_bias)
{
    wtk_heap_t *heap = wtk_heap_new( 4096);
    qtk_nn_gru_t *cell=wtk_heap_malloc( heap, sizeof(*cell));
    int gate_col = 2*num_units;

    cell->heap = heap;
    cell->num_units = num_units;
    cell->is_forward = 1;
    cell->gate_in = wtk_matf_heap_new( heap, 1, input_size+num_units);
    cell->gate_out = wtk_matf_heap_new( heap, 1, gate_col);
    cell->gate_kernel = wtk_matf_heap_new( heap, gate_col, input_size + num_units);
    cell->gate_bias = wtk_vecf_heap_new( heap, gate_col);
    cell->candidate_in = wtk_matf_heap_new( heap, 1, input_size+num_units);
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
    cell->r_state->p = cell->candidate_in->p + input_size;
    cell->r_state->len = num_units;
    cell->prev_h->p = cell->gate_in->p + input_size;
    cell->prev_h->len = num_units;
    cell->new_h = cell->prev_h;
    cell->r->p = cell->gate_out->p;
    cell->u->p = cell->gate_out->p + num_units; // 内存复用
    cell->r->len = cell->u->len = num_units;
    // cell->activation = activation==NULL? wtk_nn_tanh : activation;
    cell->candidate_out_hh=wtk_matf_heap_new( heap, 1, num_units);
    cell->candidate_bias_hh=wtk_vecf_heap_new( cell->heap, cell->num_units);
    cell->candidate_kernel = wtk_matf_heap_new( heap, num_units, input_size);
    cell->candidate_kernel_hh = wtk_matf_heap_new( heap, num_units, num_units);

    cell->weight_ih=wtk_matf_heap_new( heap, 3*num_units, input_size);
    cell->weight_hh=wtk_matf_heap_new( heap, 3*num_units, num_units);
    cell->bias_ih=wtk_vecf_heap_new( heap, 3*num_units);
    cell->bias_hh=wtk_vecf_heap_new( heap, 3*num_units);

    return cell;
}

int qtk_nn_gru_forward(qtk_nn_gru_t *cell, wtk_matf_t *in, wtk_matf_t *out)
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

    for (i=0; i<dim; ++i, it+=it_offset, ht+=ht_offset)
    {
        cell_in.p = it;
        cell_out.p = ht;
        qtk_nn_gru_forward_cell(cell, &cell_in, &cell_out);
    }
    return 0;
}

int qtk_nn_gru_forward_cell(qtk_nn_gru_t *cell, wtk_matf_t *in, wtk_matf_t *out)
{
    int num_units = cell->num_units
      , incol=in->col
      , i;
    
    wtk_matf_t
        *gin = cell->gate_in,
        *gout = cell->gate_out,
        *cin = cell->candidate_in,
        *cout = cell->candidate_out,
        *cout2 = cell->candidate_out_hh,
        cin1,
        cin2;
    wtk_vecf_t
        *u = cell->u,
        *prev_h = cell->prev_h;
    size_t incol_stlen=sizeof(float)*incol;
    float 
        *fi = in->p,
        *fgi = gin->p,
        *fci = cin->p,
        *fco = cout->p,
        *fu = u->p,
        *fs = prev_h->p,
        *fnew_h = cell->new_h->p;

    memcpy(fgi, fi, incol_stlen);
    memcpy(fci, fi, incol_stlen);
    // wtk_nn_layer_dense( gin, cell->gate_kernel, cell->gate_bias, wtk_nn_sigmoid, gout);
    wtk_matf_vecf_cpy(gout, cell->gate_bias);
    qtk_gru_matf_mul_trans(gin,cell->gate_kernel, gout);
    wtk_nn_sigmoid(gout->p,gout->col*gout->row);

    assert(cin->row == 1);
    cin1.row=cin2.row=1;
    cin1.p=cin->p;
    cin1.col=incol;
    cin2.p=prev_h->p;
    cin2.col=num_units;
    // wtk_mer_blas_sgemm2( &cin1, cell->candidate_kernel, cell->candidate_bias, cout);
    wtk_matf_vecf_cpy(cout,cell->candidate_bias);
    qtk_gru_matf_mul_trans(&cin1,cell->candidate_kernel,cout);
    // wtk_mer_blas_sgemm2( &cin2, cell->candidate_kernel_hh, cell->candidate_bias_hh, cout2);
    wtk_matf_vecf_cpy(cout2,cell->candidate_bias_hh);
    qtk_gru_matf_mul_trans(&cin2,cell->candidate_kernel_hh,cout2);

    wtk_matf_vecf_multi( cout2, cell->r);
    wtk_matf_add2(cout, cout2);
    wtk_nn_tanh( fco, num_units);

    for (i=0; i<num_units; ++i)
    {
        fnew_h[i] = fu[i] * fs[i] + (1 - fu[i]) * fco[i];
    }
    memcpy(out->p, fnew_h, sizeof(float)*num_units);
    return 0;
}

int qtk_nn_gru_reset(qtk_nn_gru_t *gru)
{
    wtk_vecf_zero(gru->prev_h);
    return 0;
}

int qtk_nn_gru_delete(qtk_nn_gru_t *gru)
{
    wtk_heap_delete(gru->heap);
    return 0;
}

int qtk_nn_gru_loadfile(qtk_nn_gru_t *gru,char *gate_weight_fn,char *candidate_weight_fn,char *candidate_hh_weight_fn,
                                                    char *gate_bias_fn,char *candidate_bias_fn,char *candidate_hh_bias_fn)
{
    wtk_source_loader_t sl;
    wtk_source_t source;
    
    wtk_source_loader_init_file(&sl);
    wtk_mer_source_loader_load_matf(&sl,&source,gate_weight_fn,gru->gate_kernel);
    wtk_mer_source_loader_load_matf(&sl,&source,candidate_weight_fn,gru->candidate_kernel);
    wtk_mer_source_loader_load_matf(&sl,&source,candidate_hh_weight_fn,gru->candidate_kernel_hh);
    wtk_mer_source_loader_load_vecf(&sl,&source,gate_bias_fn,gru->gate_bias);
    wtk_mer_source_loader_load_vecf(&sl,&source,candidate_bias_fn,gru->candidate_bias);
    wtk_mer_source_loader_load_vecf(&sl,&source,candidate_hh_bias_fn,gru->candidate_bias_hh);
    return 0;
}

#ifdef __SSE__
void qtk_gru_matf_mul_trans(wtk_matf_t *in,wtk_matf_t *kernel, wtk_matf_t *out)
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
void qtk_gru_matf_mul_trans(wtk_matf_t *in,wtk_matf_t *kernel, wtk_matf_t *out)
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
