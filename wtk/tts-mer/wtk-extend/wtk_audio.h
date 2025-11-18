#ifndef WTK_MER_AUDIO_H
#define WTK_MER_AUDIO_H
#include "wtk/tts-mer/wtk-extend/wtk_tts_common.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    WaveHeader hdr;
    short *data;
    float frame_period; /* 帧率 通常是5毫秒 */
    int fs;
    int len; /* len( char *data) */
    size_t max_size; /* 音频大小上限 */
} wtk_mer_wav_stream_t;

#define wtk_mer_wav_stream_setlen(wav, f0_len) {wav->len = ((int)((f0_len - 1) * wav->frame_period/1000.0 * wav->fs) + 1)*sizeof(short) + 44;if(wav->len>wav->max_size){wtk_exit_debug("音频超出预设内存大小 wav->len: %d > wav->max_size: %lu \n", wav->len, wav->max_size );}};

wtk_mer_wav_stream_t* wtk_mer_wav_stream_new(size_t max_size, int fs, float frame_period);
void wtk_mer_wav_stream_write_float(wtk_mer_wav_stream_t *wav, float *data, int len);
void wtk_mer_wav_stream_push_short(wtk_mer_wav_stream_t *wav, short *data, int len);
void wtk_mer_wav_stream_savefile(wtk_mer_wav_stream_t *wav, char *fn, double duration);
void wtk_mer_wav_stream_savefile2(wtk_mer_wav_stream_t *wav, char *fn, int i);
void wtk_mer_wav_stream_savefile3( wtk_mer_wav_stream_t *wav, int has_header, char *fn, double duration, char *mode);
void wtk_mer_wav_stream_delete(wtk_mer_wav_stream_t *wav);

#ifdef __cplusplus
}
#endif
#endif