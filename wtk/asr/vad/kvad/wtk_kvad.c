#include "wtk_kvad.h" 
void wtk_kvad_on_parm(wtk_kvad_t *vad,wtk_kfeat_t *feat);
void wtk_kvad_on_parm2(wtk_kvad_t *vad,wtk_kfeat_t *feat);
void wtk_kvad_on_fix_parm(wtk_kvad_t *vad,wtk_kfeat_t *feat);
void wtk_kvad_on_nnet3(wtk_kvad_t *vad,qtk_blas_matrix_t *feat,int end, int plus);

#ifdef USE_KFRAME
int wtk_kframe_bytes(wtk_kvad_t *vad)
{
	int bytes;

	bytes=sizeof(wtk_kframe_t);
	bytes+=vad->parm->parm->cfg->frame_step*sizeof(short);
	return bytes;
}

wtk_kframe_t* wtk_kvad_new_frame(wtk_kvad_t *vad)
{
	wtk_kframe_t *frame;

	frame=(wtk_kframe_t*)wtk_malloc(sizeof(wtk_kframe_t));
	//frame->wav=(short*)wtk_calloc(vad->cfg->parm2.parm.frame_step,sizeof(short));
	frame->wav=(short*)wtk_calloc(vad->parm->parm->cfg->frame_step,sizeof(short));
	frame->energy=0;
	return frame;
}

int wtk_kframe_delete(wtk_kframe_t *f)
{
	wtk_free(f->wav);
	wtk_free(f);
	return 0;
}

wtk_kframe_t* wtk_kvad_pop_frame(wtk_kvad_t *vad)
{
	wtk_kframe_t* f;

	//f=wtk_extra_new_feature(p);
	f=(wtk_kframe_t*)wtk_hoard_pop(&(vad->frame_hoard));
	f->index=vad->nframe++;
	f->speech=0;
	//wtk_debug("+frame=%d\n",f->index);
	return f;
}

void wtk_kvad_push_frame(wtk_kvad_t *vad,wtk_kframe_t *f)
{
//	static int last_ki=0;
//
//	if(f->index!=last_ki)
//	{
//		wtk_debug("-frame=%d\n",f->index);
//		exit(0);
//	}
//	last_ki=f->index+1;
	wtk_hoard_push(&(vad->frame_hoard),f);
}

int wtk_kvad_bytes(wtk_kvad_t *vad)
{
	int bytes;

	bytes=sizeof(wtk_kvad_t);
	bytes+=wtk_kxparm_bytes(vad->parm);
	bytes+=(vad->frame_hoard.cur_free+vad->frame_hoard.use_length)*wtk_kframe_bytes(vad);
	wtk_debug("vad=%.3fKB frame=%d/%d\n",bytes*1.0/1024,vad->frame_hoard.cur_free,vad->frame_hoard.use_length);
	return bytes;
}

wtk_kvad_t* wtk_kvad_new(wtk_kvad_cfg_t *cfg)
{
	wtk_kvad_t *vad;

	vad=(wtk_kvad_t*)wtk_malloc(sizeof(wtk_kvad_t));
	vad->cfg=cfg;
	vad->parm=wtk_kxparm_new(&(cfg->parm));
	if(cfg->use_fixpoint)
	{
			wtk_kxparm_set_notify(vad->parm,vad,(wtk_kxparm_notify_f)wtk_kvad_on_fix_parm);
	}else
	{
		wtk_kxparm_set_notify(vad->parm,vad,(wtk_kxparm_notify_f)wtk_kvad_on_parm);
	}
	wtk_hoard_init(&(vad->frame_hoard),offsetof(wtk_kframe_t,hoard_n),cfg->cache
			,(wtk_new_handler_t)wtk_kvad_new_frame,(wtk_delete_handler_t)wtk_kframe_delete,vad);
	wtk_queue_init(&(vad->input_q));
	wtk_queue_init(&(vad->trap_q));
	vad->cur_frame=NULL;
	wtk_queue_init(&(vad->expand_q));
	wtk_queue_init(&(vad->output_q));
	wtk_kvad_reset(vad);
	return vad;
}

void wtk_kvad_delete(wtk_kvad_t *vad)
{
	wtk_hoard_clean(&(vad->frame_hoard));
	wtk_kxparm_delete(vad->parm);
	wtk_free(vad);
}

void wtk_kvad_start(wtk_kvad_t *vad)
{
	wtk_kxparm_start(vad->parm);
}

void wtk_kvad_clean_queue(wtk_kvad_t *vad,wtk_queue_t *q)
{
	wtk_queue_node_t *qn;
	wtk_kframe_t *frame;

	while(1)
	{
		qn=wtk_queue_pop(q);
		if(!qn){break;}
		frame=data_offset2(qn,wtk_kframe_t,q_n);
		wtk_kvad_push_frame(vad,frame);
	}
}

void wtk_kvad_reset(wtk_kvad_t *vad)
{
	if(vad->expand_q.length>0)
	{
		wtk_kvad_clean_queue(vad,&(vad->expand_q));
	}
	if(vad->output_q.length>0)
	{
		wtk_kvad_clean_queue(vad,&(vad->output_q));
	}
	if(vad->cur_frame)
	{
		wtk_kvad_push_frame(vad,vad->cur_frame);
		vad->cur_frame=NULL;
	}
	wtk_queue_init(&(vad->trap_q));
	vad->state=WTK_KVAD_SIL;
	vad->expand_state=WTK_KVAD_SIL;
	vad->pos=0;
	vad->nframe=0;
	wtk_kxparm_reset(vad->parm);
	//wtk_debug("use=%d free=%d\n",vad->frame_hoard.use_length,vad->frame_hoard.cur_free);
}

void wtk_kvad_feed_raise_frame(wtk_kvad_t *vad,wtk_kframe_t *frame)
{
//	static int last_ki=0;
//
//	//wtk_debug("v[%d]=%s %p\n",frame->index,frame->speech?"speech":"sil",frame);
//	if(frame->index!=last_ki)
//	{
//		wtk_debug("v[%d]=%s %p\n",frame->index,frame->speech?"speech":"sil",frame);
//		exit(0);
//	}
//	last_ki=frame->index+1;
	wtk_queue_push(&(vad->output_q),&(frame->q_n));
}

