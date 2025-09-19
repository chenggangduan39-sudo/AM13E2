#include "wtk_cosynthesis_hmm.h" 
#include "wtk/core/cfg/wtk_source.h"

int wtk_cosynthesis_hmm_init_dur(wtk_cosynthesis_hmm_t *hmm, wtk_source_t *src)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	int ret=-1;
	int i;

	ret=wtk_source_read_int(src,&i,1,1);
	if(ret!=0){goto end;}
	hmm->nstate=i & 0x00ffffff;
	hmm->ntree=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_DUR,2);
	hmm->ndurpdf=(int*)wtk_heap_zalloc(heap,sizeof(int)*hmm->ntree);
	ret=wtk_source_read_int(src,&(hmm->ndurpdf[0]),hmm->ntree,1);
	if(ret!=0){goto end;}
	ret=0;
end:
	return ret;
}

float* wtk_cosynthesis_hmm_get_durpdf(wtk_cosynthesis_hmm_t *hmm,int idx1,int idx2)
{
	FILE *f;
	float *pf=NULL;
	wtk_source_t src;
	int i,cnt=0, ret;
	int v=hmm->nstate*2;

	if (hmm->rbin)
		f=wtk_rbin2_get_file(hmm->rbin, hmm->cfg->dur_fn);
	else
	{
		f=fopen(hmm->cfg->dur_fn,"rb");
		if(!f){
			wtk_debug("open failed %s", hmm->cfg->dur_fn);
			goto end;
		}
	}
	for(i=0;i<idx1;++i)
	{
		cnt+=hmm->ndurpdf[i]*v;
	}
	cnt+=idx2*v;
	wtk_source_init_fd(&src,f, ftell(f)+(1+hmm->ntree+cnt)*sizeof(int));
	pf=(float*)wtk_malloc(sizeof(float)*v);
	ret=wtk_source_read_float(&src,pf,v,1);
	if(ret!=0){goto end;}
end:
	wtk_source_clean_fd(&src);
	if (f)
		fclose(f);
	return pf;
}

int wtk_cosynthesis_hmm_load_dur(wtk_cosynthesis_hmm_t *hmm,wtk_source_t *src)
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

	ret=wtk_source_read_int(src,&i,1,1);
	if(ret!=0){goto end;}
	ver=i>>24;
	hmm->nstate=i & 0x00ffffff;
	// wtk_debug("ver=%d nstate=%d\n",ver,hmm->nstate);
	ntree=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_DUR,2);
	//wtk_debug("i=%d\n",i);
	//wtk_debug("ntreee=%d\n",ntree);
	hmm->ndurpdf=(int*)wtk_heap_zalloc(heap,sizeof(int)*ntree);
	hmm->durpdf=(float***)wtk_heap_zalloc(heap,sizeof(float**)*ntree);

	ret=wtk_source_read_int(src,&(hmm->ndurpdf[0]),ntree,1);
	if(ret!=0){goto end;}
	if(ver==1)
	{
		normal_base=(float*)wtk_calloc(2,sizeof(float));
		ret=wtk_source_read_float(src,normal_base,2,1);
		if(ret!=0){goto end;}
		ret=wtk_source_read_int(src,&save_var,1,1);
		if(ret!=0){goto end;}
	}
	v1=2*hmm->nstate*sizeof(float);
	v2=2*hmm->nstate;
	tmp=(float*)wtk_malloc(sizeof(float)*2*hmm->nstate);
	//read pdf mean & variance
	for(i=0;i<ntree;++i)
	{
		f1=hmm->durpdf[i]=(float**)wtk_heap_malloc(heap,sizeof(float*)*hmm->ndurpdf[i]);
		for(j=0;j<hmm->ndurpdf[i];++j)
		{
			//hmm->durpdf[i][j]=(float*)wtk_heap_malloc(heap,2*hmm->nstate*sizeof(float));
			f2=f1[j]=(float*)wtk_heap_malloc(heap,v1);
			if(ver==0)
			{
				ret=wtk_source_read_float(src,f2,v2,1);
				// int ti;
				// for(ti=0; ti<v2;ti++){
			    //    printf("t=%f\n", f2[ti]);
				// }
				// exit(0);
				if(ret!=0){goto end;}
			}else if(ver==1)
			{

			}
		}
	}
	ret=0;
end:
	if(tmp)
	{
		wtk_free(tmp);
	}
	if(normal_base)
	{
		wtk_free(normal_base);
	}
	return ret;
}

int wtk_cosynthesis_hmm_init_lf0(wtk_cosynthesis_hmm_t *hmm, wtk_source_t *src)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	int i,ret;
	int vx=0;

	ret=wtk_source_read_int(src,&i,1,1);
	if(ret!=0){goto end;}
	vx+=1;
    hmm->lf0stream=i & 0x00ffffff;
	// wtk_debug("ver=%d/%d\n",hmm->lf0stream,hmm->nstate);
	hmm->nlf0tree=(int*)wtk_heap_malloc(heap,hmm->nstate*sizeof(int));
	for(i=0;i<hmm->nstate;++i)
	{
		// hmm->nlf0tree[i]=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_LF0,i+2);
		hmm->nlf0tree[i]=1;	//默认都只有一颗树  将树去掉之后 hmm 还有相应值
		// wtk_debug("v[%d]=%d\n",i,hmm->nlf0tree[i]);
	}
	hmm->nlf0pdf=(int**)wtk_heap_malloc(heap,hmm->nstate*sizeof(int*));
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nlf0pdf[i]=(int*)wtk_heap_malloc(heap,sizeof(int)*hmm->nlf0tree[i]);
		ret=wtk_source_read_int(src,hmm->nlf0pdf[i],hmm->nlf0tree[i],1);
		if(ret!=0){goto end;}
		vx+=hmm->nlf0tree[i];
		//wtk_debug("v[%d]=%d\n",i,hmm->nlf0pdf[i][0]);
	}
	hmm->f0_pos=vx*4;
	//wtk_debug("f0=%d\n",hmm->f0_pos);
	ret=0;
