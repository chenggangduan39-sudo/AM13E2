#include "wtk_vdetect.h" 

wtk_vdetect_t* wtk_vdetect_new(wtk_vdetect_cfg_t *cfg,wtk_vparm_cfg_t *parm_cfg)
{
	wtk_vdetect_t *d;

	d=(wtk_vdetect_t*)wtk_malloc(sizeof(wtk_vdetect_t));
	d->cfg=cfg;
	if(parm_cfg)
	{
		d->parm=wtk_vparm_new(parm_cfg);
	}else
	{
		d->parm=NULL;
	}
	wtk_vdetect_reset(d);
	return d;
}

void wtk_vdetect_delete(wtk_vdetect_t *d)
{
	if(d->parm)
	{
		wtk_vparm_delete(d->parm);
	}
	wtk_free(d);
}

void wtk_vdetect_reset(wtk_vdetect_t *d)
{
	//wtk_debug("reset\n");
	d->index=1;
	d->usr=NULL;
	d->mean_prob=0;
	d->frames=0;
	d->max_llr=d->cfg->thresh;
	d->max_usr=NULL;
	wtk_vdetect_cfg_reset_usr(d->cfg);
	if(d->parm)
	{
		wtk_vparm_reset(d->parm);
	}
}

void wtk_vdetect_start(wtk_vdetect_t *d)
{
	//wtk_debug("start\n");
	if(d->parm)
	{
		wtk_vparm_start(d->parm);
	}
}

void wtk_vdetect_feed_feature(wtk_vdetect_t *d,wtk_vparm_feature_t *f,int is_end)
{
	wtk_queue_node_t *qn2;
	wtk_vdetect_usr_t *usr;
	double llr;
	wtk_fixi_t *fixi;

	if(!f){goto end;}
	if(d->cfg->skip_frame>0)
	{
		if(d->index!=f->index)
		{
			goto end;
		}
		d->index+=d->cfg->skip_frame+1;
	}
	if(f->use_fix)
	{
		fixi=f->v.fix;//wtk_vparm_get_fixi(f->parm,f,f->parm->cfg->hmmset.mean_scale);
		//wtk_fixi_print(fixi);
		for(qn2=d->cfg->usr_q.pop;qn2;qn2=qn2->next)
		{
			usr=data_offset2(qn2,wtk_vdetect_usr_t,q_n);
			if(usr->attr & WTK_UBIN_ATTR_INVALID){continue;}
			usr->prob+=wtk_hmmset_calc_prob_fix(f->parm->cfg->hmmset.hmmset,usr->hmm->pState[2],fixi,d->parm->scale);
		}
		d->mean_prob+=wtk_hmmset_calc_prob_fix(f->parm->cfg->hmmset.hmmset,f->parm->cfg->speech->pState[2],fixi,f->parm->scale);
	}else
	{
		for(qn2=d->cfg->usr_q.pop;qn2;qn2=qn2->next)
		{
			usr=data_offset2(qn2,wtk_vdetect_usr_t,q_n);
			if(usr->attr & WTK_UBIN_ATTR_INVALID){continue;}
			usr->prob+=wtk_hmmset_calc_prob(f->parm->cfg->hmmset.hmmset,usr->hmm->pState[2],f->v.feature->rv);
			//wtk_debug("[%.*s]=%f\n",usr->name.len,usr->name.data,usr->prob);
		}
		d->mean_prob += wtk_hmmset_calc_prob(f->parm->cfg->hmmset.hmmset,f->parm->cfg->speech->pState[2],f->v.feature->rv);
	}
	++d->frames;
end:
	if(is_end)
	{
		//wtk_debug("mean=%f\n",d->mean_prob);
		//wtk_debug("len=%d\n",d->cfg->usr_q.length);
		for(qn2=d->cfg->usr_q.pop;qn2;qn2=qn2->next)
		{
			usr=data_offset2(qn2,wtk_vdetect_usr_t,q_n);
			if(usr->attr & WTK_UBIN_ATTR_INVALID){continue;}
			//wtk_debug("prob=%f\n",usr->prob);
			if(d->frames==0)
			{
				llr=-1.0;
			}else
			{
				llr=(usr->prob-d->mean_prob)/d->frames;
			}
			usr->llr=llr;
			//wtk_debug("[%.*s]=%f %f/%f\n",usr->name.len,usr->name.data,usr->llr,usr->prob,d->mean_prob);
//			wtk_debug("%.*s llr=%f next=%p\n",usr->name.len,usr->name.data,llr,qn2->next);
			if(!d->max_usr || usr->llr>d->max_usr->llr)
			{
				d->max_usr=usr;
			}
			if(llr>d->max_llr)
			{
				d->max_llr=llr;
				d->usr=usr;
				//wtk_debug("llr=%f [%.*s]\n",llr,usr->name.len,usr->name.data);
			}
		}
	}
	//wtk_debug("frames=%d\n",d->frames);
}