void wtk_kvad_flush_expand_frame(wtk_kvad_t *vad,wtk_queue_t *q,int speech)
{
	wtk_queue_node_t *qn;
	wtk_kframe_t *frame;

	while(1)
	{
		qn=wtk_queue_pop(q);
		if(!qn){break;}
		frame=data_offset2(qn,wtk_kframe_t,q_n);
		frame->speech=speech;
		wtk_kvad_feed_raise_frame(vad,frame);
	}
}

void wtk_kvad_flush_expand(wtk_kvad_t *vad)
{
	//wtk_debug("v[%d]=%s\n",frame->index,frame->speech?"speech":"sil");
	switch(vad->expand_state)
	{
	case WTK_KVAD_SIL:
		if(vad->expand_q.length>0)
		{
			wtk_kvad_flush_expand_frame(vad,&(vad->expand_q),0);
		}
		break;
	case WTK_KVAD_SPEECH:
		wtk_kvad_flush_expand_frame(vad,&(vad->expand_q),1);
		break;
	}
}

void wtk_kvad_feed_expand_frame(wtk_kvad_t *vad,wtk_kframe_t *frame)
{
	wtk_queue_node_t *qn;

	//wtk_debug("v[%d]=%s %p\n",frame->index,frame->speech?"speech":"sil",frame);
	switch(vad->expand_state)
	{
	case WTK_KVAD_SIL:
		if(vad->cfg->left_margin>0)
		{
			if(frame->speech)
			{
				vad->expand_state=WTK_KVAD_SPEECH;
				if(vad->expand_q.length>0)
				{
					wtk_kvad_flush_expand_frame(vad,&(vad->expand_q),1);
				}
				wtk_kvad_feed_raise_frame(vad,frame);
			}else
			{
				wtk_queue_push(&(vad->expand_q),&(frame->q_n));
				if(vad->expand_q.length>vad->cfg->left_margin)
				{
					qn=wtk_queue_pop(&(vad->expand_q));
					frame=data_offset2(qn,wtk_kframe_t,q_n);
					wtk_kvad_feed_raise_frame(vad,frame);
				}
			}
		}else
		{
			if(frame->speech)
			{
				vad->expand_state=WTK_KVAD_SPEECH;
			}
			wtk_kvad_feed_raise_frame(vad,frame);
		}
		break;
	case WTK_KVAD_SPEECH:
		if(vad->cfg->right_margin>0)
		{
			if(frame->speech)
			{
				if(vad->expand_q.length>0)
				{
					wtk_kvad_flush_expand_frame(vad,&(vad->expand_q),1);
				}
				wtk_kvad_feed_raise_frame(vad,frame);
			}else
			{
				wtk_queue_push(&(vad->expand_q),&(frame->q_n));
				if(vad->expand_q.length>vad->cfg->right_margin)
				{
					wtk_kvad_flush_expand_frame(vad,&(vad->expand_q),1);
					vad->expand_state=WTK_KVAD_SIL;
				}
			}
		}else
		{
			if(frame->speech==0)
			{
				vad->expand_state=WTK_KVAD_SIL;
			}
			wtk_kvad_feed_raise_frame(vad,frame);
		}
		break;
	}
}


void wtk_kvad_flush_queue(wtk_kvad_t *vad,wtk_queue_t *q,int speech)
{
	wtk_queue_node_t *qn;
	wtk_kframe_t *f;
	//wtk_queue_t *q=&(vad->trap_q);

	while(1)
	{
		qn=wtk_queue_pop(q);
		if(!qn){break;}
		f=data_offset2(qn,wtk_kframe_t,q_n);
		f->speech=speech;
		wtk_kvad_feed_expand_frame(vad,f);
	}
}

int wtk_kvad_check_energy(wtk_kvad_t *vad)
{
	wtk_queue_node_t *qn;
	wtk_kframe_t *f;
	float e;
	int i,step=vad->cfg->parm.parm.frame_step;
	short *sv;

	e=0;
	for(qn=vad->trap_q.pop;qn;qn=qn->next)
	{
		f=data_offset2(qn,wtk_kframe_t,q_n);
		sv=f->wav;
		for(i=0;i<step;++i)
		{
			if(sv[i]>0)
			{
				e+=sv[i];
			}else
			{
				e-=sv[i];
			}
		}
	}
	//wtk_debug("e=%f/%d\n",e/(step*vad->trap_q.length),e>=(vad->trap_q.length*vad->cfg->speech_thresh*step));
	if(e>=(vad->trap_q.length*vad->cfg->speech_thresh*step))
	{
		return 1;
	}else
	{
		return 0;
	}
}

int wtk_kvad_check_energy_fix(wtk_kvad_t *vad)
{
	wtk_queue_node_t *qn;
	wtk_kframe_t *f;
	wtk_int64_t e;
	int i,step=vad->cfg->parm.parm.frame_step;
	short *sv;

	e=0;
	for(qn=vad->trap_q.pop;qn;qn=qn->next)
	{
		f=data_offset2(qn,wtk_kframe_t,q_n);
		sv=f->wav;
		for(i=0;i<step;++i)
		{
			if(sv[i]>0)
			{
				e+=sv[i];
			}else
			{
				e-=sv[i];
			}
		}
	}
	//wtk_debug("e=%f/%d\n",e/(step*vad->trap_q.length),e>=(vad->trap_q.length*vad->cfg->speech_thresh*step));
	if((e/step)>=(vad->trap_q.length*vad->cfg->fix_speech_thresh))
	{
		return 1;
	}else
	{
		return 0;
	}
}


