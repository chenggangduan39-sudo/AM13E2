#include "qtk_torchnn.h"
qtk_torchnn_t* qtk_torchnn_new(int n)
{
	qtk_torchnn_t *torchnn = (qtk_torchnn_t*)wtk_malloc(sizeof(qtk_torchnn_t));

	torchnn->nlayer = n;
	torchnn->layers = (qtk_torchnn_layer_t**) wtk_calloc(n,sizeof(qtk_torchnn_layer_t*));
	torchnn->notify = NULL;
	torchnn->notify_end =NULL;
	return torchnn;
}

qtk_torchnn_layer_t* qtk_torchnn_layer_new(void)
{
	qtk_torchnn_layer_t* layer = (qtk_torchnn_layer_t*)wtk_malloc(sizeof(qtk_torchnn_layer_t));
	layer->input = NULL;
	layer->output = NULL;
	layer->rb = NULL;
	layer->mdl = NULL;
	layer->skip = 0;
	layer->has_output = 0;

	return layer;
}

void qtk_torchnn_layer_delete(qtk_torchnn_layer_t* layer)
{
	wtk_robin_t *r = layer->rb;
	qtk_blas_matrix_t *m;

	if(layer->input)
	{
		qtk_blas_matrix_delete(layer->input);
	}
	if(layer->has_output)
	{
		qtk_blas_matrix_delete(layer->output);
	}
	if(r)
	{
		while(r->used>0)
		{
			m=wtk_robin_pop(r);
			if(!m){break;}
			qtk_blas_matrix_delete(m);
		}
		wtk_robin_delete(r);
	}
	wtk_free(layer->mdl);
	wtk_free(layer);
}

int qtk_torchnn_write(qtk_torchnn_t *mdl, FILE *f)
{
	int ret = 0,i;
	qtk_torchnn_layer_t *layer;
	int c;
	//FILE *f = fopen(fn,"wb");
	//layer cnt
    fwrite(&(mdl->nlayer),4,1,f);
	for(i = 0;i < mdl->nlayer; i++)
	{
		layer = mdl->layers[i];
		c = layer->type;
		fwrite(&c,4,1,f);
		switch(layer->type){
			case QTK_TORCHNN_CONV2D:
				qtk_torchnn_conv2d_write(layer->mdl,f);
				break;
			case QTK_TORCHNN_LSTM:
				qtk_torchnn_lstm_write(layer->mdl,f);		
				break;
			case QTK_TORCHNN_MAXPOOL:
				qtk_torchnn_maxpool_write(layer->mdl,f);
				break;
			case QTK_TORCHNN_AVPOOL:
				qtk_torchnn_avpool_write(layer->mdl,f);
				break;
			case QTK_TORCHNN_LINEAR:
				qtk_torchnn_linear_write(layer->mdl,f);
				break;
			case QTK_TORCHNN_BATCH:
				qtk_torchnn_batch_write(layer->mdl,f);
				break;
			case QTK_TORCHNN_CONV1D:
				qtk_torchnn_conv1d_write(layer->mdl,f);
				break;
			case QTK_TORCHNN_EMBED:
				qtk_torchnn_embed_write(layer->mdl,f);
				break;
			case QTK_TORCHNN_ACTIVATE:
				fwrite(&(layer->a_type),4,1,f);
				break;
			case QTK_TORCHNN_SE:
				qtk_torchnn_se_write(layer->mdl,f);
				break;
			case QTK_TORCHNN_BASIC:
				qtk_torchnn_block_write(layer->mdl,f);
				break;
			case QTK_TORCHNN_MEAN:
			default:
				break;
		}
	}
	
	return ret;
}

qtk_torchnn_t* qtk_torchnn_read_bin(wtk_source_t *src,wtk_strbuf_t *buf,int bin,int feat_col,int feat_row)
{
	int ret, v, i, n, row = -1, col = -1,type;
	int last_row, last_col, dilation;
	qtk_torchnn_t *torchnn;
	qtk_torchnn_layer_t *layer;
	qtk_torchnn_lstm_t* lstm;
	qtk_torchnn_conv2d_t* conv2d;
	wtk_robin_t *robin;
	
	ret = wtk_source_read_int_little(src, &v, 1, bin);
	//v = 23;
	torchnn = qtk_torchnn_new(v);
	last_col = feat_col;
	last_row = feat_row;
	for(i = 0;i < v; i++)
	{
		ret = wtk_source_read_int_little(src, &type, 1, bin);
		layer = qtk_torchnn_layer_new();
		torchnn->layers[i] = layer;
		if(type == 0)
		{
			layer->type = QTK_TORCHNN_CONV2D;
			conv2d = qtk_torchnn_conv2d_read_bin(src,buf,bin);
			layer->mdl = conv2d;
			layer->input_row = last_row;
			layer->input_col = last_col;

			n = conv2d->kernel_row;
			dilation = conv2d->dilation1;
			//wtk_debug("dilation:%d %d\n",((qtk_torchnn_conv2d_t*)layer->mdl)->dilation1,n);
			if(dilation > 1)
			{
				n += (dilation-1)*(n-1);
			}
			//wtk_debug("dilation:%d %d\n",((qtk_torchnn_conv2d_t*)layer->mdl)->dilation1,n);
			robin = wtk_robin_new(n);

			row = conv2d->kernel_row * last_row;
			col = last_col + conv2d->padding_col*2;
			//wtk_debug("111111 %d\n",((qtk_torchnn_conv2d_t*)layer->mdl)->padding_col);
			//wtk_debug("%d %d\n",row,col);
			layer->input = qtk_blas_matrix_new(row,col);
			//wtk_debug("%d %d %d\n",row,col,conv2d->stride1);
			row = conv2d->weight->row;
			col = last_col/conv2d->stride2;
			if(conv2d->stride2 > 1)
			{
				col = (last_col+1)/conv2d->stride2;
			}
			//wtk_debug("%d %d %d\n",last_col,conv2d->stride2,(last_col+1)/conv2d->stride2);
			//wtk_debug("%d %d %d\n",row,col,conv2d->stride1);
			layer->output = qtk_blas_matrix_new(row,col);
			layer->has_output = 1;
			qtk_blas_matrix_zero(layer->input);
			qtk_blas_matrix_zero(layer->output);

			last_row = row;
			last_col = col;

			layer->skip = ((qtk_torchnn_conv2d_t*)layer->mdl)->stride1 - 1;
			layer->skip_cnt = 0;
			layer->rb = robin;

			col = conv2d->kernel_col*conv2d->kernel_row*layer->input_row;//9 3*3*1
			row = layer->input_col/conv2d->stride2;//40 (42-2)*(3-2)
			if(conv2d->stride2 > 1)
			{
				row = (layer->input_col+1)/conv2d->stride2;
			}
			conv2d->trans = qtk_blas_matrix_new(row,col);

		} else if(type == 2)
		{
			layer->type = QTK_TORCHNN_LSTM;
			lstm = (qtk_torchnn_lstm_t*)qtk_torchnn_lstm_read(src,buf,bin);
			layer->mdl = lstm;

			layer->lstma = (qtk_torchnn_lstm_assit_t*)wtk_malloc(sizeof(qtk_torchnn_lstm_assit_t));
			//wtk_debug("%d\n",lstm->ih_w->col);
			layer->input =  qtk_blas_matrix_new(1,lstm->ih_w->col);
			layer->lstma->h1 = qtk_blas_matrix_new(1,lstm->hh_w->col);
			layer->output = qtk_blas_matrix_new(1,lstm->hh_w->col);
			layer->has_output = 1;
			layer->lstma->h2 = layer->output;
			layer->lstma->c1 = qtk_blas_matrix_new(layer->lstma->h1->row,lstm->hh_w->row/4);
			layer->lstma->c2 = qtk_blas_matrix_new(layer->lstma->h1->row,lstm->hh_w->row/4);

			qtk_blas_matrix_zero(layer->output);
			qtk_blas_matrix_zero(layer->lstma->h1);
			qtk_blas_matrix_zero(layer->lstma->c1);
			qtk_blas_matrix_zero(layer->lstma->c2);

			last_row = layer->output->row;
			last_col = layer->output->col;

		}else if(type == 1)
		{
			layer->type = QTK_TORCHNN_MAXPOOL;
			layer->mdl = (qtk_torchnn_maxpool_t*)qtk_torchnn_maxpool_read(src,buf,bin);

			row = ((qtk_torchnn_maxpool_t*)layer->mdl)->kernel_size * last_row;
			col = last_col;
			layer->input = qtk_blas_matrix_new(row,col);

			row = last_row;
			col = last_col/ ((qtk_torchnn_maxpool_t*)layer->mdl)->stride;
			layer->output = qtk_blas_matrix_new(row,col);
			layer->has_output = 1;
			qtk_blas_matrix_zero(layer->input);
			qtk_blas_matrix_zero(layer->output);

			robin = wtk_robin_new(((qtk_torchnn_maxpool_t*)layer->mdl)->kernel_size);
			layer->rb = robin;

			last_row = row;
			last_col = col;
		}else if(type == 8)
		{
			layer->type = QTK_TORCHNN_AVPOOL;
			layer->mdl = (qtk_torchnn_avpool_t*)qtk_torchnn_avpool_read(src,buf,bin);

			if(((qtk_torchnn_avpool_t*)layer->mdl)->kernel_row <= 1)
			{
				layer->input = NULL;
				row = last_row;
				col = last_col/((qtk_torchnn_avpool_t*)layer->mdl)->kernel_col;
				//wtk_debug("%d %d\n",row,col);
				layer->output = qtk_blas_matrix_new(row,col);
				layer->has_output = 1;
			}else
			{//TODO need robin
//				row = ((qtk_torchnn_avpool_t*)layer->mdl)->kernel_size * last_row;
//				col = last_col;
//				layer->input = qtk_blas_matrix_new(row,col);
//
//				row = last_row;
//				col = last_col/ ((qtk_torchnn_avpool_t*)layer->mdl)->stride;
//				layer->output = qtk_blas_matrix_new(row,col);
//				qtk_blas_matrix_zero(layer->input);
//				qtk_blas_matrix_zero(layer->output);
//
//				robin = wtk_robin_new(((qtk_torchnn_avpool_t*)layer->mdl)->kernel_size);
//				layer->rb = robin;
			}
			last_row = row;
			last_col = col;
		}else if(type == 9)
		{
			layer->type = QTK_TORCHNN_MEAN;
			//layer->mdl = (qtk_torchnn_avpool_t*)qtk_torchnn_avpool_read(src,buf,bin);

			layer->input = NULL;
			row = 1;
			col = last_row;
			//wtk_debug("mean %d %d\n",row,col);
			layer->output = qtk_blas_matrix_new(row,col);
			layer->has_output = 1;

			last_row = row;
			last_col = col;
		}else if(type == 3)
		{
			layer->type = QTK_TORCHNN_LINEAR;
			layer->mdl = (qtk_torchnn_linear_t*)qtk_torchnn_linear_read_bin(src,buf,bin);
			if (i!=0)
			{
				layer->input = NULL;//for linear use last layer output
			}else
			{
				row = last_row;
				col = last_col;
				layer->input = qtk_blas_matrix_new(row,col);
			}
			row = last_row;
			col = ((qtk_torchnn_linear_t*)layer->mdl)->weight->row;
			if(last_row * last_col == ((qtk_torchnn_linear_t*)layer->mdl)->weight->col)
			{
				row = 1;
				//col = ((qtk_torchnn_linear_t*)layer->mdl)->weight->col;
			}

			layer->output = qtk_blas_matrix_new(row,col);
			layer->has_output = 1;

			last_row = row;
			last_col = col;

		}else if(type == 7)
		{//TODO
			layer->type = QTK_TORCHNN_BATCH;
			layer->mdl = (qtk_torchnn_batch_t*)qtk_torchnn_batch_read_bin(src,buf,bin);
		}else if(type == 6)
		{//TODO    conv1d weight needs to be transed!!!!
			layer->type = QTK_TORCHNN_CONV1D;
			layer->mdl = (qtk_torchnn_conv1d_t*)qtk_torchnn_conv1d_read(src,buf,bin);
			//wtk_debug("conv1d\n");
			n = ((qtk_torchnn_conv1d_t*)layer->mdl)->kernel_row;
			dilation = ((qtk_torchnn_conv1d_t*)layer->mdl)->dilation1;
			//wtk_debug("dilation:%d %d\n",((qtk_torchnn_conv1d_t*)layer->mdl)->dilation1,n);
			if(dilation > 1)
			{
				n += (dilation-1)*(n-1);
			}
			//wtk_debug("dilation:%d %d\n",((qtk_torchnn_conv1d_t*)layer->mdl)->dilation1,n);
			robin = wtk_robin_new(n);

			row = ((qtk_torchnn_conv1d_t*)layer->mdl)->weight->row;
			col = last_col;
			layer->input = qtk_blas_matrix_new(row,col);
			//wtk_debug("%d %d %d\n",row,col,((qtk_torchnn_conv1d_t*)layer->mdl)->stride1);

			row = last_row;
			col = last_col;
			//wtk_debug("%d %d %d\n",row,col,((qtk_torchnn_conv1d_t*)layer->mdl)->stride1);
			layer->output = qtk_blas_matrix_new(row,col);
			layer->has_output = 1;

			qtk_blas_matrix_zero(layer->input);
			qtk_blas_matrix_zero(layer->output);
			last_row = row;
			last_col = col;
			layer->rb = robin;

		}else if(type == 4)
		{
			layer->type = QTK_TORCHNN_EMBED;
			layer->mdl = (qtk_torchnn_embed_t*)qtk_torchnn_embed_read_bin(src,buf,bin);
		}else if(type == 5)
		{
			int tt;
			layer->type = QTK_TORCHNN_ACTIVATE;
			ret = wtk_source_read_int_little(src, &tt, 1, bin);
			layer->a_type = tt;
		}else if(type == 10)
		{
			layer->type = QTK_TORCHNN_SE;
			layer->mdl = (qtk_torchnn_se_t*)qtk_torchnn_se_read(src,buf,bin,last_col);
			//wtk_debug("se\n");
		}else if(type == 11)
		{
			layer->type = QTK_TORCHNN_BASIC;
			layer->mdl = (qtk_torchnn_block_t*)qtk_torchnn_block_read_bin(src,buf,bin,last_col,last_row);
			//wtk_debug("basic block %d %d\n",last_row,last_col);
			last_row = ((qtk_torchnn_block_t*)layer->mdl)->mdl->output_row;;
			last_col = ((qtk_torchnn_block_t*)layer->mdl)->mdl->output_col;
		}else
		{
			wtk_debug("error!\n");
			exit(0);
		}
	}

	torchnn->output_col = last_col;
	torchnn->output_row = last_row;

	if(ret!=0)
	{
		//wtk_debug("error\n");
	}
	//wtk_debug("tormdl :%d %d\n",last_row,last_col);
	return torchnn;
}

