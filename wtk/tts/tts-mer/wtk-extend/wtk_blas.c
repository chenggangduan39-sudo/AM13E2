#include "wtk_blas.h"
#include "wtk_type.h"
#include "wtk_mat2.h"
#include "wtk_heap2.h"
#include "wtk_fix.h"
#include "wtk_sse.h"
//#include <omp.h>

#define matrix_transpose_check(a, b, c) {/* 矩阵转置检查 */ if (b->col != a->col){ wtk_debug("矩阵未转置 (%d, %d) * (%d, %d)\n", a->row, a->col, b->row, b->col);wtk_exit(1);}}

#ifdef __SSE__
static void matf_mul(float *a, int a_row, int a_col, float *b, int b_row, int b_col, float *c, int c_row, int c_col)
{
    float t1;
    float *pf1,*pf2,*pf3;
    int i,j,k,n;
    int row=b_row;
    int col=b_col;
    int x=16;
    
	n=col-x;
	pf1=a;
	pf3=c;
	for(i=0;i<a_row;++i)
	{
		pf2=b;
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
            pf3[j]=t1+sum[0]+sum[1]+sum[2]+sum[3];
		}
        pf3+=c_col;
		pf1+=col;
	}
}

void matf_mul_sse_sparse(float *a, int a_row, int a_col, float *b, int b_row, int *offset_arr, int *col_size_arr, float *c, int c_row, int c_col)
{
    // float t1, t2, t3, t4;
    float *pf1=a,*pf2=b,*pf3=c;
    int i,j,k;//,n;
    // int row=b_row, col;
    int col;
    int x=4;
    int *offset=offset_arr;
    // int thread_num = omp_get_num_procs();
    
    for (i=0; i<a_row; ++i)
    {
        pf2=b;
        offset=offset_arr;
        for (j=0; j<b_row; ++j)
        {
            col=col_size_arr[j];
            // t1=t2=t3=t4=0;
            // pf3[j=0];
            // printf("col: %d \n", col);
            // for (k=0; k<col; ++k, pf2+=x)
            sse_float32_t sum=_mm_setzero_ps();
            for (k=0; k<col-4; k+=4, pf2+=x*4)
            {
                // printf("k: %d offset: %d\n", k, offset[k]);
                sse_float32_t a1= sse_loadu( pf1 + offset[k]);
                sse_float32_t b1= sse_loadu( pf2);
                sse_float32_t a2= sse_loadu( pf1 + offset[k+1]);
                sse_float32_t b2= sse_loadu( pf2 + 4);
                sse_float32_t a3= sse_loadu( pf1 + offset[k+2]);
                sse_float32_t b3= sse_loadu( pf2 + 8);
                sse_float32_t a4= sse_loadu( pf1 + offset[k+3]);
                sse_float32_t b4= sse_loadu( pf2 + 12);
                // if (i==1)
                // { 
                //     printf("pf3[%d]: %f \n", j, pf3[j]);
                //     print_float2(pf1 + offset[k], x);
                //     print_float2(pf2, x);
                //     wtk_exit(0);
                // }
                sse_float32_t c1= sse_add(
                sse_add( sse_mul(a1, b1), sse_mul(a2, b2) ),
                sse_add( sse_mul(a3, b3), sse_mul(a4, b4) )
                );
                sum = sse_add(sum, c1);
                // pf3[j] += c1[0] + c1[1] + c1[2] + c1[3];
                // if (i==1) { printf("pf3[%d]: %f \n", j, pf3[j]); wtk_exit(1);}
            }
            for ( ; k<col; ++k, pf2+=x)
            {
                sse_float32_t a1= sse_loadu( pf1 + offset[k]);
                sse_float32_t b1= sse_loadu( pf2);
                sse_float32_t c1= sse_mul(a1, b1);
                sum = sse_add(sum, c1);
                // pf3[j] += c1[0] + c1[1] + c1[2] + c1[3];
            }
            // pf3[j] = t1 + t2 + t3 + t4;
            // t1 += sum[0];
            // t2 += sum[1];
            // t3 += sum[2];
            // t4 += sum[3];
            pf3[j] = sum[0] + sum[1] + sum[2] + sum[3];
            // printf("pf3[%d]: %f \n", j, pf3[j]);
            // if (i==1) { printf("pf3[%d]: %f \n", j, pf3[j]); wtk_exit(1);}
            offset+=col;
        }
        pf1+=a_col;
        pf3+=c_col;
    }
}
#else
/*
计算矩阵乘法 dot()
方法选择: A*B
                    原来是aij*bji 然后对B进行转置 变成 aij*bij 加快运算速度
要求B矩阵是转置好的
*/
static void matf_mul(float *a, int a_row, int a_col, float *b, int b_row, int b_col, float *c, int c_row, int c_col)
{/* 
if (b_col != a_col) {wtk_exit_debug("矩阵数值不匹配, 可能是未转置, (%d,%d)*(%d,%d)\n", a_row, a_col, row, col);}
 */
    float t1, t2, t3, t4;
    float *pf1,*pf2,*pf3;
    int i,j,k,n;
    int row=b_row;
    int col=b_col;

	n=col-4; 
	pf1=a;
	pf3=c;
	for(i=0;i<a_row;++i)
	{
		pf2=b;
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
            // for (k=0;k<col;++k)
            // {// t并行计算会所以更快
            //     t1+=pf1[k]*(*(pf2++));
            // }
            pf3[j]=t1+t2+t3+t4;
		}
        pf3+=c_col;
		pf1+=col;
	}
}
#endif

