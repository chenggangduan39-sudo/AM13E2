#include "qtk_nnet3_cfg.h"
#ifndef HEXAGON
#include "wtk/os/wtk_cpu.h"
#endif
//input feature index 1
int sched_getcpu(void);

void qtk_nnet3_pointer_init(qtk_nnet3_cfg_t *cfg);
int qtk_nnet3_cfg_init(qtk_nnet3_cfg_t *cfg)
{
	cfg->tdnn_fn = NULL;
	cfg->xvector_fn = NULL;
	cfg->xvector = NULL;
	//wtk_queue_init(&(cfg->comp_q));
	cfg->compution_fn = NULL;
	cfg->compution_fn2 = NULL;
	cfg->t_mdl = NULL;//(qtk_trans_model_cfg_t*) wtk_malloc(sizeof(qtk_trans_model_cfg_t));
	cfg->wt_mdl = NULL;
	cfg->compution = NULL;
	cfg->compution2 = NULL;

	qtk_nnet3_pointer_init(cfg);
	cfg->prior=NULL;
	cfg->left_context=-1;
	cfg->right_context=-1;
	cfg->frame_plus=3;
	cfg->frame_per_chunk=3;
	cfg->frame_subsample_factor=1;
	cfg->use_kwdec = 0;
	cfg->use_xvector = 0;
	cfg->normal = 0;
    cfg->is_fixed = 0;
    cfg->fixed_nbytes=sizeof(short);
    cfg->use_fix_res = 0;
    cfg->bin_fn = "./nnet3.bin";
    cfg->comp_cnt =0;
    cfg->max_w = 2048;//255;
    cfg->use_custom_acc = 0;
    cfg->porder = 3;
	cfg->extra_left=0;
        cfg->use_fix_wake = 0;

        return 0;
}

void qtk_nnet3_pointer_init(qtk_nnet3_cfg_t *cfg)
{
	int i = 0;
	for (i = 0; i < 120; i++) 
	{
		cfg->comp_q[i] = NULL;
	}
	for(i=0;i<1000;++i)
	{
		cfg->ms[i]=NULL;
	}
}

void qtk_nnet3_cfg_pointer_del(qtk_nnet3_cfg_t *cfg)
{
	int i = 0;
	for (i = 0; i < 1000; i++) 
	{
		if(cfg->ms[i]!=NULL)
		{
			wtk_mats_delete(cfg->ms[i]);
		}
	}

	for (i = 0; i < 120; i++)
	{
		if (cfg->comp_q[i] != NULL)
		{
			switch (cfg->comp_q[i]->type)
			{
			case QTK_AffineGlobalComponent:
				qtk_affine_global_delete(cfg->comp_q[i],cfg->is_fixed);
				break;
			case QTK_RectifiedLinearComponent:
				qtk_rectified_linear_delete(cfg->comp_q[i]);
				break;
			case QTK_NormalizeComponent:
				qtk_normallize_delete(cfg->comp_q[i]);
				break;
			case QTK_ActivateComponent:
				qtk_activate_delete(cfg->comp_q[i]);
				break;
			case QTK_LstmNonlinearityComponent:
				qtk_lstm_nolinearity_delete(cfg->comp_q[i],cfg->is_fixed);
				break;
			case QTK_TimeHeightConvolutionComponent:
				qtk_timeheight_convolution_delete(cfg->comp_q[i],cfg->is_fixed);
				break;
			case QTK_BatchNormComponent:
				qtk_batchnorm_delete(cfg->comp_q[i],cfg->is_fixed);
				break;
			case QTK_ScaleAndOffsetComponent:
				qtk_scale_offset_delete(cfg->comp_q[i],cfg->is_fixed);
				break;
			case QTK_CompactFsmnComponent:
				qtk_fsmn_component_delete(cfg->comp_q[i],cfg->is_fixed);
				break;
			case QTK_NaturalGradientAffineMaskComponent:
				qtk_namask_component_delete(cfg->comp_q[i],cfg->is_fixed);
				break;
			case QTK_LinearMaskComponent:
				qtk_linearmask_component_delete(cfg->comp_q[i],cfg->is_fixed);
				break;
			case QTK_PermuteComponent:
				qtk_permute_component_delete(cfg->comp_q[i],cfg->is_fixed);
				break;
			default:
				break;
			}
			free(cfg->comp_q[i]->component);

			free(cfg->comp_q[i]);
		} else
		{
			break;
		}
	}
}

int qtk_nnet3_cfg_clean(qtk_nnet3_cfg_t *cfg)
{
	if(cfg->prior)
	{
		qtk_blas_matrix_delete(cfg->prior);
	}
	if(cfg->use_kwdec)
	{
		if(cfg->normal)
		{
			qtk_wakeup_trans_model_cfg_clean(cfg->wt_mdl,0);
		}else
		{
			qtk_wakeup_trans_model_cfg_clean(cfg->wt_mdl,1);
		}
		
	}else if(cfg->t_mdl)
	{
		if(cfg->normal)
		{
			qtk_trans_model_cfg_clean(cfg->t_mdl,0);
		}else
		{
			qtk_trans_model_cfg_clean(cfg->t_mdl,1);
		}
	}
	qtk_nnet3_cfg_pointer_del(cfg);
	if(cfg->use_kwdec)
	{
		if (cfg->wt_mdl)
			wtk_free(cfg->wt_mdl);
	}else
	{
		if (cfg->t_mdl)
			wtk_free(cfg->t_mdl);
	}
	if(cfg->compution)
	{
		qtk_nnet3_compution_delete(cfg->compution);
	}
	if(cfg->compution2)
	{
		qtk_nnet3_compution_delete(cfg->compution2);
	}

    if(cfg->xvector)
    {
    	wtk_vecf_delete(cfg->xvector);
    }

	return 0;
}

