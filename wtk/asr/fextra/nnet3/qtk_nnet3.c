#include "qtk_nnet3.h"

qtk_nnet3_t* qtk_nnet3_new(qtk_nnet3_cfg_t* cfg, int col,int use_ivec)
{
    unsigned int i;
    qtk_nnet3_t *nnet3 = (qtk_nnet3_t *)wtk_malloc(sizeof(qtk_nnet3_t));

    nnet3->cfg = cfg;
    nnet3->gc =
        (qtk_blas_matrix_t **)wtk_malloc(sizeof(qtk_blas_matrix_t *) * 30000);
    nnet3->gc_count = 0;
    nnet3->left  =  -cfg->left_context;
    nnet3->right  =  cfg->right_context;
	nnet3->input_num = cfg->compution->input_indexes[0];
	nnet3->input_index = cfg->compution->input_indexes;
	nnet3->input_col = col;
	nnet3->delta = 0;

	nnet3->ivector=NULL;
	nnet3->ivector_dim = 100;
	nnet3->use_ivector = use_ivec;
	nnet3->extra = 0;
        nnet3->LayerInfo = (QtkNnet3LayerInfo_t **)wtk_malloc(
            sizeof(QtkNnet3LayerInfo_t *) * cfg->comp_cnt);
        for (i = 0; i < cfg->comp_cnt; i++) {
            nnet3->LayerInfo[i] =
                (QtkNnet3LayerInfo_t *)wtk_malloc(sizeof(QtkNnet3LayerInfo_t));
        }
        if(cfg->use_xvector)
	{
		nnet3->use_ivector = 1;
		nnet3->ivector_dim = cfg->xvector->len;
		nnet3->ivector = cfg->xvector->p;
		//wtk_debug("%d %d %p\n",nnet3->use_ivector,nnet3->ivector_dim,nnet3->ivector);
	}

//	if(nnet3->use_ivector)
	{
		nnet3->cur_kfeat = (wtk_kfeat_t*)wtk_malloc(sizeof(wtk_kfeat_t));
		nnet3->cur_kfeat->v=(float*)wtk_malloc(sizeof(float)*nnet3->input_col);
        }
        for (i = 0; i < 1000; ++i) {
            nnet3->m[i] = NULL;
        }

        qtk_nnet3_reset(nnet3);

	return nnet3;
}

void qtk_nnet3_gc(qtk_nnet3_t* nnet3)
{
	int i;
	for(i = 0;i<nnet3->gc_count;i++)
	{
		if(nnet3->gc[i])
		{
			qtk_blas_matrix_delete(nnet3->gc[i]);
			nnet3->gc[i] = NULL;
		}
	}
	nnet3->gc_count = 0;
}

void qtk_nnet3_delete(qtk_nnet3_t* nnet3)
{
    int i;
    wtk_free(nnet3->cur_kfeat->v);
    wtk_free(nnet3->cur_kfeat);

    for (i = 0; i < 1000; ++i) {
        if (nnet3->m[i]) {
            qtk_blas_matrix_delete(nnet3->m[i]);
            nnet3->m[i] = NULL;
        }
    }
    for (i = 0; i < nnet3->cfg->comp_cnt; i++) {
        wtk_free(nnet3->LayerInfo[i]);
    }
    wtk_free(nnet3->LayerInfo);
    qtk_nnet3_gc(nnet3);
    wtk_free(nnet3->gc);
    wtk_free(nnet3);
}

void qtk_nnet3_del(qtk_nnet3_t* nnet3)
{
    if (nnet3->m[1])
        qtk_blas_matrix_delete(nnet3->m[1]);
    if (nnet3->m[2])
        qtk_blas_matrix_delete(nnet3->m[2]);
}

void qtk_nnet3_reset_normal(qtk_nnet3_t* nnet3)
{
    if (nnet3->use_ivector != 1 && nnet3->m[1]) {
        qtk_blas_matrix_delete(nnet3->m[1]);
        nnet3->m[1] = NULL;
    }
    nnet3->program_counter = 1;
	nnet3->cur_feat = NULL;
	//nnet3->cur_kfeat = NULL;
//	if(nnet3->use_ivector)
	{
		memset(nnet3->cur_kfeat->v,0,sizeof(float)*nnet3->input_col);
	}
    //nnet3->frames_per_chunk = 21;
    nnet3->frames_per_chunk = nnet3->cfg->frame_per_chunk * nnet3->cfg->frame_subsample_factor;
    nnet3->begin_input_frame = nnet3->left;
    nnet3->end_input_frame = nnet3->frames_per_chunk+nnet3->right;
    nnet3->chunk_counter = 0;
    nnet3->row = 0;
    nnet3->irow = 0;
    nnet3->ac_index = 1;
    nnet3->hasac = 1;
    //    nnet3->m[index] =
    //    qtk_blas_matrix_new(-nnet3->begin_input_frame+nnet3->end_input_frame+1,nnet3->input_col);
    if (nnet3->use_ivector) {
        // if(nnet3->m[2])
        //{
        //   qtk_blas_matrix_delete(nnet3->m[2]);
        //   nnet3->m[2] = NULL;
        //}
        nnet3->m[1] = qtk_blas_matrix_new(-nnet3->begin_input_frame +
                                              nnet3->end_input_frame,
                                          nnet3->input_col);
        nnet3->m[2] = qtk_blas_matrix_new(
            (-nnet3->begin_input_frame + nnet3->end_input_frame + 1) / 3 + 1,
            nnet3->ivector_dim);
        }else
	{
            nnet3->m[1] = qtk_blas_matrix_new(-nnet3->begin_input_frame +
                                                  nnet3->end_input_frame + 1,
                                              nnet3->input_col);
        }
    qtk_nnet3_gc(nnet3);
}

void qtk_nnet3_reset_extra(qtk_nnet3_t* nnet3)
{
    if (nnet3->use_ivector != 1 && nnet3->m[1]) {
        qtk_blas_matrix_delete(nnet3->m[1]);
        nnet3->m[1] = NULL;
    }
    nnet3->program_counter = 1;
	nnet3->cur_feat = NULL;
    nnet3->chunk_counter = 0;
	nnet3->need_out = 0;
	{
		memset(nnet3->cur_kfeat->v,0,sizeof(float)*nnet3->input_col);
	}
    //nnet3->frames_per_chunk = 21;
    nnet3->frames_per_chunk = nnet3->cfg->frame_per_chunk * nnet3->cfg->frame_subsample_factor;
    nnet3->begin_input_frame = nnet3->left;
    nnet3->end_input_frame = nnet3->right + (nnet3->cfg->frame_per_chunk-1)*nnet3->cfg->frame_subsample_factor;

	nnet3->output_frames=0;
    nnet3->extra = 0;
	nnet3->row = 0;
        nnet3->m[1] = qtk_blas_matrix_new(-nnet3->begin_input_frame +
                                              nnet3->end_input_frame + 1,
                                          nnet3->input_col);
        qtk_nnet3_gc(nnet3);
}

void qtk_nnet3_savelayerid(qtk_nnet3_t *nnet3) {
    unsigned int i, idx = 0;
    static int index = 0;
    qtk_nnet3_compution_t *compution = nnet3->cfg->compution;
    for (i = 0; i < compution->cmd_cnt; i++) {
        if (compution->command_q[i]->type == kGotoLabel) {
            idx = compution->command_q[i]->arg1;
            break;
        }
    }
    for (i = idx; i < compution->cmd_cnt; i++) {
        if (compution->command_q[i]->type == kGotoLabel) {
            idx = compution->command_q[i]->arg1;
            break;
        } else if (index < nnet3->cfg->comp_cnt &&
                   compution->command_q[i]->type == kPropagate) {
            nnet3->LayerInfo[index]->LayerId =
                nnet3->cfg->comp_q[compution->command_q[i]->arg1]->type;
            nnet3->LayerInfo[index]->LayerPoint =
                nnet3->cfg->comp_q[compution->command_q[i]->arg1];
            index++;
        }
    }
}

