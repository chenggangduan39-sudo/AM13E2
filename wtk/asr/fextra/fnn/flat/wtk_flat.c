#include "wtk_flat.h"
#include "../../wtk_fextra.h"
#include <math.h>
#include "wtk_flat_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/math/wtk_matrix.h"
#include "../wtk_fnn.h"

#define wtk_flat_expf(f) expf(f)
//#define wtk_flat_expf(f) wtk_fast_exp(f)

void wtk_dnn_output_debug2(wtk_matrix_t *output)
{
	wtk_vector_t *v;
	int i;

	v=output[1];
	//wtk_vector_print(v);
	for(i=1;i<=wtk_vector_size(v);++i)
	{
		v[i]=sqrt(-2.0*log(v[i]));
	}
}

void wtk_dnn_output_debug3(wtk_matrix_t *output)
{
	wtk_vector_t *v;
	float *p,*pe;

	v=output[1];
	p=v+1;
	pe=p+wtk_vector_size(v);
	while(p<pe)
	{
		*p=sqrt(-2.0*log(*p));
		//wtk_debug("f=%f\n",*p);
		//exit(0);
		++p;
	}
}

void wtk_dnn_output_debug(float *p,int n)
{
	float *pe;

	pe=p+n;
	while(p<pe)
	{
		*p=log(*p);
		++p;
	}
}


wtk_flat_t* wtk_flat_new(wtk_flat_cfg_t *cfg,wtk_fnn_t *dnn)
{
	wtk_flat_t *d;
	wtk_dnn_layer_t *l;
	wtk_queue_node_t *n;
	int i,col;
	int max_col;

	d=(wtk_flat_t*)wtk_calloc(1,sizeof(*d));
	d->cfg=cfg;
	d->dnn=dnn;
	d->last_feature=NULL;
	d->fix_0_int_input=NULL;
	d->fix_0_char_input=NULL;
	d->fix_0_mul=NULL;
	d->fix_0_char_output=NULL;
	//d->fix_0_uc=NULL;
	d->output_c=NULL;
	if(cfg->use_fix_float)
	{
		d->matrixs=NULL;
		d->fix_tmp=(wtk_mati_t**)wtk_calloc(cfg->layer_q.length,sizeof(wtk_mati_t*));
		if(cfg->use_c)
		{
			d->fix_output.c.fix_output=(wtk_matuc_t**)wtk_calloc(cfg->layer_q.length,sizeof(wtk_matuc_t*));
		}else
		{
			d->fix_output.i.fix_output=(wtk_mati_t**)wtk_calloc(cfg->layer_q.length,sizeof(wtk_mati_t*));
		}
		max_col=0;
		for(i=0,n=cfg->layer_q.pop;n;n=n->next,++i)
		{
			l=data_offset(n,wtk_dnn_layer_t,q_n);
			if(cfg->use_fix_0_cx)
			{
				if(l->fix_wb->uc)
				{
					col=l->fix_wb->uc->uc->row;
				}else
				{
					col=l->fix_wb->w.c->row;
				}
				if(col>max_col)
				{
					max_col=col;
				}
			}
			if(i==0)
			{
				if(cfg->use_fix_0_layer)
				{
					if(cfg->use_fix_0_c && cfg->use_fix_0_cx)
					{
						//wtk_debug("-------xx--------\n");
						//d->fix_0_uc=wtk_matuc_new3(l->fix_wb->w.c,&(d->fix_0_min));
						d->fix_0_char_input=wtk_matc_new(cfg->cache_size,wtk_flat_cfg_in_cols(cfg));
					}else
					{
						d->fix_0_int_input=wtk_mati_new(cfg->cache_size,wtk_flat_cfg_in_cols(cfg));
					}
					if(l->fix_wb->uc)
					{
						col=cfg->use_transpose?l->fix_wb->uc->uc->col:l->fix_wb->uc->uc->row;
					}else
					{
						col=cfg->use_transpose?l->fix_wb->w.c->col:l->fix_wb->w.c->row;
					}
					d->fix_0_mul=wtk_mati_new(cfg->cache_size,col);
					if(cfg->use_c)
					{
						d->fix_0_char_output=wtk_matuc_new(cfg->cache_size,col);
					}else
					{
						d->fix_0_int_output=wtk_mati_new(cfg->cache_size,col);
					}
					//wtk_debug("%p: %d/%d\n",d->fix_0_i,d->fix_0_i->row,d->fix_0_i->col);
				}else
				{
					col=wtk_matrix_cols(l->w);
					//col=wtk_matrix_rows(l->w);
					//wtk_debug("col=%d\n",col);
					//exit(0);
					d->fix_0_output1=wtk_matrix_new2(cfg->cache_size,col);
					if(cfg->use_c)
					{
						d->fix_output.c.fix_0_output2=wtk_matuc_new(cfg->cache_size,col);
						d->fix_output.c.fix_0_output2_i=wtk_mati_new(cfg->cache_size,col);
					}else
					{
						d->fix_output.i.fix_0_output2=wtk_mati_new(cfg->cache_size,col);
					}
				}
			}else
			{
				if(cfg->use_c)
				{
					if(l->fix_wb->uc)
					{
						col=cfg->use_transpose?l->fix_wb->uc->uc->col:l->fix_wb->uc->uc->row;
					}else
					{
						col=cfg->use_transpose?l->fix_wb->w.c->col:l->fix_wb->w.c->row;
					}
					//wtk_debug("col=%d\n",col);
					d->fix_tmp[i-1]=wtk_mati_new(cfg->cache_size,col);//wtk_matrix_cols(l->w));
					d->fix_output.c.fix_output[i-1]=wtk_matuc_new(cfg->cache_size,col);
				}else
				{
					col=l->fix_wb->w.i->col;
					d->fix_tmp[i-1]=wtk_mati_new(cfg->cache_size,col);//wtk_matrix_cols(l->w));
					d->fix_output.i.fix_output[i-1]=wtk_mati_new(cfg->cache_size,col);
				}
				//d->fix_output[i-1]=wtk_mati_new(cfg->cache_size,wtk_matrix_cols(l->w));
			}
			if(!n->next)
			{
				d->last_l=l;
				d->last_scale=1.0/(l->fix_wb->scale*d->cfg->max_b);
			}
		}
		if(max_col>0)
		{
			d->output_c=wtk_matc_new(cfg->cache_size,max_col);
		}
		//d->heap=wtk_mat_heap_new(row,col);
	}else
	{
		d->fix_tmp=NULL;
		d->fix_0_output1=NULL;
		d->matrixs=(wtk_matrix_t**)wtk_calloc(cfg->layer_q.length,sizeof(wtk_matrix_t*));
		for(i=0,n=cfg->layer_q.pop;n;n=n->next,++i)
		{
			l=data_offset(n,wtk_dnn_layer_t,q_n);
			d->matrixs[i]=wtk_matrix_new2(cfg->cache_size,wtk_matrix_cols(l->w));
		}
	}
	//1*348=1*594 * 594*348 =1*348
	d->feat_matrix=wtk_matrix_new2(cfg->cache_size,wtk_flat_cfg_in_cols(cfg));
	i=dnn->cfg->skip_frame?cfg->cache_size*dnn->cfg->skip_frame:cfg->cache_size;
	d->robin=wtk_robin_new(i);
	d->index=0;
	return d;
}

void wtk_flat_delete(wtk_flat_t *d)
{
	int i;

	if(d->output_c)
	{
		wtk_matc_delete(d->output_c);
	}
	wtk_matrix_delete(d->feat_matrix);
	if(d->matrixs)
	{
		for(i=0;i<d->cfg->layer_q.length;++i)
		{
			wtk_matrix_delete(d->matrixs[i]);
		}
	}
	if(d->fix_0_output1)
	{
		wtk_matrix_delete(d->fix_0_output1);
	}
	if(d->fix_0_mul)
	{
		wtk_mati_delete(d->fix_0_mul);
	}
	if(d->fix_0_char_output)
	{
		wtk_matuc_delete(d->fix_0_char_output);
	}
	if(d->fix_0_char_input)
	{
		wtk_matc_delete(d->fix_0_char_input);
	}
	if(d->fix_0_int_input)
	{
		wtk_mati_delete(d->fix_0_int_input);
	}
	if(d->fix_0_int_output)
	{
		wtk_mati_delete(d->fix_0_int_output);
	}
	if(d->cfg->use_fix_float && d->fix_output.c.fix_0_output2)
	{
		if(d->cfg->use_c)
		{
			wtk_matuc_delete(d->fix_output.c.fix_0_output2);
			wtk_mati_delete(d->fix_output.c.fix_0_output2_i);
		}else
		{
			wtk_mati_delete(d->fix_output.i.fix_0_output2);
		}
	}
	if(d->fix_tmp)
	{
		for(i=0;i<d->cfg->layer_q.length-1;++i)
		{
			wtk_mati_delete(d->fix_tmp[i]);
			if(d->cfg->use_c)
			{
				wtk_matuc_delete(d->fix_output.c.fix_output[i]);
			}else
			{
				wtk_mati_delete(d->fix_output.i.fix_output[i]);
			}
		}
		wtk_free(d->fix_tmp);
		//wtk_free(d->fix_output);
		if(d->cfg->use_c)
		{
			wtk_free(d->fix_output.c.fix_output);
		}else
		{
			wtk_free(d->fix_output.i.fix_output);
		}
	}
	wtk_robin_delete(d->robin);
	wtk_free(d->matrixs);
	wtk_free(d);
}

#ifdef USE_SIMD
#include <arm_neon.h>

void wtk_flat_matrix_multi(wtk_matrix_t *m,wtk_matrix_t *a,wtk_matrix_t *b)
{
	int rows=wtk_matrix_rows(m);
	int cols=wtk_matrix_cols(m);
	int ac=wtk_matrix_cols(a);
	int i,k;
	float *pa,*pm;
	register float *tpm,*tpb;
	register float pak;
	register float *e;
	float32x4_t fa,fc,fb;
	//float32x4x4_t fb;

	for(i=1;i<=rows;++i)
	{
		pa=a[i];pm=m[i];
		e=pm+cols;
		for(k=1;k<=ac;++k)
		{
			tpb=b[k];pak=pa[k];
			tpm=pm;
			//wtk_debug("%d/%d=%d\n",i,k,(int)(e-tpm));
			fa=vdupq_n_f32(pak);
			if(k==1)
			{
				while(e-tpm>=4)
				{
					fb=vld1q_f32(tpb+1);
					tpb+=4;
					fc=vmulq_f32(fa,fb);
					vst1q_f32(tpm+1,fc);
					tpm+=4;
				}
				while(tpm<e)
				{
					*(++tpm)=pak*(*(++tpb));
				}
			}else
			{
				while(e-tpm>=4)
				{
					fb=vld1q_f32(tpb+1);
					tpb+=4;
					fc=vld1q_f32(tpm+1);
					fc=vmlaq_f32(fc,fa,fb);
					vst1q_f32(tpm+1,fc);
					tpm+=4;
				}
				while(tpm<e)
				{
					*(++tpm)+=pak*(*(++tpb));
				}
			}
		}
	}
}

#else