end:
	return ret;
}

float** wtk_cosynthesis_hmm_get_lf0pdf(wtk_cosynthesis_hmm_t *hmm,int state,int idx1,int idx2)
{
	FILE *f;
	int ret;
	wtk_source_t src;
	int cnt1,cnt2,cnt3;
	int i,j,v1;
	int vx;
	float **vp1=NULL,*vp2;

	if (hmm->rbin)
		f = wtk_rbin2_get_file(hmm->rbin, hmm->cfg->lf0_fn);
	else
	{
		f=fopen(hmm->cfg->lf0_fn,"rb");
		if(!f){
			wtk_debug("open failed %s", hmm->cfg->lf0_fn);
			goto end;
		}
	}

	cnt1=sizeof(float*)*hmm->lf0stream;
	cnt2=sizeof(float)*4*hmm->lf0stream;
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
	wtk_source_init_fd(&src,f, ftell(f)+hmm->f0_pos+vx*sizeof(float));
	vp1=(float**)wtk_malloc(cnt1);
	vp2=(float*)wtk_malloc(cnt2);
	ret=wtk_source_read_float(&src,vp2,cnt3,1);
	if(ret!=0){goto end;}
	for(i=0;i<hmm->lf0stream;++i)
	{
		vp1[i]=vp2+i*4;
	}
end:
	wtk_source_clean_fd(&src);
	if (f)
		fclose(f);
	return vp1;
}

