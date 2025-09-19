/*
 * qtk_vits_onnx.c
 *
 *  Created on: Sep 7, 2022
 *      Author: dm
 */
#include "qtk_vits_onnx.h"
#include "wtk/core/wtk_os.h"

qtk_vits_onnx_t* qtk_vits_onnx_new(qtk_vits_onnx_cfg_t* cfg)
{
	qtk_vits_onnx_t *onnx;
    OrtStatus *status=NULL;
//    OrtTypeInfo *typeinfo;
//    OrtTensorTypeAndShapeInfo *shapeinfo;
    const OrtApiBase* base = OrtGetApiBase();
    const OrtApi *api;
    int i;


    if (cfg->onnx_fn==NULL)
    	return NULL;
    onnx = (qtk_vits_onnx_t*)wtk_calloc(1, sizeof(*onnx));
    onnx->base = base;
    api = base->GetApi(ORT_API_VERSION);
    if (!api)
    {
    	 fprintf(stderr, "Failed to init ONNX Runtime engine.\n");
    	 goto end;
    }

    onnx->api = api;
    //initial ortEnv
    status = api->CreateEnv(ORT_LOGGING_LEVEL_ERROR, "test",&onnx->env);
    if (NULL!=status)goto end;
    //initial session options
    status = api->CreateSessionOptions(&onnx->sop);
    if (NULL!=status)goto end;
    status = api->SetInterOpNumThreads(onnx->sop, cfg->nthread);
    if (NULL!=status)goto end;
    status = api->SetSessionGraphOptimizationLevel(onnx->sop,ORT_ENABLE_EXTENDED);
    if (NULL!=status)goto end;

    //Initial session
    if (cfg->use_bin)
    	status = api->CreateSessionFromArray(onnx->env,cfg->onnx_data, cfg->onnx_data_len,onnx->sop,&onnx->session);
    else
    {
#ifdef WIN32
	TCHAR* onnx_fn = wtk_mul_to_wchar(cfg->onnx_fn);
	status = api->CreateSession(onnx->env, onnx_fn, onnx->sop, &onnx->session);
	wtk_free(onnx_fn);
#else
    	status = api->CreateSession(onnx->env,cfg->onnx_fn,onnx->sop,&onnx->session);
#endif
    }
    if (NULL!=status)goto end;

    //input_tensor
    status = api->CreateCpuMemoryInfo(OrtArenaAllocator,OrtMemTypeDefault,&onnx->meminfo);
    if (NULL!=status)goto end;
    status = api->CreateAllocator(onnx->session,onnx->meminfo,&onnx->allocator);
    if (NULL!=status)goto end;

    status = api->SessionGetInputCount(onnx->session,&onnx->num_in);
    if (NULL!=status)goto end;
    status = api->SessionGetOutputCount(onnx->session,&onnx->num_out);
    if (NULL!=status)goto end;


//    onnx->dim_in = wtk_calloc(onnx->num_in, sizeof(*(onnx->dim_in)));
//    for(i = 0; i < onnx->num_in; i++)
//    {
//        status = api->SessionGetInputTypeInfo(onnx->session,i, &typeinfo);
//        if (NULL!=status)goto end;
//        status = api->CastTypeInfoToTensorInfo(typeinfo, (const OrtTensorTypeAndShapeInfo**)&shapeinfo);
//        if (NULL!=status)goto end;
//    	status = api->GetDimensionsCount(shapeinfo, &onnx->dim_in[i]);
//    	if (NULL!=status)goto end;
//    	api->ReleaseTypeInfo(typeinfo);
//    	wtk_debug("dim=%d\n", onnx->dim_in[i]);
//    }
//    onnx->dim_out = wtk_calloc(onnx->num_out, sizeof(*(onnx->dim_out)));
//    for(i = 0; i < onnx->num_out; i++)
//    {
//        status = api->SessionGetOutputTypeInfo(onnx->session,i, &typeinfo);
//        if (NULL!=status)goto end;
//        status = api->CastTypeInfoToTensorInfo(typeinfo, (const OrtTensorTypeAndShapeInfo**)&shapeinfo);
//        if (NULL!=status)goto end;
//    	status = api->GetDimensionsCount(shapeinfo, &onnx->dim_out[i]);
//    	if (NULL!=status)goto end;
////    	wtk_debug("dim=%d\n", onnx->dim_out[i]);
//    	api->ReleaseTypeInfo(typeinfo);
//    }

    onnx->name_in = (char**)wtk_malloc(sizeof(char*)*onnx->num_in);
    onnx->name_out = (char**)wtk_malloc(sizeof(char*)*onnx->num_out);

    for(i = 0; i < onnx->num_in; ++i)
    {
    	status = api->SessionGetInputName(onnx->session,i,onnx->allocator,&(onnx->name_in[i]));
    	if (NULL!=status)goto end;
	//wtk_debug("%s\n",onnx->name_in[i]);
    }

    for(i = 0; i < onnx->num_out; ++i)
    {
    	status = api->SessionGetOutputName(onnx->session,i,onnx->allocator,&(onnx->name_out[i]));
    	if (NULL!=status)goto end;
	//wtk_debug("%s\n",onnx->name_out[i]);
    }

    onnx->in = (OrtValue**)wtk_calloc(onnx->num_in,sizeof(OrtValue*));
    onnx->out = (OrtValue**)wtk_calloc(onnx->num_out,sizeof(OrtValue*));

end:
	if(NULL != status)
	{
		wtk_debug("ErrCode: %d\n", api->GetErrorCode(status));
    		wtk_debug("ErrMsg: %s\n", api->GetErrorMessage(status));
		//char* errmsg = api->GetErrorMessage(status);
		qtk_vits_onnx_delete(onnx);
		onnx=NULL;
	}
    return onnx;
}

