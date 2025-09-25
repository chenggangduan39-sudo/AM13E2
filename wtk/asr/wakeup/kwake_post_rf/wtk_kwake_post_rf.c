#include "wtk_kwake_post_rf.h"

wtk_kwake_post_rf_inst_t* wtk_kwake_post_rf_inst_new(wtk_kwake_post_rf_cfg_t* cfg)
{
    wtk_kwake_post_rf_inst_t* rf;
    rf = (wtk_kwake_post_rf_inst_t*)wtk_malloc(sizeof(wtk_kwake_post_rf_inst_t));
    rf->cfg = cfg;
    rf->state=(float*)wtk_malloc(sizeof(float)*cfg->state_cnt*2);
    rf->mat_path=wtk_mati_new(cfg->state_cnt,cfg->win);
    rf->path = (int*)wtk_malloc(sizeof(int)*cfg->win);
    rf->path_prob = (float*)wtk_malloc(sizeof(float)*cfg->win);
    return rf;
}

void wtk_kwake_post_rf_inst_reset(wtk_kwake_post_rf_inst_t *rf)
{
    wtk_mati_zero(rf->mat_path);
    memset(rf->state,0,sizeof(float)*rf->cfg->state_cnt*2);
    memset(rf->path,-1,sizeof(int)*rf->cfg->win);
    memset(rf->path_prob,-1.0,sizeof(float)*rf->cfg->win);
}

void wtk_kwake_post_rf_inst_reset2(wtk_kwake_post_rf_inst_t *rf)
{
    rf->rf_e=-1;
    rf->rf_s=-1;
}

int wtk_kwake_post_rf_inst_delete(wtk_kwake_post_rf_inst_t *rf)
{
    wtk_mati_delete(rf->mat_path);
    wtk_free(rf->state);
    wtk_free(rf->path);
    wtk_free(rf->path_prob);
    wtk_free(rf);
    return 0;
}

void wtk_kwake_post_rf_fill_state(wtk_kwake_post_rf_inst_t* rf,float* pf,float* state)
{
    int i,id;
    for(i = 0;i<rf->cfg->state_cnt;++i)
    {
          id = rf->cfg->state_array[i];
          if(id==0)
          {
              state[i] = log((pf[rf->bkg_idx]+pf[rf->sil_idx])/rf->cfg->bkg_sil_ratio);
          }else
          {
              state[i] = log(pf[id]);
          }
          //printf("%f ",state[i]);
    }
    //printf("\n");
}

void wtk_kwake_post_rf_fill_state_fix(wtk_kwake_post_rf_inst_t* rf,short* pf,float* state,int shift)
{
    int i,id;
    for(i = 0;i<rf->cfg->state_cnt;++i)
    {
          id = rf->cfg->state_array[i];
          if(id==0)
          {
              state[i] = log(FIX2FLOAT_ANY((pf[rf->bkg_idx]+pf[rf->sil_idx]),shift)/rf->cfg->bkg_sil_ratio);
          }else
          {
              state[i] = log(FIX2FLOAT_ANY(pf[id],shift));
          }
          //printf("%f ",state[i]);
    }
    //printf("\n");
}

void wtk_kwake_post_rf_state_switch(float **x,float **y)
{
    float *tmp;
    tmp=*x;
    *x=*y;
    *y=tmp;
}

float wtk_kwake_post_rf_max(float a,float b,float c)
{
	float tmp;
	tmp=a>b?a:b;
	return c>tmp?c:tmp;
}

int wtk_kwake_post_rf_max_idx(float a,float b,float c,int x,int y,int z)
{
	int tmp_idx;
    float tmp;
	tmp=a>b?a:b;
    tmp_idx=a>b?x:y;
	return c>tmp?z:tmp_idx;
}

float wtk_kwake_post_rf_calc(float* dst)
{
    float thresh=0;
    int i=0;
    /*wtk_debug("=======rf feat========\n");
    for(i=0;i<20;++i)
    {
        printf("%f ",dst[i]);
    }
    printf("\n");*/
    float (*forest[])(float d0,float d1,float d2,float d3,float d4,float d5,float d6,float d7,float d8,float d9,float d10,float d11,float d12,float d13,float d14,float d15,float d16,float d17,float d18,float d19)
={
    tree0,tree1,tree2,tree3,tree4,tree5,tree6,tree7,tree8,tree9,tree10,tree11,tree12,tree13,tree14,tree15,tree16,tree17,tree18,tree19,
    tree20,tree21,tree22,tree23,tree24,tree25,tree26,tree27,tree28,tree29,tree30,tree31,tree32,tree33,tree34,tree35,tree36,tree37,tree38,tree39,
};
    int n = sizeof(forest)/sizeof(forest[0]);
    for(i=0;i<n;++i)
    {
        thresh+=(*forest[i])(dst[0],dst[1],dst[2],dst[3],dst[4],dst[5],dst[6],dst[7],dst[8],dst[9],dst[10],dst[11],dst[12],
        dst[13],dst[14],dst[15],dst[16],dst[17],dst[18],dst[19]);
        //  wtk_debug("thresh[%d],%f\n",i,thresh);
    }
    return thresh/n;
}
#ifdef DEBUG_X
static float t=0;
#endif

