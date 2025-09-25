#include "wtk_mat.h"
#include "wtk_type.h"
#include "wtk_file.h"
#include "wtk_sse.h"
// #include "wtk_duff.h"

/* matrix write to file */
static void wtk_mat_save(void *p, int row, int col, char type, FILE *f)
{
	int i,j;
	float *fp = NULL;
	double *dp = NULL;
	switch (type)
	{
		case 'f':
			fp = (float*)p;
			break;
		case 'd':
			dp = (double*)p;
		default:
			break;
	}
	
	for(i=0;i<row;++i)
	{
		for(j=0;j<col;++j)
		{
			switch (type)
			{
				case 'f':
					fprintf(f, (j>0?" %-10.6f":"%-10.6f"), *(fp++));
					break;
				case 'd':
					fprintf(f, (j>0?" %.6lf":"%.6lf"), *(dp++));
				default:
					break;
			}
		}
		fprintf(f,"\n");
	}
	//fprintf(f,"\n");
}
void wtk_mer_matdf_save(wtk_matdf_t *m, FILE *f, char *fn)
{
	int row, col;
	double *p;
	row = m->row;
	col = m->col;
	p=m->p;
	fprintf(f,"%s row: %d col: %d\n", fn, row, col);
	wtk_mat_save(p, row, col, 'd', f);
}
void wtk_mer_matf_save(wtk_matf_t *m, FILE *f, char *fn)
{
	int row, col;
	float *p;
	row = m->row;
	col = m->col;
	p=m->p;
	// fprintf(f,"%s row: %d col: %d\n", fn, row, col);
	wtk_mat_save(p, row, col, 'f', f);
}
int wtk_mer_mat_write_file(void* p, int row, int col, char type, char *fn, int is_bin)
{
    size_t st;
    FILE *fp;
    // wtk_debug("load_fn ---> %s \n", fn);
    fp=wtk_mer_getfp(fn, is_bin? "wb": "w");
    switch (type)
    {
        case 'f':
            st = sizeof(float);
            break;
        case 'd':
            st = sizeof(double);
            break;
        default:
            st=0;
            wtk_exit_debug("未知的数据类型 type: %c \n", type);
    }

    if (is_bin)
    {
        fwrite(p, st, row*col, fp);
    } else
    {
        if (type == 'f')
        {
            wtk_matf_t m;
            m.p = p;
            m.row = row;
            m.col = col;
            wtk_mer_matf_save(&m, fp, fn);
        } else
        {
            wtk_matdf_t m;
            m.p = p;
            m.row = row;
            m.col = col;
            wtk_mer_matdf_save(&m, fp, fn);
        }
    }
    fclose( fp);
    wtk_debug("shape(%d, %d) output_fn ---> %s \n", row, col, fn);
    return 0;
}
int wtk_mer_matf_write_file(wtk_matf_t *m, char *fn, int is_bin)
{
    return wtk_mer_mat_write_file(m->p, m->row, m->col, 'f', fn, is_bin);
}
int wtk_mer_matdf_write_file(wtk_matdf_t *m, char *fn, int is_bin)
{
    return wtk_mer_mat_write_file(m->p, m->row, m->col, 'd', fn, is_bin);
}
static void matf_load_call(wtk_matf_t *m, wtk_source_t *src)
{
    wtk_source_read_float(src, m->p, m->row*m->col, 0);
}
void wtk_mer_matf_read_file(wtk_matf_t *m, char *fn)
{
    wtk_mer_matf_read_file2(m, fn, 0);
}
void wtk_mer_matf_read_file2(wtk_matf_t *m, char *fn, int isbin)
{
    wtk_debug("load_fn ---> %s \n", fn);
    if (isbin)
    {
        FILE *fp=NULL;
        int len = m->row*m->col
		  , check_len
          , count = 0;
        //   , size = 1024
        //   , s ;
        float *p = m->p;
        fp=wtk_mer_getfp(fn, "rb");
		count=file_length(fp);
		if (count != len*sizeof(float))
        {
            wtk_debug("矩阵大小和资源文件不匹配: %s  资源size: %d != 矩阵size:  %d*%d=%d \n", fn, count, m->row, m->col, len);
            exit(1);
        }

		check_len=fread(p, sizeof(float), len, fp);
		assert(check_len==len);
        
        fclose(fp);
    } else 
    {
        wtk_source_load_file(m, (wtk_source_load_handler_t)matf_load_call, fn);
    }
}

