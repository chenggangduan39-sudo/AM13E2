#include <math.h>
#include "wtk_fnn.h"
#include "wtk/asr/fextra/wtk_fextra.h"

int wtk_fnn_bytes(wtk_fnn_t *d)
{
	int bytes;

	bytes=sizeof(wtk_fnn_t);
	bytes+=wtk_robin_bytes(d->robin);
	bytes+=sizeof(wtk_feat_t*)*d->robin->nslot;
	if(d->padding)
	{
		bytes+=d->cfg->padding_frame*sizeof(float);
	}
	if(d->cfg->use_qlas)
	{
		bytes+=wtk_qlas_bytes(d->v.qlas);
	}
	return bytes;
}

wtk_fnn_t *wtk_fnn_new(wtk_fnn_cfg_t *cfg, wtk_fextra_t *parm)
{
    wtk_fnn_t *d;
    //int i;

    d = (wtk_fnn_t *)wtk_malloc(sizeof(*d));
    d->cfg = cfg;
    d->parm = parm;
    if(cfg->use_delta)
    {
    	d->robin = wtk_robin_new(cfg->win * 2 + 1);
    }else
    {
    	d->robin = wtk_robin_new(33);
    }
    d->features = (wtk_feat_t **)wtk_calloc(d->robin->nslot, sizeof(wtk_feat_t *));
    if(cfg->padding_frame > 0)
    {
        d->padding = (float *)wtk_calloc(cfg->padding_frame, sizeof(float));
//        for(i=0;i<cfg->padding_frame;i++)
//        {
//        	d->padding[i]=*(((float *)d->cfg->expand_array->slot)+i);
//        }
    }else
    {
    	d->padding=NULL;
    }
    if(cfg->use_qlas)
    {
    	d->v.qlas=wtk_qlas_new(&(cfg->qlas),d);
    }else if(cfg->use_mlat)
    {
    	d->v.mlat=wtk_mlat_new(&(cfg->mlat),d);
    }else if (cfg->use_blas) {
#ifdef USE_BLAS
        d->v.blas = wtk_blas_new(&(cfg->blas), d);
#endif
    } else {
        d->v.flat = wtk_flat_new(&(cfg->flat),d);
    }
    d->sil_count=0;
    d->speech_count=0;
    return d;
}

void wtk_fnn_delete(wtk_fnn_t *d)
{
	if(d->padding)
	{
		wtk_free(d->padding);
	}
	if(d->cfg->use_qlas)
	{
		wtk_qlas_delete(d->v.qlas);
	}else if(d->cfg->use_mlat)
	{
		wtk_mlat_delete(d->v.mlat);
	}else if (d->cfg->use_blas) {
#ifdef USE_BLAS
        wtk_blas_delete(d->v.blas);
#endif
    } else {
        wtk_flat_delete(d->v.flat);
    }
    wtk_free(d->features);
    wtk_robin_delete(d->robin);
    wtk_free(d);
}

void wtk_fnn_reset(wtk_fnn_t *d)
{
	d->idx=0;
    d->sil_count=0;
    d->speech_count=0;
    d->tot_sil=0;
    wtk_robin_reset(d->robin);
    if(d->padding)
    {
    	memset(d->padding,0,d->cfg->padding_frame*sizeof(float));
    }
    if(d->cfg->use_qlas)
    {
    	wtk_qlas_reset(d->v.qlas);
    }else if(d->cfg->use_mlat)
    {
    	wtk_mlat_reset(d->v.mlat);
    }else if (d->cfg->use_blas) {
#ifdef USE_BLAS
        wtk_blas_reset(d->v.blas);
#endif
    } else
    {
    	wtk_flat_reset(d->v.flat);
    }
}


void wtk_dnn_flush_robin(wtk_fnn_t *p, wtk_robin_t *r)
{
    wtk_feat_t *f;

    f = (wtk_feat_t *)wtk_robin_pop(r);
    --f->used;
    //wtk_parm_dec_feature_use(p->parm,f);
  //  wtk_debug("flush robin: feature=%d/%p used=%d\n",f->index,f,f->used);
    wtk_fextra_push_feature(p->parm, f);
}


