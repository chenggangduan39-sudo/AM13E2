#include <stdlib.h>
#include <math.h>
#include "wtk_syn_pstream.h" 

wtk_syn_smat_t* wtk_syn_smat_new(int t,int width)
{
	wtk_syn_smat_t *m;

	m=(wtk_syn_smat_t*)wtk_malloc(sizeof(wtk_syn_smat_t));
	m->R=wtk_matf_new(t,width);
	m->r=wtk_vector_new(t);
	m->g=wtk_vector_new(t);
	m->b=wtk_vector_new(t);
	return m;
}

void wtk_syn_smat_delete(wtk_syn_smat_t *m)
{
	wtk_matf_delete(m->R);
	wtk_vector_delete(m->r);
	wtk_vector_delete(m->g);
	wtk_vector_delete(m->b);
	wtk_free(m);
}


wtk_syn_pstream_t* wtk_syn_pstream_new(int vsize,int t,wtk_syn_dwin_t *dwin,wtk_syn_gv_pdf_t *gvpdf)
{
	wtk_syn_pstream_t *p;

	p=(wtk_syn_pstream_t*)wtk_malloc(sizeof(wtk_syn_pstream_t));
	p->vsize=vsize;
	p->T=t;
	p->dim=vsize/dwin->cfg->fn->nslot;
	p->dw=dwin;
	p->width=dwin->maxl*2+1;
	if(gvpdf && gvpdf->var && gvpdf->mean)
	{
		p->vm=gvpdf->mean;
		p->vv=gvpdf->var;
	}else
	{
		p->vm=NULL;
		p->vv=NULL;
	}
	p->sm=wtk_syn_smat_new(t,p->width);
	p->mseq=wtk_matf_new(t,p->vsize);
	p->ivseq=wtk_matf_new(t,p->vsize);
	p->par=wtk_matf_new(t,p->dim);
//	p->mseq=wtk_matf_new(t,p->vsize);
//	p->ivseq=wtk_matf_new(t,p->vsize);
//	p->par=wtk_matf_new(t,p->dim);

	p->coef1=2.0/p->T;
	p->coef2=2.0/(p->T*p->T);
	return p;
}

void wtk_syn_pstream_delete(wtk_syn_pstream_t *s)
{
	wtk_syn_smat_delete(s->sm);
	wtk_matf_delete(s->mseq);
	wtk_matf_delete(s->ivseq);
	wtk_matf_delete(s->par);
	wtk_free(s);
}


/*------ parameter generation fuctions */
/* calc_R_and_r : calculate R=W'U^{-1}W and r=W'U^{-1}M */
void wtk_syn_pstream_calc_R_and_r(wtk_syn_pstream_t *s,int m)
{
	int i,j,k,l;
	register float wu;
	float *f1,*f2,*f3,*f4,*f5,*f6,*f7,*f8,*f9,*f10,*f11,*fe;
	int *i1;
	int v1,v2,v3,v4,v5;
	int R_col,ivseq_col,mseq_col;

	f1=s->ivseq->p+m-1;
	f2=s->mseq->p+m-1;
	f3=s->sm->R->p;
	f4=s->sm->r;
	v1=sizeof(float)*(s->width-1);
	v5=s->dim+m-1;
	R_col=s->sm->R->col;
	ivseq_col=s->ivseq->col;
	mseq_col=s->mseq->col;
	for(i=1,f10=s->sm->R->p;i<=s->T;++i,f1+=ivseq_col,f2+=mseq_col,f3+=R_col,f10+=R_col)
	{
		wu=*f1;
		*(++f4)=wu * *f2;
		*f3=wu;
		memset(f3+1,0,v1);
		v3=v5;
		for(j=1;j<s->dw->num;++j,v3+=s->dim)
		{
			i1=s->dw->width[j];
			v2=i1[1];
			v2=min(v2,s->T-i);
			k=i1[0];
			k=max(k,1-i);
			f9=s->dw->coef[j];
			f5=f9-k;
			f6=wtk_matf_at(s->ivseq,i+k-1,v3);
			f7=wtk_matf_at(s->mseq,i+k-1,v3);
			for(;k<=v2;++k,--f5,f6+=ivseq_col,f7+=mseq_col)
			{
				//wtk_debug("[%d/%d]\n",s->dw->width[j-1][0],s->dw->width[j-1][1]);
				if(((*f5)!=0.0))
				{
					//n=i+k;
					wu=*f5 * *f6;
					//wtk_debug("%f/%f/%f\n",wu,*f7,*f4);
					*f4+=wu* *f7;
					//wtk_debug("v[%d/%d]=%f %f %p/%p\n",kx,i,*f4,s->sm->r[i],f4,s->sm->r+i);
					//exit(0);
					l=s->T-i+1;
					v4=min(s->width,l);
					l=i1[1]+k+1;
					v4=min(l,v4);
					f8=f10-1;
					fe=f8+v4;
					f11=f9-k-1;
					while((++f8)<=fe)
					{
						if((*(++f11))!=0.0)
						{
							*(f8) +=wu* *f11;
						}
					}

				}
			}
			//exit(0);
		}
		//exit(0);
	}
	//wtk_debug("kx=%d\n",kx);
	//exit(0);
}