int wtk_cosynthesis_hmm_load_lf0(wtk_cosynthesis_hmm_t *hmm,wtk_source_t *src)
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
    // wtk_debug("n=%d s=%d\n",hmm->nstate,hmm->lf0stream);
	//wtk_debug("ver=%d i=%d\n",ver,i);
	hmm->nlf0tree=(int*)wtk_heap_malloc(heap,hmm->nstate*sizeof(int));
	for(i=0;i<hmm->nstate;++i)
	{
		// hmm->nlf0tree[i]=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_LF0,i+2);
		hmm->nlf0tree[i]=1;
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
	hmm->lf0pdf=(float *****)wtk_heap_malloc(heap,hmm->nstate*sizeof(float****));
	cnt1=sizeof(float*)*hmm->lf0stream;
	cnt2=sizeof(float)*4*hmm->lf0stream;
	cnt3=4*hmm->lf0stream;
	//wtk_debug("cnt1= %d\n",cnt1);
	// wtk_debug("cnt3 %d\n",cnt3);
	//wtk_debug("state=%d ver=%d\n",hmm->nstate,ver);
	//wtk_debug("pos=%d\n",(int)ftell(hmm->rbin->f));
	for(i=0;i<hmm->nstate;++i)
	{
		f1=hmm->lf0pdf[i]=(float****)wtk_heap_malloc(heap,hmm->nlf0tree[i]*sizeof(float***));
		v1=hmm->nlf0tree[i];
		for(j=0;j<v1;++j)
		{
			//hmm->lf0pdf[i][j]=(float***)wtk_heap_malloc(heap,hmm->nlf0pdf[i][j]*sizeof(float**));
			f2=f1[j]=(float***)wtk_heap_malloc(heap,hmm->nlf0pdf[i][j]*sizeof(float**));
			v2=hmm->nlf0pdf[i][j];
			//wtk_debug("state=%d v1=%d v2=%d\n",i, v1, v2);
			for(l=0;l<v2;++l)
			{
//				if(i==0 && j==0 && l==2189)
//				{
//					wtk_debug("pos=%d\n",(int)ftell(hmm->rbin->f));
//				}
				//hmm->lf0pdf[i][j][l]=(float**)wtk_heap_malloc(heap,sizeof(float*)*hmm->lf0stream);
				f3=f2[l]=(float**)wtk_heap_malloc(heap,cnt1);//sizeof(float*)*hmm->lf0stream);
				f4=(float*)wtk_heap_malloc(heap,cnt2);//sizeof(float)*4*hmm->lf0stream);
				ret=wtk_source_read_float(src,f4,cnt3,1);
				// wtk_debug("lf0===== %p %d %d\n",f3,j,l);
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

//int wtk_cosynthesis_hmm_init_rbin_lf0gv(wtk_cosynthesis_hmm_t *hmm)
//{
//	wtk_heap_t *heap=hmm->pool->hash->heap;
//	wtk_source_t src,*psrc=NULL;
//	FILE *f;
//	int i,ret;
//
//	hmm->rbin_item_f0gv=wtk_rbin2_get(hmm->rbin,hmm->cfg->lf0_gv_fn,strlen(hmm->cfg->lf0_gv_fn));
//	f=hmm->rbin->f;
//	psrc=&(src);
//	wtk_source_init_fd(psrc,f,hmm->rbin_item_f0gv->pos);
//	//wtk_debug("f0=%d\n",(int)ftell(f));
//	ret=wtk_source_read_int(psrc,&i,1,1);
//	if(ret!=0){goto end;}
//	hmm->lf0gvdim=i;
//	hmm->nlf0gvtree=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_LF0GV,2);
//	hmm->nlf0gvpdf=(int*)wtk_heap_malloc(heap,hmm->nlf0gvtree*sizeof(int));
//	ret=wtk_source_read_int(psrc,hmm->nlf0gvpdf,hmm->nlf0gvtree,1);
//	if(ret!=0){goto end;}
//end:
//	wtk_source_clean_fd(psrc);
//	return 0;
//}

int wtk_cosynthesis_hmm_init_rbin_lf0gv2(wtk_cosynthesis_hmm_t *hmm)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	wtk_source_t src,*psrc=NULL;
	FILE *f;
	int i,ret;

	if (hmm->rbin)
		f = wtk_rbin2_get_file(hmm->rbin, hmm->cfg->lf0_gv_fn);
	else
	{
		f=fopen(hmm->cfg->lf0_gv_fn,"rb");
		if(!f){
			wtk_debug("open failed %s", hmm->cfg->lf0_gv_fn);
			goto end;
		}
	}
	psrc=&(src);
	wtk_source_init_fd(psrc,f, ftell(f));
	//wtk_debug("f0=%d\n",(int)ftell(f));
	ret=wtk_source_read_int(psrc,&i,1,1);
	if(ret!=0){goto end;}
	hmm->lf0gvdim=i;
	hmm->nlf0gvtree=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_LF0GV,2);
	hmm->nlf0gvpdf=(int*)wtk_heap_malloc(heap,hmm->nlf0gvtree*sizeof(int));
	ret=wtk_source_read_int(psrc,hmm->nlf0gvpdf,hmm->nlf0gvtree,1);
	if(ret!=0){goto end;}
end:
	wtk_source_clean_fd(psrc);
	if(f)fclose(f);
	return 0;
}

float* wtk_cosynthesis_hmm_get_lf0gvpdf(wtk_cosynthesis_hmm_t *hmm,int idx1,int idx2)
{
	wtk_source_t src,*psrc=NULL;
	FILE *f;
	int i;
	int vx;
	float *pf;
	int t=hmm->lf0gvdim*2;

	if (hmm->rbin)
		f = wtk_rbin2_get_file(hmm->rbin, hmm->cfg->lf0_gv_fn);
	else
	{
		f=fopen(hmm->cfg->lf0_gv_fn,"rb");
		if(!f){
			wtk_debug("open failed %s", hmm->cfg->lf0_gv_fn);
			goto end;
		}
	}
	vx=1+hmm->nlf0gvtree;
	for(i=0;i<hmm->nlf0gvtree;++i)
	{
		vx+=hmm->nlf0gvpdf[i]*hmm->lf0gvdim*2;
	}
	psrc=&(src);
	wtk_source_init_fd(psrc,f, ftell(f)+vx*4);
	pf=(float*)wtk_malloc(sizeof(float)*t);
	wtk_source_read_float(psrc,pf,t,1);
	wtk_source_clean_fd(psrc);
	for(i=hmm->lf0gvdim;i<t;++i)
	{
		pf[i]=1.0/pf[i];
	}
end:
	if(f)fclose(f);
	return pf;
}

int wtk_cosynthesis_hmm_load_lf0_gv(wtk_cosynthesis_hmm_t *hmm,wtk_source_t *src)
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
	hmm->nlf0gvtree=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_LF0GV,2);
	hmm->nlf0gvpdf=(int*)wtk_heap_malloc(heap,hmm->nlf0gvtree*sizeof(int));
	ret=wtk_source_read_int(src,hmm->nlf0gvpdf,hmm->nlf0gvtree,1);
	if(ret!=0){goto end;}
	hmm->lf0gvpdf=(float***)wtk_heap_malloc(heap,sizeof(float**)*hmm->nlf0gvtree);
	for(i=0;i<hmm->nlf0gvtree;++i)
	{
		hmm->lf0gvpdf[i]=(float**)wtk_heap_malloc(heap,sizeof(float*)*hmm->nlf0gvpdf[i]);
		for(j=0;j<hmm->nlf0gvpdf[i];++j)
		{
			hmm->lf0gvpdf[i][j]=(float*)wtk_heap_malloc(heap,sizeof(float)*2*hmm->lf0gvdim);
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

int wtk_cosynthesis_hmm_init_mcp(wtk_cosynthesis_hmm_t *hmm, wtk_source_t *src)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	int i,ret;
	int *state=NULL;

	ret=wtk_source_read_int(src,&i,1,1);
	if(ret!=0){goto end;}
	//wtk_debug("i=%d\n",i);
	//ver=i>>24;
	hmm->mcepvsize=i & 0x00ffffff;
	hmm->nmceptree=(int*)wtk_heap_malloc(heap,hmm->nstate*sizeof(int));
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nmceptree[i]=1;	//因为把树给清除了 但是hmm还没变 暂时先这样
		// hmm->nmceptree[i]=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_MCP,i+2);
		// wtk_debug("mcp v[%d]=%d\n",i,hmm->nmceptree[i]);
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
end:
	if(state)
	{
		wtk_free(state);
	}
	return 0;
}

float* wtk_cosynthesis_hmm_get_mcp(wtk_cosynthesis_hmm_t *hmm,int state,int idx1,int idx2)
{
	wtk_source_t src;
	FILE *f;
	int i,j;
	int vx;
	float *pf=NULL;
	int vt=2*hmm->mcepvsize;

	if (hmm->rbin)
		f = wtk_rbin2_get_file(hmm->rbin, hmm->cfg->mcp_fn);
	else
	{
		f=fopen(hmm->cfg->mcp_fn,"rb");
		if(!f){
			wtk_debug("open failed %s", hmm->cfg->mcp_fn);
			goto end;
		}
	}

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
	//wtk_debug("state=%d/%d/%d vx=%d\n",state,idx1,idx2,vx);
	wtk_source_init_fd(&src,f, ftell(f) + vx*4);
	pf=(float*)wtk_malloc(sizeof(float)*vt);
	wtk_source_read_float(&src,pf,vt,1);
	wtk_source_clean_fd(&src);
end:
	if (f)
		fclose(f);
	return pf;
}

int wtk_cosynthesis_hmm_load_mcp(wtk_cosynthesis_hmm_t *hmm,wtk_source_t *src)
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
	//wtk_debug("i=%d\n",hmm->mcepvsize);
	hmm->nmceptree=(int*)wtk_heap_malloc(heap,hmm->nstate*sizeof(int));
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nmceptree[i]=1;
		// hmm->nmceptree[i]=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_MCP,i+2);
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
	hmm->mceppdf=(float****)wtk_heap_malloc(heap,hmm->nstate*sizeof(float***));
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->mceppdf[i]=(float***)wtk_heap_malloc(heap,hmm->nmceptree[i]*sizeof(float**));
		for(j=0;j<hmm->nmceptree[i];++j)
		{
			//wtk_debug("v[%d/%d]\n",i,j);
			f=hmm->mceppdf[i][j]=(float**)wtk_heap_malloc(heap,hmm->nmceppdf[i][j]*sizeof(float*));
			n=hmm->nmceppdf[i][j];
			//wtk_debug("n=%d\n", n);
			for(k=0;k<n;++k)
			{
				f1=f[k]=(float*)wtk_heap_malloc(heap,2*hmm->mcepvsize*sizeof(float));
				ret=wtk_source_read_float(src,f1,2*hmm->mcepvsize,1);
				//print_float(f1, 2*hmm->mcepvsize);
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

int wtk_cosynthesis_hmm_load_mcp_gv(wtk_cosynthesis_hmm_t *hmm,wtk_source_t *src)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	int i,ret,j,k;
	float tmp;

	ret=wtk_source_read_int(src,&i,1,1);
	if(ret!=0){goto end;}
	//wtk_debug("i=%d\n",i);
	hmm->mcepgvdim=i;
	hmm->nmcepgvtree=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_MCPGV,2);
	hmm->nmcepgvpdf=(int*)wtk_heap_malloc(heap,hmm->nmcepgvtree*sizeof(int));
	ret=wtk_source_read_int(src,hmm->nmcepgvpdf,hmm->nmcepgvtree,1);
	if(ret!=0){goto end;}
	hmm->mcepgvpdf=(float***)wtk_heap_malloc(heap,hmm->nmcepgvtree*sizeof(float**));
	for(i=0;i<hmm->nmcepgvtree;++i)
	{
		hmm->mcepgvpdf[i]=(float**)wtk_heap_malloc(heap,hmm->nmcepgvpdf[i]*sizeof(float*));
		for(j=0;j<hmm->nmcepgvpdf[i];++j)
		{
			hmm->mcepgvpdf[i][j]=(float*)wtk_heap_malloc(heap,2*hmm->mcepgvdim*sizeof(float));
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

int wtk_cosynthesis_hmm_init_bap(wtk_cosynthesis_hmm_t *hmm, wtk_source_t *src)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	int i,ret;
	int vx=0;

	ret=wtk_source_read_int(src,&i,1,1);
	if(ret!=0){goto end;}
	//wtk_debug("i=%d\n",i);
	//ver=i>>24;
	hmm->bapvsize=i & 0x00ffffff;
	hmm->nbaptree=(int*)wtk_heap_malloc(heap,hmm->nstate*sizeof(int));
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nbaptree[i]=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_BAP,i+2);
	}
	hmm->nbappdf=(int**)wtk_heap_malloc(heap,hmm->nstate*sizeof(int*));
	vx=1;
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nbappdf[i]=(int*)wtk_heap_malloc(heap,hmm->nbaptree[i]*sizeof(int));
		ret=wtk_source_read_int(src,hmm->nbappdf[i],hmm->nbaptree[i],1);
		if(ret!=0){goto end;}
		vx+=hmm->nbaptree[i];
	}
	hmm->bap_pos=vx;
end:
	return 0;
}

float* wtk_cosynthesis_hmm_get_bap(wtk_cosynthesis_hmm_t *hmm,int state,int idx1,int idx2)
{
	wtk_source_t src;
	FILE *f;
	int i,j;
	float *pf=NULL;
	int v1,v3;//,v4;
	int vx;

	if (hmm->rbin)
		f = wtk_rbin2_get_file(hmm->rbin, hmm->cfg->bap_fn);
	else
	{
		f=fopen(hmm->cfg->bap_fn,"rb");
		if(!f){
			wtk_debug("open failed %s", hmm->cfg->bap_fn);
			goto end;
		}
	}

	v3=2*hmm->bapvsize;
	//v4=2*hmm->bapvsize*sizeof(float);
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
	wtk_source_init_fd(&src,f, ftell(f) + vx*4);    //add by dmd 2018.03.07, err use mcp.

	pf=(float*)wtk_malloc(sizeof(float)*v3);
	wtk_source_read_float(&src,pf,v3,1);
	wtk_source_clean_fd(&src);
end:
	if (f)
		fclose(f);
	return pf;
}

int wtk_cosynthesis_hmm_load_bap(wtk_cosynthesis_hmm_t *hmm,wtk_source_t *src)
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
		hmm->nbaptree[i]=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_BAP,i+2);
	}
	hmm->nbappdf=(int**)wtk_heap_malloc(heap,hmm->nstate*sizeof(int*));
	for(i=0;i<hmm->nstate;++i)
	{
		hmm->nbappdf[i]=(int*)wtk_heap_malloc(heap,hmm->nbaptree[i]*sizeof(int));
		ret=wtk_source_read_int(src,hmm->nbappdf[i],hmm->nbaptree[i],1);
		if(ret!=0){goto end;}
	}
	hmm->bappdf=(float****)wtk_heap_malloc(heap,hmm->nstate*sizeof(float***));
	v3=2*hmm->bapvsize;
	v4=2*hmm->bapvsize*sizeof(float);
	for(i=0;i<hmm->nstate;++i)
	{
		f1=hmm->bappdf[i]=(float***)wtk_heap_malloc(heap,hmm->nbaptree[i]*sizeof(float**));
		v1=hmm->nbaptree[i];
		for(j=0;j<v1;++j)
		{
			//hmm->bappdf[i][j]=(float**)wtk_heap_malloc(heap,hmm->nbappdf[i][j]*sizeof(float*));
			f2=f1[j]=(float**)wtk_heap_malloc(heap,hmm->nbappdf[i][j]*sizeof(float*));
			v2=hmm->nbappdf[i][j];
			for(k=0;k<v2;++k)
			{
				//hmm->bappdf[i][j][k]=(float*)wtk_heap_malloc(heap,2*hmm->bapvsize*sizeof(float));
				f3=f2[k]=(float*)wtk_heap_malloc(heap,v4);
				ret=wtk_source_read_float(src,f3,v3,1);
				if(ret!=0){goto end;}
			}
		}
	}