int qtk_vits_onnx_reset(qtk_vits_onnx_t *onnx)
{
	int i = 0;

	for(i=0; i < onnx->num_in; i++ )
	{
		if (onnx->in && onnx->in[i])
			onnx->api->ReleaseValue(onnx->in[i]);
		onnx->in[i] = NULL;
	}
	for(i=0; i < onnx->num_out; i++ )
	{
		if (onnx->out[i])
			onnx->api->ReleaseValue(onnx->out[i]);
		onnx->out[i] = NULL;
	}

	return 0;
}

int qtk_vits_onnx_feed_inparam(qtk_vits_onnx_t *onnx, void *input, int in_dim, int64_t *in_shape, int shape_len,int type,int index)
{
	OrtStatus *status;
	int ret;

	switch(type)
	{
	case 0:
		status = onnx->api->CreateTensorWithDataAsOrtValue(onnx->meminfo, input, sizeof(float)*in_dim,(const int64_t*)in_shape,shape_len,ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,onnx->in+index);
		if (NULL != status) {
			ret = -1;
			goto end;
		}
		break;
	case 1:
		status = onnx->api->CreateTensorWithDataAsOrtValue(onnx->meminfo, input, sizeof(int)*in_dim,(const int64_t*)in_shape,shape_len,ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32,onnx->in+index);
		if (NULL != status) {
			ret = -1;
			goto end;
		}
		break;
	case 2:
		status = onnx->api->CreateTensorWithDataAsOrtValue(onnx->meminfo, input, sizeof(int64_t)*in_dim,(const int64_t*)in_shape,shape_len,ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64,onnx->in+index);
		if (NULL != status) {
			ret = -1;
			goto end;
		}
		break;
	default:
		break;
	}
	ret = 0;
end:
	if (NULL != status)
	{
		wtk_debug("ErrCode: %d\n", onnx->api->GetErrorCode(status));
		wtk_debug("ErrMsg: %s\n", onnx->api->GetErrorMessage(status));
	}

	return ret;
}

int qtk_vits_onnx_run(qtk_vits_onnx_t *onnx)
{
	OrtStatus *status;

	status = onnx->api->Run(onnx->session,NULL,(const char* const*)onnx->name_in,(const OrtValue* const*)onnx->in,onnx->num_in,(const char * const*)onnx->name_out,onnx->num_out,onnx->out);
	if (NULL != status)
	{
    	printf("ErrCode: %d\n", onnx->api->GetErrorCode(status));
    	printf("ErrMsg: %s\n", onnx->api->GetErrorMessage(status));
		return -1;
	}
	else
		return 0;
}

int64_t* qtk_vits_onnx_get_outshape(qtk_vits_onnx_t *onnx, int index, int64_t *size_len)
{
	OrtStatus *status;
	OrtTensorTypeAndShapeInfo *info;
	int64_t *size = NULL;

	if(index < onnx->num_out)
	{
		status = onnx->api->GetTensorTypeAndShape(onnx->out[index],&info);
		if (NULL != status)goto end;
		status = onnx->api->GetDimensionsCount(info,(size_t*)size_len);
		if (NULL != status)goto end;
		size = (int64_t*)wtk_calloc(*size_len,sizeof(int64_t));
		status = onnx->api->GetDimensions(info,size,*size_len);
		if (NULL != status)goto end;

		onnx->api->ReleaseTensorTypeAndShapeInfo(info);
	}
end:
    if (NULL != status && size)
    {
    	wtk_free(size);
    	size=NULL;
    }

	return size;
}

int64_t* qtk_vits_onnx_get_outvalue(qtk_vits_onnx_t *onnx, int index, int64_t *size_len)
{
	OrtStatus *status;
	OrtTensorTypeAndShapeInfo *info;
	int64_t *size = NULL;

	if(index < onnx->num_out)
	{
		status = onnx->api->GetTensorTypeAndShape(onnx->out[index],&info);
		if (NULL != status)goto end;
	}
end:
    if (NULL != status && size)
    {
    	wtk_free(size);
    	size=NULL;
    }

	return size;
}

void* qtk_vits_onnx_getout(qtk_vits_onnx_t *onnx, int index)
{
	OrtStatus *status;
	void *value = NULL;

	if(index < onnx->num_out)
	{
		status = onnx->api->GetTensorMutableData(onnx->out[index],&value);
		if (NULL != status)
			return value;
	}

	return value;
}

void qtk_vits_onnx_delete(qtk_vits_onnx_t *onnx)
{
	qtk_vits_onnx_reset(onnx);
	if( onnx->meminfo)
		onnx->api->ReleaseMemoryInfo(onnx->meminfo);
	if (onnx->allocator)
		onnx->api->ReleaseAllocator(onnx->allocator);

	if (onnx->name_in)
		wtk_free(onnx->name_in);
	if (onnx->name_out)
		wtk_free(onnx->name_out);
	if (onnx->in)
		wtk_free(onnx->in);
	if (onnx->out)
		wtk_free(onnx->out);

	if (onnx->sop)
	{
		onnx->api->ReleaseSessionOptions(onnx->sop);
		onnx->sop = NULL;
	}
	if (onnx->session)
	{
		onnx->api->ReleaseSession(onnx->session);
		onnx->session = NULL;
	}
	if (onnx->env)
	{
		onnx->api->ReleaseEnv(onnx->env);
		onnx->env = NULL;
	}

	wtk_free(onnx);
}
