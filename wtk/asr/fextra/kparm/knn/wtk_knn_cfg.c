#include "wtk_knn_cfg.h" 
wtk_fixvec_t* wtk_fixvec_read_fix(wtk_source_t *src);
wtk_knn_ab_t* wtk_knn_ab_read_fix_short(wtk_source_t *src);
void wtk_batch_norm_read_fix_short(wtk_batch_norm_t *norm,wtk_source_t *src);
wtk_fixvecs_t* wtk_fixvecs_read_fix(wtk_source_t *src);
void wtk_batch_norm_read_fix_char(wtk_batch_norm_t *norm,wtk_source_t *src);
wtk_knn_ab_t* wtk_knn_ab_read_fix_char(wtk_source_t *src);
void wtk_fixmatc_write_fix(wtk_fixmatc_t *fix,FILE *f);
void wtk_fixvecc_write_fix(wtk_fixvecc_t *fix,FILE *f);
void wtk_fixvecs_write_fix(wtk_fixvecs_t *fix,FILE *f);
wtk_fixvecc_t* wtk_fixvecc_read_fix(wtk_source_t *src);
wtk_fixmats_t* wtk_fixmats_read_fix(wtk_source_t *src);
wtk_knn_conv_t* wtk_knn_conv_new();
void wtk_knn_conv_input2inputpatches(wtk_knn_conv_t *conv, wtk_vecf_t * input, wtk_matf_t* patches);
void wtk_knn_conv_input2inputpatches_short(wtk_knn_conv_t *conv, wtk_vecs2_t * input, wtk_mats_t* patches);
void wtk_knn_conv_input2inputpatches_int(wtk_knn_conv_t *conv, wtk_veci_t * input, wtk_mati_t* patches);
int* wtk_knn_conv_get_input2inputpatches_map(wtk_knn_conv_t *conv, int patches_row,int patches_col);
int wtk_knn_cfg_xvec_load(wtk_knn_cfg_t *cfg,wtk_source_t *src);
void wtk_knn_cfg_xvec_delete(wtk_knn_cfg_t *cfg);

typedef struct
{
	wtk_string_t name;
	wtk_knn_layer_type_t type;
}wtk_knn_type_map_t;

wtk_knn_type_map_t map[]={
		{wtk_string("LinearMaskComponent"),WTK_LinearMaskComponent},
		{wtk_string("LinearComponent"),WTK_LinearComponent},
		{wtk_string("AffineComponent"),WTK_AffineComponent},
		{wtk_string("FixedAffineComponent"),WTK_FixedAffineComponent},
		{wtk_string("NaturalGradientAffineComponent"),WTK_NaturalGradientAffineComponent},
		{wtk_string("NaturalGradientAffineMaskComponent"),WTK_NaturalGradientAffineMaskComponent},
		{wtk_string("RectifiedLinearComponent"),WTK_RectifiedLinearComponent},
		{wtk_string("BatchNormComponent"),WTK_BatchNormComponent},
		{wtk_string("NormalizeComponent"),WTK_NormalizeComponent},
		{wtk_string("LogSoftmaxComponent"),WTK_LogSoftmaxComponent},
		{wtk_string("SigmoidComponent"),WTK_SigmoidComponent},
		{wtk_string("ConvolutionPadComponent"),WTK_ConvolutionComponent},
		{wtk_string("LstmNonlinearityComponent"),WTK_LstmNonlinearityComponent},
		{wtk_string("BackpropTruncationComponent"),WTK_BackpropTruncationComponent},
		{wtk_string("ScaleAndOffsetComponent"),WTK_ScaleAndOffsetComponent},
		{wtk_string("NoOpComponent"),WTK_NoOpComponent},
		{wtk_string("GeneralDropoutComponent"),WTK_GeneralDropoutComponent},
		{wtk_string("CompactFsmnComponent"),WTK_CompactFsmnComponent},
		{wtk_string("StatisticsExtractionComponent"),WTK_StatisticsExtractionComponent},
		{wtk_string("StatisticsPoolingComponent"),WTK_StatisticsPoolingComponent},
		{{NULL,0},-1},
};



static wtk_knn_ab_t* wtk_knn_ab_new(void)
{
	wtk_knn_ab_t *ab;

	ab=(wtk_knn_ab_t*)wtk_malloc(sizeof(wtk_knn_ab_t));
	ab->a=NULL;
	ab->b=NULL;
	ab->fix_char=NULL;
	ab->fix_short=NULL;
	return ab;
}


static wtk_knn_ab_t* wtk_knn_ab_read(wtk_source_t *src)
{
	wtk_knn_ab_t *ab;

	ab=wtk_knn_ab_new();
	ab->a=wtk_matf_read(src);
	ab->b=wtk_vecf_read(src);
	return ab;
}

wtk_knn_conv_t* wtk_knn_conv_read(wtk_source_t *src,wtk_knn_cfg_t* cfg)
{
	int v;
	wtk_knn_conv_t *conv;
	conv=wtk_knn_conv_new();

	wtk_source_fill(src,(char*)&v,sizeof(int));
	conv->input_x_dim=v;
	wtk_source_fill(src,(char*)&v,sizeof(int));
	conv->input_y_dim=v;
	wtk_source_fill(src,(char*)&v,sizeof(int));
	conv->input_z_dim=v;	
	wtk_source_fill(src,(char*)&v,sizeof(int));
	conv->filter_x_dim=v;
	wtk_source_fill(src,(char*)&v,sizeof(int));
	conv->filter_y_dim=v;
	wtk_source_fill(src,(char*)&v,sizeof(int));
	conv->filter_x_step=v;
	wtk_source_fill(src,(char*)&v,sizeof(int));
	conv->filter_y_step=v;
	wtk_source_fill(src,(char*)&v,sizeof(int));
	conv->num_y_step=v;
	conv->height_offset=wtk_veci_read(src);
	int fdim;
	if(cfg->use_fixpoint)
	{	
		if(cfg->use_fixchar)
		{
			conv->ab=wtk_knn_ab_read_fix_char(src);
			fdim=conv->ab->fix_char->a->col;
		}else
		{
			conv->ab=wtk_knn_ab_read_fix_short(src);
			fdim=conv->ab->fix_short->a->col;
		}	
	}else
	{		
		conv->ab=wtk_knn_ab_read(src);
		fdim=conv->ab->a->col;
	}
	int num_x_steps = (1+(conv->input_x_dim-conv->filter_x_dim)/conv->filter_x_step);
	int num_y_steps = conv->num_y_step;
	int filter_dim = fdim;
	int nframe= 1;
	int* map;
	map=wtk_knn_conv_get_input2inputpatches_map(conv,nframe,num_x_steps*num_y_steps*filter_dim);
	conv->column_map = map;
	return conv;
}

void wtk_knn_conv_calc(wtk_knn_conv_t *conv, wtk_vecf_t *input,wtk_vecf_t *output)
{
	wtk_matf_t *filter =conv->ab->a;
	wtk_vecf_t *bias = conv->ab->b;
	int input_x_dim=conv->input_x_dim;
	int num_frames;
	//int input_y_dim=conv->input_y_dim;
	int filter_x_dim=conv->filter_x_dim;
//	int filter_y_dim=conv->filter_y_dim;
	int filter_x_step = conv->filter_x_step;
//	int filter_y_step = conv->filter_y_step;
	int num_x_steps = (1+(input_x_dim-filter_x_dim)/filter_x_step);
	int num_y_steps = conv->num_y_step;
//	int filter_dim = conv->ab->a->col;
	int filter_dim = conv->ab->a->col;
//	int num_filters = conv->ab->a->row;
	num_frames=1;
	wtk_matf_t * patches;
	//wtk_vecf_print(input);
	patches=wtk_matf_new(num_frames,num_x_steps*num_y_steps*filter_dim);
	wtk_matf_zero(patches);
	wtk_knn_conv_input2inputpatches(conv,input,patches);
	//wtk_matf_print(patches);

	int i,j;
	int input_col = num_frames*num_x_steps*num_y_steps;
	int input_row = filter_dim;
	int w_row = filter->row;
	int w_col = filter->col;
	//wtk_debug("irow=%d,icol=%d,wrow=%d,wcol=%d\n",input_row,input_col,w_row,w_col);
	int out_row = input_col;
	int out_col = w_row;
	//wtk_debug("input:%d/%d w:%d,%d output:%d,%d\n",filter_dim,num_frames*num_x_steps*num_y_steps,
	//w_row,w_col,out_row,out_col);
	float *in,*w,*out;
	int col = (w_col>>2)<<2;
	//wtk_debug("output-len=%d\n",output->len);
	wtk_vecf_zero(output);
	//int x=0;
	float *e,*e2;
	for(i=0;i<out_row;++i)
	{
		out = output->p + i*out_col;
		//wtk_debug("ori out=%f\n",*out);
		for(j=0;j<out_col;++j)
		{
			in = patches->p +i*input_row;
			w = filter->p + j*w_col;
			e = w + w_col;
			e2 = w + col;
			while(w<e2)
			{
				*out +=(*in)*(*w)+(*(in+1))*(*(w+1))+(*(in+2))*(*(w+2))+(*(in+3))*(*(w+3));
				//wtk_debug("%f * %f + %f * %f + %f * %f + %f * %f\n",*in,*w,*(in+1),*(w+1),*(in+2),*(w+2),*(in+3),*(w+3));
				in+=4;
				w+=4;
			}
			while(w<e)
			{
				*out += (*in) * (*w);
				//wtk_debug("%f * %f \n",*in,*w);
				in++;
				w++;
			}
			*out += bias->p[j];
			//wtk_debug("v[%d]/out=%f\n",x,*out);
			//x++;
			out++;
		}

	}
	// print_float(output->p,output->len);
	wtk_matf_delete(patches);
	// exit(0);
}

void wtk_knn_linear_mask_calc(wtk_knn_linear_mask_t *m,wtk_vecf_t *input,wtk_vecf_t *output)
{
	wtk_matf_t *a=m->a;
	int row=a->row;
	int col=a->col;
	int i,j;
	float f;
	float *pfa;
	float *pfi;
	float *pfo;

	//wtk_debug("input=%d output=%d m=%d/%d\n",input->len,output->len,a->row,a->col);
	pfa=a->p;
	pfi=input->p;
	pfo=output->p;
	for(i=0;i<row;++i)
	{
		f=0;
		for(j=0;j<col;++j)
		{
			f+=pfa[j]*pfi[j];
			//wtk_debug("v[%d]=%f/%f/%f\n",j,pfa[j],pfi[j],f);
		}
		//exit(0);
		pfo[i]=f;
		pfa+=col;
	}
	//exit(0);
}

void wtk_knn_linear_calc(wtk_knn_linear_t *m,wtk_vecf_t *input,wtk_vecf_t *output)
{
	wtk_matf_t *a=m->a;
	int row=a->row;
	int col=a->col;
	int i,j;
	float f;
	float *pfa;
	float *pfi;
	float *pfo;
// wtk_debug("========linear input========\n");
// for(i=0;i<input->len;++i)
// {
// 	printf("v[%d]=%f\n",i,input->p[i]);
// }
	//wtk_debug("input=%d output=%d m=%d/%d\n",input->len,output->len,a->row,a->col);
	pfa=a->p;
	pfi=input->p;
	pfo=output->p;
	for(i=0;i<row;++i)
	{
		f=0;
		for(j=0;j<col;++j)
		{
			f+=pfa[j]*pfi[j];
			//wtk_debug("v[%d]=%f/%f/%f\n",j,pfa[j],pfi[j],f);
		}
		//exit(0);
		pfo[i]=f;
		pfa+=col;
	}
	//exit(0);
}

void wtk_knn_linear_calc_fix_is(wtk_knn_linear_t *l, wtk_veci_t *input, wtk_vecs2_t *output)
{
    int idx, idx1, df;
    wtk_int64_t sum;
    wtk_fixmats_t *a = l->fixs_a;
    int row = a->row;
    int col = a->col;
    short *pfa = a->pv;
    unsigned char *shifts = a->shift;
    short *pfo = output->p;
    int *pfi = input->p;

    for (idx=0; idx<row; idx++) {
        sum = 0;
        for (idx1=0; idx1<col; idx1++) {
            sum += pfa[idx1] * pfi[idx1];
        }
		sum = ((sum)>>DEFAULT_RADIX);
        df = shifts[idx] - output->shift;
        if (df > 0) {
            sum = sum >> df;
        } else {
            sum = sum << (-df);
        }
        if (sum > 32767) {
            sum = 32767;
        } else if (sum < -32768) {
            sum = -32768;
        }
        pfo[idx] = sum;
        pfa += col;
    }
}

void wtk_knn_linear_calc_fix_ss(wtk_knn_linear_t *l, wtk_vecs2_t *input, wtk_vecs2_t *output)
{
    int idx, idx1, df;
    wtk_int64_t sum; //sumi;
    wtk_fixmats_t *a = l->fixs_a;
    int row = a->row;
    int col = a->col;
    short *pfa = a->pv;
    unsigned char *shifts = a->shift;
    short *pfo = output->p;
    short *pfi = input->p;
	int shiftx = input->shift;
// int i;
// wtk_debug("========linear input========\n");
// for(i=0;i<input->len;++i)
// {
// 	printf("v[%d]=%f,%d\n",i,FIX2FLOAT_ANY(input->p[i],input->shift),input->shift);
// }

//    sumi = pfi[0];
//    for (idx=1; idx<col; idx++) {
//        sumi += pfi[idx];
//    }
    for (idx=0; idx<row; idx++) {
        sum = 0;
        for (idx1=0; idx1<col; idx1++) {
            sum += pfa[idx1] * pfi[idx1];
        }
		// sum = ((sum)>>DEFAULT_RADIX)+((sumi*min[idx])>>DEFAULT_RADIX);
		sum = ((sum)>>shiftx);
        df = shifts[idx] - output->shift;
        if (df > 0) {
            sum = sum >> df;
        } else {
            sum = sum << (-df);
        }
        if (sum > 32767) {
            sum = 32767;
        } else if (sum < -32768) {
            sum = -32768;
        }
        pfo[idx] = sum;
        pfa += col;
    }
}

void wtk_knn_linear_calc_fix_ic(wtk_knn_linear_t *l, wtk_veci_t *input, wtk_vecs2_t *output)
{
    int idx, idx1, df;
    wtk_int64_t sumi, sum;
    wtk_fixmatc_t *a = l->fixc_a;
    short *min = a->min;
    int row = a->row;
    int col = a->col;
    unsigned char *pfa = a->pv;
    unsigned char *shifts = a->shift;
    short *pfo = output->p;
    int *pfi = input->p;

    sumi = pfi[0];
    for (idx=1; idx<col; idx++) {
        sumi += pfi[idx];
    }
    for (idx=0; idx<row; idx++) {
        sum = 0;
        for (idx1=0; idx1<col; idx1++) {
            sum += pfa[idx1] * pfi[idx1];
        }
		sum = ((sum)>>DEFAULT_RADIX)+((sumi*min[idx])>>DEFAULT_RADIX);
        df = shifts[idx] - output->shift;
        if (df > 0) {
            sum = sum / (1 << df);
        } else {
            sum = sum << (-df);
        }
        if (sum > 32767) {
            sum = 32767;
        } else if (sum < -32768) {
            sum = -32768;
        }
        pfo[idx] = sum;
        pfa += col;
    }
}

void wtk_knn_linear_calc_fix_sc(wtk_knn_linear_t *l, wtk_vecs2_t *input, wtk_vecs2_t *output)
{
    int idx, idx1, df;
    wtk_int64_t sumi, sum;
    wtk_fixmatc_t *a = l->fixc_a;
    short *min = a->min;
    int row = a->row;
    int col = a->col;
    unsigned char *pfa = a->pv;
    unsigned char *shifts = a->shift;
    short *pfo = output->p;
    short *pfi = input->p;
	int shiftx = input->shift;
// int i;
// wtk_debug("========linear input========\n");
// for(i=0;i<input->len;++i)
// {
// 	printf("v[%d]=%f,%d\n",i,FIX2FLOAT_ANY(input->p[i],input->shift),input->shift);
// }

    sumi = pfi[0];
    for (idx=1; idx<col; idx++) {
        sumi += pfi[idx];
    }
    for (idx=0; idx<row; idx++) {
        sum = 0;
        for (idx1=0; idx1<col; idx1++) {
            sum += pfa[idx1] * pfi[idx1];
        }
		// sum = ((sum)>>DEFAULT_RADIX)+((sumi*min[idx])>>DEFAULT_RADIX);
		sum = ((sum)>>shiftx)+((sumi*min[idx])>>shiftx);
        df = shifts[idx] - output->shift;
        if (df > 0) {
            sum = sum / (1 << df);
        } else {
            sum = sum << (-df);
        }
        if (sum > 32767) {
            sum = 32767;
        } else if (sum < -32768) {
            sum = -32768;
        }
        pfo[idx] = sum;
        pfa += col;
    }

}

void wtk_knn_ng_mask_affine_calc(wtk_knn_ng_mask_affine_t *layer,wtk_vecf_t *input,wtk_vecf_t *output)
{
	wtk_matf_t *a=layer->a;
	wtk_vecf_t *b=layer->b;
	int row=a->row;
	int col=a->col;
	int i,j;
	float f;
	float *pfa;
	float *pfb;
	float *pfi;
	float *pfo;

	pfa=a->p;
	pfb=b->p;
	pfi=input->p;
	pfo=output->p;
	for(i=0;i<row;++i)
	{
		f=0;
		for(j=0;j<col;++j)
		{
			f+=pfa[j]*pfi[j];
			//wtk_debug("v[%d]=%f/%f/%f\n",j,pfa[j],pfi[j],f);
		}
		//wtk_debug("v[%d]=%f/%f/%f\n",i,pfb[i],f,f+pfb[i]);
		//exit(0);
		f+=pfb[i];
		pfa+=col;
		pfo[i]=f;
		//wtk_debug("v[%d]=%f b=%f f=%f\n",i,f-pfb[i],pfb[i],f);
		//wtk_debug("v[%d]=%f\n",i,f);
	}
	//exit(0);
}

void wtk_knn_conv_calc_fix_ic(wtk_knn_conv_t *conv, wtk_veci_t *input, wtk_vecs2_t *output)
{
	wtk_fixmatc_t *filter =conv->ab->fix_char->a;
	wtk_fixvecc_t *bias = conv->ab->fix_char->b;
	int input_x_dim=conv->input_x_dim;
	int num_frames;
	//int input_y_dim=conv->input_y_dim;
	int filter_x_dim=conv->filter_x_dim;
//	int filter_y_dim=conv->filter_y_dim;
	int filter_x_step = conv->filter_x_step;
//	int filter_y_step = conv->filter_y_step;
	int num_x_steps = (1+(input_x_dim-filter_x_dim)/filter_x_step);
	int num_y_steps = conv->num_y_step;
//	int filter_dim = conv->ab->a->col;
	int filter_dim = conv->ab->fix_char->a->col;
//	int num_filters = conv->ab->a->row;
	num_frames=1;
	wtk_mati_t * patches;
	//wtk_vecf_print(input);
	patches=wtk_mati_new(num_frames,num_x_steps*num_y_steps*filter_dim);
	wtk_mati_zero(patches);
	wtk_knn_conv_input2inputpatches_int(conv,input,patches);
	
	int i,j,k;
	wtk_int64_t out,out2;
	int input_col = num_frames*num_x_steps*num_y_steps;
	int input_row = filter_dim;
	int w_row = filter->row;
	int w_col = filter->col;
	unsigned char* shifts=filter->shift;
	int bmin=bias->min;
	// int shiftx=input->shift;
	// wtk_debug("irow=%d,icol=%d,wrow=%d,wcol=%d\n",input_row,input_col,w_row,w_col);
	int out_row = input_col;
	int out_col = w_row;
	int df,df2;
	short *min = filter->min;
	df2=bias->shift-output->shift;

	int *in;
	unsigned char* w;
	unsigned char* b = bias->pv;
	// wtk_debug("output-len=%d\n",output->len);
	memset(output->p,0,output->len*sizeof(short));
	for(i=0;i<out_row;++i)
	{
		out=0;
		out2=0;
		in = patches->p + i*input_row;
		for(j=0;j<input_row;++j)
		{
			out2+=*(in+j);
		}
		for(j=0;j<out_col;++j)
		{
			w = filter->pv + j*w_col;
			for(k=0;k<input_row;++k)
			{
				// wtk_debug("in=%d,w=%d\n",in[k],w[k]);
				out+=in[k]*w[k];
			}
			out=((out)>>DEFAULT_RADIX)+((out2*min[j])>>DEFAULT_RADIX);
			df=shifts[j]-output->shift;
			if(df>0)
			{
				out=out/(1<<df);
			}else
			{
				out=out<<(-df);
			}
			if(df2>0)
			{
				out+=(b[j]+bmin)/(1<<df2);
			}else
			{
				out+=(b[j]+bmin)<<(-df2);
			}
			if(out>32767)
			{
				out=32767;
			}else if(out<-32768)
			{
				out=-32768;
			}
			output->p[i*out_col+j]=out;
			// wtk_debug("v[%d]=%d/%f\n",i*out_col+j,(int)out,FIX2FLOAT_ANY(out,output->shift));
		}
	}
	// for(i=0;i<out_col*out_row;++i)
	// {
	// 	wtk_debug("out[%d]=%f\n",i,FIX2FLOAT_ANY(output->p[i],output->shift));
	// }
	wtk_mati_delete(patches);
	// exit(0);

}

void wtk_knn_conv_calc_fix_sc(wtk_knn_conv_t *conv, wtk_vecs2_t *input, wtk_vecs2_t *output)
{
	wtk_fixmatc_t *filter =conv->ab->fix_char->a;
	wtk_fixvecc_t *bias = conv->ab->fix_char->b;
	int input_x_dim=conv->input_x_dim;
	int num_frames;
	//int input_y_dim=conv->input_y_dim;
	int filter_x_dim=conv->filter_x_dim;
//	int filter_y_dim=conv->filter_y_dim;
	int filter_x_step = conv->filter_x_step;
//	int filter_y_step = conv->filter_y_step;
	int num_x_steps = (1+(input_x_dim-filter_x_dim)/filter_x_step);
	int num_y_steps = conv->num_y_step;
//	int filter_dim = conv->ab->a->col;
	int filter_dim = conv->ab->fix_char->a->col;
//	int num_filters = conv->ab->a->row;
	num_frames=1;
	wtk_mats_t * patches;
	//wtk_vecf_print(input);
	patches=wtk_mats_new(num_frames,num_x_steps*num_y_steps*filter_dim);
	wtk_mats_zero(patches);
	wtk_knn_conv_input2inputpatches_short(conv,input,patches);
	
	int i,j,k;
	wtk_int64_t out,out2;
	int input_col = num_frames*num_x_steps*num_y_steps;
	int input_row = filter_dim;
	int w_row = filter->row;
	int w_col = filter->col;
	short *min = filter->min;
	int bmin=bias->min;
	int shiftx=input->shift;
	// wtk_debug("irow=%d,icol=%d,wrow=%d,wcol=%d\n",input_row,input_col,w_row,w_col);
	int out_row = input_col;
	int out_col = w_row;
	int df,df2;
	unsigned char* shifts=filter->shift;
	df2=bias->shift-output->shift;

	short *in;
	unsigned char* w;
	unsigned char* b = bias->pv;
	//wtk_debug("output-len=%d\n",output->len);
	memset(output->p,0,output->len*sizeof(short));
	for(i=0;i<out_row;++i)
	{
		out=0;
		out2=0;
		in = patches->p + i*input_row;
		for(j=0;j<input_row;++j)
		{
			out2+=*(in+j);
		}
		for(j=0;j<out_col;++j)
		{
			w = filter->pv + j*w_col;
			for(k=0;k<input_row;++k)
			{
				// wtk_debug("in=%d,w=%d\n",in[k],w[k]);
				out+=in[k]*w[k];
			}
			out=((out)>>shiftx)+((out2*min[j])>>shiftx);
			df=shifts[j]-output->shift;
			if(df>0)
			{
				out=out/(1<<df);
			}else
			{
				out=out<<(-df);
			}
			if(df2>0)
			{
				out+=(b[j]+bmin)/(1<<df2);
			}else
			{
				out+=(b[j]+bmin)<<(-df2);
			}
			if(out>32767)
			{
				out=32767;
			}else if(out<-32768)
			{
				out=-32768;
			}
			output->p[i*out_col+j]=out;
			// wtk_debug("v[%d]=%d/%f\n",i*out_col+j,(int)out,FIX2FLOAT_ANY(out,output->shift));
		}
	}
	// for(i=0;i<out_col*out_row;++i)
	// {
	// 	wtk_debug("out[%d]=%f\n",i,FIX2FLOAT_ANY(output->p[i],output->shift));
	// }
	wtk_mats_delete(patches);
	// exit(0);
}

void wtk_knn_conv_calc_fix_is(wtk_knn_conv_t *conv, wtk_veci_t *input, wtk_vecs2_t *output)
{
	wtk_fixmats_t *filter =conv->ab->fix_short->a;
	wtk_fixvecs_t *bias = conv->ab->fix_short->b;
	int input_x_dim=conv->input_x_dim;
	int num_frames;
	//int input_y_dim=conv->input_y_dim;
	int filter_x_dim=conv->filter_x_dim;
//	int filter_y_dim=conv->filter_y_dim;
	int filter_x_step = conv->filter_x_step;
//	int filter_y_step = conv->filter_y_step;
	int num_x_steps = (1+(input_x_dim-filter_x_dim)/filter_x_step);
	int num_y_steps = conv->num_y_step;
//	int filter_dim = conv->ab->a->col;
	int filter_dim = conv->ab->fix_short->a->col;
//	int num_filters = conv->ab->a->row;
	num_frames=1;
	wtk_mati_t * patches;
	//wtk_vecf_print(input);
	patches=wtk_mati_new(num_frames,num_x_steps*num_y_steps*filter_dim);
	wtk_mati_zero(patches);
	wtk_knn_conv_input2inputpatches_int(conv,input,patches);
	
	int i,j,k;
	wtk_int64_t out;
	int input_col = num_frames*num_x_steps*num_y_steps;
	int input_row = filter_dim;
	int w_row = filter->row;
	int w_col = filter->col;
	unsigned char* shifts=filter->shift;
	// int shiftx=input->shift;
	// wtk_debug("irow=%d,icol=%d,wrow=%d,wcol=%d\n",input_row,input_col,w_row,w_col);
	int out_row = input_col;
	int out_col = w_row;
	int df,df2;
	df2=bias->shift-output->shift;

	int *in;
	short* w;
	short* b = bias->pv;
	// wtk_debug("output-len=%d\n",output->len);
	memset(output->p,0,output->len*sizeof(short));
	for(i=0;i<out_row;++i)
	{
		out=0;
		in = patches->p + i*input_row;
		for(j=0;j<out_col;++j)
		{
			w = filter->pv + j*w_col;
			for(k=0;k<input_row;++k)
			{
				// wtk_debug("in=%d,w=%d\n",in[k],w[k]);
				out+=in[k]*w[k];
			}
			out=((out)>>DEFAULT_RADIX);
			df=shifts[j]-output->shift;
			if(df>0)
			{
				out=out>>df;
			}else
			{
				out=out<<(-df);
			}
			if(df2>0)
			{
				out+=b[j]>>df2;
			}else
			{
				out+=b[j]<<(-df2);
			}
			if(out>32767)
			{
				out=32767;
			}else if(out<-32768)
			{
				out=-32768;
			}
			output->p[i*out_col+j]=out;
			// wtk_debug("v[%d]=%d/%f\n",i*out_col+j,(int)out,FIX2FLOAT_ANY(out,output->shift));
		}
	}
	// for(i=0;i<out_col*out_row;++i)
	// {
	// 	wtk_debug("out[%d]=%f\n",i,FIX2FLOAT_ANY(output->p[i],output->shift));
	// }
	wtk_mati_delete(patches);
	// exit(0);

}

void wtk_knn_conv_calc_fix_ss(wtk_knn_conv_t *conv, wtk_vecs2_t *input, wtk_vecs2_t *output)
{
	wtk_fixmats_t *filter =conv->ab->fix_short->a;
	wtk_fixvecs_t *bias = conv->ab->fix_short->b;
	int input_x_dim=conv->input_x_dim;
	int num_frames;
	//int input_y_dim=conv->input_y_dim;
	int filter_x_dim=conv->filter_x_dim;
//	int filter_y_dim=conv->filter_y_dim;
	int filter_x_step = conv->filter_x_step;
//	int filter_y_step = conv->filter_y_step;
	int num_x_steps = (1+(input_x_dim-filter_x_dim)/filter_x_step);
	int num_y_steps = conv->num_y_step;
//	int filter_dim = conv->ab->a->col;
	int filter_dim = conv->ab->fix_short->a->col;
//	int num_filters = conv->ab->a->row;
	num_frames=1;
	wtk_mats_t * patches;
	//wtk_vecf_print(input);
	patches=wtk_mats_new(num_frames,num_x_steps*num_y_steps*filter_dim);
	wtk_mats_zero(patches);
	wtk_knn_conv_input2inputpatches_short(conv,input,patches);
	
	int i,j,k;
	wtk_int64_t out;
	int input_col = num_frames*num_x_steps*num_y_steps;
	int input_row = filter_dim;
	int w_row = filter->row;
	int w_col = filter->col;
	unsigned char* shifts=filter->shift;
	int shiftx=input->shift;
	// wtk_debug("irow=%d,icol=%d,wrow=%d,wcol=%d\n",input_row,input_col,w_row,w_col);
	int out_row = input_col;
	int out_col = w_row;
	int df,df2;
	df2=bias->shift-output->shift;

	short *in;
	short* w;
	short* b = bias->pv;
	// wtk_debug("output-len=%d\n",output->len);
	memset(output->p,0,output->len*sizeof(short));
	for(i=0;i<out_row;++i)
	{
		out=0;
		in = patches->p + i*input_row;
		for(j=0;j<out_col;++j)
		{
			w = filter->pv + j*w_col;
			for(k=0;k<input_row;++k)
			{
				// wtk_debug("in=%d,w=%d\n",in[k],w[k]);
				out+=in[k]*w[k];
			}
			out=(out)>>shiftx;
			df=shifts[j]-output->shift;
			if(df>0)
			{
				out=out>>df;
			}else
			{
				out=out<<(-df);
			}
			if(df2>0)
			{
				out+=b[j]>>df2;
			}else
			{
				out+=b[j]<<(-df2);
			}
			if(out>32767)
			{
				out=32767;
			}else if(out<-32768)
			{
				out=-32768;
			}
			output->p[i*out_col+j]=out;
			// wtk_debug("v[%d]=%d/%f\n",i*out_col+j,(int)out,FIX2FLOAT_ANY(out,output->shift));
		}
	}
	// for(i=0;i<out_col*out_row;++i)
	// {
	// 	wtk_debug("out[%d]=%f\n",i,FIX2FLOAT_ANY(output->p[i],output->shift));
	// }
	wtk_mats_delete(patches);
	// exit(0);

}

