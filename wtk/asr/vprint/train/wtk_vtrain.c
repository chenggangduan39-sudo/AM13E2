#include "wtk_vtrain.h" 
void wtk_vtrain_acc_reset(wtk_vtrain_acc_t *a);
void wtk_vtrain_acc2_reset(wtk_vtrain_acc2_t *a);

wtk_vtrain_acc_t* wtk_vtrain_acc_new(int nmix,int vsize)
{
	wtk_vtrain_acc_t *a;
	int i;

	a=(wtk_vtrain_acc_t*)wtk_malloc(sizeof(wtk_vtrain_acc_t));
	a->nmix=nmix;
	a->vsize=vsize;
	a->mixprob=(wtk_vtrain_float_t*)wtk_calloc(nmix,sizeof(wtk_vtrain_float_t));
	a->mix=(wtk_vtrain_mix_t*)wtk_calloc(nmix,sizeof(wtk_vtrain_mix_t));
	for(i=0;i<nmix;++i)
	{
		a->mix[i].mp=(wtk_vtrain_float_t*)wtk_calloc(vsize,sizeof(wtk_vtrain_float_t));
	}
	wtk_vtrain_acc_reset(a);
	return a;
}

void wtk_vtrain_acc_delete(wtk_vtrain_acc_t *a)
{
	int i;

	for(i=0;i<a->nmix;++i)
	{
		wtk_free(a->mix[i].mp);
	}
	wtk_free(a->mix);
	wtk_free(a->mixprob);
	wtk_free(a);
}

void wtk_vtrain_acc_reset(wtk_vtrain_acc_t *a)
{
	int i;
	wtk_vtrain_mix_t *mix;

	memset(a->mixprob,0,sizeof(float)*a->nmix);
	for(i=0;i<a->nmix;++i)
	{
		//a->mixprob[i]=0;
		mix=a->mix+i;
		mix->occ=0;
		memset(mix->mp,0,sizeof(float)*a->vsize);
//		for(j=0;j<a->vsize;++j)
//		{
//			mix->mp[j]=0;
//		}
	}
}


void wtk_vtrain_acc_print(wtk_vtrain_acc_t *a)
{
	int i;

	for(i=0;i<a->nmix;++i)
	{
		wtk_debug("v[%d]=%f\n",i,a->mix[i].occ);
	}
}

wtk_vtrain_acc2_t* wtk_vtrain_acc2_new(int nmix,int vsize)
{
	wtk_vtrain_acc2_t *a;
	int i;

	a=(wtk_vtrain_acc2_t*)wtk_malloc(sizeof(wtk_vtrain_acc2_t));
	a->nmix=nmix;
	a->vsize=vsize;
	a->mix=(wtk_vtrain_mix_t*)wtk_calloc(nmix,sizeof(wtk_vtrain_mix_t));
	for(i=0;i<nmix;++i)
	{
		a->mix[i].mp=(wtk_vtrain_float_t*)wtk_calloc(vsize,sizeof(wtk_vtrain_float_t));
	}
	wtk_vtrain_acc2_reset(a);
	return a;
}

void wtk_vtrain_acc2_delete(wtk_vtrain_acc2_t *a)
{
	int i;

	for(i=0;i<a->nmix;++i)
	{
		wtk_free(a->mix[i].mp);
	}
	wtk_free(a->mix);
	wtk_free(a);
}

void wtk_vtrain_acc2_reset(wtk_vtrain_acc2_t *a)
{
	int i;
	wtk_vtrain_mix_t *mix;

	for(i=0;i<a->nmix;++i)
	{
		//a->mixprob[i]=0;
		mix=a->mix+i;
		mix->occ=0;
		memset(mix->mp,0,sizeof(float)*a->vsize);
//		for(j=0;j<a->vsize;++j)
//		{
//			mix->mp[j]=0;
//		}
	}
}


