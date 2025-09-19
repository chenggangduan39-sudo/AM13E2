#include "wtk_fnnvad_post.h"
#include "wtk_fnnvad.h"

int wtk_fnnvad_post_bytes(wtk_fnnvad_post_t *post)
{
	int bytes;

	bytes=sizeof(wtk_fnnvad_post_t);
	bytes+=wtk_robin_bytes(post->sil_robin);
	bytes+=wtk_robin_bytes(post->speech_robin);
	return bytes;
}

wtk_fnnvad_post_t* wtk_fnnvad_post_new(wtk_fnnvad_t *vad)
{
	wtk_fnnvad_post_t *p;

	p=(wtk_fnnvad_post_t*)wtk_malloc(sizeof(*p));
	p->vad=vad;
	p->sil_robin=wtk_robin_new(vad->cfg->siltrap);
	p->speech_engine_thresh=0;
	p->speech_robin=wtk_robin_new(vad->cfg->speechtrap);
	p->echo_state=wtk_fnnvad_ECHO_INIT;
	wtk_fnnvad_post_reset(p);
	return p;
}

void  wtk_fnnvad_post_delete(wtk_fnnvad_post_t *p)
{
	wtk_robin_delete(p->sil_robin);
	wtk_robin_delete(p->speech_robin);
	wtk_free(p);
}

void  wtk_fnnvad_post_reset(wtk_fnnvad_post_t *p)
{
	p->force_sil=0;
	p->speech_state=wtk_fnnvad_SPEECH_INIT;
	p->speech_sil_frames=0;
	p->echo_state=wtk_fnnvad_ECHO_INIT;
	p->speech_high_frame=0;
	p->state=WTK_FNNVAD_SIL;
	wtk_robin_reset(p->sil_robin);
	//p->speech_cnt=0;
	p->speech_engine_thresh=0;
	p->speech_engine_thresh_low=0;
	wtk_robin_reset(p->speech_robin);
	//p->speech_engine_thresh=223925024.000000;
	//42962748.000000.
}

void wtk_fnnvad_post_flush_feature_robin(wtk_fnnvad_post_t *v,wtk_robin_t *r,int is_sil)
{
	wtk_fnnvad_feat_t *f;

	while(r->used>0)
	{
		f=wtk_robin_pop(r);
		if(!f){break;}
		--f->ref;
		wtk_fnnvad_raise_feat(v->vad,f,is_sil);
	}
}

//#define DEUBG_LOG
#ifdef DEUBG_LOG
#include "wtk/os/wtk_log.h"
#endif

/**
 * return is sil or not
 */
int wtk_fnnvad_post_end_detect(wtk_fnnvad_post_t *post,wtk_fnnvad_feat_t *f)
{
	int is_sil;
	float e;

	is_sil=f->is_sil;
	switch(post->speech_state)
	{
	case wtk_fnnvad_SPEECH_INIT:
		post->delete_e+=f->frame->energy;
		if(f->frame->energy>=post->detect_high_start_thresh)
		{
			++post->detect_high_frames;
			//wtk_debug("get high=%d\n",post->detect_high_frames);
			if(post->detect_high_frames>=post->vad->cfg->detect_min_high_frame)
			{
				post->detect_low_start_thresh=post->delete_e*post->vad->cfg->detect_high_go_low_thresh_rate/post->detect_high_frames;
				post->speech_state=wtk_fnnvad_SPEECH_HIGH;
				post->detect_low_frames=0;
				post->detect_high_frames=0;
				post->delete_e=0;
				//wtk_debug("detect =++++++++++++++++++++++++++++>  low=%f\n",post->detect_low_start_thresh);
			}
		}else
		{
			post->delete_e=0;
			post->detect_high_frames=0;
		}
		break;
	case wtk_fnnvad_SPEECH_HIGH:
		if(f->frame->energy<=post->detect_low_start_thresh)
		{
			++post->detect_low_frames;
			post->detect_high_frames=0;
			//wtk_debug("get low=%d %d=%f\n",post->detect_low_frames,f->frame->index,f->frame->energy);
			if(post->detect_low_frames>=post->vad->cfg->detect_min_low_frame)
			{
				post->speech_state=wtk_fnnvad_SPEECH_LOW;
				is_sil=1;
			}
		}else
		{
			post->detect_low_frames=0;

			post->delete_e+=f->frame->energy;
			if(f->frame->energy>=post->detect_high_start_thresh)
			{
				++post->detect_high_frames;
				//wtk_debug("get high=%d\n",post->detect_high_frames);
				if(post->detect_high_frames>=post->vad->cfg->detect_min_high_frame)
				{
					e=post->delete_e*post->vad->cfg->detect_high_go_low_thresh_rate/post->detect_high_frames;
					if(e>post->detect_low_start_thresh)
					{
						post->detect_low_start_thresh=e;
						//post->speech_state=wtk_fnnvad_SPEECH_HIGH;
						//wtk_debug("detect =++++++++++++++++++++++++++++>  low=%f\n",post->detect_low_start_thresh);
					}
					post->detect_low_frames=0;
					post->detect_high_frames=0;
					post->delete_e=0;
				}
			}else
			{
				post->delete_e=0;
				post->detect_high_frames=0;
			}
		}
		break;
	case wtk_fnnvad_SPEECH_LOW:
		is_sil=1;
		//wtk_debug("return sil=%f\n",f->frame->energy);
		break;
	}
	return is_sil;
}

