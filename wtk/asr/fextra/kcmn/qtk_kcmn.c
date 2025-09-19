#include "qtk_kcmn.h"
qtk_kcmn_t* qtk_kcmn_new(qtk_kcmn_cfg_t *cfg,int len)
{
	qtk_kcmn_t *cmn = (qtk_kcmn_t*)wtk_malloc(sizeof(qtk_kcmn_t));

	cmn->cfg=cfg;
	cmn->feat_dim = len;
	cmn->sumq = (float*)wtk_malloc(sizeof(float)*len);
	memset(cmn->sumq,0,sizeof(float)*len);
	cmn->stat = (float*)wtk_malloc(sizeof(float)*len);
	memset(cmn->stat,0,sizeof(float)*len);
	cmn->frame_cnt=0;
	cmn->cur_cnt=1;
	cmn->cmvn_cnt=cfg->global_frames+1;
	cmn->output = qtk_array_new(2,sizeof(float*));
	cmn->input = qtk_array_new(2,sizeof(float*));
	cmn->notify = NULL;

	return cmn;
}

void qtk_kcmn_set_notify(qtk_kcmn_t *cmn,void *ths,wtk_kfeat_notify_f notify)
{
	cmn->notify_ths=ths;
	cmn->notify=notify;
}

int qtk_kcmn_delete(qtk_kcmn_t *z)
{
	int i;
	float **f;
	for(i = 0; i < z->output->dim; i++)
	{
		f = qtk_array_get(z->output,i);
		wtk_free(*f);
	}
	//wtk_debug("%d\n",z->output->dim);
	qtk_array_delete(z->output);
	for(i = 0; i < z->input->dim; i++)
	{
		f = qtk_array_get(z->input,i);
		wtk_free(*f);
	}
	qtk_array_delete(z->input);
	wtk_free(z->stat);
	wtk_free(z->sumq);
	wtk_free(z);
	return 0;
}

int qtk_kcmn_reset(qtk_kcmn_t *cmn)
{
	memset(cmn->sumq,0,sizeof(float)*cmn->feat_dim);
	//cmn->stat = (float*)wtk_malloc(sizeof(float)*cmn->feat_dim);
	memset(cmn->stat,0,sizeof(float)*cmn->feat_dim);
	cmn->frame_cnt=0;
	cmn->cur_cnt=1;
	cmn->cmvn_cnt=cmn->cfg->global_frames+1;
	int i;
	float **f;
	for(i = 0; i < cmn->output->dim; i++)
	{
		f = qtk_array_get(cmn->output,i);
		wtk_free(*f);
	}
	qtk_array_clear(cmn->output);
	//wtk_debug("%d\n",z->output->dim);
	for(i = 0; i < cmn->input->dim; i++)
	{
		f = qtk_array_get(cmn->input,i);
		wtk_free(*f);
	}
	qtk_array_clear(cmn->input);

	return 0;
}
float qcmn_mean[65]=
{
6.085152e+11,-6.454417e+10,-4.560683e+10,-3.30819e+10,-1.195759e+11,-6.397689e+10,-1.000984e+11,-7.50982e+10,-7.010379e+10,-5.77506e+10,-6.972925e+10,-5.695429e+10,-5.698859e+10,-3.50651e+10,-5.664455e+10,-3.138663e+10,-3.028677e+10,-1.116223e+10,-1.39004e+10,-2.734936e+09,-3.335335e+09,3.933626e+08,-3.016418e+08,-2.356137e+08,1.059169e+09,-1.780925e+09,2.728193e+09,-1.061719e+09,5.805146e+09,2.663252e+08,6.311377e+09,2.155743e+09,7.588337e+09,2.979435e+09,6.606949e+09,2.056474e+09,4.916728e+09,1.0357e+09,3.033308e+09,-9.311711e+08,1.972906e+08,-1.063333e+09,1.993636e+08,-1.15859e+08,-1.758714e+08,-3.764263e+08,-2.47697e+09,-2.819984e+09,-4.12829e+09,-2.337815e+09,-3.12609e+08,4.505967e+09,4.507559e+09,5.807944e+09,2.821593e+09,2.63187e+08,-5.548534e+09,-6.024784e+09,-5.385605e+09,-2.65911e+09,7.224685e+08,3.624179e+09,2.595674e+09,1.052196e+09,4.982244e+09};
//for eng
//float qcmn_mean[65]=
//{4.846992e+09,5.157721e+09,5.474725e+09,5.801409e+09,6.091235e+09,6.282761e+09,6.348126e+09,6.402893e+09,6.38937e+09,6.386799e+09,6.532634e+09,6.669785e+09,6.589946e+09,6.718246e+09,6.560408e+09,6.564489e+09,6.484457e+09,6.457989e+09,6.446106e+09,6.430221e+09,6.423899e+09,6.429353e+09,6.437938e+09,6.44725e+09,6.460385e+09,6.522168e+09,6.547906e+09,6.672194e+09,6.702413e+09,6.78209e+09,6.832576e+09,6.863158e+09,6.879886e+09,6.890918e+09,6.90621e+09,6.927816e+09,6.969808e+09,7.021421e+09,7.078541e+09,7.11264e+09,7.12896e+09,7.155811e+09,7.167748e+09,7.173435e+09,7.172557e+09,7.182278e+09,7.18365e+09,7.193253e+09,7.196361e+09,7.175907e+09,7.143704e+09,7.110376e+09,7.065025e+09,7.019787e+09,6.969188e+09,6.937969e+09,6.911992e+09,6.90393e+09,6.904125e+09,6.891879e+09,6.868311e+09,6.823352e+09,6.717655e+09,6.538258e+09,4.568974e+08};
//{1.520701e+11,-1.611778e+10,-1.139335e+10,-8.269864e+09,-2.989185e+10,-1.598689e+10,-2.500736e+10,-1.87544e+10,-1.751264e+10,-1.442803e+10,-1.741355e+10,-1.423384e+10,-1.423708e+10,-8.760045e+09,-1.415631e+10,-7.844227e+09,-7.568457e+09,-2.786166e+09,-3.476125e+09,-6.867146e+08,-8.35383e+08,9.7159e+07,-7.582689e+07,-5.870472e+07,2.652925e+08,-4.45297e+08,6.81393e+08,-2.671075e+08,1.449777e+09,6.549581e+07,1.576723e+09,5.400022e+08,1.897529e+09,7.463625e+08,1.652059e+09,5.142513e+08,1.230146e+09,2.597525e+08,7.589069e+08,-2.330647e+08,4.902634e+07,-2.658677e+08,5.010448e+07,-2.89385e+07,-4.39447e+07,-9.409714e+07,-6.18759e+08,-7.045747e+08,-1.031721e+09,-5.848623e+08,-7.807273e+07,1.124908e+09,1.126905e+09,1.451129e+09,7.057492e+08,6.608106e+07,-1.38637e+09,-1.505696e+09,-1.346045e+09,-6.641608e+08,1.803748e+08,9.050205e+08,6.487997e+08,2.626164e+08,1.245013e+09};