wtk_vtrain_t* wtk_vtrain_new(wtk_vtrain_cfg_t *cfg,wtk_vparm_cfg_t *parm_cfg,int use_share_parm)
{
	wtk_vtrain_t *v;

	v=(wtk_vtrain_t*)wtk_malloc(sizeof(wtk_vtrain_t));
	v->cfg=cfg;
	v->parm_cfg=parm_cfg;
	//wtk_debug("use_share_parm=%d\n",use_share_parm);
	if(use_share_parm)
	{
		v->parm=NULL;
	}else
	{
		v->parm=wtk_vparm_new(parm_cfg);
	}
	v->acc=wtk_vtrain_acc_new(parm_cfg->speech->pState[2]->pStream[0].nMixture,parm_cfg->hmmset.hmmset->vec_size);
	if(cfg->use_with_detect)
	{
		v->acc2=wtk_vtrain_acc2_new(parm_cfg->speech->pState[2]->pStream[0].nMixture,parm_cfg->hmmset.hmmset->vec_size);
	}else
	{
		v->acc2=NULL;
	}
	//wtk_debug("use_with_detect=%d\n",cfg->use_with_detect);
	v->index=0;
	v->cnt=0;
	//v->raw_speech=parm_cfg->speech->pState[2]->pStream;
	v->speech=NULL;
	v->heap=wtk_heap_new(4096);
	v->nframe=0;
	wtk_vtrain_reset(v);
	return v;
}

void wtk_vtrain_delete(wtk_vtrain_t *t)
{
	if(t->acc2)
	{
		wtk_vtrain_acc2_delete(t->acc2);
	}
	if(t->parm)
	{
		wtk_vparm_delete(t->parm);
	}
	wtk_heap_delete(t->heap);
	wtk_vtrain_acc_delete(t->acc);
	wtk_free(t);
}

void wtk_vtrain_reset(wtk_vtrain_t *t)
{
//	if(t->log)
//	{
//		wtk_log_log0(t->log,"reset vtrain");
//	}
	if(t->parm)
	{
		wtk_vparm_reset(t->parm);
	}
	//wtk_heap_reset(t->heap);
	//t->speech=NULL;
	t->index=1;
	if(!t->acc2)
	{
		t->cnt=0;
	}
	//t->nframe=0;
	//wtk_vtrain_acc_reset(t->acc);
//	if(t->acc2)
//	{
//		wtk_vtrain_acc2_reset(t->acc2);
//	}
}

void wtk_vtrain_reset_acc2(wtk_vtrain_t *t)
{
	//wtk_debug("reset acc2\n");
	if(t->acc2)
	{
		t->cnt=0;
		wtk_vtrain_acc2_reset(t->acc2);
	}
}

void wtk_vtrain_update_acc2(wtk_vtrain_t *v)
{
	wtk_vtrain_acc_t *acc=v->acc;
	wtk_vtrain_acc2_t *acc2=v->acc2;
	wtk_vtrain_mix_t *mix1,*mix2;
	int i;
	float *fp1,*fp2,*fp1e;

	//wtk_debug("nframe=%d\n",v->nframe);
	//wtk_debug("acc2=%p nframe=%d.",v->acc2,v->nframe);
	if(!v->acc2){return;}
	if(v->nframe<=0){return;}
	++v->cnt;
	//wtk_log_log(v->log,"cnt=%d,acc-nmix=%d,acc2-nmix=%d nframe=%d.",v->cnt,acc->nmix,acc2->nmix,v->nframe);
	//wtk_debug("update acc2 cnt=%d nmix=%d\n",v->cnt,acc->nmix);
	for(i=0;i<acc->nmix;++i)
	{
		mix1=acc->mix+i;
		mix2=acc2->mix+i;
		mix2->occ+=mix1->occ;

		//wtk_log_log(v->log,"acc2=%p mix=%f mix2=%f.",v->acc2,mix1->occ,mix2->occ);
		//wtk_debug("v[%d]=%f/%f\n",i,mix2->occ,mix1->occ);
//		if(i>=10)
//		{
//			exit(0);
//		}
		fp1=mix2->mp;
		fp1e=fp1+acc->vsize;
		fp2=mix1->mp;
		while(fp1<fp1e)
		{
			*(fp1++) += *(fp2++);
		}
//		for(j=0;j<acc->vsize;++j)
//		{
//			mix2->mp[j]+=mix1->mp[j];
//		}
	}
	wtk_vtrain_acc_reset(acc);
}


