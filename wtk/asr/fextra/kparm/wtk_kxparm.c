#include "wtk_kxparm.h" 
#include "wtk/asr/fextra/kcmn/wtk_kcmn.h"
void wtk_kxparm_on_knn(wtk_kxparm_t *parm,wtk_kfeat_t *feat);
void wtk_kxparm_on_htk(wtk_kxparm_t *parm,wtk_feat_t *feat);
void wtk_kxparm_on_cmvn_parm(wtk_kxparm_t *parm,wtk_kfeat_t *feat);
void wtk_kxparm_on_kind_parm(wtk_kxparm_t *parm,wtk_kfeat_t *feat);
void wtk_kxparm_on_parm(wtk_kxparm_t *parm,wtk_kfeat_t *feat);

int wtk_kxparm_bytes(wtk_kxparm_t *parm)
{
	int bytes;

	bytes=sizeof(wtk_kxparm_t);
	if(parm->htk)
	{
		bytes+=wtk_fextra_bytes(parm->htk);
	}
	if(parm->parm)
	{
		bytes+=wtk_kparm_bytes(parm->parm);
	}
	//wtk_debug("bytes00=%.2f hoad=%d/%d\n",bytes*1.0/1024,parm->parm->hoard.use_length,parm->parm->hoard.cur_free);
	if(parm->knn)
	{
		bytes+=wtk_knn_bytes(parm->knn);
	}
	//wtk_debug("bytes01=%.2f\n",bytes*1.0/1024);
	return bytes;
}

wtk_kxparm_t* wtk_kxparm_new(wtk_kxparm_cfg_t *cfg)
{
	wtk_kxparm_t *parm;

	parm=(wtk_kxparm_t*)wtk_malloc(sizeof(wtk_kxparm_t));
	parm->cfg=cfg;
	parm->htk=NULL;
	parm->parm=NULL;
	if(cfg->use_htk)
	{
		parm->htk=wtk_fextra_new(&(cfg->htk));
		wtk_fextra_set_notify(parm->htk,(wtk_fextra_feature_notify_f)wtk_kxparm_on_htk,parm);
	}else
	{
		parm->parm=wtk_kparm_new(&(cfg->parm));
		wtk_kparm_set_notify(parm->parm,parm,(wtk_kfeat_notify_f)wtk_kxparm_on_parm);
//		if(cfg->parm->use_ivector)
//		{
//			wtk_kparm_set_ivector_notify(parm->parm,parm,(wtk_ivector_notify_f)wtk_kxparm_on_parm_ivector);
//		}
	}
	if(cfg->use_knn)
	{
		parm->knn=wtk_knn_new(&(cfg->knn));
		wtk_knn_set_notify(parm->knn,parm,(wtk_knn_notify_f)wtk_kxparm_on_knn);
	}else
	{
		parm->knn=NULL;
	}
	if(cfg->parm.use_kind_notify)
	{
		wtk_kparm_set_cmvn_raise(parm->parm,parm,(wtk_kfeat_notify_f)wtk_kxparm_on_cmvn_parm);
		wtk_kparm_set_kind_raise(parm->parm,parm,(wtk_kfeat_notify_f)wtk_kxparm_on_kind_parm);
		parm->vad_callback=NULL;
	}

	if(cfg->use_nnet3)
	{
		//TODO nnet3 needs feature cols, calculate here
		//wtk_debug("%d\n",cfg->feature_cols);
		int feature_cols;
		if(cfg->use_htk)
		{
			feature_cols = parm->htk->cfg->feature_cols;
		}else
		{
			switch(parm->parm->cfg->kfind.bkind)
			{
			case WTK_FBANK:
				feature_cols = parm->parm->cfg->melbank.num_bins;
				break;
			case WTK_MFCC:
				feature_cols = parm->parm->cfg->NUMCEPS;
				break;
			case WTK_PLP:
			default:
				wtk_debug("error type %d, do not support\n",parm->parm->cfg->kfind.bkind);
				exit(0);
				break;
			}
		}
		//if(parm->cfg->parm.use_ivector)
		//{
		//	feature_cols += parm->parm->ivector_dim;
		//}
		parm->nnet3=qtk_nnet3_new(&(cfg->nnet3),feature_cols,parm->cfg->parm.use_ivector);
		if(cfg->use_htk && parm->htk->cfg->DELTA)
		{
			parm->nnet3->delta=1;
		}
		if(parm->cfg->parm.use_ivector)
		{
			parm->nnet3->use_ivector=1;
			parm->nnet3->ivector_dim = parm->parm->ivector_dim;
			parm->nnet3->ivector = parm->parm->ivector_feat;
		}
	}else
	{
		parm->nnet3=NULL;
	}

	parm->notify=NULL;
	parm->notify2=NULL;
	parm->notify_end=NULL;
	parm->ths=NULL;

	return parm;
}

