#include "qtk_nn_conv1d.h"
#include "qtk_nn_comm.h"
#include "wtk/tts/tts-mer/wtk-extend/wtk_mat2.h"
#include "wtk/tts/tts-mer/wtk-extend/wtk_mer_source_file.h"

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

void qtk_conv1d_matf_mul_trans(wtk_matf_t *in,wtk_matf_t *kernel, wtk_matf_t *out);

/*
pytorch 的kernel多为转置好的矩阵
这里使用转置好的矩阵
padding 使用 zero 填充
*/
qtk_nn_conv1d_t *qtk_nn_conv1d_new(int in_dim,int out_dim,int kernel_size,int padding,int bias)
{
    qtk_nn_conv1d_t *conv = NULL;

    conv = wtk_malloc(sizeof(qtk_nn_conv1d_t));
    conv->in_dim = in_dim;
    conv->out_dim = out_dim;
    conv->size = kernel_size;
    conv->padding = padding;
    conv->stride = 1;
    
    conv->kernel = wtk_matf_new(out_dim,in_dim*kernel_size);
    conv->bias = NULL;
    if(bias){
        conv->bias = wtk_vecf_new(out_dim);
    }
    return conv;
}

int qtk_nn_conv1d_delete(qtk_nn_conv1d_t *conv)
{
    wtk_matf_delete(conv->kernel);
    if(conv->bias){
        wtk_vecf_delete(conv->bias);
    }
    wtk_free(conv);

    return 0;
}

/*
本函数没有完全实现 in的conv1d 的row
kernal.shape [outdim,size,indim] ==> (indim*size*outdim) 在内存中按照计算顺序排列
//in.shape [row,col,indim] ==> (indim*col*row)
in.shape [row,indim] ==> (indim*row) 
//out.shape [row,col,outdim] ==> (outdim*col*row)
out.shape [row,outdim] ==> (outdim*row)
运算时 填充为大矩阵一次性计算
实现的是卷积核一次移动一位
卷积长度计算公式(col-size+stride+padding*2)/stride
*/
int qtk_nn_conv1d_forward(qtk_nn_conv1d_t *conv, wtk_matf_t *in, wtk_matf_t *out)
{
    int ret = -1;
    wtk_matf_t *kernel = conv->kernel;
    wtk_vecf_t *bias = conv->bias;
    int kernel_size = conv->size;
    int i_col = in->col,i_row = in->row;
    int p_row = 0,p_col = 0;
    int padding = conv->padding;
    float *pf = NULL,*inp = NULL,*mp = NULL,*inep = NULL;
    int i = 0,strip = 0,ulen = 0;

    strip = conv->stride;   //每次卷积核移动的距离
    p_col = i_col * kernel_size;
    p_row = (i_row+padding*2-kernel_size)/strip+1;  //pad之后的行值
    wtk_matf_t *pad = wtk_matf_new(p_row,p_col);
    pf = pad->p;
    inp = in->p;
    inep = in->p + in->row * in->col;

    int pad_sn = padding;
    int skip = 0;
    //填充并变成大矩阵
    for(i = 0; i < p_row; ++i){
        mp = inp;
        ulen = p_col;
        skip = 0;
        strip = conv->stride;
        //填充前部
        if(pad_sn != 0){
            if(pad_sn > kernel_size){
                mp = NULL;
            }else{
                skip = pad_sn * i_col;
                ulen -= skip;
            }
            if(pad_sn >= strip){
                pad_sn -= strip;
                strip = 0;
            }else{
                strip -= pad_sn;
                pad_sn = 0;
            }
        }
        if(mp && mp+ulen > inep){
            ulen = inep - mp;
        }

        if(mp && ulen > 0){
            memcpy(pf+skip,mp,sizeof(float)*ulen);
            inp += strip * i_col;
        }
        pf += p_col;
    }
    // wtk_matf_print(pad);
    //运算
    if(bias){
        wtk_matf_vecf_cpy(out, bias);
    }
    qtk_conv1d_matf_mul_trans(pad,kernel,out);

    wtk_matf_delete(pad);
    ret = 0;
    return ret;
}

