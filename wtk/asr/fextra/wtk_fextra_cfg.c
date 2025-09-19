#include "wtk_fextra_cfg.h"
#include <math.h>

#ifdef UNIX
/* Prototype for C Library functions drand48 and srand48 */
double drand48(void);
void srand48(long);
#define RANDF() drand48()
#define SRAND(x) srand48(x)
#else
/* if not unix use ANSI C defaults */
#define RANDF() ((float)rand()/RAND_MAX)
#define SRAND(x) srand(x)
#endif

/* EXPORT->RandInit: Initialise random number generators
           if seed is -ve, then system clock is used */
static void RandInit(int seed)
{
   if (seed<0) seed = (int)time(NULL)%257;
   SRAND(seed);
}

/* EXPORT->RandomValue:  */
float wtk_random_value(void)
{
   return RANDF();
}

int wtk_fextra_cfg_init(wtk_fextra_cfg_t *cfg)
{
	memset(cfg,0,sizeof(*cfg));
	wtk_feat_cfg_init(&(cfg->feat));
	wtk_cmn_cfg_init(&(cfg->cmn));
	wtk_fnn_cfg_init(&(cfg->dnn));
	qtk_nnet3_cfg_init(&(cfg->nnet3));
	cfg->use_dnn=0;
	cfg->use_z=cfg->use_cmn=0;
	wtk_string_set_s(&(cfg->target_kind),"PLP_0_D_A_T");
	cfg->window_size=250000;
	cfg->window_step=100000;
	cfg->src_sample_rate=625.0;

	cfg->DELTAWINDOW=cfg->ACCWINDOW=cfg->THIRDWINDOW=2;
	cfg->NUMCHNAS=20; //24
	cfg->NUMCEPS=12;
	cfg->CEPLIFTER=22;
	cfg->LPCORDER=12;
	cfg->PREMCOEF=0.97f;
	cfg->CEPSCALE=1.0f; //10
	cfg->LOFREQ=-1.0;
	cfg->HIFREQ=-1.0;
	cfg->WARPFREQ=1.0;
	cfg->WARPLCUTOFF=0.0;
	cfg->WARPUCUTOFF=0.0;
	cfg->COMPRESSFACT=0.33f;
	cfg->ZMEANSOURCE=0;
	cfg->RAWENERGY=1;
	cfg->USEHAMMING=1;
    cfg->USEPOVEY=0;
	cfg->USEPOWER=0; //1
	cfg->DOUBLEFFT=0;
	cfg->SILFLOOR=50;
	cfg->ADDDITHER=0.0;
	cfg->SIMPLEDIFFS=0;

	cfg->cache_size=100;
	cfg->feature_basic_cols=12;
	cfg->align=0;

	cfg->feature_cols=0;
	cfg->static_feature_cols=0;
	cfg->base_kind=ANON;
	cfg->use_e=0;
	cfg->use_d=0;
	cfg->use_cmn2=0;
	return 0;
}

int wtk_fextra_cfg_clean(wtk_fextra_cfg_t *cfg)
{
	if(cfg->use_cmn)
	{
		wtk_cmn_cfg_clean(&(cfg->cmn));
	}
	if(cfg->use_dnn)
	{
		wtk_fnn_cfg_clean(&(cfg->dnn));
	}
	if(cfg->use_nnet3)
	{
		qtk_nnet3_cfg_clean(&(cfg->nnet3));
	}
	return 0;
}

int wtk_fextra_cfg_bytes(wtk_fextra_cfg_t *cfg)
{
	int bytes=0;

	if(cfg->use_dnn)
	{
		bytes+=wtk_fnn_cfg_bytes(&(cfg->dnn));
	}
	return bytes;
}

double wtk_win2sigma(int win)
{
	int i;
	double sigma;

	sigma=0;
	for(i=1;i<=win;++i)
	{
		sigma+=i*i;
	}
	sigma*=2;
	return sigma;
}


int wtk_fextra_cfg_update(wtk_fextra_cfg_t *cfg)
{
	/*
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	*/
	return wtk_fextra_cfg_update2(cfg,0);
}

