#include "wtk_complex.h" 

static int isinvalid(double f)
{
	return isnan(f) || isinf(f);
}

void wtk_complex_check(wtk_complex_t *c,int n)
{
	int i;

	for(i=0;i<n;++i)
	{
		if(isinvalid(c[i].a) || isinvalid(c[i].b))
		{
			wtk_debug("v[%d]=%f+%fj\n",i,c[i].a,c[i].b);
			exit(0);
		}
	}
}

wtk_complex_t wtk_complex_mul(wtk_complex_t *a, wtk_complex_t *b) {
	wtk_complex_t c;

	//a+bi c+di => ac-bd +i ad+bc
	c.a = a->a * b->a - a->b * b->b;
	c.b = a->a * b->b + a->b * b->a;
	return c;
}

double*** wtk_double_new_p3(int n1,int n2,int n3)
{
	double ***c;
	int i,j;

	c=(double***)wtk_calloc(n1,sizeof(double***));
	for(i=0;i<n1;++i)
	{
		c[i]=(double**)wtk_calloc(n2,sizeof(double*));
		for(j=0;j<n2;++j)
		{
			c[i][j]=(double*)wtk_calloc(n3,sizeof(double));
		}
	}
	return c;
}

void wtk_double_delete_p3(double ***pf,int n1,int n2)
{
	int i,j;

	for(i=0;i<n1;++i)
	{
		for(j=0;j<n2;++j)
		{
			wtk_free(pf[i][j]);
		}
		wtk_free(pf[i]);
	}
	wtk_free(pf);
}

void wtk_double_p3_set(double ***p,int n1,int n2,int n3,float v)
{
	int i,j,k;
	double **p2;
	double *p1;

	for(i=0;i<n1;++i)
	{
		p2=p[i];
		for(j=0;j<n2;++j)
		{
			p1=p2[j];
			for(k=0;k<n3;++k)
			{
				p1[k]=v;
			}
		}
	}
}

double** wtk_double_new_p2(int n1,int n2)
{
	double **c;
	int i;

	c=(double**)wtk_calloc(n1,sizeof(double*));
	for(i=0;i<n1;++i)
	{
		c[i]=(double*)wtk_calloc(n2,sizeof(double));
	}
	return c;
}

void wtk_double_delete_p2(double **pf,int n1)
{
	int i;

	for(i=0;i<n1;++i)
	{
		wtk_free(pf[i]);
	}
	wtk_free(pf);
}

void wtk_double_zero_p2(double **p,int n1,int n2)
{
	int i,nx;

	nx=n2*sizeof(double);
	for(i=0;i<n1;++i)
	{
		memset(p[i],0,nx);
	}
}

float*** wtk_float_new_p3(int n1,int n2,int n3)
{
	float ***c;
	int i,j;

	c=(float***)wtk_calloc(n1,sizeof(float***));
	for(i=0;i<n1;++i)
	{
		c[i]=(float**)wtk_calloc(n2,sizeof(float*));
		for(j=0;j<n2;++j)
		{
			c[i][j]=(float*)wtk_calloc(n3,sizeof(float));
		}
	}
	return c;
}

void wtk_float_delete_p3(float ***pf,int n1,int n2)
{
	int i,j;

	for(i=0;i<n1;++i)
	{
		for(j=0;j<n2;++j)
		{
			wtk_free(pf[i][j]);
		}
		wtk_free(pf[i]);
	}
	wtk_free(pf);
}

float** wtk_float_new_p2(int n1,int n2)
{
	float **c;
	int i;

	c=(float**)wtk_calloc(n1,sizeof(float*));
	for(i=0;i<n1;++i)
	{
		c[i]=(float*)wtk_calloc(n2,sizeof(float));
	}
	return c;
}

void wtk_float_zero_p2(float **p,int n1,int n2)
{
	int i,nx;

	nx=n2*sizeof(float);
	for(i=0;i<n1;++i)
	{
		memset(p[i],0,nx);
	}
}

void wtk_float_delete_p2(float **pf,int n1)
{
	int i;

	for(i=0;i<n1;++i)
	{
		wtk_free(pf[i]);
	}
	wtk_free(pf);
}

int wtk_float_bytes_p2(int n1,int n2)
{
	int cnt;

	cnt=n1*sizeof(float*)+n1*n2*sizeof(float);
	return cnt;
}