void wtk_flat_matrix_multi_trans(wtk_flat_t *f,wtk_matrix_t *m,wtk_matrix_t *a,wtk_matrix_t *b)
{
	register float *pa,*pae,fx,*pb;
	float *tpa;
	float *pm;
	int col,row;
	int i;

	//wtk_debug("a=%d/%d b=%d/%d m=%d/%d\n",wtk_matrix_cols(a),wtk_matrix_rows(a),wtk_matrix_cols(b),wtk_matrix_rows(b),wtk_matrix_cols(m),wtk_matrix_rows(m));
	col=wtk_matrix_cols(a);
	row=wtk_matrix_rows(b);
	tpa=a[1];
	pae=tpa+col;
	pm=m[1];
	for(i=1;i<=row;++i)
	{
		fx=0;
		pa=tpa;
		pb=b[i];
		while(pa<pae)
		{
			fx+=(*(++pa)) * (*(++pb));
		}
		pm[i]=fx;
	}
	//exit(0);
}

void wtk_flat_matrix_multi_x2(wtk_flat_t *d,wtk_matrix_t *m, wtk_matrix_t *a, wtk_matrix_t *b)
{
    register float *tpb, *tpm, *pm;
    register float fa;
    float *pb;
    int i, j, row, col, col2;

    //  wtk_debug("[%d,%d] * [%d,%d] =[%d,%d]\n",wtk_matrix_rows(a),wtk_matrix_cols(a),
    //          wtk_matrix_rows(b),wtk_matrix_cols(b),wtk_matrix_rows(m),wtk_matrix_cols(m));
    row = wtk_matrix_rows(a);
    col = wtk_matrix_cols(a);
    col2 = wtk_matrix_cols(b);
    //wtk_debug("row=%d col=%d col2=%d\n",row,col,col2);
    pb = b[1];
    for(j = 1; j <= row; ++j) {
        fa = a[j][1];
        tpb = pb;
        tpm = m[j];
        if(fa == 0) {
            memset(tpm + 1, 0, col2 << 2);
        } else {
            pm = tpm + col2;
            while(pm - tpm >= 8) {
                *(++tpm) = fa * (*(++tpb));
                *(++tpm) = fa * (*(++tpb));
                *(++tpm) = fa * (*(++tpb));
                *(++tpm) = fa * (*(++tpb));
                *(++tpm) = fa * (*(++tpb));
                *(++tpm) = fa * (*(++tpb));
                *(++tpm) = fa * (*(++tpb));
                *(++tpm) = fa * (*(++tpb));
            }
            while(tpm < pm) {
                *(++tpm) = fa * (*(++tpb));
            }
        }
    }
    for(i = 2; i <= col; ++i) {
        pb = b[i];
        for(j = 1; j <= row; ++j) {
            fa = a[j][i];
            if(fa == 0) {
                continue;
            }
            tpb = pb;
            tpm = m[j];
            pm = tpm + col2;
            while(pm - tpm >= 8) {
                *(++tpm) += fa * (*(++tpb));
                *(++tpm) += fa * (*(++tpb));
                *(++tpm) += fa * (*(++tpb));
                *(++tpm) += fa * (*(++tpb));
                *(++tpm) += fa * (*(++tpb));
                *(++tpm) += fa * (*(++tpb));
                *(++tpm) += fa * (*(++tpb));
                *(++tpm) += fa * (*(++tpb));
            }
            while(tpm < pm) {
                *(++tpm) += fa * (*(++tpb));
            }
        }
    }
}

void wtk_flat_matrix_multi(wtk_flat_t *f,wtk_matrix_t *m,wtk_matrix_t *a,wtk_matrix_t *b)
{
	register float *tpb,*tpm,*pm;
	register float fa;
	float *pb;
	float *pa;
	int i,j,row,col,col2;

	row=wtk_matrix_rows(a);
	col=wtk_matrix_cols(a);
	col2=wtk_matrix_cols(b);
	//wtk_debug("a=%d/%d b=%d/%d m=%d/%d\n",wtk_matrix_cols(a),wtk_matrix_rows(a),wtk_matrix_cols(b),wtk_matrix_rows(b),wtk_matrix_cols(m),wtk_matrix_rows(m));
	if(row==1)
	{
		pa=a[1];
		fa=pa[1];
		tpb=b[1];
		tpm=m[1];
		pm=tpm+col2;
		while(tpm<pm)
		{
			*(++tpm)=fa*(*(++tpb));
		}
		for(i=2;i<=col;++i)
		{
			fa=pa[i];
			tpb=b[i];
			tpm=m[1];
			while(tpm<pm)
			{
				*(++tpm)+=fa*(*(++tpb));
			}
		}
	}else
	{
		i=1;
		pb=b[i];
		for(j=1;j<=row;++j)
		{
			fa=a[j][i];
			tpb=pb;
			tpm=m[j];
			pm=tpm+col2;
			while(tpm<pm)
			{
				*(++tpm)=fa*(*(++tpb));
			}
		}
		for(i=2;i<=col;++i)
		{
			pb=b[i];
			for(j=1;j<=row;++j)
			{
				fa=a[j][i];
				tpb=pb;
				tpm=m[j];
				pm=tpm+col2;
				while(tpm<pm)
				{
					*(++tpm)+=fa*(*(++tpb));
				}
			}
		}
	}
}

void wtk_flat_matrix_multi2(wtk_flat_t *f,wtk_matrix_t *m,wtk_matrix_t *a,wtk_matrix_t *b)
{
	register float *tpb,*tpm,*pm;
	register float fa;
	float *pb;
	int i,j,row,col,col2;
	float f1;

	if(f->cfg->min_trans_avg_scale==0.0 && f->cfg->min_trans_avg_v==0)
	{
		f1=0;
	}else if(f->cfg->min_avg_scale==0)
	{
		f1=f->cfg->min_trans_avg_v;
	}else
	{
		f1=wtk_matrix_avg(a)*f->cfg->min_trans_avg_scale;
	}
	row=wtk_matrix_rows(a);
	col=wtk_matrix_cols(a);
	col2=wtk_matrix_cols(b);
	//wtk_debug("row=%d col=%d col2=%d\n",row,col,col2);
	pb=b[1];
	for(j=1;j<=row;++j)
	{
		fa=a[j][1];
		tpm=m[j];
		//if(fa==0)
		if(fa>=f1 && fa<=-f1)
		{
			memset(tpm+1,0,col2<<2);
		}else
		{
			tpb=pb;
			pm=tpm+col2;
			while(tpm<pm)
			{
				*(++tpm)=fa*(*(++tpb));
			}
		}
	}
	for(i=2;i<=col;++i)
	{
		pb=b[i];
		for(j=1;j<=row;++j)
		{
			fa=a[j][i];
			//if(fa==0)
			if(fa>=f1 && fa<=-f1)
			{
				continue;
			}
			tpb=pb;
			tpm=m[j];
			pm=tpm+col2;
			while(tpm<pm)
			{
				*(++tpm)+=fa*(*(++tpb));
			}
		}
	}
}
#endif

void wtk_flat_matrix_add(wtk_matrix_t *m,wtk_matrix_t *a)
{
	int rows=wtk_matrix_rows(m);
	int cols=wtk_matrix_cols(m);
	int i,j;
	float *pm,*pa;

	//wtk_debug("a=row=%d col=%d\n",wtk_matrix_rows(a),wtk_matrix_cols(a));
	//wtk_debug("m=row=%d col=%d\n",wtk_matrix_rows(m),wtk_matrix_cols(m));
	//exit(0);
	pa=a[1];
	for(i=1;i<=rows;++i)
	{
		pm=m[i];
		for(j=1;j<=cols;++j)
		{
			//wtk_debug("%d,%d,%f,%f\n",i,j,m[i][j],a[i][j]);
			pm[j]+=pa[j];
		}
	}
}

void wtk_flat_sigmoid3(wtk_matrix_t *m)
{
	int row,col;
	int i,j;

	row=wtk_matrix_rows(m);
	col=wtk_matrix_cols(m);
	for(i=1;i<=row;++i)
	{
		for(j=1;j<=col;++j)
		{
			m[i][j]=1.0/(1.0+wtk_flat_expf(-m[i][j]));
		}
	}
}

void wtk_flat_normalize(float *f,int len,float scale)
{
        float *p,*e;
        float y,sum;
        sum=0.0;
        p=f;e=p+len;
        //wtk_debug("%f %f\n",*p,*(e-1));
        //sum=alpha*cblas_sdot(len,f,1,f,1);

        while(p<e)
        {
            sum+=(*p)*(*p);
            ++p;
        }

        p=f;e=p+len;
        y=pow(sum,-0.5);
       // wtk_debug("%f\n",y);
        p=f;e=p+len;

                while(p<e)
                {
                        *p*=y;
                        ++p;
                }
}

void wtk_flat_normalize2(wtk_matrix_t *m)
{
    int row, col;
    int i, j;
    float y,sum;

    row = wtk_matrix_rows(m);
    col = wtk_matrix_cols(m);
	sum=0.0;
    //wtk_debug("%d %d\n",row,col);
    for(i = 1; i <= row; ++i) {
        for(j = 1; j <= col; ++j) {
            sum+= m[i][j]*m[i][j];
        }
    }
    y=pow(sum,-0.5);

    for(i = 1; i <= row; ++i) {
        for(j = 1; j <= col; ++j) {
        	m[i][j] = m[i][j]*y;
        }
    }
}

void wtk_flat_sigmoid(wtk_matrix_t *m)
{
    int row, col;
    int i, j;

    row = wtk_matrix_rows(m);
    col = wtk_matrix_cols(m);
    //wtk_debug("%d %d\n",row,col);
	for(i=1;i<=row;++i)
	{
		for(j=1;j<=col;++j)
		{
            m[i][j] = 1.0 / (1.0 + expf(-m[i][j]));
        }
    }
}

void wtk_flat_process_dnn_layer(wtk_flat_t *d,wtk_dnn_layer_t *l,wtk_matrix_t *input,
		wtk_matrix_t *output,int index)
{
	int n;
	int i,k;

#ifdef DEBUG_FLAT
	wtk_debug("=========== window ==============\n");
	wtk_debug("v[1]=%f\n",l->w[1][1]);
	wtk_debug("v[2]=%f\n",l->w[1][2]);
	wtk_debug("v[3]=%f\n",l->w[1][3]);
#endif
//	wtk_debug("input=%d/%d output=%d/%d w=%d/%d\n",wtk_matrix_rows(input),wtk_matrix_cols(input),
//			wtk_matrix_rows(output),wtk_matrix_cols(output),
//			wtk_matrix_rows(l->w),wtk_matrix_cols(l->w));
	//wtk_matrix_print(input);
	wtk_flat_matrix_multi(d,output,input,l->w);

	 //wtk_matrix_print(l->w);
	//wtk_matrix_print(output);
	//exit(0);
   //print_float(output[1]+1,2);
//    {
//    	static int ki=0;
//
//    	++ki;
//    	wtk_debug("----------- ki=%d ---------\n",ki);
//    	print_float(l->w[1]+1,2);
//    	if(ki==2)
//    	{
//    		exit(0);
//    	}
//    }
	//exit(0);
	//wtk_flat_matrix_multi_trans(d,output,input,l->w);
#ifdef DEBUG_FLAT
	wtk_matrix_multi3(output,input,l->w);
	wtk_debug("=========== mul ==============\n");
	wtk_debug("v[1]=%f\n",output[1][1]);
	wtk_debug("v[2]=%f\n",output[1][2]);
	wtk_debug("v[3]=%f\n",output[1][3]);
#endif
	if(l->b)
	{
		wtk_flat_matrix_add(output,l->b);
	}
	//wtk_matrix_print(output);
	switch(l->type)
	{
	case wtk_fnn_sigmoid:
		//wtk_debug("row=%d col=%d\n",wtk_matrix_rows(output),wtk_matrix_cols(output));
		//print_float(output[1]+1,10);
		wtk_flat_sigmoid(output);
		//wtk_matrix_print(output);
		//exit(0);
		/*
		n=wtk_matrix_cols(output);
		wtk_ann_sigmoid(output[1]+1,n);
		//wtk_matrix_print(output);
		 */
		break;
	case wtk_fnn_relu:
		//wtk_matrix_print(l->r_alpha);
		//wtk_matrix_print(l->r_beta);
		n=wtk_matrix_cols(output);
		k=wtk_matrix_rows(output);
		for(i=1;i<=k;++i)
		{
			if(l->r_alpha)
			{
				wtk_relu(output[i]+1,l->r_alpha[1]+1,l->r_beta[1]+1,n);
			}else
			{
				wtk_raw_relu(output[i]+1,n);
			}
		}
		break;
	case wtk_fnn_softmax:
		if(!d->dnn->cfg->use_linear_output)
		{
			n=wtk_matrix_cols(output);
			k=wtk_matrix_rows(output);
			for(i=1;i<=k;++i)
		{
				wtk_softmax(output[i]+1,n);
				wtk_dnn_output_debug(output[i]+1,n);
			}
		}
		break;
	case wtk_fnn_linear:
		break;
	case wtk_fnn_sigmoid_normal:
		//wtk_debug("normal\n");
		wtk_flat_sigmoid(output);
		//int x;
		//x = wtk_matrix_cols(output);
		//wtk_flat_normalize(&(output[1][1]),x,1);
		wtk_flat_normalize2(output);
		break;
	default:
        wtk_debug("layer->type not in list. %d\n", l->type);
        break;
	}
}

