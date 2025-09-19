#include "wtk_knn.h" 
void wtk_knn_fead_frame_fix_short(wtk_knn_t *knn,int index,int is_end);
void wtk_knn_fead_frame_fix_char(wtk_knn_t *knn,int index,int is_end);
wtk_vecs2_t* wtk_knn_feed_layer_input_fix_short0(wtk_knn_t *knn,wtk_knn_rte_layer_t *layer,wtk_veci_t *input,int is_end);
void wtk_knn_static_extraction(wtk_knn_t *knn,wtk_knn_rte_layer_t *layer,wtk_vecf_t *input);
void wtk_knn_static_pooling(wtk_knn_t *knn,wtk_knn_rte_layer_t *layer,wtk_vecf_t *input,int is_end);

static wtk_knn_feat_t* wtk_knn_feat_new(int len,int use_fix)
{
	wtk_knn_feat_t *feat;

	feat=(wtk_knn_feat_t*)wtk_malloc(sizeof(wtk_knn_feat_t));
	feat->index=0;
	if(use_fix)
	{
		feat->v.i=(int*)wtk_calloc(len,sizeof(int));
	}else
	{
		feat->v.f=(float*)wtk_calloc(len,sizeof(float));
	}
	return feat;
}

static void wtk_knn_feat_delete(wtk_knn_feat_t *feat)
{
	wtk_free(feat->v.f);
	wtk_free(feat);
}

static wtk_knn_rte_layer_t* wtk_knn_rte_layer_new(void)
{
	wtk_knn_rte_layer_t *layer;

	layer=(wtk_knn_rte_layer_t*)wtk_malloc(sizeof(wtk_knn_rte_layer_t));
	layer->input=NULL;
	layer->output=NULL;
	layer->input_i=NULL;
	layer->layer=NULL;
	layer->tdnn_robin=NULL;
	layer->input_idx=0;

	layer->input_s=NULL;
	layer->output_s=NULL;
	return layer;
}

static int wtk_knn_rte_layer_bytes(wtk_knn_rte_layer_t *l)
{
	int bytes;

	bytes=sizeof(wtk_knn_rte_layer_t);
//	wtk_debug("layer[%.*s] float=%p/%p int=%p/%p short=%p/%p\n",l->layer->name->len,l->layer->name->data,
//			l->input,l->output,l->input_i,l->output_i,l->input_s,l->output_s);
	if(l->input)
	{
		bytes+=wtk_vecf_bytes(l->input);
	}
	if(l->output)
	{
		bytes+=wtk_vecf_bytes(l->output);
	}
	if(l->input_i)
	{
		bytes+=wtk_veci_bytes(l->input_i);
	}
	if(l->input_s)
	{
		bytes+=wtk_vecs2_bytes(l->input_s);
	}
	if(l->output_s)
	{
		bytes+=wtk_vecs2_bytes(l->output_s);
	}
	if(l->tdnn_robin)
	{
		if(l->output_s)// || l->output_i)
		{
			bytes+=l->output_s->len*sizeof(short)*l->tdnn_robin->nslot;
		}else
		{
			bytes+=l->output->len*sizeof(float)*l->tdnn_robin->nslot;
		}
	}
	return bytes;
}

static int wtk_knn_rte_bytes(wtk_knn_rte_t *rte)
{
	int bytes;
	int i;

	bytes=sizeof(wtk_knn_rte_t);
	bytes+=rte->nlayer*sizeof(wtk_knn_rte_layer_t*);
	for(i=0;i<rte->nlayer;++i)
	{
		bytes+=wtk_knn_rte_layer_bytes(rte->layers[i]);
	}
	return bytes;
}

static wtk_knn_rte_t* wtk_knn_new_rte(wtk_knn_t *knn)
{
	wtk_knn_rte_t *rte;
	wtk_knn_cfg_t *cfg=knn->cfg;
	int i,dim,j;
	wtk_knn_rte_layer_t *layer;

	knn->pooling_layer_index=-1;
	//wtk_debug("input=%d\n",knn->cfg->logic_layer[0]->n_input);
	rte=(wtk_knn_rte_t*)wtk_malloc(sizeof(wtk_knn_rte_t));
	//wtk_debug("input=%d\n",rte->input->len);
	// rte->nlayer=cfg->nlogic_layer-1;
	if(knn->cfg->use_round)
		rte->nlayer=cfg->rt_nlogic_layer;
	else
		rte->nlayer=cfg->nlogic_layer-1;

	rte->layers=(wtk_knn_rte_layer_t**)wtk_calloc(rte->nlayer,sizeof(wtk_knn_rte_layer_t*));
	for(i=0;i<rte->nlayer;++i) {
		layer=rte->layers[i]=wtk_knn_rte_layer_new();
		layer->layer=cfg->logic_layer[i];
		// wtk_debug("[%.*s],sum=%d\n",layer->layer->name->len,layer->layer->name->data,layer->layer->layer_input->sum);
		if(cfg->logic_layer[i]->n_input>1) {
			dim=0;
			if(cfg->logic_layer[i]->layer_input->sum==0)
			{
				for(j=0;j<cfg->logic_layer[i]->n_input;++j) {
					// wtk_debug("i=%d,j=%d,%d\n",i,j,cfg->logic_layer[i]->layer_input[j].layer);
					if(cfg->logic_layer[i]->layer_input[j].layer>=0) {
						if(cfg->use_fixpoint) {
							dim+=rte->layers[(int)(cfg->logic_layer[i]->layer_input[j].layer)]->output_s->len;
						}else {
							dim+=rte->layers[(int)(cfg->logic_layer[i]->layer_input[j].layer)]->output->len;
						}
					}else {
						if(cfg->logic_layer[i]->layer_input[j].replace>0)
						{
							dim+=cfg->ivector_dim;
						}else
						{
							dim+=cfg->input_dim;
						}
						
					}
				}
			}else
			{
				dim = rte->layers[(int)(cfg->logic_layer[i]->layer_input[1].layer)]->output->len;
				// wtk_debug("[%.*s],dim=%d\n",layer->layer->name->len,layer->layer->name->data,dim);
			}
			if(cfg->use_fixpoint) {
				if(i>0) {
					rte->layers[i]->input_s=wtk_vecs2_new(dim);
				}else {
					rte->layers[i]->input_i=wtk_veci_new(dim);
				}
			} else {
				rte->layers[i]->input=wtk_vecf_new(dim);
			}
		}
		// wtk_debug("%.*s dim=%d\n",cfg->logic_layer[i]->layer->name->len,cfg->logic_layer[i]->layer->name->data,dim);
		dim=wtk_knn_layer_get_output(cfg->logic_layer[i]->layer);
		if(dim<=0) 
		{
			dim=0;
			if(cfg->logic_layer[i]->layer_input->sum==0)
			{
				for(j=0;j<cfg->logic_layer[i]->n_input;++j) 
				{
					if(cfg->use_fixpoint) 
					{
						dim+=rte->layers[(int)(cfg->logic_layer[i]->layer_input[j].layer)]->output_s->len;
					}else 
					{
						dim+=rte->layers[(int)(cfg->logic_layer[i]->layer_input[j].layer)]->output->len;			
					}
				}
			}else
			{
				dim=rte->layers[(int)(cfg->logic_layer[i]->layer_input[1].layer)]->output->len;
			}
			
		}
		if(cfg->use_fixpoint) {
			layer->output_s=wtk_vecs2_new(dim);
		}else {
			layer->output=wtk_vecf_new(dim);
		}
		if(layer->layer->use_tdnn_output) {
			// wtk_debug("[%.*s] [%d,%d] len=%d\n",layer->layer->name->len,layer->layer->name->data,layer->layer->tdnn_max_left,layer->layer->tdnn_max_right,layer->output->len);
			layer->tdnn_robin=wtk_robin_new(layer->layer->tdnn_max_left+layer->layer->tdnn_max_right+1+1);//layer->layer->tdnn_max_left+layer->layer->tdnn_max_right+1
			for(j=0;j<layer->tdnn_robin->nslot;++j) {
                layer->tdnn_robin->r[j] = wtk_knn_feat_new(cfg->use_fixpoint ? layer->output_s->len: layer->output->len, cfg->use_fixpoint);
			}
		}
		if(layer->layer->layer->type==WTK_NormalizeComponent) {
			layer->layer->layer->v.normalize->rms2=1.0/((layer->input?layer->input->len:rte->layers[(int)(cfg->logic_layer[i]->layer_input[0].layer)]->output->len)*layer->layer->layer->v.normalize->rms*layer->layer->layer->v.normalize->rms);
		}

		if(layer->layer->layer->type==WTK_StatisticsPoolingComponent)
		{
			knn->pooling_layer_index=i;
		}
	}
	// exit(0);
	return rte;
}

static void wtk_knn_rte_layer_reset(wtk_knn_rte_layer_t  *layer)
{
	layer->skip_idx=-1;
	layer->skip=0;
	layer->input_idx=0;
	if(layer->tdnn_robin)
	{
		wtk_robin_reset(layer->tdnn_robin);
	}
}

static void wtk_knn_rte_layer_delete(wtk_knn_rte_layer_t  *layer)
{
	if(layer->tdnn_robin)
	{
		int i;

		for(i=0;i<layer->tdnn_robin->nslot;++i)
		{
			wtk_knn_feat_delete((wtk_knn_feat_t*)layer->tdnn_robin->r[i]);
			//wtk_free(layer->tdnn_robin->r[i]);
		}
		wtk_robin_delete(layer->tdnn_robin);
	}
	if(layer->input)
	{
		wtk_vecf_delete(layer->input);
	}
	if(layer->input_s)
	{
		wtk_vecs2_delete(layer->input_s);
	}
	if(layer->output)
	{
		wtk_vecf_delete(layer->output);
	}
	if(layer->input_i)
	{
		wtk_veci_delete(layer->input_i);
	}
	if(layer->output_s)
	{
		wtk_vecs2_delete(layer->output_s);
	}
	wtk_free(layer);
}

static void wtk_knn_rte_delete(wtk_knn_rte_t *rte)
{
	int i;

	if(rte->layers)
	{
		for(i=0;i<rte->nlayer;++i)
		{
			wtk_knn_rte_layer_delete(rte->layers[i]);
		}
		wtk_free(rte->layers);
	}
	wtk_free(rte);
}

