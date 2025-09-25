#include "wtk_cnnnet_layer.h" 

void wtk_cnnnet_layer_update_wei(wtk_cnnnet_layer_t *layer,wtk_source_t *src)
{
	int i;
	int in=layer->kernel_size[1];
	int ou=layer->out_channel*layer->in_channel*layer->kernel_size[0];
	
	wtk_source_read_char(src);
	for(i=0;i<ou;++i)
	{
		wtk_source_read_float_little(src,layer->weight[i],in,1);
	}
}

void wtk_cnnnet_layer_update_bias(wtk_cnnnet_layer_t *layer,wtk_source_t *src)
{
	int ou=layer->out_channel;

	wtk_source_read_char(src);
	wtk_source_read_float_little(src,layer->bias,ou,1);
}


wtk_cnnnet_layer_t * wtk_cnnnet_layer_new(wtk_source_t *src, wtk_strbuf_t *buf)
{
	int ret;
	int in,ou;
	int i;
	wtk_cnnnet_layer_t *cnn;

    cnn=(wtk_cnnnet_layer_t *)wtk_malloc(sizeof(wtk_cnnnet_layer_t));
    cnn->use_no_nonlinearity=0;
    cnn->use_relu=0;
    cnn->use_prelu=0;
	cnn->use_tanh=0;
    cnn->use_sigmoid=0;
    cnn->use_bias=0;

    cnn->weight=NULL;
    cnn->bias=NULL;
	cnn->prelu_w=0;

	ret = wtk_source_read_string(src, buf);
	if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_zero_bias"))
	{
		cnn->use_bias=0;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_bias"))
	{
		cnn->use_bias=1;
	}else
	{
		wtk_debug("error: no use_bias \n");ret=-1;goto end;
	}

	ret = wtk_source_read_string(src, buf);
	if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_no_nonlinearity"))
	{
		cnn->use_no_nonlinearity=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_sigmoid"))
	{
		cnn->use_sigmoid=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_relu"))
	{
		cnn->use_relu=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_prelu"))
	{
		cnn->use_prelu=1;
	}else if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"use_tanh"))
	{
		cnn->use_tanh=1;
	}else
	{
		wtk_debug("error: no nonlinearity msg\n");ret=-1;goto end;
	}

	if(cnn->use_prelu){
		wtk_source_read_float(src, &cnn->prelu_w, 1, 0);
	}

	wtk_source_atoi(src, &(cnn->in_channel));
	wtk_source_atoi(src, &(cnn->out_channel));
	wtk_source_atoi(src, &(cnn->zeropad2d[0]));
	wtk_source_atoi(src, &(cnn->zeropad2d[1]));
	wtk_source_atoi(src, &(cnn->zeropad2d[2]));
	wtk_source_atoi(src, &(cnn->zeropad2d[3]));
	wtk_source_atoi(src, &(cnn->kernel_size[0]));
	wtk_source_atoi(src, &(cnn->kernel_size[1]));
	wtk_source_atoi(src, &(cnn->dilation[0]));
	wtk_source_atoi(src, &(cnn->dilation[1]));
	wtk_source_atoi(src, &(cnn->stride[0]));
	wtk_source_atoi(src, &(cnn->stride[1]));

	ret = wtk_source_read_string(src, buf);
	if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"weight"))
	{
		wtk_source_atoi(src, &ou);
		// printf("ou = %d ",ou);
		wtk_source_atoi(src, &in);
		// printf("in = %d ",in);
		if(ou !=  cnn->out_channel*cnn->in_channel*cnn->kernel_size[0])
		{
			wtk_debug("error :in error ou = %d  out_channel = %d, in_channel =%d, kernel_size=[%d %d]\n",
																		in,cnn->out_channel,cnn->in_channel,cnn->kernel_size[0],cnn->kernel_size[1]);
			ret=-1; goto end;
		}


		cnn->weight=(float **)wtk_malloc(sizeof(float *)*ou);
		for(i=0; i<ou; ++i)
		{
			cnn->weight[i]=(float *)wtk_malloc(sizeof(float)*cnn->kernel_size[1]);
		}

        if(cnn->use_bias)
        {
            cnn->bias=(float *)wtk_malloc(sizeof(float)*cnn->out_channel);
        }

		wtk_cnnnet_layer_update_wei(cnn, src);
		if(cnn->use_bias)
		{
			ret = wtk_source_read_string(src, buf);
			if(ret==0 && wtk_str_equal_s(buf->data, buf->pos,"bias"))
			{
				wtk_source_atoi(src, &ou);
				if(ou != cnn->out_channel)
				{
					wtk_debug("error: bias ou != out_channel   \n");ret=-1;goto end;
				}
				wtk_cnnnet_layer_update_bias(cnn, src);
			}else
			{
				wtk_debug("error: no bias \n");ret=-1;goto end;
			}
		}
		// printf("\n");
	}else
	{
		wtk_debug("error: no weight \n");ret=-1;goto end;
	}

	ret=0;
end:
    if(ret!=0)
    {
        wtk_cnnnet_layer_delete(cnn);
        cnn=NULL;
    }
	return cnn;
}

void wtk_cnnnet_layer_delete(wtk_cnnnet_layer_t *layer)
{
	int i;

	if(layer->weight)
	{
		for(i=0; i<layer->out_channel*layer->in_channel*layer->kernel_size[0]; ++i)
		{
			wtk_free(layer->weight[i]);
		}
		wtk_free(layer->weight);
	}
	if(layer->bias)
	{
		wtk_free(layer->bias);
	}
	wtk_free(layer);
}