void wtk_flat_raise_dnn2(wtk_flat_t *d,wtk_matrix_t *output_m)
{
	wtk_robin_t *r=d->robin;
	wtk_feat_t *f;
    int skip_frame;
    int idx=0;

	//wtk_debug("row=%d\n",output_i->row);
	skip_frame=d->dnn->cfg->skip_frame;
	while (r->used > 0) {
		f = wtk_robin_pop(r);
		if (!f) {
			break;
		}
		--f->used;
		if( 0 == skip_frame || ( f->index%(skip_frame)==1 ) )
		{
			//wtk_debug("v[%p]=%d\n",f,f->index);
			//wtk_debug("row=%d col=%d\n",wtk_matrix_rows(output_m),wtk_matrix_cols(output_m));
			memcpy(&(f->dnn_v[1]),(output_m[++idx]+1), wtk_vector_size(f->dnn_v)*sizeof(float));
			if(d->last_feature)
			{
				--d->last_feature->used;
				wtk_fextra_push_feature(d->dnn->parm,d->last_feature);
			}
			++f->used;
			d->last_feature=f;
			f->app_hook=NULL;
		}else
		{
			++d->last_feature->used;
			f->app_hook=d->last_feature;
		}
		wtk_fnn_raise_feature(d->dnn, f);
	}
}


void wtk_flat_trans_process(wtk_matrix_t *m,wtk_dnn_trans_t *l)
{
	register float *pb,*pm,*epb,*pv;
	int row,col;
	int i;

	row=wtk_matrix_rows(m);
	col=wtk_matrix_cols(m);
	//wtk_debug("row=%d col=%d\n",row,col);
	epb=l->b+1+col;
	for(i=1;i<=row;++i)
	{
		pb=l->b+1;
		pm=l->m+1;
		pv=m[i]+1;
		while(pb<epb)
		{
			*pv=(*pv+*(pb++))*(*(pm++));
			//*pv=(*pv)*(*(pm++))+*(pb++);
			++pv;
		}
	}
}


void wtk_flat_process_layer_matrix(wtk_flat_t *d,wtk_matrix_t *m)
{
	wtk_queue_node_t *n;
	wtk_dnn_layer_t *l;
	wtk_matrix_t *output_m=0;
	int i;

	 //wtk_matrix_print(m);
	 //exit(0);
	//wtk_debug("v[%d][%d]=%f\n",ki,idx,m[1][idx]);
	//wtk_debug("=========== 123 ================\n");
	//wtk_matrix_print(m);
	//print_float(m[1]+1,10);
	//print_float(m[2]+1,10);
	wtk_flat_trans_process(m,d->cfg->trans);
	//print_float(m[1]+1,10);
	//print_float(m[2]+1,10);
	//wtk_matrix_print(m);
//	print_float(m[1]+1,20);
//	{
//		static int ki=0;
//		++ki;
//		if(ki==2)
//		{
//			exit(0);
//		}
//	}
	//exit(0);
	//wtk_dnn_trans_process(d->cfg->trans,m[1]);
	//wtk_debug("v[%d][%d]=%f\n",ki,idx,m[1][idx]);
	//wtk_matrix_print(m);
	for(i=0,n=d->cfg->layer_q.pop;n;n=n->next,++i)
	{
		//wtk_debug("[%d/%d]\n",f->index,i);
		l=data_offset(n,wtk_dnn_layer_t,q_n);
		//wtk_debug("float=%d\n",l->float_type);
		output_m=d->matrixs[i];
		//wtk_debug("i=%d/%d\n",i,d->cfg->layer_q.length);
		//print_float(m[1]+1,2);
		wtk_flat_process_dnn_layer(d,l,m,output_m,i);
		m=output_m;
//		wtk_debug("============== i=%d ============\n",i);
//		print_float(m[1]+1,10);
//		print_float(m[2]+1,10);
//		if(!n->next)
//		{
//			exit(0);
//		}
		//exit(0);
		//exit(0);
	}
	//wtk_matrix_print(output_m);
	//print_float(m[1]+1,10);
	//print_float(m[2]+1,10);
	//exit(0);
	//exit(0);
	wtk_flat_raise_dnn2(d,output_m);
	//memcpy(&(f->dnn_v[1]),&(output_m[1][1]),wtk_vector_size(f->dnn_v)*sizeof(float));
	//wtk_feature_print(f);
	//exit(0);
	//wtk_dnn_feature_attach_log(d,f->dnn_v);
}


//void wtk_flat_mati_scale2(wtk_mati_t *m,float scale)
//{
//	int i;
//	int n;
//
//	n=m->row*m->col;
//	for(i=0;i<n;++i)
//	{
//		//wtk_debug("v[%d]=%f\n",i,(m->p[i])*scale);
//		m->p[i]=(m->p[i])*scale;
//		//wtk_debug("v[%d]=%d\n",i,m->p[i]);
//	}
//	//exit(0);
//}

static void wtk_flat_mati_scale(wtk_flat_t *f,wtk_mati_t *m,wtk_dnn_layer_t *d)
{
	float scale;

	//scale=1.0/(127.0/d->fix_wb->max);
	scale=1.0/d->fix_wb->scale;//d->fix_wb->max/(f->cfg->max_w);//127.0);
	wtk_mati_scale(m,scale);
}

void wtk_flat_mati_add(wtk_mati_t *a,wtk_mati_t *b)
{
	register int *pa,*epa,*pb;
	int i;

	//wtk_debug("a=%d/%d b=%d/%d\n",a->row,a->col,b->row,b->col);
	//wtk_debug("r=%d,c=%d\n",b->row,b->col);
	for(pa=a->p,i=0;i<a->row;++i)
	{
		pb=b->p;
		epa=pa+a->col;
		while(epa-pa>=4)
		{
			*(pa++)+=*(pb++);
			*(pa++)+=*(pb++);
			*(pa++)+=*(pb++);
			*(pa++)+=*(pb++);
		}
		while(epa>pa)
		{
			*(pa++)+=*(pb++);
		}
	}
}

void wtk_flat_sigmod(wtk_flat_t *f,wtk_matuc_t *output,wtk_mati_t *input,wtk_dnn_layer_t* l)
{
	register unsigned char *o_p,*o_e2;
	unsigned char *o_e;
	register int *i_p;
	register float scale;
	register float max_b=f->cfg->max_b;
	int v;

	scale=-1.0/(max_b*(l->fix_wb->scale));
	o_p=output->p;
	v=output->row*output->col;
	o_e2=o_p+((v>>3)<<3);
	o_e=o_p+v;
	i_p=input->p;
	while(o_p<o_e2)
	{
		o_p[0]=(unsigned char)(max_b/(1+wtk_flat_expf(i_p[0]*scale)));
		o_p[1]=(unsigned char)(max_b/(1+wtk_flat_expf(i_p[1]*scale)));
		o_p[2]=(unsigned char)(max_b/(1+wtk_flat_expf(i_p[2]*scale)));
		o_p[3]=(unsigned char)(max_b/(1+wtk_flat_expf(i_p[3]*scale)));
		o_p[4]=(unsigned char)(max_b/(1+wtk_flat_expf(i_p[4]*scale)));
		o_p[5]=(unsigned char)(max_b/(1+wtk_flat_expf(i_p[5]*scale)));
		o_p[6]=(unsigned char)(max_b/(1+wtk_flat_expf(i_p[6]*scale)));
		o_p[7]=(unsigned char)(max_b/(1+wtk_flat_expf(i_p[7]*scale)));
		o_p+=8;
		i_p+=8;
	}
	while(o_p<o_e)
	{
		*(o_p++)=(unsigned char)(max_b/(1+wtk_flat_expf(*(i_p++)*scale)));
	}
}

void wtk_flat_sigmod_2(wtk_flat_t *f,wtk_matuc_t *output,wtk_mati_t *input,wtk_dnn_layer_t* l)
{
	unsigned char *o_p,*o_e;
	int *i_p;
	float scale;

	scale=1.0/(f->cfg->max_b*(l->fix_wb->scale));
	o_p=output->p;
	o_e=o_p+output->row*output->col;
	i_p=input->p;
	while(o_p<o_e)
	{
		*(o_p++)=(unsigned char)(f->cfg->max_b/(1+wtk_flat_expf(-*(i_p++)*scale)));
	}
}




void wtk_flat_softmax(wtk_flat_t *f,wtk_matuc_t *output,wtk_mati_t *input,wtk_dnn_layer_t* l)
{
	float sum;
	int max;
	int *p,*e;
	float f1;
	float scale;//s1;
	float *fx,*fp;
	unsigned char *cs,*ce;

	//s1=f->cfg->max_b*(l->fix_wb->scale);
	scale=1.0/(f->cfg->max_b*(l->fix_wb->scale));
	max=wtk_mati_max(input);
	wtk_debug("max=%d\n",max);
	p=input->p;
	e=p+input->row*input->col;
	fp=fx=(float*)wtk_malloc(sizeof(float)*input->row*input->col);
	sum=0;
	while(p<e)
	{
		*(fp++)=f1=wtk_flat_expf((*(p++)-max)*scale);
		sum+=f1;
		//wtk_debug("f1=%f/%f\n",f1,f1*s1);
	}
	cs=output->p;
	ce=cs+output->row*output->col;
	sum=1.0f/sum;
	fp=fx;
	while(cs<ce)
	{
		f1=log(*(fp++)*sum);
		wtk_debug("f1=%f\n",f1);
		exit(0);
		++cs;
	}
	wtk_debug("%d\n",input->row*input->col);
	exit(0);
}