void qtk_nnet3_reset(qtk_nnet3_t* nnet3)
{
	if(nnet3->cfg->extra_left > 0)
	{
		qtk_nnet3_reset_extra(nnet3);
	}else
	{
		qtk_nnet3_reset_normal(nnet3);
	}
        qtk_nnet3_savelayerid(nnet3);
}

void qtk_sub_matrix_cpy_multi_rows(qtk_nnet3_t* nnet3,qtk_blas_matrix_t  *dst,qtk_nnet3_submatrix_info_t *dst_info,int index)
{
	int i,j,row,len;
	int* mulindex;
	qtk_nnet3_submatrix_info_t *src_info;
	qtk_blas_matrix_t* src;
	qtk_nnet3_cfg_t* cfg = nnet3->cfg;
    qtk_nnet3_compution_t* compution = cfg->compution;
    if(nnet3->extra)
    {
        compution = cfg->compution2;
    }

	mulindex = compution->indexes_multi[index];
	len = mulindex[0];

	for(i = 0;i<dst_info->num_rows;i++)
	{
		if(i<len/2)
		{
			j = mulindex[1+i*2];
			if(j>= 0)
			{
				src_info = compution->submatrix_info_q[j];
                                src = nnet3->m[src_info->matrix_index];
                                row = mulindex[2+i*2];
				memcpy((dst->m+(dst_info->row_offset+i)*(dst->col)+(dst_info->col_offset)),(src->m+(src_info->row_offset+row)*src->col+src_info->col_offset),sizeof(float)*dst_info->num_cols);
			}else
			{
	            memset((dst->m+(dst_info->row_offset+i)*(dst->col)+(dst_info->col_offset)),0,sizeof(float)*dst_info->num_cols);
			}
		}else
		{
			memset((dst->m+(dst_info->row_offset+i)*(dst->col)+(dst_info->col_offset)),0,sizeof(float)*dst_info->num_cols);
		}
	}
}

void qtk_nnet3_set_const(qtk_blas_matrix_t* dst,qtk_nnet3_submatrix_info_t *dst_info,float alpha)
{
	int col,row,i,j,col_offset;//row_offset,col_offset;

	col = dst_info->num_cols;
	row = dst_info->num_rows;
	col_offset = dst_info->col_offset;
	//qtk_blas_matrix_print(dst);
	for(i = 0;i<row;i++)
	{
		for(j = 0;j<col;j++)
		{
			*(dst->m+i*col+col_offset+j) = alpha;
		}
	}
	//wtk_debug("setconst2:%f\n",alpha);
	//qtk_blas_matrix_print(dst);
}

