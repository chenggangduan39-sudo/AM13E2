#include "wtk_mer_cfg_tts_syn.h"

wtk_syn_dtree_t* wtk_mer_syn_dtree_new(wtk_syn_dtree_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool)
{
	int wtk_syn_dtree_load_dur(void **p,wtk_source_t *src);
	wtk_syn_dtree_t *m;
	int ret;
	int i;
	int idx;
	void *p[2];

	m=(wtk_syn_dtree_t*)wtk_malloc(sizeof(wtk_syn_dtree_t));
	m->cfg=cfg;
	m->heap=pool->hash->heap;//wtk_heap_new(4096);
	m->pool=pool;
	for(i=0;i<WTK_SYN_DTREE_TREE_BAPGV+1;++i)
	{
		m->tree[i]=NULL;
		m->qs[i]=NULL;
	}
	p[0]=m;
	p[1]=&idx;
	//wtk_debug("m=%p idx=%d\n",m,idx);
	idx=WTK_SYN_DTREE_TREE_DUR;
	//wtk_debug("%s\n",cfg->dur_fn);
	ret=wtk_source_loader_load(sl,p,(wtk_source_load_handler_t)wtk_syn_dtree_load_dur,cfg->dur_fn);
	if(ret!=0){goto end;}
end:
	return m;
}

wtk_syn_hmm_t* wtk_mer_syn_hmm_new(wtk_syn_hmm_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool,wtk_syn_dtree_t *dtree)
{
/*
只是用时长预测时 替代 wtk_syn_hmm_new 
必须是calloc,不然初始化会出问题
*/
    int wtk_syn_hmm_load_dur(wtk_syn_hmm_t *hmm,wtk_source_t *src);
	int wtk_syn_hmm_init_rbin_dur(wtk_syn_hmm_t *hmm);
    int ret
      , isrbin = sl->hook==NULL?0:1;
	wtk_syn_hmm_t *h = wtk_calloc(1, sizeof(wtk_syn_hmm_t));
	h->cfg=cfg;
	h->pool=pool;
	h->dtree=dtree;
	h->ndurpdf=NULL;
	h->durpdf=NULL;
	h->nstate=0;
	h->lf0stream=0;
	h->rbin=(wtk_rbin2_t*)sl->hook;
	if(!isrbin && !cfg->load_all)
	{
		cfg->load_all=1;
	}
	if (!isrbin || cfg->load_all)
	{/* 非rbin模式.必须load_all */
		ret=wtk_source_loader_load(sl,h,(wtk_source_load_handler_t)wtk_syn_hmm_load_dur,cfg->dur_fn);
        if (ret!=0) {goto end;}
	} else {
		wtk_syn_hmm_init_rbin_dur(h);
	}
    ret=0;
end:
    return h;
}