/* vector write to file */
static void vecf_load_call(wtk_vecf_t *dst, wtk_source_t *src)
{
    wtk_source_read_float(src, dst->p, dst->len, 0);
}
void wtk_mer_vecf_read_file( wtk_vecf_t *dst, char *fn)
{
    wtk_mer_vecf_read_file2(dst, fn, 0);
}
void wtk_mer_vecf_read_file2( wtk_vecf_t *dst, char *fn, int isbin)
{
    FILE *fp = NULL;
    int n;
    if (isbin){
        fp=wtk_mer_getfp(fn, "rb");
        wtk_debug("load_fn ---> %s \n", fn);
        n=fread(dst->p, sizeof(float), dst->len, fp);
        if (n==0) {printf("warning load fn --> %s \n", fn);}
    } else {
        wtk_source_load_file(dst, (wtk_source_load_handler_t)vecf_load_call, fn);
    }
}
wtk_vecf_t* wtk_mer_vecf_read_binfile( char *fn)
{
    FILE *fp = NULL;
    const int size = 100;
    float buf[size];
    int n
      , i
      , nsolt;
    void *slot;
    wtk_vecf_t *dst;
    wtk_larray_t *arr = wtk_larray_new(500, sizeof(float));

    fp=wtk_mer_getfp(fn, "rb");
    wtk_debug("load_fn ---> %s \n", fn);
    while(1)
    {
        n = fread(buf, sizeof(float), size, fp);
        if (n==0) {break;}
        for (i=0; i<n; i++)
        {
            slot = wtk_larray_push_n(arr, 1);
            memcpy(slot, &buf[i], sizeof(float));
        }
    }
    nsolt = arr->nslot;
    dst = wtk_vecf_new( nsolt);
    memcpy(dst->p, arr->slot, sizeof(float)*nsolt);
    // wtk_vecf_print(dst);
    fclose(fp);
    wtk_larray_delete(arr);
    return dst;
}

/* -------------------------------------------------------------
matrix 操作拓展
 */
wtk_matf_t* wtk_matf_slice(wtk_matf_t *src, int srow, int nrow, int scol, int ncol)
{/* srow start_row
	scol start_col
 */
	wtk_matf_t *dst = wtk_matf_new( nrow, ncol);
	wtk_matf_slice2(src, dst, srow, nrow, scol, ncol);
	return dst;
}

void wtk_matf_slice2(wtk_matf_t *src, wtk_matf_t *dst, int srow, int nrow, int scol, int ncol)
{
	int old_nrow = src->row
	  , old_ncol = src->col;
	float *p;
	
	if ( srow+nrow > old_nrow || scol+ncol > old_ncol) 
	{
		wtk_debug("\n matf slice crossing \n");
		exit(-1);
	}
	if ( ncol == old_ncol)
	{
		p = wtk_matf_row( src, srow);
		memcpy( dst->p, p, nrow * old_ncol * sizeof(float));
	} else 
	{
		int i;

		float *dstp = dst->p;
		for (i=srow; i<srow+nrow; i++)
		{
			p = wtk_matf_row( src, i);
			memcpy( dstp, p+scol, ncol * sizeof(float));
			dstp += ncol;
		}
	}
}

wtk_matf_t* wtk_matf_row_slice(wtk_matf_t *src, int srow, int nrow)
{/* by row slice */
	return wtk_matf_slice(src, srow, nrow, 0, src->col);
}
void wtk_matf_init_transpose(wtk_matf_t *src, wtk_matf_t *dst)
{
	float *p;
	int i,j;

	p=src->p;
	for(i=0;i<src->row;++i)
	{
		for(j=0;j<src->col;++j)
		{
			*(wtk_matf_at(dst,j,i))=*(p++);
		}
	}
}
void wtk_matf_reshape(wtk_matf_t *src, int nr, int nc)
{
	int s = src->row*src->col
      , r = nr
	  , c = nc;
	if (r==-1)
	{
		nr = s/c;
	} else if (c==-1)
	{
		nc = s/r;
	}
	if (nr*nc != s || (r<0 && c<0))
	{
		wtk_exit_debug("illegal matf reshape : nr: %d  nc: %d \n", r, c);
	}
	src->row = nr;
	src->col = nc;
}
void wtk_mer_matf_concat( wtk_matf_t *a, wtk_matf_t *b, wtk_matf_t *dst)
{/* 
矩阵列拼接
a->col + b->col = dst->col
 */
	assert(a->col + b->col == dst->col);
	assert((a->row == b->row) && (a->row == dst->row));

	int nacol=a->col
	  , nbcol=b->col
	  , nccol=dst->col
	  , nrow=dst->row
	  , i;

	float 
		*pa=a->p,
		*pb=b->p,
		*pdst=dst->p;
	size_t 
		a_stlen=sizeof(float)*nacol,
		b_stlen=sizeof(float)*nbcol;
	
	for (i=0; i<nrow; ++i, pa+=nacol, pb+=nbcol, pdst+=nccol)
	{
		memcpy( pdst, pa, a_stlen);
		memcpy( pdst+nacol, pb, b_stlen);
	}
}
// void wtk_mer_matf_shape_print( wtk_matf_t *a)
// {
// 	wtk_debug( "r:%d c:%d \n", a->row, a->col);
// }


/* -------------------------------------------------------------------------- 
float 拓展
 */