void wtk_fnnvad_init_end_detect(wtk_fnnvad_post_t *post)
{
	wtk_robin_t *rbin=post->sil_robin;
	wtk_fnnvad_feat_t *f;
	int i;
	float e;
	int n;

	//n=min(rbin->used/2,5);
	n=rbin->used;
	e=0;
	for(i=0;i<n;++i)
	{
		f=(wtk_fnnvad_feat_t*)wtk_robin_at(rbin,i);
		e+=f->frame->energy;
		//wtk_debug("v[%d]=%f\n",f->frame->index,f->frame->energy);
	}
	if(e==0)
	{
		wtk_debug("vad end detect error.\n");
		e=100*100;
	}else
	{
		e/=n;
	}
	post->speech_state=wtk_fnnvad_SPEECH_INIT;
	post->detect_high_frames=post->detect_low_frames=0;
	post->detect_high_start_thresh=e*post->vad->cfg->detect_sil_go_high_thresh_rate;
	post->delete_e=0;
	post->force_sil=0;
	///wtk_debug("=================> detect high=%f\n",post->detect_high_start_thresh);
	for(i=0;i<rbin->used;++i)
	{
		f=(wtk_fnnvad_feat_t*)wtk_robin_at(rbin,i);
		wtk_fnnvad_post_end_detect(post,f);
	}
}

