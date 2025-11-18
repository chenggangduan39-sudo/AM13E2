#include "qtk_nnet3_compution.h"

qtk_nnet3_compution_t* qtk_nnet3_compution_new(void)
{
	qtk_nnet3_compution_t *compution;

	compution = (qtk_nnet3_compution_t*) wtk_malloc(
			sizeof(qtk_nnet3_compution_t));

	return compution;
}

void qtk_nnet3_compution_delete_backprop(qtk_nnet3_backprop_precomputed_indexes_t *index)
{
	if(index->zeroing)
	{
		qtk_blas_matrix_delete(index->zeroing);
	}
	wtk_free(index);
}

void qtk_nnet3_compution_delete_convolution(qtk_nnet3_convolution_precomputed_indexes_t *index)
{
	int i;

	for(i=0;i<index->num_steps;i++)
	{
		wtk_free(index->steps[i]->columns);
		if(index->steps[i]->backward_columns)
		{
			wtk_free(index->steps[i]->backward_columns);
		}
		wtk_free(index->steps[i]->height_map);
		wtk_free(index->steps[i]->input_part);
		wtk_free(index->steps[i]->params_part);
		wtk_free(index->steps[i]->output_reshaped);
		wtk_free(index->steps[i]->temp_mat_part);
		wtk_free(index->steps[i]->temp_mat_part_reshaped);
		wtk_free(index->steps[i]->input_reshaped);
		wtk_free(index->steps[i]->newtemp);
		wtk_free(index->steps[i]);
	}
	wtk_free(index->temp);
	wtk_free(index->temp_part);
	wtk_free(index->input_part);
	wtk_free(index->output_part);
	wtk_free(index->input_reshaped);
	wtk_free(index->steps);
	wtk_free(index);
}

void qtk_nnet3_compution_delete_gen(qtk_nnet3_generaldropout_precomputed_indexes_t *index)
{	
	//free(index->indexes);
	wtk_free(index);
}

void qtk_nnet3_compution_delete_fsmn(qtk_nnet3_fsmn_precomputed_indexes_t *index)
{
	//free(index->indexes);
	wtk_free(index->row_offsets);
	wtk_free(index);
}

void qtk_nnet3_compution_delete(qtk_nnet3_compution_t* compution)
{
	int i;

	for(i=0;i<compution->num[0];i++)
	{
		wtk_free(compution->command_q[i]);
	}
	for(i=0;i<compution->num[1];i++)
	{
		wtk_free(compution->matrix_info_q[i]);
	}
	for(i=0;i<compution->num[2];i++)
	{
		wtk_free(compution->submatrix_info_q[i]);
	}
	for(i=0;i<compution->num[3];i++)
	{
		if(compution->pre_computed[i])
		{
			switch(compution->pre_computed[i]->type)
			{
				case QTK_BackpropTruncationComponentPrecomputedIndexes:
					qtk_nnet3_compution_delete_backprop((qtk_nnet3_backprop_precomputed_indexes_t*)compution->pre_computed[i]->index);
					wtk_free(compution->pre_computed[i]);
					break;
				case QTK_TimeHeightConvolutionComponentPrecomputedIndexes:
					qtk_nnet3_compution_delete_convolution((qtk_nnet3_convolution_precomputed_indexes_t*)compution->pre_computed[i]->index);
					wtk_free(compution->pre_computed[i]);
					break;
				case QTK_GeneralDropoutComponentPrecomputedIndexes:
					qtk_nnet3_compution_delete_gen((qtk_nnet3_generaldropout_precomputed_indexes_t*)compution->pre_computed[i]->index);
					wtk_free(compution->pre_computed[i]);
					break;
				case QTK_FsmnComponentPrecomputedIndexes:
					qtk_nnet3_compution_delete_fsmn((qtk_nnet3_fsmn_precomputed_indexes_t*)compution->pre_computed[i]->index);
					wtk_free(compution->pre_computed[i]);
					break;
				default:
					break;
			}	
		}
	}
	for(i=0;i<compution->num[4];i++)
	{
		wtk_free(compution->indexes[i]);
	}
	for(i=0;i<compution->num[5];i++)
	{
		wtk_free(compution->indexes_multi[i]);
	}
	for(i=0;i<compution->num[6];i++)
	{
		wtk_free(compution->indexes_ranges[i]);
	}
	if(compution->command_q)
		wtk_free(compution->command_q);
	if (compution->matrix_info_q)
		wtk_free(compution->matrix_info_q);
	if (compution->submatrix_info_q)
		wtk_free(compution->submatrix_info_q);
	if (compution->pre_computed)
		wtk_free(compution->pre_computed);
	if (compution->indexes)
		wtk_free(compution->indexes);
	if (compution->indexes_multi)
		wtk_free(compution->indexes_multi);
	if (compution->indexes_ranges)
		wtk_free(compution->indexes_ranges);
	if (compution->input_indexes)
		wtk_free(compution->input_indexes);
	wtk_free(compution);
}