void wtk_vdetect_feed(wtk_vdetect_t *d,char *data,int bytes,int is_end)
{
	wtk_queue_node_t *qn,*qn2;
	wtk_feat_t *f;
	wtk_vdetect_usr_t *usr;
	double llr;
	wtk_fixi_t *fixi;

	wtk_vparm_feed(d->parm,data,bytes,is_end);
	while(1)
	{
		qn=wtk_queue_pop(&(d->parm->parm_output_q));
		if(!qn){break;}
		f=data_offset2(qn,wtk_feat_t,queue_n);
		//wtk_feature_print(f);
		if(d->cfg->skip_frame>0)
		{
			if(d->index!=f->index)
			{
				wtk_fextra_push_feature(d->parm->parm,f);
				continue;
			}
			d->index+=d->cfg->skip_frame+1;
		}
		if(d->parm->cfg->hmmset.use_fix)
		{
			fixi=wtk_vparm_get_fixi(d->parm,f,d->parm->cfg->hmmset.mean_scale);
			//wtk_fixi_print(fixi);

			for(qn2=d->cfg->usr_q.pop;qn2;qn2=qn2->next)
			{
				usr=data_offset2(qn2,wtk_vdetect_usr_t,q_n);
				if(usr->attr & WTK_UBIN_ATTR_INVALID){continue;}
				usr->prob+=wtk_hmmset_calc_prob_fix(d->parm->cfg->hmmset.hmmset,usr->hmm->pState[2],fixi,d->parm->scale);
			}
			d->mean_prob+=wtk_hmmset_calc_prob_fix(d->parm->cfg->hmmset.hmmset,d->parm->cfg->speech->pState[2],fixi,d->parm->scale);
		}else
		{
			for(qn2=d->cfg->usr_q.pop;qn2;qn2=qn2->next)
			{
				usr=data_offset2(qn2,wtk_vdetect_usr_t,q_n);
				if(usr->attr & WTK_UBIN_ATTR_INVALID){continue;}
				usr->prob+=wtk_hmmset_calc_prob(d->parm->cfg->hmmset.hmmset,usr->hmm->pState[2],f->rv);
			}
			d->mean_prob += wtk_hmmset_calc_prob(d->parm->cfg->hmmset.hmmset,d->parm->cfg->speech->pState[2],f->rv);
		}
		++d->frames;
		wtk_fextra_push_feature(d->parm->parm,f);
	}
	if(is_end)
	{
		//wtk_debug("mean=%f\n",d->mean_prob);
		//wtk_debug("len=%d\n",d->cfg->usr_q.length);
		for(qn2=d->cfg->usr_q.pop;qn2;qn2=qn2->next)
		{
			usr=data_offset2(qn2,wtk_vdetect_usr_t,q_n);
			if(usr->attr & WTK_UBIN_ATTR_INVALID){continue;}
			//wtk_debug("prob=%f\n",usr->prob);
			if(d->frames==0)
			{
				llr=-1.0;
			}else
			{
				llr=(usr->prob-d->mean_prob)/d->frames;
			}
			usr->llr=llr;
//			wtk_debug("%.*s llr=%f next=%p\n",usr->name.len,usr->name.data,llr,qn2->next);
			if(!d->max_usr || usr->llr>d->max_usr->llr)
			{
				d->max_usr=usr;
			}
			if(llr>d->max_llr)
			{
				d->max_llr=llr;
				d->usr=usr;
				//wtk_debug("llr=%f [%.*s]\n",llr,usr->name.len,usr->name.data);
			}
		}
	}
}

void wtk_vdetect_print(wtk_vdetect_t *d)
{
	if(d->usr)
	{
		wtk_debug("[%.*s]=%f\n",d->usr->name.len,d->usr->name.data,d->max_llr);
	}else
	{
		wtk_debug("[null]=%f\n",d->max_llr);
	}
}

wtk_string_t* wtk_vdetect_get_usr(wtk_vdetect_t *d)
{
	wtk_string_t *v=NULL;

	if(d->usr && d->usr->name.len>0)
	{
		v=&(d->usr->name);
	}
	return v;
}
