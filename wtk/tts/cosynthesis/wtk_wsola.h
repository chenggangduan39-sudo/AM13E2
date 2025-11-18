#ifndef __WTK_WSOLA_H__
#define __WTK_WSOLA_H__
#include "wtk_wsola_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void(*wtk_wsola_notity_f)(void *ths,short*data,int len);

typedef struct 
{
    wtk_wsola_cfg_t *cfg;
    short *audios;
    uint32_t audio_len;
    void *ths;
    wtk_wsola_notity_f notify;
}wtk_wsola_t;


wtk_wsola_t *wtk_wsola_new(wtk_wsola_cfg_t *cfg);
int wtk_wsola_reset(wtk_wsola_t *wsola);
int wtk_wsola_delete(wtk_wsola_t *wsola);
int wtk_wsola_feed(wtk_wsola_t *wsola, short * audio, int len, int is_end);
int wtk_wsola_feed_flow(wtk_wsola_t *wsola, short * audio, int len, int is_end);
void wtk_wsola_set_notify(wtk_wsola_t *wsola, void *ths, wtk_wsola_notity_f notify);

#ifdef __cplusplus
};
#endif

#endif
