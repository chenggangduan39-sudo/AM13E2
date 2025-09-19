#include "wtk_cosynthesis_backend.h"

wtk_cosynthesis_backend_t* wtk_cosynthesis_backend_new(wtk_cosynthesis_backend_cfg_t *cfg)
{
	wtk_cosynthesis_backend_t *s;

	s=(wtk_cosynthesis_backend_t*)wtk_malloc(sizeof(wtk_cosynthesis_backend_t));
	s->cfg=cfg;
	s->heap=wtk_heap_new(4096);
    s->glb_heap=wtk_heap_new(4096);
    s->lc=wtk_cosynthesis_lc_new(s->glb_heap,s->cfg->hmm);
    return s;
}

void wtk_cosynthesis_backend_delete(wtk_cosynthesis_backend_t *s)
{
	wtk_heap_delete(s->heap);
	wtk_heap_delete(s->glb_heap);
    wtk_free(s);
}

void wtk_cosynthesis_backend_reset(wtk_cosynthesis_backend_t *s)
{
    wtk_cosynthesis_lc_reset(s->lc);
    wtk_heap_reset(s->heap);
}

int wtk_cosynthesis_backend_process(wtk_cosynthesis_backend_t *s,char *flab,int len,wtk_queue_t *q)
{
    int idx[2];
    int ret;
    wtk_cosynthesis_hmm_lc_t *lc;
    //wtk_cosynthesis_lc_t* l = s->lc;

    lc=wtk_cosynthesis_hmm_lc_new(s->cfg->hmm,s->heap,flab,len);
    ret=wtk_cosynthesis_dtree_search(s->cfg->tree,WTK_SYN_DTREE_TREE_DUR,2,flab,len,idx);
    if(ret!=0){
    	wtk_debug("ret=%d\n", ret);
    	goto end;
    }
    wtk_cosynthesis_hmm_lc_find_durpdf(lc,idx);
    wtk_cosynthesis_hmm_lc_set_output_pdfs(lc,flab,len);
    wtk_queue_push(q,&(lc->q_n));
    ret=0;
end:
    return ret;
}

void wtk_cosynthesis_backend_get_candi_unit_dur(wtk_cosynthesis_backend_t *s,float *dur_mean,float *dur_var,int *idx,int nphone)
{
    int i,j;
    int index[2];
    float *pf;
    
    wtk_cosynthesis_hmm_t *hmm=s->cfg->hmm;

    for(i=0;i<nphone;++i)
    {
        index[0] = 0;
        index[1] = idx[i];
        if(hmm->cfg->load_all)
	    {
		    pf=hmm->durpdf[index[0]][index[1]-1];
	    }else
	    {
		    pf=wtk_cosynthesis_hmm_get_durpdf(hmm,index[0],index[1]-1);
	    }
        for(j=0;j<hmm->nstate;++j)
        {
            dur_mean[i*hmm->nstate+j] = pf[j];
            dur_var[i*hmm->nstate+j] = pf[j+hmm->nstate];
        }
        if(!hmm->cfg->load_all){
            wtk_free(pf);
        }
    }
};

void wtk_cosynthesis_backend_get_candi_unit_lf0(wtk_cosynthesis_backend_t *s,float *lf0_mean,float *lf0_var,float *lf0_weight,int state,int *idx,int nphone)
{
    int i,j;
    int index[2];
    float **pf;
    wtk_cosynthesis_hmm_t *hmm=s->cfg->hmm;

    for(i=0;i<nphone;++i)
    {
        index[0] = 0;
        index[1] = idx[i];
        if(hmm->cfg->load_all)
	    {
		    pf=hmm->lf0pdf[state][index[0]][index[1]-1];
            //wtk_debug("%p %d %d %d \n",pf,state,index[0],index[1]);
	    }else
	    {
		    pf=wtk_cosynthesis_hmm_get_lf0pdf(hmm,state,index[0],index[1]-1);
	    }
        for(j=0;j<hmm->lf0stream;++j)
        {          
        	lf0_mean[j+i*hmm->lf0stream]=pf[j][0];
            lf0_var[j+i*hmm->lf0stream]=pf[j][1];
            lf0_weight[j+i*hmm->lf0stream]=pf[j][2];
        }
        if(!hmm->cfg->load_all){
            wtk_free(pf[0]);
            wtk_free(pf);
        }
    }
};

