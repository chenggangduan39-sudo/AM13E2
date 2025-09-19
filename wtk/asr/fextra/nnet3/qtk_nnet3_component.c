#include "qtk_nnet3_component.h"
//#ifdef USE_BLAS
//#include "GotoBLAS2/common.h"
//#include "GotoBLAS2/cblas.h"
//#endif

#include "qtk_nnet3_fix.h"

int qtk_nnet3_component_update_common(wtk_source_t *src, wtk_strbuf_t *buf)
{
	int ret = 0;
	float f_val;
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if(wtk_str_equal_s(buf->data, buf->pos, "<IsGradient>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<MaxChange>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<L2Regularize>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<LearningRate>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);

		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<LearningRateFactor>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);

		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<MaxChange>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<L2Regularize>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<LearningRate>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);

		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}
	return ret;
}

int qtk_affine_global_component_read2(nnet3_component_t* com, wtk_source_t *src,
                wtk_strbuf_t *buf,float max_w, unsigned int is_fixed, int fixed_nbytes, int use_custom_acc, int porder)
{
	int ret = 0;
	qtk_affine_global_component_t* comp;
	comp =(qtk_affine_global_component_t*)com->component;
	switch(comp->type)
	{
		case QTK_AffineComponent:
			qtk_affine_component_read2(comp, src, buf, max_w, is_fixed, fixed_nbytes, use_custom_acc, porder);
			break;
		case QTK_FixedAffineComponent:
			qtk_fixed_affine_componet_read(comp,src,buf,max_w,is_fixed);
			break;
		case QTK_NaturalGradientAffineComponent:
			qtk_natural_gradient_affine_component_read2(comp,src,buf,max_w,is_fixed, fixed_nbytes, use_custom_acc, porder);
			break;
		case QTK_LinearComponent:
			qtk_linear_component_read(comp,src,buf,max_w,is_fixed);
			break;
		default:
			break;
	}
	return ret;
}

int qtk_affine_global_component_read(nnet3_component_t* com, wtk_source_t *src,
                wtk_strbuf_t *buf,float max_w, unsigned int is_fixed)
{
	return qtk_affine_global_component_read2(com, src, buf, max_w, is_fixed, sizeof(short), 0, 0);
}

void* qtk_nnet3_update_f2fix(qtk_blas_matrix_t *m, int row, int col, float max_w, int fixed_nbytes)
{
	void* mfix;

	mfix=0;
	switch(fixed_nbytes){
		case sizeof(short):
			mfix = wtk_mats_new(row,col);
			qtk_nnet3_update_fix_f2s(m,mfix,max_w);
		break;
		case sizeof(int):
			mfix = wtk_mati_new(row,col);
			qtk_nnet3_update_fix_f2i(m,mfix,max_w, fixed_nbytes);
			break;
	}
	return mfix;
}

int qtk_fixed_affine_componet_read2(qtk_affine_global_component_t* com, wtk_source_t *src,
		wtk_strbuf_t *buf,float max_w, unsigned int is_fixed, int fixed_nbytes)
{
	int ret = 0;
	int row, col;
	qtk_blas_matrix_t *m = 0;
	//wtk_mats_t *ms=0;
	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos,
			"<FixedAffineComponent>")&&!wtk_str_equal_s(buf->data,buf->pos,"<LinearParams>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "FM"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &row, 1, 1);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);

	m = qtk_blas_matrix_new(row, col);
	ret = wtk_source_read_float_little(src, m->m, row * col, 1);
	if(is_fixed)
	{
		com->ws = (wtk_mats_t*)qtk_nnet3_update_f2fix(m, row, col, max_w, fixed_nbytes);
		qtk_blas_matrix_delete(m);
	}else
	{
		com->w = m;
	}

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		wtk_debug("read failed.\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<BiasParams>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		wtk_debug("read failed.\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "FV"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	//wtk_debug("%d\n",col);

	m = qtk_blas_matrix_new(1, col);

	ret = wtk_source_read_float_little(src, m->m, 1 * col, 1);
	com->b = m;
	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		wtk_debug("read failed.\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "</FixedAffineComponent>"))
	{
		ret = -1;
		goto end;
	}

	end: return ret;
}

int qtk_fixed_affine_componet_read(qtk_affine_global_component_t* com, wtk_source_t *src,
		wtk_strbuf_t *buf,float max_w, unsigned int is_fixed)
{
	return qtk_fixed_affine_componet_read2(com, src, buf, max_w, is_fixed, sizeof(short));
}

static void qtk_matrix_transpose_customer(qtk_blas_matrix_t*m, qtk_blas_matrix_t*rm, int porder)
{
	float *v,*p;
	int i,j,col2, k, r, h,d,step;
	//Transposed
	for (i=0; i< m->col; i++)
	{
		for(j=0; j<m->row;j++)
		{
			v=m->m+j*m->col+i;
			*(rm->m+i*m->row + j) = *v;
		}
	}
    step = 1 << porder;
    col2= (rm->col >> porder) << porder ;
    p=m->m;
	for(k=0; k<col2/step; k++)
	{
	    for (i=0; i< rm->row; i++)
	    {
	    	for(j=k*step; j<(k+1)*step;j++)
	    	{
	        	v=rm->m+i*rm->col+j;
	        	*p++=*v;
	    	}
	    }
	}
	r=rm->col-col2;
	h=0;
	while(r>=4)
	{
		r=r-4;
	    for (i=0; i< rm->row; i++)
	    {
	    	for(j=0; j<4;j++)
	    	{
	        	v=rm->m+i*rm->col+k*step+h*4+j;
	        	*p++=*v;
	    	}
	    }
	    h++;
	}

	for(d=0; d<r; d++)
	{
	    for (i=0; i< rm->row; i++)
	    {
	        v=rm->m+i*rm->col+k*step+h*4+d;
	        *p++=*v;
	    }
	}
	m->row=rm->row;
	m->col=rm->col;
}

int qtk_affine_component_read2(qtk_affine_global_component_t* com, wtk_source_t *src,
		wtk_strbuf_t *buf,float max_w,unsigned int is_fixed, int fixed_nbytes, int use_custom_acc, int porder)
{
	int ret = 0;
	int row, col;
	qtk_blas_matrix_t *rm;
	qtk_blas_matrix_t *m = 0;
	qtk_nnet3_component_update_common(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos,
			"<AffineComponent>")&&!wtk_str_equal_s(buf->data,buf->pos,"<LinearParams>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		wtk_debug("read failed.\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "FM"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &row, 1, 1);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);

	m = qtk_blas_matrix_new(row, col);
	ret = wtk_source_read_float_little(src, m->m, row * col, 1);

	if(use_custom_acc)
	{
		//Transposed
		rm =qtk_blas_matrix_new(col,row);
		qtk_matrix_transpose_customer(m, rm, porder);
//		for (i=0; i< m->col; i++)
//		{
//			for(j=0; j<m->row;j++)
//			{
//				v=m->m+j*m->col+i;
//				*(rm->m+i*m->row + j) = *v;
//			}
//		}
//	    step = 1 << porder;
//	    col2= (rm->col >> porder) << porder ;
//	    p=m->m;
//		for(k=0; k<col2/step; k++)
//		{
//		    for (i=0; i< rm->row; i++)
//		    {
//		    	for(j=k*step; j<(k+1)*step;j++)
//		    	{
//		        	v=rm->m+i*rm->col+j;
//		        	*p++=*v;
//		    	}
//		    }
//		}
//		r=rm->col-col2;
//		h=0;
//		while(r>=4)
//		{
//			r=r-4;
//		    for (i=0; i< rm->row; i++)
//		    {
//		    	for(j=0; j<4;j++)
//		    	{
//		        	v=rm->m+i*rm->col+k*step+h*4+j;
//		        	*p++=*v;
//		    	}
//		    }
//		    h++;
//		}
//
//		for(d=0; d<r; d++)
//		{
//		    for (i=0; i< rm->row; i++)
//		    {
//		        v=rm->m+i*rm->col+k*step+h*4+d;
//		        *p++=*v;
//		    }
//		}
//		m->row=rm->row;
//		m->col=rm->col;
		qtk_blas_matrix_delete(rm);
	}

	if(is_fixed)
	{
		com->ws = (wtk_mats_t*)qtk_nnet3_update_f2fix(m, m->row, m->col, max_w, fixed_nbytes);
//		printf("row=%d col=%d\n", com->ws->row,com->ws->col);
//		for (i=0; i< com->ws->col; i++)
//		{
//			for(j=0; j<com->ws->row;j++)
//			{
//				printf("%d ", *((int*)com->ws->p+j*com->ws->col+i));
//			}
//			printf(" \n");
//		}
//		for (i=0; i< com->ws->row; i++)
//		{
//			for(j=0; j<com->ws->col;j++)
//			{
//				//printf("%d ", *((int*)com->ws->p+i*com->ws->col+j));
//				printf("%d ", *((int*)com->ws->p+i*com->ws->col+j));
//				//printf("%d ", wtk_float_round(*(m->m+i*m->col+j)*256));
//			}
//			printf(" \n");
//		}
		qtk_blas_matrix_delete(m);
	}else
	{
		com->w = m;
	}
	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		wtk_debug("read failed.\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<BiasParams>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "FV"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	//wtk_debug("%d\n",col);

	m = qtk_blas_matrix_new(1, col);

	ret = wtk_source_read_float_little(src, m->m, 1 * col, 1);
	com->b = m;
	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		wtk_debug("read failed.\n");
		goto end;
	}
	if (!wtk_str_equal_s(buf->data, buf->pos, "</AffineComponent>"))
	{
		ret = -1;
		goto end;
	}

	end: return ret;
}

int qtk_affine_component_read(qtk_affine_global_component_t* com, wtk_source_t *src,
		wtk_strbuf_t *buf,float max_w,unsigned int is_fixed)
{
	return qtk_affine_component_read2(com, src, buf, max_w, is_fixed, sizeof(short), 0, 0);
}

int qtk_linear_component_read2(qtk_affine_global_component_t* com, wtk_source_t* src,
		wtk_strbuf_t* buf,float max_w,unsigned int is_fixed, int fixed_nbytes)
{
	int ret = 0;
	int val, col, row;
	float f_val;
	qtk_blas_matrix_t *m = 0;
	//wtk_mats_t* ms=0;
	qtk_linear_component_t* comp;
	comp = (qtk_linear_component_t*) com->linear_com;
	comp->in =NULL;
	comp->out=NULL;

	qtk_nnet3_component_update_common(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Params>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "FM"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &row, 1, 1);

	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	m = qtk_blas_matrix_new(row, col);
	ret = wtk_source_read_float_little(src, m->m, row * col, 1);
	if(is_fixed)
	{
		com->ws = (wtk_mats_t*)qtk_nnet3_update_f2fix(m, row, col, max_w, fixed_nbytes);
		qtk_blas_matrix_delete(m);
	}else
	{
		com->w = m;
	}
	ret = wtk_source_read_string(src, buf);
	if (wtk_str_equal_s(buf->data, buf->pos, "<OrthonormalConstraint>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		comp->orthonormal_constraint = f_val;
		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<UseNaturalGradient>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<RankInOut>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &val, 1, 1);

		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &val, 1, 1);

		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<Alpha>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<NumSamplesHistory>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<UpdatePeriod>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &val, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}
	if (!wtk_str_equal_s(buf->data, buf->pos, "</LinearComponent>"))
	{
		ret = -1;
	}
	end: return ret;
}

int qtk_linear_component_read(qtk_affine_global_component_t* com, wtk_source_t* src,
		wtk_strbuf_t* buf,float max_w,unsigned int is_fixed)
{
	return qtk_linear_component_read2(com, src, buf, max_w, is_fixed, sizeof(short));
}

int qtk_natural_gradient_affine_component_read2(qtk_affine_global_component_t* com,
		wtk_source_t *src, wtk_strbuf_t *buf,float max_w,unsigned int is_fixed, int fixed_nbytes, int use_custom_acc, int porder)
{
	int ret = 0;
	int row, col, val;
	float f_val;
	double d_val;
	qtk_blas_matrix_t *rm;
	qtk_blas_matrix_t *m = 0;
	//wtk_mats_t* ms=0;
	qtk_natural_gradient_affine_component_t* comp;
	OnlineNaturalGradient_t* in;
	OnlineNaturalGradient_t* out;

	comp = (qtk_natural_gradient_affine_component_t*) com->nga_com;
	in = (OnlineNaturalGradient_t*) wtk_malloc(sizeof(OnlineNaturalGradient_t));
	comp->in = in;
	out = (OnlineNaturalGradient_t*) wtk_malloc(
			sizeof(OnlineNaturalGradient_t));
	;
	comp->out = out;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (wtk_str_equal_s(buf->data, buf->pos, "<MaxChange>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		//wtk_debug("%f\n",f_val);
		comp->max_change = f_val;
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<L2Regularize>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		//wtk_debug("%f\n",f_val);
		comp->l2_regularize = f_val;
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<LearningRate>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		//wtk_debug("%f\n",f_val);
		comp->learning_rate = f_val;

		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<LearningRateFactor>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		//wtk_debug("%f\n",f_val);
		comp->learning_rate_factor = f_val;

		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}
//TODO not readable
	if (wtk_str_equal_s(buf->data, buf->pos, "<MaxChange>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		//wtk_debug("%f\n",f_val);
		comp->max_change = f_val;
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<L2Regularize>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		//wtk_debug("%f\n",f_val);
		comp->l2_regularize = f_val;
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<LearningRate>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		//wtk_debug("%f\n",f_val);
		comp->learning_rate = f_val;

		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "<LinearParams>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "FM"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &row, 1, 1);
	//wtk_debug("%d\n",row);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	//wtk_debug("%d\n",col);

	m = qtk_blas_matrix_new(row, col);
	ret = wtk_source_read_float_little(src, m->m, row * col, 1);
	//test reset
	if(use_custom_acc)
	{
		rm =qtk_blas_matrix_new(col,row);
		qtk_matrix_transpose_customer(m, rm, porder);
//
//		for (i=0; i< m->col; i++)
//		{
//			for(j=0; j<m->row;j++)
//			{
//				v=m->m+j*m->col+i;
//				*(rm->m+i*m->row + j) = *v;
//			}
//		}
//	    step = 1 << porder;
//	    col2= (rm->col >> porder) << porder ;
//	    p=m->m;
//		for(k=0; k<col2/step; k++)
//		{
//		    for (i=0; i< rm->row; i++)
//		    {
//		    	for(j=k*step; j<(k+1)*step;j++)
//		    	{
//		        	v=rm->m+i*rm->col+j;
//		        	*p++=*v;
//		    	}
//		    }
//		}
//		r=rm->col-col2;
//		h=0;
//		while(r>=4)
//		{
//			r=r-4;
//		    for (i=0; i< rm->row; i++)
//		    {
//		    	for(j=0; j<4;j++)
//		    	{
//		        	v=rm->m+i*rm->col+k*step+h*4+j;
//		        	*p++=*v;
//		    	}
//		    }
//		    h++;
//		}
//
//		for(d=0; d<r; d++)
//		{
//		    for (i=0; i< rm->row; i++)
//		    {
//		        v=rm->m+i*rm->col+k*step+h*4+d;
//		        *p++=*v;
//		    }
//		}
//		m->row=rm->row;
//		m->col=rm->col;
		qtk_blas_matrix_delete(rm);
	}
	if(is_fixed)
	{
		com->ws = (wtk_mats_t*)qtk_nnet3_update_f2fix(m, m->row, m->col, max_w, fixed_nbytes);
//		printf("row=%d col=%d\n", com->ws->row,com->ws->col);
//		for (i=0; i< com->ws->col; i++)
//		{
//			for(j=0; j<com->ws->row;j++)
//			{
//				printf("%d ", *((int*)com->ws->p+j*com->ws->col+i));
//			}
//			printf(" \n");
//		}
//		for (i=0; i< com->ws->row; i++)
//		{
//			for(j=0; j<com->ws->col;j++)
//			{
//				//printf("%d ", *(com->ws->p+i*com->ws->col+j));
//				printf("%d ", *((int*)com->ws->p+i*com->ws->col+j));
//				//printf("%d ", wtk_float_round(*(m->m+i*m->col+j)*256));
//			}
//			printf(" \n");
//		}
		qtk_blas_matrix_delete(m);
	}else
	{
		com->w = m;
	}

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<BiasParams>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	if (!wtk_str_equal_s(buf->data, buf->pos, "FV"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	//wtk_debug("%d\n",col);
	m = qtk_blas_matrix_new(1, col);
	ret = wtk_source_read_float_little(src, m->m, 1 * col, 1);
	com->b = m;
	//wtk_debug("%f\n",comp->b[1][1]);

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	if (!wtk_str_equal_s(buf->data, buf->pos, "<RankIn>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &(in->rank), 1, 1);

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	if (!wtk_str_equal_s(buf->data, buf->pos, "<RankOut>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &(out->rank), 1, 1);
	comp->orthonormal_constraint = 0.0;

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (wtk_str_equal_s(buf->data, buf->pos, "<OrthonormalConstraint>"))
	{
		ret = wtk_source_read_int(src, &val, 1, 0);
		comp->orthonormal_constraint = val;
		ret = wtk_source_read_string(src, buf);
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "<UpdatePeriod>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &val, 1, 1);
	in->update_period = out->update_period = val;
	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumSamplesHistory>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &val, 1, 1);
	in->num_samples_history = out->num_samples_history = val;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Alpha>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_float_little(src, &f_val, 1, 1);
	in->alpha = out->alpha = f_val;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (wtk_str_equal_s(buf->data, buf->pos, "<MaxChangePerSample>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<IsGradient>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);//TODO kaldi is bool
		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<UpdateCount>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_double_little(src, &d_val, 1, 1);
		ret = wtk_source_read_string(src, buf);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_double_little(src, &d_val, 1, 1);
		ret = wtk_source_read_string(src, buf);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_double_little(src, &d_val, 1, 1);
	}

	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos,
			"</NaturalGradientAffineComponent>"))
	{
		ret = -1;
		goto end;
	}

	end: return ret;
}

int qtk_natural_gradient_affine_component_read(qtk_affine_global_component_t* com,
		wtk_source_t *src, wtk_strbuf_t *buf,float max_w,unsigned int is_fixed)
{
	return qtk_natural_gradient_affine_component_read2(com, src, buf, max_w, is_fixed, sizeof(short), 0, 0);
}

int qtk_rectified_linear_component_read(nnet3_component_t* com,
		wtk_source_t *src, wtk_strbuf_t *buf)
{
	int ret = 0;
	int val, col;
	wtk_matrix_t *m = 0;
	double d;
	float f;
	qtk_rectified_linear_component_t* comp;

	comp = (qtk_rectified_linear_component_t*) com->component;
	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Dim>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &val, 1, 1);
	//wtk_debug("%d\n",val);
	comp->dim = val;
	comp->block_dim = val;
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (wtk_str_equal_s(buf->data, buf->pos, "<BlockDim>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_int_little(src, &val, 1, 1);
		comp->block_dim = val;
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "<ValueAvg>"))
{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "FV"))
	{
		ret = -1;
		goto end;
	}

	//ret=wtk_source_read_int(src,&row,1,0);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	m = wtk_matrix_new(1, col);
	ret = wtk_source_read_matrix_little(src, m, 1);
	comp->value_sum = m;

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "<DerivAvg>"))
	{
		//wtk_debug("read failed.\n");
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "FV"))
	{
		//wtk_debug("read failed.\n");
		ret = -1;
		goto end;
	}

	//ret=wtk_source_read_int(src,&row,1,0);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	m = wtk_matrix_new(1, col);
	ret = wtk_source_read_matrix_little(src, m, 1);
	comp->deriv_sum = m;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "<Count>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_double_little(src, &d, 1, 1);
	comp->count = d;
	comp->oderiv_sum = NULL;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<OderivRms>"))
	{
		ret = wtk_source_read_string(src, buf);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &col, 1, 1);
		m = wtk_matrix_new(1, col);
		ret = wtk_source_read_matrix_little(src, m, 1);
		comp->oderiv_sum = m;
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_double_little(src, &d, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (wtk_str_equal_s(buf->data, buf->pos, "<NumDimsSelfRepaired>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_double_little(src, &d, 1, 1);

		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<NumDimsProcessed>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_double_little(src, &d, 1, 1);
		comp->num_dims_processed_ = d;
		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<SelfRepairLowerThreshold>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f, 1, 1);
		comp->self_repair_lower_threshold_ = f;
		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<SelfRepairUpperThreshold>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f, 1, 1);
		comp->self_repair_upper_threshold_ = f;
		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<SelfRepairScale>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f, 1, 1);
		comp->self_repair_scale_ = f;
		ret = wtk_source_read_string(src, buf);
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "</RectifiedLinearComponent>"))
	{
		ret = -1;
		goto end;
	}

	end: return ret;
}

int qtk_normalize_component_read(nnet3_component_t* com, wtk_source_t *src,
		wtk_strbuf_t *buf)
{
	int ret = 0;
	int val;
	float f;
	qtk_normalize_component_t* comp;

	comp = (qtk_normalize_component_t*) com->component;
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	if (!wtk_str_equal_s(buf->data, buf->pos,
			"<Dim>")&&!wtk_str_equal_s(buf->data,buf->pos,"<InputDim>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &val, 1, 1);
	comp->input_dim_ = val;
	comp->block_dim_ = val;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (wtk_str_equal_s(buf->data, buf->pos, "<BlockDim>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_int_little(src, &val, 1, 1);
		comp->block_dim_ = val;
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<TargetRms>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f, 1, 1);
		comp->target_rms_ = f;
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<AddLogStddev>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		//ret=wtk_source_read_float_little(src,&f,1,1);
		comp->add_log_stddev_ = wtk_source_read_char(src);
		//wtk_debug("%d\n",comp->add_log_stddev_);
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<ValueAvg>"))
	{
		//TODO
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "</NormalizeComponent>"))
	{
		ret = -1;
	}

	end: return ret;
}

int qtk_activate_component_read(nnet3_component_t* com, wtk_source_t *src,
		wtk_strbuf_t *buf)
{

	int ret = 0;
	int val, col;
	wtk_matrix_t *m = 0;
	double d;
	float f;
	qtk_activate_component_t* comp;

	comp = (qtk_activate_component_t*) com->component;
	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Dim>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &val, 1, 1);
	//wtk_debug("%d\n",val);
	comp->dim = val;
	comp->block_dim = val;
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (wtk_str_equal_s(buf->data, buf->pos, "<BlockDim>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_int_little(src, &val, 1, 1);
		comp->block_dim = val;
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}

	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "<ValueAvg>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "FV"))
	{
		ret = -1;
		goto end;
	}

	//ret=wtk_source_read_int(src,&row,1,0);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	m = wtk_matrix_new(1, col);
	ret = wtk_source_read_matrix_little(src, m, 1);
	comp->value_sum = m;

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "<DerivAvg>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "FV"))
	{
		ret = -1;
		goto end;
	}

	//ret=wtk_source_read_int(src,&row,1,0);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	m = wtk_matrix_new(1, col);
	ret = wtk_source_read_matrix_little(src, m, 1);
	comp->deriv_sum = m;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "<Count>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_double_little(src, &d, 1, 1);
	comp->count = d;
	comp->oderiv_sum = NULL;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<OderivRms>"))
	{
		ret = wtk_source_read_string(src, buf);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &col, 1, 1);
		m = wtk_matrix_new(1, col);
		ret = wtk_source_read_matrix_little(src, m, 1);
		comp->oderiv_sum = m;
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_double_little(src, &d, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<NumDimsSelfRepaired>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_double_little(src, &d, 1, 1);
		comp->num_dims_self_repaired_ = d;
		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<NumDimsProcessed>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_double_little(src, &d, 1, 1);
		comp->num_dims_processed_ = d;
		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<SelfRepairLowerThreshold>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f, 1, 1);
		comp->self_repair_lower_threshold_ = f;
		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<SelfRepairUpperThreshold>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f, 1, 1);
		comp->self_repair_upper_threshold_ = f;
		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<SelfRepairScale>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f, 1, 1);
		comp->self_repair_scale_ = f;
		ret = wtk_source_read_string(src, buf);
	}

	end: return ret;
}