void wtk_knn_ab_calc_fix_sc(wtk_knn_ab_t *ab,wtk_vecs2_t *input,wtk_vecs2_t *output)
{
	wtk_fixmatc_t *a=ab->fix_char->a;
	wtk_fixvecc_t *b=ab->fix_char->b;
	int row=a->row;
	int col=a->col;
	int i,j;
	wtk_int64_t v,v2;
	unsigned char *pfa=a->pv;
	unsigned char *pfb=b->pv;
	short *min=a->min;
	short *pfi;
	short *pfo;
	int df,df2;
	unsigned char* shifts=a->shift;
	//float f;
	int shiftx=input->shift;
	int bmin=b->min;
	//output->shift=12;
	pfi=input->p;
	pfo=output->p;
	df2=b->shift-output->shift;
	//wtk_debug("shift=%d\n",ab->fixa->shift);
	v2=pfi[0];
	for(j=1;j<col;++j)
	{
		v2+=pfi[j];
	}
	for(i=0;i<row;++i)
	{
		v=0;
		for(j=0;j<col;++j)
		{
			v+=pfa[j]*pfi[j];
		}
		v=((v)>>shiftx)+((v2*min[i])>>shiftx);
		df=shifts[i]-output->shift;
		if(df>0)
		{
			//v=v>>df;
			v=v/(1<<df);
		}else
		{
			v=v<<(-df);
		}
		if(df2>0)
		{
			//v+=(pfb[i]+bmin)>>df2;
			v+=(pfb[i]+bmin)/(1<<df2);
		}else
		{
			v+=(pfb[i]+bmin)<<(-df2);
		}
#ifdef DEBUG_X
		if(v>35000 || v<-35000)
		{
			wtk_debug("output ERR =%lld input=%d output=%d\n",v,input->shift,output->shift);
			//exit(0);
		}
#endif
		if(v>32767)
		{
			v=32767;
		}else if(v<-32768)
		{
			v=-32768;
		}
		pfa+=col;
		pfo[i]=v;
		//wtk_debug("v[%d]=%lld/%f\n",i,v,FIX2FLOAT_ANY(v,output->shift));
#ifdef DEBUG_X
		if(fabs(v-pfo[i])>1)
		{
			wtk_debug("v=%lld/%d\n",v,pfo[i]);
			exit(0);
		}
#endif
	}
	//exit(0);
}

/*
 * input: (1<<12)
 */
void wtk_knn_ab_calc_fix_ic(wtk_knn_ab_t *ab,wtk_veci_t *input,wtk_vecs2_t *output)
{
	wtk_fixmatc_t *a=ab->fix_char->a;
	wtk_fixvecc_t *b=ab->fix_char->b;
	int row=a->row;
	int col=a->col;
	int i,j;
	wtk_int64_t v,v2;
	unsigned char *pfa=a->pv;
	unsigned char *pfb=b->pv;
	short *min=a->min;
	int *pfi;
	short *pfo;
	int df,df2;
	unsigned char* shifts=a->shift;
	int bmin=b->min;

	pfi=input->p;
	pfo=output->p;
	df2=b->shift-output->shift;
	v2=pfi[0];
	for(j=1;j<col;++j)
	{
		v2+=pfi[j];
	}
	for(i=0;i<row;++i)
	{
		v=0;
		for(j=0;j<col;++j)
		{
			//v+=((amin+pfa[j])*pfi[j]);
			// wtk_debug("in=%d,w=%d\n",pfi[j],pfa[j]);
			v+=pfa[j]*pfi[j];
		}
		v=((v)>>DEFAULT_RADIX)+((v2*min[i])>>DEFAULT_RADIX);
		// wtk_debug("v=%d,v2=%d,min=%d\n",(int)v,(int)v2,min[i]);
		//v=v>>DEFAULT_RADIX;
		df=shifts[i]-output->shift;
		// wtk_debug("df=%d,oshift=%d\n",df,output->shift);
		if(df>0)
		{
			//v=v>>df;
			v=v/(1<<df);
		}else
		{
			v=v<<(-df);
		}
		if(df2>0)
		{
			//v+=(pfb[i]+bmin)>>df2;
			v+=(pfb[i]+bmin)/(1<<df2);
		}else
		{
			v+=(pfb[i]+bmin)<<(-df2);
		}
#ifdef DEBUG_X
		if(v>35000 || v<-35000)
		{
			wtk_debug("output ERR =%lld\n",v);
			//exit(0);
		}
#endif
		if(v>32767)
		{
			v=32767;
		}else if(v<-32768)
		{
			v=-32768;
		}
		pfo[i]=v;
		pfa+=col;
		// wtk_debug("v[%d]=%lld/%f\n",i,v,FIX2FLOAT_ANY(v,output->shift));
#ifdef DEBUG_X
		if(fabs(v-pfo[i])>1)
		{
			wtk_debug("v=%lld/%d\n",v,pfo[i]);
			exit(0);
		}
#endif
		// wtk_debug("v[%d]=%d/%f\n",i,(int)v,FIX2FLOAT_ANY(v,output->shift));
	}
	// exit(0);
}


//calc  4 row,8 col every time
void wtk_knn_ab_calc3(wtk_knn_ab_t *layer,wtk_vecf_t *input,wtk_vecf_t *output)
{
	wtk_matf_t *a=layer->a;
	wtk_vecf_t *b=layer->b;
	int row=a->row;
	int col=a->col;
	int i,j,k;
	register float f,f1,f2,f3;
	register float *pfa;
	float *pfb;
	register float *pfi;
	float *pfo;
	int row2=a->row&(~3);
	int col2=a->col&(~7);
	int left,left2;


	pfa=a->p;
	pfb=b->p;
	pfi=input->p;
	pfo=output->p;
	left=a->col&7;
	left2=a->col&3;
	for(i=0;i<row2;i+=4)
	{
		f=f1=f2=f3=0;
		for(j=0;j<col2*4;j+=4*8)
		{

			for(k=0;k<8;++k)
			{
				f+=pfa[j+k]*pfi[j/4+k];
			}

			for(k=0;k<8;++k)
			{
				f1+=pfa[j+8+k]*pfi[j/4+k];
			}

			for(k=0;k<8;++k)
			{
				f2+=pfa[j+2*8+k]*pfi[j/4+k];
			}

			for(k=0;k<8;++k)
			{
				f3+=pfa[j+3*8+k]*pfi[j/4+k];
			}
		}


		if(left>=4)
		{
			for(k=0;k<4;++k)
			{
				f+=pfa[j+k]*pfi[j/4+k];
			}

			for(k=0;k<4;++k)
			{
				f1+=pfa[j+4+k]*pfi[j/4+k];
			}

			for(k=0;k<4;++k)
			{
				f2+=pfa[j+2*4+k]*pfi[j/4+k];
			}

			for(k=0;k<4;++k)
			{
				f3+=pfa[j+3*4+k]*pfi[j/4+k];
			}

			j+=4*4;
		}

		if(left2>0)
		{
			for(k=0;k<left2;++k)
			{
				f+=pfa[j+k]*pfi[j/4+k];
			}

			for(k=0;k<left2;++k)
			{
				f1+=pfa[j+left2+k]*pfi[j/4+k];
			}

			for(k=0;k<left2;++k)
			{
				f2+=pfa[j+2*left2+k]*pfi[j/4+k];
			}

			for(k=0;k<left2;++k)
			{
				f3+=pfa[j+3*left2+k]*pfi[j/4+k];
			}

			j+=4*left2;
		}

		f+=pfb[i];
		f1+=pfb[i+1];
		f2+=pfb[i+2];
		f3+=pfb[i+3];
		pfa+=col*4;
		pfo[i]=f;
		pfo[i+1]=f1;
		pfo[i+2]=f2;
		pfo[i+3]=f3;
	}


	for(;i<row;++i)
	{
		f=0;
		for(j=0;j<col;++j)
		{
			f+=pfa[j]*pfi[j];
			//wtk_debug("v[%d/%d]=%f/%f/%f\n",i,j,pfa[j],pfi[j],f);
		}
		f+=pfb[i];
		pfa+=col;
		pfo[i]=f;
	}
}


void wtk_knn_ab_calc2(wtk_knn_ab_t *layer,wtk_vecf_t *input,wtk_vecf_t *output)
{
	wtk_matf_t *a=layer->a;
	wtk_vecf_t *b=layer->b;
	int row=a->row;
	int col=a->col;
	int i,j;
	register float f;
	register float *pfa;
	float *pfb;
	register float *pfi;
	float *pfo;

	pfa=a->p;
	pfb=b->p;
	pfi=input->p;
	pfo=output->p;
	for(i=0;i<row;++i)
	{
		f=0;
		for(j=0;j<col;++j)
		{
			f+=pfa[j]*pfi[j];
			//wtk_debug("v[%d/%d]=%f/%f/%f\n",i,j,pfa[j],pfi[j],f);
		}
		//wtk_debug("v[%d]=%f/%f/%f\n",i,pfb[i],f,f+pfb[i]);
//		if(i>0)
//		{
//			exit(0);
//		}
		f+=pfb[i];
		pfa+=col;
		pfo[i]=f;
		//wtk_debug("v[%d]=%f b=%f f=%f\n",i,f-pfb[i],pfb[i],f);
		//wtk_debug("v[%d]=%f\n",i,f);
//		if(i>0)
//		{
//			exit(0);
//		}
	}
	//exit(0);
}


void wtk_knn_ab_calc(wtk_knn_ab_t *layer,wtk_vecf_t *input,wtk_vecf_t *output)
{

#ifdef USE_ARM_MATH
	wtk_arm_math_vecf_muti_matf_transf2_add_vec(output->p,a->p,input->p,b->p,a->row,a->col);

//	wtk_neon_math_vecf_muti_matf_transf2(output->p,a->p,input->p,row,col);
//	wtk_armv8_math_vecf_muti_matf(output->p,a->p,input->p,row,col);

//	wtk_arm_math_vecf_muti_matf_transf(output->p,a->p,input->p,row,col);
////	wtk_armv8_math_vecf_muti_matf_transf2(output->p,a->p,input->p,row,col);
//	for(i=0;i<row;++i)
//	{
//		pfo[i]+=pfb[i];
////		printf("%d %f\n",i,pfo[i]);
//	}
//	wtk_armv8_math_vecf_muti_matf_transf2_add_vec(output->p,a->p,input->p,b->p,a->row,a->col);
#else
	wtk_knn_ab_calc2(layer,input,output);
// wtk_knn_ab_calc3(layer,input,output);
//	wtk_knn_ab_calc_fix_ss_1(layer,input,output);

//	for(i=0;i<row;++i)
//	{
//		printf("%d %f\n",i,pfo[i]);
//	}
#endif
}

void wtk_fsmn_calc(wtk_knn_fsmn_t *fsmn, wtk_vecf_t *input, wtk_vecf_t *output) {
    int i, j;
    float *in_ptr, *param_ptr;

    if (fsmn->bias) {
        memcpy(output->p, fsmn->bias->p, sizeof(float) * fsmn->params->col);
    } else {
        memset(output->p, 0, sizeof(float) * fsmn->params->col);
    }

    in_ptr = input->p;
    param_ptr = fsmn->params->p;

    for (i = 0; i < fsmn->params->row; i++) {
        for (j = 0; j < fsmn->params->col; j++) {
            output->p[j] += *in_ptr++ * *param_ptr++;
        }
    }
}

void wtk_knn_ab_calc_fix_is(wtk_knn_ab_t *ab,wtk_veci_t *input,wtk_vecs2_t *output)
{
	wtk_fixmats_t *a=ab->fix_short->a;
	wtk_fixvecs_t *b=ab->fix_short->b;
	int row=a->row;
	int col=a->col;
	int i,j;
	wtk_int64_t lv;
	int v;
	short *pfa=a->pv;
	short *pfb=b->pv;
	int *pfi;
	short *pfo;
	int df,df2;
	int shift=b->shift;
	unsigned char* shifts=a->shift;

	pfi=input->p;
	pfo=output->p;
	df2=shift-output->shift;
	for(i=0;i<row;++i)
	{
		lv=0;
		//v=0;
		for(j=0;j<col;++j)
		{
			lv+=pfa[j]*pfi[j];
		}
		v=lv>>DEFAULT_RADIX;
		df=shifts[i]-output->shift;
		if(df>0)
		{
			v=v>>df;
		}else if(df<0)
		{
			v=v<<(-df);
		}
		if(df2>0)
		{
			v+=pfb[i]>>df2;
		}else if(df2<0)
		{
			v+=pfb[i]<<(-df2);
		}else
		{
			v+=pfb[i];
		}
		if(v>35000 || v<-35000)
		{
			wtk_debug(" ERR v=%d shift=%d\n",v,output->shift);
		}
		if(v>32767)
		{
			v=32767;
		}else if(v<-32768)
		{
			v=-32768;
		}
		pfa+=col;
		pfo[i]=v;
		if(wtk_abs(v-pfo[i])>1)
		{
			wtk_debug("v[%d]=%d/%d\n",i,v,pfo[i]);
			exit(0);
		}
	}
}


void wtk_knn_ab_calc_fix_ss(wtk_knn_ab_t *ab,wtk_vecs2_t *input,wtk_vecs2_t *output)
{
	int row=ab->fix_short->a->row;
	int col=ab->fix_short->a->col;
	int i,j;
	wtk_int64_t lv;
	int v;
	short *pfa=ab->fix_short->a->pv;
	short *pfb=ab->fix_short->b->pv;
	short *pfi;
	short *pfo;
	int df,df2;
	int shift=ab->fix_short->b->shift;
	unsigned char* shifts=ab->fix_short->a->shift;
	int shift2;
	int shifti=input->shift;
	static int ki=0;
	//static int xi=0;

	++ki;
	pfi=input->p;
	pfo=output->p;
	//output->shift=6;
	df2=shift-output->shift;
	//wtk_debug("shift=%d\n",ab->fixa->shift);
	for(i=0;i<row;++i)
	{
		lv=0;
		for(j=0;j<col;++j)
		{
			lv+=(pfa[j]*pfi[j]);//>>shifti;
			//v+=(pfa[j]*pfi[j])>>shifti;
//			wtk_debug("v[%d]=%f*%f/%f\n",j,FIX2FLOAT_ANY(pfa[j],ab->fixa->shift),FIX2FLOAT_ANY(pfi[j],12),
//					FIX2FLOAT_ANY(pfa[i],ab->fixa->shift));
		}
		v=lv>>shifti;
		//wtk_debug("v[%d]=%f shift=%d/%d\n",i,FIX2FLOAT_ANY(v,shifts[i]),shifts[i],output->shift);
		shift2=shifts[i];
		//wtk_debug("shift=%d/%d\n",shift2,shift);
		df=shift2-output->shift;
		if(df>0)
		{
			v=v>>df;
		}else
		{
			v=v<<(-df);
		}
		if(df2>0)
		{
			v+=pfb[i]>>df2;
		}else
		{
			v+=pfb[i]<<(-df2);
		}
		pfa+=col;
		if(v>35000 || v<-35000)
		{
			wtk_debug("output ERR =%d\n",v);
			//exit(0);
		}
		if(v>32767)
		{
			v=32767;
			//++xi;
		}else if(v<-32768)
		{
			v=-32768;
			//++xi;
		}
		pfo[i]=v;
		if(wtk_abs(v-pfo[i])>1)
		{
			wtk_debug("ki=%d shifti=%d\n",ki,shifti);
			wtk_debug("v[%d]=%d/%d\n",i,v,pfo[i]);
			exit(0);
		}
		//wtk_debug("v[%d]=%d/%f\n",i,v,FIX2FLOAT_ANY(v,output->shift));
//		if(i>3)
//		{
//			exit(0);
//		}
	}
	//wtk_debug("xi=%d\n",xi);
}


static void wtk_knn_ab_update_fix_char(wtk_knn_ab_t*  ab)
{
	ab->fix_char=(wtk_knn_ab_char_t*)wtk_malloc(sizeof(wtk_knn_ab_char_t));
	ab->fix_char->a=wtk_fixmatc_new(ab->a->p,ab->a->row,ab->a->col);
	ab->fix_char->b=wtk_fixvecc_new2(ab->b->p,ab->b->len);
}

static void wtk_knn_ab_update_fix_short(wtk_knn_ab_t *ab,int short_shift)
{
	ab->fix_short=(wtk_knn_ab_short_t*)wtk_malloc(sizeof(wtk_knn_ab_short_t));
	ab->fix_short->a=wtk_fixmats_new(ab->a->p,ab->a->row,ab->a->col,short_shift);
	ab->fix_short->b=wtk_fixvecs_new(ab->b->p,ab->b->len,short_shift);
}

static void wtk_knn_ab_delete(wtk_knn_ab_t *ab)
{
	if(ab->a)
	{
		wtk_matf_delete(ab->a);
	}
	if(ab->b)
	{
		wtk_vecf_delete(ab->b);
	}
	if(ab->fix_char)
	{
		wtk_fixmatc_delete(ab->fix_char->a);
		wtk_fixvecc_delete(ab->fix_char->b);
		wtk_free(ab->fix_char);
	}
	if(ab->fix_short)
	{
		wtk_fixmats_delete(ab->fix_short->a);
		wtk_fixvecs_delete(ab->fix_short->b);
		wtk_free(ab->fix_short);
	}
	wtk_free(ab);
}

static int wtk_knn_ab_params(wtk_knn_ab_t *ab)
{
	int v;

	v=0;
	if(ab->a)
	{
		v+=ab->a->row*ab->a->col;
	}
	if(ab->b)
	{
		v+=ab->b->len;
	}
	if(v>0)
	{
		return v;
	}
	if(ab->fix_short)
	{
		v+=ab->fix_short->a->row*ab->fix_short->a->col;
		v+=ab->fix_short->b->len;
	}
	if(v>0)
	{
		return v;
	}
	if(ab->fix_char)
	{
		v+=ab->fix_char->a->row*ab->fix_char->a->col;
		v+=ab->fix_char->b->len;
	}
	return v;
}

static int wtk_knn_ab_bytes(wtk_knn_ab_t *ab)
{
	int bytes=sizeof(wtk_knn_ab_t);

	if(ab->a)
	{
		bytes+=wtk_matf_bytes(ab->a);
	}
	if(ab->b)
	{
		bytes+=wtk_vecf_bytes(ab->b);
	}
	if(ab->fix_short)
	{
		bytes+=sizeof(wtk_knn_ab_short_t);
		bytes+=wtk_fixmats_bytes(ab->fix_short->a);
		bytes+=wtk_fixvecs_bytes(ab->fix_short->b);
	}
	if(ab->fix_char)
	{
		bytes+=sizeof(wtk_knn_ab_char_t);
		bytes+=wtk_fixmatc_bytes(ab->fix_char->a);
		bytes+=wtk_fixvecc_bytes(ab->fix_char->b);
	}
	return bytes;
}

static wtk_knn_normalize_t* wtk_knn_normalize_new(void)
{
	wtk_knn_normalize_t *n;

	n=(wtk_knn_normalize_t*)wtk_malloc(sizeof(wtk_knn_normalize_t));
	n->rms=1;
	n->rms2=-1;
	n->add_log_stddev=0;
	return n;
}

wtk_knn_static_extraction_t* wtk_knn_static_extraction_new(void)
{
	wtk_knn_static_extraction_t *s;
 
    s=(wtk_knn_static_extraction_t*)wtk_malloc(sizeof(*s));
    
    return s;
}

void wtk_knn_static_extraction_delete(wtk_knn_static_extraction_t *s)
{
	wtk_free(s);
}

wtk_knn_static_pooling_t* wtk_knn_static_pooling_new(void)
{
	wtk_knn_static_pooling_t *p;

	p=(wtk_knn_static_pooling_t*)wtk_malloc(sizeof(*p));

	return p;
}

void wtk_knn_static_pooling_delete(wtk_knn_static_pooling_t *p)
{
	wtk_free(p);
}

char* wtk_knn_layer_type_str(wtk_knn_layer_type_t type)
{
	int i;

	i=0;
	while(1)
	{
		if(map[i].type==type)
		{
			return map[i].name.data;
		}
		++i;
	}
	return NULL;
}

static wtk_knn_layer_type_t wtk_knn_layer_type(char *data,int len)
{
	int i;

	i=0;
	while(1)
	{
		if(map[i].name.len==0)
		{
			return -1;
		}
		if(wtk_str_equal(data,len,map[i].name.data,map[i].name.len))
		{
			return map[i].type;
		}
		++i;
	}
}

#if 0
static int wtk_knn_cfg_check_map(int *map,int nmap,int index,int skip)
{
	int i,v;

	//check current frame has affect
	for(i=0;i<nmap;++i)
	{
		v=index-map[i];
		if(v<0)
		{
			return 0;
		}
		if(v%(skip+1)==0)
		{
			return 1;
		}
	}
	//wtk_debug("not right\n");
	return 0;
}
#endif

int wtk_knn_bitmap_check_index(wtk_knn_bitmap_t *map,int index,int skip)
{
	int i,v;
	int n;
	char *bit;

//	n=(index>>1)<<1;
//	if(n==index)
//	{
//		return 1;
//	}else
//	{
//		return 0;
//	}
	n=map->nbit;
	bit=map->bit;
	//  wtk_debug("============= full=%d =================\n",map->full);
	if(map->full)
	{
//		n=(index>>1)<<1;
//		if(n==index)
//		{
//			return 1;
//		}else
//		{
//			return 0;
//		}
		for(i=0;i<n;++i)
		{
			v=index-bit[i];
			if(v<0)
			{
				return 0;
			}
			return 1;
		}
		return 1;
	}
	//check current frame has affect
//	wtk_debug("index=%d\n",index);
//	for(i=0;i<n;++i)
//	{
//		wtk_debug("v[%d]=%d\n",i,bit[i]);
//	}
	//exit(0);
	skip+=1;
	for(i=0;i<n;++i)
	{
		v=index-bit[i];
		//  wtk_debug("v=%d,index=%d bit[i]=%d\n",v,index,bit[i]);
		if(v<0)
		{
			return 0;
		}
		if(v%(skip)==0)
		{
			//wtk_debug("v=%d skip=%d\n",v,skip+1);
			//exit(0);
			return 1;
		}
	}
	//exit(0);
	//wtk_debug("not right\n");
	return 0;
}

static wtk_knn_bitmap_t* wtk_knn_bitmap_new(int *v,int n,int skip)
{
	wtk_knn_bitmap_t *bmap;
	int i;

	bmap=(wtk_knn_bitmap_t*)wtk_malloc(sizeof(wtk_knn_bitmap_t));
	bmap->nbit=n;
	bmap->bit=(char*)wtk_calloc(n,sizeof(char));
	bmap->full=1;
	for(i=0;i<n;++i)
	{
		bmap->bit[i]=v[i];
		    // wtk_debug("v[%d]=%d/%d\n",i,v[i],v[i-1]);
		if(bmap->full==1 && i>0 && (v[i]-v[i-1]>skip))
		{
			bmap->full=0;
		}
	}
	return bmap;
}

static void wtk_knn_bitmap_delete(wtk_knn_bitmap_t *map)
{
	wtk_free(map->bit);
	wtk_free(map);
}



static wtk_knn_ng_affine_t* wtk_knn_ng_affine_new(void)
{
	wtk_knn_ng_affine_t *l;

	l=(wtk_knn_ng_affine_t*)wtk_malloc(sizeof(wtk_knn_ng_affine_t));
	l->ab=NULL;
	return l;
}

static void wtk_knn_ng_affine_delete(wtk_knn_ng_affine_t *layer)
{
	if(layer->ab)
	{
		wtk_knn_ab_delete(layer->ab);
	}
	wtk_free(layer);
}

static int wtk_knn_ng_affine_bytes(wtk_knn_ng_affine_t *layer)
{
	int bytes;

	bytes=sizeof(wtk_knn_ng_affine_t);
	if(layer->ab)
	{
		bytes+=wtk_knn_ab_bytes(layer->ab);
	}
	return bytes;
}


static wtk_batch_norm_t* wtk_batch_norm_new(void)
{
	wtk_batch_norm_t *bn;

	bn=(wtk_batch_norm_t*)wtk_malloc(sizeof(wtk_batch_norm_t));
	bn->offset=NULL;
	bn->scale=NULL;
	bn->fix_char=NULL;
	bn->fix_short=NULL;
	return bn;
}

static int wtk_batch_norm_params(wtk_batch_norm_t *bn)
{
	int v=0;

	if(bn->offset)
	{
		v+=bn->offset->len;
	}
	if(bn->scale)
	{
		v+=bn->scale->len;
	}
	if(v>0)
	{
		return v;
	}
	if(bn->fix_char)
	{
		v+=bn->fix_char->offset->len;
		v+=bn->fix_char->scale->len;
		return v;
	}
	if(bn->fix_short)
	{
		v+=bn->fix_short->offset->len;
		v+=bn->fix_short->scale->len;
		return v;
	}
	return v;
}

static int wtk_batch_norm_bytes(wtk_batch_norm_t *bn)
{
	int bytes;

	bytes=sizeof(wtk_batch_norm_t);
	if(bn->offset)
	{
		bytes+=wtk_vecf_bytes(bn->offset);
	}
	if(bn->scale)
	{
		bytes+=wtk_vecf_bytes(bn->scale);
	}
	if(bn->fix_char)
	{
		bytes+=sizeof(wtk_batch_norm_char_t);
		bytes+=wtk_fixvecc_bytes(bn->fix_char->offset);
		bytes+=wtk_fixvecc_bytes(bn->fix_char->scale);
	}
	if(bn->fix_short)
	{
		bytes+=sizeof(wtk_batch_norm_short_t);
		bytes+=wtk_fixvecs_bytes(bn->fix_short->offset);
		bytes+=wtk_fixvecs_bytes(bn->fix_short->scale);
	}
	return bytes;
}

static void wtk_batch_norm_delete(wtk_batch_norm_t *bn)
{
	if(bn->offset)
	{
		wtk_vecf_delete(bn->offset);
	}
	if(bn->scale)
	{
		wtk_vecf_delete(bn->scale);
	}
	if(bn->fix_char)
	{
		wtk_fixvecc_delete(bn->fix_char->offset);
		wtk_fixvecc_delete(bn->fix_char->scale);
		wtk_free(bn->fix_char);
	}
	if(bn->fix_short)
	{
		wtk_fixvecs_delete(bn->fix_short->offset);
		wtk_fixvecs_delete(bn->fix_short->scale);
		wtk_free(bn->fix_short);
	}
	wtk_free(bn);
}

static int wtk_knn_conv_params(wtk_knn_conv_t *conv)
{
	//need to do;
	static int v=0;
	return v;
}

static wtk_knn_affine_t* wtk_knn_affine_new(void)
{
	wtk_knn_affine_t *layer;

	layer=(wtk_knn_affine_t*)wtk_malloc(sizeof(wtk_knn_affine_t));
	layer->ab=NULL;
	return layer;
}

static int wtk_knn_affine_bytes(wtk_knn_affine_t *layer)
{
	int bytes;

	bytes=sizeof(wtk_knn_affine_t);
	if(layer->ab)
	{
		bytes+=wtk_knn_ab_bytes(layer->ab);
	}
	return bytes;
}

static void wtk_knn_affine_delete(wtk_knn_affine_t *layer)
{
	if(layer->ab)
	{
		wtk_knn_ab_delete(layer->ab);
	}
	wtk_free(layer);
}

static wtk_knn_fixed_affine_t* wtk_knn_fixed_affine_new(void)
{
	wtk_knn_fixed_affine_t *l;

	l=(wtk_knn_fixed_affine_t*)wtk_malloc(sizeof(wtk_knn_fixed_affine_t));
	l->ab=NULL;
	return l;
}

static void wtk_knn_fixed_affine_delete(wtk_knn_fixed_affine_t *layer)
{
	if(layer->ab)
	{
		wtk_knn_ab_delete(layer->ab);
	}
	wtk_free(layer);
}

static int wtk_knn_fixed_affine_bytes(wtk_knn_fixed_affine_t *layer)
{
	int bytes;

	bytes=sizeof(wtk_knn_fixed_affine_t);
	if(layer->ab)
	{
		bytes+=wtk_knn_ab_bytes(layer->ab);
	}
	return bytes;
}

static wtk_knn_range_layer_t* wtk_knn_range_layer_new(void)
{
	wtk_knn_range_layer_t *rlayer;
	rlayer=(wtk_knn_range_layer_t*)wtk_malloc(sizeof(wtk_knn_range_layer_t));
	rlayer->input=NULL;
	rlayer->dim=0;
	rlayer->dim_offset=0;

	return rlayer;
}

static wtk_knn_logic_layer_t* wtk_knn_logic_layer_new(void)
{
	wtk_knn_logic_layer_t *layer;

	layer=(wtk_knn_logic_layer_t*)wtk_malloc(sizeof(wtk_knn_logic_layer_t));
	layer->name=NULL;//wtk_string_dup_data(name,nl);
	layer->component=NULL;
	layer->layer=NULL;
	layer->layer_input=NULL;
	layer->range_layer=NULL;
	layer->n_input=0;
	layer->skip=0;
	layer->input.type=WTK_KNN_CMD_V_I;
	layer->use_tdnn_input=0;
	layer->use_tdnn_output=0;
    layer->tdnn_max_left = 0;
    layer->tdnn_max_right = 0;
	layer->bitmap=NULL;
	layer->shift=0;
	return layer;
}