int qtk_nnet3_cfg_update_local(qtk_nnet3_cfg_t *cfg, wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc, cfg, tdnn_fn, v);
	wtk_local_cfg_update_cfg_str(lc, cfg, compution_fn, v);
	wtk_local_cfg_update_cfg_str(lc, cfg, compution_fn2, v);
	wtk_local_cfg_update_cfg_str(lc, cfg, feat_fn, v);
	wtk_local_cfg_update_cfg_i(lc,cfg,left_context,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,right_context,v);
	wtk_local_cfg_update_cfg_str(lc, cfg, xvector_fn, v);
	wtk_local_cfg_update_cfg_i(lc,cfg,frame_plus,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,fixed_nbytes,v);
	wtk_local_cfg_update_cfg_i2(lc,cfg,use_custom_acc,op_level,v);  //fit old res.
	wtk_local_cfg_update_cfg_i(lc,cfg,use_custom_acc,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,porder,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,frame_per_chunk,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,frame_subsample_factor,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,extra_left,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_kwdec,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_xvector,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fix_res,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,is_fixed,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,normal,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,bin_fn,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_w,v);
        wtk_local_cfg_update_cfg_f(lc, cfg, use_fix_wake, v);

        return 0;
}

void qtk_merge_fix_component(nnet3_component_t* fix_comp,
		nnet3_component_t* comp2, qtk_affine_global_component_t* out,float max_w, int is_fixed)
{
	qtk_affine_global_component_t* fa_com;
	qtk_affine_global_component_t* nga_com;
	int col2, row2, row, col;
	int i, j, k, x;
	qtk_blas_matrix_t *w1, *b1, *w2, *b2;

	fa_com = (qtk_affine_global_component_t*) fix_comp->component;
	nga_com = (qtk_affine_global_component_t*) comp2->component;
	w1 = fa_com->w;
	b1 = fa_com->b;
	w2 = nga_com->w;
	b2 = nga_com->b;
	col2 = b2->col;
	row2 = b1->col;
	row = w2->row;
	col = w1->col;
	x = w1->row;

	out->b = qtk_blas_matrix_new(1, col2);
	out->w = qtk_blas_matrix_new(row, col);

	for (i = 0; i < col2; ++i)
	{
		for (j = 0; j < row2; ++j)
		{
			*(out->b->m + i) += (*(b1->m + j)) * (*(w2->m + i * w2->col + j));
		}
	}
	for (i = 0; i < col2; ++i)
	{
		*(out->b->m + i) += *(b2->m + i);
	}
	for (i = 0; i < row; ++i)
	{
		for (j = 0; j < col; ++j)
		{
			for (k = 0; k < x; ++k)
			{
				*(out->w->m + i * out->w->col + j) +=
						(*(w2->m + i * w2->col + k))
								* (*(w1->m + k * w1->col + j));
			}
		}
	}
        if(is_fixed)
        {
                wtk_mats_new(row,col);
                qtk_nnet3_update_fix_f2s(out->w,out->ws,max_w);
                qtk_blas_matrix_delete(out->w);
        }

}

