#ifdef ONNX_DEC
#include "qtk_onnxruntime.h"
void qtk_onnxruntime_info_clean(qtk_onnxruntime_t *onnx);
qtk_onnxruntime_t* qtk_onnxruntime_new(qtk_onnxruntime_cfg_t *cfg)
{
	qtk_onnxruntime_t *onnx = (qtk_onnxruntime_t*)wtk_malloc(sizeof(qtk_onnxruntime_t));
    OrtStatus *status;
    //wtk_rbin2_t *rbin;
    wtk_rbin2_item_t *model;

    const OrtApiBase* base = OrtGetApiBase();
    const OrtApi *api = base->GetApi(ORT_API_VERSION);
    int i;

    onnx->base = base;
    onnx->api = api;
	onnx->cfg = cfg;

    status = api->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "test",&onnx->env);
    if (NULL!=status)goto end;
    status = api->CreateSessionOptions(&onnx->sop);
    if (NULL!=status)goto end;
    status = api->SetSessionExecutionMode(onnx->sop, ORT_PARALLEL);
    if (NULL!=status)goto end;
    status = api->SetInterOpNumThreads(onnx->sop, cfg->num_inter_threads);  // 
    if (NULL!=status)goto end;
    status = api->SetIntraOpNumThreads(onnx->sop, cfg->num_intra_threads);  // 
    if (NULL!=status)goto end;
    status = api->SetSessionGraphOptimizationLevel(onnx->sop,ORT_ENABLE_EXTENDED);
    if (NULL!=status)goto end;
    // status = api->EnableProfiling(onnx->sop, "onnx");
    // if (NULL!=status)goto end;

    if (cfg->use_reduce_mem) {
        status = api->DisableCpuMemArena(onnx->sop);
        if (NULL != status) {
            goto end;
        }
    }

    if(cfg->use_mem)
    {
    	//rbin = (wtk_rbin2_t*)cfg->sl->hook;
    	model = wtk_rbin2_get2(cfg->rb,cfg->onnx_fn,strlen(cfg->onnx_fn));
        status = api->CreateSessionFromArray(onnx->env,model->data->data,model->data->len,onnx->sop,&onnx->session);
        if (NULL!=status)goto end;
    }else
    {
#ifdef _WIN32
        WCHAR *wfn = wtk_mul_to_wchar(cfg->onnx_fn);
        status = api->CreateSession(onnx->env, wfn, onnx->sop,
                                    &onnx->session);
        if (NULL!=status)goto end;
        wtk_free(wfn);
#else
        status = api->CreateSession(onnx->env,cfg->onnx_fn,onnx->sop,&onnx->session);
        if (NULL!=status)goto end;
#endif
    }

    status = api->CreateCpuMemoryInfo(OrtArenaAllocator,OrtMemTypeDefault,&onnx->meminfo);
    if (NULL!=status)goto end;
    status = api->CreateAllocator(onnx->session,onnx->meminfo,&onnx->allocator);
    if (NULL!=status)goto end;

    status = api->SessionGetInputCount(onnx->session,&onnx->num_in);
    if (NULL!=status)goto end;
    status = api->SessionGetOutputCount(onnx->session,&onnx->num_out);
    if (NULL!=status)goto end;

    onnx->name_in = (char**)wtk_malloc(sizeof(char*)*onnx->num_in);
    onnx->name_out = (char**)wtk_malloc(sizeof(char*)*onnx->num_out);
    (void)status;

    for(i = 0; i < onnx->num_in; ++i)
    {
    	status = api->SessionGetInputName(onnx->session,i,onnx->allocator,&(onnx->name_in[i]));
    	if (NULL!=status)goto end;
	//wtk_debug("%s\n",onnx->name_in[i]);
    }
    onnx->stream_val = NULL;
	if(onnx->cfg->use_inner_item){
		qtk_onnxruntime_print_type_info(onnx);
	}

    for(i = 0; i < onnx->num_out; ++i)
    {
    	status = api->SessionGetOutputName(onnx->session,i,onnx->allocator,&(onnx->name_out[i]));
    	if (NULL!=status)goto end;
	//wtk_debug("%s\n",onnx->name_out[i]);
    }
    onnx->in = (OrtValue**)wtk_calloc(onnx->num_in,sizeof(OrtValue*));
    onnx->out = (OrtValue**)wtk_calloc(onnx->num_out,sizeof(OrtValue*));
    onnx->stream_len = 0;
