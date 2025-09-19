#include "wtk_syn_hmm.h" 
#include "wtk/core/cfg/wtk_source.h"

int wtk_syn_hmm_init_rbin_dur(wtk_syn_hmm_t *hmm)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	FILE *f;
	int ret=-1;
	wtk_source_t src,*psrc=NULL;
	int i;

	hmm->rbin_item_dur=wtk_rbin2_get(hmm->rbin,hmm->cfg->dur_fn,strlen(hmm->cfg->dur_fn));
	if(!hmm->rbin_item_dur)
	{
		wtk_debug("load rbin %s failed\n",hmm->cfg->dur_fn);
		goto end;
	}
	f=hmm->rbin->f;
//	wtk_debug("f=%p\n",f);
	psrc=&(src);
	wtk_source_init_fd(psrc,f,hmm->rbin_item_dur->pos);
	ret=wtk_source_read_int(psrc,&i,1,1);
	if(ret!=0){goto end;}
	hmm->nstate=i & 0x00ffffff;
	hmm->ntree=wtk_syn_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_DUR,2);
	hmm->ndurpdf=(int*)wtk_heap_zalloc(heap,sizeof(int)*hmm->ntree);
	ret=wtk_source_read_int(psrc,&(hmm->ndurpdf[0]),hmm->ntree,1);
	if(ret!=0){goto end;}
//	{
//		for(i=0;i<hmm->ntree;++i)
//		{
//			wtk_debug("v[%d]=%d\n",i,hmm->ndurpdf[i]);
//		}
//	}
	ret=0;
end:
	if(psrc)
	{
		wtk_source_clean_fd(psrc);
	}
	return ret;
}

wtk_syn_float_t* wtk_syn_hmm_get_durpdf(wtk_syn_hmm_t *hmm,int idx1,int idx2)
{
	wtk_rbin2_t *rbin=hmm->rbin;
	FILE *f=rbin->f;
	int ret;
	wtk_syn_float_t *pf;
	wtk_source_t src,*psrc=NULL;
	int i;
	int v=hmm->nstate*2;
	int cnt=0;

	psrc=&(src);
	for(i=0;i<idx1;++i)
	{
		cnt+=hmm->ndurpdf[i]*v;
	}
	cnt+=idx2*v;
	wtk_source_init_fd(psrc,f,hmm->rbin_item_dur->pos+(1+hmm->ntree+cnt)*sizeof(int));
	pf=(wtk_syn_float_t*)wtk_malloc(sizeof(wtk_syn_float_t)*v);
	ret=wtk_source_read_float(psrc,pf,v,1);
	if(ret!=0){goto end;}
end:
	wtk_source_clean_fd(psrc);
	return pf;
}

int wtk_syn_hmm_load_dur(wtk_syn_hmm_t *hmm,wtk_source_t *src)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	int ret;
	int i,ntree,ver,j;
	int save_var=0;
	float *normal_base=NULL;
	float *tmp=NULL;
	float **f1;
	float *f2;
	int v1,v2;

	//wtk_debug("sawp=%d\n",src->swap);
	ret=wtk_source_read_int(src,&i,1,1);
	if(ret!=0){goto end;}
	ver=i>>24;
	hmm->nstate=i & 0x00ffffff;
	//wtk_debug("ver=%d nstate=%d\n",ver,hmm->nstate);
	ntree=wtk_syn_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_DUR,2);
	//wtk_debug("i=%d\n",i);
	//wtk_debug("ntreee=%d\n",ntree);
	hmm->ndurpdf=(int*)wtk_heap_zalloc(heap,sizeof(int)*ntree);
	hmm->durpdf=(wtk_syn_float_t***)wtk_heap_zalloc(heap,sizeof(wtk_syn_float_t**)*ntree);

	ret=wtk_source_read_int(src,&(hmm->ndurpdf[0]),ntree,1);
	if(ret!=0){goto end;}