int qtk_tdnn_mdl_load(qtk_nnet3_cfg_t *cfg, wtk_source_t *src) {
	if(cfg->use_kwdec)
	{
		cfg->wt_mdl = (qtk_wakeup_trans_model_cfg_t*) wtk_malloc(sizeof(qtk_wakeup_trans_model_cfg_t));
	}else
	{
        	cfg->t_mdl = (qtk_trans_model_cfg_t*) wtk_malloc(sizeof(qtk_trans_model_cfg_t));
	}
	int ret = 0;
	wtk_strbuf_t *buf;
	int num_components;
	nnet3_component_t* comp;
	qtk_affine_global_component_t* global_aff_com;
	qtk_natural_gradient_affine_component_t* nga_com;
	qtk_rectified_linear_component_t* rl_com;
	qtk_normalize_component_t* n_com;
	qtk_activate_component_t* ls_com;
	qtk_element_wise_product_component_t* wise_com;
	qtk_dropout_component_t* drop_com;
	qtk_natural_gradient_per_element_scale_component_t* scale_com;
	qtk_backprop_truncation_component_t* backprop_com;
	qtk_batch_norm_component_t* batchnorm_com;
	qtk_linear_component_t* linear_com;
	qtk_lstm_nolinearity_component_t* lstm_com;
	qtk_timeheight_convolution_component_t* conv_com;
	qtk_scale_offset_component_t* scaleoff_com;
	qtk_max_pooling_component_t* maxp_com;
	qtk_pnorm_component_t* pnorm_com;
	qtk_compactfsmn_component_t *fsmn;
        qtk_blockweight_component_t *block_weight;
        qtk_general_dropout_component_t *gd_comp;
	qtk_natrual_gradient_affine_mask_component_t *gm_comp;
	qtk_linear_mask_component_t *lm_comp;
	qtk_permute_component_t *pc_comp;
	buf = wtk_strbuf_new(256, 1);

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if(cfg->use_kwdec==1)
	{
		if(cfg->normal)
			qtk_wakeup_trans_model_load_normal(cfg->wt_mdl, src);		
		else
		{
			qtk_wakeup_trans_model_load_chain(cfg->wt_mdl, src);
		}
		
	}else
	{
		if(cfg->normal)
			ret = qtk_trans_model_load_normal(cfg->t_mdl, src);
		else
			ret = qtk_trans_model_load_chain(cfg->t_mdl, src);
		if(ret != 0)
		{
			wtk_free(cfg->t_mdl->trans_model);
			wtk_free(cfg->t_mdl);
			cfg->t_mdl = NULL;
		}
	}
//	qtk_trans_model_cal_id2pdf_chain(cfg->t_mdl);
//	if(qtk_trans_model_load(cfg->t_mdl,src))
//	{
//		ret=-1;
//		goto end;
//	}
	if(ret == 0)
	{
		ret = wtk_source_read_string(src, buf);
		if (!wtk_str_equal_s(buf->data, buf->pos, "<Nnet3>"))
		{
			ret = -1;
			goto end;
		}
	}
	while (1) 
	{
		wtk_source_read_line(src, buf);
		if (wtk_str_equal_s(buf->data, buf->pos,
				"")||wtk_str_equal_s(buf->data,buf->pos,"\r"))
		{
			break;
		}
	}
	ret = wtk_source_read_string(src, buf);
	if (ret != 0) 
	{
		goto end;
	}
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumComponents>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &num_components, 1, 1);
	//wtk_debug("%d\n",num_components);
	if (ret != 0) {
		goto end;
	}
	cfg->comp_cnt = num_components;

	int i = 0;
	int fix_index=-1;
	int is_fix = 0;
	for (i = 0; i < num_components; i++)
	{
		ret = wtk_source_read_string(src, buf);
		if (ret != 0) 
		{
			goto end;
		}
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		if (!wtk_str_equal_s(buf->data, buf->pos, "<ComponentName>"))
		{
			ret = -1;
			goto end;
		}
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);

		cfg->comp_q[i] = (nnet3_component_t*) wtk_malloc(sizeof(nnet3_component_t));
		comp = cfg->comp_q[i];

		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		//wtk_debug("%p %d\n",comp,i);
		if (wtk_str_equal_s(buf->data, buf->pos, "<FixedAffineComponent>"))
		{
			is_fix = 1;
			fix_index = i;
			global_aff_com = (qtk_affine_global_component_t*)wtk_malloc(sizeof(qtk_affine_global_component_t));
			comp->component = global_aff_com;
			comp->type = QTK_AffineGlobalComponent;
			global_aff_com->type = QTK_FixedAffineComponent;
			qtk_affine_global_component_read(comp, src, buf, cfg->max_w, cfg->is_fixed);
		} else if (wtk_str_equal_s(buf->data, buf->pos,"<NaturalGradientAffineComponent>"))
		{
			global_aff_com = (qtk_affine_global_component_t*)wtk_malloc(sizeof(qtk_affine_global_component_t));
			comp->component = global_aff_com;
			nga_com = (qtk_natural_gradient_affine_component_t*) wtk_malloc(
					sizeof(qtk_natural_gradient_affine_component_t));
			global_aff_com->nga_com = nga_com;
			comp->type = QTK_AffineGlobalComponent;
			global_aff_com->type = QTK_NaturalGradientAffineComponent;
			qtk_affine_global_component_read2(comp, src, buf, cfg->max_w,cfg->is_fixed, cfg->fixed_nbytes, cfg->use_custom_acc, cfg->porder);
			if (is_fix)
			{
				nnet3_component_t* tmp;

				is_fix = 0;
				num_components = num_components - 2;
				tmp = (nnet3_component_t*) wtk_malloc(sizeof(nnet3_component_t));
				global_aff_com = (qtk_affine_global_component_t*) wtk_malloc(sizeof(qtk_affine_global_component_t));

				qtk_merge_fix_component(cfg->comp_q[fix_index], cfg->comp_q[i],global_aff_com,cfg->max_w,cfg->is_fixed);
				tmp->component = global_aff_com;
				global_aff_com->type = QTK_AffineComponent;
				tmp->type = QTK_AffineGlobalComponent;
				qtk_affine_global_delete(cfg->comp_q[fix_index],cfg->is_fixed);
				wtk_free(cfg->comp_q[fix_index]->component);
				wtk_free(cfg->comp_q[fix_index]);
				qtk_affine_global_delete(cfg->comp_q[i],cfg->is_fixed);
				wtk_free(cfg->comp_q[i]->component);
				wtk_free(cfg->comp_q[i]);
				i = fix_index - 1;
				//wtk_debug("fixindex:%d %d\n",fix_index,num_components);
				cfg->comp_q[num_components] = tmp;
				//cfg->comp_q[add_index]=tmp;
			}
		} else if (wtk_str_equal_s(buf->data, buf->pos,"<RectifiedLinearComponent>"))
		{
			rl_com = (qtk_rectified_linear_component_t*) wtk_malloc(sizeof(qtk_rectified_linear_component_t));
			comp->component = rl_com;
			comp->type = QTK_RectifiedLinearComponent;
			qtk_rectified_linear_component_read(comp, src, buf);
			is_fix = 0;
		} else if (wtk_str_equal_s(buf->data, buf->pos,"<NormalizeComponent>"))
		{
			n_com = (qtk_normalize_component_t*) wtk_malloc(sizeof(qtk_normalize_component_t));
			comp->component = n_com;
			comp->type = QTK_NormalizeComponent;
			qtk_normalize_component_read(comp, src, buf);
			is_fix = 0;
		} else if (wtk_str_equal_s(buf->data, buf->pos,"<LogSoftmaxComponent>"))
		{
			ls_com = (qtk_activate_component_t*) wtk_malloc(sizeof(qtk_activate_component_t));
			comp->component = ls_com;
			ls_com->type = QTK_LogSoftmaxComponent;
			comp->type = QTK_ActivateComponent;
			qtk_activate_component_read(comp, src, buf);
                        is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<SoftmaxComponent>")) {
                    ls_com = (qtk_activate_component_t *)wtk_malloc(
                        sizeof(qtk_activate_component_t));
                    comp->component = ls_com;
                    ls_com->type = QTK_SoftmaxComponent;
                    comp->type = QTK_ActivateComponent;
                    qtk_activate_component_read(comp, src, buf);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<SigmoidComponent>")) {
                    ls_com = (qtk_activate_component_t *)wtk_malloc(
                        sizeof(qtk_activate_component_t));
                    comp->component = ls_com;
                    ls_com->type = QTK_SigmoidComponent;
                    comp->type = QTK_ActivateComponent;
                    qtk_activate_component_read(comp, src, buf);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<TanhComponent>")) {
                    ls_com = (qtk_activate_component_t *)wtk_malloc(
                        sizeof(qtk_activate_component_t));
                    comp->component = ls_com;
                    ls_com->type = QTK_TanhComponent;
                    comp->type = QTK_ActivateComponent;
                    qtk_activate_component_read(comp, src, buf);
                    is_fix = 0;
                } else if (wtk_str_equal_s(
                               buf->data, buf->pos,
                               "<NaturalGradientPerElementScaleComponent>")) {
                    scale_com = (qtk_natural_gradient_per_element_scale_component_t
                                     *)
                        wtk_malloc(sizeof(
                            qtk_natural_gradient_per_element_scale_component_t));
                    comp->component = scale_com;
                    comp->type = QTK_NaturalGradientPerElementScaleComponent;
                    qtk_natural_gradient_per_element_scale_component_read(
                        comp, src, buf, cfg->max_w, cfg->is_fixed);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<DropoutComponent>")) {
                    drop_com = (qtk_dropout_component_t *)wtk_malloc(
                        sizeof(qtk_dropout_component_t));
                    comp->component = drop_com;
                    comp->type = QTK_DropoutComponent;
                    qtk_dropout_component_read(comp, src, buf);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<ElementwiseProductComponent>")) {
                    wise_com =
                        (qtk_element_wise_product_component_t *)wtk_malloc(
                            sizeof(qtk_element_wise_product_component_t));
                    comp->component = wise_com;
                    comp->type = QTK_ElementwiseProductComponent;
                    qtk_element_wise_product_component_read(comp, src, buf);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<BackpropTruncationComponent>")) {
                    backprop_com =
                        (qtk_backprop_truncation_component_t *)wtk_malloc(
                            sizeof(qtk_backprop_truncation_component_t));
                    comp->component = backprop_com;
                    comp->type = QTK_BackpropTruncationComponent;
                    qtk_backprop_truncation_component_read(comp, src, buf);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<NoOpComponent>")) {
                    comp->component = NULL;
                    comp->type = QTK_NoopComponent;
                    is_fix = 0;
                    qtk_noop_component_read(comp, src, buf);
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<BatchNormComponent>")) {
                    batchnorm_com = (qtk_batch_norm_component_t *)wtk_malloc(
                        sizeof(qtk_batch_norm_component_t));
                    comp->component = batchnorm_com;
                    comp->type = QTK_BatchNormComponent;
                    qtk_batch_norm_component_read(comp, src, buf, cfg->max_w,
                                                  cfg->is_fixed);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<AffineComponent>")) {
                    global_aff_com =
                        (qtk_affine_global_component_t *)wtk_malloc(
                            sizeof(qtk_affine_global_component_t));
                    comp->component = global_aff_com;
                    global_aff_com->type = QTK_AffineComponent;
                    comp->type = QTK_AffineGlobalComponent;
                    // qtk_affine_global_component_read(comp, src, buf,
                    // cfg->max_w,cfg->is_fixed);
                    qtk_affine_global_component_read2(
                        comp, src, buf, cfg->max_w, cfg->is_fixed,
                        cfg->fixed_nbytes, cfg->use_custom_acc, cfg->porder);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<LinearComponent>")) {
                    global_aff_com =
                        (qtk_affine_global_component_t *)wtk_malloc(
                            sizeof(qtk_affine_global_component_t));
                    comp->component = global_aff_com;
                    linear_com = (qtk_linear_component_t *)wtk_malloc(
                        sizeof(qtk_linear_component_t));
                    global_aff_com->linear_com = linear_com;
                    global_aff_com->type = QTK_LinearComponent;
                    comp->type = QTK_AffineGlobalComponent;
                    qtk_affine_global_component_read(comp, src, buf, cfg->max_w,
                                                     cfg->is_fixed);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<LstmNonlinearityComponent>")) {
                    lstm_com = (qtk_lstm_nolinearity_component_t *)wtk_malloc(
                        sizeof(qtk_lstm_nolinearity_component_t));
                    comp->component = lstm_com;
                    comp->type = QTK_LstmNonlinearityComponent;
                    qtk_lstm_nolinearity_component_read(
                        comp, src, buf, cfg->max_w, cfg->is_fixed);
                    is_fix = 0;
                } else if (wtk_str_equal_s(
                               buf->data, buf->pos,
                               "<TimeHeightConvolutionComponent>")) {
                    conv_com =
                        (qtk_timeheight_convolution_component_t *)wtk_malloc(
                            sizeof(qtk_timeheight_convolution_component_t));
                    comp->component = conv_com;
                    comp->type = QTK_TimeHeightConvolutionComponent;
                    qtk_timeheight_convolution_component_read(
                        comp, src, buf, cfg->max_w, cfg->is_fixed);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<ScaleAndOffsetComponent>")) {
                    scaleoff_com = (qtk_scale_offset_component_t *)wtk_malloc(
                        sizeof(qtk_scale_offset_component_t));
                    comp->component = scaleoff_com;
                    comp->type = QTK_ScaleAndOffsetComponent;
                    qtk_scale_offset_component_read(comp, src, buf, cfg->max_w,
                                                    cfg->is_fixed);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<MaxpoolingComponent>")) {
                    maxp_com = (qtk_max_pooling_component_t *)wtk_malloc(
                        sizeof(qtk_max_pooling_component_t));
                    comp->component = maxp_com;
                    comp->type = QTK_MaxPoolingComponent;
                    qtk_max_pooling_component_read(comp, src, buf, cfg->max_w,
                                                   cfg->is_fixed);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<PnormComponent>")) {
                    pnorm_com = (qtk_pnorm_component_t *)wtk_malloc(
                        sizeof(qtk_pnorm_component_t));
                    comp->component = pnorm_com;
                    comp->type = QTK_PnormComponent;
                    qtk_pnorm_component_read(comp, src, buf, cfg->max_w,
                                             cfg->is_fixed);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<CompactFsmnComponent>")) {
                    fsmn = (qtk_compactfsmn_component_t *)wtk_malloc(
                        sizeof(qtk_compactfsmn_component_t));
                    comp->component = fsmn;
                    comp->type = QTK_CompactFsmnComponent;
                    qtk_fsmn_component_read(comp, src, buf, cfg->max_w,
                                            cfg->is_fixed);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<BlockWeightComponent>")) {
                    block_weight = (qtk_blockweight_component_t *)wtk_malloc(
                        sizeof(qtk_blockweight_component_t));
                    comp->component = block_weight;
                    comp->type = QTK_BlockWeightComponent;
                    qtk_blockweight_component_read(comp, src, buf, cfg->max_w,
                                                   cfg->is_fixed);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<GeneralDropoutComponent>")) {
                    gd_comp = (qtk_general_dropout_component_t *)wtk_malloc(
                        sizeof(qtk_general_dropout_component_t));
                    comp->component = gd_comp;
                    comp->type = QTK_GeneralDropoutComponent;
                    qtk_general_dropout_component_read(
                        comp, src, buf, cfg->max_w, cfg->is_fixed);
                    is_fix = 0;
                } else if (wtk_str_equal_s(
                               buf->data, buf->pos,
                               "<NaturalGradientAffineMaskComponent>")) {
                    gm_comp = (qtk_natrual_gradient_affine_mask_component_t *)
                        wtk_malloc(sizeof(
                            qtk_natrual_gradient_affine_mask_component_t));
                    comp->component = gm_comp;
                    comp->type = QTK_NaturalGradientAffineMaskComponent;
                    // qtk_namask_component_read(comp, src, buf,
                    // cfg->max_w,cfg->is_fixed);
                    qtk_namask_component_read2(comp, src, buf, cfg->max_w,
                                               cfg->is_fixed,
                                               cfg->use_custom_acc);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<LinearMaskComponent>")) {
                    lm_comp = (qtk_linear_mask_component_t *)wtk_malloc(
                        sizeof(qtk_linear_mask_component_t));
                    comp->component = lm_comp;
                    comp->type = QTK_LinearMaskComponent;
                    qtk_linearmask_component_read(comp, src, buf, cfg->max_w,
                                                  cfg->is_fixed);
                    is_fix = 0;
                } else if (wtk_str_equal_s(buf->data, buf->pos,
                                           "<PermuteComponent>")) {
                    pc_comp = (qtk_permute_component_t *)wtk_malloc(
                        sizeof(qtk_permute_component_t));
                    comp->component = pc_comp;
                    comp->type = QTK_PermuteComponent;
                    qtk_permute_component_read(comp, src, buf, cfg->max_w,
                                               cfg->is_fixed);
                    ;
                    is_fix = 0;
                } else {
                    printf("%.*s\n", buf->pos, buf->data);
                    wtk_debug("not support component\n");
                    exit(0);
                }
        }

	ret = wtk_source_read_string(src, buf);
	if (ret != 0)
	{
		goto end;
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "</Nnet3>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	ret = wtk_source_read_string(src, buf);
    if (!wtk_str_equal_s(buf->data, buf->pos, "<LeftContext>"))
    {
        ret = 0;
        goto end;
    }
	int val;
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
	cfg->left_context=val;
	//wtk_debug("%d\n",cfg->left_context);
    ret = wtk_source_read_string(src, buf);
    if (!wtk_str_equal_s(buf->data, buf->pos, "<RightContext>"))
    {
        ret = -1;
        goto end;
    }
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
	cfg->right_context=val;

	//wtk_debug("%d\n",val);
    ret = wtk_source_read_string(src, buf);
    if (!wtk_str_equal_s(buf->data, buf->pos, "<Priors>"))
    {
        ret = -1;
        goto end;
    }
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_string(src, buf);
    if (!wtk_str_equal_s(buf->data, buf->pos, "FV"))
    {
        ret = -1;
        goto end;
    }

    ret = wtk_source_read_char(src);
    ret = wtk_source_read_char(src);
    ret = wtk_source_read_int_little(src, &val, 1, 1);
	//wtk_debug("%d\n",val);

	if(val>0)
	{
		cfg->prior=qtk_blas_matrix_new(1, val);
		ret = wtk_source_read_float_little(src, cfg->prior->m, val, 1);
		int x;
		for(x = 0; x < val; x++)
		{
			cfg->prior->m[x] = -log(cfg->prior->m[x]);
		}
	}

	end:

	wtk_strbuf_delete(buf);
	return ret;
}

