#include "wtk/bfio/maskdenoise/cmask_pse/wtk_cmask_sv.h"
#include "qtk/math/qtk_vector.h"
#include "wtk/core/wtk_alloc.h"


void wtk_cmask_sv_set_notify(wtk_cmask_sv_t *cmask_pse, void *ths, wtk_cmask_sv_notify_f notify) {
    cmask_pse->notify = notify;
    cmask_pse->ths = ths;
}

void wtk_cmask_sv_fbank_on(wtk_cmask_sv_t *cmask_pse, float *data, int len)
{
    wtk_strbuf_push(cmask_pse->fbank_buf, (char *)data, len * sizeof(float));
    int i;
    for(i=0; i<len; ++i){
        cmask_pse->fbank_mean[i] += data[i];
    }
    ++cmask_pse->fbank_frame;
}


wtk_cmask_sv_t* wtk_cmask_sv_new(wtk_cmask_sv_cfg_t *cfg)
{
    wtk_cmask_sv_t *cmask_pse = wtk_malloc(sizeof(wtk_cmask_sv_t));
    cmask_pse->cfg = cfg;
#ifdef ONNX_DEC
    cmask_pse->emb = NULL;

    if(cmask_pse->cfg->use_onnx){
        cmask_pse->emb = qtk_onnxruntime_new(&(cmask_pse->cfg->emb));
    }
#endif

    cmask_pse->fbank = wtk_fbank_new(&(cmask_pse->cfg->fbank));
    cmask_pse->fbank_len = cmask_pse->cfg->fbank.num_fbank;
    cmask_pse->fbank_mean = (float *)wtk_malloc(sizeof(float) * cmask_pse->fbank_len);
    cmask_pse->fbank_buf = wtk_strbuf_new(80, 1);
    cmask_pse->wav_buf = wtk_strbuf_new(80, 1);
    wtk_fbank_set_notify(cmask_pse->fbank, cmask_pse, (wtk_fbank_notify_f)wtk_cmask_sv_fbank_on);
    wtk_fbank_reset(cmask_pse->fbank);
    memset(cmask_pse->fbank_mean, 0, sizeof(float) * cmask_pse->fbank_len);
    wtk_strbuf_reset(cmask_pse->fbank_buf);
    cmask_pse->fbank_frame = 0;
    cmask_pse->output = NULL;
    cmask_pse->output_len = 0;
    cmask_pse->normalize = 0;
    cmask_pse->enroll_cycle = 1;
    return cmask_pse;
}


void wtk_cmask_sv_reset(wtk_cmask_sv_t *cmask_pse)
{
#ifdef ONNX_DEC
    if(cmask_pse->cfg->use_onnx){
        qtk_onnxruntime_reset(cmask_pse->emb);
    }
    if(cmask_pse->output){
        memset(cmask_pse->output,0,sizeof(float)*cmask_pse->output_len);
    }
    wtk_strbuf_reset(cmask_pse->fbank_buf);
    //wtk_strbuf_reset(cmask_pse->wav_buf);
    wtk_fbank_reset(cmask_pse->fbank);
    cmask_pse->fbank_frame = 0;
    memset(cmask_pse->fbank_mean, 0, sizeof(float) * cmask_pse->fbank_len);
#endif
    cmask_pse->normalize = 0;
}

void wtk_cmask_sv_reset2(wtk_cmask_sv_t *cmask_pse)
{
#ifdef ONNX_DEC
    if(cmask_pse->cfg->use_onnx){
        qtk_onnxruntime_reset(cmask_pse->emb);
    }
    wtk_strbuf_reset(cmask_pse->fbank_buf);
    //wtk_strbuf_reset(cmask_pse->wav_buf);
    wtk_fbank_reset(cmask_pse->fbank);
    cmask_pse->fbank_frame = 0;
    memset(cmask_pse->fbank_mean, 0, sizeof(float) * cmask_pse->fbank_len);
#endif
}

