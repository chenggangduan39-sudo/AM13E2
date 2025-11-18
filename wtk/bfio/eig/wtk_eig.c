#include "wtk_eig.h"

typedef struct {
    double real;
    double img;
} doublecomplex;

extern int zgeev_(char *jobvl, char *jobvr, int *n, doublecomplex *a, int *lda,
                  doublecomplex *w, doublecomplex *vl, int *ldvl,
                  doublecomplex *vr, int *ldvr, doublecomplex *work, int *lwork,
                  double *rwork, int *info);

wtk_eig_t* wtk_eig_new(int n)
{
    uint8_t *mem_buff = NULL;
    uint8_t *mem_buff2 = NULL;
    uint8_t *a, *w, *vr, *work, *rwork;
    size_t safe_n = n;
    size_t a_size = safe_n * safe_n * sizeof(fortran_doublecomplex);
    size_t w_size = safe_n * sizeof(fortran_doublecomplex);
    size_t vr_size =safe_n * safe_n * sizeof(fortran_doublecomplex);
    size_t rwork_size = 2 * safe_n * sizeof(double);
    size_t work_count = 0;
    DOUBLECOMPLEX_t work_size_query;
    fortran_int do_size_query = -1;
    fortran_int rv;
    size_t total_size = a_size + w_size  + vr_size + rwork_size;
	wtk_eig_t *eig;
	char jobvl='N';
	char jobvr='V';


	eig=(wtk_eig_t*)wtk_malloc(sizeof(wtk_eig_t));
	eig->A=NULL;
	eig->WORK=NULL;
    mem_buff = malloc(total_size);
    a = mem_buff;
    w = a + a_size;
    //vl = w + w_size;
    vr = w + w_size;
    rwork = vr + vr_size;
    zgeev_(&jobvl, &jobvr, &n,
                          (void *)a, &n, (void *)w,
                          NULL, &n, (void *)vr, &n,
                          (void *)&work_size_query, &do_size_query,
                          (void *)rwork,
                          &rv);
    if(rv!=0)
    {
    	goto end;
   	}
    work_count = (size_t) work_size_query.array[0];
    mem_buff2 = malloc(work_count*sizeof(fortran_doublecomplex));
    work = mem_buff2;
    eig->A = a;
    eig->WR = rwork;
    eig->WI = NULL;
    eig->VLR = NULL;
    eig->VRR = NULL;
    eig->VR = vr;
    eig->WORK = work;
    eig->W = w;
    eig->N = n;
    eig->LDA = n;
    eig->LDVL = n;
    eig->LDVR = n;
    eig->LWORK = (fortran_int)work_count;
    eig->JOBVL = jobvl;
    eig->JOBVR = jobvr;
 end:
 	 if(rv!=0)
 	 {
 		 wtk_eig_delete(eig);
 		 eig=NULL;
 	 }
	return eig;
}

void wtk_eig_delete(wtk_eig_t *eig)
{
	if(eig->A)
	{
		wtk_free(eig->A);
	}
	if(eig->WORK)
	{
		wtk_free(eig->WORK);
	}
	wtk_free(eig);
}


static int wtk_eig_process(wtk_eig_t *eig,wtk_complex_t *v)
{
	   fortran_int rv;
	   DOUBLECOMPLEX_t *x;
	   int i,j;
	   int n=eig->N;

	   x=(DOUBLECOMPLEX_t*)(eig->A);
	   for(i=0;i<n;++i)
	   {
		   for(j=0;j<n;++j)
		   {
			   //x[j][i]=v[i][j];
			   x[j*n+i].array[0]=v[i*n+j].a;
			   x[j*n+i].array[1]=v[i*n+j].b;
		   }
	   }
	   zgeev_(&eig->JOBVL, &eig->JOBVR,
	                         &eig->N, eig->A, &eig->LDA,
	                         eig->W,
	                         NULL, &eig->LDVL,
	                         eig->VR, &eig->LDVR,
	                         eig->WORK, &eig->LWORK,
	                         eig->WR, /* actually RWORK */
	                         &rv);
	   return rv;
}

int wtk_eig_process_maxfv(wtk_eig_t *eig,wtk_complex_t *v,wtk_complex_t *vector)
{
	int ret;
	DOUBLECOMPLEX_t *x;
	int i;
	int n=eig->N;
	int idx;
	float f;
	float t;

	ret=wtk_eig_process(eig,v);
	if(ret!=0){goto end;}
	x=(DOUBLECOMPLEX_t*)(eig->W);
	idx=0;
	f=x[0].npy.real;
	//wtk_debug("v[0]=%f\n",x[0].npy.real);
	for(i=1;i<n;++i)
	{
		//wtk_debug("v[%d]=%f\n",i,x[i].npy.real);
		t=x[i].npy.real;
		if(t>f)
		{
			f=t;
			idx=i;
		}
	}
	//wtk_debug("v[%d]=%f\n",idx,f);
	x=(DOUBLECOMPLEX_t*)(eig->VR);
	x+=idx*n;

	for(i=0;i<eig->N;++i)
	{
		vector[i].a=x[i].array[0];
		vector[i].b=x[i].array[1];
	}

	ret=0;
end:
	return ret;
}