int qtk_element_wise_product_component_read(nnet3_component_t* com,
		wtk_source_t *src, wtk_strbuf_t *buf)
{
	int ret = 0;
	int val;

	qtk_element_wise_product_component_t* comp;
	comp = (qtk_element_wise_product_component_t*) com->component;
	ret = wtk_source_read_string(src, buf);

	if (!wtk_str_equal_s(buf->data, buf->pos, "<InputDim>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &val, 1, 1);
	comp->input_dim = val;

	ret = wtk_source_read_string(src, buf);

	if (!wtk_str_equal_s(buf->data, buf->pos, "<OutputDim>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &val, 1, 1);
	comp->output_dim = val;

	ret = wtk_source_read_string(src, buf);

	if (!wtk_str_equal_s(buf->data, buf->pos,
			"</ElementwiseProductComponent>"))
	{
		ret = -1;
		goto end;
	}

	end: return ret;
}

int qtk_natural_gradient_per_element_scale_component_read(
		nnet3_component_t* com, wtk_source_t *src, wtk_strbuf_t *buf,float max_w,unsigned int is_fixed)
{
	int ret = 0;
	int val, col;
	float f_val;
	wtk_matrix_t *m = 0;
	wtk_mats_t* ms=0;
	qtk_natural_gradient_per_element_scale_component_t* comp;
	comp = (qtk_natural_gradient_per_element_scale_component_t*) com->component;

	qtk_nnet3_component_update_common(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Params>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "FV"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	m = wtk_matrix_new(1, col);
	ret = wtk_source_read_matrix_little(src, m, 1);
	if(is_fixed)
	{
		ms = wtk_mats_new(1,col);
		qtk_nnet3_update_fix_f2s2(m,ms,max_w);
		comp->scale2 = ms;
		wtk_matrix_delete(m);
	}else
	{
		comp->scale = m;
	}
	ret = wtk_source_read_string(src, buf);
	if (wtk_str_equal_s(buf->data, buf->pos, "<IsGradient>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		//ret=wtk_source_read_int_little(src,&val,1,1);//TODO kaldi is bool
		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<Rank>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_int_little(src, &val, 1, 1);	//TODO kaldi is bool
		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<UpdatePeriod>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_int_little(src, &val, 1, 1);	//TODO kaldi is bool
		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<NumSamplesHistory>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);//TODO kaldi is bool
		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<Alpha>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);//TODO kaldi is bool
		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<MaxChangePerMinibatch>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);//TODO kaldi is bool
		ret = wtk_source_read_string(src, buf);
	}

	if (!wtk_str_equal_s(buf->data, buf->pos,
			"</NaturalGradientPerElementScaleComponent>"))
	{
		ret = -1;
	}

	end: return ret;
}

int qtk_dropout_component_read(nnet3_component_t* com, wtk_source_t *src,
		wtk_strbuf_t *buf)
{
	int ret = 0;
	int val;
	float f_val;
	qtk_dropout_component_t *comp;
	comp = (qtk_dropout_component_t*) com->component;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Dim>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &val, 1, 1);
	comp->dim = val;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<DropoutProportion>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_float_little(src, &f_val, 1, 1);
	comp->dropout_proportion = f_val;

	ret = wtk_source_read_string(src, buf);
	if (wtk_str_equal_s(buf->data, buf->pos, "<DropoutPerFrame>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		//ret=wtk_source_read_float_little(src,&f_val,1,1);
		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<TestMode>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		//ret=wtk_source_read_float_little(src,&f_val,1,1);
		ret = wtk_source_read_string(src, buf);
	}
	if (!wtk_str_equal_s(buf->data, buf->pos, "</DropoutComponent>"))
	{
		ret = -1;
	}

	end: return ret;
}

int qtk_backprop_truncation_component_read(nnet3_component_t* com,
		wtk_source_t *src, wtk_strbuf_t *buf)
{
	int ret = 0;
	int val;
	float f_val;
	double d;
	qtk_backprop_truncation_component_t* comp =
			(qtk_backprop_truncation_component_t*) com->component;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Dim>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &val, 1, 1);
	comp->dim = val;

	ret = wtk_source_read_string(src, buf);
	if (wtk_str_equal_s(buf->data, buf->pos, "<Scale>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		comp->scale = f_val;
		ret = wtk_source_read_string(src, buf);
	} else {
		comp->scale = 1.0;
	}
	if (!wtk_str_equal_s(buf->data, buf->pos, "<ClippingThreshold>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_float_little(src, &f_val, 1, 1);
	comp->clipping_threshold = f_val;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<ZeroingThreshold>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_float_little(src, &f_val, 1, 1);

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<ZeroingInterval>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &val, 1, 1);

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<RecurrenceInterval>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &val, 1, 1);

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumElementsClipped>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_double_little(src, &d, 1, 1);

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumElementsZeroed>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_double_little(src, &d, 1, 1);

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumElementsProcessed>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_double_little(src, &d, 1, 1);

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumZeroingBoundaries>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_double_little(src, &d, 1, 1);

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos,
			"</BackpropTruncationComponent>"))
	{
		ret = -1;
		goto end;
	}

	end: return ret;
}

int qtk_noop_component_read(nnet3_component_t* com, wtk_source_t* src,
		wtk_strbuf_t* buf)
{
	int val, col, ret = 0;
	float f_val;
	double d;
	wtk_matrix_t *m = 0;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Dim>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &val, 1, 1);

	ret = wtk_source_read_string(src, buf);

	if(wtk_str_equal_s(buf->data, buf->pos, "<BackpropScale>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		ret = wtk_source_read_string(src, buf);
		return 0;
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<BlockDim>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &val, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}
	if (!wtk_str_equal_s(buf->data, buf->pos, "<ValueAvg>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		goto end;
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "FV"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	m = wtk_matrix_new(1, col);
	ret = wtk_source_read_matrix_little(src, m, 1);
	wtk_matrix_delete(m);

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		goto end;
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "<DerivAvg>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		goto end;
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "FV"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	m = wtk_matrix_new(1, col);
	ret = wtk_source_read_matrix_little(src, m, 1);
	wtk_matrix_delete(m);
	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		goto end;
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "<Count>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_double_little(src, &d, 1, 1);

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		goto end;
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<OderivRms>"))
	{
		ret = wtk_source_read_string(src, buf);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &col, 1, 1);
		m = wtk_matrix_new(1, col);
		ret = wtk_source_read_matrix_little(src, m, 1);
		wtk_matrix_delete(m);
		ret = wtk_source_read_string(src, buf);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_double_little(src, &d, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<NumDimsSelfRepaired>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_double_little(src, &d, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<NumDimsProcessed>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_double_little(src, &d, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<SelfRepairLowerThreshold>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<SelfRepairUpperThreshold>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<SelfRepairScale>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}
	if (!wtk_str_equal_s(buf->data, buf->pos, "</NoOpComponent>"))
	{
		ret = -1;
	}

	end: return ret;
}

/*
 void qtk_batchnorm_res_generate(qtk_batch_norm_component_t* comp)
 {
 int col;

 col=comp->stats_sum->col;
 cblas_sgbmv(CblasRowMajor,CblasNoTrans,col,col,0,0,1.0,comp->stats_sum->m,1,comp->stats_sum->m,1,1.0,comp->stats_sumsq->m,1);
 cblas_sscal(col,comp->count,comp->stats_sum->m,1);
 cblas_sscal(col,comp->count,comp->stats_sumsq->m,1);

 comp->offset=qtk_blas_matrix_new(1,comp->stats_sum->col);
 comp->scale=qtk_blas_matrix_new(1,comp->stats_sumsq->col);
 comp->offset->col=comp->stats_sum->col;
 comp->scale->col=comp->stats_sumsq->col;
 comp->scale->row=comp->offset->col=1;
 memcpy(comp->offset->m,comp->stats_sum->m,sizeof(float)*comp->stats_sum->col);
 memcpy(comp->scale->m,comp->stats_sumsq->m,sizeof(float)*comp->stats_sumsq->col);

 cblas_sscal(col,-1.0/comp->count,comp->offset->m,1);
 cblas_sscal(col,1.0/comp->count,comp->scale->m,1);

 //cblas_Xgbmv(kNoTrans, comp->scale->col, comp->scale->col, 0, 0, -1.0, comp->offset->m, 1,comp->offset->m, 1, 1.0, comp->scale->m, 1);
 cblas_sgbmv(CblasRowMajor, CblasNoTrans, comp->scale->col, comp->scale->col, 0, 0, -1.0, comp->offset->m, 1,comp->offset->m, 1, 1.0, comp->scale->m, 1);
 qtk_blas_matrix_apply_floor(comp->scale,0.0);
 qtk_blas_matrix_add(comp->scale,comp->epsilon);
 qtk_blas_matrix_apply_power(comp->scale,-0.5);
 cblas_sscal(col,comp->target_rms,comp->scale->m,1);
 qtk_blas_matrix_mul_element(comp->offset,comp->scale);

 }
 */

int qtk_batch_norm_component_read(nnet3_component_t* com, wtk_source_t* src,
		wtk_strbuf_t* buf, float max_w, unsigned int is_fixed)
{
	qtk_batch_norm_component_t* comp =
			(qtk_batch_norm_component_t*) com->component;
	int val, col, ret = 0;
	float f;
	double d;
	qtk_blas_matrix_t *m = 0;
	wtk_mats_t* ms=0;
	
	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Dim>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &val, 1, 1);
	//wtk_debug("%d\n",val);
	comp->dim = val;

	ret = wtk_source_read_string(src, buf);
	if (wtk_str_equal_s(buf->data, buf->pos, "<BlockDim>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_int_little(src, &val, 1, 1);
		comp->block_dim = val;
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<Epsilon>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f, 1, 1);
		comp->epsilon = f;
		ret = wtk_source_read_string(src, buf);
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (wtk_str_equal_s(buf->data, buf->pos, "<TargetRms>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f, 1, 1);
		comp->target_rms = f;
		ret = wtk_source_read_string(src, buf);
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (wtk_str_equal_s(buf->data, buf->pos, "<TestMode>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_string(src, buf);
		//	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	}
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Count>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_double_little(src, &d, 1, 1);
	comp->count = d;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<StatsMean>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "FV"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	m = qtk_blas_matrix_new(1, col);
	wtk_source_read_float_little(src, m->m, col, 1);
	comp->stats_sum = m;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<StatsVar>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "FV"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	m = qtk_blas_matrix_new(1, col);
	wtk_source_read_float_little(src, m->m, col, 1);
	comp->stats_sumsq = m;

//only for decode,may cause error in training?
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Scale>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "FV"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	m = qtk_blas_matrix_new(1, col);
	wtk_source_read_float_little(src, m->m, col, 1);
	if(is_fixed)
	{
		ms = wtk_mats_new(1,col);
		qtk_nnet3_update_fix_f2s(m,ms,max_w);
		comp->scale2= ms;
		qtk_blas_matrix_delete(m);
	}else
	{
		comp->scale = m;
	}

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Offset>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "FV"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_char(src);
	//wtk_debug("%d\n",ret);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	m = qtk_blas_matrix_new(1, col);
	wtk_source_read_float_little(src, m->m, col, 1);
	if(is_fixed)
	{
		ms = wtk_mats_new(1,col);
		qtk_nnet3_update_fix_f2s(m,ms,max_w);
		comp->offset2 = ms;
		qtk_blas_matrix_delete(m);
	}else
	{
		comp->offset = m;
	}

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "</BatchNormComponent>"))
	{
		ret = -1;
		goto end;
	}

	//qtk_batchnorm_res_generate(comp);	

	end:
	comp->input = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
	comp->output = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
	comp->in_reshape = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
	comp->out_reshape = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
	return ret;
}

int qtk_lstm_nolinearity_component_read(nnet3_component_t* com,
		wtk_source_t* src, wtk_strbuf_t* buf,float max_w, unsigned int is_fixed)
{
	int ret = 0;
	int col, row;
	double d;
	qtk_blas_matrix_t *m = 0;
	qtk_lstm_nolinearity_component_t* comp;
	comp = (qtk_lstm_nolinearity_component_t*) com->component;

	qtk_nnet3_component_update_common(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Params>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "FM"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &row, 1, 1);

	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	m = qtk_blas_matrix_new(row, col);
	ret = wtk_source_read_float_little(src, m->m, row * col, 1);
/*	if(is_fixed)
	{
		ms =wtk_mats_new(row,col);
		qtk_nnet3_update_fix_f2s(m,ms,max_w);
		comp->params2=ms;
		qtk_blas_matrix_delete(m);
	}else*/
	{
		comp->params = m;
	}
	ret = wtk_source_read_string(src, buf);
	if (wtk_str_equal_s(buf->data, buf->pos, "<ValueAvg>"))
	{
		ret = wtk_source_read_string(src, buf);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &row, 1, 1);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &col, 1, 1);

		//dm=wtk_double_matrix_new(row,col);
		m = qtk_blas_matrix_new(row, col);
		wtk_source_read_float_little(src, m->m, col * row, 1);
		qtk_blas_matrix_delete(m);

		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<DerivAvg>"))
	{
		ret = wtk_source_read_string(src, buf);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &row, 1, 1);

		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &col, 1, 1);

		m = qtk_blas_matrix_new(row, col);
		wtk_source_read_float_little(src, m->m, col * row, 1);
		qtk_blas_matrix_delete(m);

		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<SelfRepairConfig>"))
	{
		ret = wtk_source_read_string(src, buf);
		ret = wtk_source_read_char(src);

		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &col, 1, 1);

		m = qtk_blas_matrix_new(row, col);
		ret = wtk_source_read_float_little(src, m->m, col, 1);
		qtk_blas_matrix_delete(m);

		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<SelfRepairProb>"))
	{
		ret = wtk_source_read_string(src, buf);
		ret = wtk_source_read_char(src);

		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &col, 1, 1);

		m = qtk_blas_matrix_new(1, col);
		wtk_source_read_float_little(src, m->m, col, 1);
		qtk_blas_matrix_delete(m);

		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<Count>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_double_little(src, &d, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "</LstmNonlinearityComponent>"))
	{
		ret = -1;
	}
	end: return ret;
}

int qtk_convolution_model_read(qtk_timeheight_convolution_component_t* comp,
		wtk_source_t* src, wtk_strbuf_t* buf,
		qtk_timeheight_convolution_model_t* model)
{
	int ret = 0;
	int v;
	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<ConvolutionModel>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumFiltersIn>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	model->num_filters_in = v;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumFiltersOut>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	model->num_filters_out = v;
	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<HeightIn>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	model->height_in = v;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<HeightOut>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	model->height_out = v;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<HeightSubsampleOut>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Offsets>"))
	{
		ret = -1;
		goto end;
	}
	//TODO read pair
	ret = wtk_source_read_string(src, buf);

	ret = wtk_source_read_string(src, buf);
	// if(!wtk_str_equal_s(buf->data,buf->pos,"</ConvolutionModel>"))
	// {
	//         ret=-1;
	//         goto end;
	// }

	end: return ret;
}

int qtk_timeheight_convolution_component_read(nnet3_component_t* com,
		wtk_source_t* src, wtk_strbuf_t* buf, float max_w,unsigned int is_fixed)
{
	int ret = 0;
	int val, col, row;
	float f_val;
	qtk_blas_matrix_t *m = 0;
	wtk_mats_t* ms=0;
	qtk_timeheight_convolution_component_t* comp;
	comp = (qtk_timeheight_convolution_component_t*) com->component;
	qtk_timeheight_convolution_model_t* model =
			(qtk_timeheight_convolution_model_t*) wtk_malloc(
					sizeof(qtk_timeheight_convolution_model_t));

	qtk_nnet3_component_update_common(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Model>"))
	{
		ret = -1;
		goto end;
	}
	qtk_convolution_model_read(comp, src, buf, model);
	comp->model = model;

	ret = wtk_source_read_string(src, buf);

	if (!wtk_str_equal_s(buf->data, buf->pos, "<LinearParams>"))
	{
		ret = wtk_source_read_string(src, buf);
		if (!wtk_str_equal_s(buf->data, buf->pos, "<LinearParams>"))
		{
			ret = -1;
			goto end;
		}
	}

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	if (!wtk_str_equal_s(buf->data, buf->pos, "FM"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &row, 1, 1);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);

	m = qtk_blas_matrix_new(row, col);
	ret = wtk_source_read_float_little(src, m->m, row * col, 1);
	if(is_fixed)
	{
		ms = wtk_mats_new(row,col);
		qtk_nnet3_update_fix_f2s(m,ms,max_w);
		comp->linear_params2 = ms;
		qtk_blas_matrix_delete(m);
	}else
	{
		comp->linear_params = m;
	}

	ret = wtk_source_read_string(src, buf);

	if (!wtk_str_equal_s(buf->data, buf->pos, "<BiasParams>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		//wtk_debug("read failed.\n");
		goto end;
	}
	if (!wtk_str_equal_s(buf->data, buf->pos, "FV"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);

	m = qtk_blas_matrix_new(1, col);
	ret = wtk_source_read_float_little(src, m->m, 1 * col, 1);
/*	if(is_fixed)
	{
		ms =wtk_mats_new(row,col);
		qtk_nnet3_update_fix_f2s(m,ms,max_w);
		comp->bias_params2 =ms;
		qtk_blas_matrix_delete(m);
	}else*/
	{
		comp->bias_params = m;
	}

	ret = wtk_source_read_string(src, buf);
	if (wtk_str_equal_s(buf->data, buf->pos, "<MaxMemoryMb>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<UseNaturalGradient>"))
	{
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		ret = wtk_source_read_char(src);
		//wtk_debug("%d\n",ret);
		//ret=wtk_source_read_int_little(src,&val,1,1);
		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<NumMinibatchesHistory>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}
	if (wtk_str_equal_s(buf->data, buf->pos, "<AlphaInOut>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_float_little(src, &f_val, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}

	if (wtk_str_equal_s(buf->data, buf->pos, "<RankInOut>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &val, 1, 1);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &val, 1, 1);
		ret = wtk_source_read_string(src, buf);
	}

	if (!wtk_str_equal_s(buf->data, buf->pos,
			"</TimeHeightConvolutionComponent>"))
	{
		ret = -1;
	}

	end:
	comp->input = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
	comp->output = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
	comp->output_shape = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));

	return ret;
}

int qtk_scale_offset_component_read(nnet3_component_t* com, wtk_source_t* src,
		wtk_strbuf_t* buf, float max_w, unsigned int is_fixed)
{
	int ret = 0;
	int val, col;
	float *f;
	qtk_blas_matrix_t *m = 0;
	wtk_mats_t* ms=0;
	qtk_scale_offset_component_t* comp;

	comp = (qtk_scale_offset_component_t*) com->component;
	qtk_nnet3_component_update_common(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Dim>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &val, 1, 1);
	comp->dim = val;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Scales>"))
	{
		ret = -1;
	}
	ret = wtk_source_read_string(src, buf);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	m = qtk_blas_matrix_new(1, col);
	ret = wtk_source_read_float_little(src, m->m, 1 * col, 1);
	if(is_fixed)
	{
		ms =wtk_mats_new(1,col);
		qtk_nnet3_update_fix_f2s(m,ms,max_w);
		comp->scales2 =ms;
		qtk_blas_matrix_delete(m);
	}else
	{
		comp->scales = m;
	}

	comp->nonzero_scales = qtk_blas_matrix_new(1, col);
	for (val = 0; val < col; val++)
	{
		f = m->m + val;
		if (*f >= 0.0001 || *f <= -0.0001)
		{
			*(comp->nonzero_scales->m + val) = *f;
		} else if (*f >= 0.0)
		{
			*(comp->nonzero_scales->m + val) = 0.0001;
		} else {
			*(comp->nonzero_scales->m + val) = -0.0001;
		}
	}

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Offsets>"))
	{
		ret = -1;
	}
	ret = wtk_source_read_string(src, buf);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	m = qtk_blas_matrix_new(1, col);
	ret = wtk_source_read_float_little(src, m->m, 1 * col, 1);
        if(is_fixed)
        {
                ms =wtk_mats_new(1,col);
                qtk_nnet3_update_fix_f2s(m,ms,max_w);
                comp->offsets2 = ms;
                qtk_blas_matrix_delete(m);
        }else
	{
		comp->offsets = m;
	}

	ret = wtk_source_read_string(src, buf);
	if (wtk_str_equal_s(buf->data, buf->pos, "<UseNaturalGradient>"))
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_string(src, buf);
	}

	if (!wtk_str_equal_s(buf->data, buf->pos, "<Rank>"))
	{
		ret = -1;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &val, 1, 1);

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "</ScaleAndOffsetComponent>"))
	{
		ret = -1;
	}
	end: return ret;
}

int qtk_max_pooling_component_read(nnet3_component_t* com,wtk_source_t* src,wtk_strbuf_t* buf,float max_w,unsigned int is_fixed)
{
	qtk_max_pooling_component_t *comp;
	int val,ret=0;

	comp = (qtk_max_pooling_component_t*)com->component;
	
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<InputXDim>"))
	{
		ret=-1;
		goto end;
	}
	ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
    comp->input_x_dim = val;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<InputYDim>"))
	{
		ret=-1;
		goto end;
	}
	ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
	comp->input_y_dim = val;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<InputZDim>"))
	{
		ret=-1;
		goto end;
	}
	ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
    comp->input_z_dim = val;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<PoolXSize>"))
	{
		ret=-1;
		goto end;
	}
	ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
    comp->pool_x_size = val;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<PoolYSize>"))
	{
		ret=-1;
		goto end;
	}
	ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
    comp->pool_y_size = val;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<PoolZSize>"))
	{
		ret=-1;
		goto end;
	}
	ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
    comp->pool_z_size = val;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<PoolXStep>"))
	{
		ret=-1;
		goto end;
	}
	ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
    comp->pool_x_step = val;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<PoolYStep>"))
	{
		ret=-1;
		goto end;
	}
	ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
    comp->pool_y_step = val;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<PoolZStep>"))
	{
		ret=-1;
		goto end;
	}
	ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
    comp->pool_z_step = val;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	comp->pool_size = comp->pool_x_size * comp->pool_y_size * comp->pool_z_size;
	int num_pools_x = 1 + (comp->input_x_dim - comp->pool_x_size) / comp->pool_x_step;
	int num_pools_y = 1 + (comp->input_y_dim - comp->pool_y_size) / comp->pool_y_step;
	int num_pools_z = 1 + (comp->input_z_dim - comp->pool_z_size) / comp->pool_z_step;
	comp->output_dim = num_pools_x * num_pools_y * num_pools_z;

end:
	return ret;
}

int qtk_pnorm_component_read(nnet3_component_t* com,wtk_source_t* src,wtk_strbuf_t* buf,float max_w,unsigned int is_fixed)
{
    qtk_pnorm_component_t *comp;
    int val,ret=0;

    comp = (qtk_pnorm_component_t*)com->component;
    ret = wtk_source_read_string(src, buf);
    //wtk_debug("[%.*s]\n",buf->pos,buf->data);
    if (!wtk_str_equal_s(buf->data, buf->pos, "<InputDim>"))
    {
       ret=-1;
       goto end;
    }
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
    comp->input_dim = val;
	ret = wtk_source_read_string(src, buf);
    if (!wtk_str_equal_s(buf->data, buf->pos, "<OutputDim>"))
    {
       ret=-1;
       goto end;
    }
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
    comp->output_dim = val;
	ret = wtk_source_read_string(src, buf);
    if (!wtk_str_equal_s(buf->data, buf->pos, "</PnormComponent>"))
    {
       ret=-1;
       goto end;
    }
end:
	return ret;
}

int qtk_general_dropout_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed)
{
	qtk_general_dropout_component_t *comp;
    int val,ret=0;
    float f_val;

    comp = (qtk_general_dropout_component_t*)com->component;
    ret = wtk_source_read_string(src, buf);
    //wtk_debug("[%.*s]\n",buf->pos,buf->data);
    if (!wtk_str_equal_s(buf->data, buf->pos, "<Dim>"))
    {
       ret=-1;
       goto end;
    }
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
    comp->dim = val;
	ret = wtk_source_read_string(src, buf);
    if (!wtk_str_equal_s(buf->data, buf->pos, "<BlockDim>"))
    {
       ret=-1;
       goto end;
    }
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
    comp->block_dim = val;

	ret = wtk_source_read_string(src, buf);
    if (!wtk_str_equal_s(buf->data, buf->pos, "<TimePeriod>"))
    {
       ret=-1;
       goto end;
    }
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
    comp->time_period = val;

	ret = wtk_source_read_string(src, buf);
    if (!wtk_str_equal_s(buf->data, buf->pos, "<DropoutProportion>"))
    {
       ret=-1;
       goto end;
    }
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_float_little(src, &f_val, 1, 1);
    comp->dropout_proportion = f_val;


	ret = wtk_source_read_string(src, buf);
	comp->test_mode=0;
	comp->continuous=0;
	if(wtk_str_equal_s(buf->data, buf->pos, "<TestMode>"))
	{
		comp->test_mode=1;
		ret = wtk_source_read_string(src, buf);
	}

	if(wtk_str_equal_s(buf->data, buf->pos, "<Continuous>"))
	{
		comp->continuous=1;
		ret = wtk_source_read_string(src, buf);
	}

    if (!wtk_str_equal_s(buf->data, buf->pos, "</GeneralDropoutComponent>"))
    {
       ret=-1;
       goto end;
    }
end:
	return ret;
}

void qtk_nnet3_component_mat_add(qtk_blas_matrix_t *input,
		qtk_blas_matrix_t *output, qtk_nnet3_submatrix_info_t *input_info,
		qtk_nnet3_submatrix_info_t *output_info, float alpha)
{
	int i,j, row, col;

	row = output_info->num_rows;
	col = output_info->num_cols;
	float *p1,*p2;
	for (i = 0; i < row; i++)
	{
//		wtk_debug("add mat %d %d %d %d %d %d\n",(i+input_info->row_offset)*col+input_info->col_offset,(i+output_info->row_offset)*output->col+output_info->col_offset,input->col,input->row,output->col,output->row);
//		wtk_debug("%d %f\n",output_info->num_cols,alpha);
/*		cblas_saxpy(output_info->num_cols, alpha,
				input->m + (i + input_info->row_offset) * input->col
						+ input_info->col_offset, 1,
				output->m + (i + output_info->row_offset) * output->col
						+ (utput_info->col_offset, 1);             //maybe error*/
		p1=input->m + (i + input_info->row_offset) * input->col + input_info->col_offset;
		p2=output->m + (i + output_info->row_offset) * output->col + output_info->col_offset;
		for(j = 0; j < col; j++)
		{
//			wtk_debug("%f %f\n",*(p2),*(p1));
			*(p2) = *(p2)*alpha + *(p1);
			p1++;
			p2++;
		}
	}
}
//a =input m=output  fixaffine and NaturalGradientAffine
void qtk_affine_global_propagate(nnet3_component_t* com1, qtk_blas_matrix_t *input,
		qtk_blas_matrix_t *out_put, qtk_nnet3_submatrix_info_t *input_info,
		qtk_nnet3_submatrix_info_t *out_put_info)
{
	qtk_affine_global_component_t* com;
	int col, row, i, j, row_offset, col_offset;
	qtk_blas_matrix_t *m;
	qtk_blas_matrix_t *b;
	com = (qtk_affine_global_component_t*) com1->component;

	m = com->w;
	b = com->b;
	col = out_put_info->num_cols;
	row = out_put_info->num_rows;
	col_offset = out_put_info->col_offset;
	row_offset = out_put_info->row_offset;
	//print_float(input->m,20);
	//print_float(m->m,20);
//double t=0.0;
//t=time_get_ms();

	float *p1, *p2, *p3, *pe2, *pe1;
	int col2, col3;
	col2 = m->col;
	col3 = (col2 >> 2) << 2;
	//wtk_debug("input->row=%d input->col=%d output->row=%d output->col=%d row=%d col=%d col2=%d col3=%d\n", input->row, input->col, out_put->row, out_put->col, row, col, col2, col3);
	//wtk_debug("row=%d,col=%d,row_input=%d,col_input=%d,out_col=%d,out_row=%d,row_win=%d,col_win=%d\n",row,col,input->row,input->col,out_put->col,out_put->row,m->row,m->col);
	for (i = 0; i < row; i++)
	{
		p3 = out_put->m + (i + row_offset) * out_put->col + col_offset;
		for (j = 0; j < col; j++)
		{
			p1 = input->m + (input_info->row_offset + i) * input->col
					+ input_info->col_offset;
			p2 = m->m + j * col2;
			pe2 = p2 + col3;
			pe1 = p2 + col2;
			while (p2 < pe2)
			{
				*p3 += (*p1) * (*p2) + (*(p1 + 1)) * (*(p2 + 1))
						+ (*(p1 + 2)) * (*(p2 + 2)) + (*(p1 + 3)) * (*(p2 + 3));
				p1 += 4;
				p2 += 4;
			}
			while(p2 < pe1)
			{
			  *p3 += *(p1)*(*p2);
			  p1++;
			  p2++;
			}
			//*p3+=t;
			p3++;
		}
	}

	//wtk_debug("%d %d %d %d %d %d\n",input->row,input->col,m->row,m->col,out_put->row,out_put->col);
	//wtk_debug("row=%d,col=%d,row2=%d,col2=%di,offset=%d,%d\n",m->row,m->col,out_put->row,out_put->col,row_offset,col_offset);
//t = time_get_ms()-t;
//wtk_debug("ffffffffffff  t=%f\n",t);
	//print_float(out_put->m,20);
	//qtk_blas_matrix_print(b);
	//exit(0);
	//wtk_debug("com->type=%d\n", com->type);
	if(com->type!=QTK_LinearComponent)
	{
		for (i = 0; i < row; i++)
		{
			for (j = 0; j < col; j++)
			{
				*(out_put->m + (i + row_offset) * out_put->col + j + col_offset) +=
					*(b->m + j);
			}
		}
	}
}

#ifdef USE_AFFINE_FAST
#ifdef USE_NEON
#include <arm_neon.h>
/* this routine computes a 4x4 block of matrix A

       C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 ).
       C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 ).
       C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 ).
       C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ).

 Notice that this routine is called with c = C( i, j ) in the
 previous routine, so these are actually the elements

       C( i  , j ), C( i  , j+1 ), C( i  , j+2 ), C( i  , j+3 )
       C( i+1, j ), C( i+1, j+1 ), C( i+1, j+2 ), C( i+1, j+3 )
       C( i+2, j ), C( i+2, j+1 ), C( i+2, j+2 ), C( i+2, j+3 )
       C( i+3, j ), C( i+3, j+1 ), C( i+3, j+2 ), C( i+3, j+3 )

 in the original matrix C */
#define A(i, j) A[(i)*lda + (j)]
#define B(i, j) B[(i)*ldb + (j)]
#define C(i, j) C[(i)*ldc + (j)]
#define BLOCK 4
void GemmNeonAxpy4x4(int K, float *A, int lda, float *B, int ldb, float *C,
                     int ldc) {
    // loop parameters
    int i;
    // k_4 and n_1 is the multiple of four and one
    int k_4, k_1;
    // Neon values
    float32x4_t
        /* C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 )
           C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 )
           C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 )
           C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ) */
        c_00_neon,
        c_01_neon, c_02_neon, c_03_neon, c_10_neon, c_11_neon, c_12_neon,
        c_13_neon, c_20_neon, c_21_neon, c_22_neon, c_23_neon, c_30_neon,
        c_31_neon, c_32_neon, c_33_neon,
        /* A( 0, i ), A( 0, i + 1 ), A( 0, i + 2 ), A( 0, i + 3 ) */
        a_0i_neon,
        /* A( 1, i ), A( 1, i + 1 ), A( 1, i + 2 ), A( 1, i + 3 ) */
        a_1i_neon,
        /* A( 2, i ), A( 2, i + 1 ), A( 2, i + 2 ), A( 2, i + 3 ) */
        a_2i_neon,
        /* A( 3, i ), A( 3, i + 1 ), A( 3, i + 2 ), A( 3, i + 3 ) */
        a_3i_neon,
        /* B( 0, i ), B( 0, i + 1 ), B( 0, i + 2 ), B( 0, i + 3 ) */
        b_0i_neon,
        /* B( 1, i ), B( 1, i + 1 ), B( 1, i + 2 ), B( 1, i + 3 ) */
        b_1i_neon,
        /* B( 2, i ), B( 2, i + 1 ), B( 2, i + 2 ), B( 2, i + 3 ) */
        b_2i_neon,
        /* B( 3, i ), B( 3, i + 1 ), B( 3, i + 2 ), B( 3, i + 3 ) */
        b_3i_neon;

    k_4 = K / BLOCK;
    k_1 = K - 4 * k_4;

    // if out neon register not load zero, the result is error
    c_00_neon = vmovq_n_f32(0);
    c_01_neon = vmovq_n_f32(0);
    c_02_neon = vmovq_n_f32(0);
    c_03_neon = vmovq_n_f32(0);
    c_10_neon = vmovq_n_f32(0);
    c_11_neon = vmovq_n_f32(0);
    c_12_neon = vmovq_n_f32(0);
    c_13_neon = vmovq_n_f32(0);
    c_20_neon = vmovq_n_f32(0);
    c_21_neon = vmovq_n_f32(0);
    c_22_neon = vmovq_n_f32(0);
    c_23_neon = vmovq_n_f32(0);
    c_30_neon = vmovq_n_f32(0);
    c_31_neon = vmovq_n_f32(0);
    c_32_neon = vmovq_n_f32(0);
    c_33_neon = vmovq_n_f32(0);

    if (k_4) {
        /* the loop-for is divided into four because cache line if the size of
           64 bytes(The possible reason needs to be confirmed later) */
        for (i = 0; i < k_4; i++) {
            // first row
            a_0i_neon = vld1q_f32(&A(0, i * BLOCK));

            b_0i_neon = vld1q_f32(&B(0, i * BLOCK));
            b_1i_neon = vld1q_f32(&B(1, i * BLOCK));
            b_2i_neon = vld1q_f32(&B(2, i * BLOCK));
            b_3i_neon = vld1q_f32(&B(3, i * BLOCK));

            c_00_neon = vfmaq_f32(c_00_neon, a_0i_neon, b_0i_neon);
            c_01_neon = vfmaq_f32(c_01_neon, a_0i_neon, b_1i_neon);
            c_02_neon = vfmaq_f32(c_02_neon, a_0i_neon, b_2i_neon);
            c_03_neon = vfmaq_f32(c_03_neon, a_0i_neon, b_3i_neon);
        }

        for (i = 0; i < k_4; i++) {
            // second row
            a_1i_neon = vld1q_f32(&A(1, i * BLOCK));

            b_0i_neon = vld1q_f32(&B(0, i * BLOCK));
            b_1i_neon = vld1q_f32(&B(1, i * BLOCK));
            b_2i_neon = vld1q_f32(&B(2, i * BLOCK));
            b_3i_neon = vld1q_f32(&B(3, i * BLOCK));

            c_10_neon = vfmaq_f32(c_10_neon, a_1i_neon, b_0i_neon);
            c_11_neon = vfmaq_f32(c_11_neon, a_1i_neon, b_1i_neon);
            c_12_neon = vfmaq_f32(c_12_neon, a_1i_neon, b_2i_neon);
            c_13_neon = vfmaq_f32(c_13_neon, a_1i_neon, b_3i_neon);
        }

        for (i = 0; i < k_4; i++) {
            // third row
            a_2i_neon = vld1q_f32(&A(2, i * BLOCK));

            b_0i_neon = vld1q_f32(&B(0, i * BLOCK));
            b_1i_neon = vld1q_f32(&B(1, i * BLOCK));
            b_2i_neon = vld1q_f32(&B(2, i * BLOCK));
            b_3i_neon = vld1q_f32(&B(3, i * BLOCK));

            c_20_neon = vfmaq_f32(c_20_neon, a_2i_neon, b_0i_neon);
            c_21_neon = vfmaq_f32(c_21_neon, a_2i_neon, b_1i_neon);
            c_22_neon = vfmaq_f32(c_22_neon, a_2i_neon, b_2i_neon);
            c_23_neon = vfmaq_f32(c_23_neon, a_2i_neon, b_3i_neon);
        }

        for (i = 0; i < k_4; i++) {
            // four row
            a_3i_neon = vld1q_f32(&A(3, i * BLOCK));

            b_0i_neon = vld1q_f32(&B(0, i * BLOCK));
            b_1i_neon = vld1q_f32(&B(1, i * BLOCK));
            b_2i_neon = vld1q_f32(&B(2, i * BLOCK));
            b_3i_neon = vld1q_f32(&B(3, i * BLOCK));

            c_30_neon = vfmaq_f32(c_30_neon, a_3i_neon, b_0i_neon);
            c_31_neon = vfmaq_f32(c_31_neon, a_3i_neon, b_1i_neon);
            c_32_neon = vfmaq_f32(c_32_neon, a_3i_neon, b_2i_neon);
            c_33_neon = vfmaq_f32(c_33_neon, a_3i_neon, b_3i_neon);
        }

        C(0, 0) += vgetq_lane_f32(c_00_neon, 0) + vgetq_lane_f32(c_00_neon, 1) +
                   vgetq_lane_f32(c_00_neon, 2) + vgetq_lane_f32(c_00_neon, 3);
        C(0, 1) += vgetq_lane_f32(c_01_neon, 0) + vgetq_lane_f32(c_01_neon, 1) +
                   vgetq_lane_f32(c_01_neon, 2) + vgetq_lane_f32(c_01_neon, 3);
        C(0, 2) += vgetq_lane_f32(c_02_neon, 0) + vgetq_lane_f32(c_02_neon, 1) +
                   vgetq_lane_f32(c_02_neon, 2) + vgetq_lane_f32(c_02_neon, 3);
        C(0, 3) += vgetq_lane_f32(c_03_neon, 0) + vgetq_lane_f32(c_03_neon, 1) +
                   vgetq_lane_f32(c_03_neon, 2) + vgetq_lane_f32(c_03_neon, 3);

        C(1, 0) += vgetq_lane_f32(c_10_neon, 0) + vgetq_lane_f32(c_10_neon, 1) +
                   vgetq_lane_f32(c_10_neon, 2) + vgetq_lane_f32(c_10_neon, 3);
        C(1, 1) += vgetq_lane_f32(c_11_neon, 0) + vgetq_lane_f32(c_11_neon, 1) +
                   vgetq_lane_f32(c_11_neon, 2) + vgetq_lane_f32(c_11_neon, 3);
        C(1, 2) += vgetq_lane_f32(c_12_neon, 0) + vgetq_lane_f32(c_12_neon, 1) +
                   vgetq_lane_f32(c_12_neon, 2) + vgetq_lane_f32(c_12_neon, 3);
        C(1, 3) += vgetq_lane_f32(c_13_neon, 0) + vgetq_lane_f32(c_13_neon, 1) +
                   vgetq_lane_f32(c_13_neon, 2) + vgetq_lane_f32(c_13_neon, 3);

        C(2, 0) += vgetq_lane_f32(c_20_neon, 0) + vgetq_lane_f32(c_20_neon, 1) +
                   vgetq_lane_f32(c_20_neon, 2) + vgetq_lane_f32(c_20_neon, 3);
        C(2, 1) += vgetq_lane_f32(c_21_neon, 0) + vgetq_lane_f32(c_21_neon, 1) +
                   vgetq_lane_f32(c_21_neon, 2) + vgetq_lane_f32(c_21_neon, 3);
        C(2, 2) += vgetq_lane_f32(c_22_neon, 0) + vgetq_lane_f32(c_22_neon, 1) +
                   vgetq_lane_f32(c_22_neon, 2) + vgetq_lane_f32(c_22_neon, 3);
        C(2, 3) += vgetq_lane_f32(c_23_neon, 0) + vgetq_lane_f32(c_23_neon, 1) +
                   vgetq_lane_f32(c_23_neon, 2) + vgetq_lane_f32(c_23_neon, 3);

        C(3, 0) += vgetq_lane_f32(c_30_neon, 0) + vgetq_lane_f32(c_30_neon, 1) +
                   vgetq_lane_f32(c_30_neon, 2) + vgetq_lane_f32(c_30_neon, 3);
        C(3, 1) += vgetq_lane_f32(c_31_neon, 0) + vgetq_lane_f32(c_31_neon, 1) +
                   vgetq_lane_f32(c_31_neon, 2) + vgetq_lane_f32(c_31_neon, 3);
        C(3, 2) += vgetq_lane_f32(c_32_neon, 0) + vgetq_lane_f32(c_32_neon, 1) +
                   vgetq_lane_f32(c_32_neon, 2) + vgetq_lane_f32(c_32_neon, 3);
        C(3, 3) += vgetq_lane_f32(c_33_neon, 0) + vgetq_lane_f32(c_33_neon, 1) +
                   vgetq_lane_f32(c_33_neon, 2) + vgetq_lane_f32(c_33_neon, 3);
    }

    /* when k is not a multiple of four, k_1 that is the remainder of four need
       to be calculate */
    if (k_1) {
        // the offset of k_4
        int offset;
        float
            /* C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 )
            C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 )
            C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 )
            C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ) */
            c_00,
            c_01, c_02, c_03, c_10, c_11, c_12, c_13, c_20, c_21, c_22, c_23,
            c_30, c_31, c_32, c_33,
            /* Point to the current elements in the four rows of A */
            *a_0i_pntr, *a_1i_pntr, *a_2i_pntr, *a_3i_pntr,
            /* Point to the current elements in the four rows of B */
            *b_0i_pntr, *b_1i_pntr, *b_2i_pntr, *b_3i_pntr;

        offset = k_4 * 4;

        c_00 = 0.0;
        c_01 = 0.0;
        c_02 = 0.0;
        c_03 = 0.0;
        c_10 = 0.0;
        c_11 = 0.0;
        c_12 = 0.0;
        c_13 = 0.0;
        c_20 = 0.0;
        c_21 = 0.0;
        c_22 = 0.0;
        c_23 = 0.0;
        c_30 = 0.0;
        c_31 = 0.0;
        c_32 = 0.0;
        c_33 = 0.0;

        a_0i_pntr = &A(0, 0) + offset;
        a_1i_pntr = &A(1, 0) + offset;
        a_2i_pntr = &A(2, 0) + offset;
        a_3i_pntr = &A(3, 0) + offset;

        b_0i_pntr = &B(0, 0) + offset;
        b_1i_pntr = &B(1, 0) + offset;
        b_2i_pntr = &B(2, 0) + offset;
        b_3i_pntr = &B(3, 0) + offset;

        for (i = 0; i < k_1; i++) {
            // first row
            c_00 += *a_0i_pntr * *b_0i_pntr;
            c_01 += *a_0i_pntr * *b_1i_pntr;
            c_02 += *a_0i_pntr * *b_2i_pntr;
            c_03 += *a_0i_pntr * *b_3i_pntr;

            // second row
            c_10 += *a_1i_pntr * *b_0i_pntr;
            c_11 += *a_1i_pntr * *b_1i_pntr;
            c_12 += *a_1i_pntr * *b_2i_pntr;
            c_13 += *a_1i_pntr * *b_3i_pntr;

            // third row
            c_20 += *a_2i_pntr * *b_0i_pntr;
            c_21 += *a_2i_pntr * *b_1i_pntr;
            c_22 += *a_2i_pntr * *b_2i_pntr;
            c_23 += *a_2i_pntr * *b_3i_pntr;

            // four row
            c_30 += *a_3i_pntr * *b_0i_pntr;
            c_31 += *a_3i_pntr * *b_1i_pntr;
            c_32 += *a_3i_pntr * *b_2i_pntr;
            c_33 += *a_3i_pntr * *b_3i_pntr;

            a_0i_pntr++;
            a_1i_pntr++;
            a_2i_pntr++;
            a_3i_pntr++;
            b_0i_pntr++;
            b_1i_pntr++;
            b_2i_pntr++;
            b_3i_pntr++;
        }

        C(0, 0) += c_00;
        C(0, 1) += c_01;
        C(0, 2) += c_02;
        C(0, 3) += c_03;

        C(1, 0) += c_10;
        C(1, 1) += c_11;
        C(1, 2) += c_12;
        C(1, 3) += c_13;

        C(2, 0) += c_20;
        C(2, 1) += c_21;
        C(2, 2) += c_22;
        C(2, 3) += c_23;

        C(3, 0) += c_30;
        C(3, 1) += c_31;
        C(3, 2) += c_32;
        C(3, 3) += c_33;
    }
}

/* this routine computes a 4x4 block of matrix A

       C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 ).
       C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 ).
       C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 ).
       C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ).

 Notice that this routine is called with c = C( i, j ) in the
 previous routine, so these are actually the elements

       C( i  , j ), C( i  , j+1 ), C( i  , j+2 ), C( i  , j+3 )
       C( i+1, j ), C( i+1, j+1 ), C( i+1, j+2 ), C( i+1, j+3 )
       C( i+2, j ), C( i+2, j+1 ), C( i+2, j+2 ), C( i+2, j+3 )
       C( i+3, j ), C( i+3, j+1 ), C( i+3, j+2 ), C( i+3, j+3 )

 in the original matrix C */
void NeonAxpy4x4(int k, float *a, int lda, float *b, int ldb, float *c,
                 int ldc) {
    // loop parameters
    int i;
    // k_4 and n_1 is the multiple of four and one
    int k_4, k_1;
    // Neon values
    float32x4_t
        /* C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 )
           C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 )
           C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 )
           C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ) */
        c_00_neon,
        c_01_neon, c_02_neon, c_03_neon, c_10_neon, c_11_neon, c_12_neon,
        c_13_neon, c_20_neon, c_21_neon, c_22_neon, c_23_neon, c_30_neon,
        c_31_neon, c_32_neon, c_33_neon,
        /* A( 0, i ), A( 0, i + 1 ), A( 0, i + 2 ), A( 0, i + 3 ) */
        *a_0i_neon,
        /* A( 1, i ), A( 1, i + 1 ), A( 1, i + 2 ), A( 1, i + 3 ) */
        *a_1i_neon,
        /* A( 2, i ), A( 2, i + 1 ), A( 2, i + 2 ), A( 2, i + 3 ) */
        *a_2i_neon,
        /* A( 3, i ), A( 3, i + 1 ), A( 3, i + 2 ), A( 3, i + 3 ) */
        *a_3i_neon,
        /* B( 0, i ), B( 0, i + 1 ), B( 0, i + 2 ), B( 0, i + 3 ) */
        *b_0i_neon,
        /* B( 1, i ), B( 1, i + 1 ), B( 1, i + 2 ), B( 1, i + 3 ) */
        *b_1i_neon,
        /* B( 2, i ), B( 2, i + 1 ), B( 2, i + 2 ), B( 2, i + 3 ) */
        *b_2i_neon,
        /* B( 3, i ), B( 3, i + 1 ), B( 3, i + 2 ), B( 3, i + 3 ) */
        *b_3i_neon;

    k_4 = k >> 2;
    k_1 = k - 4 * k_4;

    // if out neon register not load zero, the result is error
    c_00_neon = vmovq_n_f32(0);
    c_01_neon = vmovq_n_f32(0);
    c_02_neon = vmovq_n_f32(0);
    c_03_neon = vmovq_n_f32(0);
    c_10_neon = vmovq_n_f32(0);
    c_11_neon = vmovq_n_f32(0);
    c_12_neon = vmovq_n_f32(0);
    c_13_neon = vmovq_n_f32(0);
    c_20_neon = vmovq_n_f32(0);
    c_21_neon = vmovq_n_f32(0);
    c_22_neon = vmovq_n_f32(0);
    c_23_neon = vmovq_n_f32(0);
    c_30_neon = vmovq_n_f32(0);
    c_31_neon = vmovq_n_f32(0);
    c_32_neon = vmovq_n_f32(0);
    c_33_neon = vmovq_n_f32(0);

    // Forced type conversion instead of loading to neon register to speed up
    a_0i_neon = (float32x4_t *)a;
    a_1i_neon = (float32x4_t *)(a + lda);
    a_2i_neon = (float32x4_t *)(a + 2 * lda);
    a_3i_neon = (float32x4_t *)(a + 3 * lda);

    b_0i_neon = (float32x4_t *)b;
    b_1i_neon = (float32x4_t *)(b + ldb);
    b_2i_neon = (float32x4_t *)(b + 2 * ldb);
    b_3i_neon = (float32x4_t *)(b + 3 * ldb);

    if (k_4) {
        /* the loop-for is divided into four because cache line if the size of
           64 bytes(The possible reason needs to be confirmed later) */
        for (i = 0; i < k_4; i++) {
            // first row
            c_00_neon = vfmaq_f32(c_00_neon, a_0i_neon[i], b_0i_neon[i]);
            c_01_neon = vfmaq_f32(c_01_neon, a_0i_neon[i], b_1i_neon[i]);
            c_02_neon = vfmaq_f32(c_02_neon, a_0i_neon[i], b_2i_neon[i]);
            c_03_neon = vfmaq_f32(c_03_neon, a_0i_neon[i], b_3i_neon[i]);
        }

        for (i = 0; i < k_4; i++) {
            // second row
            c_10_neon = vfmaq_f32(c_10_neon, a_1i_neon[i], b_0i_neon[i]);
            c_11_neon = vfmaq_f32(c_11_neon, a_1i_neon[i], b_1i_neon[i]);
            c_12_neon = vfmaq_f32(c_12_neon, a_1i_neon[i], b_2i_neon[i]);
            c_13_neon = vfmaq_f32(c_13_neon, a_1i_neon[i], b_3i_neon[i]);
        }

        for (i = 0; i < k_4; i++) {
            // third row
            c_20_neon = vfmaq_f32(c_20_neon, a_2i_neon[i], b_0i_neon[i]);
            c_21_neon = vfmaq_f32(c_21_neon, a_2i_neon[i], b_1i_neon[i]);
            c_22_neon = vfmaq_f32(c_22_neon, a_2i_neon[i], b_2i_neon[i]);
            c_23_neon = vfmaq_f32(c_23_neon, a_2i_neon[i], b_3i_neon[i]);
        }

        for (i = 0; i < k_4; i++) {
            // four row
            c_30_neon = vfmaq_f32(c_30_neon, a_3i_neon[i], b_0i_neon[i]);
            c_31_neon = vfmaq_f32(c_31_neon, a_3i_neon[i], b_1i_neon[i]);
            c_32_neon = vfmaq_f32(c_32_neon, a_3i_neon[i], b_2i_neon[i]);
            c_33_neon = vfmaq_f32(c_33_neon, a_3i_neon[i], b_3i_neon[i]);
        }

        /* Forced type conversion for the pointer instead of neon API
           (eg:vgetq_lane_f32) */
        float *neon_values = NULL;

        neon_values = (float *)(&c_00_neon);
        *c = neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_01_neon);
        *(c + 1) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_02_neon);
        *(c + 2) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_03_neon);
        *(c + 3) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];

        neon_values = (float *)(&c_10_neon);
        *(c + ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_11_neon);
        *(c + 1 + ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_12_neon);
        *(c + 2 + ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_13_neon);
        *(c + 3 + ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];

        neon_values = (float *)(&c_20_neon);
        *(c + 2 * ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_21_neon);
        *(c + 1 + 2 * ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_22_neon);
        *(c + 2 + 2 * ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_23_neon);
        *(c + 3 + 2 * ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];

        neon_values = (float *)(&c_30_neon);
        *(c + 3 * ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_31_neon);
        *(c + 1 + 3 * ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_32_neon);
        *(c + 2 + 3 * ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
        neon_values = (float *)(&c_33_neon);
        *(c + 3 + 3 * ldc) =
            neon_values[0] + neon_values[1] + neon_values[2] + neon_values[3];
    }

    /* when k is not a multiple of four, k_1 that is the remainder of four need
       to be calculate */
    if (k_1) {
        // the offset of k_4
        int offset;
        float
            /* C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 )
            C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 )
            C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 )
            C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ) */
            c_00,
            c_01, c_02, c_03, c_10, c_11, c_12, c_13, c_20, c_21, c_22, c_23,
            c_30, c_31, c_32, c_33,
            /* Point to the current elements in the four rows of A */
            *a_0i_pntr, *a_1i_pntr, *a_2i_pntr, *a_3i_pntr,
            /* Point to the current elements in the four rows of B */
            *b_0i_pntr, *b_1i_pntr, *b_2i_pntr, *b_3i_pntr;

        offset = k_4 * 4;

        a_0i_pntr = a + offset;
        a_1i_pntr = a + lda + offset;
        a_2i_pntr = a + 2 * lda + offset;
        a_3i_pntr = a + 3 * lda + offset;

        b_0i_pntr = b + offset;
        b_1i_pntr = b + ldb + offset;
        b_2i_pntr = b + 2 * ldb + offset;
        b_3i_pntr = b + 3 * ldb + offset;

        for (i = 0; i < k_1; i++) {
            // first row
            c_00 += *a_0i_pntr * *b_0i_pntr;
            c_01 += *a_0i_pntr * *b_1i_pntr;
            c_02 += *a_0i_pntr * *b_2i_pntr;
            c_03 += *a_0i_pntr * *b_3i_pntr;

            // second row
            c_10 += *a_1i_pntr * *b_0i_pntr;
            c_11 += *a_1i_pntr * *b_1i_pntr;
            c_12 += *a_1i_pntr * *b_2i_pntr;
            c_13 += *a_1i_pntr * *b_3i_pntr;

            // third row
            c_20 += *a_2i_pntr * *b_0i_pntr;
            c_21 += *a_2i_pntr * *b_1i_pntr;
            c_22 += *a_2i_pntr * *b_2i_pntr;
            c_23 += *a_2i_pntr * *b_3i_pntr;

            // four row
            c_30 += *a_3i_pntr * *b_0i_pntr;
            c_31 += *a_3i_pntr * *b_1i_pntr;
            c_32 += *a_3i_pntr * *b_2i_pntr;
            c_33 += *a_3i_pntr * *b_3i_pntr;

            a_0i_pntr++;
            a_1i_pntr++;
            a_2i_pntr++;
            a_3i_pntr++;
            b_0i_pntr++;
            b_1i_pntr++;
            b_2i_pntr++;
            b_3i_pntr++;
        }

        *c += c_00;
        *(c + 1) += c_01;
        *(c + 2) += c_02;
        *(c + 3) += c_03;

        *(c + ldc) += c_10;
        *(c + 1 + ldc) += c_11;
        *(c + 2 + ldc) += c_12;
        *(c + 3 + ldc) += c_13;

        *(c + 2 * ldc) += c_20;
        *(c + 1 + 2 * ldc) += c_21;
        *(c + 2 + 2 * ldc) += c_22;
        *(c + 3 + 2 * ldc) += c_23;

        *(c + 3 * ldc) += c_30;
        *(c + 1 + 3 * ldc) += c_31;
        *(c + 2 + 3 * ldc) += c_32;
        *(c + 3 + 3 * ldc) += c_33;
    }
}
#endif

/* So, this routine computes a 4x4 block of matrix A

       C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 ).
       C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 ).
       C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 ).
       C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ).

 Notice that this routine is called with c = C( i, j ) in the
 previous routine, so these are actually the elements

       C( i  , j ), C( i  , j+1 ), C( i  , j+2 ), C( i  , j+3 )
       C( i+1, j ), C( i+1, j+1 ), C( i+1, j+2 ), C( i+1, j+3 )
       C( i+2, j ), C( i+2, j+1 ), C( i+2, j+2 ), C( i+2, j+3 )
       C( i+3, j ), C( i+3, j+1 ), C( i+3, j+2 ), C( i+3, j+3 )

 in the original matrix C */
void Axpy4x4(int k, float *a, int lda, float *b, int ldb, float *c, int ldc) {
    // loop parameters
    int i;
    // Point to the address at the beginning of the rows
    float
        /* Point to the current elements in the four rows of A */
        *a_0i_pntr,
        *a_1i_pntr, *a_2i_pntr, *a_3i_pntr,
        /* Point to the current elements in the four rows of B */
        *b_0i_pntr, *b_1i_pntr, *b_2i_pntr, *b_3i_pntr;
    // register values
    register float
        /* C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 )
           C( 1, 0 ), C( 1, 1 ), C( 1, 2 ), C( 1, 3 )
           C( 2, 0 ), C( 2, 1 ), C( 2, 2 ), C( 2, 3 )
           C( 3, 0 ), C( 3, 1 ), C( 3, 2 ), C( 3, 3 ) */
        c_00_reg,
        c_01_reg, c_02_reg, c_03_reg, c_10_reg, c_11_reg, c_12_reg, c_13_reg,
        c_20_reg, c_21_reg, c_22_reg, c_23_reg, c_30_reg, c_31_reg, c_32_reg,
        c_33_reg,
        /* A( 0, i ), A( 1, i ), A( 2, i ), A( 3, i ) */
        a_0i_reg, a_1i_reg, a_2i_reg, a_3i_reg,
        /* B( 0, i ), B( 1, i ), B( 2, i ), B( 3, i ) */
        b_0i_reg, b_1i_reg, b_2i_reg, b_3i_reg;

    c_00_reg = 0.0;
    c_01_reg = 0.0;
    c_02_reg = 0.0;
    c_03_reg = 0.0;
    c_10_reg = 0.0;
    c_11_reg = 0.0;
    c_12_reg = 0.0;
    c_13_reg = 0.0;
    c_20_reg = 0.0;
    c_21_reg = 0.0;
    c_22_reg = 0.0;
    c_23_reg = 0.0;
    c_30_reg = 0.0;
    c_31_reg = 0.0;
    c_32_reg = 0.0;
    c_33_reg = 0.0;

    a_0i_pntr = a;
    a_1i_pntr = a + lda;
    a_2i_pntr = a + 2 * lda;
    a_3i_pntr = a + 3 * lda;

    b_0i_pntr = b;
    b_1i_pntr = b + ldb;
    b_2i_pntr = b + 2 * ldb;
    b_3i_pntr = b + 3 * ldb;

    for (i = 0; i < k; i++) {
        a_0i_reg = *a_0i_pntr++;
        a_1i_reg = *a_1i_pntr++;
        a_2i_reg = *a_2i_pntr++;
        a_3i_reg = *a_3i_pntr++;

        b_0i_reg = *b_0i_pntr++;
        b_1i_reg = *b_1i_pntr++;
        b_2i_reg = *b_2i_pntr++;
        b_3i_reg = *b_3i_pntr++;

        // first row
        c_00_reg += a_0i_reg * b_0i_reg;
        c_01_reg += a_0i_reg * b_1i_reg;
        c_02_reg += a_0i_reg * b_2i_reg;
        c_03_reg += a_0i_reg * b_3i_reg;

        // second row
        c_10_reg += a_1i_reg * b_0i_reg;
        c_11_reg += a_1i_reg * b_1i_reg;
        c_12_reg += a_1i_reg * b_2i_reg;
        c_13_reg += a_1i_reg * b_3i_reg;

        // third row
        c_20_reg += a_2i_reg * b_0i_reg;
        c_21_reg += a_2i_reg * b_1i_reg;
        c_22_reg += a_2i_reg * b_2i_reg;
        c_23_reg += a_2i_reg * b_3i_reg;

        // four row
        c_30_reg += a_3i_reg * b_0i_reg;
        c_31_reg += a_3i_reg * b_1i_reg;
        c_32_reg += a_3i_reg * b_2i_reg;
        c_33_reg += a_3i_reg * b_3i_reg;
    }
    *c += c_00_reg;
    *(c + 1) += c_01_reg;
    *(c + 2) += c_02_reg;
    *(c + 3) += c_03_reg;

    *(c + ldc) += c_10_reg;
    *(c + 1 + ldc) += c_11_reg;
    *(c + 2 + ldc) += c_12_reg;
    *(c + 3 + ldc) += c_13_reg;

    *(c + 2 * ldc) += c_20_reg;
    *(c + 1 + 2 * ldc) += c_21_reg;
    *(c + 2 + 2 * ldc) += c_22_reg;
    *(c + 3 + 2 * ldc) += c_23_reg;

    *(c + 3 * ldc) += c_30_reg;
    *(c + 1 + 3 * ldc) += c_31_reg;
    *(c + 2 + 3 * ldc) += c_32_reg;
    *(c + 3 + 3 * ldc) += c_33_reg;
}

/* So, this routine computes a 4x4 block of matrix A

       C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 ).

 Notice that this routine is called with c = C( i, j ) in the
 previous routine, so these are actually the elements

       C( i  , j ), C( i  , j+1 ), C( i  , j+2 ), C( i  , j+3 )

 in the original matrix C */
void Axpy1x4(int k, float *a, int lda, float *b, int ldb, float *c, int ldc) {
    // loop parameters
    int i;
    // Point to the address at the beginning of the rows
    float
        /* Point to the current elements in the one rows of A */
        *a_0i_pntr,
        /* Point to the current elements in the four rows of B */
        *b_0i_pntr, *b_1i_pntr, *b_2i_pntr, *b_3i_pntr;
    // register values
    register float
        /* C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 ) */
        c_00_reg,
        c_01_reg, c_02_reg, c_03_reg,
        /* A( 0, p ) */
        a_0i_reg,
        /* B( 0, p ), B( 1, p ), B( 2, p ), B( 3, p ) */
        b_0i_reg, b_1i_reg, b_2i_reg, b_3i_reg;

    c_00_reg = 0.0;
    c_01_reg = 0.0;
    c_02_reg = 0.0;
    c_03_reg = 0.0;

    a_0i_pntr = a;

    b_0i_pntr = b;
    b_1i_pntr = b + ldb;
    b_2i_pntr = b + 2 * ldb;
    b_3i_pntr = b + 3 * ldb;

    for (i = 0; i < k; i++) {
        a_0i_reg = *a_0i_pntr++;

        b_0i_reg = *b_0i_pntr++;
        b_1i_reg = *b_1i_pntr++;
        b_2i_reg = *b_2i_pntr++;
        b_3i_reg = *b_3i_pntr++;

        c_00_reg += a_0i_reg * b_0i_reg;
        c_01_reg += a_0i_reg * b_1i_reg;
        c_02_reg += a_0i_reg * b_2i_reg;
        c_03_reg += a_0i_reg * b_3i_reg;
    }

    *c += c_00_reg;
    *(c + 1) += c_01_reg;
    *(c + 2) += c_02_reg;
    *(c + 3) += c_03_reg;
}

/* So, this routine computes a 4x4 block of matrix A

       C( 0, 0 ).
       C( 1, 0 ).
       C( 2, 0 ).
       C( 3, 0 ).

 Notice that this routine is called with c = C( i, j ) in the
 previous routine, so these are actually the elements

       C( i  , j )
       C( i+1, j )
       C( i+2, j )
       C( i+3, j )

 in the original matrix C */
void Axpy4x1(int k, float *a, int lda, float *b, int ldb, float *c, int ldc) {
    // loop parameters
    int i;
    // Point to the address at the beginning of the rows
    float
        /* Point to the current elements in the four rows of A */
        *a_0i_pntr,
        *a_1i_pntr, *a_2i_pntr, *a_3i_pntr,
        /* Point to the current elements in the one rows of B */
        *b_0i_pntr;
    // register values
    register float
        /* C( 0, 0 )
           C( 1, 0 )
           C( 2, 0 )
           C( 3, 0 ) */
        c_00_reg,
        c_10_reg, c_20_reg, c_30_reg,
        /* A( 0, p ), A( 1, p ), A( 2, p ), A( 3, p ) */
        a_0i_reg, a_1i_reg, a_2i_reg, a_3i_reg,
        /* B( 0, p ) */
        b_0i_reg;

    c_00_reg = 0.0;
    c_10_reg = 0.0;
    c_20_reg = 0.0;
    c_30_reg = 0.0;

    a_0i_pntr = a;
    a_1i_pntr = a + lda;
    a_2i_pntr = a + 2 * lda;
    a_3i_pntr = a + 3 * lda;

    b_0i_pntr = b;

    for (i = 0; i < k; i++) {
        a_0i_reg = *a_0i_pntr++;
        a_1i_reg = *a_1i_pntr++;
        a_2i_reg = *a_2i_pntr++;
        a_3i_reg = *a_3i_pntr++;

        b_0i_reg = *b_0i_pntr++;

        c_00_reg += a_0i_reg * b_0i_reg;
        c_10_reg += a_1i_reg * b_0i_reg;
        c_20_reg += a_2i_reg * b_0i_reg;
        c_30_reg += a_3i_reg * b_0i_reg;
    }

    *c += c_00_reg;
    *(c + ldc) += c_10_reg;
    *(c + 2 * ldc) += c_20_reg;
    *(c + 3 * ldc) += c_30_reg;
}

/* So, this routine computes a 4x4 block of matrix A

       C( 0, 0 ).

 Notice that this routine is called with c = C( i, j ) in the
 previous routine, so these are actually the elements

       C( i , j )

 in the original matrix C */
void Axpy1x1(int k, float *a, int lda, float *b, int ldb, float *c, int ldc) {
    // loop parameters
    int i;
    // Point to the address at the beginning of the rows
    float
        /* Point to the current elements in the one rows of A */
        *a_0i_pntr,
        /* Point to the current elements in the one rows of B */
        *b_0i_pntr;
    // register values
    register float
        /* C( 0, 0 ) */
        c_00_reg,
        /* A( 0, p ) */
        a_0i_reg,
        /* B( 0, p ) */
        b_0i_reg;

    c_00_reg = 0.0;
    a_0i_pntr = a;
    b_0i_pntr = b;

    for (i = 0; i < k; i++) {
        a_0i_reg = *a_0i_pntr++;
        b_0i_reg = *b_0i_pntr++;

        c_00_reg += a_0i_reg * b_0i_reg;
    }

    *c += c_00_reg;
}

/*b matrix transpose is performed by default*/
void matrix_mul(int m, int n, int k, float *a, int lda, float *b, int ldb,
                float *c, int ldc) {
    // loop parameters
    int i, j;
    // m_4 and n_4 is the multiple of four
    int m_4, n_4;
    // m_1 and n_1 is the remainder of one
    int m_1, n_1;
    // A, B and C pointer to the beginning of loop
    float *A, *B, *C;

    m_4 = (m >> 2) << 2;
    n_4 = (n >> 2) << 2;

    m_1 = m - m_4;
    n_1 = n - n_4;
    /*
            c(m, n) = a(m, k) * b(n, k)
                                    ||
            c(m_4, n_4) = a(m_4, n_4) * b(m_4, n_4)
            c(m_1, n_4) = a(m_1, n_4) * b(m_1, n_4)
            c(m_4, n_1) = a(m_4, n_1) * b(m_4, n_1)
            c(m_1, n_1) = a(m_1, n_1) * b(m_1, n_1)
    */

    /* A B C point to the beginning of memory to AddDot4x4 */
    if (m_4 && n_4) {
        for (j = 0; j < n_4;
             j += 4) { // loop over the columns of c, unrolled by 4
            B = (b) + j * ldb;
            for (i = 0; i < m_4;
                 i += 4) { // loop over the rows of c, unrolled by 4
                A = (a) + i * lda;
                C = (c) + i * ldc + j;
#ifdef USE_NEON
                GemmNeonAxpy4x4(k, A, lda, B, ldb, C, ldc);
                // NeonAxpy4x4(k, A, lda, B, ldb, C, ldc);
#else
                Axpy4x4(k, A, lda, B, ldb, C, ldc);
#endif
            }
        }
    }

    if (m_1 && n_4) {
        for (j = 0; j < n_4;
             j += 4) { // loop over the columns of c, unrolled by 4
            B = (b) + j * ldb;
            for (i = 0; i < m_1; i++) { // loop over the rows of c, unrolled by
                                        // 1
                A = (a + m_4 * lda) + i * lda;
                C = (c + m_4 * ldc) + i * ldc + j;
                Axpy1x4(k, A, lda, B, ldb, C, ldc);
            }
        }
    }

    if (m_4 && n_1) {
        for (j = 0; j < n_1; j++) { // loop over the rows of c, unrolled by 1
            B = (b + n_4 * ldb) + j * ldb;
            for (i = 0; i < m_4;
                 i += 4) { // loop over the rows of c, unrolled by 4
                A = (a) + i * lda;
                C = (c + n_4) + i * ldc + j;
                Axpy4x1(k, A, lda, B, ldb, C, ldc);
            }
        }
    }

    if (m_1 && n_1) {
        for (j = 0; j < n_1; j++) { // loop over the rows of c, unrolled by 1
            B = (b + n_4 * ldb) + j * ldb;
            for (i = 0; i < m_1; i++) { // loop over the rows of c, unrolled by
                                        // 1
                A = (a + m_4 * lda) + i * lda;
                C = (c + m_4 * ldc + n_4) + i * ldc + j;
                Axpy1x1(k, A, lda, B, ldb, C, ldc);
            }
        }
    }
}

void qtk_affine_global_propagate_fast(
    nnet3_component_t *com1, qtk_blas_matrix_t *input,
    qtk_blas_matrix_t *out_put, qtk_nnet3_submatrix_info_t *input_info,
    qtk_nnet3_submatrix_info_t *out_put_info) {
    qtk_affine_global_component_t *com;
    /* loop */
    int i, j;
    /* m is out row
       n is out col
       k is col of in or com->w */
    int m, n, k;
    /* ldi is offset the small matrix of col
       ldo is offset the small matrix of col
       ldw is offset the small matrix of col */
    int ldi, ldo, ldw;
    /* big output matrix info */
    int col, row, row_offset, col_offset;
    /* model parameters matrix y = w * x + b */
    qtk_blas_matrix_t *w, *b;
    /* in_pntr is input small matrix pointer
       w_pntr is parameters matrix pointer
   out_pntr is output small matrix pointer */
    float *in_pntr, *out_pntr, *w_pntr;

    com = (qtk_affine_global_component_t *)com1->component;
    w = com->w;
    b = com->b;

    col = out_put_info->num_cols;
    row = out_put_info->num_rows;
    col_offset = out_put_info->col_offset;
    row_offset = out_put_info->row_offset;

    m = out_put_info->num_rows;
    n = out_put_info->num_cols;
    k = w->col;

    ldi = input->col;
    ldw = w->col;
    ldo = out_put->col;

    in_pntr =
        input->m + input_info->row_offset * input->col + input_info->col_offset;
    w_pntr = w->m;
    out_pntr = out_put->m + out_put_info->row_offset * out_put->col +
               out_put_info->col_offset;

    /*w matrix transpose is performed by default*/
    matrix_mul(m, n, k, in_pntr, ldi, w_pntr, ldw, out_pntr, ldo);

    if (com->type != QTK_LinearComponent) {
        for (i = 0; i < row; i++) {
            for (j = 0; j < col; j++) {
                *(out_put->m + (i + row_offset) * out_put->col + j +
                  col_offset) += *(b->m + j);
            }
        }
    }
}
#endif

#ifdef USE_AVX
#include <immintrin.h>
void qtk_affine_global_propagate_avx(nnet3_component_t* com1, qtk_blas_matrix_t *input,
                qtk_blas_matrix_t *out_put, qtk_nnet3_submatrix_info_t *input_info,
                qtk_nnet3_submatrix_info_t *out_put_info)
{
        qtk_affine_global_component_t* com;
        int col, row, i, j,k,row_offset, col_offset,col2;
        qtk_blas_matrix_t *m;
        qtk_blas_matrix_t *b;
        com = (qtk_affine_global_component_t*) com1->component;

        m = com->w;
        b = com->b;
        col = out_put_info->num_cols;
        row = out_put_info->num_rows;
        col_offset = out_put_info->col_offset;
        row_offset = out_put_info->row_offset;
//wtk_debug("w:row=%d,col=%d,, input:row=%d,col=%d,,output:row=%d,col=%d\n",m->row,m->col,input->row,input->col,row,col);
       // col2 = m->col;
       // col3 = (col2 >> 2) << 2;
        //wtk_debug("%d %d\n",col2,col3);
//wtk_debug("*********************************w**************\n");
//print_float(m->m,m->col*m->row);
//wtk_debug("*****************w*********************************\n");
        float * vc;
        const float * va,*vb;

    {
        __m256 v;
        __m128 v2;
        for (i = 0; i < row; i++)
        {
                va = input->m + (input_info->row_offset + i) * input->col + input_info->col_offset;
                vc = out_put->m + (i + row_offset) * out_put->col + col_offset;
                for(j=0;j< col ;j++)
                {
                        vb = m->m + j*m->col;
                        col2 = m->col&(~7);
        //              wtk_debug("col2=%d\n",col2);
                        k = 0;
                        for (; k < col2; k+=8)
                        {
                                v = _mm256_dp_ps(_mm256_loadu_ps(va+k),_mm256_loadu_ps(vb+k),0xff);
                                *vc+=v[0]+v[4];

                        }
                        col2 =m->col&(~3);
        //              wtk_debug("col2=%d,k=%d\n",col2,k);
                        for (; k < col2; k+=4)
                        {
                                v2 =_mm_dp_ps(_mm_loadu_ps(va+k),_mm_loadu_ps(vb+k),0xf1);
                                *vc+=v2[0];
                        }
        //              wtk_debug("k=%d\n",k);
                        for(;k <m->col; k++)
                        {
                                *vc+=va[k]*vb[k];
                        }
//                        printf("vc=%.2f\n",*vc);
                        vc++;

                }

        }
//print_float(out_put->m,20);
	if(com->type!=QTK_LinearComponent)
	{
        	for (i = 0; i < row; i++)
        	{
                	for (j = 0; j < col; j++)
                	{
                        	*(out_put->m + (i + row_offset) * out_put->col + j + col_offset) +=
                                        *(b->m + j);
                	}
        	}
    	}
    }
}

/*
 * add by dmd
 * A[m,n] * B[n,k] =C[m,k]
 * c[n,i->i+4/8] += A[n,i] * B[i->i+4/8, i]
 *i->i+4/8: one compute with 4/8 values.
 */
//double tm, tm2, tm_t;  //debug
void qtk_affine_global_propagate_avx2custom(nnet3_component_t* com1, qtk_blas_matrix_t *input,
		qtk_blas_matrix_t *out_put, qtk_nnet3_submatrix_info_t *input_info,
		qtk_nnet3_submatrix_info_t *out_put_info, int porder)
{
	qtk_affine_global_component_t* com;
	int col, row, i, j, row_offset, col_offset;
	qtk_blas_matrix_t *m,*b;
	float *p1, *p2, *p3;
	register float *bp1, *bp3;
	int col2, col3, row2, row3, icol, ocol;
	int step,n,d, ns; //,r,k;
	register int k,r;
	__m128 x1,y1;
	__m256 x, y;

	if(porder != 3)
	{
		wtk_debug("error: porder should set 3[porder=%d]\n", porder);
		return ;
	}

	//tm=time_get_ms();
	porder=3; //only support 3.
	step = 1 << porder;
	com = (qtk_affine_global_component_t*) com1->component;
	m = com->w;
	b = com->b;
	col = out_put_info->num_cols;
	row = out_put_info->num_rows;
	col_offset = out_put_info->col_offset;
	row_offset = out_put_info->row_offset;
	//print_float(input->m,20);
	//print_float(m->m,20);

	col2 = m->col;
	col3 = (col2 >> porder) << porder;
	ns = col3 >> porder;
	row2 = m->row;
	row3 = (row2 >> porder) << porder;
	icol = input->col;
	ocol = out_put->col;
	bp3 = out_put->m + row_offset * ocol + col_offset;
	bp1 = input->m + input_info->row_offset * icol + input_info->col_offset;
	//wtk_debug("input->row=%d input->col=%d output->row=%d output->col=%d row=%d col=%d col2=%d col3=%d\n", input->row, input->col, out_put->row, out_put->col, row, col, col2, col3);
	//wtk_debug("row=%d,col=%d,row_input=%d,col_input=%d,out_col=%d,out_row=%d,row_win=%d,col_win=%d\n",row,col,input->row,input->col,out_put->col,out_put->row,m->row,m->col);
	for (i = 0; i < row; i++)
	{
		//p3 = bp3 + i * out_put->col;
		//bp1 = input->m + (input_info->row_offset+i) * icol + input_info->col_offset;
		//bp3 += out_put->col;
		//bp1 +=  icol ;
		//for(k=0; k<ns; k++)
		p2=m->m;
		p3 = bp3;
		k=ns;
		while(k)
		{
			p1 = bp1;
			n=row2;
			y=_mm256_loadu_ps(p3);
			while(n--)
			{
				x = _mm256_set1_ps(*p1);
				y = _mm256_add_ps(y, _mm256_mul_ps(x, _mm256_loadu_ps(p2)));

				p1++;
				p2 += step;
			}
			_mm256_storeu_ps(p3, y);
			//_mm256_storeu_ps(p3+8, yy);
//			for(n=0;n<step;n++)
//				printf("vc=%.2f\n", *(p3+n));
			//printf("%f\n", *(p3+8));
			p3 += step;
			k--;
		}
		r=col2-col3;
		while(r>>2)
		{
			r=r-4;
			p1 = bp1;
			n=row2;
			y1=_mm_loadu_ps(p3);
			while(n--)
			{
				x1=_mm_set1_ps(*p1);
				y1 = _mm_add_ps(y1, _mm_mul_ps(x1, _mm_loadu_ps(p2)));
				p1++;
				p2 += 4;
			}
			_mm_storeu_ps(p3, y1);
//			for(n=0;n<4;n++)
//				printf("vc=%.2f\n", *(p3+n));
			p3 += 4;
		}
		//for(d=0; d<r; d++)
		d=r;
		while(d--)
		{
			p1 = bp1;
			for(n=0; n+8<row3; n+=8)
			{
				//*p3 += (*p1) * (*p2) + (*(p1 + 1)) * (*(p2 + 1))
				//						+ (*(p1 + 2)) * (*(p2 + 2)) + (*(p1 + 3)) * (*(p2 + 3));
				x=_mm256_dp_ps(_mm256_loadu_ps(p1),_mm256_loadu_ps(p2),0xff);
				*p3 += x[0]+x[4];

				p1 += 8;
				p2 += 8;
			}
			for(; n<row2;n++)
			{
				*p3 += (*p1) * (*p2);
				p1++;
				p2++;
			}
//			printf("vc=%.2f\n", *(p3));
			p3++;
		}
		bp3 += ocol;
		bp1 += icol;
	}
	//wtk_debug("%d %d %d %d %d %d\n",input->row,input->col,m->row,m->col,out_put->row,out_put->col);
	//wtk_debug("row=%d,col=%d,row2=%d,col2=%di,offset=%d,%d\n",m->row,m->col,out_put->row,out_put->col,row_offset,col_offset);
	//print_float(out_put->m,20);
	//qtk_blas_matrix_print(b);
	//exit(0);
	//wtk_debug("com->type=%d\n", com->type);
	bp3=out_put->m + row_offset * ocol + col_offset;
	bp1=b->m;
	if(com->type!=QTK_LinearComponent)
	{
		for (i = 0; i < row; i++)
		{
			for (j = 0; j < col; j++)
			{
				*(bp3 + i * ocol + j) +=
					*(bp1 + j);
			}
		}
	}
	//tm2=time_get_ms();
	//tm_t+=tm2-tm;
	//wtk_debug("tm1-tm=%f tm2=%f\n", tm2-tm, tm_t);
}

void qtk_affine_global_propagate_avxcustom(nnet3_component_t* com1, qtk_blas_matrix_t *input,
		qtk_blas_matrix_t *out_put, qtk_nnet3_submatrix_info_t *input_info,
		qtk_nnet3_submatrix_info_t *out_put_info, int porder)
{
	qtk_affine_global_component_t* com;
	int col, row, i, j, row_offset, col_offset;
	qtk_blas_matrix_t *m,*b;
	float *p1, *p2, *p3;
	register float *bp1, *bp3;
	int col2, col3, row2, row3, icol, ocol;
	int step,n,d, ns; //,r,k;
	register int k,r;
	__m128 x, y;

	if(porder != 2)
	{
		wtk_debug("error: porder should set 2[porder=%d]\n", porder);
		return ;
	}

	step = 1 << porder;
	com = (qtk_affine_global_component_t*) com1->component;
	m = com->w;
	b = com->b;
	col = out_put_info->num_cols;
	row = out_put_info->num_rows;
	col_offset = out_put_info->col_offset;
	row_offset = out_put_info->row_offset;
	//print_float(input->m,20);
	//print_float(m->m,20);

	col2 = m->col;
	col3 = (col2 >> porder) << porder;
	ns = col3 >> porder;
	row2 = m->row;
	row3 = (row2 >> porder) << porder;
	icol = input->col;
	ocol = out_put->col;
	bp3 = out_put->m + row_offset * ocol + col_offset;
	bp1 = input->m + input_info->row_offset * icol + input_info->col_offset;
	//wtk_debug("input->row=%d input->col=%d output->row=%d output->col=%d row=%d col=%d col2=%d col3=%d\n", input->row, input->col, out_put->row, out_put->col, row, col, col2, col3);
	//wtk_debug("row=%d,col=%d,row_input=%d,col_input=%d,out_col=%d,out_row=%d,row_win=%d,col_win=%d\n",row,col,input->row,input->col,out_put->col,out_put->row,m->row,m->col);
	for (i = 0; i < row; i++)
	{
		//p3 = bp3 + i * out_put->col;
		//bp1 = input->m + (input_info->row_offset+i) * icol + input_info->col_offset;
		//bp3 += out_put->col;
		//bp1 +=  icol ;
		//for(k=0; k<ns; k++)
		p2=m->m;
		p3 = bp3;
		k=ns;
		while(k)
		{
			p1 = bp1;
			n=row2;
			y=_mm_loadu_ps(p3);
			while(n--)
			{
				x = _mm_set1_ps(*p1);
				y = _mm_add_ps(y, _mm_mul_ps(x, _mm_loadu_ps(p2)));

				p1++;
				p2 += step;
			}
			_mm_storeu_ps(p3, y);
			//_mm256_storeu_ps(p3+8, yy);
//			for(n=0;n<step;n++)
//				printf("vc=%.2f\n", *(p3+n));
			//printf("%f\n", *(p3+8));
			p3 += step;
			k--;
		}
		r=col2-col3;
		//for(d=0; d<r; d++)
		d=r;
		while(d--)
		{
			p1 = bp1;
			for(n=0; n+4<row3; n+=4)
			{
				//*p3 += (*p1) * (*p2) + (*(p1 + 1)) * (*(p2 + 1))
				//						+ (*(p1 + 2)) * (*(p2 + 2)) + (*(p1 + 3)) * (*(p2 + 3));
				x=_mm_dp_ps(_mm_loadu_ps(p1),_mm_loadu_ps(p2),0xf1);
				*p3 += x[0];

				p1 += 4;
				p2 += 4;
			}
			for(; n<row2;n++)
			{
				*p3 += (*p1) * (*p2);
				p1++;
				p2++;
			}
//			printf("vc=%.2f\n", *(p3));
			p3++;
		}
		bp3 += ocol;
		bp1 += icol;
	}
	//wtk_debug("%d %d %d %d %d %d\n",input->row,input->col,m->row,m->col,out_put->row,out_put->col);
	//wtk_debug("row=%d,col=%d,row2=%d,col2=%di,offset=%d,%d\n",m->row,m->col,out_put->row,out_put->col,row_offset,col_offset);
	//print_float(out_put->m,20);
	//qtk_blas_matrix_print(b);
	//exit(0);
	//wtk_debug("com->type=%d\n", com->type);
	bp3=out_put->m + row_offset * ocol + col_offset;
	bp1=b->m;
	if(com->type!=QTK_LinearComponent)
	{
		for (i = 0; i < row; i++)
		{
			for (j = 0; j < col; j++)
			{
				*(bp3 + i * ocol + j) +=
					*(bp1 + j);
			}
		}
	}
}
#endif
/*
void qtk_linear_propagate(nnet3_component_t* com1, qtk_blas_matrix_t *input,
		qtk_blas_matrix_t *out_put, qtk_nnet3_submatrix_info_t *input_info,
		qtk_nnet3_submatrix_info_t *out_put_info)
{
	qtk_linear_component_t* com;
	int col, row, i, j, row_offset, col_offset;
	qtk_blas_matrix_t *m;

	com = (qtk_linear_component_t*) com1->component;
	m = com->params;
	col = out_put_info->num_cols;
	row = out_put_info->num_rows;
	col_offset = out_put_info->col_offset;
	row_offset = out_put_info->row_offset;

	//cblas_sgemm(CblasRowMajor, CblasNoTrans,CblasTrans,input_info->num_rows, m->row, input_info->num_cols,
	//            1, input->m+input_info->row_offset*input->col+input_info->col_offset, input_info->num_cols, m->m, m->col,
	//            1, out_put->m+row_offset*out_put->col+col_offset, out_put->col);

	float *p1, *p2, *p3, *pe2, *pe1;
	int col2, col3;
	col2 = m->col;
	col3 = (col2 >> 2) << 2;
	for (i = 0; i < row; i++)
	{
		p3 = out_put->m + (i + row_offset) * out_put->col + col_offset;
		//p1=input->m+(input_info->row_offset+i)*input->col+input_info->col_offset;
		for (j = 0; j < col; j++)
		{
			p1 = input->m + (input_info->row_offset + i) * input->col
					+ input_info->col_offset;
			p2 = m->m + j * col2;
			pe2 = p2 + col3;
			pe1 = p2 + col2;
			while (p2 < pe2)
			{
				*p3 += (*p1) * (*p2) + (*(p1 + 1)) * (*(p2 + 1))
						+ (*(p1 + 2)) * (*(p2 + 2)) + (*(p1 + 3)) * (*(p2 + 3));
				p1 += 4;
				p2 += 4;
			}
            while(p2 < pe1)
            {
              *p3 += *(p1)*(*p2);
              //wtk_debug("%f %f\n",*p1,*p2);
              p1++;
              p2++;
            }

			p3++;
		}
	}
}
*/

void qtk_rectified_linear_propagate(nnet3_component_t* com1,
		qtk_blas_matrix_t *dst, qtk_blas_matrix_t *src,
		qtk_nnet3_submatrix_info_t *dst_info,
		qtk_nnet3_submatrix_info_t *src_info)
{
	//qtk_rectified_linear_component_t* com;

	//com=(qtk_rectified_linear_component_t*)com1->component;
	wtk_sub_matrix_cpy_relu2(dst, src, dst_info, src_info, 0.0);
}

void qtk_lstm_nolinearity_propagate(nnet3_component_t* com1,
		qtk_blas_matrix_t *dst, qtk_blas_matrix_t *src,
		qtk_nnet3_submatrix_info_t *dst_info,
		qtk_nnet3_submatrix_info_t *src_info)
{
	qtk_lstm_nolinearity_component_t* comp;
	comp = (qtk_lstm_nolinearity_component_t*) com1->component;
	int num_rows = src_info->num_rows;
	int input_cols = src_info->num_cols;
	int cell_dim = input_cols / 5;

	float* params_data = comp->params->m;
	int params_stride = comp->params->col;
	int r, c;
	float i_scale, f_scale, o_scale, *input_row, *output_row;
	float i_part, f_part, c_part, o_part, c_prev, w_ic, w_fc, w_oc, i_t, f_t,
			c_t, o_t, m_t;

	for (r = 0; r < num_rows; r++)
	{
		input_row = src->m + src_info->col_offset
				+ src->col * (r + src_info->row_offset);
		i_scale = ( input_cols == cell_dim * 5 ? 1.0 : *(input_row + cell_dim * 5));
		f_scale = ( input_cols == cell_dim * 5 ? 1.0 : *(input_row + cell_dim * 5 + 1));
		o_scale = ( input_cols == cell_dim * 5 ? 1.0 : *(input_row + cell_dim * 5 + 2));
		//wtk_debug("%f %f %f\n",i_scale,f_scale,o_scale);	
		output_row = dst->m + dst_info->col_offset + dst->col * (r + dst_info->row_offset);
		for (c = 0; c < cell_dim; c++)
		{
			i_part = *(input_row + c);
			f_part = *(input_row + c + cell_dim);
			c_part = *(input_row + c + 2 * cell_dim);
			o_part = *(input_row + c + 3 * cell_dim);
			c_prev = *(input_row + c + 4 * cell_dim);
			w_ic = *(params_data + c);
			w_fc = *(params_data + params_stride + c);
			w_oc = *(params_data + params_stride * 2 + c);
			i_t = wtk_scalar_sigmoid(i_part + w_ic * c_prev);
			f_t = wtk_scalar_sigmoid(f_part + w_fc * c_prev);
			c_t = f_t * f_scale * c_prev + i_t * i_scale * wtk_scalar_tanh(c_part);
			o_t = wtk_scalar_sigmoid(o_part + w_oc * c_t);
			m_t = o_t * o_scale * wtk_scalar_tanh(c_t);
			//wtk_debug("%f %f %f %f %f\n",i_part,f_part,c_part,o_part,c_prev);
			//wtk_debug("%f %f %f \n",w_ic,w_fc,w_oc);
			//wtk_debug("%f %f %f %f %f\n",i_t,f_t,c_t,o_t,m_t);
			*(output_row + c) = c_t;
			*(output_row + c + cell_dim) = m_t;
		}
	}
}

void qtk_normalize(float *f, int len, float scale)
{
	float *p, *e;
	float y, sum;
	float alpha = 0.0;

	alpha = 1 / (len * scale * scale);			//
	sum = 0.0;

	p = f;
	e = p + len;
	while (p < e)
	{
		sum += *p * (*p);
		++p;
	}
	sum = sum * alpha;
	//sum=alpha*cblas_sdot(len,f,1,f,1);
	//wtk_debug("%f\n",sum);
	y = pow(sum, -0.5);
	//wtk_debug("%f\n",y);
	//wtk_debug("sum %f\n",sum);
	p = f;
	e = p + len;
	while (p < e)
	{
		*p *= y;
		++p;
	}
}
//a =input m=output normallizeComponent
void qtk_normallize_propagate(nnet3_component_t* com1, qtk_blas_matrix_t *dst,
		qtk_blas_matrix_t *src, qtk_nnet3_submatrix_info_t *dst_info,
		qtk_nnet3_submatrix_info_t *src_info)
{
	qtk_normalize_component_t* com;
	int i;

	com = (qtk_normalize_component_t*) com1->component;
	wtk_sub_matrix_cpy2(dst, src, dst_info, src_info);
	//qtk_blas_matrix_print(dst);
	for (i = dst_info->row_offset; i < dst_info->num_rows; ++i)
	{
		qtk_normalize(dst->m + i * dst->col + dst_info->col_offset,
				dst_info->num_cols, com->target_rms_);
	}

}

void wtk_nnet3_log(float *p, int n)
{
	float *pe;

	pe = p + n;
	while (p < pe)
	{
		*p = log(*p);
		++p;
	}
}

void qtk_activate_propagate(nnet3_component_t* com1, qtk_blas_matrix_t *dst,
		qtk_blas_matrix_t *src, qtk_nnet3_submatrix_info_t *dst_info,
		qtk_nnet3_submatrix_info_t *src_info)
{
	qtk_activate_component_t* com;
	int i;
	com = (qtk_activate_component_t*) com1->component;
	//wtk_debug("%d\n",com->type);
	wtk_sub_matrix_cpy2(dst, src, dst_info, src_info);
	switch (com->type)
	{
	case QTK_LogSoftmaxComponent:
		//wtk_sub_matrix_cpy2(dst,src,dst_info,src_info);
		for (i = dst_info->row_offset; i < dst_info->num_rows; ++i)
		{
                    wtk_softmax(dst->m + dst->col * i + dst_info->col_offset,
                                dst_info->num_cols);
                    wtk_nnet3_log(dst->m + dst->col * i + dst_info->col_offset,
                                  dst_info->num_cols);
                }
		break;
        case QTK_SoftmaxComponent:
            // wtk_sub_matrix_cpy2(dst,src,dst_info,src_info);
            for (i = dst_info->row_offset; i < dst_info->num_rows; ++i) {
                wtk_softmax(dst->m + dst->col * i + dst_info->col_offset,
                            dst_info->num_cols);
            }
            break;
        case QTK_SigmoidComponent:
		//wtk_sub_matrix_cpy2(dst,src,dst_info,src_info);
		for (i = dst_info->row_offset; i < dst_info->num_rows; ++i)
		{
                    wtk_sigmoid(dst->m + dst->col * i + dst_info->col_offset,
                                dst_info->num_cols);
                }
		break;
	case QTK_TanhComponent:
		for (i = dst_info->row_offset; i < dst_info->num_rows; ++i)
		{
                    wtk_tanh(dst->m + dst->col * i + dst_info->col_offset,
                             dst_info->num_cols);
                }
		break;
	default:
		break;
	}
}

void qtk_batchnorm_cal(qtk_sub_matrix_t* in, qtk_sub_matrix_t* out,
		qtk_batch_norm_component_t *com)
{
	int i, j, stride;
	float scale, offset;

	stride = out->stride;
	qtk_matrix_copy_from_mat(out, in);

/*        float x=0;
        for(i=0;i<out->row*out->col;++i)
        {
                x = out->f[i];
                wtk_debug("out[%d]=%f\n",i,x);
        }
*/
	for (i = 0; i < out->row; i++)
	{
		for (j = 0; j < out->col; j++)
		{
			scale = *(com->scale->m + j);
			offset = *(com->offset->m + j);
			//wtk_debug("%f %f %f\n",scale,offset,*(out->f+j+i*stride));
			*(out->f + j + i * stride) *= scale;
			//wtk_debug("%f %f %f\n",scale,offset,*(out->f+j+i*stride));
			*(out->f + j + i * stride) += offset;
			//wtk_debug("%f %f %f\n",scale,offset,*(out->f+j+i*stride));
		}
	}
}

void qtk_batchnorm_propagate(nnet3_component_t* com1, qtk_blas_matrix_t *src,
		qtk_blas_matrix_t *dst, qtk_nnet3_submatrix_info_t *dst_info,
		qtk_nnet3_submatrix_info_t *src_info)
{
	qtk_batch_norm_component_t *com =
			(qtk_batch_norm_component_t*) com1->component;
	qtk_sub_matrix_t *input, *output ,*in_reshape, *out_reshape;;

	input = com->input;
	output = com->output;
	in_reshape = com->in_reshape;
	out_reshape = com->out_reshape;

	qtk_sub_matrix_set(input, src->m, src_info->row_offset, src_info->num_rows,
			src_info->col_offset, src_info->num_cols, src->col);
	qtk_sub_matrix_set(output, dst->m, dst_info->row_offset,
			dst_info->num_rows, dst_info->col_offset, dst_info->num_cols,
			dst->col);
/*	int i;
	float x=0;
	for(i=0;i<input->row*input->col;++i)
	{
		x = input->f[i];
		wtk_debug("input[%d]=%f\n",i,x);
	}*/

	if (input->col != com->block_dim)
	{
		int ratio = com->dim / com->block_dim;
		int orig_rows = input->row;
		int orig_cols = input->col;
		int new_rows = orig_rows * ratio;
		int new_cols = orig_cols / ratio;
		//wtk_debug("batch %d %d\n",new_rows,new_cols);
		qtk_sub_matrix_set2(in_reshape, input->f, new_rows, new_cols, new_cols);
		qtk_sub_matrix_set2(out_reshape, output->f, new_rows, new_cols, new_cols);
		qtk_batchnorm_cal(in_reshape, out_reshape, com);
	} else {
		qtk_batchnorm_cal(input, output, com);
	}
}

void qtk_backprop_truncation_propagate(nnet3_component_t* com1,
		qtk_blas_matrix_t *src, qtk_blas_matrix_t *dst,
		qtk_nnet3_submatrix_info_t *dst_info,
		qtk_nnet3_submatrix_info_t *src_info)
{
	qtk_backprop_truncation_component_t *com =
			(qtk_backprop_truncation_component_t*) com1->component;
        qtk_sub_matrix_t input, output;
        int i, j;

        qtk_sub_matrix_init(&input, src->m, src_info->row_offset,
                            src_info->num_rows, src_info->col_offset,
                            src_info->num_cols, src->col);
        qtk_sub_matrix_init(&output, dst->m, dst_info->row_offset,
                            dst_info->num_rows, dst_info->col_offset,
                            dst_info->num_cols, dst->col);
        qtk_matrix_copy_from_mat(&output, &input);

        for (i = 0; i < output.row; i++)
                for (j = 0; j < output.col; j++)
                        *(output.f + j + i * output.stride) *= com->scale;
}

void qtk_natural_gradient_per_element_scale_propagate(nnet3_component_t* com1,
		qtk_blas_matrix_t *src, qtk_blas_matrix_t *dst,
		qtk_nnet3_submatrix_info_t *dst_info,
		qtk_nnet3_submatrix_info_t *src_info)
{
	int i, j;
	float scale;
	qtk_natural_gradient_per_element_scale_component_t *com =
			(qtk_natural_gradient_per_element_scale_component_t*) com1->component;
	wtk_sub_matrix_cpy2(dst, src, dst_info, src_info);
	for (i = 0; i < dst_info->num_rows; ++i)
	{
		for (j = 0; j < dst_info->num_cols; j++)
		{
			scale = com->scale[1][1 + j];
			//wtk_debug("heiha:%f %f\n",*(dst->m+(i+dst_info->row_offset)*(dst_info->num_cols+dst_info->col_offset)+j+dst_info->col_offset),scale);
			*(dst->m
					+ (i + dst_info->row_offset)
							* (dst_info->num_cols + dst_info->col_offset) + j
					+ dst_info->col_offset) *= scale;
		}
		//wtk_float_scale(src->m+src->col*i,com->scale[1][1+i],dst->m+dst->col*i,dst_info->num_cols);
	}

}
void qtk_dropout_propagate(nnet3_component_t* com1, qtk_blas_matrix_t *src,
		qtk_blas_matrix_t *dst, qtk_nnet3_submatrix_info_t *src_info,
		qtk_nnet3_submatrix_info_t *dst_info)
{
	int i;
	qtk_dropout_component_t* com = (qtk_dropout_component_t*) com1->component;
	float scale;
	scale = 1.0 - com->dropout_proportion;
	for (i = dst_info->row_offset; i < dst_info->num_rows; ++i)
		wtk_float_scale(src->m + src->col * i + src_info->col_offset, scale,
				dst->m + dst->col * i + dst_info->col_offset,
				dst_info->num_cols);
}
void qtk_element_wise_product_propagate(nnet3_component_t* com1,
		qtk_blas_matrix_t *src, qtk_blas_matrix_t *dst,
		qtk_nnet3_submatrix_info_t *dst_info,
		qtk_nnet3_submatrix_info_t *src_info)
{
	qtk_element_wise_product_component_t* com =
			(qtk_element_wise_product_component_t*) com1->component;
	int num_inputs = com->input_dim / com->output_dim;
	int i, j;
	float *f, *f2, *p;
	p = src->m + src_info->row_offset * src_info->num_cols
			+ src_info->col_offset;
//  wtk_debug("elewise %d %d %d\n",com->input_dim,com->output_dim,src->col);
	for (i = 0; i < num_inputs; i++)
	{
		if (i == 0)
		{
			memcpy(dst->m + dst_info->row_offset * dst_info->num_cols + dst_info->col_offset, p,sizeof(float) * com->output_dim);
		} else {
			//TODO
			f = dst->m + dst_info->row_offset * dst_info->num_cols
					+ dst_info->col_offset;
			f2 = p + i * com->output_dim;
			for (j = 0; j < com->output_dim; j++)
			{
				(*f) *= (*f2);
				f++;
				f2++;
			}
		}
	}
}

void qtk_noop_propagate(nnet3_component_t* com1, qtk_blas_matrix_t *src,
		qtk_blas_matrix_t *dst, qtk_nnet3_submatrix_info_t *dst_info,
		qtk_nnet3_submatrix_info_t *src_info)
{
	wtk_sub_matrix_cpy2(dst, src, dst_info, src_info);
}
#ifdef USE_AFFINE_FAST
void qtk_nnet3_add_matmat_fast(qtk_sub_matrix_t *dst, qtk_sub_matrix_t *src,
                               qtk_sub_matrix_t *linear) {
    /* m is out row
       n is out col
       k is col of in or com->w */
    int m, n, k;
    /* ldi is offset the small matrix of col
       ldo is offset the small matrix of col
       ldw is offset the small matrix of col */
    int ldi, ldo, ldw;
    /* in_pntr is input small matrix pointer
       w_pntr is parameters matrix pointer
   out_pntr is output small matrix pointer */
    float *in_pntr, *out_pntr, *w_pntr;

    m = dst->row;
    n = dst->col;
    k = linear->col;

    ldi = src->stride;
    ldw = linear->stride;
    ldo = dst->stride;

    in_pntr = src->f;
    w_pntr = linear->f;
    out_pntr = dst->f;

    /*w matrix transpose is performed by default*/
    matrix_mul(m, n, k, in_pntr, ldi, w_pntr, ldw, out_pntr, ldo);
}
#endif

void qtk_timeheight_convolution_forward_internal(
		qtk_nnet3_convolution_precomputed_indexes_t* index,
		qtk_sub_matrix_t* input, qtk_blas_matrix_t *linear,
		qtk_sub_matrix_t* temp_mat, qtk_sub_matrix_t* output,
		qtk_blas_matrix_t* bias)
{
	int num_steps = index->num_steps;
	int s;
	qtk_nnet3_convolution_step_t *step;
	qtk_sub_matrix_t *input_part, *params_part, *output_reshaped,
			*temp_mat_part, *temp_mat_part_reshaped, *input_reshaped;

//	wtk_debug("cnn forword internal steps %d\n",num_steps);	
	for (s = 0; s < num_steps; s++)
	{
		step = index->steps[s];
		input_part = step->input_part;
		params_part = step->params_part;
		output_reshaped = step->output_reshaped;
		temp_mat_part = step->temp_mat_part;
		temp_mat_part_reshaped = step->temp_mat_part_reshaped;
		input_reshaped = step->input_reshaped;

		int input_row_start = step->input_time_shift * index->num_images;
		int temp_num_cols = step->columns[0];
		int params = temp_num_cols / index->height_out;
		qtk_sub_matrix_set(input_part, input->f, input_row_start, output->row,
				0, input->col, input->stride);
//		wtk_debug("%d %d %d %d\n",linear->row,step->params_start_col,linear->col,params);
		qtk_sub_matrix_set(params_part,linear->m, 0, linear->row,
				step->params_start_col, params, linear->col);
		qtk_sub_matrix_set2(output_reshaped,output->f,
				output->row * index->height_out, index->num_filters_out,
				index->num_filters_out);

		if (!step->columns_are_contiguous || temp_num_cols != input->col)
		{
			qtk_sub_matrix_set2(temp_mat_part,temp_mat->f, temp_mat->row,
					temp_num_cols, temp_num_cols);
//			wtk_debug("%d\n",step->columns_are_contiguous);
			if (!step->columns_are_contiguous)
			{
				qtk_matirx_copy_cols(temp_mat_part, input_part, step->columns);
			} else {
				qtk_sub_matrix_t* new_temp;
				new_temp = step->newtemp;
				qtk_sub_matrix_set(new_temp,input_part->f, 0, input_part->row,
						step->first_column, step->columns[0],
						input_part->stride);
				qtk_matrix_copy_from_mat(temp_mat_part, new_temp);
			}

//			wtk_debug("---------tempart\n");
//			qtk_sub_matrix_print(temp_mat_part);
			qtk_sub_matrix_set2(temp_mat_part_reshaped,temp_mat_part->f,
					temp_mat_part->row * index->height_out,
					temp_num_cols / index->height_out,
					temp_num_cols / index->height_out);
			//ADDMATMAT
//			wtk_debug("---------temp1\n");
//			qtk_sub_matrix_print(temp_mat_part_reshaped);
//			wtk_debug("---------parms1\n");
//			qtk_sub_matrix_print(params_part);
#ifdef USE_AVX
            qtk_matrix_add_matmat_avx(output_reshaped, temp_mat_part_reshaped,
                     params_part);
#elif USE_AFFINE_FAST
            qtk_nnet3_add_matmat_fast(output_reshaped, temp_mat_part_reshaped,
                                      params_part);
#else
			qtk_matrix_add_matmat(output_reshaped, temp_mat_part_reshaped,
					params_part,1.0);
#endif
//			wtk_debug("---------sub2\n");
//			qtk_sub_matrix_print(output_reshaped);
		} else {
			qtk_sub_matrix_set2(input_reshaped,input_part->f,
					input_part->row * index->height_out,
					input_part->col / index->height_out,
					input_part->col / index->height_out);
			//ADDMATMAT
			//wtk_debug("--------temp2\n");
			//qtk_sub_matrix_print(temp_mat_part_reshaped);) {
			//wtk_debug("--------parms2\n");
			//qtk_sub_matrix_print(params_part);
#ifdef USE_AVX
            qtk_matrix_add_matmat_avx(output_reshaped, input_reshaped,
                    params_part);
#else
			qtk_matrix_add_matmat(output_reshaped, input_reshaped, params_part,1.0);
#endif
			//wtk_debug("--------sub2\n");
			//qtk_sub_matrix_print(output_reshaped);
		}
	}

}

void qtk_timeheight_convolution_forward(
		qtk_nnet3_convolution_precomputed_indexes_t* index,
		qtk_sub_matrix_t* input, qtk_blas_matrix_t *linear,
		qtk_sub_matrix_t* output, qtk_blas_matrix_t *bias)
{
	int input_rows = input->row;
	int required_input_rows = index->num_images * index->num_t_in;
	//wtk_debug("%d %d\n",index->num_images,index->num_t_in);
	if (input_rows != required_input_rows)
	{
//		wtk_debug("reshape cnn input %d %d\n",input_rows,required_input_rows);
		int num_cols = input->col;
		int multiple = input_rows / required_input_rows;
		int new_num_cols = num_cols * multiple;
		int new_stride = new_num_cols;
		qtk_sub_matrix_t* input_reshaped;

		input_reshaped = index->input_reshaped;
		qtk_sub_matrix_set2(input_reshaped, input->f, required_input_rows,
				new_num_cols, new_stride);
		qtk_timeheight_convolution_forward(index, input_reshaped, linear,
				output, bias);
		return;
	}

	qtk_blas_matrix_t *temp_mat = qtk_blas_matrix_new(index->temp_rows,
			index->temp_cols);

	if (index->temp_rows != 0 && index->temp_cols != input_rows)
	{
//		wtk_debug("reshape all part\n");
		int num_time_steps_per_chunk = index->temp_rows / index->num_images;
		int num_extra_in = index->num_t_in - index->num_t_out;
		int t_start;

		for (t_start = 0; t_start < index->num_t_out; t_start +=
				num_time_steps_per_chunk)
		{
//			wtk_debug("reshape all part\n");
			int num_t_left = index->num_t_out - t_start;
			int this_num_t_out =
					(num_t_left < num_time_steps_per_chunk) ?
							num_t_left : num_time_steps_per_chunk;
			int this_num_t_in = this_num_t_out + num_extra_in;
			qtk_sub_matrix_t* input_part, *output_part, *temp_part;
			input_part = index->input_part;
			output_part = index->output_part;
			temp_part = index->temp_part;

			qtk_sub_matrix_set(input_part, input->f,
					t_start * index->num_images,
					this_num_t_in * index->num_images, 0, input->col,
					input->stride);
			qtk_sub_matrix_set(output_part, output->f,
					t_start * index->num_images,
					this_num_t_out * index->num_images, 0, output->col,
					output->stride);
			qtk_sub_matrix_set(temp_part, temp_mat->m, 0,
					this_num_t_out * index->num_images, 0, temp_mat->col,
					temp_mat->col);

			qtk_timeheight_convolution_forward_internal(index, input_part,
					linear, temp_part, output_part, bias);
		}
		qtk_blas_matrix_delete(temp_mat);
		return;
	}
	qtk_sub_matrix_t* temp = index->temp;
	qtk_sub_matrix_set2(temp, temp_mat->m, index->temp_rows,
			index->temp_cols, index->temp_cols);
	qtk_timeheight_convolution_forward_internal(index, input, linear, temp,
			output, bias);
	qtk_blas_matrix_delete(temp_mat);

}

void qtk_timeheight_convolution_propagate(nnet3_component_t* com1,
		qtk_blas_matrix_t *dst, qtk_blas_matrix_t *src,
		qtk_nnet3_submatrix_info_t *dst_info,
		qtk_nnet3_submatrix_info_t *src_info,
		qtk_nnet3_precomputed_indexes_t* index)
{
	qtk_timeheight_convolution_component_t *com = com1->component;
	qtk_nnet3_convolution_precomputed_indexes_t *precompute =
			(qtk_nnet3_convolution_precomputed_indexes_t*) index->index;
	qtk_sub_matrix_t *input, *output, *output_shape;
	qtk_timeheight_convolution_model_t *model = com->model;

	input = com->input;
	output = com->output;
	output_shape = com->output_shape;

	qtk_sub_matrix_set(input, src->m, src_info->row_offset, src_info->num_rows,
			src_info->col_offset, src_info->num_cols, src->col);
	qtk_sub_matrix_set(output, dst->m, dst_info->row_offset,
			dst_info->num_rows, dst_info->col_offset, dst_info->num_cols,
			dst->col);
	qtk_sub_matrix_set2(output_shape,output->f,
			output->row * model->height_out, model->num_filters_out,
			model->num_filters_out);
	qtk_matrix_copy_rows_fromvec(output_shape, com->bias_params);

	//wtk_debug("=====test=====%d %d\n",output->row*model->height_out,model->num_filters_out);
	//qtk_sub_matrix_print(output_shape);

	qtk_timeheight_convolution_forward(precompute, input, com->linear_params,
			output, com->bias_params);
}

void qtk_general_dropout_component_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *dst,qtk_blas_matrix_t *src,
        qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info,qtk_nnet3_precomputed_indexes_t* index)
{
//	qtk_general_dropout_component_t *com =
//				(qtk_scale_offset_component_t*) com1->component;
//	qtk_nnet3_generaldropout_precomputed_indexes_t *indexes =
//			(qtk_nnet3_generaldropout_precomputed_indexes_t*) index->index;

	wtk_sub_matrix_cpy2(dst, src, dst_info, src_info);

	//qtk_blas_matrix_t *memo = qtk_blas_matrix_new(indexes->num_mask_rows,com->block_dim);
	//float dropout_proportion = com->dropout_proportion;

}