//	for(i=0;i<ntree;++i)
//	{
//		ret=wtk_source_read_int(src,&(hmm->ndurpdf[i]),1,1);
//		if(ret!=0){goto end;}
//		//wtk_debug("v[%d]=%d\n",i,hmm->ndurpdf[i]);
//	}
//	printf("version=%d\n",ver);
	if(ver==1)
	{
		normal_base=(float*)wtk_calloc(2,sizeof(float));
		ret=wtk_source_read_float(src,normal_base,2,1);
		if(ret!=0){goto end;}
		ret=wtk_source_read_int(src,&save_var,1,1);
		if(ret!=0){goto end;}
	}
	v1=2*hmm->nstate*sizeof(wtk_syn_float_t);
	v2=2*hmm->nstate;
	tmp=(float*)wtk_malloc(sizeof(float)*2*hmm->nstate);
	//read pdf mean & variance
	for(i=0;i<ntree;++i)
	{
		f1=hmm->durpdf[i]=(wtk_syn_float_t**)wtk_heap_malloc(heap,sizeof(wtk_syn_float_t*)*hmm->ndurpdf[i]);
		//wtk_debug("%d/%d\n",i,hmm->ndurpdf[i]);
		for(j=0;j<hmm->ndurpdf[i];++j)
		{
			//hmm->durpdf[i][j]=(wtk_syn_float_t*)wtk_heap_malloc(heap,2*hmm->nstate*sizeof(wtk_syn_float_t));
			f2=f1[j]=(wtk_syn_float_t*)wtk_heap_malloc(heap,v1);
			if(ver==0)
			{
				ret=wtk_source_read_float(src,f2,v2,1);
//				int ti;
//				for(ti=0; ti<v2;ti++){
//			       printf("t=%f\n", f2[ti]);
//				}
				if(ret!=0){goto end;}

//				ret=wtk_source_read_float(src,tmp,v2,1);
//				if(ret!=0){goto end;}
//				for(k=0;k<hmm->nstate;++k)
//				{
//					f2[k<<1]=tmp[k<<1];
//					f2[(k<<1)+1]=tmp[k<<1 + 1];
//					wtk_debug("v[%d]: %d/%d/%d\n",k,1<<k,(1<<k) + 1,v2);
//				}
//				exit(0);

//				for(k=0;k<hmm->nstate;++k)
//				{
//					ret=wtk_source_read_float(src,tmp,2,1);
//					if(ret!=0){goto end;}
//					hmm->durpdf[i][j][k<<1]=tmp[0];
//					hmm->durpdf[i][j][(k<<1)+1]=tmp[1];
//				}
			}else if(ver==1)
			{
				/*
				for(k=0;k<hmm->nstate;++k)
				{
					ret=wtk_source_read_short(src,&ntmp,1,1);
					if(ret!=0){goto end;}
					hmm->durpdf[i][j][k]=ntmp*normal_base[0]/32767;
				}
				if(save_var)
				{
					for(k=0;k<hmm->n)
				}*/
				/*
				for (j = 2; j < (nstate + 2); j++) {
					AIS_HTS_Fread(&ntemp, sizeof(short), 1, fp, ver);
					durpdf[t][i][j] = (double)(ntemp * normal_base[0] / 32767);
				}
				if(save_var){
					for (j = nstate + 2; j < 2 * (nstate + 1); j++) {
						AIS_HTS_Fread(&ntemp, sizeof(short), 1, fp, ver);
						durpdf[t][i][j] = (double)(ntemp * normal_base[1] / 32767);
					}
				}else{
					for (j = nstate + 2; j < 2 * (nstate + 1); j++) {
						durpdf[t][i][j] = (double)0.0;
					}
				}*/
			}
		}
	}
	ret=0;
end:
	if(tmp)
	{
		wtk_free(tmp);
	}
//	exit(0);
	if(normal_base)
	{
		wtk_free(normal_base);
	}
	//exit(0);
	return ret;
}

int wtk_syn_hmm_init_rbin_lf0(wtk_syn_hmm_t *hmm)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	wtk_source_t src,*psrc=NULL;
	FILE *f;
	int i,ret;
	int vx=0;

	hmm->rbin_item_f0=wtk_rbin2_get(hmm->rbin,hmm->cfg->lf0_fn,strlen(hmm->cfg->lf0_fn));
	f=hmm->rbin->f;
	psrc=&(src);
	wtk_source_init_fd(psrc,f,hmm->rbin_item_f0->pos);
	//wtk_debug("f0=%d\n",(int)ftell(f));
	ret=wtk_source_read_int(psrc,&i,1,1);
	if(ret!=0){goto end;}
	vx+=1;
    hmm->lf0stream=i & 0x00ffffff;
	//wtk_debug("ver=%d/%d\n",hmm->lf0stream,hmm->nstate);
	hmm->nlf0tree=(int*)wtk_heap_malloc(heap,hmm->nstate*sizeof(int));
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nlf0tree[i]=wtk_syn_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_LF0,i+2);
		//wtk_debug("v[%d]=%d\n",i,hmm->nlf0tree[i]);
	}
	hmm->nlf0pdf=(int**)wtk_heap_malloc(heap,hmm->nstate*sizeof(int*));
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nlf0pdf[i]=(int*)wtk_heap_malloc(heap,sizeof(int)*hmm->nlf0tree[i]);
		ret=wtk_source_read_int(psrc,hmm->nlf0pdf[i],hmm->nlf0tree[i],1);
		if(ret!=0){goto end;}
		vx+=hmm->nlf0tree[i];
		//wtk_debug("v[%d]=%d\n",i,hmm->nlf0pdf[i][0]);
	}
	hmm->f0_pos=vx*4;
	//wtk_debug("f0=%d\n",hmm->f0_pos);
	ret=0;
end:
	wtk_source_clean_fd(psrc);
	return ret;
}