void wtk_flat_process_dnn_fix_layer(wtk_flat_t *d,wtk_dnn_layer_t *l,
		wtk_mati_t *tmp,
		wtk_matuc_t *output)
{
	if(l->fix_wb->b)
	{
		wtk_flat_mati_add(tmp,l->fix_wb->b);
	}
	switch(l->type)
	{
	case wtk_fnn_sigmoid:
		output->row=tmp->row;
		wtk_flat_sigmod(d,output,tmp,l);
		//wtk_ann_sigmoid(output[1]+1,n);
		//wtk_matrix_print(output);
		break;
	case wtk_fnn_softmax:
//		if(!d->dnn->cfg->use_linear_output)
//		{
//			wtk_flat_softmax(d,output,tmp,l);
//			/*
//			n=wtk_matrix_cols(output);
//			wtk_ann_softmax(output[1]+1,n);
//			wtk_dnn_output_debug(output);
//			*/
//		}
		/*
		wtk_mati_print(tmp);
		wtk_debug("index=%d max=%f\n",l->index,l->fix_wb->max);
		exit(0);
		*/
		break;
	case wtk_fnn_linear:
		//wtk_flat_mati_scale(d,tmp,l);
		break;
	default:
        wtk_debug("layer->type not in list. %d\n", l->type);
        break;
	}
}

static float LogTable256[256] =
{
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
    -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
    LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
};

int flog(int v)
{
	//unsigned int v; // 32-bit word to find the log of
	unsigned r;     // r will be lg(v)
	register unsigned int t, tt; // temporaries

	if ((tt = v >> 16))
	{
	  r = (t = tt >> 8) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
	}
	else
	{
	  r = (t = v >> 8) ? 8 + LogTable256[t] : LogTable256[v];
	}
	return r;
}

void wtk_flat_hack_soft_max2(wtk_flat_t *d,float *f,int n)
{
#define RANGE 1000000
#define EXP_A (1048576/M_LN2)
#define EXP_C 60801
	union {
		double d;
		struct {
			int j, i;
		} n;
	} d2i;
	float max,sum;
	float *p,*e;
	float t;
	unsigned int v;
	//static float x1=-1e10,x2=1e10;
	//static float s=0;
#define LOG2 0.693147	//log(2);
	int r;     // r will be lg(v)
	unsigned int tt,t2; // temporaries

	v=1072693248-EXP_C;
	t=EXP_A;
	d2i.d=0;
	d2i.n.j=0;
	max=wtk_math_max(f,n);
	sum=0;
	p=f;e=p+n;
	while(p<e)
	{
		//d2i.n.i = EXP_A*(*p-max)+(1072693248-EXP_C);
		d2i.n.i = t*(*p-max)+v;
		sum+=*(p++)=d2i.d;
		//sum+=*(p++);
		//++p;
	}
	sum=1.0f/sum;
	t=log(sum/RANGE);
	p=f;//e=p+len;
	while(p<e)
	{
		v=*p*RANGE;
		if ((tt = v >> 16))
		{
		  r = (t2 = tt >> 8) ? 24 + LogTable256[t2] : 16 + LogTable256[tt];
		}
		else
		{
		  r = (t2 = v >> 8) ? 8 + LogTable256[t2] : LogTable256[v];
		}
		//wtk_debug("r=%d/%d\n",r,flog(*p*1000000));
		//exit(0);
		//*p=r+t;
		*(p++)=r*LOG2+t;
		//*p=flog(*p*1000000)*s+t;
		//*p=flog(*p*1000000)*log(2)/1000000+t;
		//*p=log(*p)+t;//log(*p*sum);
		//++p;
	}
	//wtk_debug("x1=%f x2=%f\n",x1,x2);	//0.971008 x2=0.000000
}

void wtk_flat_hack_soft_max(wtk_flat_t *d,float *f,int n)
{
#define EXP_A (1048576/M_LN2)
#define EXP_C 60801
	union {
		double d;
		struct {
			int j, i;
		} n;
	} d2i;
	float max,sum;
	float *p,*e;
	float t;
	unsigned int v;

	v=1072693248-EXP_C;
	t=EXP_A;
	d2i.d=0;
	d2i.n.j=0;
	max=wtk_math_max(f,n);
	sum=0;
	p=f;e=p+n;
	while(p<e)
	{
		//d2i.n.i = EXP_A*(*p-max)+(1072693248-EXP_C);
		d2i.n.i = t*(*p-max)+v;
		sum+=*(p++)=d2i.d;
		//sum+=*(p++);
		//++p;
	}
	sum=1.0f/sum;
	t=log(sum);
	p=f;//e=p+len;
	while(p<e)
	{
		*p=log(*p)+t;//log(*p*sum);
		++p;
	}
	//wtk_debug("x1=%f x2=%f\n",x1,x2);	//0.971008 x2=0.000000
}

void wtk_flat_post_soft_max(wtk_flat_t *d,float *a,int len)
{
	float max,sum;
	float *p,*e;
	float t;

	max=wtk_math_max(a,len);
	sum=0;
	p=a;e=p+len;
	while(p<e)
	{
		*p=wtk_flat_expf(*p-max);
		sum+=*(p++);
		//++p;
	}
	sum=1.0f/sum;
	t=log(sum);
	p=a;//e=p+len;
	while(p<e)
	{
		*p=log(*p)+t;//log(*p*sum);
		++p;
	}
}

void wtk_flat_raise_dnn(wtk_flat_t *d,wtk_mati_t *output_i,wtk_dnn_layer_t *layer)
{
	wtk_robin_t *r=d->robin;
	float *p,*pe;
	int *pi;
	float scale;
	wtk_feat_t *f;
    int skip_frame;

    //scale=layer->fix_wb->max/(d->cfg->max_w*d->cfg->max_b);//255.0*127.0)
	//scale=layer->fix_wb->max/(d->cfg->max_w*d->cfg->max_b);//255.0*127.0);
	scale=1.0/(layer->fix_wb->scale*d->cfg->max_b);//255.0*127.0);
	//wtk_debug("row=%d,col=%d\n",output_i->row,output_i->col);
	pi=output_i->p;
	//wtk_debug("row=%d\n",output_i->row);
	skip_frame=d->dnn->cfg->skip_frame;
	while (r->used > 0) {
		f = wtk_robin_pop(r);
		if (!f) {
			break;
		}
		--f->used;
		if(skip_frame==0 || f->index%(skip_frame)==1)
		{
			//wtk_debug("v[%p]=%d\n",f,f->index);
			p=f->dnn_v;
			pe=p+output_i->col;//wtk_vector_size(f->dnn_v);
			//wtk_debug("col=%d/%d\n",wtk_vector_size(f->dnn_v),output_i->col);
			while(p<pe)
			{
				*(++p)=*(pi++)*scale;//output_i->p;
				//wtk_debug("%f/%f\n",*(p),l->fix_wb->max);
			}
			if(layer->type==wtk_fnn_softmax)
			{
				if(d->dnn->cfg->use_hack_output)
				{
					wtk_flat_hack_soft_max(d,f->dnn_v+1,output_i->col);
				}else if(!d->dnn->cfg->use_linear_output)
				{
					wtk_flat_post_soft_max(d,f->dnn_v+1,output_i->col);
				}
			}
#ifdef DUBUG_DNN
			int i;
			int n;

			n=wtk_vector_size(f->dnn_v);
			n=10;
			for(i=1;i<n;++i)
			{
				wtk_debug("v[%d]=%f\n",i,f->dnn_v[i]);
			}
			exit(0);
#endif
			if(d->last_feature)
			{
				--d->last_feature->used;
				wtk_fextra_push_feature(d->dnn->parm,d->last_feature);
			}
			++f->used;
			d->last_feature=f;
			f->app_hook=NULL;
		}else
		{
			++d->last_feature->used;
			f->app_hook=d->last_feature;
		}
		wtk_fnn_raise_feature(d->dnn, f);
	}
}

void wtk_flat_raise_dnn_lazy(wtk_flat_t *d,wtk_mati_t *output_i)
{
	wtk_robin_t *r=d->robin;
	wtk_feat_t *f;
    int skip_frame;
    int idx=0;

	//wtk_debug("row=%d\n",output_i->row);
	skip_frame=d->dnn->cfg->skip_frame;
	while (r->used > 0) {
		f = wtk_robin_pop(r);
		if (!f) {
			break;
		}
		--f->used;
		if(skip_frame==0 || f->index%(skip_frame)==1)
		{
			//wtk_debug("v[%p]=%d\n",f,f->index);
			memcpy(&(f->dnn_v[1]),output_i->p+(idx*output_i->col),output_i->col*sizeof(int));
			++idx;
			/*
			wtk_debug("v[0]=%f\n",f->dnn_v[1]);
			wtk_debug("v[1]=%f\n",f->dnn_v[2]);
			wtk_debug("v[2]=%f\n",f->dnn_v[3]);
			wtk_debug("v[3]=%f\n",f->dnn_v[4]);
			exit(0);
			*/
			if(d->last_feature)
			{
				--d->last_feature->used;
				wtk_fextra_push_feature(d->dnn->parm,d->last_feature);
			}
			++f->used;
			d->last_feature=f;
			f->app_hook=NULL;
		}else
		{
			++d->last_feature->used;
			f->app_hook=d->last_feature;
		}
		wtk_fnn_raise_feature(d->dnn, f);
	}
}

void wtk_flat_mati_multi2(wtk_flat_t *f,wtk_mati_t *m,wtk_mati_t *a,wtk_matc_t *b)
{
	int *pa;
	register signed char *pb;
	register int pak;
	register int *pm,*epm;
	int *tpm;
	int i,j;
	int min_0;
	int max_0;

	if(f->cfg->min_avg_scale==0 && f->cfg->min_avg_v==0)
	{
		i=0;
	}else if(f->cfg->min_avg_scale==0)
	{
		i=f->cfg->min_avg_v;
	}else
	{
		i=wtk_mati_avg(a)*f->cfg->min_avg_scale;
	}
	min_0=-i;
	max_0=i;
	//wtk_debug("a=[%d*%d] b=[%d*%d]\n",a->row,a->col,b->row,b->col);
	for(tpm=m->p,pa=a->p,i=0;i<a->row;++i,tpm+=m->col)
	{
		pak=*pa;
		pa+=a->col;
		if(pak>=min_0 && pak<=max_0)
		{
			memset(tpm,0,b->col<<2);
		}else
		{
			pm=tpm;
			epm=pm+b->col;
			pb=b->p;
			while(epm>pm)
			{
				*(pm++)=pak*(*(pb++));
			}
		}
	}

	for(tpm=m->p,pa=a->p,i=0;i<a->row;++i,tpm+=m->col)
	{
		for(pb=b->p+b->col,j=1,++pa;j<a->col;++j)
		{
			pak=*(pa++);//pa[j];
			//wtk_debug("pak=%d\n",pak);
			if(pak>=min_0 && pak<=max_0)
			{
				pb+=b->col;
			}else
			{
				pm=tpm;
				epm=pm+b->col;
				while(epm>pm)
				{
					*(pm++)+=pak*(*(pb++));
				}
			}
		}
	}
}

