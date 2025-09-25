#include "wtk_vparm.h" 


wtk_vparm_t* wtk_vparm_new(wtk_vparm_cfg_t *cfg)
{
	wtk_vparm_t *p;

	p=(wtk_vparm_t*)wtk_malloc(sizeof(wtk_vparm_t));
	p->cfg=cfg;
	wtk_queue_init(&(p->parm_output_q));
	p->parm=wtk_fextra_new(&(cfg->parm));
	wtk_fextra_set_output_queue(p->parm,&(p->parm_output_q));
	if(cfg->use_vad)
	{
		p->vad2=wtk_vad_new(&(cfg->vad2),&(p->vad_output_q));
		wtk_queue_init(&(p->vad_output_q));
		wtk_queue_init(&(p->sil_q));
	}else
	{
		p->vad2=NULL;
	}
	p->fix=NULL;
	if(cfg->hmmset.use_fix)
	{
		p->scale=cfg->hmmset.mean_scale*cfg->hmmset.mean_scale*cfg->hmmset.var_scale;
	}
	wtk_vparm_reset(p);
	return p;
}

void wtk_vparm_delete(wtk_vparm_t *vp)
{
	if(vp->fix)
	{
		wtk_fixi_delete(vp->fix);
	}
	wtk_fextra_delete(vp->parm);
	if(vp->vad2)
	{
		wtk_vad_delete(vp->vad2);
	}
	wtk_free(vp);
}

void wtk_vparm_reset(wtk_vparm_t *vp)
{
	wtk_fextra_reset(vp->parm);
	if(vp->vad2)
	{
		wtk_queue_init(&(vp->sil_q));
		wtk_vad_reset(vp->vad2);
		vp->state=WTK_VPARM_INIT;
	}
}

void wtk_vparm_start(wtk_vparm_t *vp)
{
	if(vp->vad2)
	{
		wtk_vad_start(vp->vad2);
	}
}

void wtk_vparm_feed_parm(wtk_vparm_t *vp,char *data,int bytes,int is_end)
{
	wtk_fextra_feed2(vp->parm,data,bytes,is_end);
}

void wtk_vparm_flush_sil(wtk_vparm_t *vp)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *vf;

	while(1)
	{
		qn=wtk_queue_pop(&(vp->sil_q));
		if(!qn){break;}
		vf=data_offset2(qn,wtk_vframe_t,q_n);
		wtk_vparm_feed_parm(vp,(char*)(vf->wav_data),vf->frame_step<<1,0);
		wtk_vad_push_vframe(vp->vad2,vf);
	}
}


void wtk_vparm_feed_vad(wtk_vparm_t *vp,char *data,int bytes,int is_end)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *vf;

	wtk_vad_feed(vp->vad2,data,bytes,is_end);
	while(1)
	{
		qn=wtk_queue_pop(&(vp->vad_output_q));
		if(!qn){break;}
		vf=data_offset2(qn,wtk_vframe_t,q_n);
		//wtk_debug("v[%d]=%d\n",vf->index,vf->state);
		switch(vp->state)
		{
		case WTK_VPARM_INIT:
			if(vf->state!=wtk_vframe_sil)
			{
				vp->state=WTK_VPARM_SPEECH;
				wtk_vparm_feed_parm(vp,(char*)(vf->wav_data),vf->frame_step<<1,0);
				wtk_vad_push_vframe(vp->vad2,vf);
			}else
			{
				wtk_vad_push_vframe(vp->vad2,vf);
			}
			break;
		case WTK_VPARM_SIL:
			if(vf->state!=wtk_vframe_sil)
			{
				vp->state=WTK_VPARM_SPEECH;
				//ki+=vp->sil_q.length;
				wtk_vparm_flush_sil(vp);
				wtk_vparm_feed_parm(vp,(char*)(vf->wav_data),vf->frame_step<<1,0);
				wtk_vad_push_vframe(vp->vad2,vf);
			}else
			{
				wtk_queue_push(&(vp->sil_q),&(vf->q_n));
			}
			break;
		case WTK_VPARM_SPEECH:
			if(vf->state!=wtk_vframe_sil)
			{
				wtk_vparm_feed_parm(vp,(char*)(vf->wav_data),vf->frame_step<<1,0);
				wtk_vad_push_vframe(vp->vad2,vf);
			}else
			{
				vp->state=WTK_VPARM_SIL;
				wtk_queue_push(&(vp->sil_q),&(vf->q_n));
			}
			break;
		}
	}
	if(is_end)
	{
		wtk_vparm_feed_parm(vp,0,0,is_end);
		//wtk_vparm_acc_print(vp->acc);
	}
}

void wtk_vparm_feed(wtk_vparm_t *vp,char *data,int bytes,int is_end)
{
	//wtk_debug("use_vad=%d\n",vp->cfg->use_vad);
	if(vp->cfg->use_vad)
	{
		wtk_vparm_feed_vad(vp,data,bytes,is_end);
	}else
	{
		wtk_vparm_feed_parm(vp,data,bytes,is_end);
	}
}


wtk_fixi_t* wtk_vparm_get_fixi(wtk_vparm_t *vp,wtk_feat_t *f,float scale)
{
	wtk_fixi_t *fix;

	if(vp->fix)
	{
		fix=vp->fix;
	}else
	{
		fix=vp->fix=wtk_fixi_new(1,wtk_vector_size(f->rv));
	}
	//wtk_feature_print(f);
	wtk_fixi_scale(fix,f->rv,scale);
	//exit(0);
	return fix;
}

