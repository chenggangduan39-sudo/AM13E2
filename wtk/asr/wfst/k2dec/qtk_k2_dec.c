#include "qtk_k2_dec.h"
void qtk_k2_dec_on_kxparm(qtk_k2_dec_t* wrapper,wtk_kfeat_t *feat);
void qtk_k2_dec_on_kxparm_end(qtk_k2_dec_t* wrapper);
void qtk_k2_dec_reset2(qtk_k2_dec_t* wrapper);
void qtk_k2_helper_delete(qtk_k2_dec_t* wrapper)
{
	qtk_k2_encoder_helper_t *ehelp = wrapper->ehelp;

	if(ehelp->input1){
		wtk_free(ehelp->input1);
	}
	wtk_free(ehelp->in_shape[0]);
	wtk_free(ehelp->input2);
	wtk_free(ehelp->in_shape[1]);

#ifdef ONNX_DEC
	wtk_free(ehelp->in);
#endif
	wtk_free(ehelp->in_shape_len);
	wtk_free(ehelp->in_dim);
	wtk_free(ehelp->in_shape);
	wtk_free(ehelp);
}

qtk_k2_dec_t* qtk_k2_dec_new(qtk_k2_wrapper_cfg_t* cfg)
{
	qtk_k2_dec_t *wrapper;

	wrapper=(qtk_k2_dec_t*)wtk_malloc(sizeof(*wrapper));
	wrapper->cfg=cfg;
	wrapper->kxparm=wtk_kxparm_new(&(cfg->parm));
	wtk_kxparm_set_notify(wrapper->kxparm,wrapper,(wtk_kxparm_notify_f)qtk_k2_dec_on_kxparm);
	wtk_kxparm_set_notify_end(wrapper->kxparm,wrapper,(wtk_kxparm_notify_end_f)qtk_k2_dec_on_kxparm_end);
	if(wrapper->cfg->method == QTK_K2_KWS_ASR){
		wrapper->searcher = NULL;
		wrapper->ins[0].searcher = qtk_beam_searcher_new(cfg);
		wrapper->ins[1].searcher = qtk_beam_searcher_new(cfg);
#ifdef ONNX_DEC
		wrapper->ins[0].decoder = qtk_onnxruntime_new(&(cfg->decoder));
		wrapper->ins[0].joiner = qtk_onnxruntime_new(&(cfg->joiner));
		wrapper->ins[1].decoder = qtk_onnxruntime_new(&(cfg->decoder));
		wrapper->ins[1].joiner = qtk_onnxruntime_new(&(cfg->joiner));
#endif
		wrapper->ins[0].decoder_input[0] = 0;
		wrapper->ins[0].decoder_input[1] = 0;
		wrapper->ins[0].decoder_shape[0] = 1;
		wrapper->ins[0].decoder_shape[1] = 2;
		wrapper->ins[1].decoder_input[0] = 0;
		wrapper->ins[1].decoder_input[1] = 0;
		wrapper->ins[1].decoder_shape[0] = 1;
		wrapper->ins[1].decoder_shape[1] = 2;
		wrapper->ins[0].prob = NULL;
		wrapper->ins[1].prob = NULL;
	}else{
		wrapper->searcher = qtk_beam_searcher_new(cfg);
	}
	wrapper->env_parser=wtk_cfg_file_new();
	wrapper->pp = NULL;
	wrapper->found = 0;
	if(cfg->use_pp){
		wrapper->pp = qtk_punctuation_prediction_new(&(cfg->pp));
	}
#ifdef ONNX_DEC
	wrapper->encoder = qtk_onnxruntime_new(&(cfg->encoder));
	if(wrapper->cfg->method == QTK_K2_BEAM_SEARCH){
		wrapper->decoder = qtk_onnxruntime_new(&(cfg->decoder));
		wrapper->joiner = qtk_onnxruntime_new(&(cfg->joiner));
	}else{
		wrapper->decoder = NULL;
		wrapper->joiner = NULL;
	}
#endif
	wtk_queue_init(&(wrapper->parm_q));
	wrapper->res_buf=wtk_strbuf_new(1024,1);
	wrapper->feat=wtk_strbuf_new(1024,1);
	wrapper->json_buf=wtk_strbuf_new(1024,1);
	wrapper->json=wtk_json_new();
	wrapper->hint_buf=wtk_strbuf_new(1024,1);
	wrapper->timestamp_buf=wtk_strbuf_new(1024,1);

	wrapper->num_feats = 0;
	wrapper->frame = 0;
	wrapper->chunk_nums = 0;
	wrapper->hint_len = 0;
	wrapper->stream_val = NULL;
	wrapper->need_detect = 0;

	wrapper->idle = 1;
	wrapper->recommand_conf = cfg->idle_conf;
	wrapper->valid_index = 0;
	wrapper->vad_index = 0;
	wrapper->conf = 0.0;
	wrapper->st = 0.0;
	wrapper->ed = 0.0;
	wrapper->prob = NULL;
	wrapper->last_timestampe = 0;

	if(wrapper->cfg->method == QTK_K2_BEAM_SEARCH || wrapper->cfg->method == QTK_K2_KWS_ASR){
		// wrapper->ret_len = cfg->subsample * cfg->chunk;
		// wrapper->pad_len = (cfg->right_context + 2)* cfg->subsample + 3;
		// wrapper->chunk_len = cfg->chunk * cfg->subsample + wrapper->pad_len;
		// wrapper->tail_len = 7 + (2 + cfg->right_context) * cfg->subsample;

		wrapper->ret_len = cfg->chunk;//cfg->subsample * cfg->chunk;
		wrapper->pad_len = 13;//(cfg->right_context + 2)* cfg->subsample + 3;
		wrapper->chunk_len = wrapper->ret_len + wrapper->pad_len;
		wrapper->tail_len = 7 + (2 + cfg->right_context) * cfg->subsample;

		wrapper->dec = NULL;
#ifdef ONNX_DEC
		int num_input_nodes = wrapper->encoder->num_in;
		wrapper->ehelp = (qtk_k2_encoder_helper_t*)wtk_malloc(sizeof(qtk_k2_encoder_helper_t));
		wrapper->ehelp->in = (OrtValue**)wtk_calloc(num_input_nodes,sizeof(OrtValue*));
		wrapper->ehelp->in_shape = (int64_t**)wtk_calloc(num_input_nodes,sizeof(int64_t*));
		wrapper->ehelp->in_shape_len = (int*)wtk_calloc(num_input_nodes,sizeof(int));
		wrapper->ehelp->in_dim = (int*)wtk_calloc(num_input_nodes,sizeof(int));

		wrapper->ehelp->input1 = NULL;
		if(wrapper->cfg->use_stream){
			wrapper->ehelp->input1 = (float*)wtk_calloc(1*wrapper->chunk_len*80,sizeof(float));
			wrapper->stream_val = wrapper->encoder->stream_val;
		}
		wrapper->ehelp->in_shape[0] = (int64_t*)wtk_calloc(3,sizeof(int64_t));
		*(wrapper->ehelp->in_shape[0]) = 1;
		*(wrapper->ehelp->in_shape[0] + 1) = wrapper->chunk_len;
		*(wrapper->ehelp->in_shape[0] + 2) = 80;
		wrapper->ehelp->in_shape_len[0] = 3;
		wrapper->ehelp->in_dim[0] = 1*wrapper->chunk_len*80;

		wrapper->ehelp->input2 = (int64_t*)wtk_calloc(1,sizeof(int64_t));
		wrapper->ehelp->in_shape[1] = (int64_t*)wtk_calloc(1,sizeof(int64_t));
		*(wrapper->ehelp->in_shape[1]) = 1;
		wrapper->ehelp->in_shape_len[1] = 1;
		wrapper->ehelp->in_dim[1] = 1;
		*(wrapper->ehelp->input2) = wrapper->chunk_len;

		wrapper->decoder_input[0] = 0;
		wrapper->decoder_input[1] = 0;
		wrapper->decoder_shape[0] = 1;
		wrapper->decoder_shape[1] = 2;

		wrapper->je_shape[1] = 512;
#endif
	}else{
		wrapper->ehelp = NULL;
		wrapper->ret_len = cfg->chunk;//cfg->subsample * cfg->chunk;
		wrapper->pad_len = 13;//(cfg->right_context + 2)* cfg->subsample + 3;
		wrapper->chunk_len = wrapper->ret_len + wrapper->pad_len;
		wrapper->tail_len = 7 + (2 + cfg->right_context) * cfg->subsample;
		wrapper->index = 0;
		wrapper->dec = qtk_kwfstdec_new(&(cfg->kwfstdec));
		wrapper->dec->onnx_dec = 1;
	}

	return wrapper;
}