void qtk_scale_offset_propagate(nnet3_component_t* com1, qtk_blas_matrix_t *dst,
		qtk_blas_matrix_t *src, qtk_nnet3_submatrix_info_t *dst_info,
		qtk_nnet3_submatrix_info_t *src_info)
{
	qtk_scale_offset_component_t *com =
			(qtk_scale_offset_component_t*) com1->component;
	float scale, offset;
	int i, j, multiple, dim;

	dim = com->scales->col;
	multiple = com->dim / dim;
	wtk_sub_matrix_cpy2(dst, src, dst_info, src_info);
	//wtk_debug("%d\n",dst_info->num_cols);
	//wtk_debug("scalex %d %d\n",multiple,dim);
	for (i = 0; i < multiple * src_info->num_rows; i++)
	{
		for (j = 0; j < dim; j++)
		{
			scale = *(com->scales->m + j);
			offset = *(com->offsets->m + j);
			*(dst->m + dst_info->col_offset + j
					+ (dst_info->row_offset + i) * dim) *= scale;
			*(dst->m + dst_info->col_offset + j
					+ (dst_info->row_offset + i) * dim) += offset;
		}
	}
}

void qtk_affine_global_delete(nnet3_component_t* com,unsigned int is_fixed)
{
	qtk_affine_global_component_t* comp;
        comp = (qtk_affine_global_component_t*) com->component;
	if(is_fixed)
	{
		if(comp->ws!=NULL)
        	{
                	wtk_mats_delete(comp->ws);
        	}
	}else
	{
        	if(comp->w!=NULL)
        	{
              		qtk_blas_matrix_delete(comp->w);
        	}
	}
	switch(comp->type)
	{
		case QTK_AffineComponent:
		case QTK_FixedAffineComponent:
			if(comp->b!=NULL)
			{
		        	qtk_blas_matrix_delete(comp->b);
			}
			break;
		case QTK_NaturalGradientAffineComponent:
			if(comp->b!=NULL)
			{
                        	qtk_blas_matrix_delete(comp->b);
			}
			if(comp->nga_com)
			{
				if (comp->nga_com->out)
        			{
                			wtk_free(comp->nga_com->out);
        			}
        			if (comp->nga_com->in)
        			{
                			wtk_free(comp->nga_com->in);
        			}
				wtk_free(comp->nga_com);
			}
			break;
		case QTK_LinearComponent:
			if(comp->linear_com)
			{
				if(comp->linear_com->in)
				{
					wtk_free(comp->linear_com->in);
				}
				if(comp->linear_com->out)
				{
					wtk_free(comp->linear_com->out);
				}
			}
			wtk_free(comp->linear_com);
			break;
		default:
			break;
	}
}

