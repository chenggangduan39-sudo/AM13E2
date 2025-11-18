#ifdef USE_BLAS
#include "wtk_blas.h"
#include "math.h"
#include "../wtk_fnn.h"
#include "wtk/asr/fextra/wtk_fextra.h"

#if defined(KILL_BLAS)
#   define cblas_saxpy(...)
#   define cblas_sgemm(...)
#   define CblasRowMajor 0
#   define CblasNoTrans 0
#else
#if !defined(USE_MKL)
#   include "GotoBLAS2/common.h"
#   include "GotoBLAS2/cblas.h"
#else
#define CblasRowMajor 101
#define CblasNoTrans  111
#endif
#endif

wtk_blas_t *wtk_blas_new(wtk_blas_cfg_t *cfg, wtk_fnn_t *dnn)
{
    wtk_blas_t *b;

    b = (wtk_blas_t *)wtk_malloc(sizeof(*b));
    b->cfg = cfg;
    b->feat = wtk_blas_vector_new(cfg->align, cfg->in_cols * cfg->cache_size);
    b->input_feature_robin = wtk_robin_new(cfg->cache_size);
    b->tmp_m = wtk_blas_matrix_new(cfg->align, cfg->cache_size, cfg->max_row);
    b->tmp_m2 = wtk_blas_matrix_new(cfg->align, cfg->cache_size, cfg->max_row);
    b->dnn = dnn;
    b->last_feature=NULL;
    //wtk_debug("%d,%d\n",cfg->in_cols,cfg->cache_size);
    //wtk_debug("%d: [%d,%d]\n",cfg->cache_size,cfg->max_row,cfg->max_col);
    return b;
}

void wtk_blas_delete(wtk_blas_t *b)
{
    wtk_blas_matrix_delete(b->tmp_m);
    wtk_blas_matrix_delete(b->tmp_m2);
    wtk_robin_delete(b->input_feature_robin);
    wtk_blas_vector_delete(b->feat);
    wtk_free(b);
}

void wtk_blas_reset(wtk_blas_t *b)
{
	if(b->last_feature)
	{
		wtk_fextra_reuse_feature(b->dnn->parm,b->last_feature);
		b->last_feature=NULL;
	}
	b->frame_index=0;
    wtk_robin_reset(b->input_feature_robin);
}

void wtk_blas_feature_to_matirx(float *p, wtk_feat_t **pv, int step)
{
    wtk_vector_t *v;
    int i, j, n;
    int k;

    for (i = 0; i < step; ++i) {
        v = pv[i]->v;
        n = wtk_vector_size(v);
        for (k = i, j = 1; j <= n; ++j, k += step) {
            p[k] = v[j];
        }
    }
}

void wtk_blas_log(float *p, int len)
{
    float *pe;

    pe = p + len;
    while (p < pe) {
        *p = log(*p);
        ++p;
    }
}

void wtk_blas_add_last_trans(wtk_blas_t *d, float *a,int len)
{
    float *p,*e,*pv;
    wtk_vector_t *v=d->cfg->last_trans;
    pv=&(v[1]);

    p=a;e=p+len;
    while(p<e)
    {
        *p+=-1*(*pv);
        ++p;
        ++pv;
    }
}