int wtk_kwake_post_rf_feat_raise(int* pth, float* prob,int*st,int nst,int n,int len,float rf_thresh)
{
    int i,j,k;
    int duration_cache[8] = {0};
    float sum_cache[8] = {0};
    float max_cache[8] = {0};
    //float duration_ratio[4] = {0};
    float sum_ratio[4]={0};
    float average_cache[8] = {0};
    float ghw_prob,average_prob,f=1,f2=1;
    int index[8]={0};
    float dst[20]={0};
    int ret =0;
    float ans;

    for(j=0,k=0;j<nst;++j)
	{
		if(st[j]>0)
		{
			index[k]=j;
			k++;
		}
	}

	for(i=0;i<len;++i)
	{
		for(k=0;k<n;++k)
		{
			
			if(pth[i]==index[k])
			{
				//max = wtk_wakeup_max(max,path_prob[i],0);
				if(max_cache[k]>prob[i])
				{

				}else
				{
					max_cache[k] = prob[i];
					//max_pos_cache[k]=i;
				}
				duration_cache[k]++;
				sum_cache[k]+=prob[i];

			}
		}

	}
	for(k=0;k<n;++k)
	{
		f*=max_cache[k];
		average_cache[k]=sum_cache[k]/duration_cache[k];
		f2*=average_cache[k];
	}
	for(k=0;k<=n/2;++k)
	{
		//duration_ratio[k]=((float)duration_cache[k+1])/((float)duration_cache[k]);
        sum_ratio[k]=((float)sum_cache[k+1])/((float)sum_cache[k]);
	}
	ghw_prob=pow(f,1.0/n);
	average_prob = pow(f2,1.0/n);
    for (i=0; i<4; i++) {
        dst[i] = max_cache[i];
        dst[i+4] = sum_cache[i];
        dst[i+8] = duration_cache[i];
        dst[i+12] = average_cache[i];
    }
    dst[16] = ghw_prob;
    dst[17] = sum_ratio[0];
    dst[18] = sum_ratio[2];
    dst[19] = average_prob;
    if(rf_thresh < (ans=wtk_kwake_post_rf_calc(dst)))
    {
        wtk_debug("thresh=%f/%f\n",rf_thresh,ans);
        ret =1;
    }else
    {
        wtk_debug("thresh=%f/%f\n",rf_thresh,ans);
    }
    
    return ret;
}


void wtk_kwake_post_rf_get_best_pth_fix(wtk_kwake_post_rf_inst_t* rf,wtk_robin_t*rb,int s,int e,int shift)
{
    int * best_pth = rf->path;
    float* best_pth_prob = rf->path_prob;
    wtk_mati_t* pth = rf->mat_path;
    short *pf;
    int*st = rf->cfg->state_array;
    int k,i,j;
    int sil_idx=rf->sil_idx;
    int bkg_idx=rf->bkg_idx;
    float rate = rf->cfg->bkg_sil_ratio;
    int col = rf->cfg->win;

    pf=wtk_robin_at(rb,s);
	best_pth[sil_idx] = 0;
	best_pth_prob[sil_idx]= FIX2FLOAT_ANY((pf[sil_idx]+pf[bkg_idx]),shift)/rate;

	pf=wtk_robin_at(rb,e);
	pf[sil_idx]+=pf[bkg_idx];
	pf[sil_idx]/=rate;
	best_pth[e-s] = rf->cfg->state_cnt-1;
	best_pth_prob[e-s] =FIX2FLOAT_ANY(pf[st[best_pth[e-s]]],shift);
	pf[sil_idx]*=rate;
	pf[sil_idx]-=pf[1];

    
	k=best_pth[e-s];
	for(i=e-1,j=e-s;i>s;--i,--j)
	{
		pf=wtk_robin_at(rb,i);
		pf[sil_idx]+=pf[bkg_idx];
		pf[sil_idx]/=rate;
		best_pth[j-1]= pth->p[j+k*col];
		best_pth_prob[j-1]= FIX2FLOAT_ANY(pf[st[best_pth[j-1]]],shift);
		k=best_pth[j-1];
		pf[sil_idx]*=rate;
		pf[sil_idx]-=pf[bkg_idx];
		//wtk_debug("======v[%d,%d]=%f/%f/%f/%f/%f/%f ======\n",i,j,pf[0],pf[1],pf[2],pf[3],pf[4],pf[5]);
		//wtk_debug("i=%d,j=%d,k=%d beta=%d  prob=%f\n",i,j,k,best_pth[j-1],best_pth_prob[j-1]);
	}
}