void qtk_max_pooling_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *dst,qtk_blas_matrix_t *src,
			                qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info)
{
	//qtk_blas_matrix_print(src);
	qtk_max_pooling_component_t* comp = (qtk_max_pooling_component_t*)com1->component;
	int num_frames = src_info->num_rows;
	int num_pools = comp->output_dim;
	int pool_size = comp->pool_size;
	int x,y,z,x_pool,y_pool,z_pool,index=0;
    int num_pools_x = 1 + (comp->input_x_dim - comp->pool_x_size) / comp->pool_x_step;
    int num_pools_y = 1 + (comp->input_y_dim - comp->pool_y_size) / comp->pool_y_step;
    int num_pools_z = 1 + (comp->input_z_dim - comp->pool_z_size) / comp->pool_z_step;

	int column_size = (comp->pool_x_size)*(comp->pool_y_size)*(comp->pool_z_size)*num_pools_x*num_pools_y*num_pools_z;
	int *map = (int*)wtk_malloc(column_size*sizeof(int));
	memset(map,0,column_size*sizeof(int));
	for(x = 0;x < comp->pool_x_size; x++)
	{
		for(y = 0;y < comp->pool_y_size; y++)
		{
			for(z = 0;z < comp->pool_z_size; z++)
			{
				for (x_pool = 0; x_pool < num_pools_x; x_pool++) 
				{
					for (y_pool = 0; y_pool < num_pools_y; y_pool++) 
					{
						for (z_pool = 0; z_pool < num_pools_z; z_pool++, index++) 
						{
							map[index] = (x_pool * comp->pool_x_step + x) * comp->input_y_dim * comp->input_z_dim +
                                  (y_pool * comp->pool_y_step + y) * comp->input_z_dim +
                                  (z_pool * comp->pool_z_step + z);
						}
					}
				}
			}
		}
	}
	qtk_sub_matrix_t* sub = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
	sub->row = num_frames;
	sub->col = num_pools*pool_size;
	sub->stride = sub->col;
	sub->f = (float*)wtk_malloc(sizeof(float)*sub->row*sub->col);

	int r,c;
	int* index_ptr;
   	index_ptr = map;
	float *src_data,*this_data;
	this_data = sub->f; 
	src_data = src->m + src_info->col_offset + src_info->row_offset*src_info->num_cols;
	for(r=0;r<sub->row;r++,this_data+=sub->stride,src_data+=src_info->num_cols)
	{
		for(c=0;c<sub->col;c++)
		{
			if(*index_ptr < 0)
			{
				this_data[c] = 0;
			}else
			{
				this_data[c] = src_data[*(index_ptr+c)];
			}
			//wtk_debug("%d %f\n",c,this_data[c]);
		}
	}
	//wtk_debug("maxpool %d %d %d\n",dst_info->col_offset,dst_info->num_cols,dst->col)

	memset(dst->m+dst_info->row_offset*dst->col+dst_info->col_offset,-10000.0,dst_info->num_cols*dst_info->num_rows*sizeof(float));
	int q;
	float *subf,*row_data,*orow_data;
	//wtk_debug("poolsize:%d %d\n",pool_size,pool_size*num_pools);
	for(q=0;q<pool_size;q++)
	{
		subf=sub->f+q * num_pools;//row:sub->row  col:num_pools
		//wtk_debug("%d\n",q*num_pools);
		for(r=0;r<dst_info->num_rows;r++)
		{	//wtk_debug("heihei %d\n",sub->stride*r);
			row_data=dst->m+dst_info->col_offset+(r+dst_info->row_offset)*dst->col;
			orow_data=subf+sub->stride*r;
			for(c=0;c<dst_info->num_cols;c++)
			{
				//wtk_debug("%d %f %f\n",c,row_data[c],orow_data[c]);
				row_data[c]=max(row_data[c],orow_data[c]);
			}
		}
	}

	//wtk_debug("output\n");
	//qtk_blas_matrix_print(dst);
        wtk_free(map);
        wtk_free(sub->f);
        wtk_free(sub);
        //exit(0);
}

