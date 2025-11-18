#include "qtk_mic_check.h"
#define MCEPS 0.000001
static void _on_stft(qtk_mic_check_t *mc, wtk_stft2_msg_t *msg, int pos,
                     int is_end);

double *_mel_init(qtk_mic_check_t* mc,int gap, int hopsize){
    int i,j,n = gap*2 + 1;
    int numbin = (hopsize + 1) / gap - 1;
    mc->numbin = numbin;
    double *coef = (double*)wtk_calloc(n,sizeof(double));
    double *coef2 = (double*)wtk_calloc(n,sizeof(double));
    double *mel = (double*)wtk_calloc((hopsize + 1) * numbin,sizeof(double));
    int st,et;
    for(i = 0; i < n; i++){
        coef[i] = (i + 1)*1.0/(gap + 1);
    }
    memcpy(coef2,coef,sizeof(double)*9);
    for(i = gap + 1,j = 1; i < n; i++,j++){
        coef2[i] = coef[n - j] - 1.0;
    }

    for(i = 0; i < numbin; i++){
        st = i * gap;
        et = st + n;
        if(i + 1 == numbin){
            et = min(et,hopsize + 1);
        }
        memcpy(mel + i * (hopsize + 1) + st, coef2, sizeof(double) * (et - st));
    }
    wtk_free(coef);
    wtk_free(coef2);
    //print_double(mel,(hopsize + 1)*numbin);
    mc->mel = mel;
    return mel;
}

qtk_mic_check_t *qtk_mic_check_new(qtk_mic_check_cfg_t *cfg){
    qtk_mic_check_t *mc = (qtk_mic_check_t*)wtk_malloc(sizeof(qtk_mic_check_t));
    mc->cfg = cfg;
    mc->winsize = 1024;
    mc->hopsize = 512;
    mc->mel = _mel_init(mc,8,mc->hopsize);
    mc->stft2 = wtk_stft2_new(&cfg->stft2);
    mc->f = NULL;
    mc->freq = NULL;
    mc->fb = NULL;
    mc->medfilt2_out = NULL;
    mc->res = NULL;
    mc->nchannel = -1;
    mc->nframe = 0;
    wtk_stft2_set_notify(mc->stft2, mc, (wtk_stft2_notify_f)_on_stft);
    return mc;
}

void qtk_mic_check_delete(qtk_mic_check_t *mc){
    mc->nframe = 0;
    wtk_stft2_delete(mc->stft2);
    wtk_free(mc->mel);
    if(mc->res){
        wtk_free(mc->res);
        mc->res = NULL;
    }
    if(mc->f){
        wtk_free(mc->f);
        mc->f = NULL;
    }
    if(mc->freq){
        wtk_free(mc->freq);
        mc->freq = NULL;
    }
    if(mc->fb){
        wtk_free(mc->fb);
        mc->fb = NULL;
    }
    if(mc->medfilt2_out){
        wtk_free(mc->medfilt2_out);
        mc->medfilt2_out = NULL;
    }
    wtk_free(mc);
}

void qtk_mic_check_reset(qtk_mic_check_t *mc){
    wtk_stft2_reset(mc->stft2);
    if(mc->res){
        wtk_free(mc->res);
        mc->res = NULL;
    }     
    if(mc->f){
        wtk_free(mc->f);
        mc->f = NULL;
    }
    if(mc->freq){
        wtk_free(mc->freq);
        mc->freq = NULL;
    }
    if(mc->fb){
        wtk_free(mc->fb);
        mc->fb = NULL;
    }
    if(mc->medfilt2_out){
        wtk_free(mc->medfilt2_out);
        mc->medfilt2_out = NULL;
    }
}

static void _filter(const double* x, double* y, int xlen, double* a, double* b, int nfilt)  {
    double tmp;
    int i,j;
    //normalization
    if( (*a-1.0>MCEPS) || (*a-1.0<-MCEPS)){
        tmp=*a;
        for(i=0;i<nfilt;i++){
            b[i]/=tmp;
            a[i]/=tmp;
        }
    }
  
    memset(y,0,xlen*sizeof(double));
  
    a[0]=0.0;
    for(i=0;i<xlen;i++){
        for(j=0;i>=j&&j<nfilt;j++){
            y[i] += (b[j]*x[i-j]-a[j]*y[i-j]);
        }
    }
    a[0]=1.0;
}

static void _on_stft(qtk_mic_check_t *mc, wtk_stft2_msg_t *msg, int pos,
                     int is_end) {
    int i,nbin = mc->stft2->nbin;

    for (i = 0; i < mc->stft2->nbin; i++) {
        mc->freq[i + mc->nframe * nbin] = msg->fft[i][0];
        //printf("%d %.12f\n",i + mc->nframe * nbin,mc->freq[i + mc->nframe * nbin].a);
    }
    wtk_stft2_push_msg(mc->stft2, msg);
    //wtk_debug("%d\n",mc->nframe);
    //exit(0);
    mc->nframe++;
}