void wtk_parm_cfg_update_inform(wtk_fextra_cfg_t *cfg)
{
	switch(cfg->base_kind)
	{
	case FBANK:
		cfg->feature_basic_cols=cfg->NUMCHNAS;
		break;
	case MFCC:
		cfg->feature_basic_cols=cfg->NUMCEPS;
		break;
	default:
		//cfg->feature_basic_cols=12;
		break;
	}
	cfg->static_feature_cols=cfg->feature_basic_cols+cfg->Zero+cfg->ENERGY;
	cfg->feature_cols=(cfg->static_feature_cols)*(1+cfg->DELTA+cfg->ACCS+cfg->THIRD);
	//wtk_debug("[%d,%d] D=%d A=%d T=%d\n",cfg->static_feature_cols,cfg->feature_cols,cfg->DELTA,cfg->ACCS,cfg->THIRD);
}

void wtk_fextra_cfg_update_dep(wtk_fextra_cfg_t *cfg)
{

	if(cfg->ADDDITHER!=0.0)
	{
		RandInit(12345);
	}
	cfg->frame_size=cfg->window_size/cfg->src_sample_rate;
	cfg->frame_step=cfg->window_step/cfg->src_sample_rate;
	cfg->frame_dur=(cfg->frame_step*cfg->src_sample_rate)/1e7;//10000000.0;
	if(!cfg->ENERGY)
	{
		cfg->ENORMALISE=0;
	}
	if(cfg->ENORMALISE)
	{
		cfg->esilfloor=(cfg->SILFLOOR*log(10.0))/10.0;
	}
	cfg->sigma[0]=wtk_win2sigma(cfg->DELTAWINDOW);
	cfg->sigma[1]=wtk_win2sigma(cfg->ACCWINDOW);
	cfg->sigma[2]=wtk_win2sigma(cfg->THIRDWINDOW);
	wtk_parm_cfg_update_inform(cfg);
	cfg->feat.sig_size=cfg->feature_cols;
	cfg->feat.use_dnn=cfg->use_dnn;
	cfg->feat.dnn_size=cfg->dnn.out_cols;
}

int wtk_fextra_cfg_update2(wtk_fextra_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret=0;
	wtk_source_loader_t file_sl;

	if(cfg->ADDDITHER!=0.0)
	{
		RandInit(12345);
	}
	if(!sl)
	{
		file_sl.hook=0;
		file_sl.vf=wtk_source_load_file_v;
		sl=&(file_sl);
	}
	wtk_fkind_from_string(&cfg->pkind,cfg->target_kind.data,cfg->target_kind.len);
	cfg->frame_size=cfg->window_size/cfg->src_sample_rate;
	cfg->frame_step=cfg->window_step/cfg->src_sample_rate;
	cfg->frame_dur=(cfg->frame_step*cfg->src_sample_rate)/1e7;//10000000.0;
	cfg->base_kind=cfg->pkind&BASEMASK;
	/*
	switch(kind)
	{
	case MFCC:
		cfg->use_mfcc=1;
		break;
	case FBANK:
		cfg->use_fbank=1;
		break;
	case PLP:
		cfg->use_plp=1;
		break;
	}*/
	cfg->Zero=(cfg->pkind & HASZEROC)!=0;
	cfg->DELTA=(cfg->pkind & HASDELTA)!=0;
	cfg->ACCS=(cfg->pkind & HASACCS)!=0;
	cfg->THIRD=(cfg->pkind & HASTHIRD)!=0;
	cfg->ENERGY=(cfg->pkind & HASENERGY) !=0;
	cfg->ZMEAN=(cfg->pkind & HASZEROM)!=0;
	//wtk_parm_cfg_print(cfg);
	if(!cfg->ENERGY)
	{
		cfg->ENORMALISE=0;
	}
	if(cfg->ENORMALISE)
	{
		cfg->esilfloor=(cfg->SILFLOOR*log(10.0))/10.0;
	}
	cfg->sigma[0]=wtk_win2sigma(cfg->DELTAWINDOW);
	cfg->sigma[1]=wtk_win2sigma(cfg->ACCWINDOW);
	cfg->sigma[2]=wtk_win2sigma(cfg->THIRDWINDOW);
	if(cfg->use_cmn)
	{
		ret=wtk_cmn_cfg_update(&(cfg->cmn),sl);
		if(ret!=0){goto end;}
	}
	//wtk_debug("================ use_dnn=%d ========================\n",cfg->use_dnn);
	if(cfg->use_dnn)
	{
		ret=wtk_fnn_cfg_update2(&(cfg->dnn),sl);
		if(ret!=0){goto end;}
	}
	if(cfg->use_nnet3)
	{
            ret = qtk_nnet3_cfg_update2(&(cfg->nnet3), sl);
            if (ret != 0) {
                goto end;}
	}
	wtk_parm_cfg_update_inform(cfg);
	cfg->feat.sig_size=cfg->feature_cols;
	cfg->feat.use_dnn=cfg->use_dnn;
	cfg->feat.dnn_size=cfg->dnn.out_cols;
end:
	return ret;
}

