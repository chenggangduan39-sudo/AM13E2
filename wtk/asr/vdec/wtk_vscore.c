#include "wtk_vscore.h" 

wtk_vscore_t* wtk_vscore_new(wtk_vscore_cfg_t *cfg)
{
	wtk_vscore_t *s;

	s=(wtk_vscore_t*)wtk_malloc(sizeof(wtk_vscore_t));
	s->cfg=cfg;
	wtk_queue_init(&(s->queue));
	s->parm=wtk_fextra_new(&(cfg->parm));
	s->parm->output_queue=&(s->queue);
	s->fa=wtk_vrec_new(&(cfg->fa),cfg->hmmset.hmmset,cfg->parm.frame_dur);
	s->loop=wtk_vrec_new(&(cfg->loop),cfg->hmmset.hmmset,cfg->parm.frame_dur);
	s->score=NULL;
	return s;
}

void wtk_vscore_delete(wtk_vscore_t *s)
{
	wtk_fextra_delete(s->parm);
	wtk_vrec_delete(s->fa);
	wtk_vrec_delete(s->loop);
	wtk_free(s);
}

void wtk_vscore_start(wtk_vscore_t *s)
{
	wtk_vrec_start(s->fa);
	wtk_vrec_start(s->loop);
}

void wtk_vscore_reset(wtk_vscore_t *s)
{
	s->score=NULL;
	wtk_vrec_reset(s->fa);
	wtk_vrec_reset(s->loop);
	wtk_fextra_reset(s->parm);
}

void wtk_vscore_feed(wtk_vscore_t *s,char *data,int len,int is_end)
{
	wtk_fextra_t *p=s->parm;
	wtk_queue_t *q=p->output_queue;
	wtk_queue_node_t *n;
	wtk_feat_t *f;

	//wtk_debug("%d\n",audio_bytes/2);
	wtk_fextra_feed2(p,data,len,is_end);
	while(1)
	{
		n=wtk_queue_pop(q);
		if(!n){break;}
		f=data_offset(n,wtk_feat_t,queue_n);
		//wtk_vector_print(f->rv);
		//wtk_debug("rec=%d,use_dnn=%d\n",f->index,v->rec->cfg->use_dnn);
		wtk_vrec_feed(s->fa,f->rv);
		wtk_vrec_feed(s->loop,f->rv);
		--f->used;
		wtk_fextra_push_feature(p,f);
	}
	if(is_end)
	{
		wtk_transcription_t *trans1,*trans2;

		trans1=wtk_rec_finish(s->fa->rec);
		//wtk_transcription_print(trans1);
		trans2=wtk_rec_finish(s->loop->rec);
		//wtk_transcription_print(trans2);
		s->score=wtk_fa_new_h(s->fa->rec->local_heap,trans1,trans2,s->cfg->parm.frame_dur);
		//wtk_fa_print(s->score);
	}
}
