#include "wtk_cosynthesis_lc.h" 

wtk_cosynthesis_hmm_lc_t* wtk_cosynthesis_hmm_lc_new(wtk_cosynthesis_hmm_t *hmm,wtk_heap_t *heap,char *name,int name_bytes)
{
	wtk_cosynthesis_hmm_lc_t *lc;

	lc=(wtk_cosynthesis_hmm_lc_t*)wtk_heap_malloc(heap,sizeof(wtk_cosynthesis_hmm_lc_t));
	// lc->phn=NULL;
	lc->name=wtk_heap_dup_string(heap,name,name_bytes);
	lc->hmm=hmm;
	lc->totaldur=0;

	lc->durmean=wtk_matrix_newh(heap,1,hmm->nstate);
	lc->durvariance=wtk_matrix_newh(heap,1,hmm->nstate);

	lc->lf0mean=wtk_matrix_newh(heap,1,hmm->lf0stream);
	lc->lf0variance=wtk_matrix_newh(heap,1,hmm->lf0stream);
 	lc->lf0weight = wtk_matrix_newh(heap,1,hmm->lf0stream);

	lc->mcepmean=wtk_matrix_newh(heap,1,hmm->mcepvsize);
	lc->mcepvariance=wtk_matrix_newh(heap,1,hmm->mcepvsize);

	lc->conca_lf0mean=wtk_matrix_newh(heap,hmm->conca_nstate,hmm->conca_lf0stream);
	lc->conca_lf0variance=wtk_matrix_newh(heap,hmm->conca_nstate,hmm->conca_lf0stream);
	lc->conca_lf0weight=wtk_matrix_newh(heap,hmm->conca_nstate,hmm->conca_lf0stream);

	lc->conca_mcepmean=wtk_matrix_newh(heap,hmm->conca_nstate,hmm->conca_mcepvsize);
	lc->conca_mcepvariance=wtk_matrix_newh(heap,hmm->conca_nstate,hmm->conca_mcepvsize);

	return lc;
}

void wtk_cosynthesis_hmm_lc_find_lf0pdf(wtk_cosynthesis_hmm_lc_t* lc,int *idx,int state,float uvthresh)
{
	wtk_cosynthesis_hmm_t *hmm=lc->hmm;
	int i;
	float **f;
	float *fm,*fv,*fw;

	//wtk_debug("idx=%d/%d\n",idx[0],idx[1]);
	if(hmm->cfg->load_all)
	{
		f=hmm->lf0pdf[state][idx[0]][idx[1]-1];
	}else
	{
		f=wtk_cosynthesis_hmm_get_lf0pdf(hmm,state,idx[0],idx[1]-1);
	}
	//wtk_debug("%d/%d/%d\n",state,idx[0],idx[1]-1);
	//wtk_debug("%f/%f\n",f[0][0],f[0][1]);
	//exit(0);
	fm=lc->lf0mean[1];
	fv=lc->lf0variance[1];
	fw=lc->lf0weight[1];
	for(i=0;i<hmm->lf0stream;++i)
	{
		//wtk_debug("v[%d]=%f/%f\n",i,f[i][0],f[i][1]);
		fm[i+1]=f[i][0];
		fv[i+1]=f[i][1];
		fw[i+1]=f[i][2];
	}
	if(!hmm->cfg->load_all)
	{
		wtk_free(f[0]);
		wtk_free(f);
	}
	//exit(0);
}

void wtk_cosynthesis_hmm_lc_find_conca_lf0pdf(wtk_cosynthesis_hmm_lc_t* lc,int *idx,int state,float uvthresh)
{
	wtk_cosynthesis_hmm_t *hmm=lc->hmm;
	int i;
	float **f;
	float *fm,*fv,*fw;

	//wtk_debug("idx=%d/%d\n",idx[0],idx[1]);
	if(hmm->cfg->load_all)
	{
		f=hmm->conca_lf0pdf[state][idx[0]][idx[1]-1];
	}else
	{
		f=wtk_cosynthesis_hmm_get_conca_lf0pdf(hmm,state,idx[0],idx[1]-1);
	}
	//wtk_debug("%d/%d/%d\n",state,idx[0],idx[1]-1);
	//wtk_debug("%f/%f\n",f[0][0],f[0][1]);
	//exit(0);
	fm=lc->conca_lf0mean[1];
	fv=lc->conca_lf0variance[1];
	fw=lc->conca_lf0weight[1];
	if (hmm->conca_lf0stream!=hmm->cfg->dynamic_pitch->nslot)
	{
		wtk_debug("Error: conca_lf0stream[%d] diff dynamic_pitch[%d]\n", hmm->conca_lf0stream, hmm->cfg->dynamic_pitch->nslot);
	}
	for(i=0;i<hmm->conca_lf0stream;++i)
	{
		//wtk_debug("v[%d]=%f/%f\n",i,f[i][0],f[i][1]);
		fm[i+1]=f[i][0];
		fv[i+1]=f[i][1] + ((float*)hmm->cfg->dynamic_pitch->slot)[i];
		fw[i+1]=f[i][2];
	}
	if(!hmm->cfg->load_all)
	{
		wtk_free(f[0]);
		wtk_free(f);
	}
	//exit(0);
}

