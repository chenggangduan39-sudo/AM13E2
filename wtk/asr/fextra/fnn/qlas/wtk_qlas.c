#include "wtk_qlas.h" 
#include "../wtk_fnn.h"
#include "wtk/asr/fextra/wtk_fextra.h"

#ifdef USE_NEON
void wtk_qlas_mats_mul_neon(wtk_vecs_t *a,wtk_mats_t *b,wtk_veci_t *c)
{
	int row=b->row;
	int col=b->col;
	int16x4_t *pax4;
	int16x4_t pbx4;
	short *pa, *pb;
	int32x4_t t1;
	int32_t t2;
	int col_tmp;
	int i;
    int tmp[4] = {0, 0, 0, 0};

	pb=b->p;
	for(i=0;i<row;++i){
		pax4=(int16x4_t *)a->p;
		pa=a->p;
		col_tmp=col;
		t1=vld1q_s32(tmp);
		while(col_tmp>=4)
		{
			pbx4=vld1_s16(pb);
			t1=vmlal_s16(t1, pax4[0], pbx4);
			++pax4;
			pa+=4;
			pb+=4;
			col_tmp-=4;
		}
		t2 = vgetq_lane_s32(t1, 0) + vgetq_lane_s32(t1, 1) + vgetq_lane_s32(t1, 2) + vgetq_lane_s32(t1, 3);
		while(col_tmp>0)
		{
			t2 += *pa++ * *pb++;
			--col_tmp;
		}
		c->p[i]=t2;
	}
}
#endif
void wtk_qlas_mats_mul_c(wtk_vecs_t *a,wtk_mats_t *b,wtk_veci_t *c)
{
	short *p2;
	int *p3;
	int i;
	int row=b->row;
	register int t,t1,t2,t3,t4;
	register short *pa,*pe,*pe2;

	p3=c->p;
	p2=b->p;
	pa=a->p;
	pe=pa+b->col;
	pe2=pa+((b->col>>2)<<2);
	for(i=0;i<row;++i)
	{
		pa=a->p;
		t1=t2=t3=t4=0;
		while(pa<pe2)
		{
			t1+=pa[0]*p2[0];
			t2+=pa[1]*p2[1];
			t3+=pa[2]*p2[2];
			t4+=pa[3]*p2[3];
			pa+=4;
			p2+=4;
		}
		t=t1+t2+t3+t4;
		while(pa<pe)
		{
			t+=*(pa++)*(*(p2++));
		}
		p3[i]=t;
		//*(p3++)=t;
	}
}

void wtk_qlas_matb_mul_c(wtk_vecb_t *a,wtk_matb_t *b,wtk_veci_t *c)
{
	register char *pa,*pe,*pe2;
	signed char*p2;
	int *p3;
	int row=b->row;
	int col=b->col;
	int i,t,t1,t2,t3,t4;
	int col2;

	p3=c->p;
	p2=b->p;
	pa=a->p;
	col2=(col>>2)<<2;
	pe=pa+col;
	pe2=pa+col2;
	for(i=0;i<row;++i)
	{
		pa=a->p;
		t1=t2=t3=t4=0;
		while(pa<pe2)
		{
			t1+=pa[0]*p2[0];
			t2+=pa[1]*p2[1];
			t3+=pa[2]*p2[2];
			t4+=pa[3]*p2[3];
			pa+=4;
			p2+=4;
		}
		t=t1+t2+t3+t4;
		while(pa<pe)
		{
			t+=*(pa++)*(*(p2++));
		}
		*(p3++)=t;
	}
}

int wtk_qlas_mats_mul_row_asm_value( short *p1,short *p2,int col)
{
	int i,t;

	t=0;
	for(i=0;i<col;++i)
	{
		t+=p1[i]*p2[i];
	}
	return t;
}

void wtk_qlas_matb_mul_raw(wtk_vecb_t *a,wtk_matb_t *b,wtk_veci_t *c)
{
	char *p1;
	signed char*p2;
	int *p3;
	int row=b->row;
	int col=b->col;
	int i,j,t;

	p1=a->p;
	p3=c->p;
	p2=b->p;
	for(i=0;i<row;++i)
	{
		t=0;
		for(j=0;j<col;++j)
		{
			t+=p1[j]*p2[j];
		}
		*(p3++)=t;
		p2+=col;
	}
}

#ifdef USE_NEON_ASM
//#define wtk_qlas_mats_mul wtk_qlas_mats_mul_neon32
//#define wtk_qlas_matb_mul wtk_qlas_matb_mul_neon32
//#define wtk_qlas_mats_mul wtk_qlas_mat_cache_doverflow
#define wtk_qlas_mats_mul wtk_qlas_mat_cache_doverflow4
//#define wtk_qlas_mats_mul wtk_qlas_mats_mul_c
#define wtk_qlas_matb_mul wtk_qlas_matb_mul_c
#else
#ifdef USE_NEON
#define wtk_qlas_mats_mul wtk_qlas_mats_mul_neon
#define wtk_qlas_matb_mul wtk_qlas_matb_mul_c
#else
#define wtk_qlas_mats_mul wtk_qlas_mats_mul_c
#define wtk_qlas_matb_mul wtk_qlas_matb_mul_c
#endif
#endif


int wtk_qlas_bytes(wtk_qlas_t *qlas)
{
	int bytes;
	wtk_queue_node_t *qn;
	wtk_qlas_layer_t *l;
	int i;

	bytes=sizeof(wtk_qlas_t);
	bytes+=qlas->cfg->transf->bias->len*qlas->cfg->cache_size;
	if(qlas->cfg->use_fix)
	{
		if(qlas->cfg->use_char)
		{
			bytes+=qlas->cfg->transf->bias->len*sizeof(float);
			bytes+=qlas->cfg->transf->bias->len*qlas->cfg->cache_size*sizeof(char);
		}else
		{
			bytes+=qlas->cfg->transf->bias->len*qlas->cfg->cache_size*sizeof(short)+qlas->cfg->cache_size*sizeof(float);
		}
		bytes+=qlas->cfg->layer_q.length*sizeof(wtk_qlas_fix_t);
		for(i=0,qn=qlas->cfg->layer_q.pop;i<qlas->cfg->layer_q.length;++i,qn=qn->next)
		{
			l=data_offset(qn,wtk_qlas_layer_t,q_n);
			if(qlas->cfg->use_char)
			{
				bytes+=l->bias->len*qlas->cfg->cache_size*sizeof(char);
			}else
			{
				bytes+=l->bias->len*qlas->cfg->cache_size*sizeof(short)+qlas->cfg->cache_size*sizeof(float);
			}
			bytes+=l->bias->len*qlas->cfg->cache_size*sizeof(int);
			if(l->type==wtk_fnn_softmax)
			{
				bytes+=l->bias->len*qlas->cfg->cache_size*sizeof(float);
			}else
			{
			}
			//wtk_debug("mul=%p %d/%d\n",qlas->fix_output+i,qlas->fix_output[i].mul->row,qlas->fix_output[i].mul->col);
		}
	}else
	{
		bytes+=qlas->cfg->layer_q.length*sizeof(wtk_vecf_t*);
		for(i=0,qn=qlas->cfg->layer_q.pop;i<qlas->cfg->layer_q.length;++i,qn=qn->next)
		{
			l=data_offset(qn,wtk_qlas_layer_t,q_n);
			bytes+=l->bias->len*qlas->cfg->cache_size*sizeof(float);
		}
	}
	bytes+=wtk_robin_bytes(qlas->robin);
	return bytes;
}