//for sourcer
int qtk_kcmn_last_frame(qtk_kcmn_t *cmn,int n)
{
	if(n < 500)
	{
		return 0;
	}
	return 1;
}

int qtk_kcmn_dim(qtk_kcmn_t *cmn)
{
	return cmn->feat_dim;
}

int qtk_kcmn_frame_ready(qtk_kcmn_t *cmn)
{
	return cmn->frame_cnt;
}

void qtk_kcmn_get_raw_frame(qtk_kcmn_t *cmn,int frame, float* feats, int len)
{
	//wtk_debug("no cmn get frame:%d\n",frame);
	if(frame < 0)
		frame = 0;
	//memcpy(feats,qcmn_fbank+len*frame,sizeof(float)*len);
	float **out = qtk_array_get(cmn->input,frame);
	memcpy(feats,*out,sizeof(float)*len);
}

void qtk_kcmn_get_out_frame(qtk_kcmn_t *cmn,int frame, float* feats, int len)
{
	//wtk_debug("cmn get frame:%d\n",frame);
	if(frame < 0)
		frame = 0;
	float **out = qtk_array_get(cmn->output,frame);
	memcpy(feats,*out,sizeof(float)*len);
}

void qtk_kcmn_process(qtk_kcmn_t *cmn,float *stat,float *f)
{
	double count = cmn->cmvn_cnt;
	int i;
	int dim = cmn->feat_dim;
	float *c = (float*)wtk_malloc(sizeof(float) * dim);
	memset(c,0,sizeof(float)*dim);
	for(i = 0; i < dim; i++)
	{
//		wtk_debug("%f\n",*(stat+i));
		*(c+i) = *(f+i) - (1.0/count)*(*(stat+i));
	}
	qtk_array_push(cmn->output,&c);

	float *d = (float*)wtk_malloc(sizeof(float) * dim);
	memcpy(d,f,sizeof(float)*dim);
	qtk_array_push(cmn->input,&d);

	memcpy(f,c,sizeof(float)*dim);
}

void qtk_kcmn_feed(qtk_kcmn_t *cmn,wtk_kfeat_t *feat)
{
//	memcpy(feat->v,qcmn_fbank+40*feat->index,sizeof(float)*40);
//	wtk_debug("cmn feed %d\n",feat->index);
	int i;
	int dim = cmn->feat_dim;
	if(cmn->frame_cnt <cmn->cfg->cmn_window)
	{
		for(i=0;i<dim;i++)
		{
			*(cmn->sumq+i)+=*(feat->v+i);
		}
	}else
	{
		float **out = qtk_array_get(cmn->input,cmn->frame_cnt-cmn->cfg->cmn_window);
		//wtk_debug("%d\n",cmn->frame_cnt-cmn->cfg->cmn_window);
		for(i=0;i<dim;i++)
		{
			*(cmn->sumq+i)+=*(feat->v+i);
			//*(cmn->sumq+i)-=qcmn_fbank[dim*(cmn->frame_cnt-cmn->cfg->cmn_window)+i];
			*(cmn->sumq+i)-=*((*out)+i);
			
		}
	}
	if(cmn->cur_cnt  < cmn->cfg->cmn_window)
	{
		double count_from_global=cmn->cfg->cmn_window-cmn->cur_cnt;
		if(count_from_global > cmn->cfg->global_frames)
		{
			count_from_global=cmn->cfg->global_frames;
		}
		for(i=0;i<dim;i++)
		{
			*(cmn->stat+i)=(*(cmn->sumq+i))+(count_from_global/qcmn_mean[dim])*qcmn_mean[i];
		}
	}else
	{
		memcpy(cmn->stat,cmn->sumq,dim*sizeof(float));
	}
	qtk_kcmn_process(cmn,cmn->stat,feat->v);
	if(cmn->notify)
	{
			cmn->notify(cmn->notify_ths,feat);
	}

	if(cmn->frame_cnt < cmn->cfg->cmn_window)
	{
		cmn->cur_cnt++;
	}
	if(cmn->cmvn_cnt < cmn->cfg->cmn_window)
	{
		cmn->cmvn_cnt++;
	}

	cmn->frame_cnt++;
	//wtk_debug("%d\n",cmn->frame_cnt);
}
