#include "qtk_nn_fc.h"
#include "tts-mer/wtk-extend/nn/wtk_nn_activation.h"
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

static void qtk_fc_matf_mul_notrans(wtk_matf_t *a, wtk_matf_t *b,wtk_matf_t *c);
static void qtk_fc_matf_mul_trans(wtk_matf_t *a, wtk_matf_t *b, wtk_matf_t *c);

qtk_nn_fc_t *qtk_nn_fc_new(int inl,int outl,QTK_NN_ACTINATION_TYPE_T actination_type, int use_bias, int use_kernel_trans)
{
    qtk_nn_fc_t *layer = wtk_calloc(1, sizeof(qtk_nn_fc_t));
    
    layer->use_kernel_trans = use_kernel_trans;
    if(use_kernel_trans){
        layer->kernel = wtk_matf_new(outl,inl);
    }else{
        layer->kernel = wtk_matf_new(inl,outl);
    }

    if(use_bias){
        layer->bias = wtk_vecf_new(outl);
    }
    
    //action type
    if(actination_type == QTK_NN_ACTINATION_RULE){
        layer->activation = wtk_nn_relu;
    }else if(actination_type == QTK_NN_ACTINATION_TANH){
        layer->activation = wtk_nn_tanh;
    }else if(actination_type == QTK_NN_ACTINATION_SIGMOID){
        layer->activation = wtk_nn_sigmoid;
    }else if(actination_type == QTK_NN_ACTINATION_SOFTMAX){
        layer->activation = wtk_nn_softmax;
    }else if(actination_type == QTK_NN_ACTINATION_SOFTPLUS){
        layer->activation = wtk_nn_softplus;
    }else{
        layer->activation = NULL;
    }
    return layer;
}



int qtk_nn_fc_forward(qtk_nn_fc_t *layer,wtk_matf_t *in,wtk_matf_t *out)
{
    wtk_matf_t *kernel = layer->kernel;
    wtk_vecf_t *bias = layer->bias;

    if(layer->bias){
        wtk_matf_vecf_cpy(out, bias);
    }

    if(layer->use_kernel_trans){
        qtk_fc_matf_mul_trans(in,kernel,out);
    }else{
        qtk_fc_matf_mul_notrans(in,kernel,out);
    }
    if(layer->activation){
        layer->activation(out->p,out->row*out->col);
    }
    return 0;
}

int qtk_nn_fc_delete(qtk_nn_fc_t *layer)
{
    if(layer->kernel)
        wtk_matf_delete(layer->kernel);
    if(layer->bias)
        wtk_vecf_delete(layer->bias);
    wtk_free(layer);
    return 0;
}

int qtk_nn_fc_load_file(qtk_nn_fc_t *layer, char *weight, char *bias)
{
    wtk_source_loader_t sl;
    wtk_source_t source;
    
    wtk_source_loader_init_file(&sl);
    wtk_mer_source_loader_load_matf(&sl,&source,weight,layer->kernel);
    if(bias){
        wtk_mer_source_loader_load_vecf(&sl,&source,bias,layer->bias);
    }
    return 0;
}

//fc 乘法运算
#ifdef __AVX__
void qtk_fc_matf_mul_notrans(wtk_matf_t *a, wtk_matf_t *b,wtk_matf_t *c)
{
    assert( a->col == b->row);

    int i = 0,j = 0,k = 0;
    int irow = a->row, icol = a->col;
    int kcol = b->col;
    int n = (kcol/16)*16;

    float *ip = NULL, *kp = NULL, *op = NULL;
    float vi;
    ip = a->p;
    op = c->p;
    sse256_float32_t visp;
    sse256_float32_t y0;
    sse256_float32_t y1;
    sse256_float32_t w0;
    sse256_float32_t w1;

    for(i = 0; i < irow; ++i){
        for(j = 0; j < icol; ++j){
            vi=ip[j];
            visp=sse256_set(&vi);
            kp = b->p+j*kcol;
            for(k = 0; k < n; k +=16){
                y0=sse256_loadu(op+k);
                y1=sse256_loadu(op+k+8);
                w0=sse256_loadu(kp+k);
                w1=sse256_loadu(kp+k+8);

                y0=sse256_add(y0, sse256_mul(w0, visp));
                y1=sse256_add(y1, sse256_mul(w1, visp));
                sse256_storeu(op+k, y0);
                sse256_storeu(op+k+8, y1);
            }
            for(;k < kcol; ++k){
                op[k] += kp[k] * vi;
            }
        }
        ip += icol;
        op += kcol;
    }
    return;
}

