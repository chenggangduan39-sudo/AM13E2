#include "wtk_bankfeat_cfg.h" 

#define bankfeat_cfg_tobank(n)   (13.1f*atan(.00074f*(n))+2.24f*atan((n)*(n)*1.85e-8f)+1e-4f*(n))

void wtk_bankfeat_cfg_xfilterbank_init(int *eband, int bands,int rate,int len)
{
	float df;
	float max_mel, mel_interval;
	int i;
	int id;
	float curr_freq;
	float mel;

	df =rate*1.0/(2*(len-1));
	max_mel = bankfeat_cfg_tobank(rate/2);
	mel_interval =max_mel/(bands-1);
	for(i=0; i<bands; ++i)
   {
	   eband[i]=-1;
   }
   for (i=0;i<len;++i)
   {
		curr_freq = i*df;
		// printf("%f\n",curr_freq);
		mel = bankfeat_cfg_tobank(curr_freq);
		if (mel > max_mel)
		{
			break;
		}
		id = (int)(floor(mel/mel_interval));
		// printf("%d %d %d %f %f\t", i, id, eband[id], mel, mel_interval);
		// printf("%f\n",curr_freq);
		if(eband[id]==-1)
		{
			eband[id]=i;
		}
		//  printf("%d  %d %f  %f  %f\n",id,eband[id],curr_freq, mel,mel_interval);
   }
   eband[bands-1]=len-1;
//    for(i=0; i<bands; ++i)
//    {
// 	   printf("%d ",eband[i]);
//    }
//    printf("\n");
}

int wtk_bankfeat_cfg_init(wtk_bankfeat_cfg_t *cfg)
{
	cfg->rate=16000;

	cfg->wins=1024;
 
    cfg->ceps_mem=8;
    cfg->nb_delta_ceps=9;
    cfg->nb_bands=64;  

	cfg->eband=NULL;

	cfg->nb_features=0;

	cfg->use_ceps=1;

	return 0;
}

int wtk_bankfeat_cfg_clean(wtk_bankfeat_cfg_t *cfg)
{
	wtk_free(cfg->eband);

	return 0;
}

int wtk_bankfeat_cfg_update_local(wtk_bankfeat_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,ceps_mem,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nb_delta_ceps,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nb_bands,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_ceps,v);	

	return 0;
}

int wtk_bankfeat_cfg_update(wtk_bankfeat_cfg_t *cfg)
{
	if(cfg->use_ceps)
	{
		cfg->nb_features=cfg->nb_bands+2*cfg->nb_delta_ceps+1;
	}else
	{
		cfg->nb_features=cfg->nb_bands;
	}
	cfg->eband=wtk_malloc(sizeof(int)*cfg->nb_bands);
	wtk_bankfeat_cfg_xfilterbank_init(cfg->eband, cfg->nb_bands, cfg->rate, cfg->wins/2+1);

	return 0;
}