#ifdef __AVX__
static void sgemv_accum16(float *out, const float *weights, int rows, int cols, const float *x)
{
   int i, j;
   float *y;
   for (i=0;i<cols;i+=16)
   {
        y = &out[i];
        sse256_float32_t y0=sse256_loadu(y);
        sse256_float32_t y1=sse256_loadu(y+8);
        for (j=0;j<rows;j++)
        {
            const float *w;
            float xj;
            w = &weights[j*cols + i];
            xj = x[j];
            sse256_float32_t vxj=sse256_set(&xj);
            sse256_float32_t w0=sse256_loadu(w);
            sse256_float32_t w1=sse256_loadu(w+8);
            y0=sse256_add(y0, sse256_mul(w0, vxj));
            y1=sse256_add(y1, sse256_mul(w1, vxj));
        }
        sse256_storeu(y, y0);
        sse256_storeu(y+8, y1);
   }
}
#elif defined(__SSE__)
static inline void sgemv_accum16(float *out, const float *weights, int rows, int cols, const float *x)
{
    int i, j;
    float *y;
    for (i=0;i<cols;i+=16)
    {
        y = &out[i];
        sse_float32_t y0=sse_loadu(y);
        sse_float32_t y1=sse_loadu(y+4);
        sse_float32_t y2=sse_loadu(y+8);
        sse_float32_t y3=sse_loadu(y+12);
        for (j=0;j<rows;j++)
        {
            const float *w;
            float xj;
            w = &weights[j*cols + i];
            xj = x[j];
            sse_float32_t vxj=sse_set(xj);
            sse_float32_t w0=sse_loadu(w);
            sse_float32_t w1=sse_loadu(w+4);
            sse_float32_t w2=sse_loadu(w+8);
            sse_float32_t w3=sse_loadu(w+12);
            y0=sse_add(y0, sse_mul(w0, vxj));
            y1=sse_add(y1, sse_mul(w1, vxj));
            y2=sse_add(y2, sse_mul(w2, vxj));
            y3=sse_add(y3, sse_mul(w3, vxj));
        }
        sse_storeu(y, y0);
        sse_storeu(y+4, y1);
        sse_storeu(y+8, y2);
        sse_storeu(y+12, y3);
    }
}
#else
static void sgemv_accum16(float *out, const float *weights, int rows, int cols, const float *x)
{
   int i, j;
   for (i=0;i<cols;i+=16)
   {
        for (j=0;j<rows;j++)
        {
            const float *w;
            float xj;
            w = &weights[j*cols + i];
            xj = x[j];
            float *y = &out[i];
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
        }
   }
}
#endif
void sgemv_accum(float *out, const float *weights, int rows, int cols, const float *x)
{
    // assert( rows%16==0 && cols%16==0);
    assert(  cols%16==0);
    sgemv_accum16( out, weights, rows, cols, x);
}

