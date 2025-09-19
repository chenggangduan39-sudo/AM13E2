#include "wtk_dnnnet_layer.h" 

void wtk_dnnnet_layer_update_wei(wtk_dnnnet_layer_t *layer,wtk_source_t *src)
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

void wtk_dnnnet_layer_update_bias(wtk_dnnnet_layer_t *layer,wtk_source_t *src)
{
	int ou=layer->output_dim;
	
	wtk_source_read_char(src);
	wtk_source_read_float_little(src,layer->bias,ou,1);
}

wtk_dnnnet_layer_t * wtk_dnnnet_layer_new(wtk_source_t *src, wtk_strbuf_t *buf)
{
    int ret;
	int in,ou,i;
	wtk_dnnnet_layer_t *dnn;

    dnn=(wtk_dnnnet_layer_t *)wtk_malloc(sizeof(wtk_dnnnet_layer_t));
    dnn->use_no_nonlinearity=0;
    dnn->use_relu=0;
    dnn->use_prelu=0;
    dnn->use_sigmoid=0;
	dnn->use_tanh=0;
	dnn->use_log_softmax=0;
    dnn->use_bias=0;
    dnn->weight=NULL;
    dnn->bias=NULL;
	dnn->prelu_w=0;

	ret = wtk_source_read_string(src, buf);
	if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_zero_bias"))
	{
		dnn->use_bias=0;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_bias"))
	{
		dnn->use_bias=1;
	}else
	{
		wtk_debug("error: no use_bias \n");goto end;
	}

	ret = wtk_source_read_string(src, buf);
	if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_no_nonlinearity"))
	{
		dnn->use_no_nonlinearity=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_sigmoid"))
	{
		dnn->use_sigmoid=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_relu"))
	{
		dnn->use_relu=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_prelu"))
	{
		dnn->use_prelu=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_tanh"))
	{
		dnn->use_tanh=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_log_softmax"))
	{
		dnn->use_log_softmax=1;
	}else
	{
		wtk_debug("error: no nonlinearity msg\n");goto end;
	}
	if(dnn->use_prelu)
	{
		wtk_source_read_float(src, &dnn->prelu_w, 1, 0);
	}

	ret = wtk_source_read_string(src, buf);
	if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"weight"))
	{
		wtk_source_atoi(src, &ou);

		wtk_source_atoi(src, &in);

        dnn->input_dim=in;
        dnn->output_dim=ou;

        dnn->weight=(float **)wtk_malloc(sizeof(float *)*ou);
        for(i=0;i<ou;++i)
        {
            dnn->weight[i]=(float *)wtk_malloc(sizeof(float)*in);
        }
        if(dnn->use_bias)
        {
            dnn->bias=(float *)wtk_malloc(sizeof(float)*ou);
        }

		wtk_dnnnet_layer_update_wei(dnn, src);
		if(dnn->use_bias)
		{
			ret = wtk_source_read_string(src, buf);
			if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"bias"))
			{
				wtk_source_atoi(src, &ou);
				if(ou != dnn->output_dim)
				{
					wtk_debug("error: bias ou != output_dim   \n");ret=-1;goto end;
				}
				wtk_dnnnet_layer_update_bias(dnn, src);
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
        wtk_dnnnet_layer_delete(dnn);
        dnn=NULL;
    }
	return dnn;
}

void wtk_dnnnet_layer_delete(wtk_dnnnet_layer_t *dnn)
{
	int i;

    if(dnn->weight)
    {
        for(i=0;i<dnn->output_dim;++i)
        {
            wtk_free(dnn->weight[i]);
        }
        wtk_free(dnn->weight);
    }
	if(dnn->bias)
	{
		wtk_free(dnn->bias);
	}
	wtk_free(dnn);
}