wtk_syn_float_t** wtk_syn_hmm_get_lf0pdf(wtk_syn_hmm_t *hmm,int state,int idx1,int idx2)
{
	wtk_rbin2_t *rbin=hmm->rbin;
	FILE *f=rbin->f;
	int ret;
	wtk_source_t src,*psrc=NULL;
	int cnt1,cnt2,cnt3;
	int i,j,v1;
	int vx;
	wtk_syn_float_t **vp1,*vp2;

	cnt1=sizeof(wtk_syn_float_t*)*hmm->lf0stream;
	cnt2=sizeof(wtk_syn_float_t)*4*hmm->lf0stream;
	cnt3=4*hmm->lf0stream;
	vx=0;
	for(i=0;i<state;++i)
	{
		v1=hmm->nlf0tree[i];
		for(j=0;j<v1;++j)
		{
			vx+=hmm->nlf0pdf[i][j]*cnt3;
		}
	}
	for(j=0;j<idx1;++j)
	{
		vx+=hmm->nlf0pdf[state][j]*cnt3;
	}
	vx+=idx2*cnt3;
	psrc=&(src);
	//wtk_debug("pos=%d %d\n",vx,(int)(hmm->f0_pos+vx*sizeof(wtk_syn_float_t)));
	wtk_source_init_fd(psrc,f,hmm->rbin_item_f0->pos+hmm->f0_pos+vx*sizeof(wtk_syn_float_t));
	//wtk_debug("fpos=%d\n",(int)ftell(f))
	vp1=(wtk_syn_float_t**)wtk_malloc(cnt1);
	vp2=(wtk_syn_float_t*)wtk_malloc(cnt2);
	ret=wtk_source_read_float(psrc,vp2,cnt3,1);
	if(ret!=0){goto end;}
	for(i=0;i<hmm->lf0stream;++i)
	{
		vp1[i]=vp2+i*4;
	}
end:
	wtk_source_clean_fd(psrc);
	return vp1;
}

int wtk_syn_hmm_load_lf0(wtk_syn_hmm_t *hmm,wtk_source_t *src)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	int ret,i,ver,j,k,l;
	float *normal_base=NULL;
	float ****f1;
	float ***f2;
	float **f3;
	float *f4;
	int cnt1,cnt2,cnt3;
	int v1,v2;

	//wtk_debug("pos=%d\n",(int)ftell(hmm->rbin->f));
	ret=wtk_source_read_int(src,&i,1,1);
	if(ret!=0){goto end;}
	ver=i>>24;
    hmm->lf0stream=i & 0x00ffffff;
    //wtk_debug("n=%d s=%d\n",hmm->nstate,hmm->lf0stream);
	//wtk_debug("ver=%d i=%d\n",ver,i);
	hmm->nlf0tree=(int*)wtk_heap_malloc(heap,hmm->nstate*sizeof(int));
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nlf0tree[i]=wtk_syn_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_LF0,i+2);
		//wtk_debug("v[%d]=%d\n",i,hmm->nlf0tree[i]);
	}
	hmm->nlf0pdf=(int**)wtk_heap_malloc(heap,hmm->nstate*sizeof(int*));
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nlf0pdf[i]=(int*)wtk_heap_malloc(heap,sizeof(int)*hmm->nlf0tree[i]);
		ret=wtk_source_read_int(src,hmm->nlf0pdf[i],hmm->nlf0tree[i],1);
		if(ret!=0){goto end;}
		//wtk_debug("v[%d]=%d\n",i,hmm->nlf0pdf[i][0]);
	}
	if(ver==1)
	{
		normal_base=(float*)wtk_calloc(3*hmm->lf0stream,sizeof(float));
		ret=wtk_source_read_float(src,normal_base,3*hmm->lf0stream,1);
		if(ret!=0){goto end;}
	}
	hmm->lf0pdf=(wtk_syn_float_t *****)wtk_heap_malloc(heap,hmm->nstate*sizeof(wtk_syn_float_t****));
	cnt1=sizeof(wtk_syn_float_t*)*hmm->lf0stream;
	cnt2=sizeof(wtk_syn_float_t)*4*hmm->lf0stream;
	cnt3=4*hmm->lf0stream;
	//wtk_debug("state=%d ver=%d\n",hmm->nstate,ver);
	//wtk_debug("pos=%d\n",(int)ftell(hmm->rbin->f));
	for(i=0;i<hmm->nstate;++i)
	{
		f1=hmm->lf0pdf[i]=(wtk_syn_float_t****)wtk_heap_malloc(heap,hmm->nlf0tree[i]*sizeof(wtk_syn_float_t***));
		v1=hmm->nlf0tree[i];
		for(j=0;j<v1;++j)
		{
			//hmm->lf0pdf[i][j]=(wtk_syn_float_t***)wtk_heap_malloc(heap,hmm->nlf0pdf[i][j]*sizeof(wtk_syn_float_t**));
			f2=f1[j]=(wtk_syn_float_t***)wtk_heap_malloc(heap,hmm->nlf0pdf[i][j]*sizeof(wtk_syn_float_t**));
			v2=hmm->nlf0pdf[i][j];
			for(l=0;l<v2;++l)
			{
//				if(i==0 && j==0 && l==2189)
//				{
//					wtk_debug("pos=%d\n",(int)ftell(hmm->rbin->f));
//				}
				//hmm->lf0pdf[i][j][l]=(wtk_syn_float_t**)wtk_heap_malloc(heap,sizeof(wtk_syn_float_t*)*hmm->lf0stream);
				f3=f2[l]=(wtk_syn_float_t**)wtk_heap_malloc(heap,cnt1);//sizeof(wtk_syn_float_t*)*hmm->lf0stream);
				f4=(wtk_syn_float_t*)wtk_heap_malloc(heap,cnt2);//sizeof(wtk_syn_float_t)*4*hmm->lf0stream);
				ret=wtk_source_read_float(src,f4,cnt3,1);
				if(ret!=0){goto end;}
				for(k=0;k<hmm->lf0stream;++k)
				{
					f3[k]=f4+k*4;
				}
			}
		}
	}
	ret=0;