wtk_qlas_t* wtk_qlas_new(wtk_qlas_cfg_t *cfg,wtk_fnn_t *fnn)
{
	wtk_qlas_t *qlas;
	wtk_queue_node_t *qn;
	wtk_qlas_layer_t *l;
	int i;

	qlas=(wtk_qlas_t*)wtk_malloc(sizeof(wtk_qlas_t));
	qlas->cfg=cfg;
	qlas->fnn=fnn;
	qlas->input_feat=wtk_vecf_new(cfg->transf->bias->len*cfg->cache_size);
	qlas->bias=NULL;
	if(cfg->use_fix)
	{
		if(cfg->use_char)
		{
			qlas->bias=(float*)wtk_calloc(cfg->transf->bias->len,sizeof(float));
			qlas->fixinput.binput=wtk_vecb_new(cfg->transf->bias->len*cfg->cache_size);
		}else
		{
			qlas->fixinput.sinput=wtk_vecs_new_qlas(cfg->transf->bias->len,cfg->cache_size);
		}
		qlas->fix_output=(wtk_qlas_fix_t*)wtk_calloc(cfg->layer_q.length,sizeof(wtk_qlas_fix_t));
		for(i=0,qn=cfg->layer_q.pop;i<cfg->layer_q.length;++i,qn=qn->next)
		{
			l=data_offset(qn,wtk_qlas_layer_t,q_n);
			qlas->fix_output[i].softmax=NULL;
			if(cfg->use_char)
			{
				qlas->fix_output[i].output.boutput=wtk_vecb_new(l->bias->len*cfg->cache_size);
			}else
			{
				//printf("%d %d\n",i,l->bias->len*cfg->cache_size);
				qlas->fix_output[i].output.soutput=wtk_vecs_new_qlas(l->bias->len,cfg->cache_size);
			}
			qlas->fix_output[i].mul=wtk_veci_new(l->bias->len*cfg->cache_size);
			//printf("%d %p %d\n",i,qlas->fix_output[i].mul,qlas->fix_output[i].mul->len);
			if(l->type==wtk_fnn_softmax)
			{
				qlas->fix_output[i].softmax=wtk_vecf_new(l->bias->len*cfg->cache_size);
			}else
			{
				qlas->fix_output[i].softmax=NULL;
			}
			//wtk_debug("mul=%p %d/%d\n",qlas->fix_output+i,qlas->fix_output[i].mul->row,qlas->fix_output[i].mul->col);
			if(!qn->next)
			{
				qlas->last_l=l;
				//d->last_scale=1.0/(l->fix_wb->scale*d->cfg->max_b);
				qlas->last_scale=1.0/(l->fixwin.bwin->scale * l->max_bias);
			}
		}
	}else
	{
		qlas->output=(wtk_vecf_t**)wtk_calloc(cfg->layer_q.length,sizeof(wtk_vecf_t*));
		for(i=0,qn=cfg->layer_q.pop;i<cfg->layer_q.length;++i,qn=qn->next)
		{
			l=data_offset(qn,wtk_qlas_layer_t,q_n);
			qlas->output[i]=wtk_vecf_new(l->bias->len*cfg->cache_size);
		}
	}
	i=fnn->cfg->skip_frame?fnn->cfg->skip_frame*cfg->cache_size:cfg->cache_size;
	qlas->robin=wtk_robin_new(i);
	wtk_qlas_reset(qlas);
	return qlas;
}

void wtk_qlas_delete(wtk_qlas_t *qlas)
{
	wtk_qlas_cfg_t *cfg=qlas->cfg;
	int i;

	if(cfg->use_fix)
	{
		for(i=0;i<cfg->layer_q.length;++i)
		{
			if(cfg->use_char)
			{
				wtk_vecb_delete(qlas->fix_output[i].output.boutput);
			}else
			{
				wtk_vecs_delete(qlas->fix_output[i].output.soutput);
			}
			if(qlas->fix_output[i].mul)
			{
				wtk_veci_delete(qlas->fix_output[i].mul);
			}
			if(qlas->fix_output[i].softmax)
			{
				wtk_vecf_delete(qlas->fix_output[i].softmax);
			}
		}
		wtk_free(qlas->fix_output);
		if(cfg->use_char)
		{
			wtk_vecb_delete(qlas->fixinput.binput);
		}else
		{
			wtk_vecs_delete(qlas->fixinput.sinput);
		}
	}else
	{
		for(i=0;i<qlas->cfg->layer_q.length;++i)
		{
			wtk_vecf_delete(qlas->output[i]);
		}
		wtk_free(qlas->output);
	}
	if(qlas->bias)
	{
		wtk_free(qlas->bias);
	}
	wtk_robin_delete(qlas->robin);
	wtk_vecf_delete(qlas->input_feat);
	wtk_free(qlas);
}

void wtk_qlas_reset(wtk_qlas_t *qlas)
{
	qlas->index=0;
	qlas->last_feat=NULL;
	wtk_robin_reset(qlas->robin);
}

void wtk_qlas_expand_feat(wtk_qlas_t *qlas,float *pf,int index,wtk_feat_t **pv,int npv)
{
	float *f1,*f2;
	int i,j;
	int n;
    int padding =qlas->fnn->cfg->padding_frame;

	n=wtk_vector_size(pv[0]->v);
	//print_float(pv[0]->v+1,10);
	pf=pf+qlas->cfg->transf->bias->len*index;

	for(i=0;i<npv;++i)
	{
		f1=pv[i]->v+1;
		f2=pf+i;
		for(j=0;j<n;++j,f2+=npv)
		{
			*f2=f1[j];
		}
	}
    if(padding>0)
    {
    	memcpy(pf+n*npv,qlas->fnn->padding,padding*sizeof(float));
    }
   // print_float(pf,qlas->cfg->transf->bias->len);
    //exit(0);
}

void wtk_qlas_expand_feat2(wtk_qlas_t *qlas,float *pf,int index,wtk_feat_t **pv,int npv)
{
    float *f1,*f2;
    int i,j;
    int n;//,ntrans;
    int padding =qlas->fnn->cfg->padding_frame;

    n=wtk_vector_size(pv[0]->v);
    pf=pf+qlas->cfg->transf->bias->len*index;
    f2=pf;

    for(i=0;i<npv;++i)
    {
		f1=pv[i]->v+1;
        for(j=0;j<n;++j,f2+=1)
        {
            *f2=f1[j];
        }
    }
    if(padding>0)
    {
        memcpy(pf+n*npv,qlas->fnn->padding,padding*sizeof(float));
    }
}

void wtk_qlas_process_transf(wtk_qlas_t *qlas,wtk_vecf_t *input)
{
	wtk_qlas_trans_t *transf=qlas->cfg->transf;
	float *bias=transf->bias->p;
	float *w=transf->win->p;
	float *pf=input->p;
	float *be;
	int i;

	be=bias+transf->bias->len;
	for(i=0;i<qlas->cfg->cache_size;i++)
	{
		while(bias<be)
		{
			//*pf=(*pf+*(b++))*(*(w++));
			*pf=(*pf)*(*(w++))+*(bias++);
			++pf;
		}
		//pf+=transf->bias->len;
		bias=transf->bias->p;
		w=transf->win->p;
	}
//	while(bias<be)
//	{
//		//*pf=(*pf+*(b++))*(*(w++));
//		*pf=(*pf)*(*(w++))+*(bias++);
//		++pf;
//	}
}