end:
	//exit(0);
	return 0;
}


int wtk_cosynthesis_hmm_load_bap_gv(wtk_cosynthesis_hmm_t *hmm,wtk_source_t *src)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	int i,ret,j,k;
	float tmp;

	ret=wtk_source_read_int(src,&i,1,1);
	if(ret!=0){goto end;}
	//wtk_debug("i=%d\n",i);
	hmm->bapgvdim=i;
	hmm->nbapgvtree=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_BAPGV,2);
	hmm->nbapgvpdf=(int*)wtk_heap_malloc(heap,hmm->nbapgvtree*sizeof(int));
	ret=wtk_source_read_int(src,hmm->nbapgvpdf,hmm->nbapgvtree,1);
	if(ret!=0){goto end;}
	hmm->bapgvpdf=(float***)wtk_heap_malloc(heap,hmm->nbapgvtree*sizeof(float**));
	for(i=0;i<hmm->nbapgvtree;++i)
	{
		hmm->bapgvpdf[i]=(float**)wtk_heap_malloc(heap,hmm->nbapgvpdf[i]*sizeof(float*));
		for(j=0;j<hmm->nbapgvpdf[i];++j)
		{
			hmm->bapgvpdf[i][j]=(float*)wtk_heap_malloc(heap,2*hmm->bapgvdim*sizeof(float));
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

int wtk_cosynthesis_hmm_init_conca_lf0(wtk_cosynthesis_hmm_t *hmm, wtk_source_t *src)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	int i,ret;
	int vx=0;

	ret=wtk_source_read_int(src,&i,1,1);
	if(ret!=0){goto end;}
	vx+=1;
    hmm->conca_lf0stream=i & 0x00ffffff;
	//wtk_debug("ver=%d/%d\n",hmm->lf0stream,hmm->nstate);
	hmm->conca_nlf0tree=(int*)wtk_heap_malloc(heap,hmm->conca_nstate*sizeof(int));
	for(i=0;i<hmm->conca_nstate;++i)
	{
		hmm->conca_nlf0tree[i]=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_CONCA_LF0,i+2);
		//wtk_debug("v[%d]=%d\n",i,hmm->nlf0tree[i]);
	}
	hmm->conca_nlf0pdf=(int**)wtk_heap_malloc(heap,hmm->conca_nstate*sizeof(int*));
	for(i=0;i<hmm->conca_nstate;++i)
	{
		hmm->conca_nlf0pdf[i]=(int*)wtk_heap_malloc(heap,sizeof(int)*hmm->conca_nlf0tree[i]);
		ret=wtk_source_read_int(src,hmm->conca_nlf0pdf[i],hmm->conca_nlf0tree[i],1);
		if(ret!=0){goto end;}
		vx+=hmm->conca_nlf0tree[i];
		//wtk_debug("v[%d]=%d\n",i,hmm->nlf0pdf[i][0]);
	}
	hmm->conca_f0_pos=vx*4;
	//wtk_debug("f0=%d\n",hmm->f0_pos);
	ret=0;
end:
	return ret;
}
int wtk_cosynthesis_hmm_load_conca_lf0(wtk_cosynthesis_hmm_t *hmm,wtk_source_t *src)
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
    hmm->conca_lf0stream=i & 0x00ffffff;
    //wtk_debug("n=%d s=%d\n",hmm->nstate,hmm->lf0stream);
	//wtk_debug("ver=%d i=%d\n",ver,i);
	hmm->conca_nlf0tree=(int*)wtk_heap_malloc(heap,hmm->conca_nstate*sizeof(int));
	for(i=0;i<hmm->conca_nstate;++i)
	{
		hmm->conca_nlf0tree[i]=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_CONCA_LF0,i+2);
		//wtk_debug("v[%d]=%d\n",i,hmm->nlf0tree[i]);
	}
	hmm->conca_nlf0pdf=(int**)wtk_heap_malloc(heap,hmm->conca_nstate*sizeof(int*));
	for(i=0;i<hmm->conca_nstate;++i)
	{
		hmm->conca_nlf0pdf[i]=(int*)wtk_heap_malloc(heap,sizeof(int)*hmm->conca_nlf0tree[i]);
		ret=wtk_source_read_int(src,hmm->conca_nlf0pdf[i],hmm->conca_nlf0tree[i],1);
		if(ret!=0){goto end;}
		//wtk_debug("v[%d]=%d\n",i,hmm->nlf0pdf[i][0]);
	}
	if(ver==1)
	{
		normal_base=(float*)wtk_calloc(3*hmm->conca_lf0stream,sizeof(float));
		ret=wtk_source_read_float(src,normal_base,3*hmm->conca_lf0stream,1);
		if(ret!=0){goto end;}
	}
	hmm->conca_lf0pdf=(float *****)wtk_heap_malloc(heap,hmm->conca_nstate*sizeof(float****));
	cnt1=sizeof(float*)*hmm->conca_lf0stream;
	cnt2=sizeof(float)*4*hmm->conca_lf0stream;
	cnt3=4*hmm->conca_lf0stream;
	//wtk_debug("state=%d ver=%d\n",hmm->nstate,ver);
	//wtk_debug("pos=%d\n",(int)ftell(hmm->rbin->f));
	for(i=0;i<hmm->conca_nstate;++i)
	{
		f1=hmm->conca_lf0pdf[i]=(float****)wtk_heap_malloc(heap,hmm->conca_nlf0tree[i]*sizeof(float***));
		v1=hmm->conca_nlf0tree[i];
		for(j=0;j<v1;++j)
		{
			//hmm->lf0pdf[i][j]=(float***)wtk_heap_malloc(heap,hmm->nlf0pdf[i][j]*sizeof(float**));
			f2=f1[j]=(float***)wtk_heap_malloc(heap,hmm->conca_nlf0pdf[i][j]*sizeof(float**));
			v2=hmm->conca_nlf0pdf[i][j];
			for(l=0;l<v2;++l)
			{
//				if(i==0 && j==0 && l==2189)
//				{
//					wtk_debug("pos=%d\n",(int)ftell(hmm->rbin->f));
//				}
				//hmm->lf0pdf[i][j][l]=(float**)wtk_heap_malloc(heap,sizeof(float*)*hmm->lf0stream);
				f3=f2[l]=(float**)wtk_heap_malloc(heap,cnt1);//sizeof(float*)*hmm->lf0stream);
				f4=(float*)wtk_heap_malloc(heap,cnt2);//sizeof(float)*4*hmm->lf0stream);
				ret=wtk_source_read_float(src,f4,cnt3,1);
				if(ret!=0){goto end;}
				for(k=0;k<hmm->conca_lf0stream;++k)
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

float** wtk_cosynthesis_hmm_get_conca_lf0pdf(wtk_cosynthesis_hmm_t *hmm,int state,int idx1,int idx2)
{
	FILE *f;
	int ret;
	wtk_source_t src;
	int cnt1,cnt2,cnt3;
	int i,j,v1;
	int vx;
	float **vp1=NULL,*vp2;

	if (hmm->rbin)
		f = wtk_rbin2_get_file(hmm->rbin, hmm->cfg->conca_lf0_fn);
	else
	{
		f=fopen(hmm->cfg->conca_lf0_fn,"rb");
		if(!f){
			wtk_debug("open failed %s", hmm->cfg->conca_lf0_fn);
			goto end;
		}
	}

	cnt1=sizeof(float*)*hmm->conca_lf0stream;
	cnt2=sizeof(float)*4*hmm->conca_lf0stream;
	cnt3=4*hmm->conca_lf0stream;
	vx=0;
	for(i=0;i<state;++i)
	{
		v1=hmm->conca_nlf0tree[i];
		for(j=0;j<v1;++j)
		{
			vx+=hmm->conca_nlf0pdf[i][j]*cnt3;
		}
	}
	for(j=0;j<idx1;++j)
	{
		vx+=hmm->conca_nlf0pdf[state][j]*cnt3;
	}
	vx+=idx2*cnt3;
	//wtk_debug("pos=%d %d\n",vx,(int)(hmm->f0_pos+vx*sizeof(float)));
	wtk_source_init_fd(&src,f, ftell(f) + hmm->conca_f0_pos + vx*sizeof(float));
	//wtk_debug("fpos=%d\n",(int)ftell(f))
	vp1=(float**)wtk_malloc(cnt1);
	vp2=(float*)wtk_malloc(cnt2);
	ret=wtk_source_read_float(&src,vp2,cnt3,1);
	if(ret!=0){goto end;}
	for(i=0;i<hmm->conca_lf0stream;++i)
	{
		vp1[i]=vp2+i*4;
	}
end:
	wtk_source_clean_fd(&src);
	if (f)
		fclose(f);
	return vp1;
}

int wtk_cosynthesis_hmm_init_conca_mcp(wtk_cosynthesis_hmm_t *hmm, wtk_source_t* src)
{
	wtk_heap_t *heap=hmm->pool->hash->heap;
	int i,ret;
	int *state=NULL;

	ret=wtk_source_read_int(src,&i,1,1);
	if(ret!=0){goto end;}
	//wtk_debug("i=%d\n",i);
	//ver=i>>24;
	hmm->conca_mcepvsize=i & 0x00ffffff;
	hmm->conca_nmceptree=(int*)wtk_heap_malloc(heap,hmm->conca_nstate*sizeof(int));
	for(i=0;i<hmm->conca_nstate;++i)
	{
		hmm->conca_nmceptree[i]=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_CONCA_MGC,i+2);
	}
	hmm->conca_nmceppdf=(int**)wtk_heap_malloc(heap,sizeof(int*)*hmm->conca_nstate);
	state=(int*)wtk_malloc(sizeof(int)*hmm->conca_nstate);
	ret=wtk_source_read_int(src,state,hmm->conca_nstate,1);
	if(ret!=0){goto end;}
	for(i=0;i<hmm->conca_nstate;++i)
	{
		hmm->conca_nmceppdf[i]=(int*)wtk_heap_malloc(heap,hmm->conca_nmceptree[i]*sizeof(int));
		hmm->conca_nmceppdf[i][0]=state[i];
		//ret=wtk_source_read_int(src,hmm->nmceppdf[i],1,1);
		//if(ret!=0){goto end;}
	}
end:
	if(state)
	{
		wtk_free(state);
	}
	return 0;
}

int wtk_cosynthesis_hmm_load_conca_mcp(wtk_cosynthesis_hmm_t *hmm,wtk_source_t *src)
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
	hmm->conca_mcepvsize=i & 0x00ffffff;
	hmm->conca_nmceptree=(int*)wtk_heap_malloc(heap,hmm->conca_nstate*sizeof(int));
	for(i=0;i<hmm->conca_nstate;++i)
	{
		hmm->conca_nmceptree[i]=wtk_cosynthesis_dtree_count(hmm->dtree,WTK_SYN_DTREE_TREE_CONCA_MGC,i+2);
	}
	hmm->conca_nmceppdf=(int**)wtk_heap_malloc(heap,sizeof(int*)*hmm->conca_nstate);
	state=(int*)wtk_malloc(sizeof(int)*hmm->conca_nstate);
	ret=wtk_source_read_int(src,state,hmm->conca_nstate,1);
	if(ret!=0){goto end;}
	for(i=0;i<hmm->conca_nstate;++i)
	{
		hmm->conca_nmceppdf[i]=(int*)wtk_heap_malloc(heap,hmm->conca_nmceptree[i]*sizeof(int));
		hmm->conca_nmceppdf[i][0]=state[i];
		//ret=wtk_source_read_int(src,hmm->nmceppdf[i],1,1);
		//if(ret!=0){goto end;}
	}
	hmm->conca_mceppdf=(float****)wtk_heap_malloc(heap,hmm->conca_nstate*sizeof(float***));
	for(i=0;i<hmm->conca_nstate;++i)
	{
		hmm->conca_mceppdf[i]=(float***)wtk_heap_malloc(heap,hmm->conca_nmceptree[i]*sizeof(float**));
		for(j=0;j<hmm->conca_nmceptree[i];++j)
		{
			//wtk_debug("v[%d/%d]\n",i,j);
			f=hmm->conca_mceppdf[i][j]=(float**)wtk_heap_malloc(heap,hmm->conca_nmceppdf[i][j]*sizeof(float*));
			n=hmm->conca_nmceppdf[i][j];
			for(k=0;k<n;++k)
			{
				f1=f[k]=(float*)wtk_heap_malloc(heap,2*hmm->conca_mcepvsize*sizeof(float));
				ret=wtk_source_read_float(src,f1,2*hmm->conca_mcepvsize,1);
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
	return ret;
}

float* wtk_cosynthesis_hmm_get_conca_mcp(wtk_cosynthesis_hmm_t *hmm,int state,int idx1,int idx2)
{
	wtk_source_t src;
	FILE *f;
	int i,j;
	int vx;
	float *pf=NULL;
	int vt=2*hmm->conca_mcepvsize;

	if (hmm->rbin)
		f = wtk_rbin2_get_file(hmm->rbin, hmm->cfg->conca_mcp_fn);
	else
	{
		f=fopen(hmm->cfg->conca_mcp_fn,"rb");
		if(!f){
			wtk_debug("open failed %s", hmm->cfg->conca_mcp_fn);
			goto end;
		}
	}

	vx=(hmm->conca_nstate+1);
	for(i=0;i<state;++i)
	{
		for(j=0;j<hmm->conca_nmceptree[i];++j)
		{
			vx+=hmm->conca_nmceppdf[i][j]*vt;
			//wtk_debug("state=%d/%d/%d vx=%d n=%d/%d\n",state,i,j,vx,hmm->nmceppdf[i][j],vt);
		}
	}
	//wtk_debug("state=%d/%d/%d vx=%d\n",state,idx1,idx2,vx);
	for(j=0;j<idx1;++j)
	{
		vx+=hmm->conca_nmceppdf[state][j]*vt;
	}
	vx+=idx2*vt;
	//wtk_debug("state=%d/%d/%d vx=%d\n",state,idx1,idx2,vx);
	wtk_source_init_fd(&src,f, ftell(f) + vx*4);
	pf=(float*)wtk_malloc(sizeof(float)*vt);
	wtk_source_read_float(&src,pf,vt,1);
	wtk_source_clean_fd(&src);
end:
	if (f)
		fclose(f);
	return pf;
}
wtk_cosynthesis_hmm_t* wtk_cosynthesis_hmm_new(wtk_cosynthesis_hmm_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool,wtk_cosynthesis_dtree_t *dtree)
{
	wtk_cosynthesis_hmm_t *h;
	int ret;

	h=(wtk_cosynthesis_hmm_t*)wtk_calloc(1,sizeof(wtk_cosynthesis_hmm_t));
	h->cfg=cfg;
	h->pool=pool;
	h->dtree=dtree;
	h->ndurpdf=NULL;
	h->durpdf=NULL;
	h->nstate=0;
	h->lf0stream=0;
	h->conca_nstate=1;
	h->conca_f0_pos=0;
	h->f0_pos=0;
	h->bap_pos=0;

	h->rbin=(wtk_rbin2_t*)sl->hook;
	if(cfg->load_all)
	{
		ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_cosynthesis_hmm_load_dur,cfg->dur_fn);
		if(ret!=0){goto end;}
		//read log f0 model
		ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_cosynthesis_hmm_load_lf0,cfg->lf0_fn);
		if(ret!=0){goto end;}
		ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_cosynthesis_hmm_load_mcp,cfg->mcp_fn);
		if(ret!=0){goto end;}
		if (cfg->bap_fn)
		{
			ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_cosynthesis_hmm_load_bap,cfg->bap_fn);
			if(ret!=0){goto end;}
		}
		ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_cosynthesis_hmm_load_conca_lf0,cfg->conca_lf0_fn);
		if(ret!=0){goto end;}
		ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_cosynthesis_hmm_load_conca_mcp,cfg->conca_mcp_fn);
		if(ret!=0){goto end;}
	}else
	{
		h->ndurpdf=NULL;
		h->durpdf=NULL;
		if (cfg->dur_fn)
		{
			ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_cosynthesis_hmm_init_dur,cfg->dur_fn);
			if(ret!=0){goto end;}
		}
		if (cfg->mcp_fn)
		{
			ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_cosynthesis_hmm_init_mcp,cfg->mcp_fn);
			if(ret!=0){goto end;}
		}
		if (cfg->bap_fn)
		{
			ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_cosynthesis_hmm_init_bap,cfg->bap_fn);
			if(ret!=0){goto end;}
		}
		if (cfg->conca_lf0_fn)
		{
			ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_cosynthesis_hmm_init_conca_lf0,cfg->conca_lf0_fn);
			if(ret!=0){goto end;}
		}
		if (cfg->conca_mcp_fn)
		{
			ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_cosynthesis_hmm_init_conca_mcp,cfg->conca_mcp_fn);
			if(ret!=0){goto end;}
		}
		if (cfg->lf0_fn)
		{
			ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_cosynthesis_hmm_init_lf0,cfg->lf0_fn);
			if(ret!=0){goto end;}
		}
	}
	// read GV of log F0 model
	if(cfg->lf0_gv_fn)
	{
		ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_cosynthesis_hmm_load_lf0_gv,cfg->lf0_gv_fn);
		if(ret!=0){goto end;}
	}else
	{
		h->lf0gvpdf=NULL;
	}
	if(cfg->mcp_gv_fn)
	{
		ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_cosynthesis_hmm_load_mcp_gv,cfg->mcp_gv_fn);
		if(ret!=0){goto end;}
	}else
	{
		h->mcepgvpdf=NULL;
	}
	if(cfg->bap_gv_fn)
	{
		ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_cosynthesis_hmm_load_bap_gv,cfg->bap_gv_fn);
		if(ret!=0){goto end;}
	}else
	{
		h->bapgvpdf=NULL;
	}
	ret=0;
end:
    if (ret !=0){
    	wtk_debug("Warming: maybe exist error!\n");
    }
	return h;
}

void wtk_cosynthesis_hmm_delete(wtk_cosynthesis_hmm_t *hmm)
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