void wtk_kwake_post_rf_get_best_pth(wtk_kwake_post_rf_inst_t* rf,wtk_robin_t*rb,int s,int e)
{
    int * best_pth = rf->path;
    float* best_pth_prob = rf->path_prob;
    wtk_mati_t* pth = rf->mat_path;
    float *pf;
    int*st = rf->cfg->state_array;
    int k,i,j;
    int sil_idx=rf->sil_idx;
    int bkg_idx=rf->bkg_idx;
    float rate = rf->cfg->bkg_sil_ratio;
    int col = rf->cfg->win;

    pf=wtk_robin_at(rb,s);
	best_pth[sil_idx] = 0;
	best_pth_prob[sil_idx]= (pf[sil_idx]+pf[bkg_idx])/rate;

	pf=wtk_robin_at(rb,e);
	pf[sil_idx]+=pf[bkg_idx];
	pf[sil_idx]/=rate;
	best_pth[e-s] = rf->cfg->state_cnt-1;
	best_pth_prob[e-s] = pf[st[best_pth[e-s]]];
	pf[sil_idx]*=rate;
	pf[sil_idx]-=pf[1];

    
	k=best_pth[e-s];
	for(i=e-1,j=e-s;i>s;--i,--j)
	{
		pf=wtk_robin_at(rb,i);
		pf[sil_idx]+=pf[bkg_idx];
		pf[sil_idx]/=rate;
		best_pth[j-1]= pth->p[j+k*col];
		best_pth_prob[j-1]= pf[st[best_pth[j-1]]];
		k=best_pth[j-1];
		pf[sil_idx]*=rate;
		pf[sil_idx]-=pf[bkg_idx];
		//wtk_debug("======v[%d,%d]=%f/%f/%f/%f/%f/%f ======\n",i,j,pf[0],pf[1],pf[2],pf[3],pf[4],pf[5]);
		//wtk_debug("i=%d,j=%d,k=%d beta=%d  prob=%f\n",i,j,k,best_pth[j-1],best_pth_prob[j-1]);
	}
}

int wtk_kwake_check_post_rf(wtk_kwake_post_rf_inst_t *rf,wtk_robin_t *rb,int* wake_slot,int n)
{
    int s,e,i,j,k,col,t;
    float *pf;
    float *state,*cur_state,*pre_state;
	wtk_mati_t *pth;
    s=min(rf->cfg->rf_win_left_pad,wake_slot[0]);
    e=min(rf->cfg->rf_win_right_pad,rb->used-wake_slot[n-1]-1);
    s = wake_slot[0] - s;
    e = wake_slot[n-1] + e;
    int ret=0;
    if(s==rf->rf_s && e==rf->rf_e)
    {
        return 0;
    }else
    {
        rf->rf_s=s;
        rf->rf_e=e;
    }
    
    wtk_kwake_post_rf_inst_reset(rf);
    state=rf->state;
    pth=rf->mat_path;
    col=pth->col;
    cur_state=state;
    pre_state=cur_state+rf->cfg->state_cnt;
    
    pf=wtk_robin_at(rb,s);
    wtk_kwake_post_rf_fill_state(rf,pf,cur_state);

    wtk_kwake_post_rf_state_switch(&cur_state,&pre_state);
    for(i=s+1,j=1;i<=e;++i,++j)
    {
        pf=wtk_robin_at(rb,i);
        //wtk_debug("======v[%d,%d]=%f/%f/%f/%f/%f,%f ======\n",s,e,pf[0],pf[1],pf[2],pf[3],pf[4],pf[5]);
        wtk_kwake_post_rf_fill_state(rf,pf,cur_state);
        for(k=0;k<rf->cfg->state_cnt;++k)
        {
            if(k!=0)
            {
                if(rf->cfg->state_array[k]==0||k==1)
                {
                    if(pre_state[k]>=pre_state[k-1])
                    {
                        cur_state[k] += pre_state[k];
                        pth->p[j+k*col]=k;
                    }else
                    {
                        cur_state[k] += pre_state[k-1];
                        pth->p[j+k*col]=k-1;
                    }
                }else
                {
                    cur_state[k] += wtk_kwake_post_rf_max(pre_state[k],pre_state[k-1],pre_state[k-2]);
                    t=wtk_kwake_post_rf_max_idx(pre_state[k],pre_state[k-1],pre_state[k-2],k,k-1,k-2);
                    pth->p[j+k*col]=t;
                }
                
            }else
            {
                cur_state[k]+=pre_state[k];
                pth->p[j+k*col]= k;
            }
            //wtk_debug("xxxx %d,%d,%d\n",j,k,pth->p[j+k*col]);
        }
        wtk_kwake_post_rf_state_switch(&cur_state,&pre_state);
    }

    //wtk_mati_print(pth);
    //exit(0);
    wtk_kwake_post_rf_get_best_pth(rf,rb,s,e);
    ret = wtk_kwake_post_rf_feat_raise(rf->path,rf->path_prob,rf->cfg->state_array,rf->cfg->state_cnt,n,e-s,rf->thresh);
    return ret;
}