/*
void wtk_syn_pstream_calc_R_and_r(wtk_syn_pstream_t *s,int m)
{
	int i,j,k,l,n;
	float wu;
	int ki=0;

	//wtk_debug("%f\n",s->sm->R[2][2]);

	//wtk_debug("%f\n",s->sm->R[326][2]);
	for(i=1;i<=s->T;++i)
	{
		s->sm->r[i]=s->ivseq[i][m]*s->mseq[i][m];
		//wtk_debug("v[%d]=%f\n",i,s->sm->r[i]);
		s->sm->R[i][1]=s->ivseq[i][m];
		//wtk_debug("v[%d]=%f\n",i,s->sm->R[i][1]);
		for(j=2;j<=s->width;++j)
		{
			s->sm->R[i][j]=0;
		}
//		wtk_debug("i=%d\n",i);
		for(j=2;j<=s->dw->num;++j)
		{
			for(k=s->dw->width[j-1][0];k<=s->dw->width[j-1][1];++k)
			{
				//wtk_debug("[%d/%d]\n",s->dw->width[j-1][0],s->dw->width[j-1][1]);
				n=i+k;
				if( (n>0) && (n<=s->T) &&(s->dw->coef[j-1][-k]!=0.0))
				{
					l=(j-1)*s->dim+m;
					//wtk_debug("n=%d l=%d\n",n,l);
					//wtk_debug("%d/%d %d/%d\n",j-1,-k,n,l);
					wu=s->dw->coef[j-1][-k]*s->ivseq[n][l];
					//wtk_debug("wu=%f %f/%f\n",wu,s->dw->coef[j-1][-k],s->ivseq[n][l]);
					s->sm->r[i]+=wu*s->mseq[n][l];
					//wtk_debug("r=%f\n",s->sm->r[i]);
					for(l=0;l<s->width;++l)
					{
						n=l-k;
						if((n<=s->dw->width[j-1][1]) && (i+l<=s->T) && (s->dw->coef[j-1][n]!=0.0))
						{
							//wtk_debug("v[%d][%d]=%f/%f\n",i,l,s->sm->R[i][l+1],wu*s->dw->coef[j-1][n]);
							s->sm->R[i][l+1]+=wu*s->dw->coef[j-1][n];
//							if(i==326 && l==1)
//							{
//								wtk_debug("%f=%f*%f\n",s->sm->R[326][2],wu,s->dw->coef[j-1][n]);
//							}
						}
					}
				}
			}
		}
		//exit(0);
	}
//	wtk_debug("%f\n",s->sm->R[326][2]);
//	exit(0);
}*/