short** wtk_short_new_p2(int n1,int n2)
{
	short **c;
	int i;

	c=(short**)wtk_calloc(n1,sizeof(short*));
	for(i=0;i<n1;++i)
	{
		c[i]=(short*)wtk_calloc(n2,sizeof(short));
	}
	return c;
}

void wtk_short_zero_p2(short **ps,int n1,int n2)
{
	int i,nx;

	nx=n2*sizeof(short);
	for(i=0;i<n1;++i)
	{
		memset(ps[i],0,nx);
	}
}

void wtk_short_delete_p2(short **ps,int n1)
{
	int i;

	for(i=0;i<n1;++i)
	{
		wtk_free(ps[i]);
	}
	wtk_free(ps);
}

wtk_complex_t**** wtk_complex_new_p4(int n1,int n2,int n3,int n4)
{
	wtk_complex_t ****c;
	int i,j,k;

	c=(wtk_complex_t****)wtk_calloc(n1,sizeof(wtk_complex_t****));
	for(i=0;i<n1;++i)
	{
		c[i]=(wtk_complex_t***)wtk_calloc(n2,sizeof(wtk_complex_t**));
		for(j=0;j<n2;++j)
		{
			c[i][j]=(wtk_complex_t**)wtk_calloc(n3,sizeof(wtk_complex_t*));
			for(k=0;k<n3;++k)
			{
				c[i][j][k]=(wtk_complex_t*)wtk_calloc(n4,sizeof(wtk_complex_t));
			}
		}
	}
	return c;
}

void wtk_complex_delete_p4(wtk_complex_t ****p,int n1,int n2,int n3)
{
	int i,j,k;

	for(i=0;i<n1;++i)
	{
		for(j=0;j<n2;++j)
		{
			for(k=0;k<n3;++k)
			{
				wtk_free(p[i][j][k]);
			}
			wtk_free(p[i][j]);
		}
		wtk_free(p[i]);
	}
	wtk_free(p);
}

wtk_complex_t*** wtk_complex_new_p3(int n1,int n2,int n3)
{
	wtk_complex_t ***c;
	wtk_complex_t **p;
	int i,j;

	c=(wtk_complex_t***)wtk_calloc(n1,sizeof(wtk_complex_t***));
	for(i=0;i<n1;++i)
	{
		p=c[i]=(wtk_complex_t**)wtk_calloc(n2,sizeof(wtk_complex_t*));
		for(j=0;j<n2;++j)
		{
			//c[i][j]=(wtk_complex_t*)wtk_calloc(n3,sizeof(wtk_complex_t));
			p[j]=(wtk_complex_t*)wtk_calloc(n3,sizeof(wtk_complex_t));
		}
	}
	return c;
}

void wtk_complex_zero_p3(wtk_complex_t ***p,int n1,int n2,int n3)
{
	int i,j;
	int t;
	wtk_complex_t **p2;

	t=n3*sizeof(wtk_complex_t);
	for(i=0;i<n1;++i)
	{
		p2=p[i];
		for(j=0;j<n2;++j)
		{
			memset(p2[j],0,t);
		}
	}
}

void wtk_complex_delete_p3(wtk_complex_t ***p3,int n1,int n2)
{
	int i,j;

	for(i=0;i<n1;++i)
	{
		for(j=0;j<n2;++j)
		{
			wtk_free(p3[i][j]);
		}
		wtk_free(p3[i]);
	}
	wtk_free(p3);
}

void wtk_complex_cpy_p3(wtk_complex_t ***dst,wtk_complex_t ***src,int n1,int n2,int n3)
{
	int i,j;
	int t;

	t=n3*sizeof(wtk_complex_t);
	for(i=0;i<n1;++i)
	{
		for(j=0;j<n2;++j)
		{
			memcpy(dst[i][j],src[i][j],t);
		}
	}
}

wtk_complex_t** wtk_complex_new_p2(int n1,int n2)
{
	wtk_complex_t **c;
	int i;

	c=(wtk_complex_t**)wtk_calloc(n1,sizeof(wtk_complex_t*));
	for(i=0;i<n1;++i)
	{
		c[i]=(wtk_complex_t*)wtk_calloc(n2,sizeof(wtk_complex_t));
	}
	return c;
}

void wtk_complex_zero_p2(wtk_complex_t **p,int n1,int n2)
{
	int i,t;

	t=n2*sizeof(wtk_complex_t);
	for(i=0;i<n1;++i)
	{
		memset(p[i],0,t);
	}
}

void wtk_complex_delete_p2(wtk_complex_t **p2,int n1)
{
	int i;

	for(i=0;i<n1;++i)
	{
		wtk_free(p2[i]);
	}
	wtk_free(p2);
}