void qtk_nnet3_execute_command(qtk_nnet3_t* nnet3, qtk_nnet3_command_t* command)
{
	qtk_nnet3_cfg_t* cfg = nnet3->cfg;
	qtk_nnet3_compution_t* compution = nnet3->cfg->compution;
	if(nnet3->extra)
	{
		compution = nnet3->cfg->compution2;
	}
	qtk_nnet3_precomputed_indexes_t *pre_compute;
	int m1,m2;
	int row,col;
	qtk_blas_matrix_t *tmp,*tmp2;
	qtk_nnet3_submatrix_info_t *sub_info,*sub_info2;
	nnet3_component_t* com;
	int *index;
        float alpha;

        //command = cfg->command_q[0];
    //wtk_debug("command type %d\n",command->type);
	m2 = command->arg1;
	switch(command->type)
	{
		case kAllocMatrix:
			m1 = compution->submatrix_info_q[m2]->matrix_index;
			row = compution->matrix_info_q[m1]->num_rows;
			col = compution->matrix_info_q[m1]->num_cols;
			//wtk_debug("alloc mat:%d %d %d\n",m1,row,col);
                        if (nnet3->m[m1] != NULL) {
                            // qtk_blas_matrix_delete(nnet3->m[m1]);
                            nnet3->gc[nnet3->gc_count] = nnet3->m[m1];
                            nnet3->gc_count++;
                            nnet3->m[m1] = NULL;
                        }
                        // nnet3->m[m1] = wtk_matrix_new2(row,col);
                        nnet3->m[m1] = qtk_blas_matrix_new(row, col);
                        qtk_blas_matrix_zero(nnet3->m[m1]);
                        break;
                case kDeallocMatrix:
			//m2 = command->arg1;
			m1 = compution->submatrix_info_q[m2]->matrix_index;
			if(m1 == 1 && nnet3->cfg->extra_left >0)
			{
				break;
			}
                        if (nnet3->m[m1]) {
                            qtk_blas_matrix_delete(nnet3->m[m1]);
                            nnet3->m[m1] = NULL;
                        }
                        break;
		case kSwapMatrix:
			//m2 = command->arg1;
			m1 = compution->submatrix_info_q[m2]->matrix_index;
			m2 = command->arg2;
			m2 = compution->submatrix_info_q[m2]->matrix_index;
			//wtk_debug("swap from %d to %d\n",m1,m2);
                        tmp = nnet3->m[m1];
                        nnet3->m[m1] = nnet3->m[m2];
                        nnet3->m[m2] = tmp;
                        break;
		case kSetConst://TODO STRID?
			m1 = compution->submatrix_info_q[m2]->matrix_index;
                        tmp = nnet3->m[m1];
                        qtk_nnet3_set_const(tmp,compution->submatrix_info_q[m2],command->alpha);
			break;
		case kPropagate:
			com = cfg->comp_q[m2];
			//wtk_debug("kPropagate %p %d\n",com,m2);
			m2 = command->arg3;
			sub_info = compution->submatrix_info_q[m2];
			//wtk_debug("sub_info->matrix_index=%d cfg->max_w=%f\n",sub_info->matrix_index, cfg->max_w);
                        tmp = nnet3->m[sub_info->matrix_index];

                        m2 = command->arg4;
			sub_info2 = compution->submatrix_info_q[m2];
			//wtk_debug("sub_info2->matrix_index=%d\n",sub_info2->matrix_index);
                        tmp2 = nnet3->m[sub_info2->matrix_index];
                        // wtk_debug("===========input=========
                        // %d\n",com->type); qtk_blas_matrix_print(tmp);
                        int row_cnt;
			switch(com->type)
			{
			//arg3/tmp src arg4/tmp2 dst
			case QTK_AffineGlobalComponent:
			   for(row_cnt=sub_info2->row_offset;row_cnt<sub_info2->num_rows;row_cnt++)
			   {
				 memset(tmp2->m+row_cnt*tmp2->col+sub_info2->col_offset,0,sizeof(float)*sub_info2->num_cols);
			   }
#ifdef USE_AVX
				if (cfg->use_custom_acc)
				{
					if (WTK_CUSTOM_AVX2==cfg->use_custom_acc)
						qtk_affine_global_propagate_avx2custom(com,tmp,tmp2,sub_info,sub_info2, cfg->porder);
					else
						qtk_affine_global_propagate_avxcustom(com,tmp,tmp2,sub_info,sub_info2, cfg->porder);
				}
				else
					qtk_affine_global_propagate_avx(com,tmp,tmp2,sub_info,sub_info2);
#elif USE_AFFINE_FAST
                           qtk_affine_global_propagate_fast(
                               com, tmp, tmp2, sub_info, sub_info2);
#else
				qtk_affine_global_propagate(com,tmp,tmp2,sub_info,sub_info2);
				//qtk_blas_matrix_print(tmp2);
#endif
				break;
			case QTK_NaturalGradientAffineMaskComponent:
#ifdef USE_AVX
				if (cfg->use_custom_acc)
					qtk_namask_component_propagate_avxcustom(com,tmp,tmp2,sub_info,sub_info2);
				else
					qtk_namask_component_propagate_avx(com,tmp,tmp2,sub_info,sub_info2);
#else
				if (cfg->use_custom_acc)
					qtk_namask_component_propagate_custom(com,tmp,tmp2,sub_info,sub_info2);
				else
					qtk_namask_component_propagate(com,tmp,tmp2,sub_info,sub_info2);
#endif
				break;
			case QTK_LinearMaskComponent:
				qtk_linearmask_component_propagate(com,tmp,tmp2,sub_info,sub_info2);
				break;
			case QTK_PermuteComponent:
				qtk_permute_component_propagate(com,tmp,tmp2,sub_info,sub_info2);
				break;
			case QTK_RectifiedLinearComponent:
				qtk_rectified_linear_propagate(com,tmp2,tmp,sub_info2,sub_info);
				break;
			case QTK_NormalizeComponent:
				qtk_normallize_propagate(com,tmp2,tmp,sub_info2,sub_info);
				break;
			case QTK_ActivateComponent:
				qtk_activate_propagate(com,tmp2,tmp,sub_info2,sub_info);
				break;
			case QTK_ElementwiseProductComponent:
				qtk_element_wise_product_propagate(com,tmp,tmp2,sub_info2,sub_info);//TODO
				break;
            case QTK_NaturalGradientPerElementScaleComponent:
				qtk_natural_gradient_per_element_scale_propagate(com,tmp,tmp2,sub_info2,sub_info);
               	break;
            case QTK_DropoutComponent:
				qtk_dropout_propagate(com,tmp,tmp2,sub_info,sub_info2);
               	break;
            case QTK_BackpropTruncationComponent:
				qtk_backprop_truncation_propagate(com,tmp,tmp2,sub_info,sub_info2);
               	break;
            case QTK_NoopComponent:
				qtk_noop_propagate(com,tmp,tmp2,sub_info2,sub_info);
               	break;
            case QTK_BatchNormComponent:
				qtk_batchnorm_propagate(com,tmp,tmp2,sub_info2,sub_info);
               	break;
			case QTK_LstmNonlinearityComponent:
				qtk_lstm_nolinearity_propagate(com,tmp2,tmp,sub_info2,sub_info);
				break;
			case QTK_TimeHeightConvolutionComponent:
				pre_compute = compution->pre_computed[command->arg2-1];
				qtk_timeheight_convolution_propagate(com,tmp2,tmp,sub_info2,sub_info,pre_compute);
				break;
			case QTK_ScaleAndOffsetComponent:
				qtk_scale_offset_propagate(com,tmp2,tmp,sub_info2,sub_info);
				break;
			case QTK_MaxPoolingComponent:
                qtk_max_pooling_propagate(com,tmp2,tmp,sub_info2,sub_info);
				break;
			case QTK_PnormComponent:
				qtk_pnorm_propagate(com,tmp2,tmp,sub_info2,sub_info);
				break;
			case QTK_CompactFsmnComponent:
				pre_compute = compution->pre_computed[command->arg2-1];
				qtk_fsmn_component_propagate(com,tmp2,tmp,sub_info2,sub_info,pre_compute);
                                break;
                        case QTK_BlockWeightComponent:
                            qtk_blockweight_component_propagate(
                                com, tmp2, tmp, sub_info2, sub_info);
                            break;
                        case QTK_GeneralDropoutComponent:
                            pre_compute =
                                compution->pre_computed[command->arg2 - 1];
                            qtk_general_dropout_component_propagate(
                                com, tmp2, tmp, sub_info2, sub_info,
                                pre_compute);
                            break;
			default:
				break;
			}
                        // wtk_debug("===========output=========\n");
                        // qtk_blas_matrix_print(tmp2);
                        // wtk_debug("end\n");
                        break;
		case kBackprop:
		case kBackpropNoModelUpdate:
			break;
		case kMatrixCopy:
			//wtk_debug("copy matrix %p %d\n",cfg->compution->submatrix_info_q[m2],m2);
			sub_info = compution->submatrix_info_q[m2];
                        tmp = nnet3->m[sub_info->matrix_index];
                        m2 = command->arg2;
			sub_info2 = compution->submatrix_info_q[m2];
                        tmp2 = nnet3->m[sub_info2->matrix_index];
                        //			wtk_debug("copy mat from %d to
                        //%d\n",sub_info2->matrix_index,sub_info->matrix_index);
                        //			qtk_blas_matrix_print(tmp2);
                        wtk_sub_matrix_cpy2(tmp,tmp2,sub_info,sub_info2);
			if(command->alpha != 1.0)
			{
				qtk_blas_matrix_scale(tmp,sub_info,command->alpha);
			}
			break;
		case kMatrixAdd:
			sub_info = compution->submatrix_info_q[m2];
                        tmp = nnet3->m[sub_info->matrix_index];
                        m2 = command->arg2;
			sub_info2 = compution->submatrix_info_q[m2];
                        tmp2 = nnet3->m[sub_info2->matrix_index];
                        //qtk_blas_matrix_print(tmp);
			qtk_nnet3_component_mat_add(tmp2,tmp,sub_info2,sub_info,command->alpha);
			//qtk_blas_matrix_print(tmp2);
			break;

		case kAddRows://TODO
                    /*alpha = command->alpha;
                    sub_info = &(cfg->submatrix_info_q[m2]);
                    tmp = nnet3->m[sub_info->matrix_index];
                    qtk_nnet3_get_sub_matrix(cfg->sub1,tmp,sub_info->row_offset,sub_info->num_rows,sub_info->col_offset,sub_info->num_cols);

                    m2 = command->arg2;
                    sub_info = &(cfg->submatrix_info_q[m2]);
                    tmp = nnet3->m[sub_info->matrix_index];
                    qtk_nnet3_get_sub_matrix(cfg->sub2,tmp,sub_info->row_offset,sub_info->num_rows,sub_info->col_offset,sub_info->num_cols);

                    wtk_sub_matrix_add_rows(cfg->sub1,cfg->sub2,alpha);*/
                    sub_info = compution->submatrix_info_q[m2];
                    tmp = nnet3->m[sub_info->matrix_index];

                    m2 = command->arg2;
                    sub_info2 = compution->submatrix_info_q[m2];
                    tmp2 = nnet3->m[sub_info2->matrix_index];
                    index = compution->indexes[command->arg3];
                    wtk_sub_matrix_add_rows(tmp, tmp2, sub_info, sub_info2,
                                            index, command->alpha);
                    break;
		case kCopyRows:
			sub_info = compution->submatrix_info_q[m2];
                        tmp = nnet3->m[sub_info->matrix_index];

                        m2 = command->arg2;
			sub_info2 = compution->submatrix_info_q[m2];
                        tmp2 = nnet3->m[sub_info2->matrix_index];
                        index = compution->indexes[command->arg3];
			//wtk_debug("copy rows from %d to %d %d\n",sub_info2->matrix_index,sub_info->matrix_index,sub_info->num_rows);
                        alpha = command->alpha;
                        if (alpha != 1.0) {
                            if (alpha == 0.0)
                                break;
                            qtk_blas_matrix_scale(tmp, sub_info, 1.0 / alpha);
                            wtk_sub_matrix_cpy_rows(tmp, tmp2, sub_info,
                                                    sub_info2, index);
                            qtk_blas_matrix_scale(tmp, sub_info,
                                                  command->alpha);
                        } else {
                            wtk_sub_matrix_cpy_rows(tmp, tmp2, sub_info,
                                                    sub_info2, index);
                        }
                        //qtk_blas_matrix_print(tmp);
			//qtk_blas_matrix_print(tmp2);
			//exit(0);
			break;
		case kCopyRowsMulti:
			//wtk_debug("copt rows multi\n");
			sub_info = compution->submatrix_info_q[m2];
			//wtk_debug("%d\n",sub_info->matrix_index);
			//qtk_nnet3_matrix_info_print(sub_info);
                        tmp = nnet3->m[sub_info->matrix_index];
                        alpha = command->alpha;
                        // wtk_debug("alpha = %f\n", alpha);
                        if (alpha != 1.0) {
                            if (alpha == 0.0)
                                break;
                            qtk_blas_matrix_scale(tmp, sub_info, 1.0 / alpha);
                            qtk_sub_matrix_cpy_multi_rows(nnet3, tmp, sub_info,
                                                          command->arg2);
                            qtk_blas_matrix_scale(tmp, sub_info,
                                                  command->alpha);
                        } else {
                            qtk_sub_matrix_cpy_multi_rows(nnet3, tmp, sub_info,
                                                          command->arg2);
                        }
                        break;
		case kGotoLabel:
			nnet3->program_counter = m2;
			break;
		case kCompressMatrix:
		case kDecompressMatrix:
		case kProvideOutput:
		case kNoOperationPermanent:
			break;
		default:
			break;

	}
}

