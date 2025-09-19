#include "wtk_fixlog.h"


int wtk_fixlog_bytes(wtk_fixlog_t *fixe)
{
	return sizeof(wtk_fixlog_t)+fixe->n*sizeof(short);
}

wtk_fixlog_t* wtk_fixlog_read(wtk_source_t *src)
{
	int v[5];
	wtk_fixlog_t *exp=NULL;
	int ret;

	ret=wtk_source_fill(src,(char*)&v,sizeof(int)*4);
	if(ret!=0){goto end;}
	exp=(wtk_fixlog_t*)wtk_malloc(sizeof(wtk_fixlog_t));
	exp->shift=v[0];
	exp->step=v[1];
	exp->vs=v[2];
	exp->n=v[3];
	exp->table=(short*)wtk_calloc(exp->n,sizeof(short));
	ret=wtk_source_fill(src,(char*)exp->table,sizeof(short)*exp->n);
	if(ret!=0){goto end;}
end:
	return exp;
}


void wtk_fixlog_write(wtk_fixlog_t *exp,FILE *f)
{
#ifndef USE_RTOS_OF_5215
	int v[4];

	v[0]=exp->shift;
	v[1]=exp->step;
	v[2]=exp->vs;
	v[3]=exp->n;
	fwrite((char*)v,4*sizeof(int),1,f);
	fwrite((char*)exp->table,exp->n*sizeof(short),1,f);
#endif
}


wtk_fixlog_t* wtk_fixlog_new(char shift,int step)
{
	wtk_fixlog_t *fixl;
	float min=1;
	float max=10;
	int vs,ve;
	int nx;
	int i,j;
	float f;

	fixl=(wtk_fixlog_t*)wtk_malloc(sizeof(wtk_fixlog_t));
	fixl->shift=shift;
	fixl->step=step;
	vs=min*(1<<shift);
	ve=max*(1<<shift);
	nx=(ve-vs)/step+1;
	ve=vs+nx*step;
	fixl->vs=vs;
	fixl->n=nx;
	fixl->table=(short*)wtk_calloc(nx,sizeof(short));
	for(i=vs,j=0;i<ve;i+=step,++j)
	{
		//wtk_debug("v[%d/%d]\n",j,nx);
		f=log(FIX2FLOAT_ANY(i,shift));
		fixl->table[j]=FLOAT2FIX_ANY(f,shift);
	}
	return fixl;
}

void wtk_fixlog_delete(wtk_fixlog_t *fixl)
{
	wtk_free(fixl->table);
	wtk_free(fixl);
}

int wtk_fixlog_calc(wtk_fixlog_t *fixl,int v)
{
	int idx;
	int v1;
	int step=fixl->step;
	short *table=fixl->table;

	//wtk_debug("v=%d\n",v);
	v=v-fixl->vs;
	idx=v/step;
	//wtk_debug("idx=%d/%d\n",idx,exp->n);
	//wtk_debug("v=%d %d/%d\n",v,idx,exp_table_size);
	if(idx>=fixl->n-1)
	{
		return table[fixl->n-1];
	}else if(idx<0)
	{
		return table[0];
	}
	//wtk_debug("v=%d %d/%d/%d\n",idx,exp_table[idx-1],exp_table[idx],exp_table[idx+1]);
	v1=idx*step;
	if(v>v1 && idx<fixl->n-1)
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