int wtk_eig_process_fv(wtk_eig_t *eig,wtk_complex_t *v,wtk_complex_t *fv, float *val)
{
	int ret;
	DOUBLECOMPLEX_t *x;
	int i,j;
	int n=eig->N;

	ret=wtk_eig_process(eig,v);
	if(ret!=0){goto end;}

	x=(DOUBLECOMPLEX_t*)(eig->VR);
	for(i=0;i<n;++i)
	{
		for(j=0;j<n;++j)
		{
			fv[i*n+j].a=x[j].array[0];
			fv[i*n+j].b=x[j].array[1];
		}
		x+=n;
	}

	x=(DOUBLECOMPLEX_t*)(eig->W);

	for(i=0; i<n; ++i)
	{
		val[i]=x[i].npy.real;
	}

	ret=0;
end:
	return ret;
}

int wtk_eig_process_fv2(wtk_eig_t *eig,wtk_complex_t *v,wtk_complex_t *fv, float *val)
{
	int ret;
	DOUBLECOMPLEX_t *x, *x2;
	int i, j;
	int n=eig->N;
	int idx[512]={0};
	float f;
	float t;

	ret=wtk_eig_process(eig,v);
	if(ret!=0){goto end;}
	
	x=(DOUBLECOMPLEX_t*)(eig->W);

	for(i=0; i<n; ++i)
	{
		val[i]=x[i].npy.real;
		idx[i]=i;
	}
	for(i=0; i<n-1; ++i)
	{
		for(j=i+1; j<n; ++j)
		{
			if(val[i]<val[j])
			{
				f=val[i];
				val[i]=val[j];
				val[j]=f;

				t=idx[i];
				idx[i]=idx[j];
				idx[j]=t;
			}
		}
	}

	x=(DOUBLECOMPLEX_t*)(eig->VR);
	for(i=0; i<n; ++i)
	{
		x2=x+idx[i]*n;

		for(j=0; j<n; ++j)
		{
			fv[i*n+j].a=x2[j].array[0];
			fv[i*n+j].b=x2[j].array[1];
		}
	}

	ret=0;
end:
	return ret;
}


int wtk_eig_process2_matlab(wtk_eig_t *eig,wtk_complex_t *v,wtk_complex_t *fv)
{
	int ret;
	DOUBLECOMPLEX_t *x;
	int i,j;
	int n=eig->N;

	ret=wtk_eig_process(eig,v);
	if(ret!=0){goto end;}

	x=(DOUBLECOMPLEX_t*)(eig->VR);
	for(i=0;i<n;++i)
	{
		for(j=0;j<n;++j)
		{
			fv[j*n+i].a=x[j].array[0];
			fv[j*n+i].b=x[j].array[1];
		}
		x+=n;
	}

	ret=0;
end:
	return ret;
}

int wtk_eig_process_all_eig3(wtk_eig_t *eig,wtk_complex_t *v,wtk_complex_t *eig_v,float *rk_v,float *rk_vi)
{
	int ret;
	DOUBLECOMPLEX_t *x1,*x2;
	int i,j;
	int n=eig->N;
	//int idx;
	float f;
	float a,b;
	float f1;
	int id;

	ret=wtk_eig_process(eig,v);
	if(ret!=0){goto end;}
	x1=(DOUBLECOMPLEX_t*)(eig->W);
	x2=(DOUBLECOMPLEX_t*)(eig->VR);
	for(i=0;i<n;++i)
	{
		rk_v[i]=x1[i].npy.real;
		if(rk_vi)
		{
			rk_vi[i]=x1[i].npy.imag;
		}
		id=n-1;
		a=x2[id].array[0];
		b=x2[id].array[1];
		f1=1.0/sqrt(a*a+b*b);
		f=rk_v[i];
		//wtk_debug("f=%f a=%f b=%f\n",f,a,b);
		if(f!=fabs(f))
		{
			f1=-f1;
		}
		a=x2[id].array[0]*f1;
		b=-x2[id].array[1]*f1;
		//wtk_debug("v[%d]: %f %f+%f\n",i,f1,a,b);
		for(j=0;j<eig->N;++j)
		{
			//(a+bi)*(c+di)=(ac-bd)+i(ad+bc);
			eig_v[j].a=x2[j].array[0]*a - x2[j].array[1]*b;
			eig_v[j].b=x2[j].array[0]*b + x2[j].array[1]*a;
		}
		x2+=n;
		eig_v+=n;
	}
	ret=0;
end:
	return ret;
}