/* Cholesky : Cholesky factorization of Matrix R */
void wtk_syn_pstream_cholesky(wtk_syn_pstream_t *s)
{
	int j,t,k;
	register float f,tf;
	float *f1,*f2,*f3,*f4;
	int col;
	int width,maxl;
	wtk_matf_t *R;

	R=s->sm->R;
	f1=R->p;
	f=*f1=sqrt(*f1);
	f=1.0/f;

	f2=f1+s->width-1;
	while(f1<f2)
	{
		*(++f1) *=f;
	}
	col=R->col;
	f1=R->p+col;
	width=s->width;
	maxl=s->dw->maxl;
	for(t=2;t<=s->T;++t,f1+=col)
	{
		tf=*f1;
		for(j=min(2,t),f2=wtk_matf_at(R,t-j,j-1);j<=width;++j,f2-=col-1)
		{
			f=*f2;//wtk_matf_at(s->sm->R,t-j,j-1);
			tf -=f*f;
		}
		tf=*f1=sqrt(tf);
		f=1.0/ tf;
		f2=f1+1;
		for(j=2;j<=width;++j,++f2)
		{
			tf=*f2;
			for(k=0,f3=wtk_matf_at(R,t-2,j-1),f4=wtk_matf_at(R,t-2,j);k<maxl;++k,f4-=col,f3-=col+1)
			{
				//wtk_debug("j=%d/%d\n",j,s->width-1);
				if(j!=width)
				{
					tf -=*f3 * *f4;//*wtk_matf_at(s->sm->R,t-k-2,j-k-1) * *wtk_matf_at(s->sm->R,t-k-2,j);
					//wtk_debug("%f/%f %f/%f\n",*f3,*wtk_matf_at(s->sm->R,t-k-2,j-k-1),*f4,*wtk_matf_at(s->sm->R,t-k-2,j));
				}
			}
			*f2 = tf * f;
		}
	}
	//exit(0);
}


/* Cholesky_forward : forward substitution to solve linear equations */
void wtk_syn_pstream_cholesky_forward(wtk_syn_pstream_t *s)
{
	int t,j;
	float hold;
	float *f1,*f2,*f3,*f4,*f5;
	int col;
	int v;

	f1=s->sm->R->p;
	s->sm->g[1]=s->sm->r[1]/ *f1;//wtk_matf_at(s->sm->R,0,0);
	//wtk_debug("%f\n",s->sm->g[1]);
	col=s->sm->R->col;
	f1+=col;
	f3=s->sm->g+2;
	f4=s->sm->r+2;
	for(t=2;t<=s->T;++t,f1+=col,++f3,++f4)
	{
		hold=0;
		f2=s->sm->R->p+1+(t-2)*col;
		v=min(s->width,t);
		f5=s->sm->g+t-1;
		for(j=2;j<=v;++j,f2-=col-1,--f5)
		{
			if((*f2!=0.0))
			{
				//hold+=s->sm->R[t-j+1][j]*s->sm->g[t-j+1];
				hold+=*f2 * *f5;//s->sm->g[t-j+1];
				//wtk_debug("f5=%f/%f\n",*f5,s->sm->g[t-j+1]);
			}
		}
		//s->sm->g[t]=(s->sm->r[t]-hold)/s->sm->R[t][1];
		*f3=(*f4-hold)/ *f1;//[1];
		//wtk_debug("v[%d]=%f\n",t,s->sm->g[t]);
	}
	//exit(0);
}

/* Cholesky_backward : backward substitution to solve linear equations */
void wtk_syn_pstream_cholesky_backward(wtk_syn_pstream_t *s,int m)
{
	int t,j;
	float hold;
	float *f1,*f2,*f3,*f4,*f5;
	int col,col2;

	col=s->par->col;
	f1=wtk_matf_at(s->par,s->T-1,m-1);
	*f1=s->sm->g[s->T]/ *wtk_matf_at(s->sm->R,s->T-1,0);
	f1-=col;
	col2=s->sm->R->col;
	for(t=s->T-1,f2=s->sm->g+t,f3=wtk_matf_at(s->sm->R,t-1,0);t>0;--t,f1-=col,--f2,f3-=col2)
	{
		hold=0;
		f5=wtk_matf_at(s->par,t,m-1);
		for(j=1,f4=f3+j;j<s->width;++j,++f4,f5+=col)
		{
			if((t+j<=s->T) && ( *f4 !=0.0))
			{
				hold+=*f4 * *f5;
			}
		}
		*f1=(*f2-hold)/ *f3;
	}
}


void wtk_syn_pstream_mlpg(wtk_syn_pstream_t *s)
{
	int m;

	for(m=1;m<=s->dim;++m)
	{
		wtk_syn_pstream_calc_R_and_r(s,m);
		wtk_syn_pstream_cholesky(s);
		wtk_syn_pstream_cholesky_forward(s);
		wtk_syn_pstream_cholesky_backward(s,m);
	}
}