static void wtk_knn_rte_reset(wtk_knn_rte_t *rte)
{
	int i;

	for(i=0;i<rte->nlayer;++i)
	{
		wtk_knn_rte_layer_reset(rte->layers[i]);
	}
}

int wtk_knn_bytes(wtk_knn_t *knn)
{
	int bytes;

	bytes=sizeof(wtk_knn_t);
	bytes+=wtk_robin_bytes(knn->rb);
	bytes+=wtk_knn_rte_bytes(knn->rte);
	bytes+=knn->rb->nslot*knn->cfg->input_dim*sizeof(float);
	return bytes;
}

wtk_knn_t* wtk_knn_new(wtk_knn_cfg_t *cfg)
{
	wtk_knn_t *knn;
	int i;

	//exit(0);
	knn=(wtk_knn_t*)wtk_malloc(sizeof(wtk_knn_t));
	knn->cfg=cfg;
	knn->rb=wtk_robin_new(cfg->tdnn_max_left+cfg->tdnn_max_right+1+1);
	// wtk_debug("%d\n",knn->rb->nslot);
	//exit(0);
	for(i=0;i<knn->rb->nslot;++i)
	{
		knn->rb->r[i]=wtk_knn_feat_new(cfg->input_dim,cfg->use_fixpoint);//(float*)wtk_calloc(cfg->input_dim,sizeof(float));
	}
	knn->rte=wtk_knn_new_rte(knn);
	knn->name=NULL;
	knn->notify=NULL;
	knn->ths=NULL;
	wtk_knn_reset(knn);
	return knn;
}

void wtk_knn_delete(wtk_knn_t *knn)
{
	int i;

	for(i=0;i<knn->rb->nslot;++i)
	{
		wtk_knn_feat_delete((wtk_knn_feat_t*)knn->rb->r[i]);
		//wtk_free(knn->rb->r[i]);
	}
	wtk_knn_rte_delete(knn->rte);
	wtk_robin_delete(knn->rb);
	wtk_free(knn);
}

void wtk_knn_set_notify(wtk_knn_t *knn,void *ths,wtk_knn_notify_f notify)
{
	knn->ths=ths;
	knn->notify=notify;
}

void wtk_knn_reset(wtk_knn_t *knn)
{
	knn->use_skip=(knn->cfg->skip>0 || knn->cfg->use_fast_skip)?1:0;
	if(knn->use_skip)
	{
		if(knn->cfg->use_fast_skip)
		{
			knn->skip=1;
		}else
		{
			knn->skip=knn->cfg->skip;
		}
	}else
	{
		knn->skip=0;
	}
	knn->nframe=0;
	knn->oframe=0;
	knn->flag=0;
	knn->pooling_nframe=0;
	//knn->skip_layer=NULL;
	//memset(knn->input->p,0,knn->input->len*sizeof(float));
	wtk_robin_reset(knn->rb);
	wtk_knn_rte_reset(knn->rte);
}


static void wtk_knn_add_log(wtk_knn_t *knn,wtk_vecf_t *input)
{
	float *pf;
	float *pi=input->p;
	int i,len;

	//wtk_debug("prior=%d\n",knn->cfg->prior->len);
	if(knn->cfg->use_prior)
	{
		len=input->len;
		pf=knn->cfg->prior->p;
		for(i=0;i<len;++i)
		{
			//wtk_debug("v[%d]=%f/%f\n",i,pi[i],pf[i]);
			pi[i]-=pf[i];
		}
		return;
	}
	if(knn->cfg->use_exp)
	{
		len=input->len;
		if(knn->cfg->use_fast_exp)
		{
			for(i=0;i<len;++i)
			{
				pi[i]=wtk_fast_exp2(pi[i]);
			}
		}else
		{
		 	for(i=0;i<len;++i)
		    	{
		        		pi[i]=wtk_exp(pi[i]);
		    	}
		}
		return;
		}
		//exit(0);
}


#if 0
static float wtk_fnn_calc_prob(wtk_knn_t *knn,wtk_knn_layer_t *layer,wtk_vecf_t *input,int id)
{
    //wtk_debug("type=%d id=%d\n",layer->type,id);
    switch(layer->type)
    {
        case WTK_AffineComponent:
            return wtk_knn_affine_calc2(layer->v.affine,input,id);
            break;
        case WTK_FixedAffineComponent:
            return wtk_knn_fixed_affine_calc2(layer->v.fixed_affine,input,id);
            break;
        case WTK_NaturalGradientAffineComponent:
            exit(0);
            break;
        case WTK_RectifiedLinearComponent:
            exit(0);
            break;
        case WTK_BatchNormComponent:
            exit(0);
            break;
        case WTK_LogSoftmaxComponent:
            exit(0);
            break;
        case WTK_NormalizeComponent:
            exit(0);
            break;
        default:
            exit(0);
            break;
    }
    exit(0);
    return 0;
}
#endif


float wtk_knn_get_prob(wtk_knn_t *knn,int id)
{
    return knn->input->p[id];
}


static void wtk_knn_rte_layer_update_tdnn_output(wtk_knn_rte_layer_t *layer,wtk_vecf_t *output,int index,int valid)
{
    wtk_knn_feat_t *ft;
    //float *pf;

    ft=(wtk_knn_feat_t*)wtk_robin_next(layer->tdnn_robin);
    ft->index=index;
    ft->valid=valid;
    //	if(knn->cfg->use_fixpoint)
    //	{
    //		memcpy(ft->v.i,output->p,output->len*sizeof(int));
    //	}else
    {
        memcpy(ft->v.f,output->p,output->len*sizeof(float));
    }
    //exit(0);
}

static wtk_knn_feat_t* wtk_knn_rte_layer_get_tdnn_output(wtk_knn_t *knn,wtk_knn_rte_layer_t *layer,int offset,int min_offset, int input_idx)
{
    wtk_robin_t *rb=layer->tdnn_robin;
    wtk_knn_feat_t *ft;
    int idx;

    idx=input_idx%rb->used+offset-min_offset;
    if (idx >= rb->used) {
        idx = idx - rb->used;
    }
/*	 wtk_debug("layer=%.*s,idx=%d,input_idx=%d,rb: used=%d,nslot=%d,offset=%d,min_offset=%d\n",layer->layer->name->len,layer->layer->name->data,
	idx,input_idx,rb->used,rb->nslot,offset,min_offset);
	int i;
	if(wtk_string_cmp(layer->layer->name,"tdnn4l",layer->layer->name->len)==0)
	{
		for(i=0;i<rb->nslot;++i)
		{
			ft = (wtk_knn_feat_t *)(rb->r[i]);
			print_float(ft->v.f,160);
			wtk_debug("====================\n");
		}
	}*/

    ft = (wtk_knn_feat_t *)(rb->r[idx]);
	return ft;//->pf;
}

static wtk_knn_feat_t* wtk_knn_rte_layer_get_tdnn_output2(wtk_knn_t *knn,wtk_knn_rte_layer_t *layer)
{
	wtk_robin_t *rb=layer->tdnn_robin;
	wtk_knn_feat_t *ft;
	int idx;
	// int i;
	idx=1;
	// wtk_debug("input_idx=%d idx=%d used=%d,off=%d,min_off=%d\n",input_idx,idx,rb->used,offset,min_offset);
	
	// for(i=0;i<rb->nslot;++i)
	// {
	// 	ft=(wtk_knn_feat_t*)wtk_robin_at(rb,i);
	// 	print_float(ft->v.f,10);
	// }
	ft=(wtk_knn_feat_t*)wtk_robin_at(rb,idx);
	return ft;
}

static wtk_knn_feat_t* wtk_knn_rte_layer_get_input(wtk_knn_t *knn,wtk_knn_rte_layer_t *layer,int offset,int is_end)
{
	wtk_robin_t *rb=knn->rb;
	wtk_knn_feat_t *ft;
	int idx;

    idx=offset+(layer->input_idx%rb->used)-layer->layer->layer_input[0].offset;
    if (idx >= rb->used) {
        idx = idx - rb->used;
    }
	// wtk_debug("idx=%d,input_idx=%d,rb: used=%d,nslot=%d,offset=%d\n",idx,layer->input_idx,rb->used,rb->nslot,offset);
    ft = (wtk_knn_feat_t *)(rb->r[idx]);
	return ft;//->pf;
}

wtk_knn_feat_t* wtk_knn_rte_layer_get_input2(wtk_knn_t *knn,wtk_knn_rte_layer_t *layer,int index,int is_end)
{
	wtk_robin_t *rb=knn->rb;
	wtk_knn_feat_t *ft;
	int idx;

	//wtk_debug("[%.*s] cur=%d get=%d\n",layer->layer->name->len,layer->layer->name->data,layer->input_idx,index);
	idx=index-(knn->nframe-rb->used);
	// wtk_debug("index=%d/%d\n",index,rb->used);
	if(idx>=(rb->used))
	{
		if(is_end)
		{
			ft=(wtk_knn_feat_t*)wtk_robin_at(rb,rb->used-1);
			return ft;//->pf;
		}else
		{
			//wtk_debug("break get index=%d/%d %d failed\n",knn->nframe,layer->input_idx,index);
			return NULL;
		}
		//exit(0);
	}
	if(idx<0)
	{
		idx=0;
	}
	// wtk_debug("idx=%d/%d %d/%d input=%d\n",index,idx,knn->nframe,rb->used,layer->input_idx);
	ft=(wtk_knn_feat_t*)wtk_robin_at(rb,idx);
	return ft;//->pf;
}

#if 0
static void wtk_knn_print_feat4(wtk_knn_t *knn,wtk_vecf_t *input)
{
	int xd[]={0,1,4,3,5,2};
	int i;

	printf("v[%d]=",knn->oframe);
	for(i=0;i<input->len;++i)
	{
		if(i>0)
		{
			printf("/");
		}
		printf("%.3f",exp(input->p[xd[i]]));
	}
	printf("\n");
}
#endif

static void wtk_knn_print_feat(wtk_knn_t *knn,wtk_vecf_t *input)
{
	int i;

	wtk_debug("knn[%d]=",knn->oframe);
	for(i=0;i<input->len;++i)
	{
		if(i>0)
		{
			printf("/");
		}
		printf("%.3f",exp(input->p[i]));
	}
	printf("\n");
}

