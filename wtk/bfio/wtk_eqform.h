#ifndef WTK_BFIO_WTK_EQFORM
#define WTK_BFIO_WTK_EQFORM
#include "wtk_eqform_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void(*wtk_eqform_notify_f)(void *ths,short *data,int len);
typedef void(*wtk_eqform_notify_enrcheck_f)(void *ths,float enr_thresh,float enr, int on_run);

typedef struct wtk_eqform wtk_eqform_t;

struct wtk_eqform
{
	wtk_eqform_cfg_t *cfg;

    int channel;

    wtk_aec_t *aec;
    wtk_qform9_t *qform9;
    wtk_qform11_t *qform11;
    wtk_qform3_t *qform3;
    wtk_strbuf_t *obuf;

    wtk_strbuf_t **buf;

    void *ths;
	wtk_eqform_notify_f notify;

    void *en_ths;
    wtk_eqform_notify_enrcheck_f en_notify;
};

wtk_eqform_t* wtk_eqform_new(wtk_eqform_cfg_t *cfg);

void wtk_eqform_delete(wtk_eqform_t *eqform);

void wtk_eqform_reset(wtk_eqform_t *eqform);

void wtk_eqform_set_notify(wtk_eqform_t *eqform,void *ths,wtk_eqform_notify_f notify);

void wtk_eqform_set_enrcheck_notify(wtk_eqform_t *eqform,void *ths,wtk_eqform_notify_enrcheck_f notify);

void wtk_eqform_start(wtk_eqform_t *eqform,float theta,float phi);

void wtk_eqform_feed(wtk_eqform_t *eqform,short **data,int len,int is_end);


#ifdef __cplusplus
};
#endif
#endif