void wtk_blas_process_layer3(wtk_blas_t *d, int nframex)
{
	wtk_fnn_t *dnn=d->dnn;
    wtk_blas_cfg_t *cfg = d->cfg;
    wtk_blas_vector_t *input_v = d->feat;
    wtk_queue_node_t *n;
    wtk_blas_layer_t *l;
    int j, len;
    wtk_blas_matrix_t *tmp_m = d->tmp_m;
    wtk_blas_matrix_t feat_m, *input_m;
    float *p,*p2;
    wtk_blas_trans_t *trans = cfg->expand_trans;
    wtk_feat_t *f;
    wtk_robin_t *r = d->input_feature_robin;
    unsigned int idx;
    int skip_frame;
    int nx_frame;
    float *fp;

    skip_frame=dnn->cfg->skip_frame;
    //wtk_debug("v=%d %d\n",skip_frame,dnn->cfg->min_flush_frame);
    input_v->len = nframex * cfg->in_cols;
    wtk_blas_vector_mult(input_v, trans->w);
    cblas_saxpy(input_v->len, 1, trans->b->v, 1, input_v->v, 1);
    fp=input_v->v;
    if(skip_frame>0)
    {
		for(nx_frame=0,j = 0,idx=d->frame_index; j < nframex; ++j,++idx)
		{
			if(idx%(skip_frame)==0)
			{
				if(j>0)
				{
					memcpy(fp,input_v->v+(j*cfg->in_cols),cfg->in_col_bytes);//cfg->in_cols*sizeof(float));
				}
				fp+=cfg->in_cols;
				++nx_frame;
			}
		}
    }else
    {
    	nx_frame=nframex;
    }
    feat_m.row = nx_frame;
    feat_m.col = cfg->in_cols;
    feat_m.m = input_v->v;
    input_m = &(feat_m);
    for (n = d->cfg->layer_q.pop; n; n = n->next)
    {
        l = data_offset(n, wtk_blas_layer_t, q_n);
        //B=M*X+B
        p = tmp_m->m;
        len =l->b->bytes;// l->b->len * sizeof(float);
        for (j = 0; j < nx_frame; ++j) {
            memcpy(p, l->b->v, len);
            p += l->b->len;
        }
        tmp_m->row = nx_frame;
        tmp_m->col = l->b->len;
        cblas_sgemm(
            CblasRowMajor, CblasNoTrans, CblasNoTrans,
            input_m->row, l->w->col, input_m->col,
            1, input_m->m, input_m->col,
            l->w->m, l->w->col,
            1, tmp_m->m, tmp_m->col);
        switch(l->type)
        {
        case wtk_fnn_sigmoid:
			p = tmp_m->m;
			for (j = 0; j < nx_frame; ++j)
			{
				wtk_sigmoid(p, l->b->len);
				//wtk_ann_sigmoid2(v->v,v->len);
				p += l->b->len;
			}
        	break;
        case wtk_fnn_softmax:
			if(!dnn->cfg->use_linear_output)
		    {
				p = tmp_m->m;
				for (j = 0; j < nx_frame; ++j)
				{
					wtk_softmax(p, l->b->len);
					wtk_blas_log(p, l->b->len);
					p += l->b->len;
				}
			}else{
				p=tmp_m->m;
	            for(j=0;j<nx_frame;++j)
    	        {
        	        wtk_blas_add_last_trans(d,p,l->b->len);
            	    p+=l->b->len; 
	            }
			}
        	break;
        case wtk_fnn_rescale:
			if(!dnn->cfg->use_linear_output)
			{
				p = tmp_m->m;
				for (j = 0; j < nx_frame; ++j)
				{
				//	wtk_debug("%f\n",*(p));
			
					wtk_blas_vector_mult2(p,l->b->len, l->rescale);
				//	wtk_debug("%f\n",*(p));
				//	wtk_softmax(p, l->b->len);
				//	wtk_blas_log(p, l->b->len);
					 wtk_blas_add_last_trans(d,p,l->b->len);
					p += l->b->len;
				}
			}
			break;
        case wtk_fnn_pnorm:
        	//n2 = n->next;
        	//l2 = data_offset(n, wtk_blas_layer_t, q_n);
        	//wtk_debug("ppppp\n");
		p = p2 = tmp_m->m;
        	for (j = 0; j < nx_frame; ++j)
        	{
        		wtk_pnorm(p,p2,l->b->len,l->out_dim);
			p += l->b->len;
			p2 += l->out_dim;
        	}
        	break;
//        case wtk_fnn_normalize:
//        	p = tmp_m->m;
//        	for (j = 0; j < nx_frame; ++j)
//        	{
//        		wtk_normalize(p,l->b->len);
//        	}
//        	break;
        case wtk_fnn_linear:
        	break;
        default:
            wtk_debug("layer->type not in list. %d\n", l->type);
            break;
        }
        input_m = tmp_m;
	if(l->type==wtk_fnn_pnorm)
	{
	    input_m->col=l->out_dim;
	}
        if (tmp_m == d->tmp_m) {
            tmp_m = d->tmp_m2;
        } else {
            tmp_m = d->tmp_m;
        }
    }
    len = input_m->col * sizeof(float);
    p = input_m->m;
   // wtk_debug("skip_frame=%d frame: %d/%d\n",skip_frame,r->used,d->frame_index);
    if(skip_frame>0)
    {
		d->frame_index+=r->used;
		//wtk_debug("frame: %d/%d\n",r->used,d->frame_index);
		while (r->used > 0) {
			f = wtk_robin_pop(r);
			if (!f) {
				break;
			}
			--f->used;
			if(f->index%(skip_frame)==1)
			{
				//wtk_debug("v[%p]=%d\n",f,f->index);
				memcpy(&(f->dnn_v[1]), p, len);
				if(d->last_feature)
				{
					--d->last_feature->used;
					wtk_fextra_push_feature(dnn->parm,d->last_feature);
				}
				++f->used;
				d->last_feature=f;
				f->app_hook=NULL;
				p += input_m->col;
			}else
			{
#ifndef USE_X
				++d->last_feature->used;
				f->app_hook=d->last_feature;
#else
				memcpy(&(f->dnn_v[1]),&(d->last_feature->dnn_v[1]),len);
				f->app_hook=NULL;
#endif
			}
			//wtk_debug("raise last %p=%d:%d\n",f,f->index,f->used);
			wtk_fnn_raise_feature(dnn, f);
		}
    }else
    {
		while (r->used > 0) {
			f = wtk_robin_pop(r);
			if (!f) {
				break;
			}
			memcpy(&(f->dnn_v[1]), p, len);
			--f->used;
			wtk_fnn_raise_feature(dnn, f);
			p += input_m->col;
		}
    }
    //exit(0);
}

void wtk_blas_process_layer(wtk_blas_t *d, wtk_feat_t **pv, int npv, wtk_feat_t *f)
{
    wtk_robin_t *r = d->input_feature_robin;

    //wtk_debug("v[%d]\n",f->index);
    ++f->used;
    wtk_blas_feature_to_matirx(d->feat->v + d->cfg->in_cols * r->used, pv, npv);
    wtk_robin_push(r, f);
    if (r->used == r->nslot) {
        wtk_blas_process_layer3(d, r->used);
    }
}

void wtk_blas_flush_layer(wtk_blas_t *d)
{
    wtk_robin_t *r = d->input_feature_robin;

    if (r->used > 0) {
        wtk_blas_process_layer3(d, r->used);
    }
}

void wtk_blas_flush_end(wtk_blas_t *d)
{
	if(d->last_feature)
	{
		--d->last_feature->used;
		wtk_fextra_push_feature(d->dnn->parm,d->last_feature);
		d->last_feature=NULL;
	}
}

#endif
