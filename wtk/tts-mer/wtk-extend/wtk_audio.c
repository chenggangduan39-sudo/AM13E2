#include "wtk_type.h"
#include "wtk_audio.h"
#include "wtk_file.h"

wtk_mer_wav_stream_t* wtk_mer_wav_stream_new(size_t max_size, int fs, float frame_period)
{/* frame_period 5ms */
    wtk_mer_wav_stream_t *wav;
    WaveHeader *hdr;
    char *p = malloc(sizeof(*wav)+max_size*sizeof(*wav->data));
    
    memset(p,0,sizeof(*wav)+max_size*sizeof(*wav->data));
    wav = (wtk_mer_wav_stream_t*)p;
    wav->data = (short*)(p + sizeof(*wav));
    wav->len = 0;
    wav->max_size = max_size;
    wav->fs = fs;
    wav->frame_period = frame_period;

    hdr=&(wav->hdr);
    wavehdr_init(hdr);
    wavehdr_set_fmt(hdr, 1, fs, sizeof(short));
    return wav;
}
void wtk_mer_wav_stream_write_float(wtk_mer_wav_stream_t *wav, float *data, int len)
{
    int i;
    short *wav_data=wav->data;
    float maxf= 32766.0 / max(0.01, wtk_float_abs_max(data, len));
    size_t wav_stlen=len;

    if (wav_stlen > wav->max_size)
    {
        wtk_exit_debug("音频大小超出限制 %lu >  %lu bytes\n", wav_stlen*sizeof(*wav_data), wav->max_size);
    }
    wav->len=wav_stlen;

    wavehdr_set_size(&wav->hdr, wav_stlen*sizeof(*wav_data));

    for (i=0; i<len; ++i)
    {
        wav_data[i]=data[i]*maxf;
    }
}

void wtk_mer_wav_stream_push_short(wtk_mer_wav_stream_t *wav, short *data, int len)
{
    short *wav_data=wav->data;
    size_t wav_stlen=wav->len;

    if (wav_stlen+len > wav->max_size)
    {
        wtk_exit_debug("音频大小超出限制 %lu >  %lu bytes\n", (wav_stlen+len)*sizeof(*wav_data), wav->max_size);
    }
    memcpy(wav_data+wav->len,data,len*sizeof(*wav_data));
    wav->len+=len;

    wavehdr_set_size(&wav->hdr, wav->len*2);
}

void wtk_mer_wav_stream_savefile(wtk_mer_wav_stream_t *wav, char *fn, double duration)
{/* 含音频头 */
    wtk_mer_wav_stream_savefile3(wav, 1, fn, duration, "wb");
}
void wtk_mer_wav_stream_savefile2(wtk_mer_wav_stream_t *wav, char *fn, int i)
{/* 去除音频头 */
    char *mode = i==0?"wb":"ab";
    // char *data = wav->data;
    wtk_mer_wav_stream_savefile3( wav, 0, fn, 0, mode);
    // wtk_mer_wav_stream_savefile3(wav->data+44, wav->len-44, wav->fs, fn, 0, mode);
}
void wtk_mer_wav_stream_savefile3( wtk_mer_wav_stream_t *wav, int has_header, char *fn, double duration, char *mode)
{
    char *data=(char*)(wav->data);
    int len=wav->len
      , fs=wav->hdr.fmt_sample_rate;

    if (fn != NULL)
    {
        FILE *fp = wtk_mer_getfp(fn, mode);
        if (has_header)
        { fwrite(&wav->hdr, sizeof(wav->hdr), 1, fp); }
        fwrite(data, 1, len*sizeof(*wav->data), fp);
        printf(" output: %s \n", fn);
        fclose(fp);
    }
    if (duration != 0)
    {
        printf( " single time: %lfs, rate: %lf \n", duration, duration/(len/(fs*sizeof(short)*1.0)));
    }
}
void wtk_mer_wav_stream_delete(wtk_mer_wav_stream_t *wav)
{
    free(wav);
}