void wtk_syn_pstream_calc_varstats(wtk_syn_pstream_t *s,wtk_matf_t *mt,int m,float *av,float *var)
{
	int i;
	float d;
	float f1,f2;
	float *pf;
	int col;

	f1=0;f2=0;
	col=mt->col;
	for(i=1,pf=mt->p+m-1;i<=s->T;++i,pf+=col)
	{
		f1+=*pf;//wtk_matf_at(mt,i-1,m-1);
		//wtk_debug("%f/%f\n",*pf,*wtk_matf_at(mt,i-1,m-1));
	}
	f1/=s->T;
	for(i=1,pf=mt->p+m-1;i<=s->T;++i,pf+=col)
	{
		d=*pf -f1;
		f2+=d*d;
	}
	*av=f1;
	*var=f2/s->T;
}


void wtk_syn_pstream_varconv(wtk_syn_pstream_t *s,wtk_matf_t *mt,int m,float var)
{
	int n;
	float sd,osd,oav=0.0,ovar=0.0;
	float *f1;
	float v1,v2;

	wtk_syn_pstream_calc_varstats(s,mt,m,&oav,&ovar);
	osd=sqrt(ovar);
	sd=sqrt(var);
	//wtk_debug("%f/%f\n",oav,ovar);
	v1=sd/osd;
	v2=oav*(1-v1);
	for(n=1,f1=mt->p+m-1;n<=s->T;++n,f1+=mt->col)
	{
		//wtk_debug("%f\%f\n",(*f1 -oav)/osd*sd+oav,*f1*v1+v2);
		*f1=*f1*v1+v2;//(*f1 -oav)/osd*sd+oav;

	}
	//exit(0);
}

void wtk_syn_pstream_calc_varstats2(wtk_syn_pstream_t *s,int m,float *av,float *var)
{
	wtk_matf_t *mt;
	int i;
	float d;
	register float f1,f2,*f,*pf;

	mt=s->par;
	f1=0;f2=0;
	for(i=1,pf=mt->p+m-1;i<=s->T;++i,pf+=mt->col)
	{
		f1+=*pf;//wtk_matf_at(mt,i-1,m-1);
		//wtk_debug("%f/%f\n",*pf,*wtk_matf_at(mt,i-1,m-1));
	}
	f1/=s->T;
	for(i=1,f=s->sm->b+1,pf=mt->p+m-1;i<=s->T;++i,++f,pf+=mt->col)
	{
		*f=d=*pf -f1;
		f2+=d*d;
		//*f=d;
	}
	*av=f1;
	*var=f2/s->T;
	//exit(0);
}

void wtk_syn_pstream_calc_grad2(wtk_syn_pstream_t *s,int m)
{
	int i,j;
	register float *fg,*fi;
	wtk_matf_t *par;
	wtk_matf_t *R;

	par=s->par;R=s->sm->R;
	for(i=1,fg=s->sm->g+1,fi=s->sm->r+1;i<=s->T;++i,++fg,++fi)
	{
		//*fg=*fi-par[i][m]*R[i][1];
		*fg=*fi- *wtk_matf_at(par,i-1,m-1) * *wtk_matf_at(R,i-1,0);
		//exit(0);
		//wtk_debug("i=%d t=%d w=%d\n",i,s->T,s->width);
		for(j=2;j<=s->width;++j)
		{
			if(i+j-2<s->T)
			{
				//*fg-=par[i+j-1][m]*R[i][j];
				*fg-=*wtk_matf_at(par,i+j-2,m-1) * *wtk_matf_at(R,i-1,j-1);
			}
			if(i>=j)
			{
				//*fg-=par[i-j+1][m]*R[i-j+1][j];
				*fg-=*wtk_matf_at(par,i-j,m-1) * *wtk_matf_at(R,i-j,j-1);
			}
		}
	}
	//exit(0);
}