int qtk_nn_conv1d_forward_blk(qtk_nn_conv1d_t *conv, wtk_matf_t *in, wtk_matf_t *out)
{
    int ret = -1;
    wtk_matf_t *kernel = conv->kernel;
    wtk_vecf_t *bias = conv->bias;
    int kernel_size = conv->size;
    int i_col = in->col,i_row = in->row;
    int p_row = 0,p_col = 0;
    int padding = conv->padding;
    float *pf = NULL,*inp = NULL,*mp = NULL,*inep = NULL;
    int i = 0,strip = 0,ulen = 0;

    strip = conv->stride;   //每次卷积核移动的距离
    p_col = i_col * kernel_size;
    p_row = (i_row+padding*2-kernel_size)/strip+1;  //pad之后的行值
    wtk_matf_t *pad = wtk_matf_new(p_row,p_col);
    pf = pad->p;
    inp = in->p;
    inep = in->p + in->row * in->col;

    int pad_sn = padding;
    int skip = 0;
    //填充并变成大矩阵
    for(i = 0; i < p_row; ++i){
        mp = inp;
        ulen = p_col;
        skip = 0;
        strip = conv->stride;
        //填充前部
        if(pad_sn != 0){
            if(pad_sn > kernel_size){
                mp = NULL;
            }else{
                skip = pad_sn * i_col;
                ulen -= skip;
            }
            if(pad_sn >= strip){
                pad_sn -= strip;
                strip = 0;
            }else{
                strip -= pad_sn;
                pad_sn = 0;
            }
        }
        if(mp && mp+ulen > inep){
            ulen = inep - mp;
        }

        if(mp && ulen > 0){
        	//printf("ulen=%d\n", ulen);
            memcpy(pf+skip,mp,sizeof(float)*ulen);
            inp += strip * i_col;
        }
        pf += p_col;
    }
    // wtk_matf_print(pad);
    //运算
    if(bias){
        wtk_matf_vecf_cpy(out, bias);
    }
    wtk_matf_mul_blk(pad->p, kernel->p, out->p, pad->row, kernel->col, pad->col);

    wtk_matf_delete(pad);
    ret = 0;
    return ret;
}

//load bin file
int qtk_nn_conv1d_load_file(qtk_nn_conv1d_t *conv,char *kernel_fn,char *bias_fn)
{
    int ret = -1;
    wtk_source_loader_t sl;
    wtk_source_t source;
    
    wtk_source_loader_init_file(&sl);
    wtk_mer_source_loader_load_matf(&sl,&source,kernel_fn,conv->kernel);
    if(bias_fn && conv->bias){
        wtk_mer_source_loader_load_vecf(&sl,&source,bias_fn,conv->bias);
    }
    ret = 0;
    return ret;
}

int qtk_nn_conv1d_out_row(qtk_nn_conv1d_t *conv,int in_row)
{
    return (in_row+conv->padding*2-conv->size)/conv->stride+1;
}

#ifdef __SSE__
void qtk_conv1d_matf_mul_trans(wtk_matf_t *in,wtk_matf_t *kernel, wtk_matf_t *out)
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
void qtk_conv1d_matf_mul_trans(wtk_matf_t *in,wtk_matf_t *kernel, wtk_matf_t *out)
{
    float t1;
    float *pf1,*pf2,*pf3,*kp;
    int i,j,k,n;
    int row=kernel->row;
    int col=kernel->col;
    int irow = in->row;
    float32x4_t a,b,c;
    float vc[4];

//    wtk_debug("col =%d\n", col);
//	n=col-4;
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
				c = vaddq_f32(c, vmulq_f32(a, b));
				//c = vmlaq_f32(c, a, b);

				//test
				vst1q_f32(vc, vmulq_f32(a, b));
//				wtk_debug("%f %f %f\n", *(pf1+k), *pf2, vc[0]);
//				wtk_debug("%f %f %f\n", *(pf1+k+1), *(pf2+1), vc[1]);
//				wtk_debug("%f %f %f\n", *(pf1+k+2), *(pf2+2), vc[2]);
//				wtk_debug("%f %f %f\n", *(pf1+k+3), *(pf2+3), vc[3]);
			}

			vst1q_f32(vc, c);
			t1 = vc[0] + vc[1] + vc[2] + vc[3];
//			wtk_debug("t1=%f k=%d col=%d\n", t1, k, col);
            for (;k<col;++k)
            {
//            	wtk_debug("%f %f %f\n", *(pf1+k), *(pf2), *(pf1+k) * *(pf2));
                t1+=pf1[k]*(*(pf2++));
            }
            pf3[j]=t1;
//    		wtk_debug("%f\n", pf3[j]);
//    		exit(0);
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
void qtk_conv1d_matf_mul_trans(wtk_matf_t *in,wtk_matf_t *kernel, wtk_matf_t *out)
{
    float t1, t2, t3, t4;
    float *pf1,*pf2,*pf3,*kp;
    int i,j,k,n;
    int row=kernel->row;
    int col=kernel->col;
    int irow = in->row;

//	n=col-4;
    n=(col>>2)<<2;
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