end:
	//exit(0);
	return ret;
}

int wtk_syn_hmm_init_rbin_lf0gv(wtk_syn_hmm_t *hmm)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	wtk_source_t src,*psrc=NULL;
	FILE *f;
	int i,ret;

	hmm->rbin_item_f0gv=wtk_rbin2_get(hmm->rbin,hmm->cfg->lf0_gv_fn,strlen(hmm->cfg->lf0_gv_fn));
	f=hmm->rbin->f;
	psrc=&(src);
	wtk_source_init_fd(psrc,f,hmm->rbin_item_f0gv->pos);
	//wtk_debug("f0=%d\n",(int)ftell(f));
	ret=wtk_source_read_int(psrc,&i,1,1);
	if(ret!=0){goto end;}
	hmm->lf0gvdim=i;
	hmm->nlf0gvtree=wtk_syn_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_LF0GV,2);
	hmm->nlf0gvpdf=(int*)wtk_heap_malloc(heap,hmm->nlf0gvtree*sizeof(int));
	ret=wtk_source_read_int(psrc,hmm->nlf0gvpdf,hmm->nlf0gvtree,1);
	if(ret!=0){goto end;}
end:
	wtk_source_clean_fd(psrc);
	return 0;
}

wtk_syn_float_t* wtk_syn_hmm_get_lf0gvpdf(wtk_syn_hmm_t *hmm,int idx1,int idx2)
{
	wtk_rbin2_t *rbin=hmm->rbin;
	wtk_source_t src,*psrc=NULL;
	FILE *f=rbin->f;
	int i;
	int vx;
	wtk_syn_float_t *pf;
	int t=hmm->lf0gvdim*2;

	vx=1+hmm->nlf0gvtree;
	for(i=0;i<hmm->nlf0gvtree;++i)
	{
		vx+=hmm->nlf0gvpdf[i]*hmm->lf0gvdim*2;
	}
	psrc=&(src);
	wtk_source_init_fd(psrc,f,hmm->rbin_item_f0gv->pos+vx*4);
	pf=(wtk_syn_float_t*)wtk_malloc(sizeof(wtk_syn_float_t)*t);
	wtk_source_read_float(psrc,pf,t,1);
	wtk_source_clean_fd(psrc);
	for(i=hmm->lf0gvdim;i<t;++i)
	{
		pf[i]=1.0/pf[i];
	}
	return pf;
}

int wtk_syn_hmm_load_lf0_gv(wtk_syn_hmm_t *hmm,wtk_source_t *src)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	int ret,i,j,k;
	float tmp;

	ret=wtk_source_read_int(src,&i,1,1);
	if(ret!=0){goto end;}
	if(i!=1)
	{
		wtk_debug("vector size of log f0 GV part must be one[%d].\n",i);
		return -1;
	}
	hmm->lf0gvdim=i;
	hmm->nlf0gvtree=wtk_syn_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_LF0GV,2);
	hmm->nlf0gvpdf=(int*)wtk_heap_malloc(heap,hmm->nlf0gvtree*sizeof(int));
	ret=wtk_source_read_int(src,hmm->nlf0gvpdf,hmm->nlf0gvtree,1);
	if(ret!=0){goto end;}
	hmm->lf0gvpdf=(wtk_syn_float_t***)wtk_heap_malloc(heap,sizeof(wtk_syn_float_t**)*hmm->nlf0gvtree);
	for(i=0;i<hmm->nlf0gvtree;++i)
	{
		hmm->lf0gvpdf[i]=(wtk_syn_float_t**)wtk_heap_malloc(heap,sizeof(wtk_syn_float_t*)*hmm->nlf0gvpdf[i]);
		for(j=0;j<hmm->nlf0gvpdf[i];++j)
		{
			hmm->lf0gvpdf[i][j]=(wtk_syn_float_t*)wtk_heap_malloc(heap,sizeof(wtk_syn_float_t)*2*hmm->lf0gvdim);
			for(k=0;k<hmm->lf0gvdim;++k)
			{
				ret=wtk_source_read_float(src,&tmp,1,1);
				if(ret!=0){goto end;}
				hmm->lf0gvpdf[i][j][k]=tmp;
				//wtk_debug("v[%d]=%f\n",k,hmm->lf0gvpdf[i][j][k]);
			}
			for(;k<2*hmm->lf0gvdim;++k)
			{
				ret=wtk_source_read_float(src,&tmp,1,1);
				if(ret!=0){goto end;}
				if(tmp>0)
				{
					hmm->lf0gvpdf[i][j][k]=1.0/tmp;
				}
				//wtk_debug("v[%d]=%f\n",k,hmm->lf0gvpdf[i][j][k]);
			}
		}
	}
	ret=0;