#if 0
static void wtk_knn_print_feat3(wtk_knn_t *knn,wtk_vecf_t *input)
{
	int i;
	int id[]={0,1,2,3,4,5};
	char *xy[]={"sil","bg","i4","iao3","x","zh"};
	float f=-100;
	int idx=-1;

	for(i=0;i<sizeof(id)/sizeof(int);++i)
	{
		//wtk_debug("v[%d/%s]=%f\n",i,xy[i],exp(input->p[id[i]]));
		if(input->p[id[i]]>f)
		{
			f=input->p[id[i]];
			idx=i;
		}
	}
	if(knn->cfg->prior->len>0)
	{
		wtk_debug("v[%d]=%s/%f\n",knn->oframe,xy[idx],exp(f));
	}else
	{
		wtk_debug("v[%d]=%s/%f\n",knn->oframe,xy[idx],f);
	}
}


static void wtk_knn_print_feat2(wtk_knn_t *knn,wtk_vecf_t *input)
{
	int i;
	float f=-100;
	int idx=-1;
	int id[]={1997,273,1452,280,2041,1113,0,2088};
	char *xy[]={"sil-x+iao3","x-iao3+zh","iao3-zh+i4","zh-i4+x","i4-x+iao3","zh-i4+sil","sil","filler"};
	float f1=-100;
	int idx1=-1;

	for(i=0;i<input->len;++i)
	{
		//v=wtk_kwmdl2_cfg_get_phn_name(&(dec->cfg->mdl),i);
		//wtk_debug("v[%d/%d/%.*s]=%f\n",dec->nframe,i,v->len,v->data,feat->v[i]);
		if(input->p[i]>f)
		{
			f=input->p[i];
			idx=i;
		}
	}
	for(i=0;i<sizeof(id)/sizeof(int);++i)
	{
		//wtk_debug("v[%d/%d]=%s/%f\n",knn->rte->layers[knn->cfg->nlogic_layer-2]->input_idx,i,xy[i],exp(input->p[id[i]]));
		if(input->p[id[i]]>f1)
		{
			f1=input->p[id[i]];
			idx1=i;
		}
	}
	wtk_debug("v[%d]=%d/%f %s/%f\n",knn->rte->layers[knn->cfg->nlogic_layer-2]->input_idx,
			idx,exp(f),xy[idx1],exp(f1));
	//printf("\n");
	//exit(0);
}
#endif

static void wtk_knn_calc_layer_shift(wtk_knn_t *knn,wtk_knn_rte_layer_t *layer,wtk_vecf_t *output)
{
	static float xmin[32]={0};
	static float xmax[32]={0};
	static int b=1;
	int i;
	float max,min;
	int idx;

	if(b==1)
	{
		for(i=0;i<30;++i)
		{
			xmin[i]=100000;
			xmax[i]=-10000;
		}
		b=0;
	}
	idx=-1;
	for(i=0;i<knn->cfg->nlogic_layer;++i)
	{
		if(layer->layer==knn->cfg->logic_layer[i])
		{
			idx=i;
			break;
		}
	}
	max=wtk_float_max(output->p,output->len);
	min=wtk_float_min(output->p,output->len);
	if(min<xmin[idx])
	{
		xmin[idx]=min;
	}
	if(max>xmax[idx])
	{
		xmax[idx]=max;
	}
	//if(layer->layer->layer->type==WTK_LogSoftmaxComponent)
	if(wtk_string_cmp(layer->layer->name,"output.affine",layer->layer->name->len)==0)
	{
		int shift;

		shift=15;
		for(i=0;i<knn->cfg->nlogic_layer-1;++i)
		{
			float f;

			f=max(fabs(xmin[i]),fabs(xmax[i]));
			wtk_debug("v[%d]=[%f,%f] shift=%f\n",i,xmin[i],xmax[i],wtk_log2(wtk_pow(2,shift)*1.0/f));
		}
		printf("layer_shift=[");
		for(i=0;i<knn->cfg->nlogic_layer-1;++i)
		{
			float f;

			f=max(fabs(xmin[i]),fabs(xmax[i]));
			if(i>0)
			{
				printf(",");
			}
			printf("%d",(int)(wtk_log2(wtk_pow(2,shift)*1.0/f)));
		}
		printf("];\n");
	}
}


static wtk_vecf_t* wtk_knn_feed_layer_input(wtk_knn_t *knn,wtk_knn_rte_layer_t *layer,wtk_vecf_t *input,int is_end)
{
	wtk_knn_layer_t *l=layer->layer->layer;
	wtk_vecf_t *output=NULL;
	
#ifdef KNN_DEBUG
    {
        FILE *f = fopen("/tmp/cmp/qtmp-input", "a");
        fprintf(f, "%.*s_%s [\n", layer->layer->name->len, layer->layer->name->data, wtk_knn_layer_type_str(l->type));
        int i;
        for (i=0; i<input->len; i++) {
            fprintf(f, " %f", input->p[i]);
        }
        fprintf(f, " \n");
        fprintf(f, "]\n");
        fclose(f);
    }
#endif

//   wtk_debug("v[%d/%.*s]=%0d %s input=%d output=%d\n",knn->nframe,layer->layer->name->len,layer->layer->name->data,layer->input_idx,
// 				 wtk_knn_layer_type_str(layer->layer->layer->type),input->len,layer->output->len);
				
// if(wtk_string_cmp_s(layer->layer->name,"tdnn1.affine")==0)
	// {
		// static int i=0;
		// wtk_debug("affine  idx=%d\n",i++);
//		 wtk_debug("======input=====\n");
//		 print_float(input->p,input->len);
//		 wtk_debug("======end=====\n");
	// }
	switch(l->type)
	{
	case WTK_NaturalGradientAffineMaskComponent:
		wtk_knn_ng_mask_affine_calc(l->v.ng_mask_affine,input,layer->output);
		output=layer->output;
		break;
	case WTK_LinearMaskComponent:
		wtk_knn_linear_mask_calc(l->v.linear_mask,input,layer->output);
		output=layer->output;
		break;
	case WTK_LinearComponent:
		wtk_knn_linear_calc(l->v.linear,input,layer->output);
		output=layer->output;
		break;
	case WTK_ConvolutionComponent:
		wtk_knn_conv_calc(l->v.conv,input,layer->output);
		output=layer->output;
		break;
	case WTK_AffineComponent:
		//wtk_debug("%d*%d\n",l->v.affine->a->row,l->v.affine->a->col);
		wtk_knn_ab_calc(l->v.affine->ab,input,layer->output);
		output=layer->output;
		break;
	case WTK_FixedAffineComponent:
		//wtk_debug("%d*%d\n",l->v.fixed_affine->a->row,l->v.fixed_affine->a->col);
//		wtk_debug("fixed\n");
//		wtk_debug("sum=%f/%f\n",wtk_float_sum(l->v.fixed_affine->a->p,l->v.fixed_affine->a->row*l->v.fixed_affine->a->col),
//				wtk_float_sum(l->v.fixed_affine->b->p,l->v.fixed_affine->b->len));
		wtk_knn_ab_calc(l->v.fixed_affine->ab,input,layer->output);
		output=layer->output;
		break;
	case WTK_NaturalGradientAffineComponent:
		//wtk_debug("%d*%d\n",l->v.ng_affine->a->row,l->v.ng_affine->a->col);
		wtk_knn_ab_calc(l->v.ng_affine->ab,input,layer->output);
		output=layer->output;
		break;
	case WTK_RectifiedLinearComponent:
		//wtk_debug("%d*%d\n",l->v.ng_affine->a->row,l->v.ng_affine->a->col);
		wtk_rectified_linear_calc(NULL,input,layer->output);
		output=layer->output;
		break;
	case WTK_BatchNormComponent:
		wtk_batch_norm_calc(l->v.batch_norm,input,layer->output);
		output=layer->output;
		break;
	case WTK_NormalizeComponent:
		wtk_normalize_calc(l->v.normalize,input,layer->output);
		output=layer->output;
		break;
	case WTK_NoOpComponent:
		memcpy(layer->output->p,input->p,sizeof(float)*input->len);
		output=input;
		// exit(0);
		break;
	case WTK_GeneralDropoutComponent:
		memcpy(layer->output->p,input->p,sizeof(float)*input->len);
		output=input;
		break;
	case WTK_SigmoidComponent:
		wtk_knn_sigmoid(input,layer->output);
		output=layer->output;
		break;
	case WTK_LogSoftmaxComponent:
		if(1)
		{
			output=layer->output;
			memcpy(output->p,input->p,input->len*sizeof(float));
			wtk_log_softmax_calc(NULL,output);
		}else
		{
			wtk_log_softmax_calc(NULL,input);
			output=input;
		}
		break;
    	case WTK_CompactFsmnComponent:
		wtk_fsmn_calc(l->v.fsmn,input,layer->output);
        	output = layer->output;
        	break;
	case WTK_StatisticsExtractionComponent:
		wtk_knn_static_extraction(knn,layer,input);
		output=layer->output;
		break;
	case WTK_StatisticsPoolingComponent:
		wtk_knn_static_pooling(knn,layer,input,is_end);
		if(is_end) 
		{
			output=layer->output;
		}
		break;
	default:
		break;
	}
	if(knn->cfg->calc_shift)
	{
		wtk_knn_calc_layer_shift(knn,layer,output);
	}
#ifdef KNN_DEBUG
    {
        FILE *f = fopen("/tmp/cmp/qtmp", "a");
        fprintf(f, "%.*s_%s [\n", layer->layer->name->len, layer->layer->name->data, wtk_knn_layer_type_str(l->type));
        int i;
        for (i=0; i<output->len; i++) {
            fprintf(f, " %f", output->p[i]);
        }
        fprintf(f, " \n");
        fprintf(f, "]\n");
        fclose(f);
    }
#endif
//	 wtk_debug("=====output=====\n");
//	 print_float(output->p,output->len);
//	 wtk_debug("=====end=====\n");
	 //wtk_debug("=====output=====\n");
	// int i;
	// for(i=0;i<output->len;++i)
	// {
	// 	printf("out[%d]=%f\n",i,output->p[i]);
	// }
	// wtk_debug("=====end=====\n");
	// exit(0);
	return output;
}

