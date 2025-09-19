#include "qtk_nnet3_fix.h"
//#include <immintrin.h>
#include "wtk/core/wtk_os.h"
#include "wtk/core/math/wtk_mat.h"

//support int/short, other expand
void qtk_nnet3_update_fix_f2i(qtk_blas_matrix_t* m,void* mfix,float max_w, int fixed_nbytes)
{
	wtk_mati_t *mi=NULL;
	wtk_mats_t *ms=NULL;
    int i,n=0;
    float *pf, t;

    switch(fixed_nbytes){
	case sizeof(short):
		ms=(wtk_mats_t*)mfix;
		n=ms->col*ms->row;
		ms->scale=1/max_w;
		break;
	case sizeof(int):
		mi=(wtk_mati_t*)mfix;
		n=mi->col*mi->row;
		mi->scale=1/max_w;
		break;
    }
    pf = m->m;
    for(i=0;i<n;++i)
	{
		t = pf[i]*max_w;

		//wtk_debug("pf[%d]= %f\n",i,pf[i]);
		//wtk_debug("ms[%d]= %d\n",i,(int)t);
		switch(fixed_nbytes){
		case sizeof(short):
			ms->p[i]=wtk_float_round(t);
			break;
		case sizeof(int):
			mi->p[i]=wtk_float_round(t);
			break;
		}
	}
}

void qtk_nnet3_update_fix_f2s(qtk_blas_matrix_t *m,wtk_mats_t *ms,float max_w)
{
        int i,n;
        short *ps;
        float *pf;
        float t;
//wtk_debug("================================\n");
//print_float(m->m,20);
//wtk_debug("================================\n");
        //wtk_debug("col=%d row=%d\n", ms->col, ms->row);
        n=ms->col*ms->row;
        ps = ms->p;
        pf = m->m;
//	wtk_debug("f_max=%f,f_min=%f\n",f_max,f_min);
        ms->scale=1/max_w;
        for(i=0;i<n;++i)
	{
	//wtk_debug("pf[%d]= %f\n",i,pf[i]);
		t = pf[i]*max_w;
		ps[i]=wtk_float_round(t);
	//wtk_debug("ms[%d]= %d\n",i,(int)t);
	}

}

void qtk_nnet3_update_fix_f2s2(wtk_matrix_t *m,wtk_mats_t *ms,float max_w)
{
        int i,n;
        short *ps;
        float *pf;
	float t;
        n=ms->col*ms->row;
        ps = ms->p;
        pf = *m;
        ms->scale=1/max_w;
        for(i=0;i<n;++i)
	{
        //wtk_debug("pf[%d]= %f\n",i,pf[i]);
		t = pf[i]*max_w;
		ps[i]=wtk_float_round(t);
		//wtk_debug("ms[%d]= %d\n",i,(int)t);
	}

}

void qtk_nnet3_update_fix_s2f(float scale ,float scale2,wtk_mati_t* outi,float* pf)
{
	int i,n;
	int * pi;
	n = outi->col*outi->row;
	pi = outi->p;
//	wtk_debug("bbbbbbbbbb  scale1=%f,scale2=%f,int%d,%d======float%d,%d,offset%d,%d\n",scale,scale2,outi->col,outi->row,outf->col,outf->row,r_offset,c_offset);
	for(i=0;i<n;++i)
	{
		pf[i]=pi[i]*scale*scale2;		
	//	wtk_debug("pf[%d]=%f,pi[%d]=%d\n",j,pf[j],i,pi[i]);
	}
}


void qtk_nnet3_write_mats(FILE *f,wtk_mats_t *ms)
{
        fwrite(&(ms->scale),4,1,f);
//        wtk_debug("scale=%f\n",ms->scale);
	fwrite(&(ms->row),4,1,f);
//       wtk_debug("row=%d\n",ms->row);
	fwrite(&(ms->col),4,1,f);
//        wtk_debug("col=%d\n",ms->col);
        fwrite(ms->p,2,ms->col*ms->row,f);
        print_short(ms->p,20);
}

/*
 * Note: when use_custom_acc > 0; matrix had recover. not retran matrix.
 */
void wtk_nnet3_write_mati2(FILE *f,void* mfix, int fixed_nbytes)
{
	wtk_mati_t *mi=NULL;
//	wtk_mats_t *ms=NULL;
	int i,j;
//	void *p=NULL;
	int* v=NULL;

//    switch(fixed_nbytes){
//	case sizeof(short):
//		ms=(wtk_mats_t*)mfix;
//		p=ms->p;
//		break;
//	case sizeof(int):
		mi=(wtk_mati_t*)mfix;
//		p=mi->p;
//		break;
//    }
    fwrite(&(mi->scale),sizeof(int),1,f);
    fwrite(&(mi->row),sizeof(int),1,f);
    fwrite(&(mi->col),sizeof(int),1,f);

    //default save reset matrix.
    //here adjust comm matrix, for compute option.

	wtk_mati_t *rm;
//	rm =wtk_mati_new(mi->col,mi->row);
//  fwrite(&(rm->row),sizeof(int),1,f);
//  fwrite(&(rm->col),sizeof(int),1,f);
//	wtk_mati_zero(rm);
//	for (i=0; i< mi->col; i++)
//	{
//		for(j=0; j<mi->row;j++)
//		{
//			v=(int*)p+j*mi->col+i;
//			*(rm->p+i*mi->row + j) = *v;
//			printf("%d ", *v);
//			//fwrite(v, fixed_nbytes, 1, f);
//		}
//		printf("\n");
//	}
	rm=mi;
    int step, porder=2;
    int col2, k, r, d;
    step = 1 << porder;
    col2= (rm->col >> porder) << porder ;
	for(k=0; k<col2/step; k++)
	{
	    for (i=0; i< rm->row; i++)
	    {
	    	for(j=k*step; j<(k+1)*step;j++)
	    	{
	        	v=rm->p+i*rm->col+j;
	        	printf("%d ", *v);
	        	fwrite(v, fixed_nbytes, 1, f);
	    	}
	    	printf("\n");
	    }
	}
	r=rm->col-col2;
	//printf("r=%d row=%d col=%d\n", r, rm->row, rm->col);
	for(d=0; d<r; d++)
	{
	    for (i=0; i< rm->row; i++)
	    {
	        v=rm->p+i*rm->col+k*step+d;
	        //printf("%d ", *v);
	        fwrite(v, fixed_nbytes, 1, f);
	    }
	    //printf("\n");
	}
    //exit(0);
}

void qtk_nnet3_write_mati(FILE *f,void* mfix, int fixed_nbytes)
{
	wtk_mati_t *mi=NULL;
	wtk_mats_t *ms=NULL;
	void *p=NULL;

    switch(fixed_nbytes){
	case sizeof(short):
		ms=(wtk_mats_t*)mfix;
		p=ms->p;
                fwrite(&(ms->scale), sizeof(int), 1, f);
                fwrite(&(ms->row), sizeof(int), 1, f);
                fwrite(&(ms->col), sizeof(int), 1, f);
                fwrite(p, fixed_nbytes, ms->col * ms->row, f);
                break;
	case sizeof(int):
            mi = (wtk_mati_t *)mfix;
            p = mi->p;
            fwrite(&(mi->scale), sizeof(int), 1, f);
            fwrite(&(mi->row), sizeof(int), 1, f);
            fwrite(&(mi->col), sizeof(int), 1, f);
            fwrite(p, fixed_nbytes, mi->col * mi->row, f);
            break;
        }
    //print_short(ms->p,20);
    //print_int((int*)ms->p,20);
}

void qtk_nnet3_write_matrix(FILE *f,qtk_blas_matrix_t *mb)
{
    unsigned int row, col;
    row = mb->row;
    col = mb->col;
    fwrite(&row, sizeof(unsigned int), 1, f);
    fwrite(&col, sizeof(unsigned int), 1, f);
    fwrite(mb->m, 4, row * col, f);
    // print_float(mb->m ,row * col);
}

void qtk_nnet3_trans_mdl_write(FILE *f,qtk_trans_model_t *t_mdl)
{
        int pdf_id,phone_id, forward_id, loop_id;

        wtk_queue_node_t *qn;
        wtk_queue_t q;
        qtk_trans_tuples_t* tuple;
        //qtk_trans_phone_t* phone;

        q = t_mdl->tuple_q;
	fwrite(&(q.length),4,1,f);
        for (qn = q.pop; qn; qn = qn->next)
        {
                tuple = data_offset(qn, qtk_trans_tuples_t, q_n);
		phone_id = tuple->phone_id;
		pdf_id = tuple->pdf_id;
                forward_id = tuple->forward_trans;
                loop_id = tuple->loop_trans;
		fwrite(&(phone_id),4,1,f);
		fwrite(&(pdf_id),4,1,f);
                fwrite(&(forward_id),4,1,f);
                fwrite(&(loop_id),4,1,f);
	}
}

void qtk_nnet3_trans_mdl_write_normal(FILE *f,qtk_trans_model_t *t_mdl)
{
        int phone_id, state_id,pdf_id, repeat,entry_id;
        wtk_queue_node_t *qn;
        wtk_queue_t q;
        qtk_trans_triples_t * triple;

       	q = t_mdl->triple_q;
        //wtk_debug("q.length=%d\n", q.length);
        fwrite(&(q.length),4,1,f);
        for (qn = q.pop; qn; qn = qn->next)
        {
       		triple = data_offset(qn, qtk_trans_triples_t, q_n);
       		phone_id = triple->phone_id;
       		state_id = triple->pdf_id;
       		pdf_id = triple->transition_id;
       		entry_id = (t_mdl->phones+phone_id-1)->entry_id;
       		repeat = *((t_mdl->entries+entry_id)->state_trans+state_id);
       		//wtk_debug("%d %d %d %d\n",phone_id,pdf_id,forward_id,repeat);
           	fwrite(&(phone_id),4,1,f);
           	fwrite(&(state_id),4,1,f);
           	fwrite(&(pdf_id),4,1,f);
          	fwrite(&(repeat),4,1,f);
	}
}

