#include "wtk_kcmn.h"
static float qcmn_mean[31]=
{
//	13541690.132999996, -8361471.737259992, -5370212.997731985, -4502953.9993079975, -19455585.765200038, -14453722.118350005, -16925307.118760016, -15643810.408000017, -10089221.05649996, -13675811.628940025, -8143362.915636997, -12635023.282, -7852771.813917507, -7812235.044049007, -7731261.24986461, -4515094.734000005, -4415128.1840200005, -1577625.9418796967, -1910749.4696971003, -381067.2308421004, -364287.11528774945, -131363.81687090005, -7914.834172849992, 34685.91656026708, 64204.436596749016, 182053.36145183985, 94056.8589071998, 378871.1871163902, 65364.27437167979, 193691.06770971988, 801760.0
	1.453621e+10,-8.983481e+09,-4.503292e+09,-6.158312e+09,-1.372354e+10,-1.025e+10,-1.24164e+10,-9.829967e+09,-7.658004e+09,-7.546095e+09,-6.57094e+09,-7.392692e+09,-5.470145e+09,-4.841511e+09,-5.465258e+09,-3.152541e+09,-2.651424e+09,-9.849719e+08,-1.216752e+09,-2.316141e+08,-2.709723e+08,-4.878457e+07,-2.766296e+07,1.087503e+07,5.25944e+07,4.799809e+07,1.018905e+08,9.053118e+07,1.840187e+08,1.60559e+08,7.943019e+08
};


wtk_kcmn_t* wtk_kcmn_new(wtk_kcmn_cfg_t *cfg,int len)
{
	wtk_kcmn_t *cmn = (wtk_kcmn_t*)wtk_malloc(sizeof(wtk_kcmn_t));
	int i;

	cmn->cfg=cfg;
	cmn->feat_dim = len;
	cmn->sumq = (float*)wtk_malloc(sizeof(float)*len);
	memset(cmn->sumq,0,sizeof(float)*len);
	cmn->stat = (float*)wtk_malloc(sizeof(float)*len);
	memset(cmn->stat,0,sizeof(float)*len);
	cmn->frame_cnt=0;
	cmn->cur_cnt=0;
	cmn->cmvn_cnt=cfg->global_frames+1;
	cmn->output = wtk_larray_new(50,sizeof(float*));
	cmn->input = wtk_larray_new(50,sizeof(float*));
	cmn->notify = NULL;
	cmn->global_qcmn_mean=qcmn_mean;
	if(cmn->cfg->cmn_def)
	{
		for(i=0;i<=wtk_vector_size(cmn->cfg->cmn_def);++i)
		{
			cmn->cfg->cmn_def[i+1]=cmn->cfg->cmn_def[i+1]*0.3f+qcmn_mean[i]*0.7f;
		}
		cmn->global_qcmn_mean=cmn->cfg->cmn_def+1;
	}

	return cmn;
}

void wtk_kcmn_set_notify(wtk_kcmn_t *cmn,void *ths,wtk_kfeat_notify_f notify)
{
	cmn->notify_ths=ths;
	cmn->notify=notify;
}

int wtk_kcmn_delete(wtk_kcmn_t *z)
{
	int i;
	float **f;

	for(i = 0; i < z->output->nslot; i++)
	{
		f = wtk_larray_get(z->output,i);
		wtk_free(*f);
	}
	//wtk_debug("%d\n",z->output->dim);
	wtk_larray_delete(z->output);
	for(i = 0; i < z->input->nslot; i++)
	{
		f = wtk_larray_get(z->input,i);
		wtk_free(*f);
	}
	wtk_larray_delete(z->input);
	wtk_free(z->stat);
	wtk_free(z->sumq);
	wtk_free(z);

	return 0;
}

