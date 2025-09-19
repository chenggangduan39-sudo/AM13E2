#include "wtk_ann_stream.h"
#include <math.h>

wtk_ann_stream_t* wtk_ann_stream_new(wtk_ann_normal_t *normal,
		wtk_ann_wb_t* wb)
{
	wtk_ann_stream_t *s;

	s=(wtk_ann_stream_t*)malloc(sizeof(*s));
	wtk_ann_stream_init(s,normal,wb);
	return s;
}

int wtk_ann_stream_delete(wtk_ann_stream_t *s)
{
	wtk_matrix_delete(s->dct_matrix);
	wtk_matrix_delete(s->hid_matrix);
	wtk_matrix_delete(s->out_matrix);
	free(s);
	return 0;
}

int wtk_ann_stream_init(wtk_ann_stream_t *s,wtk_ann_normal_t *normal,
		wtk_ann_wb_t* wb)
{
	int dct_cols,hid_cols,out_cols;

	dct_cols=wtk_matrix_rows(wb->hid_w);
	hid_cols=wtk_matrix_cols(wb->hid_w);
	out_cols=wtk_matrix_cols(wb->out_w);
	//wtk_debug("dct=%d,hid=%d,out=%d\n",dct_cols,hid_cols,out_cols);
	s->normal=normal;
	s->wb=wb;
	s->dct_matrix=wtk_matrix_new(1,dct_cols);
	s->hid_matrix=wtk_matrix_new(1,hid_cols);
	s->out_matrix=wtk_matrix_new(1,out_cols);
	return 0;
}

void wtk_ann_stream_pre_process(wtk_ann_stream_t *s,wtk_matrix_t *m)
{
	wtk_matrix_t *mean=s->normal->mean;
	wtk_matrix_t *bias=s->normal->bias;
	wtk_matrix_t *p=s->dct_matrix;
	int i,j;
	int rows,cols;

	rows=wtk_matrix_rows(m);
	cols=wtk_matrix_cols(m);
	for(i=1;i<=rows;++i)
	{
		for(j=1;j<=cols;++j)
		{
			p[1][(j-1)*rows+i]=(m[i][j]+mean[i][j])*bias[i][j];
			//wtk_debug("%f,%f,%f\n",p[1][k],mean[i][j],bias[i][j]);
		}
	}
}

void wtk_ann_stream_pre_process2(wtk_ann_stream_t *s,wtk_matrix_t *m)
{
	wtk_matrix_t *mean=s->normal->mean;
	wtk_matrix_t *bias=s->normal->bias;
	int i,j;
	int rows,cols;

	rows=wtk_matrix_rows(m);
	cols=wtk_matrix_cols(m);
	for(i=1;i<=rows;++i)
	{
		for(j=1;j<=cols;++j)
		{
			m[i][j]=(m[i][j]+mean[i][j])*bias[i][j];
			//wtk_debug("%f,%f,%f\n",m[i][j],mean[i][j],bias[i][j]);
		}
	}
}

void wtk_ann_softmax_vf(float* a,int len)
{
	float max,sum;
	int i;

	max=wtk_math_max(a,len);
	sum=0;
	for(i=0;i<len;++i)
	{
		a[i]=expf(a[i]-max);
		sum+=a[i];
	}
	sum=1.0f/sum;
	for(i=0;i<len;++i)
	{
		a[i]=log(a[i]*sum);
		//wtk_debug("%d,%f\n",i,a[i]);
	}
}


void wtk_ann_stream_feed(wtk_ann_stream_t *s)
{
	wtk_matrix_t *m=s->hid_matrix;
	wtk_matrix_t *o=s->out_matrix;
	wtk_ann_wb_t *wb=s->wb;
	int i,n;

	// |1*78| * |78*400| = |1*400|
	wtk_matrix_multi(m,s->dct_matrix,wb->hid_w);
	wtk_matrix_add(m,wb->hid_b);
	n=wtk_matrix_cols(m);
	for(i=1;i<=n;++i)
	{
		m[1][i]=1.0/(1.0+expf(-m[1][i]));
	}
	//|1*400| * |400 *138| = |1*138|
	wtk_matrix_multi(o,m,wb->out_w);
	wtk_matrix_add(o,wb->out_b);
	n=wtk_matrix_cols(o);
	wtk_ann_softmax_vf(o[1]+1,n);
}


int wtk_ann_stream_process(wtk_ann_stream_t *s,wtk_matrix_t *mul_mat,wtk_matrix_t *dct_mat,wtk_matrix_t *fea_mat)
{
	//do dct
	// |6*8| * |8*13| = 6*13
	wtk_matrix_multi(mul_mat,dct_mat,fea_mat);
	//do pre and transpose.
	//6*13 => 1 * 78
	wtk_ann_stream_pre_process(s,mul_mat);
	wtk_ann_stream_feed(s);
	return 0;
}

int wtk_ann_stream_do_merge(wtk_ann_stream_t *s)
{
	wtk_ann_stream_pre_process2(s,s->dct_matrix);
	wtk_ann_stream_feed(s);
	return 0;
}