void wtk_vtrain_start(wtk_vtrain_t *t)
{
	wtk_mixture_t *mix;
	double f1,f2;
	int i;

//	if(t->log)
//	{
//		wtk_log_log0(t->log,"start vtrain");
//	}
	t->nframe=0;
	if(t->parm)
	{
		wtk_vparm_start(t->parm);
	}
	wtk_vtrain_acc_reset(t->acc);
	wtk_heap_reset(t->heap);
	t->speech=wtk_stream_dup(t->parm_cfg->speech->pState[2]->pStream,t->heap);
	if(t->parm && t->parm->cfg->hmmset.use_fix)
	{
		f1=1.0/t->parm->cfg->hmmset.mean_scale;
		f2=1.0/t->parm->cfg->hmmset.var_scale;
		for(i=0;i<t->speech->nMixture;++i)
		{
			mix=t->speech->pmixture+i;
			wtk_vector_fix_scale(mix->pdf->mean,f1);
			wtk_vector_fix_scale(mix->pdf->variance,f2);
		}
	}
}

void wtk_vtrain_feed_feature(wtk_vtrain_t *v,wtk_vparm_feature_t *f,int is_end)
{
	wtk_stream_t *stream;
	wtk_mixture_t *mix;
	int i,j;
	double prob,streamprob,gamma;
	int nmix=v->acc->nmix;
	wtk_vtrain_acc_t *acc=v->acc;
	wtk_vtrain_mix_t *vi;
	wtk_fixi_t *fixi=NULL;
	float *fp1,*fp2,*fp1e;

	//wtk_debug("fead feaute=%p\n",f->v.feature);
	if(!f){goto end;}
	stream=v->speech;
	if(v->cfg->skip_frame>0)
	{
		if(f->index!=v->index)
		{
			goto end;
		}
		v->index+=v->cfg->skip_frame+1;
	}
	streamprob = LZERO;
	if(f->use_fix)
	{
		fixi=f->v.fix;
		//wtk_fixi_print(fixi);
		for(mix=stream->pmixture,j=0;j<stream->nMixture;++j,++mix)
		{
			prob=mix->fWeight+wtk_mixpdf_calc_dia_prob_fix(mix->pdf,fixi,f->parm->scale);
			//wtk_debug("w=%f prob=%f scale=%f\n",mix->fWeight,prob,v->parm->scale);
			//exit(0);
			acc->mixprob[j]=prob;
			streamprob=wtk_log_add(streamprob,prob,f->parm->cfg->min_log_exp);
		}
	}else
	{
		for(mix=stream->pmixture,j=0;j<stream->nMixture;++j,++mix)
		{
			prob=mix->fWeight+wtk_mixpdf_calc_dia_prob(mix->pdf,f->v.feature->rv);
			//wtk_debug("w=%f prob=%f\n",mix->fWeight,prob);
			acc->mixprob[j]=prob;
			streamprob=wtk_log_add(streamprob,prob,f->parm->cfg->min_log_exp);
			//wtk_debug("v[%d]=%f\n",j,streamprob);
		}
	}
	//wtk_debug("outp=%f\n",streamprob);
	//exit(0);
	++v->nframe;
	for(i=0;i<nmix;++i)
	{
		vi=acc->mix+i;
		gamma=exp(acc->mixprob[i]-streamprob);
		//wtk_debug("%f,%f=%f\n",acc->mixprob[i],out_p,gamma);
		vi->occ+=gamma;
		fp1=vi->mp;
		fp1e=fp1+acc->vsize;
		fp2=f->v.feature->rv+1;
		while(fp1<fp1e)
		{
			*(fp1++)+=gamma * (*fp2++);
		}
	}
end:
	if(is_end)
	{
		if(!v->acc2)
		{
			++v->cnt;
		}
	}
	//wtk_debug("frame=%d cnt=%d\n",v->nframe,v->cnt);
}


