/*
 * qtk_tts_syn.c
 *
 *  Created on: Apr 26, 2023
 *      Author: dm
 */

#include "qtk_tts_syn.h"

static void qtk_tts_syn_on_wav(qtk_tts_syn_t *syn,void *data,int len,int is_end)
{
	short *data_s;
	float *data_f, maxf;
	int i;

    if(syn->notify == NULL)
        return;

    if (syn->cfg->use_sample_float)
    {
    	data_f = data;
        //float >> short data
        data_s = wtk_malloc(sizeof(short)*len);
        maxf = 32766.0 / max(0.01, wtk_float_abs_max(data_f, len));
        for(i = 0; i < len; ++i){
            data_s[i] = data_f[i]*maxf;
        }
    }else{
    	data_s = data;
    }

    syn->notify(syn->user_data,data_s,len,is_end);
    if (syn->cfg->use_sample_float)
    	wtk_free(data_s);

    return;
}

typedef struct{
	float *p;
	int len;
	int idx;
}qtk_math_randv_t;
qtk_math_randv_t* qtk_math_randv_new(int len)
{
	qtk_math_randv_t* rv;
	int i;

	rv = wtk_malloc(sizeof(*rv) + sizeof(float) * len);
	rv->p  = (float*)(((char*)rv) + sizeof(*rv));
	rv->len = len;
	rv->idx = 0;
	for (i=0; i < len; i++)
	{
		rv->p[i] = (rand()%100)/100.0;
	}

	return rv;
}

void qtk_math_randv_back(qtk_math_randv_t* rv, int n)
{
	if (rv->idx - n >= 0)
		rv->idx -= n;
}

float qtk_math_randv_getv(qtk_math_randv_t* rv)
{
	if (rv->idx > rv->len -1){
		wtk_debug("Warnning: may be error\n");
		return 0;
	}
	return rv->p[rv->idx++];
}
void qtk_math_randv_delete(qtk_math_randv_t* rv)
{
	wtk_free(rv);
}
wtk_matf_t* qtk_math_multinomial(int n, float* pval, int plen, int size, qtk_math_randv_t* rv)
{
	wtk_matf_t* m;
	float v, *p;
	int i,j;

	p = (float*)wtk_malloc(sizeof(float)*plen);
	j=0;
	p[j]=pval[j];
	for (j=1; j < plen; j++)
		p[j] = p[j-1]+pval[j];
	m = wtk_matf_new(size, plen);
	wtk_matf_zero(m);
	for(i=0; i < size; i++)
	{
		if (rv){
			v = qtk_math_randv_getv(rv);
		}else
			v = (rand()%100)/100.0;
		//printf("v=%f\n", v);
		j=0;
//		wtk_debug("%f %f v=%f\n", pval[0], pval[1], v);
		if (v <= pval[j])
			*(m->p + i * m->col + j) += 1;

		for (j=1; j < plen; j++)
		if (v > pval[j-1] && v <= pval[j])
		{
			*(m->p + i * m->col + j) += 1;
		}
	}
	wtk_free(p);
	return m;
}

