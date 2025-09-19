#include "wtk_bankfeat.h" 

void wtk_bankfeat_compute_band_energy(wtk_bankfeat_t *bankfeat, float *bandE, wtk_complex_t *fft) 
{
	int nb_bands=bankfeat->cfg->nb_bands;
	int *eband=bankfeat->cfg->eband;
    int i;
    int j;
    int band_size;
    float tmp;
    float frac;

    memset(bandE, 0, sizeof(float)*nb_bands);
    for (i=0;i<nb_bands-1;++i)
    {
        band_size = eband[i+1]-eband[i];
        for (j=0;j<band_size;j++) 
        {
            frac = (float)j/band_size;
            tmp = fft[eband[i] + j].a* fft[eband[i] + j].a+ fft[eband[i] + j].b* fft[eband[i] + j].b;
            bandE[i] += (1-frac)*tmp;
            bandE[i+1] += frac*tmp;
        }
    }
    bandE[0] *= 2;
    bandE[nb_bands-1] *= 2;
}

void wtk_bankfeat_compute_band_energy2(wtk_bankfeat_t *bankfeat, float *bandE, wtk_complex_t *fft, float *mask)
{
	int nb_bands=bankfeat->cfg->nb_bands;
	int *eband=bankfeat->cfg->eband;
    int i;
    int j;
    int band_size;
    float tmp;
    float frac;

    memset(bandE, 0, sizeof(float)*nb_bands);
    for (i=0;i<nb_bands-1;++i)
    {
        band_size = eband[i+1]-eband[i];
        for (j=0;j<band_size;j++) 
        {
            frac = (float)j/band_size;
            tmp = (fft[eband[i] + j].a* fft[eband[i] + j].a+ fft[eband[i] + j].b* fft[eband[i] + j].b)*mask[eband[i] + j]*mask[eband[i] + j];
            bandE[i] += (1-frac)*tmp;
            bandE[i+1] += frac*tmp;
        }
    }
    bandE[0] *= 2;
    bandE[nb_bands-1] *= 2;
}

void wtk_bankfeat_interp_band_gain(wtk_bankfeat_t *bankfeat, int nbin, float *g, const float *bandE)
{
	int nb_bands=bankfeat->cfg->nb_bands;
	int *eband=bankfeat->cfg->eband;
	int i,j;
	int band_size;
	float frac;

	memset(g, 0, nbin*sizeof(float));
	for (i=0;i<nb_bands-1;++i)
	{
		band_size = eband[i+1]-eband[i];
		for (j=0;j<band_size;j++)
		{
			frac = (float)j/band_size;
			g[eband[i] + j] = (1-frac)*bandE[i] + frac*bandE[i+1];
		}
	}
}

void wtk_bankfeat_dct_table_init(float *dct_table, int nb_bands)
{
	int i,j;

	for (i=0;i<nb_bands;++i) 
	{
		for (j=0;j<nb_bands;++j)
		{
			dct_table[i*nb_bands + j] = cosf((i+.5)*j*PI/nb_bands);
			if (j==0)
			{
				dct_table[i*nb_bands + j] *= sqrtf(.5);
			}
		}
	}
}

void wtk_bankfeat_dct(float *dct_table, int nb_bands, float *out, const float *in)
{
	int i,j;
	float sum;

	for (i=0;i<nb_bands;++i)
	{
		sum = 0;
		for (j=0;j<nb_bands;++j)
		{
			sum += in[j] * dct_table[j*nb_bands + i];
		}
		out[i] = sum*sqrtf(2.f/nb_bands);
	}
}


wtk_bankfeat_t *wtk_bankfeat_new(wtk_bankfeat_cfg_t *cfg)
{
	wtk_bankfeat_t *bankfeat;

	bankfeat=(wtk_bankfeat_t *)wtk_malloc(sizeof(wtk_bankfeat_t));
	bankfeat->cfg=cfg;

	bankfeat->Ex=wtk_malloc(sizeof(float)*cfg->nb_bands);
	bankfeat->dct_table=wtk_malloc(sizeof(float)*cfg->nb_bands*cfg->nb_bands);

	bankfeat->cepstral_mem=NULL;
	if(cfg->use_ceps)
	{
		bankfeat->cepstral_mem=wtk_float_new_p2(cfg->ceps_mem, cfg->nb_bands);
	}
	bankfeat->features=wtk_malloc(sizeof(float)*cfg->nb_features);

	wtk_bankfeat_reset(bankfeat);

	return bankfeat;
}


