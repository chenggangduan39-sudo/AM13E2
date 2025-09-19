#include "qtk_punctuation_prediction.h"

typedef struct{
	wtk_string_t punctuation;
	qtk_punctuation_type_t type;
}qtk_punctuation_type_map_t;

qtk_punctuation_type_map_t pp_map[]={
        {wtk_string("_"),QTK_PP_NORM},
        {wtk_string("，"),QTK_PP_COMMA},
        {wtk_string("。"),QTK_PP_PERIOD},
        {wtk_string("？"),QTK_PP_QMARK},
        {wtk_string("、"),QTK_PP_PAUSE},
        {{NULL,0},-1},
};

qtk_punctuation_prediction_t* qtk_punctuation_prediction_new(qtk_punctuation_prediction_cfg_t* cfg){
	qtk_punctuation_prediction_t *wrapper;

	wrapper=(qtk_punctuation_prediction_t*)wtk_malloc(sizeof(*wrapper));
	wrapper->cfg=cfg;

#ifdef ONNX_DEC
	wrapper->model = qtk_onnxruntime_new(&(cfg->model));
#endif
	wrapper->buf = wtk_strbuf_new(1024,1);
	wrapper->id = wtk_strbuf_new(1024,1);
	wrapper->token_type = wtk_strbuf_new(1024,1);
	wrapper->res_buf = wtk_strbuf_new(1024,1);

	return wrapper;
}

int qtk_punctuation_prediction_start(qtk_punctuation_prediction_t *wrapper) {
	return 0;
}

void qtk_punctuation_prediction_reset(qtk_punctuation_prediction_t* wrapper){
#ifdef ONNX_DEC
	qtk_onnxruntime_reset(wrapper->model);
#endif
	wtk_strbuf_reset(wrapper->buf);
	wtk_strbuf_reset(wrapper->id);
	wtk_strbuf_reset(wrapper->token_type);
	wtk_strbuf_reset(wrapper->res_buf);
	}

void qtk_punctuation_prediction_delete(qtk_punctuation_prediction_t* wrapper){
#ifdef ONNX_DEC
	qtk_onnxruntime_delete(wrapper->model);
#endif
	wtk_strbuf_delete(wrapper->buf);
	wtk_strbuf_delete(wrapper->id);
	wtk_strbuf_delete(wrapper->token_type);
	wtk_strbuf_delete(wrapper->res_buf);
	wtk_free(wrapper);
}

int qtk_punctuation_prediction_feed(qtk_punctuation_prediction_t* wrapper,char *data,int bytes){
	if(bytes > 0){
		wtk_strbuf_push(wrapper->buf,data,bytes);
		char *s,*e;
		wtk_fst_sym_t *sym = wrapper->cfg->sym;
		wtk_string_t str;
		int cnt,id,dim = 0;
		int64_t tmp = 0,idx;
		s = wrapper->buf->data;
		e = wrapper->buf->data + wrapper->buf->pos;

		while(s < e){
			cnt = wtk_utf8_bytes(*s);
			str.data = s;
			str.len = cnt;
			id = wtk_fst_sym_get_index(sym,&str);
			idx = id;
			if(id == -1){
				break;
			}
			wtk_strbuf_push(wrapper->id,(char*)(&idx),sizeof(int64_t));
			wtk_strbuf_push(wrapper->token_type,(char*)(&tmp),sizeof(int64_t));
			dim += 1;
			s += cnt;
		}
#ifdef ONNX_DEC
		float *out;
	 	int64_t size,*out_shape=0;
		int64_t shape[2],shape2[3];
		int64_t *mask = wtk_malloc(dim*sizeof(int64_t));
		int i;

		for(i = 0; i < dim; i++){
			mask[i] = 1;
		}
		shape[0] = 1;
		shape[1] = dim;
		shape2[0] = 1;
		shape2[1] = 1;
		shape2[2] = dim;
		//wtk_debug("%d %d %d\n",wrapper->id->pos,wrapper->token_type->pos,dim);
		qtk_onnxruntime_feed(
			wrapper->model, wrapper->id->data, dim,
			cast(int64_t *, shape), 2, 2, 0);
		qtk_onnxruntime_feed(
			wrapper->model, mask, dim,
			cast(int64_t *, shape2), 3, 2, 1);
		qtk_onnxruntime_feed(
			wrapper->model, wrapper->token_type->data, dim,
			cast(int64_t *, shape), 2, 2, 2);
		qtk_onnxruntime_run(wrapper->model);
		out_shape = qtk_onnxruntime_get_outshape(wrapper->model,0,&size);
		out = qtk_onnxruntime_getout(wrapper->model,0);

		int64_t *sym_id = (int64_t*)wrapper->id->data;
		wtk_string_t *resv;
		for(i = 0; i < out_shape[0]; i++){
			int index = wtk_float_argmax(out,out_shape[1]);
			out += 5;
			resv = sym->strs[sym_id[i]];
			wtk_strbuf_push(wrapper->res_buf,resv->data,resv->len);
			if(index != 0){
				str = pp_map[index].punctuation;
				wtk_strbuf_push(wrapper->res_buf,str.data,str.len);
			}else if(i == out_shape[0] - 1){
				str = pp_map[2].punctuation;
				wtk_strbuf_push(wrapper->res_buf,str.data,str.len);
			}
		}
		wtk_free(out_shape);
		wtk_free(mask);
		qtk_onnxruntime_reset(wrapper->model);
#endif
	}
	return 0;
}

void qtk_punctuation_prediction_get_result(qtk_punctuation_prediction_t *wrapper,wtk_string_t *v){

}