int qtk_math_argmax(float* pval, int len)
{
	int i, idx;

	idx = 0;
	for (i=1; i < len; i++)
	{
		if (pval[i] > pval[i-1])
			idx = i;
	}
	return idx;
}
#ifdef USE_DEVICE
#ifdef USE_WORLD
//base on wav
static void qtk_tts_syn_on_device_world(qtk_tts_syn_t *syn,wtk_matf_t *decoder_out,int is_end)
{
    wtk_matf_t
        *lf0_mf,
        *bap_mf,
        *mgc_mf,
        *f0_mf,
		*vuv_mf,
		*vuv,
        *ap_mf,
        *sp_mf;
    wtk_matf_t tmp_in, tmp_in2;
    int f0_len, i, v, step, idx, pad=10;
    qtk_math_randv_t*rv=NULL;

	char* wav_data, *wav_data2;
	int wav_len;

	if (syn->cfg->use_flow)
	{
		step = syn->cfg->flow_step;
		rv = qtk_math_randv_new(decoder_out->row);
	}else
		step = decoder_out->row;

	for (idx=0; idx < decoder_out->row;)
	{
		tmp_in.row = step < decoder_out->row - idx?step:(decoder_out->row - idx);
		tmp_in.col = decoder_out->col;
		tmp_in.p = decoder_out->p + idx * decoder_out->col;


		if (idx > 0){
			tmp_in.row += pad;   //left pad
			tmp_in.p -= pad * decoder_out->col;
			if (rv)
				qtk_math_randv_back(rv, pad + pad);  //last pad + current pad
		}
		if (idx + step < decoder_out->row)
		{
			if (idx + step + pad < decoder_out->row)
				tmp_in.row += pad;  //right pad
			else
				tmp_in.row += decoder_out->row - (idx + step) ;  //all rest add to.
		}

		//wtk_debug("idx=%d  %d rv->idx=%d  len=%d\n", idx, tmp_in.row, rv->idx, decoder_out->row);
		mgc_mf = qtk_devicetts_decode_getPostnet(syn->device->dec, &tmp_in, syn->device->dec->conv_mgc);
		qtk_devicetts_normalizer_denormal(syn->device->normalizer->mgc_mean_vector, syn->device->normalizer->mgc_std_vector, mgc_mf);
		//print_float(mgc_mf->p, mgc_mf->row * mgc_mf->col);

		tmp_in2 = tmp_in;
		tmp_in2.p = (float*)wtk_malloc(tmp_in2.row * tmp_in2.col * sizeof(float));
		memcpy (tmp_in2.p, tmp_in.p, tmp_in2.row * tmp_in2.col * sizeof(float));
	    wtk_nn_tanh(tmp_in2.p, tmp_in2.row * tmp_in2.col);
	    //lf0
		lf0_mf = qtk_devicetts_decode_getlinear(syn->device->dec, &tmp_in2,syn->device->dec->linear_lf0);
		qtk_devicetts_normalizer_denormal(syn->device->normalizer->lf0_mean_vector, syn->device->normalizer->lf0_std_vector, lf0_mf);
		//print_float(lf0_mf->p, lf0_mf->row);
	    //bap
	    bap_mf = qtk_devicetts_decode_getlinear(syn->device->dec, &tmp_in2,syn->device->dec->linear_bap);
	    qtk_devicetts_normalizer_denormal(syn->device->normalizer->bap_mean_vector, syn->device->normalizer->bap_std_vector, bap_mf);
	    //print_float(bap_mf->p, bap_mf->row * bap_mf->col);
	    //vuv
	    vuv_mf = qtk_devicetts_decode_getlinear(syn->device->dec, &tmp_in2,syn->device->dec->linear_vuv);
	    for (i=0; i < vuv_mf->row; i++)
	    {
	    	wtk_nn_softmax(vuv_mf->p+i*vuv_mf->col, vuv_mf->col);
	    	vuv = qtk_math_multinomial(1, vuv_mf->p+i*vuv_mf->col, vuv_mf->col, 1, rv);
	    	v = qtk_math_argmax(vuv->p, vuv->col);
	    	if (v == 0)
	    		*(lf0_mf->p+i*lf0_mf->col)=-10000000000;
	    	wtk_matf_delete(vuv);
	    }

//	    print_float(lf0_mf->p, lf0_mf->col);
	    f0_mf = wtk_mer_process_lf0(lf0_mf);
	    //print_float(f0_mf->p, f0_mf->col);
	    f0_len = f0_mf->row*f0_mf->col;
//	    print_float(bap_mf->p, bap_mf->col);
	    ap_mf = wtk_mer_process_bap(bap_mf);
	    //print_float(ap_mf->p, ap_mf->col);
//	    print_float(mgc_mf->p, mgc_mf->col);
	    sp_mf = wtk_mer_process_mgc(syn->wparam->sintbl, mgc_mf);
	    //print_float(sp_mf->p, sp_mf->col);

	    wav_data = wtk_malloc(sizeof(short) * 16000 * 5.0/1000 * f0_len);
	    wav_len = wtk_world_synth(syn->wparam->rf, syn->wparam->win, 1024, 16000, 5.0,
	        f0_mf->p, f0_len,
	        sp_mf->p, sp_mf->row*sp_mf->col,
	        ap_mf->p, ap_mf->row*ap_mf->col,
	        NULL, wav_data);

	    //printf("f0_len=%d wav_len=%d\n", f0_len, wav_len);
	    wav_data2=wav_data;
		if (idx > 0){
			wav_len -= pad * 160; //pad * sizeof(short) * 16000 * 5.0/1000;
			wav_data2 += pad * 160; //pad * sizeof(short) * 16000 * 5.0/1000;
		}

		if (idx + step + pad < decoder_out->row)
		{
			wav_len -= pad * 160 - 79 * 2; //pad * sizeof(short) * 16000 * 5.0/1000;
			idx+=step;
		}
		else
			idx=decoder_out->row;

		//	    print_short(wav_data, 100);
		if (syn->wsola)
		{
			if (idx < decoder_out->row)
	            wtk_wsola_feed_flow(syn->wsola, (short*)wav_data2,wav_len/2, 0);
	        else
	        	wtk_wsola_feed_flow(syn->wsola, (short*)wav_data2,wav_len/2,is_end);
		}else{
		    if(syn->notify){
		    	if (idx < decoder_out->row)
		    		syn->notify(syn->user_data,(short*)wav_data2,wav_len/2,0);
		    	else
		    		syn->notify(syn->user_data,(short*)wav_data2,wav_len/2,is_end);
		    }
		}

		wtk_free(tmp_in2.p);
	    wtk_free(wav_data);
	    wtk_matf_delete(f0_mf);
	    wtk_matf_delete(ap_mf);
	    wtk_matf_delete(sp_mf);
	}
	if (rv)
		qtk_math_randv_delete(rv);

    return;
}
//base on mgc
static void qtk_tts_syn_on_device_world2(qtk_tts_syn_t *syn,wtk_matf_t *decoder_out,int is_end)
{
    wtk_matf_t
        *lf0_mf,
        *bap_mf,
        *mgc_mf,
        *f0_mf,
		*vuv_mf,
		*vuv,
        *ap_mf,
        *sp_mf;
    wtk_matf_t tmp_in, tmp_in2;
    int f0_len, i, v, step, idx, pad=10;
    qtk_math_randv_t* rv=NULL;

	void* wav_data;
	int wav_len;

	if (syn->cfg->use_flow)
	{
		step = syn->cfg->flow_step;
		rv = qtk_math_randv_new(decoder_out->row);
	}else
		step = decoder_out->row;

	for (idx=0; idx < decoder_out->row;)
	{
		tmp_in.row = step < decoder_out->row - idx?step:(decoder_out->row - idx);
		tmp_in.col = decoder_out->col;
		tmp_in.p = decoder_out->p + idx * decoder_out->col;


		tmp_in2=tmp_in;
		if (idx > 0){
			tmp_in2.row += pad;   //left pad
			tmp_in2.p -= pad * decoder_out->col;
		}
		if (idx + step < decoder_out->row)
		{
			if (idx + step + pad < decoder_out->row)
				tmp_in.row += pad;  //right pad
			else
				tmp_in.row += decoder_out->row - (idx + step) ;  //all rest add to.
		}

		mgc_mf = qtk_devicetts_decode_getPostnet(syn->device->dec, &tmp_in2, syn->device->dec->conv_mgc);
		if (idx > 0){
			mgc_mf->row -= pad;
			mgc_mf->p += pad * mgc_mf->col;
		}
		if (idx + step + pad < decoder_out->row)
		{
			mgc_mf->row -= pad;  //right pad
			idx+=step;
		}
		else
			idx=decoder_out->row;
		//print_float(mgc_mf->p, mgc_mf->row * mgc_mf->col);
		qtk_devicetts_normalizer_denormal(syn->device->normalizer->mgc_mean_vector, syn->device->normalizer->mgc_std_vector, mgc_mf);
		//print_float(mgc_mf->p, mgc_mf->row * mgc_mf->col);

		tmp_in2 = tmp_in;
		tmp_in2.p = (float*)wtk_malloc(tmp_in2.row * tmp_in2.col * sizeof(float));
		memcpy (tmp_in2.p, tmp_in.p, tmp_in2.row * tmp_in2.col * sizeof(float));
	    wtk_nn_tanh(tmp_in2.p, tmp_in2.row * tmp_in2.col);
	    //lf0
		lf0_mf = qtk_devicetts_decode_getlinear(syn->device->dec, &tmp_in2,syn->device->dec->linear_lf0);
		qtk_devicetts_normalizer_denormal(syn->device->normalizer->lf0_mean_vector, syn->device->normalizer->lf0_std_vector, lf0_mf);
		//print_float(lf0_mf->p, lf0_mf->row);
	    //bap
	    bap_mf = qtk_devicetts_decode_getlinear(syn->device->dec, &tmp_in2,syn->device->dec->linear_bap);
	    qtk_devicetts_normalizer_denormal(syn->device->normalizer->bap_mean_vector, syn->device->normalizer->bap_std_vector, bap_mf);
	    //print_float(bap_mf->p, bap_mf->row * bap_mf->col);
	    //vuv
	    vuv_mf = qtk_devicetts_decode_getlinear(syn->device->dec, &tmp_in2,syn->device->dec->linear_vuv);
	    for (i=0; i < vuv_mf->row; i++)
	    {
	    	wtk_nn_softmax(vuv_mf->p+i*vuv_mf->col, vuv_mf->col);
	    	//print_float(vuv_mf->p+i*vuv_mf->col, vuv_mf->col);
	    	vuv = qtk_math_multinomial(1, vuv_mf->p+i*vuv_mf->col, vuv_mf->col, 1, rv);
	    	v = qtk_math_argmax(vuv->p, vuv->col);
	    	if (v == 0)
	    		*(lf0_mf->p+i*lf0_mf->col)=-10000000000;
	    	wtk_matf_delete(vuv);
	    }

	    //print_float(lf0_mf->p, lf0_mf->row * lf0_mf->col);
	    f0_mf = wtk_mer_process_lf0(lf0_mf);
	    //print_float(f0_mf->p, f0_mf->row * f0_mf->col);
	    f0_len = f0_mf->row*f0_mf->col;
//	    print_float(bap_mf->p, bap_mf->col);
	    ap_mf = wtk_mer_process_bap(bap_mf);
	    //print_float(ap_mf->p, ap_mf->row * ap_mf->col);
//	    print_float(mgc_mf->p, mgc_mf->col);
	    sp_mf = wtk_mer_process_mgc(syn->wparam->sintbl, mgc_mf);
	    //print_float(sp_mf->p, sp_mf->row * sp_mf->col);

	    wav_data = wtk_malloc(sizeof(short) * 16000 * 5.0/1000 * f0_len);
	    wav_len = wtk_world_synth(syn->wparam->rf, syn->wparam->win, 1024, 16000, 5.0,
	        f0_mf->p, f0_len,
	        sp_mf->p, sp_mf->row*sp_mf->col,
	        ap_mf->p, ap_mf->row*ap_mf->col,
	        NULL, wav_data);

//	    print_short(wav_data, 100);
		if (syn->wsola)
		{
			if (idx < decoder_out->row)
	            wtk_wsola_feed_flow(syn->wsola, (short*)wav_data,wav_len/2, 0);
	        else
	        	wtk_wsola_feed_flow(syn->wsola, (short*)wav_data,wav_len/2,is_end);
		}else{
		    if(syn->notify){
		    	if (idx < decoder_out->row)
		    		syn->notify(syn->user_data,(short*)wav_data,wav_len/2,0);
		    	else
		    		syn->notify(syn->user_data,(short*)wav_data,wav_len/2,is_end);
		    }
		}

		wtk_free(tmp_in2.p);
	    wtk_free(wav_data);
	    wtk_matf_delete(f0_mf);
	    wtk_matf_delete(ap_mf);
	    wtk_matf_delete(sp_mf);
	}
	if (rv)
		qtk_math_randv_delete(rv);

    return;
}
#endif