void qtk_nnet3_wakeup_trans_mdl_write(FILE *f,qtk_wakeup_trans_model_t *wt_mdl)
{
	int pdf_id,phone_id, forward_id, loop_id;

        wtk_queue_node_t *qn;
        wtk_queue_t q;
        qtk_trans_tuples_t* tuple;
        //qtk_trans_phone_t* phone;

        q = wt_mdl->tuple_q;       
	fwrite(&(q.length),4,1,f);
        for (qn = q.pop; qn; qn = qn->next)
        {
                tuple = data_offset(qn, qtk_trans_tuples_t, q_n);
                //pdf_id=tuple->pdf_id;
                phone_id = tuple->phone_id;
                pdf_id = tuple->pdf_id;
                forward_id = tuple->forward_trans;
                loop_id = tuple->loop_trans;
                fwrite(&(phone_id),4,1,f);
                fwrite(&(pdf_id),4,1,f);
                fwrite(&(forward_id),4,1,f);
                fwrite(&(loop_id),4,1,f);
	}
}

int qtk_trans_model_cal_id2pdf_chain2_fix(qtk_trans_model_t* t_model)
{
	int in_label, pdf_id, j, repeat;
	int label1[30000];
	wtk_queue_node_t *qn;
	wtk_queue_t q;
	qtk_trans_triples_t* triple;

	in_label = 1;
	q = t_model->triple_q;
	for (qn = q.pop; qn; qn = qn->next)
	{
		triple = data_offset(qn, qtk_trans_triples_t, q_n);
		pdf_id = triple->transition_id;
		if(pdf_id < 0)
		{
			wtk_debug("res model exception\n");
			return -1;
		}
		repeat = triple->repeat;
		for(j=0;j<repeat;j++)
		{
			 label1[in_label] = pdf_id;
			 //wtk_debug("label1[%d]= %d\n", in_label, pdf_id);
			 in_label++;
		}
	}
	t_model->id2pdf_id_ = (int*) wtk_malloc(sizeof(int) * in_label);
	memcpy(t_model->id2pdf_id_, label1, sizeof(int) * in_label);

	return 0;
}

int qtk_nnet3_trans_model_load_chain_fix_bin(qtk_trans_model_cfg_t *t_mdl,wtk_source_t *src)
{
	int ret;
	int num,i;
	int pdf_id,phone_id,forward_id,loop_id;
	qtk_trans_tuples_t* tuple;
	t_mdl->trans_model = qtk_trans_model_new();
	ret=wtk_source_fill(src,(char*)(&num),4);
	if(ret!=0){goto end;}

	for(i=0;i<num;++i)
	{
		tuple = (qtk_trans_tuples_t*) wtk_malloc(sizeof(qtk_trans_tuples_t));
		ret=wtk_source_fill(src,(char*)(&phone_id),4);
		if(ret!=0){goto end;}
		ret=wtk_source_fill(src,(char*)(&pdf_id),4);
		if(ret!=0){goto end;}
       	ret=wtk_source_fill(src,(char*)(&forward_id),4);
       	if(ret!=0){goto end;}
       	ret=wtk_source_fill(src,(char*)(&loop_id),4);
       	if(ret!=0){goto end;}
       	tuple->phone_id =phone_id;
       	tuple->pdf_id = pdf_id;
       	tuple->forward_trans = forward_id;
       	tuple->loop_trans = loop_id;
       	//printf("triple: %d %d %d %d\n", tuple->phone_id, tuple->pdf_id, tuple->forward_trans, tuple->loop_trans);
       	wtk_queue_push(&(t_mdl->trans_model->tuple_q), &(tuple->q_n));
	}
	qtk_trans_model_cal_id2pdf_chain(t_mdl->trans_model);

end:
	return ret;
}

int qtk_nnet3_trans_model_load_chain2_fix_bin(qtk_trans_model_cfg_t *t_mdl,wtk_source_t *src)
{
	int ret;
	int num,i;
	int phone_id, state_id, pdf_id, repeat;   //real info define.
	qtk_trans_triples_t *triple;
	t_mdl->trans_model = qtk_trans_model_new();
	ret=wtk_source_fill(src,(char*)(&num),4);
        if(ret!=0){goto end;}
	for(i=0;i<num;++i)
	{
		triple = (qtk_trans_triples_t*) wtk_malloc(sizeof(qtk_trans_triples_t));
		ret=wtk_source_fill(src,(char*)(&phone_id),4);
        if(ret!=0){goto end;}
        ret=wtk_source_fill(src,(char*)(&state_id),4);
       	if(ret!=0){goto end;}
       	ret=wtk_source_fill(src,(char*)(&pdf_id),4);
       	if(ret!=0){goto end;}
      	ret=wtk_source_fill(src,(char*)(&repeat),4);
       	if(ret!=0){goto end;}
        triple->phone_id =phone_id;
        triple->pdf_id = state_id;
        triple->transition_id = pdf_id;
        triple->repeat = repeat;
        //printf("triple: %d %d %d %d\n", triple->phone_id, triple->pdf_id, triple->transition_id, triple->repeat);
		wtk_queue_push(&(t_mdl->trans_model->triple_q), &(triple->q_n));
	}
	ret=qtk_trans_model_cal_id2pdf_chain2_fix(t_mdl->trans_model);

end:
	return ret;
}

int qtk_nnet3_wakeup_trans_model_load_chain_fix_bin(qtk_wakeup_trans_model_cfg_t* wt_mdl,wtk_source_t *src)
{
        int ret;
        int num,i;
        int pdf_id,phone_id,forward_id,loop_id;
        qtk_wakeup_trans_tuples_t* tuple;	
        wt_mdl->trans_model = qtk_wakeup_trans_model_new();
        wtk_debug("load trans mdl ");
        ret=wtk_source_fill(src,(char*)(&num),4);
        if(ret!=0){goto end;}
        for(i=0;i<num;++i)
        {
                tuple = (qtk_wakeup_trans_tuples_t*) wtk_malloc(sizeof(qtk_wakeup_trans_tuples_t));
                ret=wtk_source_fill(src,(char*)(&phone_id),4);
                if(ret!=0){goto end;}
                ret=wtk_source_fill(src,(char*)(&pdf_id),4);
                if(ret!=0){goto end;}
                ret=wtk_source_fill(src,(char*)(&forward_id),4);
                if(ret!=0){goto end;}
                ret=wtk_source_fill(src,(char*)(&loop_id),4);
                if(ret!=0){goto end;}
                tuple->phone_id =phone_id;
                tuple->pdf_id = pdf_id;
                tuple->forward_trans = forward_id;
                tuple->loop_trans = loop_id;
                wtk_queue_push(&(wt_mdl->trans_model->tuple_q), &(tuple->q_n));
	}
	qtk_wakeup_trans_model_cal_id2pdf_chain(wt_mdl->trans_model);
end:
        return ret;
}



int qtk_nnet3_affine_global_component_source_fill2(qtk_affine_global_component_t *ga_com,wtk_source_t *src, int fixed_nbytes)
{
	int ret;
	int row,col,t;
	float scale;
	int row2,col2;
	ret=wtk_source_fill(src,(char*)(&t),4);
        if(ret!=0){goto end;}
//	wtk_debug("type=%d\n",t);
	ret=wtk_source_fill(src,(char*)(&scale),4);
        if(ret!=0){goto end;}
//	wtk_debug("scale=%f\n",scale);
        ret=wtk_source_fill(src,(char*)(&row),4);
        if(ret!=0){goto end;}
//	wtk_debug("row=%d\n",row);
	ret=wtk_source_fill(src,(char*)(&col),4);
        if(ret!=0){goto end;}
//	wtk_debug("row=%d\n",col);
    switch(fixed_nbytes){
    case sizeof(short):
    	ga_com->ws = wtk_mats_new(row,col);
    	break;
    case sizeof(int):
		ga_com->ws = (wtk_mats_t*)wtk_mati_new(row,col);
    	break;
    }

    //wtk_debug("row=%d col=%d fixed_nbytes=%d\n",row, col, fixed_nbytes);
	ret=wtk_source_fill(src,(char*)(ga_com->ws->p),row*col*fixed_nbytes);
	if(ret!=0){goto end;}
	ga_com->type = t;
	ga_com->ws->scale =scale;
	ga_com->ws->row =row;
	ga_com->ws->col =col;
	//print_short(ga_com->ws->p,20);
	//print_int((int*)ga_com->ws->p,20);
	//exit(0);
	ga_com->nga_com = NULL;
	ga_com->linear_com = NULL;
	
	if(t!=QTK_LinearComponent)
	{
        	ret=wtk_source_fill(src,(char*)(&row2),4);
        	if(ret!=0){goto end;}
		ret=wtk_source_fill(src,(char*)(&col2),4);
        	if(ret!=0){goto end;}
//		wtk_debug("row2=%d\n,col2=%d\n",row2,col2);
		ga_com->b = qtk_blas_matrix_new(row2,col2);
		ret=wtk_source_fill(src,(char*)(ga_com->b->m),row2*col2*4);
//		print_float(ga_com->b->m,20);
		if(ret!=0){goto end;}
		ga_com->b->row =row2;
		ga_com->b->col =col2;
	}

end:
	return ret;
}