void wtk_knn_static_pooling(wtk_knn_t *knn,wtk_knn_rte_layer_t *layer,wtk_vecf_t *input,int is_end)
{
	int i;
	int var_index=layer->output->len>>1;
	float floor_v=layer->layer->layer->v.static_pooling->variance_floor;




	if(is_end)
	{	//mean
#ifdef USE_NEON
		wtk_neon_math_vec_mul_const(layer->output->p,layer->output->p,1.0f/knn->pooling_nframe,layer->output->len);
#else
		for(i=0;i<layer->output->len;++i)
		{
			layer->output->p[i]/=knn->pooling_nframe;
			// printf("%d %f\n",i,layer->output->p[i]);
		}
#endif
	}
	else
	{
		if(knn->pooling_nframe==0)
		{
			memset(layer->output->p,0,layer->output->len*sizeof(float));
		}
		knn->pooling_nframe++;
#ifdef USE_NEON
		wtk_neon_math_vec_add(layer->output->p,layer->output->p,input->p+1,layer->output->len);
#else
		for(i=0;i<layer->output->len;++i)
		{
			// printf("%d %f\n",i,input->p[i]);
			layer->output->p[i]+=input->p[i+1];
		}
#endif
		return;
	}

	if(layer->layer->layer->v.static_pooling->numlog_count_features>0)
	{
		//not implement
	}

#ifdef USE_NEON
//	wtk_neon_math_vec_sub_square2
	if(layer->layer->layer->v.static_pooling->output_stddevs)
	{
		//var=E(x^2)-E(x)*E(x)
		wtk_neon_math_vec_sub_square2(layer->output->p+var_index,layer->output->p+var_index,layer->output->p,layer->output->len>>1);

		for(i=0;i<layer->output->len>>1;++i)
		{
			layer->output->p[i+var_index]=layer->output->p[i+var_index]<floor_v?floor_v:layer->output->p[i+var_index];
			// layer->output->p[i+var_index]=pow(layer->output->p[i+var_index],0.5f);
			layer->output->p[i+var_index]=sqrtf(layer->output->p[i+var_index]);
			// printf("%d %f\n",i,layer->output->p[i+var_index]);
		}
	}
#else
	if(layer->layer->layer->v.static_pooling->output_stddevs)
	{
		//var=E(x^2)-E(x)*E(x)
		for(i=0;i<layer->output->len>>1;++i)
		{
			layer->output->p[i+var_index]-=(layer->output->p[i]*layer->output->p[i]);
			layer->output->p[i+var_index]=layer->output->p[i+var_index]<floor_v?floor_v:layer->output->p[i+var_index];
			// layer->output->p[i+var_index]=pow(layer->output->p[i+var_index],0.5f);
			layer->output->p[i+var_index]=sqrtf(layer->output->p[i+var_index]);
			// printf("%d %f\n",i,layer->output->p[i+var_index]);
		}
	}
#endif

}

void wtk_knn_static_extraction(wtk_knn_t *knn,wtk_knn_rte_layer_t *layer,wtk_vecf_t *input)
{
	int i,j,n;
	float *src;
	float *dst;

	n=input->len/layer->layer->layer->v.static_extraction->input_dim;
	layer->output->p[0]=1;

#ifdef USE_NEON
	wtk_neon_math_memcpy(layer->output->p+1,input->p,layer->layer->layer->v.static_extraction->input_dim);
#else
	memcpy(layer->output->p+1,input->p,layer->layer->layer->v.static_extraction->input_dim*sizeof(float));
#endif

	src=input->p+layer->layer->layer->v.static_extraction->input_dim;
	for(i=1;i<n;++i)
	{
		dst=layer->output->p+1;
#ifdef USE_NEON
		wtk_neon_math_vec_add(dst,dst,src,layer->layer->layer->v.static_extraction->input_dim);
#else
		for(j=0;j<layer->layer->layer->v.static_extraction->input_dim;++j)
		{
			dst[j]+=src[j];
		}
#endif
		src+=layer->layer->layer->v.static_extraction->input_dim;
	}

	if(layer->layer->layer->v.static_extraction->include_var)
	{
		src=input->p;
		dst=layer->output->p+layer->layer->layer->v.static_extraction->input_dim+1;

#ifdef USE_NEON
		wtk_neon_math_vec_square(dst,src,layer->layer->layer->v.static_extraction->input_dim);
#else
		for(j=0;j<layer->layer->layer->v.static_extraction->input_dim;++j)
		{
			// dst[j]=pow(src[j],2);
			dst[j]=src[j]*src[j];
		}
#endif

		src=input->p+layer->layer->layer->v.static_extraction->input_dim;
		for(i=1;i<n;++i)
		{
			dst=layer->output->p+layer->layer->layer->v.static_extraction->input_dim+1;
#ifdef USE_NEON
			wtk_neon_math_vec_square(dst,src,layer->layer->layer->v.static_extraction->input_dim);
#else
			for(j=0;j<layer->layer->layer->v.static_extraction->input_dim;++j)
			{
				// dst[j]=pow(src[j],2);
				dst[j]=src[j]*src[j];
			}
#endif
			src+=layer->layer->layer->v.static_extraction->input_dim;
		}

	}

	// for(j=0;j<layer->output->len;++j)
	// {
	// 	printf("%d %f\n",j,layer->output->p[j]);
	// 	// printf("%d %f\n",j,input->p[j]);
	// }
}

static int wtk_knn_check_bitmap(wtk_knn_t *knn,wtk_knn_bitmap_t *map,int index)
{
	wtk_knn_cfg_t *cfg=knn->cfg;
	int i,b;

	if(knn->cfg->use_fast_skip)
	{
		i=(index>>1)<<1;
		b=(i==index)?1:0;
//		if(b==1)
//		{
//			b=wtk_knn_bitmap_check_index(map,index,cfg->skip);
//		}
	}else
	{
		b=wtk_knn_bitmap_check_index(map,index,cfg->skip);
	}
	return b;
}

static wtk_vecf_t* wtk_knn_flush_frame(wtk_knn_t *knn,int i,int is_end)
{
	wtk_knn_rte_layer_t *layer;
	wtk_knn_rte_layer_t *rl;
	wtk_knn_input_t *xi;
	wtk_vecf_t *input=NULL;
	float *pf;
	float *xvec;
	wtk_knn_feat_t  *ft;
	int j,k;
	wtk_vecf_t *output;
	int b, min_offset;
	int skip=0;
	int rb_idx;
	float scale;

	layer=knn->rte->layers[i];
	// wtk_debug("v[%d/%.*s]=%0d\n",knn->nframe,layer->layer->name->len,layer->layer->name->data,layer->input_idx)
	if(knn->use_skip) {
		if (layer->input_idx!=layer->skip_idx) {
            b=wtk_knn_check_bitmap(knn,layer->layer->bitmap,layer->input_idx);
			layer->skip=skip=(b==0)?1:0;
			//wtk_debug("layer skip=%d,b=%d\n",layer->skip);
			layer->skip_idx=layer->input_idx;
		} else {
			skip=layer->skip;
		}
	}
	if(i>0)
	{
		min_offset=layer->layer->layer_input[0].offset;
		if(layer->layer->layer_input->sum==1)
		{
			input=layer->input;
			pf = input->p;
			memset(pf,0,sizeof(float)*input->len);
			for(j=0;j<layer->layer->n_input;++j)
			{
				scale=1.0;
				if(layer->layer->layer_input[j].scale>0.001)
				{
					scale = layer->layer->layer_input[j].scale;
				}
				// output=knn->rte->layers[(int)(layer->layer->layer_input[j].layer)]->output;
				// if(knn->rte->layers[(int)(layer->layer->layer_input[j].layer)]->layer->use_tdnn_output==1)

				xi=layer->layer->layer_input+j;
				rl=knn->rte->layers[xi->layer];
				if(rl->tdnn_robin!=NULL)
				{
					// wtk_debug("%s\n",rl->layer->name->data);
					ft=wtk_knn_rte_layer_get_tdnn_output2(knn,rl);
					memcpy(pf,ft->v.f,input->len*sizeof(float));
					for(k=0;k<input->len;++k)
					{
						// wtk_debug("%f,%f\n",pf[k],scale);
						pf[k]*=scale;
					}
					
				}else
				{
					// wtk_debug("%s\n",knn->rte->layers[(int)(layer->layer->layer_input[j].layer)]->layer->name->data);
					output=knn->rte->layers[(int)(layer->layer->layer_input[j].layer)]->output;
					for(k=0;k<input->len;++k)
					{
						pf[k]+=output->p[k]*scale;
						// wtk_debug("%f,%f,%f\n",pf[k],output->p[k],scale);
					}
				}
				// wtk_debug("%d,layer=%d\n",output->len,layer->layer->layer_input[j].layer);
				// wtk_debug("output=%d input=%d\n",output->len,layer->input->len);
			}	
		}else
		{
			if(skip)
			{
				if(layer->layer->use_tdnn_input)
				{
					xi=layer->layer->layer_input+layer->layer->n_input-1;
					rl=knn->rte->layers[xi->layer];
					if (rl->tdnn_robin->used < rl->tdnn_robin->nslot-1) {
						return NULL;
					}
					ft=wtk_knn_rte_layer_get_tdnn_output(knn,rl,xi->offset,min_offset, layer->input_idx);
					if(!ft){return NULL;}
				}
			}else
			{
				if(layer->layer->use_tdnn_input)
				{
					input=layer->input;
					pf=input->p+input->len;
					for(i=layer->layer->n_input-1;i>=0;--i)
					{
						xi=layer->layer->layer_input+i;
						rl=knn->rte->layers[xi->layer];
						if (rl->tdnn_robin->used < rl->tdnn_robin->nslot-1) {
							return NULL;
						}
						ft=wtk_knn_rte_layer_get_tdnn_output(knn,rl,xi->offset,min_offset, layer->input_idx);
						// print_float(ft->v.f,rl->output->len);
						if(!ft)
						{
							return NULL;
						}
						pf-=rl->output->len;
						memcpy(pf,ft->v.f,rl->output->len*sizeof(float));
					}
				}else
				{
					if(layer->input)
					{
						input=layer->input;
						pf=input->p;
						for(j=0;j<layer->layer->n_input;++j)
						{
							if(layer->layer->layer_input[j].layer<0)
							{	
								// wtk_debug("v[%d/%.*s]=%0d,full=%d\n",knn->nframe,layer->layer->name->len,layer->layer->name->data,layer->input_idx,layer->layer->bitmap->full)
								rb_idx = (knn->rb->pop+knn->rb->used-1)%(knn->rb->nslot-1);
								ft=(wtk_knn_feat_t*)knn->rb->r[rb_idx];
								// wtk_debug("xxxxxx   feat=%d   pop=%d used=%d \n",ft->index,knn->rb->pop,knn->rb->used);
								memcpy(pf,ft->v.f,knn->cfg->input_dim*sizeof(float));
								pf+=knn->cfg->input_dim;
							}else
							{
								output=knn->rte->layers[(int)(layer->layer->layer_input[j].layer)]->output;
								memcpy(pf,output->p,output->len*sizeof(float));
								pf+=output->len;
							}
						}
					}else
					{
						input=knn->rte->layers[layer->layer->layer_input[0].layer]->output;
					}
				}
			}
		}
	}else
	{
        if (knn->rb->used < knn->rb->nslot-1) {
            return NULL;
        }
		input=layer->input;
		pf=input->p+input->len;
		for(j=layer->layer->n_input-1;j>=0;--j)
		{
			if(layer->layer->layer_input[j].replace>0)
			{
				xvec=wtk_knn_get_xvector(knn->cfg,knn->name);
				pf-=knn->cfg->ivector_dim;
				memcpy(pf,xvec,knn->cfg->ivector_dim*sizeof(float));
			}else
			{
				ft=wtk_knn_rte_layer_get_input(knn,layer,layer->layer->layer_input[j].offset,is_end);
				if(!ft)
				{
					return NULL;
				}
				pf-=knn->cfg->input_dim;
				memcpy(pf,ft->v.f,knn->cfg->input_dim*sizeof(float));
			}
		}
	}
	b=1;
	if(knn->use_skip)
	{
		if(layer->skip==0)
		{
			input=wtk_knn_feed_layer_input(knn,layer,input,is_end);
		}else
		{
			// wtk_debug("============skip===========v[%d/%.*s]=%0d,full=%d\n",knn->nframe,layer->layer->name->len,layer->layer->name->data,layer->input_idx,layer->layer->bitmap->full)
			b=0;
			//input=layer->output;
			input=NULL;
		}
	}else
	{
		input=wtk_knn_feed_layer_input(knn,layer,input,is_end);
	}
	if(input && layer->layer->use_tdnn_output)
	{
		wtk_knn_rte_layer_update_tdnn_output(layer,input,layer->input_idx,b);
	}
	++layer->input_idx;
	return input;
}