end:
	return ret;
}

int wtk_syn_hmm_init_rbin_mcp(wtk_syn_hmm_t *hmm)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	wtk_source_t src,*psrc=NULL;
	FILE *f;
	int i,ret;
	int *state=NULL;

	hmm->rbin_item_mcp=wtk_rbin2_get(hmm->rbin,hmm->cfg->mcp_fn,strlen(hmm->cfg->mcp_fn));
	f=hmm->rbin->f;
	psrc=&(src);
	wtk_source_init_fd(psrc,f,hmm->rbin_item_mcp->pos);
	//wtk_debug("f0=%d\n",(int)ftell(f));
	ret=wtk_source_read_int(psrc,&i,1,1);
	if(ret!=0){goto end;}
	//wtk_debug("i=%d\n",i);
	//ver=i>>24;
	hmm->mcepvsize=i & 0x00ffffff;
	hmm->nmceptree=(int*)wtk_heap_malloc(heap,hmm->nstate*sizeof(int));
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nmceptree[i]=wtk_syn_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_MCP,i+2);
	}
	hmm->nmceppdf=(int**)wtk_heap_malloc(heap,sizeof(int*)*hmm->nstate);
	state=(int*)wtk_malloc(sizeof(int)*hmm->nstate);
	ret=wtk_source_read_int(psrc,state,hmm->nstate,1);
	if(ret!=0){goto end;}
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nmceppdf[i]=(int*)wtk_heap_malloc(heap,hmm->nmceptree[i]*sizeof(int));
		hmm->nmceppdf[i][0]=state[i];
		//ret=wtk_source_read_int(src,hmm->nmceppdf[i],1,1);
		//if(ret!=0){goto end;}
	}
end:
	if(state)
	{
		wtk_free(state);
	}
	wtk_source_clean_fd(psrc);
	return 0;
}

wtk_syn_float_t* wtk_syn_hmm_get_mcp(wtk_syn_hmm_t *hmm,int state,int idx1,int idx2)
{
	wtk_rbin2_t *rbin=hmm->rbin;
	wtk_source_t src,*psrc=NULL;
	FILE *f=rbin->f;
	int i,j;
	int vx;
	wtk_syn_float_t *pf;
	int vt=2*hmm->mcepvsize;

	vx=(hmm->nstate+1);
	for(i=0;i<state;++i)
	{
		for(j=0;j<hmm->nmceptree[i];++j)
		{
			vx+=hmm->nmceppdf[i][j]*vt;
			//wtk_debug("state=%d/%d/%d vx=%d n=%d/%d\n",state,i,j,vx,hmm->nmceppdf[i][j],vt);
		}
	}
	//wtk_debug("state=%d/%d/%d vx=%d\n",state,idx1,idx2,vx);
	for(j=0;j<idx1;++j)
	{
		vx+=hmm->nmceppdf[state][j]*vt;
	}
	vx+=idx2*vt;
	psrc=&(src);
	//wtk_debug("state=%d/%d/%d vx=%d\n",state,idx1,idx2,vx);
	wtk_source_init_fd(psrc,f,hmm->rbin_item_mcp->pos+vx*4);
	pf=(wtk_syn_float_t*)wtk_malloc(sizeof(wtk_syn_float_t)*vt);
	wtk_source_read_float(psrc,pf,vt,1);
	wtk_source_clean_fd(psrc);
	return pf;
}