/**
 *   |x*N|*|y*N|=|x*y|
 * c=a*b
 */
void wtk_matf_mul_add_raw(wtk_vecf_t *a,wtk_matf_t *b,wtk_vecf_t *c,wtk_vecf_t *bias)
{
	register float t;
	register float *pf1,*pf2,*pf3,*pf4;
	int j,k;
	int row=b->row;
	int col=b->col;

	pf1=a->p;
	pf3=c->p;
	pf4=bias->p;
	pf2=b->p;
	for(j=0;j<row;++j)
	{
		t=pf4[j];
		for(k=0;k<col;++k)
		{
			t+=pf1[k]*(*(pf2++));
			//wtk_debug("v[%d]=%f/%f/%f\n",k,pf1[k],*(pf2-1),t-pf4[j]);
		}
		//exit(0);
		*(pf3++)=t;
	}
}

void wtk_matf_mul_add(wtk_vecf_t *a,wtk_matf_t *b,wtk_vecf_t *c,wtk_vecf_t *bias)
{
	register float t1,t2,t3,t4;
	register float *pa,*pe2;
	float t;
	float *pf2,*pf3,*pf4,*pe;
	int j;
	int row=b->row;
	int col=b->col;
	int col2;

	pf3=c->p;
	pf4=bias->p;
	pf2=b->p;
	col2=(col>>2)<<2;
	pa=a->p;
	pe2=pa+col2;
	pe=pa+col;
	for(j=0;j<row;++j)
	{
		t1=t2=t3=t4=0;
		pa=a->p;
		while(pa<pe2)
		{
			t1+=pa[0]*pf2[0];
			t2+=pa[1]*pf2[1];
			t3+=pa[2]*pf2[2];
			t4+=pa[3]*pf2[3];
			pa+=4;
			pf2+=4;
		}
		if(pa<pe)
		{
			t=pf4[j]+t1+t2+t3+t4;
			while(pa<pe)
			{
				t+=*(pa++)*(*(pf2++));
			}
			*(pf3++)=t;
		}else
		{
			*(pf3++)=pf4[j]+t1+t2+t3+t4;
		}
	}
}

void wtk_matf_mul_add2(wtk_matf_t *a,wtk_matf_t *b,wtk_matf_t *c,wtk_vecf_t *bias)
{
	float t1,t2,t3,t4;
	float t;
	register float *pa,*pae,*pae2,*pf2;
	float *pf1,*pf3,*pf4;
	int i,j;
	int row=b->row;
	int col=b->col;
	int col2;

	c->row=a->row;
	pf1=a->p;
	pf3=c->p;
	pf4=bias->p;
	col2=(col>>2)<<2;
	for(i=0;i<a->row;++i)
	{
		pf2=b->p;
		pae=pf1+col;
		pae2=pf1+col2;
		for(j=0;j<row;++j)
		{
			pa=pf1;
			t1=t2=t3=t4=0;
			while(pa<pae2)
			{
				t1+=pa[0]*pf2[0];
				t2+=pa[1]*pf2[1];
				t3+=pa[2]*pf2[2];
				t4+=pa[3]*pf2[3];
				pa+=4;
				pf2+=4;
			}
			t=pf4[j]+t1+t2+t3+t4;
			while(pa<pae)
			{
				t+=*(pa++)*(*(pf2++));
			}
			*(pf3++)=t;
		}
		pf1=pae;
	}
}

void wtk_qlas_sigmoid(wtk_vecf_t *m)
{
    int col,j;
    float *pf;

    pf=m->p;
    col=m->len;
	for(j=0;j<col;++j)
	{
		*pf=1.0/(1.0+expf(-*pf));
		++pf;
	}
}

void wtk_dnn_output_debug(float *p,int n);

void wtk_qlas_process_dnn_layer(wtk_qlas_t *qlas,wtk_qlas_layer_t *l,wtk_vecf_t *input,wtk_vecf_t *output)
{
	wtk_matf_mul_add(input,l->win,output,l->bias);
	switch(l->type)
	{
	case wtk_fnn_sigmoid:
		wtk_qlas_sigmoid(output);
		//print_float(output->p,output->row*output->col);
		//exit(0);
		break;
	case wtk_fnn_softmax:
		if(!qlas->fnn->cfg->use_linear_output)
		{
			wtk_softmax(output->p,output->len);
			wtk_dnn_output_debug(output->p,output->len);
			//exit(0);
		}
		break;
	case wtk_fnn_linear:
		break;
	default:
        wtk_debug("layer->type not in list. %d\n", l->type);
        break;
	}
}

void wtk_qlas_raise_dnn(wtk_qlas_t *d,wtk_vecf_t *output)
{
	wtk_robin_t *r=d->robin;
	wtk_feat_t *f;
    int skip_frame;
   float *pf;
   int nx;

   nx=output->len/(d->cfg->cache_size);
   //nx=output->len*sizeof(float);
   pf=output->p;
   skip_frame=d->fnn->cfg->skip_frame;

	while (r->used > 0)
	{
		f = wtk_robin_pop(r);
		if (!f)
		{
			break;
		}
		--f->used;
		if( 0 == skip_frame || ( f->index%(skip_frame)==1 ))
		{
			//wtk_debug("%f/%f\n",pf[0],pf[1]);
			memcpy(&(f->dnn_v[1]),pf, nx*sizeof(float));
			if(d->last_feat)
			{
				--d->last_feat->used;
				wtk_fextra_push_feature(d->fnn->parm,d->last_feat);
			}
			++f->used;
			d->last_feat=f;
			f->app_hook=NULL;
		}else
		{
			++d->last_feat->used;
			f->app_hook=d->last_feat;
		}
		wtk_fnn_raise_feature(d->fnn, f);
		pf+=nx;
	}
}

void wtk_qlas_process_matf_raw(wtk_qlas_t *qlas,wtk_vecf_t *input)
{
	wtk_queue_node_t *qn;
	wtk_qlas_layer_t *l;
	wtk_vecf_t *output=NULL;
	int i;

	wtk_qlas_process_transf(qlas,input);
	for(i=0,qn=qlas->cfg->layer_q.pop;qn;qn=qn->next,++i)
	{
		l=data_offset(qn,wtk_qlas_layer_t,q_n);
		output=qlas->output[i];
		wtk_qlas_process_dnn_layer(qlas,l,input,output);
		input=output;
	}
	wtk_qlas_raise_dnn(qlas,output);
}


void wtk_qlas_sigmoid_fix(wtk_mati_t *input,float scale,wtk_vecf_t *bias,wtk_matb_t *output,float max_b)
{
    int row, col;
    int i, j;
    int *pf;
    signed char *pb;
    float f;
    float *pf2;

    output->row=input->row;
    pf=input->p;
    pb=output->p;
    row=input->row;
    col=input->col;
	pf2=bias->p;
	output->scale=1.0/max_b;
    for(i=0;i<row;++i)
    {
    	for(j=0;j<col;++j)
    	{
    		f=*(pf++)*scale+pf2[j];
    		*(pb++)=max_b/(1.0+expf(-f));
//    		if(j<10)
//    		{
//    			wtk_debug("v[%d]=%f\n",j,1.0/(1.0+expf(-f)));
//    		}
    	}
    	//exit(0);
    }
}

