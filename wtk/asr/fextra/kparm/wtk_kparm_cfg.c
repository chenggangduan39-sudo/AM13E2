#include "wtk_kparm_cfg.h" 

void wtk_kfkind_init(wtk_kfkind_t *kind)
{
	kind->bkind=WTK_FBANK;
	kind->has_energy=0;
//	kind->has_d=0;
//	kind->has_a=0;
//	kind->has_t=0;
	kind->has_z=0;
	kind->has_0=0;
}

int wtk_kfkind_update(wtk_kfkind_t *kind,char *str)
{
	int ret;
	char *s;

	if(!str)
	{
		wtk_debug("str is null\n");
		ret=-1;goto end;
	}
	s=str;
	while(1)
	{
		if(*str=='_' || *str==0)
		{
			//wtk_debug("%.*s\n",(int)(str-s),s);
			if(wtk_str_equal_s(s,str-s,"MFCC"))
			{
				kind->bkind=WTK_MFCC;
			}else if(wtk_str_equal_s(s,str-s,"PLP"))
			{
				kind->bkind=WTK_PLP;
			}else if(wtk_str_equal_s(s,str-s,"FBANK"))
			{
				kind->bkind=WTK_FBANK;
			}else if(wtk_str_equal_s(s,str-s,"E"))
			{
				kind->has_energy=1;
			}else if(wtk_str_equal_s(s,str-s,"Z"))
			{
				kind->has_z=1;
			}else if(wtk_str_equal_s(s,str-s,"0"))
			{
				kind->has_0=1;
			}else
			{
				wtk_debug("[%.*s] not found\n",(int)(str-s),s);
				ret=-1;
				goto end;
			}
			//exit(0);
			if(*str==0)
			{
				break;
			}
			s=str+1;
		}
		++str;
	}
	ret=0;
end:
	//exit(0);
	return ret;
}


void wtk_melbank_cfg_init(wtk_melbank_cfg_t *cfg)
{
    // defaults the #mel-banks to 23 for the FBANK computations.
    // this seems to be common for 16khz-sampled data,
    // but for 8khz-sampled data, 15 may be better.
	cfg->num_bins=23;
	cfg->low_freq=20;
	cfg->high_freq=0;
	cfg->vtln_low=100;
	cfg->vtln_high=-500;
	cfg->debug_mel=0;
	cfg->htk_mode=0;
	cfg->use_htk=0;
	cfg->use_torch=0;
	cfg->use_normal=0;
}


void wtk_melbank_cfg_clean(wtk_melbank_cfg_t *cfg)
{
}



void wtk_melbank_cfg_update_local(wtk_melbank_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_i(lc,cfg,num_bins,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,low_freq,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,high_freq,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,vtln_low,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,vtln_high,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,debug_mel,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,htk_mode,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_htk,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_torch,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_normal,v);
}


void wtk_melbank_cfg_update(wtk_melbank_cfg_t *cfg)
{
}