static void wtk_knn_fead_frame(wtk_knn_t *knn,int index,int is_end)
{
	wtk_knn_rte_t *rte=knn->rte;
	int i;
	wtk_vecf_t *input=NULL;
	wtk_kfeat_t feat;
	for(i=index;i<rte->nlayer;++i)
	{
		input=wtk_knn_flush_frame(knn,i,is_end);
		if(!input)
		{
			//wtk_debug("break\n");
			return;
		}
	}
	if(input)
	{
		//print_float(input->p,input->len);
		//exit(0);
		if(knn->use_skip)
		{
			i=knn->rte->layers[knn->rte->nlayer-1]->skip?0:1;
			//i=wtk_knn_bitmap_check_index(knn->rte->layers[knn->rte->nlayer-1]->layer->bitmap,knn->oframe,knn->cfg->skip);
			//wtk_debug("oframe=%d skip=%d i=%d/%d\n",knn->oframe,knn->rte->layers[knn->rte->nlayer-1]->skip_idx,i,knn->rte->layers[knn->rte->nlayer-1]->skip);
			//wtk_debug("nframe=%d/%d\n",knn->oframe,i);
		}else
		{
			i=1;
		}
		if(i==0)
		{
			++knn->oframe;
			//exit(0);
		}else
		{
			if(knn->cfg->debug)
			{
				wtk_knn_print_feat(knn,input);
			}
			if(knn->cfg->use_add_log)
			{
			  wtk_knn_add_log(knn,input);
			//   exit(0);
			}
			knn->input=input;
			// wtk_knn_print_feat(knn,input);
			// exit(0);
			if(knn->notify)
			{
				feat.v=input->p;
				//wtk_debug("feat=%d\n",knn->oframe);
				//wtk_knn_print_feat(knn,input);
				if(knn->use_skip)
				{
					feat.index=knn->oframe++;//knn->rte->layers[knn->rte->nlayer-1]->input_idx;
					//wtk_debug("feat=%d\n",feat.index);
					knn->notify(knn->ths,&feat);
					if(0)
					{
						for(i=0;i<knn->skip;++i)
						{
							++feat.index;
							knn->notify(knn->ths,&feat);
						}
					}
				}else
				{
					if(1)
					{
						//wtk_knn_print_feat(knn,input);
						feat.index=knn->oframe++;//knn->rte->layers[knn->rte->nlayer-1]->input_idx;
						knn->notify(knn->ths,&feat);
					}else
					{
						//wtk_debug("v[%d]=%d\n",knn->oframe,knn->oframe%3);
						if(knn->oframe%3==0)
						{
							feat.index=knn->oframe;//knn->rte->layers[knn->rte->nlayer-1]->input_idx;
							for(i=0;i<3;++i)
							{
								//wtk_debug("v[%d/%d]=%d\n",knn->oframe,i,feat.index);
								knn->notify(knn->ths,&feat);
							}
						}
						++knn->oframe;
					}
				}
			}
		}
	}
}

void wtk_knn_fead(wtk_knn_t *knn,float *feat,int index)
{
	wtk_knn_feat_t *ft;
    int i;
	// wtk_debug("feat=%d\n",index);
	// print_float(feat,knn->cfg->input_dim);
	//exit(0);
	if(knn->use_skip==0)
	{
		ft=(wtk_knn_feat_t*)wtk_robin_next(knn->rb);
		ft->index=index;
		memcpy(ft->v.f,feat,knn->cfg->input_dim*sizeof(float));
		++knn->nframe;
		wtk_knn_fead_frame(knn,0,0);
		if(knn->cfg->use_round==0)
		{
			if(index==0 && knn->flag==0)
			{
				for(i=0;i<knn->cfg->left_context;++i)
				{
					ft=(wtk_knn_feat_t*)wtk_robin_next(knn->rb);
					memcpy(ft->v.f,feat,knn->cfg->input_dim*sizeof(float));
					//++knn->nframe;
					wtk_knn_fead_frame(knn,0,0);
					if(i==knn->cfg->left_context-1)
					{
						knn->flag=1;
					}
				}
			}
		}
	}else
	{
		ft=(wtk_knn_feat_t*)wtk_robin_next(knn->rb);
		ft->index=knn->nframe;
		// ft->index=index;
		ft->valid=1;
		//ft->valid=wtk_knn_bitmap_check_index(knn->cfg->input_map,ft->index,knn->cfg->skip);
		if(ft->valid)
		{
			memcpy(ft->v.f,feat,knn->cfg->input_dim*sizeof(float));
		}
		++knn->nframe;
		// wtk_debug("raw_idx=%d feat idx=%d pop=%d,used=%d\n",index,ft->index,knn->rb->pop,knn->rb->used);
		wtk_knn_fead_frame(knn,0,0);
		//wtk_debug("%d/%d\n",knn->nframe,knn->oframe);
		if(index==0 && knn->flag==0)
        {
            for(i=0;i<knn->cfg->left_context;++i)
            {
                ft=(wtk_knn_feat_t*)wtk_robin_next(knn->rb);
                memcpy(ft->v.f,feat,knn->cfg->input_dim*sizeof(float));
                //++knn->nframe;
				// wtk_debug("raw_idx=%d feat idx=%d pop=%d,used=%d\n",index,ft->index,knn->rb->pop,knn->rb->used);
                wtk_knn_fead_frame(knn,0,0);
				if(i==knn->cfg->left_context-1)
				{
					knn->flag=1;
				}
            }
        }
	}
}

void wtk_knn_flush_pooling(wtk_knn_t *knn)
{
	wtk_knn_fead_frame(knn,knn->pooling_layer_index,1);
}

void wtk_knn_flush(wtk_knn_t *knn)
{
	wtk_knn_feat_t *ft,*ft2;
	wtk_robin_t* r;
	int i,j,index,dim;

	if(knn->cfg->right_context >=knn->rb->nslot-1)
	{
		dim=knn->cfg->input_dim;
		r=knn->rb;
		index=((r->pop-1)<0)?r->nslot-1:r->pop-1;
		ft=(wtk_knn_feat_t*)(r->r[index]);

        for (j=0; j<knn->rb->nslot-1; j++) {
            ft2=(wtk_knn_feat_t*)wtk_robin_next(r);
            if (knn->cfg->use_fixpoint) {
                memcpy(ft2->v.i,ft->v.i,dim*sizeof(int));
                if (knn->cfg->use_fixshort) {
                    wtk_knn_fead_frame_fix_short(knn, 0, 0);
                } else {
                    wtk_knn_fead_frame_fix_char(knn,0,0);
                }
            } else {
                memcpy(ft2->v.i,ft->v.i,dim*sizeof(float));
                wtk_knn_fead_frame(knn, 0, 0);
            }
        }

		for(i=0;i<knn->cfg->right_context-(knn->rb->nslot-1);++i)
		{
            if (knn->cfg->use_fixpoint) {
                if (knn->cfg->use_fixshort) {
                    wtk_knn_fead_frame_fix_short(knn, 0, 0);
                } else {
                    wtk_knn_fead_frame_fix_char(knn,0,0);
                }
            } else {
                wtk_knn_fead_frame(knn, 0, 0);
            }
		}
	}

	for(i=0;i<knn->rte->nlayer;++i)
	{
		//wtk_debug("######## i=%d %d##############\n",i,knn->rte->layers[i]->layer->skip);
		int frame=knn->nframe;

		if(knn->skip>0)
		{
			frame=knn->nframe/(knn->skip+1);
		}
		for(j=knn->rte->layers[i]->input_idx;j<frame;++j)
		{
            if (knn->cfg->use_fixpoint) {
                if (knn->cfg->use_fixshort) {
                    wtk_knn_fead_frame_fix_short(knn, 0, 0);
                } else {
                    wtk_knn_fead_frame_fix_char(knn,0,0);
                }
            } else {
                wtk_knn_fead_frame(knn, 0, 0);
            }
		}
		if(knn->rte->layers[i]->layer->skip >0)
		{
			knn->skip=knn->rte->layers[i]->layer->skip;
		}
		//exit(0);
	}
}