static void _calc_fbank(qtk_mic_check_t *mc,double *val, int nsample){
    int nframes = (nsample - mc->winsize)/mc->hopsize + 2,i;
    if(!mc->freq){
        mc->freq = wtk_malloc(sizeof(wtk_complex_t) * mc->stft2->nbin * nframes);
    }
    if(!mc->f){
        mc->f = (float*)wtk_malloc(sizeof(float)*nsample);
    }
    for(i = 0; i < nsample; i++){
        mc->f[i] = (float)val[i];
    }
    wtk_stft2_feed_float(mc->stft2, &mc->f, nsample, 1);

    if(!mc->fb){
        mc->fb = (double*)wtk_calloc(nframes * mc->numbin,sizeof(double));
    }

    int j,k;
    double tmp;
    for(i = 0; i < nframes; i++){
        for(j = 0; j < mc->numbin; j++){
            for(k = 0; k < mc->hopsize+1; k++){
                tmp = sqrt(mc->freq[i*(mc->hopsize+1)+k].a * mc->freq[i*(mc->hopsize+1)+k].a + mc->freq[i*(mc->hopsize+1)+k].b * mc->freq[i*(mc->hopsize+1)+k].b);
                //mc->fb[i*mc->numbin+j] += mc->mel[j*(mc->hopsize+1)+k] * tmp;
                mc->fb[j*nframes+i] += mc->mel[j*(mc->hopsize+1)+k] * tmp;
            }
        }
    }
    //wtk_debug("%d %d %d\n",nframes,mc->numbin,mc->hopsize+1);
    //print_double(mc->fb,nframes*mc->numbin);
}

static int _double_cmp(const void *a, const void *b) {
    double arg1 = *(double*)a;
    double arg2 = *(double*)b;
    if (arg1 > arg2) return 1;
    else if (arg1 < arg2) return -1;
    else return 0;
}
 
static void _medfilt2_c(double *input, double *output, int m, int n, int p, int q) {
    int i, j, k, l, index;
    double *temp = (double *)malloc(p * q * sizeof(double));
 
    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            index = 0;
            for (k = i - p / 2; k <= i + p / 2; k++) {
                for (l = j - q / 2; l <= j + q / 2; l++) {
                	if(l < 0 || k < 0){
                		temp[index] = 0.0;
                	}else if(l >= n || k >= m){
                		temp[index] = 0.0;//input[k * n + n - 1];
                	}else if (k >= 0 && l >= 0 && k < m && l < n) {
                        temp[index] = input[k * n + l];
                    }
                    index++;
                }
            }
            qsort(temp, p * q, sizeof(double), _double_cmp);
            output[i * n + j] = temp[p * q / 2];
        }
    }
 
    free(temp);
}

static double _mc_max(double *p, int row, int col){
    double ret = -1000.0;
    int i,j;

    for(i = 0; i < row; i++){
        for(j = 0; j < col; j++){
            if(*p > ret){
                ret = *p;
            }
            p++;
        }
    }
    return ret;
}

int qtk_mic_check(qtk_mic_check_t *mc, double *src, double *dst, int nsample){
    int fs = 16000;
    int ndrop = fs * 0.1, ret = 0;
    double max_val;
    double *mc_a = mc->cfg->filt_a;
    double *mc_b = mc->cfg->filt_b;
    //wtk_debug("filter\n");
    _filter(src,dst,nsample,mc_a,mc_b,16);
    nsample -= ndrop * 2;
    memcpy(src,dst + ndrop,sizeof(double)*nsample);
    _calc_fbank(mc,src,nsample);
    if(!mc->medfilt2_out){
        mc->medfilt2_out = (double*)wtk_malloc(sizeof(double) * mc->nframe * mc->numbin);
    }
    //wtk_debug("medfilt\n");
    _medfilt2_c(mc->fb,mc->medfilt2_out,mc->nframe,mc->numbin,1,7);
    //wtk_debug("mc max\n");
    max_val = _mc_max(mc->medfilt2_out,mc->nframe,mc->numbin);
    if(max_val <= mc->cfg->thresh){
        ret = 1;
    }
    memset(mc->fb,0,(mc->nframe)* mc->numbin*sizeof(double));
    mc->nframe = 0;
    return ret;
}

void qtk_mic_check_feed(qtk_mic_check_t *mc, char *fn){
    wtk_strbuf_t **buf;
    int i,j,n,nsample;
    double *src,*dst;
    short *wav;
    //wtk_debug("read channel\n");
    buf = wtk_riff_read_channel(fn,&n);
    mc->nchannel = n;
    mc->res = (int*)wtk_malloc(sizeof(int)*n);
    nsample = buf[0]->pos/sizeof(short);
    src = (double*)wtk_malloc(sizeof(double)*nsample);
    dst = (double*)wtk_malloc(sizeof(double)*nsample);

    for(i = 0; i < n; i++){
        wav = (short*)buf[i]->data;
        for(j = 0; j < nsample; j++){
            src[j] = (*wav)*1.0/32768;
            wav++;
        }
        mc->res[i] = qtk_mic_check(mc,src,dst,nsample);
    }
    wtk_strbufs_delete(buf,n);
    wtk_free(src);
    wtk_free(dst);
}

int* qtk_mic_check_get_result(qtk_mic_check_t *mc, int *n){
    *n = mc->nchannel;
    return mc->res;
}