//double wtk_fast_exp(double y)
//{
//#define EXP_A (1048576/M_LN2)
//#define EXP_C 60801
//	union {
//		double d;
//		struct {
//			int j, i;
//		} n;
//	} d2i;
//
//	d2i.n.j=0;
//	d2i.n.i = EXP_A*(y)+(1072693248-EXP_C);
//	return d2i.d;
//}

void wtk_qlas_softmax_linear(wtk_veci_t *input,float scale,wtk_vecf_t *bias,wtk_vecf_t *output)
{
    int  col;
    int j;
    int *pf;
    float *pb;
    float *pf2;

    pf=input->p;
    pb=output->p;
    col=input->len;
	pf2=bias->p;
	for(j=0;j<col;++j)
	{
		//wtk_debug("%f/%f\n",*(pf++)*scale,pf2[j]);
		*(pb++)=*(pf++)*scale+pf2[j];
		//exit(0);
	}
}

void wtk_qlas_softmax_linear2(wtk_veci_t *input,float* c_scale,float w_scale,wtk_vecf_t *bias,wtk_vecf_t *output,int c_size)
{
    int  col;
    int i,j;
    int *pf;
    float *pb;
    float *pf2;
    float scale;

    pf=input->p;
    pb=output->p;
    col=input->len/c_size;
	pf2=bias->p;

	for(i=0;i<c_size;++i)
	{
		scale=(*(c_scale+i))*w_scale;
		for(j=0;j<col;++j)
		{
			//wtk_debug("%f/%f\n",*(pf++)*scale,pf2[j]);
			*(pb++)=*(pf++)*scale+pf2[j];
			//exit(0);
		}
		pf2=bias->p;
	}
}

void wtk_qlas_softmax(float* a,int len)
{
	float max,sum;
	float *p,*e;
	int i;

	max=a[0];
	for(i=1;i<len;++i)
	{
		if(a[i]>max)
		{
			max=a[i];
		}
	}
	sum=0;
	p=a;e=p+len;
	while(p<e)
	{
		*p=expf(*p-max);
		sum+=*p;
		++p;
	}
	sum=-log(sum);
	p=a;e=p+len;
	while(p<e)
	{
		*p=log(*p)+sum;
		++p;
	}
}

void wtk_qlas_softmax2(float* a,int len,int c_size)
{
	float max,sum;
	float *p,*e,*m;
	int i,j,l;

	l=len/c_size;

	for(j=0;j<c_size;++j)
	{
		m=a+j*l;
		max=*m;
		for(i=1;i<l;++i)
		{
			if(*(m+i)>max)
			{
				max=*(m+i);
			}
		}
		sum=0;
		p=a+j*l;e=p+l;
		while(p<e)
		{
			*p=expf(*p-max);
			sum+=*p;
			++p;
		}
		sum=-log(sum);
		p=a+j*l;e=p+l;
		while(p<e)
		{
			*p=log(*p)+sum;
			++p;
		}
	}
}

void wtk_qlas_mats_mul_raw(wtk_vecs_t *a,wtk_mats_t *b,wtk_veci_t *c)
{
	short *p1,*p2;
	int *p3;
	int i,j;
	int row=b->row;
	int col=b->col;
	int t;

	p1=a->p;
	p3=c->p;
	p2=b->p;
	for(i=0;i<row;++i)
	{
		t=0;
		for(j=0;j<col;++j)
		{
			t+=p1[j]*(*(p2++));
		}
		*(p3++)=t;
	}
}

void wtk_qlas_sigmoid_fix_short(wtk_veci_t *input,float* c_scale,float w_scale,wtk_vecf_t *bias,wtk_vecs_t *output,float max_b,int c_size)
{
    int col;
    int i,j;
    int *pf;
    short *pb;
    float f,scale;
    float *pf2;

    pf=input->p;
    pb=output->p;
    col=input->len/c_size;
	pf2=bias->p;
	//output->scale=1.0/max_b;
	for(i=0;i<c_size;++i)
	{
		scale=(*(c_scale+i))*w_scale;
		for(j=0;j<col;++j)
		{
			f=*(pf++)*scale+pf2[j];
			*(pb++)=max_b/(1.0+expf(-f));
		}
		pf2=bias->p;
		*(output->c_scale+i)=1.0/max_b;
	}
	//exit(0);
}

void wtk_qlas_sigmoid_normalize_fix_short(wtk_veci_t *input,float* c_scale,
		float w_scale,wtk_vecf_t *bias,wtk_vecs_t *output,float max_b,int c_size,wtk_vecf_t *tmp)
{
    int col;
    int i,j;
    int *pf;
    short *pb;
    float f,scale,sum,alpha,y,t,max;
    float *pf2;
    float *pf3;

    pf=input->p;
    pb=output->p;
    col=input->len/c_size;
    alpha=1.0/col;
	//output->scale=1.0/max_b;
	for(i=0;i<c_size;++i)
	{
		pf2=bias->p;
		pf3=tmp->p;
		max=sum=0.0;

		scale=(*(c_scale+i))*w_scale;
		for(j=0;j<col;++j)
		{
			f=*(pf++)*scale+pf2[j];
			*(pf3++)=1/(1.0+expf(-f));
		}
        pf3=tmp->p;

        for(j=0;j<col;++j)
        {
            sum+=*(pf3+j)*(*(pf3+j));
        }
        //wtk_debug("%f\n",sum*alpha);
        y=pow(sum*alpha,-0.5);
        //wtk_debug("%f\n",y);
        pf3=tmp->p;
        for(j=0;j<col;++j)
        {
            f = *(pf3+j)*y;
            t=f>0?f:-f;
            *(pf3+j)=f;
            if(t>max)
            {
                max=t;
            }
            //wtk_debug("%f\n",*(pf3+j));
        }

        *(output->c_scale+i)=max/max_b;
        pf3=tmp->p;
        for(j=0;j<col;++j)
        {
            *(pb++)=*(pf3++)*max_b/max;
        }
	}
	//exit(0);
}

void wtk_qlas_linear_fixshort(wtk_veci_t *input,float* c_scale,float w_scale,wtk_vecf_t *bias,float max_bias,wtk_vecs_t *output,float max_b,int c_size)
{
	short *pb;
	int *pi;
	int i,j,n=input->len/c_size;
	int max=0,t;
	float f,max_value,f2,scale;
	float *pf;

	for(j=0;j<c_size;j++)
	{
		pf=bias->p;
		pb=output->p+j*n;
		pi=input->p+j*n;
		scale=(*(c_scale+j)*w_scale);
		max=pi[0]>0?pi[0]:-pi[0];
		for(i=1;i<n;++i)
		{
			t=*(pi+i)>0?*(pi+i):-(*(pi+i));
			if(t>max)
			{
				max=t;
			}
		}
		max_value=max*scale+max_bias;
		f=scale*max_b/max_value;
		*(output->c_scale+j)=max_value/max_b;
		f2=max_b/max_value;
		for(i=0;i<n;++i)
		{
			pb[i]=pi[i]*f+pf[i]*f2;
		}
	}
}

