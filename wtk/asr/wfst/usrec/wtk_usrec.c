#include "wtk_usrec.h"

wtk_usrec_t* wtk_usrec_new(wtk_usrec_cfg_t *cfg)
{
	wtk_usrec_t *usrec;

	usrec=(wtk_usrec_t*)wtk_malloc(sizeof(wtk_usrec_t));
	usrec->cfg=cfg;
	usrec->rec=wtk_wfstr_new(&(cfg->rec),&(cfg->net),cfg->hmmset.hmmset);
	return usrec;
}


void wtk_usrec_delete(wtk_usrec_t *r)
{
	wtk_wfstr_delete(r->rec);
	wtk_free(r);
}

void wtk_usrec_reset(wtk_usrec_t *r)
{
	wtk_fst_net_reset(r->cfg->usr_net);
	wtk_wfstr_reset(r->rec);
}

int wtk_usrec_start(wtk_usrec_t *r)
{
	int ret;

	ret=wtk_wfstr_start(r->rec,r->cfg->usr_net);
	return ret;
}

void wtk_usrec_feed(wtk_usrec_t *u,wtk_feat_t *f)
{
	wtk_wfstr_feed2(u->rec,f);
}

void wtk_usrec_finish(wtk_usrec_t *r)
{
//	wtk_strbuf_t *buf;
//	float f;
//
//	if(1)
//	{
//		wtk_debug("lm=%f\n",r->cfg->net.lmscale);
//		f=wtk_fst_rec_get_conf(r->rec);
//		buf=wtk_strbuf_new(256,1);
//		wtk_fst_rec_finish2(r->rec,buf," ",1);
//		wtk_debug("[%.*s]=%f\n",buf->pos,buf->data,f);
//		wtk_strbuf_delete(buf);
//		exit(0);
//	}
}
