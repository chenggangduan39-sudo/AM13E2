#include "wtk/bfio/drc/wtk_drc.h"

void snd_buffer_new(sf_snd_st *buf,int size,int rate){
    buf->size = size;
    buf->rate = rate;
    buf->samples = (sf_sample_st *)wtk_malloc(size * sizeof(sf_sample_st));
}

wtk_drc_t* wtk_drc_new(wtk_drc_cfg_t *cfg){
    wtk_drc_t *drc = (wtk_drc_t*)malloc(sizeof(wtk_drc_t));
    drc->cfg = cfg;

    sf_advancecomp(
        &drc->state,
        cfg->rate,
        cfg->pregain,
        cfg->threshold,
        cfg->knee,
        cfg->ratio,
        cfg->attack,
        cfg->release,
        cfg->predelay,
        cfg->releasezone1,
        cfg->releasezone2,
        cfg->releasezone3,
        cfg->releasezone4,
        cfg->postgain,
        cfg->wet
    );

    snd_buffer_new(&drc->input,cfg->size,cfg->rate);
    if(cfg->size%SF_COMPRESSOR_SPU != 0){
        wtk_debug("input size not multiple of SF_COMPRESSOR_SPU\n");
        exit(1);
    }
    snd_buffer_new(&drc->output,cfg->size,cfg->rate);

    drc->out = (short*)wtk_malloc(cfg->size * cfg->numchannels * sizeof(short));

    drc->ths = NULL;
    drc->notify = NULL;

    return drc;
}

void wtk_drc_delete(wtk_drc_t *drc){

    if(drc->input.samples){
        wtk_free(drc->input.samples);
    }
    if(drc->output.samples){
        wtk_free(drc->output.samples);
    }
    if(drc->out){
        wtk_free(drc->out);
    }
    wtk_free(drc);
}

void wtk_drc_start(wtk_drc_t *drc){

}

void wtk_drc_reset(wtk_drc_t *drc){

}

void wtk_drc_set_notify(wtk_drc_t *drc,void *ths,wtk_drc_notify_f notify){
    drc->ths = ths;
    drc->notify = notify;
}

/*
numchannels: 1 or 2, if numchannels==1 , then copy left channel to both left and right channels.
convert to stereo floating point is convenient for the compressor.
len: number of samples per channel.
*/
void wtk_drc_feed(wtk_drc_t *drc,short *data, int numchannels, int len,int is_end){
    
    if(len != drc->input.size){
        wtk_debug("input size mismatch, len:%d, drc->input.size:%d\n", len, drc->input.size);
        return;
    }

    sf_sample_st *input = drc->input.samples;
    sf_sample_st *output = drc->output.samples;

    int16_t L,R;
    for(int i=0; i<len; i++){
        L = data[i*numchannels];
        if(numchannels==1){
            R = L;
        }else if(numchannels==2){
            R = data[i*numchannels+1];
        }else{
            wtk_debug("numchannels must be 1 or 2\n");
            return;
        }

        if (L < 0)
            input[i].L = (float)L / 32768.0f;
        else
            input[i].L = (float)L / 32767.0f;
        if (R < 0)
            input[i].R = (float)R / 32768.0f;
        else
            input[i].R = (float)R / 32767.0f;
    }

    sf_compressor_process(&drc->state, len, input, output);

    for(int i=0; i<len; i++){
        if(numchannels==1){
            if(output[i].L>0){
                drc->out[i*numchannels] = (short)(output[i].L * 32767.0f);
            }else{
                drc->out[i*numchannels] = (short)(output[i].L * 32768.0f);
            }
        }else if(numchannels==2){
            if(output[i].L>0){
                drc->out[i*numchannels] = (short)(output[i].L * 32767.0f);
            }else{
                drc->out[i*numchannels] = (short)(output[i].L * 32768.0f);
            }

            if(output[i].R>0){
                drc->out[i*numchannels+1] = (short)(output[i].R * 32767.0f);
            }else{
                drc->out[i*numchannels+1] = (short)(output[i].R * 32768.0f);
            }
        }else{
            wtk_debug("numchannels must be 1 or 2\n");
        }
    }
    if(drc->notify){
        drc->notify(drc->ths,drc->out,len*numchannels);
    }
}