int wtk_fextra_cfg_update_local(wtk_fextra_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc=main;
	wtk_string_t *v;
	int ret=0;

	wtk_local_cfg_update_cfg_b(lc,cfg,use_cmn2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_e,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,feature_basic_cols,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,cache_size,v);
	wtk_local_cfg_update_cfg_f2(lc,cfg,window_size,WINDOWSIZE,v);
	wtk_local_cfg_update_cfg_f2(lc,cfg,window_step,TARGETRATE,v);
	wtk_local_cfg_update_cfg_f2(lc,cfg,src_sample_rate,SOURCERATE,v);
	v=wtk_local_cfg_find_string_s(lc,"TARGETKIND");
	if(v)
	{
		wtk_string_set(&(cfg->target_kind),v->data,v->len);
	}
	wtk_local_cfg_update_cfg_i2(lc,cfg,NUMCHNAS,NUMCHANS,v);
	wtk_local_cfg_update_cfg_i2(lc,cfg,NUMCEPS,NUMCEPS,v);
	wtk_local_cfg_update_cfg_i2(lc,cfg,CEPLIFTER,CEPLIFTER,v);
	wtk_local_cfg_update_cfg_i2(lc,cfg,LPCORDER,LPCORDER,v);
	wtk_local_cfg_update_cfg_i2(lc,cfg,DELTAWINDOW,DELTAWINDOW,v);
	wtk_local_cfg_update_cfg_i2(lc,cfg,ACCWINDOW,ACCWINDOW,v);
	wtk_local_cfg_update_cfg_i2(lc,cfg,THIRDWINDOW,THIRDWINDOW,v);

	wtk_local_cfg_update_cfg_f2(lc,cfg,PREMCOEF,PREEMCOEF,v);
	wtk_local_cfg_update_cfg_f2(lc,cfg,CEPSCALE,CEPSCALE,v);
	wtk_local_cfg_update_cfg_f2(lc,cfg,LOFREQ,LOFREQ,v);
	wtk_local_cfg_update_cfg_f2(lc,cfg,HIFREQ,HIFREQ,v);
	wtk_local_cfg_update_cfg_f2(lc,cfg,WARPFREQ,WARPFREQ,v);
	wtk_local_cfg_update_cfg_f2(lc,cfg,WARPLCUTOFF,WARPLCUTOFF,v);
	wtk_local_cfg_update_cfg_f2(lc,cfg,WARPUCUTOFF,WARPUCUTOFF,v);
	wtk_local_cfg_update_cfg_f2(lc,cfg,COMPRESSFACT,COMPRESSFACT,v);
	wtk_local_cfg_update_cfg_f2(lc,cfg,ADDDITHER,ADDDITHER,v);

	wtk_local_cfg_update_cfg_b2(lc,cfg,ZMEANSOURCE,ZMEANSOURCE,v);
	wtk_local_cfg_update_cfg_b2(lc,cfg,RAWENERGY,RAWENERGY,v);
	wtk_local_cfg_update_cfg_b2(lc,cfg,USEHAMMING,USEHAMMING,v);
    wtk_local_cfg_update_cfg_b2(lc,cfg,USEPOVEY,USEPOVEY,v);
	wtk_local_cfg_update_cfg_b2(lc,cfg,USEPOWER,USEPOWER,v);
	wtk_local_cfg_update_cfg_b2(lc,cfg,DOUBLEFFT,DOUBLEFFT,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,ESCALE,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,SILFLOOR,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,ENORMALISE,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,SIMPLEDIFFS,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_z,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_cmn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dnn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_nnet3,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_d,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,align,v);
	if(cfg->use_cmn)
	{
		lc=wtk_local_cfg_find_lc_s(main,"cmn");
		if(!lc)
		{
			lc=wtk_local_cfg_find_lc_s(main,"zmean");
		}
		if(lc)
		{
			ret=wtk_cmn_cfg_update_local(&(cfg->cmn),lc);
			if(ret!=0){goto end;}
		}
	}
	if(cfg->use_dnn)
	{
		lc=wtk_local_cfg_find_lc_s(main,"fnn");
		if(!lc)
		{
			lc=wtk_local_cfg_find_lc_s(main,"dnn");
		}
		if(lc)
		{
			ret=wtk_fnn_cfg_update_local(&(cfg->dnn),lc);
			if(ret!=0){goto end;}
		}
	}
	if(cfg->use_nnet3)
	{
		lc=wtk_local_cfg_find_lc_s(main,"nnet3");
		if(lc)
		{
			ret=qtk_nnet3_cfg_update_local(&(cfg->nnet3),lc);
			if(ret!=0){goto end;}
		}
	}
end:
	return ret;
}