int qtk_nnet3_compution_load_matrix_info(qtk_nnet3_matrix_info_t** mat_info,
		int mat_num, wtk_source_t* src, wtk_strbuf_t* buf)
{
	int ret = 0, i, v;

	ret = wtk_source_read_string(src, buf);
	for (i = 0; i < mat_num; i++)
	{
		mat_info[i] = (qtk_nnet3_matrix_info_t*) wtk_malloc(
				sizeof(qtk_nnet3_matrix_info_t));
		ret = wtk_source_read_string(src, buf);
		if (!wtk_str_equal_s(buf->data, buf->pos, "<MatrixInfo>"))
		{
			ret = -1;
			goto end;
		}

		ret = wtk_source_read_string(src, buf);
		//ret=wtk_source_read_string(src,buf);
		if (!wtk_str_equal_s(buf->data, buf->pos, "<NumRows>"))
		{
			ret = -1;
			goto end;
		}
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		mat_info[i]->num_rows = v;

		ret = wtk_source_read_string(src, buf);
		if (!wtk_str_equal_s(buf->data, buf->pos, "<NumCols>"))
		{
			ret = -1;
			goto end;
		}
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		mat_info[i]->num_cols = v;
		//wtk_debug("%d %d %d\n",i,mat_info[i]->num_rows,mat_info[i]->num_cols);
		ret = wtk_source_read_string(src, buf);
		if (wtk_str_equal_s(buf->data, buf->pos, "</MatrixInfo>"))
		{
			mat_info[i]->stride_type = 0;

		} else if (wtk_str_equal_s(buf->data, buf->pos, "<StrideEqualNumCols>"))
		{
			mat_info[i]->stride_type = 1;
			ret = wtk_source_read_string(src, buf);
		}
	}

	end: return ret;
}

int qtk_nnet3_compution_load_submatrix_info(
		qtk_nnet3_submatrix_info_t** mat_info, int mat_num, wtk_source_t* src,
		wtk_strbuf_t* buf)
{
	int ret = 0, i, v;

	for (i = 0; i < mat_num; i++)
	{
		mat_info[i] = (qtk_nnet3_submatrix_info_t*) wtk_malloc(
				sizeof(qtk_nnet3_submatrix_info_t));
		ret = wtk_source_read_string(src, buf);
		if (wtk_str_equal_s(buf->data, buf->pos, "<SubMatrixInfo>"))
		{
			ret = wtk_source_read_string(src, buf);
		}

		if (!wtk_str_equal_s(buf->data, buf->pos, "<MatrixIndex>"))
		{
			ret = -1;
			goto end;
		}
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		mat_info[i]->matrix_index = v;

		ret = wtk_source_read_string(src, buf);
		if (!wtk_str_equal_s(buf->data, buf->pos, "<RowOffset>"))
		{
			ret = -1;
			goto end;
		}
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		mat_info[i]->row_offset = v;

		ret = wtk_source_read_string(src, buf);
		if (!wtk_str_equal_s(buf->data, buf->pos, "<NumRows>"))
		{
			ret = -1;
			goto end;
		}
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		mat_info[i]->num_rows = v;

		ret = wtk_source_read_string(src, buf);
		if (!wtk_str_equal_s(buf->data, buf->pos, "<ColOffset>"))
		{
			ret = -1;
			goto end;
		}
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		mat_info[i]->col_offset = v;

		ret = wtk_source_read_string(src, buf);
		if (!wtk_str_equal_s(buf->data, buf->pos, "<NumCols>"))
		{
			ret = -1;
			goto end;
		}
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		mat_info[i]->num_cols = v;

		ret = wtk_source_read_string(src, buf);
		if (!wtk_str_equal_s(buf->data, buf->pos, "</SubMatrixInfo>"))
		{
			ret = -1;
			goto end;
		}
	}

	end: return ret;
}

