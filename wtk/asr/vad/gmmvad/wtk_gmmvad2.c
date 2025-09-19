#include "wtk_gmmvad2.h" 

wtk_vframe_t* wtk_gmmvad2_new_vframe(wtk_gmmvad2_t *vad)
{
	wtk_gmmvad_cfg_t *cfg=vad->vad->cfg;

	return wtk_vframe_new2(cfg->frame_size,cfg->frame_size);
}

wtk_gmmvad2_t* wtk_gmmvad2_new(wtk_gmmvad_cfg_t *cfg,void *raise_ths,wtk_vframe_raise_f raise)
{
	wtk_gmmvad2_t *vad;
	int cache=20;

	vad=(wtk_gmmvad2_t*)wtk_malloc(sizeof(wtk_gmmvad2_t));
	vad->vad=wtk_gmmvad_new(cfg);
	vad->raise_ths=raise_ths;
	vad->raise=raise;
	vad->frame_buffer=wtk_short_buffer_new(cfg->frame_size*cache);
	wtk_hoard_init(&vad->frame_hoard,offsetof(wtk_vframe_t,hoard_n),cache,
			(wtk_new_handler_t)wtk_gmmvad2_new_vframe,(wtk_delete_handler_t)wtk_vframe_delete,vad);
	wtk_gmmvad2_reset(vad);
	return vad;
}

void wtk_gmmvad2_delete(wtk_gmmvad2_t *v)
{
	wtk_short_buffer_delete(v->frame_buffer);
	wtk_hoard_clean(&(v->frame_hoard));
	wtk_gmmvad_delete(v->vad);
	wtk_free(v);
}

void wtk_gmmvad2_reset(wtk_gmmvad2_t *v)
{
	wtk_gmmvad_reset(v->vad);
	wtk_short_buffer_reset(v->frame_buffer);
	v->n_frame_index=0;
}

void wtk_gmmvad2_push_vframe(wtk_gmmvad2_t *vad,wtk_vframe_t *f)
{
	wtk_hoard_push(&(vad->frame_hoard),f);
}

wtk_vframe_t* wtk_gmmvad2_pop_vframe(wtk_gmmvad2_t *vad)
{
	wtk_vframe_t *f;

	f=wtk_hoard_pop(&(vad->frame_hoard));
	wtk_frame_reset(f);
	f->index=++vad->n_frame_index;
	return f;
}


void wtk_gmmvad2_feed(wtk_gmmvad2_t *vad,char *data,int bytes,int is_end)
{
	wtk_gmmvad_cfg_t *cfg=vad->vad->cfg;
	wtk_short_buffer_t *b=vad->frame_buffer;
	wtk_vframe_t *f;
	char *end=data+bytes;
	int ret;

	while(data<end)
	{
		data+=wtk_short_buffer_push_c(b,data,end-data);
		while(wtk_short_buffer_used_samples(b)>=cfg->frame_size)
		{
			f=wtk_gmmvad2_pop_vframe(vad);
			wtk_short_buffer_peek(b,f);
			ret=wtk_gmmvad_process(vad->vad,f->wav_data,f->frame_size);
			//wtk_debug("ret=%d\n",ret);
			f->state=f->raw_state=ret==0?wtk_vframe_sil:wtk_vframe_speech;
			//wtk_debug("v[%d]=%d\n",vad->n_frame_index,ret);
			vad->raise(vad->raise_ths,f);
			wtk_short_buffer_skip(b,cfg->frame_size,cfg->frame_size);
		}
	}
}


void wtk_gmmvad2_flush_frame_queue(wtk_gmmvad2_t *v,wtk_queue_t *q)
{
	wtk_queue_node_t *n;
	wtk_vframe_t *f;

	while(1)
	{
		n=wtk_queue_pop(q);
		if(!n){break;}
		f=data_offset(n,wtk_vframe_t,q_n);
		wtk_gmmvad2_push_vframe(v,f);
	}
}

