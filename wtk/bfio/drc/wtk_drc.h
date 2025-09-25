//dynamic range compressor

#ifndef WTK_BFIO_DRC
#define WTK_BFIO_DRC
#include "wtk_drc_cfg.h"
#include "wtk/bfio/drc/snd.h"
#include "wtk/bfio/drc/compressor.h"
#include "wtk/core/wtk_alloc.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_drc wtk_drc_t;
typedef void(*wtk_drc_notify_f)(void *ths,short *output,int len);


struct wtk_drc
{
	wtk_drc_cfg_t *cfg;

	void *ths;
	wtk_drc_notify_f notify;

    sf_compressor_state_st state;
    sf_snd_st input;
    sf_snd_st output;

    short *out;
};

wtk_drc_t* wtk_drc_new(wtk_drc_cfg_t *cfg);
void wtk_drc_delete(wtk_drc_t *drc);
void wtk_drc_start(wtk_drc_t *drc);
void wtk_drc_reset(wtk_drc_t *drc);
void wtk_drc_set_notify(wtk_drc_t *drc,void *ths,wtk_drc_notify_f notify);
void wtk_drc_feed(wtk_drc_t *drc,short *data, int numchannels, int len,int is_end);
#ifdef __cplusplus
};
#endif
#endif