void wtk_vtrain_feed(wtk_vtrain_t *v,char *data,int bytes,int is_end)
{
	wtk_queue_node_t *qn;
	wtk_feat_t *f;
	wtk_stream_t *stream;
	wtk_mixture_t *mix;
	int i,j;
	double prob,streamprob,gamma;
	int nmix=v->acc->nmix;
	wtk_vtrain_acc_t *acc=v->acc;
	wtk_vtrain_mix_t *vi;
	wtk_fixi_t *fixi=NULL;
	float *fp1,*fp2,*fp1e;

	wtk_vparm_feed(v->parm,data,bytes,is_end);
	stream=v->speech;
	while(1)
	{
		qn=wtk_queue_pop(&(v->parm->parm_output_q));
		if(!qn){break;}
		f=data_offset2(qn,wtk_feat_t,queue_n);
		//wtk_feature_print(f);
		if(v->cfg->skip_frame>0)
		{
			if(f->index!=v->index)
			{
				wtk_fextra_push_feature(v->parm->parm,f);
				continue;
			}
			v->index+=v->cfg->skip_frame+1;
		}
		//wtk_debug("v[%d]\n",f->index);
		streamprob = LZERO;
		if(v->parm->cfg->hmmset.use_fix)
		{
			fixi=wtk_vparm_get_fixi(v->parm,f,v->parm->cfg->hmmset.mean_scale);
			//wtk_fixi_print(fixi);
			for(mix=stream->pmixture,j=0;j<stream->nMixture;++j,++mix)
			{
				prob=mix->fWeight+wtk_mixpdf_calc_dia_prob_fix(mix->pdf,fixi,v->parm->scale);
				//wtk_debug("w=%f prob=%f scale=%f\n",mix->fWeight,prob,v->parm->scale);
				//exit(0);
				acc->mixprob[j]=prob;
				streamprob=wtk_log_add(streamprob,prob,v->parm->cfg->min_log_exp);
			}
		}else
		{
			for(mix=stream->pmixture,j=0;j<stream->nMixture;++j,++mix)
			{
				prob=mix->fWeight+wtk_mixpdf_calc_dia_prob(mix->pdf,f->rv);
				//wtk_debug("w=%f prob=%f\n",mix->fWeight,prob);
				acc->mixprob[j]=prob;
				streamprob=wtk_log_add(streamprob,prob,v->parm->cfg->min_log_exp);
				//wtk_debug("v[%d]=%f\n",j,streamprob);
			}
		}
		++v->nframe;
		//wtk_debug("outp=%f\n",streamprob);
		//exit(0);
		for(i=0;i<nmix;++i)
		{
			vi=acc->mix+i;
			gamma=exp(acc->mixprob[i]-streamprob);
			//wtk_debug("%f,%f=%f\n",acc->mixprob[i],out_p,gamma);
			vi->occ+=gamma;
//			if(fixi)
//			{
//				//wtk_debug("gamma=%f\n",gamma);
//				gamma=gamma*v->parm->cfg->hmmset.mean_scale;
//				vx=wtk_float_round(gamma);
//				//wtk_debug("vx=%d\n",vx);
//				for(j=0;j<acc->vsize;++j)
//				{
//					vi->mp[j]+=vx*fixi->p[j];
//					//wtk_debug("v[%d]=%f\n",j,vi->mp[j]);
//				}
//				//exit(0);
//			}else
			{
				fp1=vi->mp;
				fp1e=fp1+acc->vsize;
				fp2=f->rv+1;
				while(fp1<fp1e)
				{
					*(fp1++)+=gamma * (*fp2++);
				}
//				for(j=0;j<acc->vsize;++j)
//				{
//					vi->mp[j]+=gamma*f->rv[j+1];
//				}
			}
		}
		//exit(0);
		wtk_fextra_push_feature(v->parm->parm,f);
	}
	if(is_end)
	{
		if(!v->acc2)
		{
			++v->cnt;
		}
		wtk_vparm_reset(v->parm);
	}
}


