#ifndef WTK_BFIO_WTK_KVADWAKE
#define WTK_BFIO_WTK_KVADWAKE
#include "wtk_kvadwake_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	WTK_KVADWAKE_VAD_START,
	WTK_KVADWAKE_VAD_DATA,
	WTK_KVADWAKE_VAD_END,
	WTK_KVADWAKE_VAD_CANCEL,
    WTK_KVADWAKE_WAKE,
    WTK_KVADWAKE_WAKE2,
}wtk_kvadwake_cmd_t;

typedef void (*wtk_kvadwake_notify_f)(void *ths,wtk_kvadwake_cmd_t cmd,float fs,float fe,short *data, int len);
typedef void (*wtk_kvadwake_notify_f2)(void *ths,wtk_kvadwake_cmd_t cmd,float fs,float fe,short *data, int len);

typedef struct wtk_kvadwake wtk_kvadwake_t;
struct wtk_kvadwake
{
    wtk_kvadwake_cfg_t *cfg;

    wtk_vad2_t *vad2;
    wtk_kwake_t *kwake;
    wtk_wdec_t *wdec;
    wtk_kwdec2_t *kwdec2;
    qtk_img_rec_t *img;
    qtk_decoder_wrapper_t *decoder;
    long cnt;
    long start_wake_cnt;

    void *ths;
    wtk_kvadwake_notify_f notify;
    void *ths2;
    wtk_kvadwake_notify_f2 notify2;

    wtk_strbuf_t *asr_res;
    wtk_strbuf_t *wav_buf;
    int wav_pos;

    int idx;
    int vad_len;

    unsigned sil:1;
};

wtk_kvadwake_t *wtk_kvadwake_new(wtk_kvadwake_cfg_t *cfg);
void wtk_kvadwake_delete(wtk_kvadwake_t *vwake);
void wtk_kvadwake_reset(wtk_kvadwake_t *vwake);
void wtk_kvadwake_start(wtk_kvadwake_t *vwake);
void wtk_kvadwake_feed(wtk_kvadwake_t *vwake, short *data, int len,int is_end);
void wtk_kvadwake_set_notify(wtk_kvadwake_t *vwake, void *ths, wtk_kvadwake_notify_f notify);
void wtk_kvadwake_set_notify2(wtk_kvadwake_t *vwake, void *ths, wtk_kvadwake_notify_f2 notify);

float wtk_kvadwake_get_conf(wtk_kvadwake_t *vwake);
void wtk_kvadwake_set_idx(wtk_kvadwake_t *vwake, int idx);
void wtk_kvadwake_get_sp_sil(wtk_kvadwake_t *vwake, int sp_sil);

int wtk_kvadwake_set_words(wtk_kvadwake_t *vwake,char *words,int len);

#ifdef __cplusplus
};
#endif
#endif