void wtk_qlas_process_dnn_layer_fix_short(wtk_qlas_t *qlas,wtk_qlas_layer_t *l,wtk_vecs_t *input,wtk_qlas_fix_t *output)
{
	wtk_mats_t *fixwin=l->fixwin.swin;

	//wtk_debug("output=%d/%d\n",output->mul->row,output->mul->col);
	wtk_qlas_mats_mul(input,fixwin,output->mul);

	switch(l->type)
	{
	case wtk_fnn_sigmoid:
		wtk_qlas_sigmoid_fix_short(output->mul,input->c_scale,fixwin->scale,l->bias,output->output.soutput,qlas->cfg->max_w,qlas->cfg->cache_size);
		break;
	case wtk_fnn_sigmoid_normal:
		wtk_qlas_sigmoid_normalize_fix_short(output->mul,input->c_scale,fixwin->scale,l->bias,output->output.soutput,qlas->cfg->max_w,qlas->cfg->cache_size,l->tmp);
		break;
	case wtk_fnn_softmax:
		//wtk_debug("scale=%f/%f max=%f\n",input->scale,l->cwin->scale,1.0/qlas->cfg->max_w);
		wtk_qlas_softmax_linear2(output->mul,input->c_scale,fixwin->scale,l->bias,output->softmax,qlas->cfg->cache_size);
		if(!qlas->fnn->cfg->use_linear_output)
		{
			wtk_qlas_softmax2(output->softmax->p,output->softmax->len,qlas->cfg->cache_size);
		}
		break;
	case wtk_fnn_linear:
		wtk_qlas_linear_fixshort(output->mul,input->c_scale,fixwin->scale,l->bias,l->max_bias,output->output.soutput,qlas->cfg->max_w,qlas->cfg->cache_size);
		break;
	default:
        wtk_debug("layer->type not in list. %d\n", l->type);
        break;
	}
}


void wtk_qlas_process_matf_fix_short(wtk_qlas_t *qlas,wtk_vecf_t *input)
{
	wtk_queue_node_t *qn;
	wtk_qlas_layer_t *l;
	wtk_qlas_fix_t *output=qlas->fix_output;
	wtk_vecs_t *inputi=qlas->fixinput.sinput;
	int i,j;

	wtk_qlas_process_transf(qlas,input);
	j=qlas->cfg->transf->bias->len;
	//wtk_vecs_fix_vecf(inputi,input,qlas->cfg->max_w);
	wtk_vecs_fix_vecf_qlas(inputi,input,qlas->cfg->max_w,j,qlas->cfg->cache_size);

	for(i=0,qn=qlas->cfg->layer_q.pop;qn;qn=qn->next,++i)
	{
		l=data_offset(qn,wtk_qlas_layer_t,q_n);
		//wtk_debug("output=%p %d/%d  %d/%d\n",output,output->output->row,output->output->col,output->mul->row,output->mul->col);
		wtk_qlas_process_dnn_layer_fix_short(qlas,l,inputi,output);
		inputi=output->output.soutput;
		if(qn->next)
		{
			++output;
		}
	}
	wtk_qlas_raise_dnn(qlas,output->softmax);
	//exit(0);
}

void wtk_qlas_linear_fixchar(wtk_veci_t *input,float scale,wtk_vecf_t *bias,float max_bias,wtk_vecb_t *output,float max_b)
{
	char *pb;
	int *pi;
	int i,n=input->len;
	int max=0,t;
	float f,max_value,f2;
	float *pfb=bias->p;

	pb=output->p;
	pi=input->p;
	max=pi[0]>0?pi[0]:-pi[0];
	for(i=1;i<n;++i)
	{
		t=pi[i]>0?pi[i]:-pi[i];
		if(t>max)
		{
			max=t;
		}
	}
	max_value=max*scale+max_bias;
	f=scale*max_b/max_value;
	output->scale=max_value/max_b;
	f2=max_b/max_value;
	for(i=0;i<n;++i)
	{
		pb[i]=pi[i]*f+pfb[i]*f2;
		//wtk_debug("v[%d]=%d,%f   %d,%f\n",i,pi[i],pi[i]*scale,pb[i],pb[i]*output->scale);
	}
}

void wtk_qlas_sigmoid_fix_char(wtk_veci_t *input,float scale,wtk_vecf_t *bias,wtk_vecb_t *output,float max_b)
{
    int col;
    int j;
    int *pf;
    char *pb;
    float f;
    float *pf2;

    pf=input->p;
    pb=output->p;
    col=input->len;
	pf2=bias->p;
	output->scale=1.0/max_b;
	for(j=0;j<col;++j)
	{
		f=*(pf++)*scale+pf2[j];
		*(pb++)=max_b/(1.0+expf(-f));
	}
}

void wtk_qlas_sigmoid_fix_char_normalize(wtk_veci_t *input,float scale,
		wtk_vecf_t *bias,wtk_vecb_t *output,float max_b,wtk_vecf_t *tmp)
{
    int col;
    int j;
    int *pf;
    char *pb;
    float f,max,sum,alpha,y,t;;
    float *pf2;
    float *pf3;

    pf=input->p;
    pb=output->p;
    col=input->len;
	pf2=bias->p;
	pf3=tmp->p;
    max=sum=0.0;
    alpha=1/col;

	for(j=0;j<col;++j)
	{
		f=*(pf++)*scale+pf2[j];
		*(pf3++)=1.0/(1.0+expf(-f));
	}
    pf3=tmp->p;
    alpha=1/col;

    for(j=0;j<col;++j)
    {
        sum+=*(pf3+j)*(*(pf3+j));
    }
    y=pow(sum*alpha,-0.5);

    pf3=tmp->p;
    for(j=0;j<col;++j)
    {
        f = *(pf3+j)*y;
        t=f>0?f:-f;
        *(pf3+j)=f;
        if(t>max)
        {
            max=t;
        }
    }
    output->scale=max/max_b;
    pf3=tmp->p;
    for(j=0;j<col;++j)
    {
        *(pb++)=*(pf3++)*max_b/max;
    }
}

/**
 * reference: https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html  ASM
 */
#ifdef USE_NEON_ASM

int wtk_qlas_matb_mul_row_asm_value(char *p1,signed char *p2,int col)
{
	int t;
	int *p;
	int cnt;

	//printf("p1=%p/%d p2=%p/%d\n",p1,((long)p1)%16,p2,((long)p2)%16);
	p=&t;
	cnt=0;
	asm volatile(
			"veor q5,q5,q5\n"
			"ldr r0,[%[col]]\n"
			"mov r1,%[p1]\n"
			"mov r2,%[p2]\n"
			"1:\n"
			"subs r0,r0,#32\n"
			"blt 2f\n"
			"vld1.8 {q0},[r1]! \n" //16  q2
			"vld1.8 {q1},[r1]! \n" //16  q2
			"vld1.8 {q2},[r2]! \n" //16  q3
			"vld1.8 {q3},[r2]! \n"//16   q4
			"vmull.s8 q4,d0,d4\n"
			"vmlal.s8 q4,d1,d5\n"
			"vmlal.s8 q4,d2,d6\n"
			"vmlal.s8 q4,d3,d7\n"
			"vpadal.s16  q5,q4\n"
			"b 1b\n"
			"2:\n"
			"add r0,r0,#32\n"
			"3:\n"
			"subs r0,r0,#8\n"
			"blt 4f\n"
			"vld1.16 d0,[r1]!\n"
			"vld1.16 d1,[r2]!\n"
			"vmull.s8 q4,d0,d1\n"
			"vpadal.s16  q5,q4\n"
			"b 3b\n"
			"4:\n"
			"add r0,r0,#8\n"
			"vadd.i32 d4,d10,d11\n"
			"vpaddl.s32 d0,d4\n"  //前8位有效
			"mov r1,%[t]\n"
			"vst1.32 {d0[0]},[r1]\n"
			"mov %[cnt],r0\n"
			:[t]"+r"(p),
			 [cnt]"+r"(cnt)
			:[p1]"r"(p1),
			 [p2]"r"(p2),
			 [col]"r"(&col)
			:"memory","cc","r0","r1","r2","r3","q0","q1","q2","q3","q4","q5"
			);
	//t2=p[0]+p[1];
	//printf("v[%d/%d]=%d\n",cnt,col,t);
	if(cnt>0)
	{
		cnt=col-cnt;
		switch(cnt)
		{
		case 1:
			t+=p1[cnt]*p2[cnt];
			break;
		case 2:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1];
			break;
		case 3:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2];
			break;
		case 4:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2]+p1[cnt+3]*p2[cnt+3];
			break;
		case 5:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2]+p1[cnt+3]*p2[cnt+3]+p1[cnt+4]*p2[cnt+4];
			break;
		case 6:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2]+p1[cnt+3]*p2[cnt+3]+p1[cnt+4]*p2[cnt+4]+p1[cnt+5]*p2[cnt+5];
			break;
		case 7:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2]+p1[cnt+3]*p2[cnt+3]+p1[cnt+4]*p2[cnt+4]+p1[cnt+5]*p2[cnt+5]+p1[cnt+6]*p2[cnt+6];
			break;
		default:
			for(;cnt<col;++cnt)
			{
				t+=p1[cnt]*p2[cnt];
			}
			break;
		}
	}
	return t;
}