static wtk_vecs2_t* wtk_knn_feed_layer_input_fix_char(wtk_knn_t *knn,wtk_knn_rte_layer_t *layer,wtk_vecs2_t *input,int is_end)
{
    unuse(is_end);
	wtk_knn_layer_t *l=layer->layer->layer;
	wtk_vecs2_t *output=NULL;

	output=layer->output_s;
	output->shift=layer->layer->shift;
//   wtk_debug("v[%d/%.*s]=%0d %s input=%d output=%d,shift=%d\n",knn->nframe,layer->layer->name->len,layer->layer->name->data,layer->input_idx,
				//  wtk_knn_layer_type_str(layer->layer->layer->type),input->len,output->len,output->shift);
	switch(l->type)
	{
	case WTK_NaturalGradientAffineMaskComponent:
		exit(0);
		break;
	case WTK_LinearMaskComponent:
		exit(0);
		break;
	case WTK_LinearComponent:
        wtk_knn_linear_calc_fix_sc(l->v.linear, input, output);
		break;
	case WTK_ConvolutionComponent:
		wtk_knn_conv_calc_fix_sc(l->v.conv,input,output);
		break;
	case WTK_AffineComponent:
		wtk_knn_ab_calc_fix_sc(l->v.affine->ab,input,output);
		break;
	case WTK_FixedAffineComponent:
		wtk_knn_ab_calc_fix_sc(l->v.fixed_affine->ab,input,output);
		break;
	case WTK_NaturalGradientAffineComponent:
		wtk_knn_ab_calc_fix_sc(l->v.ng_affine->ab,input,output);
		break;
	case WTK_RectifiedLinearComponent:
		output->shift=input->shift;
		wtk_rectified_linear_calc_fix_char(NULL,input,output);
		break;
	case WTK_BatchNormComponent:
		output->shift=input->shift;
		wtk_batch_norm_calc_fix_char(l->v.batch_norm,input,output);
		break;
	case WTK_NormalizeComponent:
		exit(0);
		break;
	case WTK_SigmoidComponent:
		exit(0);
		break;
	case WTK_LogSoftmaxComponent:
		wtk_log_softmax_calc_fix_short(knn->cfg->softmax_fixe,knn->cfg->softmax_fixl,input,output);
		break;
	case WTK_NoOpComponent:
	case WTK_GeneralDropoutComponent:
	case WTK_LstmNonlinearityComponent:
	case WTK_BackpropTruncationComponent:
	case WTK_ScaleAndOffsetComponent:
	case WTK_StatisticsExtractionComponent:
	case WTK_StatisticsPoolingComponent:
		break;
	default:
		break;
	}
	// int i;
	// for(i=0;i<output->len;++i)
	// {
	// 	printf("out[%d]=%f\n",i,FIX2FLOAT_ANY(output->p[i],output->shift));
	// }
	return output;
}

static wtk_vecs2_t* wtk_knn_feed_layer_input_fix_char0(wtk_knn_t *knn,wtk_knn_rte_layer_t *layer,wtk_veci_t *input,int is_end)
{
    unuse(knn);
    unuse(is_end);
	wtk_knn_layer_t *l=layer->layer->layer;
	wtk_vecs2_t *output=NULL;
	output=layer->output_s;
	output->shift=layer->layer->shift;
//   wtk_debug("v[%d/%.*s]=%0d %s input=%d output=%d,shift=%d\n",knn->nframe,layer->layer->name->len,layer->layer->name->data,layer->input_idx,
				//  wtk_knn_layer_type_str(layer->layer->layer->type),input->len,output->len,output->shift);
	switch(l->type)
	{
	case WTK_NaturalGradientAffineMaskComponent:
		exit(0);
		break;
	case WTK_LinearMaskComponent:
		exit(0);
		break;
	case WTK_LinearComponent:
        wtk_knn_linear_calc_fix_ic(l->v.linear, input, output);
		break;
	case WTK_ConvolutionComponent:
		wtk_knn_conv_calc_fix_ic(l->v.conv,input,output);
		break;
	case WTK_AffineComponent:
		wtk_knn_ab_calc_fix_ic(l->v.affine->ab,input,output);
		break;
	case WTK_FixedAffineComponent:
		wtk_knn_ab_calc_fix_ic(l->v.fixed_affine->ab,input,output);
		break;
	case WTK_NaturalGradientAffineComponent:
		wtk_knn_ab_calc_fix_ic(l->v.ng_affine->ab,input,output);
		break;
	case WTK_RectifiedLinearComponent:
		exit(0);
		break;
	case WTK_BatchNormComponent:
		exit(0);
		break;
	case WTK_NormalizeComponent:
		exit(0);
		break;
	case WTK_SigmoidComponent:
		exit(0);
		break;
	case WTK_LogSoftmaxComponent:
		exit(0);
		break;
	case WTK_NoOpComponent:
	case WTK_GeneralDropoutComponent:
	case WTK_LstmNonlinearityComponent:
	case WTK_BackpropTruncationComponent:
	case WTK_ScaleAndOffsetComponent:
	case WTK_StatisticsExtractionComponent:
	case WTK_StatisticsPoolingComponent:
		break;
	default:
		break;
	}
	// int i;
	// for(i=0;i<output->len;++i)
	// {
	// 	printf("out[%d]=%f\n",i,FIX2FLOAT_ANY(output->p[i],output->shift));
	// }
	return output;
}

static void wtk_knn_rte_layer_update_tdnn_output_fix_char(wtk_knn_rte_layer_t *layer,wtk_vecs2_t *output,int index,int valid)
{
	wtk_knn_feat_t *ft;

	ft=(wtk_knn_feat_t*)wtk_robin_next(layer->tdnn_robin);
	ft->index=index;
	ft->valid=valid;
	memcpy(ft->v.s,output->p,output->len*sizeof(short));
}


static wtk_vecs2_t* wtk_knn_flush_frame_fix_char(wtk_knn_t *knn,int i,int is_end)
{
	wtk_knn_rte_layer_t *layer;
	wtk_knn_rte_layer_t *rl;
	wtk_knn_input_t *xi;
	wtk_knn_feat_t  *ft;
	int j, min_offset;
	wtk_vecs2_t *output=NULL;
	int b,skip=0;
	wtk_vecs2_t *input=NULL;
	short *pf;

	layer=knn->rte->layers[i];
	if(knn->use_skip) {
		if (layer->input_idx!=layer->skip_idx) {
            b=wtk_knn_check_bitmap(knn,layer->layer->bitmap,layer->input_idx);
			layer->skip=skip=(b==0)?1:0;
			//wtk_debug("slayer skip=%d,b=%d\n",layer->skip);
			layer->skip_idx=layer->input_idx;
		} else {
			skip=layer->skip;
		}
	}
	if(i>0)
	{
        min_offset=layer->layer->layer_input[0].offset;
		if(skip)
		{
			if(layer->layer->use_tdnn_input)
			{
				xi=layer->layer->layer_input+layer->layer->n_input-1;
				rl=knn->rte->layers[xi->layer];
                if (rl->tdnn_robin->used != rl->tdnn_robin->nslot) {
                    return NULL;
                }
				ft=wtk_knn_rte_layer_get_tdnn_output(knn,rl,xi->offset,min_offset, layer->input_idx);
				if(!ft){return NULL;}
			}
		}else
		{
			if(layer->layer->use_tdnn_input)
			{
				input=layer->input_s;
				pf=input->p+input->len;

				//for(i=0;i<layer->layer->n_input;++i)
				for(i=layer->layer->n_input-1;i>=0;--i)
				{
					//wtk_debug("v[%d]=%d/%d\n",i,layer->layer->layer_input[i].layer,layer->layer->layer_input[i].offset);
					xi=layer->layer->layer_input+i;
					rl=knn->rte->layers[xi->layer];
					if (rl->tdnn_robin->used < rl->tdnn_robin->nslot-1) {
						return NULL;
					}
					ft=wtk_knn_rte_layer_get_tdnn_output(knn,rl,xi->offset,min_offset, layer->input_idx);
					if(!ft)
					{
						return NULL;
					}
					input->shift=rl->output_s->shift;
					// wtk_debug("%d/%.*s/%d: %d,%.*s shift=%d\n",knn->nframe,layer->layer->name->len,layer->layer->name->data,i,ft->index,rl->layer->name->len,rl->layer->name->data,rl->output_s->shift);
					pf-=rl->output_s->len;
					memcpy(pf,ft->v.s,rl->output_s->len*sizeof(short));
					//pf+=rl->output->len;
				}
				//exit(0);
			}else
			{
				if(layer->input_s)
				{
					int rb_idx;
					int t;
					input=layer->input_s;
					input->shift=layer->output_s->shift;
					pf=input->p;
					// wtk_debug("%d,%s\n",input->shift,layer->layer->name->data);
					for(j=0;j<layer->layer->n_input;++j)
					{
						if(layer->layer->layer_input[j].layer<0)
						{	
							// wtk_debug("v[%d/%.*s]=%0d,full=%d\n",knn->nframe,layer->layer->name->len,layer->layer->name->data,layer->input_idx,layer->layer->bitmap->full)
							rb_idx = (knn->rb->pop+knn->rb->used-1)%(knn->rb->nslot-1);
							ft=(wtk_knn_feat_t*)knn->rb->r[rb_idx];	
							// wtk_debug("xxxxxx   feat=%d   pop=%d used=%d \n",ft->index,knn->rb->pop,knn->rb->used);
							for(i=0;i<knn->cfg->input_dim;++i)
							{
								t=ft->v.i[i];
								// if(output->shift>DEFAULT_RADIX)
								// {
								// 	tshift=output->shift-DEFAULT_RADIX;
								// 	t=t<<tshift;
								// 	// input->shift=output->shift;
								// }else if(output->shift<DEFAULT_RADIX)
								// {
								// 	tshift=DEFAULT_RADIX-output->shift;
								// 	t=t>>tshift;
								// 	// input->shift=output->shift;
								// }else
								// {
								// 	// input->shift=DEFAULT_RADIX;
								// }
								if(t>32767)
								{
									t=32767;
								}else if(t<-32768)
								{
									t=-32768;
								}
								// input->shift=output->shift;
								pf[i]=t;
								// printf("%f,%d\n",FIX2FLOAT_ANY(pf[i],output->shift),output->shift);
							}
							if(input->shift==0)input->shift=layer->layer->shift;
							pf+=knn->cfg->input_dim;
							
						}else
						{
							output=knn->rte->layers[(int)(layer->layer->layer_input[j].layer)]->output_s;
							memcpy(pf,output->p,output->len*sizeof(short));
							pf+=output->len;
						}
					}
				}else
				{
					input=knn->rte->layers[layer->layer->layer_input[0].layer]->output_s;
				}
			}
		}
		if(knn->use_skip)
		{
			if(layer->skip==0)
			{
				output=wtk_knn_feed_layer_input_fix_char(knn,layer,input,is_end);
			}else
			{
				// wtk_debug("============skip===========v[%d/%.*s]=%0d,full=%d\n",knn->nframe,layer->layer->name->len,layer->layer->name->data,layer->input_idx,layer->layer->bitmap->full)
				output=NULL;
			}
		}else
		{
			output=wtk_knn_feed_layer_input_fix_char(knn,layer,input,is_end);
		}					
		if(output && layer->layer->use_tdnn_output)
		{
			wtk_knn_rte_layer_update_tdnn_output_fix_char(layer,output,layer->input_idx,1);
		}
	}else
	{
        if (knn->rb->used < knn->rb->nslot-1) {
            return NULL;
        }
		wtk_veci_t *input=NULL;
		int *pf;

		input=layer->input_i;
		pf=input->p+input->len;
		//for(j=0;j<layer->layer->n_input;++j)
		for(j=layer->layer->n_input-1;j>=0;--j)
		{
			//wtk_debug("v[%d]=%d/%d\n",j,layer->layer->layer_input[j].layer,layer->layer->layer_input[j].offset);
			ft=wtk_knn_rte_layer_get_input(knn,layer,layer->layer->layer_input[j].offset,is_end);
			if(!ft)
			{
				return NULL;
			}
			//wtk_debug("%d/%.*s/%d: %d\n",knn->nframe,layer->layer->name->len,layer->layer->name->data,j,ft->index);
			pf-=knn->cfg->input_dim;
			memcpy(pf,ft->v.i,knn->cfg->input_dim*sizeof(int));
			//pf+=knn->cfg->input_dim;
		}
		//wtk_debug("sum=%f\n",wtk_float_sum(input->p,input->len));
		if(knn->cfg->use_fixchar0_short)
		{
			output=wtk_knn_feed_layer_input_fix_short0(knn,layer,input,is_end);
		}else
		{
			output=wtk_knn_feed_layer_input_fix_char0(knn,layer,input,is_end);
		}
		if(output && layer->layer->use_tdnn_output)
		{
			wtk_knn_rte_layer_update_tdnn_output_fix_char(layer,output,layer->input_idx,1);
		}
	}
	//wtk_fix_print(input->p,input->len,12);
	//exit(0);
	++layer->input_idx;
	//print_float(input->p,input->len);
	return output;
}

