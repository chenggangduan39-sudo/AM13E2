#include "qtk_signal_compress.h"

void qtk_signal_compress_push_long(wtk_strbuf_t *buf,int64_t *p,int n)
{
	wtk_strbuf_push(buf,(char*)p,n*sizeof(int64_t));
}

qtk_signal_compress_t* qtk_signal_compress_new(qtk_signal_compress_cfg_t *cfg)
{
	qtk_signal_compress_t *cp;

	cp = (qtk_signal_compress_t*)wtk_malloc(sizeof(qtk_signal_compress_t));
	cp->cfg = cfg;

	cp->encoder = qtk_nnrt_new(&cfg->encoder);
	cp->rvq_encoder = qtk_nnrt_new(&cfg->rvq_encoder);
	cp->wav = wtk_strbuf_new(1024,1);
	cp->codec = wtk_strbuf_new(1024,1);
	return cp;
}

void qtk_signal_compress_delete(qtk_signal_compress_t *cp)
{
	qtk_nnrt_delete(cp->encoder);
	qtk_nnrt_delete(cp->rvq_encoder);
	wtk_strbuf_delete(cp->wav);
	wtk_strbuf_delete(cp->codec);
	wtk_free(cp);
}

void qtk_signal_compress_reset(qtk_signal_compress_t *cp)
{
	qtk_nnrt_reset(cp->encoder);
	qtk_nnrt_reset(cp->rvq_encoder);
	wtk_strbuf_reset(cp->wav);
	wtk_strbuf_reset(cp->codec);
}

void qtk_signal_compress_feed(qtk_signal_compress_t *cp, short *data, int len){
	int i;
	float pv;
	for(i = 0; i < len; i++){
		pv = data[i] / 32768.0f;
		wtk_strbuf_push_float(cp->wav,&pv,1);
	}
}

static int _process_rvq_encoder(qtk_signal_compress_t *cp, float *input, int64_t *shape){
	qtk_nnrt_value_t in,codec1,codec2,codec3,codec4;
	int64_t out_shape[2],*out;
	int ret;
    in = qtk_nnrt_value_create_external(cp->rvq_encoder, QTK_NNRT_VALUE_ELEM_F32,
                                       shape, 3, input);
	qtk_nnrt_feed(cp->rvq_encoder, in, 0);
	ret = qtk_nnrt_run(cp->rvq_encoder);
	if(ret < 0){
		return ret;
	}

	qtk_nnrt_get_output(cp->rvq_encoder, &codec1, 0);
	out = qtk_nnrt_value_get_data(cp->rvq_encoder, codec1);
	qtk_nnrt_value_get_shape(cp->rvq_encoder, codec1, out_shape, 2);
	qtk_signal_compress_push_long(cp->codec,out,out_shape[1]);
	cp->len[0] = out_shape[1];

	qtk_nnrt_get_output(cp->rvq_encoder, &codec2, 1);
	out = qtk_nnrt_value_get_data(cp->rvq_encoder, codec2);
	qtk_nnrt_value_get_shape(cp->rvq_encoder, codec2, out_shape, 2);
	qtk_signal_compress_push_long(cp->codec,out,out_shape[1]);
	cp->len[1] = out_shape[1];

	qtk_nnrt_get_output(cp->rvq_encoder, &codec3, 2);
	out = qtk_nnrt_value_get_data(cp->rvq_encoder, codec3);
	qtk_nnrt_value_get_shape(cp->rvq_encoder, codec3, out_shape, 2);
	qtk_signal_compress_push_long(cp->codec,out,out_shape[1]);
	cp->len[2] = out_shape[1];

	qtk_nnrt_get_output(cp->rvq_encoder, &codec4, 3);
	out = qtk_nnrt_value_get_data(cp->rvq_encoder, codec4);
	qtk_nnrt_value_get_shape(cp->rvq_encoder, codec4, out_shape, 2);
	qtk_signal_compress_push_long(cp->codec,out,out_shape[1]);
	cp->len[3] = out_shape[1];

	qtk_nnrt_value_release(cp->rvq_encoder, in);
	qtk_nnrt_value_release(cp->rvq_encoder, codec1);
	qtk_nnrt_value_release(cp->rvq_encoder, codec2);
	qtk_nnrt_value_release(cp->rvq_encoder, codec3);
	qtk_nnrt_value_release(cp->rvq_encoder, codec4);
	return ret;
}

static int _process_encoder(qtk_signal_compress_t *cp){
	qtk_nnrt_value_t wav,output;
	int len = cp->wav->pos/sizeof(float);
	int64_t shape[3] = {1, 1, len},out_shape[3];
	int ret;

    wav = qtk_nnrt_value_create_external(cp->encoder, QTK_NNRT_VALUE_ELEM_F32,
                                       shape, 3, (float*)cp->wav->data);
	qtk_nnrt_feed(cp->encoder, wav, 0);
	ret = qtk_nnrt_run(cp->encoder);
	if(ret < 0){
		return ret;
	}

	qtk_nnrt_get_output(cp->encoder, &output, 0);
	qtk_nnrt_value_get_shape(cp->encoder, output, out_shape, 3);
	ret = _process_rvq_encoder(cp,qtk_nnrt_value_get_data(cp->encoder, output),out_shape);

	qtk_nnrt_value_release(cp->encoder, wav);
	qtk_nnrt_value_release(cp->encoder, output);

	return ret;
}

int64_t* qtk_signal_compress_get_result(qtk_signal_compress_t *cp, int64_t **len, int *codec_len){
	int pad_to = 1600;
	int length = cp->wav->pos/sizeof(float);
	int right_pad = ceilf(length * 1.0/ pad_to) * pad_to - length;
	if(right_pad > 0){
		wtk_strbuf_push_float(cp->wav,NULL,right_pad);
	}
	int ret = _process_encoder(cp);
	if(ret < 0){
		return NULL;
	}
	*codec_len = 4;
	*len = cp->len;
	return (int64_t*)cp->codec->data;
}