int wtk_qlas_matb_mul_row_asm_value3(char *p1,signed char *p2,int col)
{
	int t;
	int *p;
	int cnt;

	p=&t;
	cnt=0;
	asm volatile(
			"veor q8,q8,q8\n"
			"ldr r0,[%[col]]\n"
			"mov r1,%[p1]\n"
			"mov r2,%[p2]\n"
			"1:\n"
			"subs r0,r0,#32\n"
			"blt 2f\n"
			"vld1.8 {q2},[r1]! \n" //16  q2
			"vld1.8 {q3},[r1]! \n" //16  q3
			"vld1.8 {q4},[r2]! \n"//16   q4
			"vld1.8 {q5},[r2]! \n"//16   q5
			"vmull.s8 q6,d4,d8\n"
			"vpadal.s16  q8,q6\n"
			"vmull.s8 q6,d5,d9\n"
			"vpadal.s16  q8,q6\n"
			"vmull.s8 q6,d6,d10\n"
			"vpadal.s16  q8,q6\n"
			"vmull.s8 q6,d7,d11\n"
			"vpadal.s16  q8,q6\n"
			"b 1b\n"
			"2:\n"
			"add r0,r0,#32\n"
			"3:\n"
			"subs r0,r0,#8\n"
			"blt 4f\n"
			"vld1.16 d2,[r1]!\n"
			"vld1.16 d3,[r2]!\n"
			"vmull.s8 q6,d2,d3\n"
			"vpadal.s16  q8,q6\n"
			"b 3b\n"
			"4:\n"
			"add r0,r0,#8\n"
			"vadd.i32 d4,d16,d17\n"
			"vpaddl.s32 d0,d4\n"  //前8位有效
			"mov r1,%[t]\n"
			"vst1.32 {d0[0]},[r1]\n"
			"mov %[cnt],r0\n"
			:[t]"+r"(p),
			 [cnt]"+r"(cnt)
			:[p1]"r"(p1),
			 [p2]"r"(p2),
			 [col]"r"(&col)
			:"memory","cc","r0","r1","r2","r3","q2","q3","q4","q5","q6","q7","q8"
			);
	//t2=p[0]+p[1];
	//printf("v[%d/%d]=%d\n",cnt,col,t);
	if(cnt>0)
	{
		cnt=col-cnt;
		switch(cnt)
		{
		case 1:
			t+=p1[cnt]*p2[cnt];
			break;
		case 2:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1];
			break;
		case 3:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2];
			break;
		case 4:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2]+p1[cnt+3]*p2[cnt+3];
			break;
		case 5:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2]+p1[cnt+3]*p2[cnt+3]+p1[cnt+4]*p2[cnt+4];
			break;
		case 6:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2]+p1[cnt+3]*p2[cnt+3]+p1[cnt+4]*p2[cnt+4]+p1[cnt+5]*p2[cnt+5];
			break;
		case 7:
			t+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2]+p1[cnt+3]*p2[cnt+3]+p1[cnt+4]*p2[cnt+4]+p1[cnt+5]*p2[cnt+5]+p1[cnt+6]*p2[cnt+6];
			break;
		default:
			for(;cnt<col;++cnt)
			{
				t+=p1[cnt]*p2[cnt];
			}
			break;
		}
	}
	return t;
}

int wtk_qlas_matb_mul_row_asm_value1(char *p1,signed char *p2,int col)
{
	int t;
	char *p;
	int t2,cnt;

	//128/8 => 16*4
	asm volatile(
			"veor q6,q6,q6\n"
			"veor q5,q5,q5\n"
			"ldr r0,[%[col]]\n"
			"mov r1,%[p1]\n"
			"mov r2,%[p2]\n"
			"mov r3,%[t]\n"
			"1:\n"
			"subs r0,r0,#32\n"
			"blt 2f\n"
			"vld1.8 {d2,d3},[r1]! \n" //16  q1
			"vld1.8 {d4,d5},[r1]! \n" //16  q2
			"vld1.8 {d6,d7},[r2]! \n"//16   q3
			"vld1.8 {d8,d9},[r2]! \n"//16   q4
			"vmla.i8 q5,q1,q3\n"   //16 q5
			"vmla.i8 q6,q2,q4\n"  //17 q6
			"b 1b\n"
			"2:\n"
			"add r0,r0,#32\n"
			"3:\n"
			"subs r0,r0,#8\n"
			"blt 4f\n"
			"vld1.8 d2,[r1]!\n"
			"vld1.8 d3,[r2]!\n"
			"vmla.i8 d10,d2,d3\n "
			"b 3b\n"
			"4:\n"
			"add r0,r0,#8\n"
			"vadd.i8 q5,q5,q6\n"
			"vpadd.i8 d1,d10,d11 \n"
			"vpadd.i8 d0,d1,d1\n"  //前8位有效
			"vst1.8 {d0[0]},[r3]\n"
			"mov %[cnt],r0\n"
			:[t]"=r"(t),
			 [cnt]"=r"(cnt)
			:[p1]"r"(p1),
			 [p2]"r"(p2),
			 [col]"r"(&col)
			:"memory","cc","r0","r1","r2","r3","q1","q2","q3","q4","q5","q6","q7"
			);
	p=(char*)&(t);
	t2=p[0]+p[1]+p[2]+p[3];
	//printf("cnt=%d\n",cnt);
	if(cnt>0)
	{
		cnt=col-cnt;
		switch(cnt)
		{
		case 1:
			t2+=p1[cnt]*p2[cnt];
			break;
		case 2:
			t2+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1];
			break;
		case 3:
			t2+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2];
			break;
		case 4:
			t2+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2]+p1[cnt+3]*p2[cnt+3];
			break;
		case 5:
			t2+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2]+p1[cnt+3]*p2[cnt+3]+p1[cnt+4]*p2[cnt+4];
			break;
		case 6:
			t2+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2]+p1[cnt+3]*p2[cnt+3]+p1[cnt+4]*p2[cnt+4]+p1[cnt+5]*p2[cnt+5];
			break;
		case 7:
			t2+=p1[cnt]*p2[cnt]+p1[cnt+1]*p2[cnt+1]+p1[cnt+2]*p2[cnt+2]+p1[cnt+3]*p2[cnt+3]+p1[cnt+4]*p2[cnt+4]+p1[cnt+5]*p2[cnt+5]+p1[cnt+6]*p2[cnt+6];
			break;
		default:
			for(;cnt<col;++cnt)
			{
				t2+=p1[cnt]*p2[cnt];
			}
			break;
		}
	}
	return t2;
}
#else
int wtk_qlas_matb_mul_row_asm_value(char *p1,signed char *p2,int col)
{
	int i,t;

	t=0;
	for(i=0;i<col;++i)
	{
		t+=p1[i]*p2[i];
	}
	return t;
}
#endif