qtk_torchnn_t* qtk_torchnn_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin,int feat_col,int feat_row)
{
	int ret, v, i, n, row = -1, col = -1;
	int last_row, last_col, dilation;
	qtk_torchnn_t *torchnn;
	qtk_torchnn_layer_t *layer;
	qtk_torchnn_lstm_t* lstm;
	qtk_torchnn_conv2d_t* conv2d;
	wtk_robin_t *robin;

	ret = wtk_source_read_string(src, buf);
	wtk_debug("%.*s\n",buf->pos,buf->data);
	ret = wtk_source_read_int_little(src, &v, 1, bin);
	//v = 23;
	torchnn = qtk_torchnn_new(v);
	last_col = feat_col;
	last_row = feat_row;
	wtk_debug("%d\n",v);
	for(i = 0;i < v; i++)
	{
		ret = wtk_source_read_string(src, buf);
		wtk_debug("%.*s %d\n",buf->pos,buf->data,i);
		layer = qtk_torchnn_layer_new();
		torchnn->layers[i] = layer;
		if(wtk_str_equal_s(buf->data, buf->pos, "<Conv2d>"))
		{
			layer->type = QTK_TORCHNN_CONV2D;
			conv2d = qtk_torchnn_conv2d_read(src,buf,bin);
			layer->mdl = conv2d;
			layer->input_row = last_row;
			layer->input_col = last_col;

			n = conv2d->kernel_row;
			dilation = conv2d->dilation1;
			//wtk_debug("dilation:%d %d\n",((qtk_torchnn_conv2d_t*)layer->mdl)->dilation1,n);
			if(dilation > 1)
			{
				n += (dilation-1)*(n-1);
			}
			//wtk_debug("dilation:%d %d\n",((qtk_torchnn_conv2d_t*)layer->mdl)->dilation1,n);
			robin = wtk_robin_new(n);

			row = conv2d->kernel_row * last_row;
			col = last_col + conv2d->padding_col*2;
			//wtk_debug("111111 %d\n",((qtk_torchnn_conv2d_t*)layer->mdl)->padding_col);
			//wtk_debug("%d %d\n",row,col);
			layer->input = qtk_blas_matrix_new(row,col);
			//wtk_debug("%d %d %d\n",row,col,conv2d->stride1);
			row = conv2d->weight->row;
			col = last_col/conv2d->stride2;
			if(conv2d->stride2 > 1)
			{
				col = (last_col+1)/conv2d->stride2;
			}
			//wtk_debug("%d %d %d\n",last_col,conv2d->stride2,(last_col+1)/conv2d->stride2);
			//wtk_debug("%d %d %d\n",row,col,conv2d->stride1);
			layer->output = qtk_blas_matrix_new(row,col);
			layer->has_output = 1;
			qtk_blas_matrix_zero(layer->input);
			qtk_blas_matrix_zero(layer->output);

			last_row = row;
			last_col = col;

			layer->skip = ((qtk_torchnn_conv2d_t*)layer->mdl)->stride1 - 1;
			layer->skip_cnt = 0;
			layer->rb = robin;

			col = conv2d->kernel_col*conv2d->kernel_row*layer->input_row;//9 3*3*1
			row = layer->input_col/conv2d->stride2;//40 (42-2)*(3-2)
			if(conv2d->stride2 > 1)
			{
				row = (layer->input_col+1)/conv2d->stride2;
			}
			conv2d->trans = qtk_blas_matrix_new(row,col);

		} else if(wtk_str_equal_s(buf->data, buf->pos, "<Lstm>"))
		{
			layer->type = QTK_TORCHNN_LSTM;
			lstm = (qtk_torchnn_lstm_t*)qtk_torchnn_lstm_read(src,buf,bin);
			layer->mdl = lstm;

			layer->lstma = (qtk_torchnn_lstm_assit_t*)wtk_malloc(sizeof(qtk_torchnn_lstm_assit_t));
			//wtk_debug("%d\n",lstm->ih_w->col);
			layer->input =  qtk_blas_matrix_new(1,lstm->ih_w->col);
			layer->lstma->h1 = qtk_blas_matrix_new(1,lstm->hh_w->col);
			layer->output = qtk_blas_matrix_new(1,lstm->hh_w->col);
			layer->has_output = 1;
			layer->lstma->h2 = layer->output;
			layer->lstma->c1 = qtk_blas_matrix_new(layer->lstma->h1->row,lstm->hh_w->row/4);
			layer->lstma->c2 = qtk_blas_matrix_new(layer->lstma->h1->row,lstm->hh_w->row/4);

			qtk_blas_matrix_zero(layer->output);
			qtk_blas_matrix_zero(layer->lstma->h1);
			qtk_blas_matrix_zero(layer->lstma->c1);
			qtk_blas_matrix_zero(layer->lstma->c2);

			last_row = layer->output->row;
			last_col = layer->output->col;

		}else if(wtk_str_equal_s(buf->data, buf->pos, "<Maxpool>"))
		{
			layer->type = QTK_TORCHNN_MAXPOOL;
			layer->mdl = (qtk_torchnn_maxpool_t*)qtk_torchnn_maxpool_read(src,buf,bin);

			row = ((qtk_torchnn_maxpool_t*)layer->mdl)->kernel_size * last_row;
			col = last_col;
			layer->input = qtk_blas_matrix_new(row,col);

			row = last_row;
			col = last_col/ ((qtk_torchnn_maxpool_t*)layer->mdl)->stride;
			layer->output = qtk_blas_matrix_new(row,col);
			layer->has_output = 1;
			qtk_blas_matrix_zero(layer->input);
			qtk_blas_matrix_zero(layer->output);

			robin = wtk_robin_new(((qtk_torchnn_maxpool_t*)layer->mdl)->kernel_size);
			layer->rb = robin;

			last_row = row;
			last_col = col;
		}else if(wtk_str_equal_s(buf->data, buf->pos, "<AvgPool>"))
		{
			layer->type = QTK_TORCHNN_AVPOOL;
			layer->mdl = (qtk_torchnn_avpool_t*)qtk_torchnn_avpool_read(src,buf,bin);

			if(((qtk_torchnn_avpool_t*)layer->mdl)->kernel_row <= 1)
			{
				layer->input = NULL;
				row = last_row;
				col = last_col/((qtk_torchnn_avpool_t*)layer->mdl)->kernel_col;
				//wtk_debug("%d %d\n",row,col);
				layer->output = qtk_blas_matrix_new(row,col);
				layer->has_output = 1;
			}else
			{//TODO need robin
//				row = ((qtk_torchnn_avpool_t*)layer->mdl)->kernel_size * last_row;
//				col = last_col;
//				layer->input = qtk_blas_matrix_new(row,col);
//
//				row = last_row;
//				col = last_col/ ((qtk_torchnn_avpool_t*)layer->mdl)->stride;
//				layer->output = qtk_blas_matrix_new(row,col);
//				qtk_blas_matrix_zero(layer->input);
//				qtk_blas_matrix_zero(layer->output);
//
//				robin = wtk_robin_new(((qtk_torchnn_avpool_t*)layer->mdl)->kernel_size);
//				layer->rb = robin;
			}
			last_row = row;
			last_col = col;
		}else if(wtk_str_equal_s(buf->data, buf->pos, "<Mean>"))
		{
			layer->type = QTK_TORCHNN_MEAN;
			//layer->mdl = (qtk_torchnn_avpool_t*)qtk_torchnn_avpool_read(src,buf,bin);

			layer->input = NULL;
			row = 1;
			col = last_row;
			//wtk_debug("mean %d %d\n",row,col);
			layer->output = qtk_blas_matrix_new(row,col);
			layer->has_output = 1;

			last_row = row;
			last_col = col;
		}else if(wtk_str_equal_s(buf->data, buf->pos, "<Linear>"))
		{
			layer->type = QTK_TORCHNN_LINEAR;
			layer->mdl = (qtk_torchnn_linear_t*)qtk_torchnn_linear_read(src,buf,bin);
			if (i!=0)
			{
				layer->input = NULL;//for linear use last layer output
			}else
			{
				row = last_row;
				col = last_col;
				layer->input = qtk_blas_matrix_new(row,col);
			}
			row = last_row;
			col = ((qtk_torchnn_linear_t*)layer->mdl)->weight->row;
			if(last_row * last_col == ((qtk_torchnn_linear_t*)layer->mdl)->weight->col)
			{
				row = 1;
				//col = ((qtk_torchnn_linear_t*)layer->mdl)->weight->col;
			}

			layer->output = qtk_blas_matrix_new(row,col);
			layer->has_output = 1;

			last_row = row;
			last_col = col;

		}else if(wtk_str_equal_s(buf->data, buf->pos, "<BatchNorm>"))
		{//TODO
			layer->type = QTK_TORCHNN_BATCH;
			layer->mdl = (qtk_torchnn_batch_t*)qtk_torchnn_batch_read(src,buf,bin);
		}else if(wtk_str_equal_s(buf->data, buf->pos, "<Conv1d>"))
		{//TODO    conv1d weight needs to be transed!!!!
			layer->type = QTK_TORCHNN_CONV1D;
			layer->mdl = (qtk_torchnn_conv1d_t*)qtk_torchnn_conv1d_read(src,buf,bin);
			//wtk_debug("conv1d\n");
			n = ((qtk_torchnn_conv1d_t*)layer->mdl)->kernel_row;
			dilation = ((qtk_torchnn_conv1d_t*)layer->mdl)->dilation1;
			//wtk_debug("dilation:%d %d\n",((qtk_torchnn_conv1d_t*)layer->mdl)->dilation1,n);
			if(dilation > 1)
			{
				n += (dilation-1)*(n-1);
			}
			//wtk_debug("dilation:%d %d\n",((qtk_torchnn_conv1d_t*)layer->mdl)->dilation1,n);
			robin = wtk_robin_new(n);

			row = ((qtk_torchnn_conv1d_t*)layer->mdl)->weight->row;
			col = last_col;
			layer->input = qtk_blas_matrix_new(row,col);
			//wtk_debug("%d %d %d\n",row,col,((qtk_torchnn_conv1d_t*)layer->mdl)->stride1);

			row = last_row;
			col = last_col;
			//wtk_debug("%d %d %d\n",row,col,((qtk_torchnn_conv1d_t*)layer->mdl)->stride1);
			layer->output = qtk_blas_matrix_new(row,col);
			layer->has_output = 1;

			qtk_blas_matrix_zero(layer->input);
			qtk_blas_matrix_zero(layer->output);
			last_row = row;
			last_col = col;
			layer->rb = robin;

		}else if(wtk_str_equal_s(buf->data, buf->pos, "<Embed>"))
		{
			layer->type = QTK_TORCHNN_EMBED;
			layer->mdl = (qtk_torchnn_embed_t*)qtk_torchnn_embed_read(src,buf,bin);
		}else if(wtk_str_equal_s(buf->data, buf->pos, "<Tanh>"))
		{
			layer->type = QTK_TORCHNN_ACTIVATE;
			layer->a_type = QTK_TANH;
			ret = wtk_source_read_string(src, buf);
		}else if(wtk_str_equal_s(buf->data, buf->pos, "<Logsoftmax>"))
		{
			layer->type = QTK_TORCHNN_ACTIVATE;
			layer->a_type = QTK_LOGSOFTMAX;
			ret = wtk_source_read_string(src, buf);
		}else if(wtk_str_equal_s(buf->data, buf->pos, "<Softmax>"))
		{
			layer->type = QTK_TORCHNN_ACTIVATE;
			layer->a_type = QTK_SOFTMAX;
			ret = wtk_source_read_string(src, buf);
		}else if(wtk_str_equal_s(buf->data, buf->pos, "<Sigmoid>"))
		{
			layer->type = QTK_TORCHNN_ACTIVATE;
			layer->a_type = QTK_SIGMOID;
			ret = wtk_source_read_string(src, buf);
		}else if(wtk_str_equal_s(buf->data, buf->pos, "<Relu>"))
		{
			layer->type = QTK_TORCHNN_ACTIVATE;
			layer->a_type = QTK_RELU;
			ret = wtk_source_read_string(src, buf);
		}else if(wtk_str_equal_s(buf->data, buf->pos, "<SE>"))
		{
			layer->type = QTK_TORCHNN_SE;
			layer->mdl = (qtk_torchnn_se_t*)qtk_torchnn_se_read(src,buf,bin,last_col);
			//wtk_debug("se\n");
			ret = wtk_source_read_string(src, buf);
		}else if(wtk_str_equal_s(buf->data, buf->pos, "<BasicBlock>"))
		{
			layer->type = QTK_TORCHNN_BASIC;
			layer->mdl = (qtk_torchnn_block_t*)qtk_torchnn_block_read(src,buf,bin,last_col,last_row);
			//wtk_debug("basic block %d %d\n",last_row,last_col);
			last_row = ((qtk_torchnn_block_t*)layer->mdl)->mdl->output_row;;
			last_col = ((qtk_torchnn_block_t*)layer->mdl)->mdl->output_col;
			ret = wtk_source_read_string(src, buf);
		}else
		{
			wtk_debug("error!\n");
			exit(0);
		}
	}

	torchnn->output_col = last_col;
	torchnn->output_row = last_row;

	if(ret!=0)
	{
		//wtk_debug("error\n");
	}

	//wtk_debug("tormdl :%d %d\n",last_row,last_col);
	return torchnn;
}

