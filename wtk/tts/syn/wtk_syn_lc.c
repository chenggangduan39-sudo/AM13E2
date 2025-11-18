#include "wtk_syn_lc.h" 
wtk_syn_hmm_lc_t* wtk_syn_hmm_lc_new(wtk_syn_hmm_t *hmm,wtk_heap_t *heap,char *name,int name_bytes)
{
	wtk_syn_hmm_lc_t *lc;

	lc=(wtk_syn_hmm_lc_t*)wtk_heap_malloc(heap,sizeof(wtk_syn_hmm_lc_t));
	lc->phn=NULL;
	lc->name=wtk_heap_dup_string(heap,name,name_bytes);
	lc->hmm=hmm;
	lc->totaldur=0;
	lc->dur=(int*)wtk_heap_malloc(heap,sizeof(int)*hmm->nstate);

	lc->lf0mean=wtk_matrix_newh(heap,hmm->nstate,hmm->lf0stream);
	lc->lf0variance=wtk_matrix_newh(heap,hmm->nstate,hmm->lf0stream);
	lc->voiced=(char*)wtk_heap_malloc(heap,sizeof(char)*hmm->nstate);

	lc->mcepmean=wtk_matrix_newh(heap,hmm->nstate,hmm->mcepvsize);
	lc->mcepvariance=wtk_matrix_newh(heap,hmm->nstate,hmm->mcepvsize);

	lc->bapmean=wtk_matrix_newh(heap,hmm->nstate,hmm->bapvsize);
	lc->bapvariance=wtk_matrix_newh(heap,hmm->nstate,hmm->bapvsize);

	return lc;
}

void wtl_syn_hmm_lc_find_lf0pdf(wtk_syn_hmm_lc_t* lc,int *idx,int state,float uvthresh)
{
	wtk_syn_hmm_t *hmm=lc->hmm;
	int i;
	float **f;
	float *fm,*fv;

	//wtk_debug("idx=%d/%d\n",idx[0],idx[1]);
	if(hmm->cfg->load_all)
	{
		f=hmm->lf0pdf[state][idx[0]][idx[1]-1];
	}else
	{
		f=wtk_syn_hmm_get_lf0pdf(hmm,state,idx[0],idx[1]-1);
	}
	//wtk_debug("%d/%d/%d\n",state,idx[0],idx[1]-1);
	//wtk_debug("%f/%f\n",f[0][0],f[0][1]);
	//exit(0);
	fm=lc->lf0mean[state+1];
	fv=lc->lf0variance[state+1];
	for(i=0;i<hmm->lf0stream;++i)
	{
		//wtk_debug("v[%d]=%f/%f\n",i,f[i][0],f[i][1]);
		fm[i+1]=f[i][0];
		fv[i+1]=f[i][1];
		if(i==0)
		{
			lc->voiced[state]=f[i][2]>uvthresh?1:0;
			//wtk_debug("state[%d]=%d thresh=%f/%f %f/%f\n",state,lc->voiced[state],f[i][2],uvthresh,fm[i+1],fv[i+1]);
		}
	}
	if(!hmm->cfg->load_all)
	{
		wtk_free(f[0]);
		wtk_free(f);
	}
	//exit(0);
}


void wtl_syn_hmm_lc_find_mcppdf(wtk_syn_hmm_lc_t* lc,int *idx,int state)
{
	wtk_syn_hmm_t *hmm=lc->hmm;
	float *f1,*f2;
	int n;

	//wtk_debug("state=%d/%d/%d\n",state,idx[0],idx[1]-1);
	//wtk_debug("idx=%d/%d state=%d %p/%p\n",idx[0],idx[1],state,lc->hmm->mceppdf[state],lc->hmm->mceppdf[state][idx[0]]);
	if(hmm->cfg->load_all)
	{
		f1=lc->hmm->mceppdf[state][idx[0]][idx[1]-1];
		f2=lc->hmm->mceppdf[state][idx[0]][idx[1]-1]+lc->hmm->mcepvsize;
		n=sizeof(float)*lc->hmm->mcepvsize;
		memcpy(lc->mcepmean[state+1]+1,f1,n);
		memcpy(lc->mcepvariance[state+1]+1,f2,n);
	}else
	{
		f1=wtk_syn_hmm_get_mcp(hmm,state,idx[0],idx[1]-1);
		f2=f1+lc->hmm->mcepvsize;
		n=sizeof(float)*lc->hmm->mcepvsize;
		memcpy(lc->mcepmean[state+1]+1,f1,n);
		memcpy(lc->mcepvariance[state+1]+1,f2,n);
		wtk_free(f1);
	}
	/*
	int i;
	for(i=1;i<=lc->hmm->mcepvsize;++i)
	{
		//wtk_debug("v[%d]=%f\n",i,f1[i-1]);
		lc->mcepmean[state+1][i]=f1[i-1];
		lc->mcepvariance[state+1][i]=f2[i-1];
		//wtk_debug("v[%d]=%f/%f\n",i,lc->mcepmean[state+1][i],lc->mcepvariance[state+1][i]);
	}*/
	//exit(0);
}