int wtk_syn_hmm_load_mcp(wtk_syn_hmm_t *hmm,wtk_source_t *src)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	int i,ret,j,k;
	float **f,*f1;
	int n;
	//float **p=NULL;
	int *state=NULL;

	ret=wtk_source_read_int(src,&i,1,1);
	if(ret!=0){goto end;}
	//wtk_debug("i=%d\n",i);
	//ver=i>>24;
	hmm->mcepvsize=i & 0x00ffffff;
	hmm->nmceptree=(int*)wtk_heap_malloc(heap,hmm->nstate*sizeof(int));
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nmceptree[i]=wtk_syn_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_MCP,i+2);
	}
	hmm->nmceppdf=(int**)wtk_heap_malloc(heap,sizeof(int*)*hmm->nstate);
	state=(int*)wtk_malloc(sizeof(int)*hmm->nstate);
	ret=wtk_source_read_int(src,state,hmm->nstate,1);
	if(ret!=0){goto end;}
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nmceppdf[i]=(int*)wtk_heap_malloc(heap,hmm->nmceptree[i]*sizeof(int));
		hmm->nmceppdf[i][0]=state[i];
		//ret=wtk_source_read_int(src,hmm->nmceppdf[i],1,1);
		//if(ret!=0){goto end;}
	}
	hmm->mceppdf=(wtk_syn_float_t****)wtk_heap_malloc(heap,hmm->nstate*sizeof(wtk_syn_float_t***));
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->mceppdf[i]=(wtk_syn_float_t***)wtk_heap_malloc(heap,hmm->nmceptree[i]*sizeof(wtk_syn_float_t**));
		for(j=0;j<hmm->nmceptree[i];++j)
		{
			//wtk_debug("v[%d/%d]\n",i,j);
			f=hmm->mceppdf[i][j]=(wtk_syn_float_t**)wtk_heap_malloc(heap,hmm->nmceppdf[i][j]*sizeof(wtk_syn_float_t*));
			n=hmm->nmceppdf[i][j];
			for(k=0;k<n;++k)
			{
				f1=f[k]=(wtk_syn_float_t*)wtk_heap_malloc(heap,2*hmm->mcepvsize*sizeof(wtk_syn_float_t));
				ret=wtk_source_read_float(src,f1,2*hmm->mcepvsize,1);
				if(ret!=0){goto end;}
			}
		}
	}
	ret=0;
end:
	if(state)
	{
		wtk_free(state);
	}
//	i=0;j=0;k=1098;
//	if(i==0 && j==0 && k==1098)
//	{
//		wtk_debug("=========> i=%d j=%d k=%d %p/%p/%p\n",i,j,k,hmm->mceppdf[i],hmm->mceppdf[i][j],hmm->mceppdf[i][j][k]);
//	}
	//exit(0);
	return ret;
}

int wtk_syn_hmm_load_mcp_gv(wtk_syn_hmm_t *hmm,wtk_source_t *src)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	int i,ret,j,k;
	float tmp;

	ret=wtk_source_read_int(src,&i,1,1);
	if(ret!=0){goto end;}
	//wtk_debug("i=%d\n",i);
	hmm->mcepgvdim=i;
	hmm->nmcepgvtree=wtk_syn_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_MCPGV,2);
	hmm->nmcepgvpdf=(int*)wtk_heap_malloc(heap,hmm->nmcepgvtree*sizeof(int));
	ret=wtk_source_read_int(src,hmm->nmcepgvpdf,hmm->nmcepgvtree,1);
	if(ret!=0){goto end;}
	hmm->mcepgvpdf=(wtk_syn_float_t***)wtk_heap_malloc(heap,hmm->nmcepgvtree*sizeof(wtk_syn_float_t**));
	for(i=0;i<hmm->nmcepgvtree;++i)
	{
		hmm->mcepgvpdf[i]=(wtk_syn_float_t**)wtk_heap_malloc(heap,hmm->nmcepgvpdf[i]*sizeof(wtk_syn_float_t*));
		for(j=0;j<hmm->nmcepgvpdf[i];++j)
		{
			hmm->mcepgvpdf[i][j]=(wtk_syn_float_t*)wtk_heap_malloc(heap,2*hmm->mcepgvdim*sizeof(wtk_syn_float_t));
			for(k=0;k<hmm->mcepgvdim;++k)
			{
				ret=wtk_source_read_float(src,&tmp,1,1);
				if(ret!=0){goto end;}
				hmm->mcepgvpdf[i][j][k]=tmp;
				//wtk_debug("v[%d]=%f\n",k,hmm->mcepgvpdf[i][j][k]);
			}
			for(;k<2*hmm->mcepgvdim;++k)
			{
				ret=wtk_source_read_float(src,&tmp,1,1);
				if(ret!=0){goto end;}
				if(tmp>0)
				{
					hmm->mcepgvpdf[i][j][k]=1.0/tmp;
				}
				//wtk_debug("v[%d]=%f\n",k,hmm->mcepgvpdf[i][j][k]);
			}
		}
	}
	ret=0;
end:
	//exit(0);
	return ret;
}

int wtk_syn_hmm_init_rbin_bap(wtk_syn_hmm_t *hmm)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	wtk_source_t src,*psrc=NULL;
	FILE *f;
	int i,ret;
	int vx=0;

	hmm->rbin_item_bap=wtk_rbin2_get(hmm->rbin,hmm->cfg->bap_fn,strlen(hmm->cfg->bap_fn));
	f=hmm->rbin->f;
	psrc=&(src);
	wtk_source_init_fd(psrc,f,hmm->rbin_item_bap->pos);
	//wtk_debug("f0=%d\n",(int)ftell(f));
	ret=wtk_source_read_int(psrc,&i,1,1);
	if(ret!=0){goto end;}
	//wtk_debug("i=%d\n",i);
	//ver=i>>24;
	hmm->bapvsize=i & 0x00ffffff;
	hmm->nbaptree=(int*)wtk_heap_malloc(heap,hmm->nstate*sizeof(int));
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nbaptree[i]=wtk_syn_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_BAP,i+2);
	}
	hmm->nbappdf=(int**)wtk_heap_malloc(heap,hmm->nstate*sizeof(int*));
	vx=1;
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nbappdf[i]=(int*)wtk_heap_malloc(heap,hmm->nbaptree[i]*sizeof(int));
		ret=wtk_source_read_int(psrc,hmm->nbappdf[i],hmm->nbaptree[i],1);
		if(ret!=0){goto end;}
		vx+=hmm->nbaptree[i];
	}
	hmm->bap_pos=vx;
