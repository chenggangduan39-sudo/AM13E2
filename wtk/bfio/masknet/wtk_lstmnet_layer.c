#include "wtk_lstmnet_layer.h"

wtk_lstmnet_wb_t *wtk_lstmnet_wb_new(int lstm_depth, int lstm_hidden, int input_dim)
{
	wtk_lstmnet_wb_t * layer;
	int i,j;

	layer=(wtk_lstmnet_wb_t *)wtk_malloc(sizeof(wtk_lstmnet_wb_t)*lstm_depth);
	for(i=0;i<lstm_depth;++i)
	{

		layer[i].weight_ih_l=(float **)wtk_malloc(sizeof(float *)*lstm_hidden*4);
		layer[i].weight_hh_l=(float **)wtk_malloc(sizeof(float *)*lstm_hidden*4);
		layer[i].bias_ih_l=(float *)wtk_malloc(sizeof(float)*lstm_hidden*4);
		layer[i].bias_hh_l=(float *)wtk_malloc(sizeof(float)*lstm_hidden*4);
#ifdef USE_NEON
		layer[i].lstm_tmp1=(float *)wtk_malloc(sizeof(float)*lstm_hidden);  ////
		layer[i].lstm_tmp2=(float *)wtk_malloc(sizeof(float)*lstm_hidden);  //// 
#endif
		if(i==0)
		{
			layer[i].input_idim=input_dim;
			layer[i].output_idim=lstm_hidden*4;
			layer[i].input_hdim=lstm_hidden;
			layer[i].output_hdim=lstm_hidden*4;

			for(j=0;j<lstm_hidden*4;++j)
			{
				layer[i].weight_ih_l[j]=(float *)wtk_malloc(sizeof(float)*input_dim);
			}
#ifdef USE_NEON
			layer[i].weight_i_l=(float *)wtk_malloc(sizeof(float *)*lstm_hidden*4*input_dim);  //// 
#endif
		}else
		{
			layer[i].input_idim=lstm_hidden;
			layer[i].output_idim=lstm_hidden*4;
			layer[i].input_hdim=lstm_hidden;
			layer[i].output_hdim=lstm_hidden*4;

			for(j=0;j<lstm_hidden*4;++j)
			{
				layer[i].weight_ih_l[j]=(float *)wtk_malloc(sizeof(float)*lstm_hidden);
			}
#ifdef USE_NEON
			layer[i].weight_i_l=(float *)wtk_malloc(sizeof(float *)*lstm_hidden*4*lstm_hidden);  //// 
#endif
		}
		for(j=0;j<lstm_hidden*4;++j)
		{
			layer[i].weight_hh_l[j]=(float *)wtk_malloc(sizeof(float)*lstm_hidden);
		}
#ifdef USE_NEON
		layer[i].weight_h_l=(float *)wtk_malloc(sizeof(float *)*lstm_hidden*4*lstm_hidden);  //// 
#endif
	}

	return layer;
}

void wtk_lstmnet_wb_delete(wtk_lstmnet_wb_t *layer,int lstm_depth)
{
	int i,j;
	int len=layer[0].output_hdim;

	for(i=0;i<lstm_depth;++i)
	{
		for(j=0;j<len;++j)
		{
			wtk_free(layer[i].weight_ih_l[j]);
			wtk_free(layer[i].weight_hh_l[j]);
		}
		wtk_free(layer[i].weight_ih_l);
		wtk_free(layer[i].weight_hh_l);
		wtk_free(layer[i].bias_ih_l);
		wtk_free(layer[i].bias_hh_l);
#ifdef USE_NEON
		wtk_free(layer[i].weight_i_l);  //// 
		wtk_free(layer[i].weight_h_l);  //// 
		wtk_free(layer[i].lstm_tmp1);  //// 
		wtk_free(layer[i].lstm_tmp2);
#endif
	}
	wtk_free(layer);
}

void wtk_lstmnet_wb_update_wei(float **wei,int in, int ou,wtk_source_t *src)
{
	int i;

	wtk_source_read_char(src);
	for(i=0;i<ou;++i)
	{
		wtk_source_read_float_little(src,wei[i],in,1);
	}
}

void wtk_lstmnet_wb_update_bias(float *bias, int ou,wtk_source_t *src)
{
	wtk_source_read_char(src);
	wtk_source_read_float_little(src,bias,ou,1);
}

#ifdef USE_NEON
static void wtk_lstmnet_data_rearrangement(float **in, float *out, int col, int rowx4, int len)
{
    int row_1, col_1;
    int i,j,k;
    int pos;
    int pos_x,pos_y;
	int row;
	row=rowx4/4;
    pos=0;
    pos_x=0;
    pos_y=0;
    // len=8;
	for(i=0;i<4;i++){
    	row_1=row;
		while(row_1 >= 4){
			col_1=col;
			while(col_1 >= len){
				for(j=0;j<4;++j){
					for(k=0;k<len;++k){
						out[pos++]=in[pos_x][pos_y++];
					}
					++pos_x;
					pos_y-=len;
				}
				pos_x-=4;
				pos_y+=len;
				col_1-=len;
			}

			if(col_1 >= 4){
				for(j=0;j<4;++j){
					for(k=0;k<4;++k){
						out[pos++]=in[pos_x][pos_y++];
					}
					++pos_x;
					pos_y-=4;
				}
				pos_x-=4;
				pos_y+=4;
				col_1-=4;
			}

			if(col_1 > 0){
				for(k=0;k<col_1;++k){
					for(j=0;j<4;++j){
						out[pos++]=in[pos_x++][pos_y];
					}
					++pos_y;
					pos_x-=4;
				}
			}
			pos_x+=4;
			pos_y=0;
			row_1 -= 4;
		}
		if(row_1>0){
			for(j=0;j<row_1;++j){
				for(k=0;k<col;++k){
					out[pos++]=in[pos_x][pos_y++];
				}
				pos_y-=col;
				++pos_x;
			}
		}
	}
}
#endif