int wtk_kparm_cfg_init(wtk_kparm_cfg_t *cfg)
{
	cfg->kind=NULL;
	wtk_kfkind_init(&(cfg->kfind));
	wtk_melbank_cfg_init(&(cfg->melbank));
	wtk_cmvn_cfg_init(&(cfg->cmvn));
	wtk_pcen_cfg_init(&(cfg->pcen));
	qtk_kcmn_cfg_init(&(cfg->kcmn));
	qtk_ivector_cfg_init(&(cfg->ivector));
	wtk_kcmn_cfg_init(&(cfg->kcmvn));
	wtk_delta_cfg_init(&(cfg->delta));
	wtk_lda_cfg_init(&(cfg->lda));
	cfg->rate=16000;
	cfg->frame_size_ms=25;
	cfg->frame_step_ms=10;
	cfg->frame_size=256;
	cfg->frame_step=128;
	cfg->window=NULL;
	cfg->window_type="povey";
	cfg->blackman_coeff=0.42;
	cfg->preemph_coeff=0.97;
	cfg->dither=0.0;
	cfg->use_pad=0;
	cfg->remove_dc=1;
	cfg->use_power=1;
	cfg->use_log_fbank=1;
	cfg->NUMCEPS=13;
	cfg->CEPLIFTER=22;
	cfg->cache=5;
	cfg->use_lda=0;
	cfg->use_fixpoint=0;
	cfg->fix_preemph_coeff=0;

	cfg->use_kcmvn=0;
	cfg->use_kcmn=0;
	cfg->use_ivector=0;
	cfg->use_pcen=0;
	cfg->use_kind_notify=0;
	cfg->use_snip_edges=1;
	cfg->energy_floor=0;
	cfg->log_energy_floor=log(cfg->energy_floor);
	
	cfg->cmvn_stats_fn=NULL;
	cfg->cmvn_stats=NULL;
	cfg->fix_win=0;
	cfg->use_trick=0;
	cfg->idle_trigger_frame = 2000;

	cfg->use_k2_offline = 0;
	return 0;
}


int wtk_kparm_cfg_bytes(wtk_kparm_cfg_t *cfg)
{
	int bytes;

	bytes=sizeof(wtk_kparm_cfg_t);
	if(cfg->window)
	{
		bytes+=cfg->frame_size*sizeof(float);
	}
	if(cfg->fix_win)
	{
		bytes+=cfg->frame_size*sizeof(wtk_fix_t);
	}
	return bytes;
}


int wtk_kparm_cfg_clean(wtk_kparm_cfg_t *cfg)
{
	wtk_kcmn_cfg_clean(&(cfg->kcmvn));
	wtk_cmvn_cfg_clean(&(cfg->cmvn));
	wtk_pcen_cfg_clean(&(cfg->pcen));
	qtk_kcmn_cfg_clean(&(cfg->kcmn));
	qtk_ivector_cfg_clean(&(cfg->ivector));
	if(cfg->cmvn_stats)
	{
		wtk_vector_delete(cfg->cmvn_stats);
	}
	wtk_delta_cfg_clean(&(cfg->delta));
	wtk_lda_cfg_clean(&(cfg->lda));
	wtk_melbank_cfg_clean(&(cfg->melbank));
	if(cfg->window)
	{
	  wtk_free(cfg->window);
	}
	if(cfg->fix_win)
	{
		wtk_free(cfg->fix_win);
	}
	return 0;
}

