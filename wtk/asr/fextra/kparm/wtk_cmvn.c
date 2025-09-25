#include "wtk_cmvn.h"

wtk_cmvn_t* wtk_cmvn_new(wtk_cmvn_cfg_t *cfg,int vec_size)
{
	wtk_cmvn_t *cmvn;

	cmvn=(wtk_cmvn_t*)wtk_malloc(sizeof(wtk_cmvn_t));
	cmvn->cfg=cfg;
	cmvn->vec_size=vec_size;
	cmvn->mean=(float*)wtk_calloc(vec_size,sizeof(float));
	wtk_queue_init(&(cmvn->q));
        wtk_queue_init(&(cmvn->post_q));
        wtk_queue_init(&(cmvn->feat_q));
        cmvn->notify=NULL;
	cmvn->notify_ths=NULL;
	//wtk_debug("online=%d\n",cfg->use_online);
	wtk_cmvn_reset(cmvn);
	return cmvn;
}

int wtk_cmvn_bytes(wtk_cmvn_t *cmvn)
{
	int bytes;

	bytes=sizeof(wtk_cmvn_t);
	bytes+=cmvn->vec_size*sizeof(float);
	//wtk_debug("q=%d post=%d\n",cmvn->q.length,cmvn->post_q.length);
	return bytes;
}


void wtk_cmvn_delete(wtk_cmvn_t *cmvn)
{
	wtk_queue_node_t *qn;
	wtk_kfeat_t *feat;

	while(cmvn->post_q.length>0){
		qn=wtk_queue_pop(&(cmvn->post_q));
		if(!qn)break;
		feat = data_offset2(qn, wtk_kfeat_t, q_n);
		wtk_kfeat_delete(feat);
	}
	wtk_free(cmvn->mean);
	wtk_free(cmvn);
}

void wtk_cmvn_set_notify(wtk_cmvn_t *cmvn,void *ths,wtk_kfeat_notify_f notify)
{
	cmvn->notify_ths=ths;
	cmvn->notify=notify;
}

void wtk_cmvn_reset(wtk_cmvn_t *cmvn)
{
	int v=cmvn->vec_size*sizeof(float);

	cmvn->sliding_flag=0;
	//wtk_debug("################ reset ###################\n");
	cmvn->nframe=0;
	cmvn->nvalid=0;
	if(cmvn->cfg->use_save_cmn==0)
	{
		memset(cmvn->mean,0,v);
	}
	//wtk_debug("############## cmvn=%d cmvn=%p save=%d\n",cmvn->inited,cmvn,cmvn->cfg->use_save_cmn);
	if(cmvn->q.length>0 || cmvn->post_q.length>0)
	{
		wtk_debug("cmvn=%d post=%d\n",cmvn->q.length,cmvn->post_q.length);
		exit(0);
	}
}

void wtk_cmvn_flush_feat(wtk_cmvn_t *cmvn,wtk_kfeat_t *feat)
{
	int i;
	int n=cmvn->vec_size;
	float *pf=feat->v;
	float *pf2=cmvn->mean;

	for(i=0;i<n;++i)
	{
		//wtk_debug("v[%d]=%f/%f/%f\n",i,pf2[i],pf[i],pf[i]-pf2[i]);
		pf[i]-=pf2[i];
		// printf("%d %f\n",i,pf[i]);
	}
	// exit(0);
	cmvn->notify(cmvn->notify_ths,feat);
}