#ifdef USE_NEON64
#define qtk_affine_global_propagate_f qtk_affine_global_propagate_neon64
#elif USE_NEON32
#define qtk_affine_global_propagate_f qtk_affine_global_propagate_neon32
#else
#define qtk_affine_global_propagate_f qtk_affine_global_propagate_fix
//#define qtk_affine_global_propagate_fi qtk_affine_global_propagate_fixi
//#define qtk_affine_global_propagate_fiop1 qtk_affine_global_propagate_fixiop1
#endif

void qtk_affine_global_propagate_f2(nnet3_component_t* com1, wtk_mats_t *input,
		wtk_mats_t *out_put,qtk_blas_matrix_t *out_putf, qtk_nnet3_submatrix_info_t *input_info,
		qtk_nnet3_submatrix_info_t *out_put_info, int fixed_nbytes, int use_custom_acc, int porder)
{
	switch(fixed_nbytes)
	{
	case sizeof(short):
		qtk_affine_global_propagate_f(com1, input, out_put, out_putf,input_info,out_put_info);
		break;
	case sizeof(int):
		if (use_custom_acc)
		{
#ifdef __ANDROID__
#ifdef USE_NEON
			if (use_custom_acc == WTK_CUSTOM_NEON)
			{
				qtk_affine_global_propagate_fixiop1_neon(com1, input, out_put, out_putf,input_info,out_put_info, porder);
			}
			else
#endif
#endif
				qtk_affine_global_propagate_fixiop1(com1, input, out_put, out_putf,input_info,out_put_info, porder);
		}
		else
			qtk_affine_global_propagate_fixi(com1, input, out_put, out_putf,input_info,out_put_info);
		break;
	}
}