int wtk_complex_bytes_p2(int n1,int n2)
{
        int cnt;

        cnt=n1*sizeof(wtk_complex_t*);
        cnt+=n1*n2*sizeof(wtk_complex_t);
        return cnt;
}

void wtk_complex_cpy_p2(wtk_complex_t **dst,wtk_complex_t **src,int n1,int n2)
{
	int i;
	int t;

	t=n2*sizeof(wtk_complex_t);
	for(i=0;i<n1;++i)
	{
		memcpy(dst[i],src[i],t);
	}
}

void wtk_complex_zero(wtk_complex_t *c,int n)
{
	memset(c,0,n*sizeof(wtk_complex_t));
}

//a是nx x nx*2维的
int wtk_complex_invx4(wtk_complex_t *input,wtk_dcomplex_t *a,int nx,wtk_complex_t *b,int sym)
{
	int i,j,k;
	int nx2=nx<<1;
	wtk_dcomplex_t *dc,*dc2,*dc3,tmp;
	wtk_dcomplex_t e={1,0},z={0,0};
	wtk_complex_t *c,*c2;
	double mx,fa,fb,fa2,fb2,f;
	int mi,k2,k3;

	dc=a;
	c=input;
	for(i=0;i<nx;++i)
	{
		for(j=0;j<nx;++j,++dc,++c)
		{
			dc->a=c->a;
			dc->b=c->b;
			if(j==i)
			{
				*(dc+nx)=e;
			}else
			{
				*(dc+nx)=z;
			}
		}
		dc+=nx;
	}

	for(k=0;k<nx;++k)
	{
		dc=a+k*nx2+k;
		mx=0;
		mi=k;
		for(i=k;i<nx;++i)
		{
			f=dc->a*dc->a+dc->b*dc->b;
			if(f>mx)
			{
				mx=f;
				mi=i;
			}
			dc+=nx2;
		}
		if(mx==0.0)
		{
			return -1;
		}
		if(mi!=k)
		{
			k2=k*nx2+k;
			k3=mi*nx2+k;
			for(i=k;i<nx2;++i,++k2,++k3)
			{
				tmp=*(a+k2);
				*(a+k2)=*(a+k3);
				*(a+k3)=tmp;
			}
		}
		mx=1.0/mx;
		dc3=a+k*nx2+k;
		fa=dc3->a*mx;
		fb=dc3->b*mx;
		dc2=a+k;
		for(i=0;i<nx;++i)
		{
			if(i==k)
			{
				dc2+=nx2;
				continue;
			}
			dc=dc3;
			fa2=fa*dc2->a+fb*dc2->b;
			fb2=fa*dc2->b-fb*dc2->a;
			for(j=k;j<nx2;++j,++dc2,++dc)
			{
				dc2->a-=dc->a*fa2-dc->b*fb2;
				dc2->b-=dc->a*fb2+dc->b*fa2;
			}
			dc2+=k;
		}
	}

	if(sym)
	{
		dc=dc2=a;
		c=b;
		for(i=0,dc2+=nx;i<nx;++i)
		{
			fa=dc->a;
			fb=dc->b;
			mx=fa*fa+fb*fb;
			if(mx==0.0)
			{
				return -1;
			}
			mx=1/mx;
			fa*=mx;
			fb*=mx;
			dc2+=i;
			c=b+nx*i+i;
			c2=c+nx;
			for(j=i;j<nx;++j,++dc2,++c)
			{
				c->a=dc2->a*fa+dc2->b*fb;
				c->b=dc2->b*fa-dc2->a*fb;
				if(1.0/c->a == 0 || 1.0/c->b == 0)
				{
					return -1;
				}
				if(i!=j)
				{
					c2->a=c->a;
					c2->b=-c->b;
					c2+=nx;
				}
			}
			dc2+=nx;
			dc+=nx2+1;
		}

		return 0;
	}

	dc=dc2=a;
	for(i=0,dc2+=nx;i<nx;++i)
	{
		fa=dc->a;
		fb=dc->b;
		mx=fa*fa+fb*fb;
		if(mx==0.0)
		{
			return -1;
		}
		mx=1/mx;
		fa*=mx;
		fb*=mx;
		for(j=0;j<nx;++j,++dc2,++b)
		{
			b->a=dc2->a*fa+dc2->b*fb;
			b->b=dc2->b*fa-dc2->a*fb;
		}
		dc2+=nx;
		dc+=nx2+1;
	}

	return 0;
}