float qtk_pnorm(float *p, int len)
{
	int i;
	float sum = 0.0;
	for(i=0;i<len;i++)
	{
		sum += *(p+i)*(*(p+i));
	}
	//wtk_debug("%f\n", sum);
	return sqrt(sum);
}

void qtk_pnorm_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *dst,qtk_blas_matrix_t *src,
				                qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info)
{
	int group_size = src_info->num_cols/dst_info->num_cols;
	int num_rows = dst_info->num_rows;
	int num_cols = dst_info->num_cols;
	int i,j,len;
	float *p;
	//wtk_debug("num_rows=%d num_cols=%d group_size=%d\n", num_rows, num_cols, group_size);
	//wtk_debug("src_info->row_offset=%d src_info->col_offset=%d src->col=%d\n", src_info->row_offset,src_info->col_offset,src->col);
	for(i=0;i<num_rows;i++)
	{
		for(j=0;j<num_cols;j++)
		{
			len = group_size;
			p = src->m+(src_info->row_offset+i)*src->col+(src_info->col_offset+j*len);
			*(dst->m+(dst_info->row_offset+i)*num_cols+(dst_info->col_offset+j)) = qtk_pnorm(p,len);
		}
	}
	
}

/*
void qtk_fix_affine_delete(nnet3_component_t* com)
{
	qtk_fixed_affine_componet_t* comp;
	comp = (qtk_fixed_affine_componet_t*) com->component;

	qtk_blas_matrix_delete(comp->w);
	qtk_blas_matrix_delete(comp->b);
}
void qtk_affine_delete(nnet3_component_t* com)
{
	qtk_affine_componet_t* comp;

	comp = (qtk_affine_componet_t*) com->component;
	qtk_blas_matrix_delete(comp->w);
	qtk_blas_matrix_delete(comp->b);
}
void qtk_natural_gradient_affine_delete(nnet3_component_t* com)
{
	qtk_natural_gradient_affine_component_t* comp;

	comp = (qtk_natural_gradient_affine_component_t*) com->component;
	if (comp->out)
	{
		wtk_free(comp->out);
	}
	if (comp->in)
	{
		wtk_free(comp->in);
	}
	qtk_blas_matrix_delete(comp->w);
	qtk_blas_matrix_delete(comp->b);
}*/
void qtk_rectified_linear_delete(nnet3_component_t* com)
{
	qtk_rectified_linear_component_t* comp;

	comp = (qtk_rectified_linear_component_t*) com->component;
	if (comp->value_sum != NULL)
	{
		wtk_matrix_delete(comp->value_sum);
	}
	if (comp->deriv_sum != NULL)
	{
		wtk_matrix_delete(comp->deriv_sum);
	}
	if (comp->oderiv_sum != NULL)
	{
		wtk_matrix_delete(comp->oderiv_sum);
	}
}
void qtk_normallize_delete(nnet3_component_t* com)
{
	return;
}
void qtk_activate_delete(nnet3_component_t* com)
{
	qtk_activate_component_t* comp;

	comp = (qtk_activate_component_t*) com->component;
	if (comp->value_sum != NULL)
	{
		wtk_matrix_delete(comp->value_sum);
	}
	if (comp->deriv_sum != NULL)
	{
		wtk_matrix_delete(comp->deriv_sum);
	}
	if (comp->oderiv_sum != NULL)
	{
		wtk_matrix_delete(comp->oderiv_sum);
	}
}

