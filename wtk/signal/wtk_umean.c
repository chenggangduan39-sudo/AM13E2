#include "wtk_umean.h" 


wtk_umean_t* wtk_umean_new(int bak,int n,int len)
{
	wtk_umean_t *m;
	int i;

	m=(wtk_umean_t*)wtk_malloc(sizeof(wtk_umean_t));
	if(bak>0)
	{
		m->bak=wtk_robin_new(bak);
		for(i=0;i<m->bak->nslot;++i)
		{
			m->bak->r[i]=(wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t)*len);
		}
	}else
	{
		m->bak=NULL;
	}
	m->rb=wtk_robin_new(n);
	for(i=0;i<n;++i)
	{
		m->rb->r[i]=(wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t)*len);
	}
	m->sum=(wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t)*len);
	m->mean=(wtk_complex_t*)wtk_malloc(sizeof(wtk_complex_t)*len);
	m->len=len;
	wtk_umean_reset(m);
	return m;
}

void wtk_umean_delete(wtk_umean_t *m)
{
	int i;

	wtk_free(m->sum);
	wtk_free(m->mean);
	if(m->bak)
	{
		for(i=0;i<m->bak->nslot;++i)
		{
			wtk_free(m->bak->r[i]);
		}
		wtk_robin_delete(m->bak);
	}
	for(i=0;i<m->rb->nslot;++i)
	{
		wtk_free(m->rb->r[i]);
	}
	wtk_robin_delete(m->rb);
	wtk_free(m);
}

void wtk_umean_reset(wtk_umean_t *m)
{
	memset(m->sum,0,sizeof(wtk_complex_t)*m->len);
	wtk_robin_reset(m->rb);
	if(m->bak)
	{
		wtk_robin_reset(m->bak);
	}
}

void wtk_umean_feed_sum(wtk_umean_t *m,wtk_complex_t *v)
{
	wtk_robin_t *rb=m->rb;
	wtk_complex_t *dst;
	wtk_complex_t *cur;
	wtk_complex_t *sum=m->sum;
	int i;
	int n=m->len;

	if(rb->used==rb->nslot)
	{
		dst=wtk_robin_pop(rb);
		for(i=0;i<n;++i)
		{
			sum[i].a+=v[i].a-dst[i].a;
			sum[i].b+=v[i].b-dst[i].b;
		}
	}else
	{
		for(i=0;i<n;++i)
		{
			sum[i].a+=v[i].a;
			sum[i].b+=v[i].b;
		}
	}
	cur=wtk_robin_next(rb);
	memcpy(cur,v,sizeof(wtk_complex_t)*n);
}

void wtk_umean_feed(wtk_umean_t *m,wtk_complex_t *v)
{
	wtk_robin_t *rb=m->bak;
	wtk_complex_t *dst;
	wtk_complex_t *cur;
	int n=m->len;

	//wtk_debug("bak=%d rb=%d\n",m->bak->used,m->rb->used);
	if(rb==NULL)
	{
		wtk_umean_feed_sum(m,v);
		return;
	}
	if(rb->used==rb->nslot)
	{
		dst=wtk_robin_pop(rb);
		wtk_umean_feed_sum(m,dst);
	}
	cur=wtk_robin_next(rb);
	memcpy(cur,v,sizeof(wtk_complex_t)*n);

}

wtk_complex_t* wtk_umean_get_mean(wtk_umean_t *m)
{
	wtk_complex_t *mean=m->mean;
	wtk_complex_t *sum=m->sum;
	float f=1.0/m->rb->used;
	int i;
	int len=m->len;

	//wtk_debug("f=%f\n",f);
	for(i=0;i<len;++i)
	{
		mean[i].a=sum[i].a*f;
		mean[i].b=sum[i].b*f;
	}
	return mean;
}

void wtk_umean_feed_sumf(wtk_umean_t *m,float *v)
{
	wtk_robin_t *rb=m->rb;
	float *dst;
	float *cur;
	float *sum=(float*)m->sum;
	int i;
	int n=m->len;

	if(rb->used==rb->nslot)
	{
		dst=wtk_robin_pop(rb);
		for(i=0;i<n;++i)
		{
			sum[i]+=v[i]-dst[i];
		}
	}else
	{
		for(i=0;i<n;++i)
		{
			sum[i]+=v[i];
		}
	}
	cur=wtk_robin_next(rb);
	memcpy(cur,v,sizeof(float)*n);
}


void wtk_umean_feedf(wtk_umean_t *m,float *v,int nx)
{
	wtk_robin_t *rb=m->bak;
	float *dst;
	float *cur;
	int n=m->len;

	//wtk_debug("bak=%d rb=%d\n",m->bak->used,m->rb->used);
	if(nx!=n)
	{
		wtk_debug("nx=%d/%d\n",nx,n);
		exit(0);
	}
	if(rb==NULL)
	{
		wtk_umean_feed_sumf(m,v);
		return;
	}
	if(rb->used==rb->nslot)
	{
		dst=wtk_robin_pop(rb);
		wtk_umean_feed_sumf(m,dst);
	}
	cur=wtk_robin_next(rb);
	memcpy(cur,v,sizeof(float)*n);
}

float* wtk_umean_get_meanf(wtk_umean_t *m)
{
	float *mean=(float*)m->mean;
	float *sum=(float*)m->sum;
	float f=1.0/m->rb->used;
	int i;
	int len=m->len;

	//wtk_debug("f=%f\n",f);
	for(i=0;i<len;++i)
	{
		mean[i]=sum[i]*f;
	}
	return mean;
}
