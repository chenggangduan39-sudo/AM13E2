#include "qtk_signal_decompress.h"

qtk_signal_decompress_t* qtk_signal_decompress_new(qtk_signal_decompress_cfg_t *cfg)
{
	qtk_signal_decompress_t *dp;

	dp = (qtk_signal_decompress_t*)wtk_malloc(sizeof(qtk_signal_decompress_t));
	dp->cfg = cfg;

	dp->decoder = qtk_nnrt_new(&cfg->decoder);
	dp->rvq_decoder = qtk_nnrt_new(&cfg->rvq_decoder);
	dp->head = qtk_nnrt_new(&cfg->head);
	int i;
	for(i = 0; i < 4;i++){
		dp->in[i] = wtk_strbuf_new(1024,1);
	}
	dp->fft =wtk_drft_new2(512);
	dp->tmp = wtk_malloc(sizeof(float) * 512);
	return dp;
}

void qtk_signal_decompress_delete(qtk_signal_decompress_t *dp)
{
	qtk_nnrt_delete(dp->decoder);
	qtk_nnrt_delete(dp->rvq_decoder);
	qtk_nnrt_delete(dp->head);
	int i;
	for(i = 0; i < 4;i++){
		wtk_strbuf_delete(dp->in[i]);
	}
	wtk_drft_delete2(dp->fft);
	wtk_free(dp->tmp);
	wtk_free(dp);
}


void qtk_signal_decompress_reset(qtk_signal_decompress_t *dp)
{
	qtk_nnrt_reset(dp->decoder);
	qtk_nnrt_reset(dp->rvq_decoder);
	qtk_nnrt_reset(dp->head);
	int i;
	for(i = 0; i < 4;i++){
		wtk_strbuf_reset(dp->in[i]);
	}
}

void qtk_signal_decompress_feed(qtk_signal_decompress_t *dp, int64_t *data, int64_t* len){
	int64_t *p = data;
	int64_t *dim = len;
	int i;
	for(i = 0; i < 4; i++){
		qtk_signal_compress_push_long(dp->in[i],p,*dim);
		p+=*dim;
		dim++;
	}
}

static int _torch_fold(float *in, float *out, int64_t shape, int kernal, int stride){
	int i,j,n = 1,k;
	int m[]={0,0,0,0,0};
	float *p = out;
	memcpy(out,in,sizeof(float)*stride);
	p += stride;
	for(i = stride; i < kernal - stride; i++){
		n = m[0]/stride + 1;
		*p += in[i];
		for(j = 0; j < n; j++){
			*p += in[(j + 1) * kernal + m[j]];
			m[j]++;
		}
		p++;
	}

	int start_idx = kernal - stride;
	for(i = 0; i < shape; i++){
		for(j = start_idx; j < kernal; j++){
			k = j;
			n = i;
			while(k > 0 && n < shape){
				*p += in[n * kernal + k];//TODO
				k -= stride;
				n++;
			}
			p++;
		}
	}


	return 0;
}

static void drft_ifft(wtk_drft_t* rfft,  float *rfft_in, wtk_complex_t *fft,int wins,float *window)
{
	int i;
	wtk_drft_ifft22(rfft, fft, rfft_in);
	for (i=0;i<wins;++i)
	{
		rfft_in[i] /= wins;
		rfft_in[i] *= window[i];
	}
}

static int _process_header(qtk_signal_decompress_t *dp, float *in, int64_t *shape){
	qtk_nnrt_value_t input,mag,cos,sin;
	int64_t out_shape[3];
	int ret;
	wtk_complex_t cpx[257];
	int i,j,dim,out_len,pad = 156;
	float *mag_v,*cos_v,*sin_v;
	short *pv;
	float *tmp,*tmp_window;
	float *out,*window_envelope;
	float *window = (float*)dp->cfg->window->data;


    input = qtk_nnrt_value_create_external(dp->head, QTK_NNRT_VALUE_ELEM_F32,
                                       shape, 3, in);
	qtk_nnrt_feed(dp->head, input, 0);
	ret = qtk_nnrt_run(dp->head);
	if(ret < 0){
		return ret;
	}

	qtk_nnrt_get_output(dp->head, &mag, 0);
	qtk_nnrt_value_get_shape(dp->head, mag, out_shape, 3);
	qtk_nnrt_get_output(dp->head, &cos, 1);
	qtk_nnrt_get_output(dp->head, &sin, 2);

	mag_v = qtk_nnrt_value_get_data(dp->head, mag);
	cos_v = qtk_nnrt_value_get_data(dp->head, cos);
	sin_v = qtk_nnrt_value_get_data(dp->head, sin);
	dim = out_shape[2];

	tmp = wtk_malloc(sizeof(float) * 512 * out_shape[2]);
	tmp_window = wtk_malloc(sizeof(float) * 512 * out_shape[2]);
	out_len = (out_shape[2] - 1) * 200 + 512;
	out = wtk_calloc(sizeof(float) ,out_len);
	window_envelope = wtk_calloc(sizeof(float) ,out_len);

	for(j = 0; j < 512; j++){
		tmp_window[j] = window[j] * window[j];
	}
	for(j = 1; j < out_shape[2]; j++){
		memcpy(tmp_window + j * 512, tmp_window, sizeof(float) * 512);
	}
	for(j = 0; j < out_shape[2]; j++){
		for(i = 0; i < 257; i++){
			cpx[i].a = mag_v[i*dim+j] * cos_v[i*dim+j];
			cpx[i].b = mag_v[i*dim+j] * sin_v[i*dim+j];
			//wtk_debug("%i %f %f\n",i,cpx[i].a,cpx[i].b);
		}
		drft_ifft(dp->fft, tmp + j * 512,cpx,512,window);
	}
	_torch_fold(tmp, out, out_shape[2], 512, 200);
	//print_float(out,out_len);
	//exit(0);
	_torch_fold(tmp_window,window_envelope,out_shape[2], 512, 200);

	out_len -= pad * 2;
	for(i = pad; i < out_len; i++){
		out[i] /= window_envelope[i];
	}

	pv = wtk_malloc(sizeof(short) * out_len);
	for(j = 0; j < out_len; j++){
		pv[j] = (short)floorf(out[j] * 32768 + 0.5);
	}

	if (dp->notify) {
	    dp->notify(dp->ths, pv, out_len);
	}

	qtk_nnrt_value_release(dp->head, input);
	qtk_nnrt_value_release(dp->head, mag);
	qtk_nnrt_value_release(dp->head, cos);
	qtk_nnrt_value_release(dp->head, sin);
	wtk_free(tmp);
	wtk_free(tmp_window);
	wtk_free(out);
	wtk_free(window_envelope);
	wtk_free(pv);
	return ret;
}