// calc_grad: calculate -RX + r = -W'U^{-1}W * X + W'U^{-1}M
void wtk_syn_pstream_calc_grad(wtk_syn_pstream_t *s,int m)
{
	int i,j;
	float *fi,*f1,*f2;
	register float *fg,*f3,*f4;
	wtk_matf_t *par;
	wtk_matf_t *R;
	int w,t,t2,t3;
	int par_col,R_col;
	register float f;

	w=s->width;
	par=s->par;R=s->sm->R;
	par_col=par->col;
	R_col=R->col;
	t3=R_col-1;
	t2=s->T;
	for(i=1,fg=s->sm->g,fi=s->sm->r,f1=par->p+m-1,f2=R->p;i<=t2;++i,f1+=par_col,f2+=R_col)
	{
		f=*(++fi)- *f1 * *f2;
		t=s->T+1-i;
		t=min(t,w);
		f3=f1;
		f4=f2;
		j=1;
		while((++j)<=t)
		{
			f-=*(f3+=par_col) * *(++f4);
			//wtk_debug("v[%d][%d]=%f/%f\n",i,j,*f3,*f4);
		}
		t=min(w,i);
		f3=f1;
		f4=f2;
		j=1;
		while((++j)<=t)
		{
			f-=*(f3-=par_col) * *(f4-=t3);
			//wtk_debug("v[%d][%d]=%f/%f\n",i,j,*f3,*f4);
		}
		*(++fg)=f;
	}
	//exit(0);
}

void wtk_syn_pstream_calc_grad5(wtk_syn_pstream_t *s,int m)
{
	int i,j;
	register float *fi,*f1,*f2;
	float *fg,*f3,*f4,*f5,*f6;
	wtk_matf_t *par;
	wtk_matf_t *R;
	int w,t,t2,t3;
	int par_col,R_col;
	register float f;

	w=s->width;
	par=s->par;R=s->sm->R;
	par_col=par->col;
	R_col=R->col;
	t3=R_col-1;
	t2=s->T;
	for(i=1,fg=s->sm->g+1,fi=s->sm->r+1,f1=par->p+m-1,f2=R->p;i<=t2;++i)
	{
		f3=f5=f1;
		f4=f6=f2;
		f=*(fi++)- *f1 * *f2;
		f1+=par_col;f2+=R_col;
		j=1;
		t=s->T+2-i;
		while((++j)<=w)
		{
			if(j<t)
			{
				//f3+=par_col;
				f-=*(f3+=par_col) * *(++f4);

			}
			if(i>=j)
			{
				//f5-=par_col;f6-=R_col-1;
				f-=*(f5-=par_col) * *(f6-=t3);
			}
			//++j;
		}
		*(fg++)=f;
	}
	//exit(0);
}

void wtk_syn_pstream_calc_grad3(wtk_syn_pstream_t *s,int m)
{
	int i,j;
	float *fi,*f1,*f2;
	register float *fg,*f3,*f4;
	wtk_matf_t *par;
	wtk_matf_t *R;
	int v;

	par=s->par;R=s->sm->R;
	for(i=1,fg=s->sm->g+1,fi=s->sm->r+1,f1=par->p+m-1,f2=R->p;i<=s->T;++i,++fg,++fi,f1+=par->col,f2+=R->col)
	{
		*fg=*fi- *f1 * *f2;
		v=min(s->width,s->T+1-i);
		f3=f1;//+par->col;
		f4=f2;//+1;
		j=1;
		//wtk_debug("v=%d\n",v);
		while(j<v)
		{
			f3+=par->col;++f4;
			*fg-=*f3 * *f4;
			++j;
		}
		v=min(s->width,i);
		//f3=wtk_matf_at(par,i-2,m-1);
		f3=f1;//-par->col;
		f4=f2;//-R->col+1;
		j=1;
		//wtk_debug("v=%d/%d\n",v,s->width);
		while(j<v)
		{
			f3-=par->col;f4-=R->col-1;
			*fg-=*f3 * *f4;
			++j;
		}
	}
}