void qtk_torchnn_delete(qtk_torchnn_t *torchnn)
{
	int i;
	qtk_torchnn_layer_t *layer;

	for(i = 0;i < torchnn->nlayer; i++)
	{
		layer = torchnn->layers[i];
		switch(layer->type)
		{
		case QTK_TORCHNN_CONV2D:
			qtk_torchnn_conv2d_delete(layer->mdl);
			break;
		case QTK_TORCHNN_CONV1D:
			qtk_torchnn_conv1d_delete(layer->mdl);
			break;
		case QTK_TORCHNN_MAXPOOL:
			break;
		case QTK_TORCHNN_LSTM:
			qtk_torchnn_lstm_delete(layer->mdl);
			break;
		case QTK_TORCHNN_LINEAR:
			qtk_torchnn_linear_delete(layer->mdl);
			break;
		case QTK_TORCHNN_EMBED:
			qtk_torchnn_embed_delete(layer->mdl);
			break;
		case QTK_TORCHNN_BATCH:
			qtk_torchnn_batch_delete(layer->mdl);
			break;
		case QTK_TORCHNN_SE:
			qtk_torchnn_se_delete(layer->mdl);
			break;
		case QTK_TORCHNN_BASIC:
			qtk_torchnn_block_delete(layer->mdl);
			break;
		default:
			break;
		}
		qtk_torchnn_layer_delete(layer);
	}
	wtk_free(torchnn->layers);
	wtk_free(torchnn);
}