end:
    if(NULL != status)
    {
	    wtk_debug("ErrCode: %d\n", api->GetErrorCode(status));
		wtk_debug("ErrMsg: %s\n", api->GetErrorMessage(status));
		qtk_onnxruntime_delete(onnx);
	    onnx=NULL;
    }
	return onnx;
}

void qtk_onnxruntime_delete(qtk_onnxruntime_t *onnx) {
    int i = 0;

    qtk_onnxruntime_reset(onnx);
    for (i = 0; i < onnx->num_in; ++i) {
        onnx->allocator->Free(onnx->allocator, onnx->name_in[i]);
    }
    for (i = 0; i < onnx->num_out; ++i) {
        onnx->allocator->Free(onnx->allocator, onnx->name_out[i]);
    }

    onnx->api->ReleaseEnv(onnx->env);
	onnx->api->ReleaseSessionOptions(onnx->sop);
	onnx->api->ReleaseSession(onnx->session);
	onnx->api->ReleaseMemoryInfo(onnx->meminfo);
	onnx->api->ReleaseAllocator(onnx->allocator);

	wtk_free(onnx->name_in);
	wtk_free(onnx->name_out);
	wtk_free(onnx->in);
	wtk_free(onnx->out);
	if(onnx->cfg->use_inner_item){
		qtk_onnxruntime_info_clean(onnx);
	}
    if(onnx->stream_val != NULL){
        wtk_free(onnx->stream_val);
    }
	wtk_free(onnx);
}

void qtk_onnxruntime_reset(qtk_onnxruntime_t *onnx)
{
	int i = 0;
	for(i=0; i < onnx->num_in; i++ )
	{

        if (onnx->in[i]) {
            onnx->api->ReleaseValue(onnx->in[i]);
            onnx->in[i] = NULL;
        }
    }
	for(i=0; i < onnx->num_out; i++ )
	{
        if (onnx->out[i]) {
            onnx->api->ReleaseValue(onnx->out[i]);
            // wtk_free(onnx->out[i]);
            onnx->out[i] = NULL;
        }
    }
}

void qtk_onnxruntime_reset2(qtk_onnxruntime_t *onnx)
{
	OrtStatus *status;
	int i = 0;
    for (i = 0; i < onnx->num_in; i++) {
        if (onnx->in[i]) {
            onnx->api->ReleaseValue(onnx->in[i]);
            onnx->in[i] = NULL;
        }
    }
    for (i = 0; i < onnx->num_out; i++) {
        if (onnx->out[i]) {
            onnx->api->ReleaseValue(onnx->out[i]);
            // wtk_free(onnx->out[i]);
            onnx->out[i] = NULL;
        }
    }
    onnx->api->ReleaseMemoryInfo(onnx->meminfo);
    onnx->api->ReleaseAllocator(onnx->allocator);
    status = onnx->api->CreateCpuMemoryInfo(OrtArenaAllocator,OrtMemTypeDefault,&onnx->meminfo);
    status = onnx->api->CreateAllocator(onnx->session,onnx->meminfo,&onnx->allocator);
    (void)status;
}

