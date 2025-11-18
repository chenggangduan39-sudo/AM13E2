#include "wtk_conv2.h"
#include "wtk/core/fft/wtk_rfft.h"

wtk_conv2_t *wtk_conv2_new(float *kernel, int klen, qtk_fft_t *fft, int ilen) {
    wtk_conv2_t *conv;
    int len;
    float *x;
    float *f;

    conv = (wtk_conv2_t *)wtk_malloc(sizeof(wtk_conv2_t));
    len = klen + ilen - 1;
    len = pow(2, wtk_rfft_next_pow(len));
    if (!fft) {
        fft = qtk_fft_new(len);
        conv->ref_fft = 0;
    } else {
        conv->ref_fft = 1;
    }
    conv->fft_len = len;
    conv->notify = NULL;
    conv->ths = NULL;
    conv->fft = fft;
    conv->len = ilen;
    conv->shift = klen / 2;
    conv->klen = klen;
    // wtk_debug("%d\n",klen+ilen-1);
    conv->dst = (float *)wtk_malloc(sizeof(float) * (klen + ilen));
    // wtk_debug("klen=%d ilen=%d shift=%d\n",klen,ilen,conv->shift);
    // exit(0);
    // wtk_debug("rbin=%d klen=%d\n",conv->left_pad->nslot,klen);
    // wtk_debug("len=%d\n",fft->len);
    // exit(0);
    f = (float *)wtk_malloc(sizeof(float) * len);
    x = (float *)wtk_malloc(sizeof(float) * len);
    memset(x, 0, sizeof(float) * len);
    memcpy(x, kernel, klen * sizeof(float));
    // memcpy(x+klen,kernel,klen*sizeof(float));
    qtk_fft_fft(fft, x, f);
    // wtk_rfft_print_fft(f,len);
    // wtk_debug("len=%d\n",len);
    // print_float(kernel,klen);
    // wtk_debug("klen=%d\n",klen);
    // exit(0);
    conv->kf = f;
    conv->buf = wtk_strbuf_new(1024, 1);
    conv->xf = (float *)wtk_malloc(sizeof(float) * len);
    conv->xx = (float *)wtk_malloc(sizeof(float) * len);
    conv->cf = (float *)wtk_malloc(sizeof(float) * len);
    conv->cx = (float *)wtk_malloc(sizeof(float) * len);
    memset(conv->xx, 0, sizeof(float) * len);
    wtk_free(x);
    wtk_conv2_reset(conv);
    return conv;
}

void wtk_conv2_delete(wtk_conv2_t *conv)
{
	if(conv->ref_fft==0)
	{
            qtk_fft_delete(conv->fft);
        }
	wtk_free(conv->dst);
	wtk_free(conv->cf);
	wtk_free(conv->cx);
	wtk_free(conv->xf);
	wtk_free(conv->xx);
	wtk_strbuf_delete(conv->buf);
	wtk_free(conv->kf);
	wtk_free(conv);
}

void wtk_conv2_set_notify(wtk_conv2_t *conv,wtk_conv2_notify_f notify,void *ths)
{
	conv->notify=notify;
	conv->ths=ths;
}


void wtk_conv2_reset(wtk_conv2_t *conv)
{
	//wtk_debug("%d\n",conv->klen+conv->len-1);
	memset(conv->dst,0,sizeof(float)*(conv->klen+conv->len));
	conv->nframe=0;
	wtk_strbuf_reset(conv->buf);
}


void wtk_conv2_feed_frame(wtk_conv2_t *conv,float *pv,int len)
{
	float *x=conv->xx;
	int i,j;
        int fsize = conv->fft_len;
        int win=fsize/2;
	float *cv=conv->cf;
	float *kf=conv->kf;
	float *xf=conv->xf;
	float *cx=conv->cx;
	float *dst=conv->dst;

//	print_float(pv,len);
	//exit(0);
	++conv->nframe;
	memset(x,0,sizeof(float)*fsize);
	//memset(x+len,0,sizeof(float)*(fsize-len));
	memcpy(x,pv,sizeof(float)*len);
	//print_float(x,10);
	//exit(0);
        qtk_fft_fft(conv->fft, x, xf);
        //wtk_rfft_print_fft(xf,conv->fft->len);
	cv[0]=xf[0]*kf[0];
        cv[1] = xf[1] * kf[1];
        for (i = 1; i < win; i++) {
            cv[i * 2] = xf[i * 2] * kf[i * 2] - xf[i * 2 + 1] * kf[i * 2 + 1];
            cv[i * 2 + 1] =
                xf[i * 2] * kf[i * 2 + 1] + xf[i * 2 + 1] * kf[i * 2];
        }

//	wtk_rfft_print_fft(cv,conv->fft->len);
        qtk_fft_ifft(conv->fft, cv, cx);
        //	print_float(cx,conv->fft->len);
        //	exit(0);
        // print_float(cx+conv->shift,conv->len);
        // print_float(cx+conv->shift,10);
	len=conv->klen+conv->len;
	for(i=0;i<len;++i)
	{
            dst[i] += cx[i] / conv->fft_len;
            // wtk_debug("v[%d]: %f/%f\n",i,dst[i],cx[i]);
	}
	//print_float(dst,len);
	//exit(0);
	//wtk_debug("len=%d/%d\n",len,conv->len);
	if(conv->notify)
	{
		if(conv->nframe==1)
		{
			len=conv->len-conv->shift;
			cx=dst+conv->shift;
		}else
		{
			cx=dst;
			len=conv->len;
		}
		//print_float(cx,len);
		//exit(0);
		//wtk_debug("nframe=%d\n",conv->nframe);
//		if(conv->nframe>1)
//		{
//			exit(0);
//		}
		conv->notify(conv->ths,cx,len);
	}
	//print_float(dst,conv->len+conv->klen);
	//exit(0);
	memmove(dst,dst+conv->len,conv->klen*sizeof(float));
	memset(dst+conv->klen,0,sizeof(float)*(conv->len));
	//wtk_debug("klen=%d le=%d\n",conv->klen,conv->len);
	//exit(0);
	//print_float(dst,conv->len+conv->klen);
	//exit(0);
}


