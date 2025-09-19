#ifndef WTK_BFIO_WTK_CHECKWAV
#define WTK_BFIO_WTK_CHECKWAV
#include "wtk/core/wtk_queue.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_wavfile.h"
#ifdef __cplusplus
extern "C" {
#endif


typedef enum{
    WTK_CHECKWAV_BROKE,
    WTK_CHECKWAV_BASS,
    WTK_CHECKWAV_ZERO,
    WTK_CHECKWAV_CHENERGY,
}wtk_checkwav_cmd_t;

typedef void (*wtk_checkwav_notify_f)(void *ths, wtk_checkwav_cmd_t cmd, float tm, int channel);
typedef struct wtk_checkwav wtk_checkwav_t;


struct wtk_checkwav
{
    wtk_strbuf_t **buf;
    int channel;
    int rate;
    int check_maxlen;
    float min_en;

    long input;

    void *ths;
    wtk_checkwav_notify_f notify;
};

//bass_maxtm (ms) zero_maxtm (ms) rate 16000
wtk_checkwav_t* wtk_checkwav_new(int channel, int check_maxtm, int rate, float min_en);

void wtk_checkwav_delete(wtk_checkwav_t *checkwav);

void wtk_checkwav_reset(wtk_checkwav_t *checkwav);

void wtk_checkwav_set_notify(wtk_checkwav_t *checkwav, void *ths, wtk_checkwav_notify_f notify);

void wtk_checkwav_feed(wtk_checkwav_t *checkwav,short **data,int len, int is_end);


#ifdef __cplusplus
};
#endif
#endif
