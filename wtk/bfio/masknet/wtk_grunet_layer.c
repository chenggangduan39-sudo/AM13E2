#include "wtk_grunet_layer.h"

wtk_grunet_wb_t *wtk_grunet_wb_new(int gru_depth, int gru_hidden, int input_dim)
{
	wtk_grunet_wb_t * layer;
	int i,j;

	layer=(wtk_grunet_wb_t *)wtk_malloc(sizeof(wtk_grunet_wb_t)*gru_depth);
	for(i=0;i<gru_depth;++i)
	{

		layer[i].weight_ih_l=(float **)wtk_malloc(sizeof(float *)*gru_hidden*3);
		layer[i].weight_hh_l=(float **)wtk_malloc(sizeof(float *)*gru_hidden*3);
		layer[i].bias_ih_l=(float *)wtk_malloc(sizeof(float)*gru_hidden*3);
		layer[i].bias_hh_l=(float *)wtk_malloc(sizeof(float)*gru_hidden*3);
#ifdef USE_NEON
		layer[i].gru_tmp1=(float *)wtk_malloc(sizeof(float)*gru_hidden);  ////
		layer[i].gru_tmp2=(float *)wtk_malloc(sizeof(float)*gru_hidden);  //// 
#endif

		if(i==0)
		{
			layer[i].input_idim=input_dim;
			layer[i].output_idim=gru_hidden*3;
			layer[i].input_hdim=gru_hidden;
			layer[i].output_hdim=gru_hidden*3;

			for(j=0;j<gru_hidden*3;++j)
			{
				layer[i].weight_ih_l[j]=(float *)wtk_malloc(sizeof(float)*input_dim);
			}
#ifdef USE_NEON
			layer[i].weight_i_l=(float *)wtk_malloc(sizeof(float *)*gru_hidden*3*input_dim);  //// 
#endif
		}else
		{
			layer[i].input_idim=gru_hidden;
			layer[i].output_idim=gru_hidden*3;
			layer[i].input_hdim=gru_hidden;
			layer[i].output_hdim=gru_hidden*3;

			for(j=0;j<gru_hidden*3;++j)
			{
				layer[i].weight_ih_l[j]=(float *)wtk_malloc(sizeof(float)*gru_hidden);
			}
#ifdef USE_NEON
			layer[i].weight_i_l=(float *)wtk_malloc(sizeof(float *)*gru_hidden*3*gru_hidden);  //// 
#endif
		}
		for(j=0;j<gru_hidden*3;++j)
		{
			layer[i].weight_hh_l[j]=(float *)wtk_malloc(sizeof(float)*gru_hidden);
		}
#ifdef USE_NEON
		layer[i].weight_h_l=(float *)wtk_malloc(sizeof(float *)*gru_hidden*3*gru_hidden);  //// 
#endif
	}

	return layer;
}

void wtk_grunet_wb_delete(wtk_grunet_wb_t *layer,int gru_depth)
{
	int i,j;
	int len=layer[0].output_hdim;

	for(i=0;i<gru_depth;++i)
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
		wtk_free(layer[i].gru_tmp1);  //// 
		wtk_free(layer[i].gru_tmp2);
#endif
	}
	wtk_free(layer);
}

void wtk_grunet_wb_update_wei(float **wei,int in, int ou,wtk_source_t *src)
{
	int i;

	wtk_source_read_char(src);
	for(i=0;i<ou;++i)
	{
		wtk_source_read_float_little(src,wei[i],in,1);
	}
}

void wtk_grunet_wb_update_bias(float *bias, int ou,wtk_source_t *src)
{
	wtk_source_read_char(src);
	wtk_source_read_float_little(src,bias,ou,1);
}