void wtk_cmvn_feed_sliding(wtk_cmvn_t *cmvn,wtk_kfeat_t *feat1)
{
	int i;
	int n=cmvn->vec_size;
	float *pmean=cmvn->mean;
	wtk_cmvn_cfg_t *cfg=cmvn->cfg;
	wtk_queue_node_t *qn;
	wtk_kfeat_t *feat,*feat2;
	float *pf;
	// static int first=0;

	++cmvn->nframe;

	pf=feat1->v;
	for(i=0;i<n;++i)
	{
		pmean[i]+=pf[i];
	}

	wtk_queue_push(&(cmvn->q),&(feat1->q_n));

	feat2=wtk_kfeat_new(n);
	memcpy(feat2->v,feat1->v,n*sizeof(float));
	wtk_queue_push(&(cmvn->post_q),&(feat2->q_n));
        if (cmvn->post_q.length > cfg->left_frame + 1) {
            cmvn->nframe = cmvn->nframe - 1;
            qn = wtk_queue_pop(&(cmvn->post_q));
            if (!qn) {
                return;
            }
            feat = data_offset2(qn, wtk_kfeat_t, q_n);
            pf = feat->v;
            for (i = 0; i < n; ++i) {
                pmean[i] = pmean[i] - pf[i];
            }
            wtk_kfeat_delete(feat);
        }

        if(cmvn->sliding_flag==0)
	{
		if(cmvn->q.length==cfg->right_frame)
		{
			while(cmvn->q.length>=0)
			{
				qn=wtk_queue_pop(&(cmvn->q));
				if(!qn){break;}
				feat=data_offset2(qn,wtk_kfeat_t,q_n);
				
				for(i=0;i<n;++i)
				{
					//wtk_debug("v[%d]=%f/%f/%f\n",i,pf2[i],pf[i],pf[i]-pf2[i]);
					feat->v[i]-=(pmean[i]/cfg->right_frame);
					
					// printf("%d %f\n",i,feat->v[i]);
					// printf("%d %f\n",i,pmean[i]);
				}
				// exit(0);
				//print_float(feat->v,40);
                                if (cmvn->cfg->use_array) {
                                    wtk_queue_push(&(cmvn->feat_q),
                                                   &(feat->feat_n));
                                } else {
                                    cmvn->notify(cmvn->notify_ths, feat);
                                }
                        }
			cmvn->sliding_flag=1;
		}
	}else
	{
		qn=wtk_queue_pop(&(cmvn->q));
		if(!qn){return;}
		feat=data_offset2(qn,wtk_kfeat_t,q_n);
		
		for(i=0;i<n;++i)
		{
			//wtk_debug("v[%d]=%f/%f/%f\n",i,pf2[i],pf[i],pf[i]-pf2[i]);
			// printf("%d %f\n",i,feat->v[i]);
			//feat->v[i]-=(pmean[i]/cfg->right_frame);
			feat->v[i]-=(pmean[i]/cmvn->nframe);
			// printf("%d %f\n",i,feat->v[i]);
		}
		//print_float(feat->v,40);
                if (cmvn->cfg->use_array) {
                    wtk_queue_push(&(cmvn->feat_q), &(feat->feat_n));
                } else {
                    cmvn->notify(cmvn->notify_ths, feat);
                }
        }
                //wtk_debug("cmvn->nframe %d\n",cmvn->nframe);
}

