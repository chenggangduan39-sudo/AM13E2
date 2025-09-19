#include "splitf02.h"
#include "wtk_fpost.h"
#include "wtk/asr/fextra/f0/wtk_f0.h"
#define NEGTIVEINFINITE -1.0e+10  //define negtive infinite
void wtk_fpost_clean_mem(wtk_fpost_t *p);

wtk_fpost_t* wtk_fpost_new(wtk_fpost_cfg_t *cfg,wtk_f0_t *f0)
{
	wtk_fpost_t *f;

	f=(wtk_fpost_t*)wtk_malloc(sizeof(*f));
	f->cfg=cfg;
	f->f0=f0;
	f->f=0;f->fe=0;f->f_delta=0;
	wtk_fpost_reset(f);
	return f;
}

void wtk_fpost_delete(wtk_fpost_t *p)
{
	wtk_fpost_reset(p);
	wtk_free(p);
}


void wtk_fpost_clean_mem(wtk_fpost_t *p)
{
	if(p->f)
	{
		wtk_free(p->f);
		p->f=0;
	}
	if(p->fe)
	{
		wtk_free(p->fe);
		p->fe=0;
	}
	if(p->f_delta)
	{
		wtk_free(p->f_delta);
		p->f_delta=0;
	}
	/*
	if(p->fe_delta)
	{
		wtk_free(p->fe_delta);
		p->fe_delta=0;
	}*/
}

void wtk_fpost_reset(wtk_fpost_t *p)
{
	wtk_fpost_clean_mem(p);
	p->phase=0;
	p->v1=0;
	p->v2=0;
	p->s=0;
}

void wtk_fpost_update_sample(wtk_fpost_t *p)
{
	wtk_f0_t *f0=p->f0;
	int bytes;
	float *f;
	float *e;
	int i,nlen;

	f=(float*)f0->f0_array->slot;
	e=(float*)f0->f0e_array->slot;
	nlen=f0->f0_array->nslot;
	bytes=nlen*sizeof(double);
	p->f=(double*)wtk_malloc(bytes);
	p->fe=(double*)wtk_malloc(bytes);
	for(i=0;i<nlen;++i)
	{
		p->f[i]=f[i];
		p->fe[i]=e[i];
	}
	p->f_delta=(double*)wtk_malloc(bytes);
	//p->fe_delta=(double*)wtk_malloc(bytes);
	p->nframe=nlen;
}

double wtk_fpost_gaussrand(wtk_fpost_t *p,double mu, double delta)
{
    double x;
    double u1,u2;
    double v;

    if(p->phase==0)
    {
        do
        {
            u1=(double)rand()/RAND_MAX;
            u2=(double)rand()/RAND_MAX;
            p->v1=2*u1-1;
            p->v2=2*u2-1;
            p->s=p->v1*p->v1+p->v2*p->v2;
        } while(p->s>=1||p->s==0);
        x=p->v1*sqrt(-2*log(p->s)/p->s);
    }else
    {
        x=p->v2*sqrt(-2*log(p->s)/p->s);
    }
    p->phase=1-p->phase;
    v=x*sqrt(delta)+mu;
    return v;
}

int wtk_fpost_process(wtk_fpost_t *p,int nword)
{
	wtk_splitf0_cfg_t *cfg;
	VoiceSeg2 *head=0;
	int ret=-1;
	double *f;
	double *fe;
	double *f_delta;
	int nframe;
	int i;
	double var;

	if(nword==1)
	{
		cfg=&(p->cfg->ctone);
	}else
	{
		cfg=&(p->cfg->wtone);
	}
	wtk_fpost_update_sample(p);
	f=p->f;
	fe=p->fe;
	f_delta=p->f_delta;
	nframe=p->nframe;
	//1. construct voice segments
	head=GetVoiceSegFromF02(f,fe,nframe,cfg->energy_thresh);
	if(!head || head->dur<1){goto end;}
	// 2. discard voiced regions with very low average energy
	DiscardLowAveEngyRegion2(head,f,fe,cfg->energy_ratio);
	// 3. merge short or very close voiced regions and
	//delete isolated short segments after merging, result should be only 1 seg
	MergeShortVoiceRegion2(head,f,fe,nword, cfg->min_voice_dur,0);
	if(head->dur<1){goto end;}
	//smooth f0 in each voiced segments
	SmoothF02(f_delta,f,nframe,head,cfg->pls_thresh);
	NormF0Engy2(f,fe,nframe,head);
	//	  1264122275
	srand(1264122275);
	var=cfg->glb_var*cfg->noise_var_ratio;
	for(i=0;i<nframe;i++)
	{
		if(f[i]==NEGTIVEINFINITE)
		{
			f[i] =wtk_fpost_gaussrand(p,cfg->glb_mean,var);
		}
	}
	FreeVoiceSeg2(head);
	ret=0;
end:
	return ret;
}


/*
int wtk_fpost_process2(wtk_fpost_t *p,int nword)
{
	wtk_splitf0_cfg_t *cfg;
	VoiceSeg *head=0;
	int ret=-1;
	double *f;
	double *fe;
	double *f_delta;
	int nframe;
	//int i;
	//double var;

	if(nword==1)
	{
		cfg=&(p->cfg->ctone);
	}else
	{
		cfg=&(p->cfg->wtone);
	}
	wtk_fpost_update_sample(p);
	f=p->f;
	fe=p->fe;
	f_delta=p->f_delta;
	nframe=p->nframe;
	//1. construct voice segments
	head=GetInitialVoiceSeg(f,nframe);
	//head=GetVoiceSegFromF0(f,fe,nframe,cfg->energy_thresh);
	if(!head || head->dur<1){goto end;}
	// 2. discard voiced regions with very low average energy
	DiscardLowAveEngyRegion(head,f,fe);
	// 3. merge short or very close voiced regions and
	//delete isolated short segments after merging, result should be only 1 seg
	MergeShortVoiceRegion(head,f,fe,nword);
	if(head->dur<1){goto end;}
	//smooth f0 in each voiced segments
	SmoothF0(f_delta,f,nframe,head);
	SmoothEngy(f,fe,nframe);
	srand(1264122275);
	var=cfg->glb_var*cfg->noise_var_ratio;
	for(i=0;i<nframe;i++)
	{
		//wtk_debug("i=%d,%f\n",i,f[i]);
		if(f[i]==NEGTIVEINFINITE)
		{
			f[i] = wtk_fpost_gaussrand(p,cfg->glb_mean,var);
		}
	}
	FreeVoiceSeg(head);
	ret=0;
end:
	return ret;
}
*/

void wtk_fpost_print(wtk_fpost_t *p)
{
	int i;

	for(i=0;i<p->nframe;++i)
	{
		//printf("%f\t%f\n",p->f[i],p->fe[i]);
		printf("v[%d]=%f,%f\n",i,p->f[i],p->fe[i]);
	}
}