void wtk_fnnvad_post_feed(wtk_fnnvad_post_t *v,wtk_fnnvad_feat_t *f)
{
	int is_sil;
	float t;

	if(v->vad->cfg->use_speech_end_detect || v->speech_engine_thresh>0)// && f->frame->energy==0)
	{
		wtk_vframe_calc_energy2(f->frame);
	}
	is_sil=f->frame->state==wtk_vframe_sil;
#ifdef DEBUG_SIL_ENERGE
	//if(is_sil==0)
	{
		t=wtk_audio_sample_energy(f->frame->wav_data,f->frame->frame_step);
		wtk_debug("v[%d]=%f sil=%f rate=%f\n",f->frame->index,t,v->speech_engine_thresh_low,t/v->speech_engine_thresh_low);
	}
#endif
	//wtk_debug("v[%d]=%s\n",f->frame->index,f->is_sil?"sil":"speech");
	//wtk_debug("state=%d speech=%d sil=%d is_sil=%d ki=%d\n",v->state,v->speech_robin->used,v->sil_robin->used,is_sil,ki);
	switch(v->state)
	{
	case WTK_FNNVAD_SIL:
		//wtk_debug("f0[%d]=%f/%f is_sil=%d %f/%f\n",f->frame->index,f->f0,f->fe,is_sil,v->vad->cfg->min_f0,v->vad->cfg->max_f0);
#ifdef DEUBG_LOG
		{

			t=wtk_audio_sample_energy(f->frame->wav_data,f->frame->frame_size);
			wtk_log_log(glb_log,"v[%d]=%f/%f sil=%d",f->frame->index,t,v->speech_engine_thresh,is_sil);
			//wtk_log_log(glb_log,"f0[%d]=%f/%f is_sil=%d %f/%f\n",f->frame->index,f->f0,f->fe,is_sil,v->vad->cfg->min_f0,v->vad->cfg->max_f0);
		}
#endif
		//wtk_debug("f0[%d]=%f/%f is_sil=%d\n",f->frame->index,f->f0,f->fe,is_sil);
		//wtk_debug("is_sil=%d\n",is_sil);
		//wtk_debug("%p/%f/%d energy=%f/%d\n",v,v->speech_engine_thresh,f->frame->index,f->frame->energy,f->frame->frame_size);
		if(v->speech_engine_thresh>0)
		{
			if(!is_sil && f->frame->energy<v->speech_engine_thresh)
			{
				is_sil=1;
			}
		}
		if(is_sil)
		{
			if(v->sil_robin->used>0)
			{
				wtk_fnnvad_post_flush_feature_robin(v,v->sil_robin,1);
			}
			wtk_fnnvad_raise_feat(v->vad,f,1);
		}else
		{
			++f->ref;
			wtk_robin_push(v->sil_robin,f);
			//wtk_debug("sil=%d/%d\n",v->sil_robin->used,v->vad->cfg->siltrap);
			if(v->sil_robin->used>=v->vad->cfg->siltrap || ((v->speech_high_frame>=v->vad->cfg->high_speech_min_frame)))
			{
				//wtk_debug("goto speech\n");
				if(v->vad->cfg->use_speech_end_detect)
				{
					wtk_fnnvad_init_end_detect(v);
				}
				wtk_fnnvad_post_flush_feature_robin(v,v->sil_robin,0);
				v->speech_high_frame=0;
				v->state=WTK_FNNVAD_SPEECH;
			}
		}
		break;
	case WTK_FNNVAD_SPEECH:
		if(v->vad->cfg->use_speech_end_detect)
		{
			is_sil=wtk_fnnvad_post_end_detect(v,f);
			if(is_sil)
			{
				v->force_sil=1;
			}
		}
		if(!is_sil)
		{
			if(v->speech_engine_thresh_low>0 && v->speech_engine_thresh>0)
			{
				t=f->frame->energy;
				if(t<v->speech_engine_thresh_low)
				{
					is_sil=1;
					//wtk_debug("frame=%d v=%f/%f robin=%d\n",f->frame->index,t,v->speech_engine_thresh_low,v->speech_robin->used);
				}
			}
		}
		if(is_sil)
		{
			++f->ref;
			wtk_robin_push(v->speech_robin,f);
			if((v->speech_robin->used>=v->vad->cfg->speechtrap))
			{
				wtk_fnnvad_post_flush_feature_robin(v,v->speech_robin,1);
				v->state=WTK_FNNVAD_SIL;
				v->echo_state=wtk_fnnvad_ECHO_INIT;
			}
		}else
		{
			//t=wtk_audio_sample_energy(f->frame->wav_data,f->frame->frame_step);
			if(v->speech_robin->used>0)
			{
				wtk_fnnvad_post_flush_feature_robin(v,v->speech_robin,0);
			}
			wtk_fnnvad_raise_feat(v->vad,f,0);
		}
		break;
	}
}

void wtk_fnnvad_flush_end(wtk_fnnvad_post_t *v)
{
	//wtk_debug("sil=%d\n",v->sil_robin->used);
	//wtk_debug("speech=%d\n",v->speech_robin->used);
	switch(v->state)
	{
	case WTK_FNNVAD_SIL:
		if(v->sil_robin->used>0)
		{
			wtk_fnnvad_post_flush_feature_robin(v,v->sil_robin,1);
		}
		break;
	case WTK_FNNVAD_SPEECH:
		if(v->speech_robin->used>0)
		{
			wtk_fnnvad_post_flush_feature_robin(v,v->speech_robin,0);
		}
		break;
	}
}