int qtk_k2_dec_start(qtk_k2_dec_t *wrapper) {
	return qtk_k2_dec_start2(wrapper,0,0);
}

int qtk_k2_dec_start2(qtk_k2_dec_t *wrapper,char *data,int bytes) {
	int ret = 0;

	wtk_wfstenv_cfg_init2(&(wrapper->env));
	if(bytes>0){
		ret=wtk_cfg_file_feed(wrapper->env_parser,data,bytes);
		if(ret!=0){goto end;}
		ret=wtk_wfstenv_cfg_update_local2(&(wrapper->env),wrapper->env_parser->main,0);
		if(ret!=0){goto end;}
	}

	wtk_kxparm_start(wrapper->kxparm);
	if(wrapper->dec){
		wrapper->index = 0;
		ret = qtk_kwfstdec_start(wrapper->dec);
	}
	ret = 0;
	end:
		return ret;
}
#ifdef ONNX_DEC
void* qtk_k2_dec_get_state_val(qtk_k2_dec_t *wrapper,long unsigned int *len){
	if(wrapper->cfg->use_stream && wrapper->stream_val){
		*len = wrapper->encoder->stream_len;
		return wrapper->stream_val;
	}
	return NULL;
}

int qtk_k2_dec_set_state_val(qtk_k2_dec_t *wrapper,void *val,long unsigned int len){
	if(wrapper->cfg->use_stream && wrapper->stream_val && len == wrapper->encoder->stream_len){
		qtk_onnxruntime_t *encoder = wrapper->encoder;
		qtk_onnx_item_t *item;
		int i;
		long unsigned int p_len = 0,vlen;
		for(i = 1; i < encoder->num_in;i++){
			item = encoder->in_items + i;
			vlen = item->bytes*item->in_dim;
			memcpy(item->val,val + p_len,vlen);
			p_len += vlen;
		}
		return 0;
	}
	return -1;
}
#endif

void qtk_k2_dec_instance_run(qtk_k2_dec_instance_t* dec,float blank_penalty,float *ej_out, int64_t cnt,int64_t ej_shape){
#ifdef ONNX_DEC
	int decoder_dim,val;
	int64_t size=0, *d_out_shape=0, *j_out_shape,dj_shape[2],je_shape[2];
	float *d_out, *dj_out, *j_out, je_in[4][512];
	int i,j;

	je_shape[1] = 512;

	for(i=0; i<cnt; i++){
		decoder_dim = dec->decoder_shape[0]*dec->decoder_shape[1];
		qtk_onnxruntime_feed(
			dec->decoder, dec->decoder_input, decoder_dim,
			cast(int64_t *, dec->decoder_shape), 2, 2, 0);
		qtk_onnxruntime_run(dec->decoder);
		d_out_shape = qtk_onnxruntime_get_outshape(dec->decoder,0,&size);
		d_out = qtk_onnxruntime_getout(dec->decoder,0);
		dj_shape[0] = d_out_shape[0];
		dj_shape[1] = d_out_shape[1];
		dj_out = d_out;
		//=================================
		//joiner run
		int num_toks = dec->searcher->num_toks;
		for(j=0; j<num_toks; j++){
			memcpy(je_in[j],ej_out+i*512,sizeof(float)*512);
		}
		je_shape[0] = num_toks;
		qtk_onnxruntime_feed(
			dec->joiner, je_in, num_toks * ej_shape,
			cast(int64_t *, je_shape), 2, 0, 0); // TODO
		qtk_onnxruntime_feed(dec->joiner, dj_out,
								dj_shape[0] * dj_shape[1],
								cast(int64_t *, dj_shape), 2, 0, 1);
		qtk_onnxruntime_run(dec->joiner);
		j_out_shape =
			qtk_onnxruntime_get_outshape(dec->joiner, 0, &size);
		j_out = qtk_onnxruntime_getout(dec->joiner, 0);
		if(!dec->prob){
			dec->prob = (float*)wtk_calloc(j_out_shape[1]*4,sizeof(float));//TODO
		}
		for (j = 0; j < j_out_shape[0]; j++) {
			val = j*j_out_shape[1];
			wtk_softmax(j_out+val,j_out_shape[1]);
			memcpy(dec->prob+val,j_out+val,j_out_shape[1]*sizeof(float));
			wtk_add_log(j_out+val,j_out_shape[1]);
			*(j_out+j*j_out_shape[1]) += blank_penalty;
		}
		//beam search
		dec->searcher->out_col = j_out_shape[1];//TODO
		qtk_beam_searcher_feed(dec->searcher,j_out,dec->prob,j_out_shape[0]*j_out_shape[1],dec->decoder_shape,dec->decoder_input);

		qtk_onnxruntime_reset(dec->decoder);
		qtk_onnxruntime_reset(dec->joiner);
		wtk_free(d_out_shape);
		wtk_free(j_out_shape);
	}
#endif
}


void qtk_k2_dec_nn_run2(qtk_k2_dec_t* wrapper, int st_frame, int num_frames)
{
	wrapper->need_detect = 1;
	wrapper->chunk_nums += 1;
#ifdef ONNX_DEC
	const OrtApi *api = wrapper->encoder->api;
	OrtMemoryInfo *meminfo = wrapper->encoder->meminfo;
	qtk_k2_encoder_helper_t *helper = wrapper->ehelp;
	qtk_onnxruntime_t *encoder = wrapper->encoder;
	qtk_onnx_item_t *item;
	void *encoder_out;
	OrtStatus *status;
	int i;
	int64_t size=0, *e_out_shape;
	float *e_out, *ej_out;
	// encoder feature input prepare
	if(wrapper->cfg->use_stream){
		memcpy(encoder->in_items->val,
			wrapper->feat->data + st_frame * 80 * sizeof(float),
			num_frames * 80 * sizeof(float));
	}else{
		helper->in_dim[0] = 80 * num_frames;
		*(helper->in_shape[0] + 1) = num_frames;
		*(helper->input2) = num_frames;
		*(wrapper->ehelp->in_shape[0] + 1) = num_frames;
		wrapper->ehelp->in_dim[0] = 1*num_frames*80;
		status = api->CreateTensorWithDataAsOrtValue(meminfo,wrapper->feat->data,sizeof(float)*helper->in_dim[0],helper->in_shape[0],helper->in_shape_len[0],ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,helper->in);
		status = api->CreateTensorWithDataAsOrtValue(meminfo,helper->input2,sizeof(int64_t)*helper->in_dim[1],helper->in_shape[1],helper->in_shape_len[1],ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64,helper->in+1);
	}

	if(wrapper->cfg->use_stream){
		for(i = 0; i < encoder->num_in;i++){
			item = encoder->in_items + i;
			status = api->CreateTensorWithDataAsOrtValue(meminfo,item->val,item->bytes*item->in_dim,item->shape,item->shape_len,item->type,encoder->in+i);
		}
	}

	if(wrapper->cfg->use_stream){
		status = api->Run(
			encoder->session, NULL, cast(const char *const *, encoder->name_in),
			cast(const OrtValue *const *, encoder->in), encoder->num_in,
			cast(const char *const *, encoder->name_out), encoder->num_out,
			encoder->out);
	}else{
		status = api->Run(
			encoder->session, NULL, cast(const char *const *, encoder->name_in),
			cast(const OrtValue *const *, helper->in), encoder->num_in,
			cast(const char *const *, encoder->name_out), encoder->num_out,
			encoder->out);
	}

	(void)status;

	e_out_shape = qtk_onnxruntime_get_outshape(wrapper->encoder,0,&size);
	e_out = qtk_onnxruntime_getout(wrapper->encoder,0);
	//encoder stream input prepare
	if(wrapper->cfg->use_stream){
		for(i = 1; i < encoder->num_in;i++){
			item = encoder->in_items + i;
			encoder_out = qtk_onnxruntime_getout(wrapper->encoder,i);
			memcpy(item->val,encoder_out,item->bytes*item->in_dim);
		}
	}
	//=============================
	//joiner encoder run
	int64_t ej_shape[2];
	ej_shape[0] = e_out_shape[1];
	ej_shape[1] = e_out_shape[2];
	ej_out = e_out;
	//=============================
	//decoder run
	qtk_k2_dec_instance_run(&(wrapper->ins[0]),wrapper->cfg->blank_penalty,ej_out,ej_shape[0],ej_shape[1]);
	qtk_k2_dec_instance_run(&(wrapper->ins[1]),wrapper->cfg->blank_penalty,ej_out,ej_shape[0],ej_shape[1]);
	wtk_free(e_out_shape);
#endif
}