end:
	wtk_source_clean_fd(psrc);
	return 0;
}

wtk_syn_float_t* wtk_syn_hmm_get_bap(wtk_syn_hmm_t *hmm,int state,int idx1,int idx2)
{
	wtk_rbin2_t *rbin=hmm->rbin;
	wtk_source_t src,*psrc=NULL;
	FILE *f=rbin->f;
	int i,j;
	wtk_syn_float_t *pf;
	int v1,v3;//,v4;
	int vx;

	v3=2*hmm->bapvsize;
	//v4=2*hmm->bapvsize*sizeof(wtk_syn_float_t);
	vx=hmm->bap_pos;
	for(i=0;i<state;++i)
	{
		v1=hmm->nbaptree[i];
		for(j=0;j<v1;++j)
		{
			vx+=hmm->nbappdf[i][j]*v3;
		}
	}
	for(j=0;j<idx1;++j)
	{
		vx+=hmm->nbappdf[state][j]*v3;
	}
	vx+=idx2*v3;
	psrc=&(src);
	//wtk_debug("state=%d/%d/%d vx=%d\n",state,idx1,idx2,vx);
	//wtk_source_init_fd(psrc,f,hmm->rbin_item_mcp->pos+vx*4);  //old error
	wtk_source_init_fd(psrc,f,hmm->rbin_item_bap->pos+vx*4);    //add by dmd 2018.03.07, err use mcp.

	pf=(wtk_syn_float_t*)wtk_malloc(sizeof(wtk_syn_float_t)*v3);
	wtk_source_read_float(psrc,pf,v3,1);
	wtk_source_clean_fd(psrc);
	return pf;
}

int wtk_syn_hmm_load_bap(wtk_syn_hmm_t *hmm,wtk_source_t *src)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	int i,ret,j,k;
	int v1,v2,v3,v4;
	float ***f1;
	float **f2;
	float *f3;

	ret=wtk_source_read_int(src,&i,1,1);
	if(ret!=0){goto end;}
	//wtk_debug("i=%d\n",i);
	//ver=i>>24;
	hmm->bapvsize=i & 0x00ffffff;
	hmm->nbaptree=(int*)wtk_heap_malloc(heap,hmm->nstate*sizeof(int));
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nbaptree[i]=wtk_syn_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_BAP,i+2);
	}
	hmm->nbappdf=(int**)wtk_heap_malloc(heap,hmm->nstate*sizeof(int*));
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nbappdf[i]=(int*)wtk_heap_malloc(heap,hmm->nbaptree[i]*sizeof(int));
		ret=wtk_source_read_int(src,hmm->nbappdf[i],hmm->nbaptree[i],1);
		if(ret!=0){goto end;}
	}
	hmm->bappdf=(wtk_syn_float_t****)wtk_heap_malloc(heap,hmm->nstate*sizeof(wtk_syn_float_t***));
	v3=2*hmm->bapvsize;
	v4=2*hmm->bapvsize*sizeof(wtk_syn_float_t);
	for(i=0;i<hmm->nstate;++i)
	{
		f1=hmm->bappdf[i]=(wtk_syn_float_t***)wtk_heap_malloc(heap,hmm->nbaptree[i]*sizeof(wtk_syn_float_t**));
		v1=hmm->nbaptree[i];
		for(j=0;j<v1;++j)
		{
			//hmm->bappdf[i][j]=(wtk_syn_float_t**)wtk_heap_malloc(heap,hmm->nbappdf[i][j]*sizeof(wtk_syn_float_t*));
			f2=f1[j]=(wtk_syn_float_t**)wtk_heap_malloc(heap,hmm->nbappdf[i][j]*sizeof(wtk_syn_float_t*));
			v2=hmm->nbappdf[i][j];
			for(k=0;k<v2;++k)
			{
				//hmm->bappdf[i][j][k]=(wtk_syn_float_t*)wtk_heap_malloc(heap,2*hmm->bapvsize*sizeof(wtk_syn_float_t));
				f3=f2[k]=(wtk_syn_float_t*)wtk_heap_malloc(heap,v4);
				ret=wtk_source_read_float(src,f3,v3,1);
				if(ret!=0){goto end;}
			}
		}
	}
end:
	//exit(0);
	return 0;
}