int wtk_complex_invx_and_det(wtk_complex_t *input,wtk_dcomplex_t *a,int nx,wtk_complex_t *b,int sym,double *det)
{
	int i,j,k;
	int nx2=nx<<1;
	wtk_dcomplex_t *dc,*dc2,*dc3,tmp;
	wtk_dcomplex_t e={1,0},z={0,0};
	wtk_complex_t *c,*c2;
	double f,mx,fa,fb,fa2,fb2;
	int mi,k2,k3;
	double fa3,fb3;

	dc=a;
	c=input;
	for(i=0;i<nx;++i)
	{
		for(j=0;j<nx;++j,++dc,++c)
		{
			dc->a=c->a;
			dc->b=c->b;
			if(j==i)
			{
				*(dc+nx)=e;
			}else
			{
				*(dc+nx)=z;
			}
		}
		dc+=nx;
	}

	for(k=0;k<nx;++k)
	{
		dc=a+k*nx2+k;
		mx=0;
		mi=k;
		for(i=k;i<nx;++i)
		{
			f=dc->a*dc->a+dc->b*dc->b;
			if(f>mx)
			{
				mx=f;
				mi=i;
			}
			dc+=nx2;
		}
		if(mx==0.0)
		{
			*det=0;
			return -1;
		}
		if(mi!=k)
		{
			k2=k*nx2+k;
			k3=mi*nx2+k;
			for(i=k;i<nx2;++i,++k2,++k3)
			{
				tmp=*(a+k2);
				*(a+k2)=*(a+k3);
				*(a+k3)=tmp;
			}
		}
		mx=1.0/mx;
		dc3=a+k*nx2+k;
		fa=dc3->a*mx;
		fb=dc3->b*mx;
		dc2=a+k;
		for(i=0;i<nx;++i)
		{
			if(i==k)
			{
				dc2+=nx2;
				continue;
			}
			dc=dc3;
			fa2=fa*dc2->a+fb*dc2->b;
			fb2=fa*dc2->b-fb*dc2->a;
			for(j=k;j<nx2;++j,++dc2,++dc)
			{
				dc2->a-=dc->a*fa2-dc->b*fb2;
				dc2->b-=dc->a*fb2+dc->b*fa2;
			}
			dc2+=k;
		}
	}

	fa3=fb3=0;
	if(sym)
	{
		dc=dc2=a;
		c=b;
		for(i=0,dc2+=nx;i<nx;++i)
		{
			fa=dc->a;
			fb=dc->b;
			mx=fa*fa+fb*fb;
			if(mx==0.0)
			{
				*det=0;
				return -1;
			}
			if(i==0)
			{
				fa3=fa;
				fb3=fb;
			}else
			{
				f=fa3;
				fa3=fa3*fa-fb3*fb;
				fb3=f*fb+fb3*fa;
			}
			mx=1/mx;
			fa*=mx;
			fb*=mx;
			dc2+=i;
			c=b+nx*i+i;
			c2=c+nx;
			for(j=i;j<nx;++j,++dc2,++c)
			{
				c->a=dc2->a*fa+dc2->b*fb;
				c->b=dc2->b*fa-dc2->a*fb;
				if(i!=j)
				{
					c2->a=c->a;
					c2->b=-c->b;
					c2+=nx;
				}
			}
			dc2+=nx;
			dc+=nx2+1;
		}

		*det=sqrt(fa3*fa3+fb3*fb3);
		return 0;
	}

	dc=dc2=a;
	for(i=0,dc2+=nx;i<nx;++i)
	{
		fa=dc->a;
		fb=dc->b;
		mx=fa*fa+fb*fb;
		if(mx==0.0)
		{
			return -1;
		}
		if(i==0)
		{
			fa3=fa;
			fb3=fb;
		}else
		{
			f=fa3;
			fa3=fa3*fa-fb3*fb;
			fb3=f*fb+fb3*fa;
		}
		mx=1/mx;
		fa*=mx;
		fb*=mx;
		for(j=0;j<nx;++j,++dc2,++b)
		{
			b->a=dc2->a*fa+dc2->b*fb;
			b->b=dc2->b*fa-dc2->a*fb;
		}
		dc2+=nx;
		dc+=nx2+1;
	}

	*det=sqrt(fa3*fa3+fb3*fb3);
	return 0;
}