static void qtk_tts_syn_on_device(qtk_tts_syn_t *syn,wtk_matf_t *decoder_out,int is_end)
{
	if (syn->cfg->use_lpcnet)
	{
#ifdef USE_LPCNET
		wtk_tac_lpcnet_process(&syn->cfg->lpcnet,decoder_out,NULL,is_end);
#endif
	}else if (syn->cfg->use_world)
	{
#ifdef USE_WORLD
		qtk_tts_syn_on_device_world(syn, decoder_out, is_end);
#endif
	}
}
#endif

static int qtk_tts_syn_encode_new(qtk_tts_syn_t* syn, qtk_tts_syn_cfg_t* cfg)
{
	int ret=-1;
#ifdef USE_VITS
    if (cfg->use_vits)
    {
        syn->vits = qtk_vits_new(&(cfg->vits));
        if (syn->vits)
        {
        	qtk_vits_set_notify(syn->vits, (qtk_vits_notify_f)qtk_tts_syn_on_wav, syn);
        	ret=0;
        }
        goto end;
    }
#endif

#ifdef USE_DEVICE
    if (cfg->use_device)
    {
    	if(cfg->use_lpcnet)
    		syn->device = qtk_devicetts_new_lpcnet(&(cfg->device));
    	else if (cfg->use_world)
    		syn->device = qtk_devicetts_new_world(&(cfg->device));
        qtk_devicetts_set_notify(syn->device,(qtk_devicetts_notify_f)qtk_tts_syn_on_device, syn);
        ret=0;
        goto end;
    }
#endif
end:

    return ret;
}