/*
void wtk_ann_sigmoid2(float *f,int len)
{
	float *p;
	float *e;

	p=f;e=p+len;
	while(p<e)
	{
		*p=1.0/(1.0+expf(-*p));
		++p;
	}
}

void wtk_flat_sigmod4(wtk_flat_t *f,wtk_matuc_t *output,wtk_mati_t *input,wtk_dnn_layer_t* l)
{
	unsigned char *o_p,*o_e;
	int *i_p;
	float scale;

	scale=1.0/(f->cfg->max_b*(l->fix_wb->scale));
	o_p=output->p;
	o_e=o_p+output->row*output->col;
	i_p=input->p;
	while(o_p<o_e)
	{
		*(o_p++)=(unsigned char)(f->cfg->max_b/(1+expf(-*(i_p++)*scale)));
	}
}*/

void wtk_flat_process_dnn_fix_layer3(wtk_flat_t *d,wtk_dnn_layer_t *l,
		wtk_mati_t *tmp,wtk_matuc_t *output,float input_scale)
{
	int *i_s,*i_e;
	float *b_s,*b_e,*b_ss;
	unsigned char *o_s;
	float f,f1;
	//int i=0;

	i_s=tmp->p;
	i_e=i_s+tmp->row*tmp->col;
	b_s=b_ss=l->b[1]+1;
	b_e=b_s+tmp->col;
	o_s=output->p;
	f1=1.0/(l->fix_wb->scale*input_scale);
	while(i_s<i_e)
	{
		while(b_s<b_e)
		{
			//wtk_debug("%f\n",*b_s);
			f=*(i_s++)*f1+*(b_s++);//*(b_s++)/(d->cfg->max_b*l->fix_wb->scale);
			*(o_s++)=(unsigned char)(d->cfg->max_b/(1+wtk_flat_expf(-f)));
			//wtk_debug("v[%d]: %f/%d\n",i++,f,*(o_s-1));
			//exit(0);
		}
		b_s=b_ss;
	}
}

void wtk_flat_process_dnn_fix_layer4(wtk_flat_t *d,wtk_dnn_layer_t *l,
		wtk_mati_t *tmp,wtk_mati_t *output,float input_scale)
{
#define EXP_A (1048576/M_LN2)
#define EXP_C 60801
	union {
		double d;
		struct {
			int j, i;
		} n;
	} d2i;
	int *i_s,*i_e;
	float *b_s,*b_e,*b_ss;
	int *o_s;
	float f,f1;
	//int i=0;

	d2i.n.j=0;
	i_s=tmp->p;
	i_e=i_s+tmp->row*tmp->col;
	b_s=b_ss=l->b[1]+1;
	b_e=b_s+tmp->col;
	o_s=output->p;
	f1=1.0/(l->fix_wb->scale*input_scale);
	while(i_s<i_e)
	{
		while(b_s<b_e)
		{
			//wtk_debug("%f\n",*b_s);
			f=*(i_s++)*f1+*(b_s++);//*(b_s++)/(d->cfg->max_b*l->fix_wb->scale);
			//*(o_s++)=(int)(d->cfg->max_b/(1+wtk_flat_expf(-f)));
			d2i.n.i = EXP_A*(-f)+(1072693248-EXP_C);
			*(o_s++)=(int)(d->cfg->max_b/(1+d2i.d));
			//wtk_debug("v[%d]: %f/%d\n",i++,f,*(o_s-1));
			//exit(0);
		}
		b_s=b_ss;
	}
}

void wtk_flat_process_dnn_fix_layer5(wtk_flat_t *d,wtk_dnn_layer_t *l,
		wtk_mati_t *tmp,wtk_mati_t *output,float input_scale)
{
	int *i_s,*i_e;
	float *b_s,*b_e,*b_ss;
	int *o_s;
	float f,f1;
	//int i=0;

	//d2i.n.j=0;
	i_s=tmp->p;
	i_e=i_s+tmp->row*tmp->col;
	b_s=b_ss=l->b[1]+1;
	b_e=b_s+tmp->col;
	o_s=output->p;
	f1=1.0/(l->fix_wb->scale*input_scale);
	while(i_s<i_e)
	{
		while(b_s<b_e)
		{
			//wtk_debug("%f\n",*b_s);
			f=*(i_s++)*f1+*(b_s++);//*(b_s++)/(d->cfg->max_b*l->fix_wb->scale);
			*(o_s++)=(int)(d->cfg->max_b/(1+wtk_flat_expf(-f)));
			//d2i.n.i = EXP_A*(-f)+(1072693248-EXP_C);
			//*(o_s++)=(int)(d->cfg->max_b/(1+d2i.d));
			//wtk_debug("v[%d]: %f/%d\n",i++,f,*(o_s-1));
			//exit(0);
		}
		b_s=b_ss;
	}
}

#ifdef USE_NEON
#include <arm_neon.h>
#endif

void wtk_flat_mati_multi4(wtk_flat_t *f,wtk_mati_t *m,wtk_mati_t *a,wtk_mati_t *b)
{
	int *pa;
	register int *pb;
	 int pak;
	register int *pm,*epm;
	int *tpm;
	int i,j;
	int min_0;
	int max_0;
#ifdef USE_NEON
	int32x4_t xa,xb,xc;
	int *pc;

	pc=(int*)&(xc);
#endif
	if(f->cfg->min_trans_avg_scale==0 && f->cfg->min_trans_avg_v==0)
	{
		i=0;
	}else if(f->cfg->min_trans_avg_scale==0)
	{
		i=f->cfg->min_trans_avg_v;
	}else
	{
		i=wtk_mati_avg(a)*f->cfg->min_trans_avg_scale;
	}
	//wtk_debug("i=%d/%f\n",i,f->cfg->min_trans_avg_scale);
	min_0=-i;
	max_0=i;
	//wtk_debug("a=[%d*%d] b=[%d*%d]\n",a->row,a->col,b->row,b->col);
	for(tpm=m->p-1,pa=a->p,i=0;i<a->row;++i,tpm+=m->col)
	{
		pak=*pa;
		pa+=a->col;
		if(pak>=min_0 && pak<=max_0)
		{
			memset(tpm,0,b->col<<2);
		}else
		{
			pm=tpm;
			epm=pm+b->col;
			pb=b->p-1;
			while(epm>pm)
			{
				*(++pm)=pak*(*(++pb));
			}
		}
	}
	for(tpm=m->p-1,pa=a->p-1,i=0;i<a->row;++i,tpm+=m->col)
	{
		epm=tpm+b->col;
		for(pb=b->p+b->col-1,j=1,++pa;j<a->col;++j)
		{
			pak=*(++pa);//pa[j];
			//wtk_debug("pak=%d\n",pak);
			if(pak>=min_0 && pak<=max_0)
			{
				pb+=b->col;
			}else
			{
				pm=tpm;
				//epm=pm+b->col;
#ifdef USE_NEON
				xa=vld1q_dup_s32(&(pak));
				wtk_debug("%d/%d/%d/%d\n",pak*(pb[1]),pak*(pb[2]),pak*(pb[3]),pak*(pb[4]))
				while(epm-pm>4)
				{
					xb=vld1q_s32(++pb);
					xc=vmulq_s32(xa,xb);
					wtk_debug("pc=%d %d %d %d\n",pc[0],pc[1],pc[2],pc[3]);
					exit(0);
				}
#endif
				while(epm>pm)
				{
					*(++pm)+=pak*(*(++pb));
				}
			}
		}
	}
}

float wtk_float_scale_int_to_uchar(wtk_flat_t *d,wtk_mati_t *src,wtk_matuc_t *c,double scale)
{
	int i;
	int min;
	int *p;
	int max;
	float t;
	unsigned char *px;
	float s;

	p=src->p;
	min=p[0];
	max=p[0];
	for(i=1;i<src->col;++i)
	{
		if(p[i]<min)
		{
			min=p[i];
		}else if(p[i]>max)
		{
			max=p[i];
		}
	}
	//wtk_debug("min=%d max=%d\n",min,max);
	t=255.0/(max-min);
	px=c->p;
	s=1.0/(t*scale);
	//wtk_debug("min=%f/%f\n",min*1.0/scale,d->linear_uc_min);
	//(px[i]/t+min)/scale;
	//(px[i]+min*t)/(t*scale);
	for(i=0;i<src->col;++i)
	{
		px[i]=(p[i]-min)*t;
		//wtk_debug("v[%d]=%f/%f\n",i,p[i]/scale,px[i]*s+d->linear_uc_min);//(px[i]/t+min)/scale);
		//exit(0);
		//wtk_debug("v[%d]=%d\n",i,px[i]);
	}
	//wtk_debug("min=%f\n",d->linear_uc_min);
	//exit(0);
	//exit(0);
	return s;
}

double wtk_float_scale_int_to_char(wtk_mati_t *src,wtk_matc_t *c,double scale)
{
	int i;
	int max;
	int *p;
	signed char *px;
	double t;
	double s;
	int v;

	p=src->p;
	max=abs(p[0]);
	//min=p[0];
	for(i=1;i<src->col;++i)
	{
		v=abs(p[i]);
		if(v>max)
		{
			max=v;
		}
	}
	//wtk_debug("max=%d min=%d\n",max,min);
	//max=max(abs(max),abs(min));
	//(p[i]/scale)*127.0/max;
	//p[i]/scale * 127.0/max;
	//wtk_debug("scale=%f\n",scale);
	t=127.0/max;
	//scale=127.0/(max*scale);
	//wtk_debug("scale=%f max=%d t=%f\n",scale,max,t);
	px=c->p;
	s=1.0/(t*scale);
	//wtk_debug("s=%f/%f\n",s,t);
	for(i=0;i<src->col;++i)
	{
		px[i]=p[i]*t;
		//wtk_debug("v[%d]=%f/%f\n",i,p[i]/scale,px[i]*s);
		//exit(0);
	}
	//exit(0);
	//exit(0);
	//exit(0);
	return s;
}

typedef enum
{
	WTK_FLAT_FIX_UC,
	WTK_FLAT_FIX_SC,
	WTK_FLAT_FIX_INT,
}wtk_flat_fix_output_t;