void qtk_k2_dec_nn_run(qtk_k2_dec_t* wrapper, int st_frame, int num_frames)
{
	wrapper->need_detect = 1;
	wrapper->chunk_nums += 1;
#ifdef ONNX_DEC
	const OrtApi *api = wrapper->encoder->api;
	OrtMemoryInfo *meminfo = wrapper->encoder->meminfo;
	qtk_k2_encoder_helper_t *helper = wrapper->ehelp;
	qtk_onnxruntime_t *encoder = wrapper->encoder;
	qtk_onnx_item_t *item;
	void *encoder_out;
	OrtStatus *status;
	int i, j;
	int64_t size=0, *e_out_shape, *d_out_shape=0, *j_out_shape;
	float *e_out, *ej_out, *d_out, *dj_out,
		je_in[4][512], *j_out;
	// encoder feature input prepare
	if(wrapper->cfg->use_stream){
		memcpy(encoder->in_items->val,
			wrapper->feat->data + st_frame * 80 * sizeof(float),
			num_frames * 80 * sizeof(float));
	}else{
		helper->in_dim[0] = 80 * num_frames;
		*(helper->in_shape[0] + 1) = num_frames;
		*(helper->input2) = num_frames;
		*(wrapper->ehelp->in_shape[0] + 1) = num_frames;
		wrapper->ehelp->in_dim[0] = 1*num_frames*80;
		status = api->CreateTensorWithDataAsOrtValue(meminfo,wrapper->feat->data,sizeof(float)*helper->in_dim[0],helper->in_shape[0],helper->in_shape_len[0],ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,helper->in);
		status = api->CreateTensorWithDataAsOrtValue(meminfo,helper->input2,sizeof(int64_t)*helper->in_dim[1],helper->in_shape[1],helper->in_shape_len[1],ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64,helper->in+1);
	}

	if(wrapper->cfg->use_stream){
		for(i = 0; i < encoder->num_in;i++){
			item = encoder->in_items + i;
			status = api->CreateTensorWithDataAsOrtValue(meminfo,item->val,item->bytes*item->in_dim,item->shape,item->shape_len,item->type,encoder->in+i);
		}
	}

	if(wrapper->cfg->use_stream){
		status = api->Run(
			encoder->session, NULL, cast(const char *const *, encoder->name_in),
			cast(const OrtValue *const *, encoder->in), encoder->num_in,
			cast(const char *const *, encoder->name_out), encoder->num_out,
			encoder->out);
	}else{
		status = api->Run(
			encoder->session, NULL, cast(const char *const *, encoder->name_in),
			cast(const OrtValue *const *, helper->in), encoder->num_in,
			cast(const char *const *, encoder->name_out), encoder->num_out,
			encoder->out);
	}

	(void)status;

	e_out_shape = qtk_onnxruntime_get_outshape(wrapper->encoder,0,&size);
	e_out = qtk_onnxruntime_getout(wrapper->encoder,0);
	//encoder stream input prepare
	if(wrapper->cfg->use_stream){
		for(i = 1; i < encoder->num_in;i++){
			item = encoder->in_items + i;
			encoder_out = qtk_onnxruntime_getout(wrapper->encoder,i);
			memcpy(item->val,encoder_out,item->bytes*item->in_dim);
		}
	}
	//=============================
	//joiner encoder run
	int64_t ej_shape[2];
	ej_shape[0] = e_out_shape[1];
	ej_shape[1] = e_out_shape[2];
	ej_out = e_out;
	//=============================
	//decoder run
	int decoder_dim,val;
	for(i=0; i<e_out_shape[1]; i++){
		decoder_dim = wrapper->decoder_shape[0]*wrapper->decoder_shape[1];
		qtk_onnxruntime_feed(
			wrapper->decoder, wrapper->decoder_input, decoder_dim,
			cast(int64_t *, wrapper->decoder_shape), 2, 2, 0);
		qtk_onnxruntime_run(wrapper->decoder);
		d_out_shape = qtk_onnxruntime_get_outshape(wrapper->decoder,0,&size);
		d_out = qtk_onnxruntime_getout(wrapper->decoder,0);
		wrapper->dj_shape[0] = d_out_shape[0];
		wrapper->dj_shape[1] = d_out_shape[1];
		dj_out = d_out;
		//=================================
		//joiner run
		int num_toks = wrapper->searcher->num_toks;
		for(j=0; j<num_toks; j++){
			memcpy(je_in[j],ej_out+i*512,sizeof(float)*512);
		}
		wrapper->je_shape[0] = num_toks;
		qtk_onnxruntime_feed(
			wrapper->joiner, je_in, num_toks * ej_shape[1],
			cast(int64_t *, wrapper->je_shape), 2, 0, 0); // TODO
		qtk_onnxruntime_feed(wrapper->joiner, dj_out,
								wrapper->dj_shape[0] * wrapper->dj_shape[1],
								cast(int64_t *, wrapper->dj_shape), 2, 0, 1);
		qtk_onnxruntime_run(wrapper->joiner);
		j_out_shape =
			qtk_onnxruntime_get_outshape(wrapper->joiner, 0, &size);
		j_out = qtk_onnxruntime_getout(wrapper->joiner, 0);
		if(!wrapper->prob){
			wrapper->prob = (float*)wtk_calloc(j_out_shape[1]*wrapper->cfg->beam,sizeof(float));
		}
		for (j = 0; j < j_out_shape[0]; j++) {
			val = j*j_out_shape[1];
			wtk_softmax(j_out+val,j_out_shape[1]);
			memcpy(wrapper->prob+val,j_out+val,j_out_shape[1]*sizeof(float));
			wtk_add_log(j_out+val,j_out_shape[1]);
			*(j_out+j*j_out_shape[1]) += wrapper->cfg->blank_penalty;
		}
		//beam search
		wrapper->searcher->out_col = j_out_shape[1];//TODO
		qtk_beam_searcher_feed(wrapper->searcher,j_out,wrapper->prob,j_out_shape[0]*j_out_shape[1],wrapper->decoder_shape,wrapper->decoder_input);

		qtk_onnxruntime_reset(wrapper->decoder);
		qtk_onnxruntime_reset(wrapper->joiner);
		wtk_free(d_out_shape);
		wtk_free(j_out_shape);
	}
	wtk_free(e_out_shape);
#endif
}