void wtk_knn_fead_frame_fix_char(wtk_knn_t *knn,int index,int is_end)
{
	wtk_knn_rte_t *rte=knn->rte;
	int i;
	wtk_vecs2_t *input=NULL;
	wtk_kfeat_t feat;

	for(i=index;i<rte->nlayer;++i)
	{
		input=wtk_knn_flush_frame_fix_char(knn,i,is_end);
		if(!input)
		{
			//wtk_debug("break\n");
			return;
		}
	}
	if(input)
	{
		if(knn->cfg->use_exp)
		{
			short *pv;
			wtk_fixexp_t *fixe=knn->cfg->fixe;

			pv=input->p;
			for(i=0;i<input->len;++i)
			{
				pv[i]=wtk_fixexp_calc(fixe,pv[i]);
			}
			feat.index=knn->oframe++;
			feat.v=(float*)pv;
			knn->notify(knn->ths,&feat);
		}else
		{
			feat.index=knn->oframe++;
			feat.v=(float*)input->p;
			knn->notify(knn->ths,&feat);
		}
	}
}

static void wtk_knn_fead_fix_char(wtk_knn_t *knn,int *feat,int index)
{
    int i;
	wtk_knn_feat_t *ft;

	ft=(wtk_knn_feat_t*)wtk_robin_next(knn->rb);
	ft->index=index;
	memcpy(ft->v.i,feat,knn->cfg->input_dim*sizeof(int));
	++knn->nframe;
	wtk_knn_fead_frame_fix_char(knn,0,0);

 	if(index==0 && knn->flag==0)
 	{
 		for(i=0;i<knn->cfg->left_context;++i)
 		{
 			ft=(wtk_knn_feat_t*)wtk_robin_next(knn->rb);
 			memcpy(ft->v.i,feat,knn->cfg->input_dim*sizeof(int));
 			//++knn->nframe;
 			wtk_knn_fead_frame_fix_char(knn,0,0);
			if(i==knn->cfg->left_context-1)
			{
				knn->flag=1;
			}
 		}
 	}
}



static wtk_vecs2_t* wtk_knn_feed_layer_input_fix_short(wtk_knn_t *knn,wtk_knn_rte_layer_t *layer,wtk_vecs2_t *input,int is_end)
{
    unuse(is_end);
	wtk_knn_layer_t *l=layer->layer->layer;
	wtk_vecs2_t *output=NULL;

	output=layer->output_s;
	output->shift=layer->layer->shift;
//   wtk_debug("v[%d/%.*s]=%0d %s input=%d output=%d,shift=%d\n",knn->nframe,layer->layer->name->len,layer->layer->name->data,layer->input_idx,
				//  wtk_knn_layer_type_str(layer->layer->layer->type),input->len,output->len,output->shift);
	switch(l->type)
	{
	case WTK_NaturalGradientAffineMaskComponent:
		exit(0);
		break;
	case WTK_LinearMaskComponent:
		exit(0);
		break;
	case WTK_LinearComponent:
		wtk_knn_linear_calc_fix_ss(l->v.linear,input,output);
		break;
	case WTK_ConvolutionComponent:
		wtk_knn_conv_calc_fix_ss(l->v.conv,input,output);
		break;
	case WTK_AffineComponent:
		wtk_knn_ab_calc_fix_ss(l->v.affine->ab,input,output);
		break;
	case WTK_FixedAffineComponent:
		wtk_knn_ab_calc_fix_ss(l->v.fixed_affine->ab,input,output);
		break;
	case WTK_NaturalGradientAffineComponent:
		wtk_knn_ab_calc_fix_ss(l->v.ng_affine->ab,input,output);
		break;
	case WTK_RectifiedLinearComponent:
		wtk_rectified_linear_calc_fix_char(NULL,input,output);
		break;
	case WTK_BatchNormComponent:
		wtk_batch_norm_calc_fix_short(l->v.batch_norm,input,output);
		break;
	case WTK_NormalizeComponent:
		exit(0);
		break;
	case WTK_SigmoidComponent:
		exit(0);
		break;
	case WTK_LogSoftmaxComponent:
		wtk_log_softmax_calc_fix_short(knn->cfg->softmax_fixe,knn->cfg->softmax_fixl,input,output);
		break;
	case WTK_NoOpComponent:
	case WTK_GeneralDropoutComponent:
	case WTK_LstmNonlinearityComponent:
	case WTK_BackpropTruncationComponent:
	case WTK_ScaleAndOffsetComponent:
	case WTK_StatisticsExtractionComponent:
	case WTK_StatisticsPoolingComponent:
		break;
	default:
		break;
	}
	// int i;
	// for(i=0;i<output->len;++i)
	// {
	// 	printf("out[%d]=%f\n",i,FIX2FLOAT_ANY(output->p[i],output->shift));
	// }
	return output;
}

wtk_vecs2_t* wtk_knn_feed_layer_input_fix_short0(wtk_knn_t *knn,wtk_knn_rte_layer_t *layer,wtk_veci_t *input,int is_end)
{
    unuse(knn);
    unuse(is_end);
	wtk_knn_layer_t *l=layer->layer->layer;
	wtk_vecs2_t *output=NULL;

	output=layer->output_s;
	output->shift=layer->layer->shift;
//   wtk_debug("v[%d/%.*s]=%0d %s input=%d output=%d,shift=%d\n",knn->nframe,layer->layer->name->len,layer->layer->name->data,layer->input_idx,
				//  wtk_knn_layer_type_str(layer->layer->layer->type),input->len,output->len,output->shift);
	switch(l->type)
	{
	case WTK_NaturalGradientAffineMaskComponent:
		exit(0);
		break;
	case WTK_LinearMaskComponent:
		exit(0);
		break;
	case WTK_ConvolutionComponent:
		wtk_knn_conv_calc_fix_is(l->v.conv,input,output);
		break;
	case WTK_LinearComponent:
        wtk_knn_linear_calc_fix_is(l->v.linear, input, output);
		break;
	case WTK_AffineComponent:
		wtk_knn_ab_calc_fix_is(l->v.affine->ab,input,output);
		break;
	case WTK_FixedAffineComponent:
		wtk_knn_ab_calc_fix_is(l->v.fixed_affine->ab,input,output);
		break;
	case WTK_NaturalGradientAffineComponent:
		wtk_knn_ab_calc_fix_is(l->v.ng_affine->ab,input,output);
		break;
	case WTK_RectifiedLinearComponent:
		exit(0);
		break;
	case WTK_BatchNormComponent:
		exit(0);
		break;
	case WTK_NormalizeComponent:
		exit(0);
		break;
	case WTK_SigmoidComponent:
		exit(0);
		break;
	case WTK_LogSoftmaxComponent:
		exit(0);
		break;
	case WTK_NoOpComponent:
	case WTK_GeneralDropoutComponent:
	case WTK_LstmNonlinearityComponent:
	case WTK_BackpropTruncationComponent:
	case WTK_ScaleAndOffsetComponent:
	case WTK_StatisticsExtractionComponent:
	case WTK_StatisticsPoolingComponent:
		break;
	default:
		break;
	}
	// int i;
	// for(i=0;i<output->len;++i)
	// {
	// 	printf("out[%d]=%f\n",i,FIX2FLOAT_ANY(output->p[i],output->shift));
	// }
	return output;
}

static void wtk_knn_rte_layer_update_tdnn_output_fix_short(wtk_knn_rte_layer_t *layer,wtk_vecs2_t *output,int index,int valid)
{
	wtk_knn_feat_t *ft;
	//float *pf;

	ft=(wtk_knn_feat_t*)wtk_robin_next(layer->tdnn_robin);
	ft->index=index;
	ft->valid=valid;
	memcpy(ft->v.s,output->p,output->len*sizeof(short));
	//exit(0);
}

