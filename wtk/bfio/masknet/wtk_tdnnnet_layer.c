#include "wtk_tdnnnet_layer.h" 

void wtk_tdnnnet_layer_update_wei(wtk_tdnnnet_layer_t *layer,wtk_source_t *src)
{
	int i;
	int in=layer->input_dim;
	int ou=layer->output_dim;

	wtk_source_read_char(src);
	for(i=0;i<ou;++i)
	{
		wtk_source_read_float_little(src,layer->weight[i],in,1);
	}
}

void wtk_tdnnnet_layer_update_bias(wtk_tdnnnet_layer_t *layer,wtk_source_t *src)
{
	int ou=layer->output_dim;

	wtk_source_read_char(src);
	wtk_source_read_float_little(src,layer->bias,ou,1);
}

wtk_tdnnnet_layer_t * wtk_tdnnnet_layer_new(wtk_source_t *src, wtk_strbuf_t *buf)
{
    int ret;
	int left_context,right_context,dilation;
	int in,ou,i;
	wtk_tdnnnet_layer_t *tdnn;

    tdnn=(wtk_tdnnnet_layer_t *)wtk_malloc(sizeof(wtk_tdnnnet_layer_t));
    tdnn->use_no_nonlinearity=0;
    tdnn->use_relu=0;
    tdnn->use_prelu=0;
	tdnn->use_tanh=0;
    tdnn->use_sigmoid=0;
    tdnn->use_bias=0;
    tdnn->weight=NULL;
    tdnn->bias=NULL;

	ret = wtk_source_read_string(src, buf);
	if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_zero_bias"))
	{
		tdnn->use_bias=0;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_bias"))
	{
		tdnn->use_bias=1;
	}else
	{
		wtk_debug("error: no use_bias \n");goto end;
	}

	ret = wtk_source_read_string(src, buf);
	if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_no_nonlinearity"))
	{
		tdnn->use_no_nonlinearity=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_sigmoid"))
	{
		tdnn->use_sigmoid=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_relu"))
	{
		tdnn->use_relu=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_prelu"))
	{
		tdnn->use_prelu=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_tanh"))
	{
		tdnn->use_tanh=1;
	}else
	{
		wtk_debug("error: no nonlinearity msg\n");goto end;
	}

	if(tdnn->use_prelu){
		wtk_source_read_float(src, &tdnn->prelu_w, 1, 0);
	}

	wtk_source_atoi(src, &left_context);
    tdnn->left_context=left_context;

	wtk_source_atoi(src, &right_context);
    tdnn->right_context=right_context;

	wtk_source_atoi(src, &dilation);
    tdnn->dilation=dilation;

	ret = wtk_source_read_string(src, buf);
	if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"weight"))
	{
		wtk_source_atoi(src, &ou);
		// printf("ou = %d ",ou);
		wtk_source_atoi(src, &in);
		// printf("in = %d ",in);
		if(in % (left_context+right_context+1) !=0)
		{
			wtk_debug("error :in error in = %d  left_context = %d, right_context =%d\n",in,left_context,right_context);
		}
        tdnn->input_dim=in;
        tdnn->output_dim=ou;

        tdnn->weight=(float **)wtk_malloc(sizeof(float *)*ou);
        for(i=0;i<ou;++i)
        {
            tdnn->weight[i]=(float *)wtk_malloc(sizeof(float)*tdnn->input_dim);
        }

        if(tdnn->use_bias)
        {
            tdnn->bias=(float *)wtk_malloc(sizeof(float)*ou);
        }

		wtk_tdnnnet_layer_update_wei(tdnn, src);
		if(tdnn->use_bias)
		{
			ret = wtk_source_read_string(src, buf);
			if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"bias"))
			{
				wtk_source_atoi(src, &ou);
				if(ou != tdnn->output_dim)
				{
					wtk_debug("error: bias ou != output_dim   \n");ret=-1;goto end;
				}
				wtk_tdnnnet_layer_update_bias(tdnn, src);
			}else
			{
				wtk_debug("error: no bias \n");ret=-1;goto end;
			}
		}
		// printf("\n");
	}else
	{
		wtk_debug("error: no weight \n");goto end;
	}

	ret=0;
end:
    if(ret!=0)
    {
        wtk_tdnnnet_layer_delete(tdnn);
        tdnn=NULL;
    }
	return tdnn;
}

void wtk_tdnnnet_layer_delete(wtk_tdnnnet_layer_t *tdnn)
{
	int i;

    if(tdnn->weight)
    {
        for(i=0;i<tdnn->output_dim;++i)
        {
            wtk_free(tdnn->weight[i]);
        }
        wtk_free(tdnn->weight);
    }
	if(tdnn->bias)
	{
		wtk_free(tdnn->bias);
	}
	wtk_free(tdnn);
}