void qtk_k2_dec_nn_run_wfst(qtk_k2_dec_t* wrapper, int st_frame, int num_frames)
{
#ifdef ONNX_DEC
	const OrtApi *api = wrapper->encoder->api;
	OrtMemoryInfo *meminfo = wrapper->encoder->meminfo;
	qtk_onnxruntime_t *encoder = wrapper->encoder;
	qtk_onnx_item_t *item;
	OrtStatus *status;
	int i;
	int64_t size=0, *e_out_shape;
	float *e_out;
	void *encoder_out;
	// encoder feature input prepare
	memcpy(encoder->in_items->val,
			wrapper->feat->data + st_frame * 80 * sizeof(float),
			num_frames * 80 * sizeof(float));

	for(i = 0; i < encoder->num_in;i++){
		item = encoder->in_items + i;
		status = api->CreateTensorWithDataAsOrtValue(meminfo,item->val,item->bytes*item->in_dim,item->shape,item->shape_len,item->type,encoder->in+i);
	}

	status = api->Run(
		encoder->session, NULL, cast(const char *const *, encoder->name_in),
		cast(const OrtValue *const *, encoder->in), encoder->num_in,
		cast(const char *const *, encoder->name_out), encoder->num_out,
		encoder->out);
	(void)status;

	e_out_shape = qtk_onnxruntime_get_outshape(wrapper->encoder,0,&size);
	e_out = qtk_onnxruntime_getout(wrapper->encoder,0);

	//encoder stream input prepare
	if(wrapper->cfg->use_stream){
		for(i = 2; i < encoder->num_in;i++){
			item = encoder->in_items + i;
			encoder_out = qtk_onnxruntime_getout(wrapper->encoder,i);
			//TODO check out shape;
			memcpy(item->val,encoder_out,item->bytes*item->in_dim);
		}
	}
	//=============================
	for(i=0; i<e_out_shape[1]; i++){
		*(e_out+i*e_out_shape[2]) += wrapper->cfg->blank_penalty;
		wtk_softmax(e_out+i*e_out_shape[2],e_out_shape[2]);
		wtk_add_log(e_out+i*e_out_shape[2],e_out_shape[2]);
		qtk_kwfstdec_feed2(wrapper->dec,e_out+i*e_out_shape[2],wrapper->index);
		wrapper->index++;
	}
	wtk_free(e_out_shape);
#endif
}

float eps_feat[]={
	-23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
	-23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
	-23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
	-23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
	-23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
	-23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
	-23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
	-23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
	-23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
	-23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
	-23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
	-23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
	-23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
	-23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
	-23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
	-23.025850, -23.025850, -23.025850, -23.025850, -23.025850
};

void qtk_k2_dec_on_kxparm_end(qtk_k2_dec_t* wrapper)
{
	if(wrapper->dec){
		if(wrapper->num_feats - wrapper->frame > wrapper->chunk_len/4){
			float tmp[80];
			memcpy(tmp,wrapper->feat->data+(wrapper->num_feats-1)*80*sizeof(float),80*sizeof(float));
			while(1){
				if(wrapper->cfg->use_eps_feat){
					wtk_strbuf_push_float(wrapper->feat,eps_feat,80);
				}else{
					wtk_strbuf_push_float(wrapper->feat,tmp,80);
				}
				wrapper->num_feats++;
				if(wrapper->num_feats - wrapper->frame==wrapper->chunk_len){
					break;
				}
			}
			qtk_k2_dec_nn_run_wfst(wrapper,wrapper->frame,wrapper->chunk_len);
		}
	}else{
		if(wrapper->num_feats < wrapper->tail_len){
			return;
		}else{
			if(wrapper->cfg->use_stream){
				int chunks = (wrapper->kxparm->parm->wav_bytes*1.0/wrapper->kxparm->parm->cfg->rate - 
				0.01* wrapper->chunk_len)/(0.01*wrapper->ret_len) + 2;
				chunks -= wrapper->chunk_nums;
				//if((wrapper->num_feats - wrapper->frame) > wrapper->tail_len)
				{
					//wtk_strbuf_pop(wrapper->feat,NULL,sizeof(float)*80);
					if(wrapper->num_feats > 0){
						wrapper->feat->pos -= sizeof(float)*80;
						wrapper->num_feats--;
					}
					
					float tmp[80];
					memcpy(tmp,wrapper->feat->data+(wrapper->num_feats-1)*80*sizeof(float),80*sizeof(float));
					while(1){
						if(wrapper->cfg->use_eps_feat){
							wtk_strbuf_push_float(wrapper->feat,eps_feat,80);
						}else{
							wtk_strbuf_push_float(wrapper->feat,tmp,80);
						}
						wrapper->num_feats++;
						if(wrapper->num_feats - wrapper->frame==wrapper->chunk_len){
							break;
						}
					}
					if(wrapper->searcher){
						qtk_k2_dec_nn_run(wrapper,wrapper->frame,wrapper->chunk_len);
					}else{
						qtk_k2_dec_nn_run2(wrapper,wrapper->frame,wrapper->chunk_len);
					}

					chunks-=1;
#ifdef ONNX_DEC
					qtk_onnxruntime_reset(wrapper->encoder);
#endif
					if(chunks > 1){
						int i;
						wtk_strbuf_reset(wrapper->feat);
						for(i = 0;i < wrapper->chunk_len; i++){
							if(wrapper->cfg->use_eps_feat){
								wtk_strbuf_push_float(wrapper->feat,eps_feat,80);
							}else{
								wtk_strbuf_push_float(wrapper->feat,tmp,80);
							}
						}
						if(wrapper->searcher){
							qtk_k2_dec_nn_run(wrapper,0,wrapper->chunk_len);
						}else{
							qtk_k2_dec_nn_run2(wrapper,0,wrapper->chunk_len);
						}
					}
#ifdef ONNX_DEC
					qtk_onnxruntime_reset(wrapper->encoder);
#endif
				}
			}else{
				if((wrapper->num_feats - wrapper->frame) > wrapper->tail_len){
					qtk_k2_dec_nn_run(wrapper,wrapper->frame,wrapper->num_feats - wrapper->frame);
#ifdef ONNX_DEC
					qtk_onnxruntime_reset(wrapper->encoder);
#endif
				}
			}
		}
	}

}

void qtk_k2_encoder_reset(qtk_k2_dec_t* wrapper)
{
#ifdef ONNX_DEC
	qtk_onnxruntime_t *onnx = wrapper->encoder;
	int i = 0;
	if(!wrapper->dec){
		for (i = 0; i < onnx->num_in; i++) {
			onnx->api->ReleaseValue(wrapper->ehelp->in[i]);
			wrapper->ehelp->in[i] = NULL;
		}
	}
	for (i = 0; i < onnx->num_out; i++) {
		onnx->api->ReleaseValue(onnx->out[i]);
		onnx->out[i] = NULL;
	}
#endif
}

static void qtk_k2_dec_prepare_hint_result(qtk_k2_dec_t *wrapper)
{
	float conf;
	wtk_strbuf_t *hint = wrapper->hint_buf;
	wtk_strbuf_t *timestamp = wrapper->timestamp_buf;
	wtk_fst_sym_t *sym_out;
	wtk_string_t *resv;
	qtk_beam_search_hyp_t *hyp;

	if(!wrapper->cfg->use_stream){
		qtk_k2_dec_on_kxparm_end(wrapper);
	}

	wtk_strbuf_reset(hint);
	wtk_strbuf_reset(timestamp);
	wtk_strbuf_push_s(hint,"{\"hint\":\"");

	int i,k=0;
	float max = -100,val = 0.0;

	for(i=0;i<4;i++){
		hyp = wrapper->searcher->toks[i];
		if(hyp->ys->pos > 0){
			val = sizeof(int)*hyp->log_prob/hyp->ys->pos;
			if(max < val){
				k = i;
				max = val;
			}
		}
	}

	conf = max;
	hyp = wrapper->searcher->toks[k];
	int *res = (int*)hyp->ys->data;
	int *time = (int*)hyp->timestamp->data;
	int cnt = hyp->ys->pos/sizeof(int);
	sym_out = wrapper->cfg->sym;
	for(i=0;i<cnt;i++){
		if(*res>1){
			resv = sym_out->strs[*res];
			wtk_strbuf_push(hint,resv->data,resv->len);
			wtk_strbuf_push_f(timestamp,"%.2f,",0.04*(*time));
			time++;
		}
		res++;
	}
	wtk_strbuf_push_f(hint, "\",\"conf\": %f", conf);
	if(wrapper->env.use_timestamp){
		wtk_strbuf_push_s(hint, ",\"timestamp\":\"");
		wtk_strbuf_push(hint,timestamp->data,timestamp->pos);
		wtk_strbuf_push_s(hint,"\"");
	}
	wtk_strbuf_push_s(hint,"}");
	if(!wrapper->cfg->use_stream){
		qtk_k2_dec_reset2(wrapper);
	}
}