#ifdef USE_MAT_FIX
static void matf_mul_fix(void *in, int a_row, int a_col, void *b, int b_row, int b_col, void *c, int c_row, int c_col)
{
    int t1, t2, t3, t4;
    float *a;
    FIX_TYPE *pf1, *pf2;
    int i,j,k,n;
    int alen=a_row*a_col;
    int row=b_row;
    int col=b_col;

    pf1 = (FIX_TYPE*)in;
    // int *pf3;
    // pf3 = (FIX_TYPE*)c;
    float *pf3;
    pf3=(float*)c;
    a = (float*)in;
    for (i=0; i<alen-4; i+=4)
    {
        // printf("%f ", (float)pf1[i]);
        pf1[i] = FTOI(a[i]);
        pf1[i+1] = FTOI(a[i+1]);
        pf1[i+2] = FTOI(a[i+2]);
        pf1[i+3] = FTOI(a[i+3]);
        // printf("%d ", pf1[i]);
    }
    for (; i<alen; ++i)
    {
        pf1[i] = FTOI(a[i]);
    }

	n=col-4;
	for(i=0;i<a_row;++i)
	{
		pf2=(FIX_TYPE*)b;
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
            pf3[j]=ITOF((t1+t2+t3+t4)/FIX);
		}
        pf3+=c_col;
		pf1+=col;
	}
}
#elif defined(USE_NEON_ASM)
#ifdef USE_NEON64
void wtk_mer_qlas_mat_cache_doverflow4_neno64(wtk_vecs_t *a1,wtk_mats_t *b,wtk_veci_t *c)
{
	int row=b->row;
	int col=b->col;
    int in_col=a1->p
      , in_row=a1->row
      , o_col=c->col;
	int i,cnt;
	int xt[4];
    int r_cnt=0;
    int st=col-row%4;
    int dst[16];
    int col_offset=0
      , row_offset=0;
	
    while((in_row-r_cnt)>=4)
	{
		//wtk_debug("%d %d\n",row,r_cnt);
		short *p1=b->p;
		int *pi,*pi1,*pi2,*pi3;
		//wtk_debug("%d\n",(r_cnt + row_offset) * o_col + col_offset);
		pi=c->p+ (r_cnt + row_offset) * o_col + col_offset;
		pi1=pi+o_col;
		pi2=pi1+o_col;
		pi3=pi2+o_col;

		int i,cnt;
		for(i=0;i<row;i++)
		{
			short *p21 = a1->p+r_cnt*i_col;
			short *p22=p21+col;
			short *p23=p22+col;
			short *p24=p23+col;
			//wtk_debug("xxx %d\n",r_cnt);
			//wtk_debug("%d %d %d %d %d\n",*p21,*p22,*p23,*p24,*p1);
			asm volatile(
					"movi      v8.4s,#0      \n"
					"movi      v9.4s,#0      \n"
					"movi      v10.4s,#0    \n"
					"movi      v11.4s,#0    \n"
					"ldr	         x1,[%[col]]  \n"

					"1:\n"
					"subs      w1,w1,#16\n"
					"blt         2f\n"
					"ld1        {v0.4h,v1.4h,v2.4h,v3.4h},[%[p1]],#32\n"
					"ld1        {v4.4h,v5.4h,v6.4h,v7.4h},[%[p21]],#32\n"
					"smlal     v8.4s,v0.4h,v4.4h\n"
					"smlal     v8.4s,v1.4h,v5.4h\n"
					"smlal     v8.4s,v2.4h,v6.4h\n"
					"smlal     v8.4s,v3.4h,v7.4h\n"

					"ld1        {v4.4h,v5.4h,v6.4h,v7.4h},[%[p22]],#32\n"
					"smlal     v9.4s,v0.4h,v4.4h\n"
					"smlal     v9.4s,v1.4h,v5.4h\n"
					"smlal     v9.4s,v2.4h,v6.4h\n"
					"smlal     v9.4s,v3.4h,v7.4h\n"

					"ld1        {v4.4h,v5.4h,v6.4h,v7.4h},[%[p23]],#32\n"
					"smlal     v10.4s,v0.4h,v4.4h\n"
					"smlal     v10.4s,v1.4h,v5.4h\n"
					"smlal     v10.4s,v2.4h,v6.4h\n"
					"smlal     v10.4s,v3.4h,v7.4h\n"

					"ld1        {v4.4h,v5.4h,v6.4h,v7.4h},[%[p24]],#32\n"
					"smlal     v11.4s,v0.4h,v4.4h\n"
					"smlal     v11.4s,v1.4h,v5.4h\n"
					"smlal     v11.4s,v2.4h,v6.4h\n"
					"smlal     v11.4s,v3.4h,v7.4h\n"
					"b 1b\n"

					"2:\n"
					"add        w1,w1,#16\n"

					"3:\n"
					"subs      w1,w1,#4\n"
					"blt         4f\n"
					"ld1        {v0.4h},[%[p1]],#8\n"
					"ld1        {v4.4h},[%[p21]],#8\n"
					"smlal     v8.4s,v0.4h,v4.4h\n"

					"ld1        {v4.4h},[%[p22]],#8\n"
					"smlal     v9.4s,v0.4h,v4.4h\n"

					"ld1        {v4.4h},[%[p23]],#8\n"
					"smlal     v10.4s,v0.4h,v4.4h\n"

					"ld1        {v4.4h},[%[p24]],#8\n"
					"smlal     v11.4s,v0.4h,v4.4h\n"
			//
					"b           3b\n"
					"4:\n"
					"add      w1,w1,#4\n"
					//"saddlp   v8.2d,         v8.4s\n"
					//"st1         {v8.4s},       [%[dst]]\n"
					"st1         {v8.4s,v9.4s,v10.4s,v11.4s},       [%[dst]]\n"
					//"st1         {},       [%[dst]]\n"
					: [dst] "+r" (dst)
					: [col] "r" (&col),
					  [p1] "r" (p1),
					  [p21] "r" (p21),
					  [p22] "r" (p22),
					  [p23] "r" (p23),
					  [p24] "r" (p24)
					 :"memory","cc","x1","w1","v0","v1","v2","v3","v4","v5","v6","v7","v8","v9","v10","v11"
					);
//			for(int xx=0;xx<16;xx++)
//			{
//				wtk_debug("%d\n",*(dst+xx));
//			}
			xt[0]=dst[0]+dst[1]+dst[2]+dst[3];
			xt[1]=dst[4]+dst[5]+dst[6]+dst[7];
			xt[2]=dst[8]+dst[9]+dst[10]+dst[11];
			xt[3]=dst[12]+dst[13]+dst[14]+dst[15];
//			wtk_debug("%d %d %d %d\n",xt[0],xt[1],xt[2],xt[3]);
			switch (cnt)
			{
			case 1:
				xt[0]+=p1[st]*p21[st];
				xt[1]+=p1[st]*p22[st];
				xt[2]+=p1[st]*p23[st];
				xt[3]+=p1[st]*p24[st];
				break;
			case 2:
				xt[0]+=p1[st]*p21[st]+p1[st+1]*p21[st+1];
				xt[1]+=p1[st]*p22[st]+p1[st+1]*p22[st+1];
				xt[2]+=p1[st]*p23[st]+p1[st+1]*p23[st+1];
				xt[3]+=p1[st]*p24[st]+p1[st+1]*p24[st+1];
				break;
			case 3:
				xt[0]+=p1[st]*p21[st]+p1[st+1]*p21[st+1]+p1[st+2]*p21[st+2];
				xt[1]+=p1[st]*p22[st]+p1[st+1]*p22[st+1]+p1[st+2]*p22[st+2];
				xt[2]+=p1[st]*p23[st]+p1[st+1]*p23[st+1]+p1[st+2]*p23[st+2];
				xt[3]+=p1[st]*p24[st]+p1[st+1]*p24[st+1]+p1[st+2]*p24[st+2];
				break;
			default:
				break;
			}
			//wtk_debug("haha %d \n",i);
				pi[i]=xt[0];
				pi1[i]=xt[1];
				pi2[i]=xt[2];
				pi3[i]=xt[3];
			}
        r_cnt+=4;
	}
	//printf("%d\n",i);
	//for(i=0;i<c->len;i++)
	//{
	//	printf("%d\n",*(c->p+i));
	//}
}
#endif