void wtk_kvad_feed_frame(wtk_kvad_t *vad,wtk_kframe_t *frame)
{
//	wtk_debug("v[%d]=%s robin=%d/%d state=%d frame=%p\n",frame->index,frame->speech?"speech":"sil",vad->speech_trap->used,
//			vad->sil_trap->used,vad->state,frame);
	switch(vad->state)
	{
	case WTK_KVAD_SIL:
		if(frame->speech)
		{
			wtk_queue_push(&(vad->trap_q),&(frame->q_n));
			if(vad->trap_q.length>=vad->cfg->speech_trap)
			{
				if(vad->cfg->use_fixpoint)
				{
					if(vad->cfg->fix_speech_thresh>0 && wtk_kvad_check_energy_fix(vad)==0)
					{
						wtk_kvad_flush_queue(vad,&(vad->trap_q),0);
					}else
					{
						wtk_kvad_flush_queue(vad,&(vad->trap_q),1);
						vad->state=WTK_KVAD_SPEECH;
					}
				}else
				{
					if(vad->cfg->speech_thresh>0 && wtk_kvad_check_energy(vad)==0)
					{
						wtk_kvad_flush_queue(vad,&(vad->trap_q),0);
					}else
					{
						wtk_kvad_flush_queue(vad,&(vad->trap_q),1);
						vad->state=WTK_KVAD_SPEECH;
					}
				}
			}
		}else
		{
			if(vad->trap_q.length>0)
			{
				wtk_kvad_flush_queue(vad,&(vad->trap_q),0);
			}
			wtk_kvad_feed_expand_frame(vad,frame);
		}
		break;
	case WTK_KVAD_SPEECH:
		if(frame->speech)
		{
			if(vad->trap_q.length>0)
			{
				wtk_kvad_flush_queue(vad,&(vad->trap_q),1);
			}
			wtk_kvad_feed_expand_frame(vad,frame);
		}else
		{
			wtk_queue_push(&(vad->trap_q),&(frame->q_n));
			if(vad->trap_q.length>=vad->cfg->sil_trap)
			{
				wtk_kvad_flush_queue(vad,&(vad->trap_q),0);
				vad->state=WTK_KVAD_SIL;
			}
		}
		break;
	}
}

void wtk_kvad_flush_frame(wtk_kvad_t *vad)
{
	wtk_queue_node_t *qn;
	wtk_kframe_t *frame;

	//wtk_debug("v[%d]=%s\n",frame->index,frame->speech?"speech":"sil");
	while(1)
	{
		qn=wtk_queue_pop(&(vad->input_q));
		if(!qn){break;}
		frame=data_offset2(qn,wtk_kframe_t,q_n);
		frame->speech=vad->state==WTK_KVAD_SIL?0:1;
		wtk_kvad_feed_frame(vad,frame);
	}
	switch(vad->state)
	{
	case WTK_KVAD_SIL:
		if(vad->trap_q.length>0)
		{
			wtk_kvad_flush_queue(vad,&(vad->trap_q),0);
		}
		break;
	case WTK_KVAD_SPEECH:
		if(vad->trap_q.length>0)
		{
			wtk_kvad_flush_queue(vad,&(vad->trap_q),1);
		}
		break;
	}
	wtk_kvad_flush_expand(vad);
}

void wtk_kvad_on_fix_parm(wtk_kvad_t *vad,wtk_kfeat_t *feat)
{
	wtk_queue_node_t *qn;
	wtk_kframe_t *frame;
	int thresh;
	short *sv;

	if(vad->input_q.length<=0)
	{
		return;
	}
	qn=wtk_queue_pop(&(vad->input_q));
	frame=data_offset2(qn,wtk_kframe_t,q_n);
	thresh=vad->state==WTK_KVAD_SPEECH?vad->cfg->fix_speech_leave_prob:vad->cfg->fix_speech_enter_prob;//speech_enter_prob;
	sv=(short*)(feat->v);
	frame->speech=wtk_fixexp_calc(vad->cfg->parm.knn.fixe,sv[1])>=thresh?1:0;
	wtk_kvad_feed_frame(vad,frame);
}


void wtk_kvad_on_parm(wtk_kvad_t *vad,wtk_kfeat_t *feat)
{
	wtk_queue_node_t *qn;
	wtk_kframe_t *frame;
	float thresh;

//	int i;
//	int n=vad->parm->knn->cfg->output_dim;
//
//	for(i=0;i<n;++i)
//	{
//		wtk_debug("v[%d/%d]=%f\n",feat->index,i,exp(feat->v[i]));
//	}
	//wtk_debug("v[%d]=%f/%f %s\n",feat->index,feat->v[0],feat->v[1],(feat->v[0]-feat->v[1])>0?"sil":"speech");
	//wtk_debug("v[%d]=%f/%f %s\n",feat->index,exp(feat->v[0]),exp(feat->v[1]),(feat->v[0]-feat->v[1])>0?"sil":"speech");
	//exit(0);
	if(vad->input_q.length<=0)
	{
		return;
	}
	qn=wtk_queue_pop(&(vad->input_q));
	frame=data_offset2(qn,wtk_kframe_t,q_n);
	thresh=vad->state==WTK_KVAD_SPEECH?vad->cfg->speech_leave_prob:vad->cfg->speech_enter_prob;
	frame->speech=wtk_fast_exp2(feat->v[1])>=thresh?1:0;//(feat->v[0]-feat->v[1])>0?0:1;
	//wtk_debug("v[%d]=%f\n",feat->index,exp(feat->v[1]));
//	if(feat->index>100)
//	{
//		exit(0);
//	}
	wtk_kvad_feed_frame(vad,frame);
}

int wtk_kvad_get_left_data(wtk_kvad_t *vad,wtk_string_t *v)
{
	if(vad->cur_frame)
	{
		v->data=(char*)(vad->cur_frame->wav);
		v->len=(vad->pos<<1);
	}else
	{
		v->len=0;
	}
	return vad->pos;
}