void wtk_dnn_feature_attach_log(wtk_vector_t *v)
{
    int i;
    int n;

    //tmp="%.3f" %(math.sqrt((-2.0)*float(self.a[i][j])))
    n = wtk_vector_size(v);
    for (i = 1; i <= n; ++i) {
        v[i] = sqrt(-2.0 * v[i]);
    }
}

void wtk_dnn_process_feature(wtk_fnn_t *d,wtk_feat_t **pv,int npv,wtk_feat_t *f)
{
    if(d->cfg->use_qlas)
    {
    	wtk_qlas_process_layer(d->v.qlas,pv,npv,f);
    }else if(d->cfg->use_mlat)
    {
    	wtk_mlat_process_layer(d->v.mlat, pv, npv, f);
    }else if (d->cfg->use_blas) {
#ifdef USE_BLAS
        wtk_blas_process_layer(d->v.blas, pv, npv, f);
#endif
    } else {
    	wtk_flat_process_layer(d->v.flat, pv, npv, f);
    }
}

void wtk_dnn_flush_mini_end(wtk_fnn_t *d)
{
    wtk_robin_t *r = d->robin;
    wtk_feat_t **pv = d->features;
    wtk_feat_t *f;
    int i,n,left,j,k;
    int win=d->cfg->win;

	if(r->used>d->cfg->win)
	{
    	i=r->used-win;
    	n=r->used-i;
    	//wtk_debug("n=%d i=%d\n",n,i);
// 	   for(j=0;j<r->used;++j)
// 	   {
// 		   f = (wtk_feat_t *)wtk_robin_at(r, j);
// 		   wtk_debug("v[%d]=%d used=%d\n",j,f->index,f->used);
// 	   }
		for(;i<r->used;++i)
		{
			left=win-i;
			f = (wtk_feat_t *)wtk_robin_at(r, i);
			//wtk_debug("i=%d  index=%d left=%d\n",i,f->index,left);
	        f = (wtk_feat_t *)wtk_robin_at(r, 0);
	        for (j=0; j< left; ++j) {
	            pv[j] = f;
	        }
	        if(left<0)
	        {
	        	k=-left;
	        }else
	        {
	        	k=0;
	        }
	        for(;k<r->used;++j,++k)
	        {
		        f = (wtk_feat_t *)wtk_robin_at(r, k);
		        //wtk_debug("k=%d/%d\n",k,f->index);
	        	pv[j]=f;
	        }
	        f = (wtk_feat_t *)wtk_robin_at(r, r->used-1);
	        for (; j < r->nslot; ++j)
	        {
	            pv[j] = f;
	        }
//	       for(j=0;j<r->nslot;++j)
//	       {
//	    	   wtk_debug("v[%d]=%d\n",j,pv[j]->index);
//	       }
	        f = pv[win];
	        //wtk_debug("v[%d]=%d used=%d\n",i,f->index,f->used);
	        wtk_dnn_process_feature(d,pv,r->nslot,f);
		}
	   for(j=0;j<r->used;++j)
	   {
		   f = (wtk_feat_t *)wtk_robin_at(r, j);
		   --f->used;
		   wtk_fextra_push_feature(d->parm,f);
		 //  wtk_debug("v[%d]=%d used=%d\n",j,f->index,f->used);
	   }
		wtk_robin_reset(r);
	}else
	{
		n=r->used;
		for(i=0;i<n;++i)
		{
			left=win-i;
			//wtk_debug("left=%d\n",left);
	        f = (wtk_feat_t *)wtk_robin_at(r, 0);
	        for (j=0; j< left; ++j) {
	            pv[j] = f;
	        }
	        for(k=0;k<n;++j,++k)
	        {
		        f = (wtk_feat_t *)wtk_robin_at(r, k);
		        //wtk_debug("k=%d/%d\n",k,f->index);
	        	pv[j]=f;
	        }
	        f = (wtk_feat_t *)wtk_robin_at(r, n-1);
	        for (; j < r->nslot; ++j) {
	            pv[j] = f;
	        }
	        f = pv[win];
	        //wtk_debug("v[%d]=%d used=%d\n",i,f->index,f->used);
	        wtk_dnn_process_feature(d,pv,r->nslot,f);
//	       for(j=0;j<r->nslot;++j)
//	       {
//	    	   wtk_debug("v[%d]=%d\n",j,pv[j]->index);
//	       }
		}
	   for(j=0;j<r->used;++j)
	   {
		   f = (wtk_feat_t *)wtk_robin_at(r, j);
		   --f->used;
		   wtk_fextra_push_feature(d->parm,f);
		 //  wtk_debug("v[%d]=%d used=%d\n",j,f->index,f->used);
	   }
		wtk_robin_reset(r);
	}
}