static void matf_mul_neon(void *a, int a_row, int a_col, void *b, int b_row, int b_col, void *c, int c_row, int c_col)
{
    wtk_vecs_t in;
    wtk_mats_t in2;
    wtk_veci_t out;
    float *af;
    short *pf1
        , *pf2 = (short*)b;
    float *outp;
    int *pf3;
    int i,j,k,n;
    int alen = a_row*a_col
      , n4row = a_row/4;
    int t1, t2, t3, t4;

    af = (float*)a;
    pf1=(short*)a;
    for (i=0; i<alen-4; i+=4)
    {
        pf1[i] = FTOI(af[i]);
        pf1[i+1] = FTOI(af[i+1]);
        pf1[i+2] = FTOI(af[i+2]);
        pf1[i+3] = FTOI(af[i+3]);
    }
    for (; i<alen; ++i)
    {
        pf1[i] = FTOI(af[i]);
    }

	pf3=(int*)c;
    in.len = a_col;
    in2.p = pf2;
    in2.row = b_row;
    in2.col = b_col;
    out.len = c_col;
    n = 4*c_col - 4;

    for (j=0; j<n4row; ++j, pf1+=4*a_col, pf3+=4*c_col)
    {/* 一次计算4个col */
        in.p = pf1;
        out.p = pf3;
        outp = (float*)pf3;
        #ifdef USE_NEON64
            wtk_mer_qlas_mat_cache_doverflow4_neno64(&in, &in2, &out);
        #else
            wtk_qlas_mat_cache_doverflow4(&in, &in2, &out);
        #endif
        for (i=0; i<n; i+=4)
        {
            outp[i] = ITOF(pf3[i]/FIX);
            outp[i+1] = ITOF(pf3[i+1]/FIX);
            outp[i+2] = ITOF(pf3[i+2]/FIX);
            outp[i+3] = ITOF(pf3[i+3]/FIX);
        }
        for (;i<4*c_col; ++i)
        {
            outp[i] = ITOF(pf3[i]/FIX);
        }
    }

    n=b_col-4;
    for (i=0; i<a_row%4; ++i, pf1+=a_col,pf3+=c_col)
    {
        pf2=(short*)b;
        outp = (float*)pf3;
		for(j=0;j<b_row;++j)
		{
			t1=t2=t3=t4=0;
			for(k=0; k<n; k+=4,pf2+=4)
			{
                t1 += pf1[k]*pf2[0];
                t2 += pf1[k+1]*pf2[1];
                t3 += pf1[k+2]*pf2[2];
                t4 += pf1[k+3]*pf2[3];
			}
            for (;k<b_col;++k)
            {
                t1+=pf1[k]*(*(pf2++));
            }
            outp[j]=ITOF((t1+t2+t3+t4)/FIX);
		}
    }
}
#endif