void qtk_onnxruntime_feed(qtk_onnxruntime_t *onnx, void *input, int in_dim, int64_t *in_shape, int shape_len,int type,int index)
{
	OrtStatus *status;

	switch(type)
	{
	case 0:
        status = onnx->api->CreateTensorWithDataAsOrtValue(
            onnx->meminfo, input, sizeof(float) * in_dim,
            cast(const int64_t *, in_shape), shape_len,
            ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, onnx->in + index);
        break;
	case 1:
        status = onnx->api->CreateTensorWithDataAsOrtValue(
            onnx->meminfo, input, sizeof(int) * in_dim,
            cast(const int64_t *, in_shape), shape_len,
            ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32, onnx->in + index);
        break;
	case 2:
        status = onnx->api->CreateTensorWithDataAsOrtValue(
            onnx->meminfo, input, sizeof(int64_t) * in_dim,
            cast(const int64_t *, in_shape), shape_len,
            ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64, onnx->in + index);
        break;
	default:
		break;
	}
        (void)status;
}

void qtk_onnxruntime_run(qtk_onnxruntime_t *onnx)
{
	OrtStatus *status;

    status = onnx->api->Run(
        onnx->session, NULL, cast(const char *const *, onnx->name_in),
        cast(const OrtValue *const *, onnx->in), onnx->num_in,
        cast(const char *const *, onnx->name_out), onnx->num_out,
        onnx->out);
    (void)status;
}

int64_t *qtk_onnxruntime_get_inshape(qtk_onnxruntime_t *onnx, int index,
                                     int64_t *size_len) {
    OrtStatus *status;
    OrtTypeInfo *type_info;
    OrtTensorTypeAndShapeInfo *Tensor_info;
    int64_t *size = NULL;

    if (index < onnx->num_in) {
        status = onnx->api->SessionGetInputTypeInfo(onnx->session, index,
                                                    &type_info);
        status = onnx->api->CastTypeInfoToTensorInfo(
            type_info, (const OrtTensorTypeAndShapeInfo **)&Tensor_info);
        status = onnx->api->GetDimensionsCount(Tensor_info,
                                               cast(size_t *, size_len));
        size = (int64_t *)wtk_calloc(*size_len, sizeof(int64_t));
        status = onnx->api->GetDimensions(Tensor_info, size, *size_len);

        onnx->api->ReleaseTypeInfo(type_info);
    }

    (void)status;
    return size;
}

int64_t* qtk_onnxruntime_get_outshape(qtk_onnxruntime_t *onnx, int index, int64_t *size_len)
{
	OrtStatus *status;
	OrtTensorTypeAndShapeInfo *info;
	int64_t *size = NULL;

	if(index < onnx->num_out)
	{
		status = onnx->api->GetTensorTypeAndShape(onnx->out[index],&info);
		status = onnx->api->GetDimensionsCount(
			info, cast(size_t *, size_len));
		size = (int64_t*)wtk_calloc(*size_len,sizeof(int64_t));
		status = onnx->api->GetDimensions(info,size,*size_len);

		onnx->api->ReleaseTensorTypeAndShapeInfo(info);
	}

        (void)status;
        return size;
}

void* qtk_onnxruntime_getout(qtk_onnxruntime_t *onnx, int index)
{
	OrtStatus *status;
	void *value = NULL;

	if(index < onnx->num_out)
	{
		status = onnx->api->GetTensorMutableData(onnx->out[index],&value);
	}

    (void)status;
    return value;
}

void qtk_onnxruntime_info_clean(qtk_onnxruntime_t *onnx)
{
	int i;
	qtk_onnx_item_t *item;

	for(i=0; i<onnx->num_in; ++i)
	{
		item = onnx->in_items + i;
        if(i < onnx->cfg->outer_in_num){
            wtk_free(item->val);
        }
        // if(item->name != NULL){
		// 	wtk_free(item->name);
        // }
		wtk_free(item->shape);
	}
	wtk_free(onnx->in_items);
}