int wtk_kparm_cfg_update_local(wtk_kparm_cfg_t *cfg,wtk_local_cfg_t *main)
{
  wtk_string_t *v;
  wtk_local_cfg_t *lc;
  int ret;

  lc=main;
  wtk_local_cfg_update_cfg_str(lc,cfg,kind,v);
  wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
  wtk_local_cfg_update_cfg_i(lc,cfg,cache,v);
  wtk_local_cfg_update_cfg_f(lc,cfg,frame_size_ms,v);
  wtk_local_cfg_update_cfg_f(lc,cfg,frame_step_ms,v);
  wtk_local_cfg_update_cfg_str(lc,cfg,window_type,v);
  wtk_local_cfg_update_cfg_f(lc,cfg,dither,v);
  wtk_local_cfg_update_cfg_b(lc,cfg,use_pad,v);
  wtk_local_cfg_update_cfg_b(lc,cfg,remove_dc,v);
  wtk_local_cfg_update_cfg_b(lc,cfg,use_power,v);
  wtk_local_cfg_update_cfg_b(lc,cfg,use_log_fbank,v);

  wtk_local_cfg_update_cfg_i(lc,cfg,NUMCEPS,v);
  wtk_local_cfg_update_cfg_f(lc,cfg,CEPLIFTER,v);

  wtk_local_cfg_update_cfg_b(lc,cfg,use_delta,v);
  wtk_local_cfg_update_cfg_b(lc,cfg,use_cmvn,v);
  wtk_local_cfg_update_cfg_b(lc,cfg,use_pcen,v);
  wtk_local_cfg_update_cfg_b(lc,cfg,use_kcmvn,v);
  wtk_local_cfg_update_cfg_b(lc,cfg,use_kcmn,v);
  wtk_local_cfg_update_cfg_b(lc,cfg,use_ivector,v);
  wtk_local_cfg_update_cfg_b(lc,cfg,use_lda,v);
  wtk_local_cfg_update_cfg_b(lc,cfg,use_fixpoint,v);
  wtk_local_cfg_update_cfg_b(lc,cfg,use_snip_edges,v);
  wtk_local_cfg_update_cfg_b(lc,cfg,use_kind_notify,v);
  wtk_local_cfg_update_cfg_b(lc,cfg,use_trick,v);
  wtk_local_cfg_update_cfg_i(lc,cfg,idle_trigger_frame,v);
  wtk_local_cfg_update_cfg_b(lc,cfg,use_k2_offline,v);

  lc=wtk_local_cfg_find_lc_s(main,"melbank");
  if(lc)
  {
	  wtk_melbank_cfg_update_local(&(cfg->melbank),lc);
  }
  if(cfg->use_cmvn)
  {
	  lc=wtk_local_cfg_find_lc_s(main,"cmvn");
	  if(lc)
	  {
		  wtk_cmvn_cfg_update_local(&(cfg->cmvn),lc);
		 // wtk_debug("online=%d\n",cfg->cmvn.use_online);
	  }
  }
  if(cfg->use_pcen)
  {
	  lc=wtk_local_cfg_find_lc_s(main,"pcen");
	  if(lc)
	  {
		  wtk_pcen_cfg_update_local(&(cfg->pcen),lc);
		 // wtk_debug("online=%d\n",cfg->cmvn.use_online);
	  }
  }

  if(cfg->use_kcmvn)
  {
	lc=wtk_local_cfg_find_lc_s(main,"kcmvn");
	if(lc)
	{
		wtk_kcmn_cfg_update_local(&(cfg->kcmvn),lc);
	}
  }
  wtk_local_cfg_update_cfg_str(lc,cfg,cmvn_stats_fn,v);
  if(cfg->use_kcmn)
  {
	  lc=wtk_local_cfg_find_lc_s(main,"kcmn");
	  if(lc)
	  {
		  qtk_kcmn_cfg_update_local(&(cfg->kcmn),lc);
	  }
  }
  if(cfg->use_ivector)
  {
	  lc=wtk_local_cfg_find_lc_s(main,"ivector");
	  if(lc)
	  {
		  qtk_ivector_cfg_update_local(&(cfg->ivector),lc);
	  }
  }
  if(cfg->use_delta)
  {
	  lc=wtk_local_cfg_find_lc_s(main,"delta");
	  if(lc)
	  {
		  wtk_delta_cfg_update_local(&(cfg->delta),lc);
	  }
  }
  if(cfg->use_lda)
  {
	  lc=wtk_local_cfg_find_lc_s(main,"lda");
	  if(lc)
	  {
		  wtk_lda_cfg_update_local(&(cfg->lda),lc);
	  }
  }
  ret=0;
//end:
  return ret;
}

int wtk_kparm_cfg_init_fixwin(wtk_kparm_cfg_t *cfg)
{
#define XM_PI		3.14159265358979323846	/* pi */
	int fs=cfg->frame_size;
	int i,n;
	double f,a;

	n=fs;
	cfg->fix_win=(int*)wtk_calloc(n,sizeof(wtk_fix_t));
	a= M_2PI/(fs-1);
	for(i=0;i<n;++i)
	{
		//f=(0.54-0.46*cos(2*XM_PI*i/(fs-1)));
		f=pow(0.5 - 0.5*cos(a * i), 0.85);
		cfg->fix_win[i]=FLOAT2COS(f);
		//wtk_debug("v[%d]=%f/%d\n",i,f,cfg->fix_win[i]);
	}
	//exit(0);
	return 0;
}