int qtk_nnet3_compution_reverse_column_mapping(int *columns, int input_dim,
		int **backward_columns)
{
	int columns_dim = columns[0];
	int **temp = (int**) wtk_calloc(input_dim, sizeof(int*));
	int i, j, k, cnt, max_overlap = 0;

	for (i = 0; i < input_dim; i++)
	{
		temp[i] = (int*) wtk_malloc(sizeof(int) * (columns_dim + 1));
		temp[i][0] = 0;
	}
	cnt = 1;
	for (i = 0; i < columns_dim; i++)
	{
		j = columns[i + 1];
		if (j != -1) {
			cnt = temp[j][0];
			temp[j][cnt + 1] = i;
			temp[j][0]++;
		}
	}

	for (j = 0; j < input_dim; j++)
	{
		max_overlap = ((max_overlap > temp[j][0]) ? max_overlap : temp[j][0]);
	}
	
	if(max_overlap >0)
	{
		backward_columns = (int**) wtk_calloc(max_overlap, sizeof(int*));
		for (i = 0; i < max_overlap; i++)
		{
			backward_columns[i] = (int*) wtk_malloc(sizeof(int) * input_dim);
		}

		for (j = 0; j < input_dim; j++)
		{
			for (k = 0; k < temp[j][0]; k++)
			{
				i = temp[j][k + 1];
				backward_columns[k][j] = i;
			}
		}
	}
	for (i = 0; i < input_dim; i++)
	{
		wtk_free(temp[i]);
	}
	wtk_free(temp);
	//TODO delete temp
	return max_overlap;
}

int qtk_nnet3_vector_is_contiguous(int *vec)
{
	int s = vec[0];
	int i;

	for (i = 1; i + 1 < s; i++)
	{
		if (vec[i + 1] != vec[i] + 1)
		{
			return 0;
		}
	}
	return 1;
}