void qtk_nnet3_execute_command_fixed(qtk_nnet3_t* nnet3, qtk_nnet3_command_t* command)
{
        qtk_nnet3_cfg_t* cfg = nnet3->cfg;
        qtk_nnet3_compution_t* compution = nnet3->cfg->compution;
        qtk_nnet3_precomputed_indexes_t *pre_compute;
        int m1,m2;
        int row,col;
        qtk_blas_matrix_t *tmp,*tmp2;
        wtk_mats_t *tmp_s,*tmp2_s;
        qtk_nnet3_submatrix_info_t *sub_info,*sub_info2;
        nnet3_component_t* com;
        int *index;
        float alpha;
        //command = cfg->command_q[0];
        //wtk_debug("command type %d\n",command->type);
        m2 = command->arg1;
        switch(command->type)
        {
        case kAllocMatrix:
            m1 = compution->submatrix_info_q[m2]->matrix_index;
            row = compution->matrix_info_q[m1]->num_rows;
            col = compution->matrix_info_q[m1]->num_cols;
            //      wtk_debug("alloc mat:%d %d %d\n",m1,row,col);
            if (nnet3->m[m1] != NULL) {
                // qtk_blas_matrix_delete(nnet3->m[m1]);
                nnet3->gc[nnet3->gc_count] = nnet3->m[m1];
                nnet3->gc_count++;
                nnet3->m[m1] = NULL;
            }
            if (cfg->ms[m1] != NULL) {
                wtk_mats_delete(cfg->ms[m1]);
            }
            // nnet3->m[m1] = wtk_matrix_new2(row,col);
            nnet3->m[m1] = qtk_blas_matrix_new(row, col);
            qtk_blas_matrix_zero(nnet3->m[m1]);
            cfg->ms[m1] =
                (wtk_mats_t *)wtk_matfix_new(row, col, cfg->fixed_nbytes);

            memset(cfg->ms[m1]->p, 0, cfg->fixed_nbytes * row * col);
            break;
        case kDeallocMatrix:
            // m2 = command->arg1;
            m1 = compution->submatrix_info_q[m2]->matrix_index;
            //      wtk_debug("free %d\n",m1);
            if (nnet3->m[m1]) {
                qtk_blas_matrix_delete(nnet3->m[m1]);
                nnet3->m[m1] = NULL;
            }
            if (cfg->ms[m1]) {
                wtk_matfix_delete(cfg->ms[m1], cfg->fixed_nbytes);
                cfg->ms[m1] = NULL;
            }
            break;
        case kSwapMatrix:
            // m2 = command->arg1;
            m1 = compution->submatrix_info_q[m2]->matrix_index;
            m2 = command->arg2;
            m2 = compution->submatrix_info_q[m2]->matrix_index;
            tmp = nnet3->m[m1];
            nnet3->m[m1] = nnet3->m[m2];
            nnet3->m[m2] = tmp;
            break;
        case kSetConst: // TODO STRID?
            m1 = compution->submatrix_info_q[m2]->matrix_index;
            tmp = nnet3->m[m1];
            qtk_nnet3_set_const(tmp, compution->submatrix_info_q[m2],
                                command->alpha);
            break;
        case kPropagate:
            com = cfg->comp_q[m2];
            // wtk_debug("kPropagate %d\n",m2);
            m2 = command->arg3;
            sub_info = compution->submatrix_info_q[m2];
            tmp = nnet3->m[sub_info->matrix_index];
            // wtk_debug("sub_info->matrix_index=%d
            // cfg->max_w=%f\n",sub_info->matrix_index, cfg->max_w);
            tmp_s = cfg->ms[sub_info->matrix_index];
            // qtk_nnet3_update_fix_f2s(tmp,tmp_s,cfg->max_w);
            qtk_nnet3_update_fix_f2i(tmp, tmp_s, cfg->max_w, cfg->fixed_nbytes);
            // print_short(tmp_s->p,20);
            m2 = command->arg4;
            sub_info2 = compution->submatrix_info_q[m2];
            // wtk_debug("sub_info2->matrix_index=%d\n",sub_info2->matrix_index);
            tmp2 = nnet3->m[sub_info2->matrix_index];
            tmp2_s = cfg->ms[sub_info2->matrix_index];
            //      qtk_nnet3_update_fix_f2s(tmp2,tmp2_s,cfg->max_w);
            //   print_float(tmp2->m,30);
            // print_short(tmp2_s->p,30);

            //      wtk_debug("row=%d,col=%d,row2=%d,col2=%d\n",sub_info2->num_rows,sub_info2->num_cols,tmp2_s->row,tmp2_s->col);
            // wtk_debug("===========input====type=%d=====\n",com->type);
            // qtk_blas_matrix_print(tmp);
            switch (com->type) {
            // arg3/tmp src arg4/tmp2 dst
            case QTK_AffineGlobalComponent:
                qtk_affine_global_propagate_f2(
                    com, tmp_s, tmp2_s, tmp2, sub_info, sub_info2,
                    cfg->fixed_nbytes, cfg->use_custom_acc, cfg->porder);
                // qtk_blas_matrix_print(tmp2);
                break;
            case QTK_RectifiedLinearComponent:
                qtk_rectified_linear_propagate(com, tmp2, tmp, sub_info2,
                                               sub_info);
                break;
            case QTK_NormalizeComponent:
                qtk_normallize_propagate(com, tmp2, tmp, sub_info2, sub_info);
                break;
            case QTK_ActivateComponent:
                qtk_activate_propagate(com, tmp2, tmp, sub_info2, sub_info);
                break;
            case QTK_ElementwiseProductComponent:
                qtk_element_wise_product_propagate(com, tmp, tmp2, sub_info2,
                                                   sub_info); // TODO
                break;
            case QTK_NaturalGradientPerElementScaleComponent:
                qtk_natural_gradient_per_element_scale_propagate(
                    com, tmp, tmp2, sub_info2, sub_info);
                break;
            case QTK_DropoutComponent:
                qtk_dropout_propagate(com, tmp, tmp2, sub_info, sub_info2);
                break;
            case QTK_BackpropTruncationComponent:
                qtk_backprop_truncation_propagate(com, tmp, tmp2, sub_info,
                                                  sub_info2);
                break;
            case QTK_NoopComponent:
                qtk_noop_propagate(com, tmp, tmp2, sub_info, sub_info2);
                break;
            case QTK_BatchNormComponent:
                qtk_batchnorm_propagate_fix(com, tmp_s, tmp2, sub_info2,
                                            sub_info);
                break;
            case QTK_LstmNonlinearityComponent:
                qtk_lstm_nolinearity_propagate(com, tmp2, tmp, sub_info2,
                                               sub_info);
                break;
            case QTK_TimeHeightConvolutionComponent:
                pre_compute = compution->pre_computed[command->arg2 - 1];
                qtk_timeheight_convolution_propagate_fix(
                    com, tmp2, tmp_s, sub_info2, sub_info, pre_compute);
                break;
            case QTK_ScaleAndOffsetComponent:
                qtk_scale_offset_propagate_fix(com, tmp2, tmp_s, sub_info2,
                                               sub_info);
                wtk_debug("scale_offset\n");
                break;
            case QTK_MaxPoolingComponent:
                // TODO
                break;
            case QTK_PnormComponent:
                qtk_pnorm_propagate(com, tmp2, tmp, sub_info2, sub_info);
                break;
            case QTK_CompactFsmnComponent:
                pre_compute = compution->pre_computed[command->arg2 - 1];
                qtk_fsmn_component_propagate(com, tmp2, tmp, sub_info2,
                                             sub_info, pre_compute);
                break;
            case QTK_BlockWeightComponent:
                qtk_blockweight_component_propagate(com, tmp2, tmp, sub_info2,
                                                    sub_info);
                break;

            default:
                break;
            }
            //                      if(tmp2->col==3045)
            //                      {
            //                              qtk_blas_matrix_print(tmp2);
            //                      }
            //                      wtk_debug("end\n");
            break;
        case kBackprop:
        case kBackpropNoModelUpdate:
            break;
        case kMatrixCopy:
            // wtk_debug("copy matrix %p
            // %d\n",cfg->compution->submatrix_info_q[m2],m2);
            sub_info = compution->submatrix_info_q[m2];
            tmp = nnet3->m[sub_info->matrix_index];
            m2 = command->arg2;
            sub_info2 = compution->submatrix_info_q[m2];
            tmp2 = nnet3->m[sub_info2->matrix_index];
            //			wtk_debug("copy mat from %d to
            //%d\n",sub_info2->matrix_index,sub_info->matrix_index);
            //			qtk_blas_matrix_print(tmp2);
            wtk_sub_matrix_cpy2(tmp, tmp2, sub_info, sub_info2);
            if (command->alpha != 1.0) {
                qtk_blas_matrix_scale(tmp, sub_info, command->alpha);
            }
            break;
        case kMatrixAdd:
            sub_info = compution->submatrix_info_q[m2];
            tmp = nnet3->m[sub_info->matrix_index];
            m2 = command->arg2;
            sub_info2 = compution->submatrix_info_q[m2];
            tmp2 = nnet3->m[sub_info2->matrix_index];
            // qtk_blas_matrix_print(tmp);
            qtk_nnet3_component_mat_add(tmp2, tmp, sub_info2, sub_info,
                                        command->alpha);
            // qtk_blas_matrix_print(tmp2);
            break;

        case kAddRows: // TODO
            break;
        case kCopyRows:
            sub_info = compution->submatrix_info_q[m2];
            tmp = nnet3->m[sub_info->matrix_index];

            m2 = command->arg2;
            sub_info2 = compution->submatrix_info_q[m2];
            tmp2 = nnet3->m[sub_info2->matrix_index];
            index = compution->indexes[command->arg3];
            // wtk_debug("copy rows from %d to %d
            // %d\n",sub_info2->matrix_index,sub_info->matrix_index,sub_info->num_rows);
            alpha = command->alpha;
            if (alpha != 1.0) {
                if (alpha == 0.0)
                    break;
                qtk_blas_matrix_scale(tmp, sub_info, 1.0 / alpha);
                wtk_sub_matrix_cpy_rows(tmp, tmp2, sub_info, sub_info2, index);
                qtk_blas_matrix_scale(tmp, sub_info, command->alpha);
            } else {
                wtk_sub_matrix_cpy_rows(tmp, tmp2, sub_info, sub_info2, index);
            }
            break;
        case kCopyRowsMulti:
            // wtk_debug("copt rows multi\n");
            sub_info = compution->submatrix_info_q[m2];
            // wtk_debug("%d\n",sub_info->matrix_index);
            // qtk_nnet3_matrix_info_print(sub_info);
            tmp = nnet3->m[sub_info->matrix_index];
            alpha = command->alpha;
            // wtk_debug("alpha = %f\n", alpha);
            if (alpha != 1.0) {
                if (alpha == 0.0)
                    break;
                qtk_blas_matrix_scale(tmp, sub_info, 1.0 / alpha);
                qtk_sub_matrix_cpy_multi_rows(nnet3, tmp, sub_info,
                                              command->arg2);
                qtk_blas_matrix_scale(tmp, sub_info, command->alpha);
            } else {
                qtk_sub_matrix_cpy_multi_rows(nnet3, tmp, sub_info,
                                              command->arg2);
            }
            break;
        case kGotoLabel:
            nnet3->program_counter = m2;
            break;
        case kCompressMatrix:
        case kDecompressMatrix:
        case kProvideOutput:
        case kNoOperationPermanent:
            break;
        default:
            break;
        }
}



void qtk_nnet3_set_notify(qtk_nnet3_t* nnet3,qtk_nnet3_feature_notify_f notify,void* notify_ths)
{
	nnet3->notify_ths = notify_ths;
	nnet3->notify = notify;
}