int wtk_kparm_cfg_init_win(wtk_kparm_cfg_t *cfg)
{
  double a;
  int fs=cfg->frame_size;
  int i;
  float *win;

  a= M_2PI/(fs-1);
  cfg->window=(float*)wtk_calloc(cfg->frame_size,sizeof(float));
  win=cfg->window;
  if(strcmp(cfg->window_type,"hanning")==0)
  {
      for(i=0;i<fs;++i)
      {
    	  win[i]=0.5*(1-cos(a*i));
      }
  }else if(strcmp(cfg->window_type,"hamming")==0)
  {
	  for(i=0;i<fs;++i)
	  {
		  win[i]=0.54-0.46*cos(a*i);
	  }
  }else if(strcmp(cfg->window_type,"povey")==0)
  {
	  for(i=0;i<fs;++i)
	  {
		  win[i]=pow(0.5 - 0.5*cos(a * i), 0.85);
		 // wtk_debug("v[%d]=%f\n",i,win[i]);
	  }
	 // exit(0);
  }else if(strcmp(cfg->window_type,"rectangular")==0)
  {
	  for(i=0;i<fs;++i)
	  {
		  win[i]=1.0;
	  }
  }else if(strcmp(cfg->window_type,"blackman")==0)
  {
	  for(i=0;i<fs;++i)
	  {
		  win[i]=cfg->blackman_coeff-0.5*cos(a*i)+(0.5-cfg->blackman_coeff)*cos(2*a*i);
	  }
  }else
  {
	  wtk_debug("unkown window type\n");
	  return -1;
  }
  return 0;
}


int wtk_kparm_cfg_update(wtk_kparm_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_kparm_cfg_update2(cfg,&sl);
}




int wtk_kparm_cfg_update2(wtk_kparm_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;
	wtk_source_loader_t sl2;

	ret=wtk_kfkind_update(&(cfg->kfind),cfg->kind);
	if(ret!=0)
	{
		wtk_debug("update kind failed\n");
		goto end;
	}
	cfg->frame_size=cfg->rate*cfg->frame_size_ms/1000;
	cfg->frame_step=cfg->rate*cfg->frame_step_ms/1000;
	//wtk_debug("size=%d/%d\n",cfg->frame_size,cfg->frame_step);
	//exit(0);
	if(cfg->use_fixpoint)
	{
		cfg->fix_preemph_coeff=FLOAT2FIX(cfg->preemph_coeff);
		ret=wtk_kparm_cfg_init_fixwin(cfg);
	}else
	{
		ret=wtk_kparm_cfg_init_win(cfg);
	}
	if(ret!=0)
	{
		wtk_debug("int win failed\n");
		goto end;
	}
	wtk_melbank_cfg_update(&(cfg->melbank));
	if(cfg->use_cmvn)
	{
		ret=wtk_cmvn_cfg_update(&(cfg->cmvn));
		if(ret!=0){goto end;}
	}
	if(cfg->use_pcen)
	{
		ret=wtk_pcen_cfg_update(&(cfg->pcen));
		if(ret!=0){goto end;}
	}
	if(cfg->use_kcmvn)
	{
		ret=wtk_kcmn_cfg_update(&(cfg->kcmvn));
		if(ret!=0){goto end;}
	}

	if(cfg->cmvn_stats_fn && wtk_file_exist(cfg->cmvn_stats_fn)==0)
	{
		sl2.hook=0;
		sl2.vf=wtk_source_load_file_v;
		ret=wtk_source_loader_load(&sl2,cfg,(wtk_source_load_handler_t)wtk_kparm_cfg_load_cmvn_stats,cfg->cmvn_stats_fn);
	}

	if(cfg->use_kcmn)
	{
		ret=qtk_kcmn_cfg_update(&(cfg->kcmn),sl);
		if(ret!=0){goto end;}
	}
	if(cfg->use_ivector)
	{
		ret=qtk_ivector_cfg_update(&(cfg->ivector),sl);
		if(ret!=0){goto end;}
	}
	if(cfg->use_delta)
	{
		ret=wtk_delta_cfg_update(&(cfg->delta));
		if(ret!=0){goto end;}
	}
	//wtk_debug("use_hlda=%d use_lda=%d\n",cfg->use_hlda,cfg->use_lda);
	if(cfg->use_lda)
	{
		ret=wtk_lda_cfg_update2(&(cfg->lda),sl);
		if(ret!=0){goto end;}
	}
	//print_float(cfg->window,cfg->frame_size);
	cfg->vec_size=wtk_kparm_cfg_feature_base_size(cfg);
	cfg->vec_size2=wtk_kparm_cfg_feature_size(cfg);
	ret=0;
end:
	return ret;
}