void qtk_lstm_nolinearity_delete(nnet3_component_t* com,unsigned int is_fixed)
{
	qtk_lstm_nolinearity_component_t* comp;

	comp = (qtk_lstm_nolinearity_component_t*) com->component;
	/*if(is_fixed)
	{
	        if(comp->params2!=NULL)
        	{
            		wtk_mats_delete(comp->params2);
        	}
	}else*/
	{
        	if (comp->params != NULL)
        	{
            		qtk_blas_matrix_delete(comp->params);
        	}
	}

	return;
}

void qtk_timeheight_convolution_delete(nnet3_component_t* com,unsigned int is_fixed)
{
    qtk_timeheight_convolution_component_t* comp;
    
    comp = (qtk_timeheight_convolution_component_t*) com->component;
    if(is_fixed)
    {
	    if (comp->linear_params2 != NULL)
    	    {
            	wtk_mats_delete(comp->linear_params2);
   	    }
/*    	    if (comp->bias_params2 != NULL)
    	    {
   	   	wtk_mats_delete(comp->bias_params2);
           }*/
    }else
    {
    	if (comp->linear_params != NULL)
    	{
        	qtk_blas_matrix_delete(comp->linear_params);
    	}
    }

	if (comp->bias_params != NULL)
    	{
        	qtk_blas_matrix_delete(comp->bias_params);
    	}
   // }

    if (comp->model != NULL)
    {
    	wtk_free(comp->model);
    }
    if(comp->input)
    {
        wtk_free(comp->input);
        wtk_free(comp->output);
        wtk_free(comp->output_shape);
    }
    return;
}