#ifdef USE_NEON
static void wtk_grunet_data_rearrangement(float **in, float *out, int col, int rowx3, int len)
{
    int row_1, col_1;
    int i,j,k;
    int pos;
    int pos_x,pos_y;
	int row;
	row=rowx3/3;
    pos=0;
    pos_x=0;
    pos_y=0;
    // len=8;
	for(i=0;i<3;i++){
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

wtk_grunet_layer_t * wtk_grunet_layer_new(wtk_source_t *src, wtk_strbuf_t *buf)
{
    wtk_grunet_layer_t *layer;
    int input_dim,gru_hidden,gru_depth;
    int ret;
    int i;
    int in,ou;

    wtk_source_atoi(src, &input_dim);
    wtk_source_atoi(src, &gru_hidden);
    wtk_source_atoi(src, &gru_depth);
    layer=(wtk_grunet_layer_t *)wtk_malloc(sizeof(wtk_grunet_layer_t));
    layer->input_dim=input_dim;
    layer->gru_hidden=gru_hidden;
    layer->gru_depth=gru_depth;
    layer->gru_wb=wtk_grunet_wb_new(gru_depth,gru_hidden,input_dim);

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

	for(i=0;i<gru_depth;++i)
	{
		ret = wtk_source_read_string(src, buf);
		if(ret != 0)
		{
			wtk_debug("error: empty gru\n");
			goto end;
		}
		// wtk_debug("gru_layer%d:  ",i);
		if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"weight_ih_l"))
		{
			wtk_source_atoi(src, &ou);
			// printf("weight_ih_l  ou = %d ",ou);
			wtk_source_atoi(src, &in);
			// printf("in = %d ",in);
			wtk_grunet_wb_update_wei(layer->gru_wb[i].weight_ih_l,in,ou,src);
			ret = wtk_source_read_string(src, buf);
			if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"bias_ih_l"))
			{
				wtk_source_atoi(src, &ou);
				// printf("bias_ih_l ou = %d ",ou);
				if(ou != layer->gru_wb[i].output_idim)
				{
					wtk_debug("error: bias ou != output_idim   \n");ret=-1;goto end;
				}
				wtk_grunet_wb_update_bias(layer->gru_wb[i].bias_ih_l, ou, src);
			}else
			{
				wtk_debug("error: no bias_ih_l \n");ret=-1;goto end;
			}
#ifdef USE_NEON
			wtk_grunet_data_rearrangement(layer->gru_wb[i].weight_ih_l, layer->gru_wb[i].weight_i_l, input_dim, gru_hidden*3, 4);  //// 
#endif
			// printf("\n");
		}else
		{
			wtk_debug("error: no weight_ih_l \n");ret=-1;goto end;
		}
		ret = wtk_source_read_string(src, buf);
		if(ret != 0)
		{
			wtk_debug("error: empty gru\n");
			goto end;
		}
		if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"weight_hh_l"))
		{
			wtk_source_atoi(src, &ou);
			// printf("weight_hh_l ou = %d ",ou);
			wtk_source_atoi(src, &in);
			// printf("in = %d ",in);
			wtk_grunet_wb_update_wei(layer->gru_wb[i].weight_hh_l,in,ou,src);
			ret = wtk_source_read_string(src, buf);
			if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"bias_hh_l"))
			{
				wtk_source_atoi(src, &ou);
				// printf("bias ou = %d ",ou);
				if(ou != layer->gru_wb[i].output_hdim)
				{
					wtk_debug("error: bias ou != output_hdim   \n");ret=-1;goto end;
				}
				wtk_grunet_wb_update_bias(layer->gru_wb[i].bias_hh_l, ou, src);
			}else
			{
				wtk_debug("error: no bias_hh_l \n");ret=-1;goto end;
			}
#ifdef USE_NEON
			wtk_grunet_data_rearrangement(layer->gru_wb[i].weight_hh_l, layer->gru_wb[i].weight_h_l, gru_hidden, gru_hidden*3, 4);  //// 
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
        wtk_grunet_layer_delete(layer);
        layer=NULL;
    }
	return layer;
}

void wtk_grunet_layer_delete(wtk_grunet_layer_t *layer)
{
    wtk_grunet_wb_delete(layer->gru_wb, layer->gru_depth);
    wtk_free(layer);
}