// a[n1*(n1+1)]  b[n1*1]
int wtk_complex_guass_elimination_p1(wtk_complex_t *input,wtk_complex_t *b,wtk_dcomplex_t *a,int nx,wtk_complex_t *out)
{
	int i,j,k;
	int nx2=nx+1;
	wtk_dcomplex_t *dc,*dc2,*dc3,tmp;
	wtk_complex_t *c,*c2;
	double mx,fa,fb,fa2,fb2,f;
	int mi,k2,k3;

	dc=a;
	c=input;
	c2=b;
	for(i=0;i<nx;++i,++c2)
	{
		for(j=0;j<nx;++j,++dc,++c)
		{
			dc->a=c->a;
			dc->b=c->b;
		}
		dc->a=c2->a;
		dc->b=c2->b;
		++dc;
	}

	for(k=0;k<nx;++k)
	{
		dc=a+k*nx2+k;
		mx=0;
		mi=k;
		for(i=k;i<nx;++i)
		{
			f=dc->a*dc->a+dc->b*dc->b;
			if(f>mx)
			{
				mx=f;
				mi=i;
			}
			dc+=nx2;
		}
		if(mx==0.0)
		{
			return -1;
		}
		if(mi!=k)
		{
			k2=k*nx2+k;
			k3=mi*nx2+k;
			for(i=k;i<nx2;++i,++k2,++k3)
			{
				tmp=*(a+k2);
				*(a+k2)=*(a+k3);
				*(a+k3)=tmp;
			}
		}
		mx=1.0/mx;
		dc3=a+k*nx2+k;
		fa=dc3->a*mx;
		fb=dc3->b*mx;
		dc2=a+k;
		for(i=0;i<nx;++i)
		{
			if(i==k)
			{
				dc2+=nx2;
				continue;
			}
			dc=dc3;
			fa2=fa*dc2->a+fb*dc2->b;
			fb2=fa*dc2->b-fb*dc2->a;
			for(j=k;j<nx2;++j,++dc2,++dc)
			{
				dc2->a-=dc->a*fa2-dc->b*fb2;
				dc2->b-=dc->a*fb2+dc->b*fa2;
			}
			dc2+=k;
		}
	}

	dc=dc2=a;
	for(i=0,dc2+=nx;i<nx;++i)
	{
		fa=dc->a;
		fb=dc->b;
		mx=fa*fa+fb*fb;
		if(mx==0.0)
		{
			return -1;
		}
		mx=1/mx;
		fa*=mx;
		fb*=mx;

		out->a=dc2->a*fa+dc2->b*fb;
		out->b=dc2->b*fa-dc2->a*fb;
		++out;
		
		dc2+=nx+1;
		dc+=nx2+1;
	}

	return 0;
}

// a[nx*(2*nx)]  b[nx*nx]
int wtk_complex_guass_elimination_p2(wtk_complex_t *input,wtk_complex_t *b,wtk_dcomplex_t *a,int nx,wtk_complex_t *out)
{
	int i,j,k;
	int nx2=nx*2;
	wtk_dcomplex_t *dc,*dc2,*dc3,tmp;
	wtk_complex_t *c,*c2;
	double mx,fa,fb,fa2,fb2,f;
	int mi,k2,k3;

	dc=a;
	c=input;
	c2=b;
	for(i=0;i<nx;++i)
	{
		for(j=0;j<nx;++j,++dc,++c)
		{
			dc->a=c->a;
			dc->b=c->b;
		}
		for(j=0;j<nx;++j,++dc,++c2)
		{
			dc->a=c2->a;
			dc->b=c2->b;
		}
	}

	for(k=0;k<nx;++k)
	{
		dc=a+k*nx2+k;
		mx=0;
		mi=k;
		for(i=k;i<nx;++i)
		{
			f=dc->a*dc->a+dc->b*dc->b;
			if(f>mx)
			{
				mx=f;
				mi=i;
			}
			dc+=nx2;
		}
		if(mx==0.0)
		{
			return -1;
		}
		if(mi!=k)
		{
			k2=k*nx2+k;
			k3=mi*nx2+k;
			for(i=k;i<nx2;++i,++k2,++k3)
			{
				tmp=*(a+k2);
				*(a+k2)=*(a+k3);
				*(a+k3)=tmp;
			}
		}
		mx=1.0/mx;
		dc3=a+k*nx2+k;
		fa=dc3->a*mx;
		fb=dc3->b*mx;
		dc2=a+k;
		for(i=0;i<nx;++i)
		{
			if(i==k)
			{
				dc2+=nx2;
				continue;
			}
			dc=dc3;
			fa2=fa*dc2->a+fb*dc2->b;
			fb2=fa*dc2->b-fb*dc2->a;
			for(j=k;j<nx2;++j,++dc2,++dc)
			{
				dc2->a-=dc->a*fa2-dc->b*fb2;
				dc2->b-=dc->a*fb2+dc->b*fa2;
			}
			dc2+=k;
		}
	}

	dc=dc2=a;
	for(i=0,dc2+=nx;i<nx;++i)
	{
		fa=dc->a;
		fb=dc->b;
		mx=fa*fa+fb*fb;
		if(mx==0.0)
		{
			return -1;
		}
		mx=1/mx;
		fa*=mx;
		fb*=mx;

		for(j=0;j<nx;++j,++out,++dc2)
		{
			out->a=dc2->a*fa+dc2->b*fb;
			out->b=dc2->b*fa-dc2->a*fb;
		}

		dc2+=nx;
		dc+=nx2+1;
	}

	return 0;
}