int qtk_nnet3_compution_load_convolution(
		qtk_nnet3_convolution_precomputed_indexes_t* convolution,
		wtk_source_t* src, wtk_strbuf_t* buf)
{
	int ret = 0, v, num, col, i;
	qtk_nnet3_convolution_step_t *step;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Computation>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<ConvComputation>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumFiltersInOut>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	convolution->num_filters_in = v;
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	convolution->num_filters_out = v;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<HeightInOut>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	convolution->height_in = v;
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	convolution->height_out = v;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumTInOut>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	convolution->num_t_in = v;
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	convolution->num_t_out = v;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumImages>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	convolution->num_images = v;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<TempRowsCols>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	convolution->temp_rows = v;
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	convolution->temp_cols = v;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumSteps>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	convolution->steps = (qtk_nnet3_convolution_step_t**) wtk_calloc(v,
			sizeof(qtk_nnet3_convolution_step_t*));
	convolution->num_steps = v;
	num = v;

	for (i = 0; i < num; i++)
	{
		step = (qtk_nnet3_convolution_step_t*) wtk_malloc(
				sizeof(qtk_nnet3_convolution_step_t));
		convolution->steps[i] = step;
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		if (!wtk_str_equal_s(buf->data, buf->pos, "<TimeShift>"))
		{
			ret = -1;
			goto end;
		}
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		step->input_time_shift = v;

		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		if (!wtk_str_equal_s(buf->data, buf->pos, "<ParamsStartCol>"))
		{
			ret = -1;
			goto end;
		}
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &v, 1, 1);
		step->params_start_col = v;

		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		if (!wtk_str_equal_s(buf->data, buf->pos, "<HeightMap>"))
		{
			ret = -1;
			goto end;
		}
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &col, 1, 1);
		int *height;
		height = (int*) wtk_malloc(sizeof(int) * (col + 1)); //height[0] for vector size
		height[0] = col;
		step->height_map = height;
		//ret=wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, step->height_map + 1, col, 1);
		//ret=wtk_source_read_char(src);//TODO

		int *columns = (int*) wtk_malloc(sizeof(int) * (col * convolution->num_filters_in + 1));
		columns[0] = col * convolution->num_filters_in;
		//compute derived
		int h, fc;
		//input_dim = convolution->height_in * convolution->num_filters_in;
		for (h = 1; h < col + 1; h++)
		{
			if (height[h] != -1)
			{
				for (fc = 1; fc < convolution->num_filters_in + 1; fc++)
				{
					columns[(h - 1) * convolution->num_filters_in + fc] = height[h] * convolution->num_filters_in + fc - 1;
				}
			} else
			{
				for (fc = 1; fc < convolution->num_filters_in + 1; fc++)
				{
					columns[(h - 1) * convolution->num_filters_in + fc] = -1;
				}
			}
		}
		step->columns = (int*) wtk_malloc(
				sizeof(int) * (col * convolution->num_filters_in + 1));
		step->columns[0] = col * convolution->num_filters_in;
		memcpy(step->columns + 1, columns + 1, col * convolution->num_filters_in * sizeof(int));
		step->backward_columns=NULL;

		//step->max_overlap = qtk_nnet3_compution_reverse_column_mapping(columns, input_dim, step->backward_columns);

		if (step->height_map[1] != -1 && qtk_nnet3_vector_is_contiguous(step->height_map))
		{
			step->columns_are_contiguous = 1;
		} else
		{
			step->columns_are_contiguous = 0;
		}
		step->first_column = columns[1];
		step->input_part = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
		step->params_part = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
		step->output_reshaped = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
		step->temp_mat_part = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
		step->temp_mat_part_reshaped = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
		step->input_reshaped = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
		step->newtemp = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));

		wtk_free(columns);
	}
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "</ConvComputation>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos,
			"</TimeHeightConvolutionComponentPrecomputedIndexes>"))
	{
		ret = -1;
		goto end;
	}
	end:
	convolution->temp = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
	convolution->temp_part = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
	convolution->input_part = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
	convolution->output_part = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
	convolution->input_reshaped = (qtk_sub_matrix_t*)wtk_malloc(sizeof(qtk_sub_matrix_t));
	return ret;
}

int qtk_nnet3_compution_load_backprop(qtk_nnet3_backprop_precomputed_indexes_t* backprop, wtk_source_t* src, wtk_strbuf_t* buf)
{
	int ret = 0, col;
	float f;
	qtk_blas_matrix_t *m;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Zeroing>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	m = qtk_blas_matrix_new(1, col);
	ret = wtk_source_read_float_little(src, m->m, 1 * col, 1);
	backprop->zeroing = m;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<ZeroingSum>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_float_little(src, &f, 1, 1);
	backprop->zeroing_sum = f;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos,
			"</BackpropTruncationComponentPrecomputedIndexes>"))
	{
		ret = -1;
		goto end;
	}

	end: return ret;
}

int qtk_nnet3_compution_load_generaldrop(qtk_nnet3_generaldropout_precomputed_indexes_t* gen, wtk_source_t* src, wtk_strbuf_t* buf)
{
	int ret = 0, col;
	//float f;
	//qtk_blas_matrix_t *m;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumMaskRows>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	gen->num_mask_rows = col;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Indexes>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	gen->indexes = (int*)wtk_malloc(sizeof(int)*col);
	gen->indexes[0] = col;
	ret = wtk_source_read_int_little(src, gen->indexes+1, col, 1);

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos,
			"</GeneralDropoutComponentPrecomputedIndexes>"))
	{
		ret = -1;
		goto end;
	}

	end: return ret;
}

int qtk_nnet3_compution_load_fsmn(qtk_nnet3_fsmn_precomputed_indexes_t* fsmn, wtk_source_t* src, wtk_strbuf_t* buf)
{
	int ret = 0, col;
	//float f;
	//qtk_blas_matrix_t *m;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<RowStride>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	fsmn->row_stride = col;

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<RowOffsets>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	fsmn->row_offsets = (int*)wtk_malloc(sizeof(int)*(col+1));
	fsmn->row_offsets[0] = col;
	ret = wtk_source_read_int_little(src, fsmn->row_offsets+1, col, 1);

	ret = wtk_source_read_string(src, buf);
	if (!wtk_str_equal_s(buf->data, buf->pos,
			"</CompactFsmnComponentPrecomputedIndexes>"))
	{
		ret = -1;
		goto end;
	}
	end: return ret;
}