void qtk_torchnn_reset(qtk_torchnn_t *torchnn)
{
	int i,n;
	qtk_torchnn_layer_t *layer;
	wtk_robin_t *r;
	qtk_blas_matrix_t *m;
	qtk_torchnn_block_t *b;

	for(i = 0;i < torchnn->nlayer; i++)
	{
		layer = torchnn->layers[i];
		r = layer->rb;
		if(r)
		{
			n = layer->rb->nslot;

			while(r->used>0)
			{
				m=wtk_robin_pop(r);
				if(!m){break;}
				qtk_blas_matrix_delete(m);
			}
			wtk_robin_delete(r);
			layer->rb = wtk_robin_new(n);
		}
                layer->skip_cnt = 0;
		if(layer->has_output)
		{
			qtk_blas_matrix_zero(layer->output);
		}
		switch(layer->type)
		{
		case QTK_TORCHNN_BASIC:
			b = (qtk_torchnn_block_t*)layer->mdl;
			qtk_torchnn_reset(b->mdl);
			if(b->downsample)
			{
				qtk_torchnn_reset(b->mdl_down);
			}
			b->mat = NULL;
//			r = b->cache_rb;
//			if(r)
//			{
//				n = r->nslot;
//				for(j=0;j<n;j++)
//				{
//					m = wtk_robin_next(r);
//					qtk_blas_matrix_zero(m);
//				}
//				wtk_robin_reset(r);
//			}

			break;

		default:
			break;
		}
	}
}

void qtk_torchnn_set_notify(qtk_torchnn_t *torchnn,void *ths,qtk_torchnn_notify_f notify)
{
	torchnn->ths = ths;
	torchnn->notify = notify;
}

void qtk_torchnn_set_notify_end(qtk_torchnn_t *torchnn,qtk_torchnn_notify_end_f notify)
{
	torchnn->notify_end = notify;
}

qtk_torchnn_se_t* qtk_torchnn_se_new(void)
{
	qtk_torchnn_se_t *se = (qtk_torchnn_se_t*)wtk_malloc(sizeof(qtk_torchnn_se_t));

	se->mdl = NULL;
	se->index = 0;
	se->online = 1;
	return se;
}

void qtk_torchnn_se_delete(qtk_torchnn_se_t *se)
{
	qtk_torchnn_delete(se->mdl);
	if(se->online)
	{
		qtk_blas_matrix_delete(se->input);
	}
}

int qtk_torchnn_se_write(void *mdl,FILE *f){
	int ret;
	qtk_torchnn_se_t* se = (qtk_torchnn_se_t*)mdl;
	ret = fwrite(&(se->moving_avg),4,1,f);

	qtk_torchnn_write(se->mdl,f);

	return ret;
}

qtk_torchnn_se_t* qtk_torchnn_se_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin, int length)
{
	//int ret,v,i,n,row,col;
	//int last_row,last_col,dilation;
	//qtk_torchnn_t *torchnn;
	qtk_torchnn_se_t *se = qtk_torchnn_se_new();

	wtk_source_read_string(src, buf);
	wtk_source_read_int(src, &se->moving_avg, 1, bin);

	se->mdl = qtk_torchnn_read(src,buf,bin,length,1);
	if(se->online)
	{
		se->input = qtk_blas_matrix_new(1,length);
	}
	return se;
}

void qtk_torchnn_se_cal(qtk_torchnn_se_t *se, qtk_blas_matrix_t *in)
{

}

qtk_torchnn_block_t* qtk_torchnn_block_new(void)
{
	qtk_torchnn_block_t * block = (qtk_torchnn_block_t*)wtk_malloc(sizeof(qtk_torchnn_block_t));

	block->mdl = NULL;
	block->mdl_down = NULL;
	block->mat = NULL;

	return block;
}

void qtk_torchnn_block_delete(qtk_torchnn_block_t *block)
{
	wtk_robin_t *r = block->cache_rb;
	qtk_blas_matrix_t *m;
	int x=0;
	if(r)
	{
		while(r->used>0)
		{
			m=wtk_robin_pop(r);
			if(!m){break;}
			x++;
			qtk_blas_matrix_delete(m);
		}
		//wtk_debug("%d\n",x);
		wtk_robin_delete(r);
	}
	qtk_torchnn_delete(block->mdl);
	if(block->downsample)
	{
		qtk_torchnn_delete(block->mdl_down);
	}
	//qtk_blas_matrix_delete(block->mat);
}

int qtk_torchnn_get_delay_frame(qtk_torchnn_t *torchnn)
{
	qtk_torchnn_layer_t *layer;
	int i,ret=0;
	qtk_torchnn_conv2d_t *conv;

	for(i = 0;i < torchnn->nlayer; i++)
	{
		layer = torchnn->layers[i];
		if(layer->type == QTK_TORCHNN_CONV2D)
		{
			conv = (qtk_torchnn_conv2d_t*)layer->mdl;
			//wtk_debug("%d %d %d %d\n",conv->padding_row,conv->kernel_row,conv->stride1,conv->dilation1);
			ret += conv->dilation1*conv->kernel_row - conv->padding_col - 1*conv->dilation1;
		}
	}

	return ret;
}

int qtk_torchnn_block_write(void *mdl,FILE *f){
	int ret;
	qtk_torchnn_block_t* block = (qtk_torchnn_block_t*)mdl;
	ret = fwrite(&(block->downsample),4,1,f);
	if(block->downsample){
		qtk_torchnn_write(block->mdl_down,f);
	}
	qtk_torchnn_write(block->mdl,f);
	return ret;
}

qtk_torchnn_block_t* qtk_torchnn_block_read_bin(wtk_source_t *src,wtk_strbuf_t *buf,int bin, int length, int row)
{
	int i;
	int nrbin;
	int rb_row,rb_col;
	//qtk_torchnn_t *torchnn;
	qtk_torchnn_block_t *block = qtk_torchnn_block_new();
	qtk_blas_matrix_t *m;

	wtk_source_read_int_little(src, &block->downsample, 1, bin);
	//wtk_debug("block read:%d %d\n",length,row);
	if(block->downsample == 1)
	{
		//wtk_debug("down sample\n");
		block->mdl_down = qtk_torchnn_read_bin(src,buf,bin,length,row);
	}
	block->mdl = qtk_torchnn_read_bin(src,buf,bin,length,row);
	nrbin = qtk_torchnn_get_delay_frame(block->mdl);
	block->cache_rb = wtk_robin_new(nrbin);
	rb_row = block->mdl->output_row;
	rb_col = block->mdl->output_col;
	//wtk_debug("%d %d %d %d\n",row,length,rb_row,rb_col);
	for(i=0;i<nrbin;i++)
	{
		m = qtk_blas_matrix_new(rb_row,rb_col);
		wtk_robin_push(block->cache_rb,m);
	}

	return block;
}

qtk_torchnn_block_t* qtk_torchnn_block_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin, int length, int row)
{
	int i;
	int nrbin;
	int rb_row,rb_col;
	//qtk_torchnn_t *torchnn;
	qtk_torchnn_block_t *block = qtk_torchnn_block_new();
	qtk_blas_matrix_t *m;

	wtk_source_read_string(src, buf);
	wtk_source_read_int(src, &block->downsample, 1, bin);
	//wtk_debug("block read:%d %d\n",length,row);

	if(block->downsample == 1)
	{
		//wtk_debug("down sample\n");
		block->mdl_down = qtk_torchnn_read(src,buf,bin,length,row);
	}
	wtk_source_read_string(src, buf);
	block->mdl = qtk_torchnn_read(src,buf,bin,length,row);
	nrbin = qtk_torchnn_get_delay_frame(block->mdl);
	block->cache_rb = wtk_robin_new(nrbin);
	rb_row = block->mdl->output_row;
	rb_col = block->mdl->output_col;
	//wtk_debug("%d %d %d %d\n",row,length,rb_row,rb_col);
	for(i=0;i<nrbin;i++)
	{
		m = qtk_blas_matrix_new(rb_row,rb_col);
		wtk_robin_push(block->cache_rb,m);
	}

	return block;
}

void qtk_torchnn_block_cal(qtk_torchnn_block_t *se, qtk_blas_matrix_t *in)
{

}