void wtk_cmask_sv_delete(wtk_cmask_sv_t *cmask_pse)
{
#ifdef ONNX_DEC
    if(cmask_pse->cfg->use_onnx){
        if (cmask_pse->emb) {
            qtk_onnxruntime_delete(cmask_pse->emb);
        }
    }
#endif
    wtk_fbank_delete(cmask_pse->fbank);
    wtk_free(cmask_pse->fbank_mean);
    wtk_strbuf_delete(cmask_pse->fbank_buf);
    wtk_strbuf_delete(cmask_pse->wav_buf);
    if(cmask_pse->output){
        wtk_free(cmask_pse->output);
    }
    wtk_free(cmask_pse);
}


void wtk_cmask_sv_feed_emb(wtk_cmask_sv_t *cmask_sv)
{
#ifdef ONNX_DEC
    wtk_strbuf_t *fbank_buf = cmask_sv->fbank_buf;
    float *v;
    int64_t shape[3] = {1,0,80};
    shape[1] = cmask_sv->fbank_frame;
    qtk_onnxruntime_feed(cmask_sv->emb, fbank_buf->data,
        cmask_sv->fbank_frame * 80,
        shape, 3, 0, 0);
    qtk_onnxruntime_run(cmask_sv->emb);

    v = qtk_onnxruntime_getout(cmask_sv->emb, 0);
    if(!cmask_sv->output){
        int64_t *oshape, size;
        oshape = qtk_onnxruntime_get_outshape(cmask_sv->emb, 0, &size);
        cmask_sv->output_len = oshape[1];
        cmask_sv->output = wtk_malloc(sizeof(float) * cmask_sv->output_len);
        wtk_free(oshape);
    }
    memcpy(cmask_sv->output,v,sizeof(float)*cmask_sv->output_len);
#endif
}

void wtk_cmask_sv_feed_emb_cycle(wtk_cmask_sv_t *cmask_sv)
{
    #ifdef ONNX_DEC
    wtk_strbuf_t *fbank_buf = cmask_sv->fbank_buf;
    float *v;
    int64_t shape[3] = {1,0,80};
    shape[1] = cmask_sv->fbank_frame;
    qtk_onnxruntime_feed(cmask_sv->emb, fbank_buf->data,
        cmask_sv->fbank_frame * 80,
        shape, 3, 0, 0);
    qtk_onnxruntime_run(cmask_sv->emb);

    v = qtk_onnxruntime_getout(cmask_sv->emb, 0);
    if(!cmask_sv->output){
        int64_t *oshape, size;
        oshape = qtk_onnxruntime_get_outshape(cmask_sv->emb, 0, &size);
        cmask_sv->output_len = oshape[1];
        cmask_sv->output = wtk_malloc(sizeof(float) * cmask_sv->output_len);
        memcpy(cmask_sv->output,v,sizeof(float)*cmask_sv->output_len);
        wtk_free(oshape);
    }else{
        int i;
        for(i = 0; i < cmask_sv->output_len; i++){
            cmask_sv->output[i] += v[i];
        }
    }
#endif
}

void wtk_cmask_sv_feed(wtk_cmask_sv_t *cmask_pse, short *data, int len, int is_end)
{
    int i;
    int length;

    wtk_fbank_feed(cmask_pse->fbank, data, len, is_end);
    if(is_end){
        if(cmask_pse->fbank_frame==0){
            wtk_debug("error need feed data first\n");
            exit(0);
        }
        if(cmask_pse->cfg->use_cmvn){
            int pos=0;
            float *fbank;
            for(i=0; i<cmask_pse->fbank_len; ++i){
                cmask_pse->fbank_mean[i] /= cmask_pse->fbank_frame;
            }
            length = cmask_pse->fbank_buf->pos/sizeof(float);
            while(length>pos){
                fbank = (float *)(cmask_pse->fbank_buf->data+pos*sizeof(float));
                for(i=0;i<cmask_pse->fbank_len;++i){
                    fbank[i] -= cmask_pse->fbank_mean[i];
                }
                pos += cmask_pse->fbank_len;
            }
        }
        wtk_cmask_sv_feed_emb(cmask_pse);
    }
}