void qtk_nnet3_run3(qtk_nnet3_t* nnet3,int end, int plus)
{
    int cmd_cnt,out_index,i;
	qtk_blas_matrix_t *out;
	qtk_nnet3_compution_t *compution = nnet3->cfg->compution;

	if(nnet3->extra)
	{
		compution = nnet3->cfg->compution2;
	}

    qtk_nnet3_command_t* command = NULL;
    // wtk_debug("accept input\n");
    // qtk_blas_matrix_print(nnet3->m[1]);
    for(cmd_cnt = nnet3->program_counter;cmd_cnt<compution->cmd_cnt;nnet3->program_counter++)
    {
        command = compution->command_q[nnet3->program_counter];
        //if(command->type =  = kAcceptInput||command->type =  = kProvideOutput)
        if(command->type == kProvideOutput)
        {
            break;
        }
        if(command->type == kAcceptInput)
        {
        	nnet3->program_counter++;
        	command = compution->command_q[nnet3->program_counter];
        }

        if (nnet3->gc_count > 2500) {
            qtk_nnet3_gc(nnet3);
        }

        if(nnet3->cfg->use_fix_res)
        {
        	qtk_nnet3_execute_command_fixed(nnet3,command);
        }else
        {
        	qtk_nnet3_execute_command(nnet3,command);
        }
        cmd_cnt = nnet3->program_counter;
    }
    nnet3->chunk_counter++;
    nnet3->begin_input_frame = nnet3->frames_per_chunk*nnet3->chunk_counter+nnet3->right;
    nnet3->end_input_frame = nnet3->end_input_frame+nnet3->frames_per_chunk;
    nnet3->program_counter = 0;

    out_index = compution->submatrix_info_q[command->arg1]->matrix_index;
    out = nnet3->m[out_index];
    if(nnet3->extra)
    {
		//wtk_debug("===========%d\n",nnet3->frames_per_chunk);
                memcpy(nnet3->m[1]->m,
                       nnet3->m[1]->m +
                           nnet3->input_col * nnet3->frames_per_chunk,
                       sizeof(float) * nnet3->input_col *
                           (nnet3->m[1]->row - nnet3->frames_per_chunk));
    }else
    {
        qtk_blas_matrix_t *tmp = nnet3->m[1];
        nnet3->m[1] = qtk_blas_matrix_new(tmp->row + nnet3->cfg->extra_left,
                                          nnet3->input_col);
        for(i=0;i<nnet3->cfg->frame_per_chunk;i++)
    	{
            memcpy(nnet3->m[1]->m + nnet3->input_col * i, tmp->m,
                   nnet3->input_col * sizeof(float));
        }
        memcpy(nnet3->m[1]->m + nnet3->input_col * i, tmp->m,
               tmp->col * tmp->row * sizeof(float));
        qtk_blas_matrix_delete(tmp);
    }
    nnet3->row = nnet3->m[1]->row - nnet3->frames_per_chunk;
    nnet3->extra = 1;
	//qtk_blas_matrix_print(out);
	nnet3->output_frames+=nnet3->cfg->frame_per_chunk;
	if(nnet3->need_out > 0 && nnet3->output_frames > nnet3->need_out)
	{
		out->row = nnet3->need_out%10;
		//wtk_debug("???? %d\n",out->row);
	}
        if (nnet3->notify) {
            nnet3->notify(nnet3->notify_ths, out, end, plus);
        }
}

void qtk_nnet3_run2(qtk_nnet3_t* nnet3,int end, int plus)
{
    int cmd_cnt,out_index;
	qtk_blas_matrix_t *out;
	qtk_nnet3_compution_t *compution = nnet3->cfg->compution;
    qtk_nnet3_command_t* command = NULL;
    for(cmd_cnt = nnet3->program_counter;cmd_cnt<compution->cmd_cnt;nnet3->program_counter++)
    {
        command = compution->command_q[nnet3->program_counter];
        //if(command->type =  = kAcceptInput||command->type =  = kProvideOutput)
        if(command->type == kProvideOutput)
        {
            break;
        }
        if(command->type == kAcceptInput)
        {
        	nnet3->program_counter++;
        	command = compution->command_q[nnet3->program_counter];
        }

        if (nnet3->gc_count > 2500) {
            qtk_nnet3_gc(nnet3);
        }

        if(nnet3->cfg->use_fix_res)
        {
        	qtk_nnet3_execute_command_fixed(nnet3,command);
        }else
        {
        	qtk_nnet3_execute_command(nnet3,command);
        }
        cmd_cnt = nnet3->program_counter;
    }
    nnet3->chunk_counter++;
    nnet3->begin_input_frame = nnet3->frames_per_chunk*nnet3->chunk_counter+nnet3->right;
    nnet3->end_input_frame = nnet3->begin_input_frame+nnet3->frames_per_chunk;
    nnet3->program_counter++;

    out_index = compution->submatrix_info_q[command->arg1]->matrix_index;
    out = nnet3->m[out_index];
    nnet3->row = 0;
    nnet3->irow = 0;
	//qtk_blas_matrix_print(out);
	//wtk_debug("cnt=%d out_index=%d command->arg1=%d\n", cnt,out_index,command->arg1);
    if (nnet3->notify) {
        nnet3->notify(nnet3->notify_ths, out, end, plus);
    }

    nnet3->hasac = 0;

    if(nnet3->use_ivector)
    {
       if(nnet3->ac_index != nnet3->input_num - 1)
       {
          nnet3->ac_index++;
          nnet3->ac_index++;
       }
   }else
   {
   	   if(nnet3->ac_index != nnet3->input_num)
  	   {
 		  nnet3->ac_index++;
 	   }
   }
   //wtk_debug("%d\n",nnet3->ac_index);
}
void qtk_nnet3_run(qtk_nnet3_t* nnet3,wtk_feat_t *feature,int end)
{
    //qtk_nnet3_cfg_t* cfg = nnet3->cfg;
    //qtk_nnet3_compution_t *compution = cfg->compution;
    qtk_blas_matrix_t *mat;//*out,*mat;
    mat = NULL;
    //int out_index;
    int i;
    int xxindex = 1;

    if(feature)
    {
    	//wtk_feat_print(feature);
    	if(!nnet3->delta && nnet3->cur_feat)
    	{
			if(nnet3->cur_feat->used>0)
			{
    			wtk_feat_push_back(nnet3->cur_feat);
			}else
			{
                wtk_feat_send(nnet3->cur_feat);
			}
    	}
        nnet3->cur_feat = feature; 

    	xxindex = nnet3->input_index[nnet3->ac_index];
    	if(nnet3->ac_index != 0 && nnet3->hasac != 1)
    	{
			xxindex = nnet3->input_index[nnet3->ac_index];
                        if (nnet3->m[xxindex] != NULL) {
                            qtk_blas_matrix_delete(nnet3->m[xxindex]);
                            nnet3->m[xxindex] = NULL;
                        }
                        //wtk_debug("new\n");
                        nnet3->m[xxindex] = qtk_blas_matrix_new(
                            nnet3->frames_per_chunk, nnet3->input_col);
                        nnet3->hasac = 1;
    	}

        mat = nnet3->m[xxindex];
        if(feature->index == 1 && nnet3->begin_input_frame<0)
    	{
    		for(i = nnet3->begin_input_frame; i <= 0; i++)
    		{
        		memcpy(mat->m + nnet3->row * mat->col, feature->v+1, sizeof(float)*nnet3->input_col);
        		nnet3->row++;
    		}
    	}else
    	{
    		//TODO
    		memcpy(mat->m+nnet3->row*mat->col,feature->v+1,sizeof(float)*nnet3->input_col);
        	nnet3->row++;
    	}
		//wtk_debug("%d %d %d \n",feature->index,nnet3->end_input_frame,nnet3->row-1);
    	if(feature->index<nnet3->end_input_frame)
    	{
    		return;
    	}
		qtk_nnet3_run2(nnet3,end, 0);
    }else
    {
		if(end)
		{
			if(!nnet3->cur_feat || nnet3->begin_input_frame >=  nnet3->cur_feat->index)
			{
				if(nnet3->cur_feat)
				{
                	int j;
                	for(j = 0; j < nnet3->cfg->frame_plus; j++)
                	{   
                    	xxindex = nnet3->input_index[nnet3->ac_index];
                        if (!nnet3->m[xxindex]) {
                            nnet3->m[xxindex] = qtk_blas_matrix_new(
                                nnet3->frames_per_chunk, nnet3->input_col);
                        }
                        mat = nnet3->m[xxindex];
                        for(i = nnet3->row;i<nnet3->frames_per_chunk;i++)
	                    {
                      		//wtk_debug("======== %d %d %d\n",i,nnet3->begin_input_frame,nnet3->cur_feat->index);
                     		memcpy(mat->m+nnet3->row*mat->col,nnet3->cur_feat->v+1,sizeof(float)*nnet3->input_col);
                     		nnet3->row++;
                    	}
                    	qtk_nnet3_run2(nnet3,0, 1);
                	}
                	if(!nnet3->delta && nnet3->cur_feat)
                	{
                		if(nnet3->cur_feat->used>0)
                			wtk_feat_push_back(nnet3->cur_feat);
                	}
				}
				if (nnet3->notify)
					nnet3->notify(nnet3->notify_ths,NULL,1,0);
				return;
			}else
			{
                            /*	int j;
                                    for(j = 0; j < nnet3->cfg->frame_plus; j++)
                                    {
                                            xxindex =
                        nnet3->input_index[nnet3->ac_index];
                                            if(!nnet3->m[xxindex])
                                            {
                                    nnet3->m[xxindex] =
                        qtk_blas_matrix_new(nnet3->frames_per_chunk,nnet3->input_col);
                                            }
                                    mat = nnet3->m[xxindex];
                            for(i = nnet3->row;i<nnet3->frames_per_chunk;i++)
                        {
                              //wtk_debug("%d %d
                        %d\n",i,nnet3->begin_input_frame,nnet3->cur_feat->index);
                              memcpy(mat->m+nnet3->row*mat->col,nnet3->cur_feat->v+1,sizeof(float)*nnet3->input_col);
                          nnet3->row++;
                            }
                            qtk_nnet3_run2(nnet3,0);
                                    }*/

                            xxindex = nnet3->input_index[nnet3->ac_index];
                            // nnet3->m[xxindex] =
                            // qtk_blas_matrix_new(nnet3->frames_per_chunk,nnet3->input_col);
                            mat = nnet3->m[xxindex];
                            for (i = nnet3->row; i < nnet3->frames_per_chunk;
                                 i++) {
                                // wtk_debug("%d %d
                                // %d\n",i,nnet3->begin_input_frame,nnet3->cur_feat->index);
                                memcpy(mat->m + nnet3->row * mat->col,
                                       nnet3->cur_feat->v + 1,
                                       sizeof(float) * nnet3->input_col);
                                nnet3->row++;
                }
                qtk_nnet3_run2(nnet3,0, 0);

				int j;
				for(j = 0; j < nnet3->cfg->frame_plus; j++)
				{
					xxindex = nnet3->input_index[nnet3->ac_index];
                                        if (!nnet3->m[xxindex]) {
                                            nnet3->m[xxindex] =
                                                qtk_blas_matrix_new(
                                                    nnet3->frames_per_chunk,
                                                    nnet3->input_col);
                                        }
                                        mat = nnet3->m[xxindex];
                                        for(i = nnet3->row;i<nnet3->frames_per_chunk;i++)
					{
						//wtk_debug("======== %d %d %d\n",i,nnet3->begin_input_frame,nnet3->cur_feat->index);
						memcpy(mat->m+nnet3->row*mat->col,nnet3->cur_feat->v+1,sizeof(float)*nnet3->input_col);
						nnet3->row++;
					}
					qtk_nnet3_run2(nnet3,0, 1);
				}

				if (nnet3->notify)
					nnet3->notify(nnet3->notify_ths,NULL,1, 0);
			   if(!nnet3->delta && nnet3->cur_feat)
			   {
					   if(nnet3->cur_feat->used>0)
					   {
							   wtk_feat_push_back(nnet3->cur_feat);
					   }else
					   {
							   wtk_feat_send(nnet3->cur_feat);
					   }
				 }
			}
		}
    }
}