wtk_feat_t *wtk_dnn_flush_feature(wtk_fnn_t *d, int is_end)
{
    wtk_feat_t **pv = d->features;
    wtk_robin_t *r = d->robin;
    wtk_feat_t *f;
    int win = d->cfg->win;
    int i, pad, j;

    if (r->used <= win) {
        return 0;
    }
    pad = r->nslot - r->used; i = 0;
    //wtk_debug("r=%p,nslot=%d,used=%d\n",r,r->nslot,r->used);
    if (pad > 0 && !is_end) {
        //if not end, add pad to front.
        // * |f0|f1|f2|0|0|  => |f0|f0|f0|f1|f2|
        // * |f0|f1|f2|f3|0| => |f0|f0|f1|f2|f3|
        f = (wtk_feat_t *)wtk_robin_at(r, 0);
        for (; i < pad; ++i) {
            pv[i] = f;
        }
    }
    for (j = 0; j < r->used; ++i, ++j) {
        f = ((wtk_feat_t *)wtk_robin_at(r, j));
        pv[i] = f;
    }
    if (pad > 0 && is_end) {
        //if is end and pad to the end.
        //|f0|f1|f2|f3|0| => |f0|f1|f2|f3|f3|
        //|f0|f1|f2|0|0| => |f0|f1|f2|f2|f2|
        f = (wtk_feat_t *)wtk_robin_at(r, r->used - 1);
        for (j = 0; j < pad; ++i, ++j) {
            pv[i] = f;
        }
    }
    f = pv[win];
    wtk_dnn_process_feature(d,pv,r->nslot,f);
    if (r->nslot == r->used || is_end) {
        //if robin is full or got end hint, remove the front feature in the robin.
        wtk_dnn_flush_robin(d, r);
    }
    f=NULL;
    return f;
}


void wtk_fnn_feed(wtk_fnn_t *d, wtk_feat_t *f)
{
	//print_float(f->v+1,10);
	//exit(0);
	d->idx=f->index;
    ++f->used;
    wtk_robin_push(d->robin, f);
    f = wtk_dnn_flush_feature(d, 0);
    if (f)
    {
        wtk_fextra_output_feature_to_queue(d->parm, f);
    }
}

void wtk_dnn_expand_vector(wtk_fnn_t *dnn, wtk_feat_t *f)
{
	int numchans= dnn->parm->cfg->NUMCHNAS;
    //  wtk_vector_print(f->rv);
    int i, j, k;
    int num = numchans/ (dnn->cfg->padding_frame / 2);
    float p = 0.0;

    //wtk_debug("v[%d]=%f/%f\n",f->index,f->rv[1],f->rv[2]);
    if(f->rv[1] > f->rv[2] && f->rv[1] > dnn->cfg->sil_thresh) {
        k = 0;
        dnn->sil_count++;
        ++dnn->tot_sil;
        //test.wav.10 vad切不准，增加重置功能
        if(dnn->cfg->padding_sil_reset_cnt>0 && dnn->tot_sil>dnn->cfg->padding_sil_reset_cnt)
        {
            dnn->sil_count=dnn->cfg->padding_sil_set_cnt;
            dnn->speech_count=0;
            dnn->tot_sil=0;
            //wtk_debug("------------- set .....\n");
            memset(dnn->padding,0,dnn->cfg->padding_frame*sizeof(float));
//            for(i=0;i<dnn->cfg->padding_frame;++i)
//            {
//            	dnn->padding[i]=0;
//            }
        }
    } else if(f->rv[2] > f->rv[1] && f->rv[2] > dnn->cfg->speech_thresh) {
        k = dnn->cfg->padding_frame / 2;
        dnn->speech_count++;
        dnn->tot_sil=0;
    } else {
        k = -1;
    }
    //wtk_debug("sil/speech=%d/%d\n",dnn->sil_count,dnn->speech_count);
    if(k == 0 || k == dnn->cfg->padding_frame / 2)
    {
        i = 1;
        while(i <= numchans)
        {
            j = 0;
            p = 0.0;
            while(j < num) {
                p += f->v[i];
                j++;
                i++;
            }
            p = p / num;
            if(k >= dnn->cfg->padding_frame / 2)
            {
                dnn->padding[k] = (dnn->padding[k] * (dnn->speech_count - 1) + p) / dnn->speech_count;
            } else {
                dnn->padding[k] = (dnn->padding[k] * (dnn->sil_count - 1) + p) / dnn->sil_count;
                //wtk_debug("padd[%d]=%f\n",k,dnn->padding[k]);
            }
            k++;
        }
    }
}