void qtk_batchnorm_delete(nnet3_component_t* com,unsigned int is_fixed)
{
    qtk_batch_norm_component_t* comp;

    comp = (qtk_batch_norm_component_t*) com->component;
    if (comp->stats_sum != NULL)
    {
        qtk_blas_matrix_delete(comp->stats_sum);
    }
    if (comp->stats_sumsq != NULL)
    {
        qtk_blas_matrix_delete(comp->stats_sumsq);
    }
    if(is_fixed)
    {
	if (comp->scale2 != NULL)
        {
                wtk_mats_delete(comp->scale2);
        }
	if (comp->offset2 != NULL)
    	{
        	wtk_mats_delete(comp->offset2);
    	}
    }else
    {
    	if (comp->scale != NULL)
    	{
        	qtk_blas_matrix_delete(comp->scale);
    	}
    	if (comp->offset != NULL)
    	{
        	qtk_blas_matrix_delete(comp->offset);
    	}
    }
    if(comp->input)
    {
    	wtk_free(comp->input);
    	wtk_free(comp->output);
    	wtk_free(comp->in_reshape);
    	wtk_free(comp->out_reshape);
    }
}

void qtk_scale_offset_delete(nnet3_component_t* com,unsigned int is_fixed)
{
	qtk_scale_offset_component_t *comp;

    comp = (qtk_scale_offset_component_t*) com->component;
    if (comp->nonzero_scales != NULL)
    {
        qtk_blas_matrix_delete(comp->nonzero_scales);
    }
    if(is_fixed)
    {
    	if (comp->scales2 != NULL)
    	{
        	wtk_mats_delete(comp->scales2);
        }
	if (comp->offsets2 != NULL)
        {
                wtk_mats_delete(comp->offsets2);
        }
    }else
    {
	if (comp->scales != NULL)
        {
        	qtk_blas_matrix_delete(comp->scales);
    	}
	if (comp->offsets != NULL)
    	{
        	qtk_blas_matrix_delete(comp->offsets);
    	}

    }

}

void qtk_natural_gradient_per_element_scale_delete(nnet3_component_t* com,unsigned int is_fixed)
{
	qtk_natural_gradient_per_element_scale_component_t *comp;
	comp = (qtk_natural_gradient_per_element_scale_component_t*)com->component;
	if(is_fixed)
	{
	        if(comp->scale2)
        	{
                	wtk_mats_delete(comp->scale2);
        	}
	}else
	{
		if(comp->scale)
		{
			wtk_matrix_delete(comp->scale);
		}
	}
}

void qtk_max_pooling_delete(nnet3_component_t* com,unsigned int is_fixed)
{
}

void qtk_pnorm_delete(nnet3_component_t* com,unsigned int is_fixed)
{
}

void qtk_namask_component_generate(qtk_natrual_gradient_affine_mask_component_t* sp)
{
	float *p=sp->maskt->m;
	int i,j;
	int *b;

	sp->block = (int*)wtk_malloc(sizeof(int)*sp->row*sp->block_col);
	b = sp->block;
	//wtk_debug("%d %d\n",12*sp->row*sp->block_col,src->row*src->col);
	int num1=0;
	for(i = 0; i < sp->row*sp->block_col;i++)
	{
		*b = 0;
		for(j = 0; j < sp->block_size; j++)
		{
			if(*p != 0)
			{
				*b = 1;
			}
			p++;
		}
		if(*b!=0)
		{
			num1++;
		}
		b++;
	}

	sp->f = (float*)wtk_malloc(sizeof(float)*num1*sp->block_size);
	sp->num_block = num1;
	//wtk_debug("num_block=%d row=%d block_col=%d rate=%d\n", num1, sp->row,sp->block_col, sp->row*sp->block_col);
	float* f = sp->f;
	p = sp->rawt->m;

	for(i = 0; i < sp->row*sp->block_col;i++)
	{
		if(*(sp->block+i)==1)
		{
			memcpy(f,p,sizeof(float)*sp->block_size);
			f = f + sp->block_size;
		}
		p = p + sp->block_size;
	}

	sp->block2 = (int*)wtk_malloc(sizeof(int)*num1);
	sp->in = (int*)wtk_malloc(sizeof(int)*num1);
	b = sp->block;
	num1 = 0;
	//int num2 = 0;
	//wtk_debug("%d\n",sp->row*sp->block_col);
	for(i = 0; i < sp->row ;i++)
	{
		for(j = 0;j < sp->block_col;j++)
		{
			if(*b==1)
			{
				*(sp->block2 + num1) = j;
				*(sp->in + num1) = i;
				//wtk_debug("%d %d\n",*(sp->block2 + num1),*(sp->in + num1));
				num1++;
			}
			b++;
		}
	}
	//wtk_debug("%d %d %d %d\n",sp->num_block,sp->block_size,sp->raw->row,sp->raw->col);
}

/**
 * add by dmd 2020.11.2
 * obtain all block with nozero. and adjust store structure, overwrite sp->rawt.
 * note:
 *     every block, it's values will be all zero. or not be zero for all. also will be zero and nozero..
 *     mask should int type(0,1), but fact float type.
 */
void qtk_namask_component_generate_custom(qtk_natrual_gradient_affine_mask_component_t* sp)
{
	float *p, *f;
	int i,k,num, num2;
	int *b;

	sp->block = (int*)wtk_calloc(1, sizeof(int)*sp->row*sp->block_col);
	b = sp->block;
	//wtk_debug("%d %d\n",12*sp->row*sp->block_col,src->row*src->col);
	p=sp->maskt->m;
	num=0;
	for(i=0,num2=sp->row*sp->block_col; i < num2;i++)
	{
		k=sp->block_size;
		while(k)
		{
			if(*p != 0)
			{
				*b = 1;
				p += k;
				break;
			}
			p++;
			k--;
		}
		if(*b!=0)num++;
		b++;
	}

	sp->f = (float*)wtk_malloc(sizeof(float)*num*sp->block_size);
	sp->num_block = num;
	sp->block2 = (int*)wtk_malloc(sizeof(int)*num);
	sp->in = (int*)wtk_malloc(sizeof(int)*num);
	sp->ncblock = (int*)wtk_malloc(sizeof(int)*sp->block_col);
	f = sp->f;
	for (num=0,num2=0, k=0; k < sp->block_col; k++)
	{
		for (i=0; i< sp->row; i++)
		{
			b=sp->block+i*sp->block_col+k;
			p=sp->rawt->m + i*sp->block_col*sp->block_size + k*sp->block_size;
			if(*b==1)
			{
				memcpy(f,p,sizeof(float)*sp->block_size);
				f = f + sp->block_size;
				*(sp->block2 + num) = k;
				*(sp->in + num) = i;
				num++;
			}
		}
		sp->ncblock[k]=num-num2;
		num2=num;
	}
}

/*
 * add by dmd
 * for verifying perfermance with no cosidering soarse feats. same to compute with all value(zero and nozero.)
 */
//void qtk_namask_component_generate_test(qtk_natrual_gradient_affine_mask_component_t* sp)
//{
//	//verify comm compute.
//	float *p, *v;
//	int i,j,k, step, col2,r,h,d;
//
//	//wtk_debug("%d %d\n",12*sp->row*sp->block_col,src->row*src->col);
//    step = 8;
//    col2= (sp->rawt->col >> 3) << 3;
//    p=sp->raw->m;
//	for(k=0; k<col2/step; k++)
//	{
//	    for (i=0; i< sp->rawt->row; i++)
//	    {
//	    	for(j=k*step; j<(k+1)*step;j++)
//	    	{
//	        	v=sp->rawt->m+i*sp->rawt->col+j;
//	        	*p++=*v;
//	    	}
//	    }
//	}
//	r=sp->rawt->col-col2;
//	h=0;
//	while(r>=4)
//	{
//		r=r-4;
//	    for (i=0; i< sp->rawt->row; i++)
//	    {
//	    	for(j=0; j<4;j++)
//	    	{
//	        	v=sp->rawt->m+i*sp->rawt->col+k*step+h*4+j;
//	        	*p++=*v;
//	    	}
//	    }
//	    h++;
//	}
//
//	for(d=0; d<r; d++)
//	{
//	    for (i=0; i< sp->rawt->row; i++)
//	    {
//	        v=sp->rawt->m+i*sp->rawt->col+k*step+h*4+d;
//	        *p++=*v;
//	    }
//	}
//	sp->raw->row=sp->rawt->row;
//	sp->raw->col=sp->rawt->col;
//}

void qtk_linearmask_component_generate(qtk_linear_mask_component_t* sp)
{
	float *p=sp->maskt->m;
	int i,j;
	int *b;

	sp->block = (int*)wtk_malloc(sizeof(int)*sp->row*sp->block_col);
	b = sp->block;
	//wtk_debug("%d %d\n",12*sp->row*sp->block_col,src->row*src->col);
	int num1=0;
	for(i = 0; i < sp->row*sp->block_col;i++)
	{
		*b = 0;
		for(j = 0; j < sp->block_size; j++)
		{
			if(*p != 0)
			{
				*b = 1;
			}
			p++;
		}
		if(*b!=0)
		{
			num1++;
		}
		b++;
	}

	sp->f = (float*)wtk_malloc(sizeof(float)*num1*sp->block_size);
	sp->num_block = num1;
	float* f = sp->f;
	p = sp->rawt->m;

	for(i = 0; i < sp->row*sp->block_col;i++)
	{
		if(*(sp->block+i)==1)
		{
			memcpy(f,p,sizeof(float)*sp->block_size);
			f = f + sp->block_size;
		}
		p = p + sp->block_size;
	}

	sp->block2 = (int*)wtk_malloc(sizeof(int)*num1);
	sp->in = (int*)wtk_malloc(sizeof(int)*num1);
	b = sp->block;
	num1 = 0;
	//int num2 = 0;
	//wtk_debug("%d\n",sp->row*sp->block_col);
	for(i = 0; i < sp->row ;i++)
	{
		for(j = 0;j < sp->block_col;j++)
		{
			if(*b==1)
			{
				*(sp->block2 + num1) = j;
				*(sp->in + num1) = i;
				//wtk_debug("%d %d\n",*(sp->block2 + num1),*(sp->in + num1));
				num1++;
			}
			b++;
		}
	}
}

int qtk_namask_component_read2(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed,int use_custom_acc)
{

	int ret = 0;
	int row,col;
	qtk_natrual_gradient_affine_mask_component_t* sp;
	sp = (qtk_natrual_gradient_affine_mask_component_t*) com->component;

	//ret=wtk_source_seek_to_s(src,"<NaturalGradientAffineMaskComponent>");
    ret=wtk_source_seek_to_s(src,"<LinearParams>");
	if(ret!=0)
	{
		wtk_debug("expected <LinearParams> failed\n");
		goto end;
	}
    ret = wtk_source_read_string(src, buf);//FM
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &row, 1, 1);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &col, 1, 1);
    sp->raw = qtk_blas_matrix_new(row,col);
    sp->rawt = qtk_blas_matrix_new(col,row);
    ret = wtk_source_read_float_little(src, sp->raw->m, row * col, 1);
    qtk_blas_matrix_trans(sp->raw,sp->rawt);

    ret=wtk_source_seek_to_s(src,"<LinearParamsMask>");
	if(ret!=0)
	{
		wtk_debug("expected <LinearParamsMask> failed\n");
		goto end;
	}
    ret = wtk_source_read_string(src, buf);//FM
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &row, 1, 1);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &col, 1, 1);
    sp->mask = qtk_blas_matrix_new(row,col);
    sp->maskt = qtk_blas_matrix_new(col,row);
    ret = wtk_source_read_float_little(src, sp->mask->m, row * col, 1);
    qtk_blas_matrix_trans(sp->mask,sp->maskt);

    ret=wtk_source_seek_to_s(src,"<BlockDim>");
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &col, 1, 1);
    sp->block_size = col;
    sp->row = sp->rawt->row;
    sp->block_col = sp->rawt->col/sp->block_size;
	//wtk_debug("%d %d %d\n",sp->block_size,sp->maskt->row,sp->maskt->col);
    ret=wtk_source_seek_to_s(src,"<BiasParams>");
	if(ret!=0)
	{
		wtk_debug("expected <BiasParams> failed\n");
		goto end;
	}
    ret = wtk_source_read_string(src, buf);//FV

    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &col, 1, 1);
    sp->bias = qtk_blas_matrix_new(1, col);
    ret = wtk_source_read_float_little(src, sp->bias->m, 1 * col, 1);

    ret=wtk_source_seek_to_s(src,"</NaturalGradientAffineComponent>");
    sp->block=NULL;
    sp->block2=NULL;
    sp->in = NULL;
    sp->f =NULL;
    sp->ncblock=NULL;
    if (use_custom_acc) {
                qtk_namask_component_generate_custom(sp);
    } else {
                qtk_namask_component_generate(sp);
    }
        end:
	return ret;

}

int qtk_namask_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed)
{
	return qtk_namask_component_read2(com, src, buf, max_w, is_fixed, 0);
}

void qtk_namask_component_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
        qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info)
{
	qtk_natrual_gradient_affine_mask_component_t *linear = (qtk_natrual_gradient_affine_mask_component_t*)com1->component;

    float *p1,*p2,*p3;
    int row,col,col2,col3,block_size;
    int i,j,m,n;
    int *block,*index;
    qtk_blas_matrix_t *b = linear->bias;

    qtk_sub_matrix_t src, dst;
    //qtk_sub_matrix_t* qtk_sub_matrix_new(float *f,int row_offset,int num_rows,int col_offset,int num_cols,int stride)
    qtk_sub_matrix_init(&src, input->m, input_info->row_offset,
                        input_info->num_rows, input_info->col_offset,
                        input_info->num_cols, input->col);
    qtk_sub_matrix_init(&dst, out_put->m, out_put_info->row_offset,
                        out_put_info->num_rows, out_put_info->col_offset,
                        out_put_info->num_cols, out_put->col);

    p1 = src.f;
    row = src.row;
    col = src.stride;
    //row2 = linear->row;
    col2 = linear->num_block;//linear->block_col;
    col3 = dst.stride;
    block_size = linear->block_size;

    for(i = 0; i < row; i++)
    {
        p2 = linear->f;
    	block = linear->block2;
    	//p1 = src->f + i*src->stride;
    	index = linear->in;
		//p3 = dst->f + col3*i;
		for(m = 0; m < col2; m++)//process block
		{
                        p3 = dst.f + col3 * i + block_size * (*block);
                        p1 = src.f + i * col + *index;
                        for(n = 0; n < block_size; n++)
			{
				*p3 += (*p1)*(*p2);
				p2++;
				p3++;
			}
			block++;
			index++;
    	}
    }

    for (i = 0; i < dst.row; i++) {
        for (j = 0; j < dst.col; j++) {
                        *(dst.f + i * dst.stride + j) += *(b->m + j);
        }
    }
}

void qtk_namask_component_propagate_custom(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
        qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info)
{
	qtk_natrual_gradient_affine_mask_component_t *linear = (qtk_natrual_gradient_affine_mask_component_t*)com1->component;

    float *p1,*p2,*bp2, *p3, *bp3, *bp1;
    int row,col,col2,col3,block_size;
    int i,j,m,n, bsn,step;
    int *block,*bblock, *index, *bindex;
    qtk_blas_matrix_t *b = linear->bias;

    //qtk_sub_matrix_t *src,*dst;
    //qtk_sub_matrix_t* qtk_sub_matrix_new(float *f,int row_offset,int num_rows,int col_offset,int num_cols,int stride)
    float *src, *dst;
    src=input->m +input_info->col_offset+input_info->row_offset*input->col;
    dst = out_put->m + out_put_info->col_offset + out_put_info->row_offset * out_put->col;
    p1 = src;
    row = input_info->num_rows;
    col = input->col;
    //row2 = linear->row;
    col2 = linear->num_block;//linear->block_col;
    col3 = out_put->col;
    block_size = linear->block_size;
    bsn = linear->block_size >> 3;
    bp2 = linear->f;
    bblock = linear->block2;
    bindex = linear->in;
    step = 1>>3;
    bp3=dst;
    bp1=src;
    for(i = 0; i < row; i++)
    {
        p2 = bp2;
    	block = bblock;
    	//p1 = src->f + i*src->stride;
    	index = bindex;
		//p3 = dst->f + col3*i;
    	//bp3 +=  col3*i;
    	//bp1 = src + i*col;
		for(m = 0; m < col2; m++)//process block
		{
			p3 = bp3+block_size*(*block);
			p1 = bp1 + *index;
			n = bsn;
			while(n--)
			{
				*p3 += (*p1) * (*p2);
				*(p3+1) += (*p1) * (*(p2 + 1));
				*(p3+2) += (*p1) * (*(p2 + 2));
				*(p3+3) += (*p1) * (*(p2 + 3));
				*(p3+4) += (*p1) * (*(p2 + 4));
				*(p3+5) += (*p1) * (*(p2 + 5));
				*(p3+6) += (*p1) * (*(p2 + 6));
				*(p3+7) += (*p1) * (*(p2 + 7));
				p2+=step;
				p3+=step;
			}
			n=bsn*step;
			while(n++ < block_size)
			{
				*p3 += (*p1)*(*p2);
				p2++;
				p3++;
			}
			block++;
			index++;
    	}
		bp3 += col3;
		bp1 += col;
    }

	for (i = 0; i < out_put_info->num_rows; i++)
	{
		for (j = 0; j < out_put_info->num_cols; j++)
		{
			*(dst + i* out_put->col + j) += *(b->m + j);
		}
	}
}

#ifdef USE_AVX
/**
 * only for block_size=8.
 * equal to qtk_namask_component_propagate
 */