int qtk_nnet3_compution_load_precomputed_index(
		qtk_nnet3_precomputed_indexes_t** index, int num, wtk_source_t* src,
		wtk_strbuf_t* buf)
{
	int ret = 0, i;
	qtk_nnet3_convolution_precomputed_indexes_t *convolution;
	qtk_nnet3_backprop_precomputed_indexes_t *backprop;
	qtk_nnet3_generaldropout_precomputed_indexes_t *gendrop;
	qtk_nnet3_fsmn_precomputed_indexes_t *fsmn;

	for (i = 0; i < num - 1; i++) //TODO num is 85?
	{
		index[i] = NULL;
		ret = wtk_source_read_string(src, buf);
		//wtk_debug("[%.*s] %d %d\n",buf->pos,buf->data,num,i);
		if (wtk_str_equal_s(buf->data, buf->pos,
				"<TimeHeightConvolutionComponentPrecomputedIndexes>"))
		{
	        index[i]=(qtk_nnet3_precomputed_indexes_t*)wtk_malloc(sizeof(qtk_nnet3_precomputed_indexes_t));
			convolution = (qtk_nnet3_convolution_precomputed_indexes_t*) wtk_malloc(
							sizeof(qtk_nnet3_convolution_precomputed_indexes_t));
			index[i]->index = convolution;
			index[i]->type = QTK_TimeHeightConvolutionComponentPrecomputedIndexes;
			qtk_nnet3_compution_load_convolution(convolution, src, buf);
		} else if (wtk_str_equal_s(buf->data, buf->pos,
				"<BackpropTruncationComponentPrecomputedIndexes>"))
		{
	        index[i]=(qtk_nnet3_precomputed_indexes_t*)wtk_malloc(sizeof(qtk_nnet3_precomputed_indexes_t));
			backprop = (qtk_nnet3_backprop_precomputed_indexes_t*) wtk_malloc(
					sizeof(qtk_nnet3_backprop_precomputed_indexes_t));
			index[i]->index = backprop;
			index[i]->type = QTK_BackpropTruncationComponentPrecomputedIndexes;
			qtk_nnet3_compution_load_backprop(backprop, src, buf);
		}else if (wtk_str_equal_s(buf->data, buf->pos,
				"<GeneralDropoutComponentPrecomputedIndexes>"))
		{
	        index[i]=(qtk_nnet3_precomputed_indexes_t*)wtk_malloc(sizeof(qtk_nnet3_precomputed_indexes_t));
	        gendrop = (qtk_nnet3_generaldropout_precomputed_indexes_t*) wtk_malloc(
					sizeof(qtk_nnet3_generaldropout_precomputed_indexes_t));
			index[i]->index = gendrop;
			index[i]->type = QTK_GeneralDropoutComponentPrecomputedIndexes;
			qtk_nnet3_compution_load_generaldrop(gendrop, src, buf);
		}else if(wtk_str_equal_s(buf->data, buf->pos,
				"<CompactFsmnComponentPrecomputedIndexes>"))
		{
	        index[i]=(qtk_nnet3_precomputed_indexes_t*)wtk_malloc(sizeof(qtk_nnet3_precomputed_indexes_t));
	        fsmn = (qtk_nnet3_fsmn_precomputed_indexes_t*) wtk_malloc(
					sizeof(qtk_nnet3_fsmn_precomputed_indexes_t));
			index[i]->index = fsmn;
			index[i]->type = QTK_FsmnComponentPrecomputedIndexes;
			qtk_nnet3_compution_load_fsmn(fsmn, src, buf);

		}else
		{
			wtk_debug("not support Component PrecomputedIndexes\n");
			exit(0);
		}
	}

	return ret;
}