void qtk_nnet3_cfg_write_fix_bin(qtk_nnet3_cfg_t *cfg, char *fn)
{
    int i,t;
    FILE *f;
    int com_cnt;
    //int ret;
    nnet3_component_t* com;
    com_cnt =cfg->comp_cnt;
    f=fopen(fn,"wb");
        qtk_affine_global_component_t* ga_com;
        //qtk_rectified_linear_component_t* rl_com;
        qtk_normalize_component_t* n_com;
        qtk_activate_component_t* ls_com;
        //qtk_element_wise_product_component_t* wise_com;
        //qtk_dropout_component_t* drop_com;
        qtk_natural_gradient_per_element_scale_component_t* scale_com;
        qtk_backprop_truncation_component_t* backprop_com;
        qtk_batch_norm_component_t* batchnorm_com;
        qtk_lstm_nolinearity_component_t* lstm_com;
        qtk_timeheight_convolution_component_t* conv_com;
        qtk_scale_offset_component_t* scaleoff_com;
        qtk_pnorm_component_t* pnorm_com;
        qtk_compactfsmn_component_t *FsmnCom;
        if(cfg->use_kwdec)
        {
                qtk_nnet3_wakeup_trans_mdl_write(f,cfg->wt_mdl->trans_model);
        }else
        {
            if (cfg->t_mdl) {
                if (cfg->normal)
                    qtk_nnet3_trans_mdl_write_normal(f,
                                                     cfg->t_mdl->trans_model);
                else
                    qtk_nnet3_trans_mdl_write(f, cfg->t_mdl->trans_model);
            }
        }
        fwrite(&com_cnt,4,1,f);
        for(i=0;i<com_cnt;++i)
        {
        com = cfg->comp_q[i];
        t = com->type;
        fwrite(&t,4,1,f);
//      wtk_debug("t=%d\n",t);
        switch(t)
        {
            case QTK_AffineGlobalComponent:
                                ga_com=(qtk_affine_global_component_t*)com->component;
                                fwrite(&(ga_com->type),4,1,f);
                                //wtk_debug("type=%d\n",ga_com->type);
                                //wtk_nnet3_write_mati2(f,ga_com->ws, cfg->fixed_nbytes);
                                qtk_nnet3_write_mati(f,ga_com->ws, cfg->fixed_nbytes);
                                if(ga_com->type!=QTK_LinearComponent)
                                {
                                        qtk_nnet3_write_matrix(f,ga_com->b);
                                }
            break;
            case QTK_NormalizeComponent:
                                n_com=(qtk_normalize_component_t*)com->component;
                                fwrite(&(n_com->target_rms_),4,1,f);
            break;
            case QTK_BatchNormComponent:
                                batchnorm_com = (qtk_batch_norm_component_t*)com->component;
                                fwrite(&(batchnorm_com->dim),4,1,f);
                                fwrite(&(batchnorm_com->block_dim),4,1,f);
                                qtk_nnet3_write_mati(f,batchnorm_com->scale2, cfg->fixed_nbytes);
                                qtk_nnet3_write_mati(f,batchnorm_com->offset2, cfg->fixed_nbytes);
            break;
            case QTK_RectifiedLinearComponent:
            break;
            case QTK_ActivateComponent:
                                ls_com = (qtk_activate_component_t*)com->component;
                                fwrite(&(ls_com->type),4,1,f);
            break;
            case QTK_NaturalGradientPerElementScaleComponent:
                                scale_com = (qtk_natural_gradient_per_element_scale_component_t*)com->component;
                                qtk_nnet3_write_mati(f,scale_com->scale2, cfg->fixed_nbytes);
            break;
            case QTK_DropoutComponent:
            break;
            case QTK_ElementwiseProductComponent:
            break;
            case QTK_BackpropTruncationComponent:
 				backprop_com = (qtk_backprop_truncation_component_t*)com->component;
				fwrite(&(backprop_com->scale),4,1,f);
            break;
            case QTK_NoopComponent:
            break;
           case QTK_LstmNonlinearityComponent:
                                lstm_com = (qtk_lstm_nolinearity_component_t*)com->component;
                               // qtk_nnet3_write_mats(f,lstm_com->params2);
				qtk_nnet3_write_matrix(f,lstm_com->params);
            break;
            case QTK_TimeHeightConvolutionComponent:
                                conv_com = (qtk_timeheight_convolution_component_t*)com->component;
                                fwrite(&(conv_com->model->height_out),4,1,f);
                                fwrite(&(conv_com->model->num_filters_out),4,1,f);
                                fwrite(&(conv_com->model->height_in),4,1,f);
                                fwrite(&(conv_com->model->num_filters_in),4,1,f);
                //              qtk_nnet3_write_mats(f,conv_com->bias_params2);
                                qtk_nnet3_write_matrix(f,conv_com->bias_params);
                                qtk_nnet3_write_mati(f,conv_com->linear_params2, cfg->fixed_nbytes);
		//		qtk_nnet3_write_matrix(f,conv_com->linear_params);
            break;
            case QTK_ScaleAndOffsetComponent:
                                scaleoff_com = (qtk_scale_offset_component_t*)com->component;
                                qtk_nnet3_write_mati(f,scaleoff_com->scales2, cfg->fixed_nbytes);
                                qtk_nnet3_write_mati(f,scaleoff_com->offsets2, cfg->fixed_nbytes);
            break;
            case QTK_PnormComponent:
            	pnorm_com = (qtk_pnorm_component_t*)com->component;
                fwrite(&(pnorm_com->input_dim),4,1,f);
                fwrite(&(pnorm_com->output_dim),4,1,f);
            break;
            case QTK_CompactFsmnComponent:
                FsmnCom = (qtk_compactfsmn_component_t *)com->component;
                fwrite(&(FsmnCom->l_order), sizeof(int), 1, f);
                fwrite(&(FsmnCom->r_order), sizeof(int), 1, f);
                fwrite(&(FsmnCom->l_stride), sizeof(int), 1, f);
                fwrite(&(FsmnCom->r_stride), sizeof(int), 1, f);
                fwrite(&(FsmnCom->params->row), sizeof(unsigned int), 1, f);
                fwrite(&(FsmnCom->params->col), sizeof(unsigned int), 1, f);
                fwrite(FsmnCom->params->m, sizeof(float),
                       FsmnCom->params->row * FsmnCom->params->col, f);
                break;
            default:
            break;
        }
        }
        fwrite(&(cfg->left_context), 4, 1, f);
        fwrite(&(cfg->right_context), 4, 1, f);
        //prior
        int isprior;
        if (cfg->prior){
        	isprior=1;
        	fwrite(&isprior,4,1,f);
        	fwrite(&(cfg->prior->col),4,1,f);
        	fwrite(cfg->prior->m,sizeof(float),cfg->prior->col,f);
        }else
        {
        	isprior=0;
        	fwrite(&isprior,4,1,f);
        }


fclose(f);

}