int wtk_complex_guass_elimination_p1_f(wtk_complex_t *input,wtk_complex_t *b,wtk_complex_t *a,int nx,wtk_complex_t *out)
{
	int i,j,k;
	int nx2=nx+1;
	wtk_complex_t *dc,*dc2,*dc3,tmp;
	wtk_complex_t *c,*c2;
	float mx,fa,fb,fa2,fb2,f;
	int mi,k2,k3;

	dc=a;
	c=input;
	c2=b;
	for(i=0;i<nx;++i,++c2)
	{
		for(j=0;j<nx;++j,++dc,++c)
		{
			dc->a=c->a;
			dc->b=c->b;
		}
		dc->a=c2->a;
		dc->b=c2->b;
		++dc;
	}

	for(k=0;k<nx;++k)
	{
		dc=a+k*nx2+k;
		mx=0;
		mi=k;
		for(i=k;i<nx;++i)
		{
			f=dc->a*dc->a+dc->b*dc->b;
			if(f>mx)
			{
				mx=f;
				mi=i;
			}
			dc+=nx2;
		}
		if(mx==0.0)
		{
			return -1;
		}
		if(mi!=k)
		{
			k2=k*nx2+k;
			k3=mi*nx2+k;
			for(i=k;i<nx2;++i,++k2,++k3)
			{
				tmp=*(a+k2);
				*(a+k2)=*(a+k3);
				*(a+k3)=tmp;
			}
		}
		mx=1.0/mx;
		dc3=a+k*nx2+k;
		fa=dc3->a*mx;
		fb=dc3->b*mx;
		dc2=a+k;
		for(i=0;i<nx;++i)
		{
			if(i==k)
			{
				dc2+=nx2;
				continue;
			}
			dc=dc3;
			fa2=fa*dc2->a+fb*dc2->b;
			fb2=fa*dc2->b-fb*dc2->a;
			for(j=k;j<nx2;++j,++dc2,++dc)
			{
				dc2->a-=dc->a*fa2-dc->b*fb2;
				dc2->b-=dc->a*fb2+dc->b*fa2;
			}
			dc2+=k;
		}
	}

	dc=dc2=a;
	for(i=0,dc2+=nx;i<nx;++i)
	{
		fa=dc->a;
		fb=dc->b;
		mx=fa*fa+fb*fb;
		if(mx==0.0)
		{
			return -1;
		}
		mx=1/mx;
		fa*=mx;
		fb*=mx;

		out->a=dc2->a*fa+dc2->b*fb;
		out->b=dc2->b*fa-dc2->a*fb;
		++out;
		
		dc2+=nx+1;
		dc+=nx2+1;
	}

	return 0;
}

static float wtk_complex_itereig_mul(wtk_complex_t *w,wtk_complex_t *u,wtk_complex_t *v,int n)
{
	int i,j;
	float fa,fb;
	float max=0;

	for(i=0;i<n;++i)
	{
		fa=fb=0;
		for(j=0;j<n;++j)
		{
			//(a+bi)*(c+di)=(ac-bd)+i(ad+bc);
			fa+=w->a*u[j].a - w->b*u[j].b;
			fb+=w->a*u[j].b + w->b*u[j].a;
			++w;
		}
		v[i].a=fa;
		v[i].b=fb;
		fa=fa*fa+fb*fb;
		if(fa>max)
		{
			max=fa;
		}
	}
	return sqrt(max);
}