void wtk_cmvn_feed2(wtk_cmvn_t *cmvn,wtk_kfeat_t *feat)
{
	int i;
	int n=cmvn->vec_size;
	float *pmean=cmvn->mean;
	float *pf=feat->v;
	wtk_cmvn_cfg_t *cfg=cmvn->cfg;
	int nf;
	float f;
	wtk_queue_node_t *qn;

	++cmvn->nframe;
	//wtk_debug("v[%d]=%f\n",cmvn->nframe,pf[0]);
	if(cfg->use_online)
	{
		if(cfg->use_hist)
		{
			if(1)
			{
//				/wtk_debug("push %d/%d\n",cmvn->nframe,cmvn->q.length);
				if(cmvn->nframe<=cmvn->cfg->init_frame)
				{
					if(cmvn->nframe==cmvn->cfg->init_frame)
					{
						f=1.0/cmvn->nframe;
						for(i=0;i<n;++i)
						{
							pmean[i]=(pmean[i]+pf[i])*f;
						}
					}else
					{
						for(i=0;i<n;++i)
						{
							pmean[i]+=pf[i];
						}
						wtk_queue_push(&(cmvn->q),&(feat->q_n));
						return;
					}
				}else
				{
					if(cmvn->cfg->alpha>0 && cmvn->nframe>cmvn->cfg->alpha_frame)
					{
						float f1;
						float f2;

						f1=cmvn->cfg->alpha;
						f2=1-f1;
						for(i=0;i<n;++i)
						{
							//wtk_debug("v[%d]=%f/%f %f\n",i,pmean[i],pf[i],pmean[i]*f1+pf[i]*f2);
							pmean[i]=pmean[i]*f1+pf[i]*f2;
						}
					}else
					{
						nf=cmvn->nframe-1;
						f=1.0/(nf+1);
						for(i=0;i<n;++i)
						{
							pmean[i]=(pmean[i]*nf+pf[i])*f;
							//wtk_debug("v[%d/%d]=%f\n",cmvn->nframe,i,pmean[i]);
						}
					}
				}
			}else
			{
				nf=cmvn->nframe-1;
				f=1.0/(nf+1);
				for(i=0;i<n;++i)
				{
					pmean[i]=(pmean[i]*nf+pf[i])*f;
				}
			}
			wtk_queue_push(&(cmvn->q),&(feat->q_n));
			while(cmvn->q.length>cfg->right_frame)
			{
				qn=wtk_queue_pop(&(cmvn->q));
				if(!qn){break;}
				feat=data_offset2(qn,wtk_kfeat_t,q_n);
				wtk_cmvn_flush_feat(cmvn,feat);
			}
		}else
		{
			nf=cmvn->post_q.length;
			f=1.0/(nf+1);
			for(i=0;i<n;++i)
			{
				pmean[i]=(pmean[i]*nf+pf[i])*f;
			}
			{
				wtk_kfeat_t* feat2;

				feat2=wtk_kfeat_new(n);
				memcpy(feat2->v,feat->v,n*sizeof(float));
				wtk_queue_push(&(cmvn->post_q),&(feat2->q_n));
			}
			wtk_queue_push(&(cmvn->q),&(feat->q_n));
			// wtk_debug("cmvn=%d\n",cmvn->q.length);
			
			while(cmvn->q.length>cfg->right_frame)
			{
				qn=wtk_queue_pop(&(cmvn->q));
				if(!qn){break;}
				feat=data_offset2(qn,wtk_kfeat_t,q_n);
				wtk_cmvn_flush_feat(cmvn,feat);
			}
			while(cmvn->post_q.length>cfg->left_frame)
			{
				qn=wtk_queue_pop(&(cmvn->post_q));
				if(!qn){break;}
				feat=data_offset2(qn,wtk_kfeat_t,q_n);
				nf=cmvn->post_q.length+1;
				f=1.0/(nf-1);
				pf=feat->v;
				for(i=0;i<n;++i)
				{
					pmean[i]=(pmean[i]*nf-pf[i])*f;
				}
				wtk_kfeat_delete(feat);
			}
			//exit(0);
		}
	}else
	{
		for(i=0;i<n;++i)
		{
			pmean[i]+=pf[i];
		}
		wtk_queue_push(&(cmvn->q),&(feat->q_n));
	}

}

void wtk_cmvn_feed(wtk_cmvn_t *cmvn,wtk_kfeat_t *feat)
{
	if(cmvn->cfg->use_sliding)
	{
		wtk_cmvn_feed_sliding(cmvn,feat);
	}else
	{	//wtk_debug("1111111111111\n");
		wtk_cmvn_feed2(cmvn,feat);
	}
}