void wtk_kxparm_delete(wtk_kxparm_t *parm)
{
	if(parm->htk)
	{
		wtk_fextra_delete(parm->htk);
	}
	if(parm->parm)
	{
		wtk_kparm_delete(parm->parm);
	}
	if(parm->knn)
	{
		wtk_knn_delete(parm->knn);
	}
	if(parm->nnet3)
	{
    	qtk_nnet3_delete(parm->nnet3);
	}
	wtk_free(parm);
}

void wtk_kxparm_set_notify(wtk_kxparm_t *parm,void *ths,wtk_kxparm_notify_f notify)
{
	parm->ths=ths;
	parm->notify=notify;
}

void wtk_kxparm_set_notify2(wtk_kxparm_t *parm,void *ths,wtk_kxparm_notify_f2 notify)
{
	//parm->ths=ths;
	parm->notify2=notify;
}

void wtk_kxparm_set_notify_end(wtk_kxparm_t *parm,void *ths,wtk_kxparm_notify_end_f notify)
{
	parm->notify_end=notify;
}

void wtk_kxparm_set_vad_callback(wtk_kxparm_t *parm,void *ths,wtk_kxparm_callback_f notify)
{
	parm->vad_ths=ths;
	parm->vad_callback=notify;
}

void wtk_kxparm_set_kind_notify(wtk_kxparm_t *parm,void *ths,wtk_kxparm_notify_f notify)
{
	parm->kind_ths=ths;
	parm->kind_notify=notify;
}

void wtk_kxparm_start(wtk_kxparm_t *parm)
{
	if(parm->htk)
	{

	}else
	{
		wtk_kparm_start(parm->parm);
	}
}

void wtk_kxparm_reset(wtk_kxparm_t *parm)
{
	wtk_kxparm_reset2(parm,1);
}

void wtk_kxparm_reset2(wtk_kxparm_t *parm,int reset_cmn)
{
	if(parm->knn)
	{
		wtk_knn_reset(parm->knn);
	}
	if(parm->htk)
	{
		wtk_fextra_reset(parm->htk);
	}else
	{
		wtk_kparm_reset(parm->parm);
	}
	if(parm->nnet3)
	{
		qtk_nnet3_reset(parm->nnet3);
	}
}

void wtk_kxparm_on_knn(wtk_kxparm_t *parm,wtk_kfeat_t *feat)
{
	//wtk_debug("index=%d\n",feat->index);
	//exit(0);
	parm->notify(parm->ths,feat);
}

void wtk_kxparm_on_htk(wtk_kxparm_t *parm,wtk_feat_t *feat)
{
	//wtk_debug("index=%d\n",feat->index);
	if(parm->knn)
	{
		wtk_knn_fead(parm->knn,feat->rv+1,feat->index);
	}else
	{
		if(parm->notify2)
		{
			parm->notify2(parm->ths,feat);
		}
	}
	--feat->used;
	//wtk_debug("used=%d use=%d free=%d\n",feat->used,parm->htk->feature_hoard.use_length,parm->htk->feature_hoard.cur_free);
	wtk_fextra_push_feature(parm->htk,feat);
}

//void wtk_kxparm_on_parm_ivector(wtk_kxparm_t *parm,float *feat,int dim)
//{
//
//}

void wtk_kxparm_on_cmvn_parm(wtk_kxparm_t *parm,wtk_kfeat_t *feat)
{
	char state;

	if(parm->vad_callback)
	{
		state=parm->vad_callback(parm->vad_ths,feat);
	}else
	{
		state=1;
	}
	
	if(state)
	{
		wtk_knn_fead(parm->knn,feat->v,feat->index);
	}

	--feat->used;
	wtk_kparm_push_feat(parm->parm,feat);
}

void wtk_kxparm_on_kind_parm(wtk_kxparm_t *parm,wtk_kfeat_t *feat)
{
	// printf("%d %p\n",__LINE__,parm);
	parm->kind_notify(parm->kind_ths,feat);
}