void wtk_float_set(float *dst, float f, int n)
{
	int i, n2;
	n2 = (n>>2)<<2;
	for (i=0; i<n2; i+=4)
	{
		dst[i]= f;
		dst[i+1]= f;
		dst[i+2]= f;
		dst[i+3]= f;
	}
	for (;i<n; ++i)
	{
		dst[i]=f;
	}
}
void wtk_float_tanh(float *dst, const int n)
{
	int i;
	for (i=0; i<n; ++i)
	{
		dst[i] = tanhf(dst[i]);
	}
}
void wtk_float_set_bound(float min, float max, float *p, int len)
{
	int i;
    for (i=0; i<len; ++i)
    {
        if (p[i]<min) { p[i]=min;}
        if (p[i]>max) { p[i]=max;}
    }
}

/* --------------------------------------------------------------
vector 拓展
 */
void wtk_vecf_concat( wtk_vecf_t *dst, wtk_vecf_t *a, wtk_vecf_t *b)
{
	int s1 = a->len
	  , s2 = b->len
	  , s3 = s1 + s2
	  , i;
	for (i=0; i<s3; i++)
	{
		if (i<s1) { dst->p[i] = a->p[i]; }
		else { dst->p[i] = b->p[i-s1]; }
	}
}

wtk_vecf_t* wtk_vecf_new_concat( wtk_vecf_t *a, wtk_vecf_t *b)
{
	wtk_vecf_t *c = wtk_vecf_new(a->len+b->len);
	wtk_vecf_concat(c, a, b);
	return c;
}

void wtk_vecf_minus(wtk_vecf_t *dst, wtk_vecf_t *a, wtk_vecf_t *b)
{
	int i;
	for (i=0; i<dst->len;++i)
	{
		dst->p[i] = a->p[i] - b->p[i];
	}
}


/* --------------------------------------------------------------------------
matrix 和 vecf 混合拓展
 */

void wtk_matf_vecf_multi(wtk_matf_t *dst, wtk_vecf_t *vec)
{/* 矩阵 * 向量 逐行相乘 */
	int nr = dst->row
	  , nc = dst->col
	  , step=4
	  , i
	  , j;
	float *p = dst->p, *vp=vec->p;

	for (i=0; i<nr; i++)
	{
		for (j=0; j<nc-step; j+=step, p+=step)
		{
			// *p*=vp[j];
			// p++;
			p[0]*=vp[j];
			p[1]*=vp[j+1];
			p[2]*=vp[j+2];
			p[3]*=vp[j+3];
			// sse_float32_t c=sse_loadu(p);
			// sse_float32_t a=sse_loadu(vp+j);
			// sse_storeu(p, sse_mul(c, a));
		}
		for ( ; j<nc; ++j)
		{
			*p*=vp[j];
			p++;
		}
	}
}

/*
A(r,c) B(1,c)
A+B = A(n,c) + B(1,c)
*/
void wtk_matf_vecf_add(wtk_matf_t *dst, wtk_vecf_t *vec)
{
	int nr = dst->row
	  , nc = dst->col
	  , n = nc-4
	  , i
	  , j;
	float 
		*p = dst->p,
		*vp = vec->p;

	#ifdef __SSE__
	sse_float32_t a, b;
	#endif

	for (i=0; i<nr; ++i)
	{
		vp = vec->p;
		for (j=0; j<n; j+=4, p+=4, vp+=4)
		{
			#ifdef __SSE__
			a = sse_loadu(p);
			b = sse_loadu(vp);
			sse_storeu( p, sse_add(a, b));
			#else
			p[0] += vp[0];
			p[1] += vp[1];
			p[2] += vp[2];
			p[3] += vp[3];
			#endif
			// p[0] += vp[0];
			// p[1] += vp[1];
			// p[2] += vp[2];
			// p[3] += vp[3];
		}
		for (; j<nc; ++j)
		{
			*p+=*vp++;
			p++;
		}
	}
}

inline void wtk_matf_vecf_cpy(wtk_matf_t *dst, wtk_vecf_t *vec)
{
	int ncol=dst->col
	  , nrow=dst->row
	  , i;
	float *p=vec->p, *dstp=dst->p;
	size_t len_st=sizeof(float)*ncol;

	for (i=0; i<nrow; ++i, dstp+=ncol)
	{
		memcpy(dstp, p, len_st);
	}
}

void wtk_matdf_init_transpose(wtk_matdf_t *src, wtk_matdf_t *dst)
{
	double *p;
	int i,j;

	p=src->p;
	for(i=0;i<src->row;++i)
	{
		for(j=0;j<src->col;++j)
		{
			*(wtk_matf_at(dst,j,i))=*(p++);
		}
	}
}

void wtk_double_set_bound(double min, double max, double *p, int len)
{
	int i;
    for (i=0; i<len; ++i)
    {
        if (p[i]<min) { p[i]=min;}
        if (p[i]>max) { p[i]=max;}
    }
}