/*
计算矩阵乘法 dot()
方法选择: A*B
                    原来是aij*bji 然后对B进行转置 变成 aij*bij 加快运算速度
要求B矩阵是转置好的
double *float
*/
static void matdf_matf_mul(double *a, int a_row, int a_col, float *b, int b_row, int b_col, double *c, int c_row, int c_col)
{/* 
if (b_col != a_col) {wtk_exit_debug("矩阵数值不匹配, 可能是未转置, (%d,%d)*(%d,%d)\n", a_row, a_col, row, col);}
 */
    double t1, t2, t3, t4;
    double *pf1,*pf3;
    float *pf2;
    int i,j,k,n;
    int row=b_row;
    int col=b_col;

	n=(col>>2)<<2; 
	pf1=a;
	pf3=c;
	for(i=0;i<a_row;++i)
	{
		pf2=b;
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
            // for (k=0;k<col;++k)
            // {// t并行计算会所以更快
            //     t1+=pf1[k]*(*(pf2++));
            // }
            pf3[j]=t1+t2+t3+t4;
		}
        pf3+=c_col;
		pf1+=col;
	}
}

void wtk_mer_matf_mul(wtk_matf_t *a, wtk_matf_t *b, wtk_matf_t *c)
{
    #ifdef USE_NEON_ASM
        matf_mul_neon(a->p, a->row, a->col, b->p, b->row, b->col, c->p, c->row, c->col);
    #elif defined(USE_MAT_FIX)
        matf_mul_fix(a->p, a->row, a->col, b->p, b->row, b->col, c->p, c->row, c->col);
    #else
        matf_mul(a->p, a->row, a->col, b->p, b->row, b->col, c->p, c->row, c->col);
        // matf_mul_sse(a->p, a->row, a->col, b->p, b->row, b->col, c->p, c->row, c->col);
    #endif
}