void qtk_k2_dec_on_kxparm(qtk_k2_dec_t* wrapper,wtk_kfeat_t *feat)
{
	int v = 80;
	wtk_strbuf_push_float(wrapper->feat,feat->v,v);
	wrapper->num_feats++;
	if(wrapper->cfg->use_stream && wrapper->num_feats >= wrapper->ret_len){
		if((wrapper->num_feats-wrapper->pad_len) % wrapper->ret_len == 0){
			if(wrapper->dec){
				qtk_k2_dec_nn_run_wfst(wrapper,wrapper->frame,wrapper->chunk_len);
#ifdef ONNX_DEC
				qtk_onnxruntime_reset(wrapper->encoder);
#endif
			}else if(!wrapper->searcher){
				qtk_k2_dec_nn_run2(wrapper,wrapper->frame,wrapper->chunk_len);
				if(wrapper->cfg->use_stream){
#ifdef ONNX_DEC
					qtk_onnxruntime_reset(wrapper->encoder);
#endif
				}else{
					qtk_k2_encoder_reset(wrapper);
				}
			}else{
				qtk_k2_dec_nn_run(wrapper,wrapper->frame,wrapper->chunk_len);
				if(wrapper->cfg->use_stream){
#ifdef ONNX_DEC
					qtk_onnxruntime_reset(wrapper->encoder);
#endif
				}else{
					qtk_k2_encoder_reset(wrapper);
				}
			}
			wrapper->frame+=wrapper->ret_len;
		}
	}

	if(wrapper->env.use_hint && wrapper->hint_len >= wrapper->cfg->hint_len){
		qtk_k2_dec_prepare_hint_result(wrapper);
		wrapper->hint_len = 0;
	}
}

int qtk_k2_dec_check2(int *a, int *b, int idx2){
	int i;
	int *data = a;
	int *data2 = b;
	for(i = 0; i < idx2; i++){
		if(*data != *data2){
			return -1;
		}
		data++;
		data2++;
	}
	return 0;
}

int qtk_k2_dec_check_path_contains(wtk_strbuf_t *src, wtk_strbuf_t *ref){
	int *data = (int*)src->data;
	int *data2 = (int*)ref->data;
	int len = src->pos/sizeof(int);
	int len2 = ref->pos/sizeof(int);
	int j,ret;
	if(len2 > len){
		return -1;
	}

	for(j = 0; j < len; j++){
		data2 = (int*)ref->data;
		if(*(data + j) == *data2){
			ret = qtk_k2_dec_check2(data + j,data2,len2);
			if(ret == 0){
				return ret;
			}
		}
	}

	return -1;
}

int qtk_k2_dec_keyword_detect(qtk_k2_dec_t* wrapper){
	int check = -1;
	qtk_beam_searcher_keyword_detect(wrapper->searcher);
	qtk_beam_searcher_keyword_t *keywrd = &wrapper->searcher->keywrd;
	if(keywrd->best_pth && keywrd->keywrd > wrapper->last_timestampe){
		int success = 0;
		float aver_amprob;
		wrapper->last_timestampe = keywrd->keywrd;
		if(wrapper->cfg->use_trick){
			aver_amprob = wrapper->recommand_conf;
		}else{
			aver_amprob = wrapper->cfg->aver_amprob;
		}
		//wtk_debug("%f %f\n",keywrd->aver_amprob,aver_amprob);
		if(keywrd->aver_amprob > aver_amprob){
			if(keywrd->whole_avg_prob/keywrd->key_avg_prob >= 0.6/1.3){
				if(keywrd->ref_pth){
					if(keywrd->best_pth->timestamp->pos <= 16){
						check = qtk_k2_dec_check_path_contains(keywrd->best_pth->ys,keywrd->ref_pth->ys);
					}
					if(check == -1){
						float conf = keywrd->aver_amprob - keywrd->ref_aver_amprob;
						if(conf >= -3.5){
							if(wrapper->idle == 1){
								if(keywrd->ref_aver_amprob > -0.8 && conf < -0.4){
									success = 0;
								}else if(keywrd->nihao == 1 && keywrd->num_toks > 5 && conf < 0.0){
									success = 0;
								}else{
									success = 1;
								}
							}else{
								if(keywrd->ref_aver_amprob > -0.8 && keywrd->aver_amprob < -1.5){
									success = 0;
								}else{
									success = 1;
								}
							}
						}else{
							success = 0;
						}
					}else{
						success = 1;
					}
				}else{
					success = 1;
				}
			}
		}

		if(success == 1){
			// wtk_debug("%f %f %p\n",keywrd->aver_amprob,aver_amprob,keywrd->ref_pth);
			// wtk_debug("%f %f %f\n",keywrd->whole_avg_prob/keywrd->key_avg_prob,keywrd->ref_aver_amprob,keywrd->aver_amprob - keywrd->ref_aver_amprob);
			// qtk_beam_searcher_debug(wrapper->searcher,&keywrd->best_pth,1);
			// qtk_beam_searcher_debug(wrapper->searcher,&keywrd->ref_pth,1);
			int *f = (int*)keywrd->timestamp->data;
			wrapper->st = f[0] * 0.04;
			wrapper->ed = *(f + keywrd->timestamp->pos/sizeof(int) - 1)*0.04;
			wrapper->conf = keywrd->aver_amprob;
			wrapper->valid_index = wrapper->vad_index;
			wrapper->found = 1;
			qtk_beam_search_keywrd_reset(wrapper->searcher);
			return 1;
		}
	}
	qtk_beam_search_keywrd_reset(wrapper->searcher);
	return 0;
}

int qtk_k2_dec_keyword_detect2(qtk_k2_dec_t* wrapper){
	qtk_beam_searcher_t *searcher = wrapper->ins[0].searcher;
	qtk_k2_wrapper_cfg_t *cfg = wrapper->cfg;
	qtk_beam_searcher_keyword_detect(searcher);
	qtk_beam_searcher_keyword_t *keywrd = &searcher->keywrd;
	int i;

	if(keywrd->best_pth){
		//int *res = (int*)keywrd->best_pth->hw_t->data; 
		//int cnt = keywrd->best_pth->hw_t->pos/sizeof(int);
		//wtk_string_t *resv;
		//wtk_fst_sym_t *sym_out = wrapper->cfg->sym2;
		int success = 0;
		//float conf;

		//wtk_debug("%f %f\n",keywrd->conf,cfg->conf);
		//wtk_debug("%f %f\n",keywrd->wrd_speed,cfg->wrd_speed);
		float aver_amprob;
		if(wrapper->cfg->use_trick){
			aver_amprob = wrapper->recommand_conf;
		}else{
			aver_amprob = wrapper->cfg->aver_amprob;
		}
		//wtk_debug("%f %f\n",keywrd->aver_amprob,aver_amprob);
		if(keywrd->aver_amprob > aver_amprob){
			// int last_res = -1;
			// for(i=0;i<cnt;i++){
			// 	if(*res > 1){
			// 		if(last_res > 1){
			// 			wrapper->res_buf->pos -= resv->len;
			// 		}
			// 		resv = sym_out->strs[*res];
			// 		//wtk_strbuf_push(wrapper->res_buf,resv->data,resv->len);
			// 	}
			// 	last_res = *res;
			// 	res++;
			// }
			//wtk_debug("%d %d\n",keywrd->num_toks,keywrd->num_wrds);
			if(keywrd->num_toks <= 8){
				int *f = (int*)keywrd->timestamp->data;
				success = 1;
				wrapper->st = f[0] * 0.04;
				wrapper->ed = *(f + keywrd->timestamp->pos/sizeof(int) - 1)*0.04;
				//wtk_debug("%f %f\n",wrapper->st,wrapper->ed);
				for(i = 0; i < keywrd->timestamp->pos/sizeof(int) - 1; i++){
					//wtk_debug("timeinterval: %f %f %d\n",(f[i + 1] - f[i])*0.04,cfg->interval,i);
					if((f[i + 1] - f[i])*0.04 > cfg->interval){
						success = 0;
					}
				}
			}
		}

		if(success == 1){
			wrapper->conf = keywrd->aver_amprob;
			wrapper->valid_index = wrapper->vad_index;
			wrapper->found = 1;
			return 1;
		}
	}
	return 0;
}