void qtk_nnet3_run_kfeat_extra(qtk_nnet3_t* nnet3,wtk_kfeat_t *feature,int end)
{
    int dim = nnet3->input_col;
    int i;
    qtk_blas_matrix_t *mat = nnet3->m[1]; //*out,*mat;

    if (feature) {
        memcpy(nnet3->cur_kfeat->v,feature->v,sizeof(float)*dim);
		nnet3->cur_kfeat->index = feature->index;
		if(feature->index == 0 && nnet3->begin_input_frame < 0)
		{
			for(i = nnet3->begin_input_frame; i <= 0; i++)
			{
				memcpy(mat->m + nnet3->row * mat->col, feature->v, sizeof(float)*dim);
        		nnet3->row++;
			}
		}else
		{
			memcpy(mat->m+nnet3->row*mat->col,feature->v,sizeof(float)*dim);
    		nnet3->row++;
		}

    	if(feature->index<nnet3->end_input_frame)
    	{
    		goto end;
    	}
		qtk_nnet3_run3(nnet3,0, 0);
	}
end:
	if(end)
	{
		if(nnet3->cur_kfeat->v)
		{
        	int j;
			nnet3->need_out = (int)(nnet3->cur_kfeat->index*1.0/nnet3->cfg->frame_subsample_factor + 1);
			int need_out = nnet3->need_out -nnet3->output_frames;
        	for(j = 0; j < (need_out/nnet3->cfg->frame_per_chunk+1); j++)
        	{
                    for (i = nnet3->row; i < nnet3->m[1]->row; i++) {
                        memcpy(mat->m + nnet3->row * mat->col,
                               nnet3->cur_kfeat->v, sizeof(float) * dim);
                        nnet3->row++;
                    }
                                qtk_nnet3_run3(nnet3,0,0);
        	}
		}
		if (nnet3->notify)
		{
			nnet3->notify(nnet3->notify_ths,NULL,1,0);
		}
	}
}