int qtk_nnet3_cfg_load_fix_bin(qtk_nnet3_cfg_t *cfg,wtk_source_t *src)
{
        int ret = -1;
        int i,t;
        //float scale;
        int com_cnt, ti;
        int j;
        unsigned int row, col;
        qtk_affine_global_component_t* ga_com;
        qtk_rectified_linear_component_t* rl_com;
        qtk_normalize_component_t* n_com;
        qtk_activate_component_t* ls_com;
        //qtk_element_wise_product_component_t* wise_com;
        //qtk_dropout_component_t* drop_com;
        qtk_natural_gradient_per_element_scale_component_t* scale_com;
        qtk_backprop_truncation_component_t* backprop_com;
        qtk_batch_norm_component_t* batchnorm_com;
        qtk_lstm_nolinearity_component_t* lstm_com;
        qtk_timeheight_convolution_component_t* conv_com;
        qtk_scale_offset_component_t* scaleoff_com;
        nnet3_component_t* com;
        qtk_pnorm_component_t* pnorm_com;
        qtk_compactfsmn_component_t *FsmnCom;

        if(cfg->use_kwdec)
        {
                cfg->wt_mdl = (qtk_wakeup_trans_model_cfg_t*) wtk_malloc(sizeof(qtk_wakeup_trans_model_cfg_t));
                ret=qtk_nnet3_wakeup_trans_model_load_chain_fix_bin(cfg->wt_mdl, src);
        //      wtk_debug("load mdl ret =%d\n",ret);
                if(ret!=0){goto end;}
        }else
        {
            if (!cfg->use_fix_wake) {
                cfg->t_mdl = (qtk_trans_model_cfg_t*) wtk_malloc(sizeof(qtk_trans_model_cfg_t));
                if (cfg->normal)
                    ret = qtk_nnet3_trans_model_load_chain2_fix_bin(cfg->t_mdl,
                                                                    src);
                else
                    ret = qtk_nnet3_trans_model_load_chain_fix_bin(cfg->t_mdl,
                                                                   src);
                if(ret!=0){goto end;}
            }
        }
        cfg->is_fixed = 1;
        ret = wtk_source_fill(src,(char*)&com_cnt,4);
        if(ret!=0){goto end;}
        cfg->comp_cnt = com_cnt;
        //wtk_debug("load_fix_bin com_cnt=%d\n",com_cnt);
        for(i=0;i<com_cnt;++i)
        {
                cfg->comp_q[i] = (nnet3_component_t*) wtk_calloc(1, sizeof(nnet3_component_t));
                com = cfg->comp_q[i];
                ret=wtk_source_fill(src,(char*)&t,4);
                if(ret!=0){goto end;}
                com->type=t;
                //wtk_debug("i=%d,t=%d\n",i,t);
                switch(t)
        {
            case QTK_AffineGlobalComponent:
                                ga_com=(qtk_affine_global_component_t*)wtk_malloc(sizeof(qtk_affine_global_component_t));
                                ret=qtk_nnet3_affine_global_component_source_fill2(ga_com, src, cfg->fixed_nbytes);
                                if(ret!=0){goto end;}
				if(ga_com->type == QTK_NaturalGradientAffineComponent)
				{
					if(ga_com->nga_com)
					{
						ga_com->nga_com->in=NULL;
						ga_com->nga_com->out=NULL;
					}	
				}
				if(ga_com->type == QTK_LinearComponent)
				{
					if(ga_com->linear_com)
                                        {
                                                ga_com->linear_com->in=NULL;
                                                ga_com->linear_com->out=NULL;
                                        }

				}
                                com->component=ga_com;
            break;
            case QTK_NormalizeComponent:
                                n_com = (qtk_normalize_component_t*) wtk_malloc(sizeof(qtk_normalize_component_t));
                                ret=qtk_nnet3_normalize_component_source_fill(n_com,src);
                                if(ret!=0){goto end;}
                                com->component = n_com;
            break;
            case QTK_BatchNormComponent:
                                batchnorm_com = (qtk_batch_norm_component_t*)wtk_malloc(sizeof(qtk_batch_norm_component_t));
                                ret=qtk_nnet3_batch_norm_component_source_fill(batchnorm_com,src);
                                if(ret!=0){goto end;}
                                com->component=batchnorm_com;
				batchnorm_com->stats_sum =NULL;
				batchnorm_com->stats_sumsq = NULL;
            break;
            case QTK_RectifiedLinearComponent:
                                rl_com = (qtk_rectified_linear_component_t*) wtk_malloc(sizeof(qtk_rectified_linear_component_t));
                                com->component = rl_com;
                                rl_com->value_sum = NULL;
                                rl_com->deriv_sum =NULL;
                                rl_com->oderiv_sum =NULL;
            break;
            case QTK_ActivateComponent:
                                ls_com = (qtk_activate_component_t*)wtk_malloc(sizeof(qtk_activate_component_t));
                                ret=qtk_nnet3_activate_component_source_fill(ls_com,src);
                                if(ret!=0){goto end;}
                                com->component=ls_com;
                                ls_com->value_sum = NULL;
                                ls_com->deriv_sum =NULL;
                                ls_com->oderiv_sum =NULL;

            break;
            case QTK_NaturalGradientPerElementScaleComponent:
                                scale_com = (qtk_natural_gradient_per_element_scale_component_t*)wtk_malloc(sizeof(qtk_natural_gradient_per_element_scale_component_t));
                                //ret=qtk_nnet3_activate_component_source_fill(scale_com,src);
                               // if(ret!=0){goto end;}TODO
                                com->component=scale_com;
            break;
            case QTK_DropoutComponent:

            break;
            case QTK_ElementwiseProductComponent:
            break;
            case QTK_BackpropTruncationComponent:
				backprop_com = (qtk_backprop_truncation_component_t*)wtk_malloc(sizeof(qtk_backprop_truncation_component_t));
                                ret = qtk_nnet3_backpro_tru_component_source_fill(backprop_com,src);
                                if(ret!=0){goto end;}
                                com->component=backprop_com;

            break;
            case QTK_NoopComponent:
            break;
            case QTK_LstmNonlinearityComponent:
                                lstm_com = (qtk_lstm_nolinearity_component_t*)wtk_malloc(sizeof(qtk_lstm_nolinearity_component_t));
                                ret=qtk_nnet3_lstm_component_source_fill(lstm_com,src);
                                if(ret!=0){goto end;}
                                com->component=lstm_com;
            break;
            case QTK_TimeHeightConvolutionComponent:
                                conv_com = (qtk_timeheight_convolution_component_t*)wtk_malloc(sizeof(qtk_timeheight_convolution_component_t));
                                ret=qtk_nnet3_conv_component_source_fill(conv_com,src);
                                if(ret!=0){goto end;}
                                com->component=conv_com;
            break;
            case QTK_ScaleAndOffsetComponent:
                                scaleoff_com = (qtk_scale_offset_component_t*)wtk_malloc(sizeof(qtk_scale_offset_component_t));
                                ret=qtk_nnet3_scale_offset_component_source_fill(scaleoff_com,src);
                                if(ret!=0){goto end;}
                                com->component=scaleoff_com;
            break;
            case QTK_PnormComponent:
            	pnorm_com = (qtk_pnorm_component_t*)wtk_malloc(sizeof(qtk_pnorm_component_t));
            	ret=wtk_source_fill(src,(char*)(&ti),4);
            	if(ret!=0){goto end;}
            	pnorm_com->input_dim=ti;
            	ret=wtk_source_fill(src,(char*)(&ti),4);
            	if(ret!=0){goto end;}
            	pnorm_com->output_dim=ti;
            	com->component=pnorm_com;
            break;
            case QTK_CompactFsmnComponent:
                FsmnCom = (qtk_compactfsmn_component_t *)wtk_malloc(
                    sizeof(qtk_compactfsmn_component_t));
                ret = wtk_source_fill(src, (char *)(&ti), sizeof(int));
                if (ret != 0) {
                    goto end;
                }
                FsmnCom->l_order = ti;
                ret = wtk_source_fill(src, (char *)(&ti), sizeof(int));
                if (ret != 0) {
                    goto end;
                }
                FsmnCom->r_order = ti;
                ret = wtk_source_fill(src, (char *)(&ti), sizeof(int));
                if (ret != 0) {
                    goto end;
                }
                FsmnCom->l_stride = ti;
                ret = wtk_source_fill(src, (char *)(&ti), sizeof(int));
                if (ret != 0) {
                    goto end;
                }
                FsmnCom->r_stride = ti;
                ret =
                    wtk_source_fill(src, (char *)(&row), sizeof(unsigned int));
                if (ret != 0) {
                    goto end;
                }
                ret =
                    wtk_source_fill(src, (char *)(&col), sizeof(unsigned int));
                if (ret != 0) {
                    goto end;
                }
                FsmnCom->params = qtk_blas_matrix_new(row, col);
                FsmnCom->params->row = row;
                FsmnCom->params->col = col;
                ret = wtk_source_fill(src, (char *)(FsmnCom->params->m),
                                      row * col * sizeof(float));
                if (ret != 0) {
                    goto end;
                }
                FsmnCom->time_offset = (int *)wtk_malloc(
                    sizeof(int) * (2 + FsmnCom->r_order + FsmnCom->l_order));
                FsmnCom->time_offset[0] =
                    1 + FsmnCom->r_order + FsmnCom->l_order;
                FsmnCom->time_offset[1] = 0;
                for (j = 1; j <= FsmnCom->l_order; j++) {
                    FsmnCom->time_offset[1 + j] = -(j * FsmnCom->l_stride);
                }
                for (j = 1; j <= FsmnCom->r_order; j++) {
                    FsmnCom->time_offset[1 + FsmnCom->l_order + j] =
                        j * FsmnCom->r_stride;
                }
                FsmnCom->in =
                    (qtk_sub_matrix_t *)wtk_malloc(sizeof(qtk_sub_matrix_t));
                FsmnCom->out =
                    (qtk_sub_matrix_t *)wtk_malloc(sizeof(qtk_sub_matrix_t));
                FsmnCom->in_part =
                    (qtk_sub_matrix_t *)wtk_malloc(sizeof(qtk_sub_matrix_t));
                FsmnCom->out_part =
                    (qtk_sub_matrix_t *)wtk_malloc(sizeof(qtk_sub_matrix_t));
                com->component = FsmnCom;
                break;
            default:
            break;
        }

        }
        ret = wtk_source_fill(src, (char *)(&cfg->left_context), sizeof(int));
        if (ret != 0) {
            goto end;
        }
        ret = wtk_source_fill(src, (char *)(&cfg->right_context), sizeof(int));
        if (ret != 0) {
            goto end;
        }
        int isprior, val;
        ret=wtk_source_fill(src,(char*)(&isprior),4);
        if(ret!=0){goto end;}
        if (isprior){
            ret=wtk_source_fill(src,(char*)(&val),4);
            if(ret!=0){goto end;}
    		cfg->prior=qtk_blas_matrix_new(1, val);
    		ret = wtk_source_read_float_little(src, cfg->prior->m, val, 1);
        }

end:
        return ret;
}

