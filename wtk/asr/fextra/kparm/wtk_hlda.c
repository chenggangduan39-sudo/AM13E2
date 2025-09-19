#include "wtk_hlda.h" 

wtk_hlda_t* wtk_hlda_new(wtk_hlda_cfg_t *cfg)
{
	wtk_hlda_t *hlda;

	hlda=(wtk_hlda_t*)wtk_malloc(sizeof(wtk_hlda_t));
	hlda->cfg=cfg;
	if(cfg->fix_hlda)
	{
		hlda->vec=(float*)wtk_calloc(cfg->fix_hlda->row,sizeof(int));
	}else
	{
		hlda->vec=(float*)wtk_calloc(cfg->hlda->row,sizeof(float));
	}
	return hlda;
}

int wtk_hlda_bytes(wtk_hlda_t *hlda)
{
	int bytes;

	bytes=sizeof(wtk_hlda_t);
	if(hlda->cfg->fix_hlda)
	{
		bytes+=hlda->cfg->fix_hlda->row*sizeof(int);
	}else
	{
		bytes+=hlda->cfg->hlda->row*sizeof(float);
	}
	return bytes;
}

void wtk_hlda_delete(wtk_hlda_t *hlda)
{
	wtk_free(hlda->vec);
	wtk_free(hlda);
}

void wtk_hlda_calc(wtk_hlda_t *hlda,float *iv)
{
	wtk_matf_t *mat=hlda->cfg->hlda;
	int row,col;
	float *pf,*pv;
	int i,j;
	float f;

	row=mat->row;
	col=mat->col;
	pv=hlda->vec;
	pf=mat->p;
	for(i=0;i<row;++i)
	{
		f=0;
		for(j=0;j<col;++j)
		{
			f+=iv[j]*pf[j];
//			if(i==0)
//			{
//				wtk_debug("v[%d/%d]=%f/%f/%f\n",i,j,iv[j],pf[j],f);
//			}
		}
//		if(i==0)
//		{
//			wtk_debug("v[%d]=%f\n",i,f);
//		}
		pf+=col;
		pv[i]=f;
	}
}

void wtk_hlda_calc_fix(wtk_hlda_t *hlda,int *iv)
{
	wtk_mati_t *mat=hlda->cfg->fix_hlda;
	int row,col;
	int *pf,*pv;
	int i,j;
	int f;

	row=mat->row;
	col=mat->col;
	pv=(int*)(hlda->vec);
	pf=mat->p;
	for(i=0;i<row;++i)
	{
		f=0;
		for(j=0;j<col;++j)
		{
			//f+=iv[j]*pf[j];
			f+=FIXMUL(iv[j],pf[j]);
//			if(i==0)
//			{
//				wtk_debug("v[%d/%d]=%d/%d/%d %f/%f/%f\n",i,j,iv[j],pf[j],f,FIX2FLOAT(iv[j]),FIX2FLOAT(pf[j]),FIX2FLOAT(f));
//			}
		}
//		if(i==0)
//		{
//			wtk_debug("v[%d]=%d/%f\n",i,f,FIX2FLOAT(f));
//		}
		pf+=col;
		pv[i]=f;
	}
}