void wtk_conv2_feed(wtk_conv2_t *conv,float *data,int len,int is_end)
{
	int ilen=conv->len;
	float *pv;
	int cnt;


	if(len>0)
	{
		wtk_strbuf_push(conv->buf,(char*)data,len*sizeof(float));
	}
	len=conv->buf->pos/sizeof(float);
	if(len<ilen)
	{
		return;
	}
	pv=(float*)(conv->buf->data);
	cnt=0;
	while(len>=ilen)
	{
		//print_float(pv,ilen);
		//exit(0);
		wtk_conv2_feed_frame(conv,pv,ilen);
		pv+=ilen;
		len-=ilen;
		cnt+=ilen;
	}
	//exit(0);
	wtk_strbuf_pop(conv->buf,NULL,cnt*sizeof(float));
	//exit(0);
}

// FIXME overlap add is incorrect
wtk_conv2_cpx_t *wtk_conv2_cpx_new(wtk_complex_t *kernel, int klen, int ilen) {
    int len;
    wtk_conv2_cpx_t *conv = wtk_malloc(sizeof(wtk_conv2_cpx_t));
    len = ilen + klen - 1;
    len = pow(2, wtk_rfft_next_pow(len));
    conv->fft = qtk_cfft_new(len);
    conv->len = ilen;
    conv->shift = klen / 2;
    conv->klen = klen;
    conv->dst =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * (klen + ilen - 1));
    conv->kf = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * len);
    conv->zero_tmp = wtk_calloc(sizeof(wtk_complex_t), len);
    conv->tmp = wtk_calloc(sizeof(wtk_complex_t), len);
    memcpy(conv->zero_tmp, kernel, klen * sizeof(wtk_complex_t));
    qtk_cfft_fft(conv->fft, conv->zero_tmp, conv->kf);
    memset(conv->zero_tmp, 0, sizeof(wtk_complex_t) * len);

    conv->cf = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * len);
    conv->fft_len = len;

    wtk_conv2_cpx_reset(conv);

    return conv;
}

void wtk_conv2_cpx_delete(wtk_conv2_cpx_t *conv) {
    wtk_free(conv->dst);
    wtk_free(conv->cf);
    wtk_free(conv->kf);
    qtk_cfft_delete(conv->fft);
    wtk_free(conv->zero_tmp);
    wtk_free(conv->tmp);
    wtk_free(conv);
}

void wtk_conv2_cpx_reset(wtk_conv2_cpx_t *conv) {
    memset(conv->dst, 0, sizeof(wtk_complex_t) * (conv->klen + conv->len - 1));
    conv->nframe = 0;
}

int wtk_conv2_cpx_feed_frame(wtk_conv2_cpx_t *conv, wtk_complex_t *data,
                             wtk_complex_t **result) {
    int i;
    int len;
    wtk_complex_t *cx, *xf;
    memcpy(conv->zero_tmp, data, conv->len * sizeof(wtk_complex_t));
    qtk_cfft_fft(conv->fft, conv->zero_tmp, conv->tmp);
    xf = conv->tmp;
    conv->nframe++;
    memmove(conv->dst, conv->dst + conv->len,
            (conv->klen - 1) * sizeof(wtk_complex_t));
    memset(conv->dst + conv->klen - 1, 0, sizeof(wtk_complex_t) * (conv->len));
    for (i = 0; i < conv->fft_len; i++) {
        conv->cf[i].a = xf[i].a * conv->kf[i].a - xf[i].b * conv->kf[i].b;
        conv->cf[i].b = xf[i].a * conv->kf[i].b + xf[i].b * conv->kf[i].a;
    }
    qtk_cfft_ifft(conv->fft, conv->cf, conv->tmp);
    cx = conv->tmp;
    len = conv->klen + conv->len - 1;
    for (i = 0; i < len; i++) {
        conv->dst[i].a += cx[i].a;
        conv->dst[i].b += cx[i].b;
    }
    *result = conv->dst + conv->klen - 1;
    return conv->len;
}