qtk_blas_matrix_t* qtk_torchnn_feed(qtk_torchnn_t *torchnn,float *f,int in_col, int in_row,int layer_index)
{
	int i,j,batch_row,batch_col,dilation,cnt=0;
	int len = in_row * in_col;
        qtk_torchnn_layer_t *layer, *last_layer = NULL;
        qtk_torchnn_conv2d_t *conv;
        qtk_torchnn_conv1d_t *conv1d;
	qtk_torchnn_linear_t *linear;
	qtk_torchnn_lstm_t *lstm;
	qtk_torchnn_lstm_assit_t *lstma;
	qtk_torchnn_maxpool_t *maxpool;
	qtk_torchnn_avpool_t *avpool;
	qtk_blas_matrix_t *m,*input,*tmp,*out,*tmp1=NULL,*tmp2;
	qtk_torchnn_batch_t *batch;
	qtk_torchnn_se_t *se;
	qtk_torchnn_block_t *block;
	for(i = layer_index; i < torchnn->nlayer; i++)
	{
		layer = torchnn->layers[i];

		//wtk_debug("==========feed layer:%d %p=============\n",i,layer);
		switch(layer->type)
		{
		case QTK_TORCHNN_CONV2D:
			//void qtk_torchnn_conv2d_cal(qtk_torchnn_conv2d_t *conv, qtk_blas_matrix_t *input, qtk_blas_matrix_t *output,int batch_row, int batch_col)
			conv = (qtk_torchnn_conv2d_t*)(layer->mdl);
			//wtk_debug("%d\n",layer->rb->used);
			if(layer->rb->used == 0)
			{
				if(conv->padding_row > 0)
				{
					for(j=0;j<conv->padding_row;j++)
					{
						m = qtk_blas_matrix_new(layer->input->row/conv->kernel_row,layer->input->col);
						//if(i==0)
						//{
						//	wtk_debug("%d %d\n",layer->input->row/conv->kernel_row,layer->input->col);
						//}
						//wtk_debug("hhhhhhhhh:%d\n",conv->padding_row);
						qtk_blas_matrix_zero(m);
						wtk_robin_push(layer->rb,m);
					}
				}
			}

			if(layer->rb->used == layer->rb->nslot)
			{
				m = (qtk_blas_matrix_t*)wtk_robin_next(layer->rb);
			}else
			{
				//wtk_debug("%d %d\n",layer->input->row/conv->kernel_row,layer->input->col);
				m = qtk_blas_matrix_new(layer->input->row/conv->kernel_row,layer->input->col);
				qtk_blas_matrix_zero(m);
				wtk_robin_push(layer->rb,m);
			}

			if(i == 0)
			{
				//wtk_debug("%d %d %d %d %d\n",m->row,m->col,layer->input->row,layer->input->col,len);
				for(j=0;j<in_row;j++)
				{
					memcpy(m->m + conv->padding_col + m->col*j,f+in_col*j,in_col*sizeof(float));
					//wtk_debug("%f\n",*(m->m+conv->padding_col+m->col*j));
				}
				batch_row = in_row;
				batch_col = in_col;
				last_layer = layer;
			}else
			{
				//memcpy(m->m+conv->padding_col,torchnn->layers[i-1]->output->m,);
				input = last_layer->output;
				for(j=0;j<input->row;j++)
				{
					memcpy(m->m + conv->padding_col + m->col*j,input->m + input->col*j,input->col*sizeof(float));
				}
				batch_row = input->row;
				batch_col = input->col;
			}
			//wtk_debug("conv2d process stride1:%d %d\n",layer->skip,layer->skip_cnt);
			//wtk_debug("%d %d %d\n",layer->rb->used,layer->rb->nslot,i);
			if(layer->rb->used == layer->rb->nslot)
			{
				//wtk_debug("conv2d process stride2:%d %d\n",layer->skip,layer->skip_cnt);
				if(layer->skip > 0 && layer->skip_cnt > 0)
				{
					layer->skip_cnt--;
					return NULL;
				}
				dilation = conv->dilation1;
				for(j = 0,cnt=0; j < layer->rb->nslot; j+=dilation,cnt++)
				{

					m = (qtk_blas_matrix_t*)wtk_robin_at(layer->rb,j);
					memcpy(layer->input->m+cnt*m->row*m->col,m->m,m->row*m->col*sizeof(float));
				}

				qtk_blas_matrix_zero(layer->output);
				qtk_torchnn_conv2d_cal(conv, layer->input, layer->output,batch_row, batch_col);
				last_layer = layer;

				if(layer->skip)
				{
					if(layer->skip_cnt != 0)
					{
						wtk_debug("never be here\n");
						exit(0);
					}
					layer->skip_cnt = layer->skip;
				}
			}else
			{
				//wtk_debug("conv2d return\n");
				return NULL;
			}
			break;
		case QTK_TORCHNN_CONV1D:
			conv1d = (qtk_torchnn_conv1d_t*)(layer->mdl);
			//qtk_blas_matrix_print(last_layer->output);
			if(layer->rb->used == 0)
			{
				if(conv1d->padding_row > 0)
				{
					for(j=0;j<conv1d->padding_row;j++)
					{
						m = qtk_blas_matrix_new(layer->input->row/conv1d->kernel_row,layer->input->col);
						//wtk_debug("%d %d\n",layer->input->row/conv->kernel_row,layer->input->col);
						//wtk_debug("hhhhhhhhh:%d\n",conv1d->padding_row);
						qtk_blas_matrix_zero(m);
						wtk_robin_push(layer->rb,m);
					}
				}
			}

			if(layer->rb->used == layer->rb->nslot)
			{
				m = (qtk_blas_matrix_t*)wtk_robin_next(layer->rb);
			}else
			{
				//wtk_debug("%d %d\n",layer->input->row/conv->kernel_row,layer->input->col);
				m = qtk_blas_matrix_new(layer->input->row/conv1d->kernel_row,layer->input->col);
				qtk_blas_matrix_zero(m);
				wtk_robin_push(layer->rb,m);
			}

			if(i == 0)
			{
				memcpy(m->m,f,len*sizeof(float));
				batch_row = 1;
				batch_col = len;
				last_layer = layer;
			}else
			{
				//memcpy(m->m+conv->padding_col,torchnn->layers[i-1]->output->m,);
				input = last_layer->output;
				for(j=0;j<input->row;j++)
				{
					memcpy(m->m + m->col*j,input->m + input->col*j,input->col*sizeof(float));
				}
				batch_row = input->row;
				batch_col = input->col;
			}

			if(layer->rb->used == layer->rb->nslot)
			{
				dilation = conv1d->dilation1;
				for(j = 0,cnt=0; j < layer->rb->nslot; j+=dilation,cnt++)
				{

					m = (qtk_blas_matrix_t*)wtk_robin_at(layer->rb,j);
					memcpy(layer->input->m+cnt*m->row*m->col,m->m,m->row*m->col*sizeof(float));
				}
				qtk_blas_matrix_zero(layer->output);
				qtk_torchnn_conv1d_cal(conv1d, layer->input, layer->output);
				last_layer = layer;
			}else
			{
				//wtk_debug("conv2d return\n");
				return NULL;
			}

			break;
		case QTK_TORCHNN_AVPOOL:
			avpool = (qtk_torchnn_avpool_t*)(layer->mdl);

			qtk_torchnn_avpool_cal(avpool,last_layer->output,layer->output);

			break;
		case QTK_TORCHNN_MEAN:
			qtk_blas_matrix_zero(layer->output);
			qtk_torchnn_mean(last_layer->output,layer->output);

			break;
		case QTK_TORCHNN_MAXPOOL:
			maxpool = (qtk_torchnn_maxpool_t*)(layer->mdl);

			if(layer->rb->used == layer->rb->nslot)
			{
				m = (qtk_blas_matrix_t*)wtk_robin_next(layer->rb);
			}else
			{
				m = qtk_blas_matrix_new(last_layer->output->row,last_layer->output->col);
				wtk_robin_push(layer->rb,m);
			}

			batch_row = last_layer->output->row;
			input = torchnn->layers[i-1]->output;
			memcpy(m->m,input->m,input->col*input->row*sizeof(float));

			if(layer->rb->used == layer->rb->nslot)
			{
				for(j=0;j<maxpool->kernel_size;j++)
				{
					m = (qtk_blas_matrix_t*)wtk_robin_pop(layer->rb);
					memcpy(layer->input->m+j*m->row*m->col,m->m,m->row*m->col*sizeof(float));
				}
				//wtk_debug("maxpool cal %p\n",maxpool);
				qtk_blas_matrix_zero(layer->output);
				qtk_torchnn_maxpool_cal(maxpool, layer->input, layer->output,batch_row);
				last_layer = layer;
				//qtk_blas_matrix_print(layer->output);
			}else
			{
				//wtk_debug("maxpool return\n");
				return NULL;
			}
			break;
		case QTK_TORCHNN_LSTM:
			lstm = (qtk_torchnn_lstm_t*)(layer->mdl);
			lstma = layer->lstma;
			if(i == 0)
			{
				memcpy(layer->input->m,f,len*sizeof(float));
			}else
			{
				memcpy(layer->input->m,last_layer->output->m,last_layer->output->col*last_layer->output->row*sizeof(float));
			}

			tmp = lstma->c1;
			lstma->c1 = lstma->c2;
			lstma->c2 = tmp;
			qtk_blas_matrix_zero(lstma->c2);
			tmp = lstma->h1;
			lstma->h1 = lstma->h2;
			lstma->h2 = tmp;
			qtk_blas_matrix_zero(lstma->h2);
			//wtk_debug("lstm cal %p\n",lstm);

			//qtk_blas_matrix_print(layer->input);
			//qtk_blas_matrix_print(lstma->h1);
			qtk_torchnn_lstm_cal(lstm,layer->input,lstma->c1,lstma->h1,lstma->c2,lstma->h2);
			layer->output = lstma->h2;
			last_layer = layer;
			break;
		case QTK_TORCHNN_LINEAR:
			//wtk_debug("linear\n");
			linear = (qtk_torchnn_linear_t*)(layer->mdl);
			qtk_blas_matrix_zero(layer->output);
			//wtk_debug("%d %d\n",linear->weight->row,linear->weight->col);
			if(i == 0)
			{
				memcpy(layer->input->m,f,len*sizeof(float));
				qtk_torchnn_linear_cal(linear,layer->input,layer->output);
			}else
			{
				//layer->input = last_layer->output;
				qtk_torchnn_linear_cal(linear,last_layer->output,layer->output);
			}
			//qtk_torchnn_linear_cal(linear,layer->input,layer->output);
			last_layer = layer;
			break;
		case QTK_TORCHNN_BATCH:
			batch = (qtk_torchnn_batch_t*)(layer->mdl);

			qtk_torchnn_batch_cal(batch,last_layer->output);
			break;
		case QTK_TORCHNN_EMBED:
			break;
		case QTK_TORCHNN_SE:
			se = (qtk_torchnn_se_t*)(layer->mdl);
			//qtk_blas_matrix_print(last_layer->output);
			qtk_blas_matrix_aver(se->input,last_layer->output,se->index);
			//qtk_blas_matrix_print(se->input);
			se->index++;
			tmp = qtk_torchnn_feed(se->mdl,se->input->m,se->input->col,se->input->row,0);
			if(!tmp)
			{
				wtk_debug("error se\n");
				exit(0);
			}
			qtk_blas_matrix_mul_col(last_layer->output,last_layer->output,tmp);
			//exit(0);

			break;

		case QTK_TORCHNN_BASIC:
			block = (qtk_torchnn_block_t*)(layer->mdl);
			//wtk_debug("process basic block %p\n",block);
			//if(block->downsample)
			//{
			//wtk_debug("process basic block down sample\n");
			//}
			//qtk_blas_matrix_print(last_layer->output);
			tmp = qtk_torchnn_feed(block->mdl,last_layer->output->m,last_layer->output->col,last_layer->output->row,0);
			if(block->downsample)
			{
				//wtk_debug("process basic block down sample\n");
				tmp1 = qtk_torchnn_feed(block->mdl_down,last_layer->output->m,last_layer->output->col,last_layer->output->row,0);

				if(tmp)//2->1 delay 1frame
				{
					tmp2 = wtk_robin_next(block->cache_rb);
					//wtk_debug("%p\n",tmp2);
					//qtk_blas_matrix_print(tmp);
					//qtk_blas_matrix_print(tmp2);
					qtk_blas_matrix_add_mat(tmp,tmp2);
					for(j = 0; j < tmp->row; j++)
					{
						wtk_relu2(tmp->m + tmp->col*j,tmp->col);
					}
					block->mat = tmp2;
					//memcpy(tmp2->m,tmp1->m,sizeof(float)*tmp2->col*tmp2->row);
					layer->output = tmp;
					//qtk_blas_matrix_print(layer->output);
					last_layer = layer;
				}else
				{
					if(tmp1)//cache down sample 2->1
					{
						if(block->mat)
						{
							memcpy(block->mat->m,tmp1->m,sizeof(float)*block->mat->col*block->mat->row);
						}else
						{
							//qtk_blas_matrix_print(tmp1);
							tmp2 = wtk_robin_next(block->cache_rb);
							memcpy(tmp2->m,tmp1->m,sizeof(float)*tmp2->col*tmp2->row);
						}
					}
					return NULL;
				}
			}else
			{
				//wtk_debug("block feed end\n");
				if(tmp)
				{
					//qtk_blas_matrix_print(tmp);
					tmp1 = wtk_robin_next(block->cache_rb);
					//qtk_blas_matrix_print(tmp1);
					qtk_blas_matrix_add_mat(tmp,tmp1);
					for(j = 0; j < tmp->row; j++)
					{
						wtk_relu2(tmp->m + tmp->col*j,tmp->col);
					}
					//qtk_blas_matrix_print(tmp);
					memcpy(tmp1->m,last_layer->output->m,sizeof(float)*tmp1->col*tmp1->row);
					layer->output = tmp;
					last_layer = layer;
					//qtk_blas_matrix_print(layer->output);
				}else
				{
					tmp1 = wtk_robin_next(block->cache_rb);
					memcpy(tmp1->m,last_layer->output->m,sizeof(float)*tmp1->col*tmp1->row);
					//wtk_debug("return\n");
					return NULL;
				}
			}
			break;

		case QTK_TORCHNN_ACTIVATE:
			switch(layer->a_type)
			{
			case QTK_TANH:
				for(j = 0; j < last_layer->output->row; j++)
				{
					wtk_tanh(last_layer->output->m + last_layer->output->col*j,last_layer->output->col);
				}
				//qtk_blas_matrix_print(last_layer->output);
				break;
			case QTK_SIGMOID:
				for(j = 0; j < last_layer->output->row; j++)
				{
					wtk_sigmoid(last_layer->output->m + last_layer->output->col*j,last_layer->output->col);
				}
				//qtk_blas_matrix_print(last_layer->output);
				break;
			case QTK_RELU:
				//wtk_debug("relu\n");
				for(j = 0; j < last_layer->output->row; j++)
				{
					wtk_relu2(last_layer->output->m + last_layer->output->col*j,last_layer->output->col);
				}
				//qtk_blas_matrix_print(last_layer->output);
				break;
			case QTK_LOGSOFTMAX:
				for(j = 0; j < last_layer->output->row; j++)
				{
					wtk_torch_softmax(last_layer->output->m + last_layer->output->col*j,last_layer->output->col);
					wtk_add_log(last_layer->output->m + last_layer->output->col*j,last_layer->output->col);
				}
				//qtk_blas_matrix_print(last_layer->output);
				break;
			case QTK_SOFTMAX:
				//wtk_debug("softmax\n");
				for(j = 0; j < last_layer->output->row; j++)
				{
					wtk_torch_softmax(last_layer->output->m + last_layer->output->col*j,last_layer->output->col);
				}
				//qtk_blas_matrix_print(last_layer->output);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		//if(layer->output)
		//{
		//	wtk_debug("%d %d %p\n",layer->type,i,layer);
		//	qtk_blas_matrix_print(layer->output);
		//}
	}
	out = torchnn->layers[torchnn->nlayer-1]->output;
	if(!out)
	{
		out = torchnn->layers[torchnn->nlayer-2]->output;
	}
	/*
	if(torchnn->layers[torchnn->nlayer-1]->type != QTK_TORCHNN_ACTIVATE) //TODO may be cause error when last layer is lstm
	{
		out = torchnn->layers[torchnn->nlayer-1]->output;
	}else
	{
		out = torchnn->layers[torchnn->nlayer-2]->output;
	}
	*/
	//wtk_debug("torch nn end:%p\n",out);
	//qtk_blas_matrix_print(out);
	if(out && torchnn->notify)
	{
		torchnn->notify(torchnn->ths,out);
	}

	return out;
}

qtk_blas_matrix_t* qtk_torchnn_feed_flush(qtk_torchnn_t *torchnn,float *f,int in_col, int in_row,int layer_index,qtk_blas_matrix_t *bl)
{
	int i,j,batch_row,batch_col,dilation,cnt=0;
	int len = in_row * in_col;
	qtk_torchnn_layer_t *layer, *last_layer = NULL;
	qtk_torchnn_conv2d_t *conv = NULL;
	qtk_torchnn_conv1d_t *conv1d;
	qtk_torchnn_linear_t *linear;
	qtk_torchnn_lstm_t *lstm;
	qtk_torchnn_lstm_assit_t *lstma;
	qtk_torchnn_maxpool_t *maxpool;
	qtk_torchnn_avpool_t *avpool;
	qtk_blas_matrix_t *m, *input = NULL, *tmp, *out, *tmp1 = NULL, *tmp2;
	qtk_torchnn_batch_t *batch;
	qtk_torchnn_se_t *se;
	qtk_torchnn_block_t *block;

	for(i = layer_index; i < torchnn->nlayer; i++)
	{
		layer = torchnn->layers[i];
		//wtk_debug("==========feed layer:%d %p=============\n",i,layer);
		switch(layer->type)
		{
		case QTK_TORCHNN_CONV2D:
			//void qtk_torchnn_conv2d_cal(qtk_torchnn_conv2d_t *conv, qtk_blas_matrix_t *input, qtk_blas_matrix_t *output,int batch_row, int batch_col)
			conv = (qtk_torchnn_conv2d_t*)(layer->mdl);
			if(layer->rb->used == 0)
			{
				if(conv->padding_row > 0)
				{
					for(j=0;j<conv->padding_row;j++)
					{
						m = qtk_blas_matrix_new(layer->input->row/conv->kernel_row,layer->input->col);
						//wtk_debug("%d %d\n",layer->input->row/conv->kernel_row,layer->input->col);
						//wtk_debug("hhhhhhhhh:%d\n",conv->padding_row);
						qtk_blas_matrix_zero(m);
						wtk_robin_push(layer->rb,m);
					}
				}
			}

			if(layer->rb->used == layer->rb->nslot)
			{
				m = (qtk_blas_matrix_t*)wtk_robin_next(layer->rb);
			}else
			{
				//wtk_debug("%d %d\n",layer->input->row/conv->kernel_row,layer->input->col);
				m = qtk_blas_matrix_new(layer->input->row/conv->kernel_row,layer->input->col);
				qtk_blas_matrix_zero(m);
				wtk_robin_push(layer->rb,m);
			}

			if(i == 0)
			{
				//wtk_debug("%d %d %d %d %d\n",m->row,m->col,layer->input->row,layer->input->col,len);
				if(i == layer_index && f==NULL){
					for(j=0;j<layer->input_row;j++)
					{
						//memcpy(m->m + conv->padding_col + m->col*j,f+in_col*j,in_col*sizeof(float));
						memset(m->m + conv->padding_col + m->col*j,0,layer->input_col*sizeof(float));
					}
				}else
				{
					for(j=0;j<in_row;j++)
					{
						memcpy(m->m + conv->padding_col + m->col*j,f+in_col*j,in_col*sizeof(float));
					}
				}

				batch_row = layer->input_row;
				batch_col = layer->input_col;
				last_layer = layer;
				//wtk_debug("%d %d %d %d\n",batch_row,batch_col,layer->input_row,layer->input_col);
			}else
			{
				//memcpy(m->m+conv->padding_col,torchnn->layers[i-1]->output->m,);
				if(i == layer_index && f==NULL){
					for(j=0;j<layer->input_row;j++)
					{
						memset(m->m + conv->padding_col + m->col*j,0,layer->input_col*sizeof(float));
					}
					batch_row = layer->input_row;
					batch_col = layer->input_col;
				}else
				{
					input = last_layer->output;
					for(j=0;j<input->row;j++)
					{
						memcpy(m->m + conv->padding_col + m->col*j,input->m + input->col*j,input->col*sizeof(float));
					}
					batch_row = input->row;
					batch_col = input->col;
				}
			}
			//wtk_debug("conv2d process stride1:%d %d\n",layer->skip,layer->skip_cnt);
			if(layer->rb->used == layer->rb->nslot)
			{
				//wtk_debug("conv2d process stride2:%d %d\n",layer->skip,layer->skip_cnt);
				if(layer->skip > 0 && layer->skip_cnt > 0)
				{
					layer->skip_cnt--;
					return NULL;
				}
				dilation = conv->dilation1;
				for(j = 0,cnt=0; j < layer->rb->nslot; j+=dilation,cnt++)
				{

					m = (qtk_blas_matrix_t*)wtk_robin_at(layer->rb,j);
					memcpy(layer->input->m+cnt*m->row*m->col,m->m,m->row*m->col*sizeof(float));
				}

				qtk_blas_matrix_zero(layer->output);
				//wtk_debug("111111111 %d %d\n",batch_row,batch_col);
				qtk_torchnn_conv2d_cal(conv, layer->input, layer->output,batch_row, batch_col);
				last_layer = layer;

				if(layer->skip)
				{
					if(layer->skip_cnt != 0)
					{
						wtk_debug("never be here\n");
						exit(0);
					}
					layer->skip_cnt = layer->skip;
				}
			}else
			{
				//wtk_debug("conv2d return\n");
				return NULL;
			}
			break;
		case QTK_TORCHNN_CONV1D:
			conv1d = (qtk_torchnn_conv1d_t*)(layer->mdl);
			//qtk_blas_matrix_print(last_layer->output);
			if(layer->rb->used == 0)
			{
				if(conv1d->padding_row > 0)
				{
					for(j=0;j<conv1d->padding_row;j++)
					{
						m = qtk_blas_matrix_new(layer->input->row/conv1d->kernel_row,layer->input->col);
						//wtk_debug("%d %d\n",layer->input->row/conv->kernel_row,layer->input->col);
						//wtk_debug("hhhhhhhhh:%d\n",conv1d->padding_row);
						qtk_blas_matrix_zero(m);
						wtk_robin_push(layer->rb,m);
					}
				}
			}

			if(layer->rb->used == layer->rb->nslot)
			{
				m = (qtk_blas_matrix_t*)wtk_robin_next(layer->rb);
			}else
			{
				//wtk_debug("%d %d\n",layer->input->row/conv->kernel_row,layer->input->col);
				m = qtk_blas_matrix_new(layer->input->row/conv1d->kernel_row,layer->input->col);
				qtk_blas_matrix_zero(m);
				wtk_robin_push(layer->rb,m);
			}

			if(i == 0)
			{
				if(i == layer_index && f==NULL)
				{
					memset(m->m,0,input->col);
				}else
				{
					memcpy(m->m,f,len*sizeof(float));
				}
				batch_row = 1;
				batch_col = input->col;
				last_layer = layer;
			}else
			{
				//memcpy(m->m+conv->padding_col,torchnn->layers[i-1]->output->m,);
				if(i == layer_index && f==NULL){
					for(j=0;j<layer->input_row;j++)
					{
						memset(m->m + conv->padding_col + m->col*j,0,layer->input_col);
					}
					batch_row = layer->input_row;
					batch_col = layer->input_col;
				}else
				{
					input = last_layer->output;
					for(j=0;j<input->row;j++)
					{
						memcpy(m->m + m->col*j,input->m + input->col*j,input->col*sizeof(float));
					}
					batch_row = input->row;
					batch_col = input->col;
				}
			}

			if(layer->rb->used == layer->rb->nslot)
			{
				dilation = conv1d->dilation1;
				for(j = 0,cnt=0; j < layer->rb->nslot; j+=dilation,cnt++)
				{

					m = (qtk_blas_matrix_t*)wtk_robin_at(layer->rb,j);
					memcpy(layer->input->m+cnt*m->row*m->col,m->m,m->row*m->col*sizeof(float));
				}

				qtk_blas_matrix_zero(layer->output);
				qtk_torchnn_conv1d_cal(conv1d, layer->input, layer->output);
				last_layer = layer;

			}else
			{
				//wtk_debug("conv2d return\n");
				return NULL;
			}

			break;
		case QTK_TORCHNN_AVPOOL:
			avpool = (qtk_torchnn_avpool_t*)(layer->mdl);

			//wtk_debug("avg pool\n");
			//qtk_blas_matrix_print(last_layer->output);
			qtk_torchnn_avpool_cal(avpool,last_layer->output,layer->output);

			break;
		case QTK_TORCHNN_MEAN:
			qtk_blas_matrix_zero(layer->output);
			if(i == layer_index && bl){
				qtk_torchnn_mean(bl,layer->output);
			}else{
				qtk_torchnn_mean(last_layer->output,layer->output);
			}
			break;
		case QTK_TORCHNN_MAXPOOL:
			maxpool = (qtk_torchnn_maxpool_t*)(layer->mdl);

			if(layer->rb->used == layer->rb->nslot)
			{
				m = (qtk_blas_matrix_t*)wtk_robin_next(layer->rb);
			}else
			{
				m = qtk_blas_matrix_new(last_layer->output->row,last_layer->output->col);
				wtk_robin_push(layer->rb,m);
			}

			batch_row = last_layer->output->row;
			input = torchnn->layers[i-1]->output;
			memcpy(m->m,input->m,input->col*input->row*sizeof(float));

			if(layer->rb->used == layer->rb->nslot)
			{
				for(j=0;j<maxpool->kernel_size;j++)
				{
					m = (qtk_blas_matrix_t*)wtk_robin_pop(layer->rb);
					memcpy(layer->input->m+j*m->row*m->col,m->m,m->row*m->col*sizeof(float));
				}
				//wtk_debug("maxpool cal %p\n",maxpool);
				qtk_blas_matrix_zero(layer->output);
				qtk_torchnn_maxpool_cal(maxpool, layer->input, layer->output,batch_row);
				last_layer = layer;
				//qtk_blas_matrix_print(layer->output);
			}else
			{
				//wtk_debug("maxpool return\n");
				return NULL;
			}
			break;
		case QTK_TORCHNN_LSTM:
			lstm = (qtk_torchnn_lstm_t*)(layer->mdl);
			lstma = layer->lstma;
			if(i == 0)
			{
				memcpy(layer->input->m,f,len*sizeof(float));
			}else
			{
				memcpy(layer->input->m,last_layer->output->m,last_layer->output->col*last_layer->output->row*sizeof(float));
			}

			tmp = lstma->c1;
			lstma->c1 = lstma->c2;
			lstma->c2 = tmp;
			qtk_blas_matrix_zero(lstma->c2);
			tmp = lstma->h1;
			lstma->h1 = lstma->h2;
			lstma->h2 = tmp;
			qtk_blas_matrix_zero(lstma->h2);
			//wtk_debug("lstm cal %p\n",lstm);

			//qtk_blas_matrix_print(layer->input);
			//qtk_blas_matrix_print(lstma->h1);
			qtk_torchnn_lstm_cal(lstm,layer->input,lstma->c1,lstma->h1,lstma->c2,lstma->h2);
			layer->output = lstma->h2;
			last_layer = layer;

			break;
		case QTK_TORCHNN_LINEAR:
			//wtk_debug("linear\n");
			linear = (qtk_torchnn_linear_t*)(layer->mdl);
			//wtk_debug("%d %d\n",linear->weight->row,linear->weight->col);
			if(i == 0)
			{
				memcpy(layer->input->m,f,len*sizeof(float));
				qtk_torchnn_linear_cal(linear,layer->input,layer->output);
			}else
			{
				qtk_torchnn_linear_cal(linear,last_layer->output,layer->output);
			}
			qtk_blas_matrix_zero(layer->output);

			last_layer = layer;
			break;
		case QTK_TORCHNN_BATCH:
			batch = (qtk_torchnn_batch_t*)(layer->mdl);

			qtk_torchnn_batch_cal(batch,last_layer->output);

			break;
		case QTK_TORCHNN_EMBED:
			break;
		case QTK_TORCHNN_SE:
			se = (qtk_torchnn_se_t*)(layer->mdl);
			//qtk_blas_matrix_print(last_layer->output);
			qtk_blas_matrix_aver(se->input,last_layer->output,se->index);
			//qtk_blas_matrix_print(se->input);
			se->index++;
			tmp = qtk_torchnn_feed(se->mdl,se->input->m,se->input->col,se->input->row,0);
			if(!tmp)
			{
				wtk_debug("error se\n");
				exit(0);
			}
			qtk_blas_matrix_mul_col(last_layer->output,last_layer->output,tmp);
			//exit(0);

			break;

		case QTK_TORCHNN_BASIC:
			block = (qtk_torchnn_block_t*)(layer->mdl);
			//wtk_debug("process basic block %p\n",block);
			//if(block->downsample)
			//{
			//wtk_debug("process basic block down sample\n");
			//}
			if(i == layer_index && bl != NULL)
			{
				if(block->downsample)
				{
					tmp2 = wtk_robin_next(block->cache_rb);
					//qtk_blas_matrix_print(tmp);
					//qtk_blas_matrix_print(tmp2);
					qtk_blas_matrix_add_mat(bl,tmp2);
					for(j = 0; j < bl->row; j++)
					{
						wtk_relu2(bl->m + bl->col*j,bl->col);
					}
					block->mat = tmp2;
					//memcpy(tmp2->m,tmp1->m,sizeof(float)*tmp2->col*tmp2->row);
					layer->output = bl;
					//qtk_blas_matrix_print(layer->output);
					last_layer = layer;
				}else
				{
					tmp1 = wtk_robin_next(block->cache_rb);
					//qtk_blas_matrix_print(tmp1);
					qtk_blas_matrix_add_mat(bl,tmp1);
					for(j = 0; j < bl->row; j++)
					{
						wtk_relu2(bl->m + bl->col*j,bl->col);
					}
					//qtk_blas_matrix_print(tmp);
					//memcpy(tmp1->m,last_layer->output->m,sizeof(float)*tmp1->col*tmp1->row);
					layer->output = bl;
					last_layer = layer;
					//qtk_blas_matrix_print(layer->output);
				}
			}else{
				//qtk_blas_matrix_print(last_layer->output);
				tmp = qtk_torchnn_feed(block->mdl,last_layer->output->m,last_layer->output->col,last_layer->output->row,0);
				if(block->downsample)
				{
					//wtk_debug("process basic block down sample\n");
					tmp1 = qtk_torchnn_feed(block->mdl_down,last_layer->output->m,last_layer->output->col,last_layer->output->row,0);

					if(tmp)//2->1 delay 1frame
					{
						tmp2 = wtk_robin_next(block->cache_rb);
						//qtk_blas_matrix_print(tmp);
						//qtk_blas_matrix_print(tmp2);
						qtk_blas_matrix_add_mat(tmp,tmp2);
						for(j = 0; j < tmp->row; j++)
						{
							wtk_relu2(tmp->m + tmp->col*j,tmp->col);
						}
						block->mat = tmp2;
						//memcpy(tmp2->m,tmp1->m,sizeof(float)*tmp2->col*tmp2->row);
						layer->output = tmp;
						//qtk_blas_matrix_print(layer->output);
						last_layer = layer;
						//exit(0);
					}else
					{
						if(tmp1)//cache down sample 2->1
						{
							if(block->mat)
							{
								memcpy(block->mat->m,tmp1->m,sizeof(float)*block->mat->col*block->mat->row);
							}else
							{
								tmp2 = wtk_robin_next(block->cache_rb);
								memcpy(tmp2->m,tmp1->m,sizeof(float)*tmp2->col*tmp2->row);
							}
						}
						return NULL;
					}
				}else
				{
					//wtk_debug("block feed end\n");
					if(tmp)
					{
						//qtk_blas_matrix_print(tmp);
						tmp1 = wtk_robin_next(block->cache_rb);
						//qtk_blas_matrix_print(tmp1);
						qtk_blas_matrix_add_mat(tmp,tmp1);
						for(j = 0; j < tmp->row; j++)
						{
							wtk_relu2(tmp->m + tmp->col*j,tmp->col);
						}
						//qtk_blas_matrix_print(tmp);
						memcpy(tmp1->m,last_layer->output->m,sizeof(float)*tmp1->col*tmp1->row);
						layer->output = tmp;
						last_layer = layer;
						//qtk_blas_matrix_print(layer->output);
					}else
					{
						tmp1 = wtk_robin_next(block->cache_rb);
						memcpy(tmp1->m,last_layer->output->m,sizeof(float)*tmp1->col*tmp1->row);
						//wtk_debug("return\n");
						return NULL;
					}
					//exit(0);
				}
			}
			//exit(0);
			break;

		case QTK_TORCHNN_ACTIVATE:
			switch(layer->a_type)
			{
			case QTK_TANH:
				for(j = 0; j < last_layer->output->row; j++)
				{
					wtk_tanh(last_layer->output->m + last_layer->output->col*j,last_layer->output->col);
				}
				//qtk_blas_matrix_print(last_layer->output);
				break;
			case QTK_SIGMOID:
				for(j = 0; j < last_layer->output->row; j++)
				{
					wtk_sigmoid(last_layer->output->m + last_layer->output->col*j,last_layer->output->col);
				}
				//qtk_blas_matrix_print(last_layer->output);
				break;
			case QTK_RELU:
				for(j = 0; j < last_layer->output->row; j++)
				{
					wtk_relu2(last_layer->output->m + last_layer->output->col*j,last_layer->output->col);
				}
				break;
			case QTK_LOGSOFTMAX:
				for(j = 0; j < last_layer->output->row; j++)
				{
					wtk_torch_softmax(last_layer->output->m + last_layer->output->col*j,last_layer->output->col);
					wtk_add_log(last_layer->output->m + last_layer->output->col*j,last_layer->output->col);
				}
				//qtk_blas_matrix_print(last_layer->output);
				break;
			case QTK_SOFTMAX:
				for(j = 0; j < last_layer->output->row; j++)
				{
					wtk_torch_softmax(last_layer->output->m + last_layer->output->col*j,last_layer->output->col);
				}
				//qtk_blas_matrix_print(last_layer->output);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
	out = torchnn->layers[torchnn->nlayer-1]->output;
	if(!out)
	{
		out = torchnn->layers[torchnn->nlayer-2]->output;
	}

	//wtk_debug("torch nn end:%p\n",out);
	//qtk_blas_matrix_print(out);
	return out;
}

void qtk_torchnn_flush(qtk_torchnn_t *torchnn)//flush nn each layers when conv2d/conv1d padding 0
{
	qtk_blas_matrix_t *m;
	int i,j;
	qtk_torchnn_t *basic_mdl;
//	int len = in_row * in_col;
	qtk_torchnn_layer_t *layer;
	qtk_torchnn_conv2d_t *conv;
	qtk_torchnn_conv1d_t *conv1d;
//	qtk_torchnn_linear_t *linear;
//	qtk_torchnn_lstm_t *lstm;
//	qtk_torchnn_lstm_assit_t *lstma;
//	qtk_torchnn_maxpool_t *maxpool;
//	qtk_torchnn_avpool_t *avpool;
//	qtk_blas_matrix_t *m,*input,*tmp,*out,*tmp1=NULL,*tmp2;
//	qtk_torchnn_batch_t *batch;
//	qtk_torchnn_se_t *se;
	qtk_torchnn_block_t *block;

	for(i = 0; i < torchnn->nlayer; i++)
	{
		layer = torchnn->layers[i];
		//wtk_debug("==========flush layer:%d %p=============\n",i,layer);
		switch(layer->type)
		{
		case QTK_TORCHNN_CONV2D:
			conv = (qtk_torchnn_conv2d_t*)layer->mdl;
			for(j=0;j<conv->padding_row;j++)
			{
				//wtk_debug("-------------------- %p\n",layer);
				m = qtk_torchnn_feed_flush(torchnn,NULL,0,0,i,NULL);
				if(m)
				{
					torchnn->notify(torchnn->ths,m);
				}
				//notify other module
			}
			break;
		case QTK_TORCHNN_CONV1D:
			conv1d = (qtk_torchnn_conv1d_t*)layer->mdl;
			for(j=0;j<conv1d->padding_row;j++)
			{
				//wtk_debug("-------------------- %p\n",layer);
				m = qtk_torchnn_feed_flush(torchnn,NULL,0,0,i,NULL);
				if(m)
				{
					torchnn->notify(torchnn->ths,m);
				}
				//notify other module
			}
			break;
		case QTK_TORCHNN_BASIC:
			block = (qtk_torchnn_block_t*)layer->mdl;
			basic_mdl = block->mdl;
			m = qtk_torchnn_feed_flush(basic_mdl,NULL,0,0,0,NULL);
			if(m)
			{
				m = qtk_torchnn_feed_flush(torchnn,NULL,0,0,i,m);
				if(m)
				{
					torchnn->notify(torchnn->ths,m);
				}
			}
			m = qtk_torchnn_feed_flush(basic_mdl,NULL,0,0,3,NULL);
			if(m)
			{
				m = qtk_torchnn_feed_flush(torchnn,NULL,0,0,i,m);
				if(m)
				{
					torchnn->notify(torchnn->ths,m);
				}
			}
			break;
		default:
			break;
		}
	}

	if(torchnn->notify_end)
	{
		torchnn->notify_end(torchnn->ths);
	}

	//qtk_torchnn_feed(qtk_torchnn_t *torchnn,float *f,int in_col, int in_row,int layer_index)
}