void wtk_flat_process_layer_matrix_fixpoint(wtk_flat_t *d,wtk_matrix_t *m)
{
	wtk_queue_node_t *n;
	wtk_dnn_layer_t *l=NULL;
	wtk_matuc_t *output_uc=NULL;
	wtk_mati_t *output_i=NULL;
	wtk_matc_t *output_c=NULL;
	wtk_mati_t *tmp_i;
	wtk_matc_t *matc;
	wtk_flat_fix_output_t output_type;
	//int uc;
	int i;
	float f;
	double c_scale=0;
//#define DEBUG_FLAT

	//wtk_dnn_trans_process(d->cfg->trans,m[1]);
	wtk_flat_trans_process(m,d->cfg->trans);
	//wtk_matrix_print(m);
	output_type=WTK_FLAT_FIX_UC;
	//wtk_debug("row=%d col=%d\n",wtk_matrix_rows(m),wtk_matrix_cols(m));
	for(i=0,n=d->cfg->layer_q.pop;n;n=n->next,++i)
	{
		//wtk_debug("[%d/%d]\n",f->index,i);
		l=data_offset(n,wtk_dnn_layer_t,q_n);
//		if(i>0)
//		{
//			switch(output_type)
//			{
//			case WTK_FLAT_FIX_UC:
//				wtk_debug("row=%d col=%d\n",output_uc->row,output_uc->col);
//				break;
//			case WTK_FLAT_FIX_SC:
//				wtk_debug("row=%d col=%d\n",output_c->row,output_c->col);
//				break;
//			default:
//				wtk_debug("==========================\n");
//				exit(0);
//				break;
//			}
//		}
//		if(l->fix_wb->uc)
//		{
//			wtk_debug("row=%d col=%d\n",l->fix_wb->uc->uc->row,l->fix_wb->uc->uc->col);
//		}else
//		{
//			wtk_debug("row=%d col=%d\n",l->fix_wb->w.c->row,l->fix_wb->w.c->col);
//		}
#ifdef DEBUG_FLAT
		wtk_debug("====================== %d type=%d ==============\n",i,l->type);
#endif
		if(i==0)
		{
			//wtk_debug("%d\n",d->cfg->use_fix_0_layer);
			if(d->cfg->use_fix_0_layer)
			{
				f=wtk_matrix_max_abs(m);
				f=d->cfg->max_0_w/f;
				//wtk_debug("use f=%f\n",f);
				//wtk_mati_multi2(d->fix_0_mul,d->fix_0_int_input,l->fix_wb->w.c);
				if(d->cfg->use_fix_0_c)
				{
					//wtk_debug("max0=%f\n",d->cfg->max_0_w);
					if(d->cfg->use_fix_0_cx)
					{
						wtk_matc_init(d->fix_0_char_input,m,f);
						//wtk_debug("a=%d b=%d\n",d->fix_0_char_input->row,d->fix_0_char_input->col);
						//wtk_debug("c=%d/%d\n",l->fix_wb->w.c->row,l->fix_wb->w.c->col);
						//print_char2(d->fix_0_char_input->p,d->fix_0_char_input->row*d->fix_0_char_input->col);
						//print_char2(l->fix_wb->w.c->p,100);//l->fix_wb->w.c->row*l->fix_wb->w.c->col);
						//wtk_mati_multi_cc_x(d->fix_0_mul,d->fix_0_char_input,l->fix_wb->w.c);
#ifdef __mips_x1000
						//wtk_debug("use mips\n");
						//wtk_mati_multi_cu_mips(d->fix_0_mul,d->fix_0_char_input,d->fix_0_uc,d->fix_0_min);
						wtk_mati_multi_cu_mips(d->fix_0_mul,d->fix_0_char_input,l->fix_wb->uc->uc,l->fix_wb->uc->min);
#else
						wtk_mati_multi_cu_x(d->fix_0_mul,d->fix_0_char_input,l->fix_wb->uc->uc,l->fix_wb->uc->min);
						//wtk_mati_multi_cc_x(d->fix_0_mul,d->fix_0_char_input,l->fix_wb->w.c);
#endif
						//exit(0);
						//wtk_debug("fix=%d/%d\n",d->fix_0_mul->row,d->fix_0_mul->col);
						//print_int(d->fix_0_mul->p,d->fix_0_mul->col);//d->fix_0_mul->col*d->fix_0_mul->row);
						//exit(0);
					}else
					{
						wtk_mati_init(d->fix_0_int_input,m,f);
						wtk_mati_multi2(d->fix_0_mul,d->fix_0_int_input,l->fix_wb->w.c);
					}
					//exit(0);
				}else
				{
					wtk_mati_init(d->fix_0_int_input,m,f);
					wtk_flat_mati_multi4(d,d->fix_0_mul,d->fix_0_int_input,l->fix_wb->w.i);
					//wtk_mati_multi3(d->fix_0_mul,d->fix_0_int_input,l->fix_wb->w.i);
				}
				//exit(0);
				//wtk_flat_process_dnn_fix_layer3(d,l,d->fix_0_i,d->fix_0_uc);
				if(l->type==wtk_fnn_sigmoid)
				{
					wtk_flat_process_dnn_fix_layer3(d,l,d->fix_0_mul,d->fix_0_char_output,f);
					output_uc=d->fix_0_char_output;
					output_uc->row=wtk_matrix_rows(m);
					//print_uchar(output_uc->p,output_uc->col);
					//exit(0);
					output_type=WTK_FLAT_FIX_UC;
					//wtk_debug("uc: row=%d col=%d\n",output_uc->row,output_uc->col);
				}else
				{
					wtk_debug("layer->type not in list. %d\n", l->type);
					exit(0);
				}
			}else
			{
				wtk_flat_process_dnn_layer(d,l,m,d->fix_0_output1,i);
				//wtk_matrix_print(d->fix_0_output1);

				output_uc=d->fix_output.c.fix_0_output2;
				output_uc->row=wtk_matrix_rows(m);
				wtk_matuc_init(output_uc,d->fix_0_output1,d->cfg->max_b);
				output_type=WTK_FLAT_FIX_UC;
			}
			//exit(0);
		}else
		{
			tmp_i=d->fix_tmp[i-1];
			//wtk_debug("row=%d col=%d\n",tmp_i->row,tmp_i->col);
			matc=l->fix_wb->w.c;

			switch(output_type)
			{
			case WTK_FLAT_FIX_UC:
				//wtk_debug("row=%d/%d\n",tmp_i->row,tmp_i->col);
				tmp_i->row=output_uc->row;
				tmp_i->col=d->cfg->use_transpose?matc->col:matc->row;
				//wtk_debug("uc=%d/%d matc=%d/%d\n",output_uc->row,output_uc->col,matc->row,matc->col);
				//print_uchar(output_uc->p,output_uc->col);
				if(d->cfg->use_fix_0_cx)
				{
#ifdef __mips_x1000
					//wtk_debug("use mips\n");
					wtk_mati_multi_uc_mips(tmp_i,output_uc,matc);
#else
					wtk_mati_multi_uc_x(tmp_i,output_uc,matc);
#endif
				}else
				{
					wtk_mati_multi(tmp_i,output_uc,matc);
				}
				//print_uchar(output_uc->p,output_uc->col);
				//print_int(tmp_i->p,tmp_i->col);
				break;
			case WTK_FLAT_FIX_SC:
				//wtk_debug("foudn bug\n");
				tmp_i->row=output_c->row;
				if(matc)
				{
					tmp_i->col=matc->col;
				}else
				{
					tmp_i->col=d->cfg->use_transpose?l->fix_wb->uc->uc->col:l->fix_wb->uc->uc->row;
				}
				if(1)
				{
						//wtk_mati_multi_cu_x_scale(tmp_i,output_c,l->fix_wb->uc->uc,l->fix_wb->uc->min,c_scale);
#ifdef __mips_x1000
						//wtk_mati_multi_cu_mips(d->fix_0_mul,d->fix_0_char_input,d->fix_0_uc,d->fix_0_min);
						//wtk_mati_multi_cu_mips(d->fix_0_mul,d->fix_0_char_input,l->fix_wb->uc->uc,l->fix_wb->uc->min);
						wtk_mati_multi_cu_mips_scale(tmp_i,output_c,l->fix_wb->uc->uc,l->fix_wb->uc->min,c_scale);
#else
						wtk_mati_multi_cu_x_scale(tmp_i,output_c,l->fix_wb->uc->uc,l->fix_wb->uc->min,c_scale);
#endif
						//print_int(tmp_i->p,tmp_i->col);
						//exit(0);
				}else
				{
					wtk_mati_multi_cc_x_scale(tmp_i,output_c,matc,c_scale);
				}
				break;
			case WTK_FLAT_FIX_INT:
				tmp_i->row=output_i->row;
				tmp_i->col=matc->col;
				if(d->cfg->use_fix_0_cx)
				{
					//print_int(output_i->p,output_i->row*output_i->col);
					wtk_mati_multi_ic_x(tmp_i,output_i,matc);
				}else
				{
					wtk_flat_mati_multi2(d,tmp_i,output_i,matc);
				}
				//wtk_debug("%d/%d\n",tmp_i->row,tmp_i->col);
				//exit(0);
				break;
			}
			//wtk_debug("i=%d %p\n",i,d->fix_output.c.fix_output[i-1]);
			//wtk_debug("row=%d col=%d\n",tmp_i->row,tmp_i->col);
			//exit(0);
			//print_int(tmp_i->p,tmp_i->col);
			//exit(0);
			wtk_flat_process_dnn_fix_layer(d,l,tmp_i,d->fix_output.c.fix_output[i-1]);
			//wtk_mati_print(d->fix_tmp[i-1]);
			switch(l->type)
			{
			case wtk_fnn_sigmoid:
				//wtk_debug("sigmoid\n");
				output_type=WTK_FLAT_FIX_UC;
				output_uc=d->fix_output.c.fix_output[i-1];//d->fix_output[i-1];
				break;
			case wtk_fnn_relu:
				//TODO
				break;
			case wtk_fnn_softmax:
				//wtk_debug("softmax\n");
				output_type=WTK_FLAT_FIX_INT;
				output_i=tmp_i;//d->fix_tmp[i-1];
				break;
			case wtk_fnn_linear:
				//wtk_debug("linear row=%d col=%d\n",tmp_i->row,tmp_i->col);
				if(d->cfg->use_fix_0_cx)
				{
					output_c=d->output_c;
					output_c->row=tmp_i->row;
					output_c->col=tmp_i->col;
					c_scale=wtk_float_scale_int_to_char(tmp_i,output_c,l->fix_wb->scale);
					output_type=WTK_FLAT_FIX_SC;
				}else
				{
					output_type=WTK_FLAT_FIX_INT;
					wtk_flat_mati_scale(d,tmp_i,l);
					output_i=tmp_i;//d->fix_tmp[i-1];
				}
				break;
			case wtk_fnn_rescale:
				break;
			case wtk_fnn_pnorm:
				break;
			default:
				break;
			}
//			if(uc)
//			{
//				print_uchar(output_uc->p,20);
//			}else
//			{
//				print_int(output_i->p,20);
//			}
			//exit(0);
		}
		if(d->dnn->cfg->use_lazy_out && !n->next->next)
		{
#ifdef DEBUG_FLAT
			wtk_debug("v[0]=%d\n",output_i->p[0]);
			wtk_debug("v[1]=%d\n",output_i->p[1]);
			wtk_debug("v[2]=%d\n",output_i->p[2]);
			wtk_debug("v[3]=%d\n",output_i->p[3]);
			exit(0);
#endif
			wtk_flat_raise_dnn_lazy(d,output_i);
			//wtk_debug("row=%d col=%d\n",output_i->row,output_i->col);
			//wtk_debug("uc=%d\n",uc);
			return;
		}
	}
//	{
//		static int ki=0;
//
//		++ki;
//		wtk_debug("v[%d]=%d/%d\n",ki,output_i->p[0],output_i->p[1]);
//		//print_int(output_i->p,output_i->col);
////		if(ki>=112)
////		{
////			exit(0);
////		}
//	}
	//print_int(output_i->p,output_i->col);
	//exit(0);
	wtk_flat_raise_dnn(d,output_i,l);
}

void wtk_flat_feature_to_matirx(wtk_flat_t *b, wtk_matrix_t *m, int index, wtk_feat_t **pv,
                                 int step)
{
    wtk_vector_t *v;
    int i, j, n;
    float *p,*p1;
    int k=0;
    int padding = b->dnn->cfg->padding_frame;

    //wtk_debug("padding=%d index=%d\n",padding,index);
    p = &(m[index][1]);
    //wtk_debug("row=%d/%d\n",wtk_matrix_rows(m),wtk_matrix_cols(m));
    for(i = 0; i < step; ++i) {
        v = pv[i]->v;
        n = wtk_vector_size(v);
        //wtk_debug("n=%d\n",n);
        for(k = i, j = 1; j <= n; ++j, k += step) {
        	//wtk_debug("k=%d\n",k);
            p[k] = v[j];
            //          printf("%f ",p[k]);
        }
    }
    if(padding>0)
    {
		//wtk_debug("n=%d\n",wtk_vector_size(pv[0]->v));
		//  printf("\n");
	   // wtk_debug("step=%d\n",step);
		k -= step;
	   // wtk_debug("indx=%d\n",index);
		//wtk_debug("k=%d/%d\n",k,step);
		p1=b->dnn->padding;
		for(k++, i = 0; i < padding; k++, i++)
		{
			p[k] = p1[i];
			//wtk_debug("v[%d]=%f\n",k,p[k]);
		}
    }
  //  print_float(p,wtk_vector_size(b->cfg->trans->b));
}