void wtk_syn_hmm_lc_find_bappdf(wtk_syn_hmm_lc_t* lc,int *idx,int state)
{
	wtk_syn_hmm_t *hmm=lc->hmm;
	float *f1,*f2;
	int n;

	if(hmm->cfg->load_all)
	{
		f1=lc->hmm->bappdf[state][idx[0]][idx[1]-1];
		f2=lc->hmm->bappdf[state][idx[0]][idx[1]-1]+lc->hmm->bapvsize;
		n=sizeof(float)*lc->hmm->bapvsize;
		memcpy(lc->bapmean[state+1]+1,f1,n);
		memcpy(lc->bapvariance[state+1]+1,f2,n);
	}else
	{
		f1=wtk_syn_hmm_get_bap(hmm,state,idx[0],idx[1]-1);
		f2=f1+lc->hmm->bapvsize;
		n=sizeof(float)*lc->hmm->bapvsize;
		memcpy(lc->bapmean[state+1]+1,f1,n);
		memcpy(lc->bapvariance[state+1]+1,f2,n);
		wtk_free(f1);   // add by dmd 2018.03.07
	}
	/*
	int i;
	for(i=1;i<=lc->hmm->bapvsize;++i)
	{
		lc->bapmean[state+1][i]=f1[i-1];
		lc->bapvariance[state+1][i]=f2[i-1];
		//wtk_debug("v[%d]=%f/%f\n",i,lc->bapmean[state+1][i],lc->bapvariance[state+1][i]);
	}*/
	//exit(0);
}

void wtk_syn_hmm_lc_set_output_pdfs(wtk_syn_hmm_lc_t *lc,char *s,int s_bytes)
{
	int nstate;
	int idx[2];
	int i,ret;

	//wtk_debug("[%.*s]\n",s_bytes,s);
	nstate=lc->hmm->nstate;
	for(i=0;i<nstate;++i)
	{
		ret=wtk_syn_dtree_search(lc->hmm->dtree,WTK_SYN_DTREE_TREE_LF0,i+2,s,s_bytes,idx);
		if(ret!=0){goto end;}
		//wtk_debug("lf0 v[%d]=%d/%d\n",i,idx[0],idx[1]);
		//exit(0);
		wtl_syn_hmm_lc_find_lf0pdf(lc,idx,i,lc->hmm->cfg->uv);
	}
	for(i=0;i<nstate;++i)
	{
		ret=wtk_syn_dtree_search(lc->hmm->dtree,WTK_SYN_DTREE_TREE_MCP,i+2,s,s_bytes,idx);
		if(ret!=0){goto end;}
		//wtk_debug("mcp %d/%d\n",idx[0],idx[1]);
		wtl_syn_hmm_lc_find_mcppdf(lc,idx,i);
	}
	//if(lc->hmm->bappdf)      //old version, don't support bin res with load_all=0.
	if(lc->hmm->cfg->bap_fn)   //add by dmd123 at 2018.03.07
	{
		for(i=0;i<nstate;++i)
		{
			ret=wtk_syn_dtree_search(lc->hmm->dtree,WTK_SYN_DTREE_TREE_BAP,i+2,s,s_bytes,idx);
			if(ret!=0){goto end;}
			//wtk_debug("bap idx=%d/%d\n",idx[0],idx[1]);
			wtk_syn_hmm_lc_find_bappdf(lc,idx,i);
		}
	}
end:
	return;
}