int qtk_nnet3_read_xvector_vec(qtk_nnet3_cfg_t *cfg,wtk_source_t *src)
{
    wtk_strbuf_t *buf;
    int ret;

    buf=wtk_strbuf_new(1024,1.0f);

    cfg->xvector=wtk_knn_read_vector(src,buf);
#ifdef USE_NEON
    wtk_vecf_t *m;
    m=wtk_neon_math_mat_transf_8float(cfg->xvector);
    wtk_vecf_delete(cfg->xvector);
    cfg->xvector=m;
#endif
    if(!cfg->xvector)
    {
        wtk_debug("read xvector vec failed\n");
		ret=-1;goto end;
    }

    ret=0;
end:
    wtk_strbuf_delete(buf);
    return ret;
}

int qtk_nnet3_cfg_update(qtk_nnet3_cfg_t *cfg) {
    return qtk_nnet3_cfg_update2(cfg, NULL);
}

int qtk_nnet3_cfg_update2(qtk_nnet3_cfg_t *cfg, wtk_source_loader_t *sl) {
    int ret = 0;
    wtk_source_loader_t sl2;

    if (cfg->use_custom_acc) {
#if !(defined(_WIN32) || defined(HEXAGON))
//#ifndef _WIN32
        wtk_cpuinfo_t *cpuinfo;
        int curcpuid = 0;
        cpuinfo = wtk_cpuinfo_new();
        curcpuid = sched_getcpu();
        ret = wtk_cpuinfo_issimd(cpuinfo, curcpuid);
        if (ret > 0) {
#ifdef __ANDROID__
			cfg->use_custom_acc =  WTK_CUSTOM_NEON;
			cfg->porder = 2;
#else
			if (2==ret)
			{
				cfg->use_custom_acc =  WTK_CUSTOM_AVX2;
				cfg->porder = 3;
			}
			else
			{
				cfg->use_custom_acc =  WTK_CUSTOM_AVX;
				cfg->porder = 2;
			}
#endif
		}
		wtk_cpuinfo_delete(cpuinfo);
		#endif
	}

        if (!sl) {
            sl2.hook = 0;
	    sl2.vf=wtk_source_load_file_v;
		sl=&sl2;
        }
        if(cfg->use_fix_res)
    {
        ret = wtk_source_loader_load(sl, cfg,
                (wtk_source_load_handler_t) qtk_nnet3_cfg_load_fix_bin, cfg->bin_fn);
    }else
    {
        ret = wtk_source_loader_load(sl, cfg,
                (wtk_source_load_handler_t) qtk_tdnn_mdl_load, cfg->tdnn_fn);
    }
	if (ret!=0){goto end;}
    cfg->compution = qtk_nnet3_compution_new();

	ret = wtk_source_loader_load(sl, cfg->compution,
			(wtk_source_load_handler_t) qtk_nnet3_compution_load,
			cfg->compution_fn);

	if(cfg->extra_left > 0)
	{
    	cfg->compution2 = qtk_nnet3_compution_new();

		ret = wtk_source_loader_load(sl, cfg->compution2,
				(wtk_source_load_handler_t) qtk_nnet3_compution_load,
				cfg->compution_fn2);

	}

	if(cfg->xvector_fn)
	{
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)qtk_nnet3_read_xvector_vec,cfg->xvector_fn);
	}
end:
	return ret;
}
int qtk_nnet3_cfg_bytes(qtk_nnet3_cfg_t *cfg)
{
	return 0;
}

void qtk_nnet3_matrix_info_print(qtk_nnet3_submatrix_info_t *sub_info)
{
	wtk_debug("sub matrix: row:%d col:%d row_offset:%d col_offset:%d\n",
			sub_info->num_rows, sub_info->num_cols, sub_info->row_offset,
			sub_info->col_offset);
}