void wtk_flat_feature_to_matirx2(wtk_flat_t *b, wtk_matrix_t *m, int index, wtk_feat_t **pv,
                                 int step)
{
    wtk_vector_t *v;
    int i, j, n;
    float *p,*p1;
    int k=0;
    int padding = b->dnn->cfg->padding_frame;

    p = &(m[index][1]);
    for(i = 0; i < step; ++i) {
        v = pv[i]->v;
        n = wtk_vector_size(v);
        for( j = 1; j <= n; ++j, k += 1) {
            p[k] = v[j];
        }
    }

    if(padding>0)
    {
		k -= step;
		p1=b->dnn->padding;
		for(k++, i = 0; i < padding; k++, i++)
		{
			p[k] = p1[i];
		}
    }
}

void wtk_flat_sigmod2(wtk_flat_t *f,wtk_dnn_layer_t* l,wtk_mati_t *output,wtk_mati_t *input)
{
#define EXP_A (1048576/M_LN2)
#define EXP_C 60801
	union {
		double d;
		struct {
			int j, i;
		} n;
	} d2i;
	int *o_p,*o_e;
	int *i_p;
	double scale;
	double x1;

	d2i.n.j=0;
	//scale=1.0/(f->cfg->max_b*(f->cfg->max_w/l->fix_wb->max));
	//scale=1.0/(f->cfg->max_b*(f->cfg->max_w/l->fix_wb->max));
	scale=1.0/(f->cfg->max_b*(l->fix_wb->scale));
	o_p=output->p;
	o_e=o_p+output->row*output->col;
	i_p=input->p;
	scale*=-EXP_A;
	x1=1072693248-EXP_C;
	while(o_p<o_e)
	{
		//*(o_p++)=(f->cfg->max_b/(1+wtk_flat_expf(-*(i_p++)*scale)));
		//d2i.n.i = EXP_A*(-*(i_p++)*scale)+(1072693248-EXP_C);
		d2i.n.i = *(i_p++)*scale+x1;//(1072693248-EXP_C);
		*(o_p++)=(f->cfg->max_b/(1+d2i.d));
	}
}

void wtk_flat_process_dnn_fix_layer2(wtk_flat_t *d,wtk_dnn_layer_t *l,
		wtk_mati_t *tmp,
		wtk_mati_t *output)
{
	if(l->fix_wb->b)
	{
		wtk_flat_mati_add(tmp,l->fix_wb->b);
	}
	//wtk_debug("index=%d, type=%d\n",l->index,l->type);
	switch(l->type)
	{
	case wtk_fnn_sigmoid:
		output->row=tmp->row;
		//wtk_debug("tmp=%d/%d\n",tmp->row,tmp->col);
		wtk_flat_sigmod2(d,l,output,tmp);
		//wtk_ann_sigmoid(output[1]+1,n);
		//wtk_matrix_print(output);
		break;
	case wtk_fnn_softmax:
		/*
		wtk_mati_print(tmp);
		wtk_debug("index=%d max=%f\n",l->index,l->fix_wb->max);
		exit(0);
		*/
		break;
	case wtk_fnn_linear:
		wtk_flat_mati_scale(d,tmp,l);
		break;
	default:
        wtk_debug("layer->type not in list. %d\n", l->type);
        break;
	}
}

void wtk_mati_multi3_x3(wtk_mati_t *m,wtk_mati_t *a,wtk_mati_t *b,wtk_flat_cfg_t *cfg,int index)
{
	int *pa;
	register int *pb,*epb;
	register int pak;
	register int *pm;
	int *tpm;
	int i,j;
	int min_0;
	int max_0;

	if(cfg->min_avg_scale==0)
	{
		i=0;
	}else
	{
		i=wtk_mati_avg(a)*cfg->min_avg_scale;
	}
	min_0=-i;
	max_0=i;
	//wtk_debug("a=[%d,%d] b=[%d,%d]\n",a->row,a->col,b->row,b->col);
	for(tpm=m->p-1,pa=a->p,i=0;i<a->row;++i,tpm+=m->col)
	{
		for(pb=b->p-1,j=0;j<a->col;++j)
		{
			pak=*(pa++);//pa[j];
			epb=pb+b->col;
			pm=tpm;
			//if(pak==0)//pak>=cfg->min_0 && pak<=cfg->max_0)
			if(pak>=min_0 && pak<=max_0)
			{
				if(j==0)
				{
					memset(pm+1,0,m->col<<2);
				}
				pb+=b->col;
			}else
			{
				if(j==0)
				{
					while(epb>pb)
					{
						*(++pm)=pak*(*(++pb));
					}
				}else
				{
					while(epb>pb)
					{
						*(++pm)+=pak*(*(++pb));
					}
				}
			}
		}
	}
}

//a*b;
void wtk_mati_multi3_x2(wtk_mati_t *m,wtk_mati_t *a,wtk_mati_t *b,wtk_flat_cfg_t *cfg,int index)
{
	register int *tpb,*tpm,*pm2;
	int *pm;
	register int pak;
	int *pa;
	int *pb;
	int i,j;
	int min_0;
	int max_0;
	int col2;

	if(cfg->min_avg_scale==0)
	{
		i=0;
	}else
	{
		i=wtk_mati_avg(a)*cfg->min_avg_scale;
	}
	min_0=-i;
	max_0=i;
	i=0;
	pb=b->p;
	pa=a->p;
	col2=(m->col>>3)<<3;
	for(j=0,pm=m->p;j<a->row;++j)
	{
		pak=*pa;//wtk_mati_at(a,j,0);
		pa+=a->col;
		tpb=pb;
		tpm=pm;
		pm+=m->col;
		if(pak<min_0 || pak>max_0)
		{
			pm2=tpm+col2;
			while(tpm<pm2)
			{
				tpm[0]=pak*tpb[0];
				tpm[1]=pak*tpb[1];
				tpm[2]=pak*tpb[2];
				tpm[3]=pak*tpb[3];
				tpm[4]=pak*tpb[4];
				tpm[5]=pak*tpb[5];
				tpm[6]=pak*tpb[6];
				tpm[7]=pak*tpb[7];
				tpm+=8;
				tpb+=8;
			}
			while(tpm<pm)
			{
				*(tpm++)=pak*(*(tpb++));
			}
		}else
		{
			memset(tpm,0,m->col<<2);
		}
	}
	pb+=b->col;
	for(i=1;i<b->row;++i,pb+=b->col)
	{
		pa=a->p+i;
		for(j=0,pm=m->p;j<a->row;++j)
		{
			//pak=wtk_mati_at(a,j,i);
			pak=*pa;
			pa+=a->col;
			tpb=pb;
			tpm=pm;
			pm+=m->col;
			if(pak<min_0 || pak>max_0)
			{
				pm2=tpm+col2;
				while(tpm<pm2)
				{
					tpm[0]+=pak*tpb[0];
					tpm[1]+=pak*tpb[1];
					tpm[2]+=pak*tpb[2];
					tpm[3]+=pak*tpb[3];
					tpm[4]+=pak*tpb[4];
					tpm[5]+=pak*tpb[5];
					tpm[6]+=pak*tpb[6];
					tpm[7]+=pak*tpb[7];
					tpm+=8;
					tpb+=8;
				}
				while(tpm<pm)
				{
					*(tpm++)+=pak*(*(tpb++));
				}
			}
		}
	}
}

void wtk_mati_multi3_x2_raw(wtk_mati_t *m,wtk_mati_t *a,wtk_mati_t *b,wtk_flat_cfg_t *cfg,int index)
{
	register int *tpb,*tpm,*pm;
	register int pak;
	int *pa;
	int *pb;
	int i,j;
	int min_0;
	int max_0;

	if(cfg->min_avg_scale==0)
	{
		i=0;
	}else
	{
		i=wtk_mati_avg(a)*cfg->min_avg_scale;
	}
	min_0=-i;
	max_0=i;
	i=0;
	pb=b->p-1;
	pa=a->p;
	for(j=0,pm=m->p-1;j<a->row;++j)
	{
		pak=*pa;//wtk_mati_at(a,j,0);
		pa+=a->col;
		tpb=pb;
		tpm=pm;
		pm+=m->col;
		if(pak<min_0 || pak>max_0)
		{
			while(tpm<pm)
			{
				*(++tpm)=pak*(*(++tpb));
			}
		}else
		{
			memset(tpm+1,0,m->col<<2);
		}
	}
	pb+=b->col;
	for(i=1;i<b->row;++i,pb+=b->col)
	{
		pa=a->p+i;
		for(j=0,pm=m->p-1;j<a->row;++j)
		{
			//pak=wtk_mati_at(a,j,i);
			pak=*pa;
			pa+=a->col;
			tpb=pb;
			tpm=pm;
			pm+=m->col;
			if(pak<min_0 || pak>max_0)
			{
				while(tpm<pm)
				{
					*(++tpm)+=pak*(*(++tpb));
				}
			}
		}
	}
}

void wtk_mati_multi3_x(wtk_mati_t *m,wtk_mati_t *a,wtk_mati_t *b)
{
	register int *pb,*pbe,*pa;
	register int t;
	int i,j;
	int *pas,*pm;

//	wtk_debug("%d/%d %d\n",wtk_mati_value_count(a,0),wtk_mati_value_count(b,0),a->row*a->col);
//	exit(0);
	//wtk_debug("[%d*%d] * [%d*%d] =[%d/%d]\n",a->row,a->col,b->row,b->col,m->row,m->col);
	pm=m->p-1;
	for(i=0;i<a->row;++i)
	{
		pas=a->p+i*a->col-1;
		for(j=0,pb=b->p-1;j<b->col;++j)
		{
			t=0;
			pbe=pb+b->row;
			pa=pas;
//			while(pbe-pb>=8)
//			{
//				t+=(*(pa++))*(*(pb++));
//				t+=(*(pa++))*(*(pb++));
//				t+=(*(pa++))*(*(pb++));
//				t+=(*(pa++))*(*(pb++));
//				t+=(*(pa++))*(*(pb++));
//				t+=(*(pa++))*(*(pb++));
//				t+=(*(pa++))*(*(pb++));
//				t+=(*(pa++))*(*(pb++));
//			}
			while(pb<pbe)
			{
				t+=(*(++pa))*(*(++pb));
			}
			//wtk_debug("[%d]\n",(int)(pm-m->p));
			*(++pm)=t;
		}
	}
}