void qtk_namask_component_propagate_avx(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
        qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info)
{
	qtk_natrual_gradient_affine_mask_component_t *linear = (qtk_natrual_gradient_affine_mask_component_t*)com1->component;

    float *p1,*p2,*p3, *bp3, *bp1;
    int row,col,col2,col3,block_size;
    int i,j,m; //bsn,n;
    int *block,*index;
    qtk_blas_matrix_t *b = linear->bias;
	__m256 x, y; //, yy;
    //qtk_sub_matrix_t *src,*dst;
    //qtk_sub_matrix_t* qtk_sub_matrix_new(float *f,int row_offset,int num_rows,int col_offset,int num_cols,int stride)
    float *src, *dst;
    src=input->m +input_info->col_offset+input_info->row_offset*input->col;
    dst = out_put->m + out_put_info->col_offset + out_put_info->row_offset * out_put->col;
    p1 = src;
    row = input_info->num_rows;
    col = input->col;
    col2 = linear->num_block;//linear->block_col;
    col3 = out_put->col;
    block_size = linear->block_size;
    //bsn = block_size >> 3;
    for(i = 0; i < row; i++)
    {
        p2 = linear->f;
    	block = linear->block2;
    	//p1 = src->f + i*src->stride;
    	index = linear->in;
		//p3 = dst->f + col3*i;
    	bp3 = dst + col3*i;
    	bp1 = src + i*col;
		for(m = 0; m < col2; m++)//process block
		{
			p3 = bp3+block_size*(*block);
			p1 = bp1 + *index;
//			n = bsn;
			x = _mm256_set1_ps(*p1);
//			while(n--)
//			{
				y=_mm256_loadu_ps(p3);
				y = _mm256_add_ps(y, _mm256_mul_ps(x, _mm256_loadu_ps(p2)));
				//yy = _mm256_add_ps(yy, _mm256_mul_ps(x, _mm256_loadu_ps(p2+8)));
				_mm256_storeu_ps(p3, y);
				p2 += 8;
				p3 += 8;
//			}
//			n=bsn*8;
//			while(n++ < block_size)
//			{
				*p3 += (*p1)*(*p2);
				*(p3+1) += (*p1)*(*(p2+1));
				p2+=2;
//				p3++;
//			}
			block++;
			index++;
    	}
    }

	for (i = 0; i < out_put_info->num_rows; i++)
	{
		for (j = 0; j < out_put_info->num_cols; j++)
		{
			*(dst + i* out_put->col + j) += *(b->m + j);
		}
	}
}

/**
 * add by dmd
 * only for 8 bytes.equal to qtk_namask_component_propagate_custom
 */
void qtk_namask_component_propagate_avxcustom(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
        qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info)
{
	qtk_natrual_gradient_affine_mask_component_t *linear = (qtk_natrual_gradient_affine_mask_component_t*)com1->component;

    float *p1,*p2,*bp2,*p3, *bp3, *bp1;
    int row,col,col2,col3,block_size;
    int i,j,m,n;
    int *block,*bblock, *index,*bindex, *ncolb, *bncolb;
    qtk_blas_matrix_t *b = linear->bias;
	__m256 x, y; //, yy;
    float *src, *dst;
    src=input->m +input_info->col_offset+input_info->row_offset*input->col;
    dst=out_put->m + out_put_info->col_offset + out_put_info->row_offset * out_put->col;
    p1 = src;
    row = input_info->num_rows;
    col = input->col;
    col2 = linear->num_block;//linear->block_col;
    col3 = out_put->col;
    block_size = linear->block_size;
    bp2 = linear->f;
    bblock = linear->block2;
    bindex = linear->in;
    bncolb = linear->ncblock;
    bp3 = dst;
    bp1 = src;
    for(i = 0; i < row; i++)
    {
        p2 = bp2;
    	block = bblock;
    	//p1 = src->f + i*src->stride;
    	index = bindex;
    	ncolb = bncolb;
		//p3 = dst->f + col3*i;
    	//bp3 = dst + col3*i;
    	//bp1 = src + i*col;
		//for(m = 0; m < col2;)//process block
		m=col2;
		while(m)
		{
			p3 = bp3+block_size*(*block);
			//wtk_debug("ncolb=%d block=%p %d\n", *ncolb, block, *block);
			//wtk_debug("%p %f ncolb=%d block=%d\n", p3, *p3, *ncolb, *block);
			y = _mm256_loadu_ps(p3);
			n=*ncolb;
			block+=n;
			m-=n;
			ncolb++;
			while(n)
			{
				//wtk_debug("index=%d %p %p n=%d\n", *index, index, linear->in, n);
				p1 = bp1 + *index;
				x = _mm256_set1_ps(*p1);
				y = _mm256_add_ps(y, _mm256_mul_ps(x, _mm256_loadu_ps(p2)));
				_mm256_storeu_ps(p3, y);
				p2 += 8;
				index++;
				n--;
			}
    	}
		bp3 += col3;
		bp1 += col;
    }

    bp1=b->m;
	for (i = 0; i < out_put_info->num_rows; i++)
	{
		for (j = 0; j < out_put_info->num_cols; j++)
		{
			//*(dst + i* out_put->col + j) += *(b->m + j);
			*(dst + i* out_put->col + j) += *(bp1 + j);
		}
	}
}

// for qtk_linearmask_component_generate3().
//void qtk_namask_component_propagate_avx3(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
//        qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info)
//{
//	qtk_natrual_gradient_affine_mask_component_t *linear = (qtk_natrual_gradient_affine_mask_component_t*)com1->component;
//
//    //float *p1,*p2,*p3, *bp3, *bp1;
//    //int row,row2,col,col2,col3,block_size;
//    //int i,j,m,n, bsn;
//    //int *block,*index, *ncolb;
//    //qtk_blas_matrix_t *b = linear->bias;
//	//__m256 x, y; //, yy;
//    //qtk_sub_matrix_t *src,*dst;
//    //qtk_sub_matrix_t* qtk_sub_matrix_new(float *f,int row_offset,int num_rows,int col_offset,int num_cols,int stride)
//    //float *src, *dst;
//    //src=input->m +input_info->col_offset+input_info->row_offset*input->col;
//    //dst=out_put->m + out_put_info->col_offset + out_put_info->row_offset * out_put->col;
//	qtk_affine_global_component_t* com;
//	int col, row, i, j, row_offset, col_offset;
//	qtk_blas_matrix_t *m;
//	qtk_blas_matrix_t *b;
//	int k, step;
//
//	step = 1 << 3;
//	m = linear->raw;
//	b = linear->bias;
//	col = out_put_info->num_cols;
//	row = out_put_info->num_rows;
//	col_offset = out_put_info->col_offset;
//	row_offset = out_put_info->row_offset;
//
//
//	float *p1, *p2, *p3,*bp1, *bp3;
//	int col2, col3, row2, row3, icol;
//	int n,r,d, ns;
//	__m128 x1,y1;
//	__m256 x, y; //, yy;
//    int porder=3;
//	if(porder != 3)
//	{
//		wtk_debug("error: porder should set 3[porder=%d]\n", porder);
//		return ;
//	}
//	col2 = m->col;
//	col3 = (col2 >> porder) << porder;
//	ns = col3 >> porder;
//	row2 = m->row;
//	row3 = (row2 >> porder) << porder;
//	icol = input->col;
//	bp3 = out_put->m + row_offset * out_put->col + col_offset;
//	p2=m->m;
//	//wtk_debug("input->row=%d input->col=%d output->row=%d output->col=%d row=%d col=%d col2=%d col3=%d\n", input->row, input->col, out_put->row, out_put->col, row, col, col2, col3);
//	//wtk_debug("row=%d,col=%d,row_input=%d,col_input=%d,out_col=%d,out_row=%d,row_win=%d,col_win=%d\n",row,col,input->row,input->col,out_put->col,out_put->row,m->row,m->col);
//	for (i = 0; i < row; i++)
//	{
//		p3 = bp3 + i * out_put->col;
//		bp1 = input->m + (input_info->row_offset+i) * icol + input_info->col_offset;
//		for(k=0; k<ns; k++)
//		{
//			p1 = bp1;
//			n=row2;
//			y=_mm256_loadu_ps(p3);
//			//yy=_mm256_loadu_ps(p3+8);
//			while(n--)
//			{
//				x = _mm256_set1_ps(*p1);
//				y = _mm256_add_ps(y, _mm256_mul_ps(x, _mm256_loadu_ps(p2)));
//				//yy = _mm256_add_ps(yy, _mm256_mul_ps(x, _mm256_loadu_ps(p2+8)));
//
//				p1++;
//				p2 += step;
//			}
//			_mm256_storeu_ps(p3, y);
//			//_mm256_storeu_ps(p3+8, yy);
//			//printf("%f\n", *p3);
//			//printf("%f\n", *(p3+8));
//			p3 += step;
//		}
//		//exit(0);
//		r=col2-col3;
//		while(r>=4)
//		{
//			r=r-4;
//			p1 = bp1;
//			n=row2;
//			y1=_mm_loadu_ps(p3);
//			while(n--)
//			{
//				x1=_mm_set1_ps(*p1);
//				y1 = _mm_add_ps(y1, _mm_mul_ps(x1, _mm_loadu_ps(p2)));
//				p1++;
//				p2 += 4;
//			}
//			_mm_storeu_ps(p3, y1);
//			p3 += 4;
//		}
//		for(d=0; d<r; d++)
//		{
//			p1 = bp1;
//			for(n=0; n<row3; n+=8)
//			{
//				//*p3 += (*p1) * (*p2) + (*(p1 + 1)) * (*(p2 + 1))
//				//						+ (*(p1 + 2)) * (*(p2 + 2)) + (*(p1 + 3)) * (*(p2 + 3));
//				x=_mm256_dp_ps(_mm256_loadu_ps(p1),_mm256_loadu_ps(p2),0xff);
//				*p3 += x[0]+x[4];
//
//				p1 += 8;
//				p2 += 8;
//			}
//			for(; n<row2;n++)
//			{
//				//p3[0] +=p1[0] * p2[0];
//				*p3 += (*p1) * (*p2);
//				p1++;
//				p2++;
//			}
//			p3++;
//		}
//	}
//	//wtk_debug("%d %d %d %d %d %d\n",input->row,input->col,m->row,m->col,out_put->row,out_put->col);
//	//wtk_debug("row=%d,col=%d,row2=%d,col2=%di,offset=%d,%d\n",m->row,m->col,out_put->row,out_put->col,row_offset,col_offset);
////t = time_get_ms()-t;
////wtk_debug("ffffffffffff  t=%f\n",t);
//	//print_float(out_put->m,20);
//	//qtk_blas_matrix_print(b);
//	//exit(0);
//	//wtk_debug("com->type=%d\n", com->type);
//		for (i = 0; i < row; i++)
//		{
//			for (j = 0; j < col; j++)
//			{
//				*(out_put->m + (i + row_offset) * out_put->col + j + col_offset) +=
//					*(b->m + j);
//			}
//		}
//}
#endif

void qtk_namask_component_delete(nnet3_component_t* com,unsigned int is_fixed)
{
	qtk_natrual_gradient_affine_mask_component_t* sp;
	sp = (qtk_natrual_gradient_affine_mask_component_t*) com->component;
	qtk_blas_matrix_delete(sp->raw);
	qtk_blas_matrix_delete(sp->rawt);
	qtk_blas_matrix_delete(sp->mask);
	qtk_blas_matrix_delete(sp->maskt);
	qtk_blas_matrix_delete(sp->bias);
	if(sp->block)wtk_free(sp->block);
	if(sp->block2)wtk_free(sp->block2);
	if(sp->f)wtk_free(sp->f);
	if(sp->in)wtk_free(sp->in);
	if(sp->ncblock)wtk_free(sp->ncblock);
}

int qtk_linearmask_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed)
{

	int ret = 0;
	int row,col;
	qtk_linear_mask_component_t* sp;
	sp = (qtk_linear_mask_component_t*) com->component;

	//ret=wtk_source_seek_to_s(src,"<NaturalGradientAffineMaskComponent>");
    ret=wtk_source_seek_to_s(src,"<Params>");
	if(ret!=0)
	{
		wtk_debug("expected <Params> failed\n");
		goto end;
	}
    ret = wtk_source_read_string(src, buf);//FM
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &row, 1, 1);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &col, 1, 1);
    sp->raw = qtk_blas_matrix_new(row,col);
    sp->rawt = qtk_blas_matrix_new(col,row);
    ret = wtk_source_read_float_little(src, sp->raw->m, row * col, 1);
    qtk_blas_matrix_trans(sp->raw,sp->rawt);

    ret=wtk_source_seek_to_s(src,"<Mask>");
	if(ret!=0)
	{
		wtk_debug("expected <Mask> failed\n");
		goto end;
	}
    ret = wtk_source_read_string(src, buf);//FM
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &row, 1, 1);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &col, 1, 1);
    sp->mask = qtk_blas_matrix_new(row,col);
    sp->maskt = qtk_blas_matrix_new(col,row);
    ret = wtk_source_read_float_little(src, sp->mask->m, row * col, 1);
    qtk_blas_matrix_trans(sp->mask,sp->maskt);

    ret=wtk_source_seek_to_s(src,"<BlockDim>");
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &col, 1, 1);
    sp->block_size = col;
    sp->row = sp->rawt->row;
    sp->block_col = sp->rawt->col/sp->block_size;

	//wtk_debug("%d %d %d\n",sp->block_size,sp->maskt->row,sp->maskt->col);
    ret=wtk_source_seek_to_s(src,"</LinearComponent>");
    qtk_linearmask_component_generate(sp);
	end:
	return ret;

}

void qtk_linearmask_component_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
        qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info)
{
	qtk_linear_mask_component_t *linear = (qtk_linear_mask_component_t*)com1->component;

    float *p1,*p2,*p3;
    int row,col,col2,col3,block_size; //,row2;
    int i,m,n;
    int *block,*index;

    qtk_sub_matrix_t src, dst;
    //qtk_sub_matrix_t* qtk_sub_matrix_new(float *f,int row_offset,int num_rows,int col_offset,int num_cols,int stride)
    qtk_sub_matrix_init(&src, input->m, input_info->row_offset,
                        input_info->num_rows, input_info->col_offset,
                        input_info->num_cols, input->col);
    qtk_sub_matrix_init(&dst, out_put->m, out_put_info->row_offset,
                        out_put_info->num_rows, out_put_info->col_offset,
                        out_put_info->num_cols, out_put->col);

    p1 = src.f;
    row = src.row;
    col = src.stride;
    //row2 = linear->row;
    col2 = linear->num_block;//linear->block_col;
    col3 = dst.stride;
    block_size = linear->block_size;

    for(i = 0; i < row; i++)
    {
        p2 = linear->f;
    	block = linear->block2;
    	//p1 = src->f + i*src->stride;
    	index = linear->in;
		//p3 = dst->f + col3*i;
		for(m = 0; m < col2; m++)//process block
		{
                        p3 = dst.f + col3 * i + block_size * (*block);
                        p1 = src.f + i * col + *index;
                        for(n = 0; n < block_size; n++)
			{
				*p3 += (*p1)*(*p2);
				p2++;
				p3++;
			}
			block++;
			index++;
    	}
    }
}

void qtk_linearmask_component_delete(nnet3_component_t* com,unsigned int is_fixed)
{
	qtk_linear_mask_component_t* sp;
	sp = (qtk_linear_mask_component_t*) com->component;
	qtk_blas_matrix_delete(sp->raw);
	qtk_blas_matrix_delete(sp->rawt);
	qtk_blas_matrix_delete(sp->mask);
	qtk_blas_matrix_delete(sp->maskt);
	wtk_free(sp->block);
	wtk_free(sp->block2);
	wtk_free(sp->f);
	wtk_free(sp->in);
}

int qtk_permute_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed)
{
	int ret = 0;
	int row;
	qtk_permute_component_t* p;
	p = (qtk_permute_component_t*) com->component;

    ret=wtk_source_seek_to_s(src,"<ColumnMap>");
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &row, 1, 1);
	p->dim = row;
	p->column_map = (int*)wtk_malloc(sizeof(int)*row);
    ret = wtk_source_read_int_little(src, p->column_map, row, 1);

    ret=wtk_source_seek_to_s(src,"</PermuteComponent>");
	return ret;
}

void qtk_permute_component_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
        qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info)
{
	qtk_permute_component_t* p;
	p = (qtk_permute_component_t*) com1->component;
	int i,j;
	int row = out_put_info->num_rows;
	int col = out_put_info->num_cols;
	int *index;
	for(i = 0;i < row; i++)
	{
		index = p->column_map;
		for(j = 0; j < col; j++,index++)
		{
			if(*index < 0)
			{
				*(out_put->m + (i+out_put_info->row_offset)*out_put->col + out_put_info->col_offset + j) = 0.0;
			}else
			{
				*(out_put->m + (i+out_put_info->row_offset)*out_put->col + out_put_info->col_offset + j) = *(input->m +input_info->col_offset+(i+input_info->row_offset)*input->col+ (*index));
			}
		}
	}
	
}

void qtk_permute_component_delete(nnet3_component_t* com,unsigned int is_fixed)
{
	qtk_permute_component_t* p;
	p = (qtk_permute_component_t*) com->component;
	wtk_free(p->column_map);
}

int qtk_blockweight_component_read(nnet3_component_t *com, wtk_source_t *src,
                                   wtk_strbuf_t *buf, float max_w,
                                   unsigned int is_fixed) {
    int ret = 0;
    int block;
    qtk_blockweight_component_t *p;
    p = (qtk_blockweight_component_t *)com->component;

    ret = wtk_source_seek_to_s(src, "<BlockDim>");
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &block, 1, 1);
    p->BlockDim = block;

    ret = wtk_source_seek_to_s(src, "<NumBlocks>");
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &block, 1, 1);
    p->NumBlocks = block;
    ret = wtk_source_seek_to_s(src, "</BlockWeightComponent>");

    return ret;
}

void qtk_blockweight_component_propagate(nnet3_component_t *com,
                                         qtk_blas_matrix_t *dst,
                                         qtk_blas_matrix_t *src,
                                         qtk_nnet3_submatrix_info_t *dst_info,
                                         qtk_nnet3_submatrix_info_t *src_info) {
    int per_block_dim, i, j, k, out_row, out_col;
    float *in, *out;
    qtk_blockweight_component_t *bw =
        (qtk_blockweight_component_t *)com->component;
    per_block_dim = bw->BlockDim / bw->NumBlocks;
    in = src->m + src_info->row_offset * src->col + src_info->col_offset;
    out = dst->m + dst_info->row_offset * dst->col + dst_info->col_offset;
    out_row = dst_info->num_rows;
    out_col = dst_info->num_cols;

    for (i = 0; i < bw->NumBlocks; i++) {
        for (j = 0; j < out_row; j++) {
            for (k = 0; k < out_col; k++) {
                *(out + j * dst->col + k) +=
                    *(in + bw->BlockDim + i + j * src->col) *
                    *(in + k + j * src->col + i * per_block_dim);
            }
        }
    }
}

int qtk_fsmn_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed)
{
	int ret = 0;
	int row,col,i;
	qtk_compactfsmn_component_t* p;
	p = (qtk_compactfsmn_component_t*) com->component;

    ret=wtk_source_seek_to_s(src,"<LOrder>");
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &row, 1, 1);
	p->l_order = row;

    ret=wtk_source_seek_to_s(src,"<ROrder>");
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &row, 1, 1);
	p->r_order = row;

    ret=wtk_source_seek_to_s(src,"<LStride>");
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &row, 1, 1);
	p->l_stride = row;

    ret=wtk_source_seek_to_s(src,"<RStride>");
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &row, 1, 1);
	p->r_stride = row;

    ret=wtk_source_seek_to_s(src,"<Params>");
    ret = wtk_source_read_string(src, buf);//FM
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &row, 1, 1);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &col, 1, 1);
    p->params = qtk_blas_matrix_new(row,col);
    ret = wtk_source_read_float_little(src, p->params->m, row * col, 1);

    ret = wtk_source_read_string(src, buf);
    // wtk_debug("[%.*s]\n",buf->pos,buf->data);
    if (wtk_str_equal_s(buf->data, buf->pos, "<BiasParams>")) {
        ret = wtk_source_read_string(src, buf); // FM
        ret = wtk_source_read_char(src);
        ret = wtk_source_read_char(src);
        ret = wtk_source_read_int_little(src, &row, 1, 1);
        p->BiasParams = qtk_blas_matrix_new(1, row);
        ret = wtk_source_read_float_little(src, p->BiasParams->m, row, 1);
        ret = wtk_source_seek_to_s(src, "</CompactFsmnComponent>");
    } else {
        p->BiasParams = NULL;
        ret = wtk_source_seek_to_s(src, "</CompactFsmnComponent>");
    }

    p->time_offset = (int*)wtk_malloc(sizeof(int)*(2+p->r_order+p->l_order));
    p->time_offset[0] = 1+p->r_order+p->l_order;
    p->time_offset[1] = 0;
    for(i = 1;i <= p->l_order; i++)
    {
    	p->time_offset[1+i] = -(i * p->l_stride);
    }

    for(i = 1;i <= p->r_order; i++)
    {
    	p->time_offset[1+p->l_order+i] = i * p->r_stride;
    }

    p->in = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
    p->out = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
    p->in_part = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
    p->out_part = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));

	return ret;
}

void qtk_fsmn_component_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *dst,
		qtk_blas_matrix_t *src, qtk_nnet3_submatrix_info_t *dst_info,
		qtk_nnet3_submatrix_info_t *src_info,qtk_nnet3_precomputed_indexes_t* index)
{
	//wtk_debug("fsmn compute\n");
	qtk_sub_matrix_t *in,*out,*in_part,*out_part;
	qtk_nnet3_fsmn_precomputed_indexes_t *fsmn_index = (qtk_nnet3_fsmn_precomputed_indexes_t*)index->index;
	qtk_compactfsmn_component_t *fsmn = (qtk_compactfsmn_component_t*)com1->component;
	int offset = fsmn_index->row_offsets[1];
	int i,m,n;
	float *v;

	in = fsmn->in;
	out = fsmn->out;
	in_part = fsmn->in_part;
	out_part = fsmn->out_part;

	qtk_sub_matrix_set2(in,src->m,src_info->num_rows,src_info->num_cols,src->col);
	qtk_sub_matrix_set2(out,dst->m,dst_info->num_rows,dst_info->num_cols,dst->col);

	qtk_sub_matrix_set2(out_part,in->f + in->stride*offset,out->row,
				in->col,in->stride*fsmn_index->row_stride);

	for(m = 0;m < out->row; m++)
	{
		for(n = 0;n < out->col; n++)
		{
                    *(out->f + out->stride * m + n) =
                        *(out_part->f + out_part->stride * m + n);
                }
        }
	//qtk_sub_matrix_print(out);
	//wtk_debug("=============\n")
	for(i = 0; i < fsmn_index->row_offsets[0]; i++)
	{
		offset = fsmn_index->row_offsets[i+1];
		qtk_sub_matrix_set2(in_part,in->f + in->stride*offset,out->row,
				in->col,in->stride*fsmn_index->row_stride);
		v = fsmn->params->m + fsmn->params->col*i;
		//qtk_sub_matrix_print(in_part);
		for(m = 0; m < out->row; m++)
		{
			for(n = 0; n < out->col; n++)
			{
				*(out->f + m*out->stride + n) += v[n] * (*(in_part->f + in_part->stride*m + n));
			}
		}
        }
        if (fsmn->BiasParams) {
            for (m = 0; m < out->row; m++) {
                for (n = 0; n < out->col; n++) {
                    *(out->f + out->stride * m + n) +=
                        *(fsmn->BiasParams->m + n);
                }
            }
        }
}

void qtk_fsmn_component_delete(nnet3_component_t* com,unsigned int is_fixed)
{
	qtk_compactfsmn_component_t* p;
	p = (qtk_compactfsmn_component_t*) com->component;
	wtk_free(p->time_offset);
	wtk_free(p->in);
	wtk_free(p->out);
	wtk_free(p->in_part);
	wtk_free(p->out_part);
	qtk_blas_matrix_delete(p->params);
        if (p->BiasParams) {
            qtk_blas_matrix_delete(p->BiasParams);
        }
}