void qtk_nnet3_run_kfeat_normal(qtk_nnet3_t* nnet3,wtk_kfeat_t *feature,int end)
{
    //qtk_nnet3_cfg_t* cfg = nnet3->cfg;
    //qtk_nnet3_compution_t *compution = cfg->compution;
    qtk_blas_matrix_t *mat=0;//*out,*mat;
    qtk_blas_matrix_t *imat=0;//*out,*mat;
    mat = NULL;
    //int out_index;
    int i,ivec=0;
	int xxindex=0,iindex=0;
	if(nnet3->ivector)
	{
		ivec=1;
    	xxindex = 1;
		iindex = 2;
	}else
	{
		xxindex=1;
	}
    int dim = nnet3->input_col;

    if(feature)
    {
//    	if(!nnet3->delta && nnet3->cur_feat)
//    	{
//			if(nnet3->cur_feat->used>0)
//			{
//    			wtk_feat_push_back(nnet3->cur_feat);
//			}else
//			{
//                wtk_feat_send(nnet3->cur_feat);
//			}
//    	}
        //nnet3->cur_kfeat = feature;
        memcpy(nnet3->cur_kfeat->v,feature->v,sizeof(float)*dim);
        nnet3->cur_kfeat->index = feature->index;
		if(ivec)
		{
    		xxindex = nnet3->input_index[nnet3->ac_index];
    		iindex = nnet3->input_index[nnet3->ac_index+1];
		}else
		{
    		xxindex = nnet3->input_index[nnet3->ac_index];
		}
    	if(nnet3->ac_index != 0 && nnet3->hasac != 1)
    	{
			//if(ivec)
			//	xxindex = nnet3->input_index[nnet3->ac_index+1];
			//else
			xxindex = nnet3->input_index[nnet3->ac_index];
                        if (nnet3->m[xxindex] != NULL) {
                            qtk_blas_matrix_delete(nnet3->m[xxindex]);
                            nnet3->m[xxindex] = NULL;
                        }
                        nnet3->m[xxindex] = qtk_blas_matrix_new(
                            nnet3->frames_per_chunk, nnet3->input_col);

                        if(ivec)
			{
                            if (nnet3->m[iindex] != NULL) {
                                qtk_blas_matrix_delete(nnet3->m[iindex]);
                                nnet3->m[iindex] = NULL;
                            }
                            nnet3->m[iindex] =
                                qtk_blas_matrix_new(nnet3->frames_per_chunk / 3,
                                                    nnet3->ivector_dim);
                        }
			nnet3->hasac = 1;
    	}
        mat = nnet3->m[xxindex];
        if (ivec)
            imat = nnet3->m[iindex];
        // wtk_debug("==============%d %d %d %d
        // %p\n",feature->index,nnet3->end_input_frame,xxindex,iindex,mat);
    	if(feature->index == 0 && nnet3->begin_input_frame<0)
    	{
            // nnet3->m[xxindex] =
            // qtk_blas_matrix_new(-nnet3->begin_input_frame+nnet3->end_input_frame,nnet3->input_col);
            mat = nnet3->m[xxindex];
            for (i = nnet3->begin_input_frame; i <= 0; i++) {
                memcpy(mat->m + nnet3->row * mat->col, feature->v,
                       sizeof(float) * dim);
                nnet3->row++;
    		}
			if(ivec)
			{
    			for(i = nnet3->begin_input_frame/3; i <= 1; i++)
    			{
					//wtk_debug("ddddddddddd %d\n",nnet3->begin_input_frame);
        			memcpy(imat->m + nnet3->irow * imat->col, nnet3->ivector, sizeof(float)*nnet3->ivector_dim);
        			nnet3->irow++;
    			}
			}
    	}else
    	{
    		//TODO
    		memcpy(mat->m+nnet3->row*mat->col,feature->v,sizeof(float)*dim);
//    		memcpy(mat->m + nnet3->row * mat->col+dim, nnet3->ivector, sizeof(float)*nnet3->ivector_dim);
        	nnet3->row++;
			if(ivec)
			{
				if(feature->index%3==0)
				{
					//wtk_debug("ddddddddddd\n");
        			memcpy(imat->m + nnet3->irow * imat->col, nnet3->ivector, sizeof(float)*nnet3->ivector_dim);
	        		nnet3->irow++;
				}
			}
    	}
		//wtk_debug("%d %d %d \n",feature->index,nnet3->end_input_frame,nnet3->row-1);
		//wtk_debug("==============%d %d\n",feature->index,nnet3->end_input_frame);
    	if(feature->index<nnet3->end_input_frame-1)
    	{
    		return;
    	}
		qtk_nnet3_run2(nnet3,end, 0);
    }else
    {
	   //if (nnet3->notify)
	   //    nnet3->notify(nnet3->notify_ths,NULL,1, 0);

//			/*
		if(end)
		{
			if(!nnet3->cur_kfeat->v || nnet3->begin_input_frame >=  nnet3->cur_kfeat->index)
			{
				if(nnet3->cur_kfeat->v)
				{
                	int j;
                	for(j = 0; j < nnet3->cfg->frame_plus; j++)
                	{
						if(ivec)
						{
                    		xxindex = nnet3->input_index[nnet3->ac_index];
                    		iindex = nnet3->input_index[nnet3->ac_index+1];
						}else
						{
                    		xxindex = nnet3->input_index[nnet3->ac_index];
						}
                                                if (!nnet3->m[xxindex]) {
                                                    nnet3->m[xxindex] =
                                                        qtk_blas_matrix_new(
                                                            nnet3
                                                                ->frames_per_chunk,
                                                            nnet3->input_col);
                                                }
                                                mat = nnet3->m[xxindex];

                                                if(ivec)
						{
                                                    if (!nnet3->m[iindex]) {
                                                        nnet3->m[iindex] =
                                                            qtk_blas_matrix_new(
                                                                1,
                                                                nnet3
                                                                    ->ivector_dim);
                                                    }
                                                    imat = nnet3->m[iindex];
                                                }
						//wtk_debug("wav end:%d %d %p %p\n",xxindex,iindex,mat,imat);
    	                for(i = nnet3->row;i<nnet3->frames_per_chunk;i++)
	                    {
                      		//wtk_debug("======== %d %d %d\n",i,nnet3->begin_input_frame,nnet3->cur_feat->index);
                     		memcpy(mat->m+nnet3->row*mat->col,nnet3->cur_kfeat->v,sizeof(float)*dim);
//                    		memcpy(mat->m + nnet3->row * mat->col+dim, nnet3->ivector, sizeof(float)*nnet3->ivector_dim);
                     		nnet3->row++;
                    	}
						if(ivec)
							memcpy(imat->m, nnet3->ivector, sizeof(float)*nnet3->ivector_dim);
                    	qtk_nnet3_run2(nnet3,0, 1);
                	}
					//wtk_feat_push_back(nnet3->cur_kfeat);
				}
				if (nnet3->notify)
					nnet3->notify(nnet3->notify_ths,NULL,1,0);
				return;
			}else
			{
				if(ivec)
				{
                	xxindex = nnet3->input_index[nnet3->ac_index];
                	iindex = nnet3->input_index[nnet3->ac_index+1];
				}else
				{
                	xxindex = nnet3->input_index[nnet3->ac_index];
				}
                                // nnet3->m[xxindex] =
                                // qtk_blas_matrix_new(nnet3->frames_per_chunk,nnet3->input_col);
                                if (!nnet3->m[xxindex]) {
                                    nnet3->m[xxindex] = qtk_blas_matrix_new(
                                        nnet3->frames_per_chunk,
                                        nnet3->input_col);
                                }
                                mat = nnet3->m[xxindex];

                                if(ivec)
				{
                                    if (!nnet3->m[iindex]) {
                                        nnet3->m[iindex] = qtk_blas_matrix_new(
                                            1, nnet3->ivector_dim);
                                    }
                                    imat = nnet3->m[iindex];
                                }
				//wtk_debug("wav end:%d %d %p %p\n",xxindex,iindex,mat,imat);
                for(i = nnet3->row;i<nnet3->frames_per_chunk;i++)
                {
                    //wtk_debug("%d %d %d\n",i,nnet3->begin_input_frame,nnet3->cur_feat->index);
                  memcpy(mat->m+nnet3->row*mat->col,nnet3->cur_kfeat->v,sizeof(float)*dim);
//          		memcpy(mat->m + nnet3->row * mat->col+dim, nnet3->ivector, sizeof(float)*nnet3->ivector_dim);
                  nnet3->row++;
                }
//				wtk_debug("wwwwwwww %p %d\n",imat->m,nnet3->irow,imat->col);
				if(ivec && imat)
					memcpy(imat->m, nnet3->ivector, sizeof(float)*nnet3->ivector_dim);
                qtk_nnet3_run2(nnet3,0, 0);

				int j;
				for(j = 0; j < nnet3->cfg->frame_plus; j++)
				{
					if(ivec)
					{
                		xxindex = nnet3->input_index[nnet3->ac_index];
                		iindex = nnet3->input_index[nnet3->ac_index+1];
					}else
					{
                		xxindex = nnet3->input_index[nnet3->ac_index];
					}

					//xxindex = nnet3->input_index[nnet3->ac_index-1];
                                        if (!nnet3->m[xxindex]) {
                                            nnet3->m[xxindex] =
                                                qtk_blas_matrix_new(
                                                    nnet3->frames_per_chunk,
                                                    nnet3->input_col);
                                        }
                                        mat = nnet3->m[xxindex];
                                        if(ivec)
					{
                                            if (!nnet3->m[iindex]) {
                                                nnet3->m[iindex] =
                                                    qtk_blas_matrix_new(
                                                        1, nnet3->ivector_dim);
                                            }
                                            imat = nnet3->m[iindex];
                                        }
                                        // mat = nnet3->m[xxindex];
                                        for(i = nnet3->row;i<nnet3->frames_per_chunk;i++)
					{
						//wtk_debug("======== %d %d %d\n",i,nnet3->begin_input_frame,nnet3->cur_feat->index);
						//wtk_debug("cpy %d %p\n",xxindex,mat);
						memcpy(mat->m+nnet3->row*mat->col,nnet3->cur_kfeat->v,sizeof(float)*dim);
//		        		memcpy(mat->m + nnet3->row * mat->col+dim, nnet3->ivector, sizeof(float)*nnet3->ivector_dim);
						nnet3->row++;
					}
					if(ivec)
						memcpy(imat->m + nnet3->irow * imat->col, nnet3->ivector, sizeof(float)*nnet3->ivector_dim);
					qtk_nnet3_run2(nnet3,0, 1);
				}

				if (nnet3->notify)
					nnet3->notify(nnet3->notify_ths,NULL,1, 0);
			}
		}//*/
    }
}

void qtk_nnet3_run_kfeat(qtk_nnet3_t* nnet3,wtk_kfeat_t *feature,int end)
{
	if(nnet3->cfg->extra_left > 0)
	{
		qtk_nnet3_run_kfeat_extra(nnet3,feature,end);
	}else
	{
		qtk_nnet3_run_kfeat_normal(nnet3,feature,end);
	}
}