void wtk_kvad_update_frame(wtk_kvad_t *vad,short *data,int len)
{
	wtk_kframe_t *frame;
	int win=vad->parm->cfg->parm.frame_step;
	short *s,*e;
	int n;
	int pos=vad->pos;

	frame=vad->cur_frame;
	if(!frame)
	{
		frame=wtk_kvad_pop_frame(vad);
	}
	s=data;e=s+len;
	while(s<e)
	{
		n=min(e-s,win-pos);
		memcpy(frame->wav+pos,s,n*sizeof(short));
		pos+=n;
		if(pos>=win)
		{
			wtk_queue_push(&(vad->input_q),&(frame->q_n));
			frame=wtk_kvad_pop_frame(vad);
			pos=0;
		}
		s+=n;
	}
	vad->cur_frame=frame;
	vad->pos=pos;
}

void wtk_kvad_print_mlf2(wtk_kvad_t *vad,FILE *s)
{
	wtk_queue_node_t *n;
	wtk_kframe_t *f;
#define PAD_0 "00000"
	int sil=1;
	int i=0;
	int last_index=0;
	float dur=vad->cfg->parm.parm.frame_step_ms*1e4;//parm2.parm.frame_step_ms*1e4;
	//float dur=vad->cfg->frame_time*1e4;
	wtk_queue_t *q=&(vad->output_q);
	//FILE *s=stdout;

	if(q->length>0)
	{
		fprintf(s,"0 ");
	}
	for(n=q->pop;n;n=n->next)
	{
		f=data_offset(n,wtk_kframe_t,q_n);
		//wtk_debug("v[%d]=%s %p\n",f->index,f->speech?"speech":"sil",f);
		//exit(0);
		if(sil)
		{
			if(f->speech)
			{
				if(f->index>1)
				{
					fprintf(s,"%.0f sil\n%.0f ",(f->index-1)*dur,(f->index-1)*dur);
				}
				last_index=f->index;
				++i;
				sil=0;
			}
		}else
		{
			if(f->speech==0)
			{
				if(f->index>last_index)
				{
					++i;
					fprintf(s,"%.0f speech\n%.0f ",(f->index-1)*dur,(f->index-1)*dur);
				}
				last_index=f->index;
				sil=1;
			}
		}
		if(!n->next)
		{
			fprintf(s,"%.0f %s\n",f->index*dur,f->speech?"speech":"sil");
		}
	}
	fprintf(s,".\n");
}

#else //USE VFRAME  FOR VAD2
int wtk_vframe_bytes(wtk_kvad_t *vad)
{
	int bytes;

	bytes=sizeof(wtk_vframe_t);
	bytes+=vad->parm->htk->cfg->frame_step*sizeof(short);
	return bytes;
}

wtk_vframe_t* wtk_kvad_new_frame(wtk_kvad_t *vad)
{
	wtk_vframe_t *frame;
	frame=(wtk_vframe_t*)wtk_malloc(sizeof(wtk_vframe_t));
        if (vad->cfg->parm.use_htk) {
            frame->wav_data = (short *)wtk_calloc(
                vad->parm->htk->cfg->frame_step, sizeof(short));
            frame->frame_size = vad->parm->htk->cfg->frame_size;
            frame->frame_step = vad->parm->htk->cfg->frame_step;
        } else {
            frame->wav_data = (short *)wtk_calloc(
                vad->parm->parm->cfg->frame_step, sizeof(short));
            frame->frame_size = vad->parm->parm->cfg->frame_size;
            frame->frame_step = vad->parm->parm->cfg->frame_step;
        }

        return frame;
}

int wtk_kvad_vframe_delete(wtk_vframe_t *f)
{
	wtk_free(f->wav_data);
	wtk_free(f);
	return 0;
}

wtk_vframe_t* wtk_kvad_pop_frame(wtk_kvad_t *vad)
{
	wtk_vframe_t* f;

	//f=wtk_extra_new_feature(p);
	f=(wtk_vframe_t*)wtk_hoard_pop(&(vad->frame_hoard));
	f->index=vad->nframe++;
	f->state=wtk_vframe_sil;
	f->raw_state=wtk_vframe_sil;
	//wtk_debug("+frame=%d\n",f->index);
	return f;
}

void wtk_kvad_push_frame(wtk_kvad_t *vad,wtk_vframe_t *f)
{
	wtk_hoard_push(&(vad->frame_hoard),f);
}

int wtk_kvad_frame_bytes(wtk_kvad_t *vad)
{
	int bytes;

	bytes=sizeof(wtk_vframe_t);
	bytes+=vad->parm->parm->cfg->frame_step*sizeof(short);
	return bytes;
}

int wtk_kvad_bytes(wtk_kvad_t *vad)
{
	int bytes;

	bytes=sizeof(wtk_kvad_t);
	bytes+=wtk_kxparm_bytes(vad->parm);
	//bytes+=(vad->frame_hoard.cur_free+vad->frame_hoard.use_length)*wtk_kvad_frame_bytes(vad);
//bytes+=(vad->frame_hoard.cur_free+vad->frame_hoard.use_length)*wtk_vframe_bytes(vad);
	wtk_debug("vad=%.3fKB frame=%d/%d\n",bytes*1.0/1024,vad->frame_hoard.cur_free,vad->frame_hoard.use_length);
	return bytes;
}

wtk_kvad_t* wtk_kvad_new2(wtk_kvad_cfg_t *cfg,wtk_queue_t **output_queue)
{
	wtk_kvad_t *vad;

	vad=(wtk_kvad_t*)wtk_malloc(sizeof(wtk_kvad_t));
	vad->cfg=cfg;
	vad->parm=wtk_kxparm_new(&(cfg->parm));
	if(cfg->use_fixpoint)
	{
		wtk_kxparm_set_notify(vad->parm,vad,(wtk_kxparm_notify_f)wtk_kvad_on_fix_parm);
	}else
	{
		wtk_kxparm_set_notify(vad->parm,vad,(wtk_kxparm_notify_f)wtk_kvad_on_parm);
	}

	if(vad->parm->htk && vad->parm->htk->nnet3)
	{
		qtk_nnet3_set_notify(vad->parm->htk->nnet3,(qtk_nnet3_feature_notify_f)wtk_kvad_on_nnet3,vad);
        } else if (vad->parm->nnet3 && vad->parm->cfg->use_nnet3) {
            qtk_nnet3_set_notify(vad->parm->nnet3,
                                 (qtk_nnet3_feature_notify_f)wtk_kvad_on_nnet3,
                                 vad);
        }

        wtk_hoard_init(&(vad->frame_hoard),offsetof(wtk_vframe_t,hoard_n),cfg->cache
			,(wtk_new_handler_t)wtk_kvad_new_frame,(wtk_delete_handler_t)wtk_kvad_vframe_delete,vad);
	wtk_queue_init(&(vad->input_q));
	wtk_queue_init(&(vad->trap_q));
	vad->cur_frame=NULL;
	wtk_queue_init(&(vad->expand_q));
	wtk_queue_init(&(vad->output_q));
	*output_queue=&(vad->output_q);
	wtk_kvad_reset(vad);
	return vad;
}