int qtk_nnet3_compution_load_cmd(qtk_nnet3_command_t* cmd, wtk_source_t* src,
		wtk_strbuf_t* buf, int *ac)
{
	int ret = 0, col;
	float f;

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Cmd>"))
	{
		ret = -1;
		goto end;
	}

	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &col, 1, 1);
	//wtk_debug("cmd %d\n",col);
	cmd->type = col;
	if(cmd->type == kAcceptInput)
	{
		*ac+=1;
	}
	
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_float_little(src, &f, 1, 1);
	//wtk_debug("cmd %f\n",f);
	cmd->alpha = f;

	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &(cmd->arg1), 1, 1);
	//wtk_debug("cmd %d\n",cmd->arg1);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &(cmd->arg2), 1, 1);
	//wtk_debug("cmd %d\n",cmd->arg2);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &(cmd->arg3), 1, 1);
	//wtk_debug("cmd %d\n",cmd->arg3);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &(cmd->arg4), 1, 1);
	//wtk_debug("cmd %d\n",cmd->arg4);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &(cmd->arg5), 1, 1);
	//wtk_debug("cmd %d\n",cmd->arg5);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &(cmd->arg6), 1, 1);
	//wtk_debug("cmd %d\n",cmd->arg6);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &(cmd->arg7), 1, 1);
	//wtk_debug("cmd %d\n",cmd->arg7);

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "</Cmd>"))
	{
		ret = -1;
		goto end;
	}
	end: return ret;
}

void qtk_nnet3_compution_calc_input_index(qtk_nnet3_compution_t *compution,int ac)
{
	int i,k,sub;

	compution->input_indexes = (int*) wtk_calloc(ac + 1, sizeof(int));
	compution->input_indexes[0] = ac;
	k=1;
	for(i = 0; i < compution->cmd_cnt; i++)
	{
		if(compution->command_q[i]->type == kAcceptInput)
		{
			sub = compution->command_q[i]->arg1;
			compution->input_indexes[k] = compution->submatrix_info_q[sub]->matrix_index;
			//wtk_debug("%d\n",compution->submatrix_info_q[sub]->matrix_index);
			k++;	
		}
	}
}