void wtk_complex_itereig(wtk_complex_t *w,wtk_complex_t *u,wtk_complex_t *v,int n,float eps,int max_iter)
{
	float lst_max=10;
	int i;
	float max;
	int k=0;

	for(i=0;i<n;++i)
	{
		u[i].a=1;
		u[i].b=0;
	}
	max=wtk_complex_itereig_mul(w,u,v,n);
	while((lst_max-max)>=eps)
	{
		++k;
		if(k>max_iter)
		{
			return;
		}
		lst_max=max;
		max=1.0/max;
		for(i=0;i<n;++i)
		{
			u[i].a=v[i].a*max;
			u[i].b=v[i].b*max;
		}
		max=wtk_complex_itereig_mul(w,u,v,n);
		///wtk_debug("max=%e lst=%e\n",max,lst_max);
		//++k;
	}
}

wtk_dcomplex_t*** wtk_dcomplex_new_p3(int n1,int n2,int n3)
{
	wtk_dcomplex_t ***c;
	int i,j;

	c=(wtk_dcomplex_t***)wtk_calloc(n1,sizeof(wtk_dcomplex_t***));
	for(i=0;i<n1;++i)
	{
		c[i]=(wtk_dcomplex_t**)wtk_calloc(n2,sizeof(wtk_dcomplex_t*));
		for(j=0;j<n2;++j)
		{
			c[i][j]=(wtk_dcomplex_t*)wtk_calloc(n3,sizeof(wtk_dcomplex_t));
		}
	}
	return c;
}

void wtk_dcomplex_zero_p3(wtk_dcomplex_t ***p,int n1,int n2,int n3)
{
	int i,j;
	int t;
	wtk_dcomplex_t **p2;

	t=n3*sizeof(wtk_dcomplex_t);
	for(i=0;i<n1;++i)
	{
		p2=p[i];
		for(j=0;j<n2;++j)
		{
			memset(p2[j],0,t);
		}
	}
}

void wtk_dcomplex_delete_p3(wtk_dcomplex_t ***p3,int n1,int n2)
{
	int i,j;

	for(i=0;i<n1;++i)
	{
		for(j=0;j<n2;++j)
		{
			wtk_free(p3[i][j]);
		}
		wtk_free(p3[i]);
	}
	wtk_free(p3);
}

void wtk_dcomplex_cpy_p3(wtk_dcomplex_t ***dst,wtk_complex_t ***src,int n1,int n2,int n3)
{
	int i,j,k;
	//wtk_complex_t **src1,*src2;
	//wtk_dcomplex_t **dst1,*dst2;

	for(i=0;i<n1;++i)
	{
		//src1=src[i];
		for(j=0;j<n2;++j)
		{
			//src2=src1[j];
			for(k=0;k<n3;++k)
			{
				dst[i][j][k].a=src[i][j][k].a;
				dst[i][j][k].b=src[i][j][k].b;
			}
		}
	}
}

wtk_dcomplex_t** wtk_dcomplex_new_p2(int n1,int n2)
{
	wtk_dcomplex_t **c;
	int i;

	c=(wtk_dcomplex_t**)wtk_calloc(n1,sizeof(wtk_dcomplex_t*));
	for(i=0;i<n1;++i)
	{
		c[i]=(wtk_dcomplex_t*)wtk_calloc(n2,sizeof(wtk_dcomplex_t));
	}
	return c;
}

void wtk_dcomplex_zero_p2(wtk_dcomplex_t **p,int n1,int n2)
{
	int i;
	int t;

	t=n2*sizeof(wtk_dcomplex_t);
	for(i=0;i<n1;++i)
	{
		memset(p[i],0,t);
	}
}

void wtk_dcomplex_delete_p2(wtk_dcomplex_t **p2,int n1)
{
	int i;

	for(i=0;i<n1;++i)
	{
		wtk_free(p2[i]);
	}
	wtk_free(p2);
}

void wtk_dcomplex_cpy_p2(wtk_dcomplex_t **dst,wtk_dcomplex_t **src,int n1,int n2)
{
	int i;
	int n;

	n=n2*sizeof(wtk_dcomplex_t);
	for(i=0;i<n1;++i)
	{
		memcpy(dst[i],src[i],n);
	}
}