void wtk_mer_matdf_matf_mul(wtk_matdf_t *a, wtk_matf_t *b, wtk_matdf_t *c)
{
    #ifdef USE_NEON_ASM
    #elif defined(USE_MAT_FIX)
    #else
        matdf_matf_mul(a->p, a->row, a->col, b->p, b->row, b->col, c->p, c->row, c->col);
    #endif
}


/* Layout：表示二维矩阵存储是按行优先（CblasRowMajor）还是列优先（CblasColMajor）。

transa、transb：可为CblasNoTrans、CblasTrans、CblasConjTrans

m：矩阵a和c的行数

n：矩阵b和c的列数

k：矩阵a的列数，矩阵c的行数

lda：行优先 & 不转置时，lda≥max(1,k)
       行优先 & 转置时，lda≥max(1,m)
       由于用的是C++，不太可能会使用fortran，列优先就不管了（ldb和ldc也不考虑）

ldb：行优先 & 不转置时，ldb*k的矩阵，b矩阵左上角包含n*k的B矩阵

       行优先 & 转置时，ldb*n的矩阵，b矩阵左上角包含k*n的B矩阵

ldc：行优先时，ldc≥max(1,n)
       由于用的是C++，不太可能会使用fortran，列优先就不管了 
*/
void wtk_mer_blas_sgemm2(wtk_matf_t *a, wtk_matf_t *b, wtk_vecf_t *vf, wtk_matf_t *c)
{
    // wtk_matf_t *t = wtk_matf_transpose(b); //请转置后传参
    
    matrix_transpose_check(a, b, c);
    // if (b->col != a->col)
    // { wtk_exit_debug("矩阵未转置 (%d, %d) * (%d, %d)\n", a->row, a->col, b->row, b->col);}
    // wtk_matf_mul(a, b, c);
    // wtk_mer_matf_mul_parallel(a, b, c);
    #if defined(USE_GO2BLAS)
        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans, a->row, c->col, a->col, 1, a->p, a->col, b->p, b->col, 0, c->p, c->col);
    #elif defined(USE_MKL)
        #if defined(USE_MKL_PACK)
            cblas_sgemm_compute(CblasRowMajor, CblasNoTrans, CblasPacked, a->row, c->col, a->col, a->p, a->col, b->p, b->col, 0, c->p, c->col);
        #else
            cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans, a->row, c->col, a->col, 1, a->p, a->col, b->p, b->col, 0, c->p, c->col);
        #endif
    #else
        wtk_mer_matf_mul( a, b, c);
    #endif

    // cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans, c->row, c->col, a->col, 1, a->p, a->col, b->p, b->col, 0, c->p, c->col);
    
    if (vf) { wtk_matf_vecf_add(c, vf);}
}