int wtk_qlas_matb_mul_row_value(char *p1,signed char *p2,int col)
{
	int i,t;

	t=0;
	for(i=0;i<col;++i)
	{
		t+=p1[i]*p2[i];
	}
	return t;
}


void wtk_qlas_process_dnn_layer_fix_char(wtk_qlas_t *qlas,wtk_qlas_layer_t *l,wtk_vecb_t *input,wtk_qlas_fix_t *output)
{
	wtk_matb_t *fixwin=l->fixwin.bwin;

	//wtk_debug("========== 123 len=%d %p/%p/%p  ===============\n",input->len,input->p,fixwin->p,output->mul->p);
	if(l->type==wtk_fnn_linear)
	{
		wtk_qlas_matb_mul(input,fixwin,output->mul);
	}else
	{
		wtk_qlas_matb_mul(input,fixwin,output->mul);
	}
	switch(l->type)
	{
	case wtk_fnn_sigmoid:
		//wtk_qlas_sigmoid_fix(output->mul,input->scale*l->cwin->scale,l->bias,output->output,qlas->cfg->max_w);
		wtk_qlas_sigmoid_fix_char(output->mul,input->scale*fixwin->scale,l->bias,output->output.boutput,qlas->cfg->max_w);
		//print_float(output->p,output->row*output->col);
		break;
	case wtk_fnn_sigmoid_normal:
		//wtk_qlas_sigmoid_fix(output->mul,input->scale*l->cwin->scale,l->bias,output->output,qlas->cfg->max_w);
		wtk_qlas_sigmoid_fix_char_normalize(output->mul,input->scale*fixwin->scale,l->bias,output->output.boutput,qlas->cfg->max_w,l->tmp);
		//print_float(output->p,output->row*output->col);
		break;
	case wtk_fnn_softmax:
		//wtk_debug("scale=%f/%f max=%f\n",input->scale,l->cwin->scale,1.0/qlas->cfg->max_w);
		wtk_qlas_softmax_linear(output->mul,input->scale*fixwin->scale,l->bias,output->softmax);
		//print_float(output->softmax->p,output->softmax->row*output->softmax->col);
		if(!qlas->fnn->cfg->use_linear_output)
		{
			wtk_qlas_softmax(output->softmax->p,output->softmax->len);
		}
		//print_float(output->softmax->p,output->softmax->row*output->softmax->col);
		break;
	case wtk_fnn_linear:
		wtk_qlas_linear_fixchar(output->mul,input->scale*fixwin->scale,l->bias,l->max_bias,output->output.boutput,qlas->cfg->max_w);
		break;
	default:
        wtk_debug("layer->type not in list. %d\n", l->type);
        break;
	}
}


#ifdef USE_NEONX
void wtk_qlas_process_transf_fixchar(wtk_qlas_t *qlas,wtk_vecf_t *input,wtk_vecb_t *inputi)
{
	wtk_qlas_trans_t *transf=qlas->cfg->transf;
	float *w=transf->win->p;
	float *pf=input->p;
	float *pe;
	float t,f;
	float max=1e-10;
	int i,len;
	char *pc=inputi->p;
	float *df=qlas->bias;

	len=transf->bias->len;
	memcpy(df,transf->bias->p,len*sizeof(float));
	i=len>>2;
	//float32x4_t vmlaq_f32(float32x4_t a, float32x4_t b, float32x4_t c); // VMLA.F32 q0,q0,q0
	asm volatile(
			"ldr r0,[%[i]]\n"
			"mov r1,%[pf]\n"
			"mov r2,%[w]\n"
			"mov r3,%[df]\n"
			"1:\n"
			"subs r0,r0,#4\n"
			"blt 2f\n"
			"vld1.8 {q0},[r1]! \n" //16  q2
			"vld1.8 {q1},[r2]! \n" //16  q3
			"vld1.8 {q2},[r3] \n" //16  q3
			"vmla.f32 q2,q0,q1\n"
			"vst1.32 {q2},[r3]!\n"
			"vld1.8 {q0},[r1]! \n" //16  q2
			"vld1.8 {q1},[r2]! \n" //16  q3
			"vld1.8 {q2},[r3] \n" //16  q3
			"vmla.f32 q2,q0,q1\n"
			"vst1.32 {q2},[r3]!\n"
			"vld1.8 {q0},[r1]! \n" //16  q2
			"vld1.8 {q1},[r2]! \n" //16  q3
			"vld1.8 {q2},[r3] \n" //16  q3
			"vmla.f32 q2,q0,q1\n"
			"vst1.32 {q2},[r3]!\n"
			"vld1.8 {q0},[r1]! \n" //16  q2
			"vld1.8 {q1},[r2]! \n" //16  q3
			"vld1.8 {q2},[r3] \n" //16  q3
			"vmla.f32 q2,q0,q1\n"
			"vst1.32 {q2},[r3]!\n"
			"b 1b\n"
			"2:\n"
			"add r0,r0,#4\n"
			"3:\n"
			"subs r0,r0,#1\n"
			"blt 4f\n"
			"vld1.8 {q0},[r1]! \n" //16  q2
			"vld1.8 {q1},[r2]! \n" //16  q3
			"vld1.8 {q2},[r3] \n" //16  q3
			"vmla.f32 q2,q0,q1\n"
			"vst1.32 {q2},[r3]!\n"
			"b 3b\n"
			"4:\n"
			:[df]"+r"(df)
			:[w]"r"(w),
			 [pf]"r"(pf),
			 [i]"r"(&i)
			:"memory","cc","r0","r1","r2","r3","q0","q1","q2"
			);
	i=i<<2;
	if(i<len)
	{
		w+=i;
		pf+=i;
		pe=pf+len;
		while(pf<pe)
		{
			*(df++)+=(*pf)*(*(w++));
		}
	}
	//使用bias float;
	pf=df;
	max=wtk_float_abs_max(pf,len);
	t=qlas->cfg->max_w/max;
	inputi->scale=1.0/t;
	for(i=0;i<len;++i)
	{
		f=pf[i]*t;
		pc[i]=wtk_float_round(f);
	}
}
#else
void wtk_qlas_process_transf_fixchar(wtk_qlas_t *qlas,wtk_vecf_t *input,wtk_vecb_t *inputi)
{
	wtk_qlas_trans_t *transf=qlas->cfg->transf;
	float *bias=transf->bias->p;
	float *w=transf->win->p;
	float *pf=input->p;
	float *be;
	float t,f;
	float max=1e-10;
	int i,len;
	char *pc=inputi->p;

	len=transf->bias->len;
	be=bias+len;
	while(bias<be)
	{
		//*pf=(*pf+*(b++))*(*(w++));
		t=(*pf)*(*(w++))+*(bias++);
		*(pf++)=t;
		t=t>0?t:-t;
		if(t>max)
		{
			max=t;
		}
	}
	t=qlas->cfg->max_w/max;
	inputi->scale=1.0/t;
	pf=input->p;
	for(i=0;i<len;++i)
	{
		f=pf[i]*t;
		pc[i]=wtk_float_round(f);
	}
}
#endif