int wtk_parm_cfg_set_example(wtk_fextra_cfg_t *p)
{
	p->window_step=200000.0;
	p->window_size=300000.0;
	p->PREMCOEF=0.97;
	p->NUMCHNAS=24;
	p->USEPOWER=1;
	p->USEHAMMING=1;
	p->NUMCEPS=12;
	p->CEPSCALE=10.0;
	p->COMPRESSFACT=0.3333333f;
	p->ZMEANSOURCE=1;
	wtk_string_set_s(&(p->target_kind),"PLP_0_D_A_T");
	//p->fbank_num_chans=23;
	return 0;
}

void wtk_fextra_cfg_print(wtk_fextra_cfg_t *cfg)
{
	printf("----------- PARM -------------\n");
	print_cfg_f(cfg,window_size);
	print_cfg_f(cfg,window_step);
	print_cfg_f(cfg,src_sample_rate);
	print_cfg_f(cfg,frame_dur);
	printf("PARM:\t%*.*s\n",cfg->target_kind.len,cfg->target_kind.len,cfg->target_kind.data);
	print_cfg_i(cfg,NUMCHNAS);
	print_cfg_i(cfg,NUMCEPS);
	print_cfg_i(cfg,CEPLIFTER);
	print_cfg_i(cfg,LPCORDER);
	print_cfg_i(cfg,DELTAWINDOW);
	print_cfg_i(cfg,ACCWINDOW);
	print_cfg_i(cfg,THIRDWINDOW);
	print_cfg_f(cfg,PREMCOEF);
	print_cfg_f(cfg,CEPSCALE);
	print_cfg_f(cfg,LOFREQ);
	print_cfg_f(cfg,HIFREQ);
	print_cfg_f(cfg,WARPFREQ);
	print_cfg_f(cfg,WARPLCUTOFF);
	print_cfg_f(cfg,WARPUCUTOFF);
	print_cfg_f(cfg,COMPRESSFACT);
	print_cfg_f(cfg,HIFREQ);
	print_cfg_f(cfg,WARPFREQ);
	print_cfg_i(cfg,ZMEANSOURCE);
	print_cfg_i(cfg,RAWENERGY);
	print_cfg_i(cfg,USEHAMMING);
    print_cfg_i(cfg,USEPOVEY);
	print_cfg_i(cfg,USEPOWER);
	print_cfg_i(cfg,DOUBLEFFT);
	print_cfg_i(cfg,ENERGY);
	print_cfg_i(cfg,DELTA);
	print_cfg_i(cfg,ACCS);
	print_cfg_i(cfg,THIRD);
	print_cfg_i(cfg,ZMEAN);
	print_cfg_i(cfg,Zero);
}

int wtk_fextra_cfg_get_sample_rate(wtk_fextra_cfg_t *cfg)
{
	return (int)(1E7/cfg->src_sample_rate);
}
