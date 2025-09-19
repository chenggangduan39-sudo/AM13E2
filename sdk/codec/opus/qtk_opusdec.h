#ifndef __QTK_UTIL_QTK_OPUSDEC_H__
#define __QTK_UTIL_QTK_OPUSDEC_H__
#include "opus/opus.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/wtk_wavfile.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    QTK_OPUSDEC_DATA_LEN,
    QTK_OPUSDEC_DATA_OR,
    QTK_OPUSDEC_DATA_DATA,
} qtk_opusdec_data_type_t;

typedef void (*qtk_opusdec_notify_f)(void *ths, char *data, int len);
typedef struct qtk_opusdec qtk_opusdec_t;
struct qtk_opusdec {
    int samplerate;
    int channels;
    OpusDecoder *dec;
    qtk_opusdec_notify_f notify;
    void *ths;
    wtk_strbuf_t *buf;
    qtk_opusdec_data_type_t data_type;
    uint32_t audio_len;
    uint32_t or_len;
    int max_frame_size;
    short *out;
    unsigned char *fbytes;
    opus_int32 output_samples;
    opus_int32 skip;
    FILE *f;
    wtk_wavfile_t *wav;
    int count;
    unsigned int debug : 1;
};
qtk_opusdec_t *qtk_opusdec_new(char *params);
int qtk_opusdec_start(qtk_opusdec_t *opusdec, void *ths,
                      qtk_opusdec_notify_f notify);
int qtk_opusdec_stop(qtk_opusdec_t *opusdec);
int qtk_opusdec_delete(qtk_opusdec_t *opusdec);
int qtk_opusdec_feed(qtk_opusdec_t *opusdec, char *data, int len, int is_end);
#ifdef __cplusplus
};
#endif
#endif