#include "wtk_fixexp.h"


int wtk_fixe_bytes(wtk_fixexp_t *fixe)
{
	return sizeof(wtk_fixexp_t)+fixe->n*sizeof(short);
}

wtk_fixexp_t* wtk_fixexp_read(wtk_source_t *src)
{
	int v[5];
	wtk_fixexp_t *exp=NULL;
	int ret;

	ret=wtk_source_fill(src,(char*)&v,sizeof(int)*5);
	if(ret!=0){goto end;}
	exp=(wtk_fixexp_t*)wtk_malloc(sizeof(wtk_fixexp_t));
	exp->shifti=v[0];
	exp->shifto=v[1];
	exp->step=v[2];
	exp->vs=v[3];
	exp->n=v[4];
	exp->table=(short*)wtk_calloc(exp->n,sizeof(short));
	ret=wtk_source_fill(src,(char*)exp->table,sizeof(short)*exp->n);
	if(ret!=0){goto end;}
end:
	return exp;
}

void wtk_fixexp_write(wtk_fixexp_t *exp,FILE *f)
{
#ifndef USE_RTOS_OF_5215
	int v[5];

	v[0]=exp->shifti;
	v[1]=exp->shifto;
	v[2]=exp->step;
	v[3]=exp->vs;
	v[4]=exp->n;
	fwrite((char*)v,5*sizeof(int),1,f);
	fwrite((char*)exp->table,exp->n*sizeof(short),1,f);
#endif
}

wtk_fixexp_t* wtk_fixexp_new(char shifti,char shifto,int step)
{
	wtk_fixexp_t *e;
	float min=-7;
	float max=0;
	int nx;
	int vs,ve;
	int i,j;
	float f;

	e=(wtk_fixexp_t*)wtk_malloc(sizeof(wtk_fixexp_t));
	e->shifti=shifti;
	e->shifto=shifto;
	e->step=step;
	vs=min*(1<<shifti);
	ve=max*(1<<shifti);
	nx=(ve-vs)/step+1;
	e->table=(short*)wtk_calloc(nx,sizeof(short));
	e->n=nx;
	vs=ve-step*nx;
	e->vs=ve;

	for(i=ve,j=0;j<nx;i-=step,++j)
	{
		f=exp(FIX2FLOAT_ANY(i,shifti));
		e->table[j]=FLOAT2FIX_ANY(f,shifto);
		//wtk_debug("v[%d/%d]\n",j,nx);
	}
	return e;
}

void wtk_fixexp_delete(wtk_fixexp_t *exp)
{
	wtk_free(exp->table);
	wtk_free(exp);
}

int wtk_fixexp_calc(wtk_fixexp_t *exp,int v)
{
	int idx;
	int v1;
	int step=exp->step;
	short *table=exp->table;

	//wtk_debug("v=%d\n",v);
	v=exp->vs-v;
	idx=v/step;
	//wtk_debug("idx=%d/%d\n",idx,exp->n);
	//wtk_debug("v=%d %d/%d\n",v,idx,exp_table_size);
	if(idx>=exp->n-1)
	{
		return table[exp->n-1];
	}else if(idx<0)
	{
		return table[0];
	}
	//wtk_debug("v=%d %d/%d/%d\n",idx,exp_table[idx-1],exp_table[idx],exp_table[idx+1]);
	v1=idx*step;
//
//	wtk_debug("idx=%d %f/%f/%f %d\n",idx,FIX2FLOAT_ANY(table[idx-1],exp->shifto),
//			FIX2FLOAT_ANY(table[idx],exp->shifto),FIX2FLOAT_ANY(table[idx+1],exp->shifto),v-v1);

	if(v>v1 && idx<exp->n-1)
	{
		//wtk_debug("%d/%d %f/%f %d/%d\n",table[idx],table[idx+1],FIX2FLOAT_ANY(table[idx],exp->shifto),FIX2FLOAT_ANY(table[idx+1],exp->shifto),v,v1);
		v=table[idx]+(table[idx+1]-table[idx])*(v-v1)/step;
		//wtk_debug("v=%d\n",v);
	}else
	{
		v=table[idx];
	}
	return v;
}


void wtk_fixexp_test(void)
{
	wtk_fixexp_t *fixe;
	float f;
	int shifti=10;
	int shifto=9;
	int v;

	fixe=wtk_fixexp_new(shifti,shifto,30);

	f=-0.51;
	v=FLOAT2FIX_ANY(f,shifti);
	wtk_debug("v=%d\n",v);
	v=wtk_fixexp_calc(fixe,v);
	wtk_debug("f=%f/%f/%f\n",FIX2FLOAT_ANY(v,shifto),exp(f),FIX2FLOAT_ANY(v,shifto)-exp(f));
	exit(0);

	f=-0.51;
	v=FLOAT2FIX_ANY(f,shifti);
	wtk_debug("v=%d\n",v);
	v=wtk_fixexp_calc(fixe,v);
	wtk_debug("f=%f/%f\n",FIX2FLOAT_ANY(v,shifto),exp(f));

	wtk_fixexp_delete(fixe);
}