void qtk_onnxruntime_item_reset(qtk_onnxruntime_t *onnx)
{
	int i;

	for(i=onnx->cfg->outer_in_num; i<onnx->num_in; ++i)
	{
		qtk_onnx_item_t *item = onnx->in_items + i;
		memset(item->val,0,item->bytes*item->in_dim);
	}
}

void qtk_onnxruntime_print_type_info(qtk_onnxruntime_t *onnx)
{
	OrtStatus *status;
	OrtTypeInfo** type_info=(OrtTypeInfo**)wtk_malloc(sizeof(OrtTypeInfo*)*onnx->num_out);
	OrtTypeInfo** type_info2=(OrtTypeInfo**)wtk_malloc(sizeof(OrtTypeInfo*)*onnx->num_in);
	OrtTensorTypeAndShapeInfo* tensor_info2=NULL;//(OrtTensorTypeAndShapeInfo**)wtk_malloc(sizeof(OrtTensorTypeAndShapeInfo*)*onnx->num_in);

	int i,j;
	int len;
    int outer_in_num = onnx->cfg->outer_in_num;
    onnx->stream_len = 0;

	onnx->in_items = (qtk_onnx_item_t*)wtk_calloc(onnx->num_in,sizeof(qtk_onnx_item_t));
	for(i=0; i<onnx->num_in; ++i)
	{
		qtk_onnx_item_t *item = onnx->in_items + i;
		len = 1;
		//wtk_debug("%d %d\n",onnx->num_in,i);
		status = onnx->api->SessionGetInputTypeInfo(onnx->session,i,type_info2);
		status = onnx->api->CastTypeInfoToTensorInfo(*type_info2,cast(const OrtTensorTypeAndShapeInfo **, &tensor_info2));

		//wtk_debug("%p\n",tensor_info2);
		//item->name = (char*)wtk_malloc(sizeof(char*));
		//status = onnx->api->SessionGetInputName(onnx->session,i,onnx->allocator,&(item->name));

		status = onnx->api->GetDimensionsCount(tensor_info2, cast(size_t *, &(item->shape_len)));
		item->shape = (int64_t*)wtk_calloc(item->shape_len,sizeof(int64_t));
		status = onnx->api->GetDimensions(tensor_info2,item->shape,item->shape_len);
		for(j = 0; j < item->shape_len; j++){
			if(item->shape[j] == -1){
				item->shape[j] = 1;
			}
			len *= item->shape[j];
		}
		status = onnx->api->GetTensorElementType(tensor_info2,&(item->type));
		onnx->api->ReleaseTypeInfo(*type_info2);
		item->in_dim = len;
		switch (item->type)
		{
		case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT:
			item->bytes = sizeof(float);
			break;
		case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:
            item->bytes = sizeof(int);
			break;
		case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64:
			item->bytes = sizeof(int64_t);
			break;
		default:
			break;
		}

        if(i < outer_in_num){
            item->val = (char*)wtk_calloc(item->bytes*item->in_dim,sizeof(char));
        }else{
            onnx->stream_len += item->bytes*item->in_dim;
        }
	}
    
    if(onnx->stream_len > 0){
        long unsigned int p_len = 0;
        onnx->stream_val = (char*)wtk_calloc(onnx->stream_len,sizeof(char));
        for(i=outer_in_num; i<onnx->num_in; ++i)
        {
            qtk_onnx_item_t *item = onnx->in_items + i;
            item->val = onnx->stream_val + p_len;
            p_len += item->bytes*item->in_dim;
        }
    }

	// for(i=0; i<onnx->num_out; ++i)
	// {
	// 	ONNXType type;
	// 	status = onnx->api->SessionGetOutputTypeInfo(onnx->session,i,type_info+i);
	// 	status = onnx->api->GetOnnxTypeFromTypeInfo(type_info[i],&type);
	// 	//wtk_debug("output:%d type:%d\n",i,type);
	// }
    (void)status;

	wtk_free(type_info);
	wtk_free(type_info2);
}

#endif