void wtk_syn_pstream_calc_stepnw(wtk_syn_pstream_t *s,int m,float alpha,float n)
{
	int i;
	float coef,w1,w2,dv,h;
	float f1;
	float av,var;
	register float *f,*fs,*pf1;
	wtk_matf_t *mf;

	//swtk_debug("step inx\n");
	if(alpha>1.0 || alpha<0.0)
	{
		w1=1.0;
		w2=1.0;
	}else
	{
		w1=alpha;
		w2=1.0-alpha;
	}
	wtk_syn_pstream_calc_varstats2(s,m,&av,&var);

	//wtk_debug("av=%f var=%f\n",av,var);
	dv=var-s->vm[m];
	//wtk_debug("dv=%f %f,%f\n",dv,var,s->vm[m]);
	//coef=2.0 * s->vv[m]/s->T;
	coef=s->vv[m]*s->coef1;
	//wtk_debug("coef=%f\n",coef);
	wtk_syn_pstream_calc_grad(s,m);

	w1/=n;
	f1=w2*coef*dv;
	mf=s->par;
	for(i=1,f=s->sm->g+1,pf1=mf->p+m-1;i<=s->T;++i,++f,pf1+=mf->col)
	{
		*f=w1*(*f) - f1*(*pf1-av);
		//wtk_debug("v[%d]=%f\n",i,s->sm->g[i]);
		//wtk_debug("%f/%f\n",*pf1,*wtk_matf_at(mf,i-1,m-1));
	}
	coef=s->vv[m]*s->coef2;
	//wtk_debug("coef=%f\n",coef);
	mf=s->sm->R;
	f=s->sm->b+1;
	fs=s->sm->g+1;
	f1=w2*coef;
	dv=dv*(s->T-1)*f1;
	f1=f1*2.0;
	for(i=1,pf1=mf->p;i<=s->T;++i,++f,++fs,pf1+=mf->col)
	{
		//h=-w1*mx[i][1]-f1*(*f * *f)- dv;//w2*coef*(2.0*(*f * *f)+dv);
		h=-w1* *pf1-f1*(*f * *f)- dv;//w2*coef*(2.0*(*f * *f)+dv);
		//wtk_debug("%f/%f\n",*pf1,*wtk_matf_at(mf,i-1,0));
		if(h==0.0)
		{
			*f=0;
		}else
		{
			*f=*fs/h;
		}
		//wtk_debug("v[%d]=%f/%f\n",i,h,s->sm->b[i]);
	}
	//exit(0);
}


// generate parameter sequence from pdf sequence using gradient
void wtk_syn_pstream_mlpg_grannw(wtk_syn_pstream_t *s,int max,float th,float e,float alpha,int nrmflag,int vcflag)
{
	int m,i,t;
	float n,dth,diff,tf;
	float *f1,*f2;
	int col;
	int T;
	float ft;

	//wtk_debug("%f\n",s->par[1][1]);
	/* parameter generation */
	wtk_syn_pstream_mlpg(s);
//	wtk_debug("%f\n",s->par[1][1]);
//	exit(0);
	if(!s->vm || !s->vv){goto end;}
	// parameter generation considering GV
	if (nrmflag == 1)
	{
		n=s->T*s->vsize*1.0/s->dim;
	}else
	{
		n = 1.0;
	}
	col=s->par->col;
	T=s->T;
	ft=1.0/T;
	//wtk_debug("%f\n",s->par[1][1]);
	//wtk_debug("n=%f\n",n);
	for(m=1;m<=s->dim;++m)
	{
		wtk_syn_pstream_calc_R_and_r(s,m);
		dth=th*sqrt(s->vm[m]);
		//wtk_debug("dth=%f\n",dth);
		//wtk_debug("v[%d]=%f/%f vcflag=%d\n",m,s->par->p[0],s->par->p[1],vcflag);
		if(vcflag==1)
		{
			wtk_syn_pstream_varconv(s,s->par,m,s->vm[m]);
		}
		for(i=0;i<max;++i)
		{
			//wtk_debug("m=%d/%d %d/%d\n",m,s->dim,i,max);
			wtk_syn_pstream_calc_stepnw(s,m,alpha,n);
			//wtk_debug("%d/%d %d/%d %f/%f\n",m,s->dim,i,max,s->sm->b[1],s->sm->b[2]);
			//for(t=1,diff=0.0,f1=s->par->p+m-1,f2=s->sm->b+1;t<=s->T;++t,f1+=s->par->col,++f2)
			for(t=1,diff=0.0,f1=s->par->p+m-1,f2=s->sm->b+1;t<=T;++t,f1+=col,++f2)
			{
				tf=*f2;
				diff+=tf*tf;//s->sm->b[t]*s->sm->b[t];
				//wtk_debug("%f/%f\n",*wtk_matf_at(s->par,t-1,m-1),*f1);
				*f1-=e*tf;//s->sm->b[t];
				//wtk_debug("v[%d]=%f/%f\n",t,diff,s->par[t][m]);
			}
			//diff=sqrt(diff/s->T);
			diff=sqrt(diff*ft);
			//wtk_debug("v[%d]=%f dth=%f\n",i,diff,dth); // add by dmd
			if(diff<dth || diff==0.0)
			{
				break;
			}
		}
	}
	//exit(0);
end:
	//exit(0);
	return;
}