void wtk_bankfeat_delete(wtk_bankfeat_t *bankfeat)
{
	wtk_free(bankfeat->Ex);
	wtk_free(bankfeat->dct_table);
	if(bankfeat->cepstral_mem)
	{
		wtk_float_delete_p2(bankfeat->cepstral_mem, bankfeat->cfg->ceps_mem);
	}
	wtk_free(bankfeat->features);
	wtk_free(bankfeat);
}

void wtk_bankfeat_reset(wtk_bankfeat_t *bankfeat)
{
	memset(bankfeat->Ex, 0, sizeof(float)*(bankfeat->cfg->nb_bands));
	wtk_bankfeat_dct_table_init(bankfeat->dct_table, bankfeat->cfg->nb_bands);
	bankfeat->memid=0;

	if(bankfeat->cepstral_mem)
	{
		wtk_float_zero_p2(bankfeat->cepstral_mem, bankfeat->cfg->ceps_mem, bankfeat->cfg->nb_bands);
	}
	memset(bankfeat->features, 0, sizeof(float)*bankfeat->cfg->nb_features);

	bankfeat->silence=1;
}

void wtk_bankfeat_flush_frame_features(wtk_bankfeat_t *bankfeat, wtk_complex_t *fftx)
{
	float *Ex=bankfeat->Ex;
	float **cepstral_mem=bankfeat->cepstral_mem;
	float *features=bankfeat->features;
	float *dct_table=bankfeat->dct_table;
	int nb_bands=bankfeat->cfg->nb_bands;
	int nb_delta_ceps=bankfeat->cfg->nb_delta_ceps;
	int ceps_mem=bankfeat->cfg->ceps_mem;
	int i,j,k;
	float E = 0;
	float *ceps_0, *ceps_1, *ceps_2;
	float spec_variability = 0;
	float follow, logMax;
	float mindist,dist,tmp;
	float Ly[256];

	wtk_bankfeat_compute_band_energy(bankfeat, Ex, fftx);
	logMax = -2;
	follow = -2;
	for (i=0;i<nb_bands;++i)
	{
		E += Ex[i];
		Ly[i] = log10f(1e-2f+Ex[i]);
		Ly[i] = max(logMax-7, max(follow-1.5, Ly[i]));
		logMax = max(logMax, Ly[i]);
		follow = max(follow-1.5, Ly[i]);
	}
	wtk_bankfeat_dct(dct_table, nb_bands, features, Ly);
	
	if(cepstral_mem)
	{
		features[0] -= 12;
		features[1] -= 4;
		ceps_0 = cepstral_mem[bankfeat->memid];
		ceps_1 = (bankfeat->memid < 1) ? cepstral_mem[ceps_mem+bankfeat->memid-1] : cepstral_mem[bankfeat->memid-1];
		ceps_2 = (bankfeat->memid < 2) ? cepstral_mem[ceps_mem+bankfeat->memid-2] : cepstral_mem[bankfeat->memid-2];
		for (i=0;i<nb_bands;i++)
		{
			ceps_0[i] = features[i];
		}
		bankfeat->memid++;
		for (i=0;i<nb_delta_ceps;i++)
		{
			features[i] = ceps_0[i] + ceps_1[i] + ceps_2[i];
			features[nb_bands+i] = ceps_0[i] - ceps_2[i];
			features[nb_bands+nb_delta_ceps+i] =  ceps_0[i] - 2*ceps_1[i] + ceps_2[i];
		}
		if (bankfeat->memid == ceps_mem)
		{
			bankfeat->memid = 0;
		}
		for (i=0;i<ceps_mem;++i)
		{
			mindist = 1e15f;
			for (j=0;j<ceps_mem;++j)
			{
					dist=0;
				for (k=0;k<nb_bands;++k)
				{
					tmp = cepstral_mem[i][k] - cepstral_mem[j][k];
					dist += tmp*tmp;
				}
				if (j!=i)
				{
					mindist = min(mindist, dist);
				}
			}
			spec_variability += mindist;
		}
		features[nb_bands+2*nb_delta_ceps] = spec_variability/ceps_mem-2.1;
	}

	bankfeat->silence = E < 0.1;
}