static int qtk_tts_syn_vocoder_new(qtk_tts_syn_t* syn, qtk_tts_syn_cfg_t* cfg)
{
#ifdef USE_LPCNET
    if (cfg->use_lpcnet)
    {
    	wtk_tac_lpcnet_set_notify(&cfg->lpcnet,(wtk_lpcnet_notify_f)qtk_tts_syn_on_wav,syn);
    	return 0;
    }
#endif
#ifdef USE_WORLD
    if (cfg->use_world)
    {
    	syn->wparam = wtk_mer_wav_param_new(1024, 16000);
    }
#endif

    return 0;
}

static void qtk_syn_smooth_notity(void*thd, short*data,int len)
{
	qtk_tts_syn_t* syn;
	int is_end;

	syn = (qtk_tts_syn_t*)thd;
	if (len==0)
		is_end = 1;
	else
		is_end = 0;
    if (syn->notify)
    	syn->notify(syn->user_data,data,len,is_end);
}


qtk_tts_syn_t* qtk_tts_syn_new(qtk_tts_syn_cfg_t* cfg)
{
	qtk_tts_syn_t* syn;
	int ret;

	syn = wtk_calloc(1, sizeof(*syn));
	syn->cfg = cfg;

	ret=qtk_tts_syn_encode_new(syn, cfg);
	if (ret!=0) goto end;
	ret=qtk_tts_syn_vocoder_new(syn, cfg);
	if (ret!=0) goto end;

	if (cfg->use_smooth)
	{
		syn->wsola = wtk_wsola_new(&(cfg->wsola));
		wtk_wsola_set_notify(syn->wsola, syn, qtk_syn_smooth_notity);
	}
end:
	if(ret!=0){
        qtk_tts_syn_delete(syn);
        syn = NULL;
	}

	return syn;
}