static int _process_decoder(qtk_signal_decompress_t *dp, float *in, int64_t *shape){
	qtk_nnrt_value_t input,output;
	int64_t out_shape[3];
	int ret;
    input = qtk_nnrt_value_create_external(dp->decoder, QTK_NNRT_VALUE_ELEM_F32,
                                       shape, 3, in);
	qtk_nnrt_feed(dp->decoder, input, 0);
	ret = qtk_nnrt_run(dp->decoder);
	if(ret < 0){
		return ret;
	}

	qtk_nnrt_get_output(dp->decoder, &output, 0);
	qtk_nnrt_value_get_shape(dp->decoder, output, out_shape, 3);
	ret = _process_header(dp,qtk_nnrt_value_get_data(dp->decoder, output),out_shape);

	qtk_nnrt_value_release(dp->decoder, input);
	qtk_nnrt_value_release(dp->decoder, output);
	return ret;
}

static int _process_rvq_decoder(qtk_signal_decompress_t *dp){
	qtk_nnrt_value_t out,codec1,codec2,codec3,codec4;
	int64_t shape1[2],shape2[2],shape3[2],shape4[2],out_shape[3];
	int ret;

	shape1[0] = shape2[0] = shape3[0] = shape4[0] = 1;
	shape1[1] = dp->in[0]->pos/sizeof(int64_t);
	shape2[1] = dp->in[1]->pos/sizeof(int64_t);
	shape3[1] = dp->in[2]->pos/sizeof(int64_t);
	shape4[1] = dp->in[3]->pos/sizeof(int64_t);
    codec1 = qtk_nnrt_value_create_external(dp->rvq_decoder, QTK_NNRT_VALUE_ELEM_I64,
                                    	shape1, 2, (int64_t*)dp->in[0]->data);
    codec2 = qtk_nnrt_value_create_external(dp->rvq_decoder, QTK_NNRT_VALUE_ELEM_I64,
                                    	shape2, 2, (int64_t*)dp->in[1]->data);
	codec3 = qtk_nnrt_value_create_external(dp->rvq_decoder, QTK_NNRT_VALUE_ELEM_I64,
										shape3, 2, (int64_t*)dp->in[2]->data);
	codec4 = qtk_nnrt_value_create_external(dp->rvq_decoder, QTK_NNRT_VALUE_ELEM_I64,
										shape4, 2, (int64_t*)dp->in[3]->data);

	qtk_nnrt_feed(dp->rvq_decoder, codec1, 0);
	qtk_nnrt_feed(dp->rvq_decoder, codec2, 1);
	qtk_nnrt_feed(dp->rvq_decoder, codec3, 2);
	qtk_nnrt_feed(dp->rvq_decoder, codec4, 3);
	ret = qtk_nnrt_run(dp->rvq_decoder);
	if(ret < 0){
		return ret;
	}

	qtk_nnrt_get_output(dp->rvq_decoder, &out, 0);
	qtk_nnrt_value_get_shape(dp->rvq_decoder, out, out_shape, 3);
	_process_decoder(dp,(float*)qtk_nnrt_value_get_data(dp->rvq_decoder, out),out_shape);

	qtk_nnrt_value_release(dp->rvq_decoder, out);
	qtk_nnrt_value_release(dp->rvq_decoder, codec1);
	qtk_nnrt_value_release(dp->rvq_decoder, codec2);
	qtk_nnrt_value_release(dp->rvq_decoder, codec3);
	qtk_nnrt_value_release(dp->rvq_decoder, codec4);
	return ret;
}

void qtk_signal_decompress_get_result(qtk_signal_decompress_t *dp){
	_process_rvq_decoder(dp);
}

void qtk_signal_decompress_set_notify(qtk_signal_decompress_t *dp,void *ths,qtk_signal_decompress_notify_f notify){
	dp->notify = notify;
	dp->ths = ths;
}