// int iii=0;
void wtk_kxparm_on_parm(wtk_kxparm_t *parm,wtk_kfeat_t *feat)
{
//	if(parm->cfg->parm.use_fixpoint)
//	{
//		wtk_fix_print((int*)(feat->v),wtk_kparm_cfg_feature_size(parm->parm->cfg),12);
//	}else
//	{
//		print_float(feat->v,wtk_kparm_cfg_feature_size(parm->parm->cfg));
//	}
	if(parm->knn)
	{
		if(parm->cfg->parm.use_fixpoint)
		{
			wtk_knn_fead_fix(parm->knn,(int*)(feat->v),feat->index);
		}else
		{
			wtk_knn_fead(parm->knn,feat->v,feat->index);
		}
	}else if(parm->nnet3)
	{
		qtk_nnet3_run_kfeat(parm->nnet3,feat,0);
	}else
	{
		parm->notify(parm->ths,feat);
	}
	--feat->used;
	wtk_kparm_push_feat(parm->parm,feat);
}

void wtk_kxparm_feed(wtk_kxparm_t *parm,short *data,int len,int is_end)
{
	if(parm->htk)
	{
		wtk_fextra_feed(parm->htk,data,len,is_end);
	}else
	{
		wtk_kparm_feed(parm->parm,data,len,is_end);
	}

	if(is_end && parm->notify_end)
	{
		parm->notify_end(parm->ths);
	}

	if(is_end && parm->knn)
	{
		if(parm->knn->pooling_layer_index!=-1)
		{
			if(parm->parm->cfg->use_kind_notify)
			{
				return;
			}
			wtk_knn_flush_pooling(parm->knn);
		}else
		{
			wtk_knn_flush(parm->knn);
		}
	}

	if(is_end && parm->nnet3)
	{
		qtk_nnet3_run_kfeat(parm->nnet3,NULL,1);
	}
}

void wtk_kxparm_feed_feat2(wtk_kxparm_t *parm,wtk_kfeat_t *feat,int is_end)
{
	if(feat)
	{
		if(parm->parm->cmvn)
		{
			wtk_cmvn_feed(parm->parm->cmvn,feat);
		}

		if(parm->parm->kcmvn)
		{
			wtk_kcmn_feed(parm->parm->kcmvn,feat);
		}
	}

	if(is_end)
	{
		if(parm->parm->cmvn)
		{
			wtk_cmvn_flush(parm->parm->cmvn);
		}

		if(parm->knn && parm->knn->pooling_layer_index!=-1)
		{
			wtk_knn_flush_pooling(parm->knn);
		}

		if(parm->notify_end)
		{
			//wtk_debug("notfiy endxxxxxxxxxxxxxx\n");
			parm->notify_end(parm->ths);
		}
	}
}


void wtk_kxparm_feed_float(wtk_kxparm_t *parm,float *data,int len,int is_end)
{
	if(parm->htk)
	{
		wtk_fextra_feed_float(parm->htk,data,len,is_end);
	}else
	{
		wtk_kparm_feed_float(parm->parm,data,len,is_end);
	}
	if(is_end && parm->knn)
	{
		wtk_knn_flush(parm->knn);
	}
}


#if 0
void wtk_kxparm_feed_feat3(wtk_kxparm_t *parm,wtk_kfeat_t *feat,int is_speech,int is_end)
{
	if(feat)
	{
		if(is_speech)
		{
			wtk_kxparm_on_parm(parm,feat);
		}else
		{
			--feat->used;
			wtk_kparm_push_feat(parm->parm,feat);
		}
	}

	if(is_end)
	{
		if(parm->knn->pooling_layer_index!=-1)
		{
			wtk_knn_flush_pooling(parm->knn);
		}
	}
}

void wtk_kxparm_feed_feat(wtk_kxparm_t *parm,wtk_kfeat_t *feat,int is_speech,int is_end)
{
	if(feat)
	{
		if(parm->parm->cmvn)
		{
			wtk_cmvn_feed(parm->parm->cmvn,feat);
		}
	}

	if(is_end)
	{
		if(parm->parm->cmvn)
		{
			wtk_cmvn_flush(parm->parm->cmvn);
		}
	}
}

void wtk_kxparm_feed_feat2(wtk_kxparm_t *parm,wtk_kfeat_t *feat,int is_speech,int is_end)
{
	if(feat)
	{
		if(is_speech)
		{
			if(parm->parm->cmvn)
			{
				wtk_cmvn_feed(parm->parm->cmvn,feat);
			}else
			{
				wtk_kxparm_on_parm(parm,feat);
			}
		}else
		{
			--feat->used;
			wtk_kparm_push_feat(parm->parm,feat);
		}
	}

	if(is_end)
	{
		if(parm->parm->cmvn)
		{
			wtk_cmvn_flush(parm->parm->cmvn);
		}

		if(parm->knn->pooling_layer_index!=-1)
		{
			wtk_knn_flush_pooling(parm->knn);
		}
	}
}

#endif