void wtk_vtrain_update_hmm(wtk_vtrain_t *v)
{
	wtk_vtrain_acc_t *acc=v->acc;
	wtk_vtrain_mix_t *pm;
	wtk_mixture_t *mix;
	wtk_stream_t *speech=v->speech;
	float mapTau=v->cfg->mapTau;
	int i;
	float *fp1,*fp2,*fp1e;
	float f;

	//wtk_debug("update hmm cnt=%d nmix=%d\n",v->cnt,acc->nmix);
	for(i=0;i<acc->nmix;++i)
	{
		if(v->acc2)
		{
			pm=v->acc2->mix+i;
			//wtk_debug("occ=%f\n",pm->occ);
		}else
		{
			pm=acc->mix+i;
		}
		//wtk_debug("occ[%d/%d]=%f\n",i,acc->nmix,pm->occ);
		if(pm->occ>0)
		{
			//wtk_debug("update i=%d...\n",i);
			mix=speech->pmixture+i;
			fp1=mix->pdf->mean+1;
			fp1e=fp1+acc->vsize;
			fp2=pm->mp;
			f=1.0/(mapTau+pm->occ);
			//wtk_debug("update scale[%d/%d] f=%f\n",i,acc->nmix,f);
			while(fp1<fp1e)
			{
				*(fp1)=((*fp1)*mapTau+*(fp2++))*f;
				++fp1;
			}
//			for(j=0;j<acc->vsize;++j)
//			{
//				mix->pdf->mean[j+1]=(mix->pdf->mean[j+1]*mapTau+pm->mp[j])/(mapTau+pm->occ);
//			}
		}
	}
	//exit(0);
}

void wtk_vtrain_print_hmm(wtk_vtrain_t *v,wtk_strbuf_t *buf)
{
	wtk_hmmset_t *hl=v->parm->cfg->hmmset.hmmset;
	wtk_stream_t *stream=v->speech;
	wtk_mixture_t *mix;
	float weight;
	int i,j;

	wtk_strbuf_reset(buf);
	wtk_strbuf_push_f(buf,"~o\n");
    wtk_strbuf_push_f(buf,"<STREAMINFO> %d %d\n",hl->stream_width[0],hl->stream_width[1]);
    wtk_strbuf_push_f(buf,"<VECSIZE> %d<NULLD><PLP_D_A_Z_0><DIAGC>\n",hl->vec_size);

    wtk_strbuf_push_f(buf,"~h \"speech\"\n");
    wtk_strbuf_push_f(buf,"<BEGINHMM>\n");
    wtk_strbuf_push_f(buf,"<NUMSTATES> 3\n");
    wtk_strbuf_push_f(buf,"<STATE> 2\n");
    wtk_strbuf_push_f(buf,"<NUMMIXES> %d\n",stream->nMixture);
    for(i=0;i<stream->nMixture;i++)
    {
		mix = stream->pmixture+i;
		weight=exp(mix->fWeight);
		wtk_strbuf_push_f(buf,"<MIXTURE> %d %e\n",i+1,weight);
		wtk_strbuf_push_f(buf,"<MEAN> %d\n",hl->vec_size);
		for(j=1;j<=hl->vec_size;j++)
		{
			wtk_strbuf_push_f(buf," %e",mix->pdf->mean[j]);
			//wtk_debug("v[%d]=%f\n",j,mix->pdf->mean[j]);
		}
		wtk_strbuf_push_f(buf,"\n");
		wtk_strbuf_push_f(buf,"<VARIANCE> %d\n",hl->vec_size);
		for(j=1;j<=hl->vec_size;j++)
		{
			wtk_strbuf_push_f(buf," %e",-0.5/mix->pdf->variance[j]);
		}
		wtk_strbuf_push_f(buf,"\n");
		wtk_strbuf_push_f(buf,"<GCONST> %e\n",-2*mix->pdf->fGconst);
    }

    wtk_strbuf_push_f(buf,"<TRANSP> 3\n");
    for(i=1;i<=3;i++)
    {
    	for(j=1;j<=3;j++)
    	{
    		wtk_strbuf_push_f(buf," %e",exp(v->parm->cfg->speech->transP[i][j]));
    	}
    	wtk_strbuf_push_f(buf,"\n");
    }
    wtk_strbuf_push_f(buf,"<ENDHMM>\n");
}