//#include "wtk/asr/net/wtk_transcription.h"
//wtk_queue_node_t* wtk_syn_hmm_lc_find_setdur(wtk_syn_hmm_lc_t *lc, wtk_queue_node_t *n)
//{
//	wtk_lab_t *lab;
//	int i,ti;
//
//	wtk_syn_hmm_t *hmm=lc->hmm;
//	lc->totaldur=0;
//	for(i=0;i<hmm->nstate;++i,n=n->next)
//	{
//		lab=data_offset(n,wtk_lab_t,lablist_n);
//		ti=(int)(lab->end-lab->start)/50000;
//		//wtk_debug("lab->end=%f lab->start=%f ti=%d\n",lab->end,lab->start,ti);
//		lc->dur[i]=ti;
//		lc->totaldur+=ti;
//	}
//	return n;
//}

void wtk_syn_hmm_lc_find_durpdf(wtk_syn_hmm_lc_t *lc,float rho,int *idx,float *diff)
{
	wtk_syn_hmm_t *hmm=lc->hmm;
	wtk_syn_float_t *pf;
	float f;
	int i,ti;

	if(hmm->cfg->load_all)
	{
		pf=hmm->durpdf[idx[0]][idx[1]-1];
	}else
	{
		pf=wtk_syn_hmm_get_durpdf(hmm,idx[0],idx[1]-1);
	}
	lc->totaldur=0;
	for(i=0;i<hmm->nstate;++i)
	{
		//wtk_debug("d[%d][%d][%d]=%f\n",idx[0],idx[1]-1-1,i,pf[i]);
		f=pf[i]*rho;
		//wtk_debug("%f/%f/%f diff=%f\n",pf[i],rho,f,*diff);
		ti=(int)(f+*diff+0.5);
		//wtk_debug("ti=%d\n",ti);
		ti=ti<1?1:ti;
		lc->dur[i]=ti;
		lc->totaldur+=ti;
		//wtk_debug("v[%d]=%d f=%f\n",i,ti,f);
		*diff+=f-ti;
		//wtk_debug("diff=%f f=%f ti=%d\n",*diff,f,ti);
	}
	if(!hmm->cfg->load_all)
	{
		wtk_free(pf);
	}
}



void wtk_syn_hmm_lc_print(wtk_syn_hmm_lc_t *lc)
{
	int i;

	wtk_debug("================= hmm lc =============\n");
	printf("%.*s totdur: %d\n",lc->name->len, lc->name->data, lc->totaldur);
	for(i=0;i<lc->hmm->nstate;++i)
	{
		printf("dur[%d]=%d\n",i,lc->dur[i]);
	}
//	for(i=0;i<lc->hmm->nstate;++i)
//	{
//		printf("v[%d]=%d\n",i,lc->dur[i]);
//	}
}


wtk_syn_lc_t* wtk_syn_lc_new(wtk_heap_t *heap,wtk_syn_hmm_t *hmm)
{
	wtk_syn_lc_t *lc;

	lc=(wtk_syn_lc_t*)wtk_heap_malloc(heap,sizeof(wtk_syn_lc_t));
	lc->hmm=hmm;
	if(hmm->lf0gvpdf)
	{
		lc->lf0gv=wtk_syn_gv_pdf_new(heap,hmm->lf0gvdim);
	}else
	{
		lc->lf0gv=NULL;
	}
	if(hmm->mcepgvpdf)
	{
		lc->mcegv=wtk_syn_gv_pdf_new(heap,hmm->mcepgvdim);
	}else
	{
		lc->mcegv=NULL;
	}
	if(hmm->bapgvpdf)
	{
		wtk_debug("bapgvpdf %d\n",hmm->bapgvdim);
		lc->bapgv=wtk_syn_gv_pdf_new(heap,hmm->bapgvdim);
	}else
	{
		lc->bapgv=NULL;
	}
	wtk_syn_lc_reset(lc);
	return lc;
}

void wtk_syn_lc_reset(wtk_syn_lc_t *lc)
{
	lc->use_lf0gv=0;
	lc->use_mcegv=0;
	lc->use_bapgv=0;
	lc->totframe=0;
	wtk_queue_init(&(lc->lc_q));
}