void wtk_cmvn_feed_fix(wtk_cmvn_t *cmvn,wtk_kfeat_t *feat)
{
	wtk_cmvn_cfg_t *cfg=cmvn->cfg;
	wtk_queue_node_t *qn;
	int i;
	int *iv;
	int *mean=(int*)(cmvn->mean);
	int n=cmvn->vec_size;
	int n1,n2;
	int valid;
	int mf=500;

	++cmvn->nframe;
	iv=(int*)(feat->v);
	//print_int(iv,n);
	//wtk_debug("========= feat[%d] nvalid=%d init=%d q=%d =============\n",feat->index,cmvn->nvalid,cfg->init_frame,cmvn->q.length);
	//wtk_fix_print_float(iv,n);
	//wtk_debug("==================== update[%d]  ======================\n",cmvn->nframe);
	wtk_queue_push(&(cmvn->q),&(feat->q_n));
	if(cmvn->nvalid<cfg->init_frame)
	{
		if(cmvn->q.length<=1)
		{
			valid=0;
			for(i=1;i<n;++i)
			{
				if(iv[i]>mf || iv[i]<-mf)
				{
					valid=1;
					break;
				}
			}
			if(valid==0)
			{
				wtk_queue_pop(&(cmvn->q));
				cmvn->notify(cmvn->notify_ths,feat);
				return;
			}
		}
		++cmvn->nvalid;
		//wtk_debug("==============> nframe=%d/%d/%d\n",cmvn->nframe,cmvn->q.length,cmvn->nvalid);
		for(i=0;i<n;++i)
		{
			mean[i]+=iv[i];
		}
		if(cmvn->q.length==cfg->init_frame)
		{
			//wtk_debug("==================== update ======================\n");
			n1=cmvn->q.length;
			for(i=0;i<n;++i)
			{
				mean[i]/=n1;
				//wtk_debug("v[%d]=%d/%f\n",i,mean[i],FIX2FLOAT(mean[i]));
			}
		}else
		{
			return;
		}
	}else if(1)
	{
		int f1;
		int f2;

		if(cmvn->cfg->fix_alpha>0 && cmvn->nframe>cmvn->cfg->alpha_frame)
		{
			f1=cfg->fix_alpha;
			f2=100-f1;
			for(i=0;i<n;++i)
			{
				//wtk_debug("v[%d]=%f/%f %f\n",i,pmean[i],pf[i],pmean[i]*f1+pf[i]*f2);
				mean[i]=(mean[i]*f1+iv[i]*f2)/100;
			}
		}else
		{
			n1=cmvn->nframe;
			n2=n1-1;
			for(i=0;i<n;++i)
			{
				mean[i]=(mean[i]*n2+iv[i])/n1;
			}
		}
	}else
	{
		n1=cmvn->nframe;
		n2=n1-1;
		for(i=0;i<n;++i)
		{
			mean[i]=(mean[i]*n2+iv[i])/n1;
		}
	}
//	{
//		static int ki=0;
//
//		++ki;
//		wtk_debug("v[%d]=%d/%d/%d feat=%d\n",ki,mean[0],mean[1],mean[2],feat->index);
//	}
	while(cmvn->q.length>cfg->right_frame)
	{
		qn=wtk_queue_pop(&(cmvn->q));
		if(!qn){break;}
		feat=data_offset2(qn,wtk_kfeat_t,q_n);
		iv=(int*)(feat->v);
		for(i=0;i<n;++i)
		{
			iv[i]-=mean[i];
		}
		cmvn->notify(cmvn->notify_ths,feat);
	}
}

void wtk_cmvn_flush_fix(wtk_cmvn_t *cmvn)
{
	wtk_queue_node_t *qn;
	wtk_kfeat_t *feat;
	int *mean=(int*)(cmvn->mean);
	int *iv;
	int i,n=cmvn->vec_size;
	int n1;

	if(cmvn->nframe<cmvn->cfg->init_frame && cmvn->nframe>0)
	{
		n1=cmvn->nframe;
		for(i=0;i<n;++i)
		{
			mean[i]/=n1;
		}
	}
	//wtk_cmvn_print(cmvn);
	while(1)
	{
		qn=wtk_queue_pop(&(cmvn->q));
		if(!qn){break;}
		feat=data_offset2(qn,wtk_kfeat_t,q_n);
		iv=(int*)(feat->v);
		for(i=0;i<n;++i)
		{
			iv[i]-=mean[i];
		}
		cmvn->notify(cmvn->notify_ths,feat);
	}
	//exit(0);
} 