void wtk_mer_blas_sgemm(wtk_matf_t *a, wtk_matf_t *b, wtk_vecf_t *vf, wtk_matf_t *c)
{/* 
不使用特殊加速, 如
定点. mkl sgemm_pack
 */
    // if (b->col != a->col)
    // { wtk_exit_debug("矩阵未转置 (%d, %d) * (%d, %d)\n", a->row, a->col, b->row, b->col);}
    matrix_transpose_check(a, b, c);

    #if defined(USE_GO2BLAS) || defined(USE_MKL)
        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans, a->row, c->col, a->col, 1, a->p, a->col, b->p, b->col, 0, c->p, c->col);
    #else
        matf_mul(a->p, a->row, a->col, b->p, b->row, b->col, c->p, c->row, c->col);
    #endif

    if (vf) { wtk_matf_vecf_add(c, vf);}
}

void wtk_mer_blas_sgemm3(wtk_matdf_t *a, wtk_matf_t *b, wtk_vecf_t *vf, wtk_matdf_t *c)
{/* 
不使用特殊加速, 如
定点. mkl sgemm_pack
 */
    matrix_transpose_check(a, b, c);

    // #if defined(USE_GO2BLAS) || defined(USE_MKL)
    // #else
        matdf_matf_mul(a->p, a->row, a->col, b->p, b->row, b->col, c->p, c->row, c->col);
    // #endif
}

void wtk_mer_unblas_sgemm(wtk_matf_t *a, wtk_matf_t *b, wtk_vecf_t *vf, wtk_matf_t *c)
{/* 
不会调用第三方库
 */
    matrix_transpose_check(a, b, c);
    wtk_mer_matf_mul( a, b, c);
    if (vf) { wtk_matf_vecf_add(c, vf);}
}

#ifdef __AVX__
void wtk_mer_unblas_sgemm_notrans(wtk_matf_t *a, wtk_matf_t *b, wtk_vecf_t *vf, wtk_matf_t *c)
{
    assert( a->col == b->row);

    if (vf) { wtk_matf_vecf_cpy( c, vf); }
    else { wtk_matf_zero(c); }

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
/* b矩阵不转置,加速效果很好 */
//实现有问题 要改
void wtk_mer_unblas_sgemm_notrans(wtk_matf_t *a, wtk_matf_t *b, wtk_vecf_t *vf, wtk_matf_t *c)
{
    assert( a->col == b->row);

    if (vf) { wtk_matf_vecf_cpy( c, vf); }
    else { wtk_matf_zero(c); }

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
void wtk_mer_unblas_sgemm_notrans(wtk_matf_t *a, wtk_matf_t *b, wtk_vecf_t *vf, wtk_matf_t *c)
{
    assert( a->col == b->row);

    if (vf) { wtk_matf_vecf_cpy( c, vf); }
    else { wtk_matf_zero(c); }

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

int is_2power(int x)
{/* 判断2的次方 */
    return  (x&(x-1)) == 0;
}