int wtk_syn_hmm_load_bap_gv(wtk_syn_hmm_t *hmm,wtk_source_t *src)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	int i,ret,j,k;
	float tmp;

	ret=wtk_source_read_int(src,&i,1,1);
	if(ret!=0){goto end;}
	//wtk_debug("i=%d\n",i);
	hmm->bapgvdim=i;
	hmm->nbapgvtree=wtk_syn_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_BAPGV,2);
	hmm->nbapgvpdf=(int*)wtk_heap_malloc(heap,hmm->nbapgvtree*sizeof(int));
	ret=wtk_source_read_int(src,hmm->nbapgvpdf,hmm->nbapgvtree,1);
	if(ret!=0){goto end;}
	hmm->bapgvpdf=(wtk_syn_float_t***)wtk_heap_malloc(heap,hmm->nbapgvtree*sizeof(wtk_syn_float_t**));
	for(i=0;i<hmm->nbapgvtree;++i)
	{
		hmm->bapgvpdf[i]=(wtk_syn_float_t**)wtk_heap_malloc(heap,hmm->nbapgvpdf[i]*sizeof(wtk_syn_float_t*));
		for(j=0;j<hmm->nbapgvpdf[i];++j)
		{
			hmm->bapgvpdf[i][j]=(wtk_syn_float_t*)wtk_heap_malloc(heap,2*hmm->bapgvdim*sizeof(wtk_syn_float_t));
			for(k=0;k<hmm->bapgvdim;++k)
			{
				ret=wtk_source_read_float(src,&tmp,1,1);
				if(ret!=0){goto end;}
				hmm->bapgvpdf[i][j][k]=tmp;
			}
			for(;k<2*hmm->bapgvdim;++k)
			{
				ret=wtk_source_read_float(src,&tmp,1,1);
				if(ret!=0){goto end;}
				if(tmp>0)
				{
					hmm->bapgvpdf[i][j][k]=1.0/tmp;
				}
			}
		}

	}
end:
	return 0;
}

wtk_syn_hmm_t* wtk_syn_hmm_new(wtk_syn_hmm_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool,wtk_syn_dtree_t *dtree)
{
	wtk_syn_hmm_t *h;
	int ret;

	h=(wtk_syn_hmm_t*)wtk_calloc(1,sizeof(wtk_syn_hmm_t));
	h->cfg=cfg;
	h->pool=pool;
	h->dtree=dtree;
	h->ndurpdf=NULL;
	h->durpdf=NULL;
	h->nstate=0;
	h->lf0stream=0;
	//read duration model
	//wtk_debug("load %s\n",cfg->dur_fn);

	h->rbin=(wtk_rbin2_t*)sl->hook;
	//wtk_debug("load_all=%d\n",cfg->load_all);
	if(!h->rbin && !cfg->load_all)
	{
		cfg->load_all=1;
	}
	//wtk_debug("load_all=%d\n",cfg->load_all);
	if(cfg->load_all)
	{
		ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_syn_hmm_load_dur,cfg->dur_fn);
		if(ret!=0){goto end;}
		//read log f0 model
		ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_syn_hmm_load_lf0,cfg->lf0_fn);
		if(ret!=0){goto end;}
		ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_syn_hmm_load_mcp,cfg->mcp_fn);
		if(ret!=0){goto end;}
		if (cfg->bap_fn)
		{
			ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_syn_hmm_load_bap,cfg->bap_fn);
			if(ret!=0){goto end;}
		}
	}else
	{
		h->ndurpdf=NULL;
		h->durpdf=NULL;
		wtk_syn_hmm_init_rbin_dur(h);
		wtk_syn_hmm_init_rbin_lf0(h);
		wtk_syn_hmm_init_rbin_mcp(h);
		if(cfg->bap_fn)
		    wtk_syn_hmm_init_rbin_bap(h);
	}
	// read GV of log F0 model
	if(cfg->lf0_gv_fn)
	{
		ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_syn_hmm_load_lf0_gv,cfg->lf0_gv_fn);
		if(ret!=0){goto end;}
	}else
	{
		h->lf0gvpdf=NULL;
	}
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	if(cfg->mcp_gv_fn)
	{
		ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_syn_hmm_load_mcp_gv,cfg->mcp_gv_fn);
		if(ret!=0){goto end;}
	}else
	{
		h->mcepgvpdf=NULL;
	}
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	if(cfg->bap_gv_fn)
	{
		ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_syn_hmm_load_bap_gv,cfg->bap_gv_fn);
		if(ret!=0){goto end;}
	}else
	{
		h->bapgvpdf=NULL;
	}
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	ret=0;
end:
    if (ret !=0){
    	wtk_debug("Warming: maybe exist error!\n");
    }
	return h;
}

void wtk_syn_hmm_delete(wtk_syn_hmm_t *hmm)
{
	wtk_free(hmm);
}

wtk_syn_gv_pdf_t* wtk_syn_gv_pdf_new(wtk_heap_t *heap,int vsize)
{
	wtk_syn_gv_pdf_t *gv;

	gv=(wtk_syn_gv_pdf_t*)wtk_heap_malloc(heap,sizeof(wtk_syn_gv_pdf_t));
	gv->var=wtk_vector_new_h(heap,vsize);
	gv->mean=wtk_vector_new_h(heap,vsize);
	return gv;
}