void wtk_flat_process_layer_matrix_fixpoint2(wtk_flat_t *d,wtk_matrix_t *m)
{
	wtk_queue_node_t *n;
	wtk_dnn_layer_t *l=NULL;
	wtk_mati_t *output_i=NULL;
	wtk_mati_t *tmp_i;
	wtk_mati_t *wi;
	int i;
	float f;
//#define DEBUG_FLAT

	//wtk_dnn_trans_process(d->cfg->trans,m[1]);
	wtk_flat_trans_process(m,d->cfg->trans);
	//wtk_matrix_print(m);
	//exit(0);
	for(i=0,n=d->cfg->layer_q.pop;n;n=n->next,++i)
	{
		//wtk_debug("[%d/%d]\n",f->index,i);
		l=data_offset(n,wtk_dnn_layer_t,q_n);
#ifdef DEBUG_FLAT
		wtk_debug("============ %d type=%d ========\n",i,l->type);
#endif
		if(i==0)
		{
			if(d->cfg->use_fix_0_layer)
			{
				f=wtk_matrix_max_abs(m);
				f=d->cfg->max_0_w/f;
				//wtk_debug("use f=%f\n",f);
				wtk_mati_init(d->fix_0_int_input,m,f);
				wtk_mati_multi3(d->fix_0_mul,d->fix_0_int_input,l->fix_wb->w.i);
				//wtk_mati_print(d->fix_0_mul);
				//exit(0);
				//wtk_flat_process_dnn_fix_layer3(d,l,d->fix_0_i,d->fix_0_uc);
				if(l->type==wtk_fnn_sigmoid)
				{
					//wtk_flat_process_dnn_fix_layer4(d,l,d->fix_0_mul,d->fix_0_int_output,f);
					wtk_flat_process_dnn_fix_layer5(d,l,d->fix_0_mul,d->fix_0_int_output,f);
					output_i=d->fix_0_int_output;
					output_i->row=wtk_matrix_rows(m);
				}else
				{
					wtk_debug("layer->type not in list. %d\n", l->type);
					exit(0);
				}
				//wtk_mati_print(output_i);
				//exit(0);
			}else
			{
				wtk_flat_process_dnn_layer(d,l,m,d->fix_0_output1,i);
				//wtk_matrix_print(d->fix_0_output1);
				output_i=d->fix_output.i.fix_0_output2;
				output_i->row=wtk_matrix_rows(m);
				wtk_mati_init(output_i,d->fix_0_output1,d->cfg->max_b);
			}
		}else
		{
			tmp_i=d->fix_tmp[i-1];
			wi=l->fix_wb->w.i;
			tmp_i->row=output_i->row;
			tmp_i->col=wi->col;
			//wtk_mati_multi3(tmp_i,output_i,wi);
			//wtk_mati_multi3_x2(tmp_i,output_i,wi);
			if(d->cfg->use_fix_trans_matrix)
			{
				wtk_mati_multi3_x(tmp_i,output_i,wi);
			}else
			{
				//wtk_mati_multi3_x2_raw(tmp_i,output_i,wi,d->cfg,i);
				wtk_mati_multi3_x2(tmp_i,output_i,wi,d->cfg,i);
			}
			//wtk_mati_print(tmp_i);
			wtk_flat_process_dnn_fix_layer2(d,l,tmp_i,d->fix_output.i.fix_output[i-1]);
			//wtk_mati_print(tmp_i);
			//exit(0);
			//wtk_mati_print(d->fix_tmp[i-1]);
			switch(l->type)
			{
			case wtk_fnn_sigmoid:
				output_i=d->fix_output.i.fix_output[i-1];
				break;
			case wtk_fnn_relu:
				//TODO
				break;
			case wtk_fnn_softmax:
				output_i=tmp_i;//d->fix_tmp[i-1];
				break;
			case wtk_fnn_linear:
				output_i=tmp_i;//d->fix_tmp[i-1];
				break;
			case wtk_fnn_rescale:
				break;
			case wtk_fnn_pnorm:
				break;
			default:
				break;
			}
		}
		if(d->dnn->cfg->use_lazy_out && !n->next->next)
		{
			wtk_flat_raise_dnn_lazy(d,output_i);
			//wtk_debug("row=%d col=%d\n",output_i->row,output_i->col);
			//wtk_debug("uc=%d\n",uc);
			return;
		}
	}
	wtk_flat_raise_dnn(d,output_i,l);
}

void wtk_flat_process_matrix(wtk_flat_t *d,wtk_matrix_t *input)
{
	//wtk_debug("use_fix_float=%d use_c=%d\n",d->cfg->use_fix_float,d->cfg->use_c);
	//exit(0);
	if(d->cfg->use_fix_float)
	{
		//wtk_debug("use_c=%d\n",d->cfg->use_c);
		if(d->cfg->use_c)
		{
			wtk_flat_process_layer_matrix_fixpoint(d,input);
		}else
		{
			wtk_flat_process_layer_matrix_fixpoint2(d,input);
		}
	}else
	{
		wtk_flat_process_layer_matrix(d,input);
	}
}


void wtk_flat_process_layer(wtk_flat_t *d,wtk_feat_t **pv,int npv,wtk_feat_t *f)
{
	//wtk_debug("=============== index=%d ===================\n",f->index);
	//wtk_debug("v=%d\n",wtk_vector_size(f->v));
	//print_float(f->v+1,120);
	//exit(0);
	++f->used;
	wtk_robin_push(d->robin,f);
	//wtk_debug("used=%d/%d\n",d->robin->used,d->robin->nslot);
	//wtk_debug("index=%d skip=%d\n",f->index,d->dnn->cfg->skip_frame);
	//wtk_debug("row=%d col=%d\n",wtk_matrix_rows(d->feat_matrix),wtk_matrix_cols(d->feat_matrix));
	if( (0 == d->dnn->cfg->skip_frame) || (f->index%d->dnn->cfg->skip_frame==1) )
	{
		++d->index;
		wtk_flat_feature_to_matirx(d,d->feat_matrix,d->index,pv,npv);
	}
	if(d->robin->nslot==d->robin->used)
	{
		wtk_matrix_rows(d->feat_matrix)=d->index;
		//wtk_debug("index=%d,cache=%d,robin=%d\n",d->index,d->cfg->cache_size,d->robin->used);
		wtk_flat_process_matrix(d,d->feat_matrix);
		//wtk_flat_process_layer_matrix_fixpoint(d,d->feat_matrix);
		d->index=0;
		//exit(0);
	}
}

void wtk_flat_process_layer_kaldi(wtk_flat_t *d,wtk_feat_t **pv,int npv,wtk_feat_t *f)
{
	//wtk_debug("===========index=%d================\n",f->index);
	if( (0 == d->dnn->cfg->skip_frame) || (f->index%d->dnn->cfg->skip_frame==1) )
	{
		++f->used;
		wtk_robin_push(d->robin,f);
		++d->index;
		wtk_flat_feature_to_matirx2(d,d->feat_matrix,d->index,pv,npv);

		if(d->robin->nslot==d->robin->used)
		{
			wtk_matrix_rows(d->feat_matrix)=d->index;
			wtk_flat_process_matrix(d,d->feat_matrix);
			d->index=0;
		}
	}
}



void wtk_flat_flush(wtk_flat_t *f)
{
	if(f->robin->used>0 && f->index>0)
	{
		wtk_matrix_rows(f->feat_matrix)=f->index;
		wtk_flat_process_matrix(f,f->feat_matrix);
		f->index=0;
	}
}

void wtk_flat_flush_end(wtk_flat_t *d)
{
	if(d->last_feature)
	{
		--d->last_feature->used;
		wtk_fextra_push_feature(d->dnn->parm,d->last_feature);
		d->last_feature=NULL;
	}
}

void wtk_flat_reset(wtk_flat_t *f)
{
	wtk_flat_flush(f);
	wtk_robin_reset(f->robin);
	f->index=0;
	f->last_feature=NULL;
}

float wtk_flat_get_dnn_value2(wtk_flat_t *d,wtk_feat_t *f,int index)
{
	wtk_dnn_layer_t *l;
	wtk_matc_t *matc;
	int *pi,*pe;
	signed char *pc;
	int t;
	float x;

	//wtk_debug("index=%d\n",index);
	l=d->last_l;
	matc=l->fix_wb->w.c;
	pi=(int*)(f->dnn_v+1);
	pe=pi+matc->row;
	pc=matc->p+index-1;
	t=0;
	while(pi<pe-8)
	{
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
	}
	while(pi<pe)
	{
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
	}
	if(l->fix_wb->b)
	{
		t+=l->fix_wb->b->p[index-1];
	}
	x=t*d->last_scale;
	return x;
}

#ifdef USE_ARM_X
float wtk_flat_get_dnn_value(wtk_flat_t *d,wtk_feat_t *f,int index)
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

float wtk_flat_get_dnn_value_char(wtk_flat_t *d,wtk_feat_t *f,int index)
{
	wtk_dnn_layer_t *l;
	wtk_matc_t *matc;
	register int *pi,*pe;
	register signed char *pc;
	register int t;
	float x;

	//wtk_debug("index=%d\n",index);
	l=d->last_l;
	matc=l->fix_wb->w.c;
	pi=(int*)(f->dnn_v+1);
	pe=pi+matc->row;
	pc=matc->p+(index-1)*matc->row;
	t=0;
	while(pi<pe-8)
	{
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
	if(l->fix_wb->b)
	{
		t+=l->fix_wb->b->p[index-1];
	}
	x=t*d->last_scale;
	//wtk_debug("t=%d/%f %f b=%d\n",t,d->last_scale,x,(int)sizeof(int));
	return x;
}

float wtk_flat_get_dnn_value4(wtk_flat_t *d,wtk_feat_t *f,int index)
{
	wtk_dnn_layer_t *l;
	wtk_mati_t *matc;
	int *pi,*pe;
	int *pc;
	int t;
	float x;

	//wtk_debug("index=%d\n",index);
	//index=1;
	l=d->last_l;
	matc=l->fix_wb->w.i;
	pi=(int*)(f->dnn_v+1);
	pe=pi+matc->row;
	pc=matc->p+index-1;
	t=0;
	while(pi<pe-8)
	{
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
	}
	while(pi<pe)
	{
		t+=(*(pi++))*(*pc);
		pc+=matc->col;
	}
	if(l->fix_wb->b)
	{
		//wtk_debug("t=%d/%d\n",t,l->fix_wb->b->p[index-1]);
		t+=l->fix_wb->b->p[index-1];
	}
	x=t*d->last_scale;
	//exit(0);
	return x;
}

float wtk_flat_get_dnn_value_int(wtk_flat_t *d,wtk_feat_t *f,int index)
{
	wtk_dnn_layer_t *l;
	wtk_mati_t *matc;
	int *pi,*pe;
	int *pc;
	int t;
	float x;

	//wtk_debug("index=%d\n",index)
	l=d->last_l;
	matc=l->fix_wb->w.i;
	pi=(int*)(f->dnn_v+1);
	pe=pi+matc->row;
	pc=matc->p+(index-1)*matc->row;
	t=0;
	while(pi<pe-8)
	{
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
	if(l->fix_wb->b)
	{
		//wtk_debug("t=%d /%d\n",t,l->fix_wb->b->p[index-1]);
		t+=l->fix_wb->b->p[index-1];
	}
	x=t*d->last_scale;
	//wtk_debug("t=%d/%f %f b=%d\n",t,d->last_scale,x,(int)sizeof(int));
	return x;
}

float wtk_flat_get_dnn_value(wtk_flat_t *d,wtk_feat_t *f,int index)
{
	if(d->cfg->use_c)
	{
		return wtk_flat_get_dnn_value_char(d,f,index);
	}else
	{
		return wtk_flat_get_dnn_value_int(d,f,index);
	}
}
#endif