int qtk_nnet3_affine_global_component_source_fill(qtk_affine_global_component_t *ga_com,wtk_source_t *src)
{
	return qtk_nnet3_affine_global_component_source_fill2(ga_com, src, sizeof(short));
}

int qtk_nnet3_batch_norm_component_source_fill(qtk_batch_norm_component_t *batchnorm_com,wtk_source_t *src)
{
	int ret;
	int row,col;
	float scale;
	int row2,col2;
	int dim,block_dim;
	ret=wtk_source_fill(src,(char*)(&dim),4);
	if(ret!=0){goto end;}
        ret=wtk_source_fill(src,(char*)(&block_dim),4);
        if(ret!=0){goto end;}
	ret=wtk_source_fill(src,(char*)(&scale),4);
        if(ret!=0){goto end;}
        ret=wtk_source_fill(src,(char*)(&row),4);
        if(ret!=0){goto end;}
	ret=wtk_source_fill(src,(char*)(&col),4);
        if(ret!=0){goto end;}
	batchnorm_com->scale2 = wtk_mats_new(row,col);
	ret=wtk_source_fill(src,(char*)(batchnorm_com->scale2->p),row*col*2);
	if(ret!=0){goto end;}
	batchnorm_com->dim = dim;
	batchnorm_com->block_dim =block_dim;
	batchnorm_com->scale2->scale =scale;
	batchnorm_com->scale2->row =row;
	batchnorm_com->scale2->col =col;

	ret=wtk_source_fill(src,(char*)(&scale),4);
        if(ret!=0){goto end;}
        ret=wtk_source_fill(src,(char*)(&row2),4);
        if(ret!=0){goto end;}
	ret=wtk_source_fill(src,(char*)(&col2),4);
        if(ret!=0){goto end;}
	batchnorm_com->offset2 = wtk_mats_new(row2,col2);
	ret=wtk_source_fill(src,(char*)(batchnorm_com->offset2->p),row2*col2*2);
	if(ret!=0){goto end;}
	batchnorm_com->offset2->scale = scale;
	batchnorm_com->offset2->row =row2;
	batchnorm_com->offset2->col =col2;

        batchnorm_com->input=NULL;
	batchnorm_com->output=NULL;
	batchnorm_com->out_reshape=NULL;
	batchnorm_com->in_reshape=NULL;

end:
	return ret;
}

int qtk_nnet3_normalize_component_source_fill(qtk_normalize_component_t* n_com,wtk_source_t *src)
{
	int ret;
	float t;
	ret=wtk_source_fill(src,(char*)(&t),4);
	n_com->target_rms_ = t;
	return ret;
}

int qtk_nnet3_activate_component_source_fill(qtk_activate_component_t* ls_com,wtk_source_t *src)
{
	int ret;
	int t;
	ret=wtk_source_fill(src,(char*)(&t),4);
	ls_com->type =t;
	return ret;
}

int qtk_nnet3_lstm_component_source_fill(qtk_lstm_nolinearity_component_t* lstm_com,wtk_source_t *src)
{
	int ret;
	int row,col;
/*	float scale;

	ret=wtk_source_fill(src,(char*)(&scale),4);
        if(ret!=0){goto end;}
        ret=wtk_source_fill(src,(char*)(&row),4);
        if(ret!=0){goto end;}
	ret=wtk_source_fill(src,(char*)(&col),4);
        if(ret!=0){goto end;}
	lstm_com->params2 = wtk_mats_new(row,col);
	ret=wtk_source_fill(src,(char*)(lstm_com->params2->p),row*col*2);
	if(ret!=0){goto end;}
	lstm_com->params2->scale =scale;
	lstm_com->params2->row =row;
	lstm_com->params2->col =col;*/

        ret=wtk_source_fill(src,(char*)(&row),4);
        if(ret!=0){goto end;}
        ret=wtk_source_fill(src,(char*)(&col),4);
        if(ret!=0){goto end;}
        lstm_com->params = qtk_blas_matrix_new(row,col);
        ret=wtk_source_fill(src,(char*)(lstm_com->params->m),row*col*4);
        if(ret!=0){goto end;}
        lstm_com->params->row =row;
        lstm_com->params->col =col;


end:
	return ret;
}

int qtk_nnet3_conv_component_source_fill(qtk_timeheight_convolution_component_t* conv_com,wtk_source_t *src)
{
	int ret;
	int row,col;
//	float scale,scale2;
	float scale;
	int row2,col2;
	int height_out,height_in,num_filters_out,num_filters_in;
	ret=wtk_source_fill(src,(char*)(&height_out),4);
        if(ret!=0){goto end;}
        ret=wtk_source_fill(src,(char*)(&num_filters_out),4);
        if(ret!=0){goto end;}
        ret=wtk_source_fill(src,(char*)(&height_in),4);
        if(ret!=0){goto end;}
        ret=wtk_source_fill(src,(char*)(&num_filters_in),4);
        if(ret!=0){goto end;}
	conv_com->model = (qtk_timeheight_convolution_model_t*)wtk_malloc(sizeof(qtk_timeheight_convolution_model_t));
	conv_com->model->height_out=height_out;
//	wtk_debug("model height out=%d\n",height_out);
        conv_com->model->height_in=height_out;
//      wtk_debug("model height in=%d\n",height_in);
        conv_com->model->num_filters_out=num_filters_out;
//      wtk_debug("model filter out=%d\n",num_filters_out);
        conv_com->model->num_filters_in=num_filters_in;
//      wtk_debug("model filter in=%d\n",num_filters_in);

//     	ret=wtk_source_fill(src,(char*)(&scale),4);
//        if(ret!=0){goto end;}
//        ret=wtk_source_fill(src,(char*)(&min),4);
//        if(ret!=0){goto end;}
        ret = wtk_source_fill(src, (char *)(&row), 4);
        if (ret != 0) {
            goto end;
        }
        ret = wtk_source_fill(src, (char *)(&col), 4);
        if (ret != 0) {
            goto end;
        }
        conv_com->bias_params = qtk_blas_matrix_new(row,col);
	ret=wtk_source_fill(src,(char*)(conv_com->bias_params->m),row*col*4);
	if(ret!=0){goto end;}
//	conv_com->bias_params->scale =scale;
	conv_com->bias_params->row =row;
	conv_com->bias_params->col =col;

	ret=wtk_source_fill(src,(char*)(&scale),4);
        if(ret!=0){goto end;}
        if(ret!=0){goto end;}
        ret=wtk_source_fill(src,(char*)(&row2),4);
        if(ret!=0){goto end;}
	ret=wtk_source_fill(src,(char*)(&col2),4);
        if(ret!=0){goto end;}
	conv_com->linear_params2 = wtk_mats_new(row2,col2);
	ret=wtk_source_fill(src,(char*)(conv_com->linear_params2->p),row2*col2*2);
	if(ret!=0){goto end;}
	conv_com->linear_params2->scale =scale;
	conv_com->linear_params2->row =row2;
	conv_com->linear_params2->col =col2;

    conv_com->input=NULL;
    conv_com->output=NULL;
    conv_com->output_shape=NULL;
end:
	return ret;
}

int qtk_nnet3_scale_offset_component_source_fill(qtk_scale_offset_component_t* scaleoff_com,wtk_source_t *src)
{
	int ret;
	int row,col;
	float scale;
	int row2,col2;
	ret=wtk_source_fill(src,(char*)(&scale),4);
        if(ret!=0){goto end;}
        ret=wtk_source_fill(src,(char*)(&row),4);
        if(ret!=0){goto end;}
	ret=wtk_source_fill(src,(char*)(&col),4);
        if(ret!=0){goto end;}
	scaleoff_com->scales2 = wtk_mats_new(row,col);
	ret=wtk_source_fill(src,(char*)(scaleoff_com->scales2->p),row*col*2);
	if(ret!=0){goto end;}
	scaleoff_com->scales2->scale =scale;
	scaleoff_com->scales2->row =row;
	scaleoff_com->scales2->col =col;

        ret=wtk_source_fill(src,(char*)(&scale),4);
        if(ret!=0){goto end;}
        ret=wtk_source_fill(src,(char*)(&row2),4);
        if(ret!=0){goto end;}
	ret=wtk_source_fill(src,(char*)(&col2),4);
        if(ret!=0){goto end;}
	scaleoff_com->offsets2 = wtk_mats_new(row2,col2);
	ret=wtk_source_fill(src,(char*)(scaleoff_com->offsets2->p),row2*col2*2);
	if(ret!=0){goto end;}
        scaleoff_com->offsets2->scale =scale;
	scaleoff_com->offsets2->row =row2;
	scaleoff_com->offsets2->col =col2;

end:
	return ret;
}

int qtk_nnet3_backpro_tru_component_source_fill(qtk_backprop_truncation_component_t *backpro_com,wtk_source_t *src)
{
       int ret;
       float scale;
       ret=wtk_source_fill(src,(char*)(&scale),4);
       if(ret!=0){goto end;}
       backpro_com->scale =scale;
end:
        return ret;
}