int wtk_kparm_cfg_load_cmvn_stats(wtk_kparm_cfg_t *cfg,wtk_source_t *s)
{
	wtk_strbuf_t *buf;
	int ret,n;

	s->swap=0;
	buf=wtk_strbuf_new(32,1);
	ret=wtk_source_read_string(s,buf);
	if(ret!=0){goto end;}
	if(!wtk_str_equal_s(buf->data,buf->pos,"<MEAN>"))
	{
		wtk_debug("mean string error %.*s\n",buf->pos,buf->data);
		ret=-1;goto end;
	}
    n=0;
	wtk_source_skip_sp(s,&n);
	ret=wtk_source_read_int(s,&n,1,1);
	if(ret!=0)
	{
		wtk_debug("read int data error\n");
		goto end;
	}

	cfg->cmvn_stats=wtk_vector_new(n+1);
	ret=wtk_source_read_vector(s,cfg->cmvn_stats,1);
	if(ret!=0)
	{
		wtk_debug("read cmvn vector error\n");
		goto end;
	}
//	int i;
//	for(i=1;i<=n+1;++i)
//	{
//		printf("%f\n",cfg->cmn_def[i]);
//	}
//	exit(0);

	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

int wtk_kparm_cfg_feature_base_size(wtk_kparm_cfg_t *cfg)
{
	switch(cfg->kfind.bkind)
	{
	case WTK_FBANK:
		return cfg->melbank.num_bins;
	case WTK_MFCC:
		//return cfg->NUMCEPS+(cfg->kfind.has_energy?1:0);//TODO
		return  cfg->NUMCEPS; //kaldi has_energy:feat[0]=log(energy) ??
	case WTK_PLP:
#ifndef USE_RTOS_OF_5215
		exit(0);
#endif
		break;
	}
	return -1;
}

int wtk_kparm_cfg_feature_size(wtk_kparm_cfg_t *cfg)
{
	int v;

	v=wtk_kparm_cfg_feature_base_size(cfg);
	//wtk_debug("size=%d\n",v);
	//wtk_debug("v=%d %d/%d\n",v*(cfg->delta.order+1),cfg->hlda->row,cfg->hlda->col);
	if(cfg->use_delta)
	{
		return v*(cfg->delta.order+1);
	}else if(cfg->use_lda)
	{
		return cfg->lda.lda->row;
	}else
	{
		return v;
	}
}

int wtk_kparm_cfg_feature_use_size(wtk_kparm_cfg_t *cfg)
{
	int v;

	v=wtk_kparm_cfg_feature_base_size(cfg);
	//wtk_debug("v=%d %d/%d\n",v*(cfg->delta.order+1),cfg->hlda->row,cfg->hlda->col);
	if(cfg->use_delta)
	{
		return v*(cfg->delta.order+1);
	}else if(cfg->use_lda)
	{
		return cfg->lda.lda->row;
	}else
	{
		return v;
	}
}