void wtk_complex_dff_ffts(wtk_complex_t* linear,int size,int start,int step)
{
	int i,j,k;
	wtk_complex_t* dft=(wtk_complex_t*)malloc(sizeof(wtk_complex_t)*size);
	//wtk_complex_t mulf;
	wtk_complex_t *p;
	float fa,fb;

	for(i=0;i<size;i++)	//频率循环
	{
		dft[i].a=0;
		dft[i].b=0;
		p=linear+start;
		for(k=0;k<size;k++)	//时域循环
		{
			fa=cosf(2*PI*i*k/size);
			fb=(-1)*sinf(2*PI*i*k/size);
			//a+bi c+di => ac-bd +i ad+bc
			dft[i].a += fa * p->a - fb * p->b;
			dft[i].b+= fa * p->b + fb * p->a;
			p+=step;
//			{
//				//mulf=wtk_dcomplex_mul(linear+k*step+start,&mulf);
//				mulf=wtk_dcomplex_mul(&mulf,linear+k*step+start);
//				dft[i].a+=mulf.a;
//				dft[i].b+=mulf.b;
//			}
		}
	}
	for(i=start,j=0;j<size;i+=step,j++)
	{
		linear[i].a=dft[j].a;
		linear[i].b=dft[j].b;
	}
	free(dft);
}

/* fpix为时域序列，用复数表示，可支持复数序列变换*/
/* step为fpix数组中序列元素的间隔，做二维fft的接口用，若只进行以为fft可直接设为1 */
void wtk_complex_ffts(wtk_complex_t* fpix,int fu_step,int size,int start)
{
	int h=0,sizemin=size,step=1;
	int i,j,k,z;
	wtk_complex_t *W;
	wtk_complex_t *fft;
	wtk_complex_t m1,m2,mulf;
	int index;

	if(size%2==0)
	{
		W=(wtk_complex_t*)malloc(sizeof(wtk_complex_t)*(size/2));
	}else
	{
		wtk_complex_dff_ffts(fpix,size,start,fu_step);
		return;
	}
	fft=(wtk_complex_t*)malloc(sizeof(wtk_complex_t)*size);
	/*计算Wn*/
	for(j=0;j<size/2;j++)
	{
		W[j].a=cosf(2*PI*j/size);
		W[j].b=(-1)*sinf(2*PI*j/size);
	}
	i=size;
	while(i%2==0)
	{
		h++;
		i/=2;
		sizemin=i;
		step*=2;
	}
	/*初始化fft*/
	//int n=step;
	//index=0;
	for(i=0;i<step;i++)	//每块,i为起始编号
	{
		index=0;
		for(j=0;j<h;j++)
		{
			index=index<<1;
			index+=(i>>j)&0x01;
		}
		index=index*sizemin;
		k=index;
		for(j=0,z=i;j<sizemin;j++,z+=step)
		{
			fft[index]=fpix[z*fu_step+start];
			index++;
		}
		wtk_complex_dff_ffts(fft,sizemin,k,1);
	}
	for(i=1;h>0;h--,i++)	//每层
	{
		for(j=0;j<step/2;j++)	//每块,step/2 变化完的块数
		{
			for(k=j*sizemin*2,z=0;z<sizemin;z++,k++)
			{
				m1=fft[k];
				m2=fft[k+sizemin];
				mulf=wtk_complex_mul(&m2,W+z*step/2);
				fft[k].a=m1.a+mulf.a;
				fft[k].b=m1.b+mulf.b;
				fft[k+sizemin].a=m1.a-mulf.a;
				fft[k+sizemin].b=m1.b-mulf.b;
			}
		}
		sizemin*=2;
		step/=2;
	}
	for(i=0;i<size;i++)
	{
		fpix[i*fu_step+start]=fft[i];
	}
	free(fft);
	wtk_free(W);
}

void wtk_complex_fft(wtk_complex_t* fpix,int n)
{
	wtk_complex_ffts(fpix,1,n,0);
}


//IFFT(X)=(1/N)conj(FFT(conj(X));
void wtk_complex_ifft(wtk_complex_t* f,wtk_complex_t *x,int n)
{
	int i;
	float fx=1.0/n;

	for(i=0;i<n;++i)
	{
		x[i].a=f[i].a;
		x[i].b=-f[i].b;
	}
	wtk_complex_fft(x,n);
	for(i=0;i<n;++i)
	{
		x[i].a*=fx;
		x[i].b*=-fx;
	}
}

void wtk_complex_exp(wtk_complex_t *f){
	// 欧拉公式：e^(a+bi) = e^a * (cos(b) + i*sin(b))
	float exp_real = expf(f->a);
	f->a = exp_real * cosf(f->b);
	f->b = exp_real * sinf(f->b);
}