void wtk_knn_cmd_delete(wtk_knn_cmd_t *cmd);

static void wtk_knn_cmd_v_delete(wtk_knn_cmd_v_t *v)
{
	switch(v->type)
	{
	case WTK_KNN_CMD_V_I:
		break;
	case WTK_KNN_CMD_V_STRING:
		wtk_string_delete(v->v.str);
		break;
	case WTK_KNN_CMD_V_CMD:
		wtk_knn_cmd_delete(v->v.cmd);
		break;
	case WTK_KNN_CMD_V_F:
		break;
	default:
		break;
	}
	wtk_free(v);
}

int wtk_knn_cmd_bytes(wtk_knn_cmd_t *cmd);

static int wtk_knn_cmd_v_bytes(wtk_knn_cmd_v_t *v)
{
	int bytes;

	bytes=sizeof(wtk_knn_cmd_v_t);
	switch(v->type)
	{
	case WTK_KNN_CMD_V_I:
		break;
	case WTK_KNN_CMD_V_STRING:
		bytes+=wtk_string_bytes(v->v.str);
		break;
	case WTK_KNN_CMD_V_CMD:
		bytes+=wtk_knn_cmd_bytes(v->v.cmd);
		break;
	case WTK_KNN_CMD_V_F:
		break;
	default:
		break;
	}
	return bytes;
}

int wtk_knn_cmd_bytes(wtk_knn_cmd_t *cmd)
{
	int bytes;
	int i;

	bytes=sizeof(wtk_knn_cmd_t);
	for(i=0;i<cmd->nv;++i)
	{
		bytes+=wtk_knn_cmd_v_bytes(cmd->v[i]);
	}
	bytes+=cmd->nv*sizeof(wtk_knn_cmd_v_t*);
	return bytes;
}

void wtk_knn_cmd_delete(wtk_knn_cmd_t *cmd)
{
	int i;

	for(i=0;i<cmd->nv;++i)
	{
		wtk_knn_cmd_v_delete(cmd->v[i]);
	}
	wtk_free(cmd->v);
	wtk_free(cmd);
}

static int wtk_knn_logic_layer_bytes(wtk_knn_logic_layer_t *layer)
{
	int bytes;

	bytes=sizeof(wtk_knn_logic_layer_t);
	if(layer->name)
	{
		bytes+=wtk_string_bytes(layer->name);
	}
	if(layer->component)
	{
		bytes+=wtk_string_bytes(layer->component);
	}
	if(layer->range_layer)
	{
		bytes+=sizeof(wtk_knn_range_layer_t);
	}
	switch(layer->input.type)
	{
	case WTK_KNN_CMD_V_I:
		break;
	case WTK_KNN_CMD_V_STRING:
		//wtk_debug("delete %.*s\n",layer->input.v.str->len,layer->input.v.str->data);
		bytes+=wtk_string_bytes(layer->input.v.str);
		break;
	case WTK_KNN_CMD_V_CMD:
		bytes+=wtk_knn_cmd_bytes(layer->input.v.cmd);
		break;
	case WTK_KNN_CMD_V_F:
		break;
	default:
		break;
	}
	return bytes;
}

static void  wtk_knn_logic_layer_delete(wtk_knn_logic_layer_t *layer)
{
	if(layer->name)
	{
		wtk_string_delete(layer->name);
	}
	if(layer->component)
	{
		wtk_string_delete(layer->component);
	}
	switch(layer->input.type)
	{
	case WTK_KNN_CMD_V_I:
		break;
	case WTK_KNN_CMD_V_STRING:
		//wtk_debug("delete %.*s\n",layer->input.v.str->len,layer->input.v.str->data);
		wtk_string_delete(layer->input.v.str);
		break;
	case WTK_KNN_CMD_V_CMD:
		wtk_knn_cmd_delete(layer->input.v.cmd);
		break;
	case WTK_KNN_CMD_V_F:
		break;
	default:
		break;
	}
	if(layer->bitmap)
	{
		wtk_knn_bitmap_delete(layer->bitmap);
	}
	if(layer->layer_input)
	{
		wtk_free(layer->layer_input);
	}
	if(layer->range_layer)
	{
		wtk_free(layer->range_layer);
	}
	wtk_free(layer);
}

static wtk_knn_layer_t* wtk_knn_layer_new(char *name,int nl)
{
	wtk_knn_layer_t *layer;

	layer=(wtk_knn_layer_t*)wtk_malloc(sizeof(wtk_knn_layer_t));
	layer->name=wtk_string_dup_data(name,nl);
	return layer;
}

static wtk_knn_linear_t* wtk_knn_linear_new(void)
{
	wtk_knn_linear_t *linear;

	linear=(wtk_knn_linear_t*)wtk_malloc(sizeof(wtk_knn_linear_t));
	linear->a=NULL;
    linear->fixc_a = NULL;
	linear->fixs_a = NULL;
	return linear;
}

static void wtk_knn_linear_delete(wtk_knn_linear_t *linear)
{
	if(linear->a)
	{
		wtk_matf_delete(linear->a);
	}
    if (linear->fixc_a) {
        wtk_fixmatc_delete(linear->fixc_a);
    }
	if(linear->fixs_a)
	{
		wtk_fixmats_delete(linear->fixs_a);
	}
	wtk_free(linear);
}

static int wtk_knn_linear_bytes(wtk_knn_linear_t *linear)
{
	int bytes;

	bytes=sizeof(wtk_knn_linear_t);
	if(linear->a)
	{
		bytes+=wtk_matf_bytes(linear->a);
	}
    if(linear->fixc_a) {
        bytes += wtk_fixmatc_bytes(linear->fixc_a);
    }
	if(linear->fixs_a) {
        bytes += wtk_fixmats_bytes(linear->fixs_a);
    }
	return bytes;
}

static int wtk_knn_linear_params(wtk_knn_linear_t *linear)
{
	return linear->a ? linear->a->row*linear->a->col :
           linear->fixc_a ? linear->fixc_a->col * linear->fixc_a->row :
           linear->fixs_a->col * linear->fixs_a->row;
}

wtk_knn_conv_t* wtk_knn_conv_new(void)
{
	wtk_knn_conv_t *m;

	m=(wtk_knn_conv_t*)wtk_malloc(sizeof(wtk_knn_conv_t));
	m->ab=NULL;
	m->input_x_dim=0;
	m->input_y_dim=0;
	m->input_z_dim=0;
	m->filter_x_dim=0;
	m->filter_y_dim=0;
	m->filter_x_step=0;
	m->filter_y_step=0;
	m->num_y_step=0;
	m->height_offset=NULL;
	m->column_map=NULL;
	return m;
}

int wtk_knn_conv_bytes(wtk_knn_conv_t *layer)
{
	int bytes;

	bytes=sizeof(wtk_knn_conv_t);
	if(layer->ab)
	{
		bytes+=wtk_knn_ab_bytes(layer->ab);
	}
	if(layer->height_offset)
	{
		bytes+=wtk_veci_bytes(layer->height_offset);
	}
	if(layer->column_map)
	{
		bytes+=sizeof(int)*(layer->num_y_step*(1+(layer->input_x_dim-layer->filter_x_dim)/layer->filter_x_step)
		*layer->filter_x_dim*layer->filter_y_dim);
	}
	return bytes;
}

void wtk_knn_conv_delete(wtk_knn_conv_t *layer)
{
	if(layer->ab)
	{
		wtk_knn_ab_delete(layer->ab);
	}
	if(layer->height_offset)
	{
		wtk_veci_delete(layer->height_offset);
	}
	if(layer->column_map)
	{
		wtk_free(layer->column_map);
	}
	wtk_free(layer);
}

wtk_knn_lstm_t* wtk_knn_lstm_new(void)
{
	wtk_knn_lstm_t *lstm;
	lstm=(wtk_knn_lstm_t*)wtk_malloc(sizeof(wtk_knn_lstm_t));
	lstm->params=NULL;
	return lstm;
}

void wtk_knn_lstm_delete(wtk_knn_lstm_t *lstm)
{
	if(lstm->params)
	{
		wtk_matf_delete(lstm->params);
	}
	wtk_free(lstm);
}

wtk_knn_backproptrunc_t* wtk_knn_backproptrunc_new(void)
{
	wtk_knn_backproptrunc_t* bp_trunc;
	bp_trunc=(wtk_knn_backproptrunc_t*)wtk_malloc(sizeof(wtk_knn_backproptrunc_t));
	bp_trunc->dim=0;
	bp_trunc->scale=0;
	return bp_trunc;
}

void wtk_knn_backproptrunc_delete(wtk_knn_backproptrunc_t *bp_trunc)
{
	wtk_free(bp_trunc);
}

wtk_knn_scale_offset_t* wtk_knn_scale_offset_new(void)
{
	wtk_knn_scale_offset_t* so;
	so = (wtk_knn_scale_offset_t*)wtk_malloc(sizeof(wtk_knn_scale_offset_t));
	so->dim=0;
	so->scale=NULL;
	so->offset=NULL;
	return so;
}
void wtk_knn_scale_offset_delete(wtk_knn_scale_offset_t *so)
{
	if(so->scale)
	{
		wtk_vecf_delete(so->scale);
	}
	if(so->offset)
	{
		wtk_vecf_delete(so->offset);
	}
}

wtk_knn_general_dropout_t* wtk_knn_general_dropout_new(void)
{
	wtk_knn_general_dropout_t *gdrop_out;
	gdrop_out=(wtk_knn_general_dropout_t*)wtk_malloc(sizeof(wtk_knn_general_dropout_t));
	gdrop_out->block_dim=0;
	gdrop_out->dim=0;
	gdrop_out->dropout_proportion=0;
	gdrop_out->time_period=0;
	return  gdrop_out;
}

void wtk_knn_general_dropout_delete(wtk_knn_general_dropout_t* gdrop_out)
{
	wtk_free(gdrop_out);
}

wtk_knn_noop_t* wtk_knn_noop_new(void)
{
	wtk_knn_noop_t *noop;
	noop=(wtk_knn_noop_t*)wtk_malloc(sizeof(wtk_knn_noop_t));
	noop->backprop_scale=0;
	noop->dim=0;
	return noop;
}

void wtk_knn_noop_delete(wtk_knn_noop_t* noop)
{
	wtk_free(noop);
}

static wtk_knn_fsmn_t *wtk_knn_fsmn_new(void) {
    wtk_knn_fsmn_t *fsmn;

    fsmn = wtk_malloc(sizeof(wtk_knn_fsmn_t));

    fsmn->params = NULL;
    fsmn->bias = NULL;
    fsmn->lorder = 0;
    fsmn->rorder = 0;
    fsmn->lstride = 0;
    fsmn->rstride = 0;

    return fsmn;
}

static void wtk_knn_fsmn_delete(wtk_knn_fsmn_t *fsmn) {
    if (fsmn->params) {
        wtk_matf_delete(fsmn->params);
    }
    wtk_free(fsmn);
}

static wtk_knn_linear_mask_t* wtk_knn_linear_mask_new(void)
{
	wtk_knn_linear_mask_t *mask;

	mask=(wtk_knn_linear_mask_t*)wtk_malloc(sizeof(wtk_knn_linear_mask_t));
	mask->a=NULL;
	return mask;
}

static int wtk_knn_linear_mask_bytes(wtk_knn_linear_mask_t *mask)
{
	int bytes;

	bytes=sizeof(wtk_knn_linear_mask_t);
	if(mask->a)
	{
		bytes+=wtk_matf_bytes(mask->a);
	}
	return bytes;
}

static int wtk_knn_linear_mask_params(wtk_knn_linear_mask_t *m)
{
	int v;

	v=0;
	if(m->a)
	{
		v+=m->a->row*m->a->col;
	}
	return v;
}

static void wtk_knn_linear_mask_delete(wtk_knn_linear_mask_t *mask)
{
	if(mask->a)
	{
		wtk_matf_delete(mask->a);
	}
	wtk_free(mask);
}

static wtk_knn_ng_mask_affine_t* wtk_knn_ng_mask_affine_new(void)
{
	wtk_knn_ng_mask_affine_t *m;

	m=(wtk_knn_ng_mask_affine_t*)wtk_malloc(sizeof(wtk_knn_ng_mask_affine_t));
	m->a=NULL;
	m->b=NULL;
	return m;
}

static int wtk_knn_ng_mask_affine_bytes(wtk_knn_ng_mask_affine_t *m)
{
	int bytes;

	bytes=sizeof(wtk_knn_ng_mask_affine_t);
	if(m->a)
	{
		bytes+=wtk_matf_bytes(m->a);
	}
	if(m->b)
	{
		bytes+=wtk_vecf_bytes(m->b);
	}
	return bytes;
}

static int wtk_knn_ng_mask_affine_params(wtk_knn_ng_mask_affine_t *m)
{
	int v;

	v=0;
	if(m->a)
	{
		v+=m->a->row*m->a->col;
	}
	if(m->b)
	{
		v+=m->b->len;
	}
	return v;
}

static void wtk_knn_ng_mask_affine_delete(wtk_knn_ng_mask_affine_t *m)
{
	if(m->a)
	{
		wtk_matf_delete(m->a);
	}
	if(m->b)
	{
		wtk_vecf_delete(m->b);
	}
	wtk_free(m);
}

static int wtk_knn_layer_params(wtk_knn_layer_t *layer)
{
	int params;

	params=0;
	switch(layer->type)
	{
	case WTK_NaturalGradientAffineMaskComponent:
		params=wtk_knn_ng_mask_affine_params(layer->v.ng_mask_affine);
		break;
	case WTK_LinearMaskComponent:
		params+=wtk_knn_linear_mask_params(layer->v.linear_mask);
		break;
	case WTK_LinearComponent:
		params+=wtk_knn_linear_params(layer->v.linear);
		break;
	case WTK_AffineComponent:
		params+=wtk_knn_ab_params(layer->v.affine->ab);
		break;
	case WTK_FixedAffineComponent:
		params+=wtk_knn_ab_params(layer->v.fixed_affine->ab);
		break;
	case WTK_NaturalGradientAffineComponent:
		params+=wtk_knn_ab_params(layer->v.ng_affine->ab);
		break;
	case WTK_RectifiedLinearComponent:
		break;
	case WTK_BatchNormComponent:
		params+=wtk_batch_norm_params(layer->v.batch_norm);
		break;
	case WTK_NormalizeComponent:
		break;
	case WTK_LogSoftmaxComponent:
		break;
	case WTK_SigmoidComponent:
		break;
	case WTK_ConvolutionComponent:
		params+=wtk_knn_conv_params(layer->v.conv);
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
	return params;
}

static int wtk_knn_layer_bytes(wtk_knn_layer_t *layer)
{
	int bytes;

	bytes=sizeof(wtk_knn_layer_t);
	bytes+=wtk_string_bytes(layer->name);
	switch(layer->type)
	{
	case WTK_NaturalGradientAffineMaskComponent:
		bytes+=wtk_knn_ng_mask_affine_bytes(layer->v.ng_mask_affine);
		break;
	case WTK_LinearMaskComponent:
		bytes+=wtk_knn_linear_mask_bytes(layer->v.linear_mask);
		break;
	case WTK_LinearComponent:
		bytes+=wtk_knn_linear_bytes(layer->v.linear);
		break;
	case WTK_AffineComponent:
		bytes+=wtk_knn_affine_bytes(layer->v.affine);
		break;
	case WTK_FixedAffineComponent:
		bytes+=wtk_knn_fixed_affine_bytes(layer->v.fixed_affine);
		break;
	case WTK_NaturalGradientAffineComponent:
		bytes+=wtk_knn_ng_affine_bytes(layer->v.ng_affine);
		break;
	case WTK_RectifiedLinearComponent:
		break;
	case WTK_BatchNormComponent:
		bytes+=wtk_batch_norm_bytes(layer->v.batch_norm);
		break;
	case WTK_NormalizeComponent:
		bytes+=sizeof(WTK_NormalizeComponent);
		break;
	case WTK_LogSoftmaxComponent:
		break;
	case WTK_SigmoidComponent:
		break;
	case WTK_ConvolutionComponent:
		bytes+=wtk_knn_conv_bytes(layer->v.conv);
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
	return bytes;
}

static void wtk_knn_layer_delete(wtk_knn_cfg_t *cfg, wtk_knn_layer_t *layer)
{
	switch(layer->type)
	{
	case WTK_NaturalGradientAffineMaskComponent:
		wtk_knn_ng_mask_affine_delete(layer->v.ng_mask_affine);
		break;
	case WTK_LinearMaskComponent:
		wtk_knn_linear_mask_delete(layer->v.linear_mask);
		break;
	case WTK_LinearComponent:
        wtk_knn_linear_delete(layer->v.linear);
		break;
	case WTK_AffineComponent:
		wtk_knn_affine_delete(layer->v.affine);
		break;
	case WTK_FixedAffineComponent:
		wtk_knn_fixed_affine_delete(layer->v.fixed_affine);
		break;
	case WTK_NaturalGradientAffineComponent:
		wtk_knn_ng_affine_delete(layer->v.ng_affine);
		break;
	case WTK_RectifiedLinearComponent:
		break;
	case WTK_BatchNormComponent:
		wtk_batch_norm_delete(layer->v.batch_norm);
		break;
	case WTK_NormalizeComponent:
		wtk_free(layer->v.normalize);
		break;
	case WTK_LogSoftmaxComponent:
		break;
	case WTK_SigmoidComponent:
		break;
	case WTK_ConvolutionComponent:
		wtk_knn_conv_delete(layer->v.conv);
		break;
	case WTK_BackpropTruncationComponent:
		wtk_knn_backproptrunc_delete(layer->v.bp_trunc);
		break;
	case WTK_LstmNonlinearityComponent:
		wtk_knn_lstm_delete(layer->v.lstm);
		break;
	case WTK_ScaleAndOffsetComponent:
		wtk_knn_scale_offset_delete(layer->v.so);
		break;
	case WTK_NoOpComponent:
		wtk_knn_noop_delete(layer->v.noop);
		break;
	case WTK_GeneralDropoutComponent:
		wtk_knn_general_dropout_delete(layer->v.gdrop_out);
		break;
    case WTK_CompactFsmnComponent:
        wtk_knn_fsmn_delete(layer->v.fsmn);
        break;
	case WTK_StatisticsExtractionComponent:
		wtk_knn_static_extraction_delete(layer->v.static_extraction);
		break;
	case WTK_StatisticsPoolingComponent:
		wtk_knn_static_pooling_delete(layer->v.static_pooling);
		break;
	default:
		break;
	}
	wtk_string_delete(layer->name);
	wtk_free(layer);
}

void wtk_rectified_linear_calc(wtk_rectified_linear_t *layer,wtk_vecf_t *input,wtk_vecf_t *output)
{
    unuse(layer);
	float *pi,*po;
	int i,n;

	n=input->len;
	pi=input->p;
	po=output->p;
	for(i=0;i<n;++i)
	{
		po[i]=(pi[i]<0)?0:pi[i];
	}
}

void wtk_rectified_linear_calc_fix(wtk_rectified_linear_t *layer,wtk_veci_t *input,wtk_veci_t *output)
{
	int *pi,*po;
	int i,n;

	n=input->len;
	pi=input->p;
	po=output->p;
	for(i=0;i<n;++i)
	{
		po[i]=(pi[i]<0)?0:pi[i];
	}
}

void wtk_rectified_linear_calc_fix_char(wtk_rectified_linear_t *layer,wtk_vecs2_t *input,wtk_vecs2_t *output)
{
    unuse(layer);
	short *pi,*po;
	int i,n;

	output->shift=input->shift;
	n=input->len;
	pi=input->p;
	po=output->p;
	for(i=0;i<n;++i)
	{
		po[i]=(pi[i]<0)?0:pi[i];
	}
}

void wtk_normalize_calc(wtk_knn_normalize_t *normal,wtk_vecf_t *input,wtk_vecf_t *output)
{
	float alpha,sum;
	int len=input->len;
	float *pf;
	float *pf2=output->p;
	int i;

	alpha=normal->rms2;//1.0/(len*normal->rms*normal->rms);
	//wtk_debug("alpha=%f\n",alpha);
	//exit(0);
	sum=0;
	pf=input->p;
	for(i=0;i<len;++i)
	{
		sum+=pf[i]*pf[i];
	}
	sum=1.0/sqrt(sum*alpha);
	for(i=0;i<len;++i)
	{
		pf2[i]=pf[i]*sum;
	}
}

void wtk_batch_norm_calc(wtk_batch_norm_t *layer,wtk_vecf_t *input,wtk_vecf_t *output)
{
	float *p1,*p2;
	float *pi,*po;
	int i,j,k,len;
	int t;

	len=input->len;
	p1=layer->offset->p;
	p2=layer->scale->p;
	pi=input->p;
	po=output->p;
	t = len/layer->offset->len;
	k=0;
	for(j=0;j<t;++j)
	{
		for(i=0;i<layer->offset->len;++i)
		{
			po[k]=p1[i]+p2[i]*pi[k];
			k++;
			// wtk_debug("v[%d/%d]=%f/%f/%f/%f\n",k,i,p1[i],p2[i],pi[i],po[i]);
		}
	}
}

void wtk_batch_norm_calc_fix_char(wtk_batch_norm_t *layer,wtk_vecs2_t *input,wtk_vecs2_t *output)
{
	wtk_fixvecc_t  *offset=layer->fix_char->offset;
	wtk_fixvecc_t  *scale=layer->fix_char->scale;
	unsigned char *p1,*p2;
	short *pi,*po;
	int i,j,len;
	int shift2;
	int df1;
	int m1=offset->min;
	int m2=scale->min;
	int v2;
	int t,k;

	len=input->len;
	p1=offset->pv;
	p2=scale->pv;
	pi=input->p;
	po=output->p;
	shift2=scale->shift;
	t = len/layer->fix_char->offset->len;
	//wtk_debug("shift1=%d shift2=%d\n",shift1,shift2);
	output->shift=input->shift;
	df1=offset->shift-input->shift;
	k=0;
	short tmpo;
	if(df1>0)
	{
		//v1=1<<df1;
		for(j=0;j<t;++j)
		{
			for(i=0;i<layer->fix_char->offset->len;++i)
			{
				if((((p1[i]+m1)>>df1)+(((p2[i]+m2)*pi[k])>>shift2))>32767)
				{
					tmpo=32767;
				}else if((((p1[i]+m1)>>df1)+(((p2[i]+m2)*pi[k])>>shift2))<-32768)
				{
					tmpo=-32768;
				}else
				{
					tmpo=((p1[i]+m1)>>df1)+(((p2[i]+m2)*pi[k])>>shift2);
				}
				po[k]=tmpo;
				k++;
			}
		}
	}else
	{
		v2=1<<shift2;
		df1=-df1;
		for(j=0;j<t;++j)
		{
			for(i=0;i<layer->fix_char->offset->len;++i)
			{
				if((((p1[i]+m1)<<(df1))+(((p2[i]+m2)*pi[k])/v2)) > 32767)
				{
					tmpo=32767;
				}else if((((p1[i]+m1)<<(df1))+(((p2[i]+m2)*pi[k])/v2))<-32768)
				{
					tmpo=-32768;
				}else
				{
					tmpo = ((p1[i]+m1)<<(df1))+(((p2[i]+m2)*pi[k])/v2);
				}
				po[k]=tmpo;
				// wtk_debug("v[%d] %d,%d\n",i,(p1[i]+m1)<<(df1),((p2[i]+m2)*pi[k])/v2);
				k++;
			}
		}
	}
}

void wtk_batch_norm_calc_fix_short(wtk_batch_norm_t *layer,wtk_vecs2_t *input,wtk_vecs2_t *output)
{
	short *p1,*p2;
	short *pi,*po;
	int i,j,len;
	int t,k;
	int shift1,shift2;
	int df1;

	len=input->len;
	p1=layer->fix_short->offset->pv;
	p2=layer->fix_short->scale->pv;
	pi=input->p;
	po=output->p;
	shift1=layer->fix_short->offset->shift;
	shift2=layer->fix_short->scale->shift;
	//wtk_debug("shift1=%d shift2=%d\n",shift1,shift2);
	output->shift=input->shift;
	df1=shift1-input->shift;
	t = len/layer->fix_short->offset->len;
	k=0;
	short tmpo;
	//exit(0);
	for(j=0;j<t;++j)
	{
		for(i=0;i<layer->fix_short->offset->len;++i)
		{
			if(df1>0)
			{
				if(((p1[i]>>df1)+((p2[i]*pi[k])>>shift2))>32767)
				{
					tmpo=32767;
				}else if(((p1[i]>>df1)+((p2[i]*pi[k])>>shift2))<-32768)
				{
					tmpo=-32768;
				}else
				{
					tmpo=(p1[i]>>df1)+((p2[i]*pi[k])>>shift2);
				}
				po[k]=tmpo;
				k++;
			}else
			{
				if(((p1[i]<<df1)+((p2[i]*pi[k])>>shift2))>32767)
				{
					tmpo= 32767;
				}else if(((p1[i]<<df1)+((p2[i]*pi[k])>>shift2))<-32768)
				{
					tmpo=-32768;
				}else
				{
					tmpo=(p1[i]<<df1)+((p2[i]*pi[k])>>shift2);
				}
				po[k]=tmpo;
				k++;
			}
		}
	}
	//exit(0);
}

void wtk_knn_sigmoid(wtk_vecf_t *m,wtk_vecf_t *o)
{
    int col,j;
    float *pf,*pf2;

    pf=m->p;
    pf2=o->p;
    col=m->len;
	for(j=0;j<col;++j)
	{
		pf2[j]=1.0/(1.0+expf(-pf[j]));
	}
}

void wtk_log_softmax_calc_fix_short(wtk_fixexp_t *fixe,wtk_fixlog_t *fixl,wtk_vecs2_t *input,wtk_vecs2_t *output)
{
	int max;
	short *pi,*po;
	int i,len;
	int v;
	int df;
	int vx,v2;

	df=input->shift-output->shift;
	len=output->len;
	po=output->p;
	pi=input->p;
	max=pi[0];
	for(i=1;i<len;++i)
	{
		if(pi[i]>max)
		{
			max=pi[i];
		}
	}
	vx=0;
	if(df>0)
	{
		v2=1<<df;
		for(i=0;i<len;++i)
		{
			v=pi[i]-max;
			vx+=wtk_fixexp_calc(fixe,v);
			po[i]=v/v2;//(1<<df);
		}
	}else if(df<0)
	{
		for(i=0;i<len;++i)
		{
			v=pi[i]-max;
			vx+=wtk_fixexp_calc(fixe,v);
			po[i]=v<<(-df);
		}
	}else
	{
		for(i=0;i<len;++i)
		{
			vx+=wtk_fixexp_calc(fixe,pi[i]-max);
			if(pi[i]-max<-32768)
			{
				po[i] = -32768;
			}else
			{
				po[i]=pi[i]-max;
			}
		}
	}
	v=-wtk_fixlog_calc(fixl,vx);
	for(i=0;i<len;++i)
	{
		if(po[i]+v < -32768)
		{
			po[i]=-32768;
		}else
		{
			po[i]+=v;
		}
	}
}

void wtk_log_softmax_calc(wtk_log_softmax_t *layer,wtk_vecf_t *input)
{
    unuse(layer);
	float max,sum;
	float *pi;
	int i,len;

	len=input->len;
	pi=input->p;
	max=pi[0];
	for(i=1;i<len;++i)
	{
		if(pi[i]>max)
		{
			max=pi[i];
		}
	}
	//wtk_debug("max=%f\n",max);
	sum=0;
	for(i=0;i<len;++i)
	{
		pi[i]-=max;
		sum+=expf(pi[i]);
		//wtk_debug("v[%d]=%f/%f\n",i,pi[i],sum);
	}
	//wtk_debug("sum=%f\n",sum);
	sum=-log(sum);
	for(i=0;i<len;++i)
	{
		pi[i]+=sum;
	}
}


float wtk_knn_affine_calc2(wtk_knn_affine_t *layer,wtk_vecf_t *input,int id)
{
	float *pi,*pa;
	int i,len;
	float f;

	len=input->len;
	pi=input->p;
	pa=layer->ab->a->p+id*len;
	f=layer->ab->b->p[id];
	//wtk_debug("v[%d]=%f %d/%d/%d\n",id,f,layer->a->row,layer->a->col,layer->b->len);
	for(i=0;i<len;++i)
	{
		f+=pi[i]*pa[i];
		//wtk_debug("v[%d]=%f/%f/%f\n",i,f,pi[i],pa[i]);
	}
	//wtk_debug("f=%f\n",f);
	//exit(0);
	return f;
}


float wtk_knn_fixed_affine_calc2(wtk_knn_fixed_affine_t *layer,wtk_vecf_t *input,int id)
{
	float *pi,*pa;
	int i,len;
	float f;

	len=input->len;
	pi=input->p;
	pa=layer->ab->a->p+id*len;
	f=layer->ab->b->p[id];
	for(i=0;i<len;++i)
	{
		f+=pi[i]*pa[i];
	}
	return f;
}

int wtk_knn_layer_get_output(wtk_knn_layer_t *layer)
{
	switch(layer->type)
	{
	case WTK_NaturalGradientAffineMaskComponent:
		return layer->v.ng_mask_affine->b->len;
	case WTK_LinearMaskComponent:
		return layer->v.linear_mask->a->row;
	case WTK_LinearComponent:
		if(layer->v.linear->a)
		{
			return layer->v.linear->a->row;
		}else if(layer->v.linear->fixc_a)
		{
			return layer->v.linear->fixc_a->row;
		}else
		{
			return layer->v.linear->fixs_a->row;
		}
		
		
	case WTK_FixedAffineComponent:
		if(layer->v.fixed_affine->ab->b)
		{
			return layer->v.fixed_affine->ab->b->len;
		}else if(layer->v.fixed_affine->ab->fix_char)
		{
			return layer->v.fixed_affine->ab->fix_char->b->len;
		}else
		{
			return layer->v.fixed_affine->ab->fix_short->b->len;
		}
	case WTK_RectifiedLinearComponent:
		break;
	case WTK_BatchNormComponent:
		if(layer->v.batch_norm->offset)
		{
			//return layer->v.batch_norm->offset->len;
			// wtk_debug("batch_norm-dim=%d\n",layer->v.batch_norm->dim);
			return layer->v.batch_norm->dim;
		}else if(layer->v.batch_norm->fix_char)
		{
			return layer->v.batch_norm->dim;
			//return layer->v.batch_norm->fix_char->offset->len;
		}else
		{
			return layer->v.batch_norm->dim;
			//return layer->v.batch_norm->fix_short->offset->len;
		}
	case WTK_LogSoftmaxComponent:
		break;
	case WTK_AffineComponent:
		if(layer->v.affine->ab->b)
		{
			return layer->v.affine->ab->b->len;
		}else if(layer->v.affine->ab->fix_char)
		{
			return layer->v.affine->ab->fix_char->b->len;
		}else
		{
			return layer->v.affine->ab->fix_short->b->len;
		}
	case WTK_NaturalGradientAffineComponent:
		if(layer->v.ng_affine->ab->b)
		{
			return layer->v.ng_affine->ab->b->len;
		}else if(layer->v.ng_affine->ab->fix_char)
		{
			return layer->v.ng_affine->ab->fix_char->b->len;
		}else
		{
			return layer->v.ng_affine->ab->fix_short->b->len;
		}
	case WTK_NormalizeComponent:
		break;
	case WTK_SigmoidComponent:
		break;
	case WTK_ConvolutionComponent:
		if(layer->v.conv->ab->b)
		{
			return layer->v.conv->num_y_step*layer->v.conv->ab->b->len;
		}else if(layer->v.conv->ab->fix_char)
		{
			return layer->v.conv->num_y_step*layer->v.conv->ab->fix_char->b->len;
		}else
		{
			return layer->v.conv->num_y_step*layer->v.conv->ab->fix_short->b->len;
		}
    case WTK_CompactFsmnComponent:
        return layer->v.fsmn->params->col;
	case WTK_StatisticsExtractionComponent:
		if(layer->v.static_extraction->include_var)
		{
			return ((layer->v.static_extraction->input_dim<<1)+1);
		}else
		{
			return (layer->v.static_extraction->input_dim+1);
		}
		break;
	case WTK_StatisticsPoolingComponent:
		return (layer->v.static_pooling->input_dim-1);
		break;
	case WTK_NoOpComponent:
	case WTK_GeneralDropoutComponent:
	case WTK_LstmNonlinearityComponent:
	case WTK_BackpropTruncationComponent:
	case WTK_ScaleAndOffsetComponent:
		break;
	default:
		break;
	}
	return -1;
}

int wtk_knn_cfg_init(wtk_knn_cfg_t *cfg)
{
	cfg->use_prior=0;
	cfg->lc_prior=NULL;
	cfg->skip=0;
	cfg->output_dim=0;
	cfg->prior=NULL;
	cfg->fn=NULL;
	cfg->nlayer=0;
	cfg->layer=NULL;
	cfg->nlogic_layer=0;
	cfg->logic_layer=NULL;
	cfg->input_dim=13;
	cfg->ivector_dim=0;
	cfg->xvec_fn=NULL;
	cfg->xvec_map=NULL;
	cfg->skip_layer=NULL;
	cfg->bin_fn=NULL;
	cfg->use_bin=0;

	cfg->nmap=0;
	cfg->map=NULL;
	cfg->use_tdnn=0;

	cfg->tdnn_max_left=0;
	cfg->tdnn_max_right=0;

	cfg->input_map=NULL;

	cfg->use_exp=0;
	cfg->debug=0;
	cfg->use_fast_exp=0;
	cfg->use_fast_skip=0;
	cfg->use_fixpoint=0;
	cfg->use_fixchar=0;
	cfg->use_fixshort=0;
	cfg->use_fixchar0_short=1;
	cfg->max_shift=15;
	cfg->layer_shift=NULL;

	cfg->softmax_fixe=NULL;
	cfg->softmax_fixl=NULL;
	cfg->fixe=NULL;
	cfg->calc_shift=0;
	cfg->rt_nlogic_layer=0;
    cfg->left_context = 0;
    cfg->right_context = 0;
	cfg->update_offset = 0;
	cfg->use_add_log=0;
	cfg->offset_compress=3;
	cfg->output_name="output.affine";
	cfg->use_round=0;
	return 0;
}


int wtk_knn_cfg_bytes(wtk_knn_cfg_t *cfg)
{
	int bytes;
	int i;
	int v;

	//wtk_debug("############################# KNN #############################\n");
	bytes=sizeof(wtk_knn_cfg_t);
	if(cfg->map)
	{
		bytes+=cfg->nmap*sizeof(wtk_knn_map_t);
	}
	if(cfg->logic_layer)
	{
		v=0;
		for(i=0;i<cfg->nlogic_layer;++i)
		{
			v+=wtk_knn_logic_layer_bytes(cfg->logic_layer[i]);
		}
		v+=cfg->nlogic_layer*sizeof(wtk_knn_logic_layer_t*);
		bytes+=v;
		//wtk_debug("logic: %fKB\n",v*1.0/1024);
	}
	if(cfg->layer)
	{
		v=0;
		for(i=0;i<cfg->nlayer;++i)
		{
			v+=wtk_knn_layer_bytes(cfg->layer[i]);
		}
		v+=cfg->nlayer*sizeof(wtk_knn_layer_t*);
		bytes+=v;
		if(0)
		{
			wtk_debug("layer: %fKB\n",v*1.0/1024);
			v=0;
			for(i=0;i<cfg->nlayer;++i)
			{
				v+=wtk_knn_layer_params(cfg->layer[i]);
			}
			wtk_debug("parmas: %fKB\n",v*1.0/1024);
		}
	}
	//wtk_debug("knn: %fKB\n",bytes*1.0/1024);
	if(cfg->prior)
	{
		bytes+=wtk_vecf_bytes(cfg->prior);
	}
	//wtk_debug("knn: %fKB\n",bytes*1.0/1024);
	if(cfg->softmax_fixe)
	{
		bytes+=wtk_fixe_bytes(cfg->softmax_fixe);
	}
	if(cfg->softmax_fixl)
	{
		bytes+=wtk_fixlog_bytes(cfg->softmax_fixl);
	}
	//wtk_debug("knn: %fKB\n",bytes*1.0/1024);
	if(cfg->fixe)
	{
		bytes+=wtk_fixe_bytes(cfg->fixe);
	}
	wtk_debug("knn_cfg: %fKB\n",bytes*1.0/1024);
	return bytes;
}

int wtk_knn_cfg_clean(wtk_knn_cfg_t *cfg)
{
	int i;

	if(cfg->softmax_fixe)
	{
		wtk_fixexp_delete(cfg->softmax_fixe);
	}
	if(cfg->softmax_fixl)
	{
		wtk_fixlog_delete(cfg->softmax_fixl);
	}
	if(cfg->fixe)
	{
		wtk_fixexp_delete(cfg->fixe);
	}
	if(cfg->map)
	{
		wtk_free(cfg->map);
	}
	if(cfg->input_map)
	{
		wtk_knn_bitmap_delete(cfg->input_map);
	}
	if(cfg->logic_layer)
	{
		for(i=0;i<cfg->nlogic_layer;++i)
		{
			wtk_knn_logic_layer_delete(cfg->logic_layer[i]);
		}
		wtk_free(cfg->logic_layer);
	}
	if(cfg->layer)
	{
		for(i=0;i<cfg->nlayer;++i)
		{
			wtk_knn_layer_delete(cfg, cfg->layer[i]);
		}
		wtk_free(cfg->layer);
	}
	if(cfg->layer_shift)
	{
		wtk_free(cfg->layer_shift);
	}
	if(cfg->prior)
	{
		wtk_vecf_delete(cfg->prior);
	}
	if(cfg->xvec_map)
	{
		wtk_knn_cfg_xvec_delete(cfg);
	}
	return 0;
}

int wtk_knn_cfg_update_local(wtk_knn_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	wtk_array_t *a;

	//wtk_local_cfg_print(lc);
	wtk_local_cfg_update_cfg_str(lc,cfg,fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,xvec_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,bin_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,output_name,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_bin,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_exp,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fast_exp,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_prior,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fast_skip,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fixpoint,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fixchar,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fixchar0_short,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fixshort,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,calc_shift,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_shift,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,left_context,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,right_context,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,offset_compress,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_add_log,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,skip,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,skip_layer,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,update_offset,v);

	cfg->lc_prior=wtk_local_cfg_find_lc_s(lc,"prior");

    if (cfg->use_fixpoint) {
        a=wtk_local_cfg_find_array_s(lc,"layer_shift");
        if(a)
        {
            wtk_string_t **str;
            int i, tmp;

            cfg->layer_shift=(short*)wtk_calloc(a->nslot,sizeof(short));
            str=(wtk_string_t**)a->slot;
            for(i=0;i<a->nslot;++i)
            {
                //wtk_debug("v[%d]=[%.*s]\n",i,str[i]->len,str[i]->data);
                tmp=wtk_str_atoi(str[i]->data,str[i]->len);
                cfg->layer_shift[i]=tmp;
            }
            //wtk_debug("%d/%d\n",cfg->nmap,a->nslot);
            //exit(0);
        }
    }
    a=wtk_local_cfg_find_array_s(lc,"id");
    if(a)
	{
		wtk_string_t **str;
		int i,j;
		int id=0;
		int vi,b;

		cfg->nmap=0;//a->nslot;
		cfg->map=(wtk_knn_map_t*)wtk_calloc(a->nslot,sizeof(wtk_knn_map_t));
		str=(wtk_string_t**)a->slot;
		for(i=0;i<a->nslot;++i)
		{
			//wtk_debug("v[%d]=[%.*s]\n",i,str[i]->len,str[i]->data);
			vi=wtk_str_atoi(str[i]->data,str[i]->len);
			b=0;
			for(j=0;j<i;++j)
			{
				if(cfg->map[j].id==vi)
				{
					b=1;
					break;
				}
			}
			if(b){continue;}
			cfg->map[i].id=vi;
			cfg->map[i].knn_id=id++;
			++cfg->nmap;
		}
		//wtk_debug("%d/%d\n",cfg->nmap,a->nslot);
		//exit(0);
	}
	return 0;
}

int wtk_knn_cfg_update(wtk_knn_cfg_t *cfg)
{
    unuse(cfg);
	return 0;
}



static void wtk_knn_layer_set_type(wtk_knn_layer_t *layer,wtk_knn_layer_type_t type)
{
	layer->type=type;
}

wtk_veci_t* wtk_knn_read_vector2(wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	wtk_larray_t *a;
	int i;
	wtk_veci_t *vi=NULL;

	a=wtk_larray_new(100,sizeof(float));
	ret=wtk_source_seek_to_s(src,"[");
	if(ret!=0){goto end;}
	while(1)
	{
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){goto end;}
		if(buf->data[0]==']'){break;}
		i=wtk_str_atoi(buf->data,buf->pos);
		wtk_larray_push2(a,&i);
		//wtk_debug("[%.*s]=%f\n",buf->pos,buf->data,f);
	}
	vi=wtk_veci_new(a->nslot);
	memcpy(vi->p,a->slot,a->nslot*sizeof(float));
	ret=0;
end:
	wtk_larray_delete(a);
	//exit(0);
	return vi;
}

wtk_vecf_t* wtk_knn_read_vector(wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	wtk_larray_t *a;
	float f;
	wtk_vecf_t *vf=NULL;

	a=wtk_larray_new(100,sizeof(float));
	ret=wtk_source_seek_to_s(src,"[");
	if(ret!=0){goto end;}
	while(1)
	{
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){goto end;}
		if(buf->data[0]==']'){break;}
		f=wtk_str_atof(buf->data,buf->pos);
		wtk_larray_push2(a,&f);
		//wtk_debug("[%.*s]=%f\n",buf->pos,buf->data,f);
	}
	vf=wtk_vecf_new(a->nslot);
	memcpy(vf->p,a->slot,a->nslot*sizeof(float));
	ret=0;
end:
	wtk_larray_delete(a);
	//exit(0);
	return vf;
}

