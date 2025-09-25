#include "wtk_vrec.h" 

wtk_vrec_t* wtk_vrec_new(wtk_vrec_cfg_t *cfg,wtk_hmmset_t *hmmset,float frame_dur)
{
	wtk_vrec_t *rec;

	rec=(wtk_vrec_t*)wtk_malloc(sizeof(wtk_vrec_t));
	rec->cfg=cfg;
	rec->rec=wtk_rec_new(&(cfg->rec),hmmset,cfg->dict,frame_dur);
	rec->glb_heap=wtk_heap_new(4096);
	if(cfg->use_ebnf)
	{
		rec->net=wtk_lat_dup(cfg->ebnf->lat,rec->glb_heap);
	}else
	{
		rec->net=wtk_lat_dup(cfg->latset->main,rec->glb_heap);
	}
	return rec;
}

void wtk_vrec_delete(wtk_vrec_t *r)
{
	wtk_rec_delete(r->rec);
	wtk_heap_delete(r->glb_heap);
	wtk_free(r);
}

void wtk_vrec_start(wtk_vrec_t *r)
{
	wtk_lat_t *lat;

	lat=r->net;
	wtk_rec_start(r->rec,lat);
}

void wtk_vrec_reset(wtk_vrec_t *r)
{
	wtk_rec_reset(r->rec);
}


void wtk_vrec_feed(wtk_vrec_t *r,wtk_vector_t *obs)
{
	wtk_rec_feed(r->rec,obs);
}