int qtk_tts_syn_feed(qtk_tts_syn_t* syn, wtk_veci_t **id_vec, int nid)
{
	int ret=0, i;

#ifdef USE_VITS
	if (syn->vits)
	{
		ret = qtk_vits_feed(syn->vits, id_vec, nid);
		goto end;
	}
#endif
#ifdef USE_DEVICE
	if (syn->device)
	{
	    for(i = 0; i < nid; ++i){
			if (syn->cfg->use_lpcnet)
				ret=qtk_devicetts_process_lpcnet(syn->device,id_vec[i],(i==nid-1)?1:0);
			else if (syn->cfg->use_world)
				ret=qtk_devicetts_process_world(syn->device,id_vec[i],(i==nid-1)?1:0);
			else{
				ret=-1;
				wtk_debug("unknow\n");
			}
	    	if(ret!=0)goto end;
	    }
		goto end;
	}
#endif
end:
	return ret;
}

int qtk_tts_syn_reset(qtk_tts_syn_t* syn)
{
	int ret=0;
#ifdef USE_VITS
	if (syn->vits)
		ret = qtk_vits_reset(syn->vits);
#endif
#ifdef USE_DEVICE
	if (syn->device)
		ret = qtk_devicetts_reset(syn->device);
#endif
	if (syn->wsola)
		wtk_wsola_reset(syn->wsola);
	return ret;
}

void qtk_tts_syn_delete(qtk_tts_syn_t* syn)
{
#ifdef USE_VITS
	if (syn->vits)
		qtk_vits_delete(syn->vits);
#endif
#ifdef USE_DEVICE
	if (syn->device)
		qtk_devicetts_delete(syn->device);
#endif

#ifdef USE_WORLD
	if (syn->wparam)
		wtk_mer_wav_param_delete(syn->wparam);
#endif
	if (syn->wsola)
		wtk_wsola_delete(syn->wsola);
    wtk_free(syn);
}

void qtk_tts_syn_set_notify(qtk_tts_syn_t* syn, qtk_tts_syn_notify_f notify, void* user_data)
{
	syn->notify = notify;
	syn->user_data = user_data;
}