void wtk_syn_lc_find_lf0gvpdf(wtk_syn_lc_t *lc,int *idx)
{
	//wtk_syn_hmm_t *hmm=lc->hmm;
	wtk_syn_gv_pdf_t *pdf;
	float *f1,*f2;
	int i;

	f1=lc->hmm->lf0gvpdf[idx[0]][idx[1]-1];
	f2=f1+lc->hmm->lf0gvdim;
	pdf=lc->lf0gv;
	//wtk_debug("pdf=%p\n",lc->lf0gv);
	for(i=1;i<=lc->hmm->lf0gvdim;++i)
	{
		pdf->mean[i]=f1[i-1];
		pdf->var[i]=f2[i-1];
		//wtk_debug("lf0gv v[%d]=%f/%f\n",i,pdf->mean[i],pdf->var[i]);
	}
}

void wtk_syn_lc_find_mcpgvpdf(wtk_syn_lc_t *lc,int *idx)
{
	wtk_syn_gv_pdf_t *pdf;
	float *f1,*f2;
	int i;

	f1=lc->hmm->mcepgvpdf[idx[0]][idx[1]-1];
	f2=f1+lc->hmm->mcepgvdim;
	pdf=lc->mcegv;
	for(i=1;i<=lc->hmm->mcepgvdim;++i)
	{
		pdf->mean[i]=f1[i-1];
		pdf->var[i]=f2[i-1];
		//wtk_debug("mcpgv v[%d]=%f/%f\n",i,pdf->mean[i],pdf->var[i]);
	}
}

void wtk_syn_lc_find_bapgvpdf(wtk_syn_lc_t *lc,int *idx)
{
	wtk_syn_gv_pdf_t *pdf;
	float *f1,*f2;
	int i;

	f1=lc->hmm->bapgvpdf[idx[0]][idx[1]-1];
	f2=f1+lc->hmm->bapgvdim;
	pdf=lc->bapgv;
	for(i=1;i<=lc->hmm->bapgvdim;++i)
	{
		pdf->mean[i]=f1[i-1];
		pdf->var[i]=f2[i-1];
		wtk_debug("bapgv v[%d]=%f/%f\n",i,pdf->mean[i],pdf->var[i]);
	}
}


void wtk_syn_lc_set_gv_pdfs(wtk_syn_lc_t *lc,char *s,int s_bytes)
{
	int idx[2];
	int ret;

	if(lc->hmm->lf0gvpdf && !lc->use_lf0gv)
	{
		ret=wtk_syn_dtree_search(lc->hmm->dtree,WTK_SYN_DTREE_TREE_LF0GV,2,s,s_bytes,idx);
		if(ret!=0)
		{
			wtk_debug("lf0 gv not found\n");
			goto end;
		}
		//wtk_debug("lf0gv %d/%d\n",idx[0],idx[1]);
		wtk_syn_lc_find_lf0gvpdf(lc,idx);
		lc->use_lf0gv=1;
		//wtl_syn_hmm_lc_find_lf0pdf(lc,idx,i,lc->hmm->cfg->uv);
	}
	if(lc->hmm->mcepgvpdf && !lc->use_mcegv)
	{
		ret=wtk_syn_dtree_search(lc->hmm->dtree,WTK_SYN_DTREE_TREE_MCPGV,2,s,s_bytes,idx);
		if(ret!=0)
		{
			wtk_debug("lf0 gv not found\n");
			goto end;
		}
		//wtk_debug("mcpgv %d/%d\n",idx[0],idx[1]);
		wtk_syn_lc_find_mcpgvpdf(lc,idx);
		lc->use_mcegv=1;
	}
	if(lc->hmm->bapgvpdf && !lc->use_bapgv)
	{
		ret=wtk_syn_dtree_search(lc->hmm->dtree,WTK_SYN_DTREE_TREE_BAPGV,2,s,s_bytes,idx);
		if(ret!=0)
		{
			wtk_debug("lf0 gv not found\n");
			goto end;
		}
		//wtk_debug("bapgv %d/%d\n",idx[0],idx[1]);
		wtk_syn_lc_find_bapgvpdf(lc,idx);
		lc->use_bapgv=1;
	}
	ret=0;
end:
	return;
}