void wtk_cosynthesis_backend_get_candi_unit_mcep(wtk_cosynthesis_backend_t *s,float *mcep_mean,float *mcep_var,int state,int *idx,int nphone)
{
    int i,j;
    int index[2];
    float *f1,*f2;
    wtk_cosynthesis_hmm_t *hmm=s->cfg->hmm;

    for(i=0;i<nphone;++i)
    {
        index[0] = 0;
        index[1] = idx[i];
        if(hmm->cfg->load_all)
	    {
		    f1=hmm->mceppdf[state][index[0]][index[1]-1];
            f2=hmm->mceppdf[state][index[0]][index[1]-1]+hmm->mcepvsize;
	    }else
	    {
		    f1=wtk_cosynthesis_hmm_get_mcp(hmm,state,index[0],index[1]-1);
            f2=f1+hmm->mcepvsize;
	    }
        for(j=0;j<hmm->mcepvsize;++j)
        {
        	mcep_mean[j+i*hmm->mcepvsize] = f1[j];
        	mcep_var[j+i*hmm->mcepvsize] = f2[j];
        }
        if(!hmm->cfg->load_all){
            wtk_free(f1);
        }  
    }
};

void wtk_cosynthesis_mdl_data_new(float **dur_mean,float **dur_var,float**lf0_mean,
float **lf0_var,float **lf0_weight,float **mcep_mean,float **mcep_var,
float **conca_lf0_mean,float **conca_lf0_var,float **conca_lf0_weight,float **conca_mcep_mean,
float **conca_mcep_var,int nphone,wtk_cosynthesis_feat_cfg_t *cfg)
{
    *dur_mean = (float *)wtk_malloc(sizeof(float*)*nphone*cfg->hmm_dur_len); //nphone x nstate
    *dur_var = (float *)wtk_malloc(sizeof(float*)*nphone*cfg->hmm_dur_len);
    *lf0_mean = (float *)wtk_malloc(sizeof(float*)*nphone*cfg->hmm_lf0_len*3);
    *lf0_var = (float *)wtk_malloc(sizeof(float*)*nphone*cfg->hmm_lf0_len*3);
    *lf0_weight = (float *)wtk_malloc(sizeof(float*)*nphone*cfg->hmm_lf0_len*3);
    *mcep_mean = (float *)wtk_malloc(sizeof(float*)*nphone*cfg->hmm_mcep_len*3);
    *mcep_var = (float *)wtk_malloc(sizeof(float*)*nphone*cfg->hmm_mcep_len*3);

    *conca_mcep_mean = (float *)wtk_malloc(sizeof(float*)*nphone*cfg->hmm_mcep_len);
    *conca_mcep_var = (float *)wtk_malloc(sizeof(float*)*nphone*cfg->hmm_mcep_len);
    *conca_lf0_mean = (float *)wtk_malloc(sizeof(float*)*nphone);
    *conca_lf0_var = (float *)wtk_malloc(sizeof(float*)*nphone);
    *conca_lf0_weight = (float *)wtk_malloc(sizeof(float*)*nphone);
}

void wtk_cosynthesis_mdl_data_new2(float **dur_mean,float **dur_var,float**lf0_mean,
float **lf0_var,float **lf0_weight,float **mcep_mean,float **mcep_var,int nphone,wtk_cosynthesis_feat_cfg_t *cfg)
{
    *dur_mean = (float*)wtk_malloc(sizeof(float)*nphone*cfg->hmm_dur_len); //nphone x nstate
    *dur_var = (float*)wtk_malloc(sizeof(float)*nphone*cfg->hmm_dur_len);
    *lf0_mean = (float*)wtk_malloc(sizeof(float)*nphone*cfg->hmm_lf0_len);
    *lf0_var = (float*)wtk_malloc(sizeof(float)*nphone*cfg->hmm_lf0_len);
    *lf0_weight = (float*)wtk_malloc(sizeof(float)*nphone*cfg->hmm_lf0_len);
    *mcep_mean = (float*)wtk_malloc(sizeof(float)*nphone*cfg->hmm_mcep_len);
    *mcep_var = (float*)wtk_malloc(sizeof(float)*nphone*cfg->hmm_mcep_len);
}

void wtk_cosynthesis_mdl_data_delete(float *dur_mean,float *dur_var,float *lf0_mean,
float *lf0_var,float *lf0_weight,float *mcep_mean,float *mcep_var,
float *conca_mcep_mean,float *conca_mcep_var,float *conca_lf0_mean,float *conca_lf0_var,float *conca_lf0_weight)
{
    if(dur_mean)wtk_free(dur_mean);
    if(dur_var)wtk_free(dur_var);
    if(lf0_mean)wtk_free(lf0_mean);
    if(lf0_var)wtk_free(lf0_var);
    if(lf0_weight)wtk_free(lf0_weight);
    if(mcep_mean)wtk_free(mcep_mean);
    if(mcep_var)wtk_free(mcep_var);
    if(conca_mcep_mean)wtk_free(conca_mcep_mean);
    if(conca_mcep_var)wtk_free(conca_mcep_var);
    if(conca_lf0_mean)wtk_free(conca_lf0_mean);
    if(conca_lf0_var)wtk_free(conca_lf0_var);
    if(conca_lf0_weight)wtk_free(conca_lf0_weight);
}