wtk_kvad_t* wtk_kvad_new(wtk_kvad_cfg_t *cfg)
{
	wtk_kvad_t *vad;

	vad=(wtk_kvad_t*)wtk_malloc(sizeof(wtk_kvad_t));
	vad->cfg=cfg;
	vad->parm=wtk_kxparm_new(&(cfg->parm));
	if(cfg->use_fixpoint)
	{
		wtk_kxparm_set_notify(vad->parm,vad,(wtk_kxparm_notify_f)wtk_kvad_on_fix_parm);
	}else
	{
		wtk_kxparm_set_notify(vad->parm,vad,(wtk_kxparm_notify_f)wtk_kvad_on_parm);
        }
        if(vad->parm->htk && vad->parm->htk->nnet3)
	{
		qtk_nnet3_set_notify(vad->parm->htk->nnet3,(qtk_nnet3_feature_notify_f)wtk_kvad_on_nnet3,vad);
        } else if (vad->parm->nnet3 && vad->parm->cfg->use_nnet3) {
            qtk_nnet3_set_notify(vad->parm->nnet3,
                                 (qtk_nnet3_feature_notify_f)wtk_kvad_on_nnet3,
                                 vad);
        }

        wtk_hoard_init(&(vad->frame_hoard),offsetof(wtk_vframe_t,hoard_n),cfg->cache
			,(wtk_new_handler_t)wtk_kvad_new_frame,(wtk_delete_handler_t)wtk_kvad_vframe_delete,vad);
	wtk_queue_init(&(vad->input_q));
	wtk_queue_init(&(vad->trap_q));
	vad->cur_frame=NULL;
	wtk_queue_init(&(vad->expand_q));
	wtk_queue_init(&(vad->output_q));
	wtk_kvad_reset(vad);
	return vad;
}

void wtk_kvad_delete(wtk_kvad_t *vad)
{
	wtk_hoard_clean(&(vad->frame_hoard));
	wtk_kxparm_delete(vad->parm);
	wtk_free(vad);
}

void wtk_kvad_start(wtk_kvad_t *vad)
{
	wtk_kxparm_start(vad->parm);
}

void wtk_kvad_clean_queue(wtk_kvad_t *vad,wtk_queue_t *q)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *frame;

	while(1)
	{
		qn=wtk_queue_pop(q);
		if(!qn){break;}
		frame=data_offset2(qn,wtk_vframe_t,q_n);
		wtk_kvad_push_frame(vad,frame);
	}
}

void wtk_kvad_reset(wtk_kvad_t *vad)
{
	if(vad->expand_q.length>0)
	{
		wtk_kvad_clean_queue(vad,&(vad->expand_q));
	}
	if(vad->output_q.length>0)
	{
		wtk_kvad_clean_queue(vad,&(vad->output_q));
	}
	if(vad->cur_frame)
	{
		wtk_kvad_push_frame(vad,vad->cur_frame);
		vad->cur_frame=NULL;
	}
	wtk_queue_init(&(vad->trap_q));
	vad->state=WTK_KVAD_SIL;
	vad->expand_state=WTK_KVAD_SIL;
	vad->pos=0;
	vad->nframe=0;
	wtk_kxparm_reset(vad->parm);
	//wtk_debug("use=%d free=%d\n",vad->frame_hoard.use_length,vad->frame_hoard.cur_free);
}

void wtk_kvad_feed_raise_frame(wtk_kvad_t *vad,wtk_vframe_t *frame)
{
	wtk_queue_push(&(vad->output_q),&(frame->q_n));
}

void wtk_kvad_flush_expand_frame(wtk_kvad_t *vad,wtk_queue_t *q,int speech)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *frame;

	while(1)
	{
		qn=wtk_queue_pop(q);
		if(!qn){break;}
		frame=data_offset2(qn,wtk_vframe_t,q_n);
		if(speech==1)
		{
			frame->state=wtk_vframe_speech;
		}else
		{
			frame->state=wtk_vframe_sil;
		}		
		wtk_kvad_feed_raise_frame(vad,frame);
	}
}

void wtk_kvad_flush_expand(wtk_kvad_t *vad)
{
	//wtk_debug("v[%d]=%s\n",frame->index,frame->speech?"speech":"sil");
	switch(vad->expand_state)
	{
	case WTK_KVAD_SIL:
		if(vad->expand_q.length>0)
		{
			wtk_kvad_flush_expand_frame(vad,&(vad->expand_q),0);
		}
		break;
	case WTK_KVAD_SPEECH:
		wtk_kvad_flush_expand_frame(vad,&(vad->expand_q),1);
		break;
	}
}