wtk_matf_t*  wtk_knn_read_matrix(wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	wtk_larray_t *a;
	float f;
	float *pf=NULL;
	int nl;
	wtk_larray_t *b;
	int row,col;
	wtk_matf_t *m=NULL;
	int i;
	float **pvf;

	row=col=0;
	b=wtk_larray_new(100,sizeof(float*));
	a=wtk_larray_new(100,sizeof(float));
	ret=wtk_source_expect_string_s(src,buf,"[");
	if(ret!=0){goto end;}
	ret=wtk_source_skip_sp(src,NULL);
	if(ret!=0){goto end;}
	while(1)
	{
		ret=wtk_source_skip_sp(src,&nl);
		if(ret!=0){goto end;}
		if(nl)
		{
			//wtk_debug("n=%d\n",a->nslot);
			if(col>0 && a->nslot!=col)
			{
				ret=-1;goto end;
			}
			++row;
			col=a->nslot;
			pf=(float*)wtk_calloc(col,sizeof(float));
			memcpy(pf,a->slot,a->nslot*sizeof(float));
			wtk_larray_reset(a);
			wtk_larray_push2(b,&(pf));
			//exit(0);
		}else
		{
			ret=wtk_source_read_string(src,buf);
			if(ret!=0){goto end;}
			if(buf->data[0]==']')
			{
				//if(col==0)
				{
					++row;
					col=a->nslot;
					pf=(float*)wtk_calloc(col,sizeof(float));
					memcpy(pf,a->slot,a->nslot*sizeof(float));
					wtk_larray_reset(a);
					wtk_larray_push2(b,&(pf));
				}
				break;
			}
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			f=(float)wtk_str_atof(buf->data,buf->pos);
			wtk_larray_push2(a,&f);
		}
	}
	m=wtk_matf_new(row,col);
	pvf=(float**)(b->slot);
	for(i=0;i<row;++i)
	{
		memcpy(m->p+i*col,pvf[i],col*sizeof(float));
	}
	ret=0;
end:
	pvf=(float**)(b->slot);
	for(i=0;i<b->nslot;++i)
	{
		wtk_free(pvf[i]);
	}
	wtk_larray_delete(a);
	wtk_larray_delete(b);
	//exit(0);
	return m;
}


static void wtk_matf_dot(wtk_matf_t *a,wtk_matf_t *b)
{
	int i,j;
	float *pf1,*pf2;

	pf1=a->p;
	pf2=b->p;
	for(i=0;i<b->row;++i)
	{
		for(j=0;j<b->col;++j)
		{
			pf1[j]*=pf2[j];
		}
		pf1+=b->col;
		pf2+=b->col;
	}
}

int wtk_knn_zyx_vector_index(int x,int y, int z, int input_x_dim,int input_y_dim,int input_z_dim)
{
	//Gwtk_debug("x=%d,y=%d,z=%d,x_dim=%d,y_dim=%d,z_dim=%d\n",x,y,z,input_x_dim,input_y_dim,input_z_dim);
	if(x>input_x_dim || y>input_y_dim || z>input_z_dim)
	{
		wtk_debug("err dim\n");
		return -1;
	}
	return ((input_y_dim*input_z_dim)*x + (input_z_dim)*y+z);
}

int* wtk_knn_conv_get_input2inputpatches_map(wtk_knn_conv_t *conv, int patches_row,int patches_col)
{
	int input_x_dim = conv->input_x_dim;
	int input_y_dim = conv->input_y_dim;
	int input_z_dim = conv->input_z_dim;
	int filter_x_dim = conv->filter_x_dim;
	//int filter_y_dim = conv->filter_y_dim;
	int filter_x_step = conv->filter_x_step;
	int filter_y_step = conv->filter_y_step;
	int num_x_steps = (1+(input_x_dim-filter_x_dim)/filter_x_step);
	int num_y_steps = conv->num_y_step;
	int filter_dim;
	if(conv->ab->fix_char)
	{
		filter_dim = conv->ab->fix_char->a->col;
	}else if(conv->ab->fix_short)
	{
		filter_dim = conv->ab->fix_short->a->col;
	}else
	{
		filter_dim = conv->ab->a->col;
	}
	//int filter_dim = conv->filter_x_dim*conv->filter_y_dim*input_z_dim;

	int* column_map = (int*)wtk_malloc(sizeof(int)*patches_col*patches_row);
	memset(column_map,-1,sizeof(int)*patches_col);
	//int map_size = patches->col;
	//wtk_debug("%d\n",patches->col);

	int xi,yi,xj,yj,zj;
	int y_idx,x_in,y_in;
	int patch_num,patch_start_idx,idx;
	for(xi=0;xi<num_x_steps;xi++)
	{
		for(yi=0;yi<num_y_steps;yi++)
		{
			patch_num = xi * num_y_steps +yi;
			patch_start_idx = patch_num * filter_dim;
			for(xj=0,idx=patch_start_idx;xj<filter_x_dim;xj++)
			{
				for(y_idx=0;y_idx<conv->height_offset->len;y_idx++)
				{
					yj = conv->height_offset->p[y_idx];
					for(zj=0;zj<input_z_dim;zj++,idx++)
					{
						if(idx>patches_col)
						{
							wtk_debug("len=%d idx=%d  out of side\n",patches_col,idx);
						}
						x_in = xi * filter_x_step + xj;
						y_in = yi * filter_y_step +yj;
						//wtk_debug("xin=%d,yin=%d\n",x_in,y_in);
						if(x_in>=0 && x_in <input_x_dim && y_in>=0 && y_in <input_y_dim)
						{
							//wtk_debug("map[%d]=%d\n",idx,column_map[idx]);
							column_map[idx]=wtk_knn_zyx_vector_index(xi*filter_x_step + xj, yi*filter_y_step +yj,zj,
							input_x_dim,input_y_dim,input_z_dim);
							//wtk_debug("map[%d]=%d\n",idx,column_map[idx]);
						}
					}
				}
			}
		}
	}
	return column_map;
}

void wtk_knn_conv_input2inputpatches(wtk_knn_conv_t *conv, wtk_vecf_t * input, wtk_matf_t* patches)
{
	float *data = patches->p;
	float *src = input->p;
	int *column_map=conv->column_map;
	int *indx;
	int i,j;
	for(i=0;i<patches->row;i++,data+=1,src+=1)
	{
		indx =&(column_map[0]);
		for(j =0;j<patches->col;j++,indx++)
		{
			if(*indx<0)
			{
				data[j]=0;
			}else
			{
				data[j]=src[*indx];
			}
		}
	}
}

void wtk_knn_conv_input2inputpatches_int(wtk_knn_conv_t *conv, wtk_veci_t * input, wtk_mati_t* patches)
{
	int *data = patches->p;
	int *src = input->p;
	int *column_map=conv->column_map;
	int *indx;
	int i,j;
	for(i=0;i<patches->row;i++,data+=1,src+=1)
	{
		indx =&(column_map[0]);
		for(j =0;j<patches->col;j++,indx++)
		{
			if(*indx<0)
			{
				data[j]=0;
			}else
			{
				data[j]=src[*indx];
			}
		}
	}
}

void wtk_knn_conv_input2inputpatches_short(wtk_knn_conv_t *conv, wtk_vecs2_t * input, wtk_mats_t* patches)
{
	short *data = patches->p;
	short *src = input->p;
	int *column_map=conv->column_map;
	int *indx;
	int i,j;
	for(i=0;i<patches->row;i++,data+=1,src+=1)
	{
		indx =&(column_map[0]);
		for(j =0;j<patches->col;j++,indx++)
		{
			if(*indx<0)
			{
				data[j]=0;
			}else
			{
				data[j]=src[*indx];
			}
		}
	}
}

static int wtk_knn_linear_mask_load(wtk_knn_linear_mask_t *mask,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	wtk_matf_t *b;

	ret=wtk_source_seek_to_s(src,"<Params>");
	if(ret!=0)
	{
		wtk_debug("expected params failed\n");
		goto end;
	}
	mask->a=wtk_knn_read_matrix(src,buf);
	if(!mask->a)
	{
		wtk_debug("read params failed\n");
		ret=-1;goto end;
	}
	ret=wtk_source_expect_string_s(src,buf,"<Mask>");
	if(ret!=0)
	{
		wtk_debug("expected bias failed\n");
		goto end;
	}
	b=wtk_knn_read_matrix(src,buf);
	if(!b)
	{
		wtk_debug("read mask failed\n");
		ret=-1;goto end;
	}
	//wtk_debug("%d/%d %d/%d\n",mask->a->row,mask->a->col,b->row,b->col);
	//wtk_matf_print(mask->a);
	wtk_matf_dot(mask->a,b);
	//wtk_matf_print(mask->a);
	//exit(0);
	wtk_matf_delete(b);
	ret=wtk_source_seek_to_s(src,"</LinearComponent>");
	if(ret!=0)
	{
		wtk_debug("expected /LinearComponent failed\n");
		goto end;
	}
	//wtk_debug("l=%p %d/%d/%d\n",l,l->a->row,l->a->col,l->b->len);
	ret=0;
end:
	return ret;
}

static int wtk_knn_linear_load(wtk_knn_linear_t *linear,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;

	ret=wtk_source_seek_to_s(src,"<Params>");
	if(ret!=0)
	{
		wtk_debug("expected params failed\n");
		goto end;
	}
	linear->a=wtk_knn_read_matrix(src,buf);
	if(!linear->a)
	{
		wtk_debug("read params failed\n");
		ret=-1;goto end;
	}
	ret=wtk_source_seek_to_s(src,"</LinearComponent>");
	if(ret!=0)
	{
		wtk_debug("expected /LinearComponent failed\n");
		goto end;
	}
	// wtk_debug("l=%p %d/%d\n",linear,linear->a->row,linear->a->col);
	ret=0;
end:
	return ret;
}

//conv load
static int wtk_knn_conv_load(wtk_knn_conv_t *conv,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	ret=wtk_source_seek_to_s(src,"<InputXDim>");
	if(ret!=0)
	{
		wtk_debug("expected input_x_dim failed\n");
		goto end;
	}
	ret=wtk_source_read_int(src,&(conv->input_x_dim),1,0);
	ret=wtk_source_seek_to_s(src,"<InputYDim>");
	if(ret!=0)
	{
		wtk_debug("expected input_y_dim failed\n");
		goto end;
	}
	ret=wtk_source_read_int(src,&(conv->input_y_dim),1,0);
	ret=wtk_source_seek_to_s(src,"<InputZDim>");
	if(ret!=0)
	{
		wtk_debug("expected input_z_dim failed\n");
		goto end;
	}
	ret=wtk_source_read_int(src,&(conv->input_z_dim),1,0);
	ret=wtk_source_seek_to_s(src,"<FiltXDim>");
	if(ret!=0)
	{
		wtk_debug("expected filter_x_dim failed\n");
		goto end;
	}
	ret=wtk_source_read_int(src,&(conv->filter_x_dim),1,0);
	ret=wtk_source_seek_to_s(src,"<FiltYDim>");
	if(ret!=0)
	{
		wtk_debug("expected filter_y_dim failed\n");
		goto end;
	}
	ret=wtk_source_read_int(src,&(conv->filter_y_dim),1,0);
	ret=wtk_source_seek_to_s(src,"<FiltXStep>");
	if(ret!=0)
	{
		wtk_debug("expected filt_y_step failed\n");
		goto end;
	}
	ret=wtk_source_read_int(src,&(conv->filter_x_step),1,0);
	ret=wtk_source_seek_to_s(src,"<FiltYStep>");
	if(ret!=0)
	{
		wtk_debug("expected filt_y_step failed\n");
		goto end;
	}
	ret=wtk_source_read_int(src,&(conv->filter_y_step),1,0);
	ret=wtk_source_seek_to_s(src,"<NumYSteps>");
	if(ret!=0)
	{
		wtk_debug("expected NumYSteps failed\n");
		goto end;
	}
	ret=wtk_source_read_int(src,&(conv->num_y_step),1,0);
	ret=wtk_source_seek_to_s(src,"<HeightOffsets>");
	if(ret!=0)
	{
		wtk_debug("expected NumYSteps failed\n");
		goto end;
	}
	conv->height_offset=wtk_knn_read_vector2(src,buf);
	ret=wtk_source_seek_to_s(src,"<FilterParams>");
	if(ret!=0)
	{
		wtk_debug("expected filter params failed\n");
		goto end;
	}
	conv->ab=wtk_knn_ab_new();
	conv->ab->a=wtk_knn_read_matrix(src,buf);
	//wtk_debug("conv row=%d,col=%d\n",conv->ab->a->row,conv->ab->a->col);
	//wtk_debug("%f %f %f %f\n",conv->ab->a->p[0],conv->ab->a->p[1],conv->ab->a->p[2],conv->ab->a->p[3]);
	if(!conv->ab->a)
	{
		wtk_debug("read filter params failed\n");
		ret=-1;goto end;
	}
	ret=wtk_source_expect_string_s(src,buf,"<BiasParams>");
	if(ret!=0)
	{
		wtk_debug("expected bias failed\n");
		goto end;
	}
	conv->ab->b=wtk_knn_read_vector(src,buf);
	//wtk_debug("conv bias len=%d\n",conv->ab->b->len);
	//wtk_debug("%f %f %f %f\n",conv->ab->b->p[0],conv->ab->b->p[1],conv->ab->b->p[2],conv->ab->b->p[3]);
	if(!conv->ab->b)
	{
		wtk_debug("read bias failed\n");
		ret=-1;goto end;
	}
	ret=wtk_source_seek_to_s(src,"</ConvolutionPadComponent>");
	if(ret!=0)
	{
		wtk_debug("expected /ConvolutionPadComponent failed\n");
		goto end;
	}
	int num_x_steps = (1+(conv->input_x_dim-conv->filter_x_dim)/conv->filter_x_step);
	int num_y_steps = conv->num_y_step;
	int filter_dim = conv->ab->a->col;
	int nframe= 1;
	int* map;
	map=wtk_knn_conv_get_input2inputpatches_map(conv,nframe,num_x_steps*num_y_steps*filter_dim);
	conv->column_map = map;
	ret=0;
end:
	return ret;
}

static int wtk_knn_lstm_load(wtk_knn_lstm_t *lstm,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	ret=wtk_source_seek_to_s(src,"<Params>");
	if(ret!=0)
	{
		wtk_debug("expected Params failed\n");
		goto end;
	}
	lstm->params=wtk_knn_read_matrix(src,buf);
	ret=wtk_source_seek_to_s(src,"</LstmNonlinearityComponent>");
	if(ret!=0)
	{
		wtk_debug("expected /LstmNonlinearityComponent failed\n");
		goto end;
	}
end:
	return ret;
}

static int wtk_knn_backproptrunc_load(wtk_knn_backproptrunc_t *bp_trunc,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	ret=wtk_source_seek_to_s(src,"<Dim>");
	if(ret!=0)
	{
		wtk_debug("expected Dim failed\n");
		goto end;
	}
	ret=wtk_source_read_int(src,&(bp_trunc->dim),1,0);
	ret=wtk_source_seek_to_s(src,"<Scale>");
	if(ret!=0)
	{
		wtk_debug("expected Scale failed\n");
		goto end;
	}
	ret=wtk_source_read_float(src,&(bp_trunc->scale),1,0);
	ret=wtk_source_seek_to_s(src,"</BackpropTruncationComponent>");
	if(ret!=0)
	{
		wtk_debug("expected /BackpropTruncationComponent failed\n");
		goto end;
	}
end:
	return ret;
}

static int wtk_knn_scale_offset_load(wtk_knn_scale_offset_t* so,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	ret=wtk_source_seek_to_s(src,"<Dim>");
	if(ret!=0)
	{
		wtk_debug("expected Dim failed\n");
		goto end;
	}
	ret=wtk_source_read_int(src,&(so->dim),1,0);
	ret=wtk_source_seek_to_s(src,"<Scales>");
	if(ret!=0)
	{
		wtk_debug("expected Scales failed\n");
		goto end;
	}
	so->scale=wtk_knn_read_vector(src,buf);
	ret=wtk_source_seek_to_s(src,"<Offsets>");
	if(ret!=0)
	{
		wtk_debug("expected Offsets failed\n");
		goto end;
	}
	so->offset=wtk_knn_read_vector(src,buf);
	ret=wtk_source_seek_to_s(src,"</ScaleAndOffsetComponent>");
	if(ret!=0)
	{
		wtk_debug("expected /ScaleAndOffsetComponent failed\n");
		goto end;
	}
end:
	return ret;

}

static int wtk_knn_noop_load(wtk_knn_noop_t* noop,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	ret=wtk_source_seek_to_s(src,"<Dim>");
	if(ret!=0)
	{
		wtk_debug("expected Dim failed\n");
		goto end;
	}
	ret=wtk_source_read_int(src,&(noop->dim),1,0);

	ret=wtk_source_seek_to_s(src,"<BackpropScale>");
	if(ret!=0)
	{
		wtk_debug("expected BackpropScale failed\n");
		goto end;
	}
	ret=wtk_source_read_float(src,&(noop->backprop_scale),1,0);

	ret=wtk_source_seek_to_s(src,"</NoOpComponent>");
	if(ret!=0)
	{
		wtk_debug("expected /NoOpComponent failed\n");
		goto end;
	}

end:
	return ret;
}