void wtk_cmask_sv_feed_cycle(wtk_cmask_sv_t *cmask_pse, short *data, int len, int is_end)
{
    int i,j;

    if(data){
        wtk_strbuf_push(cmask_pse->wav_buf, (char *)data, len * sizeof(short));
    }

    if(is_end){
        int num = (cmask_pse->wav_buf->pos/sizeof(short)) / 48000;
        int length = 48000;
        int fbank_length = 0;
        short *in_data = (short *)cmask_pse->wav_buf->data;
        if(num < 0){
            printf("error\n");
            return;
        }
        for(i = 0; i < num; i++){
            wtk_fbank_feed(cmask_pse->fbank, in_data, length, 1);
            if(cmask_pse->cfg->use_cmvn){
                int pos=0;
                float *fbank;
                for(j=0; j<cmask_pse->fbank_len; ++j){
                    cmask_pse->fbank_mean[j] /= cmask_pse->fbank_frame;
                }
                fbank_length = cmask_pse->fbank_buf->pos/sizeof(float);
                while(fbank_length>pos){
                    fbank = (float *)(cmask_pse->fbank_buf->data+pos*sizeof(float));
                    for(j=0;j<cmask_pse->fbank_len;++j){
                        fbank[j] -= cmask_pse->fbank_mean[j];
                    }
                    pos += cmask_pse->fbank_len;
                }
            }
            wtk_cmask_sv_feed_emb_cycle(cmask_pse);
            in_data += 48000;
            wtk_cmask_sv_reset2(cmask_pse);
        }
        for(i = 0; i < cmask_pse->output_len; i++){
            cmask_pse->output[i] /= num;
        }
        wtk_strbuf_reset(cmask_pse->wav_buf);
    }
}

static short _abs_max(short *p,int len){
    int i;
    short max = 0;
    for(i = 0; i < len; i++,p++){
        if(abs(*p) > max){
            max = abs(*p);
        }
    }
    return max;
}

void wtk_cmask_sv_feed2(wtk_cmask_sv_t *cmask_pse, short *data, int len, int is_end, int need_feed){
    if(len > 0){
        wtk_strbuf_push(cmask_pse->wav_buf, (char *)data, len * sizeof(short));
    }
    if(need_feed > 0){
        if(cmask_pse->normalize == 1){
            int w_len = cmask_pse->wav_buf->pos >> 1;
            int i;
            short* wav_data = (short*)cmask_pse->wav_buf->data;
            float d;
            short d_max = _abs_max(wav_data, w_len);
            if(d_max != 0){
                for(i = 0; i < w_len; i++){
                    d = (wav_data[i] / (float)d_max) * 32767.0f;
                    wav_data[i] = d;
                }
            }
        }
        wtk_cmask_sv_feed(cmask_pse, (short *)cmask_pse->wav_buf->data, cmask_pse->wav_buf->pos >> 1, 1);
        wtk_strbuf_pop(cmask_pse->wav_buf, NULL, need_feed >> 1);
    }
}

void wtk_cmask_sv_get_result(wtk_cmask_sv_t *cmask_pse, float **vec, int *len) {
    *vec = cmask_pse->output;
    *len = cmask_pse->output_len;
}

float wtk_cmask_sv_eval(wtk_cmask_sv_t *cmask_pse, float* v1, float *v2, int len){
	float res = 0.0;
	int i;
	float *p1 = v1;
	float *p2 = v2;
	float suma=0.0,sumb=0.0;

	for(i = 0;i < len; i++,p1++,p2++)
	{
		res += (*p1)*(*p2);
		suma += (*p1)*(*p1);
		sumb += (*p2)*(*p2);
	}
	res /= pow(suma,0.5)*pow(sumb,0.5);

	return res;
}