static wtk_vecs2_t* wtk_knn_flush_frame_fix_short(wtk_knn_t *knn,int i,int is_end)
{
	wtk_knn_rte_layer_t *layer;
	wtk_knn_rte_layer_t *rl;
	wtk_knn_input_t *xi;
	wtk_knn_feat_t  *ft;
	int j, min_offset,b,skip=0;
	wtk_vecs2_t *output=NULL;

	layer=knn->rte->layers[i];
	if(knn->use_skip) {
		if (layer->input_idx!=layer->skip_idx) {
            b=wtk_knn_check_bitmap(knn,layer->layer->bitmap,layer->input_idx);
			layer->skip=skip=(b==0)?1:0;
			//wtk_debug("slayer skip=%d,b=%d\n",layer->skip);
			layer->skip_idx=layer->input_idx;
		} else {
			skip=layer->skip;
		}
	}
	if(i>0)
	{
		wtk_vecs2_t *input=NULL;
		short *pf;
		min_offset=layer->layer->layer_input[0].offset;
		if(skip)
		{
			if(layer->layer->use_tdnn_input)
			{
				xi=layer->layer->layer_input+layer->layer->n_input-1;
				rl=knn->rte->layers[xi->layer];
                if (rl->tdnn_robin->used != rl->tdnn_robin->nslot) {
                    return NULL;
                }
				ft=wtk_knn_rte_layer_get_tdnn_output(knn,rl,xi->offset,min_offset, layer->input_idx);
				if(!ft){return NULL;}
			}
		}else
		{
			if(layer->layer->use_tdnn_input)
			{
				//wtk_debug("==== yes1 ===\n");
				input=layer->input_s;
				pf=input->p+input->len;
				//for(i=0;i<layer->layer->n_input;++i)
				for(i=layer->layer->n_input-1;i>=0;--i)
				{
					//wtk_debug("v[%d]=%d/%d\n",i,layer->layer->layer_input[i].layer,layer->layer->layer_input[i].offset);
					xi=layer->layer->layer_input+i;
					rl=knn->rte->layers[xi->layer];
					if (rl->tdnn_robin->used < rl->tdnn_robin->nslot-1) {
						return NULL;
					}
					ft=wtk_knn_rte_layer_get_tdnn_output(knn,rl,xi->offset,min_offset, layer->input_idx);
					if(!ft)
					{
						return NULL;
					}
					input->shift=rl->output_s->shift;
					//wtk_debug("%d/%.*s/%d: %d\n",knn->nframe,layer->layer->name->len,layer->layer->name->data,i,ft->index);
					pf-=rl->output_s->len;
					memcpy(pf,ft->v.s,rl->output_s->len*sizeof(short));
					//pf+=rl->output->len;
				}
				//exit(0);
			}else
			{
				if(layer->input_s)
				{
					int rb_idx;
					int t;
					input=layer->input_s;
					input->shift=layer->output_s->shift;
					pf=input->p;
					for(j=0;j<layer->layer->n_input;++j)
					{
						if(layer->layer->layer_input[j].layer<0)
						{	
							// wtk_debug("v[%d/%.*s]=%0d,full=%d\n",knn->nframe,layer->layer->name->len,layer->layer->name->data,layer->input_idx,layer->layer->bitmap->full)
							rb_idx = (knn->rb->pop+knn->rb->used-1)%(knn->rb->nslot-1);
							ft=(wtk_knn_feat_t*)knn->rb->r[rb_idx];						
							// wtk_debug("xxxxxx   feat=%d   pop=%d used=%d \n",ft->index,knn->rb->pop,knn->rb->used);
							for(i=0;i<knn->cfg->input_dim;++i)
							{
								t=ft->v.i[i];
								if(t>32767)
								{
									t=32767;
								}else if(t<-32768)
								{
									t=-32768;
								}
								// input->shift=output->shift;
								pf[i]=t;
								// printf("%f,%d\n",FIX2FLOAT_ANY(pf[i],output->shift),output->shift);
							}
							if(input->shift==0)input->shift=layer->layer->shift;
							pf+=knn->cfg->input_dim;
						}else
						{
							output=knn->rte->layers[(int)(layer->layer->layer_input[j].layer)]->output_s;
							memcpy(pf,output->p,output->len*sizeof(short));
							pf+=output->len;
						}
					}
				}else
				{
					input=knn->rte->layers[layer->layer->layer_input[0].layer]->output_s;
					//wtk_debug("==== yes3=%d ===\n",input->shift);
				}
			}
			//wtk_debug("input shift=%d\n",input->shift);
			if(knn->use_skip)
			{
				if(layer->skip==0)
				{
					output=wtk_knn_feed_layer_input_fix_short(knn,layer,input,is_end);
				}else
				{
					output=NULL;
				}
			}else
			{
				output=wtk_knn_feed_layer_input_fix_short(knn,layer,input,is_end);
			}
			if(output && layer->layer->use_tdnn_output)
			{
				wtk_knn_rte_layer_update_tdnn_output_fix_short(layer,output,layer->input_idx,1);
			}
		}
	}else
	{
        if (knn->rb->used < knn->rb->nslot-1) {
            return NULL;
        }
		wtk_veci_t *input=NULL;
		int *pf;

		input=layer->input_i;
		pf=input->p+input->len;
		//for(j=0;j<layer->layer->n_input;++j)
		for(j=layer->layer->n_input-1;j>=0;--j)
		{
			//wtk_debug("v[%d]=%d/%d\n",j,layer->layer->layer_input[j].layer,layer->layer->layer_input[j].offset);
			ft=wtk_knn_rte_layer_get_input(knn,layer,layer->layer->layer_input[j].offset,is_end);
			if(!ft)
			{
				return NULL;
			}
			//wtk_debug("%d/%.*s/%d: %d\n",knn->nframe,layer->layer->name->len,layer->layer->name->data,j,ft->index);
			pf-=knn->cfg->input_dim;
			memcpy(pf,ft->v.i,knn->cfg->input_dim*sizeof(int));
			//pf+=knn->cfg->input_dim;
		}
		//wtk_debug("sum=%f\n",wtk_float_sum(input->p,input->len));
		output=wtk_knn_feed_layer_input_fix_short0(knn,layer,input,is_end);
		//exit(0);
		if(output && layer->layer->use_tdnn_output)
		{
			wtk_knn_rte_layer_update_tdnn_output_fix_short(layer,output,layer->input_idx,1);
		}
	}
	//wtk_fix_print(input->p,input->len,12);
	//exit(0);
	++layer->input_idx;
	//print_float(input->p,input->len);
	return output;
}


void wtk_knn_fead_frame_fix_short(wtk_knn_t *knn,int index,int is_end)
{
	wtk_knn_rte_t *rte=knn->rte;
	int i;
	wtk_vecs2_t *input=NULL;
	wtk_kfeat_t feat;
	short *pv;

	for(i=index;i<rte->nlayer;++i)
	{
		input=wtk_knn_flush_frame_fix_short(knn,i,is_end);
		if(!input)
		{
			//wtk_debug("break\n");
			return;
		}
	}
	if(input)
	{
		pv=input->p;
		knn->shift=input->shift;
		if(knn->cfg->use_exp)
		{
			if(0)
			{
				float fv[10];

				for(i=0;i<input->len;++i)
				{
					//fv[i]=wtk_knn_fix_exp(pv[i]);
					fv[i]=exp(FIX2FLOAT_ANY(pv[i],input->shift));
					//wtk_debug("v[%d]=%f\n",i,fv[i]);
				}
				feat.index=knn->oframe++;
				feat.v=fv;
				knn->notify(knn->ths,&feat);
			}else
			{
				short *pv;
				wtk_fixexp_t *fixe=knn->cfg->fixe;

				pv=input->p;
				for(i=0;i<input->len;++i)
				{
					pv[i]=wtk_fixexp_calc(fixe,pv[i]);
				}
				feat.index=knn->oframe++;
				feat.v=(float*)pv;
				knn->notify(knn->ths,&feat);
			}	
		}else if(0)
		{
			float fv[10];

			pv=input->p;
			//wtk_debug("v[%d]:",knn->oframe);
			for(i=0;i<input->len;++i)
			{
				//fv[i]=wtk_knn_fix_exp(pv[i]);
				fv[i]=FIX2FLOAT_ANY(pv[i],input->shift);
				//fv[i]=exp(FIX2FLOAT_ANY(pv[i],input->shift));
				//wtk_debug("v[%d]=%f\n",i,fv[i]);
//				if(i>0)
//				{
//					printf("/");
//				}
//				printf("%.3f",exp(FIX2FLOAT_ANY(pv[i],input->shift)));
			}
			//printf("\n");
			//exit(0);
			feat.index=knn->oframe++;
			feat.v=fv;
			knn->notify(knn->ths,&feat);
		}else
		{
			feat.index=knn->oframe++;
			feat.v=(float*)pv;
			knn->notify(knn->ths,&feat);
		}
		//exit(0);
	}
}


static void wtk_knn_fead_fix_short(wtk_knn_t *knn,int *feat,int index)
{
    int i;
	wtk_knn_feat_t *ft;

	ft=(wtk_knn_feat_t*)wtk_robin_next(knn->rb);
	ft->index=index;
	memcpy(ft->v.i,feat,knn->cfg->input_dim*sizeof(int));
	++knn->nframe;
	wtk_knn_fead_frame_fix_short(knn,0,0);
 	if(index==0 && knn->flag==0)
 	{
 		for(i=0;i<knn->cfg->left_context;++i)
 		{
 			ft=(wtk_knn_feat_t*)wtk_robin_next(knn->rb);
 			memcpy(ft->v.i,feat,knn->cfg->input_dim*sizeof(int));
 			//++knn->nframe;
 			wtk_knn_fead_frame_fix_short(knn,0,0);
			if(i==knn->cfg->left_context-1)
			{
				knn->flag=1;
			}
 		}
 	}
}


void wtk_knn_fead_fix(wtk_knn_t *knn,int *feat,int index)
{
	wtk_knn_cfg_t *cfg=knn->cfg;

	//wtk_fix_print(feat,knn->cfg->input_dim,12);
	//exit(0);
	if(cfg->use_fixchar)
	{
		wtk_knn_fead_fix_char(knn,feat,index);
	}else if(cfg->use_fixshort)
	{
		wtk_knn_fead_fix_short(knn,feat,index);
	}else
	{
		exit(0);
	}
}