int qtk_k2_dec_feed(qtk_k2_dec_t* wrapper,char *data,int bytes,int is_end)
{
	int ret = 0;
	if(wrapper->cfg->need_reset && wrapper->found == 1){
		return 0;
	}

	if(wrapper->kxparm){
		wrapper->hint_len += bytes;
		wtk_kxparm_feed(wrapper->kxparm,(short*)data,bytes/2,is_end);
		if(wrapper->cfg->use_hc_wakeup && wrapper->hint_len > wrapper->cfg->hint_len && wrapper->need_detect){
			if(wrapper->searcher){
				ret = qtk_k2_dec_keyword_detect(wrapper);
			}else{
				ret = qtk_k2_dec_keyword_detect2(wrapper);
			}
			wrapper->need_detect = 0;
		}
	}
	return ret;
}

void qtk_k2_dec_reset(qtk_k2_dec_t* wrapper)
{
	if(wrapper->cfg->use_last_state){
		qtk_k2_dec_reset2(wrapper);
		return;
	}
	wtk_cfg_file_reset(wrapper->env_parser);
	wtk_wfstenv_cfg_init2(&(wrapper->env));
	wrapper->need_detect = 0;
	wrapper->hint_len = 0;
	wrapper->chunk_nums = 0;
	wrapper->num_feats = 0;
	wrapper->frame = 0;
	wrapper->conf = 0.0;
	wrapper->st = 0.0;
	wrapper->ed = 0.0;
	wrapper->found = 0;
	wrapper->last_timestampe = 0;
	wtk_strbuf_reset(wrapper->res_buf);
	wtk_strbuf_reset(wrapper->hint_buf);
	wtk_strbuf_reset(wrapper->timestamp_buf);
	if(wrapper->searcher){
		qtk_beam_searcher_reset(wrapper->searcher);
	}else{
		qtk_beam_searcher_reset(wrapper->ins[0].searcher);
		qtk_beam_searcher_reset(wrapper->ins[1].searcher);
#ifdef ONNX_DEC
		qtk_onnxruntime_reset(wrapper->ins[0].decoder);
		qtk_onnxruntime_reset(wrapper->ins[0].joiner);
		qtk_onnxruntime_reset(wrapper->ins[1].decoder);
		qtk_onnxruntime_reset(wrapper->ins[1].joiner);
#endif
		wrapper->ins[0].decoder_input[0] = 0;
		wrapper->ins[0].decoder_input[1] = 0;
		wrapper->ins[0].decoder_shape[0] = 1;
		wrapper->ins[0].decoder_shape[1] = 2;
		wrapper->ins[1].decoder_input[0] = 0;
		wrapper->ins[1].decoder_input[1] = 0;
		wrapper->ins[1].decoder_shape[0] = 1;
		wrapper->ins[1].decoder_shape[1] = 2;
	}
	if(wrapper->pp){
		qtk_punctuation_prediction_reset(wrapper->pp);
	}
	if(wrapper->dec){
		qtk_kwfstdec_reset(wrapper->dec);
#ifdef ONNX_DEC
		qtk_onnxruntime_reset(wrapper->encoder);
#endif
	}else{
		wrapper->decoder_input[0] = 0;
		wrapper->decoder_input[1] = 0;
		wrapper->decoder_shape[0] = 1; 
		wrapper->decoder_shape[1] = 2;

#ifdef ONNX_DEC
	if(wrapper->cfg->use_stream){
		qtk_onnxruntime_reset(wrapper->encoder);
		if(wrapper->encoder->cfg->use_inner_item){
			qtk_onnxruntime_item_reset(wrapper->encoder);
		}
	}else{
		qtk_k2_encoder_reset(wrapper);
	}

	if(wrapper->decoder){
		qtk_onnxruntime_reset(wrapper->decoder);
	}
	if(wrapper->joiner){
		qtk_onnxruntime_reset(wrapper->joiner);
	}
#endif
	}

	if(wrapper->kxparm){
		wtk_kxparm_reset(wrapper->kxparm);
	}
	wtk_json_reset(wrapper->json);
	wtk_strbuf_reset(wrapper->json_buf);
	wtk_queue_init(&(wrapper->parm_q));
	wtk_strbuf_reset(wrapper->feat);
}

void qtk_k2_dec_reset2(qtk_k2_dec_t* wrapper)
{
	wtk_cfg_file_reset(wrapper->env_parser);
	wtk_wfstenv_cfg_init2(&(wrapper->env));
	wrapper->need_detect = 0;
	wrapper->hint_len = 0;
	wrapper->chunk_nums = 0;
	wrapper->num_feats = 0;
	wrapper->frame = 0;
	wrapper->conf = 0.0;
	wrapper->st = 0.0;
	wrapper->ed = 0.0;
	wrapper->found = 0;
	wtk_strbuf_reset(wrapper->res_buf);
	wtk_strbuf_reset(wrapper->hint_buf);
	wtk_strbuf_reset(wrapper->timestamp_buf);
	if(wrapper->searcher){
		qtk_beam_searcher_reset(wrapper->searcher);
	}else{
		qtk_beam_searcher_reset(wrapper->ins[0].searcher);
		qtk_beam_searcher_reset(wrapper->ins[1].searcher);
#ifdef ONNX_DEC
		qtk_onnxruntime_reset(wrapper->ins[0].decoder);
		qtk_onnxruntime_reset(wrapper->ins[0].joiner);
		qtk_onnxruntime_reset(wrapper->ins[1].decoder);
		qtk_onnxruntime_reset(wrapper->ins[1].joiner);
#endif
		wrapper->ins[0].decoder_input[0] = 0;
		wrapper->ins[0].decoder_input[1] = 0;
		wrapper->ins[0].decoder_shape[0] = 1;
		wrapper->ins[0].decoder_shape[1] = 2;
		wrapper->ins[1].decoder_input[0] = 0;
		wrapper->ins[1].decoder_input[1] = 0;
		wrapper->ins[1].decoder_shape[0] = 1;
		wrapper->ins[1].decoder_shape[1] = 2;
	}
	if(wrapper->pp){
		qtk_punctuation_prediction_reset(wrapper->pp);
	}
	if(wrapper->dec){
		qtk_kwfstdec_reset(wrapper->dec);
#ifdef ONNX_DEC
		qtk_onnxruntime_reset(wrapper->encoder);
#endif
	}else{
		wrapper->decoder_input[0] = 0;
		wrapper->decoder_input[1] = 0;
		wrapper->decoder_shape[0] = 1; 
		wrapper->decoder_shape[1] = 2;

#ifdef ONNX_DEC
	if(wrapper->cfg->use_stream){
		qtk_onnxruntime_reset(wrapper->encoder);
		if(wrapper->encoder->cfg->use_inner_item){
			//qtk_onnxruntime_item_reset(wrapper->encoder);
		}
	}else{
		qtk_k2_encoder_reset(wrapper);
	}

	if(wrapper->decoder){
		qtk_onnxruntime_reset(wrapper->decoder);
	}
	if(wrapper->joiner){
		qtk_onnxruntime_reset(wrapper->joiner);
	}
#endif
	}

	if(wrapper->kxparm){
		wtk_kxparm_reset(wrapper->kxparm);
	}
	wtk_json_reset(wrapper->json);
	wtk_strbuf_reset(wrapper->json_buf);
	wtk_queue_init(&(wrapper->parm_q));
	wtk_strbuf_reset(wrapper->feat);
}