void wtk_cmvn_update_sliding(wtk_cmvn_t *cmvn)
{
	int i;
	int vec_size=cmvn->vec_size;
	float f;
	float *pf1;

	if(cmvn->nframe>cmvn->cfg->right_frame)
	{
		f=1.0/cmvn->cfg->right_frame;
	}else
	{
		f=1.0/cmvn->nframe;
	}
	pf1=cmvn->mean;
	for(i=0;i<vec_size;++i)
	{
		pf1[i]*=f;
		//wtk_debug("v[%d]=%f\n",i,pf1[i]);
	}
	//exit(0);
}

void wtk_cmvn_update(wtk_cmvn_t *cmvn)
{
	int i;
	int vec_size=cmvn->vec_size;
	float f;
	float *pf1;

	f=1.0/cmvn->nframe;
	pf1=cmvn->mean;
	for(i=0;i<vec_size;++i)
	{
		pf1[i]*=f;
		//wtk_debug("v[%d]=%f\n",i,pf1[i]);
	}
	//exit(0);
}

void wtk_cmvn_flush_sliding(wtk_cmvn_t *cmvn)
{
	wtk_queue_node_t *qn;
	wtk_kfeat_t *feat;

	if(cmvn->nframe>0)
	{
		wtk_cmvn_update_sliding(cmvn);
	}

	if(cmvn->post_q.length>0)
	{
		while(1)
		{
			qn=wtk_queue_pop(&(cmvn->post_q));
			if(!qn){break;}
			feat=data_offset2(qn,wtk_kfeat_t,q_n);
			wtk_kfeat_delete(feat);
		}
        }
        while(1)
	{
		qn=wtk_queue_pop(&(cmvn->q));
		if(!qn){break;}
		feat=data_offset2(qn,wtk_kfeat_t,q_n);
		wtk_cmvn_flush_feat(cmvn,feat);
	}
}

void wtk_cmvn_flush2(wtk_cmvn_t *cmvn)
{
	wtk_queue_node_t *qn;
	wtk_kfeat_t *feat;

	//wtk_debug("q=%d/%d\n",cmvn->q.length,cmvn->nframe);
	if(!cmvn->cfg->use_online)
	{
		if(cmvn->nframe>0)
		{
			wtk_cmvn_update(cmvn);
		}
	}else
	{
		if(cmvn->nframe>0 && cmvn->nframe<=cmvn->cfg->init_frame)
		{
			wtk_cmvn_update(cmvn);
		}
		if(cmvn->post_q.length>0)
		{
			while(1)
			{
				qn=wtk_queue_pop(&(cmvn->post_q));
				if(!qn){break;}
				feat=data_offset2(qn,wtk_kfeat_t,q_n);
				wtk_kfeat_delete(feat);
			}
		}
	}
	//wtk_cmvn_print(cmvn);
	while(1)
	{
		qn=wtk_queue_pop(&(cmvn->q));
		if(!qn){break;}
		feat=data_offset2(qn,wtk_kfeat_t,q_n);
		wtk_cmvn_flush_feat(cmvn,feat);
	}
	// exit(0);
}


void wtk_cmvn_flush(wtk_cmvn_t *cmvn)
{
	if(cmvn->cfg->use_sliding)
	{
		wtk_cmvn_flush_sliding(cmvn);
	}else
	{
		wtk_cmvn_flush2(cmvn);
	}
}

void wtk_cmvn_print(wtk_cmvn_t *cmvn)
{
	int i;

	for(i=0;i<cmvn->vec_size;++i)
	{
		wtk_debug("v[%d]=%f\n",i,cmvn->mean[i]);
	}
}

