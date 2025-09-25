#include "wtk_pcen.h"

wtk_pcen_t* wtk_pcen_new(wtk_pcen_cfg_t *cfg,int vec_size)
{
	wtk_pcen_t* pcen = (wtk_pcen_t*)wtk_malloc(sizeof(wtk_pcen_t));

	pcen->cfg = cfg;
	pcen->last_smooth_vec = (float*)wtk_calloc(vec_size,sizeof(float));
	pcen->smooth_vec = (float*)wtk_calloc(vec_size,sizeof(float));
	pcen->size = vec_size;
	return pcen;
}

int wtk_pcen_bytes(wtk_pcen_t *pcen)
{
	return 0;
}

void wtk_pcen_delete(wtk_pcen_t *pcen)
{
	wtk_free(pcen->last_smooth_vec);
	wtk_free(pcen->smooth_vec);
	wtk_free(pcen);
}

void wtk_pcen_reset(wtk_pcen_t *pcen)
{
	memset(pcen->last_smooth_vec,0,pcen->size);
	memset(pcen->smooth_vec,0,pcen->size);
}
//void wtk_pcen_set_notify(wtk_pcen_t *pcen,void *ths,wtk_kfeat_notify_f notify)
void wtk_pcen_apply_pow(float *f, int n,float p)
{
	int i;
	if(p == 2)
	{
		for(i = 0; i < n; i++)
		{
			f[i] = f[i] * f[i];
		}
	}else if(p == 0.5)
	{
		for(i = 0; i < n; i++)
		{
			if(f[i] < 0)
			{
				wtk_debug("sqrt error\n");
				exit(0);
			}
			f[i] = sqrt(f[i]);
		}
	}else
	{
		for(i = 0; i < n; i++)
		{
			f[i] = pow(f[i],p);
		}
	}
}

void wtk_pcen_filter(float *feat)
{
	int i;
	float b = 0.024689453, a = 0.97531055;

	feat[0] = b * feat[0] + a;

	for(i = 1;i < 40;i++)
	{
		feat[i] = b * feat[i] + a * feat[i-1];
	}
}

void wtk_pcen_feed_k(wtk_pcen_t *pcen,wtk_kfeat_t *feat)
{
	wtk_pcen_cfg_t *cfg = pcen->cfg;
	int i,size = pcen->size;
	float *f = feat->v;
	float bias = pow(cfg->delta,cfg->gamma) * -1.0;
	//print_float(feat->v,40);
	if(feat->index == 0)
	{
		for(i=0; i<size; i++)
		{
			pcen->smooth_vec[i] = f[i];
		}
	}else
	{
		for(i=0; i<size; i++)
		{
			pcen->smooth_vec[i] =+ (1.0 - cfg->smooth_factor)*
					pcen->last_smooth_vec[i] + cfg->smooth_factor*f[i];
		}
	}
	memcpy(pcen->last_smooth_vec,pcen->smooth_vec,sizeof(float)*size);
	for(i=0; i<size; i++)
	{
		if(pcen->smooth_vec[i] == 0.0)
		{
			pcen->smooth_vec[i] = cfg->epsilon;
		}
	}
	wtk_pcen_apply_pow(pcen->smooth_vec,size,cfg->alpha);

	//print_float(pcen->smooth_vec,40);

	for(i=0; i<size; i++)
	{
		f[i] /= pcen->smooth_vec[i];
		f[i] += cfg->delta;
	}
	wtk_pcen_apply_pow(f,size,cfg->gamma);
	for(i=0; i<size; i++)
	{
		f[i] += bias;
	}
	//print_float(feat->v,40);
}

void wtk_pcen_feed_torch(wtk_pcen_t *pcen,wtk_kfeat_t *feat)
{
	wtk_pcen_cfg_t *cfg = pcen->cfg;
	int i,size = pcen->size;
	float *f = feat->v;
	float bias = pow(cfg->delta,cfg->gamma);
	//print_float(feat->v,40);

	memcpy(pcen->smooth_vec,f,sizeof(float)*size);
	wtk_pcen_filter(pcen->smooth_vec);
	//print_float(pcen->smooth_vec,40);
	for(i=0; i<size; i++)
	{
		pcen->smooth_vec[i] = exp(-cfg->alpha*(log(cfg->epsilon) +
				log(pcen->smooth_vec[i]/cfg->epsilon + 1.0)));
	}

	for(i=0; i<size; i++)
	{
		f[i] = bias * (exp(cfg->gamma * log(f[i]*pcen->smooth_vec[i]/cfg->delta + 1)) - 1);
	}

	//print_float(feat->v,40);
}

void wtk_pcen_feed(wtk_pcen_t *pcen,wtk_kfeat_t *feat)
{
	if(pcen->cfg->use_torch)
	{
		wtk_pcen_feed_torch(pcen,feat);
	}else
	{
		wtk_pcen_feed_k(pcen,feat);
	}
}
