#include "wtk_eig_qr.h"

int wtk_qr_process(int n, double *b, double *c, double *q, double eps, int l)
{
    int i,j,k,m,it,u,v;
    double d,f,h,g,p,r,e,s;

    c[n-1]=0.0;
    d=0.0;
    f=0.0;
    for(j=0;j<n;++j)
    {
        it=0;
        h=eps*(fabs(b[j])+fabs(c[j]));
        if(h>d)
        {
        	d=h;
        }
        m=j;
        while((m<=n-1)&&(fabs(c[m])>d))
        {
        	m++;
        }
        if(m!=j)
        {
            do{
                if(it==l)
                {
                	return 1;
                }
                it=it+1;
                g=b[j];
                p=(b[j+1]-g)/(2.0*c[j]);
                r=sqrt(p*p+1.0);
                if(p>=0.0)
                {
                    b[j]=c[j]/(p+r);
                }else
                {
                    b[j]=c[j]/(p-r);
                }
                h=g-b[j];
                for(i=j+1;i<n;++i)
                {
                	b[i]=b[i]-h;
                }
                f=f+h;
                p=b[m];
                e=1.0;
                s=0.0;
                for(i=m-1;i>=j;--i)
                {
                    g=e*c[i];
                    h=e*p;
                    if(fabs(p)>=fabs(c[i]))
                    {
                        e=c[i]/p;
                        r=sqrt(e*e+1.0);
                        c[i+1]=s*p*r;
                        s=e/r;
                        e=1.0/r;
                    }else
					{
                        e=p/c[i];
                        r=sqrt(e*e+1.0);
                        c[i+1]=s*c[i]*r;
                        s=1.0/r;
                        e=e/r;
                    }
                    p=e*b[i]-s*g;
                    b[i+1]=h+s*(e*g+s*b[i]);
                    for(k=0;k<n;++k)
                    {
                        u=k*n+i+1;
                        v=u-1;
                        h=q[u];
                        q[u]=s*q[v]+e*h;
                        q[v]=e*q[v]-s*h;
                    }
                }
                c[j]=s*p;
                b[j]=e*p;
            }while(fabs(c[j])>d);
        }
        b[j]=b[j]+f;
    }
    return 0;
}

void wtk_householder_process(double *q, int n,double *b, double *c)
{
    int i,j,k,u;
    double h,f,g,h2;

    for(i=n-1;i>0;--i)
    {
        h=0.0;
        if(i>1)
        {
        	for(k=0;k<i;++k)
        	{
        		u=i*n+k;
        		h=h+q[u]*q[u];
        	}
        }
        if(h==0.0)
        {
            c[i]=0.0;
            if(i==1)
            {
            	c[i]=q[i*n+i-1];
            }
            b[i]=0.0;
        }else
        {
            c[i]=sqrt(h);
            u=i*n+i-1;
            if(q[u]>0.0)
            {
            	c[i]=-c[i];
            }
            h=h-q[u]*c[i];
            q[u]=q[u]-c[i];
            f=0.0;
            for(j=0;j<i;++j)
            {
                q[j*n+i]=q[i*n+j]/h;
                g=0.0;
                for(k=0;k<=j;++k)
                {
                	g=g+q[j*n+k]*q[i*n+k];
                }
                if(j+1<i)
                {
                    for(k=j+1;k<i;++k)
                    {
                    	g=g+q[k*n+j]*q[i*n+k];
                    }
                }
                c[j]=g/h;
                f=f+g*q[j*n+i];
            }
            h2=f/(h+h);
            for(j=0;j<i;++j)
            {
                f=q[i*n+j];
                g=c[j]-h2*f;
                c[j]=g;
                for(k=0;k<=j;++k)
                {
                    u=j*n+k;
                    q[u]=q[u]-f*c[k]-g*q[i*n+k];
                }
            }
            b[i]=h;
        }
    }
    for(i=0;i<n-1;++i)
    {
    	c[i]=c[i+1];
    }
    c[n-1]=0.0;
    b[0]=0.0;
    for(i=0;i<n;++i)
    {
        if((b[i]!=0.0)&&(i-1>=0))
        {
            for(j=0;j<i;++j)
            {
                g=0.0;
                for(k=0;k<i;++k)
                {
                	g=g+q[i*n+k]*q[k*n+j];
                }
                for(k=0;k<i;++k)
                {
                    u=k*n+j;
                    q[u]=q[u]-g*q[k*n+i];
                }
            }
        }
        u=i*n+i;
        b[i]=q[u];
        q[u]=1.0;
        if(i>0)
        {
            for(j=0;j<i;++j)
            {
                q[i*n+j]=0.0;
                q[j*n+i]=0.0;
            }
        }
    }
}

int wtk_eig_qr_householder_process(wtk_complex_t *a,double *fv,double *b,double *c,int n,wtk_complex_t *fvij,double *val)
{
	int i,j,k,k1,m,o,p;
	int ret;//,max_idx=0;
	// double max;
	int sign;
	int n2=n*2;

	sign=1;
	for(i=0,k=0;i<n;++i)
	{
		k+=i+1;
		for(j=i+1;j<n;++j,++k)
		{
			if(a[k].b!=0.0)
			{
				sign=0;
				break;
			}
		}
	}

	if(sign==1)
	{
		for(i=0,k=0;i<n;++i)
		{
			for(j=0;j<n;++j,++k)
			{
				fv[k]=a[k].a;
			}
		}
		wtk_householder_process(fv,n, b, c);
		ret=wtk_qr_process(n, b, c, fv, 1e-14,100);

        if(val)
        {
            for(i=0;i<n;++i)
            {
                val[i]=b[i];
            }
        }

        for(i=0;i<n;++i)
        {
		    for(j=0;j<n;++j)
		    {
			    fvij[i*n+j].a=fv[i*n+j];
			    fvij[i*n+j].b=0.0;
		    }
        }

		goto end;
	}

	k1=0;
	m=n;
	p=2*n*n;
	o=p+n;
	for(i=0,k=0;i<n;++i)
	{
		for(j=0;j<n;++j,++k,++k1,++m,++o,++p)
		{
			fv[k1]=fv[o]=a[k].a;
			fv[m]=-a[k].b;
			fv[p]=a[k].b;
		}
		k1+=n;
		m+=n;
		o+=n;
		p+=n;
	}

	wtk_householder_process(fv,n2, b, c);
	ret=wtk_qr_process(n2, b, c, fv, 1e-14,100);

    if(val)
    {
        for(i=0;i<n;++i)
        {
            val[i]=b[i];
        }
    }

    for(i=0;i<n2;++i)
    {
		for(j=0;j<n2;++j)
		{
            printf("%lf ",fv[i*n2+j]);
        }
        printf("\n");
    }

    for(i=0;i<n;++i)
    {
		for(j=0;j<n;++j)
		{
			fvij[i*n+j].a=fv[i*n2+j];
			fvij[i*n+j].b=fv[(i+n)*n2+j];
		}
    }

end:
	return ret;
}