void wtk_fnn_raise_feature(wtk_fnn_t *d, wtk_feat_t *f)
{
	//wtk_debug("v[%d]=%f/%f\n",f->index,f->dnn_v[1],f->dnn_v[2]);
    if(d->cfg->attach_htk_log)
    {
        wtk_dnn_feature_attach_log(f->dnn_v);
    }
    if(d->cfg->use_expand_vector)
    {
        wtk_dnn_expand_vector(d, f);
    }
   //wtk_debug("============== use dnn xyz ============\n");
   //wtk_vector_print(f->rv);
   // exit(0);
    wtk_fextra_output_feature_to_queue(d->parm, f);
}

void wtk_fnn_skip_feature(wtk_fnn_t *d,wtk_feat_t *f)
{
    wtk_fextra_push_feature(d->parm,f);
}


void wtk_fnn_flush(wtk_fnn_t *d)
{
    wtk_robin_t *r = d->robin;
    wtk_feat_t *f;

    if(d->idx>=r->nslot)
    {
		while (r->used>d->cfg->win) {
			f = wtk_dnn_flush_feature(d, 1);
			//wtk_debug("[%d/%d/%d]=%p\n",r->used,r->nslot,d->cfg->win,f);
			//wtk_debug("f=%p index=%d\n",f,f->index);
			if(f)
			{
				wtk_fextra_output_feature_to_queue(d->parm, f);
			}
		}
    }else
    {
    	wtk_dnn_flush_mini_end(d);
    }
    if(d->cfg->use_qlas)
    {
    	wtk_qlas_flush(d->v.qlas);
    	wtk_qlas_flush_end(d->v.qlas);
    }else if(d->cfg->use_mlat)
    {
    	wtk_mlat_flush(d->v.mlat);
    	wtk_mlat_flush_end(d->v.mlat);
    }else if (d->cfg->use_blas) {
#ifdef USE_BLAS
        wtk_blas_flush_layer(d->v.blas);
        wtk_blas_flush_end(d->v.blas);
#endif
    } else
    {
    	wtk_flat_flush(d->v.flat);
    	wtk_flat_flush_end(d->v.flat);
    }
	//wtk_debug("===============> feed end used=%d\n",r->used);
    while (r->used > 0) {
        f = (wtk_feat_t *)wtk_robin_pop(r);
        --f->used;
        //wtk_parm_dec_feature_use(d->parm,f);
        wtk_fextra_push_feature(d->parm, f);
    }
}

void wtk_fnn_wait_end(wtk_fnn_t *d)
{
	//wtk_debug("=============> dnn is end\n");
	if(d->cfg->use_mlat)
	{
		wtk_mlat_wait_end(d->v.mlat);
	}
}

void wtk_fnn_flush_layer(wtk_fnn_t *d,int force)
{
	if (d->cfg->use_blas) {
#ifdef USE_BLAS
        //wtk_debug("used=%d %d\n",d->v.blas->input_feature_robin->used,d->cfg->min_flush_frame);
        if(d->v.blas->input_feature_robin->used>d->cfg->min_flush_frame || force)
        {
            wtk_blas_flush_layer(d->v.blas);
        }
#endif
    }
}


