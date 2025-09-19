#include "wtk_vdec.h"

wtk_vdec_t* wtk_vdec_new(wtk_vdec_cfg_t *cfg)
{
	wtk_vdec_t *dec;

	dec=(wtk_vdec_t*)wtk_malloc(sizeof(wtk_vdec_t));
	dec->cfg=cfg;
	wtk_queue_init(&(dec->queue));
	dec->parm=wtk_fextra_new(&(cfg->parm));
	dec->parm->output_queue=&(dec->queue);
	dec->rec=wtk_rec_new(&(cfg->rec),cfg->hmmset.hmmset,cfg->dict,cfg->parm.frame_dur);
	dec->glb_heap=wtk_heap_new(4096);
	//dec->net=cfg->lat_set->main;
	if(cfg->use_ebnf)
	{
		dec->net=wtk_lat_dup(cfg->ebnf->lat,dec->glb_heap);
	}else
	{
		dec->net=wtk_lat_dup(cfg->lat_set->main,dec->glb_heap);
	}
	wtk_vdec_reset(dec);
	return dec;
}

void wtk_vdec_delete(wtk_vdec_t *v)
{
	wtk_heap_delete(v->glb_heap);
	wtk_fextra_delete(v->parm);
	wtk_rec_delete(v->rec);
	wtk_free(v);
}

int wtk_vdec_reset(wtk_vdec_t *v)
{
	int ret;

	v->trans=NULL;
	ret=wtk_fextra_reset(v->parm);
	if(ret!=0){goto end;}
	ret=wtk_rec_reset(v->rec);
end:
	return ret;
}

int wtk_vdec_start(wtk_vdec_t *v)
{
	wtk_lat_t *lat;

	lat=v->net;
	return wtk_rec_start(v->rec,lat);
}

int wtk_vdec_feed(wtk_vdec_t *v,int state,char *audio,int audio_bytes)
{
	wtk_rec_t *rec=v->rec;
	wtk_fextra_t *p=v->parm;
	wtk_queue_t *q=p->output_queue;
	wtk_queue_node_t *n;
	wtk_feat_t *f;
	int ret;

	//wtk_debug("%d\n",audio_bytes/2);
	ret=wtk_fextra_feed2(p,audio,audio_bytes,state);
	if(ret!=0){goto end;}
	while(1)
	{
		n=wtk_queue_pop(q);
		if(!n){break;}
		f=data_offset(n,wtk_feat_t,queue_n);

		//wtk_vector_print(f->rv);
		//wtk_debug("rec=%d,use_dnn=%d\n",f->index,v->rec->cfg->use_dnn);
		wtk_rec_feed(rec,f->rv);
		--f->used;
		wtk_fextra_push_feature(p,f);
	}
	if(state==1)
	{
		v->trans=wtk_rec_finish(rec);
		//wtk_transcription_print(v->trans);
		//wtk_transcription_print_hresults(v->trans,stdout);
	}
end:
	return ret;
}

void wtk_vdec_get_rec(wtk_vdec_t *v,wtk_strbuf_t *buf)
{
	wtk_strbuf_reset(buf);
	if(v->trans)
	{
		wtk_transcription_to_string(v->trans,buf);
	}
}

void wtk_vdec_print(wtk_vdec_t *v)
{
	wtk_transcription_print(v->trans);
}