void qtk_affine_global_propagate_fix(nnet3_component_t* com1, wtk_mats_t *input,
		wtk_mats_t *out_put,qtk_blas_matrix_t *out_putf, qtk_nnet3_submatrix_info_t *input_info,
		qtk_nnet3_submatrix_info_t *out_put_info)
{
	qtk_affine_global_component_t* com;
	int col, row, i, j, row_offset, col_offset;
	wtk_mats_t *m;
	qtk_blas_matrix_t *b;
	com = (qtk_affine_global_component_t*) com1->component;
	m = com->ws;
	b = com->b;

	col = out_put_info->num_cols;
	row = out_put_info->num_rows;
	col_offset = out_put_info->col_offset;
	row_offset = out_put_info->row_offset;
	//print_short(input->p,20);
	//print_short(m->p,20);
	int *p3;
	short *p1, *p2, *pe2, *pe1;
	int col2, col3;
	wtk_mati_t *mi;
	mi =wtk_mati_new(out_putf->row,out_putf->col);
	wtk_mati_zero(mi);
	//wtk_debug("row=%d,col=%d,row_input=%d,col_input=%d,out_col=%d,out_row=%d,row_win=%d,col_win=%d\n",row,col,input->row,input->col,out_put->col,out_put->row,m->row,m->col);
	col2 = m->col;
	col3 = (col2 >> 2) << 2;
	for (i = 0; i < row; i++)
	{
//		wtk_debug("i=%d\n",i);
		p3=mi->p + (i + row_offset) * out_put->col + col_offset;
//		p3 = out_put->p + (i + row_offset) * out_put->col + col_offset;
		for (j = 0; j < col; j++)
		{
			p1 = input->p + (input_info->row_offset + i) * input->col
					+ input_info->col_offset;
			p2 = m->p + j * col2;
			pe2 = p2 + col3;
			pe1 = p2 + col2;
//			wtk_debug("x=%d,j=%d\n",x,j);
			while (p2 < pe2)
			{
				*p3 += (*p1) * (*p2) + (*(p1 + 1)) * (*(p2 + 1))
						+ (*(p1 + 2)) * (*(p2 + 2)) + (*(p1 + 3)) * (*(p2 + 3));
			//wtk_debug("p3=%d,%dx%d  + %dx%d + %dx%d + %dx%d\n",*p3, *p1,*p2,(*(p1+1)),(*(p2+1)),(*(p1+2)),(*(p2+2)),(*(p1+3)),(*(p2+3)));
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
//			wtk_debug("x=%d,,,,p3 yyyyyy= %d\n",x,*p3);
			p3++;
		}
	}
	//exit(0);
//wtk_debug("row=%d,col=%d,row2=%d,col2=%di,offset=%d,%d\n",m->row,m->col,out_putf->row,out_putf->col,row_offset,col_offset);
//print_int(mi->p,mi->row*mi->col);
    //wtk_debug("scale=%f scale=%f", m->scale, input->scale);
	qtk_nnet3_update_fix_s2f(m->scale,input->scale,mi,out_putf->m);
	//print_float(out_putf->m,20);
	//qtk_blas_matrix_print(b);
	//exit(0);
	wtk_mati_delete(mi);
	//wtk_debug("com->type=%d\n", com->type);
	if(com->type!= QTK_LinearComponent)
	{	
		for (i = 0; i < row; i++)
		{
			for (j = 0; j < col; j++)
			{
				*(out_putf->m + (i + row_offset) * out_putf->col + j + col_offset) +=
					*(b->m + j);
			}
		}	
	}
}

void qtk_affine_global_propagate_fixi(nnet3_component_t* com1, wtk_mats_t *input,
		wtk_mats_t *out_put,qtk_blas_matrix_t *out_putf, qtk_nnet3_submatrix_info_t *input_info,
		qtk_nnet3_submatrix_info_t *out_put_info)
{
	qtk_affine_global_component_t* com;
	int col, row, i, j, row_offset, col_offset;
	wtk_mats_t *m;
	qtk_blas_matrix_t *b;
	com = (qtk_affine_global_component_t*) com1->component;
	m = com->ws;
	b = com->b;

	col = out_put_info->num_cols;
	row = out_put_info->num_rows;
	col_offset = out_put_info->col_offset;
	row_offset = out_put_info->row_offset;
	//print_short(input->p,20);
	//print_short(m->p,20);
	int *p3;
	int *p1, *p2, *pe2, *pe1;
	int col2, col3;
	wtk_mati_t *mi;
	mi =wtk_mati_new(out_putf->row,out_putf->col);
	wtk_mati_zero(mi);
	//wtk_debug("row=%d,col=%d,row_input=%d,col_input=%d,out_col=%d,out_row=%d,row_win=%d,col_win=%d\n",row,col,input->row,input->col,out_put->col,out_put->row,m->row,m->col);
	col2 = m->col;
	col3 = (col2 >> 2) << 2;
	for (i = 0; i < row; i++)
	{
		//wtk_debug("i=%d\n",i);
		p3=mi->p + (i + row_offset) * out_put->col + col_offset;
//		p3 = out_put->p + (i + row_offset) * out_put->col + col_offset;
		for (j = 0; j < col; j++)
		{
			p1 = (int*)input->p + (input_info->row_offset + i) * input->col
					+ input_info->col_offset;
			p2 = (int*)m->p + j * col2;
			pe2 = p2 + col3;
			pe1 = p2 + col2;
			//wtk_debug("x=%d,j=%d\n",x,j);
			while (p2 < pe2)
			{
				*p3 += (*p1) * (*p2) + (*(p1 + 1)) * (*(p2 + 1))
						+ (*(p1 + 2)) * (*(p2 + 2)) + (*(p1 + 3)) * (*(p2 + 3));
			//wtk_debug("p3= %d ,%dx%d  + %dx%d + %dx%d + %dx%d\n",*p3, *p1,*p2,(*(p1+1)),(*(p2+1)),(*(p1+2)),(*(p2+2)),(*(p1+3)),(*(p2+3)));
				p1 += 4;
				p2 += 4;
			}
			while(p2 < pe1)
			{
			  *p3 += *(p1)*(*p2);
			  //wtk_debug("p3=%d,%dx%d  + %dx%d + %dx%d + %dx%d\n",*p3, *p1,*p2,(*(p1+1)),(*(p2+1)),(*(p1+2)),(*(p2+2)),(*(p1+3)),(*(p2+3)));
			  p1++;
			  p2++;
			}
			//*p3+=t;
			//wtk_debug("yyyyyy= %d\n",*p3);
			p3++;
		}
	}
	//exit(0);
//wtk_debug("row=%d,col=%d,row2=%d,col2=%di,offset=%d,%d\n",m->row,m->col,out_putf->row,out_putf->col,row_offset,col_offset);
//print_int(mi->p,mi->row*mi->col);
//exit(0);
//wtk_debug("scale=%f scale=%f", m->scale, input->scale);
	qtk_nnet3_update_fix_s2f(m->scale,input->scale,mi,out_putf->m);
	//print_float(out_putf->m,20);
	//print_float(out_putf->m,out_putf->col*out_putf->row);
	//qtk_blas_matrix_print(b);
	//exit(0);
	wtk_mati_delete(mi);
	//wtk_debug("com->type=%d\n", com->type);
	if(com->type!= QTK_LinearComponent)
	{
		for (i = 0; i < row; i++)
		{
			for (j = 0; j < col; j++)
			{
				*(out_putf->m + (i + row_offset) * out_putf->col + j + col_offset) +=
					*(b->m + j);
			}
		}
	}
}
void qtk_affine_global_propagate_fixiop1(nnet3_component_t* com1, wtk_mats_t *input,
		wtk_mats_t *out_put,qtk_blas_matrix_t *out_putf, qtk_nnet3_submatrix_info_t *input_info,
		qtk_nnet3_submatrix_info_t *out_put_info, int porder)
{
	qtk_affine_global_component_t* com;
	int col, row, i, j, row_offset, col_offset;
	wtk_mats_t *m;
	qtk_blas_matrix_t *b;
	int k, step;

	step = 1 << porder;
	com = (qtk_affine_global_component_t*) com1->component;
	m = com->ws;
	b = com->b;
	col = out_put_info->num_cols;
	row = out_put_info->num_rows;
	col_offset = out_put_info->col_offset;
	row_offset = out_put_info->row_offset;
	//print_short(input->p,20);
	//print_short(m->p,20);
	int *p3, *bp3;
	int *p1, *p2, *bp1;
	int col2, col3, row2, row3, icol;
	int n,r,d, ns;

	if(porder != 4)
	{
		wtk_debug("error: porder should set 2[porder=%d]\n", porder);
		return ;
	}
	wtk_mati_t *mi;
	mi =wtk_mati_new(out_putf->row,out_putf->col);
	wtk_mati_zero(mi);
	//wtk_debug("row=%d,col=%d,row_input=%d,col_input=%d,out_col=%d,out_row=%d,row_win=%d,col_win=%d\n",row,col,input->row,input->col,out_put->col,out_put->row,m->row,m->col);
	col2 = m->col;
	col3 = (col2 >> porder) << porder;
	ns = col3 >> porder;
	row2 = m->row;
	row3 = (row2 >> porder) << porder;
	icol = input->col;
	bp3 = mi->p + row_offset * out_put->col + col_offset;
	p2=(int*)m->p;
	//wtk_debug("step=%d\n", step);
	for(i=0; i < row; i++)
	{
		p3 = bp3 + i * out_put->col;
		bp1 = (int*)input->p + (input_info->row_offset+i) * icol + input_info->col_offset;
		for(k=0;k<ns; k++)
		{
			p1 = bp1;
			n=row2;
			while(n--)
			{
				*p3 += *p1 * (*p2);
				*(p3+1) += *p1 * (*(p2+1));
				*(p3+2) += *p1 * (*(p2+2));
				*(p3+3) += *p1 * (*(p2+3));
				*(p3+4) += *p1 * (*(p2+4));
				*(p3+5) += *p1 * (*(p2+5));
				*(p3+6) += *p1 * (*(p2+6));
				*(p3+7) += *p1 * (*(p2+7));
				*(p3+8) += *p1 * (*(p2+8));
				*(p3+9) += *p1 * (*(p2+9));
				*(p3+10) += *p1 * (*(p2+10));
				*(p3+11) += *p1 * (*(p2+11));
				*(p3+12) += *p1 * (*(p2+12));
				*(p3+13) += *p1 * (*(p2+13));
				*(p3+14) += *p1 * (*(p2+14));
				*(p3+15) += *p1 * (*(p2+15));
				p1++;
				p2 += step;
			}
			p3+=step;
		}
		r=col2-col3;
		while(r>=4)
		{
			r=r-4;
			p1 = bp1;
			n=row2;
			while(n--)
			{
				*p3 += *p1 * (*p2);
				*(p3+1) += *p1 * (*(p2+1));
				*(p3+2) += *p1 * (*(p2+2));
				*(p3+3) += *p1 * (*(p2+3));
				p1++;
				p2 += 4;
			}
			p3 += 4;
		}
		for(d=0; d<r; d++)
		{
			p1 = bp1;
			for(n=0; n<row3; n+=step)
			{
				*p3 += (*p1) * (*p2) + (*(p1 + 1)) * (*(p2 + 1))
										+ (*(p1 + 2)) * (*(p2 + 2)) + (*(p1 + 3)) * (*(p2 + 3));
				p1 += step;
				p2 += step;
			}
			for(; n<row2;n++)
			{
				*p3 += (*p1) * (*p2);
				p1++;
				p2++;
			}
			p3++;
		}
	}
//	print_int(mi->p,mi->row*mi->col);
//	exit(0);
	qtk_nnet3_update_fix_s2f(m->scale,input->scale,mi,out_putf->m);
	//print_float(out_putf->m,20);
	//print_float(out_putf->m,out_putf->col*out_putf->row);
	//qtk_blas_matrix_print(b);
	//exit(0);
	wtk_mati_delete(mi);
	//wtk_debug("com->type=%d\n", com->type);
	if(com->type!= QTK_LinearComponent)
	{
		for (i = 0; i < row; i++)
		{
			for (j = 0; j < col; j++)
			{
				*(out_putf->m + (i + row_offset) * out_putf->col + j + col_offset) +=
					*(b->m + j);
			}
		}
	}
}
#ifdef __ANDROID__
#ifdef USE_NEON
#include <arm_neon.h>
void qtk_affine_global_propagate_fixiop1_neon(nnet3_component_t* com1, wtk_mats_t *input,
		wtk_mats_t *out_put,qtk_blas_matrix_t *out_putf, qtk_nnet3_submatrix_info_t *input_info,
		qtk_nnet3_submatrix_info_t *out_put_info, int porder)
{
	qtk_affine_global_component_t* com;
	int col, row, i, j, row_offset, col_offset;
	wtk_mats_t *m;
	qtk_blas_matrix_t *b;
	int k, step;
	int *p3, *bp3;
	int *p1, *p2, *bp1;
	int col2, col3, row2, row3, icol;
	int n,r,d, ns;
	int32x4_t x,y,y1,y2,y3;
	wtk_mati_t *mi;

	if(porder != 4)
	{
		wtk_debug("error: porder should set 4[porder=%d]\n", porder);
		return ;
	}

	step = 1 << porder;
	com = (qtk_affine_global_component_t*) com1->component;
	m = com->ws;
	b = com->b;
	col = out_put_info->num_cols;
	row = out_put_info->num_rows;
	col_offset = out_put_info->col_offset;
	row_offset = out_put_info->row_offset;
	//print_short(input->p,20);
	//print_short(m->p,20);

	mi =wtk_mati_new(out_putf->row,out_putf->col);
	wtk_mati_zero(mi);
	//wtk_debug("row=%d,col=%d,row_input=%d,col_input=%d,out_col=%d,out_row=%d,row_win=%d,col_win=%d\n",row,col,input->row,input->col,out_put->col,out_put->row,m->row,m->col);
	col2 = m->col;
	col3 = (col2 >> porder) << porder;
	ns = col3 >> porder;
	row2 = m->row;
	row3 = (row2 >> porder) << porder;
	icol = input->col;
	bp3 = mi->p + row_offset * out_put->col + col_offset;
	p2=(int*)m->p;
	for(i=0; i < row; i++)
	{
		p3 = bp3 + i * out_put->col;
		bp1 = (int*)input->p + (input_info->row_offset+i) * icol + input_info->col_offset;
		for(k=0;k<ns; k++)
		{
			p1 = bp1;
			n=row2;
			y=vld1q_s32(p3);
			y1=vld1q_s32(p3+4);
			y2=vld1q_s32(p3+8);
			y3=vld1q_s32(p3+12);
			while(n--)
			{
				x = vdupq_n_s32(*p1);
				y = vaddq_s32(y, vmulq_s32(x, vld1q_s32(p2)));
				y1 = vaddq_s32(y1, vmulq_s32(x, vld1q_s32(p2+4)));
				y2 = vaddq_s32(y2, vmulq_s32(x, vld1q_s32(p2+8)));
				y3 = vaddq_s32(y3, vmulq_s32(x, vld1q_s32(p2+12)));
				p1++;
				p2 += step;
			}
			vst1q_s32(p3,y);
			vst1q_s32(p3+4,y1);
			vst1q_s32(p3+8,y2);
			vst1q_s32(p3+12,y3);
			p3+=step;
		}
		r=col2-col3;
		while(r>=4)
		{
			r=r-4;
			p1 = bp1;
			n=row2;
			y=vld1q_s32(p3);
			while(n--)
			{
				x = vdupq_n_s32(*p1);
				y = vaddq_s32(y, vmulq_s32(x, vld1q_s32(p2)));
				p1++;
				p2 += 4;
			}
			vst1q_s32(p3,y);
			p3 += 4;
		}
		for(d=0; d<r; d++)
		{
			p1 = bp1;
			for(n=0; n<row3; n+=4)
			{
				*p3 += (*p1) * (*p2) + (*(p1 + 1)) * (*(p2 + 1))
														+ (*(p1 + 2)) * (*(p2 + 2)) + (*(p1 + 3)) * (*(p2 + 3));
				//x=vaddq_s32(vld1q_s32(p1),vld1q_s32(p2));
				//*p3 += x[0]+x[1]+x[2]+x[3];
				p1 += 4;
				p2 += 4;
			}
			for(; n<row2;n++)
			{
				*p3 += (*p1) * (*p2);
				p1++;
				p2++;
			}
			p3++;
		}
	}

	qtk_nnet3_update_fix_s2f(m->scale,input->scale,mi,out_putf->m);
	//print_float(out_putf->m,20);
	//print_float(out_putf->m,out_putf->col*out_putf->row);
	//qtk_blas_matrix_print(b);
	//exit(0);
	wtk_mati_delete(mi);
	//wtk_debug("com->type=%d\n", com->type);
	if(com->type!= QTK_LinearComponent)
	{
		for (i = 0; i < row; i++)
		{
			for (j = 0; j < col; j++)
			{
				*(out_putf->m + (i + row_offset) * out_putf->col + j + col_offset) +=
					*(b->m + j);
			}
		}
	}
}
#endif
#endif
#ifdef USE_NEON64
void qtk_affine_global_propagate_neon64(nnet3_component_t* com1, wtk_mats_t *input,
		wtk_mats_t *out_put,qtk_blas_matrix_t *out_putf, qtk_nnet3_submatrix_info_t *input_info,
		qtk_nnet3_submatrix_info_t *out_put_info)
{
	qtk_affine_global_component_t* com;
	int col, row, i, j, row_offset, col_offset;
	wtk_mats_t *m;
	qtk_blas_matrix_t *b;
	com = (qtk_affine_global_component_t*) com1->component;
	m = com->ws;
	b = com->b;

	col = m->col;//out_put_info->num_cols;
	row = m->row;//out_put_info->num_rows;
	col_offset = out_put_info->col_offset;
	row_offset = out_put_info->row_offset;

	wtk_mati_t *mi;
	mi =wtk_mati_new(out_putf->row,out_putf->col);
	wtk_mati_zero(mi);

	int r_cnt=0;
	int i_col=input->col;
	int o_col=out_put->col;
	int cnt=col%4;
	int st=col-cnt;
	int *dst=(int*)wtk_malloc(sizeof(int)*16);
	//wtk_debug("%d %d %d %d\n",cnt,st,row,col);
//	wtk_debug("%d %d\n",input_info->num_rows,input_info->num_cols);
//	wtk_debug("%d %d\n",row,col);
//	wtk_debug("%d %d\n",out_put_info->num_rows,out_put_info->num_cols);
//	wtk_debug("%d %d\n",input_info->row_offset,input_info->col_offset);
//	wtk_debug("%d %d\n",out_put_info->row_offset,out_put_info->col_offset);
	while((input_info->num_rows-r_cnt)>=4)
	{
		//wtk_debug("%d %d\n",row,r_cnt);
		short *p1=m->p;
		int xt[4];
		int *pi,*pi1,*pi2,*pi3;
		//wtk_debug("%d\n",(r_cnt + row_offset) * o_col + col_offset);
		pi=mi->p+ (r_cnt + row_offset) * o_col + col_offset;
		pi1=pi+o_col;
		pi2=pi1+o_col;
		pi3=pi2+o_col;

		int i,cnt;
		for(i=0;i<row;i++)
		{
			short *p21 = input->p+r_cnt*i_col;
			short *p22=p21+col;
			short *p23=p22+col;
			short *p24=p23+col;
			//wtk_debug("xxx %d\n",r_cnt);
			//wtk_debug("%d %d %d %d %d\n",*p21,*p22,*p23,*p24,*p1);
			asm volatile(
					"movi      v8.4s,#0      \n"
					"movi      v9.4s,#0      \n"
					"movi      v10.4s,#0    \n"
					"movi      v11.4s,#0    \n"
					"ldr	         x1,[%[col]]  \n"

					"1:\n"
					"subs      w1,w1,#16\n"
					"blt         2f\n"
					"ld1        {v0.4h,v1.4h,v2.4h,v3.4h},[%[p1]],#32\n"
					"ld1        {v4.4h,v5.4h,v6.4h,v7.4h},[%[p21]],#32\n"
					"smlal     v8.4s,v0.4h,v4.4h\n"
					"smlal     v8.4s,v1.4h,v5.4h\n"
					"smlal     v8.4s,v2.4h,v6.4h\n"
					"smlal     v8.4s,v3.4h,v7.4h\n"

					"ld1        {v4.4h,v5.4h,v6.4h,v7.4h},[%[p22]],#32\n"
					"smlal     v9.4s,v0.4h,v4.4h\n"
					"smlal     v9.4s,v1.4h,v5.4h\n"
					"smlal     v9.4s,v2.4h,v6.4h\n"
					"smlal     v9.4s,v3.4h,v7.4h\n"

					"ld1        {v4.4h,v5.4h,v6.4h,v7.4h},[%[p23]],#32\n"
					"smlal     v10.4s,v0.4h,v4.4h\n"
					"smlal     v10.4s,v1.4h,v5.4h\n"
					"smlal     v10.4s,v2.4h,v6.4h\n"
					"smlal     v10.4s,v3.4h,v7.4h\n"

					"ld1        {v4.4h,v5.4h,v6.4h,v7.4h},[%[p24]],#32\n"
					"smlal     v11.4s,v0.4h,v4.4h\n"
					"smlal     v11.4s,v1.4h,v5.4h\n"
					"smlal     v11.4s,v2.4h,v6.4h\n"
					"smlal     v11.4s,v3.4h,v7.4h\n"
					"b 1b\n"

					"2:\n"
					"add        w1,w1,#16\n"

					"3:\n"
					"subs      w1,w1,#4\n"
					"blt         4f\n"
					"ld1        {v0.4h},[%[p1]],#8\n"
					"ld1        {v4.4h},[%[p21]],#8\n"
					"smlal     v8.4s,v0.4h,v4.4h\n"

					"ld1        {v4.4h},[%[p22]],#8\n"
					"smlal     v9.4s,v0.4h,v4.4h\n"

					"ld1        {v4.4h},[%[p23]],#8\n"
					"smlal     v10.4s,v0.4h,v4.4h\n"

					"ld1        {v4.4h},[%[p24]],#8\n"
					"smlal     v11.4s,v0.4h,v4.4h\n"
			//
					"b           3b\n"
					"4:\n"
					"add      w1,w1,#4\n"
					//"saddlp   v8.2d,         v8.4s\n"
					//"st1         {v8.4s},       [%[dst]]\n"
					"st1         {v8.4s,v9.4s,v10.4s,v11.4s},       [%[dst]]\n"
					//"st1         {},       [%[dst]]\n"
					: [dst] "+r" (dst)
					: [col] "r" (&col),
					  [p1] "r" (p1),
					  [p21] "r" (p21),
					  [p22] "r" (p22),
					  [p23] "r" (p23),
					  [p24] "r" (p24)
					 :"memory","cc","x1","w1","v0","v1","v2","v3","v4","v5","v6","v7","v8","v9","v10","v11"
					);
//			for(int xx=0;xx<16;xx++)
//			{
//				wtk_debug("%d\n",*(dst+xx));
//			}
			xt[0]=dst[0]+dst[1]+dst[2]+dst[3];
			xt[1]=dst[4]+dst[5]+dst[6]+dst[7];
			xt[2]=dst[8]+dst[9]+dst[10]+dst[11];
			xt[3]=dst[12]+dst[13]+dst[14]+dst[15];
//			wtk_debug("%d %d %d %d\n",xt[0],xt[1],xt[2],xt[3]);
			switch (cnt)
			{
			case 1:
				xt[0]+=p1[st]*p21[st];
				xt[1]+=p1[st]*p22[st];
				xt[2]+=p1[st]*p23[st];
				xt[3]+=p1[st]*p24[st];
				break;
			case 2:
				xt[0]+=p1[st]*p21[st]+p1[st+1]*p21[st+1];
				xt[1]+=p1[st]*p22[st]+p1[st+1]*p22[st+1];
				xt[2]+=p1[st]*p23[st]+p1[st+1]*p23[st+1];
				xt[3]+=p1[st]*p24[st]+p1[st+1]*p24[st+1];
				break;
			case 3:
				xt[0]+=p1[st]*p21[st]+p1[st+1]*p21[st+1]+p1[st+2]*p21[st+2];
				xt[1]+=p1[st]*p22[st]+p1[st+1]*p22[st+1]+p1[st+2]*p22[st+2];
				xt[2]+=p1[st]*p23[st]+p1[st+1]*p23[st+1]+p1[st+2]*p23[st+2];
				xt[3]+=p1[st]*p24[st]+p1[st+1]*p24[st+1]+p1[st+2]*p24[st+2];
				break;
			default:
				break;
			}
			//wtk_debug("haha %d \n",i);
				pi[i]=xt[0];
				pi1[i]=xt[1];
				pi2[i]=xt[2];
				pi3[i]=xt[3];
			}
			r_cnt+=4;
		}
//	print_int(mi->p,mi->row*mi->col);
//	exit(0);
//print_short(input->p,20);
	//wtk_debug("%d\n",r_cnt);
	col = out_put_info->num_cols;
	row = out_put_info->num_rows;
	int *p3;
	short *p1, *p2, *pe2, *pe1;
	int col2, col3;
	col2 = m->col;
	col3 = (col2 >> 2) << 2;
	for (i = r_cnt; i < row; i++)
	{
//		wtk_debug("i=%d\n",i);
		p3=mi->p + (i + row_offset) * out_put->col + col_offset;
//		p3 = out_put->p + (i + row_offset) * out_put->col + col_offset;
		for (j = 0; j < col; j++)
		{
			p1 = input->p + (input_info->row_offset + i) * input->col
					+ input_info->col_offset;
			p2 = m->p + j * col2;
			pe2 = p2 + col3;
			pe1 = p2 + col2;
//			wtk_debug("x=%d,j=%d\n",x,j);
			while (p2 < pe2)
			{
				*p3 += (*p1) * (*p2) + (*(p1 + 1)) * (*(p2 + 1))
						+ (*(p1 + 2)) * (*(p2 + 2)) + (*(p1 + 3)) * (*(p2 + 3));
			//wtk_debug("p3=%d,%dx%d  + %dx%d + %dx%d + %dx%d\n",*p3, *p1,*p2,(*(p1+1)),(*(p2+1)),(*(p1+2)),(*(p2+2)),(*(p1+3)),(*(p2+3)));
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
			//wtk_debug("yyyyy %d\n",*p3);
//			wtk_debug("x=%d,,,,p3 yyyyyy= %d\n",x,*p3);
			p3++;
		}
	}
	//print_int(mi->p,mi->row*mi->col);
	qtk_nnet3_update_fix_s2f(m->scale,input->scale,mi,out_putf->m);
//	print_float(out_putf->m,20);
	wtk_mati_delete(mi);
	if(com->type!= QTK_LinearComponent)
	{
		for (i = 0; i < row; i++)
		{
			for (j = 0; j < col; j++)
			{
				*(out_putf->m + (i + row_offset) * out_putf->col + j + col_offset) +=
					*(b->m + j);
			}
		}
	}
	wtk_free(dst);
}
#elif USE_NEON
#include <arm_neon.h>
int dotProductNeon(short *vector1, short *vector2, int len) {
    int len_1 = len >> 2;
    int16x4_t *a, *b;
    int32x4_t r_1, r_2, r_3, r_4;
    int o_1 = 0, o_2 = 0, o_3 = 0, o_4 = 0;
    int tmp[4] = {0, 0, 0, 0};
    int result = 0;
    a = (int16x4_t *)vector1;
    b = (int16x4_t *)vector2;
    r_1 = r_2 = r_3 = r_4 = vld1q_s32(tmp);
    while (len_1 >= 4) {
        r_1 = vmlal_s16(r_1, a[0], b[0]);
        r_2 = vmlal_s16(r_2, a[1], b[1]);
        r_3 = vmlal_s16(r_3, a[2], b[2]);
        r_4 = vmlal_s16(r_4, a[3], b[3]);
        a += 4;
        b += 4;
        vector1 += 16;
        vector2 += 16;
        len_1 -= 4;
        len -= 16;
    }

    o_1 = vgetq_lane_s32(r_1, 0) + vgetq_lane_s32(r_1, 1) +
          vgetq_lane_s32(r_1, 2) + vgetq_lane_s32(r_1, 3);
    o_2 = vgetq_lane_s32(r_2, 0) + vgetq_lane_s32(r_2, 1) +
          vgetq_lane_s32(r_2, 2) + vgetq_lane_s32(r_2, 3);
    o_3 = vgetq_lane_s32(r_3, 0) + vgetq_lane_s32(r_3, 1) +
          vgetq_lane_s32(r_3, 2) + vgetq_lane_s32(r_3, 3);
    o_4 = vgetq_lane_s32(r_4, 0) + vgetq_lane_s32(r_4, 1) +
          vgetq_lane_s32(r_4, 2) + vgetq_lane_s32(r_4, 3);
    while (len_1 > 0) {
        r_1 = vmull_s16(a[0], b[0]);
        o_1 += vgetq_lane_s32(r_1, 0) + vgetq_lane_s32(r_1, 1) +
               vgetq_lane_s32(r_1, 2) + vgetq_lane_s32(r_1, 3);
        a += 1;
        b += 1;
        vector1 += 4;
        vector2 += 4;
        --len_1;
        len -= 4;
    }
    result = o_1 + o_2 + o_3 + o_4;
    while (len > 0) {
        result += *vector1++ * *vector2++;
        --len;
    }

    return result;
}

void qtk_affine_global_propagate_neon32(
    nnet3_component_t *com1, wtk_mats_t *input, wtk_mats_t *out_put,
    qtk_blas_matrix_t *out_putf, qtk_nnet3_submatrix_info_t *input_info,
    qtk_nnet3_submatrix_info_t *out_put_info) {
    qtk_affine_global_component_t *com;
    unsigned i, j, k;
    int out_col, out_row, out_roffset, out_coffset;
    wtk_mats_t *m;
    qtk_blas_matrix_t *b;

    com = (qtk_affine_global_component_t *)com1->component;
    m = com->ws;
    b = com->b;
    out_col = out_put_info->num_cols;
    out_row = out_put_info->num_rows;
    out_coffset = out_put_info->col_offset;
    out_roffset = out_put_info->row_offset;

    int *out_p;
    short *input_p, *m_p, *m_pe2, *m_pe1;
    int m_col;
    wtk_mati_t *mi;
    mi = wtk_mati_new(out_putf->row, out_putf->col);
    wtk_mati_zero(mi);
    m_col = m->col;
    for (i = 0; i < out_row; i++) {
        out_p = mi->p + (i + out_roffset) * out_put->col + out_coffset;
        for (j = 0; j < out_col; j++) {
            input_p = input->p + (input_info->row_offset + i) * input->col +
                      input_info->col_offset;
            m_p = m->p + j * m_col;
#if 0
            for (k = 0; k < m_col; k++)
            {
                *out_p += *(input_p)*(*m_p);
                input_p++;
                m_p++;
            }
#else
            *out_p = dotProductNeon(input_p, m_p, m_col);
#endif
            out_p++;
        }
    }
    qtk_nnet3_update_fix_s2f(m->scale, input->scale, mi, out_putf->m);
    wtk_mati_delete(mi);
    if (com->type != QTK_LinearComponent) {
        for (i = 0; i < out_row; i++) {
            for (j = 0; j < out_col; j++) {
                *(out_putf->m + (i + out_roffset) * out_putf->col + j +
                  out_coffset) += *(b->m + j);
            }
        }
    }
}
#endif

/*
void qtk_normalize_fix(float *f, int len, float scale)
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
void qtk_normallize_propagate_fix(nnet3_component_t* com1, qtk_blas_matrix_t *dst,
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

*/
void qtk_mats_add_matmat(qtk_sub_matrix_t* dst,wtk_sub_mats_t* src,wtk_sub_mats_t* linear,float scale,float scale2)
{
        short *p1,*p2,*pe1,*pe2;
        int col,row,col2,i,j,col3;
	int* p3;
        col=dst->col;
        row=dst->row;
        col2=linear->col;
        col3=(col2>>2)<<2;
	wtk_mati_t *mi;
	mi = wtk_mati_new(row,col);
	wtk_mati_zero(mi);

        for(i=0;i<row;i++)
        {
           // p3=dst->f+i*dst->stride;
	    p3 = mi->p + i*dst->stride;
            for(j=0;j<col;j++)
            {
                p1=src->p+i*src->stride;
                p2=linear->p+j*linear->stride;
                pe2=p2+col3;
		pe1=p2+col2;
            	while(p2<pe2)
            	{
            		*p3+=(*p1)*(*p2)+(*(p1+1))*(*(p2+1))+(*(p1+2))*(*(p2+2))+(*(p1+3))*(*(p2+3));
		//	wtk_debug("%f x %f + %f x %f + %f x %f +%f x %f\n", (*p1)*scale, (*p2)*scale, (*(p1+1))*scale, (*(p2+1))*scale, (*(p1+2))*scale ,(*(p2+2))*scale ,(*(p1+3))*scale,(*(p2+3))*scale );
            		p1+=4;
            		p2+=4;
            	}
		//	wtk_debug("aaaa%f\n",(*p3)*scale*scale);
		while(p2<pe1)
            	{
              		*p3+=*(p1)*(*p2);
		//	  wtk_debug("bbbb%f %f\n",(*p1)*scale,(*p2)*scale);
              		p1++;
             	 	p2++;
            	}
		//	wtk_debug("cccc%f\n",(*p3)*scale*scale);
            p3++;
            }	
    	}
	for(i=0;i<row*col;++i)
	{
		dst->f[i] += mi->p[i]*scale2*scale;
//		wtk_debug("dst[%d]=%f\n",i,dst->f[i]);
	}
	wtk_mati_delete(mi);
	//exit(0);
}

void wtk_sub_mats_cpy2(wtk_mati_t *dst,wtk_mats_t *src,qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info)
{
	int i,j;

	if(dst_info->num_cols==src_info->num_cols && dst_info->num_rows==src_info->num_rows)
	{
		for(i=0;i<dst_info->num_rows;i++)
		{
			for(j=0;j<dst_info->num_cols;j++)
			{
				*(dst->p+(dst_info->row_offset+i)*(dst->col)+dst_info->col_offset+j)=*(src->p+(src_info->row_offset+i)*(src->col)+src_info->col_offset+j);
			}
		}
	}
}


void qtk_mats_copy_cols(wtk_sub_mats_t* dst,wtk_sub_mats_t* src,int *vec)
{
	int r,c,*index_ptr;
	int num_rows=dst->row;
	int num_cols=dst->col;
	int this_stride=dst->stride;
	int src_stride=src->stride;
	short *this_data,*src_data;
	this_data=dst->p;src_data=src->p;

	for(r=0;r<num_rows;r++,this_data+=this_stride,src_data+=src_stride)
	{
		index_ptr=vec+1;
		for(c=0;c<num_cols;c++,index_ptr++)
		{
			if(*index_ptr<0)
			{
				*(this_data+c)=0;
			}else
			{
				*(this_data+c)=*(src_data+*index_ptr);
			}
		}
	}
}

void wtk_mati_copy_from_mats( wtk_mati_t *dst, wtk_sub_mats_t *src, int stride)
{
	int this_stride=stride;
	int other_stride=src->stride;
	int *this_data=dst->p;
	short *other_data=src->p;
	int i,j;

	for(i=0;i<dst->row;i++)
	{
		for(j=0;j<dst->col;j++)
		{
			*(this_data+i*this_stride+j)=*(other_data+j+i*other_stride);
		}
	}
}

void wtk_mats_copy_from_mats( wtk_sub_mats_t *dst, wtk_sub_mats_t *src)
{
        int this_stride=dst->stride;
        int other_stride=src->stride;
        short *this_data=dst->p;
        short *other_data=src->p;
        int i,j;

        for(i=0;i<dst->row;i++)
        {
                for(j=0;j<dst->col;j++)
                {
                        *(this_data+i*this_stride+j)=*(other_data+j+i*other_stride);
                }
        }
}


void qtk_batchnorm_cal_fix(wtk_sub_mats_t* in, qtk_sub_matrix_t* out,
		qtk_batch_norm_component_t *com, float in_scale)
{
	int i, j, stride;
	short scale, offset;
	wtk_mati_t *outi;
	stride = out->stride;
	outi=wtk_mati_new(out->row,out->col);	
	wtk_mati_copy_from_mats(outi, in, out->stride);
/*        float x=0;
        for(i=0;i<outi->col*outi->row;++i)
        {
                x = outi->p[i]*in_scale;
                wtk_debug("input[%d]=%f\n",i,x);
        }
*/
	for (i = 0; i < out->row; i++)
	{
		for (j = 0; j < out->col; j++)
		{
			scale = *(com->scale2->p + j);
			offset = *(com->offset2->p + j);
//			wtk_debug("%f %f %f\n",scale2,offset2,*(out->f+j+i*stride));
			*(outi->p + j + i * stride) *= scale;
			//wtk_debug("%f %f %f\n",scale,offset,*(out->f+j+i*stride));
			*(outi->p + j + i * stride) += (offset/com->scale2->scale);
		}
	}
	qtk_nnet3_update_fix_s2f(in_scale, com->scale2->scale ,outi,out->f);
	wtk_mati_delete(outi);
}

void qtk_batchnorm_propagate_fix(nnet3_component_t* com1, wtk_mats_t *src,
		qtk_blas_matrix_t *dst, qtk_nnet3_submatrix_info_t *dst_info,
		qtk_nnet3_submatrix_info_t *src_info)
{
	qtk_batch_norm_component_t *com =
			(qtk_batch_norm_component_t*) com1->component;
        qtk_sub_matrix_t output;
        wtk_sub_mats_t input;

        wtk_sub_mats_init(&input, src->p, src_info->row_offset,
                          src_info->num_rows, src_info->col_offset,
                          src_info->num_cols, src->col);
        qtk_sub_matrix_init(&output, dst->m, dst_info->row_offset,
                            dst_info->num_rows, dst_info->col_offset,
                            dst_info->num_cols, dst->col);
        if (input.col != com->block_dim) {
                qtk_sub_matrix_t out_reshape;
                wtk_sub_mats_t in_reshape;
                int ratio = com->dim / com->block_dim;
                int orig_rows = input.row;
                int orig_cols = input.col;
                int new_rows = orig_rows * ratio;
		int new_cols = orig_cols / ratio;
                wtk_sub_mats_init2(&in_reshape, input.p, new_rows, new_cols,
                                   new_cols);
                qtk_sub_matrix_init2(&out_reshape, output.f, new_rows, new_cols,
                                     new_cols);
                qtk_batchnorm_cal_fix(&in_reshape, &out_reshape, com,
                                      src->scale);
        } else {
                qtk_batchnorm_cal_fix(&input, &output, com, src->scale);
        }
}


void qtk_timeheight_convolution_forward_internal_fix(
		qtk_nnet3_convolution_precomputed_indexes_t* index,
		wtk_sub_mats_t* input, wtk_mats_t *linear,
		wtk_sub_mats_t* temp_mat, qtk_sub_matrix_t* output,
		qtk_blas_matrix_t* bias,float scale)
{
	int num_steps = index->num_steps;
	int s;
	qtk_nnet3_convolution_step_t *step;
        qtk_sub_matrix_t output_reshaped;
        wtk_sub_mats_t input_part, temp_mat_part, params_part,
            temp_mat_part_reshaped, input_reshaped;
        //	wtk_debug("cnn forword internal steps %d\n",num_steps);	
	for (s = 0; s < num_steps; s++)
	{
		step = index->steps[s];
		int input_row_start = step->input_time_shift * index->num_images;
		int temp_num_cols = step->columns[0];
		int params = temp_num_cols / index->height_out;
                wtk_sub_mats_init(&input_part, input->p, input_row_start,
                                  output->row, 0, input->col, input->stride);
                //		wtk_debug("%d %d %d
                //%d\n",linear->row,step->params_start_col,linear->col,params);
                wtk_sub_mats_init(&params_part, linear->p, 0, linear->row,
                                  step->params_start_col, params, linear->col);
                qtk_sub_matrix_init2(&output_reshaped, output->f,
                                     output->row * index->height_out,
                                     index->num_filters_out,
                                     index->num_filters_out);

                if (!step->columns_are_contiguous || temp_num_cols != input->col)
		{
                        wtk_sub_mats_init2(&temp_mat_part, temp_mat->p,
                                           temp_mat->row, temp_num_cols,
                                           temp_num_cols);
                        //			wtk_debug("%d\n",step->columns_are_contiguous);
			if (!step->columns_are_contiguous)
			{
                                qtk_mats_copy_cols(&temp_mat_part, &input_part,
                                                   step->columns);
                        } else {
                                wtk_sub_mats_t new_temp;
                                wtk_sub_mats_init(
                                    &new_temp, input_part.p, 0, input_part.row,
                                    step->first_column, step->columns[0],
                                    input_part.stride);
                                wtk_mats_copy_from_mats(&temp_mat_part,
                                                        &new_temp);
                        }

//			wtk_debug("---------tempart\n");
//			qtk_sub_matrix_print(temp_mat_part);
                        wtk_sub_mats_init2(
                            &temp_mat_part_reshaped, temp_mat_part.p,
                            temp_mat_part.row * index->height_out,
                            temp_num_cols / index->height_out,
                            temp_num_cols / index->height_out);
                        //ADDMATMAT
//			wtk_debug("---------temp1\n");
//			qtk_sub_matrix_print(temp_mat_part_reshaped);
//			wtk_debug("---------parms1\n");
//			qtk_sub_matrix_print(params_part);
                        qtk_mats_add_matmat(&output_reshaped,
                                            &temp_mat_part_reshaped,
                                            &params_part, linear->scale, scale);
                } else {
                        wtk_sub_mats_init2(&input_reshaped, input_part.p,
                                           input_part.row * index->height_out,
                                           input_part.col / index->height_out,
                                           input_part.col / index->height_out);
                        //ADDMATMAT
			//wtk_debug("--------temp2\n");
			//qtk_sub_matrix_print(temp_mat_part_reshaped);) {
			//wtk_debug("--------parms2\n");
			//qtk_sub_matrix_print(params_part);
                        qtk_mats_add_matmat(&output_reshaped, &input_reshaped,
                                            &params_part, linear->scale, scale);
                        //wtk_debug("--------sub2\n");
			//qtk_sub_matrix_print(output_reshaped);
                }
        }

}

void qtk_timeheight_convolution_forward_fix(
		qtk_nnet3_convolution_precomputed_indexes_t* index,
		wtk_sub_mats_t* input, wtk_mats_t *linear,
		qtk_sub_matrix_t* output, qtk_blas_matrix_t *bias,float scale)
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
                wtk_sub_mats_t input_reshaped;

                wtk_sub_mats_init2(&input_reshaped, input->p,
                                   required_input_rows, new_num_cols,
                                   new_stride);
                qtk_timeheight_convolution_forward_fix(
                    index, &input_reshaped, linear, output, bias, scale);
                return;
	}

	wtk_mats_t *temp_mat = wtk_mats_new(index->temp_rows,
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
                        qtk_sub_matrix_t output_part;
                        wtk_sub_mats_t input_part, temp_part;

                        wtk_sub_mats_init(&input_part, input->p,
                                          t_start * index->num_images,
                                          this_num_t_in * index->num_images, 0,
                                          input->col, input->stride);
                        qtk_sub_matrix_init(&output_part, output->f,
                                            t_start * index->num_images,
                                            this_num_t_out * index->num_images,
                                            0, output->col, output->stride);
                        wtk_sub_mats_init(&temp_part, temp_mat->p, 0,
                                          this_num_t_out * index->num_images, 0,
                                          temp_mat->col, temp_mat->col);

                        qtk_timeheight_convolution_forward_internal_fix(
                            index, &input_part, linear, &temp_part,
                            &output_part, bias, scale);
                }
		wtk_mats_delete(temp_mat);
		return;
	}
        wtk_sub_mats_t temp;
        wtk_sub_mats_init2(&temp, temp_mat->p, index->temp_rows,
                           index->temp_cols, index->temp_cols);
        qtk_timeheight_convolution_forward_internal_fix(
            index, input, linear, &temp, output, bias, scale);
        wtk_mats_delete(temp_mat);
}

