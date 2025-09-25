#include "wtk_ann_wbop.h"
#include <math.h>

void wtk_ann_wb_cfg_print(wtk_ann_wb_cfg_t *cfg)
{
	printf("==================== cfg =======================\n");
	printf("hide rows:\t%d\n",cfg->hide_rows);
	printf("hide cols:\t%d\n",cfg->hide_cols);
	printf("out rows:\t%d\n",cfg->out_rows);
	printf("out cols:\t%d\n",cfg->out_cols);
}

void wtk_ann_wb_init(wtk_ann_wb_t *w)
{
	w->hid_w=0;
	w->out_w=0;
	w->hid_b=0;
	w->out_b=0;
}

int wtk_ann_wb_clean(wtk_ann_wb_t *w)
{
	if(w->hid_w)
	{
		wtk_matrix_delete(w->hid_w);
	}
	if(w->out_w)
	{
		wtk_matrix_delete(w->out_w);
	}
	if(w->hid_b)
	{
		wtk_matrix_delete(w->hid_b);
	}
	if(w->out_b)
	{
		wtk_matrix_delete(w->out_b);
	}
	return 0;
}

wtk_ann_wbop_t* wtk_ann_wbop_new(wtk_ann_wb_t *wb)
{
	wtk_ann_wbop_t *w;

	w=(wtk_ann_wbop_t*)wtk_malloc(sizeof(*w));
	w->wb=wb;
	w->hide_matrix=wtk_matrix_new(1,wtk_matrix_cols(wb->hid_w));
	w->out_matrix=wtk_matrix_new(1,wtk_matrix_cols(wb->out_w));
	return w;
}

void wtk_ann_wbop_delete(wtk_ann_wbop_t *w)
{
	wtk_matrix_delete(w->out_matrix);
	wtk_matrix_delete(w->hide_matrix);
	wtk_free(w);
}

void wtk_ann_softmax(float* a,int len)
{
	float max,sum;
	float *p,*e;

	max=wtk_math_max(a,len);
	sum=0;
	p=a;e=p+len;
	while(p<e)
	{
		*p=expf(*p-max);
		sum+=*p;
		++p;
	}
	sum=1.0f/sum;
	p=a;e=p+len;
	while(p<e)
	{
		*p*=sum;
		++p;
	}
}

void wtk_ann_softmax3(float* a,int len)
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
		a[i]*=sum;
	}
}

void wtk_ann_softmax4(float* a,int len)
{
	register float max,sum;
	register float *p,*e;

	max=wtk_math_max(a,len);
	sum=0;
	p=a-1;
	e=p+len;
	while(p<e-4)
	{
		++p;*p=expf(*p-max);sum+=*p;
		++p;*p=expf(*p-max);sum+=*p;
		++p;*p=expf(*p-max);sum+=*p;
		++p;*p=expf(*p-max);sum+=*p;
	}
	while(p<e)
	{
		++p;*p=expf(*p-max);sum+=*p;
	}

	sum=1.0f/sum;
	p=a-1;
	e=p+len;
	while(p<e-4)
	{
		*(++p)*=sum;
		*(++p)*=sum;
		*(++p)*=sum;
		*(++p)*=sum;
	}
	while(p<e)
	{
		*(++p)*=sum;
	}
}

void wtk_ann_sigmoid(float *f,int len)
{
	float *p;
	float *e;

	p=f;e=p+len;
	while(p<e)
	{
		*p=1.0/(1.0+expf(-*p));
		++p;
	}
}


void wtk_ann_sigmoid3(float *f,int len)
{
	int i;
	//float fx;

	for(i=0;i<len;++i)
	{
		//fx=f[i];
		f[i]=1.0/(1.0+expf(-f[i]));

		//wtk_debug("f[%d]=%f,%f\n",i,fx,f[i]);
	}
	//exit(0);
}

//Sigmoid C++ code
//For integers, set QN_EXP_A = 1512775
 #ifndef M_LN2
 #    define M_LN2  0.69314718055994530942
 #endif

 #define QN_EXP_A (1048576/M_LN2)
 #define QN_EXP_C 60801
 #ifdef QN_WORDS_BIGENDIAN
 #define QN_EXPQ(y) (qn_d2i.n.j = (int) (QN_EXP_A*(y)) + (1072693248 - QN_EXP_C), (y > -700.0f && y < 700.0f) ? qn_d2i.d : exp(y) )
 #else
 #define QN_EXPQ(y) (qn_d2i.n.i = (int) (QN_EXP_A*(y)) + (1072693248 - QN_EXP_C), (y > -700.0f && y < 700.0f) ? qn_d2i.d : exp(y) )
 #endif
 #define QN_EXPQ_WORKSPACE union { double d; struct { int j,i;} n; } qn_d2i; qn_d2i.d = 0.0;

float
qn_fe_sigmoid_f_f(float x)
{
 QN_EXPQ_WORKSPACE;
 return 1.0f/(1.0f + QN_EXPQ(-x));
}

float
qn_exprf(float x)
{
 QN_EXPQ_WORKSPACE;
 return QN_EXPQ(x);
}

void wtk_ann_sigmoid2(float *f,int len)
{
	int i;

	for(i=0;i<len;++i)
	{
		f[i]=qn_fe_sigmoid_f_f(f[i]);//1.0/(1.0+expf(-f[i]));
	}
}

void wtk_ann_sigmoid4(float *f,int len)
{
	QN_EXPQ_WORKSPACE;
	float *p;
	float *e;

	p=f;e=p+len;
	while(p<e)
	{
		qn_d2i.d = 0;
		*p=1.0f/(1.0f + QN_EXPQ(-*p));
		++p;
	}
}


void wtk_ann_softmax2(float* a,int len)
{
	float max,sum;
	int i;

	max=wtk_math_max(a,len);
	sum=0;
	for(i=0;i<len;++i)
	{
		a[i]=qn_exprf(a[i]-max);
		sum+=a[i];
	}
	sum=1.0f/sum;
	for(i=0;i<len;++i)
	{
		a[i]*=sum;
	}
}

void wtk_ann_wbop_process(wtk_ann_wbop_t *w,wtk_matrix_t *feature)
{
	wtk_matrix_t *m=w->hide_matrix;
	wtk_ann_wb_t *wb=w->wb;
	int n;

	//wtk_matrix_print(feature);
	//|1*hide_rows|*|hide_rows*hide_cols|=|1*hide_cols|
	wtk_matrix_multi(m,feature,wb->hid_w);
	//|1*hidw_clos|+bias
	//wtk_matrix_print(m);
	wtk_matrix_add(m,wb->hid_b);
	n=wtk_matrix_cols(m);
	wtk_ann_sigmoid2(m[1]+1,n);
	//|1*hide_cols|*|out_rows*out_cols|=|1*out_cols|
	wtk_matrix_multi(w->out_matrix,m,wb->out_w);
	wtk_matrix_add(w->out_matrix,wb->out_b);
	n=wtk_matrix_cols(w->out_matrix);
	//wtk_matrix_print(w->out_matrix);
	wtk_ann_softmax2(w->out_matrix[1]+1,n);
	//wtk_matrix_print(w->out_matrix);
	//exit(0);
}