static int wtk_knn_fsmn_load(wtk_knn_fsmn_t* fsmn,wtk_source_t *src,wtk_strbuf_t *buf) {
    int seek_end = 1;

    if (wtk_source_seek_to_s(src, "<LOrder>") ||
        wtk_source_read_int(src, &fsmn->lorder, 1, 0) ||
        wtk_source_seek_to_s(src, "<ROrder>") ||
        wtk_source_read_int(src, &fsmn->rorder, 1, 0) ||
        wtk_source_seek_to_s(src, "<LStride>") ||
        wtk_source_read_int(src, &fsmn->lstride, 1, 0) ||
        wtk_source_seek_to_s(src, "<RStride>") ||
        wtk_source_read_int(src, &fsmn->rstride, 1, 0) ||
        wtk_source_seek_to_s(src, "<Params>") ||
        (fsmn->params = wtk_knn_read_matrix(src, buf)) == NULL) {
        return -1;
    }

    wtk_source_read_string(src, buf);
    if (wtk_str_equal_s(buf->data, buf->pos, "<BiasParams>")) {
        fsmn->bias = wtk_knn_read_vector(src, buf);
    } else if (wtk_str_equal_s(buf->data, buf->pos,
                               "</CompactFsmnComponent>")) {
        seek_end = 0;
    }

    if (seek_end && wtk_source_seek_to_s(src, "</CompactFsmnComponent>")) {
        return -1;
    }

    wtk_strbuf_reset(buf);
    wtk_strbuf_push(buf, (char *)fsmn->params->p, sizeof(float) *fsmn->params->col);
    memmove(fsmn->params->p, fsmn->params->p + fsmn->params->col, fsmn->params->col * fsmn->lorder * sizeof(float));
    memcpy(fsmn->params->p + fsmn->params->col * fsmn->lorder, buf->data, buf->pos);

    return 0;
}

static int wtk_knn_general_dropout_load(wtk_knn_general_dropout_t* gdrop_out,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	ret=wtk_source_seek_to_s(src,"<Dim>");
	if(ret!=0)
	{
		wtk_debug("expected Dim failed\n");
		goto end;
	}
	ret=wtk_source_read_int(src,&(gdrop_out->dim),1,0);

	ret=wtk_source_seek_to_s(src,"<BlockDim>");
	if(ret!=0)
	{
		wtk_debug("expected BlockDim failed\n");
		goto end;
	}
	ret=wtk_source_read_int(src,&(gdrop_out->block_dim),1,0);

	ret=wtk_source_seek_to_s(src,"<TimePeriod>");
	if(ret!=0)
	{
		wtk_debug("expected TimePeriod failed\n");
		goto end;
	}
	ret=wtk_source_read_float(src,&(gdrop_out->time_period),1,0);

	ret=wtk_source_seek_to_s(src,"<DropoutProportion>");
	if(ret!=0)
	{
		wtk_debug("expected DropoutProportion failed\n");
		goto end;
	}
	ret=wtk_source_read_float(src,&(gdrop_out->dropout_proportion),1,0);

	/*ret=wtk_source_seek_to_s(src,"<TestMode>");
	if(ret!=0)
	{
		wtk_debug("expected TestMode failed\n");
		goto end;
	}*/
	ret=wtk_source_seek_to_s(src,"<Continuous>");
	if(ret!=0)
	{
		wtk_debug("expected Continuous failed\n");
		goto end;
	}
	ret=wtk_source_seek_to_s(src,"</GeneralDropoutComponent>");
	if(ret!=0)
	{
		wtk_debug("expected /GeneralDropoutComponent failed\n");
		goto end;
	}
end:
	return ret;
}

static int wtk_knn_affine_load(wtk_knn_affine_t *l,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;

	ret=wtk_source_seek_to_s(src,"<LinearParams>");
	if(ret!=0)
	{
		wtk_debug("expected params failed\n");
		goto end;
	}
	l->ab=wtk_knn_ab_new();
	l->ab->a=wtk_knn_read_matrix(src,buf);
	if(!l->ab->a)
	{
		wtk_debug("read params failed\n");
		ret=-1;goto end;
	}
	ret=wtk_source_expect_string_s(src,buf,"<BiasParams>");
	if(ret!=0)
	{
		wtk_debug("expected bias failed\n");
		goto end;
	}
	l->ab->b=wtk_knn_read_vector(src,buf);
	if(!l->ab->b)
	{
		wtk_debug("read bias failed\n");
		ret=-1;goto end;
	}
	ret=wtk_source_expect_string_s(src,buf,"</AffineComponent>");
	if(ret!=0)
	{
		wtk_debug("expected /AffineComponent failed\n");
		goto end;
	}
	//wtk_debug("l=%p %d/%d/%d\n",l,l->a->row,l->a->col,l->b->len);
	ret=0;
end:
	return ret;
}

static int wtk_knn_fixed_affine_load(wtk_knn_fixed_affine_t *l,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;

	ret=wtk_source_expect_string_s(src,buf,"<LinearParams>");
	if(ret!=0)
	{
		wtk_debug("expected params failed\n");
		goto end;
	}
	l->ab=wtk_knn_ab_new();
	l->ab->a=wtk_knn_read_matrix(src,buf);
	if(!l->ab->a)
	{
		wtk_debug("read params failed\n");
		ret=-1;goto end;
	}
	ret=wtk_source_expect_string_s(src,buf,"<BiasParams>");
	if(ret!=0)
	{
		wtk_debug("expected bias failed\n");
		goto end;
	}
	l->ab->b=wtk_knn_read_vector(src,buf);
	if(!l->ab->b)
	{
		wtk_debug("read bias failed\n");
		ret=-1;goto end;
	}
	ret=wtk_source_expect_string_s(src,buf,"</FixedAffineComponent>");
	if(ret!=0)
	{
		wtk_debug("expected /FixedAffineComponent failed\n");
		goto end;
	}
	ret=0;
end:
	//wtk_debug("%d/%d\n",l->a->row,l->a->col);
	//exit(0);
	return ret;
}

static int wtk_knn_normalize_load(wtk_knn_normalize_t *l,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;

	ret=wtk_source_seek_to_s(src,"<TargetRms>");
	if(ret!=0)
	{
		wtk_debug("expected params failed\n");
		goto end;
	}
	ret=wtk_source_read_float(src,&(l->rms),1,0);
	if(ret!=0)
	{
		wtk_debug("expected params failed\n");
		goto end;
	}
	ret=wtk_source_seek_to_s(src,"<AddLogStddev>");
	if(ret!=0)
	{
		wtk_debug("expected params failed\n");
		goto end;
	}
	ret=wtk_source_read_string3(src,buf);
	if(ret!=0)
	{
		wtk_debug("expected params failed\n");
		goto end;
	}
	if(buf->data[0]=='F')
	{
		l->add_log_stddev=0;
	}else if(buf->data[0]=='T')
	{
		l->add_log_stddev=1;
	}
	ret=wtk_source_seek_to_s(src,"</NormalizeComponent>");
	//ret=wtk_source_expect_string_s(src,buf,"</FixedAffineComponent>");
	if(ret!=0)
	{
		wtk_debug("expected /NormalizeComponent failed\n");
		goto end;
	}
end:
	return ret;
}

static int wtk_knn_ng_mask_affine_load(wtk_knn_ng_mask_affine_t *l,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	wtk_matf_t *b;

	ret=wtk_source_seek_to_s(src,"<LinearParams>");
	if(ret!=0)
	{
		wtk_debug("expected params failed\n");
		goto end;
	}
	l->a=wtk_knn_read_matrix(src,buf);
	if(!l->a)
	{
		wtk_debug("read params failed\n");
		ret=-1;goto end;
	}
	ret=wtk_source_seek_to_s(src,"<LinearParamsMask>");
	if(ret!=0)
	{
		wtk_debug("expected params failed\n");
		goto end;
	}
	b=wtk_knn_read_matrix(src,buf);
	if(!b)
	{
		wtk_debug("read params failed\n");
		ret=-1;goto end;
	}
	wtk_matf_dot(l->a,b);
	wtk_matf_delete(b);
	ret=wtk_source_seek_to_s(src,"<BiasParams>");
	if(ret!=0)
	{
		wtk_debug("expected bias failed\n");
		goto end;
	}
	l->b=wtk_knn_read_vector(src,buf);
	if(!l->b)
	{
		wtk_debug("read bias failed\n");
		ret=-1;goto end;
	}
	ret=wtk_source_seek_to_s(src,"</NaturalGradientAffineComponent>");
	//ret=wtk_source_expect_string_s(src,buf,"</FixedAffineComponent>");
	if(ret!=0)
	{
		wtk_debug("expected /NaturalGradientAffineComponent failed\n");
		goto end;
	}
	ret=0;
end:
	//wtk_debug("%d/%d\n",l->a->row,l->a->col);
	//exit(0);
	return ret;
}

static int wtk_knn_ng_affine_load(wtk_knn_ng_affine_t *l,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;

	ret=wtk_source_seek_to_s(src,"<LinearParams>");
	if(ret!=0)
	{
		wtk_debug("expected params failed\n");
		goto end;
	}
	l->ab=wtk_knn_ab_new();
	l->ab->a=wtk_knn_read_matrix(src,buf);
	if(!l->ab->a)
	{
		wtk_debug("read params failed\n");
		ret=-1;goto end;
	}
	ret=wtk_source_expect_string_s(src,buf,"<BiasParams>");
	if(ret!=0)
	{
		wtk_debug("expected bias failed\n");
		goto end;
	}
	l->ab->b=wtk_knn_read_vector(src,buf);
	if(!l->ab->b)
	{
		wtk_debug("read bias failed\n");
		ret=-1;goto end;
	}
	ret=wtk_source_seek_to_s(src,"</NaturalGradientAffineComponent>");
	//ret=wtk_source_expect_string_s(src,buf,"</FixedAffineComponent>");
	if(ret!=0)
	{
		wtk_debug("expected /NaturalGradientAffineComponent failed\n");
		goto end;
	}
	ret=0;
end:
	//wtk_debug("%d/%d\n",l->a->row,l->a->col);
	//exit(0);
	return ret;
}



static int wtk_rectified_linear_load(wtk_knn_fixed_affine_t *l,wtk_source_t *src,wtk_strbuf_t *buf)
{
    unuse(l);
    unuse(buf);
	int ret;

	ret=wtk_source_seek_to_s(src,"</RectifiedLinearComponent>");
	if(ret!=0)
	{
		wtk_debug("expected /RectifiedLinearComponent failed\n");
		goto end;
	}
	ret=0;
end:
	return ret;
}


static int wtk_log_softmax_load(wtk_log_softmax_t *l,wtk_source_t *src,wtk_strbuf_t *buf)
{
    unuse(l);
    unuse(buf);
	int ret;

	ret=wtk_source_seek_to_s(src,"</LogSoftmaxComponent>");
	if(ret!=0)
	{
		wtk_debug("expected /RectifiedLinearComponent failed\n");
		goto end;
	}
	ret=0;
end:
	return ret;
}

static int wtk_knn_sigmoid_load(wtk_sigmoid_t *l,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
    unuse(l);
    unuse(buf);

	ret=wtk_source_seek_to_s(src,"</SigmoidComponent>");
	if(ret!=0)
	{
		wtk_debug("expected /RectifiedLinearComponent failed\n");
		goto end;
	}
	ret=0;
end:
	return ret;
}