void qtk_mats2_copy_rows_fromvec_fix(qtk_sub_matrix_t* dst,qtk_blas_matrix_t* bias)
{
	int num_rows=dst->row;
	int num_cols=dst->col;
	int stride=dst->stride;	
	int i,j;

	if(bias->col==num_rows*num_cols)
	{
		if(stride==num_cols)
		{
			memcpy(dst->f,bias->m,sizeof(float)*num_rows*num_cols);
		}else
		{
			for(i=0;i<num_rows;i++)
			{
				for(j=0;j<num_cols;j++)
				{
					*(dst->f+i*dst->stride+j)=*(bias->m+j);
				}
			}	
		}
	}else if(bias->col==num_cols)
	{
		for(i=0;i<num_rows;i++)
		{
			memcpy(dst->f+i*dst->stride,bias->m,sizeof(float)*num_cols);
		}
	}else
	{
		wtk_debug("error copy rows\n");
	}
}

void qtk_timeheight_convolution_propagate_fix(nnet3_component_t* com1,
		qtk_blas_matrix_t *dst, wtk_mats_t *src,
		qtk_nnet3_submatrix_info_t *dst_info,
		qtk_nnet3_submatrix_info_t *src_info,
		qtk_nnet3_precomputed_indexes_t* index)
{
	qtk_timeheight_convolution_component_t *com = com1->component;
	qtk_nnet3_convolution_precomputed_indexes_t *precompute =
			(qtk_nnet3_convolution_precomputed_indexes_t*) index->index;
        qtk_sub_matrix_t output, output_shape;
        wtk_sub_mats_t input;
        qtk_timeheight_convolution_model_t *model = com->model;

        wtk_sub_mats_init(&input, src->p, src_info->row_offset,
                          src_info->num_rows, src_info->col_offset,
                          src_info->num_cols, src->col);
        qtk_sub_matrix_init(&output, dst->m, dst_info->row_offset,
                            dst_info->num_rows, dst_info->col_offset,
                            dst_info->num_cols, dst->col);
        qtk_sub_matrix_init2(&output_shape, output.f,
                             output.row * model->height_out,
                             model->num_filters_out, model->num_filters_out);
        qtk_mats2_copy_rows_fromvec_fix(&output_shape, com->bias_params);

        qtk_timeheight_convolution_forward_fix(precompute, &input,
                                               com->linear_params2, &output,
                                               com->bias_params, src->scale);
}