void wtk_kvad_feed_expand_frame(wtk_kvad_t *vad,wtk_vframe_t *frame)
{
	wtk_queue_node_t *qn;

	//wtk_debug("v[%d]=%s %p\n",frame->index,frame->speech?"speech":"sil",frame);
	switch(vad->expand_state)
	{
	case WTK_KVAD_SIL:
		if(vad->cfg->left_margin>0)
		{
			if(frame->state==wtk_vframe_speech)
			{
				vad->expand_state=WTK_KVAD_SPEECH;
				if(vad->expand_q.length>0)
				{
					wtk_kvad_flush_expand_frame(vad,&(vad->expand_q),1);
				}
				wtk_kvad_feed_raise_frame(vad,frame);
			}else
			{
				wtk_queue_push(&(vad->expand_q),&(frame->q_n));
				if(vad->expand_q.length>vad->cfg->left_margin)
				{
					qn=wtk_queue_pop(&(vad->expand_q));
					frame=data_offset2(qn,wtk_vframe_t,q_n);
					wtk_kvad_feed_raise_frame(vad,frame);
				}
			}
		}else
		{
			if(frame->state==wtk_vframe_speech)
			{
				vad->expand_state=WTK_KVAD_SPEECH;
			}
			wtk_kvad_feed_raise_frame(vad,frame);
		}
		break;
	case WTK_KVAD_SPEECH:
		if(vad->cfg->right_margin>0)
		{
			if(frame->state==wtk_vframe_speech)
			{
				if(vad->expand_q.length>0)
				{
					wtk_kvad_flush_expand_frame(vad,&(vad->expand_q),1);
				}
				wtk_kvad_feed_raise_frame(vad,frame);
			}else
			{
				wtk_queue_push(&(vad->expand_q),&(frame->q_n));
				if(vad->expand_q.length>vad->cfg->right_margin)
				{
					wtk_kvad_flush_expand_frame(vad,&(vad->expand_q),1);
					vad->expand_state=WTK_KVAD_SIL;
				}
			}
		}else
		{
			if(frame->state==wtk_vframe_sil)
			{
				vad->expand_state=WTK_KVAD_SIL;
			}
			wtk_kvad_feed_raise_frame(vad,frame);
		}
		break;
	}
}

void wtk_kvad_flush_queue(wtk_kvad_t *vad,wtk_queue_t *q,int speech)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *f;
	//wtk_queue_t *q=&(vad->trap_q);

	while(1)
	{
		qn=wtk_queue_pop(q);
		if(!qn){break;}
		f=data_offset2(qn,wtk_vframe_t,q_n);
		if(speech==1)
		{
			f->state=wtk_vframe_speech;
		}else
		{
			f->state=wtk_vframe_sil;
		}
		wtk_kvad_feed_expand_frame(vad,f);
	}
}

int wtk_kvad_check_energy(wtk_kvad_t *vad)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *f;
	float e;
	int i,step=vad->cfg->parm.htk.frame_step;
	short *sv;

	if(vad->cfg->parm.use_knn)
	{
		step=vad->cfg->parm.parm.frame_step;
	}
	e=0;
	for(qn=vad->trap_q.pop;qn;qn=qn->next)
	{
		f=data_offset2(qn,wtk_vframe_t,q_n);
		sv=f->wav_data;
		for(i=0;i<step;++i)
		{
			if(sv[i]>0)
			{
				e+=sv[i];
			}else
			{
				e-=sv[i];
			}
		}
	}
	//wtk_debug("e=%f/%d\n",e/(step*vad->trap_q.length),e>=(vad->trap_q.length*vad->cfg->speech_thresh*step));
	if(e>=(vad->trap_q.length*vad->cfg->speech_thresh*step))
	{
		return 1;
	}else
	{
		return 0;
	}
}

void wtk_kvad_feed_frame(wtk_kvad_t *vad,wtk_vframe_t *frame)
{
//	wtk_debug("v[%d]=%s robin=%d/%d state=%d frame=%p\n",frame->index,frame->speech?"speech":"sil",vad->speech_trap->used,
//			vad->sil_trap->used,vad->state,frame);
	switch(vad->state)
	{
	case WTK_KVAD_SIL:
		if(frame->state==wtk_vframe_speech)
		{
			wtk_queue_push(&(vad->trap_q),&(frame->q_n));
			if(vad->trap_q.length>=vad->cfg->speech_trap)
			{
/*				if(vad->cfg->use_fixpoint)
				{
					if(vad->cfg->fix_speech_thresh>0 && wtk_kvad_check_energy_fix(vad)==0)
					{
						wtk_kvad_flush_queue(vad,&(vad->trap_q),0);
					}else
					{
						wtk_kvad_flush_queue(vad,&(vad->trap_q),1);
						vad->state=WTK_KVAD_SPEECH;
					}
				}else*/
				{
					if(vad->cfg->speech_thresh>0 && wtk_kvad_check_energy(vad)==0)
					{
						wtk_kvad_flush_queue(vad,&(vad->trap_q),0);
					}else
					{
						wtk_kvad_flush_queue(vad,&(vad->trap_q),1);
						vad->state=WTK_KVAD_SPEECH;
					}
				}
			}
		}else
		{
			if(vad->trap_q.length>0)
			{
				wtk_kvad_flush_queue(vad,&(vad->trap_q),0);
			}
			wtk_kvad_feed_expand_frame(vad,frame);
		}
		break;
	case WTK_KVAD_SPEECH:
		if(frame->state==wtk_vframe_speech)
		{
			if(vad->trap_q.length>0)
			{
				wtk_kvad_flush_queue(vad,&(vad->trap_q),1);
			}
			wtk_kvad_feed_expand_frame(vad,frame);
		}else
		{
			wtk_queue_push(&(vad->trap_q),&(frame->q_n));
			if(vad->trap_q.length>=vad->cfg->sil_trap)
			{
				wtk_kvad_flush_queue(vad,&(vad->trap_q),0);
				vad->state=WTK_KVAD_SIL;
			}
		}
		break;
	}
}

void wtk_kvad_flush_frame(wtk_kvad_t *vad)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *frame;

	//wtk_debug("v[%d]=%s\n",frame->index,frame->speech?"speech":"sil");
	while(1)
	{
		qn=wtk_queue_pop(&(vad->input_q));
		if(!qn){break;}
		frame=data_offset2(qn,wtk_vframe_t,q_n);
		frame->state=vad->state==WTK_KVAD_SIL?0:1;
		wtk_kvad_feed_frame(vad,frame);
	}
	switch(vad->state)
	{
	case WTK_KVAD_SIL:
		if(vad->trap_q.length>0)
		{
			wtk_kvad_flush_queue(vad,&(vad->trap_q),0);
		}
		break;
	case WTK_KVAD_SPEECH:
		if(vad->trap_q.length>0)
		{
			wtk_kvad_flush_queue(vad,&(vad->trap_q),1);
		}
		break;
	}
	wtk_kvad_flush_expand(vad);
}