static int wtk_batch_norm_load(wtk_batch_norm_t *l,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	int dim,block_dim,count;
	float eps,target_rms;
	wtk_vecf_t *mean,*var;
	int i;

	ret=wtk_source_expect_string_s(src,buf,"<Dim>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&dim,1,0);
	if(ret!=0){goto end;}
	l->dim = dim;
	ret=wtk_source_expect_string_s(src,buf,"<BlockDim>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&block_dim,1,0);
	if(ret!=0){goto end;}
	ret=wtk_source_expect_string_s(src,buf,"<Epsilon>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_float(src,&eps,1,0);
	if(ret!=0){goto end;}
	ret=wtk_source_expect_string_s(src,buf,"<TargetRms>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_float(src,&target_rms,1,0);
	if(ret!=0){goto end;}
	ret=wtk_source_expect_string_s(src,buf,"<TestMode>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_string(src,buf);
	if(ret!=0){goto end;}
//	if(buf->data[0]=='F')
//	{
//		test_mode=0;
//	}else
//	{
//		test_mode=1;
//	}
	ret=wtk_source_expect_string_s(src,buf,"<Count>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&count,1,0);
	if(ret!=0){goto end;}
	ret=wtk_source_expect_string_s(src,buf,"<StatsMean>");
	if(ret!=0){goto end;}
	mean=wtk_knn_read_vector(src,buf);
	if(!mean){goto end;}
	ret=wtk_source_expect_string_s(src,buf,"<StatsVar>");
	if(ret!=0){goto end;}
	var=wtk_knn_read_vector(src,buf);
	if(!var){goto end;}
	ret=wtk_source_expect_string_s(src,buf,"</BatchNormComponent>");
	if(ret!=0){goto end;}
	//wtk_debug("dim=%d/%d eps=%f target_rms=%f count=%d test=%d\n",dim,block_dim,eps,target_rms,count,test_mode);
	//print_float(mean->p,10);
	//print_float(var->p,10);
	for(i=0;i<var->len;++i)
	{
		if(var->p[i]<0)
		{
			var->p[i]=0;
		}
		var->p[i]+=eps;
		var->p[i]=target_rms/sqrt(var->p[i]);
		mean->p[i]*=-var->p[i];
	}
	l->scale=var;
	l->offset=mean;
	//print_float(mean->p,10);
	//print_float(var->p,10);
	ret=0;
end:
	//wtk_debug("ret=%d\n",ret);
	//exit(0);
	return ret;
}

int wtk_knn_static_extraction_load(wtk_knn_static_extraction_t *l,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	int tmp;
	wtk_strbuf_t *b;

	b=wtk_strbuf_new(50,0);

	ret=wtk_source_expect_string_s(src,buf,"<InputDim>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&tmp,1,0);
	if(ret!=0){goto end;}
	l->input_dim=tmp;

	ret=wtk_source_expect_string_s(src,buf,"<InputPeriod>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&tmp,1,0);
	if(ret!=0){goto end;}
	l->input_period=tmp;

	ret=wtk_source_expect_string_s(src,buf,"<OutputPeriod>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&tmp,1,0);
	if(ret!=0){goto end;}
	l->output_period=tmp;

	ret=wtk_source_expect_string_s(src,buf,"<IncludeVarinance>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_string(src,b);
	if(ret!=0){goto end;}
	if(strncmp(b->data,"T",1)==0)
	{
		l->include_var=1;
	}else
	{
		l->include_var=0;
	}
	ret=wtk_source_seek_to_s(src,"</StatisticsExtractionComponent>");

	ret=0;
end:
	wtk_strbuf_delete(b);
	return ret;
}

int wtk_knn_static_pooling_load(wtk_knn_static_pooling_t *l,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	int tmp;
	float tmp2;
	wtk_strbuf_t *b;

	b=wtk_strbuf_new(50,0);

	ret=wtk_source_expect_string_s(src,buf,"<InputDim>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&tmp,1,0);
	if(ret!=0){goto end;}
	l->input_dim=tmp;

	ret=wtk_source_expect_string_s(src,buf,"<InputPeriod>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&tmp,1,0);
	if(ret!=0){goto end;}

	ret=wtk_source_expect_string_s(src,buf,"<LeftContext>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&tmp,1,0);
	if(ret!=0){goto end;}


	ret=wtk_source_expect_string_s(src,buf,"<RightContext>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&tmp,1,0);
	if(ret!=0){goto end;}

	ret=wtk_source_expect_string_s(src,buf,"<NumLogCountFeatures>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,&tmp,1,0);
	if(ret!=0){goto end;}
	l->numlog_count_features=tmp;

	ret=wtk_source_expect_string_s(src,buf,"<OutputStddevs>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_string(src,b);
	if(ret!=0){goto end;}
	if(strncmp(b->data,"T",1)==0)
	{
		l->output_stddevs=1;
	}else
	{
		l->output_stddevs=0;
	}

	ret=wtk_source_expect_string_s(src,buf,"<VarianceFloor>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_float(src,&tmp2,1,0);
	if(ret!=0){goto end;}
	l->variance_floor=tmp2;

	ret=wtk_source_seek_to_s(src,"</StatisticsPoolingComponent>");

	ret=0;
end:
	wtk_strbuf_delete(b);
	return ret;
}

static int wtk_knn_layer_load(wtk_knn_layer_t *layer,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;

	switch(layer->type)
	{
	case WTK_LinearMaskComponent:
		layer->v.linear_mask=wtk_knn_linear_mask_new();
		ret=wtk_knn_linear_mask_load(layer->v.linear_mask,src,buf);
		break;
	case WTK_LinearComponent:
		layer->v.linear=wtk_knn_linear_new();
		ret=wtk_knn_linear_load(layer->v.linear,src,buf);
		break;
	case WTK_AffineComponent:
		layer->v.affine=wtk_knn_affine_new();
		ret=wtk_knn_affine_load(layer->v.affine,src,buf);
		break;
	case WTK_FixedAffineComponent:
		layer->v.fixed_affine=wtk_knn_fixed_affine_new();
		ret=wtk_knn_fixed_affine_load(layer->v.fixed_affine,src,buf);
		break;
	case WTK_NaturalGradientAffineComponent:
		layer->v.ng_affine=wtk_knn_ng_affine_new();
		ret=wtk_knn_ng_affine_load(layer->v.ng_affine,src,buf);
		break;
	case WTK_NaturalGradientAffineMaskComponent:
		layer->v.ng_mask_affine=wtk_knn_ng_mask_affine_new();
		ret=wtk_knn_ng_mask_affine_load(layer->v.ng_mask_affine,src,buf);
		break;
	case WTK_RectifiedLinearComponent:
		layer->v.rectified_linear=NULL;
		ret=wtk_rectified_linear_load(NULL,src,buf);
		break;
	case WTK_BatchNormComponent:
		layer->v.batch_norm=wtk_batch_norm_new();
		ret=wtk_batch_norm_load(layer->v.batch_norm,src,buf);
		break;
	case WTK_LogSoftmaxComponent:
		layer->v.log_softmax=NULL;
		ret=wtk_log_softmax_load(NULL,src,buf);
		break;
	case WTK_SigmoidComponent:
		layer->v.sigmoid=NULL;
		ret=wtk_knn_sigmoid_load(NULL,src,buf);
		break;
	case WTK_NormalizeComponent:
		layer->v.normalize=wtk_knn_normalize_new();
		ret=wtk_knn_normalize_load(layer->v.normalize,src,buf);
		break;
	case WTK_ConvolutionComponent:
		layer->v.conv=wtk_knn_conv_new();
		// wtk_debug("conv %.*s\n",layer->name->len,layer->name->data);
		ret=wtk_knn_conv_load(layer->v.conv,src,buf);
		break;
	case WTK_LstmNonlinearityComponent:
		layer->v.lstm=wtk_knn_lstm_new();
		ret=wtk_knn_lstm_load(layer->v.lstm,src,buf);
		break;
	case WTK_BackpropTruncationComponent:
		layer->v.bp_trunc=wtk_knn_backproptrunc_new();
		ret=wtk_knn_backproptrunc_load(layer->v.bp_trunc,src,buf);
		break;
	case WTK_ScaleAndOffsetComponent:
		layer->v.so=wtk_knn_scale_offset_new();
		ret=wtk_knn_scale_offset_load(layer->v.so,src,buf);
		break;
	case WTK_GeneralDropoutComponent:
		layer->v.gdrop_out=wtk_knn_general_dropout_new();
		ret=wtk_knn_general_dropout_load(layer->v.gdrop_out,src,buf);
		break;
	case WTK_NoOpComponent:
		layer->v.noop=wtk_knn_noop_new();
		ret=wtk_knn_noop_load(layer->v.noop,src,buf);
		break;
    case WTK_CompactFsmnComponent:
        layer->v.fsmn = wtk_knn_fsmn_new();
        ret = wtk_knn_fsmn_load(layer->v.fsmn, src, buf);
        break;
	case WTK_StatisticsExtractionComponent:
		layer->v.static_extraction=wtk_knn_static_extraction_new();
		ret=wtk_knn_static_extraction_load(layer->v.static_extraction,src,buf);
		break;
	case WTK_StatisticsPoolingComponent:
		layer->v.static_pooling=wtk_knn_static_pooling_new();
		ret=wtk_knn_static_pooling_load(layer->v.static_pooling,src,buf);
		break;
	default:
		wtk_debug("not found load\n");
		exit(0);
		ret=-1;
		break;
	}
	return ret;
}

static void wtk_knn_cfg_delete_array(wtk_larray_t *a)
{
	wtk_string_t **v;
	int i;

	v=(wtk_string_t**)(a->slot);
	for(i=0;i<a->nslot;++i)
	{
		wtk_string_delete(v[i]);
	}
	wtk_larray_delete(a);
}

static wtk_larray_t* wtk_knn_cfg_parse(char *data,int len)
{
	char *s,*e;
	wtk_larray_t *a;
	char *xs=NULL;
	wtk_string_t *v;

	a=wtk_larray_new(10,sizeof(wtk_string_t*));
	s=data;e=s+len;
	while(s<e)
	{
		//wtk_debug("%c\n",*s);
		if(!xs)
		{
			if(!isspace(*s))
			{
				if(*s=='='||*s=='('||*s==','||*s==')')
				{
					v=wtk_string_dup_data(s,1);
					wtk_larray_push2(a,&v);
				}else
				{
					xs=s;
				}
			}
		}else
		{
			if(*s=='=' || *s=='('||*s==','||*s==')')
			{
				v=wtk_string_dup_data(xs,s-xs);
				wtk_larray_push2(a,&v);
				v=wtk_string_dup_data(s,1);
				wtk_larray_push2(a,&v);
				xs=NULL;
			}else if(isspace(*s))
			{
				//wtk_debug("[%.*s]\n",(int)(s-xs),xs);
				v=wtk_string_dup_data(xs,s-xs);
				wtk_larray_push2(a,&v);
				xs=NULL;
			}else if((s+1)>=e)
			{
				//wtk_debug("[%.*s]\n",(int)(s-xs)+1,xs);
				v=wtk_string_dup_data(xs,s-xs+1);
				wtk_larray_push2(a,&v);
				xs=NULL;
			}
		}
		++s;
	}
	return a;
}




void wtk_knn_cmd_v_to_string2(wtk_knn_cmd_v_t *v,wtk_strbuf_t *buf);

static void wtk_knn_cmd_to_string(wtk_knn_cmd_t *cmd,wtk_strbuf_t *buf)
{
	int i;

	switch(cmd->type)
	{
	case WTK_KNN_APPEND:
		wtk_strbuf_push_s(buf,"Append(");
		break;
	case WTK_KNN_OFFSET:
		wtk_strbuf_push_s(buf,"Offset(");
		break;
	case WTK_KNN_IFDEFINED:
		wtk_strbuf_push_s(buf,"IfDefined(");
		break;
	case WTK_KNN_SUM:
		wtk_strbuf_push_s(buf,"Sum(");
		break;
	case WTK_KNN_SCALE:
		wtk_strbuf_push_s(buf,"Scale(");
		break;
	case WTK_KNN_ROUND:
		wtk_strbuf_push_s(buf,"Round(");
		break;
	case WTK_KNN_REPLACE:
		wtk_strbuf_push_s(buf,"ReplaceIndex(");
		break;	
	default:
		break;
	}
	//wtk_debug("[%.*s] nv=%d v=%p\n",buf->pos,buf->data,cmd->nv,cmd->v);
	for(i=0;i<cmd->nv;++i)
	{
		//wtk_debug("i=%d type=%d\n",i,cmd->v[i]->type);
		if(i>0)
		{
			wtk_strbuf_push_s(buf,",");
		}
		wtk_knn_cmd_v_to_string2(cmd->v[i],buf);
	}
	wtk_strbuf_push_s(buf,")");
}

void wtk_knn_cmd_print(wtk_knn_cmd_t *cmd)
{
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(1024,1);
	wtk_knn_cmd_to_string(cmd,buf);
	wtk_debug("%.*s\n",buf->pos,buf->data);
	wtk_strbuf_delete(buf);
}

void wtk_knn_cmd_v_to_string2(wtk_knn_cmd_v_t *v,wtk_strbuf_t *buf)
{
	switch(v->type)
	{
	case WTK_KNN_CMD_V_I:
		wtk_strbuf_push_f(buf,"%d",v->v.i);
		break;
	case WTK_KNN_CMD_V_STRING:
		wtk_strbuf_push(buf,v->v.str->data,v->v.str->len);
		break;
	case WTK_KNN_CMD_V_CMD:
		wtk_knn_cmd_to_string(v->v.cmd,buf);
		break;
	case WTK_KNN_CMD_V_F:
		break;
	default:
		break;
	}
}

static wtk_strbuf_t* wtk_knn_cmd_v_to_string(wtk_knn_cmd_v_t *v)
{
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(1024,1);
	wtk_knn_cmd_v_to_string2(v,buf);
	return buf;
}

void wtk_knn_cmd_v_print(wtk_knn_cmd_v_t *v)
{
	wtk_strbuf_t *buf;

	buf=wtk_knn_cmd_v_to_string(v);
	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	wtk_strbuf_delete(buf);
}

static void wtk_knn_logic_layer_print(wtk_knn_logic_layer_t *layer)
{
	wtk_strbuf_t *buf;

	buf=wtk_knn_cmd_v_to_string(&(layer->input));\
	if(layer->component)
	{
		printf("name=%.*s component=%.*s input=%.*s\n",layer->name->len,layer->name->data,
			layer->component->len,layer->component->data,buf->pos,buf->data);
	}else
	{
		printf("name=%.*s input=%.*s\n",layer->name->len,layer->name->data,buf->pos,buf->data);
	}
	
	wtk_strbuf_delete(buf);
}

static wtk_knn_cmd_t* wtk_knn_cmd_new(wtk_knn_cmd_type_t type)
{
	wtk_knn_cmd_t *cmd;

	cmd=(wtk_knn_cmd_t*)wtk_malloc(sizeof(wtk_knn_cmd_t));
	cmd->type=type;
	cmd->nv=0;
	cmd->v=NULL;
	return cmd;
}

typedef struct
{
	wtk_knn_cmd_t *cmd;
	wtk_larray_t *v;
}wtk_knn_cmd_item_t;

void wtk_knn_cfg_print_logic(wtk_knn_cfg_t *cfg)
{
	wtk_knn_logic_layer_t *layer;
	int i;

	for(i=0;i<cfg->nlogic_layer-1;++i)
	{
		layer=cfg->logic_layer[i];
		//wtk_debug("v[%d]=[%.*s]\n",i,layer->component->len,layer->component->data);
		//component-node name=lda component=lda input=Append(Offset(input, -1), input, Offset(input, 1))
		wtk_knn_logic_layer_print(layer);
	}
}


static int wtk_knn_cfg_load_config(wtk_knn_cfg_t *cfg,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	int nl;
	wtk_larray_t *a;
	wtk_string_t **strs,*key;
	int b,i,is_input,depth,c;
	wtk_knn_logic_layer_t *layer;
	wtk_larray_t *stack;
	wtk_knn_cmd_t *cur_cmd;
	wtk_larray_t *cur_cmd_v;
	wtk_knn_cmd_item_t *item;
	wtk_knn_cmd_v_t *cmdv;
	wtk_larray_t *layers=NULL;

	cur_cmd_v=NULL;
	layers=wtk_larray_new(10,sizeof(void*));
	wtk_source_skip_sp(src,NULL);
	while(1)
	{
		nl=0;
		wtk_source_skip_sp(src,&nl);
		if(nl){break;}
		ret=wtk_source_read_line(src,buf);
		if(ret!=0){goto end;}
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		a=wtk_knn_cfg_parse(buf->data,buf->pos);
		//wtk_larray_print_string(a);
		strs=(wtk_string_t**)(a->slot);
		if(wtk_string_cmp_s(strs[0],"input-node")==0)
		{
			b=0;
			c=0;
			for(i=0;i<a->nslot;++i)
			{
				// wtk_debug("v[%d]=[%.*s]\n",i,strs[i]->len,strs[i]->data);
				if(wtk_string_cmp_s(strs[i],"ivector")==0)
				{
					c=1;
				}else if(wtk_string_cmp_s(strs[i],"input")==0)
				{
					c=0;
				}
				if(b==0)
				{
					if(wtk_string_cmp_s(strs[i],"dim")==0)
					{
						b=1;
					}
				}else if(b==1)
				{
					if(wtk_string_cmp_s(strs[i],"=")==0)
					{
						b=2;
					}
				}else if(b==2 && c==0)
				{
					cfg->input_dim=wtk_str_atoi(strs[i]->data,strs[i]->len);
					b=0;
				}else if(b==2 && c==1)
				{
					cfg->ivector_dim=wtk_str_atoi(strs[i]->data,strs[i]->len);
					b=0;
				}
			}
			//wtk_debug("dim=%d\n",cfg->input_dim);
			//exit(0);
		}else if(wtk_string_cmp_s(strs[0],"component-node")==0 || wtk_string_cmp_s(strs[0],"output-node")==0 ||wtk_string_cmp_s(strs[0],"dim-range-node")==0)
		{
			key=NULL;
			b=is_input=depth=0;
			if(wtk_string_cmp_s(strs[0],"dim-range-node")!=0)
			{
				layer=wtk_knn_logic_layer_new();
			}else
			{
				wtk_knn_logic_layer_t **l;
				l=(wtk_knn_logic_layer_t**)wtk_larray_get(layers,layers->nslot-1);
				layer=*l;
				layer->range_layer=wtk_knn_range_layer_new();
			}
			stack=NULL;
			cur_cmd=NULL;
			for(i=1;i<a->nslot;++i)
			{
				//wtk_debug("v[%d]=[%.*s]\n",i,strs[i]->len,strs[i]->data);
				if(b==0)
				{
					b=1;
					key=strs[i];
					if(wtk_string_cmp_s(key,"input")==0)
					{
						is_input=1;
					}else
					{
						is_input=0;
					}
				}else if(b==1)
				{
					if(wtk_string_cmp_s(strs[i],"=")==0)
					{
						b=2;
					}
				}else if(b==2)
				{
					//wtk_debug("[%.*s]=%d depth=%d\n",key->len,key->data,is_input,depth);
					if(is_input)
					{
						if(wtk_string_cmp_s(strs[i],"Round")==0)
						{
							if(depth==0)
							{
								layer->input.type=WTK_KNN_CMD_V_CMD;
								cur_cmd=layer->input.v.cmd=wtk_knn_cmd_new(WTK_KNN_ROUND);
								cur_cmd_v=wtk_larray_new(10,sizeof(void*));
								stack=wtk_larray_new(10,sizeof(wtk_knn_cmd_t*));
							}else
							{
								exit(0);
							}
						}else if(wtk_string_cmp_s(strs[i],"Append")==0)
						{
							if(depth==0)
							{
								layer->input.type=WTK_KNN_CMD_V_CMD;
								cur_cmd=layer->input.v.cmd=wtk_knn_cmd_new(WTK_KNN_APPEND);
								cur_cmd_v=wtk_larray_new(10,sizeof(void*));
								stack=wtk_larray_new(10,sizeof(wtk_knn_cmd_t*));
							}else
							{
								exit(0);
							}
						}else if(wtk_string_cmp_s(strs[i],"Sum")==0)
						{
							if(depth==0)
							{
								layer->input.type=WTK_KNN_CMD_V_CMD;
								cur_cmd=layer->input.v.cmd=wtk_knn_cmd_new(WTK_KNN_SUM);
								cur_cmd_v=wtk_larray_new(10,sizeof(void*));
								stack=wtk_larray_new(10,sizeof(wtk_knn_cmd_t*));
							}else
							{
								wtk_debug("descriptor sum depth wrong!\n");
								exit(0);
							}
						}else if(wtk_string_cmp_s(strs[i],"Scale")==0)
						{
							if(cur_cmd)
							{
								item=(wtk_knn_cmd_item_t*)wtk_malloc(sizeof(wtk_knn_cmd_item_t));
								item->cmd=cur_cmd;
								item->v=cur_cmd_v;
								// wtk_debug("item=%p/%p %p/%p\n",item,&item,item->cmd,item->v);
								wtk_larray_push2(stack,&item);
							}
							cur_cmd=wtk_knn_cmd_new(WTK_KNN_SCALE);
							cur_cmd_v=wtk_larray_new(10,sizeof(void*));
						}else if(wtk_string_cmp_s(strs[i],"IfDefined")==0)
						{
							if(cur_cmd)
							{
								item=(wtk_knn_cmd_item_t*)wtk_malloc(sizeof(wtk_knn_cmd_item_t));
								item->cmd=cur_cmd;
								item->v=cur_cmd_v;
								// wtk_debug("item=%p/%p %p/%p\n",item,&item,item->cmd,item->v);
								wtk_larray_push2(stack,&item);
							}
							cur_cmd=wtk_knn_cmd_new(WTK_KNN_IFDEFINED);
							cur_cmd_v=wtk_larray_new(10,sizeof(void*));
						}else if(wtk_string_cmp_s(strs[i],"Offset")==0)
						{
							if(cur_cmd)
							{
								item=(wtk_knn_cmd_item_t*)wtk_malloc(sizeof(wtk_knn_cmd_item_t));
								item->cmd=cur_cmd;
								item->v=cur_cmd_v;
								//wtk_debug("item=%p/%p %p/%p\n",item,&item,item->cmd,item->v);
								wtk_larray_push2(stack,&item);
							}
							cur_cmd=wtk_knn_cmd_new(WTK_KNN_OFFSET);
							cur_cmd_v=wtk_larray_new(10,sizeof(void*));
							//wtk_debug("cmd=%p\n",cur_cmd);
						}else if(wtk_string_cmp_s(strs[i],"ReplaceIndex")==0)
						{
							if(cur_cmd)
							{
								item=(wtk_knn_cmd_item_t*)wtk_malloc(sizeof(wtk_knn_cmd_item_t));
								item->cmd=cur_cmd;
								item->v=cur_cmd_v;
								//wtk_debug("item=%p/%p %p/%p\n",item,&item,item->cmd,item->v);
								wtk_larray_push2(stack,&item);
							}
							cur_cmd=wtk_knn_cmd_new(WTK_KNN_REPLACE);
							cur_cmd_v=wtk_larray_new(10,sizeof(void*));
						}else if(wtk_string_cmp_s(strs[i],"(")==0)
						{
							++depth;
						}else if(wtk_string_cmp_s(strs[i],",")==0)
						{

						}else if(wtk_string_cmp_s(strs[i],")")==0)
						{
							//wtk_debug("n=%d\n",cur_cmd_v->nslot);
							--depth;
							//wtk_debug("depth=%d a=%d\n",depth,stack->nslot);
							cur_cmd->nv=cur_cmd_v->nslot;
							cur_cmd->v=(wtk_knn_cmd_v_t**)wtk_calloc(cur_cmd->nv,sizeof(wtk_knn_cmd_v_t*));
							memcpy(cur_cmd->v,cur_cmd_v->slot,cur_cmd_v->nslot*sizeof(wtk_knn_cmd_v_t*));
							//wtk_debug("n=%d %p\n",cur_cmd->nv,cur_cmd->v[0]);
							wtk_larray_delete(cur_cmd_v);
							//wtk_knn_cmd_print(cur_cmd);
							//exit(0);
							if(depth==0)
							{
							}else
							{
								item=*((wtk_knn_cmd_item_t**)wtk_larray_pop_back(stack));
								cmdv=(wtk_knn_cmd_v_t*)wtk_malloc(sizeof(wtk_knn_cmd_v_t));
								cmdv->type=WTK_KNN_CMD_V_CMD;
								cmdv->v.cmd=cur_cmd;
								wtk_larray_push2(item->v,&(cmdv));
								cur_cmd=item->cmd;
								cur_cmd_v=item->v;
								wtk_free(item);
							}
							//exit(0);
							//exit(0);
						}else
						{
							if(depth==0)
							{
								layer->input.type=WTK_KNN_CMD_V_STRING;
								layer->input.v.str=wtk_string_dup_data(strs[i]->data,strs[i]->len);
								// wtk_debug("%.*s\n",layer->input.v.str->len,layer->input.v.str->data);
								b=0;
							}else
							{
								cmdv=(wtk_knn_cmd_v_t*)wtk_malloc(sizeof(wtk_knn_cmd_v_t));
								// wtk_debug("%.*s\n",strs[i]->len,strs[i]->data);
								if((isdigit(strs[i]->data[0])&& strs[i]->len==1)||(strs[i]->data[0]=='-' && isdigit(strs[i]->data[1]))||(isdigit(strs[i]->data[0])&& strs[i]->len>1 &&strs[i]->data[1]!='.'))
								{
									cmdv->type=WTK_KNN_CMD_V_I;
									cmdv->v.i=wtk_str_atoi(strs[i]->data,strs[i]->len);
								}else if(isdigit(strs[i]->data[0])&&strs[i]->len>1&& strs[i]->data[1]=='.')
								{
									cmdv->type=WTK_KNN_CMD_V_F;
									cmdv->v.f=wtk_str_atof(strs[i]->data,strs[i]->len);
								}
								else
								{
									cmdv->type=WTK_KNN_CMD_V_STRING;
									cmdv->v.str=wtk_string_dup_data(strs[i]->data,strs[i]->len);
								}
								wtk_larray_push2(cur_cmd_v,&cmdv);
								//exit(0);
							}
						}
					}else
					{
						if(wtk_string_cmp_s(key,"name")==0)
						{
							layer->name=wtk_string_dup_data(strs[i]->data,strs[i]->len);
						}else if(wtk_string_cmp_s(key,"component")==0)
						{
							layer->component=wtk_string_dup_data(strs[i]->data,strs[i]->len);
						}else if(wtk_string_cmp_s(key,"objective")==0)
						{
							//layer->objective=wtk_string_dup_data(strs[i]->data,strs[i]->len);
						}else if(wtk_string_cmp_s(key,"input-node")==0)
						{
							layer->range_layer->input=wtk_string_dup_data(strs[i]->data,strs[i]->len);
							// wtk_debug("%.*s\n",layer->range_layer->input->len,layer->range_layer->input->data);
						}else if(wtk_string_cmp_s(key,"dim-offset")==0)
						{
							layer->range_layer->dim_offset=wtk_str_atoi(strs[i]->data,strs[i]->len);
							// wtk_debug("dim-offset=%d\n",layer->range_layer->dim_offset);
						}else if(wtk_string_cmp_s(key,"dim")==0)
						{
							layer->range_layer->dim=wtk_str_atoi(strs[i]->data,strs[i]->len);
							// wtk_debug("dim=%d\n",layer->range_layer->dim);
						}
						b=0;
					}
				}
			}
			if(stack)
			{
				wtk_larray_delete(stack);
			}
			// wtk_knn_logic_layer_print(layer);
			wtk_larray_push2(layers,&layer);
			//exit(0);
		}else
		{
			exit(0);
		}
		wtk_knn_cfg_delete_array(a);
		//exit(0);
	}
	ret=0;
	cfg->nlogic_layer=layers->nslot;
	cfg->logic_layer=(wtk_knn_logic_layer_t**)wtk_calloc(cfg->nlogic_layer,sizeof(wtk_knn_logic_layer_t*));
	memcpy(cfg->logic_layer,layers->slot,layers->nslot*sizeof(void*));
end:
	//wtk_knn_cfg_print_logic(cfg);
	wtk_larray_delete(layers);
	return ret;
}

static int wtk_knn_cfg_load_prior(wtk_knn_cfg_t *cfg,wtk_source_t *src,wtk_strbuf_t *buf)
{
	int ret;
	int i;

	ret=wtk_source_seek_to_s(src,"<Priors>");
	if(ret!=0)
	{
		wtk_debug("expected priors failed\n");
		goto end;
	}
	cfg->prior=wtk_knn_read_vector(src,buf);
	if(!cfg->prior)
	{
		wtk_debug("read prior failed\n");
		ret=-1;goto end;
	}
	for(i=0;i<cfg->prior->len;++i)
	{
		cfg->prior->p[i]=log(cfg->prior->p[i]);
//		if(cfg->prior->p[i]>-6.0)
//		{
//			wtk_debug("prior[%d]=%f\n",i,cfg->prior->p[i]);
//		}
		//exit(0);
	}
	ret=0;
end:
	//exit(0);
	return ret;
}

static int wtk_knn_cfg_load(wtk_knn_cfg_t *cfg,wtk_source_t *src)
{
	int ret;
	wtk_strbuf_t *buf;
	int v[3];
	int i;
	wtk_knn_layer_type_t type;

	buf=wtk_strbuf_new(256,1);
	ret=wtk_source_seek_to_s(src,"<Nnet3>");
	if(ret!=0){goto end;}
	ret=wtk_knn_cfg_load_config(cfg,src,buf);
	if(ret!=0){goto end;}
	ret=wtk_source_seek_to_s(src,"<NumComponents>");
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(src,v,1,0);
	if(ret!=0){goto end;}
	cfg->nlayer=v[0];
	// wtk_debug("n=%d\n",v[0]);
	cfg->layer=(wtk_knn_layer_t**)wtk_calloc(cfg->nlayer,sizeof(wtk_knn_layer_t*));
	for(i=0;i<cfg->nlayer;++i)
	{
		ret=wtk_source_expect_string_s(src,buf,"<ComponentName>");
		if(ret!=0){goto end;}
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){goto end;}
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		cfg->layer[i]=wtk_knn_layer_new(buf->data,buf->pos);
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){goto end;}
		// wtk_debug("[%.*s] [%.*s]\n",buf->pos,buf->data,cfg->layer[i]->name->len,cfg->layer[i]->name->data);
		type=wtk_knn_layer_type(buf->data+1,buf->pos-2);
		if((int)(type)==-1)
		{
			wtk_debug("[%.*s] not found\n",buf->pos,buf->data);
			ret=-1;
			goto end;
		}
		wtk_knn_layer_set_type(cfg->layer[i],type);
		ret=wtk_knn_layer_load(cfg->layer[i],src,buf);
		if(ret!=0){goto end;}
		//wtk_debug("output[%d/%.*s]=%d\n",i,cfg->layer[i]->name->len,cfg->layer[i]->name->data,wtk_knn_layer_get_output(cfg->layer[i]));
		//exit(0);
	}
	if(cfg->use_prior)
	{
		ret=wtk_knn_cfg_load_prior(cfg,src,buf);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	// wtk_debug("ret=%d\n",ret);
	//exit(0);
	wtk_strbuf_delete(buf);
	return ret;
}


static wtk_knn_layer_t* wtk_knn_cfg_find_layer(wtk_knn_cfg_t *cfg,char *name,int len)
{
	int i;

	for(i=0;i<cfg->nlayer;++i)
	{
		if(wtk_string_cmp(cfg->layer[i]->name,name,len)==0)
		{
			return cfg->layer[i];
		}
	}
	return NULL;
}



static wtk_knn_logic_layer_t* wtk_knn_cfg_find_logic_layer(wtk_knn_cfg_t *cfg,char *name,int len)
{
	int i;

	for(i=0;i<cfg->nlogic_layer-1;++i)
	{
		if(wtk_string_cmp(cfg->logic_layer[i]->name,name,len)==0)
		{
			return cfg->logic_layer[i];
		}
	}
	return NULL;
}

int wtk_knn_cfg_find_logic_layer2(wtk_knn_cfg_t *cfg,char *name,int len)
{
	int i;

	for(i=0;i<cfg->nlogic_layer;++i)
	{
		if(wtk_string_cmp(cfg->logic_layer[i]->name,name,len)==0)
		{
			return i;
		}
	}
	return (-1);
}

#define wtk_knn_cfg_find_logic_layer_s(cfg,s) wtk_knn_cfg_find_logic_layer(cfg,s,sizeof(s)-1)

static void wtk_knn_cfg_update_layers(wtk_knn_cfg_t *cfg,wtk_knn_logic_layer_t *affine,
		wtk_knn_logic_layer_t *ubm)
{
	wtk_knn_layer_t **layers;
	wtk_knn_layer_t *output1,*output2;
	int i,j,n;
	wtk_matf_t *m;
	wtk_vecf_t *b;
	wtk_knn_ab_t *a1;
	wtk_knn_ab_t *a2;
	//wtk_knn_fixed_affine_t *a1;
	//wtk_knn_affine_t *a2;
	float *pf1,*pf2;
	int row;

	output1=affine->layer;
	output2=ubm->layer;
	a1=output1->v.fixed_affine->ab;
	a2=output2->v.affine->ab;
	layers=cfg->layer;
	n=cfg->nlayer;
	cfg->nlayer=n-1;
	cfg->layer=(wtk_knn_layer_t**)wtk_calloc(cfg->nlayer,sizeof(wtk_knn_layer_t*));
	for(i=0,j=0;i<n;++i)
	{
		if(layers[i]==output2)
		{
			continue;
		}
		cfg->layer[j++]=layers[i];
	}
	m=wtk_matf_new(cfg->nmap,a1->a->col);
	b=wtk_vecf_new(cfg->nmap);
	pf1=m->p;
	for(i=0;i<cfg->nmap;++i)
	{
		//wtk_debug("v[%d]=%d/%d row=%d\n",i,cfg->map[i].id,cfg->map[i].knn_id,a1->a->row);
		if(cfg->map[i].id<a1->a->row)
		{
			row=cfg->map[i].id;
			//wtk_debug("row=%d\n",row);
			pf2=a1->a->p+row*m->col;
			b->p[i]=a1->b->p[row];
		}else
		{
			row=cfg->map[i].id-a1->a->row;
			//wtk_debug("row=%d\n",row);
			pf2=a2->a->p+row*m->col;
			b->p[i]=a2->b->p[row];
		}
		memcpy(pf1,pf2,m->col*sizeof(float));
		pf1+=m->col;
	}
	//exit(0);
	wtk_matf_delete(a1->a);
	wtk_vecf_delete(a1->b);
	a1->a=m;
	a1->b=b;
	wtk_knn_layer_delete(cfg, output2);
	wtk_free(layers);
	//wtk_debug("%d\n",cfg->prior->len);
	b=cfg->prior;
	cfg->prior=wtk_vecf_new(cfg->nmap);
	for(i=0;i<cfg->nmap;++i)
	{
		cfg->prior->p[i]=b->p[cfg->map[i].id];
	}
	wtk_vecf_delete(b);
	//exit(0);
}

static void wtk_knn_cfg_update_output(wtk_knn_cfg_t *cfg)
{
	int i,j;
	wtk_knn_logic_layer_t *output1,*output2,*softmax;
	wtk_knn_logic_layer_t **layers;
	int n;
	wtk_knn_input_t *input;

	wtk_debug("################# merge output\n");
//	for(i=0;i<cfg->nlogic_layer-1;++i)
//	{
//		wtk_debug("v[%d]=[%.*s] %s\n",i,cfg->logic_layer[i]->name->len,
//				cfg->logic_layer[i]->name->data,
//				wtk_knn_layer_type_str(cfg->logic_layer[i]->layer->type));
//	}
	//output1=wtk_knn_cfg_find_logic_layer_s(cfg,"output.affine.1");
	output1=wtk_knn_cfg_find_logic_layer_s(cfg,"output.affine");
	output2=wtk_knn_cfg_find_logic_layer_s(cfg,"output.ubm");
	layers=cfg->logic_layer;
	n=cfg->nlogic_layer;
	softmax=wtk_knn_cfg_find_logic_layer_s(cfg,"output.log-softmax");
	softmax->n_input=1;
	input=softmax->layer_input;
	softmax->layer_input=(wtk_knn_input_t*)wtk_calloc(softmax->n_input,sizeof(wtk_knn_input_t));
	for(i=0;i<n;++i)
	{
		if(cfg->logic_layer[i]==output1)
		{
			softmax->layer_input[0].layer=i;
			softmax->layer_input[0].offset=0;
			break;
		}
	}
	cfg->nlogic_layer-=1;
	cfg->logic_layer=(wtk_knn_logic_layer_t**)wtk_calloc(cfg->nlogic_layer,sizeof(wtk_knn_logic_layer_t*));
	for(i=0,j=0;i<n;++i)
	{
		if(layers[i]==output2)
		{
			continue;
		}
		cfg->logic_layer[j++]=layers[i];
	}
	wtk_knn_cfg_update_layers(cfg,output1,output2);
	wtk_free(input);
	wtk_free(layers);
	wtk_knn_logic_layer_delete(output2);
//	for(i=0;i<cfg->nlogic_layer-1;++i)
//	{
//		wtk_debug("v[%d]=[%.*s] %s input=%d\n",i,cfg->logic_layer[i]->name->len,
//				cfg->logic_layer[i]->name->data,
//				wtk_knn_layer_type_str(cfg->logic_layer[i]->layer->type),cfg->logic_layer[i]->n_input);
//	}
//	exit(0);
}

#if 0
static void wtk_knn_cfg_print(wtk_knn_cfg_t *cfg)
{
	wtk_knn_logic_layer_t *layer,*layer2;
	int i,j;

	for(i=0;i<cfg->nlogic_layer-1;++i)
	{
		layer=cfg->logic_layer[i];
		wtk_knn_logic_layer_print(layer);
		for(j=0;j<layer->n_input;++j)
		{
			layer2=cfg->logic_layer[(int)layer->layer_input[j].layer];
			wtk_debug("v[%d]=%.*s\n",j,layer2->name->len,layer2->name->data);
		}
	}
}
#endif

static int wtk_knn_cfg_find_logic_layer_index(wtk_knn_cfg_t *cfg,char *data,int len)
{
	int i;
	for(i=0;i<cfg->nlogic_layer;++i)
	{
		if(wtk_string_cmp(cfg->logic_layer[i]->name,data,len)==0)
		{
			return i;
		}
	}
	return -1;
}

#ifdef USE_NNET3_COMPILER
int wtk_knn_cfg_update_dep(wtk_knn_cfg_t *cfg)
#else
static int wtk_knn_cfg_update_dep(wtk_knn_cfg_t *cfg)
#endif
{
	wtk_knn_logic_layer_t *input,*layer;
	wtk_knn_logic_layer_t *softmax=NULL;
	int i,j;
	wtk_knn_cmd_t *cmd;
	int static_flag=0;

	for(i=0;i<cfg->nlogic_layer-1;++i)
	{
		input=cfg->logic_layer[i];
		input->layer=wtk_knn_cfg_find_layer(cfg,input->component->data,input->component->len);
		//input->layer=wtk_knn_cfg_find_layer(cfg,input->name->data,input->name->len);
		// wtk_debug("v[%d]=[%.*s] \n",i,input->name->len,input->name->data);
		if(!input->layer)
		{
			wtk_debug("v[%d]=[%.*s] not found\n",i,input->name->len,input->name->data);
			exit(0);
			return -1;
		}
		if(input->layer->type==WTK_LogSoftmaxComponent)
		{
			softmax=input;
		}
		if(cfg->skip_layer && wtk_string_cmp(input->name,cfg->skip_layer,strlen(cfg->skip_layer))==0)
		{
			input->skip=1;
		}
		// wtk_knn_cmd_v_print(&(input->input));
		// wtk_debug("v[%d]=[%.*s] output=%d\n",i,cfg->logic_layer[i]->name->len,cfg->logic_layer[i]->name->data,wtk_knn_layer_get_output(cfg->logic_layer[i]->layer));
		input->n_input=0;
		input->tdnn_max_left=0;
		input->tdnn_max_right=0;

        if (input->layer->type == WTK_CompactFsmnComponent) {
            input->n_input = input->layer->v.fsmn->lorder + 1 + input->layer->v.fsmn->rorder;
            input->layer_input = wtk_calloc(input->n_input, sizeof(wtk_knn_input_t));
            if (i == 0) {
                cfg->tdnn_max_left = max(cfg->tdnn_max_left, input->layer->v.fsmn->lstride * input->layer->v.fsmn->lorder);
                cfg->tdnn_max_right = max(cfg->tdnn_max_right, input->layer->v.fsmn->rstride * input->layer->v.fsmn->rorder);
            } else {
                layer = cfg->logic_layer[i - 1];
                input->use_tdnn_input = 1;
                layer->use_tdnn_output = 1;
                layer->tdnn_max_left = max(layer->tdnn_max_left,
                                           input->layer->v.fsmn->lstride * input->layer->v.fsmn->lorder);
                layer->tdnn_max_right = max(layer->tdnn_max_right,
                                            input->layer->v.fsmn->rstride * input->layer->v.fsmn->rorder);
            }
            printf("%d: ", i);
            for (j = 0; j < input->layer->v.fsmn->lorder; j++) {
                input->layer_input[j].layer = i - 1;
                input->layer_input[j].offset = -input->layer->v.fsmn->lstride * (input->layer->v.fsmn->lorder - j);
                printf(" <%d> / %d,%d ", j, input->layer_input[j].layer, input->layer_input[j].offset);
            }
            input->layer_input[j].layer = i - 1;
            input->layer_input[j].offset = 0;
            j++;
            printf(" <%d> / %d,%d ", j, input->layer_input[j].layer, input->layer_input[j].offset);
            for (; j < input->n_input; j++) {
                input->layer_input[j].layer = i - 1;
                input->layer_input[j].offset = input->layer->v.fsmn->rstride * (j - input->layer->v.fsmn->lorder);
                printf(" <%d> / %d,%d ", j, input->layer_input[j].layer, input->layer_input[j].offset);
            }
        } else {
		if(input->input.type==WTK_KNN_CMD_V_CMD)
		{
			cmd=input->input.v.cmd;

			switch(cmd->type)
			{
			case  WTK_KNN_ROUND:
				input->n_input=1;
				input->layer_input=(wtk_knn_input_t*)wtk_calloc(input->n_input,sizeof(wtk_knn_input_t));
				input->layer_input[0].offset=0;
				input->layer_input[0].layer=-1;
				for(j=0;j<cmd->nv;++j)
				{
					if(cmd->v[j]->type==WTK_KNN_CMD_V_STRING)
					{
						input->layer_input[j].layer=wtk_knn_cfg_find_logic_layer_index(cfg,cmd->v[j]->v.str->data,cmd->v[j]->v.str->len);
					}else
					{

					}
				}
				break;
			default:
				input->n_input=cmd->nv;
				input->layer_input=(wtk_knn_input_t*)wtk_calloc(input->n_input,sizeof(wtk_knn_input_t));
				if(cmd->type==WTK_KNN_SUM)
				{
					input->layer_input->sum=1;
				}else
				{
					input->layer_input->sum=0;
				}
			
				// wtk_debug("nv=%d\n",cmd->nv);
				for(j=0;j<cmd->nv;++j)
				{
					input->layer_input[j].offset=0;
					input->layer_input[j].layer=-1;
					// wtk_debug("%d\n",cmd->v[j]->type);
					if(cmd->v[j]->type==WTK_KNN_CMD_V_STRING)
					{
						input->layer_input[j].layer=wtk_knn_cfg_find_logic_layer_index(cfg,cmd->v[j]->v.str->data,cmd->v[j]->v.str->len);
						// wtk_debug("[%.*s],%d\n",cmd->v[j]->v.str->len,cmd->v[j]->v.str->data,input->layer_input[j].layer);
					}else if(cmd->v[j]->type==WTK_KNN_CMD_V_CMD)
					{
						wtk_knn_cmd_t *cmd2;
						cmd2=cmd->v[j]->v.cmd;
						// wtk_knn_cmd_print(cmd2);
						if(cmd2->type==WTK_KNN_OFFSET)
						{
							input->layer_input[j].layer=wtk_knn_cfg_find_logic_layer_index(cfg,
									cmd2->v[0]->v.str->data,cmd2->v[0]->v.str->len);
							input->layer_input[j].offset=cmd2->v[1]->v.i;
							if(input->layer_input[j].layer>=0)
							{
								input->use_tdnn_input=1;
								layer=cfg->logic_layer[input->layer_input[j].layer];
								layer->use_tdnn_output=1;
								if(input->layer_input[j].offset>0)
								{
									if(input->layer_input[j].offset>layer->tdnn_max_right)
									{
										layer->tdnn_max_right=input->layer_input[j].offset;
									}
								}else if(input->layer_input[j].offset<0)
								{
									if(input->layer_input[j].offset<-layer->tdnn_max_left)
									{
										layer->tdnn_max_left=-input->layer_input[j].offset;
									}
								}
							}else
							{
								if(input->layer_input[j].offset>0)
								{
									if(input->layer_input[j].offset>cfg->tdnn_max_right)
									{
										cfg->tdnn_max_right=input->layer_input[j].offset;
									}
								}else if(input->layer_input[j].offset<0)
								{
									if(input->layer_input[j].offset<-cfg->tdnn_max_left)
									{
										cfg->tdnn_max_left=-input->layer_input[j].offset;
									}
								}
							}
							//exit(0);
						}else if(cmd2->type==WTK_KNN_IFDEFINED)
						{
							input->layer_input[j].layer=wtk_knn_cfg_find_logic_layer_index(cfg,cmd2->v[0]->v.str->data,cmd2->v[0]->v.str->len);
							wtk_debug("descriptor ifdefined\n");
						}else if (cmd2->type==WTK_KNN_SCALE)
						{
							// input->use_tdnn_input=1;
							// layer=cfg->logic_layer[input->layer_input[j].layer];
							// layer->use_tdnn_output=1;
							input->layer_input[j].layer=wtk_knn_cfg_find_logic_layer_index(cfg,cmd2->v[1]->v.str->data,cmd2->v[1]->v.str->len);
							input->layer_input[j].scale=cmd2->v[0]->v.f;
							// wtk_debug("%f\n",input->layer_input[j].scale);
							// wtk_debug("descriptor scale!\n");
					}else
					{
						wtk_debug("descriptor no found!\n");
						exit(0);
					}
					}else
					{
						exit(0);
					}
					if(i!=0&& i!=cfg->nlogic_layer-1 &&cfg->logic_layer[i]->n_input>1)
					{
						cfg->use_tdnn=1;
					}
					// wtk_debug("cfg: i=%d,j=%d,%d\n",i,j,input->layer_input[j].layer);
				}
				break;
			}
		}else
		{
			switch(input->layer->type)
			{
			case WTK_StatisticsExtractionComponent:
				static_flag=1;
				input->n_input=input->layer->v.static_extraction->input_period;
				input->layer_input=(wtk_knn_input_t*)wtk_calloc(input->n_input,sizeof(wtk_knn_input_t));
				for(j=0;j<input->n_input;++j)
				{
					input->layer_input[j].offset=j;
					input->layer_input[j].layer=wtk_knn_cfg_find_logic_layer_index(cfg,input->input.v.str->data,input->input.v.str->len);
				}

				if(input->n_input>1)
				{
					layer=cfg->logic_layer[input->layer_input[0].layer];
					layer->tdnn_max_left=0;
					layer->tdnn_max_right=input->n_input-1;
					input->use_tdnn_input=1;
					layer->use_tdnn_output=1;
				}
				break;
			default:
				input->n_input=1;
				input->layer_input=(wtk_knn_input_t*)wtk_calloc(input->n_input,sizeof(wtk_knn_input_t));
				input->layer_input[0].offset=0;
				input->layer_input[0].layer=wtk_knn_cfg_find_logic_layer_index(cfg,input->input.v.str->data,input->input.v.str->len);
				break;
			}

		}
	}
		// wtk_debug("[%d,%d]\n",cfg->tdnn_max_left,cfg->tdnn_max_right);
		//exit(0);
	}
	//exit(0);
	if(cfg->map)
	{
		wtk_knn_cfg_update_output(cfg);
	}

	cfg->rt_nlogic_layer=cfg->nlogic_layer;
	cfg->output_dim=0;
	if(static_flag==1)
	{
		i=wtk_knn_cfg_find_logic_layer2(cfg,"output",sizeof("output")-1);
		// printf("%.*s\n",input->input.v.str->len,input->input.v.str->data);
		if(i==-1)
		{
			static_flag=0;
		}else
		{
			input=cfg->logic_layer[i];
			if(strncmp(input->input.v.str->data,"output.log-softmax",input->input.v.str->len)==0)
			{
				static_flag=0;
			}else
			{
				i=wtk_knn_cfg_find_logic_layer2(cfg,input->input.v.str->data,input->input.v.str->len);
				if(i==-1)
				{
					static_flag=0;
				}else
				{
					input=cfg->logic_layer[i];
					cfg->rt_nlogic_layer=i+1;
					cfg->use_round=1;
					cfg->output_dim=wtk_knn_layer_get_output(input->layer);
				}
			}
		}
	}

	if(static_flag==0)
	{
		//input=cfg->logic_layer[cfg->nlogic_layer-2];
		input=wtk_knn_cfg_find_logic_layer_s(cfg,"output.log-softmax");
		if(!input)
		{
			input=softmax;
		}
		for(i=0;i<input->n_input;++i)
		{
			cfg->output_dim+=wtk_knn_layer_get_output(cfg->logic_layer[(int)(input->layer_input[i].layer)]->layer);
		}
	}

//	input=wtk_knn_cfg_find_logic_layer(cfg,cfg->output_name,strlen(cfg->output_name));
//	if(!input)
//	{
//		input=softmax;
//	}
//	cfg->output_dim=0;
//	for(i=0;i<input->n_input;++i)
//	{
//		cfg->output_dim+=wtk_knn_layer_get_output(cfg->logic_layer[(int)(input->layer_input[i].layer+1)]->layer);
//	}

	return 0;
}

int wtk_knn_cfg_load_bin(wtk_knn_cfg_t *cfg,wtk_source_t *src);
int wtk_knn_cfg_update_dep_bin(wtk_knn_cfg_t *cfg);

typedef struct
{
	char *name;
	int id;
	float df;
}wtk_knn_prior_t;

static void wtk_knn_cfg_update_prior(wtk_knn_cfg_t *cfg)
{
	wtk_queue_node_t *qn;
	wtk_cfg_item_t *item;
	wtk_knn_prior_t p={0},*pp;
	wtk_string_t *v;
	int id;

	if(!cfg->lc_prior){return;}
	pp=&p;
	for(qn=cfg->lc_prior->cfg->queue.pop;qn;qn=qn->next)
	{
		item=data_offset2(qn,wtk_cfg_item_t,n);
		if(item->type!=WTK_CFG_LC){continue;}
		wtk_local_cfg_update_cfg_str(item->value.cfg,pp,name,v);
		wtk_local_cfg_update_cfg_i(item->value.cfg,pp,id,v);
		wtk_local_cfg_update_cfg_f(item->value.cfg,pp,df,v);
		//wtk_debug("%s/%d=%f\n",pp->name,pp->id,pp->df);
		id=wtk_knn_cfg_get_knn_id(cfg,pp->id);
		cfg->prior->p[id]+=pp->df;
	}
	//exit(0);
}

#if 0
static void wtk_knn_cfg_add_layer_offset(wtk_larray_t *a,int index,int offset)
{
	wtk_knn_input_t *v;
	int i;
	int b;

	b=1;
	v=(wtk_knn_input_t*)a->slot;
	for(i=0;i<a->nslot;++i)
	{
		if(v[i].layer==index && v[i].offset==offset)
		{
			b=0;
			break;
		}
	}
	if(b)
	{
		v=(wtk_knn_input_t*)wtk_larray_push(a);
		v->layer=index;
		v->offset=offset;
	}
}
#endif

static void wtk_knn_cfg_update_layer_skip(wtk_knn_cfg_t *cfg,wtk_larray_t **a,int index,int offset)
{
	wtk_knn_logic_layer_t *layer;
	wtk_larray_t *ax;
	int *vx;
	int i,b;

	ax=a[index+1];
	vx=(int*)ax->slot;
	b=1;
	for(i=0;i<ax->nslot;++i)
	{
		if(vx[i]==offset)
		{
			b=0;
		}
	}
	if(b)
	{
		// wtk_debug("offset=%d,index=%d\n",offset,index);
		wtk_larray_push2(ax,&offset);
	}
	if(index<0)
	{
		return;
	}
	layer=cfg->logic_layer[index];
	for(i=0;i<layer->n_input;++i)
	{
		wtk_knn_cfg_update_layer_skip(cfg,a,layer->layer_input[i].layer,offset+layer->layer_input[i].offset);
	}
	//exit(0);
}

void wtk_knn_cfg_update_skip(wtk_knn_cfg_t *cfg)
{
	wtk_larray_t **a;
	int i;
	int full;

//	for(i=0;i<cfg->nlogic_layer-1;++i)
//	{
//		wtk_debug("v[%d]=%.*s\n",i,cfg->logic_layer[i]->name->len,cfg->logic_layer[i]->name->data);
//	}
	a=(wtk_larray_t**)wtk_calloc(cfg->nlogic_layer,sizeof(wtk_larray_t*));
	for(i=0;i<cfg->nlogic_layer;++i)
	{
		a[i]=wtk_larray_new(100,sizeof(int));
	}
	wtk_knn_cfg_update_layer_skip(cfg,a,cfg->nlogic_layer-2,0);
	//wtk_knn_cfg_print_map(cfg,a);

	cfg->input_map=wtk_knn_bitmap_new((int*)(a[0]->slot),a[0]->nslot,cfg->skip);
	// wtk_debug("input: %d\n",cfg->input_map->full);
	wtk_larray_delete(a[0]);
	full=1;
	for(i=0;i<cfg->nlogic_layer-1;++i)
	{
		cfg->logic_layer[i]->bitmap=wtk_knn_bitmap_new((int*)(a[i+1]->slot),a[i+1]->nslot,cfg->skip);
		if(full==1)
		{
			if(cfg->logic_layer[i]->bitmap->full==0)
			{
				full=0;
			}
		}else
		{
			cfg->logic_layer[i]->bitmap->full=1;
		}
		// wtk_debug("v[%d]=%.*s %d\n",i,cfg->logic_layer[i]->name->len,cfg->logic_layer[i]->name->data,cfg->logic_layer[i]->bitmap->full);
		wtk_larray_delete(a[i+1]);
	}
	wtk_free(a);
	//wtk_knn_cfg_update_layer_skip(cfg,a,cfg->nlogic_layer-2,6);
	//exit(0);
}

static void wtk_batch_norm_update_fix_char(wtk_batch_norm_t *b)
{
	b->fix_char=(wtk_batch_norm_char_t*)wtk_malloc(sizeof(wtk_batch_norm_char_t));
	b->fix_char->scale=wtk_fixvecc_new2(b->scale->p,b->scale->len);
	b->fix_char->offset=wtk_fixvecc_new2(b->offset->p,b->offset->len);
}

static void wtk_batch_norm_update_fix_short(wtk_batch_norm_t *b,int short_shift)
{
	b->fix_short=(wtk_batch_norm_short_t*)wtk_malloc(sizeof(wtk_batch_norm_short_t));
	b->fix_short->scale=wtk_fixvecs_new(b->scale->p,b->scale->len,short_shift);
	b->fix_short->offset=wtk_fixvecs_new(b->offset->p,b->offset->len,short_shift);
	//wtk_debug("%d/%d\n",b->fix_scale_short->len,b->fix_offset_short->len);
}


static void wtk_knn_cfg_update_fix(wtk_knn_cfg_t *cfg)
{
	wtk_knn_logic_layer_t *softmax;
	wtk_knn_layer_t *layer;
	int i;

	for(i=0;i<cfg->nlayer;++i)
	{
		layer=cfg->layer[i];
		// wtk_debug("v[%d]=%.*s\n",i,layer->name->len,layer->name->data);
		switch(layer->type)
		{
		case WTK_NaturalGradientAffineMaskComponent:
			exit(0);
			break;
		case WTK_LinearComponent:
            if (cfg->use_fixchar) {
                layer->v.linear->fixc_a = wtk_fixmatc_new(layer->v.linear->a->p,
                                                          layer->v.linear->a->row,
                                                          layer->v.linear->a->col);
            } else {
                layer->v.linear->fixs_a = wtk_fixmats_new(layer->v.linear->a->p,
                                                          layer->v.linear->a->row,
                                                          layer->v.linear->a->col,cfg->max_shift);
            }
			break;
		case WTK_LinearMaskComponent:
			exit(0);
			break;
		case WTK_AffineComponent:
			if(cfg->use_fixshort || (i==0&&cfg->use_fixchar0_short))
			{
				wtk_knn_ab_update_fix_short(layer->v.affine->ab,cfg->max_shift);
			}else if(cfg->use_fixchar)
			{
				wtk_knn_ab_update_fix_char(layer->v.affine->ab);
			}else
			{
				exit(0);
			}
			break;
		case WTK_FixedAffineComponent:
			if(cfg->use_fixshort || (i==0&&cfg->use_fixchar0_short))
			{
				wtk_knn_ab_update_fix_short(layer->v.fixed_affine->ab,cfg->max_shift);
			}else if(cfg->use_fixchar)
			{
				wtk_knn_ab_update_fix_char(layer->v.fixed_affine->ab);
			}else
			{
				exit(0);
			}
			break;
		case WTK_NaturalGradientAffineComponent:
			if(cfg->use_fixshort || (i==0&&cfg->use_fixchar0_short))
			{
				wtk_knn_ab_update_fix_short(layer->v.ng_affine->ab,cfg->max_shift);
			}else if(cfg->use_fixchar)
			{
				wtk_knn_ab_update_fix_char(layer->v.ng_affine->ab);
			}else
			{
				exit(0);
			}
			break;
		case WTK_RectifiedLinearComponent:
			//exit(0);
			break;
		case WTK_BatchNormComponent:
			if(cfg->use_fixshort)
			{
				wtk_batch_norm_update_fix_short(layer->v.batch_norm,cfg->max_shift);
			}else if(cfg->use_fixchar)
			{
				wtk_batch_norm_update_fix_char(layer->v.batch_norm);
			}else
			{
				exit(0);
			}
			break;
		case WTK_NormalizeComponent:
			//exit(0);
			break;
		case WTK_LogSoftmaxComponent:
			//exit(0);
			break;
		case WTK_SigmoidComponent:
			//exit(0);
			break;
		case WTK_ConvolutionComponent:
			  if (cfg->use_fixchar) 
			  {
				  wtk_knn_ab_update_fix_char(layer->v.conv->ab);
			  }else if(cfg->use_fixshort)
			  {
				  wtk_knn_ab_update_fix_short(layer->v.conv->ab,cfg->max_shift);
			  }else
			  {
				  exit(0);
			  }
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
	}
	softmax=NULL;
	if(cfg->use_add_log)
	{	
		for(i=0;i<cfg->nlogic_layer;++i)
		{
			if(cfg->logic_layer[i]->layer->type==WTK_LogSoftmaxComponent)
			{
				softmax=cfg->logic_layer[i];
				break;
			}
		}
		//cfg->logic_layer[i]->layer_input[j].layer
		cfg->softmax_fixe=wtk_fixexp_new(cfg->logic_layer[softmax->layer_input->layer]->shift,softmax->shift,30);
		cfg->softmax_fixl=wtk_fixlog_new(softmax->shift,30);
		cfg->fixe=wtk_fixexp_new(softmax->shift,softmax->shift,30);
		//wtk_debug("fixe=%p fixl=%p\n",cfg->fixe,cfg->fixl);
		//fixl=wtk_fixlog_new(output->shift,30);
		//fixe=wtk_fixexp_new(input->shift,output->shift,30);
		//exit(0);
	}
	
}

void wtk_knn_cfg_update_offset(wtk_knn_cfg_t* cfg)
{
	int i,j;
	wtk_knn_logic_layer_t *layer;
	for(i=1;i<cfg->nlogic_layer-1;++i)
	{
		// wtk_debug("%s\n",cfg->logic_layer[i]->name->data);
		layer=cfg->logic_layer[i];
		for(j=0;j<layer->n_input;++j)
		{			   
				// wtk_debug("1 offset    %d    skip=%d  \n",layer->layer_input[j].offset,cfg->offset_compress);
				if(layer->layer_input[j].offset>=cfg->offset_compress ||(-layer->layer_input[j].offset)>=cfg->offset_compress)
				{
					layer->layer_input[j].offset = layer->layer_input[j].offset/(cfg->offset_compress);
				}
			   	// wtk_debug("2 offset    %d    skip=%d  \n",layer->layer_input[j].offset,cfg->offset_compress);
		}
		if(layer->tdnn_max_left>=cfg->offset_compress)
		{
			layer->tdnn_max_left/=cfg->offset_compress;
		}
		if(layer->tdnn_max_right>=cfg->offset_compress)
		{
			layer->tdnn_max_right/=cfg->offset_compress;
		}
	}
}


int wtk_knn_cfg_update2(wtk_knn_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	if(cfg->use_bin)
	{
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_knn_cfg_load_bin,cfg->bin_fn);
		if(ret!=0){goto end;}
		ret=wtk_knn_cfg_update_dep_bin(cfg);
		if(ret!=0){goto end;}
	}else
	{
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_knn_cfg_load,cfg->fn);
		if(ret!=0){goto end;}
		ret=wtk_knn_cfg_update_dep(cfg);
		if(ret!=0){goto end;}
		wtk_knn_cfg_update_prior(cfg);
		// wtk_debug("use fixpoint=%d\n",cfg->use_fixpoint);
	}
	if(cfg->xvec_fn)
	{
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_knn_cfg_xvec_load,cfg->xvec_fn);
		if(ret!=0){goto end;}		
	}
	if(cfg->layer_shift)
	{
		int i;

		for(i=0;i<cfg->nlogic_layer-1;++i)
		{
			cfg->logic_layer[i]->shift=cfg->layer_shift[i];
		}
	}
	if(cfg->use_bin==0 && cfg->use_fixpoint)
	{
		wtk_knn_cfg_update_fix(cfg);
		//exit(0);
	}
	if(cfg->skip>0)
	{
		wtk_knn_cfg_update_skip(cfg);
	}
	if(cfg->update_offset)
	{
		wtk_knn_cfg_update_offset(cfg);
	}
	if(cfg->use_fast_exp)
	{
		cfg->use_exp=1;
	}
	ret=0;
end:
	//exit(0);
	return ret;
}

int wtk_knn_cfg_update_dep_bin(wtk_knn_cfg_t *cfg)
{
	int i;

	for(i=0;i<cfg->nlogic_layer-1;++i)
	{
		cfg->logic_layer[i]->layer=wtk_knn_cfg_find_layer(cfg,cfg->logic_layer[i]->component->data,cfg->logic_layer[i]->component->len);
	}
	//exit(0);
	//wtk_debug("[%d,%d]\n",cfg->input_left_context,cfg->input_right_context);
	//print_data(cfg->input_left,cfg->input_nleft);
	//print_data(cfg->input_right,cfg->input_right_context);
	//exit(0);
	return 0;
}


int wtk_knn_cfg_load_bin(wtk_knn_cfg_t *cfg,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	int ret;
	char b,bb;
	int i;
	wtk_knn_layer_t *layer;
	wtk_knn_logic_layer_t *llayer;

	src->swap=0;
	buf=wtk_strbuf_new(256,1);
	ret=wtk_source_fill(src,&b,1);
	if(ret!=0){goto end;}
	cfg->nlayer=b;
	cfg->layer=(wtk_knn_layer_t**)wtk_calloc(cfg->nlayer,sizeof(wtk_knn_layer_t*));
	for(i=0;i<cfg->nlayer;++i)
	{
		ret=wtk_source_fill(src,&b,1);
		if(ret!=0){goto end;}
		bb=b;
		ret=wtk_source_fill(src,&b,1);
		if(ret!=0){goto end;}
		ret=wtk_source_fill(src,buf->data,b);
		if(ret!=0){goto end;}
		buf->pos=b;
		// wtk_debug("%.*s bb=%d\n",buf->pos,buf->data,bb);
		layer=cfg->layer[i]=wtk_knn_layer_new(buf->data,buf->pos);
		cfg->layer[i]->type=bb;
		// wtk_debug("v[%d]=%.*s type=[%s]\n",i,cfg->layer[i]->name->len,
				//  cfg->layer[i]->name->data,wtk_knn_layer_type_str(cfg->layer[i]->type));
		switch(layer->type)
		{
		case WTK_NaturalGradientAffineMaskComponent:
			exit(0);
			break;
		case WTK_LinearMaskComponent:
			exit(0);
			break;
		case WTK_LinearComponent:
            layer->v.linear = wtk_knn_linear_new();
            if (cfg->use_fixpoint) {
                if (cfg->use_fixchar) {
                    layer->v.linear->fixc_a = wtk_fixmatc_read_fix(src);
                } else {
                    layer->v.linear->fixs_a = wtk_fixmats_read_fix(src);
                }
            } else {
                layer->v.linear->a = wtk_matf_read(src);
            }
			break;
		case WTK_AffineComponent:
			layer->v.affine=wtk_knn_affine_new();
			if(cfg->use_fixpoint)
			{
				if(cfg->use_fixshort)
				{
					layer->v.affine->ab=wtk_knn_ab_read_fix_short(src);
				}else if(cfg->use_fixchar)
				{
					layer->v.affine->ab=wtk_knn_ab_read_fix_char(src);
				}else
				{
					exit(0);
				}
			}else
			{
				layer->v.affine->ab=wtk_knn_ab_read(src);
			}
			break;
		case WTK_FixedAffineComponent:
			layer->v.fixed_affine=wtk_knn_fixed_affine_new();
			if(cfg->use_fixpoint)
			{
				if(cfg->use_fixshort)
				{
					layer->v.fixed_affine->ab=wtk_knn_ab_read_fix_short(src);
				}else if(cfg->use_fixchar)
				{
					layer->v.fixed_affine->ab=wtk_knn_ab_read_fix_char(src);
				}else
				{
					exit(0);
					//layer->v.fixed_affine->ab=wtk_knn_ab_read_fix(src);
				}
			}else
			{
				layer->v.fixed_affine->ab=wtk_knn_ab_read(src);
			}
			break;
		case WTK_NaturalGradientAffineComponent:
			layer->v.ng_affine=wtk_knn_ng_affine_new();
			if(cfg->use_fixpoint)
			{
				if(cfg->use_fixshort)
				{
					layer->v.ng_affine->ab=wtk_knn_ab_read_fix_short(src);
				}else if(cfg->use_fixchar)
				{
					layer->v.ng_affine->ab=wtk_knn_ab_read_fix_char(src);
				}else
				{
					exit(0);
					//layer->v.ng_affine->ab=wtk_knn_ab_read_fix(src);
				}
			}else
			{
				layer->v.ng_affine->ab=wtk_knn_ab_read(src);
			}
			break;
		case WTK_RectifiedLinearComponent:
			layer->v.rectified_linear=NULL;
			break;
		case WTK_NormalizeComponent:
			layer->v.normalize=wtk_knn_normalize_new();
			{
				float f;

				ret=wtk_source_fill(src,(char*)&f,4);
				if(ret!=0){goto end;}
				layer->v.normalize->rms=f;
			}
			ret=wtk_source_fill(src,&b,1);
			if(ret!=0){goto end;}
			layer->v.normalize->add_log_stddev=b;
			break;
		case WTK_BatchNormComponent:
			layer->v.batch_norm=wtk_batch_norm_new();
			if(cfg->use_fixpoint)
			{
				if(cfg->use_fixshort)
				{
					wtk_batch_norm_read_fix_short(layer->v.batch_norm,src);
				}else if(cfg->use_fixchar)
				{
					wtk_batch_norm_read_fix_char(layer->v.batch_norm,src);
				}else
				{
					exit(0);
				}
			}else
			{
				int val;
				ret=wtk_source_fill(src,(char*)&val,4);
				layer->v.batch_norm->dim=val;
				layer->v.batch_norm->scale=wtk_vecf_read(src);
				layer->v.batch_norm->offset=wtk_vecf_read(src);
			}
			//wtk_vecf_write(layer->v.batch_norm->scale,f);
			//wtk_vecf_write(layer->v.batch_norm->offset,f);
			break;
		case WTK_LogSoftmaxComponent:
			layer->v.log_softmax=NULL;
			if(cfg->use_fixpoint)
			{
				cfg->softmax_fixe=wtk_fixexp_read(src);
				//wtk_debug("shift=%d/%d n=%d vs=%d\n",cfg->fixe->shifti,cfg->fixe->shifto,cfg->fixe->n,cfg->fixe->vs);
				cfg->softmax_fixl=wtk_fixlog_read(src);
				cfg->fixe=wtk_fixexp_read(src);
			}
			//wtk_debug("shift=%d n=%d vs=%d\n",cfg->fixl->shift,cfg->fixl->n,cfg->fixl->vs);
			break;
		case WTK_SigmoidComponent:
			layer->v.sigmoid=NULL;
			break;
		case WTK_ConvolutionComponent:
			//wtk_debug("conv xx\n");
			layer->v.conv=wtk_knn_conv_read(src,cfg);
			break;
		case WTK_NoOpComponent:
		case WTK_GeneralDropoutComponent:
		case WTK_LstmNonlinearityComponent:
		case WTK_BackpropTruncationComponent:
		case WTK_ScaleAndOffsetComponent:
		case WTK_StatisticsExtractionComponent:
		{
			int val;
			layer->v.static_extraction=wtk_knn_static_extraction_new();
			ret=wtk_source_fill(src,(char*)&val,4);
			layer->v.static_extraction->input_period=val;
			ret=wtk_source_fill(src,(char*)&val,4);
			layer->v.static_extraction->output_period=val;
			ret=wtk_source_fill(src,(char*)&val,4);
			layer->v.static_extraction->include_var=val;
			ret=wtk_source_fill(src,(char*)&val,4);
			layer->v.static_extraction->input_dim=val;
			//wtk_debug("%d %d %d %d\n",layer->v.static_extraction->input_period,
			//		layer->v.static_extraction->output_period,layer->v.static_extraction->include_var,layer->v.static_extraction->input_dim);
		}
			break;
		case WTK_StatisticsPoolingComponent:
		{
			int val;
			float f;
			layer->v.static_pooling=wtk_knn_static_pooling_new();
			ret=wtk_source_fill(src,(char*)&val,4);
			layer->v.static_pooling->input_dim=val;
			ret=wtk_source_fill(src,(char*)&val,4);
			layer->v.static_pooling->numlog_count_features=val;
			ret=wtk_source_fill(src,(char*)&val,4);
			layer->v.static_pooling->output_stddevs=val;
			ret=wtk_source_fill(src,(char*)&f,4);
			layer->v.static_pooling->variance_floor=f;
			//wtk_debug("%d %d %d %f\n",layer->v.static_pooling->input_dim
			//		,layer->v.static_pooling->numlog_count_features,layer->v.static_pooling->output_stddevs,layer->v.static_pooling->variance_floor);
		}
			break;
		default:
			break;
		}
		//exit(0);
	}
	ret=wtk_source_fill(src,&b,1);
	if(ret!=0){goto end;}
	cfg->nlogic_layer=b;
	cfg->logic_layer=(wtk_knn_logic_layer_t**)wtk_calloc(cfg->nlogic_layer,sizeof(wtk_knn_logic_layer_t*));
	for(i=0;i<cfg->nlogic_layer;++i)
	{
		// wtk_debug("%d/%d\n",i,cfg->nlogic_layer);
		llayer=cfg->logic_layer[i]=wtk_knn_logic_layer_new();
		ret=wtk_source_fill(src,&b,1);
		if(ret!=0){goto end;}
		ret=wtk_source_fill(src,buf->data,b);
		if(ret!=0){goto end;}
		buf->pos=b;
		// wtk_debug("v[%d]=[%.*s]\n",i,buf->pos,buf->data);
		llayer->name=wtk_string_dup_data(buf->data,buf->pos);
		ret=wtk_source_fill(src,&b,1);
		if(ret!=0){goto end;}
		if(b>0)
		{
			ret=wtk_source_fill(src,buf->data,b);
			if(ret!=0){goto end;}
			buf->pos=b;
			llayer->component=wtk_string_dup_data(buf->data,buf->pos);
		}
		ret=wtk_source_fill(src,&b,1);
		if(ret!=0){goto end;}
		ret=wtk_source_fill(src,buf->data,b);
		if(ret!=0){goto end;}
		buf->pos=b;
		llayer->n_input=b/sizeof(wtk_knn_input_t);
		// wtk_debug("v[%d]=%d/%d\n",i,llayer->n_input,b);
		llayer->layer_input=(wtk_knn_input_t*)wtk_data_dup2(buf->data,buf->pos);
		//exit(0);
		{
			short v[4];

			ret=wtk_source_fill(src,(char*)v,sizeof(short)*4);
			if(ret!=0){goto end;}
			llayer->tdnn_max_left=v[0];
			llayer->tdnn_max_right=v[1];
			llayer->use_tdnn_input=v[2];
			llayer->use_tdnn_output=v[3];
			//fwrite(v,sizeof(short)*4,1,f);
			//wtk_debug("input=%d output=%d\n",cfg->input_dim,cfg->output_dim);
		}
	}
	if(cfg->use_prior)
	{
		cfg->prior=wtk_vecf_read(src);
		if(!cfg->prior){ret=-1;goto end;}
	}
	{
		short v[4];

		ret=wtk_source_fill(src,(char*)v,sizeof(short)*4);
		if(ret!=0){goto end;}
		cfg->input_dim=v[0];
		cfg->output_dim=v[1];
		cfg->tdnn_max_left=v[2];
		cfg->tdnn_max_right=v[3];
		// wtk_debug("input=%d output=%d %d/%d\n",cfg->input_dim,cfg->output_dim,cfg->tdnn_max_left,cfg->tdnn_max_right);
	}
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

static void wtk_knn_ab_write(wtk_knn_ab_t *ab,FILE *f)
{
	wtk_matf_write(ab->a,f);
	wtk_vecf_write(ab->b,f);
}

static void wtk_knn_conv_write(wtk_knn_conv_t *conv,FILE *f)
{
	fwrite(&(conv->input_x_dim),4,1,f);
	fwrite(&(conv->input_y_dim),4,1,f);
	fwrite(&(conv->input_z_dim),4,1,f);
	fwrite(&(conv->filter_x_dim),4,1,f);
	fwrite(&(conv->filter_y_dim),4,1,f);
	fwrite(&(conv->filter_x_step),4,1,f);
	fwrite(&(conv->filter_y_step),4,1,f);
	fwrite(&(conv->num_y_step),4,1,f);
	wtk_veci_write(conv->height_offset,f);
	wtk_knn_ab_write(conv->ab,f);
}

static void wtk_batch_norm_write(wtk_batch_norm_t *norm,FILE *f)
{
	fwrite(&(norm->dim),4,1,f);
	wtk_vecf_write(norm->scale,f);
	wtk_vecf_write(norm->offset,f);
}

#if 0
static void wtk_fixvec_write_fix(wtk_fixvec_t *fix,FILE *f)
{
	int v[2];

	v[0]=fix->len;
	v[1]=fix->shift;
	fwrite(v,sizeof(int)*2,1,f);
	fwrite(fix->pv,fix->len*sizeof(int),1,f);
}
#endif

wtk_fixvec_t* wtk_fixvec_read_fix(wtk_source_t *src)
{
	int v[2];
	int ret;
	wtk_fixvec_t *vec=NULL;

	ret=wtk_source_fill(src,(char*)v,sizeof(int)*2);
	if(ret!=0){goto end;}
	vec=wtk_fixvec_new(v[0]);
	vec->shift=v[1];
	ret=wtk_source_fill(src,(char*)vec->pv,sizeof(int)*vec->len);
	if(ret!=0){goto end;}
	ret=0;
end:
	return vec;
}

void wtk_batch_norm_read_fix_char(wtk_batch_norm_t *norm,wtk_source_t *src)
{
	wtk_source_fill(src,(char*)&(norm->dim),sizeof(int));
	norm->fix_char=(wtk_batch_norm_char_t*)wtk_malloc(sizeof(wtk_batch_norm_char_t));
	norm->fix_char->offset=wtk_fixvecc_read_fix(src);
	norm->fix_char->scale=wtk_fixvecc_read_fix(src);
}

void wtk_batch_norm_read_fix_short(wtk_batch_norm_t *norm,wtk_source_t *src)
{
	wtk_source_fill(src,(char*)&(norm->dim),sizeof(int));
	norm->fix_short=(wtk_batch_norm_short_t*)wtk_malloc(sizeof(wtk_batch_norm_short_t));
	norm->fix_short->offset=wtk_fixvecs_read_fix(src);
	norm->fix_short->scale=wtk_fixvecs_read_fix(src);
}

wtk_fixmats_t* wtk_fixmats_read_fix(wtk_source_t *src)
{
	int v[2];
	wtk_fixmats_t *ms=NULL;
	int ret;

	ret=wtk_source_fill(src,(char*)v,sizeof(int)*2);
	if(ret!=0){goto end;}
	ms=wtk_fixmats_new2(v[0],v[1]);
	ret=wtk_source_fill(src,(char*)ms->shift,ms->row);
	if(ret!=0){goto end;}
	ret=wtk_source_fill(src,(char*)ms->pv,ms->row*ms->col*sizeof(short));
	if(ret!=0){goto end;}
end:
	if(ret!=0)
	{
		wtk_fixmats_delete(ms);
		ms=NULL;
	}
	return ms;
}

wtk_fixvecs_t* wtk_fixvecs_read_fix(wtk_source_t *src)
{
	int v[2];
	int ret;
	wtk_fixvecs_t *vec=NULL;

	ret=wtk_source_fill(src,(char*)v,sizeof(int)*2);
	if(ret!=0){goto end;}
	vec=wtk_fixvecs_new2(v[0]);
	vec->shift=v[1];
	ret=wtk_source_fill(src,(char*)vec->pv,sizeof(short)*vec->len);
	if(ret!=0){goto end;}
	ret=0;
end:
	if(ret!=0)
	{
		wtk_fixvecs_delete(vec);
		vec=NULL;
	}
	return vec;
}


static void wtk_batch_norm_write_fix_char(wtk_batch_norm_t *norm,FILE *f)
{
	fwrite(&(norm->dim),4,1,f);
	wtk_fixvecc_write_fix(norm->fix_char->offset,f);
	wtk_fixvecc_write_fix(norm->fix_char->scale,f);
}

static void wtk_batch_norm_write_fix_short(wtk_batch_norm_t *norm,FILE *f)
{
	fwrite(&(norm->dim),4,1,f);
	wtk_fixvecs_write_fix(norm->fix_short->offset,f);
	wtk_fixvecs_write_fix(norm->fix_short->scale,f);
}

static void wtk_knn_ab_write_fix_char(wtk_knn_ab_t *ab,FILE *f)
{
	wtk_fixmatc_write_fix(ab->fix_char->a,f);
	wtk_fixvecc_write_fix(ab->fix_char->b,f);
}


static void wtk_fixmats_write_fix(wtk_fixmats_t *fix,FILE *f)
{
	int v[2];

	v[0]=fix->row;
	v[1]=fix->col;
	fwrite(v,sizeof(int)*2,1,f);
	fwrite(fix->shift,fix->row*sizeof(char),1,f);
	fwrite(fix->pv,fix->row*fix->col*sizeof(short),1,f);
}

void wtk_fixvecs_write_fix(wtk_fixvecs_t *fix,FILE *f)
{
	int v[2];

	v[0]=fix->len;
	v[1]=fix->shift;
	fwrite(v,sizeof(int)*2,1,f);
	fwrite(fix->pv,fix->len*sizeof(short),1,f);
}


static void wtk_knn_ab_write_fix_short(wtk_knn_ab_t *ab,FILE *f)
{
	wtk_fixmats_write_fix(ab->fix_short->a,f);
	wtk_fixvecs_write_fix(ab->fix_short->b,f);
}

void wtk_fixmatc_write_fix(wtk_fixmatc_t *fix,FILE *f)
{
	int v[2];

	v[0]=fix->row;
	v[1]=fix->col;
	fwrite(v,sizeof(int)*2,1,f);
	fwrite(fix->shift,fix->row*sizeof(char),1,f);
	fwrite(fix->min,fix->row*sizeof(short),1,f);
	fwrite(fix->pv,fix->row*fix->col*sizeof(char),1,f);
}

void wtk_fixvecc_write_fix(wtk_fixvecc_t *fix,FILE *f)
{
	int v[3];

	v[0]=fix->len;
	v[1]=fix->shift;
	v[2]=fix->min;
	fwrite(v,sizeof(int)*3,1,f);
	fwrite(fix->pv,fix->len*sizeof(char),1,f);
}

static void wtk_knn_conv_write_fix_char(wtk_knn_conv_t *conv,FILE *f)
{
	fwrite(&(conv->input_x_dim),4,1,f);
	fwrite(&(conv->input_y_dim),4,1,f);
	fwrite(&(conv->input_z_dim),4,1,f);
	fwrite(&(conv->filter_x_dim),4,1,f);
	fwrite(&(conv->filter_y_dim),4,1,f);
	fwrite(&(conv->filter_x_step),4,1,f);
	fwrite(&(conv->filter_y_step),4,1,f);
	fwrite(&(conv->num_y_step),4,1,f);
	wtk_veci_write(conv->height_offset,f);
	wtk_knn_ab_write_fix_char(conv->ab,f);
}

static void wtk_knn_conv_write_fix_short(wtk_knn_conv_t *conv,FILE *f)
{
	fwrite(&(conv->input_x_dim),4,1,f);
	fwrite(&(conv->input_y_dim),4,1,f);
	fwrite(&(conv->input_z_dim),4,1,f);
	fwrite(&(conv->filter_x_dim),4,1,f);
	fwrite(&(conv->filter_y_dim),4,1,f);
	fwrite(&(conv->filter_x_step),4,1,f);
	fwrite(&(conv->filter_y_step),4,1,f);
	fwrite(&(conv->num_y_step),4,1,f);
	wtk_veci_write(conv->height_offset,f);
	wtk_knn_ab_write_fix_short(conv->ab,f);
}

wtk_knn_ab_t* wtk_knn_ab_read_fix_short(wtk_source_t *src)
{
	wtk_knn_ab_t *ab;

	ab=wtk_knn_ab_new();
	ab->fix_short=(wtk_knn_ab_short_t*)wtk_malloc(sizeof(wtk_knn_ab_short_t));
	ab->fix_short->a=wtk_fixmats_read_fix(src);
	ab->fix_short->b=wtk_fixvecs_read_fix(src);
	//exit(0);
	return ab;
}

wtk_fixmatc_t* wtk_fixmatc_read_fix(wtk_source_t *src)
{
	int v[2];
	wtk_fixmatc_t *ms=NULL;
	int ret;

	ret=wtk_source_fill(src,(char*)v,sizeof(int)*2);
	if(ret!=0){goto end;}
	ms=wtk_fixmatc_new2(v[0],v[1]);
	ret=wtk_source_fill(src,(char*)ms->shift,ms->row);
	if(ret!=0){goto end;}
	ret=wtk_source_fill(src,(char*)ms->min,ms->row*sizeof(short));
	if(ret!=0){goto end;}
	ret=wtk_source_fill(src,(char*)ms->pv,ms->row*ms->col*sizeof(char));
	if(ret!=0){goto end;}
end:
	if(ret!=0)
	{
		wtk_fixmatc_delete(ms);
		ms=NULL;
	}
	return ms;
}

wtk_fixvecc_t* wtk_fixvecc_read_fix(wtk_source_t *src)
{
	int v[3];
	int ret;
	wtk_fixvecc_t *vec=NULL;

	ret=wtk_source_fill(src,(char*)v,sizeof(int)*3);
	if(ret!=0){goto end;}
	vec=wtk_fixvecc_new(v[0]);
	vec->shift=v[1];
	vec->min=v[2];
	ret=wtk_source_fill(src,(char*)vec->pv,sizeof(char)*vec->len);
	if(ret!=0){goto end;}
	ret=0;
end:
	if(ret!=0)
	{
		wtk_fixvecc_delete(vec);
		vec=NULL;
	}
	return vec;
}



wtk_knn_ab_t* wtk_knn_ab_read_fix_char(wtk_source_t *src)
{
	wtk_knn_ab_t *ab;

	ab=wtk_knn_ab_new();
	ab->fix_char=(wtk_knn_ab_char_t*)wtk_malloc(sizeof(wtk_knn_ab_char_t));
	ab->fix_char->a=wtk_fixmatc_read_fix(src);
	ab->fix_char->b=wtk_fixvecc_read_fix(src);
	//wtk_debug("a=[%d/%d] b=%d\n",ab->fix_char->a->row,ab->fix_char->a->col,ab->fix_char->b->len);
	//exit(0);
	return ab;
}

void wtk_knn_cfg_write_bin(wtk_knn_cfg_t *cfg,char *fn)
{
	int i;
	FILE *f;
	char b;
	wtk_knn_layer_t *layer;
	wtk_knn_logic_layer_t *llayer;

	f=fopen(fn,"wb");
	b=cfg->nlayer;
	wtk_debug("b=%d\n",b);
	fwrite(&b,1,1,f);
	for(i=0;i<cfg->nlayer;++i)
	{
		layer=cfg->layer[i];
		b=layer->type;
		fwrite(&b,1,1,f);
		//wtk_debug("write type=%d\n",b);
		b=layer->name->len;
		fwrite(&b,1,1,f);
		fwrite(layer->name->data,layer->name->len,1,f);
		wtk_debug("v[%d]=%.*s type=[%s]\n",i,cfg->layer[i]->name->len,
				cfg->layer[i]->name->data,wtk_knn_layer_type_str(cfg->layer[i]->type));
		switch(layer->type)
		{
		case WTK_NaturalGradientAffineMaskComponent:
			exit(0);
			break;
		case WTK_LinearMaskComponent:
			exit(0);
			break;
		case WTK_LinearComponent:
			if(cfg->use_fixpoint)
			{
                if (cfg->use_fixchar) {
                    wtk_fixmatc_write_fix(layer->v.linear->fixc_a, f);
                } else {
                    wtk_fixmats_write_fix(layer->v.linear->fixs_a, f);
                }
			}else
			{
				wtk_matf_write(layer->v.linear->a,f);
			}
			break;
		case WTK_AffineComponent:
			if(cfg->use_fixpoint)
			{
				if(layer->v.affine->ab->fix_short)
				{
					wtk_knn_ab_write_fix_short(layer->v.affine->ab,f);
				}else if(layer->v.affine->ab->fix_char)
				{
					wtk_knn_ab_write_fix_char(layer->v.affine->ab,f);
				}else
				{
					exit(0);
				}
			}else
			{
				wtk_knn_ab_write(layer->v.affine->ab,f);
			}
			break;
		case WTK_FixedAffineComponent:
			if(cfg->use_fixpoint)
			{
				if(layer->v.fixed_affine->ab->fix_short)
				{
					wtk_knn_ab_write_fix_short(layer->v.fixed_affine->ab,f);
				}else if(layer->v.fixed_affine->ab->fix_char)
				{
					wtk_knn_ab_write_fix_char(layer->v.fixed_affine->ab,f);
				}else
				{
					exit(0);
				}
			}else
			{
				wtk_knn_ab_write(layer->v.fixed_affine->ab,f);
			}
			break;
		case WTK_NaturalGradientAffineComponent:
			if(cfg->use_fixpoint)
			{
				if(layer->v.fixed_affine->ab->fix_short)
				{
					wtk_knn_ab_write_fix_short(layer->v.ng_affine->ab,f);
				}else if(layer->v.fixed_affine->ab->fix_char)
				{
					wtk_knn_ab_write_fix_char(layer->v.ng_affine->ab,f);
				}else
				{
					exit(0);
				}
			}else
			{
				wtk_knn_ab_write(layer->v.ng_affine->ab,f);
			}
			break;
		case WTK_RectifiedLinearComponent:
			break;
		case WTK_BatchNormComponent:
			if(cfg->use_fixpoint)
			{
				if(layer->v.batch_norm->fix_short)
				{
					wtk_batch_norm_write_fix_short(layer->v.batch_norm,f);
				}else if(layer->v.batch_norm->fix_char)
				{
					wtk_batch_norm_write_fix_char(layer->v.batch_norm,f);
				}else
				{
					exit(0);
				}
			}else
			{
				wtk_batch_norm_write(layer->v.batch_norm,f);
			}
			break;
		case WTK_NormalizeComponent:
			{
				float t;

				t=layer->v.normalize->rms;
				fwrite((char*)&t,4,1,f);
			}
			b=layer->v.normalize->add_log_stddev;
			fwrite(&b,1,1,f);
			break;
		case WTK_LogSoftmaxComponent:
			if(cfg->softmax_fixe)
			{
				wtk_fixexp_write(cfg->softmax_fixe,f);
				wtk_debug("shift=%d/%d n=%d vs=%d\n",cfg->softmax_fixe->shifti,cfg->softmax_fixe->shifto,cfg->softmax_fixe->n,cfg->softmax_fixe->vs);
			}
			if(cfg->softmax_fixl)
			{
				wtk_fixlog_write(cfg->softmax_fixl,f);
				wtk_debug("shift=%d n=%d vs=%d\n",cfg->softmax_fixl->shift,cfg->softmax_fixl->n,cfg->softmax_fixl->vs);
			}
			if(cfg->fixe)
			{
				wtk_fixexp_write(cfg->fixe,f);
			}
			break;
		case WTK_SigmoidComponent:
			break;
		case WTK_ConvolutionComponent:
			if(cfg->use_fixpoint)
			{
				if(layer->v.conv->ab->fix_char)
				{
					wtk_knn_conv_write_fix_char(layer->v.conv,f);
				}else
				{
					wtk_knn_conv_write_fix_short(layer->v.conv,f);
				}	
			}else
			{
				wtk_knn_conv_write(layer->v.conv,f);
			}
			break;
		case WTK_NoOpComponent:
		case WTK_GeneralDropoutComponent:
		case WTK_LstmNonlinearityComponent:
		case WTK_BackpropTruncationComponent:
		case WTK_ScaleAndOffsetComponent:
		case WTK_StatisticsExtractionComponent:
		{
			int val = layer->v.static_extraction->input_period;
			fwrite((char*)&val,4,1,f);
			val = layer->v.static_extraction->output_period;
			fwrite((char*)&val,4,1,f);
			val = layer->v.static_extraction->include_var;
			fwrite((char*)&val,4,1,f);
			val = layer->v.static_extraction->input_dim;
			fwrite((char*)&val,4,1,f);
		}
			break;
		case WTK_StatisticsPoolingComponent:
		{
			int val = layer->v.static_pooling->input_dim;
			fwrite((char*)&val,4,1,f);
			val = layer->v.static_pooling->numlog_count_features;
			fwrite((char*)&val,4,1,f);
			val = layer->v.static_pooling->output_stddevs;
			fwrite((char*)&val,4,1,f);
			float t= layer->v.static_pooling->variance_floor;
			wtk_debug("%f\n",t);
			fwrite((char*)&t,4,1,f);
		}
			break;
		default:
			break;
		}
	}
	b=cfg->nlogic_layer;
	//wtk_debug("b=%d\n",b);
	fwrite(&b,1,1,f);
	for(i=0;i<cfg->nlogic_layer;++i)
	{
		llayer=cfg->logic_layer[i];
		b=llayer->name->len;
		fwrite(&b,1,1,f);
		fwrite(llayer->name->data,llayer->name->len,1,f);


		if(llayer->component)
		{
			b=llayer->component->len;
			fwrite(&b,1,1,f);
			fwrite(llayer->component->data,llayer->component->len,1,f);
		}else
		{
			b=0;
			fwrite(&b,1,1,f);
		}
		b=llayer->n_input*sizeof(wtk_knn_input_t);
		fwrite(&b,1,1,f);
		//wtk_debug("v[%d]=%d/%d\n",i,llayer->n_input,b);
		fwrite(llayer->layer_input,llayer->n_input*sizeof(wtk_knn_input_t),1,f);
		{
			short v[4];

			v[0]=llayer->tdnn_max_left;
			v[1]=llayer->tdnn_max_right;
			v[2]=llayer->use_tdnn_input;
			v[3]=llayer->use_tdnn_output;
			fwrite(v,sizeof(short)*4,1,f);
			//wtk_debug("input=%d output=%d\n",cfg->input_dim,cfg->output_dim);
		}
	}
	if(cfg->use_prior)
	{
		wtk_vecf_write(cfg->prior,f);
	}

	{
		short v[4];

		v[0]=cfg->input_dim;
		v[1]=cfg->output_dim;
		v[2]=cfg->tdnn_max_left;
		v[3]=cfg->tdnn_max_right;
		fwrite(v,sizeof(short)*4,1,f);
		wtk_debug("input=%d output=%d\n",cfg->input_dim,cfg->output_dim);
	}
	fclose(f);
}

int wtk_knn_cfg_get_knn_id(wtk_knn_cfg_t *cfg,int id)
{
	int i;

	for(i=0;i<cfg->nmap;++i)
	{
		//wtk_debug("v[%d]=%d/%d id=%d\n",i,cfg->map[i].id,cfg->map[i].knn_id,id);
		if(cfg->map[i].id==id)
		{
			return cfg->map[i].knn_id;
		}
	}
	return -1;
}

static float **xv;
static char** name;
int max_len=10;
/*
int wtk_knn_cfg_xvec_load_fix(wtk_knn_cfg_t *cfg,wtk_source_t *src)
{
    wtk_strbuf_t *buf;
    wtk_str_hash_t *xvec;
    float f;
    int x;
    int ret;
    int i=0;
    wtk_larray_t *a;
    buf=wtk_strbuf_new(4096,1);
    xvec=wtk_str_hash_new(max_len);
    cfg->xvec_map=xvec;
    a=wtk_larray_new(100,sizeof(float));
    xv = wtk_malloc(sizeof(int*)*max_len);
    name = wtk_malloc(sizeof(char*)*max_len);
    while(1)
    {
        ret=wtk_source_read_string(src,buf);
        // wtk_debug("%.*s\n",buf->pos,buf->data);
        if(ret!=0){break;}
        name[i]=wtk_calloc(sizeof(char),buf->pos+1);
        memcpy(name[i],buf->data,sizeof(char)*(buf->pos+1));
        name[i][buf->pos]='\0';
        ret=wtk_source_expect_string_s(src,buf,"[");
        if(ret!=0){goto end;}
        while(1)
        {
            ret=wtk_source_read_string(src,buf);
            if(ret!=0){goto end;}
            if(buf->data[0]==']'){break;}
            f=wtk_str_atof(buf->data,buf->pos);
            if(f<0)
            {
                x = (int)(f*(1<<DEFAULT_RADIX)-0.5);
                if(x<-32767) x=-32767;
            }else
            {
                x = (int)(f*(1<<DEFAULT_RADIX)+0.5);
                if(x>32768) x=32768;
            }
            wtk_larray_push2(a,&x);
            // wtk_debug("[%.*s]=%f,%d\n",buf->pos,buf->data,f,x);
        }
        xv[i]= wtk_malloc(sizeof(int)*a->nslot);
        memcpy(xv[i],a->slot,a->nslot*sizeof(int));
        wtk_larray_reset(a);
        wtk_debug("%s\n",name[i]);
        print_int((int*)(xv[i]),80);
        wtk_str_hash_add(xvec,name[i],sizeof(char)*strlen(name[i]),xv[i]);
        i++;

    }
	ret=0;
end:
    wtk_strbuf_delete(buf);
    wtk_larray_delete(a);
    return ret;
}
*/
int wtk_knn_cfg_xvec_load(wtk_knn_cfg_t *cfg,wtk_source_t *src)
{
    wtk_strbuf_t *buf;
    wtk_str_hash_t *xvec;
    float f;
    int ret;
    int i=0;
    wtk_larray_t *a;
    buf=wtk_strbuf_new(4096,1);
    xvec=wtk_str_hash_new(max_len);
    cfg->xvec_map=xvec;
    a=wtk_larray_new(100,sizeof(float));
    xv = wtk_malloc(sizeof(int*)*max_len);
    name = wtk_malloc(sizeof(char*)*max_len);
    while(1)
    {
        ret=wtk_source_read_string(src,buf);
        // wtk_debug("%.*s\n",buf->pos,buf->data);
        if(ret!=0){break;}
        name[i]=wtk_calloc(sizeof(char),buf->pos+1);
        memcpy(name[i],buf->data,sizeof(char)*(buf->pos+1));
        name[i][buf->pos]='\0';
        ret=wtk_source_expect_string_s(src,buf,"[");
        if(ret!=0){goto end;}
        while(1)
        {
            ret=wtk_source_read_string(src,buf);
            if(ret!=0){goto end;}
            if(buf->data[0]==']'){break;}
            f=wtk_str_atof(buf->data,buf->pos);
            wtk_larray_push2(a,&f);
            // wtk_debug("[%.*s]=%f,%d\n",buf->pos,buf->data,f,x);
        }
        xv[i]= wtk_malloc(sizeof(float)*a->nslot);
        memcpy(xv[i],a->slot,a->nslot*sizeof(float));
        wtk_larray_reset(a);
        // wtk_debug("%s\n",name[i]);
        // print_float((xv[i]),80);
        wtk_str_hash_add(xvec,name[i],sizeof(char)*strlen(name[i]),xv[i]);
        i++;

    }
	ret=0;
end:
    wtk_strbuf_delete(buf);
    wtk_larray_delete(a);
    return ret;
}

void wtk_knn_cfg_xvec_delete(wtk_knn_cfg_t *cfg)
{
    int i;
    for(i=0;i<max_len;++i)
    {
        wtk_free(xv[i]);
        wtk_free(name[i]);
    }
    wtk_free(xv);
    wtk_free(name);
    wtk_str_hash_delete(cfg->xvec_map);
}

float* wtk_knn_get_xvector(wtk_knn_cfg_t* cfg,char* speaker)
{
    float* xvec=NULL;
    char default_name[]="cc";
    if(speaker==NULL)
    {
        speaker=default_name;
    }
    xvec=(float*)wtk_str_hash_find(cfg->xvec_map,speaker,strlen(speaker));
    if(xvec==NULL)
    {   
        wtk_debug("wrong name\n");
        exit(0);
    }
    // print_float(xvec,80);
    return xvec;
}

#ifdef USE_NNET3_COMPILER
wtk_knn_logic_layer_t* qtk_knn_logic_layer_new(char *name, char *component, char *input)
{
    int i, depth = 0;
    wtk_larray_t *input_a = NULL, *stack = NULL, *cur_cmd_v = NULL;
	wtk_knn_cmd_item_t *item = NULL;
    wtk_knn_cmd_t *cur_cmd = NULL;
    wtk_string_t **strs = NULL;
	wtk_knn_cmd_v_t *cmdv = NULL;
    wtk_knn_logic_layer_t *ll = wtk_knn_logic_layer_new();

    if (name)
        ll->name = wtk_string_dup_data(name, wtk_strlen(name));
    if (component)
        ll->component = wtk_string_dup_data(component, wtk_strlen(component));

    input_a = wtk_knn_cfg_parse(input, wtk_strlen(input));
    strs = input_a->slot;
    for (i=0; i<input_a->nslot; i++) {
        if (wtk_string_equal_s(strs[i], "Append")) {
            ll->input.type = WTK_KNN_CMD_V_CMD;
            cur_cmd = ll->input.v.cmd = wtk_knn_cmd_new(WTK_KNN_APPEND);
            cur_cmd_v = wtk_larray_new(10, sizeof(void *));
            stack = wtk_larray_new(10, sizeof(wtk_knn_cmd_t*));
        } else if (wtk_string_equal_s(strs[i], "Offset")) {
            if (cur_cmd) {
                item = (wtk_knn_cmd_item_t*)wtk_malloc(sizeof(wtk_knn_cmd_item_t));
                item->cmd = cur_cmd;
                item->v = cur_cmd_v;
                wtk_larray_push2(stack,&item);
            }
            cur_cmd = wtk_knn_cmd_new(WTK_KNN_OFFSET);
            cur_cmd_v = wtk_larray_new(10, sizeof(void*));
        } else if (wtk_string_equal_s(strs[i], "(")) {
            depth ++;
        } else if (wtk_string_equal_s(strs[i], ",")) {
        } else if (wtk_string_equal_s(strs[i], ")")) {
            depth --;
            cur_cmd->nv = cur_cmd_v->nslot;
            cur_cmd->v = (wtk_knn_cmd_v_t**)wtk_calloc(cur_cmd->nv,sizeof(wtk_knn_cmd_v_t*));
            memcpy(cur_cmd->v,cur_cmd_v->slot,cur_cmd_v->nslot*sizeof(wtk_knn_cmd_v_t*));
            wtk_larray_delete(cur_cmd_v);
            if (depth != 0) {
                item = *((wtk_knn_cmd_item_t**)wtk_larray_pop_back(stack));
                cmdv = (wtk_knn_cmd_v_t*)wtk_malloc(sizeof(wtk_knn_cmd_v_t));
                cmdv->type = WTK_KNN_CMD_V_CMD;
                cmdv->v.cmd = cur_cmd;
                wtk_larray_push2(item->v,&(cmdv));
                cur_cmd = item->cmd;
                cur_cmd_v = item->v;
                wtk_free(item);
            }
        } else {
            if (depth==0) {
                ll->input.type = WTK_KNN_CMD_V_STRING;
                ll->input.v.str = wtk_string_dup_data(strs[i]->data,strs[i]->len);
            } else {
                cmdv=(wtk_knn_cmd_v_t*)wtk_malloc(sizeof(wtk_knn_cmd_v_t));
                if(isdigit(strs[i]->data[0]) ||(strs[i]->data[0]=='-' && isdigit(strs[i]->data[1]))) {
                    cmdv->type=WTK_KNN_CMD_V_I;
                    cmdv->v.i=wtk_str_atoi(strs[i]->data,strs[i]->len);
                }else {
                    cmdv->type=WTK_KNN_CMD_V_STRING;
                    cmdv->v.str=wtk_string_dup_data(strs[i]->data,strs[i]->len);
                }
                wtk_larray_push2(cur_cmd_v,&cmdv);
            }
        }
    }

    wtk_knn_cfg_delete_array(input_a);

    if (stack)
        wtk_larray_delete(stack);

    return ll;
}

wtk_knn_layer_t *qtk_knn_layer_new(char *name, int type, void *arg1, void *arg2)
{
    wtk_knn_layer_t *layer = wtk_knn_layer_new(name, wtk_strlen(name));
    wtk_knn_layer_set_type(layer, type);
    switch(layer->type) {
	case WTK_LinearMaskComponent:
		layer->v.linear_mask=wtk_knn_linear_mask_new();
        break;
	case WTK_AffineComponent:
		layer->v.affine=wtk_knn_affine_new();
        layer->v.affine->ab = wtk_knn_ab_new();
        layer->v.affine->ab->a = wtk_matf_new(1, 1);  // just a trick
        layer->v.affine->ab->b = wtk_vecf_new(1);     // just a trick
        *(layer->v.affine->ab->a) = *(wtk_matf_t *)arg1;
        *(layer->v.affine->ab->b) = *(wtk_vecf_t *)arg2;
        break;
	case WTK_FixedAffineComponent:
		layer->v.fixed_affine=wtk_knn_fixed_affine_new();
        layer->v.fixed_affine->ab = wtk_knn_ab_new();
        layer->v.fixed_affine->ab->a = wtk_matf_new(1, 1);  // just a trick
        layer->v.fixed_affine->ab->b = wtk_vecf_new(1);     // just a trick
        *(layer->v.fixed_affine->ab->a) = *(wtk_matf_t *)arg1;
        *(layer->v.fixed_affine->ab->b) = *(wtk_vecf_t *)arg2;
        break;
	case WTK_NaturalGradientAffineComponent:
		layer->v.ng_affine=wtk_knn_ng_affine_new();
        layer->v.ng_affine->ab = wtk_knn_ab_new();
        layer->v.ng_affine->ab->a = wtk_matf_new(1, 1);  // just a trick
        layer->v.ng_affine->ab->b = wtk_vecf_new(1);     // just a trick
        *(layer->v.ng_affine->ab->a) = *(wtk_matf_t *)arg1;
        *(layer->v.ng_affine->ab->b) = *(wtk_vecf_t *)arg2;
        break;
	case WTK_NaturalGradientAffineMaskComponent:
		layer->v.ng_mask_affine=wtk_knn_ng_mask_affine_new();
        break;
	case WTK_RectifiedLinearComponent:
		layer->v.rectified_linear=NULL;
        break;
	case WTK_BatchNormComponent:
		layer->v.batch_norm=wtk_batch_norm_new();
        layer->v.batch_norm->scale = wtk_vecf_new(1);
        layer->v.batch_norm->offset = wtk_vecf_new(1);
        *(layer->v.batch_norm->scale) = *(wtk_vecf_t *)arg1;
        *(layer->v.batch_norm->offset) = *(wtk_vecf_t *)arg2;
        break;
	case WTK_LogSoftmaxComponent:
		layer->v.log_softmax=NULL;
        break;
	case WTK_SigmoidComponent:
		layer->v.sigmoid=NULL;
        break;
	case WTK_NormalizeComponent:
		layer->v.normalize=wtk_knn_normalize_new();
        layer->v.normalize->rms = *(float *)arg1;
        layer->v.normalize->add_log_stddev = *(int *)arg2;
        break;
    default:
        return NULL;
    }
    return layer;
}

wtk_knn_layer_t *qtk_knn_layer_new_fix_char(char *name, int type, void *arg1, void *arg2)
{
    wtk_knn_layer_t *layer = wtk_knn_layer_new(name, wtk_strlen(name));
    wtk_knn_layer_set_type(layer, type);
    wtk_knn_ab_char_t *ab_char;
    switch(layer->type) {
	case WTK_LinearMaskComponent:
		layer->v.linear_mask=wtk_knn_linear_mask_new();
        break;
	case WTK_AffineComponent:
		layer->v.affine=wtk_knn_affine_new();
        layer->v.affine->ab = wtk_knn_ab_new();
        layer->v.affine->ab->fix_char = ab_char = (wtk_knn_ab_char_t *)wtk_malloc(sizeof(wtk_knn_ab_char_t));
        ab_char->a = wtk_fixmatc_new2(1, 1); // just a trick
        ab_char->b = wtk_fixvecc_new(1);     // just a trick
        *(ab_char->a) = *(wtk_fixmatc_t *)arg1;
        *(ab_char->b) = *(wtk_fixvecc_t *)arg2;
        break;
	case WTK_FixedAffineComponent:
		layer->v.fixed_affine=wtk_knn_fixed_affine_new();
        layer->v.fixed_affine->ab = wtk_knn_ab_new();
        layer->v.affine->ab->fix_char = ab_char = (wtk_knn_ab_char_t *)wtk_malloc(sizeof(wtk_knn_ab_char_t));
        ab_char->a = wtk_fixmatc_new2(1, 1); // just a trick
        ab_char->b = wtk_fixvecc_new(1);     // just a trick
        *(ab_char->a) = *(wtk_fixmatc_t *)arg1;
        *(ab_char->b) = *(wtk_fixvecc_t *)arg2;
        break;
	case WTK_NaturalGradientAffineComponent:
		layer->v.ng_affine=wtk_knn_ng_affine_new();
        layer->v.ng_affine->ab = wtk_knn_ab_new();
        layer->v.affine->ab->fix_char = ab_char = (wtk_knn_ab_char_t *)wtk_malloc(sizeof(wtk_knn_ab_char_t));
        ab_char->a = wtk_fixmatc_new2(1, 1); // just a trick
        ab_char->b = wtk_fixvecc_new(1);     // just a trick
        *(ab_char->a) = *(wtk_fixmatc_t *)arg1;
        *(ab_char->b) = *(wtk_fixvecc_t *)arg2;
        break;
	case WTK_NaturalGradientAffineMaskComponent:
		layer->v.ng_mask_affine=wtk_knn_ng_mask_affine_new();
        break;
	case WTK_RectifiedLinearComponent:
		layer->v.rectified_linear=NULL;
        break;
	case WTK_BatchNormComponent:
		layer->v.batch_norm=wtk_batch_norm_new();
        layer->v.batch_norm->fix_char = wtk_malloc(sizeof(wtk_batch_norm_char_t));
        layer->v.batch_norm->fix_char->scale = wtk_fixvecc_new(1);
        layer->v.batch_norm->fix_char->offset = wtk_fixvecc_new(1);
        *(layer->v.batch_norm->fix_char->scale) = *(wtk_fixvecc_t *)arg1;
        *(layer->v.batch_norm->fix_char->offset) = *(wtk_fixvecc_t *)arg2;
        break;
	case WTK_LogSoftmaxComponent:
		layer->v.log_softmax=NULL;
        break;
	case WTK_SigmoidComponent:
		layer->v.sigmoid=NULL;
        break;
	case WTK_NormalizeComponent:
		layer->v.normalize=wtk_knn_normalize_new();
        layer->v.normalize->rms = *(float *)arg1;
        layer->v.normalize->add_log_stddev = *(int *)arg2;
        break;
    default:
        return NULL;
    }
    return layer;
}
#endif