void qtk_scale_offset_propagate_fix(nnet3_component_t* com1, qtk_blas_matrix_t *dst,
		wtk_mats_t *src, qtk_nnet3_submatrix_info_t *dst_info,
		qtk_nnet3_submatrix_info_t *src_info)
{
	qtk_scale_offset_component_t *com =
			(qtk_scale_offset_component_t*) com1->component;
	float scale, offset;
	int i, j, multiple, dim;
	wtk_mati_t *dsti;

	dsti = wtk_mati_new(dst->row,dst->col);
	wtk_mati_zero(dsti);
	dim = com->scales->col;
	multiple = com->dim / dim;
	wtk_sub_mats_cpy2(dsti, src, dst_info, src_info);
	//wtk_debug("%d\n",dst_info->num_cols);
	//wtk_debug("scalex %d %d\n",multiple,dim);
	for (i = 0; i < multiple * src_info->num_rows; i++)
	{
		for (j = 0; j < dim; j++)
		{
			scale = *(com->scales2->p + j);
			offset = *(com->offsets2->p + j);
			*(dsti->p + dst_info->col_offset + j
					+ (dst_info->row_offset + i) * dim) *= scale;
			*(dsti->p + dst_info->col_offset + j
					+ (dst_info->row_offset + i) * dim) += (offset/com->scales2->scale);
		}
	}
	qtk_nnet3_update_fix_s2f(src->scale,com->scales2->scale,dsti,dst->m);
	wtk_mati_delete(dsti);
}