#elif defined(__SSE__)
void qtk_fc_matf_mul_notrans(wtk_matf_t *a, wtk_matf_t *b, wtk_matf_t *c)
{
    assert( a->col == b->row);

    int i = 0,j = 0,k = 0;
    int irow = a->row, icol = a->col;
    int kcol = b->col;
    int n = (kcol/16)*16;

    float *ip = NULL, *kp = NULL, *op = NULL;
    float vi;
    ip = a->p;
    op = c->p;
    sse_float32_t visp;
    sse_float32_t y0;
    sse_float32_t y1;
    sse_float32_t y2;
    sse_float32_t y3;
    sse_float32_t w0;
    sse_float32_t w1;
    sse_float32_t w2;
    sse_float32_t w3;

    for(i = 0; i < irow; ++i){
        for(j = 0; j < icol; ++j){
            vi=ip[j];
            visp=sse_set(vi);
            kp = b->p+j*kcol;
            for(k = 0; k < n; k +=16){
                y0=sse_loadu(op+k);
                y1=sse_loadu(op+k+4);
                y2=sse_loadu(op+k+8);
                y3=sse_loadu(op+k+12);
                w0=sse_loadu(kp+k);
                w1=sse_loadu(kp+k+4);
                w2=sse_loadu(kp+k+8);
                w3=sse_loadu(kp+k+12);

                y0=sse_add(y0, sse_mul(w0, visp));
                y1=sse_add(y1, sse_mul(w1, visp));
                y2=sse_add(y2, sse_mul(w2, visp));
                y3=sse_add(y3, sse_mul(w3, visp));
                sse_storeu(op+k, y0);
                sse_storeu(op+k+4, y1);
                sse_storeu(op+k+8, y2);
                sse_storeu(op+k+12, y3);
            }
            for(;k < kcol; ++k){
                op[k] += kp[k] * vi;
            }
        }
        ip += icol;
        op += kcol;
    }
    return;
}
#else
/* b矩阵不转置,加速效果很好 */
//实现有问题 要改
void qtk_fc_matf_mul_notrans(wtk_matf_t *a, wtk_matf_t *b, wtk_matf_t *c)
{
    assert( a->col == b->row);

    int i = 0,j = 0,k = 0;
    int irow = a->row, icol = a->col;
    int kcol = b->col;
    int n = (kcol/16)*16;

    float *ip = NULL, *kp = NULL, *op = NULL;
    float vi = 0.0f;
    ip = a->p;
    op = b->p;
    for(i = 0; i < irow; ++i){
        for(j = 0; j < a->col; ++j){
            vi = ip[j];
            kp = b->p+j*kcol;
            for(k = 0; k < n; k +=16){
                op[k] += kp[k] *vi;
                op[k+1] += kp[k+1] *vi;
                op[k+2] += kp[k+2] *vi;
                op[k+3] += kp[k+3] *vi;
                op[k+4] += kp[k+4] *vi;
                op[k+5] += kp[k+5] *vi;
                op[k+6] += kp[k+6] *vi;
                op[k+7] += kp[k+7] *vi;
                op[k+8] += kp[k+8] *vi;
                op[k+9] += kp[k+9] *vi;
                op[k+10] += kp[k+10] *vi;
                op[k+11] += kp[k+11] *vi;
                op[k+12] += kp[k+12] *vi;
                op[k+13] += kp[k+13] *vi;
                op[k+14] += kp[k+14] *vi;
                op[k+15] += kp[k+15] *vi;
            }
            for(;k < kcol; ++k){
                op[k] += kp[k] * vi;
            }
        }
        ip += icol;
        op += kcol;
    }
    return;
}
#endif

#ifdef __SSE__
void qtk_fc_matf_mul_trans(wtk_matf_t *in,wtk_matf_t *kernel, wtk_matf_t *out)
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
#elif defined USE_NEON3308
#include "arm_neon.h"
void qtk_fc_matf_mul_trans(wtk_matf_t *in,wtk_matf_t *kernel, wtk_matf_t *out)
{
    float t1;
    float *pf1,*pf2,*pf3,*kp;
    int i,j,k,n;
    int row=kernel->row;
    int col=kernel->col;
    int irow = in->row;
    float32x4_t a,b,c;
    float vc[4];

	n=(col>>2)<<2;
	pf1=in->p;
	pf3=out->p;
    kp = kernel->p;
	for(i=0;i<irow;++i)
	{
		pf2=kp;
		for(j=0;j<row;++j)
		{
			c = vdupq_n_f32(0.f);
			for(k=0; k<n; k+=4,pf2+=4)
			{
				a = vld1q_f32(pf1+k);
				b = vld1q_f32(pf2);
				//vpadd_f32(v, vmul_f32(pf1, pf2))  //step =2
				//c = vaddq_f32(c, vmulq_f32(a, b));
				c = vmlaq_f32(c, a, b);
			}
			vst1q_f32(vc, c);
			t1 = vc[0] + vc[1] + vc[2] + vc[3];
            for (;k<col;++k)
            {
                t1+=pf1[k]*(*(pf2++));
            }
            pf3[j]=t1;
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
void qtk_fc_matf_mul_trans(wtk_matf_t *in,wtk_matf_t *kernel, wtk_matf_t *out)
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
