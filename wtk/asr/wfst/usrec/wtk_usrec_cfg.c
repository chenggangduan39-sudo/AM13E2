#include "wtk_usrec_cfg.h" 

int wtk_usrec_cfg_init(wtk_usrec_cfg_t *cfg)
{
	wtk_fst_net_cfg_init(&(cfg->net));
	wtk_wfstrec_cfg_init(&(cfg->rec));
	wtk_hmmset_cfg_init(&(cfg->hmmset));
	cfg->usr_net=NULL;
	return 0;
}

int wtk_usrec_cfg_clean(wtk_usrec_cfg_t *cfg)
{
	if(cfg->usr_net)
	{
		wtk_fst_net_delete(cfg->usr_net);
	}
	wtk_fst_net_cfg_clean(&(cfg->net));
	wtk_wfstrec_cfg_clean(&(cfg->rec));
	wtk_hmmset_cfg_clean(&(cfg->hmmset));
	return 0;
}

int wtk_usrec_cfg_update_local(wtk_usrec_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	int ret;

	lc=wtk_local_cfg_find_lc_s(main,"net");
	if(lc)
	{
		ret=wtk_fst_net_cfg_update_local(&(cfg->net),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"rec");
	if(lc)
	{
		ret=wtk_wfstrec_cfg_update_local(&(cfg->rec),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"hmmset");
	if(lc)
	{
		//wtk_local_cfg_print(lc);
		ret=wtk_hmmset_cfg_update_local(&(cfg->hmmset),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_usrec_cfg_update(wtk_usrec_cfg_t *cfg)
{
	int ret;

	ret=wtk_fst_net_cfg_update(&(cfg->net));
	if(ret!=0){goto end;}
	ret=wtk_wfstrec_cfg_clean(&(cfg->rec));
	if(ret!=0){goto end;}
end:
	return 0;
}

int wtk_usrec_cfg_update2(wtk_usrec_cfg_t *cfg,wtk_source_loader_t *sl,wtk_label_t *label)
{
	int ret;

	ret=wtk_fst_net_cfg_update3(&(cfg->net),label,sl);
	if(ret!=0)
    {
        wtk_debug("update net failed\n");
        goto end;
    }
	ret=wtk_wfstrec_cfg_update2(&(cfg->rec),sl);
	if(ret!=0)
	{
		wtk_debug("update rec failed\n");
		goto end;
	}
	ret=wtk_hmmset_cfg_update2(&(cfg->hmmset),label,sl);
	if(ret!=0)
    {
        wtk_debug("update hmmset failed\n");
        goto end;
    }
//	{
//		wtk_hmm_t **hmm;
//		int i;
//
//		hmm=((wtk_hmm_t**)cfg->hmmset.hmmset->hmm_array->slot);
//		for(i=0;i<cfg->hmmset.hmmset->hmm_array->nslot;++i)
//		{
//			wtk_debug("v[%d]=%p [%.*s]\n",i,hmm[i]->transP,hmm[i]->name->len,hmm[i]->name->data);
//			if(!hmm[i]->transP)
//			{
//				exit(0);
//			}
//		}
//		exit(0);
//	}
	wtk_wfstrec_cfg_update_hmmset(&(cfg->rec),cfg->hmmset.hmmset);
	ret=0;
end:
	return ret;
}