void wtk_cosynthesis_hmm_lc_find_mcppdf(wtk_cosynthesis_hmm_lc_t* lc,int *idx,int state)
{
	wtk_cosynthesis_hmm_t *hmm=lc->hmm;
	float *f1,*f2;
	int n;

	//wtk_debug("lc=%p state=%d/%d/%d mcepmean=%p mcepvariance=%p\n",lc, state,idx[0],idx[1]-1, lc->mcepmean, lc->mcepvariance);
	//wtk_debug("idx=%d/%d state=%d %p/%p\n",idx[0],idx[1],state,lc->hmm->mceppdf[state],lc->hmm->mceppdf[state][idx[0]]);
	if(hmm->cfg->load_all)
	{
		f1=lc->hmm->mceppdf[state][idx[0]][idx[1]-1];
		f2=lc->hmm->mceppdf[state][idx[0]][idx[1]-1]+lc->hmm->mcepvsize;
		n=sizeof(float)*lc->hmm->mcepvsize;
		//wtk_debug("n=%d\n", n);
		memcpy(lc->mcepmean[1]+1,f1,n);
		memcpy(lc->mcepvariance[1]+1,f2,n);
		//print_float(f2, n);
	}else
	{
		f1=wtk_cosynthesis_hmm_get_mcp(hmm,state,idx[0],idx[1]-1);
		f2=f1+lc->hmm->mcepvsize;
		n=sizeof(float)*lc->hmm->mcepvsize;
		memcpy(lc->mcepmean[1]+1,f1,n);
		memcpy(lc->mcepvariance[1]+1,f2,n);
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

void wtk_cosynthesis_hmm_lc_find_conca_mcppdf(wtk_cosynthesis_hmm_lc_t* lc,int *idx,int state)
{
	wtk_cosynthesis_hmm_t *hmm=lc->hmm;
	float *f1,*f2;
	int i, n;

	//wtk_debug("state=%d/%d/%d\n",state,idx[0],idx[1]-1);
	//wtk_debug("idx=%d/%d state=%d %p/%p\n",idx[0],idx[1],state,lc->hmm->mceppdf[state],lc->hmm->mceppdf[state][idx[0]]);
	if(hmm->cfg->load_all)
	{
		f1=hmm->conca_mceppdf[state][idx[0]][idx[1]-1];
		f2=hmm->conca_mceppdf[state][idx[0]][idx[1]-1]+hmm->conca_mcepvsize;
		n=sizeof(float)*lc->hmm->conca_mcepvsize;
		memcpy(lc->conca_mcepmean[1]+1,f1,n);
		memcpy(lc->conca_mcepvariance[1]+1,f2,n);
	}else
	{
		f1=wtk_cosynthesis_hmm_get_conca_mcp(hmm,state,idx[0],idx[1]-1);
		f2=f1+lc->hmm->conca_mcepvsize;
		n=sizeof(float)*hmm->conca_mcepvsize;
		memcpy(lc->conca_mcepmean[1]+1,f1,n);
		memcpy(lc->conca_mcepvariance[1]+1,f2,n);
		//for gv
		if (hmm->conca_mcepvsize!=hmm->cfg->dynamic_spec->nslot)
		{
			wtk_debug("Error: conca_mcepvsize diff dynamic_spec\n");
		}
		for (i=0; i < hmm->conca_mcepvsize; i++)
		{
			lc->conca_mcepvariance[1][i+1] += ((float*)hmm->cfg->dynamic_spec->slot)[i];
		}
		//print_float(lc->conca_mcepmean[1]+1, lc->hmm->conca_mcepvsize);
		//print_float(lc->conca_mcepvariance[1]+1, lc->hmm->conca_mcepvsize);
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

void wtk_cosynthesis_hmm_lc_find_bappdf(wtk_cosynthesis_hmm_lc_t* lc,int *idx,int state)
{
	wtk_cosynthesis_hmm_t *hmm=lc->hmm;
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
		f1=wtk_cosynthesis_hmm_get_bap(hmm,state,idx[0],idx[1]-1);
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

void wtk_cosynthesis_hmm_lc_set_output_pdfs(wtk_cosynthesis_hmm_lc_t *lc,char *s,int s_bytes)
{
	int nstate;
	int idx[2];
	int i=2,ret;

	//wtk_debug("[%.*s]\n",s_bytes,s);
	nstate=lc->hmm->nstate;
	ret=wtk_cosynthesis_dtree_search(lc->hmm->dtree,WTK_SYN_DTREE_TREE_LF0,i+2,s,s_bytes,idx);
	if(ret!=0){goto end;}
	// wtk_debug("lf0 v[%d]=%d/%d\n",i,idx[0],idx[1]);
	//exit(0);
	wtk_cosynthesis_hmm_lc_find_lf0pdf(lc,idx,i,lc->hmm->cfg->uv);

	ret=wtk_cosynthesis_dtree_search(lc->hmm->dtree,WTK_SYN_DTREE_TREE_MCP,i+2,s,s_bytes,idx);
	if(ret!=0){goto end;}
	// wtk_debug("mcp %d/%d\n",idx[0],idx[1]);
	wtk_cosynthesis_hmm_lc_find_mcppdf(lc,idx,i);

	nstate = lc->hmm->conca_nstate;   //nstate:1
	for(i=0;i<nstate;++i)
	{
		ret=wtk_cosynthesis_dtree_search(lc->hmm->dtree,WTK_SYN_DTREE_TREE_CONCA_LF0,i+2,s,s_bytes,idx);
		if(ret!=0){goto end;}
		//wtk_debug("lf0 v[%d]=%d/%d\n",i,idx[0],idx[1]);
		//exit(0);
		wtk_cosynthesis_hmm_lc_find_conca_lf0pdf(lc,idx,i,lc->hmm->cfg->uv);
	}
	for(i=0;i<nstate;++i)
	{
		ret=wtk_cosynthesis_dtree_search(lc->hmm->dtree,WTK_SYN_DTREE_TREE_CONCA_MGC,i+2,s,s_bytes,idx);
		if(ret!=0){goto end;}
		//wtk_debug("mcp %d/%d\n",idx[0],idx[1]);
		wtk_cosynthesis_hmm_lc_find_conca_mcppdf(lc,idx,i);
	}

end:
	return;
}

void wtk_cosynthesis_hmm_lc_find_durpdf(wtk_cosynthesis_hmm_lc_t *lc,int *idx)
{
	wtk_cosynthesis_hmm_t *hmm=lc->hmm;
	float *pf;
	//float f;
	int i;//ti;

	if(hmm->cfg->load_all)
	{
		pf=hmm->durpdf[idx[0]][idx[1]-1];
	}else
	{
		pf=wtk_cosynthesis_hmm_get_durpdf(hmm,idx[0],idx[1]-1);
		//print_float(pf, hmm->nstate*2);
	}

	lc->totaldur=0;
	for(i=0;i<hmm->nstate;++i)
	{
		lc->durmean[1][i+1]= pf[i];
		lc->durvariance[1][i+1]= pf[i+hmm->nstate];
		//wtk_debug("pf[%d]=%f\n", i, pf[i]);
		lc->totaldur+=pf[i];
	}
	if(!hmm->cfg->load_all)
	{
		wtk_free(pf);
	}
}


wtk_cosynthesis_lc_t* wtk_cosynthesis_lc_new(wtk_heap_t *heap,wtk_cosynthesis_hmm_t *hmm)
{
	wtk_cosynthesis_lc_t *lc;

	lc=(wtk_cosynthesis_lc_t*)wtk_heap_malloc(heap,sizeof(wtk_cosynthesis_lc_t));
	lc->hmm=hmm;
	wtk_cosynthesis_lc_reset(lc);
	return lc;
}

void wtk_cosynthesis_lc_reset(wtk_cosynthesis_lc_t *lc)
{
	lc->totframe=0;
	wtk_queue_init(&(lc->lc_q));
}