int wtk_kwake_check_post_rf_fix(wtk_kwake_post_rf_inst_t *rf,wtk_robin_t *rb,int* wake_slot,int n,int shift)
{
    int s,e,i,j,k,col,t;
    short *pf;
    float *state,*cur_state,*pre_state;
	wtk_mati_t *pth;
    s=min(rf->cfg->rf_win_left_pad,wake_slot[0]);
    e=min(rf->cfg->rf_win_right_pad,rb->used-wake_slot[n-1]-1);
    s = wake_slot[0] - s;
    e = wake_slot[n-1] + e;
    int ret=0;
    if(s==rf->rf_s && e==rf->rf_e)
    {
        return 0;
    }else
    {
        rf->rf_s=s;
        rf->rf_e=e;
    }
    
    wtk_kwake_post_rf_inst_reset(rf);
    state=rf->state;
    pth=rf->mat_path;
    col=pth->col;
    cur_state=state;
    pre_state=cur_state+rf->cfg->state_cnt;
    
    pf=wtk_robin_at(rb,s);
    wtk_kwake_post_rf_fill_state_fix(rf,pf,cur_state,shift);
    //wtk_debug("======v[%d,%d]=%f/%f/%f,%f ======\n",s,e,FIX2FLOAT_ANY(pf[2],shift),FIX2FLOAT_ANY(pf[3],shift),FIX2FLOAT_ANY(pf[4],shift),FIX2FLOAT_ANY(pf[5],shift));
    wtk_kwake_post_rf_state_switch(&cur_state,&pre_state);
    for(i=s+1,j=1;i<=e;++i,++j)
    {
        pf=wtk_robin_at(rb,i);
        //wtk_debug("======v[%d,%d]=%f/%f/%f,%f ======\n",s,e,FIX2FLOAT_ANY(pf[2],shift),FIX2FLOAT_ANY(pf[3],shift),FIX2FLOAT_ANY(pf[4],shift),FIX2FLOAT_ANY(pf[5],shift));
        wtk_kwake_post_rf_fill_state_fix(rf,pf,cur_state,shift);
        for(k=0;k<rf->cfg->state_cnt;++k)
        {
            if(k!=0)
            {
                if(rf->cfg->state_array[k]==0||k==1)
                {
                    if(pre_state[k]>=pre_state[k-1])
                    {
                        cur_state[k] += pre_state[k];
                        pth->p[j+k*col]=k;
                    }else
                    {
                        cur_state[k] += pre_state[k-1];
                        pth->p[j+k*col]=k-1;
                    }
                }else
                {
                    cur_state[k] += wtk_kwake_post_rf_max(pre_state[k],pre_state[k-1],pre_state[k-2]);
                    t=wtk_kwake_post_rf_max_idx(pre_state[k],pre_state[k-1],pre_state[k-2],k,k-1,k-2);
                    pth->p[j+k*col]=t;
                }
                
            }else
            {
                cur_state[k]+=pre_state[k];
                pth->p[j+k*col]= k;
            }
            //wtk_debug("xxxx %d,%d,%d\n",j,k,pth->p[j+k*col]);
        }
        wtk_kwake_post_rf_state_switch(&cur_state,&pre_state);
    }
    wtk_kwake_post_rf_get_best_pth_fix(rf,rb,s,e,shift);
    ret = wtk_kwake_post_rf_feat_raise(rf->path,rf->path_prob,rf->cfg->state_array,rf->cfg->state_cnt,n,e-s,rf->thresh);
    //exit(0);
    return ret;
}