int wtk_kcmn_reset(wtk_kcmn_t *cmn)
{
	memset(cmn->sumq,0,sizeof(float)*cmn->feat_dim);
	//cmn->stat = (float*)wtk_malloc(sizeof(float)*cmn->feat_dim);
	memset(cmn->stat,0,sizeof(float)*cmn->feat_dim);
	cmn->frame_cnt=0;
	cmn->cur_cnt=0;
	cmn->cmvn_cnt=cmn->cfg->global_frames+1;
	int i;
	float **f;
	for(i = 0; i < cmn->output->nslot; i++)
	{
		f = wtk_larray_get(cmn->output,i);
		wtk_free(*f);
	}
	wtk_larray_reset(cmn->output);
	//wtk_debug("%d\n",z->output->dim);
	for(i = 0; i < cmn->input->nslot; i++)
	{
		f = wtk_larray_get(cmn->input,i);
		wtk_free(*f);
	}
	wtk_larray_reset(cmn->input);

	return 0;
}


//for sourcer
int wtk_kcmn_last_frame(wtk_kcmn_t *cmn,int n)
{
	if(n < 500)
	{
		return 0;
	}
	return 1;
}

int wtk_kcmn_dim(wtk_kcmn_t *cmn)
{
	return cmn->feat_dim;
}

int wtk_kcmn_frame_ready(wtk_kcmn_t *cmn)
{
	return cmn->frame_cnt;
}

void wtk_kcmn_get_raw_frame(wtk_kcmn_t *cmn,int frame, float* feats, int len)
{
	//wtk_debug("no cmn get frame:%d\n",frame);
	if(frame < 0)
	{
		frame = 0;
	}
	//memcpy(feats,qcmn_fbank+len*frame,sizeof(float)*len);
	float **out = wtk_larray_get(cmn->input,frame);
	memcpy(feats,*out,sizeof(float)*len);
}

void wtk_kcmn_get_out_frame(wtk_kcmn_t *cmn,int frame, float* feats, int len)
{
	//wtk_debug("cmn get frame:%d\n",frame);
	if(frame < 0)
	{
		frame = 0;
	}
	float **out = wtk_larray_get(cmn->output,frame);
	memcpy(feats,*out,sizeof(float)*len);
}

void wtk_kcmn_process(wtk_kcmn_t *cmn,float *stat,float *f)
{
	double count = cmn->cmvn_cnt;
	//wtk_debug("process cmvn: count %f\n",count);
	int i;
	int dim = cmn->feat_dim;
	float *c = (float*)wtk_malloc(sizeof(float) * dim);
	memset(c,0,sizeof(float)*dim);
	for(i = 0; i < dim; i++)
	{
		//wtk_debug("%f\n",*(stat+i));
		*(c+i) = *(f+i) - (1.0/count)*(*(stat+i));
	}
	wtk_larray_push2(cmn->output,&c);

	float *d = (float*)wtk_malloc(sizeof(float) * dim);
	memcpy(d,f,sizeof(float)*dim);
	wtk_larray_push2(cmn->input,&d);

	memcpy(f,c,sizeof(float)*dim);
}

void wtk_kcmn_feed(wtk_kcmn_t *cmn,wtk_kfeat_t *feat)
{
//	memcpy(feat->v,qcmn_fbank+40*feat->index,sizeof(float)*40);
//	wtk_debug("cmn feed %d\n",feat->index);
	float *global_qcmn_mean=cmn->global_qcmn_mean;
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
		float **out = wtk_larray_get(cmn->input,cmn->frame_cnt-cmn->cfg->cmn_window);
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
			*(cmn->stat+i)=(*(cmn->sumq+i))+(count_from_global/global_qcmn_mean[dim])*global_qcmn_mean[i];
			//wtk_debug("%f %f %f %f\n",*(cmn->stat+i),*(cmn->sumq+i),(count_from_global/global_qcmn_mean[dim]),global_qcmn_mean[i]);
			//wtk_debug("%f %f %d\n",count_from_global,global_qcmn_mean[dim],dim);
		}
	}

	wtk_kcmn_process(cmn,cmn->stat,feat->v);
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