void qtk_k2_dec_delete(qtk_k2_dec_t* wrapper)
{
	wtk_strbuf_delete(wrapper->res_buf);
	wtk_json_delete(wrapper->json);
	wtk_strbuf_delete(wrapper->json_buf);
	wtk_strbuf_delete(wrapper->hint_buf);
	wtk_strbuf_delete(wrapper->timestamp_buf);
	if(wrapper->prob){
		wtk_free(wrapper->prob);
	}
	if(wrapper->searcher){
		qtk_beam_searcher_delete(wrapper->searcher);
	}else{
		qtk_beam_searcher_delete(wrapper->ins[0].searcher);
		qtk_beam_searcher_delete(wrapper->ins[1].searcher);
#ifdef ONNX_DEC
		qtk_onnxruntime_delete(wrapper->ins[0].decoder);
		qtk_onnxruntime_delete(wrapper->ins[0].joiner);
		qtk_onnxruntime_delete(wrapper->ins[1].decoder);
		qtk_onnxruntime_delete(wrapper->ins[1].joiner);
		if(wrapper->ins[0].prob){
			wtk_free(wrapper->ins[0].prob);
			wtk_free(wrapper->ins[1].prob);
		}
#endif
	}

	wtk_cfg_file_delete(wrapper->env_parser);
	if(wrapper->pp){
		qtk_punctuation_prediction_delete(wrapper->pp);
	}

	if(wrapper->dec){
		qtk_kwfstdec_delete(wrapper->dec);
#ifdef ONNX_DEC
		qtk_onnxruntime_delete(wrapper->encoder);
#endif
	}else{
		qtk_k2_helper_delete(wrapper);
#ifdef ONNX_DEC
		qtk_onnxruntime_delete(wrapper->encoder);
		if(wrapper->decoder){
			qtk_onnxruntime_delete(wrapper->decoder);
		}
		if(wrapper->joiner){
			qtk_onnxruntime_delete(wrapper->joiner);
		}
#endif
	}

	if(wrapper->kxparm){
		wtk_kxparm_delete(wrapper->kxparm);
	}

	wtk_strbuf_delete(wrapper->feat);

	wtk_free(wrapper);
}

float qtk_k2_dec_get_conf(qtk_k2_dec_t *wrapper){
	float conf = 0.0;

	if(wrapper->dec){
		conf = wrapper->dec->conf;
	}
	if(wrapper->cfg->keyword_detect){
		conf = wrapper->conf;
	}
	return conf;
}

void qtk_k2_dec_get_result(qtk_k2_dec_t *wrapper,wtk_string_t *v)
{
	float conf = 0.0;
	wtk_json_item_t *item;
	wtk_json_t *json=wrapper->json;
	wtk_strbuf_t *buf=wrapper->json_buf;
	wtk_strbuf_t *timestamp = wrapper->timestamp_buf;
	//qtk_k2_wrapper_cfg_t *cfg = wrapper->cfg;
	item=wtk_json_new_object(json);
	wtk_strbuf_reset(timestamp);
	if(wrapper->dec){
		if(wrapper->cfg->filter_result){
			qtk_kwfstdec_get_filter_result(wrapper->dec,wrapper->res_buf,-5.0);
			conf = wrapper->dec->conf;
		}else{
			qtk_kwfstdec_get_result(wrapper->dec,wrapper->res_buf);
			conf = wrapper->dec->conf;
		}
	}else if(wrapper->cfg->keyword_detect){
		qtk_beam_searcher_t *searcher;
		if(wrapper->searcher){
			searcher = wrapper->searcher;
		}else{
			searcher = wrapper->ins[1].searcher;
		}
		qtk_beam_searcher_keyword_detect2(searcher);
		qtk_beam_searcher_keyword_t *keywrd = &searcher->keywrd;
		if(keywrd->best_pth){
			wtk_string_t *resv;
			wtk_fst_sym_t *sym_out = wrapper->cfg->sym2;
			int success = 0;

			//wtk_debug("%f %f\n",keywrd->conf,cfg->conf);
			//wtk_debug("%f %f\n",keywrd->wrd_speed,cfg->wrd_speed);
			float aver_amprob;
			float thresh;
			if(wrapper->cfg->use_trick){
				aver_amprob = wrapper->recommand_conf;
				if(wrapper->idle == 1){
					thresh = 0.6/1.3;
				}else{
					thresh = 0.2/1.1;
				}
			}else{
				aver_amprob = wrapper->cfg->norm_conf;
				thresh = 0.6/1.3;
			}
			//wtk_debug("%f recommand conf:%f idle:%d\n",keywrd->aver_amprob,aver_amprob,wrapper->idle);
			if(keywrd->aver_amprob > aver_amprob){
				wtk_queue_node_t *qn;
				qtk_beam_searcher_result_t *res;
				int last_label = 0;
				int last_idx = -1;
				for(qn = keywrd->resq.pop; qn; qn = qn->next){
       				res = data_offset2(qn, qtk_beam_searcher_result_t, q_n);
					success = 0;
					if(res->key_avg_prob/res->whole_avg_prob >= thresh){
						if(keywrd->ref_pth){
							int check = qtk_k2_dec_check_path_contains(keywrd->best_pth->ys,keywrd->ref_pth->ys);
							if(check == -1){
								if(keywrd->ref_aver_amprob > -0.8 && keywrd->aver_amprob < -1.5){
									success = 0;
								}else{
									if((keywrd->aver_amprob - keywrd->ref_aver_amprob) >= -3.5){
										success = 1;
									}
								}
								if(wrapper->cfg->use_trick && wrapper->idle == 1){
									//wtk_debug("%f %f\n",keywrd->ref_aver_amprob,keywrd->aver_amprob);
									if(keywrd->ref_aver_amprob > -0.3 && (keywrd->aver_amprob - keywrd->ref_aver_amprob) < -0.1){
										success = 0;
									}
								}
							}else{
								//wtk_debug("res->key_avg_prob/res->whole_avg_prob %f %f %f\n",res->key_avg_prob,res->whole_avg_prob,res->key_avg_prob/res->whole_avg_prob);
								//wtk_debug("ref path contains best path\n");
								success = 1;
							}
						}else{
							//wtk_debug("res->key_avg_prob/res->whole_avg_prob %f %f %f\n",res->key_avg_prob,res->whole_avg_prob,res->key_avg_prob/res->whole_avg_prob);
							//wtk_debug("no ref path\n");
							success = 1;
						}
					}
					//wtk_debug("res->tokens_cnt - res->kw_score_cnt > 1:%d\n",res->tokens_cnt - res->kw_score_cnt);
					if(wrapper->cfg->use_hc_asr && (res->label == 935
					|| res->label == 1688
					|| res->label == 1677
					|| (res->label >= 1700 && res->label <= 1703)
					|| (res->label >= 1714 && res->label <= 1717)
					|| res->label == 1732
					|| res->label == 1734
					|| (res->label >= 1736 && res->label <= 1745)
					|| res->label == 1747
					|| res->label == 1748
					|| res->label == 1696
					)){
						if(res->tokens_cnt - res->kw_score_cnt > 1){
							success = 0;
						}
					}

					if(success == 1){
						if(last_idx == -1){
							last_idx = res->start_idx;
							last_label = res->label;
						}else{
							if(last_idx != res->start_idx){
								if(wrapper->searcher->net->last_outid >= 0 && last_label > wrapper->searcher->net->last_outid){
									resv = qtk_k2_context_net_get_outsym(wrapper->searcher->net,last_label);
								}else{
									resv = sym_out->strs[last_label];
								}
								wtk_strbuf_push(wrapper->res_buf,resv->data,resv->len);
								wtk_strbuf_push(wrapper->res_buf," ",1);
							}
							last_idx = res->start_idx;
							last_label = res->label;
						}
					}
				}
				if(last_idx != -1){
					if(wrapper->searcher->net->last_outid >= 0 && last_label > wrapper->searcher->net->last_outid){
						resv = qtk_k2_context_net_get_outsym(wrapper->searcher->net,last_label);
					}else{
						resv = sym_out->strs[last_label];
					}
					wtk_strbuf_push(wrapper->res_buf,resv->data,resv->len);
					wrapper->valid_index = wrapper->vad_index;
					conf = keywrd->aver_amprob;
					//wtk_strbuf_push(wrapper->res_buf," ",1);
				}
			}
		}
	}else{
		int i,k=0;
		float max = -100,val = 0.0;
		wtk_fst_sym_t *sym_out;
		qtk_beam_search_hyp_t *hyp;
		for(i=0;i<4;i++){
			hyp = wrapper->searcher->toks[i];
			if(hyp->ys->pos > 0){
				val = sizeof(int)*hyp->log_prob/hyp->ys->pos;
				if(max < val){
					k = i;
					max = val;
				}
			}
		}
		conf = max;
		hyp = wrapper->searcher->toks[k];
		int *res = (int*)hyp->ys->data;
		int *time = (int*)hyp->timestamp->data;
		int cnt = hyp->ys->pos/sizeof(int);
		wtk_string_t *resv;
		sym_out = wrapper->cfg->sym;
		for(i=0;i<cnt;i++){
			if(*res>1){
				resv = sym_out->strs[*res];
				wtk_strbuf_push(wrapper->res_buf,resv->data,resv->len);
				wtk_strbuf_push_f(timestamp,"%.2f,",0.04*(*time));
				time++;
			}
			res++;
		}
	}
	if(wrapper->cfg->use_hc_asr && wrapper->res_buf->pos > 0){
		char *s,*e;
		s = wrapper->res_buf->data;
		e = wrapper->res_buf->data + wrapper->res_buf->pos;
		wtk_strbuf_push_s(buf,"{\"rec\":\"");
		while (s < e){
			if(*s == ' '){
				wtk_strbuf_push_s(buf,"\",\"conf\":");
				wtk_strbuf_push_f(buf,"%f",conf);
				wtk_strbuf_push_s(buf,"} ");
				wtk_strbuf_push_s(buf,"{\"rec\":\"");
			}else{
				wtk_strbuf_push(buf,s,1);
			}
			s++;
		}
		wtk_strbuf_push_s(buf,"\",\"conf\":");
		wtk_strbuf_push_f(buf,"%f",conf);
		wtk_strbuf_push_s(buf,"}");
	}else{
		if(wrapper->pp && wrapper->env.use_pp){
			qtk_punctuation_prediction_feed(wrapper->pp,wrapper->res_buf->data,wrapper->res_buf->pos);
			wtk_json_obj_add_str2_s(json,item,"rec",wrapper->pp->res_buf->data,wrapper->pp->res_buf->pos);
		}else{
			wtk_json_obj_add_str2_s(json,item,"rec",wrapper->res_buf->data,wrapper->res_buf->pos);
		}

		wtk_json_obj_add_ref_number_s(json,item,"conf",conf);
		if(wrapper->env.use_timestamp){
			wtk_json_obj_add_str2_s(json,item,"timestamp",timestamp->data,timestamp->pos);
		}
		wtk_json_item_print(item,buf);
	}
	wtk_string_set(v,buf->data,buf->pos);
	//wtk_string_set(v,wrapper->res_buf->data,wrapper->res_buf->pos);
}