int qtk_nnet3_compution_load(qtk_nnet3_compution_t *compution,
		wtk_source_t* src)
{
	int ret = 0;
	//qtk_nnet3_compution_t* compution;
	wtk_strbuf_t *buf;
	int i, v, count, col;

	buf = wtk_strbuf_new(256, 1);
	//compution=qtk_nnet3_compution_new();

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NnetComputation>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Version>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumMatrices>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	//wtk_debug("%d\n",v);
	compution->matrix_info_q = (qtk_nnet3_matrix_info_t**) wtk_calloc(v,
			sizeof(qtk_nnet3_matrix_info_t*));
    compution->num[1]=v;
	qtk_nnet3_compution_load_matrix_info(compution->matrix_info_q, v, src, buf);

	/*
	 ret=wtk_source_read_string(src,buf);
	 if(!wtk_str_equal_s(buf->data,buf->pos,"<NumMatrixDebugInfo>"))
	 {
	 ret=-1;
	 goto end;
	 }
	 ret=wtk_source_read_char(src);
	 ret=wtk_source_read_int_little(src,&v,1,1);
	 ret=wtk_source_read_string(src,buf);
	 if(!wtk_str_equal_s(buf->data,buf->pos,"<MatrixDebugInfo>"))
	 {
	 ret=-1;
	 goto end;
	 }
	 qtk_nnet3_compution_load_matrix_debug_info(v,src,buf);*/
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumSubMatrices>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	//wtk_debug("%d\n",v);

	ret = wtk_source_read_string(src, buf);
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<SubMatrixInfo>"))
	{
		ret = -1;
		goto end;
	}
	compution->submatrix_info_q = (qtk_nnet3_submatrix_info_t**) wtk_calloc(v,
			sizeof(qtk_nnet3_submatrix_info_t*));
    compution->num[2]=v;
	qtk_nnet3_compution_load_submatrix_info(compution->submatrix_info_q, v, src, buf);

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumComponentPrecomputedIndexes>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &v, 1, 1);
	ret = wtk_source_read_string(src, buf);
	//ret=wtk_source_read_string(src,buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	//if(!wtk_str_equal_s(buf->data,buf->pos,"<PrecomputedIndexesInfo>"))
	//{
	//    ret=-1;
	//    goto end;
	//}
	compution->pre_computed = (qtk_nnet3_precomputed_indexes_t**) wtk_calloc(v,
			sizeof(qtk_nnet3_precomputed_indexes_t*));
    compution->num[3]=v;
	ret = qtk_nnet3_compution_load_precomputed_index(compution->pre_computed, v,
			src, buf);

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumIndexes>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &count, 1, 1);
	//wtk_debug("%d\n",count);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Indexes>"))
	{
		ret = -1;
		goto end;
	}
	compution->indexes = (int**) wtk_calloc(count, sizeof(int*));
    compution->num[4]=count;
	ret = wtk_source_read_char(src);
	//ret=wtk_source_read_char(src);
	for (i = 0; i < count; i++)
	{
		//ret=wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &col, 1, 1);
		//wtk_debug("%d\n",col);
		compution->indexes[i] = (int*) wtk_malloc(sizeof(int) * (col + 1)); //height[0] for vector size
		compution->indexes[i][0] = col;
		//ret=wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, compution->indexes[i] + 1, col,
				1);
		//ret=wtk_source_read_char(src);//TODO
	}

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumIndexesMulti>")) 
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &count, 1, 1);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s] %d\n",buf->pos,buf->data,count);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<IndexesMulti>")) 
	{
		ret = -1;
		goto end;
	}
	compution->indexes_multi = (int**) wtk_calloc(count, sizeof(int*));
    compution->num[5]=count;
	ret = wtk_source_read_char(src);
	for (i = 0; i < count; i++)
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &col, 1, 1);
		//wtk_debug("%d\n",col);
		compution->indexes_multi[i] = (int*) wtk_malloc(
				sizeof(int) * (col * 2 + 1));    //height[0] for vector size
		compution->indexes_multi[i][0] = col * 2;
		//ret=wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, compution->indexes_multi[i] + 1,
				col * 2, 1);
	}

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumIndexesRanges>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &count, 1, 1);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s] %d\n",buf->pos,buf->data,count);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<IndexesRanges>"))
	{
		ret = -1;
		goto end;
	}
	if (count>0)
		compution->indexes_ranges = (int**) wtk_calloc(count, sizeof(int*));
	else
		compution->indexes_ranges = NULL;
    compution->num[6]=count;
	for (i = 0; i < count; i++)
	{
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, &col, 1, 1);
		compution->indexes_ranges[i] = (int*) wtk_malloc(
				sizeof(int) * (col + 1));    //height[0] for vector size
		compution->indexes_ranges[i][0] = col;
		ret = wtk_source_read_char(src);
		ret = wtk_source_read_int_little(src, compution->indexes_ranges[i] + 1,
				col, 1);
		//ret=wtk_source_read_char(src);//TODO
	}

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NumCommands>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_int_little(src, &count, 1, 1);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s] %d\n",buf->pos,buf->data,count);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<Commands>"))
	{
		ret = -1;
		goto end;
	}
	compution->command_q = (qtk_nnet3_command_t**) wtk_calloc(count,
			sizeof(qtk_nnet3_command_t*));
	compution->num[0]=count;
	compution->cmd_cnt = count;
	int ac=0;
	for (i = 0; i < count; i++)
	{
		compution->command_q[i] = (qtk_nnet3_command_t*) wtk_malloc(sizeof(qtk_nnet3_command_t));
		ret = qtk_nnet3_compution_load_cmd(compution->command_q[i], src, buf,&ac);
	}
	
	qtk_nnet3_compution_calc_input_index(compution,ac);

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "<NeedModelDerivative>"))
	{
		ret = -1;
		goto end;
	}
	ret = wtk_source_read_char(src);
	ret = wtk_source_read_char(src);
	//ret=wtk_source_read_int_little(src,&count,1,1);

	ret = wtk_source_read_string(src, buf);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	if (!wtk_str_equal_s(buf->data, buf->pos, "</NnetComputation>"))
	{
		ret = -1;
		goto end;
	}

	end: wtk_strbuf_delete(buf);
	return ret;
}