wtk_lstmnet_layer_t * wtk_lstmnet_layer_new(wtk_source_t *src, wtk_strbuf_t *buf)
{
    wtk_lstmnet_layer_t *layer;
    int input_dim,lstm_hidden,lstm_depth;
    int ret;
    int i;
    int in,ou;

    wtk_source_atoi(src, &input_dim);
    wtk_source_atoi(src, &lstm_hidden);
    wtk_source_atoi(src, &lstm_depth);
    layer=(wtk_lstmnet_layer_t *)wtk_malloc(sizeof(wtk_lstmnet_layer_t));
    layer->input_dim=input_dim;
    layer->lstm_hidden=lstm_hidden;
    layer->lstm_depth=lstm_depth;
    layer->lstm_wb=wtk_lstmnet_wb_new(lstm_depth,lstm_hidden,input_dim);

	layer->use_no_nonlinearity=0;
	layer->use_relu=0;
	layer->use_prelu=0;
	layer->use_sigmoid=0;
	layer->use_tanh=0;
	ret = wtk_source_read_string(src, buf);
	if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_no_nonlinearity"))
	{
		layer->use_no_nonlinearity=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_sigmoid"))
	{
		layer->use_sigmoid=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_relu"))
	{
		layer->use_relu=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_prelu"))
	{
		layer->use_prelu=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_tanh"))
	{
		layer->use_tanh=1;
	}else
	{
		wtk_debug("error: no nonlinearity msg\n");goto end;
	}

	if(layer->use_prelu){
		wtk_source_read_float(src, &layer->prelu_w, 1, 0);
	}

	for(i=0;i<lstm_depth;++i)
	{
		ret = wtk_source_read_string(src, buf);
		if(ret != 0)
		{
			wtk_debug("error: empty lstm\n");
			goto end;
		}
		// wtk_debug("lstm_layer%d:  ",i);
		if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"weight_ih_l"))
		{
			wtk_source_atoi(src, &ou);
			// printf("weight_ih_l  ou = %d ",ou);
			wtk_source_atoi(src, &in);
			// printf("in = %d ",in);
			wtk_lstmnet_wb_update_wei(layer->lstm_wb[i].weight_ih_l,in,ou,src);
			ret = wtk_source_read_string(src, buf);
			if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"bias_ih_l"))
			{
				wtk_source_atoi(src, &ou);
				// printf("bias_ih_l ou = %d ",ou);
				if(ou != layer->lstm_wb[i].output_idim)
				{
					wtk_debug("error: bias ou != output_idim   \n");ret=-1;goto end;
				}
				wtk_lstmnet_wb_update_bias(layer->lstm_wb[i].bias_ih_l, ou, src);
			}else
			{
				wtk_debug("error: no bias_ih_l \n");ret=-1;goto end;
			}
#ifdef USE_NEON
			wtk_lstmnet_data_rearrangement(layer->lstm_wb[i].weight_ih_l, layer->lstm_wb[i].weight_i_l, input_dim, lstm_hidden*4, 4);  //// 
#endif
			// printf("\n");
		}else
		{
			wtk_debug("error: no weight_ih_l \n");ret=-1;goto end;
		}
		ret = wtk_source_read_string(src, buf);
		if(ret != 0)
		{
			wtk_debug("error: empty lstm\n");
			goto end;
		}
		if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"weight_hh_l"))
		{
			wtk_source_atoi(src, &ou);
			// printf("weight_hh_l ou = %d ",ou);
			wtk_source_atoi(src, &in);
			// printf("in = %d ",in);
			wtk_lstmnet_wb_update_wei(layer->lstm_wb[i].weight_hh_l,in,ou,src);
			ret = wtk_source_read_string(src, buf);
			if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"bias_hh_l"))
			{
				wtk_source_atoi(src, &ou);
				// printf("bias ou = %d ",ou);
				if(ou != layer->lstm_wb[i].output_hdim)
				{
					wtk_debug("error: bias ou != output_hdim   \n");ret=-1;goto end;
				}
				wtk_lstmnet_wb_update_bias(layer->lstm_wb[i].bias_hh_l, ou, src);
			}else
			{
				wtk_debug("error: no bias_hh_l \n");ret=-1;goto end;
			}
#ifdef USE_NEON
			wtk_lstmnet_data_rearrangement(layer->lstm_wb[i].weight_hh_l, layer->lstm_wb[i].weight_h_l, lstm_hidden, lstm_hidden*4, 4);  //// 
#endif
			// printf("\n");
		}else
		{
			wtk_debug("error: no weight_hh_l \n");goto end;
		}
	}

    ret=0;
end:
    if(ret!=0)
    {
        wtk_lstmnet_layer_delete(layer);
        layer=NULL;
    }
	return layer;
}

void wtk_lstmnet_layer_delete(wtk_lstmnet_layer_t *layer)
{
    wtk_lstmnet_wb_delete(layer->lstm_wb, layer->lstm_depth);
    wtk_free(layer);
}