void wtk_kvad_on_nnet3(wtk_kvad_t *vad,qtk_blas_matrix_t *feat,int end, int plus)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *frame;
	float thresh,prob;

	if(!feat)
	{
		return;
	}

	if(vad->input_q.length<=0)
	{
		return;
	}
	qn=wtk_queue_pop(&(vad->input_q));
	frame=data_offset2(qn,wtk_vframe_t,q_n);
	thresh=vad->state==WTK_KVAD_SPEECH?vad->cfg->speech_leave_prob:vad->cfg->speech_enter_prob;

	if(!vad->cfg->use_prob)
	{
        prob = feat->m[1] - feat->m[0];
        if (prob > 6) {
            prob = 6;
        }
        if (prob < -6) {
            prob = -6;
        }
        prob = (prob + 6) / 12;
        //wtk_debug("%f %f\n",prob,thresh);
        frame->state = prob >= thresh ? 1 : 0; //(feat->v[0]-feat->v[1])>0?0:1;
	}else
	{
		frame->state=wtk_fast_exp2(feat->m[1])>=thresh?1:0;//(feat->v[0]-feat->v[1])>0?0:1;
	}

	frame->speechlike = prob;
	frame->raw_state = frame->state;
	if(vad->cfg->use_nnprob){
		wtk_kvad_feed_raise_frame(vad,frame);
	}else{
		wtk_kvad_feed_frame(vad,frame);
	}
	//wtk_debug("v[%d]=%f\n",feat->index,exp(feat->v[1]));

	if(vad->cfg->parm.htk.nnet3.frame_per_chunk ==2)
	{
		if(vad->input_q.length<=0)
		{
			return;
		}
		qn=wtk_queue_pop(&(vad->input_q));
		frame=data_offset2(qn,wtk_vframe_t,q_n);
		thresh=vad->state==WTK_KVAD_SPEECH?vad->cfg->speech_leave_prob:vad->cfg->speech_enter_prob;

		if(!vad->cfg->use_prob)
		{
	        prob = feat->m[1] - feat->m[0];
	        if (prob > 6) {
	            prob = 6;
	        }
	        if (prob < -6) {
	            prob = -6;
	        }
	        prob = (prob + 6) / 12;
	        //wtk_debug("%f %f\n",prob,thresh);
	        frame->state = prob >= thresh ? 1 : 0; //(feat->v[0]-feat->v[1])>0?0:1;
		}else
		{
			frame->state=wtk_fast_exp2(feat->m[1])>=thresh?1:0;//(feat->v[0]-feat->v[1])>0?0:1;
		}
		frame->speechlike = prob;
		frame->raw_state = frame->state;
		if(vad->cfg->use_nnprob){
			wtk_kvad_feed_raise_frame(vad,frame);
		}else{
			wtk_kvad_feed_frame(vad,frame);
		}
	}
}

void wtk_kvad_on_parm(wtk_kvad_t *vad,wtk_kfeat_t *feat)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *frame;
	float thresh;

//	int i;
//	int n=vad->parm->knn->cfg->output_dim;
//
//	for(i=0;i<n;++i)
//	{
//		wtk_debug("v[%d/%d]=%f\n",feat->index,i,exp(feat->v[i]));
//	}
	//wtk_debug("v[%d]=%f/%f %s\n",feat->index,feat->v[0],feat->v[1],(feat->v[0]-feat->v[1])>0?"sil":"speech");
	//wtk_debug("v[%d]=%f/%f %s\n",feat->index,exp(feat->v[0]),exp(feat->v[1]),(feat->v[0]-feat->v[1])>0?"sil":"speech");
	//exit(0);
	if(vad->input_q.length<=0)
	{
		return;
	}
	qn=wtk_queue_pop(&(vad->input_q));
	frame=data_offset2(qn,wtk_vframe_t,q_n);
	thresh=vad->state==WTK_KVAD_SPEECH?vad->cfg->speech_leave_prob:vad->cfg->speech_enter_prob;
	frame->state=wtk_fast_exp2(feat->v[1])>=thresh?1:0;//(feat->v[0]-feat->v[1])>0?0:1;
	frame->raw_state = frame->state;
	//wtk_debug("v[%d]=%f\n",feat->index,exp(feat->v[1]));
//	if(feat->index>100)
//	{
//		exit(0);
//	}
	wtk_kvad_feed_frame(vad,frame);
}

void wtk_kvad_on_fix_parm(wtk_kvad_t *vad,wtk_kfeat_t *feat)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *frame;
	int thresh;
	short *sv;

	if(vad->input_q.length<=0)
	{
		return;
	}
	qn=wtk_queue_pop(&(vad->input_q));
	frame=data_offset2(qn,wtk_vframe_t,q_n);
	thresh=vad->state==WTK_KVAD_SPEECH?vad->cfg->fix_speech_leave_prob:vad->cfg->fix_speech_enter_prob;//speech_enter_prob;
	sv=(short*)(feat->v);
	frame->state=wtk_fixexp_calc(vad->cfg->parm.knn.fixe,sv[1])>=thresh?1:0;
	wtk_kvad_feed_frame(vad,frame);
}

int wtk_kvad_get_left_data(wtk_kvad_t *vad,wtk_string_t *v)
{
	if(vad->cur_frame)
	{
		v->data=(char*)(vad->cur_frame->wav_data);
		v->len=(vad->pos<<1);
	}else
	{
		v->len=0;
	}
	return vad->pos;
}

