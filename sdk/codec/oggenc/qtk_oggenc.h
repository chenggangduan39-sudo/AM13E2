#ifndef QTK_API_SPEECHC_QTK_OGGENC
#define QTK_API_SPEECHC_QTK_OGGENC
#include "qtk_oggenc_cfg.h"
#include "wtk/core/wtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_oggenc qtk_oggenc_t;

typedef void (*qtk_oggenc_write_f)(void *ths, char *data, int len);

struct qtk_oggenc {
    qtk_oggenc_cfg_t *cfg;
    const SpeexMode *spx_mode;
    void *spx_state;
    SpeexBits spx_bits;
    SpeexHeader spx_header;
    spx_int32_t spx_frame_size;
    spx_int32_t spx_lookahead;

    spx_int32_t spx_quality;
    spx_int32_t spx_rate;
    spx_int32_t spx_channels;
    spx_int32_t spx_bits_per_sample;
    spx_int32_t spx_frames_per_packet;
    spx_int32_t spx_vbr;
    spx_int32_t spx_complexity;

    int spx_frame_id;
    int spx_frame_id_pageout; /* 4 pageout per second */

    char *spx_version;
    char spx_vendor[64];
    char *spx_comments;
    char spx_bits_buf[2048];

    spx_int16_t spx_frame_buf[1024];
    int spx_frame_buf_pos;

    ogg_stream_state og_stream_state;
    ogg_page og_page;
    ogg_packet og_packet;

    float og_pages_per_second;

    void *write_ths;
    qtk_oggenc_write_f write;
};

qtk_oggenc_t *qtk_oggenc_new(qtk_oggenc_cfg_t *cfg);
void qtk_oggenc_delete(qtk_oggenc_t *enc);
void qtk_oggenc_reset(qtk_oggenc_t *enc);
void qtk_oggenc_set_write(qtk_oggenc_t *enc, void *ths,
                          qtk_oggenc_write_f write);
int qtk_oggenc_start(qtk_oggenc_t *enc, int rate, int channels, int bits);
int qtk_oggenc_encode(qtk_oggenc_t *enc, const void *data, int size, int eof);
#ifdef __cplusplus
};
#endif
#endif