void wtk_vtrain_write_hmm(wtk_vtrain_t *v,char *fn)
{
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(1024,1);
	wtk_vtrain_print_hmm(v,buf);
	file_write_buf(fn,buf->data,buf->pos);
	wtk_strbuf_delete(buf);
}


void wtk_vtrain_print_hmm_bin(wtk_vtrain_t *v,wtk_strbuf_t *buf)
{
	wtk_hmmset_t *hl;
	wtk_stream_t *stream=v->speech;
	wtk_mixture_t *mix;
	int i;

	//wtk_debug("save hmm cnt=%d\n",v->cnt);
	if(v->parm)
	{
		hl=v->parm->cfg->hmmset.hmmset;
	}else
	{
		hl=v->parm_cfg->hmmset.hmmset;
	}
	wtk_strbuf_reset(buf);
	i=stream->nMixture;
	wtk_strbuf_push(buf,(char*)&(i),4);
	i=hl->vec_size;
	wtk_strbuf_push(buf,(char*)&(i),4);
	//print_hex(buf->data,buf->pos);
	//wtk_debug("[%d,%d]\n",stream->nMixture,hl->vec_size);
    for(i=0;i<stream->nMixture;i++)
    {
		mix = stream->pmixture+i;
		wtk_strbuf_push(buf,(char*)&(mix->fWeight),4);
		wtk_strbuf_push(buf,(char*)(mix->pdf->mean+1),hl->vec_size*4);
		wtk_strbuf_push(buf,(char*)(mix->pdf->variance+1),hl->vec_size*4);
//		for(j=1;j<=hl->vec_size;++j)
//		{
//			wtk_strbuf_push(buf,(char*)(mix->pdf->mean+j),4);
//		}
//		for(j=1;j<=hl->vec_size;++j)
//		{
//			wtk_strbuf_push(buf,(char*)(mix->pdf->variance+j),4);
//		}
		wtk_strbuf_push(buf,(char*)&(mix->pdf->fGconst),4);
    }
    /*
    i=3;
	wtk_strbuf_push(buf,(char*)&(i),4);
    for(i=1;i<=3;i++)
    {
    	for(j=1;j<=3;j++)
    	{
    		wtk_strbuf_push(buf,(char*)&(v->parm->cfg->speech->transP[i][j]),4);
    	}
    }*/
}

void wtk_vtrain_write_hmm_bin(wtk_vtrain_t *v,char *fn)
{
	wtk_strbuf_t *buf;

	wtk_debug("write %s\n",fn);
	buf=wtk_strbuf_new(1024,1);
	wtk_vtrain_print_hmm_bin(v,buf);
	file_write_buf(fn,buf->data,buf->pos);
	wtk_strbuf_delete(buf);
}

void wtk_vtrain_write_hmm_bin2(wtk_vtrain_t *v,char *fn,int len,wtk_ubin_t *ubin)
{
	wtk_strbuf_t *buf;
	wtk_string_t str_fn;
	wtk_string_t data;
	char attr=0;

	buf=wtk_strbuf_new(1024,1);
	wtk_vtrain_print_hmm_bin(v,buf);
	str_fn.data=fn;str_fn.len=len;
	data.data=buf->data;data.len=buf->pos;
	wtk_ubin_append(ubin,&str_fn,&data,attr);
	wtk_strbuf_delete(buf);
}