void wtk_kvad_update_frame(wtk_kvad_t *vad,short *data,int len)
{
    int win;
    wtk_vframe_t *frame;
    if (vad->cfg->parm.use_htk) {
        win = vad->parm->cfg->htk.frame_step;
    } else {
        win = vad->parm->cfg->parm.frame_step;
    }
        short *s,*e;
	int n;
	int pos=vad->pos;

	if(vad->cfg->parm.use_knn)
	{
		win=vad->parm->cfg->parm.frame_step;
	}
	frame=vad->cur_frame;
	if(!frame)
	{
		frame=wtk_kvad_pop_frame(vad);
	}
	s=data;e=s+len;
	while(s<e)
	{
		n=min(e-s,win-pos);
		memcpy(frame->wav_data+pos,s,n*sizeof(short));
		pos+=n;
		if(pos>=win)
		{
			wtk_queue_push(&(vad->input_q),&(frame->q_n));
			frame=wtk_kvad_pop_frame(vad);
			pos=0;
		}
		s+=n;
	}
	vad->cur_frame=frame;
	vad->pos=pos;
}

void wtk_kvad_print_mlf3(wtk_kvad_t *vad, FILE *s) {
    wtk_queue_node_t *n;
    wtk_vframe_t *f;
#define PAD_0 "00000"
    int sil = 1;
    int i = 0;
    int last_index = 0;
    // float dur=vad->cfg->frame_time*1e4;
    wtk_queue_t *q = &(vad->output_q);
    // FILE *s=stdout;

    if (q->length > 0) {
        fprintf(s, "0 ");
    }
    for (n = q->pop; n; n = n->next) {
        f = data_offset(n, wtk_vframe_t, q_n);
        // wtk_debug("v[%d]=%s %p\n",f->index,f->speech?"speech":"sil",f);
        // exit(0);
        if (sil) {
            if (f->state == wtk_vframe_speech) {
                if (f->index == 0 || f->index == 1) {
                    fprintf(s, "0-");
                }
                if (f->index > 1) {
                    fprintf(s, "%.4f-", (f->index - 1) * 0.01);
                    // fprintf(s,"%d-", f->index-1);
                }
                last_index = f->index;
                ++i;
                sil = 0;
            }
        } else {
            if (f->state == wtk_vframe_sil) {
                if (f->index > last_index) {
                    ++i;
                    fprintf(s, "%.4f ", (f->index - 1) * 0.01);
                    // fprintf(s,"%d ", f->index-1);
                }
                last_index = f->index;
                sil = 1;
            } else if (f->state == wtk_vframe_speech && !n->next) {
                fprintf(s, "%.4f ", (f->index - 1) * 0.01);
            }
        }
        if (!n->next) {
            fprintf(s, "%.4f\n", f->index * 0.01);
        }
    }
}

void wtk_kvad_print_mlf2(wtk_kvad_t *vad,FILE *s)
{
	wtk_queue_node_t *n;
	wtk_vframe_t *f;
#define PAD_0 "00000"
	int sil=1;
	int i=0;
	int last_index=0;
	float dur=vad->cfg->parm.htk.window_step;//parm2.parm.frame_step_ms*1e4;
	//float dur=vad->cfg->frame_time*1e4;
	wtk_queue_t *q=&(vad->output_q);
	//FILE *s=stdout;

//	dur=100000;
	if(vad->cfg->parm.use_knn)
	{
		dur=vad->cfg->parm.parm.frame_step_ms*1e4;
	}
	
	if(q->length>0)
	{
		fprintf(s,"0 ");
	}
	for(n=q->pop;n;n=n->next)
	{
		f=data_offset(n,wtk_vframe_t,q_n);
		//wtk_debug("v[%d]=%s %p\n",f->index,f->speech?"speech":"sil",f);
		//exit(0);
		if(sil)
		{
			if(f->state==wtk_vframe_speech)
			{
				if(f->index>1)
				{
					fprintf(s,"%.0f sil\n%.0f ",(f->index-1)*dur,(f->index-1)*dur);
				}
				last_index=f->index;
				++i;
				sil=0;
			}
		}else
		{
			if(f->state==wtk_vframe_sil)
			{
				if(f->index>last_index)
				{
					++i;
					fprintf(s,"%.0f speech\n%.0f ",(f->index-1)*dur,(f->index-1)*dur);
				}
				last_index=f->index;
				sil=1;
			}
		}
		if(!n->next)
		{
			fprintf(s,"%.0f %s\n",f->index*dur,f->state?"speech":"sil");
		}
	}
	fprintf(s,".\n");

}

#endif



void wtk_kvad_feed(wtk_kvad_t *vad,short *data,int len,int is_end)
{
	if(len>0)
	{
		wtk_kvad_update_frame(vad,data,len);
	}
	wtk_kxparm_feed(vad->parm,data,len,is_end);
	if(is_end)
	{
		wtk_kvad_flush_frame(vad);
	}
}

void wtk_kvad_print_mlf(wtk_kvad_t *vad)
{

	wtk_kvad_print_mlf2(vad,stdout);

}


void wtk_kvad_print_prob(wtk_kvad_t *v)
{
	printf("power thresh = %f\n",v->cfg->speech_thresh);
	printf("speech prob = %f\n",v->cfg->speech_enter_prob);
	printf("sil  = %f\n",v->cfg->speech_leave_prob);
}

void wtk_kvad_print_margin(wtk_kvad_t *v)
{
	printf("left_margin = %d\n",v->cfg->left_margin);
	printf("right_margin = %d\n",v->cfg->right_margin);
}

//#include<assert.h>
void wtk_kvad_set_prob(wtk_kvad_t *v,float para, float para1,float para2)
{
	if(para>0)
	{
		v->cfg->speech_thresh =para;
	}
	if(para1>0)
	{
		v->cfg->speech_enter_prob =para1;
	}
	if(para1>0)
	{
		v->cfg->speech_leave_prob =para2;
	}
}

void wtk_kvad_set_margin(wtk_kvad_t *v,int para,int para1)
{
	if(para>0 && para<250)
	{
		v->cfg->left_margin = para;
	}
	if(para1>0 && para1<250)
	{
		v->cfg->right_margin =para1;
	}
}