void wtk_qlas_process_matf_fix_char(wtk_qlas_t *qlas,wtk_vecf_t *input)
{
	wtk_queue_node_t *qn;
	wtk_qlas_layer_t *l;
	wtk_qlas_fix_t *output=qlas->fix_output;
	wtk_vecb_t *inputi=qlas->fixinput.binput;
	int i;

	wtk_qlas_process_transf_fixchar(qlas,input,inputi);
	//wtk_vecb_fix_vecf(inputi,input,qlas->cfg->max_w);
	for(i=0,qn=qlas->cfg->layer_q.pop;qn;qn=qn->next,++i)
	{
		l=data_offset(qn,wtk_qlas_layer_t,q_n);
		//wtk_debug("output=%p %d/%d  %d/%d\n",output,output->output->row,output->output->col,output->mul->row,output->mul->col);
		wtk_qlas_process_dnn_layer_fix_char(qlas,l,inputi,output);
		inputi=output->output.boutput;
		if(qn->next)
		{
			++output;
		}
	}
	wtk_qlas_raise_dnn(qlas,output->softmax);
	//exit(0);
}

void wtk_qlas_process_matf(wtk_qlas_t *qlas,wtk_vecf_t *input)
{
	//printf("use_fix=%d use_char=%d\n",qlas->cfg->use_fix,qlas->cfg->use_char);
	if(qlas->cfg->use_fix)
	{
		if(qlas->cfg->use_char)
		{
			wtk_qlas_process_matf_fix_char(qlas,input);
		}else
		{
			wtk_qlas_process_matf_fix_short(qlas,input);
		}
	}else
	{
		wtk_qlas_process_matf_raw(qlas,input);
	}
}

void wtk_qlas_process_layer(wtk_qlas_t *qlas,wtk_feat_t **pv,int npv,wtk_feat_t *f)
{
	//print_float(f->v,wtk_vector_size(f->v));
	++f->used;
	//printf("hhhhh: %d\n",(qlas->fix_output+2)->mul->len);
	wtk_robin_push(qlas->robin,f);
	//wtk_debug("used=%d/%d\n",d->robin->used,d->robin->nslot);
	//wtk_debug("index=%d skip=%d\n",f->index,d->dnn->cfg->skip_frame);
	//wtk_debug("row=%d col=%d\n",wtk_matrix_rows(d->feat_matrix),wtk_matrix_cols(d->feat_matrix));
	if( (0 == qlas->fnn->cfg->skip_frame) || (f->index%qlas->fnn->cfg->skip_frame==1) )
	{
		//printf("%d\n",npv);
		//printf("hhhhh1: %d\n",(qlas->fix_output+2)->mul->len);
		wtk_qlas_expand_feat(qlas,qlas->input_feat->p,qlas->index,pv,npv);
		//printf("qqqqq2: %d\n",(qlas->fix_output+2)->mul->len);
		++qlas->index;
	}
	if(qlas->robin->nslot==qlas->robin->used)
	{
		//printf("==============\n");
		//wtk_debug("row=%d/%d\n",qlas->input_feat->row,qlas->input_feat->col);
		wtk_qlas_process_matf(qlas,qlas->input_feat);
		qlas->index=0;
	}
}

void wtk_qlas_flush(wtk_qlas_t *f)
{
	if(f->robin->used>0 && f->index>0)
	{
		wtk_qlas_process_matf(f,f->input_feat);
		f->index=0;
	}
}

void wtk_qlas_flush_end(wtk_qlas_t *f)
{
	if(f->last_feat)
	{
		--f->last_feat->used;
		wtk_fextra_push_feature(f->fnn->parm,f->last_feat);
		f->last_feat=NULL;
	}
}

#ifdef USE_ARM_X
float wtk_qlas_get_dnn_value(wtk_qlas_t *d,wtk_feat_t *f,int index)
{
	wtk_dnn_layer_t *l;
	wtk_matc_t *matc;
	int *pi,*pe;
	signed char *pc;
	int t;
	float x;
	int32x4_t fa,fb,fc;
	int buf[4];

	//wtk_debug("index=%d\n",index);
	l=d->last_l;
	matc=l->fix_wb->w.c;
	pi=(int*)(f->dnn_v+1);
	pe=pi+matc->row;
	pc=matc->p+(index-1)*matc->row;
	t=0;
	while(pi<pe-4)
	{
		fa=vld1q_s32(pi);
		buf[0]=pc[0];
		buf[1]=pc[1];
		buf[2]=pc[2];
		buf[3]=pc[3];
		fb=vld1q_s32(buf);
		fc=vmulq_s32(fa,fb);
		vst1q_s32(buf,fc);
		t+=buf[0]+buf[1]+buf[2]+buf[3];
		pi+=4;pc+=4;
	}
	while(pi<pe)
	{
		//wtk_debug("t=%d %d/%d \n",t,*pi,*pc);
		t+=(*(pi++))*(*(pc++));
		//wtk_debug("t=%d\n",t);
	}
	if(l->fix_wb->b)
	{
		t+=l->fix_wb->b->p[index-1];
	}
	x=t*d->last_scale;
	//wtk_debug("t=%d/%f %f b=%d\n",t,d->last_scale,x,(int)sizeof(int));
	return x;
}
#else
float wtk_qlas_get_dnn_value(wtk_qlas_t *d,wtk_feat_t *f,int index)
{
	//wtk_dnn_layer_t *l;
	wtk_qlas_layer_t *l;
	//wtk_matc_t *matc;
	wtk_matb_t *matb;
	int *pi,*pe;
	signed char *pc;
	int t;
	float x;

	//wtk_debug("index=%d\n",index);
	//wtk_vector_print(f->dnn_v+1);
	l=d->last_l;
	//matc=l->fix_wb->w.c;
	matb=l->fixwin.bwin; //?
	pi=(int*)(f->dnn_v+1);
	pe=pi+matb->row;
	pc=matb->p+(index-1)*matb->row;
	t=0;
	while(pi<pe-8)
	{
		//wtk_debug("t=%d %d/%d \n",t,*pi,*pc);
		t+=(*(pi++))*(*(pc++));
		t+=(*(pi++))*(*(pc++));
		t+=(*(pi++))*(*(pc++));
		t+=(*(pi++))*(*(pc++));
		t+=(*(pi++))*(*(pc++));
		t+=(*(pi++))*(*(pc++));
		t+=(*(pi++))*(*(pc++));
		t+=(*(pi++))*(*(pc++));
	}
	while(pi<pe)
	{
		//wtk_debug("t=%d %d/%d \n",t,*pi,*pc);
		t+=(*(pi++))*(*(pc++));
		//wtk_debug("t=%d\n",t);
	}
	if(l->bias)
	//if(l->fix_wb->b)
	{
		//t+=l->fix_wb->b->p[index-1];
		t+=l->bias->p[index-1];
	}
	x=t*d->last_scale;
	//wtk_debug("t=%d/%f %f b=%d\n",t,d->last_scale,x,(int)sizeof(int));
	return x;
}
#endif