void qtk_k2_dec_get_hint_result(qtk_k2_dec_t *wrapper,wtk_string_t *v)
{
	if(wrapper->hint_buf->pos > 0){
		wtk_string_set(v,wrapper->hint_buf->data,wrapper->hint_buf->pos);
	}else{
		wtk_strbuf_t *hint = wrapper->hint_buf;
		wtk_strbuf_push_s(hint,"{\"hint\":\"");
		wtk_strbuf_push_s(hint, "\",\"conf\":0.0");
		if(wrapper->env.use_timestamp){
			wtk_strbuf_push_s(hint, ",\"timestamp\":\"\"");
		}
		wtk_strbuf_push_s(hint,"}");
		wtk_string_set(v,wrapper->hint_buf->data,wrapper->hint_buf->pos);
	}
}

int qtk_k2_dec_set_hotwords(qtk_k2_dec_t *wrapper,char *data, int len){
	if(wrapper->cfg->use_context){
		if(wrapper->cfg->keyword_detect){
			qtk_k2_context_net_reset(wrapper->searcher->net);
			qtk_k2_context_net_build_keyword(wrapper->searcher->net,data,len);
			qtk_k2_context_net_dump(wrapper->searcher->net);
			wrapper->searcher->context_net_ok = 1;
			return 0;
		}else{
			qtk_k2_context_net_reset(wrapper->searcher->net);
			qtk_k2_context_net_build(wrapper->searcher->net,data,len);
			wrapper->searcher->context_net_ok = 1;
			return 0;
		}
	}
	return -1;
}

int qtk_k2_dec_set_keywords(qtk_k2_dec_t *wrapper,char *data, int len){
	if(wrapper->cfg->use_context){
		qtk_k2_context_net_reset(wrapper->searcher->net);
		qtk_k2_context_net_build_keyword(wrapper->searcher->net,data,len);
		qtk_k2_context_net_dump(wrapper->searcher->net);
		wrapper->searcher->context_net_ok = 1;
		return 0;
	}
	return -1;
}


int qtk_k2_dec_set_keywords_asr(qtk_k2_dec_t *wrapper,char *data, int len){
	if(wrapper->cfg->use_context){
		qtk_k2_context_net_reset(wrapper->ins[1].searcher->net);
		qtk_k2_context_net_build_keyword(wrapper->ins[1].searcher->net,data,len);
		qtk_k2_context_net_dump(wrapper->ins[1].searcher->net);
		wrapper->ins[1].searcher->context_net_ok = 1;
		return 0;
	}
	return -1;
}


int qtk_k2_dec_set_keywords_wakeup(qtk_k2_dec_t *wrapper,char *data, int len){
	if(wrapper->cfg->use_context){
		qtk_k2_context_net_reset(wrapper->ins[0].searcher->net);
		qtk_k2_context_net_build_keyword(wrapper->ins[0].searcher->net,data,len);
		qtk_k2_context_net_dump(wrapper->ins[0].searcher->net);
		wrapper->ins[0].searcher->context_net_ok = 1;
		return 0;
	}
	return -1;
}

int qtk_k2_dec_set_contacts(qtk_k2_dec_t *dec, char *data, int len){
	if(dec->cfg->use_ebnf){
		return qtk_k2_context_net_build_keyword_plus_xbnf(dec->searcher->net,data,len);
	}
	return qtk_k2_context_net_build_keyword_plus(dec->searcher->net,data,len);
}

float qtk_k2_dec_set_vadindex(qtk_k2_dec_t * wrapper, int index){
	int dur_frames = index - wrapper->valid_index;
	wrapper->vad_index = index;

	if(wrapper->idle == 0){
		if(dur_frames > wrapper->cfg->idle_hint){
			wrapper->idle = 1;
			wrapper->recommand_conf = wrapper->cfg->idle_conf;
		}
		wrapper->recommand_conf = wrapper->cfg->norm_conf;
	}else{
		if(wrapper->valid_index == 0){
			wrapper->recommand_conf = wrapper->cfg->idle_conf;
		}else if(dur_frames > 0 && dur_frames < wrapper->cfg->idle_hint){
			wrapper->idle = 0;
			wrapper->recommand_conf = wrapper->cfg->norm_conf;
		}else{
			wrapper->recommand_conf = wrapper->cfg->idle_conf;
		}
	}
	//wtk_debug("%d %d %ld %ld\n",wrapper->idle,dur_frames,wrapper->vad_index,wrapper->valid_index);
	return wrapper->recommand_conf;
}

void qtk_k2_dec_enter_norm_state(qtk_k2_dec_t * wrapper){
	wrapper->recommand_conf = wrapper->cfg->norm_conf;
	wrapper->idle = 0;
}

void qtk_k2_dec_enter_idle_state(qtk_k2_dec_t * wrapper){
	wrapper->recommand_conf = wrapper->cfg->idle_conf;
	wrapper->idle = 1;
}

void qtk_k2_dec_get_wake_time(qtk_k2_dec_t *dec,float *fs,float *fe){
	*fs = dec->st;
	*fe = dec->ed;
}
